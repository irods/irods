// =-=-=-=-=-=-=
// irods includes
#include "miscServerFunct.hpp"
#include "objInfo.h"
#include "dataObjCreate.h"
#include "specColl.hpp"
#include "collection.hpp"
#include "dataObjOpr.hpp"
#include "getRescQuota.h"
#include "rsDataObjCreate.hpp"
#include "rsGetRescQuota.hpp"

// =-=-=-=-=-=-=
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "voting.hpp"

namespace {

    std::string get_keyword_from_inp(
        const dataObjInp_t& _data_obj_inp)
    {
        std::string key_word{};
        char* resc_name = getValByKey(&_data_obj_inp.condInput, RESC_NAME_KW);
        if (resc_name) {
            key_word = resc_name;
        }
        char* dest_resc_name = getValByKey(&_data_obj_inp.condInput, DEST_RESC_NAME_KW);
        if (dest_resc_name) {
            key_word = dest_resc_name;
        }
        char* backup_resc_name = getValByKey(&_data_obj_inp.condInput, BACKUP_RESC_NAME_KW);
        if (backup_resc_name) {
            key_word = backup_resc_name;
        }
        if (!key_word.empty()) {
            irods::resource_ptr resc;
            irods::error ret = resc_mgr.resolve( key_word, resc );
            if ( !ret.ok() ) {
                THROW(ret.code(), ret.result());
            }
            irods::resource_ptr parent;
            ret = resc->get_parent(parent);
            if (ret.ok()) {
                THROW(DIRECT_CHILD_ACCESS, "key_word contains child resource");
            }
        }
        return key_word;
    } // get_keyword_from_inp

    bool hier_has_replica(
        const std::string& _resc,
        const irods::file_object_ptr _file_obj)
    {
        for (const auto& r : _file_obj->replicas()) {
            if (irods::hierarchy_parser{r.resc_hier()}.resc_in_hier(_resc)) {
                return true;
            }
        }
        return false;
    } // hier_has_replica

    int apply_policy_for_create_operation(
        rsComm_t*     _comm,
        dataObjInp_t& _obj_inp,
        std::string&  _resc_name )
    {
        /* query rcat for resource info and sort it */
        ruleExecInfo_t rei{};
        initReiWithDataObjInp( &rei, _comm, &_obj_inp );
        int status = 0;
        if ( _obj_inp.oprType == REPLICATE_OPR ) {
            status = applyRule( "acSetRescSchemeForRepl", NULL, &rei, NO_SAVE_REI );
        }
        else {
            status = applyRule( "acSetRescSchemeForCreate", NULL, &rei, NO_SAVE_REI );
        }
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        if ( status < 0 ) {
            if ( rei.status < 0 ) {
                status = rei.status;
            }

            rodsLog( LOG_NOTICE,
                    "%s:acSetRescSchemeForCreate error for %s,status=%d",
                    __FUNCTION__, _obj_inp.objPath, status );

            return status;
        }

        // get resource name
        if ( !strlen( rei.rescName ) ) {
            irods::error set_err = irods::set_default_resource(
                    _comm,
                    "", "",
                    &_obj_inp.condInput,
                    _resc_name );
            if ( !set_err.ok() ) {
                irods::log( PASS( set_err ) );
                return SYS_INVALID_RESC_INPUT;
            }
        }
        else {
            _resc_name = rei.rescName;
        }

        status = setRescQuota(
                _comm,
                _obj_inp.objPath,
                _resc_name.c_str(),
                _obj_inp.dataSize );
        if( status == SYS_RESC_QUOTA_EXCEEDED ) {
            return SYS_RESC_QUOTA_EXCEEDED;
        }
        return 0;
    } // apply_policy_for_create_operation

