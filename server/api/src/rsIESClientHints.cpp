#include "rcMisc.h"
#include "rsGlobalExtern.hpp"
#include "rodsErrorTable.h"
#include "miscServerFunct.hpp"
#include "specificQuery.h"
#include "miscServerFunct.hpp"
#include "rsIESClientHints.hpp"
#include "rsSpecificQuery.hpp"

#include "irods_log.hpp"
#include "zone_report.h"
#include "ies_client_hints.h"
#include "irods_resource_manager.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_properties.hpp"

#include "jansson.h"



int _rsIESClientHints(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf );

int rsIESClientHints(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {
    rodsServerHost_t* rods_host;
    int status = getAndConnRcatHost(
                     _comm,
                     MASTER_RCAT,
                     ( const char* )nullptr,
                     &rods_host );
    if ( status < 0 ) {
        return status;
    }

    if ( rods_host->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsIESClientHints(
                         _comm,
                         _bbuf );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        status = rcIESClientHints( rods_host->conn,
                                   _bbuf );
    }

    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "rsIESClientHints: rcIESClientHints failed, status = %d",
            status );
    }

    return status;

} // rsIESClientHints

irods::error get_strict_acls(
    rsComm_t*    _comm,
    std::string& _acls ) {
    if ( ! _comm ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "comm is null" );
    }

    try {
        _acls = irods::get_server_property<const std::string>(irods::STRICT_ACL_KW);
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }

    return SUCCESS();

} // get_strict_acls

irods::error get_query_array(
    rsComm_t* _comm,
    json_t*&  _queries ) {
    if ( !_comm ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "comm is null" );
    }

    _queries = json_array();
    if ( !_queries ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "allocation of json object failed" );
    }

    specificQueryInp_t spec_inp;
    memset( &spec_inp, 0, sizeof( specificQueryInp_t ) );

    spec_inp.maxRows = MAX_SQL_ROWS;
    spec_inp.continueInx = 0;
    spec_inp.sql = "ls";

    genQueryOut_t* gen_out = nullptr;
    int status = rsSpecificQuery(
                     _comm,
                     &spec_inp,
                     &gen_out );
    if ( status < 0 ) {
        return ERROR(
                   status,
                   "rsSpecificQuery for 'ls' failed" );
    }

    // first attribute is the alias of the specific query
    int   len    = gen_out->sqlResult[ 0 ].len;
    char* values = gen_out->sqlResult[ 0 ].value;

    for ( int i = 0 ;
            i < gen_out->rowCnt ;
            ++i ) {

        char* alias = &values[ len * i ];
        json_array_append_new( _queries, json_string( alias ) );

    } // for i

    freeGenQueryOut( &gen_out );

    return SUCCESS();

} // get_query_array



int _rsIESClientHints(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {
    if ( ! _comm ) {
        return SYS_INVALID_INPUT_PARAM;

    }

    json_t* ies_hints = json_object();
    if ( !ies_hints ) {
        rodsLog(
            LOG_ERROR,
            "failed to allocate json_object" );
        return SYS_MALLOC_ERR;
    }

    std::string acls;
    irods::error ret = get_strict_acls(
                           _comm,
                           acls );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    json_object_set_new(ies_hints, "strict_acls", json_string( acls.c_str() ) );

    json_t* query_arr = nullptr;
    ret = get_query_array( _comm, query_arr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new(ies_hints, "specific_queries", query_arr);
    char* tmp_buf = json_dumps( ies_hints, JSON_INDENT( 4 ) );
    json_decref( ies_hints );

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( bytesBuf_t ) );
    if ( !( *_bbuf ) ) {
        rodsLog(
            LOG_ERROR,
            "_rsIESClientHints: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;

    }

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = strlen( tmp_buf );

    return 0;

} // _rsIESClientHints
