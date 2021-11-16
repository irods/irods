#include "rsGeneralAdmin.hpp"

#include "administration_utilities.hpp"
#include "generalAdmin.h"
#include "rodsConnect.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "rodsErrorTable.h"
#include "rsModAVUMetadata.hpp"
#include "rsGenQuery.hpp"
#include "irods_children_parser.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_resource_manager.hpp"
#include "irods_file_object.hpp"
#include "irods_resource_constants.hpp"
#include "irods_load_plugin.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_hierarchy_parser.hpp"
#include "user_validation_utilities.hpp"

#include "fmt/format.h"

#include <boost/date_time.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>

extern irods::resource_manager resc_mgr;

namespace
{
    auto contains_whitespace(std::string _value) -> bool
    {
        boost::algorithm::trim(_value);

        const auto b = std::begin(_value);
        const auto e = std::end(_value);

        return std::find_if(b, e, [](unsigned char _ch) { return ::isspace(_ch); }) != e;
    } // contains_whitespace

    auto throw_if_group_is_changing_to_user_or_user_is_changing_to_group(
        RsComm& _comm,
        const std::string_view _option,
        const std::string_view _current_user_type,
        const std::string_view _new_user_type) -> void
    {
        // If the type of the user or group is not changing, then a group is not changing to a
        // user and a user is not changing to a group by definition. Return immediately.
        if (_option != "type") {
            return;
        }

        if (!irods::user::type_is_valid(_current_user_type) ||
            !irods::user::type_is_valid(_new_user_type)) {
            THROW(CAT_INVALID_USER_TYPE, "user type is not recognized");
        }

        if (_current_user_type == "rodsgroup") {
            constexpr auto err = SYS_NOT_ALLOWED;
            const auto msg = "a group cannot have its type changed";
            addRErrorMsg(&_comm.rError, err, msg);
            THROW(err, msg);
        }

        if (_new_user_type == "rodsgroup") {
            constexpr auto err = SYS_NOT_ALLOWED;
            const auto msg = "a user cannot be changed to a group";
            addRErrorMsg(&_comm.rError, err, msg);
            THROW(err, msg);
        }

        // If this point is reached, this is a rodsuser or a rodsadmin having its type changed.
        // There are checks later on which enforce the type to which the user is being chagned.
    } // throw_if_group_is_changing_to_user_or_user_is_changing_to_group

    auto throw_if_password_is_being_set_on_a_group(
        RsComm& _comm,
        const std::string_view _option,
        const std::string_view _user_type) -> void
    {
        if (_option != "password") {
            return;
        }

        if (_user_type == "rodsgroup") {
            constexpr auto err = SYS_NOT_ALLOWED;
            const auto msg = "a group cannot have a password";
            addRErrorMsg(&_comm.rError, err, msg);
            THROW(err, msg);
        }
    } // throw_if_password_is_being_set_on_a_group
} // anonymous namespace

int _check_rebalance_timestamp_avu_on_resource(
    rsComm_t* _rsComm,
    const std::string& _resource_name) {

    // build genquery to find active or stale "rebalance operation" entries for this resource
    genQueryOut_t* gen_out = nullptr;
    char tmp_str[MAX_NAME_LEN];
    genQueryInp_t gen_inp{};

    snprintf( tmp_str, MAX_NAME_LEN, "='%s'", _resource_name.c_str() );
    addInxVal( &gen_inp.sqlCondInp, COL_R_RESC_NAME, tmp_str );
    snprintf( tmp_str, MAX_NAME_LEN, "='rebalance_operation'" );
    addInxVal( &gen_inp.sqlCondInp, COL_META_RESC_ATTR_NAME, tmp_str );
    addInxIval( &gen_inp.selectInp, COL_META_RESC_ATTR_VALUE, 1 );
    addInxIval( &gen_inp.selectInp, COL_META_RESC_ATTR_UNITS, 1 );
    gen_inp.maxRows = 1;
    int status = rsGenQuery( _rsComm, &gen_inp, &gen_out );
    // if existing entry found, report and exit
    if ( status >= 0 ) {
        sqlResult_t* hostname_and_pid;
        sqlResult_t* timestamp;
        if ( ( hostname_and_pid = getSqlResultByInx( gen_out, COL_META_RESC_ATTR_VALUE ) ) == nullptr ) {
            rodsLog( LOG_ERROR, "%s: getSqlResultByInx for COL_META_RESC_ATTR_VALUE failed", __FUNCTION__ );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( timestamp = getSqlResultByInx( gen_out, COL_META_RESC_ATTR_UNITS ) ) == nullptr ) {
            rodsLog( LOG_ERROR, "%s: getSqlResultByInx for COL_META_RESC_ATTR_UNITS failed", __FUNCTION__ );
            return UNMATCHED_KEY_OR_INDEX;
        }
        std::stringstream msg;
        msg << "A rebalance_operation on resource [";
        msg << _resource_name.c_str();
        msg << "] is still active (or stale) [";
        msg << &hostname_and_pid->value[0];
        msg << "] [";
        msg << &timestamp->value[0];
        msg << "]";
        rodsLog( LOG_ERROR, "%s: %s", __FUNCTION__, msg.str().c_str() );
        addRErrorMsg( &_rsComm->rError, REBALANCE_ALREADY_ACTIVE_ON_RESOURCE, msg.str().c_str() );
        return REBALANCE_ALREADY_ACTIVE_ON_RESOURCE;
    }
    freeGenQueryOut( &gen_out );
    clearGenQueryInp( &gen_inp );
    return 0;
}