    // function to handle collecting a vote from a resource for a given operation and fco
    irods::error request_vote_for_file_object(
        rsComm_t*                _comm,
        const std::string&       _oper,
        const std::string&       _resc_name,
        irods::file_object_ptr   _file_obj,
        std::string&             _out_hier,
        float&                   _out_vote )
    {
        namespace irv = irods::experimental::resource::voting;

        // request the resource by name
        irods::resource_ptr resc;
        irods::error err = resc_mgr.resolve( _resc_name, resc );
        if ( !err.ok() ) {
            return PASSMSG( "failed in resc_mgr.resolve", err );
        }

        // if the resource has a parent, bail as this is a grave, terrible error.
        irods::resource_ptr parent;
        irods::error p_err = resc->get_parent( parent );
        if ( p_err.ok() ) {
            return ERROR(DIRECT_CHILD_ACCESS,
                       "attempt to directly address a child resource" );
        }

        // get current hostname, which is also done by init local server host
        char host_name_str[MAX_NAME_LEN]{};
        if ( gethostname( host_name_str, MAX_NAME_LEN ) < 0 ) {
            return ERROR( SYS_GET_HOSTNAME_ERR, "failed in gethostname" );
        }

        // query the resc given the operation for a hier string which
        // will determine the host
        irods::hierarchy_parser parser;
        float vote{};
        std::string host_name{host_name_str};
        irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast<irods::first_class_object>(_file_obj);
        err = resc->call< const std::string*, const std::string*, irods::hierarchy_parser*, float* >(
                  _comm, irods::RESOURCE_OP_RESOLVE_RESC_HIER, ptr, &_oper, &host_name, &parser, &vote );
        rodsLog(LOG_DEBUG,
            "[%s:%d] - resolved hier for obj [%s] with vote:[%f],hier:[%s],err.code:[%d]",
            __FUNCTION__,
            __LINE__,
            _file_obj->logical_path().c_str(),
            vote,
            parser.str().c_str(),
            err.code());
        if ( !err.ok() || irv::vote::zero == vote ) {
            std::stringstream msg;
            msg << "failed in call to redirect";
            msg << " host [" << host_name      << "] ";
            msg << " hier [" << _out_hier << "]";
            msg << " vote [" << vote << "]";
            err.status( false );
            if ( err.code() == 0 ) {
                err.code( HIERARCHY_ERROR );
            }
            return PASSMSG( msg.str(), err );
        }

        // extract the hier string from the parser, politely.
        _out_hier = parser.str();
        _out_vote = vote;
        return SUCCESS();
    } // request_vote_for_file_object

    std::string resolve_hier_for_open_or_write(
        rsComm_t*              _comm,
        irods::file_object_ptr _file_obj,
        const std::string&     _key_word,
        const std::string&     _oper)
    {
        namespace irv = irods::experimental::resource::voting;

        bool kw_match_found{};
        std::string max_hier{};
        float max_vote = -1.0;
        std::map<std::string, float> root_map;
        for (const auto& repl : _file_obj->replicas()) {
            const std::string root_resc = irods::hierarchy_parser{repl.resc_hier()}.first_resc();
            root_map[root_resc] = irv::vote::zero;
        }

        if (root_map.empty()) {
            THROW(SYS_REPLICA_DOES_NOT_EXIST, "file object has no replicas");
        }

        for (const auto& root_resc : root_map) {
            float vote{};
            std::string voted_hier{};
            rodsLog(LOG_DEBUG,
                "[%s:%d] - requesting vote from root [%s] for [%s]",
                __FUNCTION__,
                __LINE__,
                root_resc.first.c_str(),
                _file_obj->logical_path().c_str());
            irods::error ret = request_vote_for_file_object(
                    _comm, _oper, root_resc.first, _file_obj, voted_hier, vote );
            rodsLog(LOG_DEBUG,
                "[%s:%d] - root:[%s],max_hier:[%s],max_vote:[%f],vote:[%f],hier:[%s]",
                __FUNCTION__,
                __LINE__,
                root_resc.first.c_str(),
                max_hier.c_str(),
                max_vote,
                vote,
                voted_hier.c_str());
            if (ret.ok() && vote > max_vote) {
                max_vote = vote;
                max_hier = voted_hier;
            }

            if (ret.ok() && irv::vote::zero != vote && !kw_match_found && !_key_word.empty() && root_resc.first == _key_word) {
                rodsLog(LOG_DEBUG,
                    "[%s:%d] - with keyword... kw:[%s],root:[%s],max_hier:[%s],max_vote:[%f],vote:[%f],hier:[%s]",
                    __FUNCTION__,
                    __LINE__,
                    _key_word.c_str(),
                    root_resc.first.c_str(),
                    max_hier.c_str(),
                    max_vote,
                    vote,
                    voted_hier.c_str());
                kw_match_found = true;
                _file_obj->resc_hier(voted_hier);
            }
        }

        const double diff = max_vote - 0.00000001;
        if (diff <= irv::vote::zero) {
            THROW(HIERARCHY_ERROR, "no valid resource found for data object");
        }
        if (kw_match_found) {
            return _file_obj->resc_hier();
        }
        _file_obj->resc_hier(max_hier);
        return max_hier;
    } // resolve_hier_for_open_or_write

    // function to handle resolving the hier given the fco and resource keyword
    std::string resolve_hier_for_create(
        rsComm_t*              _comm,
        irods::file_object_ptr _file_obj,
        const std::string&     _key_word,
        dataObjInp_t&          _data_obj_inp)
    {
        namespace irv = irods::experimental::resource::voting;

        _file_obj->resc_hier(_key_word);

        // =-=-=-=-=-=-=-
        // get a vote and hier for the create
        float vote{};
        std::string hier{};
        irods::error ret = request_vote_for_file_object(
                        _comm,
                        irods::CREATE_OPERATION,
                        _key_word,
                        _file_obj,
                        hier,
                        vote );
        if ( irv::vote::zero == vote ) {
            if (0 == ret.code()) {
                ret.code( HIERARCHY_ERROR );
            }
            irods::log(ret);
            THROW(ret.code(), ret.result());
        }

        return hier;
    } // resolve_hier_for_create

