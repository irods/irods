#include "irods/miscServerFunct.hpp"
#include "irods/objInfo.h"
#include "irods/dataObjCreate.h"
#include "irods/specColl.hpp"
#include "irods/collection.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/getRescQuota.h"
#include "irods/rsDataObjCreate.hpp"
#include "irods/rsGetRescQuota.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/voting.hpp"

#include <fmt/format.h>

namespace
{
    namespace irv = irods::experimental::resource::voting;

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

    auto apply_policy_for_create_operation(
        rsComm_t*     _comm,
        dataObjInp_t& _obj_inp,
        std::string&  _resc_name) -> void
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

            THROW(status, fmt::format(
                "[{}]:acSetRescSchemeForCreate error for {},status={}",
                __FUNCTION__, _obj_inp.objPath, status));
        }

        // get resource name
        if ( !strlen( rei.rescName ) ) {
            irods::error set_err = irods::set_default_resource(_comm, "", "", &_obj_inp.condInput, _resc_name);
            if ( !set_err.ok() ) {
                THROW(SYS_INVALID_RESC_INPUT, set_err.result());
            }
        }
        else {
            _resc_name = rei.rescName;
        }

        status = setRescQuota(_comm, _obj_inp.objPath, _resc_name.c_str(), _obj_inp.dataSize);
        if( status == SYS_RESC_QUOTA_EXCEEDED ) {
            THROW(SYS_RESC_QUOTA_EXCEEDED, "resource quota exceeded");
        }
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

        irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - resolved hier for obj [{}] with vote:[{}],hier:[{}],err.code:[{}]",
            __FUNCTION__, __LINE__,
            _file_obj->logical_path().c_str(),
            vote, parser.str().c_str(),
            err.code()));

        if (!err.ok() || irv::vote_is_zero(vote)) {
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
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - requesting vote from root [{}] for [{}]; resc_hier set:[{}]",
                __FUNCTION__, __LINE__,
                root_resc.first.c_str(),
                _file_obj->logical_path().c_str(),
                _file_obj->resc_hier()));

            irods::error ret = request_vote_for_file_object(
                    _comm, _oper, root_resc.first, _file_obj, voted_hier, vote );

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - root:[{}],max_hier:[{}],max_vote:[{}],vote:[{}],hier:[{}],resc_hier:[{}]",
                __FUNCTION__, __LINE__,
                root_resc.first.c_str(),
                max_hier.c_str(),
                max_vote, vote,
                voted_hier.c_str(),
                _file_obj->resc_hier()));

            if (!ret.ok()) {
                continue;
            }

            if (kw_match_found) {
                continue;
            }

            if (!irv::vote_is_zero(vote) && !_key_word.empty() && root_resc.first == _key_word) {
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - with keyword... kw:[{}],root:[{}],max_hier:[{}],max_vote:[{}],vote:[{}],hier:[{}],resc_hier:[{}]",
                    __FUNCTION__, __LINE__,
                    _key_word.c_str(),
                    root_resc.first.c_str(),
                    max_hier.c_str(), max_vote,
                    vote, voted_hier.c_str(),
                    _file_obj->resc_hier()));

                kw_match_found = true;
                _file_obj->resc_hier(voted_hier);
            }

            if (vote > max_vote) {
                max_vote = vote;
                max_hier = voted_hier;
            }
        }

        if (irv::vote_is_zero(max_vote)) {
            THROW(HIERARCHY_ERROR, "no valid resource found for data object");
        }

        // If a replica was requested, voting for other replicas is not considered. This is because the user specified
        // a specific replica. If it voted 0, this is an error. If it did not vote 0, the user should get that replica.
        // If the administrator wants to make sure that the highest vote always wins, then their policy can/should be
        // configured to accommodate that situation.
        if (_file_obj->repl_requested() > -1) {
            const auto& target_replica = _file_obj->get_replica(_file_obj->repl_requested());
            if (!target_replica) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_REPLICA_DOES_NOT_EXIST,
                      fmt::format("Requested replica [{}] of [{}] does not exist.",
                                  _file_obj->repl_requested(),
                                  _file_obj->logical_path()));
            }

            if (irv::vote_is_zero(target_replica->get().vote())) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_REPLICA_INACCESSIBLE,
                      fmt::format("Requested replica [{}] of [{}] voted 0.0.",
                                  _file_obj->repl_requested(),
                                  _file_obj->logical_path()));
            }

            // Overwrite the winning hierarchy with the hierarchy of the requested replica and return it as the winner.
            _file_obj->resc_hier(target_replica->get().resc_hier());
            return _file_obj->resc_hier();
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
        const std::string&     _key_word)
    {
        _file_obj->resc_hier(_key_word);

        // =-=-=-=-=-=-=-
        // get a vote and hier for the create
        float vote{};
        std::string hier{};
        irods::error ret = request_vote_for_file_object(_comm, irods::CREATE_OPERATION, _key_word, _file_obj, hier, vote);
        if ( irv::vote::zero == vote ) {
            if (0 == ret.code()) {
                ret.code( HIERARCHY_ERROR );
            }
            irods::log(ret);
            THROW(ret.code(), ret.result());
        }

        return hier;
    } // resolve_hier_for_create
} // anonymous namespace