int _set_rebalance_timestamp_avu_on_resource(
    rsComm_t* _rsComm,
    const std::string& _resource_name) {

    modAVUMetadataInp_t modAVUMetadataInp{};
    irods::at_scope_exit<std::function<void()>> at_scope_exit{[&] {
        free( modAVUMetadataInp.arg0 );
        free( modAVUMetadataInp.arg1 );
        free( modAVUMetadataInp.arg2 );
        free( modAVUMetadataInp.arg3 );
        free( modAVUMetadataInp.arg4 );
        free( modAVUMetadataInp.arg5 );
    }};
    // get hostname
    char hostname[MAX_NAME_LEN];
    gethostname(hostname, MAX_NAME_LEN);
    // get PID
    int pid = getpid();
    // get timestamp in defined format
    const boost::posix_time::ptime timeUTC = boost::posix_time::second_clock::universal_time();
    std::stringstream stream;
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
    facet->format("%Y%m%dT%H%M%SZ");
    stream.imbue(std::locale(std::locale::classic(), facet));
    stream << timeUTC;
    // add AVU to resource
    modAVUMetadataInp.arg0 = strdup( "set" );
    modAVUMetadataInp.arg1 = strdup( "-R" );
    modAVUMetadataInp.arg2 = strdup( _resource_name.c_str() );
    modAVUMetadataInp.arg3 = strdup( "rebalance_operation" );
    std::string value = std::string(hostname) + std::string(":") + std::to_string(pid);
    modAVUMetadataInp.arg4 = strdup( value.c_str() );
    std::string unit = stream.str();
    modAVUMetadataInp.arg5 = strdup( unit.c_str() );
    // do it
    return rsModAVUMetadata( _rsComm, &modAVUMetadataInp );
}

int _check_and_set_rebalance_timestamp_avu_on_resource(
    rsComm_t* _rsComm,
    const std::string& _resource_name) {

    int status = _check_rebalance_timestamp_avu_on_resource(_rsComm, _resource_name);
    if (status < 0 ){
        return status;
    }
    status = _set_rebalance_timestamp_avu_on_resource(_rsComm, _resource_name);
    if (status < 0 ){
        return status;
    }
    return 0;
}

int _remove_rebalance_timestamp_avu_from_resource(
    rsComm_t* _rsComm,
    const std::string& _resource_name) {
    // build genquery to find active or stale "rebalance operation" entries for this resource
    genQueryOut_t* gen_out = nullptr;
    char tmp_str[MAX_NAME_LEN];
    genQueryInp_t gen_inp{};

    snprintf( tmp_str, MAX_NAME_LEN, "='%s'", _resource_name.c_str() );
    addInxVal( &gen_inp.sqlCondInp, COL_R_RESC_NAME, tmp_str );
    snprintf( tmp_str, MAX_NAME_LEN, "='rebalance_operation'" );
    addInxVal( &gen_inp.sqlCondInp, COL_META_RESC_ATTR_NAME, tmp_str );
    addInxIval( &gen_inp.selectInp, COL_META_RESC_ATTR_VALUE, 1 );
    addInxIval( &gen_inp.selectInp, COL_META_RESC_ATTR_UNITS, 1 );
    gen_inp.maxRows = 1;
    int status = rsGenQuery( _rsComm, &gen_inp, &gen_out );
    // if existing entry found, remove it
    if ( status >= 0 ) {
        modAVUMetadataInp_t modAVUMetadataInp{};
        irods::at_scope_exit<std::function<void()>> at_scope_exit{[&] {
            free( modAVUMetadataInp.arg0 );
            free( modAVUMetadataInp.arg1 );
            free( modAVUMetadataInp.arg2 );
            free( modAVUMetadataInp.arg3 );
            free( modAVUMetadataInp.arg4 );
            free( modAVUMetadataInp.arg5 );
        }};
        // remove AVU from resource
        modAVUMetadataInp.arg0 = strdup( "rmw" );
        modAVUMetadataInp.arg1 = strdup( "-R" );
        modAVUMetadataInp.arg2 = strdup( _resource_name.c_str() );
        modAVUMetadataInp.arg3 = strdup( "rebalance_operation" );
        modAVUMetadataInp.arg4 = strdup( "%" );
        modAVUMetadataInp.arg5 = strdup( "%" );
        // do it
        return rsModAVUMetadata( _rsComm, &modAVUMetadataInp );
    }
    else {
        return 0;
    }
}