    // determine if a forced write is to be done to a destination resource which
    // does not have an existing replica of the data object
    void determine_force_write_to_new_resource(
        const std::string& _oper,
        const irods::file_object_ptr _fobj,
        const dataObjInp_t& _data_obj_inp)
    {
        char* dst_resc_kw   = getValByKey( &_data_obj_inp.condInput, DEST_RESC_NAME_KW );
        char* force_flag_kw = getValByKey( &_data_obj_inp.condInput, FORCE_FLAG_KW );
        if ( PUT_OPR != _data_obj_inp.oprType ||
                _fobj->replicas().empty()  ||
                !dst_resc_kw   ||
                !force_flag_kw ||
                strlen( dst_resc_kw ) == 0 ||
                !( irods::OPEN_OPERATION  == _oper ||
                   irods::WRITE_OPERATION == _oper ||
                   irods::CREATE_OPERATION == _oper ) ) {
            return;
        }

        bool hier_match_flg = false;
        for (const auto& repl : _fobj->replicas()) {
            const std::string root_resc = irods::hierarchy_parser{repl.resc_hier()}.first_resc();
            if (root_resc == dst_resc_kw) {
                hier_match_flg = true;
                break;
            }
        }
        if (!hier_match_flg) {
            THROW(HIERARCHY_ERROR,
                  (boost::format("cannot force put [%s] to a different resource [%s]") %
                   _data_obj_inp.objPath % dst_resc_kw).str());
        }
        return;
    } // determine_force_write_to_new_resource
}

