/**
 * @file  libmsisync_to_archive.cpp
 *
 */


// =-=-=-=-=-=-=-
#include "apiHeaderAll.h"
#include "msParam.h"
#include "irods_ms_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"

// =-=-=-=-=-=-=-
#include <string>
#include <iostream>
#include <vector>


irods::error find_compound_resource_in_hierarchy(
    const std::string&   _hier,
    irods::resource_ptr& _resc ) {

    irods::hierarchy_parser p;
    irods::error ret = p.set_string( _hier );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    std::string last;
    ret = p.last_resc( last );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    bool found = false;
    std::string prev;
    irods::hierarchy_parser::const_iterator itr;
    for( itr = p.begin(); itr != p.end(); ++itr ) {
        if( *itr == last ) {
            found = true;
            break;
        }
        prev = *itr;
    }

    if( !found ) {
        std::string msg = "Previous child not found for [";
        msg += _hier;
        msg += "]";
        return ERROR(
            CHILD_NOT_FOUND,
            msg );
    }

    ret = resc_mgr.resolve( prev, _resc );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    return SUCCESS();
}

/**
 * \fn msisync_to_archive (msParam_t* _resource_hierarchy, msParam_t* _physical_path, msParam_t* _logical_path, ruleExecInfo_t *rei)
 *
 * \brief   This microservice syncs a dataObject to archive from compound's cache
 *
 * \module microservice
 *
 * \since 4.1.8
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] _resource_hierarchy - The semicolon delimited string designating the source replica's location
 * \param[in] _physical_path - The physical path of the to-be-created replica
 * \param[in] _logical_path - The logical path of the dataObject to be replicated
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0
 * \pre none
 * \post none
 * \sa none
 **/
int msisync_to_archive(
    msParam_t*      _resource_hierarchy,
    msParam_t*      _physical_path,
    msParam_t*      _logical_path,
    ruleExecInfo_t* _rei ) {
    using std::cout;
    using std::endl;
    using std::string;
    char *resource_hierarchy = parseMspForStr( _resource_hierarchy );
    if( !resource_hierarchy ) {
        cout << "msisync_to_archive - null _resc_hier parameter" << endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    char *physical_path = parseMspForStr( _physical_path );
    if( !physical_path ) {
        cout << "msisync_to_archive - null _physical_path parameter" << endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    char *logical_path = parseMspForStr( _logical_path );
    if( !logical_path ) {
        cout << "msisync_to_archive - null _logical_path parameter" << endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    if( !_rei ) {
        cout << "msisync_to_archive - null _rei parameter" << endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    irods::file_object_ptr file_obj(
        new irods::file_object(
            _rei->rsComm,
            logical_path,
            physical_path,
            resource_hierarchy,
            0,     // fd
            0,     // mode
            0 ) ); // flags

    // get root resc to
    irods::resource_ptr resc;
    irods::error ret = find_compound_resource_in_hierarchy(
        resource_hierarchy,
        resc );
    if( !ret.ok() ) {
        return ret.code();
    }

    std::string auto_repl( "off" );
    ret = resc->get_property<std::string>(
        "auto_repl",
        auto_repl );
    if( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    bool reset_to_off = false;
    if( "off" == auto_repl ) {
        reset_to_off = true;
        resc->set_property<std::string>(
            "auto_repl",
            "on" );
    }

    // inform the resource that a write operation happened
    // to put it in a state where it will need to replicate
    ret = fileNotify(
        _rei->rsComm,
        file_obj,
        irods::WRITE_OPERATION );
    if( !ret.ok() ) {
        cout << "msisync_to_archive - fileNotify failed ["
             << ret.result().c_str() << "] - ["
             << ret.code() << "]" << endl;
        if( reset_to_off ) {
            resc->set_property<std::string>(
                "auto_repl",
                "off" );
        }
        return ret.code();
    }

    const keyValPair_t& kvp = file_obj->cond_input();
    addKeyVal(
        (keyValPair_t*)&kvp,
        ADMIN_KW,
        "true" );
    int auth_flg = _rei->rsComm->clientUser.authInfo.authFlag;
    _rei->rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    // inform the resource that a modification is complete
    // which will trigger the replication
    ret = fileModified(
        _rei->rsComm,
        file_obj );

    // reset state before anything else happens
    _rei->rsComm->clientUser.authInfo.authFlag = auth_flg;
    if( reset_to_off ) {
        resc->set_property<std::string>(
            "auto_repl",
            "off" );
    }

    if( !ret.ok() ) {
        cout << "msisync_to_archive - fileModified failed ["
             << ret.result().c_str() << "] - ["
             << ret.code() << "]" << endl;
        return ret.code();
    }

    return 0;

}

extern "C"
irods::ms_table_entry* plugin_factory() {
    irods::ms_table_entry* msvc = new irods::ms_table_entry(3);
    msvc->add_operation("msisync_to_archive",
                         std::function<int(
                             msParam_t*,
                             msParam_t*,
                             msParam_t*,
                             ruleExecInfo_t*)>(msisync_to_archive));
    return msvc;
}
