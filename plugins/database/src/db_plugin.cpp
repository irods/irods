#include "irods/authenticate.h"
#include "irods/catalog.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/checksum.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/icatStructs.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_auth_factory.hpp"
#include "irods/irods_auth_manager.hpp"
#include "irods/irods_auth_object.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/irods_children_parser.hpp"
#include "irods/irods_database_constants.hpp"
#include "irods/irods_database_plugin.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_lexical_cast.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_pam_auth_object.hpp"
#include "irods/irods_postgres_object.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_resource_manager.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_virtual_path.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/modAccessControl.h"
#include "irods/msParam.h"
#include "irods/private/irods_catalog_properties.hpp"
#include "irods/private/irods_sql_logger.hpp"
#include "irods/private/low_level.hpp"
#include "irods/private/mid_level.hpp"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rods.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsQuota.h"
#include "irods/user_validation_utilities.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <nanodbc/nanodbc.h>
#include <nlohmann/json.hpp>

#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// clang-format off
using log_db        = irods::experimental::log::database;
using log_sql       = irods::experimental::log::sql;
using leaf_bundle_t = irods::resource_manager::leaf_bundle_t;
// clang-format on

extern irods::resource_manager resc_mgr;

extern int get64RandomBytes( char *buf );
extern int icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 );

static char prevChalSig[200]; // A 'signature' of the previous challenge.
                              // This is used as a sessionSignature on the catalog provider server
                              // side. Also see getSessionSignatureClientside function. */

// Legal values for accessLevel in chlModAccessControl (Access Parameter).
// Defined here since other code does not need them (except for help messages)
#define AP_READ  "read"
#define AP_WRITE "write"
#define AP_OWN   "own"
#define AP_NULL  "null"

static rodsLong_t MAX_PASSWORDS = 40;
/* TEMP_PASSWORD_TIME is the number of seconds the temporary, one-time
   password can be used.  chlCheckAuth also checks for this column
   to be < TEMP_PASSWORD_MAX_TIME (1000) to differentiate the row
   from regular passwords and PAM passwords.
   This time, 120 seconds, should be long enough to give the iDrop and
   iDrop-lite applets enough time to download and go through their
   startup sequence.  iDrop and iDrop-lite disconnect when idle to
   reduce the number of open connections and active agents.  */

#define PASSWORD_SCRAMBLE_PREFIX ".E_"
#define PASSWORD_KEY_ENV_VAR     "IRODS_DATABASE_USER_PASSWORD_SALT"
#define PASSWORD_DEFAULT_KEY     "a9_3fker"

#define MAX_HOST_STR             2700

size_t log_sql_flg = 0;
icatSessionStruct icss; // JMC :: only for testing!!!
extern int logSQL;

int  creatingUserByGroupAdmin; // JMC - backport 4772
char mySessionTicket[NAME_LEN];
char mySessionClientAddr[NAME_LEN];

// =-=-=-=-=-=-=-
// property constants
const std::string ICSS_PROP( "irods_icss_property" );
const std::string ZONE_PROP( "irods_zone_property" );

static const auto intermediate_replica_status_str = std::to_string(INTERMEDIATE_REPLICA);

namespace
{
    // This structure holds authentication configuration values.
    struct auth_config
    {
        static constexpr bool default_password_extend_lifetime = true;
        static constexpr rodsLong_t default_password_max_time = 1209600;
        static constexpr rodsLong_t default_password_min_time = 121;

        // Holds the value for the irods::KW_CFG_PAM_PASSWORD_EXTEND_LIFETIME configuration.
        bool password_extend_lifetime = default_password_extend_lifetime;

        // Holds the value for the irods::KW_CFG_PAM_PASSWORD_MAX_TIME configuration.
        rodsLong_t password_max_time = default_password_max_time;

        // Holds the value for the irods::KW_CFG_PAM_PASSWORD_MIN_TIME configuration.
        rodsLong_t password_min_time = default_password_min_time;
    };

    auto get_auth_config(const char* _namespace, auth_config& _out) -> irods::error
    {
        try {
            auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();
            nanodbc::statement stmt{db_conn};

            nanodbc::prepare(stmt, "select option_name, option_value from R_GRID_CONFIGURATION where namespace = ?");
            stmt.bind(0, _namespace);

            for (auto result = nanodbc::execute(stmt); result.next();) {
                const auto option_name = result.get<std::string>(0);
                const auto option_value = result.get<std::string>(1);

                // Given the level of nesting that occurs here to maintain specificity in the error messages, the logic
                // has been condensed for reuse. The option_name still needs to be differentiated to apply the correct
                // setting, and given that there are only two alternatives for this case, it is sufficient to use only
                // one for a boolean flag. Therefore, check for the password_min_time option_name once to determine
                // which option_name this is and avoid duplicating the string comparison.
                if (const bool is_min_option = (option_name == irods::KW_CFG_PAM_PASSWORD_MIN_TIME);
                    is_min_option || option_name == irods::KW_CFG_PAM_PASSWORD_MAX_TIME)
                {
                    const auto default_value =
                        is_min_option ? auth_config::default_password_min_time : auth_config::default_password_max_time;
                    auto& config = is_min_option ? _out.password_min_time : _out.password_max_time;
                    try {
                        config = std::stoll(option_value);
                        if (config < 0) {
                            config = default_value;
                            log_db::warn("Invalid R_GRID_CONFIGURATION value. namespace:[{}], option:[{}], value:[{}]. "
                                         "Using default value [{}].",
                                         _namespace,
                                         option_name,
                                         option_value,
                                         config);
                        }
                    }
                    catch (const std::exception& e) {
                        config = default_value;
                        log_db::warn("Error occurred getting R_GRID_CONFIGURATION value. namespace:[{}], option:[{}], "
                                     "value:[{}]. Using default value [{}]. error:[{}]",
                                     _namespace,
                                     option_name,
                                     option_value,
                                     config,
                                     e.what());
                    }
                    catch (...) {
                        config = default_value;
                        log_db::warn("Error occurred getting R_GRID_CONFIGURATION value. namespace:[{}], option:[{}], "
                                     "value:[{}]. Using default value [{}]. error:[Unknown error]",
                                     _namespace,
                                     option_name,
                                     option_value,
                                     config);
                    }
                    continue;
                }

                if (option_name == irods::KW_CFG_PAM_PASSWORD_EXTEND_LIFETIME) {
                    if (option_value == "1") {
                        _out.password_extend_lifetime = true;
                    }
                    else if (option_value == "0") {
                        _out.password_extend_lifetime = false;
                    }
                    else {
                        _out.password_extend_lifetime = auth_config::default_password_extend_lifetime;
                        log_db::warn("Invalid R_GRID_CONFIGURATION value. namespace:[{}], option:[{}], value:[{}]. "
                                     "Using default value [{}].",
                                     _namespace,
                                     option_name,
                                     option_value,
                                     _out.password_extend_lifetime);
                    }
                    continue;
                }
            }
        }
        catch (const std::exception& e) {
            return ERROR(
                SYS_LIBRARY_ERROR,
                fmt::format(
                    "Error occurred getting grid configurations from R_GRID_CONFIGURATION namespace [{}]. error:[{}]",
                    _namespace,
                    e.what()));
        }
        catch (...) {
            return ERROR(
                SYS_UNKNOWN_ERROR,
                fmt::format(
                    "Unknown error occurred getting grid configurations from R_GRID_CONFIGURATION namespace [{}].",
                    _namespace));
        }

        return SUCCESS();
    } // get_auth_config
} // anonymous namespace

// =-=-=-=-=-=-=-
// virtual path management
#define PATH_SEPARATOR irods::get_virtual_path_separator().c_str()

// Returns the current time as a pair of strings.
//
// The first string represents the number of seconds since the epoch. It will be left-padded
// with zeros and have a minimum width of 11 characters.
//
// The second string represents the fractional part of a second in milliseconds. It will be
// left-padded with zeros and have an exact width of 3 characters.
auto get_current_time() -> std::pair<std::string, std::string>
{
    using std::chrono::duration_cast;

    const auto now = std::chrono::system_clock::now();
    const auto secs = duration_cast<std::chrono::seconds>(now.time_since_epoch());
    const auto millis = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) - secs;

    return {fmt::format("{:011}", secs.count()), fmt::format("{:03}", millis.count())};
} // get_current_time

/*
   Parse the input fullUserNameIn into an output userName and userZone
   and check that the username is a valid format, meaning at most one
   '@' and at most one '#'.
   Full userNames are of the form user@department[#zone].
   It is assumed the output strings are at least NAME_LEN characters long.
 */
int
validateAndParseUserName( const char *fullUserNameIn, char *userName, char *userZone ) {
    const auto user_and_zone_name = irods::user::validate_name(fullUserNameIn);
    if (!user_and_zone_name) {
        if (userName) {
            userName[0] = '\0';
        }

        if (userZone) {
            userZone[0] = '\0';
        }

        return USER_INVALID_USERNAME_FORMAT;
    }

    const auto& [user_name, zone_name] = *user_and_zone_name;

    if (userName) {
        std::strncpy(userName, user_name.data(), NAME_LEN);
    }

    if (userZone) {
        std::strncpy(userZone, zone_name.data(), NAME_LEN);
    }

    return 0;
} // validateAndParseUserName

[[nodiscard]] auto is_valid_avu(const char* _attribute, const char* _value, const char* _unit) noexcept -> bool
{
    if (_attribute == nullptr || _value == nullptr || _unit == nullptr) {
        return false;
    }
    if (*_attribute == '\0' || *_value == '\0') {
        return false;
    }

    return true;
}

// =-=-=-=-=-=-=-
// helper fcn to handle cast to pg object
irods::error make_db_ptr(
    const irods::first_class_object_ptr& _fc,
    irods::postgres_object_ptr&          _pg ) {
    if ( !_fc.get() ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "incoming fco is null" );

    }

    _pg = boost::dynamic_pointer_cast <
          irods::postgres_object > (
              _fc );

    if ( _pg.get() ) {
        return SUCCESS();

    }
    else {
        return ERROR(
                   INVALID_DYNAMIC_CAST,
                   "failed to dynamic cast to postgres_object_ptr" );
    }

} // make_db_ptr

// =-=-=-=-=-=-=-
//  Called internally to rollback current transaction after an error.
int _rollback( const char *functionName ) {
    // =-=-=-=-=-=-=-
    // This type of rollback is needed for Postgres since the low-level
    // now does an automatic 'begin' to create a sql block */
    int status =  cmlExecuteNoAnswerSql( "rollback", &icss );
    if ( status == 0 ) {
        log_db::info("{} cmlExecuteNoAnswerSql(rollback) succeeded", functionName);
    }
    else {
        log_db::info("{} cmlExecuteNoAnswerSql(rollback) failure {}", functionName, status);
    }

    return status;

} // _rollback

// =-=-=-=-=-=-=-
//  Internal function to return the local zone (which is the default
//  zone).  The first time it's called, it gets the zone from the DB and
//  subsequent calls just return that value.
irods::error getLocalZone(
    irods::plugin_property_map& _prop_map,
    icatSessionStruct*          _icss,
    std::string&                _zone ) {
    // =-=-=-=-=-=-=-
    // try to get the zone prop, if it is not cached
    // then we hit the catalog and request it
    irods::error ret = _prop_map.get< std::string >( ZONE_PROP, _zone );
    if ( !ret.ok() ) {
        char local_zone[ MAX_NAME_LEN ];
        int status;
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( "local" );
            status = cmlGetStringValueFromSql(
                         ( char* )"select zone_name from R_ZONE_MAIN where zone_type_name=?",
                         local_zone, MAX_NAME_LEN, bindVars, _icss );
        }
        if ( status != 0 ) {
            _rollback( "getLocalZone" );
            return ERROR( status, "getLocalZone failure" );
        }

        // =-=-=-=-=-=-=-
        // set the zone property
        _zone = local_zone;
        ret = _prop_map.set< std::string >( ZONE_PROP, _zone );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

    } // if no zone prop

    return SUCCESS();

} // getLocalZone

// =-=-=-=-=-=-=-
// @brief query for object found of a resource
int get_object_count_of_resource_by_name(
    icatSessionStruct* _icss,
    const std::string& _resc_name,
    rodsLong_t&         _count ) {

    rodsLong_t resc_id;
    irods::error ret = resc_mgr.hier_to_leaf_id(
                         _resc_name,
                         resc_id);
    if(!ret.ok()) {
        // if we have a bad resource in the database we need
        // to ignore this in order to still remove it
        if(SYS_RESC_DOES_NOT_EXIST == ret.code()) {
            _count = 0;
            return 0;
        }
        irods::log(PASS(ret));
        return ret.code();
    }

    std::string resc_id_str;
    ret = irods::lexical_cast<std::string>(resc_id, resc_id_str);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( resc_id_str );
    int status = cmlGetIntegerValueFromSql(
                     ( char* )"select count(data_id) from R_DATA_MAIN where resc_id=?",
                     &_count,
                     bindVars,
                     _icss );

    return status;

} // get_object_count_of_resource_by_name

// =-=-=-=-=-=-=-
// @brief determine if user had write permission to data object
irods::error determine_user_has_modify_metadata_access(
    const std::string& _data_name,
    const std::string& _collection,
    const std::string& _user_name,
    const std::string& _zone)
{
    int status = 0;

    log_db::debug(
        "{} :: [{}] [{}] [{}] [{}]",
        __FUNCTION__,
        _data_name.c_str(),
        _collection.c_str(),
        _user_name.c_str(),
        _zone.c_str());

    // get the number of data object to which this will apply
    rodsLong_t num_data_objects = -1;
    {
        std::vector<std::string> bind_vars;
        bind_vars.reserve(2);
        bind_vars.push_back( _data_name );
        bind_vars.push_back( _collection );
        status = cmlGetIntegerValueFromSql(
             "select count(DISTINCT DM.data_id) "
             "from R_DATA_MAIN DM, R_COLL_MAIN CM "
             "where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ?",
              &num_data_objects,
              bind_vars,
              &icss );
        if( 0 != status ) {
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "failed to get object count" );
        }

        if( 0 == num_data_objects ) {
            std::string msg = "no data objects found for collection ";
            msg += _collection;
            msg += " and object name ";
            msg += _data_name;
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( CAT_NO_ROWS_FOUND, msg );
        }
    }

    // This is the minimum permission level required of the user.
    namespace ic = irods::experimental::catalog;
    using integral_access_type = std::underlying_type_t<ic::access_type>;
    const rodsLong_t access_needed = static_cast<integral_access_type>(ic::access_type::modify_metadata);

    // Get the minimum permission level across all data objects.
    // The user is allowed to modify the metadata of all matched data objects if and only if
    // the minimum permission level found among the list of data objects is greater than or
    // equal to required permission level (i.e. "modify_metadata").
    rodsLong_t access_permission = -1;
    {
        const char* const query =
            "select min(max_access_type_id) from ( "
                "select max(access_type_id) max_access_type_id from ( "
                    "select access_type_id, DM.data_id "
                    "from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM "
                    "where DM.data_name like ? and "
                          "DM.coll_id = CM.coll_id and "
                          "CM.coll_name like ? and "
                          "UM.user_name = ? and "
                          "UM.zone_name = ? and "
                          "UM.user_type_name != 'rodsgroup' and "
                          "UM.user_id = UG.user_id and "
                          "OA.object_id = DM.data_id and "
                          "UG.group_user_id = OA.user_id "
#if ORA_ICAT
                ") "
                "group by data_id "
            ")";
#else
                ") as foo "
                "group by data_id "
            ") as bar";
#endif
        std::vector<std::string> bind_vars;
        bind_vars.reserve(4);
        bind_vars.push_back( _data_name.c_str() );
        bind_vars.push_back( _collection.c_str() );
        bind_vars.push_back( _user_name.c_str() );
        bind_vars.push_back( _zone.c_str() );
        status = cmlGetIntegerValueFromSql( query, &access_permission, bind_vars, &icss );
        if ( status == CAT_NO_ROWS_FOUND ) {
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( CAT_NO_ACCESS_PERMISSION, "access denied" );
        }

        if ( access_permission < access_needed ) {
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( CAT_NO_ACCESS_PERMISSION, "access denied" );
        }
    }

    // reproduce the count of access permission entries in "ACCESS_VIEW_TWO"
    rodsLong_t access_permission_count = -1;
    {
        const char* const query =
            "select count(max) from ( "
                "select max(access_type_id) max from ( "
                    "select access_type_id, DM.data_id "
                    "from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM "
                    "where DM.data_name like ? and "
                          "DM.coll_id = CM.coll_id and "
                          "CM.coll_name like ? and "
                          "UM.user_name = ? and "
                          "UM.zone_name = ? and "
                          "UM.user_type_name != 'rodsgroup' and "
                          "UM.user_id = UG.user_id and "
                          "OA.object_id = DM.data_id and "
                          "UG.group_user_id = OA.user_id "
#if ORA_ICAT
                ") "
                "group by data_id "
            ")";
#else
                ") as foo "
                "group by data_id "
            ") as baz";
#endif
        std::vector<std::string> bind_vars;
        bind_vars.reserve(4);
        bind_vars.push_back( _data_name.c_str() );
        bind_vars.push_back( _collection.c_str() );
        bind_vars.push_back( _user_name.c_str() );
        bind_vars.push_back( _zone.c_str() );
        status = cmlGetIntegerValueFromSql( query, &access_permission_count, bind_vars, &icss );
        if( 0 != status ) {
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "query for access permission count failed" );
        }

        if( num_data_objects > access_permission_count ) {
            const auto msg = fmt::format("access denied - num_data_objects [{}] > access_permission_count [{}]",
                                         num_data_objects, access_permission_count);
            _rollback("chlAddAVUMetadataWild");
            return ERROR(CAT_NO_ACCESS_PERMISSION, msg);
        }
    }

    // return number of data objects, keeping the semantics of the
    // original imeta addw operation.
    return CODE( num_data_objects );
} // determine_user_has_modify_metadata_access


// remove AVU (user defined metadata) for an object, the metadata mapping information, if any.
int removeMetaMapAndAVU(const char* _id)
{
    char tSQL[MAX_SQL_SIZE]{};
    cllBindVars[0] = _id;
    cllBindVarCount = 1;

    if (logSQL) {
        log_sql::debug("removeMetaMapAndAVU SQL 1 ");
    }

    snprintf(tSQL, MAX_SQL_SIZE, "delete from R_OBJT_METAMAP where object_id=?");

    const auto ec = cmlExecuteNoAnswerSql(tSQL, &icss);

    return ec < 0 && CAT_SUCCESS_BUT_WITH_NO_INFO != ec ? ec : 0;
} // removeMetaMapAndAVU

/*
 * removeAVUs - remove unused AVUs (user defined metadata), if any.
 */
static int removeAVUs() {
    char tSQL[MAX_SQL_SIZE];

    if ( logSQL != 0 ) {
        log_sql::debug("removeAVUs SQL 1 ");
    }
    cllBindVarCount = 0;

#if ORA_ICAT
    snprintf( tSQL, MAX_SQL_SIZE,
              "delete from R_META_MAIN where meta_id in (select meta_id from R_META_MAIN minus select meta_id from R_OBJT_METAMAP)" );
#elif MY_ICAT
    /* MYSQL does not have 'minus' or 'except' (to my knowledge) so
     * use previous version of the SQL, which is very slow */
    snprintf( tSQL, MAX_SQL_SIZE,
              "delete from R_META_MAIN where meta_id not in (select meta_id from R_OBJT_METAMAP)" );
#else
    /* Postgres */
    snprintf( tSQL, MAX_SQL_SIZE,
              "delete from R_META_MAIN where meta_id in (select meta_id from R_META_MAIN except select meta_id from R_OBJT_METAMAP)" );
#endif
    const int status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    log_db::debug("removeAVUs status={}\n", status);

    return status;
}

int
_canConnectToCatalog(
    rsComm_t* _rsComm ) {
    int result = 0;
    if ( !icss.status ) {
        result = CATALOG_NOT_CONNECTED;
    }
    else if ( _rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        result = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    else if ( _rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        result = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    return result;
}

static
int hostname_resolves_to_ipv4(const char* _hostname) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    struct addrinfo *p_addrinfo;
    const int ret_getaddrinfo_with_retry = getaddrinfo_with_retry(_hostname, 0, &hint, &p_addrinfo);
    if (ret_getaddrinfo_with_retry) {
        return ret_getaddrinfo_with_retry;
    }
    freeaddrinfo(p_addrinfo);
    return 0;
}


int
_resolveHostName(rsComm_t* _rsComm, const char* _hostAddress) {
    const int status = hostname_resolves_to_ipv4(_hostAddress);

    if ( status != 0 ) {
        char errMsg[155];
        snprintf( errMsg, 150,
                  "Warning, resource host address '%s' is not a valid DNS entry, hostname_resolves_to_ipv4 failed.",
                  _hostAddress );
        addRErrorMsg( &_rsComm->rError, 0, errMsg );
    }
    if ( strcmp( _hostAddress, "localhost" ) == 0 ) {
        addRErrorMsg( &_rsComm->rError, 0,
                      "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client." );
    }

    return 0;
}

// Returns success if path is not root; otherwise, generates an error
irods::error
verify_non_root_vault_path(irods::plugin_context& _ctx, const std::string& path) {
    if (0 == path.compare("/")) {
        const std::string error_message = "root directory cannot be used as vault path.";
        addRErrorMsg(&_ctx.comm()->rError, 0, error_message.c_str());
        return ERROR(CAT_INVALID_RESOURCE_VAULT_PATH, error_message.c_str() );
    }
    return SUCCESS();
}

// =-=-=-=-=-=-=-
//
irods::error _childIsValid(
    irods::plugin_property_map& _prop_map,
    const std::string&          _new_child ) {
    // =-=-=-=-=-=-=-
    // Lookup the child resource and make sure its parent field is empty
    char parent[MAX_NAME_LEN];
    int status;

    // Get the resource name from the child string
    std::string resc_name;
    irods::children_parser parser;
    parser.set_string( _new_child );
    parser.first_child( resc_name );

    std::string zone;
    irods::error ret = getLocalZone( _prop_map, &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // Get resource's parent
    irods::sql_logger logger( "_childIsValid", logSQL );
    logger.log();
    parent[0] = '\0';
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( resc_name );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
                     parent, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            std::stringstream ss;
            ss << "Child resource \"" << resc_name << "\" not found";
            irods::log( LOG_NOTICE, ss.str() );
            return ERROR( CHILD_NOT_FOUND, "child resource not found" );
        }
        else {
            _rollback( "_childIsValid" );
            return ERROR( status, "error encountered in query for _childIsValid" );
        }
    }
    else if ( strlen( parent ) != 0 ) {
        // If the resource already has a parent it cannot be added as a child of another one
        std::stringstream ss;
        ss << "Child resource \"" << resc_name << "\" already has a parent \"" << parent << "\"";
        irods::log( LOG_NOTICE, ss.str() );
        return ERROR( CHILD_HAS_PARENT, "child resource already has a parent" );
    }
    return SUCCESS();
}

irods::error update_child_parent(const std::string& _child_resc_id,
                                 const std::string& _parent_resc_id,
                                 const std::string& _parent_child_context)
{
    irods::sql_logger logger("update_child_parent", logSQL);

    const auto [current_time_secs, current_time_msecs] = get_current_time();

    // Update the parent for the child resource
    // have to do this to get around const
    cllBindVarCount = 0;
    cllBindVars[cllBindVarCount++] = _parent_resc_id.c_str();
    cllBindVars[cllBindVarCount++] = _parent_child_context.c_str();
    cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
    cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
    cllBindVars[cllBindVarCount++] = _child_resc_id.c_str();
    logger.log();

    int status = cmlExecuteNoAnswerSql(
        "update R_RESC_MAIN set resc_parent=?, resc_parent_context=?, modify_ts=?, modify_ts_millis=? "
        "where resc_id=?",
        &icss);
    if( status != 0 ) {
        _rollback("update_child_parent");
        return ERROR( status, "cmlExecuteNoAnswerSql failed" );
    }

    return SUCCESS();

} // update_child_parent

/**
 * @brief Returns true if the specified resource has associated data objects
 */
int
_rescHasData(
    icatSessionStruct* _icss,
    const std::string& _resc_name,
    bool&              _has_data ) {
    irods::sql_logger logger( "_rescHasData", logSQL );
    rodsLong_t obj_count{};

    logger.log();

    int status = get_object_count_of_resource_by_name(
                      _icss,
                      _resc_name,
                      obj_count );
    if( 0 == status ) {
        if ( 0 == obj_count ) {
            _has_data = false;
        }
    }

    return status;
}

/// @brief function for validating a resource name
irods::error validate_resource_name( std::string _resc_name ) {

    // Must be between 1 and NAME_LEN-1 characters.
    // Must start and end with a word character.
    // May contain non consecutive dashes.
    boost::regex re( "^(?=.{1,63}$)\\w+(-\\w+)*$" );

    if ( !boost::regex_match( _resc_name, re ) ) {
        std::stringstream msg;
        msg << "validate_resource_name failed for resource [";
        msg << _resc_name;
        msg << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    return SUCCESS();

} // validate_resource_name

// Not super robust
static inline auto validate_zone_connection_string(const char* _zone_conn_info, irods::plugin_context& _ctx)
    -> irods::error
{
    const std::size_t addr_len = std::strlen(_zone_conn_info);
    if (addr_len > 0) {
        // _zone_conn_info is const, so copy it
        std::vector<char> addr_buf(addr_len + 1, '\0');
        std::strncpy(addr_buf.data(), _zone_conn_info, addr_buf.size());
        rodsHostAddr_t addr{};
        auto status = parseHostAddrStr(addr_buf.data(), &addr);
        if (status < 0) {
            std::string errmsg = fmt::format("failed to validate zone connection info [{}]", status);
            log_db::error(errmsg);
            addRErrorMsg(&_ctx.comm()->rError, status, errmsg.c_str());
            return ERROR(status, errmsg);
        }
        const std::size_t host_len = std::strlen(addr.hostAddr);
        if (host_len == 0 || (host_len == 1 && addr.hostAddr[0] == ':')) {
            std::string errmsg =
                fmt::format("failed to validate zone connection info due to empty hostname [{}]", _zone_conn_info);
            log_db::error(errmsg);
            addRErrorMsg(&_ctx.comm()->rError, CAT_HOSTNAME_INVALID, errmsg.c_str());
            return ERROR(CAT_HOSTNAME_INVALID, errmsg);
        }
        if ((addr.portNum > 65535) || (addr.portNum <= 0)) {
            std::string errmsg = fmt::format(
                "failed to validate zone connection info due to invalid or unspecified port [{}]", _zone_conn_info);
            log_db::error(errmsg);
            addRErrorMsg(&_ctx.comm()->rError, CAT_INVALID_ARGUMENT, errmsg.c_str());
            return ERROR(CAT_INVALID_ARGUMENT, errmsg);
        }
    }
    return SUCCESS();
}

bool
_rescHasParentOrChild( char* rescId ) {

    char parent[MAX_NAME_LEN];
    char children[MAX_NAME_LEN];
    int status;
    irods::sql_logger logger( "_rescHasParentOrChild", logSQL );

    logger.log();
    parent[0] = '\0';
    children[0] = '\0';
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( rescId );
        status = cmlGetStringValueFromSql(
                     "select resc_parent from R_RESC_MAIN where resc_id=?",
                     parent, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            std::stringstream ss;
            ss << "Resource \"" << rescId << "\" not found";
            irods::log( LOG_NOTICE, ss.str() );
        }
        else {
            _rollback( "_rescHasParentOrChild" );
        }
        return false;
    }
    if ( strlen( parent ) != 0 ) {
        return true;
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( rescId );
        status = cmlGetStringValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_parent=?",
                     children, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            _rollback( "_rescHasParentOrChild" );
        }
        return false;
    }
    if ( strlen( children ) != 0 ) {
        return true;
    }
    return false;

}

bool _userInRUserAuth( const char* userName, const char* zoneName, const char* auth_name ) {
    int status;
    rodsLong_t iVal;
    irods::sql_logger logger( "_userInRUserAuth", logSQL );

    logger.log();
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName );
        bindVars.push_back( zoneName );
        bindVars.push_back( auth_name );
        status = cmlGetIntegerValueFromSql(
                    "select user_id from R_USER_AUTH where user_id=(select user_id from R_USER_MAIN where user_name=? and zone_name=?) and user_auth_name=?",
                    &iVal, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            _rollback( "_userInRUserAuth" );
        }
        return false;
    } else {
        return true;
    }
}

// =-=-=-=-=-=-=-
/// @brief function which determines if a char is allowed in a zone name
static bool allowed_zone_char( const char _c ) {
    return ( !std::isalnum( _c ) &&
             !( '.' == _c )      &&
             !( '_' == _c ) );
} // allowed_zone_char

// =-=-=-=-=-=-=-
/// @brief function for validating the name of a zone
irods::error validate_zone_name(
    std::string _zone_name ) {
    std::string::iterator itr = std::find_if( _zone_name.begin(),
                                _zone_name.end(),
                                allowed_zone_char );
    if ( itr != _zone_name.end() || _zone_name.length() >= NAME_LEN ) {
        std::stringstream msg;
        msg << "validate_zone_name failed for zone [";
        msg << _zone_name;
        msg << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    return SUCCESS();

} // validate_zone_name

/* delCollection (internally called),
   does not do the commit.
*/
static int _delColl( rsComm_t *rsComm, collInfo_t *collInfo ) {
    rodsLong_t iVal;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    char collIdNum[MAX_NAME_LEN];
    char parentCollIdNum[MAX_NAME_LEN];
    rodsLong_t status;

    if ( logSQL != 0 ) {
        log_sql::debug("_delColl");
    }

    if ( !icss.status ) {
        return CATALOG_NOT_CONNECTED;
    }

    if (const auto ec = splitPathByKey(collInfo->collName, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
        irods::log(LOG_ERROR, fmt::format(
                   "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                   __func__, __LINE__, collInfo->collName, ec));
        return ec;
    }

    if ( strlen( logicalParentDirName ) == 0 ) {
        snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
        snprintf( logicalEndName, sizeof( logicalEndName ), "%s", collInfo->collName + 1 );
    }

    /* Check that the parent collection exists and user has write permission,
       and get the collectionID */
    if ( logSQL != 0 ) {
        log_sql::debug("_delColl SQL 1 ");
    }
    status = cmlCheckDir( logicalParentDirName,
                          rsComm->clientUser.userName,
                          rsComm->clientUser.rodsZone,
                          ACCESS_MODIFY_OBJECT,
                          &icss );
    if ( status < 0 ) {
        char errMsg[105];
        if ( status == CAT_UNKNOWN_COLLECTION ) {
            snprintf( errMsg, 100, "collection '%s' is unknown",
                      logicalParentDirName );
            addRErrorMsg( &rsComm->rError, 0, errMsg );
            return status;
        }
        _rollback( "_delColl" );
        return status;
    }
    snprintf( parentCollIdNum, MAX_NAME_LEN, "%lld", status );

    /* Check that the collection exists and user has DELETE or better
       permission */
    if ( logSQL != 0 ) {
        log_sql::debug("_delColl SQL 2");
    }
    status = cmlCheckDir( collInfo->collName,
                          rsComm->clientUser.userName,
                          rsComm->clientUser.rodsZone,
                          ACCESS_DELETE_OBJECT,
                          &icss );
    if ( status < 0 ) {
        return status;
    }
    snprintf( collIdNum, MAX_NAME_LEN, "%lld", status );

    /* check that the collection is empty (both subdirs and files) */
    if ( logSQL != 0 ) {
        log_sql::debug("_delColl SQL 3");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( collInfo->collName );
        bindVars.push_back( collInfo->collName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where parent_coll_name=? union select coll_id from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
                     &iVal, bindVars, &icss );
    }
    if ( status != CAT_NO_ROWS_FOUND ) {
        return CAT_COLLECTION_NOT_EMPTY;
    }

    /* delete the row if it exists */
    /* The use of coll_id isn't really needed but may add a little safety.
       Previously, we included a check that it was owned by the user but
       the above cmlCheckDir is more accurate (handles group access). */
    cllBindVars[cllBindVarCount++] = collInfo->collName;
    cllBindVars[cllBindVarCount++] = collIdNum;
    if ( logSQL != 0 ) {
        log_sql::debug("_delColl SQL 4");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_COLL_MAIN where coll_name=? and coll_id=?",
                  &icss );
    if ( status != 0 ) { /* error, odd one as everything checked above */
        log_db::info("_delColl cmlExecuteNoAnswerSql delete failure {}", status);
        _rollback( "_delColl" );
        return status;
    }

    /* remove any access rows */
    cllBindVars[cllBindVarCount++] = collIdNum;
    if ( logSQL != 0 ) {
        log_sql::debug("_delColl SQL 5");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_OBJT_ACCESS where object_id=?",
                  &icss );
    if ( status != 0 ) { /* error, odd one as everything checked above */
        log_db::info("_delColl cmlExecuteNoAnswerSql delete access failure {}", status);
        _rollback( "_delColl" );
    }

    /* Remove associated AVUs, if any */
    if (const auto ec = removeMetaMapAndAVU(collIdNum); ec < 0) {
        irods::log(LOG_WARNING, fmt::format(
                   "[{}:{}] - failed to remove associated AVUs [ec=[{}]]",
                   __func__, __LINE__, ec));
    }

    return status;

} // _delColl

// The following is an artifact of the legacy authentication plugins. This operation is
// only useful for certain plugins which are not supported in 4.3.0, so it is being
// left out of compilation for now. Once we have determined that this is safe to do in
// general, this section can be removed.
#if 0
// =-=-=-=-=-=-=-
// local function to delegate the response
// verification to an authentication plugin
static
irods::error verify_auth_response(
    const char* _scheme,
    const char* _challenge,
    const char* _user_name,
    const char* _response ) {
    // =-=-=-=-=-=-=-
    // validate incoming parameters
    if ( !_scheme ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null _scheme ptr" );
    }
    else if ( !_challenge ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null _challenge ptr" );
    }
    else if ( !_user_name ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null _user_name ptr" );
    }
    else if ( !_response ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null _response ptr" );
    }

    // TODO: Is this an implicit dependence on the old auth plugins?

    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    irods::auth_object_ptr auth_obj;
    irods::error ret = irods::auth_factory( _scheme, 0, auth_obj );
    if ( !ret.ok() ) {
        return ret;
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
    if ( !ret.ok() ) {
        return ret;
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call auth verify on plugin
    ret = auth_plugin->call <const char*, const char*, const char* > ( 0, irods::AUTH_AGENT_AUTH_VERIFY, auth_obj, _challenge, _user_name, _response );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret;
    }

    return SUCCESS();

} // verify_auth_response
#endif

/*
   Possibly descramble a password (for user passwords stored in the ICAT).
   Called internally, from various chl functions.
*/
static int
icatDescramble( char *pw ) {
    char *cp1, *cp2, *cp3;
    int i, len;
    char pw2[MAX_PASSWORD_LEN + 10];
    char unscrambled[MAX_PASSWORD_LEN + 10];

    len = strlen( PASSWORD_SCRAMBLE_PREFIX );
    cp1 = pw;
    cp2 = PASSWORD_SCRAMBLE_PREFIX; /* if starts with this, it is scrambled */
    for ( i = 0; i < len; i++ ) {
        if ( *cp1++ != *cp2++ ) {
            return 0;                /* not scrambled, leave as is */
        }
    }
    snprintf( pw2, sizeof( pw2 ), "%s", cp1 );
    cp3 = getenv( PASSWORD_KEY_ENV_VAR );
    if ( cp3 == NULL ) {
        cp3 = PASSWORD_DEFAULT_KEY;
    }
    obfDecodeByKey( pw2, cp3, unscrambled );
    strncpy( pw, unscrambled, MAX_PASSWORD_LEN );

    return 0;
}

/*
   Scramble a password (for user passwords stored in the ICAT).
   Called internally.
*/
static int
icatScramble( char *pw ) {
    char *cp1;
    char newPw[MAX_PASSWORD_LEN + 10];
    char scrambled[MAX_PASSWORD_LEN + 10];

    cp1 = getenv( PASSWORD_KEY_ENV_VAR );
    if ( cp1 == NULL ) {
        cp1 = PASSWORD_DEFAULT_KEY;
    }
    obfEncodeByKey( pw, cp1, scrambled );
    snprintf( newPw, sizeof( newPw ), "%s%s", PASSWORD_SCRAMBLE_PREFIX, scrambled );
    strncpy( pw, newPw, MAX_PASSWORD_LEN );
    return 0;
}

/*
  de-scramble a password sent from the client.
  This isn't real encryption, but does obfuscate the pw on the network.
  Called internally, from chlModUser.
*/
int decodePw( rsComm_t *rsComm, const char *in, char *out ) {
    int status;
    char *cp;
    char password[MAX_PASSWORD_LEN];
    char upassword[MAX_PASSWORD_LEN + 10];
    char rand[] =
        "1gCBizHWbwIYyWLo";  /* must match clients */
    int pwLen1, pwLen2;

    if ( logSQL != 0 ) {
        log_sql::debug("decodePw - SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( rsComm->clientUser.userName );
        bindVars.push_back( rsComm->clientUser.rodsZone );
        status = cmlGetStringValueFromSql(
                     "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
                     password, MAX_PASSWORD_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback( "decodePw" );
        }
        return status;
    }

    icatDescramble( password );

    obfDecodeByKeyV2( in, password, prevChalSig, upassword );

    pwLen1 = strlen( upassword );

    memset( password, 0, MAX_PASSWORD_LEN );

    cp = strstr( upassword, rand );
    if ( cp != NULL ) {
        *cp = '\0';
    }

    pwLen2 = strlen( upassword );

    if ( pwLen2 > MAX_PASSWORD_LEN - 5 && pwLen2 == pwLen1 ) {
        /* probable failure */
        char errMsg[260];
        snprintf( errMsg, 250,
                  "Error with password encoding.  This can be caused by not connecting directly to the ICAT host, not using password authentication (using GSI or Kerberos instead), or entering your password incorrectly (if prompted)." );
        addRErrorMsg( &rsComm->rError, 0, errMsg );
        return CAT_PASSWORD_ENCODING_ERROR;
    }
    strcpy( out, upassword );
    memset( upassword, 0, MAX_PASSWORD_LEN );

    return 0;
}

int
convertTypeOption( const char *typeStr ) {
    if ( strcmp( typeStr, "-d" ) == 0 ) {
        return ( 1 );   /* dataObj */
    }
    if ( strcmp( typeStr, "-D" ) == 0 ) {
        return ( 1 );   /* dataObj */
    }
    if ( strcmp( typeStr, "-c" ) == 0 ) {
        return ( 2 );   /* collection */
    }
    if ( strcmp( typeStr, "-C" ) == 0 ) {
        return ( 2 );   /* collection */
    }
    if ( strcmp( typeStr, "-r" ) == 0 ) {
        return ( 3 );   /* resource */
    }
    if ( strcmp( typeStr, "-R" ) == 0 ) {
        return ( 3 );   /* resource */
    }
    if ( strcmp( typeStr, "-u" ) == 0 ) {
        return ( 4 );   /* user */
    }
    if ( strcmp( typeStr, "-U" ) == 0 ) {
        return ( 4 );   /* user */
    }
    return 0;
}

/*
  Check object - get an object's ID and check that the user has access.
  Called internally.
*/
rodsLong_t checkAndGetObjectId(
    rsComm_t*                   rsComm,
    irods::plugin_property_map& prop_map,
    const char*                 type,
    const char*                 name,
    const char*                 access,
    bool                        admin_mode = false)
{
    if (admin_mode && !irods::is_privileged_client(*rsComm)) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }

    int itype;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t status;
    rodsLong_t objId;
    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("checkAndGetObjectId");
    }

    if ( !icss.status ) {
        return CATALOG_NOT_CONNECTED;
    }

    if ( type == NULL ) {
        return CAT_INVALID_ARGUMENT;
    }

    if ( *type == '\0' ) {
        return CAT_INVALID_ARGUMENT;
    }


    if ( name == NULL ) {
        return CAT_INVALID_ARGUMENT;
    }

    if ( *name == '\0' ) {
        return CAT_INVALID_ARGUMENT;
    }


    itype = convertTypeOption( type );
    if ( itype == 0 ) {
        return CAT_INVALID_ARGUMENT;
    }

    if ( itype == 1 ) {
        if (const auto ec = splitPathByKey(name, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                       "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                       __func__, __LINE__, name, ec));
            return ec;
        }
        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", name );
        }
        if ( logSQL != 0 ) {
            log_sql::debug("checkAndGetObjectId SQL 1 ");
        }
        status = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                      rsComm->clientUser.userName,
                                      rsComm->clientUser.rodsZone,
                                      access, &icss, admin_mode );
        if ( status < 0 ) {
            _rollback( "checkAndGetObjectId" );
            return status;
        }
        objId = status;
    }

    if ( itype == 2 ) {
        /* Check that the collection exists and user has create_metadata permission,
           and get the collectionID */
        if ( logSQL != 0 ) {
            log_sql::debug("checkAndGetObjectId SQL 2");
        }
        status = cmlCheckDir( name,
                              rsComm->clientUser.userName,
                              rsComm->clientUser.rodsZone,
                              access, &icss, admin_mode );
        if ( status < 0 ) {
            char errMsg[105];
            if ( status == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown",
                          name );
                addRErrorMsg( &rsComm->rError, 0, errMsg );
            }
            return status;
        }
        objId = status;
    }

    if ( itype == 3 ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        std::string zone;
        irods::error ret = getLocalZone( prop_map, &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret ).code();
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("checkAndGetObjectId SQL 3");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( name );
            bindVars.push_back( zone );
            status = cmlGetIntegerValueFromSql(
                         "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return CAT_INVALID_RESOURCE;
            }
            _rollback( "checkAndGetObjectId" );
            return status;
        }
    }

    if ( itype == 4 ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        status = validateAndParseUserName( name, userName, userZone );
        if ( status ) {
            return status;
        }
        if ( userZone[0] == '\0' ) {
            std::string zone;
            irods::error ret = getLocalZone( prop_map, &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret ).code();
            }
            snprintf( userZone, sizeof( userZone ), "%s",  zone.c_str() );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("checkAndGetObjectId SQL 4");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName );
            bindVars.push_back( userZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return CAT_INVALID_USER;
            }
            _rollback( "checkAndGetObjectId" );
            return status;
        }
    }

    return objId;
}

/*
// =-=-=-=-=-=-=-
// JMC - backport 4836
+ Find existing AVU triplet.
+ Return code is error or the AVU ID.
+*/
rodsLong_t
findAVU( const char *attribute, const char *value, const char *units ) {
    rodsLong_t status;
// =-=-=-=-=-=-=-

    rodsLong_t iVal;
    iVal = 0;
    if ( *units != '\0' ) {
        if ( logSQL != 0 ) {
            log_sql::debug("findAVU SQL 1"); // JMC - backport 4836
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( attribute );
            bindVars.push_back( value );
            bindVars.push_back( units );
            status = cmlGetIntegerValueFromSql(
                         "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and meta_attr_unit=?",
                         &iVal, bindVars, &icss );
        }
    }
    else {
        if ( logSQL != 0 ) {
            log_sql::debug("findAVU SQL 2");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( attribute );
            bindVars.push_back( value );
            status = cmlGetIntegerValueFromSql(
                         "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and (meta_attr_unit='' or meta_attr_unit IS NULL)", // JMC - backport 4827
                         &iVal, bindVars, &icss );
        }
    }
    if ( status == 0 ) {
        status = iVal; /* use existing R_META_MAIN row */
        return status;
    }
// =-=-=-=-=-=-=-
// JMC - backport 4836
    return ( status ); // JMC - backport 4836
}

/*
  Find existing or insert a new AVU triplet.
  Return code is error, or the AVU ID.
*/
int
findOrInsertAVU( const char *attribute, const char *value, const char *units ) {
    char nextStr[MAX_NAME_LEN];
    char myTime[50];
    rodsLong_t status, seqNum;
    rodsLong_t iVal;
    iVal = findAVU( attribute, value, units );
    if ( iVal > 0 ) {
        return iVal;
    }
    if ( logSQL != 0 ) {
        log_sql::debug("findOrInsertAVU SQL 1");
    }
// =-=-=-=-=-=-=-
    status = cmlGetNextSeqVal( &icss );
    if ( status < 0 ) {
        log_db::info("findOrInsertAVU cmlGetNextSeqVal failure {}", status);
        return status;
    }
    seqNum = status; /* the returned status is the next sequence value */

    snprintf( nextStr, sizeof nextStr, "%lld", seqNum );

    getNowStr( myTime );

    cllBindVars[cllBindVarCount++] = nextStr;
    cllBindVars[cllBindVarCount++] = attribute;
    cllBindVars[cllBindVarCount++] = value;
    cllBindVars[cllBindVarCount++] = units;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    if ( logSQL != 0 ) {
        log_sql::debug("findOrInsertAVU SQL 2"); // JMC - backport 4836
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status < 0 ) {
        log_db::info("findOrInsertAVU insert failure {}", status);
        return status;
    }
    return seqNum;
}


/* create a path name with escaped SQL special characters (% and _) */
std::string
makeEscapedPath( const std::string &inPath ) {
    return boost::regex_replace( inPath, boost::regex( "[%_\\\\]" ), "\\\\$&" );
}

/* Internal routine to modify inheritance */
/* inheritFlag =1 to set, 2 to remove */
int _modInheritance( int inheritFlag, int recursiveFlag, const char *collIdStr, const char *pathName ) {

    const char* newValue = inheritFlag == 1 ? "1" : "0";

    char myTime[50];
    getNowStr( myTime );

    rodsLong_t status;
    /* non-Recursive mode */
    if ( recursiveFlag == 0 ) {

        if ( logSQL != 0 ) {
            log_sql::debug("_modInheritance SQL 1");
        }

        cllBindVars[cllBindVarCount++] = newValue;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = collIdStr;
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=?",
                      &icss );
    }
    else {
        /* Recursive mode */
        std::string pathStart = makeEscapedPath( pathName ) + "/%";

        cllBindVars[cllBindVarCount++] = newValue;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = pathName;
        cllBindVars[cllBindVarCount++] = pathStart.c_str();
        if ( logSQL != 0 ) {
            log_sql::debug("_modInheritance SQL 2");
        }
#ifdef ORA_ICAT
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_name = ? or coll_name like ? ESCAPE '\\'",
                      &icss );
#else
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_name = ? or coll_name like ?",
                      &icss );
#endif
    }
    if ( status != 0 ) {
        _rollback( "_modInheritance" );
        return status;
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return status;
}

/*
  Set the over_quota values (if any) using the limits and
  and the current usage; handling the various types: per-user per-resource,
  per-user total-usage, group per-resource, and group-total.

  The over_quota column is positive if over_quota and the negative value
  indicates how much space is left before reaching the quota.
*/
int setOverQuota( rsComm_t *rsComm ) {
    int status;
    int rowsFound;
    int statementNum = UNINITIALIZED_STATEMENT_NUMBER;
    char myTime[50];

    /* For each defined group limit (if any), get a total usage on that
     * resource for all users in that group: */
    char mySQL1[] = "select sum(quota_usage), UM1.user_id, R_QUOTA_USAGE.resc_id from R_QUOTA_USAGE, R_QUOTA_MAIN, R_USER_MAIN UM1, R_USER_GROUP, R_USER_MAIN UM2 where R_QUOTA_MAIN.user_id = UM1.user_id and UM1.user_type_name = 'rodsgroup' and R_USER_GROUP.group_user_id = UM1.user_id and UM2.user_id = R_USER_GROUP.user_id and R_QUOTA_USAGE.user_id = UM2.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id group by UM1.user_id, R_QUOTA_USAGE.resc_id";

    /* For each defined group limit on total usage (if any), get a
     * total usage on any resource for all users in that group: */
    char mySQL2a[] = "select sum(quota_usage), R_QUOTA_MAIN.quota_limit, UM1.user_id from R_QUOTA_USAGE, R_QUOTA_MAIN, R_USER_MAIN UM1, R_USER_GROUP, R_USER_MAIN UM2 where R_QUOTA_MAIN.user_id = UM1.user_id and UM1.user_type_name = 'rodsgroup' and R_USER_GROUP.group_user_id = UM1.user_id and UM2.user_id = R_USER_GROUP.user_id and R_QUOTA_USAGE.user_id = UM2.user_id and R_QUOTA_USAGE.resc_id != %s and R_QUOTA_MAIN.resc_id = %s group by UM1.user_id,  R_QUOTA_MAIN.quota_limit";
    char mySQL2b[MAX_SQL_SIZE];

    char mySQL3a[] = "update R_QUOTA_MAIN set quota_over= %s - ?, modify_ts=? where user_id=? and %s - ? > quota_over";
    char mySQL3b[MAX_SQL_SIZE];


    /* Initialize over_quota values (if any) to the no-usage value
       which is the negative of the limit.  */
    if ( logSQL != 0 ) {
        log_sql::debug("setOverQuota SQL 1");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_QUOTA_MAIN set quota_over = -quota_limit", &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        return ( 0 );   /* no quotas, done */
    }
    if ( status != 0 ) {
        return status;
    }

    /* Set the over_quota values for per-resource, if any */
    if ( logSQL != 0 ) {
        log_sql::debug("setOverQuota SQL 2");
    }
    status =  cmlExecuteNoAnswerSql(
#if ORA_ICAT
                  "update R_QUOTA_MAIN set quota_over = (select distinct R_QUOTA_USAGE.quota_usage - R_QUOTA_MAIN.quota_limit from R_QUOTA_USAGE where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id) where exists (select 1 from R_QUOTA_USAGE where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id)",
#elif MY_ICAT
                  "update R_QUOTA_MAIN, R_QUOTA_USAGE set R_QUOTA_MAIN.quota_over = R_QUOTA_USAGE.quota_usage - R_QUOTA_MAIN.quota_limit where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id",
#else
                  "update R_QUOTA_MAIN set quota_over = quota_usage - quota_limit from R_QUOTA_USAGE where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id",
#endif
                  & icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;    /* none */
    }
    if ( status != 0 ) {
        return status;
    }

    /* Set the over_quota values for irods-total, if any, and only if
       this over_quota value is higher than the previous.  Do it in
       two steps to keep it simpler (there may be a better way though).
    */
    if ( logSQL != 0 ) {
        log_sql::debug("setOverQuota SQL 3");
    }
    getNowStr( myTime );
    for ( rowsFound = 0;; rowsFound++ ) {
        int status2;
        if ( rowsFound == 0 ) {
            status = cmlGetFirstRowFromSql( "select sum(quota_usage), R_QUOTA_MAIN.user_id from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = '0' group by R_QUOTA_MAIN.user_id",
                                            &statementNum, 0, &icss );
        }
        else {
            status = cmlGetNextRowFromStatement( statementNum, &icss );
        }
        if ( status != 0 ) {
            break;
        }
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        if ( logSQL != 0 ) {
            log_sql::debug("setOverQuota SQL 4");
        }
        status2 = cmlExecuteNoAnswerSql( "update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and resc_id='0'",
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            cmlFreeStatement(statementNum, &icss);
            return status2;
        }
    }

    cmlFreeStatement(statementNum, &icss);

    /* Handle group quotas on resources */
    if ( logSQL != 0 ) {
        log_sql::debug("setOverQuota SQL 5");
    }
    for ( rowsFound = 0;; rowsFound++ ) {
        int status2;
        if ( rowsFound == 0 ) {
            status = cmlGetFirstRowFromSql( mySQL1, &statementNum,
                                            0, &icss );
        }
        else {
            status = cmlGetNextRowFromStatement( statementNum, &icss );
        }
        if ( status != 0 ) {
            break;
        }
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[2];
        if ( logSQL != 0 ) {
            log_sql::debug("setOverQuota SQL 6");
        }
        status2 = cmlExecuteNoAnswerSql( "update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and R_QUOTA_MAIN.resc_id=?",
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            cmlFreeStatement(statementNum, &icss);
            return status2;
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        cmlFreeStatement(statementNum, &icss);
        return status;
    }

    cmlFreeStatement(statementNum, &icss);

    /* Handle group quotas on total usage */
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    snprintf( mySQL2b, sizeof mySQL2b, mySQL2a,
              "cast('0' as integer)", "cast('0' as integer)" );
    snprintf( mySQL3b, sizeof mySQL3b, mySQL3a,
              "cast(? as integer)", "cast(? as integer)" );
#elif MY_ICAT
    snprintf( mySQL2b, sizeof mySQL2b, mySQL2a, "'0'", "'0'" );
    snprintf( mySQL3b, sizeof mySQL3b, mySQL3a, "?", "?" );
#else
    snprintf( mySQL2b, sizeof mySQL2b, mySQL2a,
              "cast('0' as bigint)", "cast('0' as bigint)" );
    snprintf( mySQL3b, sizeof mySQL3b, mySQL3a,
              "cast(? as bigint)", "cast(? as bigint)" );
#endif
    if ( logSQL != 0 ) {
        log_sql::debug("setOverQuota SQL 7");
    }
    getNowStr( myTime );
    for ( rowsFound = 0;; rowsFound++ ) {
        int status2;
        if ( rowsFound == 0 ) {
            status = cmlGetFirstRowFromSql( mySQL2b, &statementNum,
                                            0, &icss );
        }
        else {
            status = cmlGetNextRowFromStatement( statementNum, &icss );
        }
        if ( status != 0 ) {
            break;
        }
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[2];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++] = icss.stmtPtr[statementNum]->resultValue[1];
        if ( logSQL != 0 ) {
            log_sql::debug("setOverQuota SQL 8");
        }
        status2 = cmlExecuteNoAnswerSql( mySQL3b,
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            cmlFreeStatement(statementNum, &icss);
            return status2;
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        cmlFreeStatement(statementNum, &icss);
        return status;
    }

    /* To simplify the query, if either of the above group operations
       found some over_quota, will probably want to update and insert rows
       for each user into R_QUOTA_MAIN.  For now tho, this is not done and
       perhaps shouldn't be, to keep it a little less complicated. */

    cmlFreeStatement(statementNum, &icss);
    return status;
}

int
icatGetTicketUserId( irods::plugin_property_map& _prop_map, const char *userName, char *userIdStr ) {

    char userId[NAME_LEN];
    char userZone[NAME_LEN];
    char zoneToUse[NAME_LEN];
    char userName2[NAME_LEN];
    int status;

    std::string zone;
    irods::error ret = getLocalZone( _prop_map, &icss, zone );
    if ( !ret.ok() ) {
        return ret.code();
    }

    snprintf( zoneToUse, sizeof( zoneToUse ), "%s", zone.c_str() );
    status = validateAndParseUserName( userName, userName2, userZone );
    if ( status ) {
        return status;
    }
    if ( userZone[0] != '\0' ) {
        rstrcpy( zoneToUse, userZone, NAME_LEN );
    }

    userId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("icatGetTicketUserId SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName2 );
        bindVars.push_back( zoneToUse );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name!='rodsgroup'",
                     userId, NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return CAT_INVALID_USER;
        }
        return status;
    }
    strncpy( userIdStr, userId, NAME_LEN );
    return 0;
}

int
icatGetTicketGroupId( irods::plugin_property_map& _prop_map, const char *groupName, char *groupIdStr ) {
    char groupId[NAME_LEN];
    char groupZone[NAME_LEN];
    char zoneToUse[NAME_LEN];
    char groupName2[NAME_LEN];
    int status;

    std::string zone;
    irods::error ret = getLocalZone( _prop_map, &icss, zone );
    if ( !ret.ok() ) {
        return ret.code();
    }

    snprintf( zoneToUse, sizeof( zoneToUse ), "%s", zone.c_str() );
    status = validateAndParseUserName( groupName, groupName2, groupZone );
    if ( status ) {
        return status;
    }
    if ( groupZone[0] != '\0' ) {
        rstrcpy( zoneToUse, groupZone, NAME_LEN );
    }

    groupId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("icatGetTicketGroupId SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( groupName2 );
        bindVars.push_back( zoneToUse );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name='rodsgroup'",
                     groupId, NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return CAT_INVALID_GROUP;
        }
        return status;
    }
    strncpy( groupIdStr, groupId, NAME_LEN );
    return 0;
}


static
int convert_hostname_to_dotted_decimal_ipv4_and_store_in_buffer(const char* _hostname, char* _buf) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    struct addrinfo *p_addrinfo;
    const int ret_getaddrinfo_with_retry = getaddrinfo_with_retry(_hostname, 0, &hint, &p_addrinfo);
    if (ret_getaddrinfo_with_retry) {
        return ret_getaddrinfo_with_retry;
    }
    sprintf(_buf, "%s", inet_ntoa(reinterpret_cast<struct sockaddr_in*>(p_addrinfo->ai_addr)->sin_addr));
    freeaddrinfo(p_addrinfo);
    return 0;
}


char *
convertHostToIp( const char *inputName ) {
    static char ipAddr[50];
    const int status = convert_hostname_to_dotted_decimal_ipv4_and_store_in_buffer(inputName, ipAddr);
    if (status != 0) {
        log_db::error(
            "convertHostToIp convert_hostname_to_dotted_decimal_ipv4_and_store_in_buffer error. status [{}]", status);
        return NULL;
    }
    return ipAddr;
}

// XXXX HELPER FUNCTIONS ABOVE

// =-=-=-=-=-=-=-
// read a message body off of the socket
irods::error db_start_op(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    return ret;


} // db_start_op

// =-=-=-=-=-=-=-
// set debug behavior for plugin
irods::error db_debug_op(
    irods::plugin_context& _ctx,
    const char*            _mode ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // check incoming param
    if ( !_mode ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "mode is null" );
    }

    // =-=-=-=-=-=-=-
    // run tolower on mode
    std::string mode( _mode );
    std::transform(
        mode.begin(),
        mode.end(),
        mode.begin(),
        ::tolower );

    // =-=-=-=-=-=-=-
    // if mode contains 'sql' then turn SQL logging on
    if ( mode.find( "sql" ) != std::string::npos ) {
        logSQL = 1;
    }
    else {
        logSQL = 0;
    }

    return SUCCESS();

} // db_debug_op

// =-=-=-=-=-=-=-
// open a database connection
irods::error db_open_op(
    irods::plugin_context& _ctx ) {

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // check incoming param
//        if ( !_cfg ) {
//            return ERROR(
//                       CAT_INVALID_ARGUMENT,
//                       "config is null" );
//        }

    // =-=-=-=-=-=-=-
    // log as appropriate
    if ( logSQL != 0 ) {
        log_sql::debug("chlOpen");
    }

    // =-=-=-=-=-=-=-
    // cache db creds
    try {
        const auto db_plugin_map = irods::get_server_property<nlohmann::json>(std::vector<std::string>{irods::KW_CFG_PLUGIN_CONFIGURATION, irods::KW_CFG_PLUGIN_TYPE_DATABASE});
        const auto db_type   = db_plugin_map.items().begin().key();
        const auto db_plugin = db_plugin_map.items().begin().value();
        snprintf(icss.databaseUsername, DB_USERNAME_LEN, "%s", db_plugin.at(irods::KW_CFG_DB_USERNAME).get<std::string>().c_str());
        snprintf(icss.databasePassword, DB_PASSWORD_LEN, "%s", db_plugin.at(irods::KW_CFG_DB_PASSWORD).get<std::string>().c_str());
        snprintf(icss.database_plugin_type, DB_TYPENAME_LEN, "%s", db_type.c_str());
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    } catch ( const boost::exception& e ) {
        return ERROR(INVALID_ANY_CAST, "Failed any_cast in the database configuration");
    }

    // =-=-=-=-=-=-=-
    // call open in mid level
    int status = cmlOpen( &icss );
    if ( 0 != status ) {
        return ERROR(
                   status,
                   "failed to open db connection" );
    }

    // =-=-=-=-=-=-=-
    // set success flag
    icss.status = 1;

    // =-=-=-=-=-=-=-
    // Capture ICAT properties
#if MY_ICAT
#elif ORA_ICAT
#else
    irods::catalog_properties::instance().capture_if_needed( &icss );
#endif

    return CODE( status );

} // db_open_op

// =-=-=-=-=-=-=-
// close a database connection
irods::error db_close_op(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    // =-=-=-=-=-=-=-
    // call open in mid level
    int status = cmlClose( &icss );
    if ( 0 != status ) {
        return ERROR(
                   status,
                   "failed to close db connection" );
    }

    // =-=-=-=-=-=-=-
    // set success flag
    icss.status = 0;

    return CODE( status );

} // db_close_op

// =-=-=-=-=-=-=-
// return the local zone
irods::error db_check_and_get_object_id_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _access ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    rodsLong_t status = checkAndGetObjectId(
                            _ctx.comm(),
                            _ctx.prop_map(),
                            _type,
                            _name,
                            _access );
    if ( status < 0 ) {
        return ERROR( status, "checkAndGetObjectId failed" );
    }
    else {
        return SUCCESS();

    }

} // db_check_and_get_object_id_op

// =-=-=-=-=-=-=-
// return the local zone
irods::error db_get_local_zone_op(
    irods::plugin_context& _ctx,
    std::string*           _zone ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    ret = getLocalZone( _ctx.prop_map(), &icss, ( *_zone ) );
    if ( !ret.ok() ) {
        return PASS( ret );

    }
    else {
        return SUCCESS();

    }

} // db_get_local_zone_op

// =-=-=-=-=-=-=-
// update the data obj count of a resource
irods::error db_update_resc_obj_count_op(
    irods::plugin_context& _ctx,
    const std::string*     _resc,
    int                    _delta ) {

    return SUCCESS();

} // db_update_resc_obj_count_op

// =-=-=-=-=-=-=-
// update the data obj count of a resource
irods::error db_mod_data_obj_meta_op(
    irods::plugin_context& _ctx,
    dataObjInfo_t*         _data_obj_info,
    keyValPair_t*          _reg_param ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_data_obj_info ||
            !_reg_param ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );


    int status = 0, upCols = 0;
    rodsLong_t iVal = 0; // JMC cppcheck - uninit var
    int status2 = 0;

    char logicalFileName[MAX_NAME_LEN];
    char logicalDirName[MAX_NAME_LEN];
    char *theVal = 0;
    char replNum1[MAX_NAME_LEN];

    const char* whereColsAndConds[10];
    const char* whereValues[10];
    char idVal[MAX_NAME_LEN];
    int numConditions = 0;
    char replica_status_string[NAME_LEN]{};

    std::vector<const char *> updateCols;
    std::vector<const char *> updateVals;

    const std::vector<std::string_view> regParamNames = {
        COLL_ID_KW,
        DATA_CREATE_KW,
        CHKSUM_KW,
        DATA_EXPIRY_KW,
        //DATA_ID_KW,
        REPL_STATUS_KW,
        //DATA_MAP_ID_KW,
        DATA_MODE_KW,
        DATA_NAME_KW,
        DATA_OWNER_KW,
        DATA_OWNER_ZONE_KW,
        FILE_PATH_KW,
        REPL_NUM_KW,
        DATA_SIZE_KW,
        STATUS_STRING_KW,
        DATA_TYPE_KW,
        VERSION_KW,
        DATA_MODIFY_KW,
        DATA_COMMENTS_KW,
        // DATA_RESC_GROUP_NAME_KW,
        RESC_HIER_STR_KW,
        RESC_ID_KW,
        RESC_NAME_KW
    };

    const std::vector<std::string_view> colNames = {
        "coll_id",
        "create_ts",
        "data_checksum",
        "data_expiry_ts",
        //"data_id",
        "data_is_dirty",
        //"data_map_id",
        "data_mode",
        "data_name",
        "data_owner_name",
        "data_owner_zone",
        "data_path",
        "data_repl_num",
        "data_size",
        "data_status",
        "data_type_name",
        "data_version",
        "modify_ts",
        "r_comment",
        //"resc_group_name",
        "resc_hier",
        "resc_id",
        "resc_name"
    };

    int doingDataSize = 0;
    char dataSizeString[NAME_LEN] = "";
    char objIdString[MAX_NAME_LEN];
    char *neededAccess = 0;

    if ( logSQL != 0 ) {
        log_sql::debug("chlModDataObjMeta");
    }

    bool adminMode{};
    if (getValByKey(_reg_param, ADMIN_KW)) {
        if (LOCAL_PRIV_USER_AUTH != _ctx.comm()->clientUser.authInfo.authFlag) {
            return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "failed with insufficient privilege");
        }
        adminMode = true;
    }

    std::string update_resc_id_str;

    bool update_resc_id = false;
    /* Set up the updateCols and updateVals arrays */
    std::size_t i = 0, j = 0;
    for (i = 0, j = 0; i < regParamNames.size(); i++) {
        theVal = getValByKey(_reg_param, regParamNames[i].data());
        if (theVal) {
            if(colNames[i] == "resc_name") {
                continue;
            }
            else if(colNames[i] == "resc_hier") {
                updateCols.push_back( "resc_id" );

                rodsLong_t resc_id;
                resc_mgr.hier_to_leaf_id(theVal,resc_id);

                update_resc_id_str = boost::lexical_cast<std::string>(resc_id);
                updateVals.push_back( update_resc_id_str.c_str() );
            }
            else {
                updateCols.push_back(colNames[i].data());
                updateVals.push_back(theVal);
            }

            if ( std::string( "resc_id" ) == colNames[i] || std::string( "resc_hier") == colNames[i]) {
                update_resc_id = true;
            }

            if(regParamNames[i] == DATA_EXPIRY_KW) {
                /* if data_expiry, make sure it's in the standard time-stamp format: "%011d" */
                if (colNames[i] == "data_expiry_ts") { /* double check*/
                    if ( strlen( theVal ) < 11 ) {
                        static char theVal2[20];
                        time_t myTimeValue;
                        myTimeValue = atoll( theVal );
                        snprintf( theVal2, sizeof theVal2, "%011d", ( int )myTimeValue );
                        updateVals[j] = theVal2;
                    }
                }
            }

            if(regParamNames[i] == DATA_MODIFY_KW) {
                /* if modify_ts, also make sure it's in the standard time-stamp format: "%011d" */
                if (colNames[i] == "modify_ts") { /* double check*/
                    if ( strlen( theVal ) < 11 ) {
                        static char theVal3[20];
                        time_t myTimeValue;
                        myTimeValue = atoll( theVal );
                        snprintf( theVal3, sizeof theVal3, "%011d", ( int )myTimeValue );
                        updateVals[j] = theVal3;
                    }
                }
            }
            if(regParamNames[i] == DATA_SIZE_KW) {
                doingDataSize = 1; /* flag to check size */
                snprintf( dataSizeString, sizeof( dataSizeString ), "%s", theVal );
            }

            j++;

            /* If the datatype is being updated, check that it is valid */
            if(regParamNames[i] == DATA_TYPE_KW) {
                status = cmlCheckNameToken( "data_type",
                                            theVal, &icss );
                if ( status != 0 ) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Invalid data type specified.";
                    addRErrorMsg( &_ctx.comm()->rError, 0, msg.str().c_str() );
                    return ERROR(
                               CAT_INVALID_DATA_TYPE,
                               msg.str() );
                }
            }
        }
    }
    upCols = j;

    /* If the only field is the chksum then the user only needs read
       access since we can trust that the server-side code is
       calculating it properly and checksum is a system-managed field.
       For example, when doing an irsync the server may calculate a
       checksum and want to set it in the source copy.
    */
    neededAccess = ACCESS_MODIFY_METADATA;
    if ( upCols == 1 && strcmp( updateCols[0], "chksum" ) == 0 ) {
        neededAccess = ACCESS_READ_OBJECT;
    }

    /* If dataExpiry is being updated, user needs to have
       a greater access permission */
    theVal = getValByKey( _reg_param, DATA_EXPIRY_KW );
    if ( theVal != NULL ) {
        neededAccess = ACCESS_DELETE_OBJECT;
    }

    if ( _data_obj_info->dataId <= 0 ) {
        if (const auto ec = splitPathByKey(_data_obj_info->objPath, logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/'); ec < 0) {
            return ERROR(ec, fmt::format(
                         "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                         __func__, __LINE__, _data_obj_info->objPath, ec));
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModDataObjMeta SQL 1 ");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( logicalDirName );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_name=?", &iVal,
                         bindVars, &icss );
        }

        if ( status != 0 ) {
            char errMsg[105];
            snprintf( errMsg, 100, "collection '%s' is unknown",
                      logicalDirName );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            _rollback( "chlModDataObjMeta" );
            return ERROR(
                       CAT_UNKNOWN_COLLECTION,
                       "failed with unknown collection" );
        }
        snprintf( objIdString, MAX_NAME_LEN, "%lld", iVal );

        if ( logSQL != 0 ) {
            log_sql::debug("chlModDataObjMeta SQL 2");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( logicalFileName );
            status = cmlGetIntegerValueFromSql(
                         "select data_id from R_DATA_MAIN where coll_id=? and data_name=?",
                         &iVal, bindVars, &icss );
        }
        if ( status != 0 ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to find file in database by its logical path.";
            addRErrorMsg( &_ctx.comm()->rError, 0, msg.str().c_str() );
            _rollback( "chlModDataObjMeta" );
            return ERROR(
                       CAT_UNKNOWN_FILE,
                       "failed with unknown file" );
        }

        _data_obj_info->dataId = iVal;  /* return it for possible use next time, */
        /* and for use below */
    }

    snprintf( objIdString, MAX_NAME_LEN, "%lld", _data_obj_info->dataId );

    if (!adminMode) {
        if ( doingDataSize == 1 && strlen( mySessionTicket ) > 0 ) {
            status = cmlTicketUpdateWriteBytes( mySessionTicket,
                                                dataSizeString,
                                                objIdString, &icss );
            if ( status != 0 ) {
                //_rollback("chlModDataObjMeta");
                return ERROR(status, "cmlTicketUpdateWriteBytes failed");
            }
        }

        status = cmlCheckDataObjId(
                     objIdString,
                     _ctx.comm()->clientUser.userName,
                     _ctx.comm()->clientUser.rodsZone,
                     neededAccess,
                     mySessionTicket,
                     mySessionClientAddr,
                     &icss );

        if ( status != 0 ) {
            theVal = getValByKey( _reg_param, ACL_COLLECTION_KW );
            if ( theVal != NULL && upCols == 1 &&
                    strcmp( updateCols[0], "data_path" ) == 0 ) {
                int len, iVal = 0; // JMC cppcheck - uninit var ( shadows prev decl? )
                /*
                 In this case, the user is doing a 'imv' of a collection but one of
                 the sub-files is not owned by them.  We decided this should be
                 allowed and so we support it via this new ACL_COLLECTION_KW, checking
                 that the ACL_COLLECTION matches the beginning path of the object and
                 that the user has the appropriate access to that collection.
                 */
                len = strlen( theVal );
                if ( strncmp( theVal, _data_obj_info->objPath, len ) == 0 ) {

                    iVal = cmlCheckDir( theVal,
                                        _ctx.comm()->clientUser.userName,
                                        _ctx.comm()->clientUser.rodsZone,
                                        ACCESS_OWN,
                                        &icss );
                }
                if ( iVal > 0 ) {
                    status = 0;
                } /* Collection was found (id
                                   * returned) & user has access */
            }
            if ( status ) {
                _rollback( "chlModDataObjMeta" );
                return ERROR(
                           CAT_NO_ACCESS_PERMISSION,
                           "failed with no permission" );
            }
        }
    }

    whereColsAndConds[0] = "data_id=";
    snprintf( idVal, MAX_NAME_LEN, "%lld", _data_obj_info->dataId );
    whereValues[0] = idVal;
    numConditions = 1;

    /* This is up here since this is usually called to modify the
     * metadata of a single repl.  If ALL_KW is included, then apply
     * this change to all replicas (by not restricting the update to
     * only one).
     */
    std::string where_resc_id_str;
    if ( getValByKey( _reg_param, ALL_KW ) == NULL ) {
        // use resc_id instead of replNum as it is
        // always set, unless resc_id is to be
        // updated.  replNum is sometimes 0 in various
        // error cases
        if ( update_resc_id || strlen( _data_obj_info->rescHier ) <= 0 ) {
            j = numConditions;
            whereColsAndConds[j] = "data_repl_num=";
            snprintf( replNum1, MAX_NAME_LEN, "%d", _data_obj_info->replNum );
            whereValues[j] = replNum1;
            numConditions++;

        }
        else {
            rodsLong_t id = 0;
            resc_mgr.hier_to_leaf_id( _data_obj_info->rescHier, id );
            where_resc_id_str = boost::lexical_cast<std::string>(id);
            j = numConditions;
            whereColsAndConds[j] = "resc_id=";
            whereValues[j] = where_resc_id_str.c_str();
            numConditions++;
        }
    }

    std::string zone;
    ret = getLocalZone(
              _ctx.prop_map(),
              &icss,
              zone );
    if ( !ret.ok() ) {
        log_db::error("chlModObjMeta - failed in getLocalZone with status [{}]", status);
        return PASS( ret );
    }

    if (!getValByKey(_reg_param, ALL_REPL_STATUS_KW)) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModDataObjMeta SQL 4");
        }
        status = cmlModifySingleTable(
                "R_DATA_MAIN",
                &( updateCols[0] ),
                &( updateVals[0] ),
                whereColsAndConds,
                whereValues,
                upCols,
                numConditions,
                &icss );

        // Stale intermediate replicas
        if (0 == status && getValByKey(_reg_param, STALE_ALL_INTERMEDIATE_REPLICAS_KW)) {
            // Exclude this replica
            j = numConditions - 1;
            whereColsAndConds[j] = "data_repl_num!=";
            snprintf( replNum1, MAX_NAME_LEN, "%d", _data_obj_info->replNum );
            whereValues[j] = replNum1;

            // Find all intermediate replicas
            j = numConditions;
            whereColsAndConds[j] = "data_is_dirty=";
            whereValues[j] = intermediate_replica_status_str.c_str();
            numConditions++;

            // And mark them stale
            updateCols[0] = "data_is_dirty";
            memset(replica_status_string, 0, NAME_LEN);
            snprintf(replica_status_string, NAME_LEN, "%d", STALE_REPLICA);
            updateVals[0] = replica_status_string;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModDataObjMeta SQL 6");
            }
            status2 = cmlModifySingleTable( "R_DATA_MAIN", &( updateCols[0] ), &( updateVals[0] ),
                    whereColsAndConds, whereValues, 1,
                    numConditions, &icss );

            if ( status2 != 0 && status2 != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                /* Ignore NO_INFO errors but not others */
                log_db::info("chlModDataObjMeta cmlModifySingleTable failure for other replicas {}", status2);
                _rollback( "chlModDataObjMeta" );
                return ERROR(
                        status2,
                        "cmlModifySingleTable failure for other replicas" );
            }
        }
    }
    else {
        /* mark this one as GOOD_REPLICA and others as STALE_REPLICA */
        updateCols.push_back( "data_is_dirty" );
        snprintf(replica_status_string, NAME_LEN, "%d", GOOD_REPLICA);
        updateVals.push_back(replica_status_string);
        upCols++;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModDataObjMeta SQL 5");
        }
        status = cmlModifySingleTable(
                     "R_DATA_MAIN",
                     &( updateCols[0] ),
                     &( updateVals[0] ),
                     whereColsAndConds,
                     whereValues,
                     upCols,
                     numConditions,
                     &icss );

        if ( status == 0 ) {
            j = numConditions - 1;
            whereColsAndConds[j] = "data_repl_num!=";
            snprintf( replNum1, MAX_NAME_LEN, "%d", _data_obj_info->replNum );
            whereValues[j] = replNum1;

            // Only update replicas already marked as good
            // Intermediate replicas were never good, so they cannot be stale.
            j = numConditions;
            whereColsAndConds[j] = "data_is_dirty!=";
            whereValues[j] = intermediate_replica_status_str.c_str();
            numConditions++;

            updateCols[0] = "data_is_dirty";
            memset(replica_status_string, 0, NAME_LEN);
            snprintf(replica_status_string, NAME_LEN, "%d", STALE_REPLICA);
            updateVals[0] = replica_status_string;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModDataObjMeta SQL 6");
            }
            status2 = cmlModifySingleTable( "R_DATA_MAIN", &( updateCols[0] ), &( updateVals[0] ),
                                            whereColsAndConds, whereValues, 1,
                                            numConditions, &icss );

            if ( status2 != 0 && status2 != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                /* Ignore NO_INFO errors but not others */
                log_db::info("chlModDataObjMeta cmlModifySingleTable failure for other replicas {}", status2);
                _rollback( "chlModDataObjMeta" );
                return ERROR(
                           status2,
                           "cmlModifySingleTable failure for other replicas" );
            }

        }
    }
    if ( status != 0 ) {
        _rollback( "chlModDataObjMeta" );
        log_db::info("chlModDataObjMeta cmlModifySingleTable failure {}", status);
        return ERROR(
                   status,
                   "cmlModifySingleTable failure" );
    }

    if ( !( _data_obj_info->flags & NO_COMMIT_FLAG ) ) {
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            log_db::info("chlModDataObjMeta cmlExecuteNoAnswerSql commit failure {}", status);
            return ERROR(
                       status,
                       "commit failure" );
        }
    }

    return CODE( status );

} // db_mod_data_obj_meta_op


// =-=-=-=-=-=-=-
// update the data obj count of a resource
irods::error db_reg_data_obj_op(
    irods::plugin_context& _ctx,
    dataObjInfo_t*         _data_obj_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_data_obj_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    // =-=-=-=-=-=-=-
    //
    char myTime[50];
    char logicalFileName[MAX_NAME_LEN];
    char logicalDirName[MAX_NAME_LEN];
    rodsLong_t seqNum;
    rodsLong_t iVal;
    char dataIdNum[MAX_NAME_LEN];
    char collIdNum[MAX_NAME_LEN];
    char dataReplNum[MAX_NAME_LEN];
    char dataSizeNum[MAX_NAME_LEN];
    char dataStatusNum[MAX_NAME_LEN];
    int status;
    int inheritFlag;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegDataObj");
    }
    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegDataObj SQL 1 ");
    }
    seqNum = cmlGetNextSeqVal( &icss );
    if ( seqNum < 0 ) {
        log_db::info("chlRegDataObj cmlGetNextSeqVal failure {}", seqNum);
        _rollback( "chlRegDataObj" );
        return ERROR( seqNum, "chlRegDataObj cmlGetNextSeqVal failure" );
    }
    snprintf( dataIdNum, MAX_NAME_LEN, "%lld", seqNum );
    _data_obj_info->dataId = seqNum; /* store as output parameter */

    if (const auto ec = splitPathByKey(_data_obj_info->objPath, logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _data_obj_info->objPath, ec));
    }

    /* Check that collection exists and user has write permission.
       At the same time, also get the inherit flag */
    iVal = cmlCheckDirAndGetInheritFlag( logicalDirName,
                                         _ctx.comm()->clientUser.userName,
                                         _ctx.comm()->clientUser.rodsZone,
                                         ACCESS_MODIFY_OBJECT,
                                         &inheritFlag,
                                         mySessionTicket,
                                         mySessionClientAddr,
                                         &icss );
    if ( iVal < 0 ) {
        if ( iVal == CAT_UNKNOWN_COLLECTION ) {
            std::stringstream errMsg;
            errMsg << "collection '" << logicalDirName << "' is unknown";
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg.str().c_str() );
        }
        else if ( iVal == CAT_NO_ACCESS_PERMISSION ) {
            std::stringstream errMsg;
            errMsg << "no permission to update collection '" << logicalDirName << "'";
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg.str().c_str() );
        }
        //_rollback("chlRegDataObj");
        return ERROR( iVal, "" );
    }
    snprintf( collIdNum, MAX_NAME_LEN, "%lld", iVal );
    _data_obj_info->collId = iVal;

    /* Make sure no collection already exists by this name */
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegDataObj SQL 4");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _data_obj_info->objPath );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_name=?",
                     &iVal, bindVars, &icss );
    }
    if ( status == 0 ) {
        return ERROR( CAT_NAME_EXISTS_AS_COLLECTION, "collection exists" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegDataObj SQL 5");
    }
    status = cmlCheckNameToken( "data_type",
                                _data_obj_info->dataType, &icss );
    if ( status != 0 ) {
        return ERROR( CAT_INVALID_DATA_TYPE, "invalid data type" );
    }

    snprintf( dataReplNum, MAX_NAME_LEN, "%d", _data_obj_info->replNum );
    snprintf( dataStatusNum, MAX_NAME_LEN, "%d", _data_obj_info->replStatus );
    snprintf( dataSizeNum, MAX_NAME_LEN, "%lld", _data_obj_info->dataSize );

    getNowStr( myTime );
    if (0 == strcmp(_data_obj_info->dataModify, "")) {
        strcpy(_data_obj_info->dataModify, myTime);
    }
    if (0 == strcmp(_data_obj_info->dataCreate, "")) {
        strcpy(_data_obj_info->dataCreate, myTime);
    }
    strcpy(_data_obj_info->dataExpiry, "00000000000");

    std::snprintf(_data_obj_info->dataOwnerName, sizeof(_data_obj_info->dataOwnerName), "%s", _ctx.comm()->clientUser.userName);
    std::snprintf(_data_obj_info->dataOwnerZone, sizeof(_data_obj_info->dataOwnerZone), "%s", _ctx.comm()->clientUser.rodsZone);

    std::string resc_id_str = boost::lexical_cast<std::string>(_data_obj_info->rescId);

    cllBindVars[0] = dataIdNum;
    cllBindVars[1] = collIdNum;
    cllBindVars[2] = logicalFileName;
    cllBindVars[3] = dataReplNum;
    cllBindVars[4] = _data_obj_info->version;
    cllBindVars[5] = _data_obj_info->dataType;
    cllBindVars[6] = dataSizeNum;
    cllBindVars[7] = resc_id_str.c_str();
    cllBindVars[8] = _data_obj_info->filePath;
    cllBindVars[9] = _data_obj_info->dataOwnerName;
    cllBindVars[10] = _data_obj_info->dataOwnerZone;
    cllBindVars[11] = dataStatusNum;
    cllBindVars[12] = _data_obj_info->chksum;
    cllBindVars[13] = _data_obj_info->dataMode;
    cllBindVars[14] = _data_obj_info->dataCreate;
    cllBindVars[15] = _data_obj_info->dataModify;
    cllBindVars[16] = _data_obj_info->dataExpiry;
    cllBindVars[17] = "EMPTY_RESC_NAME";
    cllBindVars[18] = "EMPTY_RESC_HIER";
    cllBindVars[19] = "EMPTY_RESC_GROUP_NAME";
    cllBindVarCount = 20;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegDataObj SQL 6");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_DATA_MAIN (data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_id, data_path, data_owner_name, data_owner_zone, data_is_dirty, data_checksum, data_mode, create_ts, modify_ts, data_expiry_ts, resc_name, resc_hier, resc_group_name) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRegDataObj cmlExecuteNoAnswerSql failure {}", status);
        _rollback( "chlRegDataObj" );
        return ERROR( status, "chlRegDataObj cmlExecuteNoAnswerSql failure" );
    }
    std::string zone;
    ret = getLocalZone(
              _ctx.prop_map(),
              &icss,
              zone );
    if ( !ret.ok() ) {
        log_db::error("chlRegDataInfo - failed in getLocalZone with status [{}]", status);
        return PASS( ret );
    }

    if ( inheritFlag ) {
        /* If inherit is set (sticky bit), then add access rows for this
           dataobject that match those of the parent collection */
        cllBindVars[0] = dataIdNum;
        cllBindVars[1] = myTime;
        cllBindVars[2] = myTime;
        cllBindVars[3] = collIdNum;
        cllBindVarCount = 4;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegDataObj SQL 7");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select ?, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlRegDataObj cmlExecuteNoAnswerSql insert access failure {}", status);
            _rollback( "chlRegDataObj" );
            return ERROR( status, "cmlExecuteNoAnswerSql insert access failure" );
        }
    }
    else {
        cllBindVars[0] = dataIdNum;
        cllBindVars[1] = _ctx.comm()->clientUser.userName;
        cllBindVars[2] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[3] = ACCESS_OWN;
        cllBindVars[4] = myTime;
        cllBindVars[5] = myTime;
        cllBindVarCount = 6;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegDataObj SQL 8");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlRegDataObj cmlExecuteNoAnswerSql insert access failure {}", status);
            _rollback( "chlRegDataObj" );
            return ERROR( status, "cmlExecuteNoAnswerSql insert access failure" );
        }
    }

    if ( !( _data_obj_info->flags & NO_COMMIT_FLAG ) ) {
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            log_db::info("chlRegDataObj cmlExecuteNoAnswerSql commit failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }
    }

    return SUCCESS();

} // db_reg_data_obj_op


// =-=-=-=-=-=-=-
// register a data object into the catalog
irods::error db_reg_replica_op(
    irods::plugin_context& _ctx,
    dataObjInfo_t*         _src_data_obj_info,
    dataObjInfo_t*         _dst_data_obj_info,
    keyValPair_t*          _cond_input ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_src_data_obj_info ||
        !_dst_data_obj_info ||
        !_cond_input ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    if( _dst_data_obj_info->rescId <= 0 ) {
        std::stringstream msg;
        msg << "invalid resource id "
            << _dst_data_obj_info->rescId
            << " for ["
            << _dst_data_obj_info->objPath
            << "]";
        return ERROR(
                SYS_INVALID_INPUT_PARAM,
                msg.str() );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );


    char myTime[50];
    char logicalFileName[MAX_NAME_LEN];
    char logicalDirName[MAX_NAME_LEN];
    rodsLong_t iVal;
    rodsLong_t status;
    char tSQL[MAX_SQL_SIZE];
    char *cVal[30];
    int i;
    int statementNumber = UNINITIALIZED_STATEMENT_NUMBER;
    int nextReplNum;
    char nextRepl[30];
    char theColls[] = "data_id, \
                       coll_id,  \
                       data_name, \
                       data_repl_num, \
                       data_version, \
                       data_type_name, \
                       data_size, \
                       resc_group_name, \
                       resc_name, \
                       resc_hier, \
                       resc_id, \
                       data_path, \
                       data_owner_name, \
                       data_owner_zone, \
                       data_is_dirty, \
                       data_status, \
                       data_checksum, \
                       data_expiry_ts, \
                       data_map_id, \
                       data_mode, \
                       r_comment, \
                       create_ts, \
                       modify_ts";
    const int IX_DATA_REPL_NUM = 3; /* index of data_repl_num in theColls */
//        int IX_RESC_GROUP_NAME = 7; /* index into theColls */
    const int IX_RESC_ID = 10;
    const int IX_DATA_PATH = 11;    /* index into theColls */
    const int IX_REPLICA_STATUS = 14;
    const int IX_DATA_STATUS = 15;

    const int IX_DATA_MODE = 19;
    const int IX_CREATE_TS = 21;
    const int IX_MODIFY_TS = 22;
    const int IX_RESC_NAME2 = 23;
    const int IX_DATA_PATH2 = 24;
    const int IX_DATA_ID2 = 25;
    int nColumns = 26;

    char objIdString[MAX_NAME_LEN];
    char replNumString[MAX_NAME_LEN];
    int adminMode;
    char *theVal;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegReplica");
    }

    adminMode = 0;
    if ( _cond_input != NULL ) {
        theVal = getValByKey( _cond_input, ADMIN_KW );
        if ( theVal != NULL ) {
            adminMode = 1;
        }
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if (const auto ec = splitPathByKey(_src_data_obj_info->objPath, logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _src_data_obj_info->objPath, ec));
    }

    if ( adminMode ) {
        if ( _ctx.comm()->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
    }
    else {
        /* Check the access to the dataObj */
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegReplica SQL 1 ");
        }
        status = cmlCheckDataObjOnly( logicalDirName, logicalFileName,
                                      _ctx.comm()->clientUser.userName,
                                      _ctx.comm()->clientUser.rodsZone,
                                      ACCESS_READ_OBJECT, &icss );
        if ( status < 0 ) {
            _rollback( "chlRegReplica" );
            return ERROR( status, "cmlCheckDataObjOnly failed" );
        }
    }

    /* Get the next replica number */
    snprintf( objIdString, MAX_NAME_LEN, "%lld", _src_data_obj_info->dataId );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegReplica SQL 2");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        status = cmlGetIntegerValueFromSql(
                     "select max(data_repl_num) from R_DATA_MAIN where data_id = ?",
                     &iVal, bindVars, &icss );
    }

    if ( status != 0 ) {
        _rollback( "chlRegReplica" );
        return ERROR( status, "cmlGetIntegerValueFromSql failed" );
    }

    nextReplNum = iVal + 1;
    snprintf( nextRepl, sizeof nextRepl, "%d", nextReplNum );
    _dst_data_obj_info->replNum = nextReplNum; /* return new replica number */
    snprintf( replNumString, MAX_NAME_LEN, "%d", _src_data_obj_info->replNum );
    snprintf( tSQL, MAX_SQL_SIZE,
              "select %s from R_DATA_MAIN where data_id = ? and data_repl_num = ?",
              theColls );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegReplica SQL 3");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( replNumString );
        status = cmlGetOneRowFromSqlV2( tSQL, cVal, nColumns, bindVars, &icss );
    }
    if ( status < 0 ) {
        _rollback( "chlRegReplica" );
        return ERROR( status, "cmlGetOneRowFromSqlV2 failed" );
    }
    statementNumber = status;

    std::string resc_id_str = boost::lexical_cast<std::string>(_dst_data_obj_info->rescId);

    cVal[IX_DATA_REPL_NUM]   = nextRepl;
    //cVal[IX_RESC_NAME]       = _dst_data_obj_info->rescName;
    cVal[IX_RESC_ID]       = (char*)resc_id_str.c_str();
    cVal[IX_DATA_PATH]       = _dst_data_obj_info->filePath;
    cVal[IX_DATA_MODE]       = _dst_data_obj_info->dataMode;

    // The caller has requested that the replica be registered as intermediate. This
    // means that the replica will be written or changed at a future time. Otherwise,
    // the replica will take the replica status of the source replica. The const must
    // be cast away due to cVal being a char*[] and not a const char*[].
    if (getValByKey(_cond_input, REGISTER_AS_INTERMEDIATE_KW)) {
        cVal[IX_REPLICA_STATUS] = const_cast<char*>(intermediate_replica_status_str.data());
    }

    // data_status tracks replica status for logical locking - this is a new replica,
    // so make sure the data_status column is empty at registration time.
    std::snprintf(_dst_data_obj_info->statusString, NAME_LEN, "");
    cVal[IX_DATA_STATUS] = _dst_data_obj_info->statusString;

    getNowStr( myTime );
    cVal[IX_MODIFY_TS] = myTime;
    cVal[IX_CREATE_TS] = myTime;

    cVal[IX_RESC_NAME2] = (char*)resc_id_str.c_str();//_dst_data_obj_info->rescName; // JMC - backport 4669
    cVal[IX_DATA_PATH2] = _dst_data_obj_info->filePath; // JMC - backport 4669
    cVal[IX_DATA_ID2] = objIdString; // JMC - backport 4669

    for ( i = 0; i < nColumns; i++ ) {
        cllBindVars[i] = cVal[i];
    }
    cllBindVarCount = nColumns;
#if (defined ORA_ICAT || defined MY_ICAT) // JMC - backport 4685
    /* MySQL and Oracle */
    snprintf( tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? from DUAL where not exists (select data_id from R_DATA_MAIN where resc_id=? and data_path=? and data_id=?)",
              theColls );
#else
    /* Postgres */
    snprintf( tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? where not exists (select data_id from R_DATA_MAIN where resc_id=? and data_path=? and data_id=?)",
              theColls );

#endif
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegReplica SQL 4");
    }
    status = cmlExecuteNoAnswerSql( tSQL,  &icss );
    if ( status < 0 ) {
        log_db::info("chlRegReplica cmlExecuteNoAnswerSql(insert) failure {}", status);
        _rollback( "chlRegReplica" );
        const int free_status = cmlFreeStatement( statementNumber, &icss );
        if (free_status != 0) {
            log_db::error("db_reg_replica_op: cmlFreeStatement0 failure [{}]", free_status);
        }
        return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        log_db::error("chlRegReplica - failed in getLocalZone with status [{}]", status);
        const int free_status = cmlFreeStatement( statementNumber, &icss );
        if (free_status != 0) {
            log_db::error("db_reg_replica_op: cmlFreeStatement1 failure [{}]", free_status);
        }
        return PASS( ret );
    }

    status = cmlFreeStatement( statementNumber, &icss );
    if ( status < 0 ) {
        log_db::info("chlRegReplica cmlFreeStatement failure {}", status);
        return ERROR( status, "cmlFreeStatement failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegReplica cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();

} // db_reg_replica_op

// =-=-=-=-=-=-=-
// unregister a data object
irods::error db_unreg_replica_op(
    irods::plugin_context& _ctx,
    dataObjInfo_t*         _data_obj_info,
    keyValPair_t*          _cond_input ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_data_obj_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );


    // =-=-=-=-=-=-=-
    //
    char logicalFileName[MAX_NAME_LEN];
    char logicalDirName[MAX_NAME_LEN];
    rodsLong_t status;
    char tSQL[MAX_SQL_SIZE];
    char replNumber[30];
    char dataObjNumber[30];
    int adminMode;
    int trashMode;
    char *theVal;
    char checkPath[MAX_NAME_LEN];

    dataObjNumber[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlUnregDataObj");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    adminMode = 0;
    trashMode = 0;
    if ( _cond_input != NULL ) {
        theVal = getValByKey( _cond_input, ADMIN_KW );
        if ( theVal != NULL ) {
            adminMode = 1;
        }
        theVal = getValByKey( _cond_input, ADMIN_RMTRASH_KW );
        if ( theVal != NULL ) {
            adminMode = 1;
            trashMode = 1;
        }
    }

    if (const auto ec = splitPathByKey(_data_obj_info->objPath, logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _data_obj_info->objPath, ec));
    }

    if ( adminMode == 0 ) {
        /* Check the access to the dataObj */
        if ( logSQL != 0 ) {
            log_sql::debug("chlUnregDataObj SQL 1 ");
        }
        status = cmlCheckDataObjOnly( logicalDirName, logicalFileName,
                                      _ctx.comm()->clientUser.userName,
                                      _ctx.comm()->clientUser.rodsZone,
                                      ACCESS_DELETE_OBJECT, &icss );
        if ( status < 0 ) {
            _rollback( "chlUnregDataObj" );
            return ERROR( status, "cmlCheckDataObjOnly failed" ); /* convert long to int */
        }
        snprintf( dataObjNumber, sizeof dataObjNumber, "%lld", status );
    }
    else {
        if ( _ctx.comm()->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( trashMode ) {
            int len;
            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
            snprintf( checkPath, MAX_NAME_LEN, "/%s/trash", zone.c_str() );
            len = strlen( checkPath );
            if ( strncmp( checkPath, logicalDirName, len ) != 0 ) {
                addRErrorMsg( &_ctx.comm()->rError, 0,
                              "TRASH_KW but not zone/trash path" );
                return ERROR( CAT_INVALID_ARGUMENT, "TRASH_KW but not zone/trash path" );
            }
            if ( _data_obj_info->dataId > 0 ) {
                snprintf( dataObjNumber, sizeof dataObjNumber, "%lld",
                          _data_obj_info->dataId );
            }
        }
        else {
            if ( _data_obj_info->replNum >= 0 && _data_obj_info->dataId >= 0 ) {
                snprintf( dataObjNumber, sizeof dataObjNumber, "%lld",
                          _data_obj_info->dataId );
            }
            else {
                addRErrorMsg( &_ctx.comm()->rError, 0,
                              "dataId and replNum required" );
                _rollback( "chlUnregDataObj" );
                return ERROR( CAT_INVALID_ARGUMENT, "dataId and replNum required" );
            }
        }
    }

    cllBindVars[0] = logicalDirName;
    cllBindVars[1] = logicalFileName;
    if ( _data_obj_info->replNum >= 0 ) {
        snprintf( replNumber, sizeof replNumber, "%d", _data_obj_info->replNum );
        cllBindVars[2] = replNumber;
        cllBindVarCount = 3;
        if ( logSQL != 0 ) {
            log_sql::debug("chlUnregDataObj SQL 4");
        }
        snprintf( tSQL, MAX_SQL_SIZE,
                  "delete from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?) and data_name=? and data_repl_num=?" );
    }
    else {
        cllBindVarCount = 2;
        if ( logSQL != 0 ) {
            log_sql::debug("chlUnregDataObj SQL 5");
        }
        snprintf( tSQL, MAX_SQL_SIZE,
                  "delete from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?) and data_name=?" );
    }
    status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    if ( status != 0 ) {
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[105];
            status = CAT_UNKNOWN_FILE;  /* More accurate, in this case */
            snprintf( errMsg, 100, "data object '%s' is unknown",
                      logicalFileName );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( status, "data object unknown" );
        }
        _rollback( "chlUnregDataObj" );
        return ERROR( status, "cmlExecuteNoAnswerSql failed" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        log_db::error("chlUnRegDataObj - failed in getLocalZone with status [{}]", status);
        return PASS( ret );
    }

    /* delete the access rows, if we just deleted the last replica */
    if ( dataObjNumber[0] != '\0' ) {
        cllBindVars[0] = dataObjNumber;
        cllBindVars[1] = dataObjNumber;
        cllBindVarCount = 2;
        if ( logSQL != 0 ) {
            log_sql::debug("chlUnregDataObj SQL 3");
        }

        status = cmlExecuteNoAnswerSql(
                     "delete from R_OBJT_ACCESS where object_id=? and not exists (select * from R_DATA_MAIN where data_id=?)", &icss );
        if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
            _rollback("chlUnregDataObj");
            return ERROR(status, fmt::format(
                         "[{}:{}] - failed to delete access rows [ec=[{}]]",
                         __func__, __LINE__, status));
        }

        if (status == 0) {
            if (const auto ec = removeMetaMapAndAVU(dataObjNumber); ec < 0) {
                irods::log(LOG_WARNING, fmt::format(
                           "[{}:{}] - failed to remove associated AVUs [ec=[{}]]",
                           __func__, __LINE__, ec));
            }
        }
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlUnregDataObj cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();

} // db_unreg_replica_op

// =-=-=-=-=-=-=-
//
irods::error db_reg_rule_exec_op(
    irods::plugin_context& _ctx,
    ruleExecSubmitInp_t*   _re_sub_inp ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_re_sub_inp ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    char myTime[50];
    rodsLong_t seqNum;
    char ruleExecIdNum[MAX_NAME_LEN];
    int status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegRuleExec");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegRuleExec SQL 1 ");
    }

    seqNum = cmlGetNextSeqVal( &icss );
    if ( seqNum < 0 ) {
        log_db::info("chlRegRuleExec cmlGetNextSeqVal failure {}", seqNum);
        _rollback( "chlRegRuleExec" );
        return ERROR( seqNum, "cmlGetNextSeqVal failure" );
    }
    snprintf( ruleExecIdNum, MAX_NAME_LEN, "%lld", seqNum );

    /* store as output parameter */
    snprintf( _re_sub_inp->ruleExecId, NAME_LEN, "%s", ruleExecIdNum );

    getNowStr( myTime );

    cllBindVars[cllBindVarCount++] = ruleExecIdNum;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->ruleName;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->reiFilePath;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->userName;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->exeAddress;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->exeTime;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->exeFrequency;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->priority;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->estimateExeTime;
    cllBindVars[cllBindVarCount++] = _re_sub_inp->notificationAddr;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    // To maintain backwards compatibility, the jsonified rule execution context
    // is passed via the conditional input.
    const irods::experimental::key_value_proxy kvp{_re_sub_inp->condInput};
    cllBindVars[cllBindVarCount++] = kvp[RULE_EXECUTION_CONTEXT_KW].value().data();

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegRuleExec SQL 2");
    }
    status = cmlExecuteNoAnswerSql("insert into R_RULE_EXEC (rule_exec_id, rule_name, rei_file_path, user_name, exe_address, exe_time, exe_frequency, priority, estimated_exe_time, notification_addr, create_ts, modify_ts, exe_context) "
                                   "values (?,?,?,?,?,?,?,?,?,?,?,?,?)",
                                   &icss);
    if ( status != 0 ) {
        log_db::info("chlRegRuleExec cmlExecuteNoAnswerSql(insert) failure {}", status);
        _rollback( "chlRegRuleExec" );
        return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegRuleExec cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();
} // db_reg_rule_exec_op

// =-=-=-=-=-=-=-
// Modify an existing rule in the catalog.
irods::error db_mod_rule_exec_op(
    irods::plugin_context& _ctx,
    const char*            _re_id,
    keyValPair_t*          _reg_param )
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_re_id  || !_reg_param ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );


    // =-=-=-=-=-=-=-
    //
    int i, j, status;

    char tSQL[MAX_SQL_SIZE];
    char *theVal = 0;

    /* regParamNames has the argument names (in regParam) that this
       routine understands and colNames has the corresponding column
       names; one for one. */
    char *regParamNames[] = {
        RULE_NAME_KW,
        RULE_REI_FILE_PATH_KW,
        RULE_USER_NAME_KW,
        RULE_EXE_ADDRESS_KW,
        RULE_EXE_TIME_KW,
        RULE_EXE_FREQUENCY_KW,
        RULE_PRIORITY_KW,
        RULE_ESTIMATE_EXE_TIME_KW,
        RULE_NOTIFICATION_ADDR_KW,
        RULE_LAST_EXE_TIME_KW,
        RULE_EXE_STATUS_KW,
        RULE_EXECUTION_CONTEXT_KW,
        "END"
    };

    char *colNames[] = {
        "rule_name",
        "rei_file_path",
        "user_name",
        "exe_address",
        "exe_time",
        "exe_frequency",
        "priority",
        "estimated_exe_time",
        "notification_addr",
        "last_exe_time",
        "exe_status",
        "exe_context",

        // The following columns are handled automatically.
        // ** New columns MUST be added before these lines! **
        "create_ts",
        "modify_ts"
    };

    if ( logSQL != 0 ) {
        log_sql::debug("chlModRuleExec");
    }

    snprintf( tSQL, MAX_SQL_SIZE, "update R_RULE_EXEC set " );

    for ( i = 0, j = 0; strcmp( regParamNames[i], "END" ); i++ ) {
        theVal = getValByKey( _reg_param, regParamNames[i] );

        if (theVal) {
            if (std::string_view{regParamNames[i]} == RULE_PRIORITY_KW) {
                if (std::strlen(theVal) == 0) {
                    return ERROR(SYS_INVALID_INPUT_PARAM,
                                 "Delay rule priority cannot be empty. Delay rule priority must "
                                 "satisfy the following requirement: 1 <= P <= 9.");
                }

                try {
                    if (const auto p = std::stoi(theVal); p < 1 || p > 9) {
                        return ERROR(SYS_INVALID_INPUT_PARAM,
                                     "Delay rule priority must satisfy the following requirement: 1 <= P <= 9.");
                    }
                }
                catch (...) {
                    return ERROR(SYS_INVALID_INPUT_PARAM, "Delay rule priority is not an integer.");
                }
            }

            if ( j > 0 ) {
                rstrcat( tSQL, "," , MAX_SQL_SIZE );
            }
            rstrcat( tSQL, colNames[i] , MAX_SQL_SIZE );
            rstrcat( tSQL, "=? ", MAX_SQL_SIZE );
            cllBindVars[j++] = theVal;
        }
    }

    if ( j == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid argument" );
    }

    rstrcat( tSQL, "where rule_exec_id=?", MAX_SQL_SIZE );
    cllBindVars[j++] = _re_id;
    cllBindVarCount = j;

    if ( logSQL != 0 ) {
        log_sql::debug("chlModRuleExec SQL 1 ");
    }
    status = cmlExecuteNoAnswerSql( tSQL, &icss );

    if ( status != 0 ) {
        _rollback( "chlModRuleExec" );
        log_db::info("chlModRuleExec cmlExecuteNoAnswer(update) failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswer(update) failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModRuleExecMeta cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return CODE( status );
} // db_mod_rule_exec_op

irods::error db_del_rule_exec_op(irods::plugin_context& _ctx, const char* _re_id)
{
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if (!_re_id) {
        return ERROR(CAT_INVALID_ARGUMENT, "null parameter");
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelRuleExec");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    cllBindVars[cllBindVarCount++] = _re_id;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelRuleExec SQL 2 ");
    }
    int status = cmlExecuteNoAnswerSql("delete from R_RULE_EXEC where rule_exec_id=?", &icss);
    if ( status != 0 ) {
        log_db::info("chlDelRuleExec delete failure {}", status);
        _rollback( "chlDelRuleExec" );
        return ERROR( status, "delete failure" );
    }

    status = cmlExecuteNoAnswerSql("commit", &icss);
    if ( status != 0 ) {
        log_db::info("chlDelRuleExec cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return CODE( status );
} // db_del_rule_exec_op

static irods::error extract_resource_properties_for_operations(
    const std::string& _resc_name,
    std::string& _resc_id,
    std::string& _resc_parent ) {

    irods::resource_ptr resc;
    irods::error ret = resc_mgr.resolve(
                           _resc_name,
                           resc);
    if(!ret.ok()) {
        return PASS(ret);
    }

    ret = resc->get_property<std::string>(
                    irods::RESOURCE_PARENT,
                    _resc_parent);
    if(!ret.ok()) {
        return PASS(ret);
    }

    rodsLong_t resc_id;
    ret = resc->get_property<rodsLong_t>(
                    irods::RESOURCE_ID,
                    resc_id);
    if(!ret.ok()) {
        return PASS(ret);
    }

    try {
        _resc_id = boost::lexical_cast<std::string>(resc_id);
    } catch( boost::bad_lexical_cast& ) {
        std::stringstream msg;
        msg << "failed to cast " << resc_id;
        return ERROR(
                INVALID_LEXICAL_CAST,
                msg.str());
    }

    return SUCCESS();

} // extract_resource_properties_for_operations

irods::error db_add_child_resc_op(
    irods::plugin_context& _ctx,
    std::map<std::string, std::string> *_resc_input ) {
    // =-=-=-=-=-=-=-
    // for readability
    std::map<std::string, std::string>& resc_input = *_resc_input;

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    irods::sql_logger logger( __FUNCTION__, logSQL );

    logger.log();

    std::string new_child_string( resc_input[irods::RESOURCE_CHILDREN] );

    irods::children_parser child_parser;
    child_parser.set_string( new_child_string );

    irods::children_parser::children_map_t c_map;
    child_parser.list( c_map );

    if(c_map.empty()) {
       return ERROR(
                  SYS_INVALID_INPUT_PARAM,
                  "child map is empty" );
    }

    std::string child_name    = c_map.begin()->first;
    std::string child_context = c_map.begin()->second;

    std::string child_resource_id;
    std::string child_parent_name;
    ret = extract_resource_properties_for_operations(
              child_name,
              child_resource_id,
              child_parent_name);

    if(!ret.ok()) {
	if( SYS_RESC_DOES_NOT_EXIST == ret.code() ) {
	    return ERROR(
                       CHILD_NOT_FOUND,
                       child_parent_name.c_str());
	}
        return PASS(ret);
    }

    std::string& parent_name = resc_input[irods::RESOURCE_NAME];
    std::string parent_resource_id;
    std::string parent_parent_name;
    ret = extract_resource_properties_for_operations(
              parent_name,
              parent_resource_id,
              parent_parent_name);
    if(!ret.ok()) {
        if( SYS_RESC_DOES_NOT_EXIST == ret.code() ) {
            return ERROR(
                       CAT_INVALID_RESOURCE,
                       child_parent_name.c_str());
        }
        return PASS(ret);
    }

    int status = _canConnectToCatalog( _ctx.comm() );
    if(0 != status) {
        return ERROR(
                   status,
                   "_canConnectToCatalog failed");
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if(!ret.ok()) {
        return PASS(ret);
    }

    if(resc_input[irods::RESOURCE_ZONE].length() > 0 &&
       resc_input[irods::RESOURCE_ZONE] != zone ) {
        addRErrorMsg(
            &_ctx.comm()->rError, 0,
            "Currently, resources must be in the local zone" );

        return ERROR(
                   CAT_INVALID_ZONE,
                   "resources must be in the local zone");
    }

    logger.log();

    ret = update_child_parent(child_resource_id, parent_resource_id, child_context);
    if(!ret.ok()) {
        return PASS(ret);
    }

    status = cmlExecuteNoAnswerSql( "commit", &icss );
    if(status != 0) {
        return ERROR(
                   status,
                   "commit failure");
    }

    return SUCCESS();

} // db_add_child_resc_op

// =-=-=-=-=-=-=-
//
irods::error db_reg_resc_op(
    irods::plugin_context& _ctx,
    std::map<std::string, std::string> *_resc_input ) {

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_resc_input ) {
        return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "NULL parameter" );
    }

    // =-=-=-=-=-=-=-
    // for readability
    std::map<std::string, std::string>& resc_input = *_resc_input;

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    rodsLong_t seqNum;
    char idNum[MAX_SQL_SIZE];
    int status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegResc");
    }

    // =-=-=-=-=-=-=-
    // error trap empty resc name
    if ( resc_input[irods::RESOURCE_NAME].length() < 1 ) {
        addRErrorMsg( &_ctx.comm()->rError, 0, "resource name is empty" );
        return ERROR( CAT_INVALID_RESOURCE_NAME, "resource name is empty" );
    }

    // =-=-=-=-=-=-=-
    // error trap empty resc type
    if ( resc_input[irods::RESOURCE_TYPE].length() < 1 ) {
        addRErrorMsg( &_ctx.comm()->rError, 0, "resource type is empty" );
        return ERROR( CAT_INVALID_RESOURCE_TYPE, "resource type is empty" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    // =-=-=-=-=-=-=-
    // Validate resource name format
    ret = validate_resource_name( resc_input[irods::RESOURCE_NAME] );
    if ( !ret.ok() ) {
        irods::log( ret );
        return PASS( ret );
    }
    // =-=-=-=-=-=-=-


    if ( logSQL != 0 ) {
        log_sql::debug("chlRegResc SQL 1 ");
    }
    seqNum = cmlGetNextSeqVal( &icss );
    if ( seqNum < 0 ) {
        log_db::info("chlRegResc cmlGetNextSeqVal failure {}", seqNum);
        _rollback( "chlRegResc" );
        return ERROR( seqNum, "cmlGetNextSeqVal failure" );
    }
    snprintf( idNum, MAX_SQL_SIZE, "%lld", seqNum );

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );

    }

    if ( resc_input[irods::RESOURCE_ZONE].length() > 0 ) {
        if ( resc_input[irods::RESOURCE_ZONE] != zone ) {
            addRErrorMsg( &_ctx.comm()->rError, 0,
                          "Currently, resources must be in the local zone" );
            return ERROR( CAT_INVALID_ZONE, "resources must be in the local zone" );
        }
    }
    // =-=-=-=-=-=-=-
    // JMC :: resources may now have an empty location if they
    //     :: are coordinating nodes
    //    if (strlen(_resc_info->rescLoc)<1) {
    //        return(CAT_INVALID_RESOURCE_NET_ADDR);
    //    }
    // =-=-=-=-=-=-=-
    // if the resource is not the 'empty resource' test it
    if ( resc_input[irods::RESOURCE_LOCATION] != irods::EMPTY_RESC_HOST ) {
        // =-=-=-=-=-=-=-
        // JMC - backport 4597
        _resolveHostName( _ctx.comm(), resc_input[irods::RESOURCE_LOCATION].c_str());
    }

    // Root dir is not a valid vault path
    ret = verify_non_root_vault_path(_ctx, resc_input[irods::RESOURCE_PATH]);
    if (!ret.ok()) {
        return PASS(ret);
    }

    const auto [current_time_secs, current_time_msecs] = get_current_time();

    cllBindVars[0] = idNum;
    cllBindVars[1] = resc_input[irods::RESOURCE_NAME].c_str();
    cllBindVars[2] = ( char* )zone.c_str();
    cllBindVars[3] = resc_input[irods::RESOURCE_TYPE].c_str();
    cllBindVars[4] = resc_input[irods::RESOURCE_CLASS].c_str();
    cllBindVars[5] = resc_input[irods::RESOURCE_LOCATION].c_str();
    cllBindVars[6] = resc_input[irods::RESOURCE_PATH].c_str();
    cllBindVars[7] = current_time_secs.c_str();
    cllBindVars[8] = current_time_secs.c_str();
    cllBindVars[9] = current_time_msecs.c_str();
    cllBindVars[10] = resc_input[irods::RESOURCE_CHILDREN].c_str();
    cllBindVars[11] = resc_input[irods::RESOURCE_CONTEXT].c_str();
    cllBindVars[12] = resc_input[irods::RESOURCE_PARENT].c_str();
    cllBindVarCount = 13;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegResc SQL 4");
    }
    status = cmlExecuteNoAnswerSql("insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, "
                                   "resc_class_name, resc_net, resc_def_path, create_ts, modify_ts, modify_ts_millis, "
                                   "resc_children, resc_context, resc_parent) values (?,?,?,?,?,?,?,?,?,?,?,?,?)",
                                   &icss);

    if ( status != 0 ) {
        log_db::info("chlRegResc cmlExectuteNoAnswerSql(insert) failure {}", status);
        _rollback( "chlRegResc" );
        return ERROR( status, "cmlExectuteNoAnswerSql(insert) failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegResc cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return CODE( status );

} // db_reg_resc_op

// =-=-=-=-=-=-=-
//
irods::error db_del_child_resc_op(
    irods::plugin_context& _ctx,
    std::map<std::string, std::string> *_resc_input ) {

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_resc_input ) {
        return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "NULL parameter" );
    }

    // =-=-=-=-=-=-=-
    // for readability
    std::map<std::string, std::string>& resc_input = *_resc_input;

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    irods::sql_logger logger( "chlDelChildResc", logSQL );
    std::string child_string( resc_input[irods::RESOURCE_CHILDREN] );

    std::string& parent_name = resc_input[irods::RESOURCE_NAME];
    std::string parent_resource_id;
    std::string parent_parent_resource_id;
    ret = extract_resource_properties_for_operations(
              parent_name,
              parent_resource_id,
              parent_parent_resource_id);
    if(!ret.ok()) {
           return PASS(ret);
    }

    irods::children_parser parser;
    parser.set_string( child_string );

    std::string child_name;
    parser.first_child( child_name );

    std::string child_resource_id;
    std::string child_parent_resource_id;
    ret = extract_resource_properties_for_operations(
              child_name,
              child_resource_id,
              child_parent_resource_id);
    if(!ret.ok()) {
           return PASS(ret);
    }

    if (child_parent_resource_id != parent_resource_id) {
        return ERROR(CAT_INVALID_CHILD, "invalid parent/child relationship");
    }

    int status = _canConnectToCatalog( _ctx.comm() );
    if(0 != status) {
        return ERROR(
                   status,
                   "_canConnectToCatalog failed");
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if(!ret.ok()) {
        return PASS(ret);
    }

    ret = update_child_parent(child_resource_id, std::string(""), std::string(""));
    if(!ret.ok()) {
        return PASS(ret);
    }

    status = cmlExecuteNoAnswerSql( "commit", &icss );
    if(status != 0) {
        return ERROR(
                   status,
                   "commit failure");
    }

    return SUCCESS();

} // db_del_child_resc_op

// =-=-=-=-=-=-=-
// delete a resource
irods::error db_del_resc_op(
    irods::plugin_context& _ctx,
    const char *_resc_name,
    int                    _dry_run ) {

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_resc_name ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    int status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelResc");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4629
    if ( strncmp( _resc_name, BUNDLE_RESC, strlen( BUNDLE_RESC ) ) == 0 ) {
        char errMsg[155];
        snprintf( errMsg, 150,
                  "%s is a built-in resource needed for bundle operations.",
                  BUNDLE_RESC );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_PSEUDO_RESC_MODIFY_DISALLOWED, "cannot delete bundle resc" );
    }
    // =-=-=-=-=-=-=-

    bool has_data = true; // default to error case
    status = _rescHasData( &icss, _resc_name, has_data );
    if( status < 0 ) {
        log_db::error("{} - _rescHasData failed for [{}] {}", __FUNCTION__, _resc_name, status);
        return ERROR(
                  status,
                  "failed to get object count for resource" );
    }

    if( has_data   ) {
        char errMsg[105];
        snprintf( errMsg, 100,
                  "resource '%s' contains one or more dataObjects",
                  _resc_name );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_RESOURCE_NOT_EMPTY, "resc not empty" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelResc SQL 2 ");
    }
    char rescId[MAX_NAME_LEN]{};
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        status = cmlGetStringValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=?",
                     rescId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[105];
            snprintf( errMsg, 100,
                      "resource '%s' does not exist",
                      _resc_name );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( status, "resource does not exits" );
        }
        _rollback( "chlDelResc" );
        return ERROR( status, "resource does not exist" );
    }

    if ( _rescHasParentOrChild( rescId ) ) {
        char errMsg[105];
        snprintf( errMsg, 100,
                  "resource '%s' has a parent or child",
                  _resc_name );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CHILD_EXISTS, "resource has a parent or child" );
    }

    cllBindVars[cllBindVarCount++] = _resc_name;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelResc SQL 3");
    }
    status = cmlExecuteNoAnswerSql(
                 "delete from R_RESC_MAIN where resc_name=?",
                 &icss );
    if ( status != 0 ) {
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[105];
            snprintf( errMsg, 100,
                      "resource '%s' does not exist",
                      _resc_name );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( status, "resource does not exist" );
        }
        _rollback( "chlDelResc" );
        return ERROR( status, "resource does not exist" );
    }

    /* Remove associated AVUs, if any */
    if (const auto ec = removeMetaMapAndAVU(rescId); ec < 0) {
        irods::log(LOG_WARNING, fmt::format(
                   "[{}:{}] - failed to remove associated AVUs [ec=[{}]]",
                   __func__, __LINE__, ec));
    }

    if ( _dry_run ) { // JMC
        _rollback( "chlDelResc" );
        return CODE( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlDelResc cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }
    return CODE( status );

} // db_del_resc_op

// =-=-=-=-=-=-=-
// rollback the db
irods::error db_rollback_op(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRollback - SQL 1 ");
    }

    int status =  cmlExecuteNoAnswerSql( "rollback", &icss );
    if ( status != 0 ) {
        log_db::info("chlRollback cmlExecuteNoAnswerSql failure {}", status);
        return ERROR( status, "chlRollback cmlExecuteNoAnswerSql failure" );
    }

    return CODE( status );

} // db_rollback_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_commit_op(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    if ( logSQL != 0 ) {
        log_sql::debug("chlCommit - SQL 1 ");
    }
    int status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlCommit cmlExecuteNoAnswerSql failure {}", status);
        return ERROR( status, "chlCommit cmlExecuteNoAnswerSql failure" );
    }

    return CODE( status );

} // db_commit_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_del_user_re_op(
    irods::plugin_context& _ctx,
    userInfo_t*            _user_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_user_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char iValStr[200];
    char zoneToUse[MAX_NAME_LEN];
    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    snprintf( zoneToUse, sizeof( zoneToUse ), "%s", zone.c_str() );
    if ( strlen( _user_info->rodsZone ) > 0 ) {
        snprintf( zoneToUse, sizeof( zoneToUse ), "%s", _user_info->rodsZone );
    }

    status = validateAndParseUserName( _user_info->userName, userName2, zoneName );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( zoneName[0] != '\0' ) {
        rstrcpy( zoneToUse, zoneName, NAME_LEN );
    }

    if ( strncmp( _ctx.comm()->clientUser.userName, userName2, sizeof( userName2 ) ) == 0 &&
            strncmp( _ctx.comm()->clientUser.rodsZone, zoneToUse, sizeof( zoneToUse ) ) == 0 ) {
        addRErrorMsg( &_ctx.comm()->rError, 0, "Cannot remove your own admin account, probably unintended" );
        return ERROR( CAT_INVALID_USER, "invalid user" );
    }


    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName2 );
        bindVars.push_back( zoneToUse );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                     iValStr, 200, bindVars, &icss );
    }
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ||
            status == CAT_NO_ROWS_FOUND ) {
        addRErrorMsg( &_ctx.comm()->rError, 0, "Invalid user" );
        return ERROR( CAT_INVALID_USER, "invalid user" );
    }
    if ( status != 0 ) {
        _rollback( "chlDelUserRE" );
        return ERROR( status, "failed getting user from table" );
    }

    cllBindVars[cllBindVarCount++] = userName2;
    cllBindVars[cllBindVarCount++] = zoneToUse;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE SQL 2");
    }
    status = cmlExecuteNoAnswerSql(
                 "delete from R_USER_MAIN where user_name=? and zone_name=?",
                 &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        return ERROR( CAT_INVALID_USER, "invalid user" );
    }
    if ( status != 0 ) {
        _rollback( "chlDelUserRE" );
        return ERROR( status, "cmlExecuteNoAnswerSql for delete user failed" );
    }

    cllBindVars[cllBindVarCount++] = iValStr;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE SQL 3");
    }
    status = cmlExecuteNoAnswerSql(
                 "delete from R_USER_PASSWORD where user_id=?",
                 &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        char errMsg[MAX_NAME_LEN + 40];
        log_db::info("chlDelUserRE delete password failure {}", status);
        snprintf( errMsg, sizeof errMsg, "Error removing password entry" );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        _rollback( "chlDelUserRE" );
        return ERROR( status, "Error removing password entry" );
    }

    /* Remove both the special user_id = group_user_id entry and any
       other access entries for this user (or group) */
    cllBindVars[cllBindVarCount++] = iValStr;
    cllBindVars[cllBindVarCount++] = iValStr;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE SQL 4");
    }
    status = cmlExecuteNoAnswerSql(
                 "delete from R_USER_GROUP where user_id=? or group_user_id=?",
                 &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        char errMsg[MAX_NAME_LEN + 40];
        log_db::info("chlDelUserRE delete user_group entry failure {}", status);
        snprintf( errMsg, sizeof errMsg, "Error removing user_group entry" );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        _rollback( "chlDelUserRE" );
        return ERROR( status, "Error removing user_group entry" );
    }

    /* Remove any R_USER_AUTH rows for this user */
    cllBindVars[cllBindVarCount++] = iValStr;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelUserRE SQL 4");
    }
    status = cmlExecuteNoAnswerSql(
                 "delete from R_USER_AUTH where user_id=?",
                 &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        char errMsg[MAX_NAME_LEN + 40];
        log_db::info("chlDelUserRE delete user_auth entries failure {}", status);
        snprintf( errMsg, sizeof errMsg, "Error removing user_auth entries" );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        _rollback( "chlDelUserRE" );
        return ERROR( status, "Error removing user_auth entries" );
    }

    /* Remove associated AVUs, if any */
    if (const auto ec = removeMetaMapAndAVU(iValStr); ec < 0) {
        irods::log(LOG_WARNING, fmt::format(
                   "[{}:{}] - failed to remove associated AVUs [ec=[{}]]",
                   __func__, __LINE__, ec));
    }

    return SUCCESS();

} // db_del_user_re_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_reg_coll_by_admin_op(
    irods::plugin_context& _ctx,
    collInfo_t*            _coll_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_coll_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t iVal;
    char collIdNum[MAX_NAME_LEN];
    char nextStr[MAX_NAME_LEN];
    char currStr[MAX_NAME_LEN];
    char currStr2[MAX_SQL_SIZE];
    int status;
    char tSQL[MAX_SQL_SIZE];
    char userName[NAME_LEN];
    char zoneName[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegCollByAdmin");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
            _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
                       _ctx.comm()->clientUser.userName,
                       _ctx.comm()->clientUser.rodsZone,
                       "", &icss );
        if ( status2 != 0 ) {
            return ERROR( status2, "no group admin access" );
        }
        if ( creatingUserByGroupAdmin == 0 ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        // =-=-=-=-=-=-=-
    }

    if (const auto ec = splitPathByKey(_coll_info->collName, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _coll_info->collName, ec));
    }

    if ( strlen( logicalParentDirName ) == 0 ) {
        snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
        snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
    }

    /* Check that the parent collection exists */
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegCollByAdmin SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( logicalParentDirName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_name=?",
                     &iVal, bindVars, &icss );
    }
    if ( status < 0 ) {
        char errMsg[MAX_NAME_LEN + 40];
        if ( status == CAT_NO_ROWS_FOUND ) {
            snprintf( errMsg, sizeof errMsg,
                      "collection '%s' is unknown, cannot create %s under it",
                      logicalParentDirName, logicalEndName );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( status, "collection is unknown" );
        }
        _rollback( "chlRegCollByAdmin" );
        return ERROR( status, "collection not found" );
    }

    snprintf( collIdNum, MAX_NAME_LEN, "%d", status );

    /* String to get next sequence item for objects */
    cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegCollByAdmin SQL 2");
    }
    snprintf( tSQL, MAX_SQL_SIZE,
              "insert into R_COLL_MAIN (coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_type, coll_info1, coll_info2, create_ts, modify_ts) values (%s, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
              nextStr );

    getNowStr( myTime );

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    /* Parse input name into user and zone */
    status = validateAndParseUserName( _coll_info->collOwnerName, userName, zoneName );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( zoneName[0] == '\0' ) {
        rstrcpy( zoneName, zone.c_str(), NAME_LEN );
    }

    cllBindVars[cllBindVarCount++] = logicalParentDirName;
    cllBindVars[cllBindVarCount++] = _coll_info->collName;
    cllBindVars[cllBindVarCount++] = userName;
    if ( strlen( _coll_info->collOwnerZone ) > 0 ) {
        cllBindVars[cllBindVarCount++] = _coll_info->collOwnerZone;
    }
    else {
        cllBindVars[cllBindVarCount++] = zoneName;
    }
    cllBindVars[cllBindVarCount++] = _coll_info->collType;
    cllBindVars[cllBindVarCount++] = _coll_info->collInfo1;
    cllBindVars[cllBindVarCount++] = _coll_info->collInfo2;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegCollByAdmin SQL 3");
    }
    status =  cmlExecuteNoAnswerSql( tSQL,
                                     &icss );
    if ( status != 0 ) {
        char errMsg[105];
        if ( status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
            snprintf( errMsg, 100, "Error %d %s",
                      status,
                      "CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME"
                    );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        }

        log_db::info("chlRegCollByAdmin cmlExecuteNoAnswerSQL(insert) failure {}", status);
        _rollback( "chlRegCollByAdmin" );
        return ERROR( status, "cmlExecuteNoAnswerSQL(insert) failure" );
    }

    /* String to get current sequence item for objects */
    cllCurrentValueString( "R_ObjectID", currStr, MAX_NAME_LEN );
    snprintf( currStr2, MAX_SQL_SIZE, " %s ", currStr );

    cllBindVars[cllBindVarCount++] = userName;
    cllBindVars[cllBindVarCount++] = zoneName;
    cllBindVars[cllBindVarCount++] = ACCESS_OWN;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    snprintf( tSQL, MAX_SQL_SIZE,
              "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
              currStr2 );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegCollByAdmin SQL 4");
    }
    status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    if ( status != 0 ) {
        log_db::info("chlRegCollByAdmin cmlExecuteNoAnswerSql(insert access) failure {}", status);
        _rollback( "chlRegCollByAdmin" );
        return ERROR( status, "cmlExecuteNoAnswerSql(insert access) failure" );
    }

    return SUCCESS();

} // db_reg_coll_by_admin_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_reg_coll_op(
    irods::plugin_context& _ctx,
    collInfo_t*            _coll_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_coll_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t iVal;
    char collIdNum[MAX_NAME_LEN];
    char nextStr[MAX_NAME_LEN];
    char currStr[MAX_NAME_LEN];
    char currStr2[MAX_SQL_SIZE];
    rodsLong_t status;
    char tSQL[MAX_SQL_SIZE];
    int inheritFlag;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegColl");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if (const auto ec = splitPathByKey(_coll_info->collName, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _coll_info->collName, ec));
    }

    if ( strlen( logicalParentDirName ) == 0 ) {
        snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
        snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
    }

    /* Check that the parent collection exists and user has write permission,
       and get the collectionID.  Also get the inherit flag */
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegColl SQL 1 ");
    }

    status = cmlCheckDirAndGetInheritFlag( logicalParentDirName,
                                           _ctx.comm()->clientUser.userName,
                                           _ctx.comm()->clientUser.rodsZone,
                                           ACCESS_MODIFY_OBJECT, &inheritFlag,
                                           mySessionTicket, mySessionClientAddr, &icss );
    if ( status < 0 ) {
        char errMsg[105];
        if ( status == CAT_UNKNOWN_COLLECTION ) {
            snprintf( errMsg, 100, "collection '%s' is unknown",
                      logicalParentDirName );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( status, "collection is unknown" );
        }
        _rollback( "chlRegColl" );
        return ERROR( status, "cmlCheckDirAndGetInheritFlag failed" );
    }
    snprintf( collIdNum, MAX_NAME_LEN, "%lld", status );

    /* Check that the path is not already a dataObj */
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegColl SQL 2");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( logicalEndName );
        bindVars.push_back( collIdNum );
        status = cmlGetIntegerValueFromSql(
                     "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
                     &iVal, bindVars, &icss );
    }

    if ( status == 0 ) {
        return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "data obj already exists" );
    }


    /* String to get next sequence item for objects */
    cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

    getNowStr( myTime );

    cllBindVars[cllBindVarCount++] = logicalParentDirName;
    cllBindVars[cllBindVarCount++] = _coll_info->collName;
    cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.userName;
    cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.rodsZone;
    cllBindVars[cllBindVarCount++] = _coll_info->collType;
    cllBindVars[cllBindVarCount++] = _coll_info->collInfo1;
    cllBindVars[cllBindVarCount++] = _coll_info->collInfo2;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegColl SQL 3");
    }
    snprintf( tSQL, MAX_SQL_SIZE,
              "insert into R_COLL_MAIN (coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_type, coll_info1, coll_info2, create_ts, modify_ts) values (%s, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
              nextStr );
    status =  cmlExecuteNoAnswerSql( tSQL,
                                     &icss );
    if ( status != 0 ) {
        log_db::info("chlRegColl cmlExecuteNoAnswerSql(insert) failure {}", status);
        _rollback( "chlRegColl" );
        return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
    }

    /* String to get current sequence item for objects */
    cllCurrentValueString( "R_ObjectID", currStr, MAX_NAME_LEN );
    snprintf( currStr2, MAX_SQL_SIZE, " %s ", currStr );

    if ( inheritFlag ) {
        /* If inherit is set (sticky bit), then add access rows for this
           collection that match those of the parent collection */
        cllBindVars[0] = myTime;
        cllBindVars[1] = myTime;
        cllBindVars[2] = collIdNum;
        cllBindVarCount = 3;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegColl SQL 4");
        }
        snprintf( tSQL, MAX_SQL_SIZE,
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select %s, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
                  currStr2 );
        status =  cmlExecuteNoAnswerSql( tSQL, &icss );

        if ( status == 0 ) {
            if ( logSQL != 0 ) {
                log_sql::debug("chlRegColl SQL 5");
            }
#if ORA_ICAT
            char newCollectionID[MAX_NAME_LEN];
            /*
               For Oracle, we can't use currStr2 string in a where clause so
               do another query to get the new collection id.
            */
            status = cmlGetCurrentSeqVal( &icss );

            if ( status > 0 ) {
                /* And then use it in the where clause for the update */
                snprintf( newCollectionID, MAX_NAME_LEN, "%lld", status );
                cllBindVars[cllBindVarCount++] = "1";
                cllBindVars[cllBindVarCount++] = myTime;
                cllBindVars[cllBindVarCount++] = newCollectionID;
                status =  cmlExecuteNoAnswerSql(
                              "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=?",
                              &icss );
            }
#else
            /*
              For Postgres we can, use the currStr2 to get the current id
              and save a SQL interaction.
            */
            cllBindVars[cllBindVarCount++] = "1";
            cllBindVars[cllBindVarCount++] = myTime;
            snprintf( tSQL, MAX_SQL_SIZE,
                      "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=(select %s)",
                      currStr2 );
            status =  cmlExecuteNoAnswerSql( tSQL, &icss );
#endif
        }
    }
    else {
        cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.userName;
        cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[cllBindVarCount++] = ACCESS_OWN;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        snprintf( tSQL, MAX_SQL_SIZE,
                  "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                  currStr2 );
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegColl SQL 6");
        }
        status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    }
    if ( status != 0 ) {
        log_db::info("chlRegColl cmlExecuteNoAnswerSql(insert access) failure {}", status);
        _rollback( "chlRegColl" );
        return ERROR( status, "cmlExecuteNoAnswerSql(insert access) failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegColl cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return CODE( status );

} // db_reg_coll_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_mod_coll_op(
    irods::plugin_context& _ctx,
    collInfo_t*            _coll_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_coll_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    rodsLong_t status;
    int count;
    rodsLong_t iVal;

    if ( logSQL != 0 ) {
        log_sql::debug("chlModColl");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    /* Check that collection exists and user has write permission */
    iVal = cmlCheckDir( _coll_info->collName,  _ctx.comm()->clientUser.userName,
                        _ctx.comm()->clientUser.rodsZone,
                        ACCESS_MODIFY_OBJECT, &icss );

    if ( iVal < 0 ) {
        if ( iVal == CAT_UNKNOWN_COLLECTION ) {
            std::stringstream errMsg;
            errMsg << "collection '" << _coll_info->collName << "' is unknown";
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg.str().c_str() );
            return ERROR( CAT_UNKNOWN_COLLECTION, "unknown collection" );
        }

        if ( iVal == CAT_NO_ACCESS_PERMISSION ) {
            // Allows elevation of privileges (e.g. irods_rule_engine_plugin-update_collection_mtime).
            if (irods::is_privileged_client(*_ctx.comm())) {
                iVal = 0;
            }
            else {
                std::stringstream errMsg;
                errMsg << "no permission to update collection '" << _coll_info->collName << "'";
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg.str().c_str() );
                return ERROR( CAT_NO_ACCESS_PERMISSION, "no permission" );
            }
        }

        // If client privileges are elevated, then iVal must be checked again because
        // it could have been modified (e.g. irods_rule_engine_plugin-update_collection_mtime).
        if (iVal < 0) {
            return ERROR( iVal, "cmlCheckDir failed" );
        }
    }

    std::string tSQL( "update R_COLL_MAIN set " );
    count = 0;

    if ( strlen( _coll_info->collType ) > 0 ) {
        if ( strcmp( _coll_info->collType, "NULL_SPECIAL_VALUE" ) == 0 ) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++] = "";
        }
        else {
            cllBindVars[cllBindVarCount++] = _coll_info->collType;
        }
        tSQL += "coll_type=? ";
        count++;
    }

    if ( strlen( _coll_info->collInfo1 ) > 0 ) {
        if ( strcmp( _coll_info->collInfo1, "NULL_SPECIAL_VALUE" ) == 0 ) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++] = "";
        }
        else {
            cllBindVars[cllBindVarCount++] = _coll_info->collInfo1;
        }
        if ( count > 0 ) {
            tSQL += ",";
        }
        tSQL += "coll_info1=? ";
        count++;
    }

    if ( strlen( _coll_info->collInfo2 ) > 0 ) {
        if ( strcmp( _coll_info->collInfo2, "NULL_SPECIAL_VALUE" ) == 0 ) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++] = "";
        }
        else {
            cllBindVars[cllBindVarCount++] = _coll_info->collInfo2;
        }
        if ( count > 0 ) {
            tSQL += ",";
        }
        tSQL += "coll_info2=? ";
        count++;
    }

    if (strlen(_coll_info->collModify) > 0) {
        cllBindVars[cllBindVarCount++] = _coll_info->collModify;

        if (count > 0) {
            tSQL += ',';
        }

        ++count;
    }
    else {
        tSQL += ',';
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = myTime;
    }

    if ( count == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "count is 0" );
    }

    cllBindVars[cllBindVarCount++] = _coll_info->collName;
    tSQL += " modify_ts=? where coll_name=?";

    if ( logSQL != 0 ) {
        log_sql::debug("chlModColl SQL 1");
    }
    status =  cmlExecuteNoAnswerSql( tSQL.c_str(),
                                     &icss );
    if ( status != 0 ) {
        log_db::info("chlModColl cmlExecuteNoAnswerSQL(update) failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSQL(update) failure" );
    }

    return SUCCESS();

} // db_mod_coll_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_reg_zone_op(
    irods::plugin_context& _ctx,
    const char*            _zone_name,
    const char*            _zone_type,
    const char*            _zone_conn_info,
    const char*            _zone_comment ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_zone_name ||
        !_zone_type ||
        !_zone_conn_info ||
        !_zone_comment ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char nextStr[MAX_NAME_LEN];
    char tSQL[MAX_SQL_SIZE];
    int status;
    char myTime[50];

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegZone");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( strncmp( _zone_type, "remote", 6 ) != 0 ) {
        addRErrorMsg( &_ctx.comm()->rError, 0,
                      "Currently, only zones of type 'remote' are allowed" );
        return ERROR( CAT_INVALID_ARGUMENT, "Currently, only zones of type 'remote' are allowed" );
    }

    // =-=-=-=-=-=-=-
    // validate the zone name does not include improper characters
    ret = validate_zone_name( _zone_name );
    if ( !ret.ok() ) {
        irods::log( ret );
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // validate the connection string is well formed
    ret = validate_zone_connection_string(_zone_conn_info, _ctx);
    if (!ret.ok()) {
        return ret;
    }

    /* String to get next sequence item for objects */
    cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

    getNowStr( myTime );

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegZone SQL 1 ");
    }
    cllBindVars[cllBindVarCount++] = _zone_name;
    cllBindVars[cllBindVarCount++] = _zone_conn_info;
    cllBindVars[cllBindVarCount++] = _zone_comment;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    snprintf( tSQL, MAX_SQL_SIZE,
              "insert into R_ZONE_MAIN (zone_id, zone_name, zone_type_name, zone_conn_string, r_comment, create_ts, modify_ts) values (%s, ?, 'remote', ?, ?, ?, ?)",
              nextStr );
    status =  cmlExecuteNoAnswerSql( tSQL,
                                     &icss );
    if ( status != 0 ) {
        log_db::info("chlRegZone cmlExecuteNoAnswerSql(insert) failure {}", status);
        _rollback( "chlRegZone" );
        return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegZone cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();

} // db_reg_zone_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_mod_zone_op(
    irods::plugin_context& _ctx,
    const char*            _zone_name,
    const char*            _option,
    const char*            _option_value ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_zone_name ||
        !_option ||
        !_option_value ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status, OK;
    char myTime[50];
    char zoneId[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlModZone");
    }

    if ( *_zone_name == '\0' || *_option == '\0' || *_option_value == '\0' ) {
        return  ERROR( CAT_INVALID_ARGUMENT, "invalid argument value" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    zoneId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlModZone SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _zone_name );
        status = cmlGetStringValueFromSql(
                     "select zone_id from R_ZONE_MAIN where zone_name=?",
                     zoneId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_ZONE, "invalid zone name" );
        }
        return ERROR( status, "error getting zone" );
    }

    getNowStr( myTime );
    OK = 0;
    if ( strcmp( _option, "comment" ) == 0 ) {
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = zoneId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModZone SQL 3");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_ZONE_MAIN set r_comment = ?, modify_ts=? where zone_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlModZone cmlExecuteNoAnswerSql update failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }
        OK = 1;
    }
    if (strcmp(_option, "conn") == 0) {
        // =-=-=-=-=-=-=-
        // validate the connection string is well formed
        ret = validate_zone_connection_string(_option_value, _ctx);
        if (!ret.ok()) {
            return ret;
        }

        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = zoneId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModZone SQL 5");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_ZONE_MAIN set zone_conn_string = ?, modify_ts=? where zone_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlModZone cmlExecuteNoAnswerSql update failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }
        OK = 1;
    }
    if ( strcmp( _option, "name" ) == 0 ) {
        if ( strcmp( _zone_name, zone.c_str() ) == 0 ) {
            addRErrorMsg( &_ctx.comm()->rError, 0,
                          "It is not valid to rename the local zone via chlModZone; iadmin should use acRenameLocalZone" );
            return ERROR( CAT_INVALID_ARGUMENT, "cannot rename localzone" );
        }

        // =-=-=-=-=-=-=-
        // validate the zone name does not include improper characters
        ret = validate_zone_name( _option_value );
        if ( !ret.ok() ) {
            irods::log( ret );
            std::string msg( "zone name is invalid [" );
            msg += _option_value;
            msg += "]";
            addRErrorMsg( &_ctx.comm()->rError, 0,
                          msg.c_str() );
            return PASS( ret );
        }

        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = zoneId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModZone SQL 5");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_ZONE_MAIN set zone_name = ?, modify_ts=? where zone_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlModZone cmlExecuteNoAnswerSql update failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }
        OK = 1;
    }
    if ( OK == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModZone cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();

} // db_mod_zone_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_rename_coll_op(
    irods::plugin_context& _ctx,
    const char*            _old_coll,
    const char*            _new_coll ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_old_coll ||
        !_new_coll ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    rodsLong_t status1;

    /* See if the input path is a collection and the user owns it,
       and, if so, get the collectionID */
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameColl SQL 1 ");
    }

    status1 = cmlCheckDir( _old_coll,
                           _ctx.comm()->clientUser.userName,
                           _ctx.comm()->clientUser.rodsZone,
                           ACCESS_OWN,
                           &icss );

    if ( status1 < 0 ) {
        return ERROR( status1, "cmlCheckDir failed" );
    }

    /* call chlRenameObject to rename */
    status = chlRenameObject( _ctx.comm(), status1, _new_coll );
    if (status != 0) {
        return ERROR( status, "chlRenameObject failed" );
    }

    return CODE( status );

} // db_rename_coll_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_mod_zone_coll_acl_op(
    irods::plugin_context& _ctx,
    const char*            _access_level,
    const char*            _user_name,
    const char*            _path_name ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_access_level ||
        !_user_name ||
        !_path_name ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    if ( *_path_name != '/' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid path name" );
    }
    const char* cp = _path_name + 1;
    if ( strstr( cp, PATH_SEPARATOR ) != NULL ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid path name" );
    }
    status =  chlModAccessControl( _ctx.comm(), 0,
                                   _access_level,
                                   _user_name,
                                   _ctx.comm()->clientUser.rodsZone,
                                   _path_name );
    if ( !status ) {
        return ERROR( status, "chlModAccessControl failed" );
    }

    return CODE( status );

} // db_mod_zone_coll_acl_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_rename_local_zone_op(
    irods::plugin_context& _ctx,
    const char*            _old_zone,
    const char*            _new_zone ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_old_zone ||
        !_new_zone ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char zoneId[MAX_NAME_LEN];
    char myTime[50];

    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 1 ");
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( strcmp( zone.c_str(), _old_zone ) != 0 ) { /* not the local zone */
        return ERROR( CAT_INVALID_ARGUMENT, "not the local zone" );
    }

    /* check that the new zone does not exist */
    zoneId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 2 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _new_zone );
        status = cmlGetStringValueFromSql(
                     "select zone_id from R_ZONE_MAIN where zone_name=?",
                     zoneId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != CAT_NO_ROWS_FOUND ) {
        return ERROR( CAT_INVALID_ZONE, "zone not found" );
    }

    getNowStr( myTime );

    /* update coll_owner_zone in R_COLL_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 3 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_COLL_MAIN set coll_owner_zone = ?, modify_ts=? where coll_owner_zone=?",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    /* update data_owner_zone in R_DATA_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 4 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_DATA_MAIN set data_owner_zone = ?, modify_ts=? where data_owner_zone=?",
                  &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    /* update zone_name in R_RESC_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 5 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_RESC_MAIN set zone_name = ?, modify_ts=? where zone_name=?",
                  &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    /* update rule_owner_zone in R_RULE_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 6 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_MAIN set rule_owner_zone=?, modify_ts=? where rule_owner_zone=?",
                  &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    /* update zone_name in R_USER_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 7 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_USER_MAIN set zone_name=?, modify_ts=? where zone_name=?",
                  &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    /* update zone_name in R_ZONE_MAIN */
    cllBindVars[cllBindVarCount++] = _new_zone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _old_zone;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameLocalZone SQL 8 ");
    }
    status =  cmlExecuteNoAnswerSql(
                  "update R_ZONE_MAIN set zone_name=?, modify_ts=? where zone_name=?",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRenameLocalZone cmlExecuteNoAnswerSql update failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
    }

    return SUCCESS();

} // db_rename_local_zone_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_del_zone_op(
    irods::plugin_context& _ctx,
    const char*            _zone_name ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_zone_name ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char zoneType[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelZone");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelZone SQL 1 ");
    }

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _zone_name );
        status = cmlGetStringValueFromSql(
                     "select zone_type_name from R_ZONE_MAIN where zone_name=?",
                     zoneType, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_ZONE, "invalid zone name" );
        }
        return ERROR( status, "failed to get zone" );
    }

    if ( strcmp( zoneType, "remote" ) != 0 ) {
        addRErrorMsg( &_ctx.comm()->rError, 0,
                      "It is not permitted to remove the local zone" );
        return ERROR( CAT_INVALID_ARGUMENT, "cannot remove local zone" );
    }

    cllBindVars[cllBindVarCount++] = _zone_name;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelZone 2");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_ZONE_MAIN where zone_name = ?",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlDelZone cmlExecuteNoAnswerSql delete failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql delete failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlDelZone cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
    }

    return SUCCESS();

} // db_del_zone_op

// =-=-=-=-=-=-=-
// modify the zone
irods::error db_simple_query_op_vector(
    irods::plugin_context& _ctx,
    const char*                  _sql,
    std::vector<std::string> _bindVars,
    int                    _format,
    int*                   _control,
    char*                  _out_buf,
    int                    _max_out_buf ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int stmtNum = UNINITIALIZED_STATEMENT_NUMBER, status, nCols, i, needToGet, didGet;
    int rowSize;
    int rows;
    int OK;

    char *allowedSQL[] = {
        "select token_name from R_TOKN_MAIN where token_namespace = 'token_namespace'",
        "select token_name from R_TOKN_MAIN where token_namespace = ?",
        "select * from R_TOKN_MAIN where token_namespace = ? and token_name like ?",
        "select resc_name from R_RESC_MAIN",
        "select * from R_RESC_MAIN where resc_name=?",
        "select zone_name from R_ZONE_MAIN",
        "select * from R_ZONE_MAIN where zone_name=?",
        "select user_name from R_USER_MAIN where user_type_name='rodsgroup'",
        "select user_name||'#'||zone_name from R_USER_MAIN, R_USER_GROUP where R_USER_GROUP.user_id=R_USER_MAIN.user_id and R_USER_GROUP.group_user_id=(select user_id from R_USER_MAIN where user_name=?)",
        "select * from R_DATA_MAIN where data_id=?",
        "select data_name, data_id, data_repl_num from R_DATA_MAIN where coll_id =(select coll_id from R_COLL_MAIN where coll_name=?)",
        "select coll_name from R_COLL_MAIN where parent_coll_name=?",
        "select * from R_USER_MAIN where user_name=?",
        "select user_name||'#'||zone_name from R_USER_MAIN where user_type_name != 'rodsgroup'",
        "select R_RESC_GROUP.resc_group_name, R_RESC_GROUP.resc_id, resc_name, R_RESC_GROUP.create_ts, R_RESC_GROUP.modify_ts from R_RESC_MAIN, R_RESC_GROUP where R_RESC_MAIN.resc_id = R_RESC_GROUP.resc_id and resc_group_name=?",
        "select distinct resc_group_name from R_RESC_GROUP",
        "select coll_id from R_COLL_MAIN where coll_name = ?",
        "select * from R_USER_MAIN where user_name=? and zone_name=?",
        "select user_name from R_USER_MAIN where zone_name=? and user_type_name != 'rodsgroup'",
        "select zone_name from R_ZONE_MAIN where zone_type_name=?",
        "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_MAIN.user_name=?",
        "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_MAIN.user_name=? and R_USER_MAIN.zone_name=?",
        "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id",
        "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_AUTH.user_auth_name=?",
        "select user_name, R_USER_MAIN.zone_name, resc_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_RESC_MAIN.resc_id = R_QUOTA_MAIN.resc_id",
        "select user_name, R_USER_MAIN.zone_name, resc_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_RESC_MAIN.resc_id = R_QUOTA_MAIN.resc_id and user_name=? and R_USER_MAIN.zone_name=?",
        "select user_name, R_USER_MAIN.zone_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_QUOTA_MAIN.resc_id = 0",
        "select user_name, R_USER_MAIN.zone_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_QUOTA_MAIN.resc_id = 0 and user_name=? and R_USER_MAIN.zone_name=?",
        ""
    };
    // log_db::info( "JMC :: sql - {}", sql );
    if ( logSQL != 0 ) {
        log_sql::debug("chlSimpleQuery");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    /* check that the input sql is one of the allowed forms */
    OK = 0;
    for ( i = 0;; i++ ) {
        if ( strlen( allowedSQL[i] ) < 1 ) {
            break;
        }
        if ( strcasecmp( allowedSQL[i], _sql ) == 0 ) {
            OK = 1;
            break;
        }
    }

    if ( OK == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "query not found" );
    }

    /* done with multiple log calls so that each form will be checked
       via checkIcatLog.pl */
    if ( i == 0 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 1 ");
    }
    if ( i == 1 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 2");
    }
    if ( i == 2 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 3");
    }
    if ( i == 3 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 4");
    }
    if ( i == 4 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 5");
    }
    if ( i == 5 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 6");
    }
    if ( i == 6 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 7");
    }
    if ( i == 7 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 8");
    }
    if ( i == 8 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 9");
    }
    if ( i == 9 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 10");
    }
    if ( i == 10 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 11");
    }
    if ( i == 11 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 12");
    }
    if ( i == 12 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 13");
    }
    if ( i == 13 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 14");
    }
    // if (i==14 && logSQL) log_sql::debug( "chlSimpleQuery S Q L 15");
    // if (i==15 && logSQL) log_sql::debug( "chlSimpleQuery S Q L 16");
    // if (i==16 && logSQL) log_sql::debug( "chlSimpleQuery S Q L 17");
    if ( i == 17 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 18");
    }
    if ( i == 18 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 19");
    }
    if ( i == 19 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 20");
    }
    if ( i == 20 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 21");
    }
    if ( i == 21 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 22");
    }
    if ( i == 22 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 23");
    }
    if ( i == 23 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 24");
    }
    if ( i == 24 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 25");
    }
    if ( i == 25 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 26");
    }
    if ( i == 26 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 27");
    }
    if ( i == 27 && logSQL ) {
        log_sql::debug("chlSimpleQuery SQL 28");
    }

    _out_buf[0] = '\0';
    needToGet = 1;
    didGet = 0;
    rowSize = 0;
    rows = 0;
    if ( *_control == 0 ) {
        status = cmlGetFirstRowFromSqlBV( _sql, _bindVars,
                                          &stmtNum, &icss );
        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                log_db::info("chlSimpleQuery cmlGetFirstRowFromSqlBV failure {}", status);
            }
            cmlFreeStatement(stmtNum, &icss);
            return ERROR( status, "cmlGetFirstRowFromSqlBV failure" );
        }
        didGet = 1;
        needToGet = 0;
        *_control = stmtNum + 1; /* control is always > 0 */
    }
    else {
        stmtNum = *_control - 1;
    }

    for ( ;; ) {
        if ( needToGet ) {
            status = cmlGetNextRowFromStatement( stmtNum, &icss );
            if ( status == CAT_NO_ROWS_FOUND ) {
                *_control = 0;
                if ( didGet ) {
                    if ( _format == 2 ) {
                        i = strlen( _out_buf );
                        _out_buf[i - 1] = '\0'; /* remove the last CR */
                    }
                    return CODE( 0 );
                }
                cmlFreeStatement(stmtNum, &icss);
                return ERROR( status, "cmlGetNextRowFromStatement failed" );
            }
            if ( status < 0 ) {
                log_db::info("chlSimpleQuery cmlGetNextRowFromStatement failure {}", status);
                return ERROR( status, "cmlGetNextRowFromStatement failure" );
            }
            *_control = stmtNum + 1; /* control is always > 0 */
            didGet = 1;
        }
        needToGet = 1;
        nCols = icss.stmtPtr[stmtNum]->numOfCols;
        if ( rows == 0 && _format == 3 ) {
            for ( i = 0; i < nCols ; i++ ) {
                rstrcat( _out_buf, icss.stmtPtr[stmtNum]->resultColName[i], _max_out_buf );
                if ( i != nCols - 1 ) {
                    rstrcat( _out_buf, " ", _max_out_buf );
                }
            }
        }
        rows++;
        for ( i = 0; i < nCols ; i++ ) {
            if ( _format == 1 || _format == 3 ) {
                if ( strlen( icss.stmtPtr[stmtNum]->resultValue[i] ) == 0 ) {
                    rstrcat( _out_buf, "- ", _max_out_buf );
                }
                else {
                    rstrcat( _out_buf, icss.stmtPtr[stmtNum]->resultValue[i],
                             _max_out_buf );
                    if ( i != nCols - 1 ) {
                        /* add a space except for the last column */
                        rstrcat( _out_buf, " ", _max_out_buf );
                    }
                }
            }
            if ( _format == 2 ) {
                rstrcat( _out_buf, icss.stmtPtr[stmtNum]->resultColName[i], _max_out_buf );
                rstrcat( _out_buf, ": ", _max_out_buf );
                rstrcat( _out_buf, icss.stmtPtr[stmtNum]->resultValue[i], _max_out_buf );
                rstrcat( _out_buf, "\n", _max_out_buf );
            }
        }
        rstrcat( _out_buf, "\n", _max_out_buf );
        if ( rowSize == 0 ) {
            rowSize = strlen( _out_buf );
        }
        if ( ( int ) strlen( _out_buf ) + rowSize + 20 > _max_out_buf ) {
            return CODE( 0 ); /* success so far, but more rows available */
        }
    }

    cmlFreeStatement(stmtNum, &icss);
    return SUCCESS();

} // db_simple_query_op_vector

irods::error db_simple_query_op(
    irods::plugin_context& _ctx,
    const char*            _sql,
    const char*            _arg1,
    const char*            _arg2,
    const char*            _arg3,
    const char*            _arg4,
    int                    _format,
    int*                   _control,
    char*                  _out_buf,
    int                    _max_out_buf ) {

    std::vector<std::string> bindVars;
    if ( _arg1 != NULL && _arg1[0] != '\0' ) {
        bindVars.push_back( _arg1 );
        if ( _arg2 != NULL && _arg2[0] != '\0' ) {
            bindVars.push_back( _arg2 );
            if ( _arg3 != NULL && _arg3[0] != '\0' ) {
                bindVars.push_back( _arg3 );
                if ( _arg4 != NULL && _arg4[0] != '\0' ) {
                    bindVars.push_back( _arg4 );
                }
            }
        }
    }
    return db_simple_query_op_vector( _ctx, _sql, bindVars, _format, _control, _out_buf, _max_out_buf );
} // db_simple_query_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_del_coll_by_admin_op(
    irods::plugin_context& _ctx,
    collInfo_t*            _coll_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_coll_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    rodsLong_t iVal;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    char collIdNum[MAX_NAME_LEN];
    int status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelCollByAdmin");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if (const auto ec = splitPathByKey(_coll_info->collName, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _coll_info->collName, ec));
    }

    if ( strlen( logicalParentDirName ) == 0 ) {
        snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
        snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
    }

    /* check that the collection is empty (both subdirs and files) */
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelCollByAdmin SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _coll_info->collName );
        bindVars.push_back( _coll_info->collName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where parent_coll_name=? union select coll_id from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
                     &iVal, bindVars, &icss );
    }

    if ( status != CAT_NO_ROWS_FOUND ) {
        if ( status == 0 ) {
            char errMsg[105];
            snprintf( errMsg, 100, "collection '%s' is not empty",
                      _coll_info->collName );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( CAT_COLLECTION_NOT_EMPTY, "collection not empty" );
        }
        _rollback( "chlDelCollByAdmin" );
        return ERROR( status, "failed to get collection" );
    }

    /* remove any access rows */
    cllBindVars[cllBindVarCount++] = _coll_info->collName;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelCollByAdmin SQL 2");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_OBJT_ACCESS where object_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
                  &icss );
    if ( status != 0 ) {
        /* error, but let it fall thru to below, probably doesn't exist */
        log_db::info("chlDelCollByAdmin delete access failure {}", status);
        _rollback( "chlDelCollByAdmin" );
    }

    /* Remove associated AVUs, if any */
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelCollByAdmin SQL 3 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _coll_info->collName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_name=?",
                     &iVal, bindVars, &icss );
    }

    if ( status != 0 ) {
        _rollback( "db_del_coll_by_admin_op" );
        std::stringstream msg;
        msg << "db_del_coll_by_admin_op: should be exactly one collection id corresponding to collection name ["
            << _coll_info->collName
            << "]. status ["
            << status
            << "]";
        return ERROR( status, msg.str().c_str() );
    }

    snprintf( collIdNum, MAX_NAME_LEN, "%lld", iVal );
    if (const auto ec = removeMetaMapAndAVU(collIdNum); ec < 0) {
        irods::log(LOG_WARNING, fmt::format(
                   "[{}:{}] - failed to remove associated AVUs [ec=[{}]]",
                   __func__, __LINE__, ec));
    }

    /* delete the row if it exists */
    cllBindVars[cllBindVarCount++] = _coll_info->collName;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelCollByAdmin SQL 4");
    }
    status =  cmlExecuteNoAnswerSql( "delete from R_COLL_MAIN where coll_name=?",
                                     &icss );

    if ( status != 0 ) {
        char errMsg[105];
        snprintf( errMsg, 100, "collection '%s' is unknown",
                  _coll_info->collName );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        _rollback( "chlDelCollByAdmin" );
        return ERROR( CAT_UNKNOWN_COLLECTION, "unknown collection" );
    }

    return SUCCESS();

} // db_del_coll_by_admin_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_del_coll_op(
    irods::plugin_context& _ctx,
    collInfo_t*            _coll_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_coll_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelColl");
    }

    status = _delColl( _ctx.comm(), _coll_info );
    if ( status != 0 ) {
        return ERROR( status, "_delColl failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlDelColl cmlExecuteNoAnswerSql commit failure {}", status);
        _rollback( "chlDelColl" );
        return ERROR( status, "commit failed" );
    }

    return SUCCESS();

} // db_del_coll_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_check_auth_op(
    irods::plugin_context& _ctx,
    const char*            _scheme,
    const char*            _challenge,
    const char*            _response,
    const char*            _user_name,
    int*                   _user_priv_level,
    int*                   _client_priv_level ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_challenge || !_response || !_user_name || !_user_priv_level || !_client_priv_level ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    // =-=-=-=-=-=-=-
    // All The Variable
    int status = 0;
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    const char *cp = NULL;
    int i = 0, OK = 0, k = 0;
    char userType[MAX_NAME_LEN];
    static int prevFailure = 0;
    char goodPw[MAX_PASSWORD_LEN + 10] = "";
    char lastPw[MAX_PASSWORD_LEN + 10] = "";
    char goodPwExpiry[MAX_PASSWORD_LEN + 10] = "";
    char goodPwTs[MAX_PASSWORD_LEN + 10] = "";
    char goodPwModTs[MAX_PASSWORD_LEN + 10] = "";
    rodsLong_t expireTime = 0;
    char *cpw = NULL;
    int nPasswords = 0;
    char myTime[50];
    time_t nowTime;
    time_t pwExpireMaxCreateTime;
    char expireStr[50];
    char expireStrCreate[50];
    char myUserZone[MAX_NAME_LEN];
    char userName2[NAME_LEN + 2];
    char userZone[NAME_LEN + 2];
    int hashType = 0;
    char lastPwModTs[MAX_PASSWORD_LEN + 10];
    snprintf( lastPwModTs, sizeof( lastPwModTs ), "0" );
    char *cPwTs = NULL;
    int iTs1 = 0, iTs2 = 0;
    std::vector<char> pwInfoArray( MAX_PASSWORD_LEN * MAX_PASSWORDS * 4 );

    // This function uses goto statements. You cannot initialize variables after a goto statement.
    auth_config ac{};

    if ( logSQL != 0 ) {
        log_sql::debug("chlCheckAuth");
    }

    if ( prevFailure > 1 ) {
        /* Somebody trying a dictionary attack? */
        if ( prevFailure > 5 ) {
            sleep( 20 );    /* at least, slow it down */
        }
        sleep( 2 );
    }
    *_user_priv_level = NO_USER_AUTH;
    *_client_priv_level = NO_USER_AUTH;

    hashType = HASH_TYPE_MD5;
    std::string user_name( _user_name );
    std::string::size_type pos = user_name.find( SHA1_FLAG_STRING );
    if ( std::string::npos != pos ) {
        // truncate off the :::sha1 string
        user_name = user_name.substr( pos );
        hashType = HASH_TYPE_SHA1;
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, _challenge, CHALLENGE_LEN );
    snprintf( prevChalSig, sizeof prevChalSig,
              "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
              ( unsigned char )md5Buf[0], ( unsigned char )md5Buf[1],
              ( unsigned char )md5Buf[2], ( unsigned char )md5Buf[3],
              ( unsigned char )md5Buf[4], ( unsigned char )md5Buf[5],
              ( unsigned char )md5Buf[6], ( unsigned char )md5Buf[7],
              ( unsigned char )md5Buf[8], ( unsigned char )md5Buf[9],
              ( unsigned char )md5Buf[10], ( unsigned char )md5Buf[11],
              ( unsigned char )md5Buf[12], ( unsigned char )md5Buf[13],
              ( unsigned char )md5Buf[14], ( unsigned char )md5Buf[15] );
    status = validateAndParseUserName( user_name.c_str(), userName2, userZone );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }

    if ( userZone[0] == '\0' ) {
        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }
        snprintf( myUserZone, sizeof( myUserZone ), "%s", zone.c_str() );
    }
    else {
        snprintf( myUserZone, sizeof( myUserZone ), "%s", userZone );
    }

    // The following is an artifact of the legacy authentication plugins. This operation is
    // only useful for certain plugins which are not supported in 4.3.0, so it is being
    // left out of compilation for now. Once we have determined that this is safe to do in
    // general, this section can be removed.
#if 0
    if ( _scheme && strlen( _scheme ) > 0 ) {
        irods::error ret = verify_auth_response( _scheme, _challenge, userName2, _response );
        if ( !ret.ok() ) {
            return PASS( ret );
        }
        goto checkLevel;
    }
#endif

    if ( logSQL != 0 ) {
        log_sql::debug("chlCheckAuth SQL 1 ");
    }

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName2 );
        bindVars.push_back( myUserZone );
        /* four strings per password returned */
        status = cmlGetMultiRowStringValuesFromSql( "select rcat_password, pass_expiry_ts, R_USER_PASSWORD.create_ts, R_USER_PASSWORD.modify_ts from R_USER_PASSWORD, "
                 "R_USER_MAIN where user_name=? and zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
                 pwInfoArray.data(), MAX_PASSWORD_LEN, MAX_PASSWORDS * 4, bindVars, &icss );
    }

    if ( status < 4 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_INVALID_USER; /* Be a little more specific */
            if ( strncmp( ANONYMOUS_USER, userName2, NAME_LEN ) == 0 ) {
                /* anonymous user, skip the pw check but do the rest */
                goto checkLevel;
            }
        }
        return ERROR( status, "select rcat_password failed" );
    }

    nPasswords = status / 4; /* four strings per password returned */
    goodPwExpiry[0] = '\0';
    goodPwTs[0] = '\0';
    goodPwModTs[0] = '\0';

    if ( nPasswords == MAX_PASSWORDS ) {
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName2 );
            // There are more than MAX_PASSWORDS in the database take the extra time to get them all.
            status = cmlGetIntegerValueFromSql( "select count(UP.user_id) from R_USER_PASSWORD UP, R_USER_MAIN where user_name=?",
                                                &MAX_PASSWORDS, bindVars, &icss );
        }
        if ( status < 0 ) {
            log_db::error("cmlGetIntegerValueFromSql failed in db_check_auth_op with status {}", status);
        }
        nPasswords = MAX_PASSWORDS;
        pwInfoArray.resize( MAX_PASSWORD_LEN * MAX_PASSWORDS * 4 );

        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName2 );
            bindVars.push_back( myUserZone );
            /* four strings per password returned */
            status = cmlGetMultiRowStringValuesFromSql( "select rcat_password, pass_expiry_ts, R_USER_PASSWORD.create_ts, R_USER_PASSWORD.modify_ts from R_USER_PASSWORD, "
                     "R_USER_MAIN where user_name=? and zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
                     pwInfoArray.data(), MAX_PASSWORD_LEN, MAX_PASSWORDS * 4, bindVars, &icss );
        }
        if ( status < 0 ) {
            log_db::error("cmlGetMultiRowStringValuesFromSql failed in db_check_auth_op with status {}", status);
        }
    }

    cpw = pwInfoArray.data();
    for ( k = 0; OK == 0 && k < MAX_PASSWORDS && k < nPasswords; k++ ) {
        memset( md5Buf, 0, sizeof( md5Buf ) );
        strncpy( md5Buf, _challenge, CHALLENGE_LEN );
        rstrcpy( lastPw, cpw, MAX_PASSWORD_LEN );
        icatDescramble( cpw );
        strncpy( md5Buf + CHALLENGE_LEN, cpw, MAX_PASSWORD_LEN );

        obfMakeOneWayHash( hashType,
                           ( unsigned char * )md5Buf, CHALLENGE_LEN + MAX_PASSWORD_LEN,
                           ( unsigned char * )digest );

        for ( i = 0; i < RESPONSE_LEN; i++ ) {
            if ( digest[i] == '\0' ) {
                digest[i]++;
            }  /* make sure 'string' doesn't end
                  early (this matches client code) */
        }

        cp = _response;
        OK = 1;
        for ( i = 0; i < RESPONSE_LEN; i++ ) {
            if ( *cp++ != digest[i] ) {
                OK = 0;
            }
        }

        memset( md5Buf, 0, sizeof( md5Buf ) );
        if ( OK == 1 ) {
            rstrcpy( goodPw, cpw, MAX_PASSWORD_LEN );
            cpw += MAX_PASSWORD_LEN;
            rstrcpy( goodPwExpiry, cpw, MAX_PASSWORD_LEN );
            cpw += MAX_PASSWORD_LEN;
            rstrcpy( goodPwTs, cpw, MAX_PASSWORD_LEN );
            cpw += MAX_PASSWORD_LEN;
            rstrcpy( goodPwModTs, cpw, MAX_PASSWORD_LEN );
        }
        else {
            cPwTs = cpw + ( MAX_PASSWORD_LEN * 3 );
            iTs1 = atoi( cPwTs );
            iTs2 = atoi( lastPwModTs );
            if ( iTs1 == iTs2 ) {
                /* MAX_PASSWORDS at same time-stamp, skip ahead to avoid infinite
                   loop; things should recover eventually */
                snprintf( lastPwModTs, sizeof lastPwModTs, "%011d", iTs1 + 1 );
            }
            else {
                /* normal case */
                rstrcpy( lastPwModTs, cPwTs, sizeof( lastPwModTs ) );
            }

            cpw += MAX_PASSWORD_LEN * 4;
        }
    }

    if ( OK == 0 ) {
        prevFailure++;
        return ERROR( CAT_INVALID_AUTHENTICATION, "invalid argument" );
    }

    expireTime = atoll( goodPwExpiry );
    getNowStr( myTime );

    if (const auto err = get_auth_config("authentication", ac); !err.ok()) {
        log_db::error("Failed to get auth configuration. [{}]", err.result());
        return err;
    }

    if ((strncmp(goodPwExpiry, "9999", 4) != 0) && expireTime >= ac.password_min_time &&
        expireTime <= ac.password_max_time) {
        time_t modTime;
        /* The used pw is an iRODS-PAM type, so now check if it's expired */
        getNowStr( myTime );
        nowTime = atoll( myTime );
        modTime = atoll( goodPwModTs );

        if ( modTime + expireTime < nowTime ) {
            /* it is expired, so return the error below and first remove it */
            cllBindVars[cllBindVarCount++] = lastPw;
            cllBindVars[cllBindVarCount++] = goodPwTs;
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = myUserZone;
            if ( logSQL != 0 ) {
                log_sql::debug("chlCheckAuth SQL 2");
            }
            status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where rcat_password=? and create_ts=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)", &icss );
            memset( goodPw, 0, sizeof( goodPw ) );
            memset( lastPw, 0, sizeof( lastPw ) );
            if ( status != 0 ) {
                log_db::info("chlCheckAuth cmlExecuteNoAnswerSql delete expired password failure {}", status);
                return ERROR( status, "delete expired password failure" );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                log_db::info("chlCheckAuth cmlExecuteNoAnswerSql commit failure {}", status);
                return ERROR( status, "commit failure" );
            }
            return ERROR( CAT_PASSWORD_EXPIRED, "password expired" );
        }
    }

    int temp_password_max_time;
    try {
        temp_password_max_time = irods::get_advanced_setting<const int>(irods::KW_CFG_MAX_TEMP_PASSWORD_LIFETIME);
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }

    // Only the enigmatic temporary passwords are stored as unscrambled in the catalog, so only perform this bit for
    // passwords that are not stored as scrambled in the catalog.
    if (!std::string_view{lastPw}.starts_with(PASSWORD_SCRAMBLE_PREFIX) && expireTime < temp_password_max_time) {
        int temp_password_time;
        try {
            temp_password_time = irods::get_advanced_setting<const int>(irods::KW_CFG_DEF_TEMP_PASSWORD_LIFETIME);
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        /* in the form used by temporary, one-time passwords */

        time_t createTime;
        int returnExpired;

        /* check if it's expired */

        returnExpired = 0;
        getNowStr( myTime );
        nowTime = atoll( myTime );
        createTime = atoll( goodPwTs );
        if ( createTime == 0 || nowTime == 0 ) {
            returnExpired = 1;
        }
        if ( createTime + expireTime < nowTime ) {
            returnExpired = 1;
        }


        /* Remove this temporary, one-time password */
        cllBindVars[cllBindVarCount++] = goodPw;
        if ( logSQL != 0 ) {
            log_sql::debug("chlCheckAuth SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_USER_PASSWORD where rcat_password=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlCheckAuth cmlExecuteNoAnswerSql delete failure {}", status);
            _rollback( "chlCheckAuth" );
            return ERROR( status, "delete failure" );
        }

        /* Also remove any expired temporary passwords */

        if ( logSQL != 0 ) {
            log_sql::debug("chlCheckAuth SQL 3");
        }
        snprintf( expireStr, sizeof expireStr, "%d", temp_password_time );
        cllBindVars[cllBindVarCount++] = expireStr;

        pwExpireMaxCreateTime = nowTime - temp_password_time;
        /* Not sure if casting to int is correct but seems OK & avoids warning:*/
        snprintf( expireStrCreate, sizeof expireStrCreate, "%011d",
                  ( int )pwExpireMaxCreateTime );
        cllBindVars[cllBindVarCount++] = expireStrCreate;

        status =  cmlExecuteNoAnswerSql(
                      "delete from R_USER_PASSWORD where pass_expiry_ts = ? and create_ts < ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlCheckAuth cmlExecuteNoAnswerSql delete2 failure {}", status);
            _rollback( "chlCheckAuth" );
            return ERROR( status, "delete2 failed" );
        }

        memset( goodPw, 0, MAX_PASSWORD_LEN );
        if ( returnExpired ) {
            return ERROR( CAT_PASSWORD_EXPIRED, "password expired" );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlCheckAuth SQL 4");
        }
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            log_db::info("chlCheckAuth cmlExecuteNoAnswerSql commit failure {}", status);
            return ERROR( status, "commit failure" );
        }
        memset( goodPw, 0, MAX_PASSWORD_LEN );
        if ( returnExpired ) {
            return ERROR( CAT_PASSWORD_EXPIRED, "password is expired" );
        }
    }

    /* Get the user type so privilege level can be set */
checkLevel:

    if ( logSQL != 0 ) {
        log_sql::debug("chlCheckAuth SQL 5");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName2 );
        bindVars.push_back( myUserZone );
        status = cmlGetStringValueFromSql(
                     "select user_type_name from R_USER_MAIN where user_name=? and zone_name=?",
                     userType, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback( "chlCheckAuth" );
        }
        return ERROR( status, "select user_type_name failed" );
    }
    *_user_priv_level = LOCAL_USER_AUTH;
    if ( strcmp( userType, "rodsadmin" ) == 0 ) {
        *_user_priv_level = LOCAL_PRIV_USER_AUTH;

        /* Since the user is admin, also get the client privilege level */
        if ( strcmp( _ctx.comm()->clientUser.userName, userName2 ) == 0 &&
                strcmp( _ctx.comm()->clientUser.rodsZone, userZone ) == 0 ) {
            *_client_priv_level = LOCAL_PRIV_USER_AUTH; /* same user, no query req */
        }
        else {
            if ( _ctx.comm()->clientUser.userName[0] == '\0' ) {
                /*
                   When using GSI, the client might not provide a user
                   name, in which case we avoid the query below (which
                   would fail) and instead set up minimal privileges.
                   This is safe since we have just authenticated the
                   remote server as an admin account.  This will allow
                   some queries (including the one needed for retrieving
                   the client's DNs).  Since the clientUser is not set,
                   some other queries are still exclued.  The non-IES will
                   reconnect once the rodsUserName is determined.  In
                   iRODS 2.3 this would return an error.
                 */
                *_client_priv_level = REMOTE_USER_AUTH;
                prevFailure = 0;
                return SUCCESS();
            }
            else {
                if ( logSQL != 0 ) {
                    log_sql::debug("chlCheckAuth SQL 6");
                }
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( _ctx.comm()->clientUser.userName );
                    bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
                    status = cmlGetStringValueFromSql(
                                 "select user_type_name from R_USER_MAIN where user_name=? and zone_name=?",
                                 userType, MAX_NAME_LEN, bindVars, &icss );
                }
                if ( status != 0 ) {
                    if ( status == CAT_NO_ROWS_FOUND ) {
                        status = CAT_INVALID_CLIENT_USER; /* more specific */
                    }
                    else {
                        _rollback( "chlCheckAuth" );
                    }
                    return ERROR( status, "select user_type_name failed" );
                }
                *_client_priv_level = LOCAL_USER_AUTH;
                if ( strcmp( userType, "rodsadmin" ) == 0 ) {
                    *_client_priv_level = LOCAL_PRIV_USER_AUTH;
                }
            }
        }
    }

    prevFailure = 0;
    return SUCCESS();

} // db_check_auth_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_make_temp_pw_op(
    irods::plugin_context& _ctx,
    char*                  _pw_value_to_hash,
    const char*            _other_user ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_pw_value_to_hash ||
        !_other_user ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    int temp_password_time;
    try {
        temp_password_time = irods::get_advanced_setting<const int>(irods::KW_CFG_DEF_TEMP_PASSWORD_LIFETIME);
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    int status;
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN + 2];
    int i;
    char password[MAX_PASSWORD_LEN + 10];
    char newPw[MAX_PASSWORD_LEN + 10];
    char myTime[50];
    char myTimeExp[50];
    char rBuf[200];
    char hashValue[50];
    int j = 0;
    char tSQL[MAX_SQL_SIZE];
    int useOtherUser = 0;

    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeTempPw");
    }

    if ( _other_user != NULL && strlen( _other_user ) > 0 ) {
        if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        useOtherUser = 1;
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeTempPw SQL 1 ");
    }

    snprintf( tSQL, MAX_SQL_SIZE,
              "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id and pass_expiry_ts != '%d'",
              temp_password_time );

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValueFromSql( tSQL, password, MAX_PASSWORD_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback( "chlMakeTempPw" );
        }
        return ERROR( status, "failed to get password" );
    }

    icatDescramble( password );

    j = 0;
    get64RandomBytes( rBuf );
    for ( i = 0; i < 50 && j < MAX_PASSWORD_LEN - 1; i++ ) {
        char c;
        c = rBuf[i] & 0x7f;
        if ( c < '0' ) {
            c += '0';
        }
        if ( ( c > 'a' && c < 'z' ) || ( c > 'A' && c < 'Z' ) ||
                ( c > '0' && c < '9' ) ) {
            hashValue[j++] = c;
        }
    }
    hashValue[j] = '\0';
    /*   printf("hashValue=%s\n", hashValue); */

    /* calculate the temp password (a hash of the user's main pw and
       the hashValue) */
    memset( md5Buf, 0, sizeof( md5Buf ) );
    snprintf( md5Buf, sizeof( md5Buf ), "%s%s", hashValue, password );

    obfMakeOneWayHash( HASH_TYPE_DEFAULT,
                       ( unsigned char * ) md5Buf, 100, ( unsigned char * ) digest );

    hashToStr( digest, newPw );
    /*   printf("newPw=%s\n", newPw); */

    snprintf( _pw_value_to_hash, MAX_PASSWORD_LEN, "%s", hashValue );

    /* Insert the temporary, one-time password */

    getNowStr( myTime );
    sprintf( myTimeExp, "%d", temp_password_time );  /* seconds from create time
                                                      when it will expire */
    if ( useOtherUser == 1 ) {
        cllBindVars[cllBindVarCount++] = _other_user;
    }
    else {
        cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.userName;
    }
    cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.rodsZone,
    cllBindVars[cllBindVarCount++] = newPw;
    cllBindVars[cllBindVarCount++] = myTimeExp;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeTempPw SQL 2");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlMakeTempPw cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlMakeTempPw" );
        return ERROR( status, "insert failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlMakeTempPw cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failed" );
    }

    memset( newPw, 0, MAX_PASSWORD_LEN );
    return SUCCESS();

} // db_make_temp_pw_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_make_limited_pw_op(
    irods::plugin_context& _ctx,
    int                    _ttl,
    char*                  _pw_value_to_hash ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_pw_value_to_hash ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    int temp_password_time;
    try {
        temp_password_time = irods::get_advanced_setting<const int>(irods::KW_CFG_DEF_TEMP_PASSWORD_LIFETIME);
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN + 2];
    int i;
    char password[MAX_PASSWORD_LEN + 10];
    char newPw[MAX_PASSWORD_LEN + 10];
    char myTime[50];
    char rBuf[200];
    char hashValue[50];
    int j = 0;
    char tSQL[MAX_SQL_SIZE];
    char expTime[50];

    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeLimitedPw");
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeLimitedPw SQL 1 ");
    }

    snprintf( tSQL, MAX_SQL_SIZE,
              "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id and pass_expiry_ts != '%d'",
              temp_password_time );

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValueFromSql( tSQL, password, MAX_PASSWORD_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback( "chlMakeLimitedPw" );
        }
        return ERROR( status, "get password failed" );
    }

    icatDescramble( password );

    j = 0;
    get64RandomBytes( rBuf );
    for ( i = 0; i < 50 && j < MAX_PASSWORD_LEN - 1; i++ ) {
        char c;
        c = rBuf[i] & 0x7f;
        if ( c < '0' ) {
            c += '0';
        }
        if ( ( c > 'a' && c < 'z' ) || ( c > 'A' && c < 'Z' ) ||
                ( c > '0' && c < '9' ) ) {
            hashValue[j++] = c;
        }
    }
    hashValue[j] = '\0';

    /* calculate the limited password (a hash of the user's main pw and
       the hashValue) */
    memset( md5Buf, 0, sizeof( md5Buf ) );
    snprintf( md5Buf, sizeof( md5Buf ), "%s%s", hashValue, password );

    obfMakeOneWayHash( HASH_TYPE_DEFAULT,
                       ( unsigned char * ) md5Buf, 100, ( unsigned char * ) digest );

    hashToStr( digest, newPw );

    icatScramble( newPw );

    snprintf( _pw_value_to_hash, MAX_PASSWORD_LEN, "%s", hashValue );

    getNowStr( myTime );

    auth_config ac{};
    if (const auto err = get_auth_config("authentication", ac); !err.ok()) {
        log_db::error("Failed to get auth configuration. [{}]", err.result());
        return err;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    int timeToLive = _ttl * 3600; /* convert input hours to seconds */
    if (timeToLive < ac.password_min_time || timeToLive > ac.password_max_time) {
        log_db::error("Invalid TTL - min time: [{}] max time:[{}] ttl: [{}]",
                      ac.password_min_time,
                      ac.password_max_time,
                      timeToLive);
        return ERROR( PAM_AUTH_PASSWORD_INVALID_TTL, "invalid ttl" );
    }

    /* Insert the limited password */
    snprintf( expTime, sizeof expTime, "%d", timeToLive );
    cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.userName;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = _ctx.comm()->clientUser.rodsZone;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = newPw;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = expTime;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeLimitedPw SQL 2");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlMakeLimitedPw cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlMakeLimitedPw" );
        return ERROR( status, "insert failure" );
    }

    /* Also delete any that are expired */
    if ( logSQL != 0 ) {
        log_sql::debug("chlMakeLimitedPw SQL 3");
    }

    const auto password_min_time_str = std::to_string(ac.password_min_time);
    const auto password_max_time_str = std::to_string(ac.password_max_time);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = password_min_time_str.c_str();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = password_max_time_str.c_str();
    cllBindVars[cllBindVarCount++] = myTime;
#if MY_ICAT
    status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and (cast(pass_expiry_ts as signed integer) + cast(modify_ts as signed integer) < ?)",
                                     &icss );
#else
    status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and (cast(pass_expiry_ts as integer) + cast(modify_ts as integer) < ?)",
                                     &icss );
#endif
    // CAT_SUCCESS_BUT_WITH_NO_INFO indicates that no expired passwords exist, which is okay.
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlMakeLimitedPw cmlExecuteNoAnswerSql delete failure {}", status);
        _rollback( "chlMakeLimitedPw" );
        return ERROR( status, "delete failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlMakeLimitedPw cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failed" );
    }

    memset( newPw, 0, MAX_PASSWORD_LEN );

    return SUCCESS();

} // db_make_limited_pw_op

// =-=-=-=-=-=-=-
// authenticate user
auto db_update_pam_password_op(irods::plugin_context& _ctx,
                               const char* _user_name,
                               int _ttl,
                               const char* _test_time,
                               char** _password_buffer,
                               std::size_t _password_buffer_size) -> irods::error
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if (!_user_name || !_password_buffer) {
        return ERROR(CAT_INVALID_ARGUMENT, "null parameter");
    }

    // Plus 1 for null terminator.
    std::array<char, MAX_PASSWORD_LEN + 1> password_in_database_buffer{};
    if (password_in_database_buffer.size() > _password_buffer_size) {
        return ERROR(
            SYS_INVALID_INPUT_PARAM,
            fmt::format("{}: Buffer not large enough to hold password. Requires [{}] bytes, received [{}] bytes.",
                        __func__,
                        password_in_database_buffer.size(),
                        _password_buffer_size));
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    char myTime[50];
    int status;
    char passwordModifyTime[50];
    char *cVal[3];
    int iVal[3];
    char selUserId[MAX_NAME_LEN];
    char expTime[50];

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    getNowStr( myTime );

    auth_config ac{};
    if (const auto err = get_auth_config("authentication", ac); !err.ok()) {
        log_db::error("Failed to get auth configuration. [{}]", err.result());
        return err;
    }

    /* if ttl is unset, use the default (minimum password lifetime) */
    if ( _ttl == 0 ) {
        rstrcpy(expTime, std::to_string(ac.password_min_time).c_str(), sizeof expTime);
    }
    else {
        /* convert ttl to seconds and make sure ttl is within the limits */
        _ttl = _ttl * 3600;
        if (_ttl < ac.password_min_time || _ttl > ac.password_max_time) {
            return ERROR( PAM_AUTH_PASSWORD_INVALID_TTL, "pam ttl invalid" );
        }
        snprintf( expTime, sizeof expTime, "%d", _ttl );
    }

    /* get user id */
    if ( logSQL != 0 ) {
        log_sql::debug("chlUpdateIrodsPamPassword SQL 1");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _user_name );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name!='rodsgroup'",
                     selUserId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        return  ERROR( CAT_INVALID_USER, "invalid user" );
    }
    if ( status ) {
        return ERROR( status, "failed to get user id" );
    }

    /* first delete any that are expired */
    if ( logSQL != 0 ) {
        log_sql::debug("chlUpdateIrodsPamPassword SQL 2");
    }

    const auto password_min_time_str = std::to_string(ac.password_min_time);
    const auto password_max_time_str = std::to_string(ac.password_max_time);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = password_min_time_str.c_str();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = password_max_time_str.c_str();
    cllBindVars[cllBindVarCount++] = myTime;
#if MY_ICAT
    status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and (cast(pass_expiry_ts as signed integer) + cast(modify_ts as signed integer) < ?)",
#else
    status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and (cast(pass_expiry_ts as integer) + cast(modify_ts as integer) < ?)",
#endif
                                     & icss );
    // CAT_SUCCESS_BUT_WITH_NO_INFO indicates that no expired passwords exist, which is okay.
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql delete failure {}", status);
        _rollback( "chlUpdateIrodsPamPassword" );
        return ERROR( status, "delete failure" );
    }

    if (logSQL != 0) {
        log_sql::debug("chlUpdateIrodsPamPassword SQL 3");
    }

    cVal[0] = password_in_database_buffer.data();
    iVal[0] = MAX_PASSWORD_LEN;
    cVal[1] = passwordModifyTime;
    iVal[1] = sizeof( passwordModifyTime );
    {
        std::vector<std::string> bindVars;
        bindVars.emplace_back(selUserId);
        bindVars.emplace_back(password_min_time_str.c_str());
        bindVars.emplace_back(password_max_time_str.c_str());
        status = cmlGetStringValuesFromSql(
#if MY_ICAT
                     "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer) >= ? and cast (pass_expiry_ts as signed integer) <= ?",
#else
                     "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer) >= ? and cast (pass_expiry_ts as integer) <= ?",
#endif
                     cVal, iVal, 2, bindVars, &icss );
    }

    if ( status == 0 ) {
        if (ac.password_extend_lifetime) {
            if ( logSQL != 0 ) {
                log_sql::debug("chlUpdateIrodsPamPassword SQL 4");
            }
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = expTime;
            cllBindVars[cllBindVarCount++] = selUserId;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = password_in_database_buffer.data();
            status =  cmlExecuteNoAnswerSql( "update R_USER_PASSWORD set modify_ts=?, pass_expiry_ts=? where user_id = ? and rcat_password = ?",
                                             &icss );
            if ( status ) {
                return ERROR( status, "password update error" );
            }

            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                log_db::info("chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure {}", status);
                return ERROR( status, "commit failure" );
            }
        }

        // password_in_database_buffer holds the randomly generated password in a scrambled form. It needs to be
        // descrambled before returning to the caller.
        icatDescramble(password_in_database_buffer.data());

        // The descrambled password at time of generation is 50 characters or less (see below).
        std::strncpy(*_password_buffer, password_in_database_buffer.data(), _password_buffer_size);

        return SUCCESS();
    }

    // See db_mod_user_op operation "password" for explanation about password lengths. This is apparently the enforced
    // longest password length for native authentication. The random password is used with native authentication and so
    // it cannot exceed this size. Also, make sure the output buffer can hold the randomly generated password.
    constexpr auto random_password_len = MAX_PASSWORD_LEN - 8;
    if (random_password_len + 1 > _password_buffer_size) {
        return ERROR(
            SYS_INVALID_INPUT_PARAM,
            fmt::format("{}: Buffer not large enough to hold password. Requires [{}] bytes, received [{}] bytes.",
                        __func__,
                        random_password_len + 1,
                        _password_buffer_size));
    }

    // The length of the randomly generated password must fit into the buffer for the random password in scrambled form.
    std::array<char, MAX_PASSWORD_LEN + 1> scrambled_random_password{}; // +1 for null terminator
    static_assert(random_password_len < scrambled_random_password.size());

    // Historically, the randomly generated password had been a string no longer than MAX_PASSWORD_LEN with characters
    // in the set [1-8B-Yb-y]. The string is now guaranteed to be the specified length and contain characters in the
    // set of alphanumeric characters.
    const auto random_password = irods::generate_random_alphanumeric_string(random_password_len);

    // Copy the randomly generated password because icatScramble modifies the passed-in buffer. We want to retain the
    // unscrambled password so that it can be returned to the user for future authentication purposes.
    std::strncpy(scrambled_random_password.data(), random_password.c_str(), random_password_len + 1);
    icatScramble(scrambled_random_password.data());

    if ( _test_time != NULL && strlen( _test_time ) > 0 ) {
        snprintf( myTime, sizeof( myTime ), "%s", _test_time );
    }

    if (logSQL != 0)
        log_sql::debug("chlUpdateIrodsPamPassword SQL 5");
    cllBindVars[cllBindVarCount++] = selUserId;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    cllBindVars[cllBindVarCount++] = scrambled_random_password.data();
    cllBindVars[cllBindVarCount++] = expTime;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    status =  cmlExecuteNoAnswerSql( "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values (?, ?, ?, ?, ?)",
                                     &icss );
    if ( status ) return ERROR( status, "insert failure" );

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    // Return the unscrambled, randomly generated password to the user.
    std::strncpy(*_password_buffer, random_password.c_str(), _password_buffer_size);

    return SUCCESS();
} // db_update_pam_password_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_mod_user_op(
    irods::plugin_context& _ctx,
    const char*            _user_name,
    const char*            _option,
    const char*            _new_value ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_user_name ||
        !_option    ||
        !_new_value ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    int opType;
    char decoded[MAX_PASSWORD_LEN + 20];
    char tSQL[MAX_SQL_SIZE];
    char form1[] = "update R_USER_MAIN set %s=?, modify_ts=? where user_name=? and zone_name=?";
    char form2[] = "update R_USER_MAIN set %s=%s, modify_ts=? where user_name=? and zone_name=?";
    char form3[] = "update R_USER_PASSWORD set rcat_password=?, modify_ts=? where user_id=?";
    char form4[] = "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)";
    char form5[] = "insert into R_USER_AUTH (user_id, user_auth_name, create_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?)";
    char form6[] = "delete from R_USER_AUTH where user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?) and user_auth_name = ?";
#if MY_ICAT
    char form7[] = "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)";
#else
    char form7[] = "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)";
#endif

    char myTime[50];
    rodsLong_t iVal;

    int groupAdminSettingPassword; // JMC - backport 4772

    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlModUser");
    }

    if ( *_user_name == '\0' || *_option == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "parameter is empty" );
    }

    if( *_new_value == '\0' && (
        strcmp( _option, "type"   ) == 0 ||
        strcmp( _option, "zone"   ) == 0 ||
        strcmp( _option, "addAuth") == 0 ||
        strcmp( _option, "rmAuth" ) == 0 ) ) {
        return ERROR( CAT_INVALID_ARGUMENT, "new value is empty" );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    groupAdminSettingPassword = 0;
    if ( _ctx.comm()->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH && // JMC - backport 4773
            _ctx.comm()->proxyUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH ) {
        /* user is OK */
    }
    else {
        /* need to check */
        if ( strcmp( _option, "password" ) != 0 ) {
            /* only password (in cases below) is allowed */
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( 0 != strcmp( _user_name, _ctx.comm()->clientUser.userName ) )  {
            int status2 = cmlCheckGroupAdminAccess(
                           _ctx.comm()->clientUser.userName,
                           _ctx.comm()->clientUser.rodsZone,
                           "", &icss );
            if ( status2 != 0 ) {
                return ERROR( status2, "cmlCheckGroupAdminAccess failed" );
            }
            groupAdminSettingPassword = 1;
        }
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    tSQL[0] = '\0';
    opType = 0;

    getNowStr( myTime );

    status = validateAndParseUserName( _user_name, userName2, zoneName );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( zoneName[0] == '\0' ) {
        rstrcpy( zoneName, zone.c_str(), NAME_LEN );
    }

    if ( strcmp( _option, "type" ) == 0 ||
            strcmp( _option, "user_type_name" ) == 0 ) {
        char tsubSQL[MAX_SQL_SIZE];
        snprintf( tsubSQL, MAX_SQL_SIZE, "(select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?)" );
        cllBindVars[cllBindVarCount++] = _new_value;
        snprintf( tSQL, MAX_SQL_SIZE, form2,
                  "user_type_name", tsubSQL );
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        opType = 1;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 2");
        }
    }
    if ( strcmp( _option, "addAuth" ) == 0 ) {
        opType = 4;
        if ( !_userInRUserAuth( userName2, zoneName, _new_value ) ) {
            rstrcpy( tSQL, form5, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            cllBindVars[cllBindVarCount++] = _new_value;
            cllBindVars[cllBindVarCount++] = myTime;
        } else {
            return SUCCESS();
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 4");
        }
    }
    if ( strcmp( _option, "rmAuth" ) == 0 ) {
        rstrcpy( tSQL, form6, MAX_SQL_SIZE );
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        cllBindVars[cllBindVarCount++] = _new_value;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 5");
        }
    }

    if ( strncmp( _option, "rmPamPw", 9 ) == 0 ) {
        auth_config ac{};
        if (const auto err = get_auth_config("authentication", ac); !err.ok()) {
            log_db::error("Failed to get auth configuration. [{}]", err.result());
            return err;
        }

        const auto password_min_time_str = std::to_string(ac.password_min_time);
        const auto password_max_time_str = std::to_string(ac.password_max_time);

        rstrcpy( tSQL, form7, MAX_SQL_SIZE );
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        cllBindVars[cllBindVarCount++] = password_min_time_str.c_str();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        cllBindVars[cllBindVarCount++] = password_max_time_str.c_str();
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 6");
        }
    }

    if ( strcmp( _option, "info" ) == 0 ||
            strcmp( _option, "user_info" ) == 0 ) {
        snprintf( tSQL, MAX_SQL_SIZE, form1,
                  "user_info" );
        cllBindVars[cllBindVarCount++] = _new_value;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 6");
        }
    }
    if ( strcmp( _option, "comment" ) == 0 ||
            strcmp( _option, "r_comment" ) == 0 ) {
        snprintf( tSQL, MAX_SQL_SIZE, form1,
                  "r_comment" );
        cllBindVars[cllBindVarCount++] = _new_value;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 7");
        }
    }
    if ( strcmp( _option, "password" ) == 0 ) {
        int i;
        char userIdStr[MAX_NAME_LEN];
        i = decodePw( _ctx.comm(), _new_value, decoded );
        if (i == CAT_PASSWORD_ENCODING_ERROR || strlen(decoded) > MAX_PASSWORD_LEN - 8) {
            // Password encoding error occurs when the password is not of the correct
            // length.  Pop the existing CAT_PASSWORD_ENCODING_ERROR and return PASSWORD_EXCEEDS_MAX_SIZE
            // error.  See issue 6764.
            // Note that there are other conditions that may cause CAT_PASSWORD_ENCODING_ERROR but
            // the most likely one is a password of invalid length.
            irods::pop_error_message(_ctx.comm()->rError);
            return ERROR(PASSWORD_EXCEEDS_MAX_SIZE, "Password must be between 3 and 42 characters");
        }
        int status2 = icatApplyRule( _ctx.comm(), ( char* )"acCheckPasswordStrength", decoded );
        if ( status2 == NO_RULE_OR_MSI_FUNCTION_FOUND_ERR ) {
            addRErrorMsg( &_ctx.comm()->rError, 0, "acCheckPasswordStrength rule not found" );
        }


        if ( status2 ) {
            return ERROR( status2, "icatApplyRule failed" );
        }

        icatScramble( decoded );

        if ( i ) {
            return ERROR( i, "password scramble failed" );
        }
        if ( logSQL != 0 ) {
            log_sql::debug("chlModUser SQL 8");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName2 );
            bindVars.push_back( zoneName );
            i = cmlGetStringValueFromSql(
                    "select R_USER_PASSWORD.user_id from R_USER_PASSWORD, R_USER_MAIN where R_USER_MAIN.user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
                    userIdStr, MAX_NAME_LEN, bindVars, &icss );
        }
        if ( i != 0 && i != CAT_NO_ROWS_FOUND ) {
            return ERROR( i, "get user password failed" );
        }
        if ( i == 0 ) {
            if ( groupAdminSettingPassword == 1 ) { // JMC - backport 4772
                /* Group admin can only set the initial password, not update */
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
            }
            rstrcpy( tSQL, form3, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = decoded;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = userIdStr;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModUser SQL 9");
            }
        }
        else {
            opType = 4;
            rstrcpy( tSQL, form4, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            cllBindVars[cllBindVarCount++] = decoded;
            cllBindVars[cllBindVarCount++] = "9999-12-31-23.59.01";
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = myTime;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModUser SQL 10");
            }
        }
    }

    if ( tSQL[0] == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid argument" );
    }

    status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    memset( decoded, 0, MAX_PASSWORD_LEN );

    if ( status != 0 ) { /* error */
        if ( opType == 1 ) { /* doing a type change, check if user_type problem */
            int status2;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModUser SQL 11");
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( _new_value );
                status2 = cmlGetIntegerValueFromSql(
                              "select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?",
                              &iVal, bindVars, &icss );
            }
            if ( status2 != 0 ) {
                char errMsg[105];
                snprintf( errMsg, 100, "user_type '%s' is not valid",
                          _new_value );
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );

                log_db::info("chlModUser invalid user_type");
                return ERROR( CAT_INVALID_USER_TYPE, "invalid user type" );
            }
        }
        if ( opType == 4 ) { /* trying to insert password or auth-name */
            /* check if user exists */
            int status2;
            _rollback( "chlModUser" );
            if ( logSQL != 0 ) {
                log_sql::debug("chlModUser SQL 12");
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( userName2 );
                bindVars.push_back( zoneName );
                status2 = cmlGetIntegerValueFromSql(
                              "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                              &iVal, bindVars, &icss );
            }
            if ( status2 != 0 ) {
                log_db::info("chlModUser invalid user {} zone {}", userName2, zoneName);
                return ERROR( CAT_INVALID_USER, "invalid user" );
            }
        }
        log_db::info("chlModUser cmlExecuteNoAnswerSql failure {}", status);
        return ERROR( status, "get user_id failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModUser cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failed" );
    }

    return SUCCESS();

} // db_mod_user_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_mod_group_op(
    irods::plugin_context& _ctx,
    const char*            _group_name,
    const char*            _option,
    const char*            _user_name,
    const char*            _user_zone ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_group_name ||
        !_option     ||
        !_user_name ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status, OK;
    char myTime[50];
    char userId[MAX_NAME_LEN];
    char groupId[MAX_NAME_LEN];
    char zoneToUse[MAX_NAME_LEN];

    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlModGroup");
    }

    if ( *_group_name == '\0' || *_option == '\0' || *_user_name == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "argument is empty" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
            _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
                       _ctx.comm()->clientUser.userName,
                       _ctx.comm()->clientUser.rodsZone, _group_name, &icss );
        if ( status2 != 0 ) {
            /* User is not a groupadmin that is a member of this group. */
            /* But if we're doing an 'add' and they are a groupadmin
                and the group is empty, allow it */
            if ( strcmp( _option, "add" ) == 0 ) {
                int status3 =  cmlCheckGroupAdminAccess(
                                   _ctx.comm()->clientUser.userName,
                                   _ctx.comm()->clientUser.rodsZone, "", &icss );
                if ( status3 == 0 ) {
                    int status4 = cmlGetGroupMemberCount( _group_name, &icss );
                    if ( status4 == 0 ) { /* call succeeded and the total is 0 */
                        status2 = 0;    /* reset the error to success to allow it */
                    }
                }
            }
        }
        if ( status2 != 0 ) {
            return ERROR( status2, "group admin access invalid" );
        }
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( _user_zone != NULL && *_user_zone != '\0' ) {
        snprintf( zoneToUse, MAX_NAME_LEN, "%s", _user_zone );
    }
    else {
        snprintf( zoneToUse, MAX_NAME_LEN, "%s", zone.c_str() );
    }

    status = validateAndParseUserName( _user_name, userName2, zoneName );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( zoneName[0] != '\0' ) {
        rstrcpy( zoneToUse, zoneName, NAME_LEN );
    }

    userId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlModGroup SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userName2 );
        bindVars.push_back( zoneToUse );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name !='rodsgroup'",
                     userId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_USER, "user not found" );
        }
        _rollback( "chlModGroup" );
        return ERROR( status, "failed to get user" );
    }

    groupId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlModGroup SQL 2");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _group_name );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name='rodsgroup'",
                     groupId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_GROUP, "invalid group" );
        }
        _rollback( "chlModGroup" );
        return ERROR( status, "failed to get group" );
    }
    OK = 0;
    if ( strcmp( _option, "remove" ) == 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModGroup SQL 3");
        }
        cllBindVars[cllBindVarCount++] = groupId;
        cllBindVars[cllBindVarCount++] = userId;
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_USER_GROUP where group_user_id = ? and user_id = ?",
                      &icss );
        if ( status != 0 ) {
            if (CAT_SUCCESS_BUT_WITH_NO_INFO == status) {
                // If the removal resulted in nothing happening, the target user is not a member of the target group.
                // Some clients and REPs do not acknowledge CAT_SUCCESS_BUT_WITH_NO_INFO so we need to return something
                // a little more specific to indicate the state of affairs.
                status = USER_NOT_IN_GROUP;
            }
            log_db::info("chlModGroup cmlExecuteNoAnswerSql delete failure {}", status);
            _rollback( "chlModGroup" );
            return ERROR( status, "delete failure" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "add" ) == 0 ) {
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = groupId;
        cllBindVars[cllBindVarCount++] = userId;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModGroup SQL 4");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_USER_GROUP (group_user_id, user_id , create_ts, modify_ts) values (?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlModGroup cmlExecuteNoAnswerSql add failure {}", status);
            _rollback( "chlModGroup" );
            return ERROR( status, "add failure" );
        }
        OK = 1;
    }

    if ( OK == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModGroup cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return SUCCESS();

} // db_mod_group_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_mod_resc_op(
    irods::plugin_context& _ctx,
    const char*            _resc_name,
    const char*            _option,
    const char*            _option_value ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_resc_name  ||
        !_option     ||
        !_option_value ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status = 0, OK = 0;
    char rescId[MAX_NAME_LEN];
    char rescPath[MAX_NAME_LEN] = "";
    char rescPathMsg[MAX_NAME_LEN + 100];

    if ( logSQL != 0 ) {
        log_sql::debug("chlModResc");
    }

    if ( *_resc_name == '\0' || *_option == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "argument is empty" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4629
    if ( strncmp( _resc_name, BUNDLE_RESC, strlen( BUNDLE_RESC ) ) == 0 ) {
        char errMsg[155];
        snprintf( errMsg, 150,
                  "%s is a built-in resource needed for bundle operations.",
                  BUNDLE_RESC );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_PSEUDO_RESC_MODIFY_DISALLOWED, "cannot mod bundle resc" );
    }
    // =-=-=-=-=-=-=-

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    rescId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlModResc SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     rescId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
        }
        _rollback( "chlModResc" );
        return ERROR( status, "failed to get resource id" );
    }

    const auto [current_time_secs, current_time_msecs] = get_current_time();
    OK = 0;

    if ( strcmp( _option, "comment" ) == 0 ) {
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 3");
        }
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set r_comment=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to update comment" );
        }

        OK = 1;

    }
    else if ( *_option_value == '\0' ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "argument is empty" );
    }

    if ( strcmp( _option, "info" ) == 0 ) {
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 2");
        }
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_info=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to update info" );
        }
        OK = 1;
    }


    if (strcmp(_option, "freespace") == 0 || strcmp(_option, "free_space") == 0) {
        int inType = 0;    /* regular mode, just set as provided */
        if ( *_option_value == '+' ) {
            inType = 1;     /* increment by the input value */
            _option_value++;  /* skip over the + */
        }
        if ( *_option_value == '-' ) {
            inType = 2;    /* decrement by the value */
            _option_value++; /* skip over the - */
        }
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 4");
        }
        if ( inType == 0 ) {
            status = cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set free_space=?, free_space_ts=?, modify_ts=?, modify_ts_millis=? where resc_id=?",
                &icss);
        }
        else if (inType == 1) {
#if ORA_ICAT
            /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
            status =
                cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = cast(free_space as integer) + cast(? as "
                                      "integer), free_space_ts = ?, modify_ts=?, modify_ts_millis=? where resc_id=?",
                                      &icss);
#elif MY_ICAT
            status = cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = free_space + ?, free_space_ts = ?, "
                                           "modify_ts=?, modify_ts_millis=? where resc_id=?",
                                           &icss);
#else
            status =
                cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = cast(free_space as bigint) + cast(? as "
                                      "bigint), free_space_ts = ?, modify_ts=?, modify_ts_millis=? where resc_id=?",
                                      &icss);
#endif
        }
        else if (inType == 2) {
#if ORA_ICAT
            /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
            status =
                cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = cast(free_space as integer) - cast(? as "
                                      "integer), free_space_ts = ?, modify_ts=?, modify_ts_millis=? where resc_id=?",
                                      &icss);
#elif MY_ICAT
            status = cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = free_space - ?, free_space_ts = ?, "
                                           "modify_ts=?, modify_ts_millis=? where resc_id=?",
                                           &icss);
#else
            status =
                cmlExecuteNoAnswerSql("update R_RESC_MAIN set free_space = cast(free_space as bigint) - cast(? as "
                                      "bigint), free_space_ts = ?, modify_ts=?, modify_ts_millis=? where resc_id=?",
                                      &icss);
#endif
        }
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to update freespace" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "host" ) == 0 ) {
        // =-=-=-=-=-=-=-
        // JMC - backport 4597
        _resolveHostName( _ctx.comm(), _option_value);

        // =-=-=-=-=-=-=-
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 5");
        }
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_net=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set host" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "type" ) == 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 6");
        }

        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 7");
        }
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_type_name = ?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set type" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "path" ) == 0 ) {
        // Root dir is not a valid vault path
        ret = verify_non_root_vault_path(_ctx, std::string(_option_value));
        if (!ret.ok()) {
            return PASS(ret);
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 10");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( rescId );
            status = cmlGetStringValueFromSql(
                         "select resc_def_path from R_RESC_MAIN where resc_id=?",
                         rescPath, MAX_NAME_LEN, bindVars, &icss );
        }
        if ( status != 0 ) {
            log_db::info("chlModResc cmlGetStringValueFromSql query failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to get path" );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 11");
        }

        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_def_path=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set path" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "status" ) == 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 12");
        }
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_status=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set status" );
        }
        OK = 1;
    }

    if ( strcmp( _option, "name" ) == 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 13");
        }
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        /*    If the new name is not unique, this will return an error */
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_name=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set resc name with modify time" );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 14");
        }

        // JMC :: remove update r_data_main with resc_name
        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 15");
        }
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = _resc_name;
        status =  cmlExecuteNoAnswerSql(
                      "update R_SERVER_LOAD set resc_name=? where resc_name=?",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to update server load" );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModResc SQL 16");
        }
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = _resc_name;
        status =  cmlExecuteNoAnswerSql(
                      "update R_SERVER_LOAD_DIGEST set resc_name=? where resc_name=?",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set load digest" );
        }

        OK = 1;

    } // if name

    if ( strcmp( _option, "context" ) == 0 ) {
        cllBindVars[cllBindVarCount++] = _option_value;
        cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
        cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
        cllBindVars[cllBindVarCount++] = rescId;
        status = cmlExecuteNoAnswerSql(
            "update R_RESC_MAIN set resc_context=?, modify_ts=?, modify_ts_millis=? where resc_id=?", &icss);
        if ( status != 0 ) {
            log_db::info("chlModResc cmlExecuteNoAnswerSql update failure for resc context {}", status);
            _rollback( "chlModResc" );
            return ERROR( status, "failed to set context" );
        }
        OK = 1;
    }

    if ( OK == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModResc cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    if ( rescPath[0] != '\0' ) {
        /* if the path was gotten, return it */

        snprintf( rescPathMsg, sizeof( rescPathMsg ), "Previous resource path: %s",
                  rescPath );
        addRErrorMsg( &_ctx.comm()->rError, 0, rescPathMsg );
    }

    return SUCCESS();
} // db_mod_resc_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_mod_resc_data_paths_op(
    irods::plugin_context& _ctx,
    const char*            _resc_name,
    const char*            _old_path,
    const char*            _new_path,
    const char*            _user_name ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_resc_name ||
        !_old_path  ||
        !_new_path ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char rescId[MAX_NAME_LEN];
    int status, len, rows;
    const char *cptr;
    //   char userId[NAME_LEN]="";
    char userZone[NAME_LEN];
    char zoneToUse[NAME_LEN];
    char userName2[NAME_LEN];


    if ( logSQL != 0 ) {
        log_sql::debug("chlModRescDataPaths");
    }

    if ( *_resc_name == '\0' || *_old_path == '\0' || *_new_path == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "argument is empty" );
    }

    /* the paths must begin and end with / */
    if ( *_old_path != '/' or * _new_path != '/' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid path" );
    }
    len = strlen( _old_path );
    cptr = _old_path + len - 1;
    if ( *cptr != '/' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid old path" );
    }
    len = strlen( _new_path );
    cptr = _new_path + len - 1;
    if ( *cptr != '/' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid new path" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    rescId[0] = '\0';
    if ( logSQL != 0 ) {
        log_sql::debug("chlModRescDataPaths SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     rescId, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
        }
        _rollback( "chlModRescDataPaths" );
        return ERROR( status, "failed to get resource id" );
    }

    /* This is needed for like clause which is needed to get the
       correct number of rows that were updated (seems like the DBMS will
       return a row count for rows looked at for the replace). */
    char oldPath2[MAX_NAME_LEN];
    snprintf( oldPath2, sizeof( oldPath2 ), "%s%%", _old_path );

    if ( _user_name != NULL && *_user_name != '\0' ) {
        status = validateAndParseUserName( _user_name, userName2, userZone );
        if ( status ) {
            return ERROR( status, "Invalid username format" );
        }
        if ( userZone[0] != '\0' ) {
            snprintf( zoneToUse, sizeof( zoneToUse ), "%s", userZone );
        }
        else {
            snprintf( zoneToUse, sizeof( zoneToUse ), "%s", zone.c_str() );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModRescDataPaths SQL 2");
        }

        cllBindVars[cllBindVarCount++] = _old_path;
        cllBindVars[cllBindVarCount++] = _new_path;
        cllBindVars[cllBindVarCount++] = rescId;
        cllBindVars[cllBindVarCount++] = oldPath2;
        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneToUse;
        status =  cmlExecuteNoAnswerSql(
                      "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_id=? and data_path like ? and data_owner_name=? and data_owner_zone=?",
                      &icss );
    }
    else {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModRescDataPaths SQL 3");
        }

        cllBindVars[cllBindVarCount++] = _old_path;
        cllBindVars[cllBindVarCount++] = _new_path;
        cllBindVars[cllBindVarCount++] = rescId;
        cllBindVars[cllBindVarCount++] = oldPath2;
        status =  cmlExecuteNoAnswerSql(
                      "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_id=? and data_path like ?",
                      &icss );
    }
    if ( status != 0 ) {
        log_db::info("chlModRescDataPaths cmlExecuteNoAnswerSql update failure {}", status);
        _rollback( "chlModResc" );
        return ERROR( status, "failed to update path" );
    }

    rows = cllGetRowCount( &icss, -1 );

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlModResc cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failed" );
    }

    if ( rows > 0 ) {
        char rowsMsg[100];
        snprintf( rowsMsg, 100, "%d rows updated",
                  rows );
        addRErrorMsg( &_ctx.comm()->rError, 0, rowsMsg );
    }

    return SUCCESS();

} // db_mod_resc_data_paths_op

// =-=-=-=-=-=-=-
// authenticate user
irods::error db_mod_resc_freespace_op(
    irods::plugin_context& _ctx,
    const char*            _resc_name,
    int                    _update_value ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_resc_name ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char updateValueStr[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlModRescFreeSpace");
    }

    if ( *_resc_name == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "resc name is empty" );
    }

    /* The following checks may not be needed long term, but
       shouldn't hurt, for now.
    */

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }
    if ( _ctx.comm()->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    snprintf( updateValueStr, MAX_NAME_LEN, "%d", _update_value );

    const auto [current_time_secs, current_time_msecs] = get_current_time();

    cllBindVars[cllBindVarCount++] = updateValueStr;
    cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
    cllBindVars[cllBindVarCount++] = current_time_secs.c_str();
    cllBindVars[cllBindVarCount++] = current_time_msecs.c_str();
    cllBindVars[cllBindVarCount++] = _resc_name;

    if ( logSQL != 0 ) {
        log_sql::debug("chlModRescFreeSpace SQL 1 ");
    }
    status = cmlExecuteNoAnswerSql(
        "update R_RESC_MAIN set free_space = ?, free_space_ts=?, modify_ts=?, modify_ts_millis=? where resc_name=?",
        &icss);
    if ( status != 0 ) {
        log_db::info("chlModRescFreeSpace cmlExecuteNoAnswerSql update failure {}", status);
        _rollback( "chlModRescFreeSpace" );
        return ERROR( status, "update freespace error" );
    }

    return SUCCESS();

} // db_mod_resc_freespace_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_reg_user_re_op(
    irods::plugin_context& _ctx,
    userInfo_t*            _user_info ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (
        !_user_info ) {
        return ERROR(
                   CAT_INVALID_ARGUMENT,
                   "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    int status;
    char seqStr[MAX_NAME_LEN];
    char userZone[MAX_NAME_LEN];
    char zoneId[MAX_NAME_LEN];

    int zoneForm;
    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    static char lastValidUserType[MAX_NAME_LEN] = "";
    static char userTypeTokenName[MAX_NAME_LEN] = "";

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegUserRE");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog is not connected" );
    }

    trimWS( _user_info->userName );
    trimWS( _user_info->userType );

    if ( !strlen( _user_info->userType ) || !strlen( _user_info->userName ) ) {
        return ERROR( CAT_INVALID_ARGUMENT, "user type or user name empty" );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
            _ctx.comm()->proxyUser.authInfo.authFlag  < LOCAL_PRIV_USER_AUTH ) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
                       _ctx.comm()->clientUser.userName,
                       _ctx.comm()->clientUser.rodsZone,
                       "",
                       &icss );
        if ( status2 != 0 ) {
            return ERROR( status2, "invalid group admin access" );
        }
        creatingUserByGroupAdmin = 1;
    }
    // =-=-=-=-=-=-=-
    /*
      Check if the user type is valid.
      This check is skipped if this process has already verified this type
      (iadmin doing a series of mkuser subcommands).
    */
    if ( *_user_info->userType == '\0' ||
            strcmp( _user_info->userType, lastValidUserType ) != 0 ) {
        char errMsg[105];
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegUserRE SQL 1 ");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _user_info->userType );
            status = cmlGetStringValueFromSql(
                         "select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?",
                         userTypeTokenName, MAX_NAME_LEN, bindVars, &icss );
        }
        if ( status == 0 ) {
            snprintf( lastValidUserType, sizeof( lastValidUserType ), "%s", _user_info->userType );
        }
        else {
            snprintf( errMsg, 100, "user_type '%s' is not valid",
                      _user_info->userType );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            return ERROR( CAT_INVALID_USER_TYPE, "invalid user type" );
        }
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( strlen( _user_info->rodsZone ) > 0 ) {
        zoneForm = 1;
        snprintf( userZone, sizeof( userZone ), "%s", _user_info->rodsZone );
    }
    else {
        zoneForm = 0;
        snprintf( userZone, sizeof( userZone ), "%s", zone.c_str() );
    }

    status = validateAndParseUserName( _user_info->userName, userName2, zoneName );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( zoneName[0] != '\0' ) {
        snprintf( userZone, sizeof( userZone ), "%s", zoneName );
        zoneForm = 2;
    }

    if ( zoneForm ) {
        /* check that the zone exists (if not defaulting to local) */
        zoneId[0] = '\0';
        if ( logSQL != 0 ) {
            log_sql::debug("chlRegUserRE SQL 5 ");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userZone );
            status = cmlGetStringValueFromSql(
                         "select zone_id from R_ZONE_MAIN where zone_name=?",
                         zoneId, MAX_NAME_LEN, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                char errMsg[105];
                snprintf( errMsg, 100,
                          "zone '%s' does not exist",
                          userZone );
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
                return ERROR( CAT_INVALID_ZONE, "invalid zone name" );
            }
            return ERROR( status, "get zone id failure" );
        }
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegUserRE SQL 2");
    }
    status = cmlGetNextSeqStr( seqStr, MAX_NAME_LEN, &icss );
    if ( status != 0 ) {
        log_db::info("chlRegUserRE cmlGetNextSeqStr failure {}", status);
        return ERROR( status, "cmlGetNextSeqStr failure" );
    }

    getNowStr( myTime );

    cllBindVars[cllBindVarCount++] = seqStr;
    cllBindVars[cllBindVarCount++] = userName2;
    cllBindVars[cllBindVarCount++] = userTypeTokenName;
    cllBindVars[cllBindVarCount++] = userZone;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegUserRE SQL 3");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_USER_MAIN (user_id, user_name, user_type_name, zone_name, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );

    if ( status != 0 ) {
        if ( status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
            char errMsg[105];
            snprintf( errMsg, 100, "Error %d %s",
                      status,
                      "CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME"
                    );
            addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        }
        _rollback( "chlRegUserRE" );
        log_db::info("chlRegUserRE insert failure {}", status);
        return ERROR( status, "insert failure" );
    }


    cllBindVars[cllBindVarCount++] = seqStr;
    cllBindVars[cllBindVarCount++] = seqStr;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegUserRE SQL 4");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_USER_GROUP (group_user_id, user_id, create_ts, modify_ts) values (?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRegUserRE insert into R_USER_GROUP failure {}", status);
        _rollback( "chlRegUserRE" );
        return ERROR( status, "insert into r_user_group failure" );
    }


    /*
      The case where the caller is specifying an authstring is used in
      some specialized cases.  Using the new table (Aug 12, 2009), this
      is now set via the chlModUser call below.  This is untested, though.
    */
    if ( strlen( _user_info->authInfo.authStr ) > 0 ) {
        status = chlModUser( _ctx.comm(), _user_info->userName, "addAuth",
                             _user_info->authInfo.authStr );
        if ( status != 0 ) {
            log_db::info("chlRegUserRE chlModUser insert auth failure {}", status);
            _rollback( "chlRegUserRE" );
            return ERROR( status, "insert auth failure" );
        }
    }

    return CODE( status );

} // db_reg_user_re_op

// =-=-=-=-=-=-=-
// commit the transaction
irods::error db_set_avu_metadata_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _attribute,
    const char*            _new_value,
    const char*            _new_unit,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type || !_name || !_attribute) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char myTime[50];
    rodsLong_t objId;
    char metaIdStr[MAX_NAME_LEN * 2]; /* twice as needed to query multiple */
    char objIdStr[MAX_NAME_LEN];

    memset( metaIdStr, 0, sizeof( metaIdStr ) );
    if ( logSQL != 0 ) {
        log_sql::debug("chlSetAVUMetadata");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlSetAVUMetadata SQL 1 ");
    }
    objId = checkAndGetObjectId(_ctx.comm(), _ctx.prop_map(), _type, _name, ACCESS_CREATE_METADATA,
                                getValByKey(_cond_input, ADMIN_KW));
    if ( objId < 0 ) {
        return ERROR( objId, "checkAndGetObjectId failed" );
    }
    snprintf( objIdStr, MAX_NAME_LEN, "%lld", objId );

    if ( logSQL != 0 ) {
        log_sql::debug("chlSetAVUMetadata SQL 2");
    }

    /* Treat unspecified unit as empty string */
    if ( _new_unit == NULL ) {
        _new_unit = "";
    }

    if (!is_valid_avu(_attribute, _new_value, _new_unit)) {
        return ERROR(CAT_INVALID_ARGUMENT, "invalid AVU");
    }

    /* Query to see if the attribute exists for this object
     *
     * If status == 0: then object has zero AVUs with matching A
     * If status == 1: then object has *exactly one* AVU with this A AND said AVU is not shared with any other object
     * If status >= 2: then at least one of:
     *                     object has multiple AVUs with this A
     *                     object has an AVU with this A and said AVU is shared with another object
     */

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _attribute );
        bindVars.push_back( objIdStr );
        status = cmlGetMultiRowStringValuesFromSql( "select meta_id from R_OBJT_METAMAP where meta_id in (select meta_id from R_META_MAIN where meta_attr_name=? AND meta_id in (select meta_id from R_OBJT_METAMAP where object_id=?))",
                 metaIdStr, MAX_NAME_LEN, 2, bindVars, &icss );
    }

    if ( status <= 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            // Need to add the metadata.
            status = chlAddAVUMetadata( _ctx.comm(), _type, _name, _attribute,
                                        _new_value, _new_unit, _cond_input );
        }
        else {
            log_db::info("chlSetAVUMetadata cmlGetMultiRowStringValuesFromSql failure {}", status);
        }
        return ERROR( status, "get avu failed" );
    }

    if ( status > 1 ) {
        /* Cannot update AVU in-place, need to do a delete with wildcards then add */
        status = chlDeleteAVUMetadata( _ctx.comm(), 1, _type, _name, _attribute, "%",
                                       "%", 1, _cond_input );
        if ( status != 0 ) {
            /* Give it a second chance
             * as per r5350:
             *   "Improve the handling of an ICAT AVU metadata concurrency case.
             *
             *    When setting user-defined meta-data (AVUs), if two Agents try to
             *    modify the the same metadata (which exists with different values in
             *    R_META_MAIN), one of the chlSetAVUMetadata fails.  These changes allow
             *    the agent to retry a second time.
             *
             *    Since this is in the case of "delete then add", the
             *    chlDeleteAVUMetadata is called with noCommit=1, but the delete fails
             *    (since the AVU was changed by the other agent), so in this case
             *    (noCommit set) it should not do the rollback.  And then the caller,
             *    chlSetAVUMetadata, can try a second time to modify the AVU."
             *
             * Essentially, this is a MASSSIVE hack that attempts to bypass a race condition
             * by TRYING TWICE. This implies other major race conditions also exist related
             * to the AVUMetadata, as well as the fact that this race condition is not
             * actually fixed. Leaving it in for now, as it is marginally better than the
             * alternative, but this is a definite TODO: Implement AVUMetadata locks.
             */
            status = chlDeleteAVUMetadata( _ctx.comm(), 1, _type, _name, _attribute, "%",
                                           "%", 1, _cond_input );
        }

        if ( status != 0 ) {
            _rollback( "chlSetAVUMetadata" );
            return ERROR( status, "delete avu metadata failed" );
        }

        status = chlAddAVUMetadata( _ctx.comm(), _type, _name, _attribute,
                                    _new_value, _new_unit, _cond_input );

        return ERROR( status, "delete avu metadata failed" );
    }

    /* Only one metaId for this Attribute and Object has been found, and the metaID is not shared */
    log_db::debug("chlSetAVUMetadata found metaId {}", metaIdStr);

    if ( logSQL != 0 ) {
        log_sql::debug("chlSetAVUMetadata SQL 4");
    }

    getNowStr( myTime );
    cllBindVarCount = 0;
    cllBindVars[cllBindVarCount++] = _new_value;
    cllBindVars[cllBindVarCount++] = _new_unit;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = metaIdStr;

    if ( logSQL != 0 ) {
        log_sql::debug("chlSetAVUMetadata SQL 5");
    }
    status = cmlExecuteNoAnswerSql( "update R_META_MAIN set meta_attr_value=?,meta_attr_unit=?,modify_ts=? where meta_id=?",
                                    &icss );

    if ( status != 0 ) {
        log_db::info("chlSetAVUMetadata cmlExecuteNoAnswerSql update failure {}", status);
        _rollback( "chlSetAVUMetadata" );
        return ERROR( status, "set avu failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlSetAVUMetadata cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failed" );
    }

    return CODE( status );
} // db_set_avu_metadata_op

#define ACCESS_MAX 999999  /* A large access value (larger than the
maximum used (i.e. for fail safe)) and
    also indicates not initialized*/
// =-=-=-=-=-=-=-
// Add an Attribute-Value [Units] pair/triple metadata item to one or
// more data objects.  This is the Wildcard version, where the
// collection/data-object name can match multiple objects).

// The return value is error code (negative) or the number of objects
// to which the AVU was associated.
irods::error db_add_avu_metadata_wild_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _attribute,
    const char*            _value,
    const char*            _units,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    const bool admin_mode = _cond_input && getValByKey(_cond_input, ADMIN_KW);

    if (admin_mode && !irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Insufficient privileges");
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type || !_name || !_attribute) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    rodsLong_t status;//, status2;
    rodsLong_t seqNum;
    rodsLong_t object_count = -1;
    char collection[MAX_NAME_LEN];
    char objectName[MAX_NAME_LEN];
    char myTime[50];
    char seqNumStr[MAX_NAME_LEN];

    if (const auto ec = splitPathByKey(_name, collection, MAX_NAME_LEN, objectName, MAX_NAME_LEN, '/'); ec < 0) {
        return ERROR(ec, fmt::format(
                     "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                     __func__, __LINE__, _name, ec));
    }

    if ( strlen( collection ) == 0 ) {
        snprintf( collection, sizeof( collection ), "%s", PATH_SEPARATOR );
        snprintf( objectName, sizeof( objectName ), "%s", _name );
    }

    if (admin_mode) {
        // Skip the permission checks and get the number of matched data objects
        // if operating in administrator mode.

        std::vector<std::string> bind_vars{objectName, collection};

        status = cmlGetIntegerValueFromSql(
             "select count(distinct data_id) from R_DATA_MAIN d "
             "inner join R_COLL_MAIN c on d.coll_id = c.coll_id "
             "where d.data_name like ? and c.coll_name like ?",
              &object_count, bind_vars, &icss);

        if (0 != status) {
            _rollback("chlAddAVUMetadataWild");
            return ERROR(status, "failed to get object count");
        }
    }
    else {
        const auto ret = determine_user_has_modify_metadata_access(
            objectName,
            collection,
            _ctx.comm()->clientUser.userName,
            _ctx.comm()->clientUser.rodsZone);

        if (!ret.ok()) {
            return PASS(ret);
        }

        object_count = ret.code();
    }

    // user has write access, set up the AVU and associate it with the data-objects
    status = findOrInsertAVU( _attribute, _value, _units );
    if ( status < 0 ) {
        log_db::info("chlAddAVUMetadataWild findOrInsertAVU failure {}", status);
        _rollback( "chlAddAVUMetadata" );
        return ERROR( status, "findOrInsertAVU failure" );
    }
    seqNum = status;

    getNowStr( myTime );
    snprintf( seqNumStr, sizeof seqNumStr, "%lld", seqNum );
    cllBindVars[cllBindVarCount++] = seqNumStr;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = objectName;
    cllBindVars[cllBindVarCount++] = collection;
    if ( logSQL != 0 ) {
        log_sql::debug("chlAddAVUMetadataWild SQL 8");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_METAMAP \
                   (object_id, meta_id, create_ts, modify_ts) \
                   select DM.data_id, ?, ?, ? from R_DATA_MAIN DM, \
                   R_COLL_MAIN CM where DM.data_name like ? \
                   and DM.coll_id=CM.coll_id and CM.coll_name like ? \
                   group by DM.data_id",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlAddAVUMetadataWild cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlAddAVUMetadataWild" );
        return ERROR( status, "insert failure" );
    }

    /* Commit */
    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlAddAVUMetadataWild cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return CODE(object_count);
} // db_add_avu_metadata_wild_op

irods::error db_add_avu_metadata_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _attribute,
    const char*            _value,
    const char*            _units,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type || !_name || !_attribute) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int itype;
    char myTime[50];
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t seqNum;
    rodsLong_t objId, status;
    char objIdStr[MAX_NAME_LEN];
    char seqNumStr[MAX_NAME_LEN];
    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlAddAVUMetadata");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _type == NULL || *_type == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "type null or empty" );
    }

    if ( _name == NULL || *_name == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "name null or empty" );
    }

    if ( _attribute == NULL || *_attribute == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "attribute null or empty" );
    }

    if ( _value == NULL || *_value == '\0' ) {
        return  ERROR( CAT_INVALID_ARGUMENT, "value null or empty" );
    }

    const bool admin_mode = _cond_input && getValByKey(_cond_input, ADMIN_KW);

    if (admin_mode && !irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Insufficient privileges");
    }

    if ( _units == NULL ) {
        _units = "";
    }

    itype = convertTypeOption( _type );
    if ( itype == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid type argument" );
    }

    // Data Objects
    if (itype == 1) {
        const auto ec = splitPathByKey(_name, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); 

        if (ec < 0) {
            return ERROR(ec, fmt::format("[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                                         __func__, __LINE__, _name, ec));
        }

        if (std::strlen(logicalParentDirName ) == 0) {
            snprintf(logicalParentDirName, sizeof(logicalParentDirName), "%s", PATH_SEPARATOR);
            snprintf(logicalEndName, sizeof(logicalEndName), "%s", _name);
        }

        if (logSQL != 0) {
            log_sql::debug("chlAddAVUMetadata SQL 2");
        }

        status = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                     _ctx.comm()->clientUser.userName,
                                     _ctx.comm()->clientUser.rodsZone,
                                     ACCESS_CREATE_METADATA, &icss, admin_mode);

        if (status < 0) {
            _rollback("chlAddAVUMetadata");
            return ERROR(status, "select data_id failed");
        }

        objId = status;
    }

    // Collections
    if (itype == 2) {
        // Check that the collection exists and user has create_metadata
        // permission, and get the collectionID.
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddAVUMetadata SQL 4");
        }

        status = cmlCheckDir(_name,
                             _ctx.comm()->clientUser.userName,
                             _ctx.comm()->clientUser.rodsZone,
                             ACCESS_CREATE_METADATA, &icss, admin_mode);

        if ( status < 0 ) {
            char errMsg[105];

            _rollback( "chlAddAVUMetadata" ); // TODO We've rolled back here, so why the extra call below?

            if ( status == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown", _name );
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            }
            else {
                _rollback( "chlAddAVUMetadata" ); // TODO Why do we rollback again?
            }

            return ERROR( status, "cmlCheckDir failed" );
        }

        objId = status;
    }

    if ( itype == 3 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddAVUMetadata SQL 5");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _name );
            bindVars.push_back( zone );
            status = cmlGetIntegerValueFromSql(
                         "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            _rollback( "chlAddAVUMetadata" );
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
            }
            return ERROR( status, "select resc_id failed" );
        }
    }

    if ( itype == 4 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        status = validateAndParseUserName( _name, userName, userZone );
        if ( status ) {
            return ERROR( status, "Invalid username format" );
        }
        if ( userZone[0] == '\0' ) {
            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
            snprintf( userZone, NAME_LEN, "%s", zone.c_str() );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddAVUMetadata SQL 6");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName );
            bindVars.push_back( userZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            _rollback( "chlAddAVUMetadata" );
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_USER, "invalid user" );
            }
            return ERROR( status, "select user_id failed" );
        }
    }

    if ( itype == 5 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL , "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddAVUMetadata SQL 7");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _name );
            status = cmlGetIntegerValueFromSql(
                         "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            _rollback( "chlAddAVUMetadata" );
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
            }
            return ERROR( status, "select failure" );
        }
    }

    status = findOrInsertAVU( _attribute, _value, _units );
    if ( status < 0 ) {
        log_db::info("chlAddAVUMetadata findOrInsertAVU failure {}", status);
        _rollback( "chlAddAVUMetadata" );
        return ERROR( status, "findOrInsertAVU failure" );
    }
    seqNum = status;

    getNowStr( myTime );
    snprintf( objIdStr, sizeof objIdStr, "%lld", objId );
    snprintf( seqNumStr, sizeof seqNumStr, "%lld", seqNum );
    cllBindVars[cllBindVarCount++] = objIdStr;
    cllBindVars[cllBindVarCount++] = seqNumStr;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlAddAVUMetadata SQL 7");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) values (?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlAddAVUMetadata cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlAddAVUMetadata" );
        return ERROR( status, "insert failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlAddAVUMetadata cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return CODE( status );
} // db_add_avu_metadata_op

irods::error db_mod_avu_metadata_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _attribute,
    const char*            _value,
    const char*            _unitsOrArg0,
    const char*            _arg1,
    const char*            _arg2,
    const char*            _arg3,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type || !_name || !_attribute) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    int status, atype;
    const char *dummy = NULL;
    const char *myUnits = "";
    const char *addAttr = "";
    const char *addValue = "";
    const char  *addUnits = NULL;

    if ( _unitsOrArg0 == NULL ) {
        return ERROR( CAT_INVALID_ARGUMENT, "unitsOrArg0 empty or null" );
    }

    atype = checkModArgType( _unitsOrArg0 );
    if ( atype == 0 ) {
        myUnits = _unitsOrArg0;
    }
    else {
        dummy = _unitsOrArg0;
    }

    status = chlDeleteAVUMetadata( _ctx.comm(), 0, _type, _name, _attribute, _value,
                                   myUnits, 1, _cond_input );
    if ( status != 0 ) {
        _rollback( "chlModAVUMetadata" );
        return ERROR( status, "delete avu metadata failed" );
    }

    bool new_attr_set = false;
    bool new_val_set = false;
    bool new_unit_set = false;

    for (auto arg : { dummy , _arg1, _arg2, _arg3 } )
    {
      if (arg == NULL) continue;
      atype = checkModArgType( arg );
      if ( atype == 1 ) {
        if (new_attr_set) {
            _rollback( "chlModAVUMetadata" );
            return ERROR( CAT_INVALID_ARGUMENT, "new attribute specified more than once" );
        } else {
            new_attr_set = true;
        }
        addAttr = arg + 2;
      }
      if ( atype == 2 ) {
        if (new_val_set) {
            _rollback( "chlModAVUMetadata" );
            return ERROR( CAT_INVALID_ARGUMENT, "new value specified more than once" );
        } else {
            new_val_set = true;
        }
        addValue = arg + 2;
      }
      if ( atype == 3 ) {
        if (new_unit_set) {
            _rollback( "chlModAVUMetadata" );
            return ERROR( CAT_INVALID_ARGUMENT, "new unit specified more than once" );
        } else {
            new_unit_set = true;
        }
        addUnits = arg + 2;
      }
    }

    if ( *addAttr  == '\0' &&
            *addValue == '\0' &&
            addUnits == NULL ) {
        _rollback( "chlModAVUMetadata" );
        return ERROR( CAT_INVALID_ARGUMENT, "arg check failed" );
    }

    if ( *addAttr == '\0' ) {
        addAttr = _attribute;
    }
    if ( *addValue == '\0' ) {
        addValue = _value;
    }
    if ( addUnits == NULL ) {
        addUnits = myUnits;
    }

    status = chlAddAVUMetadata( _ctx.comm(), _type, _name, addAttr, addValue,
                                addUnits, _cond_input );

    return CODE( status );
} // db_mod_avu_metadata_op

irods::error db_del_avu_metadata_op(
    irods::plugin_context& _ctx,
    int                    _option,
    const char*            _type,
    const char*            _name,
    const char*            _attribute,
    const char*            _value,
    const char*            _unit,
    int                    _nocommit,
    const KeyValPair*      _cond_input)
{
    const bool admin_mode = _cond_input && getValByKey(_cond_input, ADMIN_KW);

    if (admin_mode && !irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Insufficient privileges");
    }

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type || !_name || !_attribute) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int itype;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t status;
    rodsLong_t objId;
    char objIdStr[MAX_NAME_LEN];
    int allowNullUnits;
    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlDeleteAVUMetadata");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    if ( _type == NULL || *_type == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid type" );
    }

    if ( _name == NULL || *_name == '\0' ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid name" );
    }
    if ( _option != 2 ) {
        if ( _attribute == NULL || *_attribute == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid attribute" );
        }

        if ( _value == NULL || *_value == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid value" );
        }
    }

    if ( _unit == NULL ) {
        _unit = "";
    }

    itype = convertTypeOption( _type );
    if ( itype == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "invalid type" );
    }

    if ( itype == 1 ) {
        if (const auto ec = splitPathByKey(_name, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
            return ERROR(ec, fmt::format(
                         "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                         __func__, __LINE__, _name, ec));
        }

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _name );
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 1 ");
        }

        status = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                     _ctx.comm()->clientUser.userName,
                                     _ctx.comm()->clientUser.rodsZone,
                                     ACCESS_DELETE_METADATA, &icss, admin_mode);
        if ( status < 0 ) {
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }

            return ERROR( status, "delete avu failed" );
        }

        objId = status;
    }

    if ( itype == 2 ) {
        // Check that the collection exists and user has delete_metadata permission,
        // and get the collectionID.
        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 2");
        }

        status = cmlCheckDir(_name,
                             _ctx.comm()->clientUser.userName,
                             _ctx.comm()->clientUser.rodsZone,
                             ACCESS_DELETE_METADATA, &icss, admin_mode);

        if ( status < 0 ) {
            char errMsg[105];

            if ( status == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown", _name );
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
            }

            return ERROR( status, "cmlCheckDir failed" );
        }

        objId = status;
    }

    if ( itype == 3 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 3");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _name );
            bindVars.push_back( zone );
            status = cmlGetIntegerValueFromSql(
                         "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
            }
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }
            return ERROR( status, "select resc_id failed" );
        }
    }

    if ( itype == 4 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        status = validateAndParseUserName( _name, userName, userZone );
        if ( status ) {
            return ERROR( status, "Invalid username format" );
        }
        if ( userZone[0] == '\0' ) {
            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
            snprintf( userZone, sizeof( userZone ), "%s", zone.c_str() );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 4");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName );
            bindVars.push_back( userZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_USER, "invalid user" );
            }
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }
            return ERROR( status, "select user_id failed" );
        }
    }

    if ( itype == 5 ) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 5");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _name );
            status = cmlGetIntegerValueFromSql(
                         "select resc_group_id from R_RESC_GROUP where resc_group_name=?",
                         &objId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
            }
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }
            return ERROR( status, "select failure" );
        }
    }


    snprintf( objIdStr, MAX_NAME_LEN, "%lld", objId );
    if ( _option == 2 ) {
        cllBindVars[cllBindVarCount++] = objIdStr;
        cllBindVars[cllBindVarCount++] = _attribute; /* attribute is really id */

        if ( logSQL != 0 ) {
            log_sql::debug("chlDeleteAVUMetadata SQL 9");
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_OBJT_METAMAP where object_id=? and meta_id =?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure {}", status);
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }

            return ERROR( status, "delete failure" );
        }

        if ( _nocommit != 1 ) {
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                log_db::info("chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure {}", status);
                return ERROR( status, "commit failure" );
            }
        }
        return ERROR( status, "delete failure" );
    }

    cllBindVars[cllBindVarCount++] = objIdStr;
    cllBindVars[cllBindVarCount++] = _attribute;
    cllBindVars[cllBindVarCount++] = _value;
    cllBindVars[cllBindVarCount++] = _unit;

    allowNullUnits = 0;
    if ( *_unit == '\0' ) {
        allowNullUnits = 1; /* null or empty-string units */
    }
    if ( _option == 1 && *_unit == '%' && *( _unit + 1 ) == '\0' ) {
        allowNullUnits = 1; /* wildcard and just % */
    }

    if ( allowNullUnits ) {
        if ( _option == 1 ) { /* use wildcards ('like') */
            if ( logSQL != 0 ) {
                log_sql::debug("chlDeleteAVUMetadata SQL 5");
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and (meta_attr_unit like ? or meta_attr_unit IS NULL) )",
                          &icss );
        }
        else {
            if ( logSQL != 0 ) {
                log_sql::debug("chlDeleteAVUMetadata SQL 6");
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and (meta_attr_unit = ? or meta_attr_unit IS NULL) )",
                          &icss );
        }
    }
    else {
        if ( _option == 1 ) { /* use wildcards ('like') */
            if ( logSQL != 0 ) {
                log_sql::debug("chlDeleteAVUMetadata SQL 7");
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and meta_attr_unit like ?)",
                          &icss );
        }
        else {
            if ( logSQL != 0 ) {
                log_sql::debug("chlDeleteAVUMetadata SQL 8");
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and meta_attr_unit = ?)",
                          &icss );
        }
    }
    if ( status != 0 ) {
        log_db::info("chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure {}", status);
        if ( _nocommit != 1 ) {
            _rollback( "chlDeleteAVUMetadata" );
        }
        return ERROR( status, "delete failure" );
    }

    if ( _nocommit != 1 ) {
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            log_db::info("chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure {}", status);
            return ERROR( status, "commit failure" );
        }
    }

    return CODE( status );
} // db_del_avu_metadata_op

irods::error db_copy_avu_metadata_op(
    irods::plugin_context& _ctx,
    const char*            _type1,
    const char*            _type2,
    const char*            _name1,
    const char*            _name2,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if (!_type1 || !_type2 || !_name1 || !_name2) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    }

    char myTime[50];
    int status;
    rodsLong_t objId1, objId2;
    char objIdStr1[MAX_NAME_LEN];
    char objIdStr2[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlCopyAVUMetadata");
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    const bool admin_mode = _cond_input && getValByKey(_cond_input, ADMIN_KW);

    if (admin_mode && !irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Insufficient privileges");
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlCopyAVUMetadata SQL 1 ");
    }
    objId1 = checkAndGetObjectId(_ctx.comm(), _ctx.prop_map(), _type1, _name1, ACCESS_READ_METADATA, admin_mode);
    if ( objId1 < 0 ) {
        return ERROR( objId1, "checkAndGetObjectId failure" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlCopyAVUMetadata SQL 2");
    }
    objId2 = checkAndGetObjectId(_ctx.comm(), _ctx.prop_map(), _type2, _name2, ACCESS_CREATE_METADATA, admin_mode);
    if ( objId2 < 0 ) {
        return ERROR( objId2, "checkAndGetObjectId failure" );
    }

    snprintf( objIdStr1, MAX_NAME_LEN, "%lld", objId1 );
    snprintf( objIdStr2, MAX_NAME_LEN, "%lld", objId2 );

    getNowStr( myTime );
    cllBindVars[cllBindVarCount++] = objIdStr2;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = objIdStr1;
    if ( logSQL != 0 ) {
        log_sql::debug("chlCopyAVUMetadata SQL 3");
    }
    status = cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) "
                  "select ?, meta_id, ?, ? from R_OBJT_METAMAP where object_id=?",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlCopyAVUMetadata cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlCopyAVUMetadata" );
        return ERROR( status, "insert failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlCopyAVUMetadata cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return CODE( status );
} // db_copy_avu_metadata_op

irods::error db_mod_access_control_resc_op(
    irods::plugin_context& _ctx,
    const int                    _recursive_flag,
    const char*                  _access_level,
    const char*                  _user_name,
    const char*                  _zone,
    const char*                  _resc_name ) {
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myAccessStr[LONG_NAME_LEN];
    char rescIdStr[MAX_NAME_LEN];
    char *myAccessLev = NULL;
    int rmFlag = 0;
    rodsLong_t status;
    const char *myZone;
    rodsLong_t userId;
    char userIdStr[MAX_NAME_LEN];
    char myTime[50];
    rodsLong_t iVal;

    snprintf( myAccessStr, sizeof( myAccessStr ), "%s", _access_level + strlen( MOD_RESC_PREFIX ) );

    if ( strcmp( myAccessStr, AP_NULL ) == 0 ) {
        myAccessLev = ACCESS_NULL;
        rmFlag = 1;
    }
    else if ( strcmp( myAccessStr, AP_READ ) == 0 ) {
        myAccessLev = ACCESS_READ_OBJECT;
    }
    else if ( strcmp( myAccessStr, AP_WRITE ) == 0 ) {
        myAccessLev = ACCESS_MODIFY_OBJECT;
    }
    else if ( strcmp( myAccessStr, AP_OWN ) == 0 ) {
        myAccessLev = ACCESS_OWN;
    }
    else {
        char errMsg[105];
        snprintf( errMsg, 100, "access level '%s' is invalid for a resource",
                  myAccessStr );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_INVALID_ARGUMENT, "invalid argument" );
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH ) {
        /* admin, so just get the resc_id */
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControlResc SQL 1");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _resc_name );
            status = cmlGetIntegerValueFromSql(
                         "select resc_id from R_RESC_MAIN where resc_name=?",
                         &iVal, bindVars, &icss );
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_UNKNOWN_RESOURCE, "unknown resource" );
        }
        if ( status < 0 ) {
            return ERROR( status, "select resc_id failure" );
        }
        status = iVal;
    }
    else {
        status = cmlCheckResc( _resc_name,
                               _ctx.comm()->clientUser.userName,
                               _ctx.comm()->clientUser.rodsZone,
                               ACCESS_OWN,
                               &icss );
        if ( status < 0 ) {
            return ERROR( status, "cmlCheckResc error" );
        }
    }
    snprintf( rescIdStr, MAX_NAME_LEN, "%lld", status );

    /* Check that the receiving user exists and if so get the userId */
    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    myZone = _zone;
    if ( _zone == NULL || strlen( _zone ) == 0 ) {
        myZone = zone.c_str();
    }

    userId = 0;
    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControlResc SQL 2");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _user_name );
        bindVars.push_back( myZone );
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=?",
                     &userId, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_INVALID_USER, "invalid user" );
        }
        return ERROR( status, "select user_id failure" );
    }

    snprintf( userIdStr, MAX_NAME_LEN, "%lld", userId );

    /* remove any access permissions */
    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = rescIdStr;
    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControlResc SQL 3");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                  &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        return ERROR( status, "delete failure" );
    }

    /* If not just removing, add the new value */
    if ( rmFlag == 0 ) {
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = rescIdStr;
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = myAccessLev;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControlResc SQL 4");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                      &icss );
        if ( status != 0 ) {
            _rollback( "chlModAccessControlResc" );
            return ERROR( status, "insert failure" );
        }
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failure" );
    }

    return SUCCESS();

} // db_mod_access_control_resc_op

irods::error db_mod_access_control_op(
    irods::plugin_context& _ctx,
    const int                    _recursive_flag,
    const char*                  _access_level,
    const char*                  _user_name,
    const char*                  _zone,
    const char*                  _path_name ) {
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl");
    }

    if ( strncmp( _access_level, MOD_RESC_PREFIX, strlen( MOD_RESC_PREFIX ) ) == 0 ) {
        ret = db_mod_access_control_resc_op(
                  _ctx,
                  _recursive_flag,
                  _access_level,
                  _user_name,
                  _zone,
                  _path_name );
        return PASS( ret );
    }

    int adminMode = 0;
    char myAccessStr[LONG_NAME_LEN];
    if ( strncmp( _access_level, MOD_ADMIN_MODE_PREFIX,
                  strlen( MOD_ADMIN_MODE_PREFIX ) ) == 0 ) {
        if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            addRErrorMsg( &_ctx.comm()->rError, 0,
                          "You must be the admin to use the -M admin mode" );
            return ERROR( CAT_NO_ACCESS_PERMISSION, "You must be the admin to use the -M admin mode" );
        }
        snprintf( myAccessStr, sizeof( myAccessStr ), "%s", _access_level + strlen( MOD_ADMIN_MODE_PREFIX ) );
        _access_level = myAccessStr;
        adminMode = 1;
    }

    static const std::vector<char const*> allowed_access_levels{ACCESS_EXECUTE,
                                                                ACCESS_READ_ANNOTATION,
                                                                ACCESS_READ_SYSTEM_METADATA,
                                                                ACCESS_READ_METADATA,
                                                                ACCESS_READ_OBJECT,
                                                                ACCESS_WRITE_ANNOTATION,
                                                                ACCESS_CREATE_METADATA,
                                                                ACCESS_MODIFY_METADATA,
                                                                ACCESS_DELETE_METADATA,
                                                                ACCESS_ADMINISTER_OBJECT,
                                                                ACCESS_CREATE_OBJECT,
                                                                ACCESS_MODIFY_OBJECT,
                                                                ACCESS_DELETE_OBJECT,
                                                                ACCESS_CREATE_TOKEN,
                                                                ACCESS_DELETE_TOKEN,
                                                                ACCESS_CURATE,
                                                                ACCESS_OWN};
    const char* myAccessLev = NULL;
    int rmFlag = 0;
    int inheritFlag = 0;
    if ( strcmp( _access_level, AP_NULL ) == 0 ) {
        myAccessLev = ACCESS_NULL;
        rmFlag = 1;
    }
    else if ( strcmp( _access_level, AP_READ ) == 0 ) {
        myAccessLev = ACCESS_READ_OBJECT;
    }
    else if ( strcmp( _access_level, AP_WRITE ) == 0 ) {
        myAccessLev = ACCESS_MODIFY_OBJECT;
    }
    else if ( strcmp( _access_level, ACCESS_INHERIT ) == 0 ) {
        inheritFlag = 1;
    }
    else if ( strcmp( _access_level, ACCESS_NO_INHERIT ) == 0 ) {
        inheritFlag = 2;
    }
    else {
        for (char const* level : allowed_access_levels) {
            if (!strcmp(_access_level, level)) {
                myAccessLev = level;
                break;
            }
        }
        if (myAccessLev == NULL) {
            char errMsg[105];
            snprintf(errMsg, 100, "access level '%s' is invalid", _access_level);
            addRErrorMsg(&_ctx.comm()->rError, 0, errMsg);
            return ERROR(CAT_INVALID_ARGUMENT, errMsg);
        }
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    int status1;
    if ( adminMode ) {
        /* See if the input path is a collection
           and, if so, get the collectionID */
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControl SQL 14");
        }
        {
            rodsLong_t iVal = 0;
            std::vector<std::string> bindVars;
            bindVars.push_back( _path_name );
            status1 = cmlGetIntegerValueFromSql(
                          "select coll_id from R_COLL_MAIN where coll_name=?",
                          &iVal, bindVars, &icss );
            if ( status1 == CAT_NO_ROWS_FOUND ) {
                status1 = CAT_UNKNOWN_COLLECTION;
            }
            else if ( status1 == 0 ) {
                status1 = iVal;
            }
        }
    }
    else {
        /* See if the input path is a collection and the user owns it,
           and, if so, get the collectionID */
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControl SQL 1 ");
        }
        status1 = cmlCheckDir( _path_name,
                               _ctx.comm()->clientUser.userName,
                               _ctx.comm()->clientUser.rodsZone,
                               ACCESS_OWN,
                               &icss );
    }
    char collIdStr[MAX_NAME_LEN];
    if ( status1 >= 0 ) {
        snprintf( collIdStr, MAX_NAME_LEN, "%d", status1 );
    }

    if ( status1 < 0 && inheritFlag != 0 ) {
        char errMsg[105];
        snprintf( errMsg, 100, "either the collection does not exist or you do not have sufficient access" );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_NO_ACCESS_PERMISSION, errMsg );
    }

    rodsLong_t objId = 0;

    /* Not a collection (with access for non-Admin) */
    if ( status1 < 0 ) {
        char logicalEndName[MAX_NAME_LEN];
        char logicalParentDirName[MAX_NAME_LEN];
        if (const auto ec = splitPathByKey(_path_name, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
            return ERROR(ec, fmt::format(
                         "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                         __func__, __LINE__, _path_name, ec));
        }

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _path_name + 1 );
        }

        int status2 = 0;
        if ( adminMode ) {
            if ( logSQL != 0 ) {
                log_sql::debug("chlModAccessControl SQL 15");
            }
            {
                rodsLong_t iVal = 0;
                std::vector<std::string> bindVars;
                bindVars.push_back( logicalEndName );
                bindVars.push_back( logicalParentDirName );
                status2 = cmlGetIntegerValueFromSql(
                              "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
                              &iVal, bindVars, &icss );
                if ( status2 == CAT_NO_ROWS_FOUND ) {
                    status2 = CAT_UNKNOWN_FILE;
                }
                if ( status2 == 0 ) {
                    status2 = iVal;
                }
            }
        }
        else {
            /* Not a collection with access, so see if the input path dataObj
               exists and the user owns it, and, if so, get the objectID */
            if ( logSQL != 0 ) {
                log_sql::debug("chlModAccessControl SQL 2");
            }
            status2 = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                           _ctx.comm()->clientUser.userName,
                                           _ctx.comm()->clientUser.rodsZone,
                                           ACCESS_OWN, &icss );
        }
        if ( status2 > 0 ) {
            objId = status2;
        }
        /* If both failed, it doesn't exist or there's no permission */
        else if ( status2 < 0 ) {
            char errMsg[205];

            if ( status1 == CAT_UNKNOWN_COLLECTION && status2 == CAT_UNKNOWN_FILE ) {
                snprintf( errMsg, 200,
                          "Input path is not a collection and not a dataObj: %s",
                          _path_name );
                addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
                return ERROR( CAT_INVALID_ARGUMENT, "unknown collection or file" );
            }
            if ( status1 != CAT_UNKNOWN_COLLECTION ) {
                return ERROR(
                    status1,
                    fmt::format(
                        "User [{}] has insufficient permission to modifiy collection: [{}]",
                        _ctx.comm()->clientUser.userName,
                        _path_name));
            }
            else {
                if ( status2 == CAT_NO_ACCESS_PERMISSION ) {
                    return ERROR(
                        status2,
                        fmt::format(
                            "User [{}] has insufficient permission to modifiy data object: [{}]",
                            _ctx.comm()->clientUser.userName,
                            _path_name));
                }
                else {
                    return ERROR( status2, "cmlCheckDataObjOnly failed" );
                }
            }
        }
    }

    /* Doing inheritance */
    if ( inheritFlag != 0 ) {
        int status = _modInheritance( inheritFlag, _recursive_flag, collIdStr, _path_name );
        return ERROR( status, "_modInheritance failed" );
    }

    /* Check that the receiving user exists and if so get the userId */
    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    const char *myZone = ( _zone && strlen( _zone ) != 0 ) ? _zone : zone.c_str();

    rodsLong_t userId = 0;
    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl SQL 3");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _user_name );
        bindVars.push_back( myZone );
        int status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=?",
                         &userId, bindVars, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_USER, "invalid user" );
            }
            return ERROR( status, "select user_id failure" );
        }
    }

    char userIdStr[MAX_NAME_LEN];
    snprintf( userIdStr, sizeof( userIdStr ), "%lld", userId );

    char objIdStr[MAX_NAME_LEN];
    snprintf( objIdStr, sizeof( objIdStr ), "%lld", objId );

    log_db::debug("recursiveFlag {}", _recursive_flag);

    /* non-Recursive mode */
    if ( _recursive_flag == 0 ) {

        /* doing a dataObj */
        if ( objId ) {
            cllBindVars[cllBindVarCount++] = userIdStr;
            cllBindVars[cllBindVarCount++] = objIdStr;
            if ( logSQL != 0 ) {
                log_sql::debug("chlModAccessControl SQL 4");
            }
            int status = cmlExecuteNoAnswerSql(
                             "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                             &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                return ERROR( status, "delete failure" );
            }
            if ( rmFlag == 0 ) { /* if not just removing: */
                char myTime[50];
                getNowStr( myTime );
                cllBindVars[cllBindVarCount++] = objIdStr;
                cllBindVars[cllBindVarCount++] = userIdStr;
                cllBindVars[cllBindVarCount++] = myAccessLev;
                cllBindVars[cllBindVarCount++] = myTime;
                cllBindVars[cllBindVarCount++] = myTime;
                if ( logSQL != 0 ) {
                    log_sql::debug("chlModAccessControl SQL 5");
                }
                int status = cmlExecuteNoAnswerSql(
                                 "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                                 &icss );
                if ( status != 0 ) {
                    _rollback( "chlModAccessControl" );
                    return ERROR( status, "insert failure" );
                }
            }

            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return ERROR( status, "commit failure" );
        }

        /* doing a collection, non-recursive */
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = collIdStr;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControl SQL 6");
        }
        int status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                          &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            _rollback( "chlModAccessControl" );
            return ERROR( status, "delete failure" );
        }
        if ( rmFlag ) { /* just removing */
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return ERROR( status, "commit failure" );
        }

        char myTime[50];
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = collIdStr;
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = myAccessLev;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModAccessControl SQL 7");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                      &icss );

        if ( status != 0 ) {
            _rollback( "chlModAccessControl" );
            return ERROR( status, "insert failure" );
        }
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        return ERROR( status, "commit failure" );
    }


    /* Recursive */
    if ( objId ) {
        char errMsg[205];

        snprintf( errMsg, 200,
                  "Input path is not a collection and recursion was requested: %s",
                  _path_name );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_INVALID_ARGUMENT, errMsg );
    }


    std::string pathStart = makeEscapedPath( _path_name ) + "/%";
    int status;

#if (defined ORA_ICAT || defined MY_ICAT)
#else
    /* The temporary table created and used below has been found to
       greatly speed up the execution of subsequent deletes and
       updates.  We did a lot of testing and 'explain' SQL using a copy
       (minus passwords) of the large iPlant ICAT DB to find this.  It
       makes sense that using a table like this will speed it up,
       except for the constraints aspect (further below).

       It is very likely that the same temporary table would also speed
       up Oracle and MySQL ICATs but we didn't have time to test that
       so this is only for Postgres for now.  The SQL for creating the
       table is probably a bit different for Oracle and MySQL but
       otherwise I expect this would work for them.

       Before this change the SQL could take minutes on a very large
       instance.  With this, it can take less than a second on a
       'ichmod -r' on a small sub-collection, and is fairly fast on
       moderate sized ones.  I expect it will perform somewhat better
       than the old SQL on large ones, but I was unable to reliably
       test this on our fairly modest hardware.

       Since these SQL statements are only for Postgres, we can't add
       log_sql::debug( ...) calls (so 'devtest' will verify it is
       called), but since the later postgres SQL depends on this table,
       we can be sure this is exercised if "chlModAccessControl SQL 8" is.
    */
    status =  cmlExecuteNoAnswerSql( "create temporary table R_MOD_ACCESS_TEMP1 (coll_id bigint not null, coll_name varchar(2700) not null) on commit drop",
                                     &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;
    }
    if ( status != 0 ) {
        log_db::info("chlModAccessControl cmlExecuteNoAnswerSql create temp table failure {}", status);
        _rollback( "chlModAccessControl" );
        return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql create temp table failure" );
    }

    status =  cmlExecuteNoAnswerSql( "create unique index idx_r_mod_access_temp1 on R_MOD_ACCESS_TEMP1 (coll_name)",
                                     &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;
    }
    if ( status != 0 ) {
        log_db::info("chlModAccessControl cmlExecuteNoAnswerSql create index failure {}", status);
        _rollback( "chlModAccessControl" );
        return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql create index failure" );
    }

    cllBindVars[cllBindVarCount++] = _path_name;
    cllBindVars[cllBindVarCount++] = pathStart.c_str();
    status =  cmlExecuteNoAnswerSql( "insert into R_MOD_ACCESS_TEMP1 (coll_id, coll_name) select  coll_id, coll_name from R_COLL_MAIN where coll_name = ? or coll_name like ?",
                                     &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;
    }
    if ( status != 0 ) {
        log_db::info("chlModAccessControl cmlExecuteNoAnswerSql insert failure {}", status);
        _rollback( "chlModAccessControl" );
        return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql insert failure" );
    }
#endif

    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = _path_name;
    cllBindVars[cllBindVarCount++] = pathStart.c_str();

    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl SQL 8");
    }
    status =  cmlExecuteNoAnswerSql(
#if defined ORA_ICAT
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY (select data_id from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ? ESCAPE '\\'))",
#elif defined MY_ICAT
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY (select data_id from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
#else
                  /*  Use the temporary table to greatly speed up this operation
                  (and similar ones below).  The last constraint, the 'where
                  coll_name = ? or coll_name like ?' isn't really needed (since
                  that table was populated via constraints like those) but,
                  oddly, does seem to make it run much faster.  Using 'explain'
                  SQL and test runs confirmed that it is faster with those
                  constraints.
                  */
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY(ARRAY(select data_id from R_DATA_MAIN where coll_id in (select coll_id from R_MOD_ACCESS_TEMP1 where coll_name = ? or coll_name like ?)))",
#endif
                  & icss );

    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        _rollback( "chlModAccessControl" );
        return ERROR( status, "delete failure" );
    }

    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = _path_name;
    cllBindVars[cllBindVarCount++] = pathStart.c_str();

    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl SQL 9");
    }
    status =  cmlExecuteNoAnswerSql(
#if defined ORA_ICAT
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ? ESCAPE '\\')",
#elif defined MY_ICAT
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
#else
                  "delete from R_OBJT_ACCESS where user_id=? and object_id = ANY(ARRAY(select coll_id from R_MOD_ACCESS_TEMP1 where coll_name = ? or coll_name like ?))",
#endif
                  & icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        _rollback( "chlModAccessControl" );
        return ERROR( status, "delete failure" );
    }
    if ( rmFlag ) { /* just removing */
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        return ERROR( status, "commit failure" );
    }

    char myTime[50];
    getNowStr( myTime );
    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = myAccessLev;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _path_name;
    cllBindVars[cllBindVarCount++] = pathStart.c_str();
    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl SQL 10");
    }
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ? ESCAPE '\\'))",
                  &icss );
#elif MY_ICAT
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
                  &icss );
#else
    /* For Postgres, also use the temporary R_MOD_ACCESS_TEMP1 table */
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, cast(? as bigint), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_MOD_ACCESS_TEMP1 where coll_name = ? or coll_name like ?))",
                  &icss );
#endif
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;    /* no files, OK */
    }
    if ( status != 0 ) {
        _rollback( "chlModAccessControl" );
        return ERROR( status, "insert failure" );
    }


    /* Now set the collections */
    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = myAccessLev;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = _path_name;
    cllBindVars[cllBindVarCount++] = pathStart.c_str();
    if ( logSQL != 0 ) {
        log_sql::debug("chlModAccessControl SQL 11");
    }
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ? ESCAPE '\\')",
                  &icss );
#elif MY_ICAT
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
                  &icss );
#else
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, cast(? as bigint), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
                  &icss );
#endif
    if ( status != 0 ) {
        _rollback( "chlModAccessControl" );
        return ERROR( status, "insert failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }

    return CODE( status );

} // db_mod_access_control_op

irods::error db_rename_object_op(
    irods::plugin_context& _ctx,
    rodsLong_t             _obj_id,
    const char*            _new_name ) {
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    rodsLong_t collId;
    rodsLong_t otherDataId;
    rodsLong_t otherCollId;
    char myTime[50];

    char parentCollName[MAX_NAME_LEN] = "";
    char collName[MAX_NAME_LEN] = "";
    char *cVal[3];
    int iVal[3];
    int pLen, cLen, len;
    int isRootDir = 0;
    char objIdString[MAX_NAME_LEN];
    char collIdString[MAX_NAME_LEN];
    char collNameTmp[MAX_NAME_LEN];

    char pLenStr[MAX_NAME_LEN];
    char cLenStr[MAX_NAME_LEN];
    char collNameSlash[MAX_NAME_LEN];
    char collNameSlashLen[20];
    char slashNewName[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameObject");
    }

    if ( strstr( _new_name, PATH_SEPARATOR ) ) {
        return ERROR( CAT_INVALID_ARGUMENT, "new name invalid" );
    }

    /* See if it's a dataObj and if so get the coll_id
       check the access permission at the same time */
    collId = 0;

    snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameObject SQL 1 ");
    }

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                     &collId, bindVars,  &icss );
    }

    if ( status == 0 ) { /* it is a dataObj and user has access to it */

        /* check that no other dataObj exists with this name in this collection*/
        snprintf( collIdString, MAX_NAME_LEN, "%lld", collId );
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 2");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _new_name );
            bindVars.push_back( collIdString );
            status = cmlGetIntegerValueFromSql(
                         "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
                         &otherDataId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "select data_id failed" );
        }

        /* check that no subcoll exists in this collection,
           with the _new_name */
        snprintf( collNameTmp, MAX_NAME_LEN, "/%s", _new_name );
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 3");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( collIdString );
            bindVars.push_back( collNameTmp );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_name = ( select coll_name from R_COLL_MAIN where coll_id=? ) || ?",
                         &otherCollId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_COLLECTION, "select coll_id failed" );
        }

        /* update the tables */
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = _new_name;
        cllBindVars[cllBindVarCount++] = objIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 4");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_DATA_MAIN set data_name = ? where data_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlRenameObject cmlExecuteNoAnswerSql update1 failure {}", status);
            _rollback( "chlRenameObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update1 failure" );
        }

        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = collIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 5");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set modify_ts=? where coll_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlRenameObject cmlExecuteNoAnswerSql update2 failure {}", status);
            _rollback( "chlRenameObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update2 failure" );
        }

        return CODE( status );
    }

    /* See if it's a collection, and get the parentCollName and
       collName, and check permission at the same time */

    cVal[0] = parentCollName;
    iVal[0] = MAX_NAME_LEN;
    cVal[1] = collName;
    iVal[1] = MAX_NAME_LEN;

    snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameObject SQL 6");
    }

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValuesFromSql(
                     "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                     cVal, iVal, 2, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it is a collection and user has access to it */

        /* check that no other dataObj exists with this name in this collection*/
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 7");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _new_name );
            bindVars.push_back( parentCollName );
            status = cmlGetIntegerValueFromSql(
                         "select data_id from R_DATA_MAIN where data_name=? and coll_id= (select coll_id from R_COLL_MAIN  where coll_name = ?)",
                         &otherDataId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "select data_id failed" );
        }

        /* check that no subcoll exists in the parent collection,
           with the _new_name */
        snprintf( collNameTmp, MAX_NAME_LEN, "%s/%s", parentCollName, _new_name );
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 8");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( collNameTmp );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_name = ?",
                         &otherCollId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_COLLECTION, "select coll_id failed" );
        }

        /* update the table */
        pLen = strlen( parentCollName );
        cLen = strlen( collName );
        if ( pLen <= 0 || cLen <= 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "coll name or parent is invalid" );
        }  /* invalid
                                                                   argument is not really right, but something is really wrong */

        if ( pLen == 1 ) {
            if ( strncmp( parentCollName, PATH_SEPARATOR, 20 ) == 0 ) { /* just to be sure */
                isRootDir = 1; /* need to treat a little special below */
            }
        }

        /* set any collection names that are under this collection to
           the new name, putting the string together from the the old upper
           part, _new_name string, and then (if any for each row) the
           tailing part of the name.
           (In the sql substr function, the index for sql is 1 origin.) */
        snprintf( pLenStr, MAX_NAME_LEN, "%d", pLen ); /* formerly +1 but without is
                                                       correct, makes a difference in Oracle, and works
                                                       in postgres too. */
        snprintf( cLenStr, MAX_NAME_LEN, "%d", cLen + 1 );
        snprintf( collNameSlash, MAX_NAME_LEN, "%s/", collName );
        len = strlen( collNameSlash );
        snprintf( collNameSlashLen, 10, "%d", len );
        snprintf( slashNewName, MAX_NAME_LEN, "/%s", _new_name );
        if ( isRootDir ) {
            snprintf( slashNewName, MAX_NAME_LEN, "%s", _new_name );
        }
        cllBindVars[cllBindVarCount++] = pLenStr;
        cllBindVars[cllBindVarCount++] = slashNewName;
        cllBindVars[cllBindVarCount++] = cLenStr;
        cllBindVars[cllBindVarCount++] = collNameSlashLen;
        cllBindVars[cllBindVarCount++] = collNameSlash;
        cllBindVars[cllBindVarCount++] = collName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 9");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_name = substr(coll_name,1,?) || ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlRenameObject cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlRenameObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* like above, but for the parent_coll_name's */
        cllBindVars[cllBindVarCount++] = pLenStr;
        cllBindVars[cllBindVarCount++] = slashNewName;
        cllBindVars[cllBindVarCount++] = cLenStr;
        cllBindVars[cllBindVarCount++] = collNameSlashLen;
        cllBindVars[cllBindVarCount++] = collNameSlash;
        cllBindVars[cllBindVarCount++] = collName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 10");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set parent_coll_name = substr(parent_coll_name,1,?) || ? || substr(parent_coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlRenameObject cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlRenameObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* And now, update the row for this collection */
        getNowStr( myTime );
        snprintf( collNameTmp, MAX_NAME_LEN, "%s/%s", parentCollName, _new_name );
        if ( isRootDir ) {
            snprintf( collNameTmp, MAX_NAME_LEN, "%s%s", parentCollName, _new_name );
        }
        cllBindVars[cllBindVarCount++] = collNameTmp;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = objIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlRenameObject SQL 11");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_name=?, modify_ts=? where coll_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlRenameObject cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlRenameObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        return CODE( status );
    }


    /* Both collection and dataObj failed, go thru the sql in smaller
       steps to return a specific error */

    snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameObject SQL 12");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_DATA_MAIN where data_id=?",
                     &otherDataId, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it IS a data obj, must be permission error */
        return ERROR( CAT_NO_ACCESS_PERMISSION, "select coll_id failed" );
    }

    snprintf( collIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlRenameObject SQL 12");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( collIdString );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_id=?",
                     &otherDataId, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it IS a collection, must be permission error */
        return ERROR( CAT_NO_ACCESS_PERMISSION, "select coll_id failed" );
    }

    return ERROR( CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION, "not a collection" );

} // db_rename_object_op

irods::error db_move_object_op(
    irods::plugin_context& _ctx,
    rodsLong_t             _obj_id,
    rodsLong_t             _target_coll_id ) {
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    rodsLong_t collId;
    rodsLong_t otherDataId;
    rodsLong_t otherCollId;
    char myTime[50];

    char dataObjName[MAX_NAME_LEN] = "";
    char *cVal[3];
    int iVal[3];

    char parentCollName[MAX_NAME_LEN] = "";
    char oldCollName[MAX_NAME_LEN] = "";
    char endCollName[MAX_NAME_LEN] = "";  /* for example: d1 portion of
                                           /tempZone/home/d1  */

    char targetCollName[MAX_NAME_LEN] = "";
    char parentTargetCollName[MAX_NAME_LEN] = "";
    char newCollName[MAX_NAME_LEN] = "";
    int pLen, ocLen;
    int i, OK, len;
    char *cp;
    char objIdString[MAX_NAME_LEN];
    char collIdString[MAX_NAME_LEN];
    char nameTmp[MAX_NAME_LEN];
    char ocLenStr[MAX_NAME_LEN];
    char collNameSlash[MAX_NAME_LEN];
    char collNameSlashLen[20];

    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject");
    }

    /* check that the target collection exists and user has write
       permission, and get the names while at it */
    cVal[0] = parentTargetCollName;
    iVal[0] = MAX_NAME_LEN;
    cVal[1] = targetCollName;
    iVal[1] = MAX_NAME_LEN;
    snprintf( objIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValuesFromSql(
            "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN "
            "UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and "
            "UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and "
            "UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' "
            "and TM.token_name = 'modify_object'",
            cVal,
            iVal,
            2,
            bindVars,
            &icss);
    }
    snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
    if ( status != 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 2");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( collIdString );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_id=?",
                         &collId, bindVars, &icss );
        }
        if ( status == 0 ) {
            return  ERROR( CAT_NO_ACCESS_PERMISSION, "permission error" );  /* does exist, must be
                                                   permission error */
        }
        return ERROR( CAT_UNKNOWN_COLLECTION, "target is not a collection" );      /* isn't a coll */
    }


    /* See if we're moving a dataObj and if so get the data_name;
       and at the same time check the access permission */
    snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject SQL 3");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValueFromSql(
                     "select data_name from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                     dataObjName, MAX_NAME_LEN, bindVars, &icss );
    }
    snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
    if ( status == 0 ) { /* it is a dataObj and user has access to it */

        /* check that no other dataObj exists with the ObjName in the
           target collection */
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 4");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( dataObjName );
            bindVars.push_back( collIdString );
            status = cmlGetIntegerValueFromSql(
                         "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
                         &otherDataId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "select data_id failed" );
        }

        /* check that no subcoll exists in the target collection, with
           the name of the object */
        /* //not needed, I think   snprintf(collIdString, MAX_NAME_LEN, "%d", collId); */
        snprintf( nameTmp, MAX_NAME_LEN, "/%s", dataObjName );
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 5");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( collIdString );
            bindVars.push_back( nameTmp );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_name = ( select coll_name from R_COLL_MAIN where coll_id=? ) || ?",
                         &otherCollId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_COLLECTION, "select coll_id failed" );
        }

        /* update the table */
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = collIdString;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = objIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 6");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_DATA_MAIN set coll_id=?, modify_ts=? where data_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlMoveObject cmlExecuteNoAnswerSql update1 failure {}", status);
            _rollback( "chlMoveObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update1 failure" );
        }


        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = collIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 7");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set modify_ts=? where coll_id=?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlMoveObject cmlExecuteNoAnswerSql update2 failure {}", status);
            _rollback( "chlMoveObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update2 failure" );
        }

        return CODE( status );
    }

    /* See if it's a collection, and get the parentCollName and
       oldCollName, and check permission at the same time */
    cVal[0] = parentCollName;
    iVal[0] = MAX_NAME_LEN;
    cVal[1] = oldCollName;
    iVal[1] = MAX_NAME_LEN;

    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject SQL 8");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetStringValuesFromSql(
                     "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                     cVal, iVal, 2, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it is a collection and user has access to it */

        pLen = strlen( parentCollName );

        ocLen = strlen( oldCollName );
        if ( pLen <= 0 || ocLen <= 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "parent or coll name null" );
        }  /* invalid
                                                                    argument is not really the right error code, but something
                                                                    is really wrong */
        OK = 0;
        for ( i = ocLen; i > 0; i-- ) {
            if ( oldCollName[i] == '/' ) {
                OK = 1;
                snprintf( endCollName, sizeof( endCollName ), "%s", ( char* )&oldCollName[i + 1] );
                break;
            }
        }
        if ( OK == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "OK == 0" );    /* not really, but...*/
        }

        /* check that the user has write access to the source collection */
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 9");
        }
        status = cmlCheckDir( parentCollName,  _ctx.comm()->clientUser.userName,
                              _ctx.comm()->clientUser.rodsZone,
                              ACCESS_MODIFY_OBJECT, &icss );
        if ( status < 0 ) {
            return ERROR( status, "cmlCheckDir failed" );
        }

        /* check that no other dataObj exists with the ObjName in the
           target collection */
        snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 10");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( endCollName );
            bindVars.push_back( collIdString );
            status = cmlGetIntegerValueFromSql(
                         "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
                         &otherDataId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "select data_id failed" );
        }

        /* check that no subcoll exists in the target collection, with
           the name of the object */
        snprintf( newCollName, sizeof( newCollName ), "%s", targetCollName );
        strncat( newCollName, PATH_SEPARATOR, strlen(PATH_SEPARATOR) );
        strncat( newCollName, endCollName, strlen(endCollName) );

        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 11");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( newCollName );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_COLL_MAIN where coll_name = ?",
                         &otherCollId, bindVars, &icss );
        }
        if ( status != CAT_NO_ROWS_FOUND ) {
            return ERROR( CAT_NAME_EXISTS_AS_COLLECTION, "select coll_id failed" );
        }


        /* Check that we're not moving the coll down into it's own
           subtree (which would create a recursive loop) */
        cp = strstr( targetCollName, oldCollName );
        if ( cp == targetCollName &&
                ( targetCollName[strlen( oldCollName )] == '/' ||
                  targetCollName[strlen( oldCollName )] == '\0' ) ) {
            return ERROR( CAT_RECURSIVE_MOVE, "moving coll into own subtree" );
        }


        /* Update the table */

        /* First, set the collection name and parent collection to the
        new strings, and update the modify-time */
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = newCollName;
        cllBindVars[cllBindVarCount++] = targetCollName;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = objIdString;
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 12");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_name = ?, parent_coll_name=?, modify_ts=? where coll_id = ?",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlMoveObject cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlMoveObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* Now set any collection names that are under this collection to
           the new name, putting the string together from the the new upper
           part, endCollName string, and then (if any for each row) the
           tailing part of the name.
           (In the sql substr function, the index for sql is 1 origin.) */
        snprintf( ocLenStr, MAX_NAME_LEN, "%d", ocLen + 1 );
        snprintf( collNameSlash, MAX_NAME_LEN, "%s/", oldCollName );
        len = strlen( collNameSlash );
        snprintf( collNameSlashLen, 10, "%d", len );
        cllBindVars[cllBindVarCount++] = newCollName;
        cllBindVars[cllBindVarCount++] = ocLenStr;
        cllBindVars[cllBindVarCount++] = newCollName;
        cllBindVars[cllBindVarCount++] = ocLenStr;
        cllBindVars[cllBindVarCount++] = collNameSlashLen;
        cllBindVars[cllBindVarCount++] = collNameSlash;
        cllBindVars[cllBindVarCount++] = oldCollName;
        if ( logSQL != 0 ) {
            log_sql::debug("chlMoveObject SQL 13");
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set parent_coll_name = ? || substr(parent_coll_name, ?), coll_name = ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name = ?",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            log_db::info("chlMoveObject cmlExecuteNoAnswerSql update failure {}", status);
            _rollback( "chlMoveObject" );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        return CODE( status );
    }


    /* Both collection and dataObj failed, go thru the sql in smaller
       steps to return a specific error */
    snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject SQL 14");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_DATA_MAIN where data_id=?",
                     &otherDataId, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it IS a data obj, must be permission error */
        return ERROR( CAT_NO_ACCESS_PERMISSION, "select coll_id failed" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlMoveObject SQL 15");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( objIdString );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_id=?",
                     &otherDataId, bindVars, &icss );
    }
    if ( status == 0 ) {
        /* it IS a collection, must be permission error */
        return  ERROR( CAT_NO_ACCESS_PERMISSION, "select coll_id failed" );
    }

    return ERROR( CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION, "invalid object or collection" );

} // db_move_object_op

irods::error db_reg_token_op(
    irods::plugin_context& _ctx,
    const char*            _name_space,
    const char*            _name,
    const char*            _value,
    const char*            _value2,
    const char*            _value3,
    const char*            _comment ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    rodsLong_t objId;
    const char *myValue1;
    const char *myValue2;
    const char *myValue3;
    const char *myComment;
    char myTime[50];
    rodsLong_t seqNum;
    char errMsg[205];
    char seqNumStr[MAX_NAME_LEN];

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegToken");
    }

    if ( _name_space == NULL || strlen( _name_space ) == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "namespace null or 0 len" );
    }
    if ( _name == NULL || strlen( _name ) == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "name null or 0 len" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegToken SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( "token_namespace" );
        bindVars.push_back( _name_space );
        status = cmlGetIntegerValueFromSql(
                     "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
                     &objId, bindVars, &icss );
    }
    if ( status != 0 ) {
        snprintf( errMsg, 200,
                  "Token namespace '%s' does not exist",
                  _name_space );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return  ERROR( CAT_INVALID_ARGUMENT, "namespace does not exist" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegToken SQL 2");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _name_space );
        bindVars.push_back( _name );
        status = cmlGetIntegerValueFromSql(
                     "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
                     &objId, bindVars, &icss );
    }
    if ( status == 0 ) {
        snprintf( errMsg, 200,
                  "Token '%s' already exists in namespace '%s'",
                  _name, _name_space );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return ERROR( CAT_INVALID_ARGUMENT, "token is already in namespace" );
    }

    myValue1 = _value;
    if ( myValue1 == NULL ) {
        myValue1 = "";
    }
    myValue2 = _value2;
    if ( myValue2 == NULL ) {
        myValue2 = "";
    }
    myValue3 = _value3;
    if ( myValue3 == NULL ) {
        myValue3 = "";
    }
    myComment = _comment;
    if ( myComment == NULL ) {
        myComment = "";
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegToken SQL 3");
    }
    seqNum = cmlGetNextSeqVal( &icss );
    if ( seqNum < 0 ) {
        log_db::info("chlRegToken cmlGetNextSeqVal failure {}", seqNum);
        return ERROR( seqNum, "chlRegToken cmlGetNextSeqVal failure" );
    }

    getNowStr( myTime );
    snprintf( seqNumStr, sizeof seqNumStr, "%lld", seqNum );
    cllBindVars[cllBindVarCount++] = _name_space;
    cllBindVars[cllBindVarCount++] = seqNumStr;
    cllBindVars[cllBindVarCount++] = _name;
    cllBindVars[cllBindVarCount++] = myValue1;
    cllBindVars[cllBindVarCount++] = myValue2;
    cllBindVars[cllBindVarCount++] = myValue3;
    cllBindVars[cllBindVarCount++] = myComment;
    cllBindVars[cllBindVarCount++] = myTime;
    cllBindVars[cllBindVarCount++] = myTime;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegToken SQL 4");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_TOKN_MAIN values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        _rollback( "chlRegToken" );
        return ERROR( status, "insert failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit error" );
    }
    else {
        return CODE( status );
    }

} // db_reg_token_op

irods::error db_del_token_op(
    irods::plugin_context& _ctx,
    const char*            _name_space,
    const char*            _name ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );


    int status;
    rodsLong_t objId;
    char errMsg[205];

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelToken");
    }

    if ( _name_space == NULL || strlen( _name_space ) == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "namespace is null or 0 len" );
    }
    if ( _name == NULL || strlen( _name ) == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "name is null or 0 len" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelToken SQL 1 ");
    }
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _name_space );
        bindVars.push_back( _name );
        status = cmlGetIntegerValueFromSql(
                     "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
                     &objId, bindVars, &icss );
    }
    if ( status != 0 ) {
        snprintf( errMsg, 200,
                  "Token '%s' does not exist in namespace '%s'",
                  _name, _name_space );
        addRErrorMsg( &_ctx.comm()->rError, 0, errMsg );
        return  ERROR( CAT_INVALID_ARGUMENT, "token is not in namespace" );
    }

    if ( logSQL != 0 ) {
        log_sql::debug("chlDelToken SQL 2");
    }
    cllBindVars[cllBindVarCount++] = _name_space;
    cllBindVars[cllBindVarCount++] = _name;
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_TOKN_MAIN where token_namespace=? and token_name=?",
                  &icss );
    if ( status != 0 ) {
        _rollback( "chlDelToken" );
        return ERROR( status, "delete failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_del_token_op

irods::error db_reg_server_load_op(
    irods::plugin_context& _ctx,
    const char*            _host_name,
    const char*            _resc_name,
    const char*            _cpu_used,
    const char*            _mem_used,
    const char*            _swap_used,
    const char*            _run_q_load,
    const char*            _disk_space,
    const char*            _net_input,
    const char*            _net_output ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    int status;
    int i;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegServerLoad");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    getNowStr( myTime );

    i = 0;
    cllBindVars[i++] = _host_name;
    cllBindVars[i++] = _resc_name;
    cllBindVars[i++] = _cpu_used;
    cllBindVars[i++] = _mem_used;
    cllBindVars[i++] = _swap_used;
    cllBindVars[i++] = _run_q_load;
    cllBindVars[i++] = _disk_space;
    cllBindVars[i++] = _net_input;
    cllBindVars[i++] = _net_output;
    cllBindVars[i++] = myTime;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlRegServerLoad SQL 1");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_SERVER_LOAD (host_name, resc_name, cpu_used, mem_used, swap_used, runq_load, disk_space, net_input, net_output, create_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRegServerLoad cmlExecuteNoAnswerSql failure {}", status);
        _rollback( "chlRegServerLoad" );
        return ERROR( status, "cmlExecuteNoAnswerSql failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegServerLoad cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return SUCCESS();

} // db_reg_server_load_op

irods::error db_purge_server_load_op(
    irods::plugin_context& _ctx,
    const char*            _seconds_ago ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    // =-=-=-=-=-=-=-
    // delete from R_LOAD_SERVER where (%i -exe_time) > %i
    int status;
    char nowStr[50];
    static char thenStr[50];
    time_t nowTime;
    time_t thenTime;
    time_t secondsAgoTime;

    if ( logSQL != 0 ) {
        log_sql::debug("chlPurgeServerLoad");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    getNowStr( nowStr );
    nowTime = atoll( nowStr );
    secondsAgoTime = atoll( _seconds_ago );
    thenTime = nowTime - secondsAgoTime;
    snprintf( thenStr, sizeof thenStr, "%011d", ( uint ) thenTime );

    if ( logSQL != 0 ) {
        log_sql::debug("chlPurgeServerLoad SQL 1");
    }

    cllBindVars[cllBindVarCount++] = thenStr;
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_SERVER_LOAD where create_ts <?",
                  &icss );
    if ( status != 0 ) {
        _rollback( "chlPurgeServerLoad" );
        return ERROR( status, "delete failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_purge_server_load_op

irods::error db_reg_server_load_digest_op(
    irods::plugin_context& _ctx,
    const char*            _resc_name,
    const char*            _load_factor ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    char myTime[50];
    int status;
    int i;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegServerLoadDigest");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    getNowStr( myTime );

    i = 0;
    cllBindVars[i++] = _resc_name;
    cllBindVars[i++] = _load_factor;
    cllBindVars[i++] = myTime;
    cllBindVarCount = i;

    if ( logSQL != 0 ) {
        log_sql::debug("chlRegServerLoadDigest SQL 1");
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_SERVER_LOAD_DIGEST (resc_name, load_factor, create_ts) values (?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlRegServerLoadDigest cmlExecuteNoAnswerSql failure {}", status);
        _rollback( "chlRegServerLoadDigest" );
        return ERROR( status, "cmlExecuteNoAnswerSql failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        log_db::info("chlRegServerLoadDigest cmlExecuteNoAnswerSql commit failure {}", status);
        return ERROR( status, "commit failure" );
    }

    return SUCCESS();

} // db_reg_server_load_digest_op

irods::error db_purge_server_load_digest_op(
    irods::plugin_context& _ctx,
    const char*            _seconds_ago ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    // =-=-=-=-=-=-=-
    /* delete from R_SERVER_LOAD_DIGEST where (%i -exe_time) > %i */
    int status;
    char nowStr[50];
    static char thenStr[50];
    time_t nowTime;
    time_t thenTime;
    time_t secondsAgoTime;

    if ( logSQL != 0 ) {
        log_sql::debug("chlPurgeServerLoadDigest");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    getNowStr( nowStr );
    nowTime = atoll( nowStr );
    secondsAgoTime = atoll( _seconds_ago );
    thenTime = nowTime - secondsAgoTime;
    snprintf( thenStr, sizeof thenStr, "%011d", ( uint ) thenTime );

    if ( logSQL != 0 ) {
        log_sql::debug("chlPurgeServerLoadDigest SQL 1");
    }

    cllBindVars[cllBindVarCount++] = thenStr;
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_SERVER_LOAD_DIGEST where create_ts <?",
                  &icss );
    if ( status != 0 ) {
        _rollback( "chlPurgeServerLoadDigest" );
        return ERROR( status, "delete failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_purge_server_load_digest_op

irods::error db_get_grid_configuration_value_op(
    irods::plugin_context& _ctx,
    const char*            _namespace,
    const char*            _option_name,
    char*                  _option_value,
    std::size_t            _option_value_buffer_size)
{
    // =-=-=-=-=-=-=-
    // check the context
    if (const irods::error ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (logSQL != 0) {
        log_sql::debug("chlGetGridConfigurationValue");
    }

    if (!icss.status) {
        return ERROR(CATALOG_NOT_CONNECTED, "catalog not connected");
    }

    std::vector<std::string> bindVars{_namespace, _option_name};

    const int status = cmlGetStringValueFromSql(
         "select option_value from R_GRID_CONFIGURATION where namespace = ? and option_name = ?",
         _option_value, _option_value_buffer_size, bindVars, &icss);

    if (status < 0) {
        log_db::info("chlGetGridConfigurationValue cmlGetStringValueFromSql failure {}", status);
        return ERROR(status, "Get Grid Configuration Value select failure");
    }

    return SUCCESS();
} // db_get_grid_configuration_value_op

irods::error db_set_grid_configuration_value_op(
    irods::plugin_context& _ctx,
    const char*            _namespace,
    const char*            _option_name,
    const char*            _option_value)
{
    if (!irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level");
    }

    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (logSQL != 0) {
        log_sql::debug("chlSetGridConfigurationValue");
    }

    if (!icss.status) {
        return ERROR(CATALOG_NOT_CONNECTED, "catalog not connected");
    }

    constexpr std::size_t grid_configuration_size = 2700;
    std::vector<std::string> bindVars{_namespace, _option_name};
    std::array<char, grid_configuration_size + 1> config_value{};
    int status = cmlGetStringValueFromSql(
        "select option_value from R_GRID_CONFIGURATION where namespace = ? and option_name = ?",
        config_value.data(),
        config_value.size(),
        bindVars,
        &icss);

    if (status < 0) {
        log_db::info("chlSetGridConfigurationValue cmlGetStringValueFromSql failure {}", status);
        return ERROR(status, "Set Grid Configuration Value select failure");
    }

    int i = 0;
    cllBindVars[i++] = _option_value;
    cllBindVars[i++] = _namespace;
    cllBindVars[i++] = _option_name;
    cllBindVarCount = i;
    if (logSQL != 0) {
        log_sql::debug("chlSetGridConfigurationValue  SQL 1");
    }

    status = cmlExecuteNoAnswerSql(
        "update R_GRID_CONFIGURATION set option_value = ? where namespace = ? and option_name = ?", &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        _rollback("chlSetGridConfigurationValue");
        log_db::info("chlSetGridConfigurationValue cmlExecuteNoAnswerSql failure {}", status);
        return ERROR(status, "Set Grid Configuration Value SQL update failure");
    }

    status = cmlExecuteNoAnswerSql("commit", &icss);
    if (status < 0) {
        return ERROR(status, "commit failed");
    }

    return SUCCESS();
} // db_set_grid_configuration_value_op

irods::error db_calc_usage_and_quota_op(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    char myTime[50];

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    log_db::info("chlCalcUsageAndQuota called");

    getNowStr( myTime );

    /* Delete the old rows from R_QUOTA_USAGE */
    if ( logSQL != 0 ) {
        log_sql::debug("chlCalcUsageAndQuota SQL 1");
    }
    cllBindVars[cllBindVarCount++] = myTime;
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_QUOTA_USAGE where modify_ts < ?", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        _rollback( "chlCalcUsageAndQuota" );
        return ERROR( status, "delete failed" );
    }

    /* Add a row to R_QUOTA_USAGE for each user's usage on each resource */
    if ( logSQL != 0 ) {
        log_sql::debug("chlCalcUsageAndQuota SQL 2");
    }
    cllBindVars[cllBindVarCount++] = myTime;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_QUOTA_USAGE (quota_usage, resc_id, user_id, modify_ts) (select sum(R_DATA_MAIN.data_size), R_RESC_MAIN.resc_id, R_USER_MAIN.user_id, ? from R_DATA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_name = R_DATA_MAIN.data_owner_name and R_USER_MAIN.zone_name = R_DATA_MAIN.data_owner_zone and R_RESC_MAIN.resc_id = R_DATA_MAIN.resc_id group by R_RESC_MAIN.resc_id, user_id)",
                  &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        status = 0;    /* no files, OK */
    }
    if ( status != 0 ) {
        _rollback( "chlCalcUsageAndQuota" );
        return ERROR( status, "insert failed" );
    }

    /* Set the over_quota flags where appropriate */
    status = setOverQuota( _ctx.comm() );
    if ( status != 0 ) {
        _rollback( "chlCalcUsageAndQuota" );
        return ERROR( status, "setOverQuota failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_calc_usage_and_quota_op

irods::error db_set_quota_op(
    irods::plugin_context& _ctx,
    const char*            _type,
    const char*            _name,
    const char*            _resc_name,
    const char*            _limit ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    rodsLong_t rescId;
    rodsLong_t userId;
    char userZone[NAME_LEN];
    char userName[NAME_LEN];
    char rescIdStr[60];
    char userIdStr[60];
    char myTime[50];
    int itype = 0;

    if ( strncmp( _type, "user", 4 ) == 0 ) {
        itype = 1;
    }
    if ( strncmp( _type, "group", 5 ) == 0 ) {
        itype = 2;
    }
    if ( itype == 0 ) {
        return ERROR( CAT_INVALID_ARGUMENT, _type );
    }

    std::string zone;
    ret = getLocalZone( _ctx.prop_map(), &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    /* Get the resource id; use rescId=0 for 'total' */
    rescId = 0;
    if ( strncmp( _resc_name, "total", 5 ) != 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlSetQuota SQL 1");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _resc_name );
            bindVars.push_back( zone );
            status = cmlGetIntegerValueFromSql(
                         "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                         &rescId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_RESOURCE, _resc_name );
            }
            _rollback( "chlSetQuota" );
            return ERROR( status, "select resc_id failed" );
        }
    }

    status = validateAndParseUserName( _name, userName, userZone );
    if ( status ) {
        return ERROR( status, "Invalid username format" );
    }
    if ( userZone[0] == '\0' ) {
        snprintf( userZone, sizeof( userZone ), "%s", zone.c_str() );
    }

    if (!_limit) {
        const auto msg = "Invalid argument for quota limit. Received null input argument.";
        log_db::error(msg);
        return ERROR(SYS_INVALID_INPUT_PARAM, msg);
    }

    std::int64_t int_limit = -1;

    if (const auto [ptr, ec] = std::from_chars(_limit, _limit + std::strlen(_limit), int_limit); ec != std::errc{}) {
        const auto msg = fmt::format("Invalid argument for quota limit. Could not convert [{}] to an integer.", _limit);
        log_db::error(msg);
        return ERROR(SYS_INVALID_INPUT_PARAM, msg);
    }

    // Handling user quota.
    //
    // Look up the entry that matches the given username and zone which does not have a type
    // of "rodsgroup".
    if ( itype == 1 ) {
        if (int_limit != 0) {
            const auto msg = fmt::format("Setting user quota limit to anything other than zero is not allowed. "
                                         "Received [{}].",
                                         int_limit);
            log_db::error(msg);
            return ERROR(SYS_NOT_ALLOWED, msg);
        }

        userId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlSetQuota SQL 2");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName );
            bindVars.push_back( userZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name!='rodsgroup'",
                         &userId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_USER, userName );
            }
            _rollback( "chlSetQuota" );
            return ERROR( status, "select user_id failed" );
        }
    }
    // Handling group quota.
    //
    // Because groups are represented as entries in r_user_main, look up the entry that matches
    // the given username and zone which has a type of "rodsgroup".
    else {
        userId = 0;
        if ( logSQL != 0 ) {
            log_sql::debug("chlSetQuota SQL 3");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userName );
            bindVars.push_back( userZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name='rodsgroup'",
                         &userId, bindVars, &icss );
        }
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_INVALID_GROUP, "invalid group" );
            }
            _rollback( "chlSetQuota" );
            return ERROR( status, "select failure" );
        }
    }

    snprintf( userIdStr, sizeof userIdStr, "%lld", userId );
    snprintf( rescIdStr, sizeof rescIdStr, "%lld", rescId );

    /* first delete previous one, if any */
    cllBindVars[cllBindVarCount++] = userIdStr;
    cllBindVars[cllBindVarCount++] = rescIdStr;
    if ( logSQL != 0 ) {
        log_sql::debug("chlSetQuota SQL 4");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_QUOTA_MAIN where user_id=? and resc_id=?",
                  &icss );
    if ( status != 0 ) {
        log_db::debug("chlSetQuota cmlExecuteNoAnswerSql delete failure {}", status);
    }
    if (int_limit > 0) {
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = rescIdStr;
        cllBindVars[cllBindVarCount++] = _limit;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            log_sql::debug("chlSetQuota SQL 5");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_QUOTA_MAIN (user_id, resc_id, quota_limit, modify_ts) values (?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlSetQuota cmlExecuteNoAnswerSql insert failure {}", status);
            _rollback( "chlSetQuota" );
            return ERROR( status, "cmlExecuteNoAnswerSql insert failure" );
        }
    }

    /* Reset the over_quota flags based on previous usage info.  The
       usage info may take a while to set, but setting the OverQuota
       should be quick.  */
    status = setOverQuota( _ctx.comm() );
    if ( status != 0 ) {
        _rollback( "chlSetQuota" );
        return ERROR( status, "setOverQuota failed" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failure" );
    }

    return SUCCESS();
} // db_set_quota_op

irods::error db_check_quota_op(
    irods::plugin_context& _ctx,
    const char*            _user_name,
    const char*            _resc_name,
    rodsLong_t*            _user_quota,
    int*                   _quota_status ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    /*
       Check on a user's quota status, returning the most-over or
       nearest-over value.

       A single query is done which gets the four possible types of quotas
       for this user on this resource (and ordered so the first row is the
       result).  The types of quotas are: user per-resource, user global,
       group per-resource, and group global.
    */
    int status;
    int statementNum = UNINITIALIZED_STATEMENT_NUMBER;

    char mySQL[] = "select distinct QM.user_id, QM.resc_id, QM.quota_limit, QM.quota_over from R_QUOTA_MAIN QM, R_USER_MAIN UM, R_RESC_MAIN RM, R_USER_GROUP UG, R_USER_MAIN UM2 where ( (QM.user_id = UM.user_id and UM.user_name = ?) or (QM.user_id = UG.group_user_id and UM2.user_name = ? and UG.user_id = UM2.user_id) ) and ((QM.resc_id = RM.resc_id and RM.resc_name = ?) or QM.resc_id = '0') order by quota_over desc";

    *_user_quota = 0;
    if ( logSQL != 0 ) {
        log_sql::debug("chlCheckQuota SQL 1");
    }
    cllBindVars[cllBindVarCount++] = _user_name;
    cllBindVars[cllBindVarCount++] = _user_name;
    cllBindVars[cllBindVarCount++] = _resc_name;

    status = cmlGetFirstRowFromSql( mySQL, &statementNum,
                                    0, &icss );

    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlCheckQuota - CAT_SUCCESS_BUT_WITH_NO_INFO");
        *_quota_status = QUOTA_UNRESTRICTED;
        cmlFreeStatement(statementNum, &icss);
        return SUCCESS();
    }

    if ( status == CAT_NO_ROWS_FOUND ) {
        log_db::info("chlCheckQuota - CAT_NO_ROWS_FOUND");
        *_quota_status = QUOTA_UNRESTRICTED;
        cmlFreeStatement(statementNum, &icss);
        return SUCCESS();
    }

    if ( status != 0 ) {
        cmlFreeStatement(statementNum, &icss);
        return ERROR( status, "check quota failed" );
    }

    /* For now, log it */
    log_db::info(
        "checkQuota: inUser:{} inResc:{} RescId:{} Quota:{}",
        _user_name,
        _resc_name,
        icss.stmtPtr[statementNum]->resultValue[1],  /* resc_id column */
        icss.stmtPtr[statementNum]->resultValue[3]); /* quota_over column */

    *_user_quota = atoll( icss.stmtPtr[statementNum]->resultValue[3] );
    if ( atoi( icss.stmtPtr[statementNum]->resultValue[1] ) == 0 ) {
        *_quota_status = QUOTA_GLOBAL;
    }
    else {
        *_quota_status = QUOTA_RESOURCE;
    }
    cmlFreeStatement( statementNum, &icss ); /* only need the one row */

    return SUCCESS();

} // db_check_quota_op

irods::error db_del_unused_avus_op(irods::plugin_context& _ctx)
{
    if (irods::error ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (!irods::is_privileged_client(*_ctx.comm())) {
        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Insufficient privileges");
    }

    // Remove any AVUs that are currently not associated with any object.
    // This is done as a separate operation for efficiency.  See 'iadmin h rum'.
    const int remove_status = removeAVUs();
    int commit_status = 0;

    if ( remove_status == CAT_SUCCESS_BUT_WITH_NO_INFO || remove_status == 0 ) {
        commit_status = cmlExecuteNoAnswerSql( "commit", &icss );
    }
    else {
        return ERROR( remove_status, "removeAVUs failed" );
    }

    if ( commit_status == CAT_SUCCESS_BUT_WITH_NO_INFO || commit_status == 0 ) {
        return SUCCESS();
    }

    return ERROR( commit_status, "commit failed" );
} // db_del_unused_avus_op

irods::error db_ins_rule_table_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _map_priority_str,
    const char*            _rule_name,
    const char*            _rule_head,
    const char*            _rule_condition,
    const char*            _rule_action,
    const char*            _rule_recovery,
    const char*            _rule_id_str,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    int i;
    rodsLong_t seqNum = -1;

    char rule_id_str[ MAX_NAME_LEN+1];
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsRuleTable");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    /* first check if the  rule already exists */
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsRuleTable SQL 1");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = _rule_name;
    cllBindVars[i++] = _rule_head;
    cllBindVars[i++] = _rule_condition;
    cllBindVars[i++] = _rule_action;
    cllBindVars[i++] = _rule_recovery;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select rule_id from R_RULE_MAIN where  rule_base_name = ? and  rule_name = ? and rule_event = ? and rule_condition = ? and rule_body = ? and  rule_recovery = ?",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        log_db::info("chlInsRuleTable cmlGetIntegerValueFromSqlV3 find rule if any failure {}", status);
        return ERROR( status, "cmlGetIntegerValueFromSqlV3 find rule if any failure" );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            log_db::info("chlInsRuleTable cmlGetNextSeqVal failure {}", seqNum);
            _rollback( "chlInsRuleTable" );
            return ERROR( seqNum, "cmlGetNextSeqVal failure" );
        }
        snprintf( rule_id_str, MAX_NAME_LEN, "%s%lld", _rule_id_str, seqNum );

        i = 0;
        cllBindVars[i++] = rule_id_str;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = _rule_name;
        cllBindVars[i++] = _rule_head;
        cllBindVars[i++] = _rule_condition;
        cllBindVars[i++] = _rule_action;
        cllBindVars[i++] = _rule_recovery;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsRuleTable SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_MAIN(rule_id, rule_base_name, rule_name, rule_event, rule_condition, rule_body, rule_recovery, rule_owner_name, rule_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsRuleTable cmlExecuteNoAnswerSql Rule Main Insert failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql Rule Main Insert failure" );
        }
    }
    else {
        snprintf( rule_id_str, MAX_NAME_LEN, "%s%lld", _rule_id_str, seqNum );
    }
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsRuleTable SQL 3");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = _map_priority_str;
    cllBindVars[i++] = rule_id_str;
    cllBindVars[i++] = _ctx.comm()->clientUser.userName;
    cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_BASE_MAP  (map_base_name, map_priority, rule_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlInsRuleTable cmlExecuteNoAnswerSql Rule Map insert failure {}", status);

        return ERROR( status, "cmlExecuteNoAnswerSql Rule Map insert failure" );
    }

    return SUCCESS();

} // db_ins_rule_table_op

irods::error db_ins_dvm_table_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _var_name,
    const char*            _action,
    const char*            _var_2_cmap,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char dvmIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsDvmTable");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    /* first check if the DVM already exists */
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsDvmTable SQL 1");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = _var_name;
    cllBindVars[i++] = _action;
    cllBindVars[i++] = _var_2_cmap;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select dvm_id from R_RULE_DVM where  dvm_base_name = ? and  dvm_ext_var_name = ? and  dvm_condition = ? and dvm_int_map_path = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        log_db::info("chlInsDvmTable cmlGetIntegerValueFromSqlV3 find DVM if any failure {}", status);
        return ERROR( status, "find DVM if any failure" );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            log_db::info("chlInsDvmTable cmlGetNextSeqVal failure {}", seqNum);
            _rollback( "chlInsDvmTable" );
            return ERROR( seqNum, "cmlGetNextSeqVal failure" );
        }
        snprintf( dvmIdStr, MAX_NAME_LEN, "%lld", seqNum );

        i = 0;
        cllBindVars[i++] = dvmIdStr;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = _var_name;
        cllBindVars[i++] = _action;
        cllBindVars[i++] = _var_2_cmap;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsDvmTable SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_DVM(dvm_id, dvm_base_name, dvm_ext_var_name, dvm_condition, dvm_int_map_path, dvm_owner_name, dvm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsDvmTable cmlExecuteNoAnswerSql DVM Main Insert failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql DVM Main Insert failure" );
        }
    }
    else {
        snprintf( dvmIdStr, MAX_NAME_LEN, "%lld", seqNum );
    }
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsDvmTable SQL 3");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = dvmIdStr;
    cllBindVars[i++] = _ctx.comm()->clientUser.userName;
    cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_DVM_MAP  (map_dvm_base_name, dvm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlInsDvmTable cmlExecuteNoAnswerSql DVM Map insert failure {}", status);

        return ERROR( status, "DVM Map insert failure" );
    }

    return SUCCESS();

} // db_ins_dvm_table_op

irods::error db_ins_fnm_table_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _func_name,
    const char*            _func_2_cmap,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char fnmIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsFnmTable");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    /* first check if the FNM already exists */
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsFnmTable SQL 1");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = _func_name;
    cllBindVars[i++] = _func_2_cmap;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select fnm_id from R_RULE_FNM where  fnm_base_name = ? and  fnm_ext_func_name = ? and  fnm_int_func_name = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        log_db::info("chlInsFnmTable cmlGetIntegerValueFromSqlV3 find FNM if any failure {}", status);
        return ERROR( status, "find FNM if any failure" );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            log_db::info("chlInsFnmTable cmlGetNextSeqVal failure {}", seqNum);
            _rollback( "chlInsFnmTable" );
            return ERROR( seqNum, "cmlGetNextSeqVal failure" );
        }
        snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );

        i = 0;
        cllBindVars[i++] = fnmIdStr;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = _func_name;
        cllBindVars[i++] = _func_2_cmap;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsFnmTable SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_FNM(fnm_id, fnm_base_name, fnm_ext_func_name, fnm_int_func_name, fnm_owner_name, fnm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsFnmTable cmlExecuteNoAnswerSql FNM Main Insert failure {}", status);
            return ERROR( status, "FNM Map insert failure" );
        }
    }
    else {
        snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );
    }
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsFnmTable SQL 3");
    }
    i = 0;
    cllBindVars[i++] = _base_name;
    cllBindVars[i++] = fnmIdStr;
    cllBindVars[i++] = _ctx.comm()->clientUser.userName;
    cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_FNM_MAP  (map_fnm_base_name, fnm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        log_db::info("chlInsFnmTable cmlExecuteNoAnswerSql FNM Map insert failure {}", status);

        return ERROR( status, "FNM Map insert failure" );
    }

    return SUCCESS();

} // db_ins_fnm_table_op

irods::error db_ins_msrvc_table_op(
    irods::plugin_context& _ctx,
    const char*            _module_name,
    const char*            _msrvc_name,
    const char*            _msrvc_signature,
    const char*            _msrvc_version,
    const char*            _msrvc_host,
    const char*            _msrvc_location,
    const char*            _msrvc_language,
    const char*            _msrvc_type_name,
    const char*            _msrvc_status,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char msrvcIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsMsrvcTable");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    /* first check if the MSRVC already exists */
    if ( logSQL != 0 ) {
        log_sql::debug("chlInsMsrvcTable SQL 1");
    }
    i = 0;
    cllBindVars[i++] = _module_name;
    cllBindVars[i++] = _msrvc_name;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select msrvc_id from R_MICROSRVC_MAIN where  msrvc_module_name = ? and  msrvc_name = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        log_db::info("chlInsMsrvcTable cmlGetIntegerValueFromSqlV3 find MSRVC if any failure {}", status);
        return ERROR( status, "cmlGetIntegerValueFromSqlV3 find MSRVC if any failure" );
    }
    if ( seqNum < 0 ) { /* No micro-service found */
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            log_db::info("chlInsMsrvcTable cmlGetNextSeqVal failure {}", seqNum);
            _rollback( "chlInsMsrvcTable" );
            return ERROR( seqNum, "cmlGetNextSeqVal failure" );
        }
        snprintf( msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum );
        /* inserting in R_MICROSRVC_MAIN */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = _msrvc_name;
        cllBindVars[i++] = _module_name;
        cllBindVars[i++] = _msrvc_signature;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsMsrvcTable SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_MICROSRVC_MAIN(msrvc_id, msrvc_name, msrvc_module_name, msrvc_signature, msrvc_doxygen, msrvc_variations, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?,   'NONE', 'NONE',  ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_MAIN Insert failure {}", status);
            return ERROR( status, "R_MICROSRVC_MAIN Insert failure" );
        }
        /* inserting in R_MICROSRVC_VER */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = _msrvc_version;
        cllBindVars[i++] = _msrvc_host;
        cllBindVars[i++] = _msrvc_location;
        cllBindVars[i++] = _msrvc_language;
        cllBindVars[i++] = _msrvc_type_name;
        cllBindVars[i++] = _msrvc_status;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsMsrvcTable SQL 3");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_status, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure {}", status);
            return ERROR( status, "R_MICROSRVC_VER Insert failure" );
        }
    }
    else { /* micro-service already there */
        snprintf( msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum );
        /* Check if same host and location exists - if so no need to insert a new row */
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsMsrvcTable SQL 4");
        }
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = _msrvc_host;
        cllBindVars[i++] = _msrvc_location;
        cllBindVarCount = i;
        status =  cmlGetIntegerValueFromSqlV3(
                      "select msrvc_id from R_MICROSRVC_VER where  msrvc_id = ? and  msrvc_host = ? and  msrvc_location = ? ",
                      &seqNum, &icss );
        if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
            log_db::info("chlInsMsrvcTable cmlGetIntegerValueFromSqlV4 find MSRVC_HOST if any failure {}", status);
            return ERROR( status, "cmlGetIntegerValueFromSqlV4 find MSRVC_HOST if any failure" );
        }
        /* insert a new row into version table */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = _msrvc_version;
        cllBindVars[i++] = _msrvc_host;
        cllBindVars[i++] = _msrvc_location;
        cllBindVars[i++] = _msrvc_language;
        cllBindVars[i++] = _msrvc_type_name;
        cllBindVars[i++] = _ctx.comm()->clientUser.userName;
        cllBindVars[i++] = _ctx.comm()->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlInsMsrvcTable SQL 3");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            log_db::info("chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure {}", status);
            return ERROR( status, "cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure" );
        }
    }

    return SUCCESS();

} // db_ins_msrvc_table_op

irods::error db_version_rule_base_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int i, status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionRuleBase");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    i = 0;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _base_name;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionRuleBase SQL 1");
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_BASE_MAP set map_version = ?, modify_ts = ? where map_base_name = ? and map_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlVersionRuleBase cmlExecuteNoAnswerSql Rule Map version update  failure {}", status);
        return ERROR( status, "Rule Map version update failure" );

    }

    return SUCCESS();

} // db_version_rule_base_op

irods::error db_version_dvm_base_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int i, status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionDvmBase");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    i = 0;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _base_name;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionDvmBase SQL 1");
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_DVM_MAP set map_dvm_version = ?, modify_ts = ? where map_dvm_base_name = ? and map_dvm_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlVersionDvmBase cmlExecuteNoAnswerSql DVM Map version update  failure {}", status);
        return ERROR( status, "DVM Map version update  failure" );

    }

    return SUCCESS();

} // db_version_dvm_base_op

irods::error db_version_fnm_base_op(
    irods::plugin_context& _ctx,
    const char*            _base_name,
    const char*            _my_time ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int i, status;

    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionFnmBase");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    i = 0;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _my_time;
    cllBindVars[i++] = _base_name;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlVersionFnmBase SQL 1");
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_FNM_MAP set map_fnm_version = ?, modify_ts = ? where map_fnm_base_name = ? and map_fnm_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        log_db::info("chlVersionFnmBase cmlExecuteNoAnswerSql FNM Map version update  failure {}", status);
        return ERROR( status, "FNM Map version update  failure" );

    }

    return SUCCESS();

} // db_version_fnm_base_op

irods::error db_add_specific_query_op(
    irods::plugin_context& _ctx,
    const char*            _sql,
    const char*            _alias ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status, i;
    char myTime[50];
    char tsCreateTime[50];
    if ( logSQL != 0 ) {
        log_sql::debug("chlAddSpecificQuery");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( strlen( _sql ) < 5 ) {
        return ERROR( CAT_INVALID_ARGUMENT, "sql string is invalid" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    getNowStr( myTime );

    if ( _alias != NULL && strlen( _alias ) > 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddSpecificQuery SQL 1");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _alias );
            status = cmlGetStringValueFromSql(
                         "select create_ts from R_SPECIFIC_QUERY where alias=?",
                         tsCreateTime, 50, bindVars, &icss );
        }
        if ( status == 0 ) {
            addRErrorMsg( &_ctx.comm()->rError, 0, "Alias is not unique" );
            return ERROR( CAT_INVALID_ARGUMENT, "alias is not unique" );
        }
        i = 0;
        cllBindVars[i++] = _sql;
        cllBindVars[i++] = _alias;
        cllBindVars[i++] = myTime;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddSpecificQuery SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_SPECIFIC_QUERY  (sqlStr, alias, create_ts) values (?, ?, ?)",
                      &icss );
    }
    else {
        i = 0;
        cllBindVars[i++] = _sql;
        cllBindVars[i++] = myTime;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlAddSpecificQuery SQL 3");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_SPECIFIC_QUERY  (sqlStr, create_ts) values (?, ?)",
                      &icss );
    }

    if ( status != 0 ) {
        log_db::info("chlAddSpecificQuery cmlExecuteNoAnswerSql insert failure {}", status);
        return ERROR( status, "insert failure" );
    }

    status = cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_add_specific_query_op

irods::error db_del_specific_query_op(
    irods::plugin_context& _ctx,
    const char*            _sql_or_alias ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status, i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelSpecificQuery");
    }

    if ( _ctx.comm()->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
    }

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    i = 0;
    cllBindVars[i++] = _sql_or_alias;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        log_sql::debug("chlDelSpecificQuery SQL 1");
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_SPECIFIC_QUERY where sqlStr = ?",
                  &icss );

    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlDelSpecificQuery SQL 2");
        }
        i = 0;
        cllBindVars[i++] = _sql_or_alias;
        cllBindVarCount = i;
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_SPECIFIC_QUERY where alias = ?",
                      &icss );
    }

    if ( status != 0 ) {
        log_db::info("chlDelSpecificQuery cmlExecuteNoAnswerSql delete failure {}", status);
        return ERROR( status, "delete failure" );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status < 0 ) {
        return ERROR( status, "commit failed" );
    }
    else {
        return SUCCESS();
    }

} // db_del_specific_query_op

#define MINIMUM_COL_SIZE 50
irods::error db_specific_query_op(
    irods::plugin_context& _ctx,
    specificQueryInp_t*    _spec_query_inp,
    genQueryOut_t*         _result ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_spec_query_inp
       ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );

    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int i, j, k;
    int needToGetNextRow;

    char combinedSQL[MAX_SQL_SIZE];

    int status, statementNum = UNINITIALIZED_STATEMENT_NUMBER;
    int numOfCols;
    int attriTextLen;
    int totalLen;
    int maxColSize;
    int currentMaxColSize;
    char *tResult, *tResult2;
    char tsCreateTime[50];

    if ( logSQL != 0 ) {
        log_sql::debug("chlSpecificQuery");
    }

    _result->attriCnt = 0;
    _result->rowCnt = 0;
    _result->totalRowCount = 0;

    currentMaxColSize = 0;

    if ( _spec_query_inp->continueInx == 0 ) {
        if ( _spec_query_inp->sql == NULL ) {
            return ERROR( CAT_INVALID_ARGUMENT, "null sql string" );
        }
        /*
          First check that this SQL is one of the allowed forms.
        */
        if ( logSQL != 0 ) {
            log_sql::debug("chlSpecificQuery SQL 1");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _spec_query_inp->sql );
            status = cmlGetStringValueFromSql(
                         "select create_ts from R_SPECIFIC_QUERY where sqlStr=?",
                         tsCreateTime, 50, bindVars, &icss );
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            int status2;
            if ( logSQL != 0 ) {
                log_sql::debug("chlSpecificQuery SQL 2");
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( _spec_query_inp->sql );
                status2 = cmlGetStringValueFromSql(
                              "select sqlStr from R_SPECIFIC_QUERY where alias=?",
                              combinedSQL, sizeof( combinedSQL ), bindVars, &icss );
            }
            if ( status2 == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_UNKNOWN_SPECIFIC_QUERY, "unknown query" );
            }
            if ( status2 != 0 ) {
                return ERROR( status2, "failed to get first query" );
            }
        }
        else {
            if ( status != 0 ) {
                return ERROR( status, "failed to get query strings" );
            }
            snprintf( combinedSQL, sizeof( combinedSQL ), "%s", _spec_query_inp->sql );
        }

        i = 0;
        while ( _spec_query_inp->args[i] != NULL && strlen( _spec_query_inp->args[i] ) > 0 ) {
            cllBindVars[cllBindVarCount++] = _spec_query_inp->args[i++];
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlSpecificQuery SQL 3");
        }
        status = cmlGetFirstRowFromSql( combinedSQL, &statementNum,
                                        _spec_query_inp->rowOffset, &icss );
        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                log_db::info("chlSpecificQuery cmlGetFirstRowFromSql failure {}", status);
            }
            cmlFreeStatement(statementNum, &icss);
            return ERROR( status, "cmlGetFirstRowFromSql failure" );
        }

        _result->continueInx = statementNum + 1;
        needToGetNextRow = 0;
    }
    else {
        statementNum = _spec_query_inp->continueInx - 1;
        needToGetNextRow = 1;
        if ( _spec_query_inp->maxRows <= 0 ) { /* caller is closing out the query */
            status = cmlFreeStatement( statementNum, &icss );
            if ( status < 0 ) {
                return ERROR( status, "failed in free statement" );
            }
            else {
                return CODE( status );
            }
        }
    }
    for ( i = 0; i < _spec_query_inp->maxRows; i++ ) {
        if ( needToGetNextRow ) {
            status = cmlGetNextRowFromStatement( statementNum, &icss );
            if ( status == CAT_NO_ROWS_FOUND ) {
                cmlFreeStatement( statementNum, &icss );
                _result->continueInx = 0;
                if ( _result->rowCnt == 0 ) {
                    return ERROR( status, "no rows found" );
                } /* NO ROWS; in this
                                                          case a continuation call is finding no more rows */
                return SUCCESS();
            }
            if ( status < 0 ) {
                cmlFreeStatement(statementNum, &icss);
                return ERROR( status, "failed to get next row" );
            }
        }
        needToGetNextRow = 1;

        _result->rowCnt++;
        numOfCols = icss.stmtPtr[statementNum]->numOfCols;
        _result->attriCnt = numOfCols;
        _result->continueInx = statementNum + 1;

        maxColSize = 0;

        for ( k = 0; k < numOfCols; k++ ) {
            j = strlen( icss.stmtPtr[statementNum]->resultValue[k] );
            if ( maxColSize <= j ) {
                maxColSize = j;
            }
        }
        maxColSize++; /* for the null termination */
        if ( maxColSize < MINIMUM_COL_SIZE ) {
            maxColSize = MINIMUM_COL_SIZE; /* make it a reasonable size */
        }

        if ( i == 0 ) { /* first time thru, allocate and initialize */
            attriTextLen = numOfCols * maxColSize;
            totalLen = attriTextLen * _spec_query_inp->maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                tResult = ( char * ) malloc( totalLen );
                if ( tResult == NULL ) {
                    cmlFreeStatement(statementNum, &icss);
                    return ERROR( SYS_MALLOC_ERR, "malloc error" );
                }
                memset( tResult, 0, totalLen );
                _result->sqlResult[j].attriInx = 0;
                /* In Gen-query this would be set to _spec_query_inp->selectInp.inx[j]; */

                _result->sqlResult[j].len = maxColSize;
                _result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }


        /* Check to see if the current row has a max column size that
           is larger than what we've been using so far.  If so, allocate
           new result strings, copy each row value over, and free the
           old one. */
        if ( maxColSize > currentMaxColSize ) {
            maxColSize += MINIMUM_COL_SIZE; /* bump it up to try to avoid
                                               some multiple resizes */
            attriTextLen = numOfCols * maxColSize;
            totalLen = attriTextLen * _spec_query_inp->maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                char *cp1, *cp2;
                int k;
                tResult = ( char * ) malloc( totalLen );
                if ( tResult == NULL ) {
                    cmlFreeStatement(statementNum, &icss);
                    return ERROR( SYS_MALLOC_ERR, "failed to allocate result" );
                }
                memset( tResult, 0, totalLen );
                cp1 = _result->sqlResult[j].value;
                cp2 = tResult;
                for ( k = 0; k < _result->rowCnt; k++ ) {
                    strncpy( cp2, cp1, _result->sqlResult[j].len );
                    cp1 += _result->sqlResult[j].len;
                    cp2 += maxColSize;
                }
                free( _result->sqlResult[j].value );
                _result->sqlResult[j].len = maxColSize;
                _result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }

        /* Store the current row values into the appropriate spots in
           the attribute string */
        for ( j = 0; j < numOfCols; j++ ) {
            tResult2 = _result->sqlResult[j].value; /* ptr to value str */
            tResult2 += currentMaxColSize * ( _result->rowCnt - 1 );  /* skip forward
                                                                  for this row */
            strncpy( tResult2, icss.stmtPtr[statementNum]->resultValue[j],
                     currentMaxColSize ); /* copy in the value text */
        }

    }

    _result->continueInx = statementNum + 1;  /* the statementnumber but
                                            always >0 */
    return SUCCESS();

} // db_specific_query_op

irods::error db_get_distinct_data_obj_count_on_resource_op(
    irods::plugin_context& _ctx,
    const char*            _resc_name,
    long long*             _count ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming pointers
    if ( !_resc_name ||
            !_count ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "null input param" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    // =-=-=-=-=-=-=-
    // the basic query string
    char query[ MAX_NAME_LEN ];
    std::string base_query = "select count(distinct data_id) from R_DATA_MAIN where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s'";
    sprintf(
        query,
        base_query.c_str(),
        _resc_name, "%",      // root node
        "%", _resc_name, "%", // mid node
        "%", _resc_name );    // leaf node

    // =-=-=-=-=-=-=-
    // invoke the query
    int statement_num = 0;
    int status = cmlGetFirstRowFromSql(
                     query,
                     &statement_num,
                     0, &icss );
    if ( status != 0 ) {
        cmlFreeStatement(statement_num, &icss);
        return ERROR( status, "cmlGetFirstRowFromSql failed" );
    }

    ( *_count ) = atol( icss.stmtPtr[ statement_num ]->resultValue[0] );

    cmlFreeStatement(statement_num, &icss);
    return SUCCESS();

} // db_get_distinct_data_obj_count_on_resource_op

irods::error db_get_distinct_data_objs_missing_from_child_given_parent_op(
    irods::plugin_context& _ctx,
    const std::string*     _parent,
    const std::string*     _child,
    int                    _limit,
    dist_child_result_t*   _results ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming pointers
    if ( !_parent    ||
            !_child     ||
            _limit <= 0 ||
            !_results ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "null or invalid input param" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    // =-=-=-=-=-=-=-
    // the basic query string
    char query[ MAX_NAME_LEN ];
#ifdef ORA_ICAT
    std::string base_query = "select distinct data_id from R_DATA_MAIN where ( resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) and data_id not in ( select data_id from R_DATA_MAIN where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) and rownum < %d";

#elif MY_ICAT
    std::string base_query = "select distinct data_id from R_DATA_MAIN where ( resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) and data_id not in ( select data_id from R_DATA_MAIN where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) limit %d;";

#else
    std::string base_query = "select distinct data_id from R_DATA_MAIN where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' except ( select data_id from R_DATA_MAIN where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) limit %d";

#endif
    sprintf(
        query,
        base_query.c_str(),
        _parent->c_str(), "%",      // root
        "%", _parent->c_str(), "%", // mid tier
        "%", _parent->c_str(),      // leaf
        _child->c_str(), "%",       // root
        "%", _child->c_str(), "%",  // mid tier
        "%", _child->c_str(),       // leaf
        _limit );

    // =-=-=-=-=-=-=-
    // snag the first row from the resulting query
    int statement_num = 0;

    // =-=-=-=-=-=-=-
    // iterate over resulting rows
    for ( int i = 0; ; i++ ) {
        // =-=-=-=-=-=-=-
        // extract either the first or next row
        int status = 0;
        if ( 0 == i ) {
            status = cmlGetFirstRowFromSql(
                         query,
                         &statement_num,
                         0, &icss );
        }
        else {
            status = cmlGetNextRowFromStatement( statement_num, &icss );
        }

        if ( status != 0 ) {
            cmlFreeStatement(statement_num, &icss);
            return ERROR( status, "failed to get a row" );
        }

        _results->push_back( atoi( icss.stmtPtr[ statement_num ]->resultValue[0] ) );

    } // for i

    cmlFreeStatement( statement_num, &icss );

    return SUCCESS();

} // db_get_distinct_data_objs_missing_from_child_given_parent_op

irods::error db_get_repl_list_for_leaf_bundles_op(
    irods::plugin_context&      _ctx,
    rodsLong_t                  _count,
    size_t                      _child_index,
    const std::vector<leaf_bundle_t>* _bundles,
    const std::string*          _invocation_timestamp,
    dist_child_result_t*        _results ) {

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if (_count <= 0) {
        return ERROR(SYS_INVALID_INPUT_PARAM, boost::format("invalid _count [%d]") % _count);
    }
    if (_bundles->empty()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "no bundles");
    }
    if (!_invocation_timestamp) {
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, "invocation timestamp is NULL");
    }
    if (_invocation_timestamp->empty()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "invocation timestamp is empty");
    }

    // capture list of child resc ids
    std::stringstream child_array_stream;
    for( auto id : (*_bundles)[_child_index] ) {
        child_array_stream << id << ",";
    }
    std::string child_array = child_array_stream.str();
    if (child_array.empty()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "leaf array is empty");
    }
    child_array.pop_back(); // trim last ','

    std::stringstream not_child_stream;
    for( size_t idx = 0; idx < _bundles->size(); ++idx ) {
        if( idx == _child_index ) {
            continue;
        }
        for( auto id : (*_bundles)[idx] ) {
            not_child_stream << id << ",";
        }
    } // for idx

    std::string not_child_array = not_child_stream.str();
    if (not_child_array.empty()) {
        return SUCCESS();
    }
    not_child_array.pop_back(); // trim last ','

#ifdef ORA_ICAT
    const std::string query = (boost::format("select data_id from (select distinct data_id from R_DATA_MAIN where data_id in (select data_id from R_DATA_MAIN where resc_id in (%s)) and data_id not in (select data_id from R_DATA_MAIN where resc_id in (%s)) and modify_ts <= '%s') where rownum <= %d") % not_child_array % child_array % _invocation_timestamp->c_str() % _count).str();
#elif MY_ICAT
    /* MySQL (MariaDB doesn't get 'except' until v10.3)*/
    const std::string query = (boost::format(
        "select distinct data_id from R_DATA_MAIN "
        "  where resc_id in (%s) and data_id not in ( "
        "    select data_id from R_DATA_MAIN "
        "      where resc_id in (%s) "
        "  ) and modify_ts <= '%s' limit %d") % not_child_array % child_array % _invocation_timestamp->c_str() % _count).str();
#else
    /* Postgres */
    const std::string query = (boost::format(
        "select distinct data_id from R_DATA_MAIN "
        "  where resc_id in (%s) and modify_ts <= '%s' "
        "except "
        "  select data_id from R_DATA_MAIN "
        "    where resc_id in (%s) "
        "limit %d") % not_child_array % _invocation_timestamp->c_str() % child_array % _count).str();
#endif

    _results->reserve(_count);

    int statement_num = 0;
    const int status_cmlGetFirstRowFromSql = cmlGetFirstRowFromSql(query.c_str(), &statement_num, 0, &icss);
    if (status_cmlGetFirstRowFromSql == CAT_NO_ROWS_FOUND) {
        cmlFreeStatement(statement_num, &icss);
        return SUCCESS();
    }
    if (status_cmlGetFirstRowFromSql != 0) {
        cmlFreeStatement(statement_num, &icss);
        return ERROR(status_cmlGetFirstRowFromSql, boost::format("failed to get first row from query [%s]") % query);
    }
    _results->push_back(atoll(icss.stmtPtr[statement_num]->resultValue[0]));

    for (rodsLong_t i=1; i<_count; ++i) {
        const int status_cmlGetNextRowFromStatement = cmlGetNextRowFromStatement(statement_num, &icss);
        if (status_cmlGetNextRowFromStatement == CAT_NO_ROWS_FOUND) {
            break;
        }
        if (status_cmlGetNextRowFromStatement != 0) {
            cmlFreeStatement(statement_num, &icss);
            return ERROR(status_cmlGetNextRowFromStatement, boost::format("failed to get row [%d] from query [%s]") % i % query);
        }
        _results->push_back(atoll(icss.stmtPtr[statement_num]->resultValue[0]));
    }
    cmlFreeStatement(statement_num, &icss);
    return SUCCESS();

} // db_get_repl_list_for_leaf_bundles_op

irods::error db_get_hierarchy_for_resc_op(
    irods::plugin_context& _ctx,
    const std::string*     _resc_name,
    const std::string*     _zone_name,
    std::string*           _hierarchy ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming pointers
    if ( !_resc_name    ||
            !_zone_name    ||
            !_hierarchy ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "null or invalid input param" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );

    char *current_node;
    char parent[MAX_NAME_LEN];
    int status;


    irods::sql_logger logger( "chlGetHierarchyForResc", logSQL );
    logger.log();

    if ( !icss.status ) {
        return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
    }

    ( *_hierarchy ) = ( *_resc_name ); // Initialize hierarchy string with resource

    current_node = ( char * )_resc_name->c_str();
    while ( current_node ) {
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( current_node );
            bindVars.push_back( *_zone_name );
            // Ask for parent of current node
            status = cmlGetStringValueFromSql( "select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
                                               parent, MAX_NAME_LEN, bindVars, &icss );
        }
        if ( status == CAT_NO_ROWS_FOUND ) { // Resource doesn't exist
            // =-=-=-=-=-=-=-
            // quick check to see if the resource actually exists
            char type_name[ 250 ] = "";
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( current_node );
                bindVars.push_back( *_zone_name );
                status = cmlGetStringValueFromSql(
                             "select resc_type_name from R_RESC_MAIN where resc_name=? and zone_name=?",
                             type_name, 250, bindVars, &icss );
            }
            if ( status < 0 ) {
                return ERROR( CAT_UNKNOWN_RESOURCE, "resource does not exist" );
            }
            else {
                ( *_hierarchy ) = "";
                return SUCCESS();
            }
        }

        if ( status < 0 ) { // Other error
            return ERROR( status, "failed to get string" );
        }

        if ( strlen( parent ) ) {
            ( *_hierarchy ) = parent + irods::hierarchy_parser::delimiter() + ( *_hierarchy );    // Add parent to hierarchy string
            current_node = parent;
        }
        else {
            current_node = NULL;
        }
    }

    return SUCCESS();

} // db_get_hierarchy_for_resc_op

namespace
{
    // A support function for executing SQL operations.
    irods::error execute_sql(const char* _sql, const char* _function_name)
    {
        auto ec = cmlExecuteNoAnswerSql(_sql, &icss);
        if (CAT_SUCCESS_BUT_WITH_NO_INFO == ec) {
            return CODE(ec);
        }

        if (0 != ec) {
            log_db::error("SQL execution error [{}].", ec);
            _rollback(_function_name);
            return ERROR(ec, "SQL execution error.");
        }

        ec = cmlExecuteNoAnswerSql("commit", &icss);
        if (ec < 0) {
            return ERROR(ec, "Failed to commit SQL updates.");
        }

        return SUCCESS();
    } // execute_sql

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    irods::error execute_ticket_operation_as_admin(irods::plugin_context& _ctx,
                                                   const char* _op_name,
                                                   const char* _ticket_string,
                                                   const char* _arg3,
                                                   const char* _arg4,
                                                   const char* _arg5,
                                                   const KeyValPair* _cond_input)
    {
        //
        // Get the ticket's id.
        //

        rodsLong_t ticket_id = 0;
        std::vector<std::string> bindVars{_ticket_string};

        auto ec = cmlGetIntegerValueFromSql("select ticket_id from R_TICKET_MAIN where ticket_string = ?",
                                            &ticket_id, bindVars, &icss);

        if (0 != ec) {
            ec = cmlGetIntegerValueFromSql("select ticket_id from R_TICKET_MAIN where ticket_id = ?",
                                           &ticket_id, bindVars, &icss);

            if (0 != ec) {
                return ERROR(CAT_TICKET_INVALID, _ticket_string);
            }
        }

        const auto ticket_id_string = std::to_string(ticket_id);

        //
        // Handle the operation.
        //

        // Delete ticket operation.
        if (std::strcmp(_op_name, "delete") == 0) {
            cllBindVars[0] = ticket_id_string.c_str();
            cllBindVarCount = 1;

            //
            // Delete ticket from primary ticket table.
            //
            
            ec = cmlExecuteNoAnswerSql("delete from R_TICKET_MAIN where ticket_id = ?", &icss);
            if (CAT_SUCCESS_BUT_WITH_NO_INFO == ec) {
                return CODE(ec);
            }

            if (0 != ec) {
                return ERROR(ec, fmt::format("Failed to delete ticket with id [{}] as user [{}].",
                                             ticket_id_string, _ctx.comm()->clientUser.userName));
            }

            //
            // Delete all relationships stored in the secondary ticket tables.
            //

            cllBindVars[0] = ticket_id_string.c_str();
            cllBindVarCount = 1;
            ec = cmlExecuteNoAnswerSql("delete from R_TICKET_ALLOWED_HOSTS where ticket_id = ?", &icss);
            if (0 != ec && CAT_SUCCESS_BUT_WITH_NO_INFO != ec) {
                log_db::warn(
                    "Failed to delete ticket information [error_code={}, ticket={}, table=R_TICKET_ALLOWED_HOSTS]",
                    ec,
                    ticket_id_string);
            }

            cllBindVars[0] = ticket_id_string.c_str();
            cllBindVarCount = 1;
            ec = cmlExecuteNoAnswerSql("delete from R_TICKET_ALLOWED_USERS where ticket_id = ?", &icss);
            if (0 != ec && CAT_SUCCESS_BUT_WITH_NO_INFO != ec) {
                log_db::warn(
                    "Failed to delete ticket information [error_code={}, ticket={}, table=R_TICKET_ALLOWED_USERS]",
                    ec,
                    ticket_id_string);
            }

            cllBindVars[0] = ticket_id_string.c_str();
            cllBindVarCount = 1;
            ec = cmlExecuteNoAnswerSql("delete from R_TICKET_ALLOWED_GROUPS where ticket_id = ?", &icss);
            if (0 != ec && CAT_SUCCESS_BUT_WITH_NO_INFO != ec) {
                log_db::warn(
                    "Failed to delete ticket information [error_code={}, ticket={}, table=R_TICKET_ALLOWED_GROUPS]",
                    ec,
                    ticket_id_string);
            }

            ec = cmlExecuteNoAnswerSql("commit", &icss);
            if (ec < 0) {
                return ERROR(ec, "Failed to commit ticket updates.");
            }

            return SUCCESS();
        } // delete ticket operation

        // Modify ticket operation.
        if (std::strcmp(_op_name, "mod") == 0) {
            if (std::strcmp(_arg3, "uses") == 0) {
                int i{0};
                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticket_id_string.c_str();
                cllBindVarCount = i;

                return execute_sql(
                    "update R_TICKET_MAIN set uses_limit = ?, modify_ts = ? where ticket_id = ?", __func__);
            } // uses

            if (std::strcmp(_arg3, "write-file") == 0) {
                int i{};
                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticket_id_string.c_str();
                cllBindVarCount = i;

                return execute_sql(
                    "update R_TICKET_MAIN set write_file_limit = ?, modify_ts = ? where ticket_id = ?", __func__);
            } // write-file

            if (std::strcmp(_arg3, "write-bytes") == 0) {
                int i{};
                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticket_id_string.c_str();
                cllBindVarCount = i;

                return execute_sql(
                    "update R_TICKET_MAIN set write_byte_limit = ?, modify_ts = ? where ticket_id = ?", __func__);
            } // write-bytes

            if (std::strcmp(_arg3, "expire") == 0) {
                std::string ticket_expiration_string;

                // Empty strings and zero (i.e. 0) are special in that they instruct the system
                // to clear the expiration timestamp.
                //
                // Prior versions of iRODS would result in setting the expiration timestamp
                // to the value passed, but it would be better to consolidate these values into
                // one outcome. Admins should not rely on the value in the database directly.
                // Admins should use the values returned by the APIs and tools.
                //
                // For this reason, if the server receives a zero or empty string, we don't have
                // to modify "ticket_expiration_string" because it is already an empty string.
                if (std::strcmp(_arg4, "") != 0 && std::strcmp(_arg4, "0") != 0) {
                    try {
                        // Try to parse the timestamp argument as seconds since epoch.
                        const auto seconds_since_epoch = boost::lexical_cast<std::int64_t>(_arg4);
                        ticket_expiration_string = fmt::format("{:011}", seconds_since_epoch);
                    }
                    catch (const boost::bad_lexical_cast&) {
                        //
                        // If an exception was thrown, the timestamp argument was not something that
                        // represented seconds since epoch. For that reason, the client may have passed
                        // an actual timestamp, which we attempt to process here.
                        //

                        std::istringstream ss{_arg4};

                        // The facet allocated via the "new" operator is managed by the std::locale.
                        // The use of "new" here is correct and there are no memory leaks caused by this line.
                        ss.imbue(std::locale(ss.getloc(), new boost::posix_time::time_input_facet{"%Y-%m-%d.%H:%M:%S"}));

                        boost::posix_time::ptime t;
                        if (!(ss >> t)) {
                            return ERROR(SYS_INTERNAL_ERR, "Could not parse timestamp string into appropriate object.");
                        }

                        ticket_expiration_string = fmt::format("{:011}", to_time_t(t));
                    }
                }

                int i{};
                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticket_expiration_string.c_str();
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticket_id_string.c_str();
                cllBindVarCount = i;

                return execute_sql(
                    "update R_TICKET_MAIN set ticket_expiry_ts = ?, modify_ts = ? where ticket_id = ?", __func__);
            } // expire

            if (std::strcmp(_arg3, "add") == 0) {
                if (std::strcmp(_arg4, "host") == 0) {
                    // Return an error if the hostname cannot be converted to an IP address.
                    char* hostIp = convertHostToIp(_arg5);
                    if (!hostIp) {
                        return ERROR(CAT_HOSTNAME_INVALID, _arg5);
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = hostIp;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "insert into R_TICKET_ALLOWED_HOSTS (ticket_id, host) values (?, ?)", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to add host [{}] to ticket [{}], status [{}]", hostIp, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while adding host [{}], status [{}]",
                            ticket_id_string,
                            hostIp,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // host

                if (std::strcmp(_arg4, "user") == 0) {
                    // Return an error if the user does not exist.
                    char user_id_string[MAX_NAME_LEN];
                    auto ec = icatGetTicketUserId(_ctx.prop_map(), _arg5, user_id_string);
                    if (0 != ec) {
                        return ERROR(ec, "icatGetTicketUserId failed");
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = _arg5;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "insert into R_TICKET_ALLOWED_USERS (ticket_id, user_name) values (?, ?)", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to add user [{}] to ticket [{}], status [{}]", _arg5, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while adding user [{}], status [{}]",
                            ticket_id_string,
                            _arg5,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // user

                if (std::strcmp(_arg4, "group") == 0) {
                    // Return an error if the group does not exist.
                    char user_id_string[MAX_NAME_LEN];
                    auto ec = icatGetTicketGroupId(_ctx.prop_map(), _arg5, user_id_string);
                    if (0 != ec) {
                        return ERROR(ec, "icatGetTicketGroupId failed");
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = _arg5;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "insert into R_TICKET_ALLOWED_GROUPS (ticket_id, group_name) values (?, ?)", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to add group [{}] to ticket [{}], status [{}]", _arg5, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while adding group [{}], status [{}]",
                            ticket_id_string,
                            _arg5,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // group
            } // add
            else if (std::strcmp(_arg3, "remove") == 0) {
                if (std::strcmp(_arg4, "host") == 0) {
                    // Return an error if the hostname cannot be converted to an IP address.
                    char* hostIp = convertHostToIp(_arg5);
                    if (!hostIp) {
                        return ERROR(CAT_HOSTNAME_INVALID, "host name null");
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = hostIp;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "delete from R_TICKET_ALLOWED_HOSTS where ticket_id = ? and host = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to remove host [{}] from ticket [{}], status [{}]", hostIp, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while removing host [{}], status [{}]",
                            ticket_id_string,
                            hostIp,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // host

                if (std::strcmp(_arg4, "user") == 0) {
                    // Return an error if the user does not exist.
                    char user_id_string[MAX_NAME_LEN];
                    auto ec = icatGetTicketUserId(_ctx.prop_map(), _arg5, user_id_string);
                    if (0 != ec) {
                        return ERROR(ec, "icatGetTicketUserId failed");
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = _arg5;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "delete from R_TICKET_ALLOWED_USERS where ticket_id = ? and user_name = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to remove user [{}] from ticket [{}], status [{}]", _arg5, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while removing user [{}], status [{}]",
                            ticket_id_string,
                            _arg5,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // user

                if (std::strcmp(_arg4, "group") == 0) {
                    // Return an error if the group does not exist.
                    char group_id_string[MAX_NAME_LEN];
                    auto ec = icatGetTicketGroupId(_ctx.prop_map(), _arg5, group_id_string);
                    if (0 != ec) {
                        return ERROR(ec, "icatGetTicketGroupId failed");
                    }

                    cllBindVars[0] = ticket_id_string.c_str();
                    cllBindVars[1] = _arg5;
                    cllBindVarCount = 2;

                    ec = cmlExecuteNoAnswerSql(
                        "delete from R_TICKET_ALLOWED_GROUPS where ticket_id = ? and group_name = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to remove group [{}] from ticket [{}], status [{}]", _arg5, ticket_id_string, ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    int i{};
                    char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                    getNowStr(myTime);
                    cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                    cllBindVars[i++] = ticket_id_string.c_str();
                    cllBindVarCount = i;

                    ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                    if (ec != 0) {
                        auto err_msg = fmt::format(
                            "Failed to update modify_ts for ticket [{}] while removing group [{}], status [{}]",
                            ticket_id_string,
                            _arg5,
                            ec);
                        log_sql::error(err_msg);
                        _rollback(__func__);
                        return ERROR(ec, std::move(err_msg));
                    }

                    ec = cmlExecuteNoAnswerSql("commit", &icss);
                    if (ec < 0) {
                        return ERROR(ec, "Failed to commit SQL updates.");
                    }

                    return SUCCESS();
                } // group
            } // remove
        } // modify ticket operation

        return ERROR(CAT_INVALID_ARGUMENT, "invalid op name");
    } // execute_ticket_operation_as_admin
} // anonymous namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
irods::error db_mod_ticket_op(
    irods::plugin_context& _ctx,
    const char*            _op_name,
    const char*            _ticket_string,
    const char*            _arg3,
    const char*            _arg4,
    const char*            _arg5,
    const KeyValPair*      _cond_input)
{
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    rodsLong_t status, status2, status3;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t objId = 0;
    rodsLong_t userId;
    rodsLong_t ticketId;
    rodsLong_t seqNum;
    char seqNumStr[NAME_LEN];
    char objIdStr[NAME_LEN];
    char objTypeStr[NAME_LEN];
    char userIdStr[NAME_LEN];
    char user2IdStr[MAX_NAME_LEN];
    char group2IdStr[NAME_LEN];
    char ticketIdStr[NAME_LEN];
    char ticketType[NAME_LEN];

    /* session ticket */
    if ( strcmp( _op_name, "session" ) == 0 ) {
        if ( strlen( _arg3 ) > 0 ) {
            /* for 2 server hops, arg3 is the original client addr */
            if (const auto ec = chlGenQueryTicketSetup(_ticket_string, _arg3); ec < 0) {
                return ERROR(ec, "failed in chlGenQueryTicketSetup");
            }
            snprintf( mySessionTicket, sizeof( mySessionTicket ), "%s", _ticket_string );
            snprintf( mySessionClientAddr, sizeof( mySessionClientAddr ), "%s", _arg3 );
        }
        else {
            /* for direct connections, rsComm has the original client addr */
            if (const auto ec = chlGenQueryTicketSetup(_ticket_string, _ctx.comm()->clientAddr); ec < 0) {
                return ERROR(ec, "failed in chlGenQueryTicketSetup");
            }
            snprintf( mySessionTicket, sizeof( mySessionTicket ), "%s", _ticket_string );
            snprintf( mySessionClientAddr, sizeof( mySessionClientAddr ), "%s", _ctx.comm()->clientAddr );
        }

        return SUCCESS();
    }

    // Handle operations that understand the admin keyword (i.e. delete and mod).
    if (getValByKey(_cond_input, ADMIN_KW)) {
        if (!irods::is_privileged_client(*_ctx.comm())) {
            const auto msg = fmt::format("User [{}] is not a rodsadmin.", _ctx.comm()->clientUser.userName);
            return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, msg);
        }

        const auto ops = {"delete", "mod"};
        const auto pred = [&_op_name](const std::string_view _s) { return _s == _op_name; };

        if (std::any_of(std::begin(ops), std::end(ops), pred)) {
            return execute_ticket_operation_as_admin(_ctx, _op_name, _ticket_string, _arg3, _arg4, _arg5, _cond_input);
        }

        // The admin keyword is ignored for other operations.
    }

    // create
    if ( strcmp( _op_name, "create" ) == 0 ) {
        if (isInteger(const_cast<char*>(_ticket_string))) {
            log_db::info("chlModTicket create ticket, string cannot be a number [{}]", _ticket_string);
            return ERROR( CAT_TICKET_INVALID, "ticket string cannot be a number" );
        }

        if (const auto ec = splitPathByKey(_arg4, logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/'); ec < 0) {
            return ERROR(ec, fmt::format(
                         "[{}:{}] - failed in splitPathByKey [path=[{}], ec=[{}]]",
                         __func__, __LINE__, _arg4, ec));
        }

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _arg4 + 1 );
        }
        status2 = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                       _ctx.comm()->clientUser.userName,
                                       _ctx.comm()->clientUser.rodsZone,
                                       ACCESS_OWN, &icss );
        if ( status2 > 0 ) {
            snprintf( objTypeStr, sizeof( objTypeStr ), "%s", TICKET_TYPE_DATA );
            objId = status2;
        }
        else {
            status3 = cmlCheckDir( _arg4,   _ctx.comm()->clientUser.userName,
                                   _ctx.comm()->clientUser.rodsZone,
                                   ACCESS_OWN, &icss );
            if ( status3 == CAT_NO_ROWS_FOUND && status2 == CAT_NO_ROWS_FOUND ) {
                return ERROR( CAT_UNKNOWN_COLLECTION, _arg4 );
            }
            if ( status3 < 0 ) {
                return ERROR( status3, "cmlCheckDir failed" );
            }
            snprintf( objTypeStr, sizeof( objTypeStr ), "%s", TICKET_TYPE_COLL );
            objId = status3;
        }

        if ( logSQL != 0 ) {
            log_sql::debug("chlModTicket SQL 1");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _ctx.comm()->clientUser.userName );
            bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                         &userId, bindVars, &icss );
        }
        if ( status != 0 ) {
            return ERROR( CAT_INVALID_USER, "select user_id failed" );
        }

        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            log_db::info("chlModTicket failure {}d", seqNum);
            return ERROR( seqNum, "cmlGetNextSeqVal failed" );
        }
        snprintf( seqNumStr, NAME_LEN, "%lld", seqNum );
        snprintf( objIdStr, NAME_LEN, "%lld", objId );
        snprintf( userIdStr, NAME_LEN, "%lld", userId );
        if ( strncmp( _arg3, "write", 5 ) == 0 ) {
            snprintf( ticketType, sizeof( ticketType ), "%s", "write" );
        }
        else {
            snprintf( ticketType, sizeof( ticketType ), "%s", "read" );
        }

        int i{};
        char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        getNowStr( myTime );
        cllBindVars[i++] = seqNumStr;
        cllBindVars[i++] = _ticket_string;
        cllBindVars[i++] = ticketType;
        cllBindVars[i++] = userIdStr;
        cllBindVars[i++] = objIdStr;
        cllBindVars[i++] = objTypeStr;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModTicket SQL 2");
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_TICKET_MAIN (ticket_id, ticket_string, ticket_type, user_id, object_id, object_type, modify_ts, create_ts) values (?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );

        if ( status != 0 ) {
            log_db::info("chlModTicket cmlExecuteNoAnswerSql insert failure {}", status);
            return ERROR( status, "insert failure" );
        }
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status < 0 ) {
            return ERROR( status, "commit failed" );
        }
        else {
            return SUCCESS();
        }
    } // create operation

    if ( logSQL != 0 ) {
        log_sql::debug("chlModTicket SQL 3");
    }

    // Get user id of user matching (user name, zone name).
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _ctx.comm()->clientUser.userName );
        bindVars.push_back( _ctx.comm()->clientUser.rodsZone );
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                     &userId, bindVars, &icss );
    }
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO || status == CAT_NO_ROWS_FOUND ) {
        if ( !addRErrorMsg( &_ctx.comm()->rError, 0, "Invalid user" ) ) {
        }
        return ERROR( CAT_INVALID_USER, _ctx.comm()->clientUser.userName );
    }
    if ( status < 0 ) {
        return ERROR( status, "failed to select user_id" );
    }
    snprintf( userIdStr, sizeof userIdStr, "%lld", userId );

    if ( logSQL != 0 ) {
        log_sql::debug("chlModTicket SQL 4");
    }

    // Get ticket id of ticket matching (user id, ticket string).
    //
    // If the user who invoked this database operation did not create the ticket,
    // the following query will fail because their user id is not associated with
    // the ticket of interest.
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( userIdStr );
        bindVars.push_back( _ticket_string );
        status = cmlGetIntegerValueFromSql(
                     "select ticket_id from R_TICKET_MAIN where user_id=? and ticket_string=?",
                     &ticketId, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( logSQL != 0 ) {
            log_sql::debug("chlModTicket SQL 5");
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( userIdStr );
            bindVars.push_back( _ticket_string );
            status = cmlGetIntegerValueFromSql(
                         "select ticket_id from R_TICKET_MAIN where user_id=? and ticket_id=?",
                         &ticketId, bindVars, &icss );
        }
        if ( status != 0 ) {
            return ERROR( CAT_TICKET_INVALID, _ticket_string );
        }
    }
    snprintf( ticketIdStr, NAME_LEN, "%lld", ticketId );

    //
    // At this point, we have the user id and ticket id for the non-admin user.
    //

    // delete
    if ( strcmp( _op_name, "delete" ) == 0 ) {
        int i{};
        cllBindVars[i++] = ticketIdStr;
        cllBindVars[i++] = userIdStr;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            log_sql::debug("chlModTicket SQL 6");
        }
        status = cmlExecuteNoAnswerSql(
                      "delete from R_TICKET_MAIN where ticket_id = ? and user_id = ?",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            return CODE( status );
        }
        if ( status != 0 ) {
            log_db::info("chlModTicket cmlExecuteNoAnswerSql delete failure {}", status);
            return ERROR( status, "delete failure" );
        }

        i = 0;
        cllBindVars[i++] = ticketIdStr;
        cllBindVarCount = i;
        status = cmlExecuteNoAnswerSql(
                      "delete from R_TICKET_ALLOWED_HOSTS where ticket_id = ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlModTicket cmlExecuteNoAnswerSql delete 2 failure {}", status);
        }

        i = 0;
        cllBindVars[i++] = ticketIdStr;
        cllBindVarCount = i;
        status = cmlExecuteNoAnswerSql(
                      "delete from R_TICKET_ALLOWED_USERS where ticket_id = ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlModTicket cmlExecuteNoAnswerSql delete 3 failure {}", status);
        }

        i = 0;
        cllBindVars[i++] = ticketIdStr;
        cllBindVarCount = i;
        status = cmlExecuteNoAnswerSql(
                      "delete from R_TICKET_ALLOWED_GROUPS where ticket_id = ?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            log_db::info("chlModTicket cmlExecuteNoAnswerSql delete 4 failure {}", status);
        }

        status = cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status < 0 ) {
            return ERROR( status, "commit failed" );
        }

        return SUCCESS();
    } // delete operation

    // modify
    if ( strcmp( _op_name, "mod" ) == 0 ) {
        if (strcmp(_arg3, "uses") == 0) {
            int i{};
            char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
            getNowStr(myTime);
            cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = userIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVarCount = i;

            if ( logSQL != 0 ) {
                log_sql::debug("chlModTicket SQL 7");
            }

            return execute_sql(
                "update R_TICKET_MAIN set uses_limit = ?, modify_ts = ? where ticket_id = ? and user_id = ?", __func__);
        } // uses

        if ( strcmp(_arg3, "write-file") == 0 ) {
            int i{};
            char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
            getNowStr(myTime);
            cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = userIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVarCount = i;

            if ( logSQL != 0 ) {
                log_sql::debug("chlModTicket SQL 8");
            }

            return execute_sql(
                "update R_TICKET_MAIN set write_file_limit = ?, modify_ts = ? where ticket_id = ? and user_id = ?",
                __func__);
        } // write-file

        if (strcmp(_arg3, "write-bytes") == 0) {
            int i{};
            char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
            getNowStr(myTime);
            cllBindVars[i++] = _arg4; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = userIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVarCount = i;

            if ( logSQL != 0 ) {
                log_sql::debug("chlModTicket SQL 9");
            }

            return execute_sql(
                "update R_TICKET_MAIN set write_byte_limit = ?, modify_ts = ? where ticket_id = ? and user_id = ?",
                __func__);
        } // write-bytes

        if (strcmp(_arg3, "expire") == 0 ) {
            std::string ticket_expiration_string;

            // Empty strings and zero (i.e. 0) are special in that they instruct the system
            // to clear the expiration timestamp.
            //
            // Prior versions of iRODS would result in setting the expiration timestamp
            // to the value passed, but it would be better to consolidate these values into
            // one outcome. Admins should not rely on the value in the database directly.
            // Admins should use the values returned by the APIs and tools.
            //
            // For this reason, if the server receives a zero or empty string, we don't have
            // to modify "ticket_expiration_string" because it is already an empty string.
            if (std::strcmp(_arg4, "") != 0 && std::strcmp(_arg4, "0") != 0) {
                try {
                    // Try to parse the timestamp argument as seconds since epoch.
                    const auto seconds_since_epoch = boost::lexical_cast<std::int64_t>(_arg4);
                    ticket_expiration_string = fmt::format("{:011}", seconds_since_epoch);
                }
                catch (const boost::bad_lexical_cast&) {
                    //
                    // If an exception was thrown, the timestamp argument was not something that
                    // represented seconds since epoch. For that reason, the client may have passed
                    // an actual timestamp, which we attempt to process here.
                    //

                    std::istringstream ss{_arg4};

                    // The facet allocated via the "new" operator is managed by the std::locale.
                    // The use of "new" here is correct and there are no memory leaks caused by this line.
                    ss.imbue(std::locale(ss.getloc(), new boost::posix_time::time_input_facet{"%Y-%m-%d.%H:%M:%S"}));

                    boost::posix_time::ptime t;
                    if (!(ss >> t)) {
                        return ERROR(SYS_INTERNAL_ERR, "Could not parse timestamp string into appropriate object.");
                    }

                    ticket_expiration_string = fmt::format("{:011}", to_time_t(t));
                }
            }

            int i{};
            char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
            getNowStr(myTime);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = ticket_expiration_string.c_str();
            cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[i++] = userIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVarCount = i;

            if (logSQL != 0) {
                log_sql::debug("chlModTicket SQL 10");
            }

            return execute_sql(
                "update R_TICKET_MAIN set ticket_expiry_ts = ?, modify_ts = ? where ticket_id = ? and user_id = ?",
                __func__);
        } // expire

        if ( strcmp( _arg3, "add" ) == 0 ) {
            if ( strcmp( _arg4, "host" ) == 0 ) {
                char *hostIp = convertHostToIp( _arg5 );
                if (!hostIp) {
                    return ERROR( CAT_HOSTNAME_INVALID, _arg5 );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = hostIp;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 11");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql("insert into R_TICKET_ALLOWED_HOSTS (ticket_id, host) values (?, ?)", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to add host [{}] to ticket [{}], status [{}]", hostIp, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while adding host [{}], status [{}]",
                                    ticketIdStr,
                                    hostIp,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // host

            if ( strcmp( _arg4, "user" ) == 0 ) {
                status = icatGetTicketUserId( _ctx.prop_map(), _arg5, user2IdStr );
                if ( status != 0 ) {
                    return ERROR( status, "icatGetTicketUserId failed" );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = _arg5;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 12");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql(
                    "insert into R_TICKET_ALLOWED_USERS (ticket_id, user_name) values (?, ?)", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to add user [{}] to ticket [{}], status [{}]", _arg5, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while adding user [{}], status [{}]",
                                    ticketIdStr,
                                    _arg5,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // user

            if ( strcmp( _arg4, "group" ) == 0 ) {
                status = icatGetTicketGroupId( _ctx.prop_map(), _arg5, user2IdStr );
                if ( status != 0 ) {
                    return ERROR( status, "icatGetTicketGroupId failed" );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = _arg5;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 13");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql(
                    "insert into R_TICKET_ALLOWED_GROUPS (ticket_id, group_name) values (?, ?)", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to add group [{}] to ticket [{}], status [{}]", _arg5, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while adding group [{}], status [{}]",
                                    ticketIdStr,
                                    _arg5,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // group
        } // add
        else if ( strcmp( _arg3, "remove" ) == 0 ) {
            if ( strcmp( _arg4, "host" ) == 0 ) {
                char *hostIp = convertHostToIp( _arg5 );
                if (!hostIp) {
                    return ERROR( CAT_HOSTNAME_INVALID, "host name null" );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = hostIp;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 14");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql("delete from R_TICKET_ALLOWED_HOSTS where ticket_id=? and host=?", &icss);
                if (ec != 0) {
                    auto err_msg = fmt::format(
                        "Failed to remove host [{}] from ticket [{}], status [{}]", hostIp, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while removing host [{}], status [{}]",
                                    ticketIdStr,
                                    hostIp,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // host

            if ( strcmp( _arg4, "user" ) == 0 ) {
                status = icatGetTicketUserId( _ctx.prop_map(), _arg5, user2IdStr );
                if ( status != 0 ) {
                    return ERROR( status, "icatGetTicketUserId failed" );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = _arg5;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 15");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql(
                    "delete from R_TICKET_ALLOWED_USERS where ticket_id=? and user_name=?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to remove user [{}] from ticket [{}], status [{}]", _arg5, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while removing user [{}], status [{}]",
                                    ticketIdStr,
                                    _arg5,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // user

            if ( strcmp( _arg4, "group" ) == 0 ) {
                status = icatGetTicketGroupId( _ctx.prop_map(), _arg5, group2IdStr );
                if ( status != 0 ) {
                    return ERROR( status, "icatGetTicketGroupId failed" );
                }

                int i{};
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = _arg5;
                cllBindVarCount = i;

                if ( logSQL != 0 ) {
                    log_sql::debug("chlModTicket SQL 16");
                }

                int ec{};
                ec = cmlExecuteNoAnswerSql(
                    "delete from R_TICKET_ALLOWED_GROUPS where ticket_id=? and group_name=?", &icss);
                if (ec != 0) {
                    auto err_msg = fmt::format(
                        "Failed to remove group [{}] from ticket [{}], status [{}]", _arg5, ticketIdStr, ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                char myTime[TIME_LEN]{}; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                getNowStr(myTime);
                i = 0;
                cllBindVars[i++] = myTime; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVars[i++] = ticketIdStr; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                cllBindVarCount = i;

                ec = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set modify_ts = ? where ticket_id = ?", &icss);
                if (ec != 0) {
                    auto err_msg =
                        fmt::format("Failed to update modify_ts for ticket [{}] while removing group [{}], status [{}]",
                                    ticketIdStr,
                                    _arg5,
                                    ec);
                    log_sql::error(err_msg);
                    _rollback(__func__);
                    return ERROR(ec, std::move(err_msg));
                }

                ec = cmlExecuteNoAnswerSql("commit", &icss);
                if (ec < 0) {
                    return ERROR(ec, "Failed to commit SQL updates.");
                }

                return SUCCESS();
            } // group
        } // remove
    } // modify operation

    return ERROR( CAT_INVALID_ARGUMENT, "invalid op name" );
} // db_mod_ticket_op

irods::error db_get_icss_op(
    irods::plugin_context& _ctx,
    icatSessionStruct**    _icss ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming pointers
    if ( !_icss ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "null or invalid input param" );
    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    if ( logSQL != 0 ) {
        log_sql::debug("chlGetRcs");
    }
    if ( icss.status != 1 ) {
        ( *_icss ) = 0;
        return ERROR( icss.status, "catalog not connected" );
    }

    ( *_icss ) = &icss;
    return SUCCESS();

} // db_get_icss_op

// =-=-=-=-=-=-=-
// from general_query.cpp ::
int chl_gen_query_impl( genQueryInp_t, genQueryOut_t* );

irods::error db_gen_query_op(
    irods::plugin_context& _ctx,
    genQueryInp_t*         _gen_query_inp,
    genQueryOut_t*         _result ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_gen_query_inp
       ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );

    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status = chl_gen_query_impl(
                     *_gen_query_inp,
                     _result );
//         if( status < 0 ) {
//             return ERROR( status, "chl_gen_query_impl failed" );
//         } else {
//             return SUCCESS();
//         }
    return CODE( status );

} // db_gen_query_op

// =-=-=-=-=-=-=-
// from general_query.cpp ::
int chl_gen_query_access_control_setup_impl( const char*, const char*, const char*, int, int );

irods::error db_gen_query_access_control_setup_op(
    irods::plugin_context& _ctx,
    const char*            _user,
    const char*            _zone,
    const char*            _host,
    int                    _priv,
    int                    _control_flag ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    //if ( ) {
    //    return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );
    //
    //}

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status = chl_gen_query_access_control_setup_impl(
                     _user,
                     _zone,
                     _host,
                     _priv,
                     _control_flag );
    if ( status < 0 ) {
        return ERROR( status, "chl_gen_query_access_control_setup_impl failed" );
    }
    else {
        return CODE( status );
    }

} // db_gen_query_access_control_setup_op

// =-=-=-=-=-=-=-
// from general_query.cpp ::
int chl_gen_query_ticket_setup_impl( const char*, const char* );

irods::error db_gen_query_ticket_setup_op(
    irods::plugin_context& _ctx,
    const char*            _ticket,
    const char*            _client_addr ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_ticket ||
            !_client_addr ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );

    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status = chl_gen_query_ticket_setup_impl(
                     _ticket,
                     _client_addr );
    if ( status < 0 ) {
        return ERROR( status, "chl_gen_query_ticket_setup_impl failed" );
    }
    else {
        return SUCCESS();
    }

} // db_gen_query_ticket_setup_op

// =-=-=-=-=-=-=-
// from general_query.cpp ::
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
int chl_general_update_impl( generalUpdateInp_t );
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
irods::error db_general_update_op(
    irods::plugin_context& _ctx,
    generalUpdateInp_t*    _update_inp ) {
#pragma GCC diagnostic pop
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_update_inp ) {
        return ERROR( CAT_INVALID_ARGUMENT, "null parameter" );

    }

    // =-=-=-=-=-=-=-
    // get a postgres object from the context
    /*irods::postgres_object_ptr pg;
    ret = make_db_ptr( _ctx.fco(), pg );
    if ( !ret.ok() ) {
        return PASS( ret );

    }*/

    // =-=-=-=-=-=-=-
    // extract the icss property
//        icatSessionStruct icss;
//        _ctx.prop_map().get< icatSessionStruct >( ICSS_PROP, icss );
    int status = chl_general_update_impl(
                     *_update_inp );
    if ( status < 0 ) {
        return ERROR( status, "chl_general_update_impl( failed" );
    }
    else {
        return SUCCESS();
    }

} // db_general_update_op

auto db_check_permission_to_modify_data_object_op(
    irods::plugin_context& _ctx,
    const rodsLong_t       _data_id) -> irods::error
{
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    const auto ec = cmlCheckDataObjId(std::to_string(_data_id).data(),
                                      _ctx.comm()->clientUser.userName,
                                      _ctx.comm()->clientUser.rodsZone,
                                      ACCESS_MODIFY_METADATA,
                                      mySessionTicket,
                                      mySessionClientAddr,
                                      &icss);

    if (ec != 0) {
        // Although the presence of a ticket is not a guarantee of a database update, it is
        // the only case where a database update occurs as a result of calling cmlCheckDataObjId.
        // Therefore, only rollback if there is ticket information present.
        if (!std::string_view{mySessionTicket}.empty()) {
            _rollback("check_permission_to_modify_data_object");
        }

        const auto msg = fmt::format("user does not have permission to modify object with data id [{}]", _data_id);

        irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

        return ERROR(ec, msg);
    }

    // Although the presence of a ticket is not a guarantee of a database update, it is
    // the only case where a database update occurs as a result of calling cmlCheckDataObjId.
    // Therefore, return success if the ticket information is empty. Else, a commit may need
    // to occur.
    if (std::string_view{mySessionTicket}.empty()) {
        return SUCCESS();
    }

    if (const auto commit_ec = cmlExecuteNoAnswerSql("commit", &icss); 0 != commit_ec) {
        irods::log(LOG_NOTICE, fmt::format(
            "[{}:{}] - failure to commit changes "
            "[error code=[{}], data_id=[{}]]",
            __FUNCTION__, __LINE__, commit_ec, _data_id));

        return ERROR(commit_ec, "commit failure");
    }

    return SUCCESS();
} // db_check_permission_to_modify_data_object_op

auto db_update_ticket_write_byte_count_op(
    irods::plugin_context& _ctx,
    const rodsLong_t       _data_id,
    const rodsLong_t       _bytes_written) -> irods::error
{
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (std::string_view{mySessionTicket}.empty()) {
        // nothing to do
        return SUCCESS();
    }

    const auto ec = cmlTicketUpdateWriteBytes(mySessionTicket, std::to_string(_bytes_written).data(), std::to_string(_data_id).data(), &icss);

    if (ec != 0) {
        _rollback("update_ticket_write_byte_count");

        const auto msg = fmt::format(
            "failed to update write_byte_count for ticket "
            "[data_id=[{}], ticket=[{}], bytes_written=[{}]]",
            _data_id, mySessionTicket, _bytes_written);

        irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

        return ERROR(ec, msg);
    }

    if (const auto commit_ec = cmlExecuteNoAnswerSql("commit", &icss); 0 != commit_ec) {
        irods::log(LOG_NOTICE, fmt::format(
            "[{}:{}] - failure to commit changes "
            "[error code=[{}], data_id=[{}], bytes written=[{}]]",
            __FUNCTION__, __LINE__, commit_ec, _data_id, _bytes_written));

        return ERROR(commit_ec, "commit failure");
    }

    return SUCCESS();
} // db_update_ticket_write_byte_count_op

auto db_get_delay_rule_info_op(irods::plugin_context& _ctx, const char* _rule_id, std::vector<std::string>* _info)
    -> irods::error
{
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (!_rule_id || !_info) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Invalid input: rule id or info container is null");
    }

    log_db::debug("{}: _rule_id => [{}]", __func__, _rule_id);

    try {
        auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();

        nanodbc::statement stmt{db_conn};
        nanodbc::prepare(stmt,
                         "select rule_name, rei_file_path, user_name, exe_address, exe_time,"
                         " exe_frequency, priority, last_exe_time, exe_status, estimated_exe_time,"
                         " notification_addr, exe_context "
                         "from R_RULE_EXEC where rule_exec_id = ?");

        stmt.bind(0, _rule_id);

        auto row = nanodbc::execute(stmt);

        if (!row.next()) {
            const auto msg = fmt::format("Could not find a delay rule with id [{}].", _rule_id);
            log_db::error(msg);
            return ERROR(CAT_NO_ROWS_FOUND, msg);
        }

        constexpr auto number_of_columns = 12;

        for (int i = 0; i < number_of_columns; ++i) {
            _info->push_back(row.get<std::string>(i, ""));
        }

        return SUCCESS();
    }
    catch (const std::exception& e) {
        return ERROR(SYS_LIBRARY_ERROR, e.what());
    }
} // db_get_delay_rule_info_op

auto db_data_object_finalize_op(irods::plugin_context& _ctx, const char* _json_input) -> irods::error
{
    using json = nlohmann::json;

    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    constexpr std::array<const char*, 17> column_names = {"data_repl_num",
                                                          "data_version",
                                                          "data_type_name",
                                                          "data_size",
                                                          "data_path",
                                                          "data_owner_name",
                                                          "data_owner_zone",
                                                          "data_is_dirty",
                                                          "data_status",
                                                          "data_checksum",
                                                          "data_expiry_ts",
                                                          "data_map_id",
                                                          "data_mode",
                                                          "r_comment",
                                                          "create_ts",
                                                          "modify_ts",
                                                          "resc_id"};

    try {
        auto input = json::parse(_json_input);

        auto& replicas = input.at("replicas");
        if (replicas.empty()) {
            return ERROR(JSON_VALIDATION_ERROR, "JSON does not conform to the expected format");
        }

        const auto get_json_string_value = [](const json& _json, const char* _key) -> const char* {
            return _json.at(_key).get_ref<const std::string&>().c_str();
        };

        // Loops over all the replicas and executes an update on that row in R_DATA_MAIN using the replica information
        // found in the "after" entry for each replica.
        for (auto& r : replicas) {
            const auto& before = r.at("before");

            auto& after = r.at("after");

            // SET_TIME_TO_NOW_KW allows the database update to reflect the time of the modification as close as
            // possible to the actual modification of the replica.
            {
                using object_time_type = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
                using clock_type = object_time_type::clock;
                using duration_type = object_time_type::duration;

                if (auto& modify_ts = after.at("modify_ts"); modify_ts.get_ref<std::string&>() == SET_TIME_TO_NOW_KW) {
                    const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());

                    modify_ts = fmt::format("{:011}", now.time_since_epoch().count());
                }
            }

            std::string sql = "update R_DATA_MAIN set";

            for (const auto& c : column_names) {
                sql += fmt::format(" {} = ?,", c);
            }
            sql.pop_back();

            sql += " where data_id = ? and resc_id = ?";

            irods::log(LOG_DEBUG8, fmt::format("statement:[{}]", sql));
            irods::log(LOG_DEBUG9, fmt::format("before:[{}]", before.dump()));
            irods::log(LOG_DEBUG9, fmt::format("after:[{}]", after.dump()));

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_repl_num");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_version");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_type_name");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_size");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_path");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_owner_name");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_owner_zone");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_is_dirty");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_status");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_checksum");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_expiry_ts");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_map_id");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "data_mode");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "r_comment");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "create_ts");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "modify_ts");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(after, "resc_id");

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(before, "data_id");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            cllBindVars[cllBindVarCount++] = get_json_string_value(before, "resc_id");

            // Execute update for this replica. If the update fails, we need to stop immediately because the data object
            // will not reflect what the caller wanted. In that case, rollback and return an error.
            if (const auto ec = cmlExecuteNoAnswerSql(sql.c_str(), &icss);
                0 != ec && CAT_SUCCESS_BUT_WITH_NO_INFO != ec) {
                _rollback("data_object_finalize");

                std::string msg = fmt::format("cmlExecuteNoAnswerSql failed [ec=[{}]]", ec);

                irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

                return ERROR(ec, std::move(msg));
            }
        }

        // If everything executed successfully above, we commit all of the changes here, which fulfills the atomicity of
        // data_object_finalize.
        if (const auto commit_ec = cmlExecuteNoAnswerSql("commit", &icss); 0 != commit_ec) {
            const auto& data_id = replicas.front().at("before").at("data_id").get_ref<std::string&>();
            irods::log(LOG_NOTICE,
                       fmt::format("[{}:{}] - failure to commit changes [error code=[{}], data_id=[{}]]",
                                   __func__,
                                   __LINE__,
                                   commit_ec,
                                   data_id));

            return ERROR(commit_ec, "commit failure");
        }
    }
    catch (const json::exception& e) {
        std::string msg = fmt::format("[{}:{}] - JSON error occurred [{}]", __func__, __LINE__, e.what());
        irods::log(LOG_ERROR, msg);
        return ERROR(SYS_LIBRARY_ERROR, std::move(msg));
    }
    catch (const std::exception& e) {
        std::string msg = fmt::format("[{}:{}] - Exception occurred [{}]", __func__, __LINE__, e.what());
        irods::log(LOG_ERROR, msg);
        return ERROR(SYS_INTERNAL_ERR, std::move(msg));
    }
    catch (...) {
        std::string msg = fmt::format("[{}:{}] - Unknown error occurred", __func__, __LINE__);
        irods::log(LOG_ERROR, msg);
        return ERROR(SYS_UNKNOWN_ERROR, std::move(msg));
    }

    return SUCCESS();
} // db_data_object_finalize_op

auto db_check_auth_credentials_op(irods::plugin_context& _ctx,
                                  const char* _username,
                                  const char* _zone,
                                  const char* _password,
                                  int* _correct) -> irods::error
{
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (!_username || !_zone || !_password || !_correct) {
        log_db::error("{}: Received one or more null pointers.", __func__);
        return ERROR(SYS_INVALID_INPUT_PARAM, "Received one or more null pointers.");
    }

    *_correct = -1; // Indicates the correctness of the credentials is unknown.

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    std::array<char, MAX_PASSWORD_LEN + 20> decoded_password{};

    if (const auto ec = decodePw(_ctx.comm(), _password, decoded_password.data()); ec < 0) {
        log_db::error("{}: Failed to decode password with error code [{}].", __func__, ec);
        return ERROR(ec, "Password decode error.");
    }

    icatScramble(decoded_password.data());

    try {
        auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();

        nanodbc::statement stmt{db_conn};
        nanodbc::prepare(stmt,
                         "select u.user_id from R_USER_MAIN u "
                         "inner join R_USER_PASSWORD p on u.user_id = p.user_id "
                         "where u.user_name = ? and u.zone_name = ? and p.rcat_password = ?");

        stmt.bind(0, _username);
        stmt.bind(1, _zone);
        stmt.bind(2, decoded_password.data());

        if (auto row = nanodbc::execute(stmt); !row.next()) {
            log_db::warn("{}: Incorrect credentials for user [{}#{}].", __func__, _username, _zone);
            *_correct = 0;
        }
        else {
            *_correct = 1;
        }

        return SUCCESS();
    }
    catch (const irods::exception& e) {
        log_db::error("{}: {}", __func__, e.client_display_what());
        return ERROR(SYS_LIBRARY_ERROR, e.what());
    }
    catch (const std::exception& e) {
        log_db::error("{}: {}", __func__, e.what());
        return ERROR(SYS_LIBRARY_ERROR, e.what());
    }
    catch (...) {
        log_db::error("{}: An unknown error was caught.", __func__);
        return ERROR(SYS_UNKNOWN_ERROR, "An unknown error was caught.");
    }
} // db_check_auth_credentials_op

auto db_execute_genquery2_sql(irods::plugin_context& _ctx,
                              const char* _sql,
                              const std::vector<std::string>* _values,
                              char** _output) -> irods::error
{
    if (const auto ret = _ctx.valid(); !ret.ok()) {
        return PASS(ret);
    }

    if (!_sql || !_values || !_output) {
        log_db::error("{}: Received one or more null pointers.", __func__);
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, "Received one or more null pointers.");
    }

    *_output = nullptr;

    try {
        auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();

        nanodbc::statement stmt{db_conn};
        nanodbc::prepare(stmt, _sql);

        for (std::vector<std::string>::size_type i = 0; i < _values->size(); ++i) {
            stmt.bind(static_cast<short>(i), _values->at(i).c_str());
        }

        using json = nlohmann::json;

        auto json_array = json::array();
        auto json_row = json::array();

        auto row = nanodbc::execute(stmt);
        const auto n_cols = row.columns();

        while (row.next()) {
            for (std::remove_cvref_t<decltype(n_cols)> i = 0; i < n_cols; ++i) {
                json_row.push_back(row.get<std::string>(i, ""));
            }

            json_array.push_back(json_row);
            json_row.clear();
        }

        *_output = strdup(json_array.dump().c_str());

        return SUCCESS();
    }
    catch (const irods::exception& e) {
        log_db::error("{}: {}", __func__, e.client_display_what());
        return ERROR(e.code(), e.what());
    }
    catch (const std::exception& e) {
        log_db::error("{}: {}", __func__, e.what());
        return ERROR(SYS_LIBRARY_ERROR, e.what());
    }
    catch (...) {
        log_db::error("{}: An unknown error was caught.", __func__);
        return ERROR(SYS_UNKNOWN_ERROR, "An unknown error was caught.");
    }
} // db_execute_genquery2_sql

// =-=-=-=-=-=-=-
//
irods::error db_start_operation( irods::plugin_property_map& _props ) {
    return SUCCESS();

} // db_start_operation


// =-=-=-=-=-=-=-
// derive a new tcp network plugin from
// the network plugin base class for handling
// tcp communications
class postgres_database_plugin : public irods::database {
    public:
        postgres_database_plugin(const std::string& _nm, const std::string& _ctx)
            : irods::database(_nm, _ctx)
        {
            // =-=-=-=-=-=-=-
            // create a property for the icat session
            // which will manage the lifetime of the db
            // connection - use a copy ctor to init
            icatSessionStruct icss;
            std::memset(&icss, 0, sizeof(icss));
            properties_.set< icatSessionStruct >( ICSS_PROP, icss );

            set_start_operation( db_start_operation );
        } // ctor

        ~postgres_database_plugin()
        {
        }
}; // class postgres_database_plugin

// =-=-=-=-=-=-=-
// factory function to provide instance of the plugin
extern "C"
irods::database* plugin_factory(
    const std::string& _inst_name,
    const std::string& _context ) {
    // =-=-=-=-=-=-=-
    // create a postgres database plugin instance
    postgres_database_plugin* pg = new postgres_database_plugin(
        _inst_name,
        _context );

    // =-=-=-=-=-=-=-
    // fill in the operation table mapping call
    // names to function na,mes
    using namespace irods;
    using namespace std;
    pg->add_operation(
        DATABASE_OP_START,
        function<error(plugin_context&)>(
            db_start_op ) );
    pg->add_operation(
        DATABASE_OP_DEBUG,
        function<error(plugin_context&, const char*)>(
            db_debug_op ) );
    pg->add_operation(
        DATABASE_OP_OPEN,
        function<error(plugin_context&)>(
            db_open_op ) );
    pg->add_operation(
        DATABASE_OP_CLOSE,
        function<error(plugin_context&)>(
            db_close_op ) );
    pg->add_operation(
        DATABASE_OP_GET_LOCAL_ZONE,
        function<error(plugin_context&,std::string*)>(
            db_get_local_zone_op ) );
    pg->add_operation(
        DATABASE_OP_UPDATE_RESC_OBJ_COUNT,
        function<error(plugin_context&,const std::string*, int)>(
            db_update_resc_obj_count_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_DATA_OBJ_META,
        function<error(plugin_context&,dataObjInfo_t*,keyValPair_t*)>(
            db_mod_data_obj_meta_op ) );
    pg->add_operation(
        DATABASE_OP_REG_DATA_OBJ,
        function<error(plugin_context&,dataObjInfo_t*)>(
            db_reg_data_obj_op ) );
    pg->add_operation(
        DATABASE_OP_REG_REPLICA,
        function<error(plugin_context&,dataObjInfo_t*,dataObjInfo_t*,keyValPair_t*)>(
            db_reg_replica_op ) );
    pg->add_operation(
        DATABASE_OP_UNREG_REPLICA,
        function<error(plugin_context&,dataObjInfo_t*,keyValPair_t*)>(
            db_unreg_replica_op ) );
    pg->add_operation(
        DATABASE_OP_REG_RULE_EXEC,
        function<error(plugin_context&,ruleExecSubmitInp_t*)>(
            db_reg_rule_exec_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_RULE_EXEC,
        function<error(plugin_context&,const char*,keyValPair_t*)>(
            db_mod_rule_exec_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_RULE_EXEC,
        function<error(plugin_context&,const char*)>(
            db_del_rule_exec_op ) );
    pg->add_operation(
        DATABASE_OP_ADD_CHILD_RESC,
        function<error(plugin_context&,map<string,string>*)>(
            db_add_child_resc_op ) );
    pg->add_operation(
        DATABASE_OP_REG_RESC,
        function<error(plugin_context&,map<string, string>*)>(
            db_reg_resc_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_CHILD_RESC,
        function<error(plugin_context&,map<string,string>*)>(
            db_del_child_resc_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_RESC,
        function<error(plugin_context&,const char*,int)>(
            db_del_resc_op ) );
    pg->add_operation(
        DATABASE_OP_ROLLBACK,
        function<error(plugin_context&)>(
            db_rollback_op ) );
    pg->add_operation(
        DATABASE_OP_COMMIT,
        function<error(plugin_context&)>(
            db_commit_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_USER_RE,
        function<error(plugin_context&,userInfo_t*)>(
            db_del_user_re_op ) );
    pg->add_operation(
        DATABASE_OP_REG_COLL_BY_ADMIN,
        function<error(plugin_context&,collInfo_t*)>(
            db_reg_coll_by_admin_op ) );
    pg->add_operation(
        DATABASE_OP_REG_COLL,
        function<error(plugin_context&,collInfo_t*)>(
            db_reg_coll_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_COLL,
        function<error(plugin_context&,collInfo_t*)>(
            db_mod_coll_op ) );
    pg->add_operation(
        DATABASE_OP_REG_ZONE,
        function<error(plugin_context&,const char*,const char*,const char*,const char*)>(
            db_reg_zone_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_ZONE,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            db_mod_zone_op ) );
    pg->add_operation(
        DATABASE_OP_RENAME_COLL,
        function<error(plugin_context&,const char*,const char*)>(
            db_rename_coll_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_ZONE_COLL_ACL,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            db_mod_zone_coll_acl_op ) );
    pg->add_operation(
        DATABASE_OP_RENAME_LOCAL_ZONE,
        function<error(plugin_context&,const char*,const char*)>(
            db_rename_local_zone_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_ZONE,
        function<error(plugin_context&,const char*)>(
            db_del_zone_op ) );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    pg->add_operation(
        DATABASE_OP_SIMPLE_QUERY,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,int,int*,char*,int)>(
            db_simple_query_op ) );
#pragma GCC diagnostic pop
    pg->add_operation(
        DATABASE_OP_DEL_COLL_BY_ADMIN,
        function<error(plugin_context&,collInfo_t*)>(
            db_del_coll_by_admin_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_COLL,
        function<error(plugin_context&,collInfo_t*)>(
            db_del_coll_op ) );
    pg->add_operation(
        DATABASE_OP_CHECK_AUTH,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,int*,int*)>(
            db_check_auth_op ) );
    pg->add_operation(
        DATABASE_OP_MAKE_TEMP_PW,
        function<error(plugin_context&,char*, const char*)>(
            db_make_temp_pw_op ) );
    pg->add_operation(DATABASE_OP_UPDATE_PAM_PASSWORD,
                      function<error(plugin_context&, const char*, int, const char*, char**, std::size_t)>(
                          db_update_pam_password_op));
    pg->add_operation(
        DATABASE_OP_MOD_USER,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            db_mod_user_op ) );
    pg->add_operation(
        DATABASE_OP_MAKE_LIMITED_PW,
        function<error(plugin_context&,int,char*)>(
            db_make_limited_pw_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_GROUP,
        function<error(plugin_context&,const char*,const char*,const char*,const char*)>(
            db_mod_group_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_RESC,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            db_mod_resc_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_RESC_DATA_PATHS,
        function<error(plugin_context&,const char*,const char*,const char*,const char*)>(
            db_mod_resc_data_paths_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_RESC_FREESPACE,
        function<error(plugin_context&,const char*,int)>(
            db_mod_resc_freespace_op ) );
    pg->add_operation(
        DATABASE_OP_REG_USER_RE,
        function<error(plugin_context&,userInfo_t*)>(
            db_reg_user_re_op ) );
    pg->add_operation(
        DATABASE_OP_SET_AVU_METADATA,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_set_avu_metadata_op ) );
    pg->add_operation(
        DATABASE_OP_ADD_AVU_METADATA_WILD,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_add_avu_metadata_wild_op ) );
    pg->add_operation(
        DATABASE_OP_ADD_AVU_METADATA,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_add_avu_metadata_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_AVU_METADATA,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_mod_avu_metadata_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_AVU_METADATA,
        function<error(plugin_context&,int,const char*,const char*,const char*,const char*,const char*,int,const KeyValPair*)>(
            db_del_avu_metadata_op ) );
    pg->add_operation(
        DATABASE_OP_COPY_AVU_METADATA,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_copy_avu_metadata_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_ACCESS_CONTROL_RESC,
        function<error(plugin_context&,int,const char*,const char*,const char*,const char*)>(
            db_mod_access_control_resc_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_ACCESS_CONTROL,
        function<error(plugin_context&,int,const char*,const char*,const char*,const char*)>(
            db_mod_access_control_op ) );
    pg->add_operation(
        DATABASE_OP_RENAME_OBJECT,
        function<error(plugin_context&,rodsLong_t,const char*)>(
            db_rename_object_op ) );
    pg->add_operation(
        DATABASE_OP_MOVE_OBJECT,
        function<error(plugin_context&,rodsLong_t,rodsLong_t)>(
            db_move_object_op ) );
    pg->add_operation(
        DATABASE_OP_REG_TOKEN,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const char*)>(
            db_reg_token_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_TOKEN,
        function<error(plugin_context&,const char*,const char*)>(
            db_del_token_op ) );
    pg->add_operation(
        DATABASE_OP_REG_SERVER_LOAD,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*)>(
            db_reg_server_load_op ) );
    pg->add_operation(
        DATABASE_OP_PURGE_SERVER_LOAD,
        function<error(plugin_context&,const char*)>(
            db_purge_server_load_op ) );
    pg->add_operation(
        DATABASE_OP_REG_SERVER_LOAD_DIGEST,
        function<error(plugin_context&,const char*,const char*)>(
            db_reg_server_load_digest_op ) );
    pg->add_operation(
        DATABASE_OP_PURGE_SERVER_LOAD_DIGEST,
        function<error(plugin_context&,const char*)>(
            db_purge_server_load_digest_op ) );
    pg->add_operation(
        DATABASE_OP_GET_GRID_CONFIGURATION_VALUE,
        function<error(plugin_context&, const char*, const char*, char*, std::size_t)>(
            db_get_grid_configuration_value_op));
    pg->add_operation(
        DATABASE_OP_SET_GRID_CONFIGURATION_VALUE,
        function<error(plugin_context&, const char*, const char*, const char*)>(
            db_set_grid_configuration_value_op));
    pg->add_operation(
        DATABASE_OP_CALC_USAGE_AND_QUOTA,
        function<error(plugin_context&)>(
            db_calc_usage_and_quota_op ) );
    pg->add_operation(
        DATABASE_OP_SET_QUOTA,
        function<error(plugin_context&,const char*,const char*,const char*,const char*)>(
            db_set_quota_op ) );
    pg->add_operation(
        DATABASE_OP_CHECK_QUOTA,
        function<error(plugin_context&,const char*,const char*,rodsLong_t*,int*)>(
            db_check_quota_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_UNUSED_AVUS,
        function<error(plugin_context&)>(
            db_del_unused_avus_op ) );
    pg->add_operation(
        DATABASE_OP_INS_RULE_TABLE,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*)>(
            db_ins_rule_table_op ) );
    pg->add_operation(
        DATABASE_OP_INS_DVM_TABLE,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*)>(
            db_ins_dvm_table_op ) );
    pg->add_operation(
        DATABASE_OP_INS_FNM_TABLE,
        function<error(plugin_context&,const char*,const char*,const char*,const char*)>(
            db_ins_fnm_table_op ) );
    pg->add_operation(
        DATABASE_OP_INS_MSRVC_TABLE,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*)>(
            db_ins_msrvc_table_op ) );
    pg->add_operation(
        DATABASE_OP_VERSION_RULE_BASE,
        function<error(plugin_context&,const char*,const char*)>(
            db_version_rule_base_op ) );
    pg->add_operation(
        DATABASE_OP_VERSION_DVM_BASE,
        function<error(plugin_context&,const char*,const char*)>(
            db_version_dvm_base_op ) );
    pg->add_operation(
        DATABASE_OP_VERSION_FNM_BASE,
        function<error(plugin_context&,const char*,const char*)>(
            db_version_fnm_base_op ) );
    pg->add_operation(
        DATABASE_OP_ADD_SPECIFIC_QUERY,
        function<error(plugin_context&,const char*,const char*)>(
            db_add_specific_query_op ) );
    pg->add_operation(
        DATABASE_OP_DEL_SPECIFIC_QUERY,
        function<error(plugin_context&,const char*)>(
            db_del_specific_query_op ) );
    pg->add_operation(
        DATABASE_OP_SPECIFIC_QUERY,
        function<error(plugin_context&,specificQueryInp_t*,genQueryOut_t*)>(
            db_specific_query_op ) );
    pg->add_operation(
        DATABASE_OP_GET_HIERARCHY_FOR_RESC,
        function<error(plugin_context&,const string*, const string*,std::string*)>(
            db_get_hierarchy_for_resc_op ) );
    pg->add_operation(
        DATABASE_OP_MOD_TICKET,
        function<error(plugin_context&,const char*,const char*,const char*,const char*,const char*,const KeyValPair*)>(
            db_mod_ticket_op ) );
    pg->add_operation(
        DATABASE_OP_CHECK_AND_GET_OBJ_ID,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            db_check_and_get_object_id_op ) );
    pg->add_operation(
        DATABASE_OP_GET_RCS,
        function<error(plugin_context&,icatSessionStruct**)>(
            db_get_icss_op ) );
    pg->add_operation(
        DATABASE_OP_GEN_QUERY,
        function<error(plugin_context&,genQueryInp_t*,genQueryOut_t*)>(
            db_gen_query_op ) );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    pg->add_operation(
        DATABASE_OP_GENERAL_UPDATE,
        function<error(plugin_context&,generalUpdateInp_t*)>(
            db_general_update_op ) );
#pragma GCC diagnostic pop
    pg->add_operation(
        DATABASE_OP_GEN_QUERY_ACCESS_CONTROL_SETUP,
        function<error(plugin_context&,const char*,const char*,const char*,int,int)>(
            db_gen_query_access_control_setup_op ) );
    pg->add_operation(
        DATABASE_OP_GEN_QUERY_TICKET_SETUP,
        function<error(plugin_context&,const char*,const char*)>(
            db_gen_query_ticket_setup_op ) );
    pg->add_operation(
        DATABASE_OP_GET_DISTINCT_DATA_OBJ_COUNT_ON_RESOURCE,
        function<error(plugin_context&,const char*,long long*)>(
            db_get_distinct_data_obj_count_on_resource_op ) );
    pg->add_operation(
        DATABASE_OP_GET_DISTINCT_DATA_OBJS_MISSING_FROM_CHILD_GIVEN_PARENT,
        function<error(plugin_context&,const string*, const string*, int, dist_child_result_t*)>(
            db_get_distinct_data_objs_missing_from_child_given_parent_op ) );
    pg->add_operation(
        DATABASE_OP_GET_REPL_LIST_FOR_LEAF_BUNDLES,
        function<error(plugin_context&,rodsLong_t,size_t,const std::vector<leaf_bundle_t>*,const std::string*,dist_child_result_t*)>(
            db_get_repl_list_for_leaf_bundles_op));
    pg->add_operation(
        DATABASE_OP_CHECK_PERMISSION_TO_MODIFY_DATA_OBJECT,
        function<error(plugin_context&,const rodsLong_t)>(
            db_check_permission_to_modify_data_object_op));
    pg->add_operation(
        DATABASE_OP_UPDATE_TICKET_WRITE_BYTE_COUNT,
        function<error(plugin_context&,const rodsLong_t,const rodsLong_t)>(
            db_update_ticket_write_byte_count_op));
    pg->add_operation<const char*, std::vector<std::string>*>(
        DATABASE_OP_GET_DELAY_RULE_INFO,
        function<error(plugin_context&, const char*, std::vector<std::string>*)>(db_get_delay_rule_info_op));
    pg->add_operation<const char*>(
        DATABASE_OP_DATA_OBJECT_FINALIZE, function<error(plugin_context&, const char*)>(db_data_object_finalize_op));
    pg->add_operation<const char*, const char*, const char*, int*>(
        DATABASE_OP_CHECK_AUTH_CREDENTIALS,
        function<error(plugin_context&, const char*, const char*, const char*, int*)>(db_check_auth_credentials_op));
    pg->add_operation<const char*, const std::vector<std::string>*, char**>(
        DATABASE_OP_EXECUTE_GENQUERY2_SQL,
        function<error(plugin_context&, const char*, const std::vector<std::string>*, char**)>(
            db_execute_genquery2_sql));

    return pg;
} // plugin_factory