namespace irods
{
    const std::string CREATE_OPERATION( "CREATE" );
    const std::string WRITE_OPERATION( "WRITE" );
    const std::string OPEN_OPERATION( "OPEN" );
    const std::string UNLINK_OPERATION( "UNLINK" );

    irods::resolve_hierarchy_result_type resolve_resource_hierarchy(
        const std::string&   oper,
        rsComm_t*            comm,
        dataObjInp_t&        data_obj_inp,
        dataObjInfo_t**      data_obj_info)
    {
        // call factory for given dataObjInp, get a file_object
        file_object_ptr file_obj(new file_object());
        file_obj->logical_path(data_obj_inp.objPath);
        error fac_err = file_object_factory(comm, &data_obj_inp, file_obj, data_obj_info);
        auto fobj_tuple = std::make_tuple(file_obj, fac_err);
        auto result = resolve_resource_hierarchy(comm, oper, data_obj_inp, fobj_tuple);

        if (data_obj_info && *data_obj_info) {
            freeAllDataObjInfo(*data_obj_info);
        }

        // Resolving the resource hierarchy can result in the replica information previously
        // captured in "data_obj_info" to be out-of-date. Refetch the replica information so
        // that subsequent API calls have the latest information.
        file_obj.reset(new file_object());
        file_obj->logical_path(data_obj_inp.objPath);
        file_object_factory(comm, &data_obj_inp, file_obj, data_obj_info);

        return result;
    } // resolve_resource_hierarchy

    irods::resolve_hierarchy_result_type resolve_resource_hierarchy(
        rsComm_t*            _comm,
        const std::string&   _oper_in,
        dataObjInp_t&        _data_obj_inp,
        irods::file_object_factory_result& _file_obj_result)
    {
        if (!_comm) {
            THROW(SYS_INVALID_INPUT_PARAM, "null comm pointer");
        }

        // =-=-=-=-=-=-=-
        // if this is a special collection then we need to get the hier
        // pass that along and bail as it is not a data object, or if
        // it is just a not-so-special collection then we continue with
        // processing the operation, as this may be a create op
        rodsObjStat_t *rodsObjStatOut = NULL;
        if (collStat(_comm, &_data_obj_inp, &rodsObjStatOut) >= 0 && rodsObjStatOut->specColl) {
            std::string hier = rodsObjStatOut->specColl->rescHier;
            freeRodsObjStat( rodsObjStatOut );
            return {irods::file_object_ptr{}, hier};
        }
        freeRodsObjStat(rodsObjStatOut);

        irods::file_object_ptr file_obj = std::get<irods::file_object_ptr>(_file_obj_result);
        irods::error fac_err = std::get<irods::error>(_file_obj_result);

        auto key_word = get_keyword_from_inp(_data_obj_inp);

        // Providing a replica number means the client is attempting to target an existing replica.
        // Therefore, usage of the replica number cannot be used during a create operation. The
        // operation must be changed so that the system does not attempt to create the replica.
        auto oper = _oper_in;
        if (irods::CREATE_OPERATION == oper && getValByKey(&_data_obj_inp.condInput, REPL_NUM_KW)) {
            oper = irods::WRITE_OPERATION;
        }

        char* default_resc_name  = getValByKey(&_data_obj_inp.condInput, DEF_RESC_NAME_KW);

        if (irods::CREATE_OPERATION == oper) {
            std::string create_resc_name{};
            if (!key_word.empty()) {
                create_resc_name = key_word;
            }
            else if (default_resc_name) {
                create_resc_name = default_resc_name;
            }

            apply_policy_for_create_operation(_comm, _data_obj_inp, create_resc_name);

            // If the data object exists, need to consider what operation is actually being done.
            // In the case of a put or copy/rsync, which operates on the logical level, this should be
            // treated as an overwrite because if any replica exists, we are considered to be
            // overwriting the data object.. Otherwise, the operation is considering individual
            // replicas and needs to see whether the hierarchy matching the provided keyword has a
            // replica for this object.
            const auto logical_operation = PUT_OPR == _data_obj_inp.oprType ||
                                           COPY_DEST == _data_obj_inp.oprType ||
                                           RSYNC_OPR == _data_obj_inp.oprType;
            if (fac_err.ok() && (logical_operation || hier_has_replica(create_resc_name, file_obj))) {
                oper = irods::WRITE_OPERATION;
            }
            else {
                const auto hier = resolve_hier_for_create( _comm, file_obj, create_resc_name);
                return {file_obj, hier};
            }
        }

        // =-=-=-=-=-=-=-
        // perform an open operation if create is not specified ( that's all we have for now )
        if (irods::OPEN_OPERATION  == oper || irods::WRITE_OPERATION == oper || irods::UNLINK_OPERATION == oper ) {
            if (!fac_err.ok()) {
                THROW(fac_err.code(), fmt::format(
                    "[{}:{}] - failed in file_object_factory:[{}]",
                    __FUNCTION__, __LINE__, fac_err.result()));
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
    } // resolve_resource_hierarchy

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
        // host name. if we do, then we're local.
        for (auto* h = last_resc_host->hostName; h; h = h->next) {
            if (h->name == host_name) {
                _out_hier = resc_hier;
                _out_flag = LOCAL_HOST;
                _out_host = 0;
                return SUCCESS();
            }
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