namespace irods {
    irods::resolve_hierarchy_result_type resolve_resource_hierarchy(
        const std::string&   _oper,
        rsComm_t*            _comm,
        dataObjInp_t&        _data_obj_inp,
        dataObjInfo_t**      _data_obj_info ) {
        if (!_comm) {
            THROW(SYS_INVALID_INPUT_PARAM, "null comm pointer");
        }

        // =-=-=-=-=-=-=-
        // cache the operation, as we may need to modify it
        std::string oper = _oper;

        // =-=-=-=-=-=-=-
        // if this is a special collection then we need to get the hier
        // pass that along and bail as it is not a data object, or if
        // it is just a not-so-special collection then we continue with
        // processing the operation, as this may be a create op
        rodsObjStat_t *rodsObjStatOut = NULL;
        if (collStat(_comm, &_data_obj_inp, &rodsObjStatOut) >= 0 && rodsObjStatOut->specColl) {
            std::string hier = rodsObjStatOut->specColl->rescHier;
            freeRodsObjStat( rodsObjStatOut );
            return {{}, hier};
        }
        freeRodsObjStat(rodsObjStatOut);

        // call factory for given dataObjInp, get a file_object
        file_object_ptr file_obj(new file_object());
        file_obj->logical_path(_data_obj_inp.objPath);
        error fac_err = file_object_factory( _comm, &_data_obj_inp, file_obj, _data_obj_info);

        determine_force_write_to_new_resource(oper, file_obj, _data_obj_inp);

        // Providing a replica number means the client is attempting to target an existing replica.
        // Therefore, usage of the replica number cannot be used during a create operation. The
        // operation must be changed so that the system does not attempt to create the replica.
        if (irods::CREATE_OPERATION == oper && getValByKey(&_data_obj_inp.condInput, REPL_NUM_KW)) {
            oper = irods::WRITE_OPERATION;
        }

        auto key_word = get_keyword_from_inp(_data_obj_inp);

        char* default_resc_name  = getValByKey(&_data_obj_inp.condInput, DEF_RESC_NAME_KW);
        if (irods::CREATE_OPERATION == oper) {
            std::string create_resc_name{};
            if (!key_word.empty()) {
                create_resc_name = key_word;
            }
            else if (default_resc_name) {
                create_resc_name = default_resc_name;
            }
            int status = apply_policy_for_create_operation(
                    _comm,
                    _data_obj_inp,
                    create_resc_name );
            if( status < 0 ) {
                THROW(status, "apply_policy_for_create_operation failed");
            }

            // If the replica exists on the target resource, use open/write
            if (fac_err.ok() && hier_has_replica(create_resc_name, file_obj)) {
                oper = irods::WRITE_OPERATION;
            }
            else {
                const auto hier = resolve_hier_for_create(
                        _comm,
                        file_obj,
                        create_resc_name,
                        _data_obj_inp);
                return {file_obj, hier};
            }
        }

        // =-=-=-=-=-=-=-
        // perform an open operation if create is not specified ( thats all we have for now )
        if (irods::OPEN_OPERATION  == oper || irods::WRITE_OPERATION == oper || irods::UNLINK_OPERATION == oper ) {
            if (!fac_err.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " :: failed in file_object_factory";
                irods::log(LOG_ERROR, msg.str());
                THROW(fac_err.code(), msg.str());
            }

            // consider force flag - we need to consider the default
            // resc if -f is specified
            if (getValByKey(&_data_obj_inp.condInput, FORCE_FLAG_KW) &&
                default_resc_name && key_word.empty()) {
                key_word = default_resc_name;
            }

            // attempt to resolve for an open
            const auto hier = resolve_hier_for_open_or_write(_comm, file_obj, key_word, oper);
            return {file_obj, hier};
        }
        // should not get here
        THROW(SYS_NOT_SUPPORTED, (boost::format("operation not supported [%s]") % oper).str());
    }

// =-=-=-=-=-=-=-
// @brief function to query resource for chosen server to which to redirect
// for a given operation
    error resource_redirect( const std::string&   _oper,
                             rsComm_t*            _comm,
                             dataObjInp_t*        _data_obj_inp,
                             std::string&         _out_hier,
                             rodsServerHost_t*&   _out_host,
                             int&                 _out_flag,
                             dataObjInfo_t**      _data_obj_info ) {
        // =-=-=-=-=-=-=-
        // default to local host if there is a failure
        _out_flag = LOCAL_HOST;

        // =-=-=-=-=-=-=-
        // resolve the resource hierarchy for this given operation and dataObjInp
        std::string resc_hier{};
        try {
            auto result = resolve_resource_hierarchy( _oper, _comm, *_data_obj_inp, _data_obj_info);
            resc_hier = std::get<std::string>(result);
        }
        catch (const irods::exception& e) {
            std::stringstream msg;
            msg << "resource_redirect - failed to resolve resource hierarchy for [";
            msg << _data_obj_inp->objPath;
            msg << "]";
            irods::log(LOG_ERROR, msg.str());
            return irods::error{e};
        }

        // =-=-=-=-=-=-=-
        // we may have an empty hier due to special collections and other
        // unfortunate cases which we cannot control, check the hier string
        // and if it is empty return success ( for now )
        if ( resc_hier.empty() ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // parse out the leaf resource id for redirection
        rodsLong_t resc_id = 0;
        irods::error ret = resc_mgr.hier_to_leaf_id(resc_hier,resc_id);
        if( !ret.ok() ) {
            return PASS(ret);
        }

        // =-=-=-=-=-=-=-
        // get the host property from the last resc and get the
        // host name from that host
        rodsServerHost_t* last_resc_host = NULL;
        error err = get_resource_property< rodsServerHost_t* >(
                        resc_id,
                        RESOURCE_HOST,
                        last_resc_host );
        if ( !err.ok() || NULL == last_resc_host ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in get_resource_property call ";
            msg << "for [" << resc_hier << "]";
            return PASSMSG( msg.str(), err );
        }


        // =-=-=-=-=-=-=-
        // get current hostname, which is also done by init local server host
        char host_name_char[ MAX_NAME_LEN ];
        if ( gethostname( host_name_char, MAX_NAME_LEN ) < 0 ) {
            return ERROR( SYS_GET_HOSTNAME_ERR, "failed in gethostname" );

        }

        std::string host_name( host_name_char );

        // =-=-=-=-=-=-=
        // iterate over the list of hostName_t* and see if any match our
        // host name.  if we do, then were local
        bool        match_flg = false;
        hostName_t* tmp_host  = last_resc_host->hostName;
        while ( tmp_host ) {
            std::string name( tmp_host->name );
            if ( name.find( host_name ) != std::string::npos ) {
                match_flg = true;
                break;

            }

            tmp_host = tmp_host->next;

        } // while tmp_host

        // =-=-=-=-=-=-=-
        // are we really, really local?
        if ( match_flg ) {
            _out_hier = resc_hier;
            _out_flag = LOCAL_HOST;
            _out_host = 0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // it was not a local resource so then do a svr to svr connection
        int conn_err = svrToSvrConnect( _comm, last_resc_host );
        if ( conn_err < 0 ) {
            return ERROR( conn_err, "failed in svrToSvrConnect" );
        }


        // =-=-=-=-=-=-=-
        // return with a hier string and new connection as remote host
        _out_hier = resc_hier;
        _out_host = last_resc_host;
        _out_flag = REMOTE_HOST;

        return SUCCESS();

    } // resource_redirect

} // namespace irods