int
rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "generalAdmin" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsGeneralAdmin( rsComm, generalAdminInp );
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
        status = rcGeneralAdmin( rodsServerHost->conn,
                                 generalAdminInp );

        // =-=-=-=-=-=-=-
        // always replicate the error stack [#2184] for 'iadmin lt' reporting
        // back to the client if it had been redirected
        replErrorStack( rodsServerHost->conn->rError, &rsComm->rError );

    }
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGeneralAdmin: rcGeneralAdmin error %d", status );
    }
    return status;
}

int _addChildToResource(generalAdminInp_t* _generalAdminInp, rsComm_t* _rsComm)
{
    int result = 0;
    std::map<std::string, std::string> resc_input;

    {
        const auto length_exceeded = [](const auto& _resource_name)
        {
            return std::strlen(_resource_name) >= NAME_LEN;
        };

        if (length_exceeded(_generalAdminInp->arg2) || length_exceeded(_generalAdminInp->arg3)) {
            return CAT_RESOURCE_NAME_LENGTH_EXCEEDED;
        }

        if (contains_whitespace(_generalAdminInp->arg2)) {
            rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", _generalAdminInp->arg2);
            return CAT_INVALID_RESOURCE_NAME;
        }

        if (contains_whitespace(_generalAdminInp->arg3)) {
            rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", _generalAdminInp->arg3);
            return CAT_INVALID_RESOURCE_NAME;
        }

        // If the resource names are the same, then return an error.
        if (std::strncmp(_generalAdminInp->arg2, _generalAdminInp->arg3, NAME_LEN) == 0) {
            std::string msg = "Cannot add resource [";
            msg += _generalAdminInp->arg3;
            msg += "] as child to itself.";

            // TODO Investigate how to detect the verbose flag.
            // Add this message to the rError stack should happen automatically.
            // The apiHandler does not know about the verbose flag for iadmin yet.
            addRErrorMsg(&_rsComm->rError, HIERARCHY_ERROR, msg.c_str());
            rodsLog(LOG_ERROR, msg.c_str());

            return HIERARCHY_ERROR;
        }

        // Case 1: Return CHILD_HAS_PARENT if the child has a parent resource.
        // Case 2: Return HIERARCHY_ERROR if the child is a parent of the parent.

        const auto is_root_resource = [](const auto& _resource_name) -> std::tuple<irods::error, boost::optional<bool>>
        {
            std::string hier;
            auto err = resc_mgr.get_hier_to_root_for_resc(_resource_name, hier);

            if (!err.ok()) {
                return std::make_tuple(err, boost::none);
            }

            irods::hierarchy_parser hier_parser;
            hier_parser.set_string(hier);

            int levels = 0;
            err = hier_parser.num_levels(levels);

            if (!err.ok()) {
                return std::make_tuple(err, boost::none);
            }

            return {SUCCESS(), 1 == levels};
        };

        {
            // Return an error if the child resource already has a parent.
            auto result = is_root_resource(_generalAdminInp->arg3);
            auto& err = std::get<0>(result);
            auto& res = std::get<1>(result);

            if (err.ok()) {
                if (res && !*res) {
                    std::string msg = "Child resource [";
                    msg += _generalAdminInp->arg3;
                    msg += "] already has a parent resource.";

                    addRErrorMsg(&_rsComm->rError, CHILD_HAS_PARENT, msg.c_str());
                    rodsLog(LOG_ERROR, msg.c_str());

                    return CHILD_HAS_PARENT;
                }
            }
            // The resource manager returns a different error code when a resource doesn't exist.
            // If "SYS_RESC_DOES_NOT_EXIST" is returned, make sure to translate it to the
            // appropriate error code to maintain backwards compatibility.
            else if (err.code() == SYS_RESC_DOES_NOT_EXIST) {
                addRErrorMsg(&_rsComm->rError, CHILD_NOT_FOUND, err.user_result().c_str());
                rodsLog(LOG_ERROR, err.result().c_str());
                return CHILD_NOT_FOUND;
            }
            else {
                addRErrorMsg(&_rsComm->rError, err.code(), err.user_result().c_str());
                rodsLog(LOG_ERROR, err.result().c_str());
                return err.code();
            }
        }

        std::string hier;
        const auto err = resc_mgr.get_hier_to_root_for_resc(_generalAdminInp->arg2, hier);

        if (!err.ok()) {
            // The resource manager returns a different error code when a resource doesn't exist.
            // If "SYS_RESC_DOES_NOT_EXIST" is returned, make sure to translate it to the
            // appropriate error code to maintain backwards compatibility.
            if (err.code() == SYS_RESC_DOES_NOT_EXIST) {
                addRErrorMsg(&_rsComm->rError, CAT_INVALID_RESOURCE, err.user_result().c_str());
                rodsLog(LOG_ERROR, err.result().c_str());
                return CAT_INVALID_RESOURCE;
            }

            addRErrorMsg(&_rsComm->rError, err.code(), err.user_result().c_str());
            rodsLog(LOG_ERROR, err.result().c_str());
            return err.code();
        }

        irods::hierarchy_parser hier_parser;
        hier_parser.set_string(hier);

        // Return an error if the child resource is an ancestor of the parent resource.
        const auto end = std::end(hier_parser);
        if (std::find(std::begin(hier_parser), end, _generalAdminInp->arg3) != end)
        {
            std::string msg = "Cannot add ancestor resource [";
            msg += _generalAdminInp->arg3;
            msg += "] as child to descendant resource [";
            msg += _generalAdminInp->arg2;
            msg += "].";

            addRErrorMsg(&_rsComm->rError, HIERARCHY_ERROR, msg.c_str());
            rodsLog(LOG_ERROR, msg.c_str());

            return HIERARCHY_ERROR;
        }
    }

    resc_input[irods::RESOURCE_NAME] = _generalAdminInp->arg2;

    std::string rescChild( _generalAdminInp->arg3 );
    std::string rescContext( _generalAdminInp->arg4 );

    if (rescContext.find(';') != std::string::npos) {
        rodsLog(LOG_ERROR, "_addChildToResource: semicolon ';' not allowed in child context string [%s]", rescContext.c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    if (rescContext.find('{') != std::string::npos) {
        rodsLog(LOG_ERROR, "_addChildToResource: open curly bracket '{' not allowed in child context string [%s]", rescContext.c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    if (rescContext.find('}') != std::string::npos) {
        rodsLog(LOG_ERROR, "_addChildToResource: close curly bracket '}' not allowed in child context string [%s]", rescContext.c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    irods::children_parser parser;
    parser.add_child( rescChild, rescContext );
    std::string rescChildren;
    parser.str( rescChildren );
    if ( rescChildren.length() >= MAX_PATH_ALLOWED ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    resc_input[irods::RESOURCE_CHILDREN] = rescChildren;

    rodsLog( LOG_NOTICE, "rsGeneralAdmin add child \"%s\" to resource \"%s\"", rescChildren.c_str(),
             resc_input[irods::RESOURCE_NAME].c_str() );

    if ( ( result = chlAddChildResc( _rsComm, resc_input ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    return result;
}

int
_removeChildFromResource(
    generalAdminInp_t* _generalAdminInp,
    rsComm_t*          _rsComm ) {

    int result = 0;
    std::map<std::string, std::string> resc_input;

    if ( strlen( _generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
        return SYS_INVALID_INPUT_PARAM;
    }

    if (contains_whitespace(_generalAdminInp->arg2)) {
        rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", _generalAdminInp->arg2);
        return CAT_INVALID_RESOURCE_NAME;
    }

    resc_input[irods::RESOURCE_NAME] = boost::algorithm::trim_copy(std::string{_generalAdminInp->arg2});

    if ( strlen( _generalAdminInp->arg3 ) >= MAX_PATH_ALLOWED ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    if (contains_whitespace(_generalAdminInp->arg3)) {
        rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", _generalAdminInp->arg3);
        return CAT_INVALID_RESOURCE_NAME;
    }

    resc_input[irods::RESOURCE_CHILDREN] = boost::algorithm::trim_copy(std::string{_generalAdminInp->arg3});

    rodsLog( LOG_NOTICE, "rsGeneralAdmin remove child \"%s\" from resource \"%s\"", resc_input[irods::RESOURCE_CHILDREN].c_str(),
             resc_input[irods::RESOURCE_NAME].c_str() );

    if ( ( result = chlDelChildResc( _rsComm, resc_input ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    return result;
}

/* extracted out the functionality for adding a resource to the grid - hcj */
int
_addResource(
    generalAdminInp_t* _generalAdminInp,
    ruleExecInfo_t&    _rei2,
    rsComm_t*          _rsComm )
{
    int result = 0;
    static const unsigned int argc = 7;
    const char *args[argc];
    std::map<std::string, std::string> resc_input;

    // =-=-=-=-=-=-=-
    // Legacy checks

    // resource name
    if ( strlen( _generalAdminInp->arg2 ) >= NAME_LEN ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    if (contains_whitespace(_generalAdminInp->arg2)) {
        rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", _generalAdminInp->arg2);
        return CAT_INVALID_RESOURCE_NAME;
    }

    // resource type
    if ( strlen( _generalAdminInp->arg3 ) >= NAME_LEN ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // resource context
    if ( strlen( _generalAdminInp->arg5 ) >= MAX_PATH_ALLOWED ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // resource zone
    if ( strlen( _generalAdminInp->arg6 ) >= NAME_LEN ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // capture all the parameters
    resc_input[irods::RESOURCE_NAME] = boost::algorithm::trim_copy(std::string{_generalAdminInp->arg2});
    resc_input[irods::RESOURCE_TYPE] = _generalAdminInp->arg3;
    resc_input[irods::RESOURCE_PATH] = _generalAdminInp->arg4;
    resc_input[irods::RESOURCE_CONTEXT] = _generalAdminInp->arg5;
    resc_input[irods::RESOURCE_ZONE] = _generalAdminInp->arg6;
    resc_input[irods::RESOURCE_CLASS] = irods::RESOURCE_CLASS_CACHE;
    resc_input[irods::RESOURCE_CHILDREN] = "";
    resc_input[irods::RESOURCE_PARENT] = "";

    // =-=-=-=-=-=-=-
    // if it is not empty, parse out the host:path otherwise
    // fill in with the EMPTY placeholders
    if ( !resc_input[irods::RESOURCE_PATH].empty() ) {
        // =-=-=-=-=-=-=-
        // separate the location:/vault/path pair
        std::vector< std::string > tok;
        irods::string_tokenize( resc_input[irods::RESOURCE_PATH], ":", tok );

        // =-=-=-=-=-=-=-
        // if we have exactly 2 tokens, things are going well
        if ( 2 == tok.size() ) {
            // =-=-=-=-=-=-=-
            // location is index 0, path is index 1
            if ( strlen( tok[0].c_str() ) >= NAME_LEN ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            resc_input[irods::RESOURCE_LOCATION] = tok[0];

            if ( strlen( tok[1].c_str() ) >= MAX_NAME_LEN ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            resc_input[irods::RESOURCE_PATH] = tok[1];
        }

    }
    else {
        resc_input[irods::RESOURCE_LOCATION] = irods::EMPTY_RESC_HOST;
        resc_input[irods::RESOURCE_PATH] = irods::EMPTY_RESC_PATH;
    }

    // =-=-=-=-=-=-=-
    args[0] = resc_input[irods::RESOURCE_NAME].c_str();
    args[1] = resc_input[irods::RESOURCE_TYPE].c_str();
    args[2] = resc_input[irods::RESOURCE_CLASS].c_str();
    args[3] = resc_input[irods::RESOURCE_LOCATION].c_str();
    args[4] = resc_input[irods::RESOURCE_PATH].c_str();
    args[5] = resc_input[irods::RESOURCE_CONTEXT].c_str();
    args[6] = resc_input[irods::RESOURCE_ZONE].c_str();

    // =-=-=-=-=-=-=-
    // Check that there is a plugin matching the resource type
    irods::plugin_name_generator name_gen;
    // =-=-=-=-=-=-=-
    // resolve plugin directory
    std::string plugin_home;
    irods::error ret = irods::resolve_plugin_path( irods::PLUGIN_TYPE_RESOURCE, plugin_home );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    if ( !name_gen.exists( resc_input[irods::RESOURCE_TYPE], plugin_home ) ) {
        rodsLog(
            LOG_DEBUG,
            "No plugin exists to provide resource [%s] of type [%s]",
            resc_input[irods::RESOURCE_NAME].c_str(),
            resc_input[irods::RESOURCE_TYPE].c_str() );
    }

    // =-=-=-=-=-=-=-
    // apply preproc policy enforcement point for creating a resource, handle errors
    if ( ( result =  applyRuleArg( "acPreProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin: acPreProcForCreateResource error for %s, stat=%d",
                 resc_input[irods::RESOURCE_NAME].c_str(), result );
    }

    // =-=-=-=-=-=-=-
    // register resource with the metadata catalog, roll back on an error
    else if ( ( result = chlRegResc( _rsComm, resc_input ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    // =-=-=-=-=-=-=-
    // apply postproc policy enforcement point for creating a resource, handle errors
    else if ( ( result =  applyRuleArg( "acPostProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin: acPostProcForCreateResource error for %s, stat=%d",
                 resc_input[irods::RESOURCE_NAME].c_str(), result );
    }

    return result;
}

int
_listRescTypes( rsComm_t* _rsComm ) {
    // =-=-=-=-=-=-=-
    // resolve plugin directory
    std::string plugin_home;
    irods::error ret = irods::resolve_plugin_path( irods::PLUGIN_TYPE_RESOURCE, plugin_home );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    int result = 0;
    irods::plugin_name_generator name_gen;
    irods::plugin_name_generator::plugin_list_t plugin_list;
    ret = name_gen.list_plugins( plugin_home, plugin_list );
    if ( ret.ok() ) {
        std::stringstream msg;
        for ( irods::plugin_name_generator::plugin_list_t::iterator it = plugin_list.begin();
                result == 0 && it != plugin_list.end(); ++it ) {
            msg << *it << std::endl;
        }
        if ( result == 0 ) {
            addRErrorMsg( &_rsComm->rError, STDOUT_STATUS, msg.str().c_str() );
        }
    }
    else {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to generate the list of resource plugins.";
        irods::error res = PASSMSG( msg.str(), ret );
        irods::log( res );
        result = res.code();
    }
    return result;
}

int
_rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp ) {
    int status;
    collInfo_t collInfo;
    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }


    rodsLog( LOG_DEBUG,
             "_rsGeneralAdmin arg0=%s",
             generalAdminInp->arg0 );

    if ( strcmp( generalAdminInp->arg0, "add" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            return irods::create_user(*rsComm,
                                      generalAdminInp->arg2 ? generalAdminInp->arg2 : "",
                                      generalAdminInp->arg3 ? generalAdminInp->arg3 : "",
                                      generalAdminInp->arg5 ? generalAdminInp->arg5 : "",
                                      generalAdminInp->arg4 ? generalAdminInp->arg4 : "");
        }
        if ( strcmp( generalAdminInp->arg1, "dir" ) == 0 ) {
            memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
            snprintf( collInfo.collName, sizeof( collInfo.collName ), "%s", generalAdminInp->arg2 );
            if ( strlen( generalAdminInp->arg3 ) > 0 ) {
                snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ), "%s", generalAdminInp->arg3 );
                status = chlRegCollByAdmin( rsComm, &collInfo );
                if ( status == 0 ) {
                    if ( !chlCommit( rsComm ) ) {
                        // JMC - ERROR?
                    }
                }
            }
            else {
                status = chlRegColl( rsComm, &collInfo );
            }
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlRegZone( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3,
                                 generalAdminInp->arg4,
                                 generalAdminInp->arg5 );
            if ( status == 0 ) {
                if ( strcmp( generalAdminInp->arg3, "remote" ) == 0 ) {
                    memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
                    snprintf( collInfo.collName, sizeof( collInfo.collName ), "/%s", generalAdminInp->arg2 );
                    snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ), "%s", rsComm->proxyUser.userName );
                    status = chlRegCollByAdmin( rsComm, &collInfo );
                    if ( status == 0 ) {
                        chlCommit( rsComm );
                    }
                }
            }
            return status;
        } // add user

        // =-=-=-=-=-=-=-
        // add a new resource to the data grid
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {
            return _addResource( generalAdminInp, rei2, rsComm );
        } // if create resource

        /* add a child resource to the specified parent resource */
        if ( strcmp( generalAdminInp->arg1, "childtoresc" ) == 0 ) {
            return _addChildToResource( generalAdminInp, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {
            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            args[2] = generalAdminInp->arg4;
            args[3] = generalAdminInp->arg5;
            args[4] = generalAdminInp->arg6;
            args[5] = generalAdminInp->arg7;
            argc = 6;
            i =  applyRuleArg( "acPreProcForCreateToken", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin: acPreProcForCreateToken error for %s.%s=%s, stat=%d",
                         args[0], args[1], args[2], i );
                return i;
            }

            status = chlRegToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3,
                                  generalAdminInp->arg4,
                                  generalAdminInp->arg5,
                                  generalAdminInp->arg6,
                                  generalAdminInp->arg7 );
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForCreateToken", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForCreateToken error for %s.%s=%s, stat=%d",
                             args[0], args[1], args[2], i );
                    return i;
                }
            }
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        } // token

        if ( strcmp( generalAdminInp->arg1, "specificQuery" ) == 0 ) {
            status = chlAddSpecificQuery( rsComm, generalAdminInp->arg2,
                                          generalAdminInp->arg3 );
            return status;
        }

    } // add

    if ( strcmp( generalAdminInp->arg0, "modify" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            const auto user_name = std::string_view{generalAdminInp->arg2 ?
                                                    generalAdminInp->arg2 : ""};

            const auto option = std::string_view{generalAdminInp->arg3 ?
                                                 generalAdminInp->arg3 : ""};

            const auto new_value = std::string_view{generalAdminInp->arg4 ?
                                                    generalAdminInp->arg4 : ""};

            try {
                // Store the user type here because we need to fetch it from the catalog and
                // it will be used multiple times. Note: subject to TOCTOU problem.
                const auto current_user_type = irods::user::get_type(*rsComm, user_name);

                throw_if_group_is_changing_to_user_or_user_is_changing_to_group(
                    *rsComm, option, current_user_type, new_value);

                throw_if_password_is_being_set_on_a_group(*rsComm, option, current_user_type);
            }
            catch (const irods::exception& e) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]",
                    __func__, __LINE__, e.client_display_what()));
                return e.code();
            }

            args[0] = user_name.data();
            args[1] = option.data();
            /* Since the obfuscated password might contain commas, single or
               double quotes, etc, it's hard to escape for processing (often
               causing a seg fault), so for now just pass in a dummy string.
               It is also unlikely the microservice actually needs the obfuscated
               password. */
            args[2] = "obfuscatedPw";
            argc = 3;
            i =  applyRuleArg( "acPreProcForModifyUser", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin: acPreProcForModifyUser error for %s and option %s, stat=%d",
                         args[0], args[1], i );
                return i;
            }

            status = chlModUser(rsComm, user_name.data(), option.data(), new_value.data());

            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyUser", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForModifyUser error for %s and option %s, stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "group" ) == 0 ) {
            userInfo_t ui;
            memset( &ui, 0, sizeof( userInfo_t ) );
            rei2.uoio = &ui;
            rstrcpy( ui.userName, generalAdminInp->arg4, NAME_LEN );
            rstrcpy( ui.rodsZone, generalAdminInp->arg5, NAME_LEN );
            args[0] = generalAdminInp->arg2; /* groupname */
            args[1] = generalAdminInp->arg3; /* option */
            args[2] = generalAdminInp->arg4; /* username */
            args[3] = generalAdminInp->arg5; /* zonename */
            argc = 4;
            i =  applyRuleArg( "acPreProcForModifyUserGroup", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin: acPreProcForModifyUserGroup error for %s and option %s, stat=%d",
                         args[0], args[1], i );
                return i;
            }

            status = chlModGroup( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3, generalAdminInp->arg4,
                                  generalAdminInp->arg5 );
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyUserGroup", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForModifyUserGroup error for %s and option %s, stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlModZone( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3, generalAdminInp->arg4 );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            if ( status == 0 &&
                    strcmp( generalAdminInp->arg3, "name" ) == 0 ) {
                char oldName[MAX_NAME_LEN];
                char newName[MAX_NAME_LEN];
                snprintf( oldName, sizeof( oldName ), "/%s", generalAdminInp->arg2 );
                snprintf( newName, sizeof( newName ), "%s", generalAdminInp->arg4 );
                status = chlRenameColl( rsComm, oldName, newName );
                if ( status == 0 ) {
                    chlCommit( rsComm );
                }
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zonecollacl" ) == 0 ) {
            status = chlModZoneCollAcl( rsComm, generalAdminInp->arg2,
                                        generalAdminInp->arg3, generalAdminInp->arg4 );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "localzonename" ) == 0 ) {
            /* run the acRenameLocalZone rule */
            const char *args[2];
            ruleExecInfo_t rei;
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            rei.uoic = &rsComm->clientUser;
            rei.uoip = &rsComm->proxyUser;
            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            status = applyRuleArg( "acRenameLocalZone", args, 2, &rei,
                                   NO_SAVE_REI );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resourcedatapaths" ) == 0 ) {
            status = chlModRescDataPaths( rsComm, generalAdminInp->arg2,
                                          generalAdminInp->arg3, generalAdminInp->arg4,
                                          generalAdminInp->arg5 );

            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {
            argc = 3;
            args[0] = generalAdminInp->arg2; // resource name
            args[1] = generalAdminInp->arg3; // option

            if (contains_whitespace(generalAdminInp->arg2)) {
                rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", generalAdminInp->arg2);
                return CAT_INVALID_RESOURCE_NAME;
            }

            // Creates storage for possibly renaming an existing resource.
            // This is required so that the string out lives the C API calls.
            std::string new_resc_name;

            if (std::strcmp(generalAdminInp->arg3, "name") == 0) {
                if (contains_whitespace(generalAdminInp->arg4)) {
                    rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", generalAdminInp->arg4);
                    return CAT_INVALID_RESOURCE_NAME;
                }

                new_resc_name = boost::algorithm::trim_copy(std::string{generalAdminInp->arg4});
                args[2] = new_resc_name.c_str();
            }
            else {
                args[2] = generalAdminInp->arg4; // new value
            }

            i =  applyRuleArg( "acPreProcForModifyResource", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin: acPreProcForModifyResource error for %s and option %s, stat=%d",
                         args[0], args[1], i );
                return i;
            }

            // =-=-=-=-=-=-=-
            // addition of 'rebalance' as an option to iadmin modresc.  not in icat code
            // due to dependency on resc_mgr, etc.
            if ( 0 == strcmp( args[1], "rebalance" ) ) {
                status = 0;

                // =-=-=-=-=-=-=-
                // resolve the plugin we wish to rebalance
                irods::resource_ptr resc;
                irods::error ret = resc_mgr.resolve( args[0], resc );
                if ( !ret.ok() ) {
                    irods::log( PASSMSG( "failed to resolve resource", ret ) );
                    status = ret.code();
                }
                else {
                    int visibility_status = _check_and_set_rebalance_timestamp_avu_on_resource(rsComm, args[0]);
                    if (visibility_status < 0){
                        return visibility_status;
                    }
                    // =-=-=-=-=-=-=-
                    // call the rebalance operation on the resource
                    irods::file_object_ptr obj( new irods::file_object() );
                    ret = resc->call( rsComm, irods::RESOURCE_OP_REBALANCE, obj );
                    if ( !ret.ok() ) {
                        irods::log( PASSMSG( "failed to rebalance resource", ret ) );
                        status = ret.code();

                    }
                    visibility_status = _remove_rebalance_timestamp_avu_from_resource(rsComm, args[0]);
                    if (visibility_status < 0){
                        return visibility_status;
                    }
                }
            }
            else {
                status = chlModResc(rsComm, generalAdminInp->arg2, generalAdminInp->arg3, generalAdminInp->arg4);
            }

            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyResource", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForModifyResource error for %s and option %s, stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }

            if ( status != 0 ) {
                chlRollback( rsComm );
            }

            return status;
        }
    }
    if ( strcmp( generalAdminInp->arg0, "rm" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            return irods::remove_user(*rsComm,
                                      generalAdminInp->arg2 ? generalAdminInp->arg2 : "",
                                      generalAdminInp->arg3 ? generalAdminInp->arg3 : "");
        }
        if ( strcmp( generalAdminInp->arg1, "dir" ) == 0 ) {
            memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
            snprintf( collInfo.collName, sizeof( collInfo.collName ), "%s", generalAdminInp->arg2 );
            if ( collInfo.collName[sizeof( collInfo.collName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            status = chlDelColl( rsComm, &collInfo );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {

            std::string resc_name = "";

            // =-=-=-=-=-=-=-
            // JMC 04.11.2012 :: add a dry run option to idamin rmresc
            //                :: basically run chlDelResc then run a rollback immediately after
            if ( strcmp( generalAdminInp->arg3, "--dryrun" ) == 0 ) {

                if ( strlen( generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
                    return SYS_INVALID_INPUT_PARAM;
                }

                resc_name = generalAdminInp->arg2;

                rodsLog( LOG_STATUS, "Executing a dryrun of removal of resource [%s]", generalAdminInp->arg2 );

                status = chlDelResc( rsComm, resc_name, 1 );
                if ( 0 == status ) {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: SUCCESS", generalAdminInp->arg2 );
                }
                else {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: FAILURE", generalAdminInp->arg2 );
                }

                return status;
            } // if dryrun
            // =-=-=-=-=-=-=-

            if ( strlen( generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
                return SYS_INVALID_INPUT_PARAM;
            }

            if (contains_whitespace(generalAdminInp->arg2)) {
                rodsLog(LOG_ERROR, "Whitespace is not allowed in resource names [%s].", generalAdminInp->arg2);
                return CAT_INVALID_RESOURCE_NAME;
            }

            resc_name = generalAdminInp->arg2;

            args[0] = resc_name.c_str();
            argc = 1;
            i =  applyRuleArg( "acPreProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin: acPreProcForDeleteResource error for %s, stat=%d",
                         resc_name.c_str(), i );
                return i;
            }

            status = chlDelResc( rsComm, resc_name );
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForDeleteResource error for %s, stat=%d",
                             resc_name.c_str(), i );
                    return i;
                }
            }

            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }

        /* remove a child resource from the specified parent resource */
        if ( strcmp( generalAdminInp->arg1, "childfromresc" ) == 0 ) {
            return _removeChildFromResource( generalAdminInp, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlDelZone( rsComm, generalAdminInp->arg2 );
            if ( status == 0 ) {
                memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
                snprintf( collInfo.collName, sizeof( collInfo.collName ), "/%s", generalAdminInp->arg2 );
                status = chlDelCollByAdmin( rsComm, &collInfo );
            }
            if ( status == 0 ) {
                status = chlCommit( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {

            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            argc = 2;
            i =  applyRuleArg( "acPreProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForDeleteToken error for %s.%s,stat=%d",
                         args[0], args[1], i );
                return i;
            }

            status = chlDelToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3 );

            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin: acPostProcForDeleteToken error for %s.%s, stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }

            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "unusedAVUs" ) == 0 ) {
            status = chlDelUnusedAVUs( rsComm );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "specificQuery" ) == 0 ) {
            status = chlDelSpecificQuery( rsComm, generalAdminInp->arg2 );
            return status;
        }
    }
    if ( strcmp( generalAdminInp->arg0, "calculate-usage" ) == 0 ) {
        status = chlCalcUsageAndQuota( rsComm );
        return status;
    }
    if ( strcmp( generalAdminInp->arg0, "set-quota" ) == 0 ) {
        status = chlSetQuota( rsComm,
                              generalAdminInp->arg1,
                              generalAdminInp->arg2,
                              generalAdminInp->arg3,
                              generalAdminInp->arg4 );

        return status;
    }

    if ( strcmp( generalAdminInp->arg0, "lt" ) == 0 ) {
        status = CAT_INVALID_ARGUMENT;
        if ( strcmp( generalAdminInp->arg1, "resc_type" ) == 0 ) {
            status = _listRescTypes( rsComm );
        }
        return status;
    }

    return CAT_INVALID_ARGUMENT;
}
