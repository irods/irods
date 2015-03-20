// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "rcConnect.hpp"
#include "readServerConfig.hpp"
#include "icatStructs.hpp"
#include "icatHighLevelRoutines.hpp"
#include "mid_level.hpp"
#include "low_level.hpp"

// =-=-=-=-=-=-=-
// new irods includes
#include "irods_database_plugin.hpp"
#include "irods_database_constants.hpp"
#include "irods_postgres_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_catalog_properties.hpp"
#include "irods_sql_logger.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_children_parser.hpp"
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"
#include "irods_resource_manager.hpp"
#include "irods_virtual_path.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <boost/regex.hpp>

extern int get64RandomBytes( char *buf );
extern int icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 );

static char prevChalSig[200]; /* a 'signature' of the previous
                          challenge.  This is used as a sessionSignature on the ICAT server
                          side.  Also see getSessionSignatureClientside function. */

// =-=-=-=-=-=-=-
// NOTE :: yanked directly from icatHighLevelRoutines
/* In 2.3, the METADATA_CLEANUP logic (SQL) (via a
 * 'DISABLE_METADATA_CLEANUP' define) was on by default but it was
 * found to be too slow when moderate amounts of user-defined metadata
 * (AVUs) were defined.  Now, we've improved the SQL to be much faster
 * but also decided to have this off by default.  When AVUs are
 * deleted, the association between the object and the AVU is removed,
 * but the actual AVU triplet remains (and may be associated with
 * other objects).
 *
 * Admins can run the new 'iadmin rum' (remove unused metadata)
 * command if large numbers of rows accumulate (which can slow down
 * meta-data functions), which will remove any AVU triplets not
 * associated with any object.
 *
 * If you want, you can also define METADATA_CLEANUP so this will
 * run each time a user-defined metadata association is deleted.  It may,
 * however, be quite slow.
 #define METADATA_CLEANUP "EACH TIME"
*/

//   Legal values for accessLevel in  chlModAccessControl (Access Parameter).
//   Defined here since other code does not need them (except for help messages)
#define AP_READ "read"
#define AP_WRITE "write"
#define AP_OWN "own"
#define AP_NULL "null"

static rodsLong_t MAX_PASSWORDS = 40;
/* TEMP_PASSWORD_TIME is the number of seconds the temporary, one-time
   password can be used.  chlCheckAuth also checks for this column
   to be < TEMP_PASSWORD_MAX_TIME (1000) to differentiate the row
   from regular passwords and PAM passwords.
   This time, 120 seconds, should be long enough to give the iDrop and
   iDrop-lite applets enough time to download and go through their
   startup sequence.  iDrop and iDrop-lite disconnect when idle to
   reduce the number of open connections and active agents.  */
#define TEMP_PASSWORD_TIME 120
#define TEMP_PASSWORD_MAX_TIME 1000

#define PASSWORD_SCRAMBLE_PREFIX ".E_"
#define PASSWORD_KEY_ENV_VAR "irodsPKey"
#define PASSWORD_DEFAULT_KEY "a9_3fker"

#define MAX_HOST_STR 2700

// =-=-=-=-=-=-=-
// local variables externed for config file setting in
bool irods_pam_auth_no_extend = false;
size_t irods_pam_password_len = 20;
char irods_pam_password_min_time[ NAME_LEN ]     = { "121" };
char irods_pam_password_max_time[ NAME_LEN ]     = { "1209600" };
char irods_pam_password_default_time[ NAME_LEN ] = { "1209600" };

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

// =-=-=-=-=-=-=-
// virtual path management
#define PATH_SEPARATOR irods::get_virtual_path_separator().c_str()

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
        rodsLog( LOG_NOTICE,
                 "%s cmlExecuteNoAnswerSql(rollback) succeeded", functionName );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "%s cmlExecuteNoAnswerSql(rollback) failure %d",
                 functionName, status );
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
// @brief Updates the specified resources object count by the specified amount
static int
_updateRescObjCount(
    icatSessionStruct* _icss,
    const std::string& _resc_name,
    const std::string& _zone,
    int _amount ) {

    int result = 0;
    int status;
    char resc_id[MAX_NAME_LEN];
    char myTime[50];
    irods::sql_logger logger( __FUNCTION__, logSQL );
    irods::hierarchy_parser hparse;

    resc_id[0] = '\0';
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        bindVars.push_back( _zone );
        status = cmlGetStringValueFromSql(
                     ( char* )"select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     resc_id, MAX_NAME_LEN, bindVars, _icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            result = CAT_INVALID_RESOURCE;
        }
        else {
            _rollback( __FUNCTION__ );
            result = status;
        }
    }
    else {
        std::stringstream ss;
        ss << "update R_RESC_MAIN set resc_objcount=resc_objcount+";
        ss << _amount;
        ss << ", modify_ts=? where resc_id=?";
        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = resc_id;
        if ( ( status = cmlExecuteNoAnswerSql( ss.str().c_str(), &icss ) ) != 0 ) {
            std::stringstream ss;
            ss << __FUNCTION__ << " cmlExecuteNoAnswerSql update failure " << status;
            irods::log( LOG_NOTICE, ss.str() );
            _rollback( __FUNCTION__ );
            result = status;
        }
    }

    return result;
}

// =-=-=-=-=-=-=-
// @brief Traverses the specified resource hierarchy updating the object counts of each resource
static int
_updateObjCountOfResources(
    icatSessionStruct* _icss,
    const std::string  _resc_hier,
    const std::string  _zone,
    int _amount ) {
    int result = 0;
    irods::hierarchy_parser hparse;

    hparse.set_string( _resc_hier );
    for ( irods::hierarchy_parser::const_iterator it = hparse.begin();
            result == 0 && it != hparse.end(); ++it ) {
        result = _updateRescObjCount( _icss, *it, _zone, _amount );
    }
    return result;
}

/*
 * removeMetaMapAndAVU - remove AVU (user defined metadata) for an object,
 *   the metadata mapping information, if any.  Optionally, also remove
 *   any unused AVUs, if any, if some mapping information was removed.
 *
 */
void removeMetaMapAndAVU( char *dataObjNumber ) {
    char tSQL[MAX_SQL_SIZE];
    int status;
    cllBindVars[0] = dataObjNumber;
    cllBindVarCount = 1;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "removeMetaMapAndAVU SQL 1 " );
    }
    snprintf( tSQL, MAX_SQL_SIZE,
              "delete from R_OBJT_METAMAP where object_id=?" );
    status =  cmlExecuteNoAnswerSql( tSQL, &icss );

    /* Note, the status will be CAT_SUCCESS_BUT_WITH_NO_INFO (not 0) if
       there were no rows deleted from R_OBJT_METAMAP, in which case there
       is no need to do the SQL below.
    */
    if ( status == 0 ) {
#ifdef METADATA_CLEANUP
        removeAVUs();
#endif
    }
    return;
}

/*
 * removeAVUs - remove unused AVUs (user defined metadata), if any.
 */
static int removeAVUs() {
    char tSQL[MAX_SQL_SIZE];
    int status;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "removeAVUs SQL 1 " );
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
    status =  cmlExecuteNoAnswerSql( tSQL, &icss );
    rodsLog( LOG_NOTICE, "removeAVUs status=%d\n", status );

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

int
_resolveHostName(
    rsComm_t* _rsComm,
    const char* _hostAddress,
    struct hostent *& _hostEnt ) {

    _hostEnt = gethostbyname( _hostAddress );
    if ( !_hostEnt ) {
        char errMsg[155];
        snprintf( errMsg, 150,
                  "Warning, resource host address '%s' is not a valid DNS entry, gethostbyname failed.",
                  _hostAddress );
        addRErrorMsg( &_rsComm->rError, 0, errMsg );
    }
    if ( strcmp( _hostAddress, "localhost" ) == 0 ) {
        addRErrorMsg( &_rsComm->rError, 0,
                      "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client." );
    }

    return 0;
}

// =-=-=-=-=-=-=-
//
irods::error _childIsValid(
    irods::plugin_property_map& _prop_map,
    const std::string&          _new_child ) {
    // =-=-=-=-=-=-=-
    // Lookup the child resource and make sure its parent field is empty
    int result = 0;
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

int
_updateRescChildren(
    char* _resc_id,
    const std::string& _new_child_string ) {

    int result = 0;
    int status;
    char children[MAX_PATH_ALLOWED];
    char myTime[50];
    irods::sql_logger logger( "_updateRescChildren", logSQL );

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_id );
        status = cmlGetStringValueFromSql(
                     "select resc_children from R_RESC_MAIN where resc_id=?",
                     children, MAX_PATH_ALLOWED, bindVars, &icss );
    }
    if ( status != 0 ) {
        _rollback( "_updateRescChildren" );
        result = status;
    }
    else {
        std::string children_string( children );
        std::stringstream ss;
        if ( children_string.empty() ) {
            ss << _new_child_string;
        }
        else {
            ss << children_string << ";" << _new_child_string;
        }
        std::string combined_children = ss.str();

        // have to do this to avoid const issues
        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = combined_children.c_str();
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _resc_id;
        logger.log();
        if ( ( status = cmlExecuteNoAnswerSql( "update R_RESC_MAIN set resc_children=?, modify_ts=? "
                                               "where resc_id=?", &icss ) ) != 0 ) {
            std::stringstream ss;
            ss << "_updateRescChildren cmlExecuteNoAnswerSql update failure " << status;
            irods::log( LOG_NOTICE, ss.str() );
            _rollback( "_updateRescChildren" );
            result = status;
        }
    }
    return result;
} // _updateRescChildren

irods::error _updateChildParent(
    irods::plugin_property_map& _prop_map,
    const std::string&          _new_child,
    const std::string&          _parent ) {

    irods::error result = SUCCESS();
    char resc_id[MAX_NAME_LEN];
    char myTime[50];
    irods::sql_logger logger( "_updateChildParent", logSQL );
    int status;

    // Get the resource name from the child string
    irods::children_parser parser;
    std::string child;
    parser.set_string( _new_child );
    parser.first_child( child );

    std::string zone;
    irods::error ret = getLocalZone( _prop_map, &icss, zone );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // Get the resource id for the child resource
    resc_id[0] = '\0';
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( child );
        bindVars.push_back( zone );
        status = cmlGetStringValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     resc_id, MAX_NAME_LEN, bindVars, &icss );
    }
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            result = ERROR( CAT_INVALID_RESOURCE, "invalid resource" );
        }
        else {
            _rollback( "_updateChildParent" );
            result = ERROR( status, "cmlGetStringValueFromSql failed" );
        }
    }
    else {
        // Update the parent for the child resource

        // have to do this to get around const
        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = _parent.c_str();
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = resc_id;
        logger.log();
        if ( ( status = cmlExecuteNoAnswerSql( "update R_RESC_MAIN set resc_parent=?, modify_ts=? "
                                               "where resc_id=?", &icss ) ) != 0 ) {
            std::stringstream ss;
            ss << "_updateChildParent cmlExecuteNoAnswerSql update failure " << status;
            irods::log( LOG_NOTICE, ss.str() );
            _rollback( "_updateChildParent" );
            result = ERROR( status, "cmlExecuteNoAnswerSql failed" );
        }
    }

    return result;

} // _updateChildParent

irods::error _get_resc_obj_count(
    const std::string& _resc_name,
    rodsLong_t & _rtn_obj_count ) {
    irods::error result = SUCCESS();
    rodsLong_t obj_count = 0;
    int status;

    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        status = cmlGetIntegerValueFromSql(
                     "select resc_objcount from R_RESC_MAIN where resc_name=?",
                     &obj_count, bindVars, &icss );
    }
    if ( status != 0 ) {
        _rollback( __FUNCTION__ );
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to get object count for resource: \"" << _resc_name << "\"";
        result = ERROR( status, msg.str() );
    }
    else {
        _rtn_obj_count = obj_count;
    }

    return result;
}

/**
 * @brief Returns true if the specified resource has associated data objects
 */
bool
_rescHasData(
    const std::string& _resc_name ) {

    bool result = false;
    irods::sql_logger logger( "_rescHasData", logSQL );
    int status;
    static const char* func_name = "_rescHasData";
    rodsLong_t obj_count;

    logger.log();
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_name );
        status = cmlGetIntegerValueFromSql(
                     "select resc_objcount from R_RESC_MAIN where resc_name=?",
                     &obj_count, bindVars, &icss );
    }
    if ( status != 0 ) {
        _rollback( func_name );
    }
    else {
        if ( obj_count > 0 ) {
            result = true;
        }
    }
    return result;
}

/**
 * @brief Returns true if the specified child, possibly including context, has data
 */
bool
_childHasData(
    const std::string& _child ) {

    bool result = true;
    irods::children_parser parser;
    parser.set_string( _child );
    std::string child;
    parser.first_child( child );
    result = _rescHasData( child );
    return result;
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

int
_removeRescChild(
    char* _resc_id,
    const std::string& _child_string ) {

    int result = 0;
    int status;
    char children[MAX_PATH_ALLOWED];
    char myTime[50];
    irods::sql_logger logger( "_removeRescChild", logSQL );

    // Get the resources current children string
    {
        std::vector<std::string> bindVars;
        bindVars.push_back( _resc_id );
        status = cmlGetStringValueFromSql(
                     "select resc_children from R_RESC_MAIN where resc_id=?",
                     children, MAX_PATH_ALLOWED, bindVars, &icss );
    }
    if ( status != 0 ) {
        _rollback( "_updateRescChildren" );
        result = status;
    }
    else {

        // Parse the children string
        irods::children_parser parser;
        irods::error ret = parser.set_string( children );
        if ( !ret.ok() ) {
            std::stringstream ss;
            ss << "_removeChildFromResource resource has invalid children string \"" << children << "\'";
            ss << ret.result();
            irods::log( LOG_NOTICE, ss.str() );
            result = CAT_INVALID_CHILD;
        }
        else {

            // Remove the specified child from the children
            ret = parser.remove_child( _child_string );
            if ( !ret.ok() ) {
                std::stringstream ss;
                ss << "_removeChildFromResource parent has no child \"" << _child_string << "\'";
                ss << ret.result();
                irods::log( LOG_NOTICE, ss.str() );
                result = CAT_INVALID_CHILD;
            }
            else {
                // Update the database with the new children string

                // have to do this to avoid const issues
                std::string children_string;
                parser.str( children_string );
                getNowStr( myTime );
                cllBindVarCount = 0;
                cllBindVars[cllBindVarCount++] = children_string.c_str();
                cllBindVars[cllBindVarCount++] = myTime;
                cllBindVars[cllBindVarCount++] = _resc_id;
                logger.log();
                if ( ( status = cmlExecuteNoAnswerSql( "update R_RESC_MAIN set resc_children=?, modify_ts=? "
                                                       "where resc_id=?", &icss ) ) != 0 ) {
                    std::stringstream ss;
                    ss << "_removeRescChild cmlExecuteNoAnswerSql update failure " << status;
                    irods::log( LOG_NOTICE, ss.str() );
                    _rollback( "_removeRescChild" );
                    result = status;
                }
            }
        }
    }
    return result;
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
                     "select resc_children from R_RESC_MAIN where resc_id=?",
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

// =-=-=-=-=-=-=-
/// @brief function which determines if a char is allowed in a zone name
static bool allowed_zone_char( const char _c ) {
    return ( !std::isalnum( _c ) &&
             !( '_' == _c )      &&
             !( '-' == _c ) );
} // allowed_zone_char

// =-=-=-=-=-=-=-
/// @brief function for validing the name of a zone
irods::error validate_zone_name(
    std::string _zone_name ) {
    std::string::iterator itr = std::find_if( _zone_name.begin(),
                                _zone_name.end(),
                                allowed_zone_char );
    if ( itr != _zone_name.end() ) {
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
        rodsLog( LOG_SQL, "_delColl" );
    }

    if ( !icss.status ) {
        return CATALOG_NOT_CONNECTED;
    }

    status = splitPathByKey( collInfo->collName,
                             logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );

    if ( strlen( logicalParentDirName ) == 0 ) {
        snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
        snprintf( logicalEndName, sizeof( logicalEndName ), "%s", collInfo->collName + 1 );
    }

    /* Check that the parent collection exists and user has write permission,
       and get the collectionID */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "_delColl SQL 1 " );
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
        rodsLog( LOG_SQL, "_delColl SQL 2" );
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
        rodsLog( LOG_SQL, "_delColl SQL 3" );
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
        rodsLog( LOG_SQL, "_delColl SQL 4" );
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_COLL_MAIN where coll_name=? and coll_id=?",
                  &icss );
    if ( status != 0 ) { /* error, odd one as everything checked above */
        rodsLog( LOG_NOTICE,
                 "_delColl cmlExecuteNoAnswerSql delete failure %d",
                 status );
        _rollback( "_delColl" );
        return status;
    }

    /* remove any access rows */
    cllBindVars[cllBindVarCount++] = collIdNum;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "_delColl SQL 5" );
    }
    status =  cmlExecuteNoAnswerSql(
                  "delete from R_OBJT_ACCESS where object_id=?",
                  &icss );
    if ( status != 0 ) { /* error, odd one as everything checked above */
        rodsLog( LOG_NOTICE,
                 "_delColl cmlExecuteNoAnswerSql delete access failure %d",
                 status );
        _rollback( "_delColl" );
    }

    /* Remove associated AVUs, if any */
    removeMetaMapAndAVU( collIdNum );

    /* Audit */
    status = cmlAudit3( AU_DELETE_COLL,
                        collIdNum,
                        rsComm->clientUser.userName,
                        rsComm->clientUser.rodsZone,
                        collInfo->collName,
                        &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlModColl cmlAudit3 failure %d",
                 status );
        _rollback( "_delColl" );
        return status;
    }

    return status;

} // _delColl

// Modifies a given resource name in all resource hierarchies (i.e for all objects)
// gets called after a resource has been modified (iadmin modresc <oldname> name <newname>)
static int _modRescInHierarchies( const std::string& old_resc, const std::string& new_resc ) {
    char update_sql[MAX_SQL_SIZE];
    int status = 0;
    const char *sep = irods::hierarchy_parser::delimiter().c_str();
    std::string std_conf_str;        // to store value of STANDARD_CONFORMING_STRINGS

#if ORA_ICAT
    // =-=-=-=-=-=-=-
    // Oracle
    snprintf( update_sql, MAX_SQL_SIZE,
              "update r_data_main set resc_hier = regexp_replace(resc_hier, '(^|(.+%s))%s($|(%s.+))', '\\1%s\\3')",
              sep, old_resc.c_str(), sep, new_resc.c_str() );


#elif MY_ICAT
    // =-=-=-=-=-=-=-
    // MySQL
    snprintf( update_sql, MAX_SQL_SIZE,
              "update R_DATA_MAIN set resc_hier = PREG_REPLACE('/(^|(.+%s))%s($|(%s.+))/', '$1%s$3', resc_hier);",
              sep, old_resc.c_str(), sep, new_resc.c_str() );
#else
    // =-=-=-=-=-=-=-
    // PostgreSQL
    // Get STANDARD_CONFORMING_STRINGS setting to determine if backslashes in regex must be escaped
    irods::catalog_properties::getInstance().get_property<std::string>( irods::STANDARD_CONFORMING_STRINGS, std_conf_str );

    // =-=-=-=-=-=-=-
    // Regex will look in r_data_main.resc_hier
    // for occurrences of old_resc with either nothing or the separator (and some stuff) on each side
    // and replace them with new_resc, e.g:
    // regexp_replace(resc_hier, '(^|(.+;))OLD_RESC($|(;.+))', '\1NEW_RESC\3')
    // Backslashes must be escaped in older versions of Postgres

    if ( std_conf_str == "on" ) {
        // Default since Postgres 9.1
        snprintf( update_sql, MAX_SQL_SIZE,
                  "update r_data_main set resc_hier = regexp_replace(resc_hier, '(^|(.+%s))%s($|(%s.+))', '\\1%s\\3');",
                  sep, old_resc.c_str(), sep, new_resc.c_str() );
    }
    else {
        // Older versions
        snprintf( update_sql, MAX_SQL_SIZE,
                  "update r_data_main set resc_hier = regexp_replace(resc_hier, '(^|(.+%s))%s($|(%s.+))', '\\\\1%s\\\\3');",
                  sep, old_resc.c_str(), sep, new_resc.c_str() );
    }

#endif
    // =-=-=-=-=-=-=-
    // SQL update
    status = cmlExecuteNoAnswerSql( update_sql, &icss );

    // =-=-=-=-=-=-=-
    // Log error. Rollback is done in calling function
    if ( status < 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        std::stringstream ss;
        ss << "_modRescInHierarchies: cmlExecuteNoAnswerSql update failure, status = " << status;
        irods::log( LOG_NOTICE, ss.str() );
        // _rollback("_modRescInHierarchies");
    }

    return status;
}


// Modifies a given resource name in all children lists (i.e for all resources)
// gets called after a resource has been modified (iadmin modresc <oldname> name <newname>)
static int _modRescInChildren( const std::string& old_resc, const std::string& new_resc ) {
    char update_sql[MAX_SQL_SIZE];
    int status;
    char sep[] = ";";        // might later get it from children parser
    std::string std_conf_str;        // to store value of STANDARD_CONFORMING_STRINGS

#if ORA_ICAT
    snprintf( update_sql, MAX_SQL_SIZE,
              "update r_resc_main set resc_children = regexp_replace(resc_children, '(^|(.+%s))%s{}(.*)', '\\1%s{}\\3')",
              sep, old_resc.c_str(), new_resc.c_str() );


#elif MY_ICAT
    snprintf( update_sql, MAX_SQL_SIZE,
              "update R_RESC_MAIN set resc_children = PREG_REPLACE('/(^|(.+%s))%s\\{\\}(.*)/', '$1%s\\{\\}$3', resc_children);",
              sep, old_resc.c_str(), new_resc.c_str() );
#else



    // Get STANDARD_CONFORMING_STRINGS setting to determine if backslashes in regex must be escaped
    irods::catalog_properties::getInstance().get_property<std::string>( irods::STANDARD_CONFORMING_STRINGS, std_conf_str );


    // Regex will look in r_resc_main.resc_children
    // for occurrences of old_resc preceded by either nothing or the separator and followed with '{}'
    // and replace them with new_resc, e.g:
    // regexp_replace(resc_hier, '(^|(.+;))OLD_RESC{}(.+)', '\1NEW_RESC{}\3')
    // This assumes that '{}' are not valid characters in resource name
    // Backslashes must be escaped in older versions of Postgres

    if ( std_conf_str == "on" ) {
        // Default since Postgres 9.1
        snprintf( update_sql, MAX_SQL_SIZE,
                  "update r_resc_main set resc_children = regexp_replace(resc_children, '(^|(.+%s))%s{}(.*)', '\\1%s{}\\3');",
                  sep, old_resc.c_str(), new_resc.c_str() );
    }
    else {
        // Older Postgres
        snprintf( update_sql, MAX_SQL_SIZE,
                  "update r_resc_main set resc_children = regexp_replace(resc_children, '(^|(.+%s))%s{}(.*)', '\\\\1%s{}\\\\3');",
                  sep, old_resc.c_str(), new_resc.c_str() );
    }

#endif
    // =-=-=-=-=-=-=-
    // SQL update
    status = cmlExecuteNoAnswerSql( update_sql, &icss );

    // =-=-=-=-=-=-=-
    // Log error. Rollback is done in calling function
    if ( status < 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        std::stringstream ss;
        ss << "_modRescInChildren: cmlExecuteNoAnswerSql update failure, status = " << status;
        irods::log( LOG_NOTICE, ss.str() );
        // _rollback("_modRescInChildren");
    }

    return status;
}

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
    ret = auth_plugin->call <const char*, const char*, const char* > ( irods::AUTH_AGENT_AUTH_VERIFY, auth_obj, _challenge, _user_name, _response );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret;
    }

    return SUCCESS();

} // verify_auth_response

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
        rodsLog( LOG_SQL, "decodePw - SQL 1 " );
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
    irods::plugin_property_map& _prop_map,
    const char*                 type,
    const char*                 name,
    const char*                 access ) {
    int itype;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t status;
    rodsLong_t objId;
    char userName[NAME_LEN];
    char userZone[NAME_LEN];


    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "checkAndGetObjectId" );
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
        status = splitPathByKey( name,
                                 logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", name );
        }
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 1 " );
        }
        status = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                      rsComm->clientUser.userName,
                                      rsComm->clientUser.rodsZone,
                                      access, &icss );
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
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 2" );
        }
        status = cmlCheckDir( name,
                              rsComm->clientUser.userName,
                              rsComm->clientUser.rodsZone,
                              access, &icss );
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
        irods::error ret = getLocalZone( _prop_map, &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret ).code();
        }

        objId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 3" );
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

        status = parseUserName( name, userName, userZone );
        if ( userZone[0] == '\0' ) {
            std::string zone;
            irods::error ret = getLocalZone( _prop_map, &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret ).code();
            }
            snprintf( userZone, sizeof( userZone ), "%s",  zone.c_str() );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 4" );
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
findAVU( char *attribute, char *value, char *units ) {
    rodsLong_t status;
// =-=-=-=-=-=-=-

    rodsLong_t iVal;
    iVal = 0;
    if ( *units != '\0' ) {
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "findAVU SQL 1" );    // JMC - backport 4836
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
            rodsLog( LOG_SQL, "findAVU SQL 2" );
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
findOrInsertAVU( char *attribute, char *value, char *units ) {
    char nextStr[MAX_NAME_LEN];
    char myTime[50];
    rodsLong_t status, seqNum;
    rodsLong_t iVal;
    iVal = findAVU( attribute, value, units );
    if ( iVal > 0 ) {
        return iVal;
    }
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "findOrInsertAVU SQL 1" );
    }
// =-=-=-=-=-=-=-
    status = cmlGetNextSeqVal( &icss );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "findOrInsertAVU cmlGetNextSeqVal failure %d",
                 status );
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
        rodsLog( LOG_SQL, "findOrInsertAVU SQL 2" );    // JMC - backport 4836
    }
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "findOrInsertAVU insert failure %d", status );
        return status;
    }
    return seqNum;
}


/* create a path name with escaped SQL special characters (% and _) */
void
makeEscapedPath( const std::string &inPath, char *outPath, int size ) {
    snprintf( outPath, size, "%s", boost::regex_replace( inPath, boost::regex( "[%_]" ), "\\$&" ).c_str() );
    return;
}

/* Internal routine to modify inheritance */
/* inheritFlag =1 to set, 2 to remove */
int _modInheritance( int inheritFlag, int recursiveFlag, char *collIdStr, char *pathName ) {
    rodsLong_t status;
    char myTime[50];
    char newValue[10];
    char tmpPath[MAX_NAME_LEN * 2];
    char pathStart[MAX_NAME_LEN * 2 + 2];
    char auditStr[30];

    if ( recursiveFlag == 0 ) {
        strcpy( auditStr, "inheritance non-recursive " );
    }
    else {
        strcpy( auditStr, "inheritance recursive " );
    }

    if ( inheritFlag == 1 ) {
        newValue[0] = '1';
        newValue[1] = '\0';
    }
    else {
        newValue[0] = '0';
        newValue[1] = '\0';
    }
    strcat( auditStr, newValue );

    getNowStr( myTime );

    /* non-Recursive mode */
    if ( recursiveFlag == 0 ) {

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "_modInheritance SQL 1" );
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
        makeEscapedPath( pathName, tmpPath, sizeof( tmpPath ) );
        snprintf( pathStart, sizeof( pathStart ), "%s/%%", tmpPath );

        cllBindVars[cllBindVarCount++] = newValue;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = pathName;
        cllBindVars[cllBindVarCount++] = pathStart;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "_modInheritance SQL 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_name = ? or coll_name like ?",
                      &icss );
    }
    if ( status != 0 ) {
        _rollback( "_modInheritance" );
        return status;
    }

    /* Audit */
    status = cmlAudit5( AU_MOD_ACCESS_CONTROL_COLL,
                        collIdStr,
                        "0",
                        auditStr,
                        &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "_modInheritance cmlAudit5 failure %d",
                 status );
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
    int statementNum;
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
        rodsLog( LOG_SQL, "setOverQuota SQL 1" );
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
        rodsLog( LOG_SQL, "setOverQuota SQL 2" );
    }
    status =  cmlExecuteNoAnswerSql(
#if ORA_ICAT
                  "update R_QUOTA_MAIN set quota_over = (select distinct R_QUOTA_USAGE.quota_usage - R_QUOTA_MAIN.quota_limit from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id) where exists (select 1 from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id)",
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
       the this over_quota value is higher than the previous.  Do it in
       two steps to keep it simplier (there may be a better way tho).
    */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "setOverQuota SQL 3" );
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
            rodsLog( LOG_SQL, "setOverQuota SQL 4" );
        }
        status2 = cmlExecuteNoAnswerSql( "update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and resc_id='0'",
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            return status2;
        }
    }

    /* Handle group quotas on resources */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "setOverQuota SQL 5" );
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
            rodsLog( LOG_SQL, "setOverQuota SQL 6" );
        }
        status2 = cmlExecuteNoAnswerSql( "update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and R_QUOTA_MAIN.resc_id=?",
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            return status2;
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        return status;
    }

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
        rodsLog( LOG_SQL, "setOverQuota SQL 7" );
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
            rodsLog( LOG_SQL, "setOverQuota SQL 8" );
        }
        status2 = cmlExecuteNoAnswerSql( mySQL3b,
                                         &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 != 0 ) {
            return status2;
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        return status;
    }

    /* To simplify the query, if either of the above group operations
       found some over_quota, will probably want to update and insert rows
       for each user into R_QUOTA_MAIN.  For now tho, this is not done and
       perhaps shouldn't be, to keep it a little less complicated. */

    return status;
}

int
icatGetTicketUserId( irods::plugin_property_map& _prop_map, char *userName, char *userIdStr ) {

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
    status = parseUserName( userName, userName2, userZone );
    if ( userZone[0] != '\0' ) {
        rstrcpy( zoneToUse, userZone, NAME_LEN );
    }

    userId[0] = '\0';
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "icatGetTicketUserId SQL 1 " );
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
icatGetTicketGroupId( irods::plugin_property_map& _prop_map, char *groupName, char *groupIdStr ) {
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
    status = parseUserName( groupName, groupName2, groupZone );
    if ( groupZone[0] != '\0' ) {
        rstrcpy( zoneToUse, groupZone, NAME_LEN );
    }

    groupId[0] = '\0';
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "icatGetTicketGroupId SQL 1 " );
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

char *
convertHostToIp( char *inputName ) {
    struct hostent *myHostent;
    static char ipAddr[50];
    myHostent = gethostbyname( inputName );
    if ( myHostent == NULL || myHostent->h_addrtype != AF_INET ) {
        printf( "unknown hostname: %s\n", inputName );
        return NULL;
    }
    snprintf( ipAddr, sizeof( ipAddr ), "%s",
              ( char * )inet_ntoa( *( struct in_addr* )( myHostent->h_addr_list[0] ) ) );
    return ipAddr;
}

// XXXX HELPER FUNCTIONS ABOVE

extern "C" {

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

        std::string prop; // server property

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
            rodsLog( LOG_SQL, "chlOpen" );
        }

        // =-=-=-=-=-=-=-
        // cache db creds
        irods::server_properties& props = irods::server_properties::getInstance();
        ret = props.get_property<std::string>( DB_USERNAME_KW, prop );
        if ( !ret.ok() ) {
            ret = props.get_property<std::string>( irods::CFG_DB_USERNAME_KW, prop );

        }
        snprintf( icss.databaseUsername, DB_USERNAME_LEN, "%s", prop.c_str() );

        ret = props.get_property<std::string>( DB_PASSWORD_KW, prop );
        if ( !ret.ok() ) {
            ret = props.get_property<std::string>( irods::CFG_DB_PASSWORD_KW, prop );
        }
        snprintf( icss.databasePassword, DB_PASSWORD_LEN, "%s", prop.c_str() );

        ret = props.get_property<std::string>( CATALOG_DATABASE_TYPE_KW, prop );
        if ( !ret.ok() ) {
            ret = props.get_property<std::string>( irods::CFG_CATALOG_DATABASE_TYPE_KW, prop );
        }
        snprintf( icss.database_plugin_type, NAME_LEN, "%s", prop.c_str() );

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
        irods::catalog_properties::getInstance().capture( &icss );

        // =-=-=-=-=-=-=-
        // set pam properties
        bool no_ex = false;
        ret = irods::server_properties::getInstance().get_property<bool>( PAM_NO_EXTEND_KW, no_ex );
        if ( ret.ok() ) {
            irods_pam_auth_no_extend = no_ex;
        }

        size_t pw_len = 0;
        ret = irods::server_properties::getInstance().get_property<size_t>( PAM_PW_LEN_KW, irods_pam_password_len );
        if ( ret.ok() ) {
            irods_pam_password_len = pw_len;
        }

        ret = irods::server_properties::getInstance().get_property<std::string>( PAM_PW_MIN_TIME_KW, prop );
        if ( ret.ok() ) {
            snprintf( irods_pam_password_min_time, NAME_LEN, "%s", prop.c_str() );
        }

        ret = irods::server_properties::getInstance().get_property<std::string>( PAM_PW_MAX_TIME_KW, prop );
        if ( ret.ok() ) {
            snprintf( irods_pam_password_max_time, NAME_LEN, "%s", prop.c_str() );
        }

        if ( irods_pam_auth_no_extend ) {
            snprintf( irods_pam_password_default_time,
                      sizeof( irods_pam_password_default_time ),
                      "%s", "28800" );
        }

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
        rsComm_t*              _comm,
        char*                  _type,
        char*                  _name,
        char*                  _access ) {
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
                                _comm,
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
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_resc ) {
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
        // get the local zone name
        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        int status = _updateRescObjCount(
                         &icss,
                         ( *_resc ),
                         zone,
                         _delta );
        if ( 0 != status ) {
            std::stringstream msg;
            msg << "Failed to update the object count for resource \""
                << ( *_resc )
                << "\"";
            return ERROR(
                       status,
                       msg.str() );
        }

        return SUCCESS();

    } // db_update_resc_obj_count_op

    // =-=-=-=-=-=-=-
    // update the data obj count of a resource
    irods::error db_mod_data_obj_meta_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
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
        if ( !_comm          ||
                !_data_obj_info ||
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


        int i = 0, j = 0, status = 0, upCols = 0;
        rodsLong_t iVal = 0; // JMC cppcheck - uninit var
        int status2 = 0;

        int mode = 0;

        char logicalFileName[MAX_NAME_LEN];
        char logicalDirName[MAX_NAME_LEN];
        char *theVal = 0;
        char replNum1[MAX_NAME_LEN];

        const char* whereColsAndConds[10];
        const char* whereValues[10];
        char idVal[MAX_NAME_LEN];
        int numConditions = 0;
        char oldCopy[NAME_LEN];
        char newCopy[NAME_LEN];
        int adminMode = 0;

        std::vector<const char *> updateCols;
        std::vector<const char *> updateVals;

        /* regParamNames has the argument names (in _reg_param) that this
           routine understands and colNames has the corresponding column
           names; one for one. */
        int dataTypeIndex = 1; /* matches table below for quick check */
        // Using the keyword defines so there is one point of truth - hcj
        char *regParamNames[] = {
            REPL_NUM_KW,        DATA_TYPE_KW,       DATA_SIZE_KW,
            RESC_NAME_KW,       FILE_PATH_KW,       DATA_OWNER_KW,
            DATA_OWNER_ZONE_KW, REPL_STATUS_KW,     CHKSUM_KW,
            DATA_EXPIRY_KW,     DATA_COMMENTS_KW,   DATA_CREATE_KW,
            DATA_MODIFY_KW,     DATA_MODE_KW,		RESC_HIER_STR_KW,
            "END"
        };

        /* If you update colNames, be sure to update DATA_EXPIRY_TS_IX if
         * you add items before "data_expiry_ts" and */
        char *colNames[] = {
            "data_repl_num",   "data_type_name",	"data_size",
            "resc_name",       "data_path",			"data_owner_name",
            "data_owner_zone", "data_is_dirty",		"data_checksum",
            "data_expiry_ts",  "r_comment",			"create_ts",
            "modify_ts",       "data_mode",			"resc_hier"
        };
        int DATA_EXPIRY_TS_IX = 9; /* must match index in above colNames table */
        int MODIFY_TS_IX = 12;   /* must match index in above colNames table */

        int DATA_SIZE_IX = 2;    /* must match index in above colNames table */
        int doingDataSize = 0;
        char dataSizeString[NAME_LEN] = "";
        char objIdString[MAX_NAME_LEN];
        char *neededAccess = 0;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModDataObjMeta" );
        }

        adminMode = 0;
        theVal = getValByKey( _reg_param, ADMIN_KW );
        if ( theVal != NULL ) {
            adminMode = 1;
        }

        bool update_resc_hier = false;
        /* Set up the updateCols and updateVals arrays */
        for ( i = 0, j = 0; strcmp( regParamNames[i], "END" ); i++ ) {
            theVal = getValByKey( _reg_param, regParamNames[i] );
            if ( theVal != NULL ) {
                updateCols.push_back( colNames[i] );
                updateVals.push_back( theVal );

                if ( std::string( "resc_hier" ) == colNames[i] ) {
                    update_resc_hier = true;
                }

                if ( i == DATA_EXPIRY_TS_IX ) {
                    /* if data_expiry, make sure it's
                                                   in the standard time-stamp
                                                   format: "%011d" */
                    if ( strcmp( colNames[i], "data_expiry_ts" ) == 0 ) { /* double check*/
                        if ( strlen( theVal ) < 11 ) {
                            static char theVal2[20];
                            time_t myTimeValue;
                            myTimeValue = atoll( theVal );
                            snprintf( theVal2, sizeof theVal2, "%011d", ( int )myTimeValue );
                            updateVals[j] = theVal2;
                        }
                    }
                }

                if ( i == MODIFY_TS_IX ) {
                    /* if modify_ts, also make sure it's
                                                    in the standard time-stamp
                                                    format: "%011d" */
                    if ( strcmp( colNames[i], "modify_ts" ) == 0 ) { /* double check*/
                        if ( strlen( theVal ) < 11 ) {
                            static char theVal3[20];
                            time_t myTimeValue;
                            myTimeValue = atoll( theVal );
                            snprintf( theVal3, sizeof theVal3, "%011d", ( int )myTimeValue );
                            updateVals[j] = theVal3;
                        }
                    }
                }
                if ( i == DATA_SIZE_IX ) {
                    doingDataSize = 1; /* flag to check size */
                    snprintf( dataSizeString, sizeof( dataSizeString ), "%s", theVal );
                }

                j++;

                /* If the datatype is being updated, check that it is valid */
                if ( i == dataTypeIndex ) {
                    status = cmlCheckNameToken( "data_type",
                                                theVal, &icss );
                    if ( status != 0 ) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Invalid data type specified.";
                        addRErrorMsg( &_comm->rError, 0, msg.str().c_str() );
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

            status = splitPathByKey( _data_obj_info->objPath,
                                     logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/' );

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModDataObjMeta SQL 1 " );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
                _rollback( "chlModDataObjMeta" );
                return ERROR(
                           CAT_UNKNOWN_COLLECTION,
                           "failed with unknown collection" );
            }
            snprintf( objIdString, MAX_NAME_LEN, "%lld", iVal );

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModDataObjMeta SQL 2" );
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
                addRErrorMsg( &_comm->rError, 0, msg.str().c_str() );
                _rollback( "chlModDataObjMeta" );
                return ERROR(
                           CAT_UNKNOWN_FILE,
                           "failed with unknown file" );
            }

            _data_obj_info->dataId = iVal;  /* return it for possible use next time, */
            /* and for use below */
        }

        snprintf( objIdString, MAX_NAME_LEN, "%lld", _data_obj_info->dataId );

        if ( adminMode ) {
            if ( _comm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
                return ERROR(
                           CAT_INSUFFICIENT_PRIVILEGE_LEVEL,
                           "failed with insufficient privilege" );
            }
        }
        else {
            if ( doingDataSize == 1 && strlen( mySessionTicket ) > 0 ) {
                status = cmlTicketUpdateWriteBytes( mySessionTicket,
                                                    dataSizeString,
                                                    objIdString, &icss );
                if ( status != 0 ) {
                    return ERROR(
                               status,
                               "cmlTicketUpdateWriteBytes failed" );
                }
            }

            status = cmlCheckDataObjId(
                         objIdString,
                         _comm->clientUser.userName,
                         _comm->clientUser.rodsZone,
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
                                            _comm->clientUser.userName,
                                            _comm->clientUser.rodsZone,
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
        if ( getValByKey( _reg_param, ALL_KW ) == NULL ) {
            // use resc_hier instead of replNum as it is
            // always set, unless resc_hier is to be
            // updated.  replNum is sometimes 0 in various
            // error cases
            if ( update_resc_hier || strlen( _data_obj_info->rescHier ) <= 0 ) {
                j = numConditions;
                whereColsAndConds[j] = "data_repl_num=";
                snprintf( replNum1, MAX_NAME_LEN, "%d", _data_obj_info->replNum );
                whereValues[j] = replNum1;
                numConditions++;

            }
            else {
                j = numConditions;
                whereColsAndConds[j] = "resc_hier=";
                whereValues[j] = _data_obj_info->rescHier;
                numConditions++;
            }
        }

        mode = 0;
        if ( getValByKey( _reg_param, ALL_REPL_STATUS_KW ) ) {
            mode = 1;
            /* mark this one as NEWLY_CREATED_COPY and others as OLD_COPY */
        }

        std::string zone;
        ret = getLocalZone(
                  _ctx.prop_map(),
                  &icss,
                  zone );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "chlModObjMeta - failed in getLocalZone with status [%d]", status );
            return PASS( ret );
        }

        // If we are moving the data object from one resource to another resource, update the object counts for those resources
        // appropriately - hcj
        char* new_resc_hier = getValByKey( _reg_param, RESC_HIER_STR_KW );
        if ( new_resc_hier != NULL ) {
            std::stringstream id_stream;
            id_stream << _data_obj_info->dataId;
            std::stringstream repl_stream;
            repl_stream << _data_obj_info->replNum;
            char resc_hier[MAX_NAME_LEN];
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( id_stream.str() );
                bindVars.push_back( repl_stream.str() );
                status = cmlGetStringValueFromSql(
                             "select resc_hier from R_DATA_MAIN where data_id=? and data_repl_num=?",
                             resc_hier, MAX_NAME_LEN, bindVars, &icss );
            }
            if ( status != 0 ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the resc hierarchy from object with id: ";
                msg << id_stream.str();
                msg << " and replNum: ";
                msg << repl_stream.str();
                irods::log( LOG_NOTICE, msg.str() );
                return ERROR(
                           status,
                           msg.str() );
            }
            // TODO - Address this in terms of resource hierarchies
            else if ( ( status = _updateObjCountOfResources( &icss, resc_hier, zone.c_str(), -1 ) ) != 0 ) {
                return ERROR(
                           status,
                           "_updateObjCountOfResources failed" );
            }
            else if ( ( status = _updateObjCountOfResources( &icss, new_resc_hier, zone.c_str(), +1 ) ) != 0 ) {
                return ERROR(
                           status,
                           "_updateObjCountOfResources failed" );
            }
        }

        if ( mode == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModDataObjMeta SQL 4" );
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
        }
        else {
            /* mark this one as NEWLY_CREATED_COPY and others as OLD_COPY */
            j = upCols;
            updateCols[j] = "data_is_dirty";
            snprintf( newCopy, NAME_LEN, "%d", NEWLY_CREATED_COPY );
            updateVals[j] = newCopy;
            upCols++;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModDataObjMeta SQL 5" );
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

                updateCols[0] = "data_is_dirty";
                snprintf( oldCopy, NAME_LEN, "%d", OLD_COPY );
                updateVals[0] = oldCopy;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModDataObjMeta SQL 6" );
                }
                status2 = cmlModifySingleTable( "R_DATA_MAIN", &( updateCols[0] ), &( updateVals[0] ),
                                                whereColsAndConds, whereValues, 1,
                                                numConditions, &icss );

                if ( status2 != 0 && status2 != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    /* Ignore NO_INFO errors but not others */
                    rodsLog( LOG_NOTICE,
                             "chlModDataObjMeta cmlModifySingleTable failure for other replicas %d",
                             status2 );
                    _rollback( "chlModDataObjMeta" );
                    return ERROR(
                               status2,
                               "cmlModifySingleTable failure for other replicas" );
                }

            }
        }
        if ( status != 0 ) {
            _rollback( "chlModDataObjMeta" );
            rodsLog( LOG_NOTICE,
                     "chlModDataObjMeta cmlModifySingleTable failure %d",
                     status );
            return ERROR(
                       status,
                       "cmlModifySingleTable failure" );
        }

        if ( !( _data_obj_info->flags & NO_COMMIT_FLAG ) ) {
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModDataObjMeta cmlExecuteNoAnswerSql commit failure %d",
                         status );
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
        rsComm_t*              _comm,
        dataObjInfo_t*         _data_obj_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm       ||
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
        char data_expiry_ts[] = { "00000000000" };
        int status;
        int inheritFlag;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegDataObj" );
        }
        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegDataObj SQL 1 " );
        }
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlRegDataObj cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlRegDataObj" );
            return ERROR( seqNum, "chlRegDataObj cmlGetNextSeqVal failure" );
        }
        snprintf( dataIdNum, MAX_NAME_LEN, "%lld", seqNum );
        _data_obj_info->dataId = seqNum; /* store as output parameter */

        status = splitPathByKey( _data_obj_info->objPath,
                                 logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/' );


        /* Check that collection exists and user has write permission.
           At the same time, also get the inherit flag */
        iVal = cmlCheckDirAndGetInheritFlag( logicalDirName,
                                             _comm->clientUser.userName,
                                             _comm->clientUser.rodsZone,
                                             ACCESS_MODIFY_OBJECT,
                                             &inheritFlag,
                                             mySessionTicket,
                                             mySessionClientAddr,
                                             &icss );
        if ( iVal < 0 ) {
            char errMsg[105];
            if ( iVal == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown",
                          logicalDirName );
                addRErrorMsg( &_comm->rError, 0, errMsg );
            }
            if ( iVal == CAT_NO_ACCESS_PERMISSION ) {
                snprintf( errMsg, 100, "no permission to update collection '%s'",
                          logicalDirName );
                addRErrorMsg( &_comm->rError, 0, errMsg );
            }
            return ERROR( iVal, "" );
        }
        snprintf( collIdNum, MAX_NAME_LEN, "%lld", iVal );

        /* Make sure no collection already exists by this name */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegDataObj SQL 4" );
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
            rodsLog( LOG_SQL, "chlRegDataObj SQL 5" );
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

        cllBindVars[0] = dataIdNum;
        cllBindVars[1] = collIdNum;
        cllBindVars[2] = logicalFileName;
        cllBindVars[3] = dataReplNum;
        cllBindVars[4] = _data_obj_info->version;
        cllBindVars[5] = _data_obj_info->dataType;
        cllBindVars[6] = dataSizeNum;
        cllBindVars[7] = _data_obj_info->rescName;
        cllBindVars[8] = _data_obj_info->rescHier;
        cllBindVars[9] = _data_obj_info->filePath;
        cllBindVars[10] = _comm->clientUser.userName;
        cllBindVars[11] = _comm->clientUser.rodsZone;
        cllBindVars[12] = dataStatusNum;
        cllBindVars[13] = _data_obj_info->chksum;
        cllBindVars[14] = _data_obj_info->dataMode;
        cllBindVars[15] = myTime;
        cllBindVars[16] = myTime;
        cllBindVars[17] = data_expiry_ts;
        cllBindVarCount = 18;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegDataObj SQL 6" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_DATA_MAIN (data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_name, resc_hier, data_path, data_owner_name, data_owner_zone, data_is_dirty, data_checksum, data_mode, create_ts, modify_ts, data_expiry_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegDataObj cmlExecuteNoAnswerSql failure %d", status );
            _rollback( "chlRegDataObj" );
            return ERROR( status, "chlRegDataObj cmlExecuteNoAnswerSql failure" );
        }
        std::string zone;
        ret = getLocalZone(
                  _ctx.prop_map(),
                  &icss,
                  zone );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "chlRegDataInfo - failed in getLocalZone with status [%d]", status );
            return PASS( ret );
        }

        if ( ( status = _updateObjCountOfResources( &icss, _data_obj_info->rescHier, zone.c_str(), 1 ) ) != 0 ) {
            return ERROR( status, "_updateObjCountOfResources failed" );
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
                rodsLog( LOG_SQL, "chlRegDataObj SQL 7" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select ?, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRegDataObj cmlExecuteNoAnswerSql insert access failure %d",
                         status );
                _rollback( "chlRegDataObj" );
                return ERROR( status, "cmlExecuteNoAnswerSql insert access failure" );
            }
        }
        else {
            cllBindVars[0] = dataIdNum;
            cllBindVars[1] = _comm->clientUser.userName;
            cllBindVars[2] = _comm->clientUser.rodsZone;
            cllBindVars[3] = ACCESS_OWN;
            cllBindVars[4] = myTime;
            cllBindVars[5] = myTime;
            cllBindVarCount = 6;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRegDataObj SQL 8" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_OBJT_ACCESS values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRegDataObj cmlExecuteNoAnswerSql insert access failure %d",
                         status );
                _rollback( "chlRegDataObj" );
                return ERROR( status, "cmlExecuteNoAnswerSql insert access failure" );
            }
        }

        status = cmlAudit3( AU_REGISTER_DATA_OBJ, dataIdNum,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone, "", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegDataObj cmlAudit3 failure %d",
                     status );
            _rollback( "chlRegDataObj" );
            return ERROR( status, "cmlAudit3 failure" );
        }


        if ( !( _data_obj_info->flags & NO_COMMIT_FLAG ) ) {
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRegDataObj cmlExecuteNoAnswerSql commit failure %d",
                         status );
                return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
            }
        }

        return SUCCESS();

    } // db_reg_data_obj_op


    // =-=-=-=-=-=-=-
    // register a data object into the catalog
    irods::error db_reg_replica_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
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
        if ( !_comm              ||
                !_src_data_obj_info ||
                !_dst_data_obj_info ||
                !_cond_input ) {
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
        char logicalFileName[MAX_NAME_LEN];
        char logicalDirName[MAX_NAME_LEN];
        rodsLong_t iVal;
        rodsLong_t status;
        char tSQL[MAX_SQL_SIZE];
        char *cVal[30];
        int i;
        int statementNumber;
        int nextReplNum;
        char nextRepl[30];
        char theColls[] = "data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_group_name, resc_name, resc_hier, data_path, data_owner_name, data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts, data_map_id, data_mode, r_comment, create_ts, modify_ts";
        int IX_DATA_REPL_NUM = 3; /* index of data_repl_num in theColls */
//        int IX_RESC_GROUP_NAME = 7; /* index into theColls */
        int IX_RESC_NAME = 8;    /* index into theColls */
        int IX_RESC_HIER = 9;
        int IX_DATA_PATH = 10;    /* index into theColls */

        int IX_DATA_MODE = 18;
        int IX_CREATE_TS = 20;
        int IX_MODIFY_TS = 21;
        int IX_RESC_NAME2 = 22;
        int IX_DATA_PATH2 = 23;
        int IX_DATA_ID2 = 24;
        int nColumns = 25;

        char objIdString[MAX_NAME_LEN];
        char replNumString[MAX_NAME_LEN];
        int adminMode;
        char *theVal;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegReplica" );
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

        status = splitPathByKey( _src_data_obj_info->objPath,
                                 logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/' );

        if ( adminMode ) {
            if ( _comm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
        }
        else {
            /* Check the access to the dataObj */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRegReplica SQL 1 " );
            }
            status = cmlCheckDataObjOnly( logicalDirName, logicalFileName,
                                          _comm->clientUser.userName,
                                          _comm->clientUser.rodsZone,
                                          ACCESS_READ_OBJECT, &icss );
            if ( status < 0 ) {
                _rollback( "chlRegReplica" );
                return ERROR( status, "cmlCheckDataObjOnly failed" );
            }
        }

        /* Get the next replica number */
        snprintf( objIdString, MAX_NAME_LEN, "%lld", _src_data_obj_info->dataId );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegReplica SQL 2" );
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
            rodsLog( LOG_SQL, "chlRegReplica SQL 3" );
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

        cVal[IX_DATA_REPL_NUM]   = nextRepl;
        cVal[IX_RESC_NAME]       = _dst_data_obj_info->rescName;
        cVal[IX_RESC_HIER]       = _dst_data_obj_info->rescHier;
        cVal[IX_DATA_PATH]       = _dst_data_obj_info->filePath;
        cVal[IX_DATA_MODE]       = _dst_data_obj_info->dataMode;


        getNowStr( myTime );
        cVal[IX_MODIFY_TS] = myTime;
        cVal[IX_CREATE_TS] = myTime;

        cVal[IX_RESC_NAME2] = _dst_data_obj_info->rescHier; // JMC - backport 4669
        cVal[IX_DATA_PATH2] = _dst_data_obj_info->filePath; // JMC - backport 4669
        cVal[IX_DATA_ID2] = objIdString; // JMC - backport 4669


        for ( i = 0; i < nColumns; i++ ) {
            cllBindVars[i] = cVal[i];
        }
        cllBindVarCount = nColumns;
#if (defined ORA_ICAT || defined MY_ICAT) // JMC - backport 4685
        /* MySQL and Oracle */
        snprintf( tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? from DUAL where not exists (select data_id from R_DATA_MAIN where resc_hier=? and data_path=? and data_id=?)",
                  theColls );
#else
        /* Postgres */
        snprintf( tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? where not exists (select data_id from R_DATA_MAIN where resc_hier=? and data_path=? and data_id=?)",
                  theColls );

#endif
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegReplica SQL 4" );
        }
        status = cmlExecuteNoAnswerSql( tSQL,  &icss );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegReplica cmlExecuteNoAnswerSql(insert) failure %d",
                     status );
            _rollback( "chlRegReplica" );
            return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "chlRegReplica - failed in getLocalZone with status [%d]", status );
            return PASS( ret );
        }

        if ( ( status = _updateObjCountOfResources( &icss, _dst_data_obj_info->rescHier, zone.c_str(), 1 ) ) != 0 ) {
            return ERROR( status, "_updateObjCountOfResources failed" );
        }

        status = cmlFreeStatement( statementNumber, &icss );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "chlRegReplica cmlFreeStatement failure %d", status );
            return ERROR( status, "cmlFreeStatement failure" );
        }

        status = cmlAudit3( AU_REGISTER_DATA_REPLICA, objIdString,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone, nextRepl, &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegDataReplica cmlAudit3 failure %d",
                     status );
            _rollback( "chlRegReplica" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegReplica cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_reg_replica_op

    // =-=-=-=-=-=-=-
    // unregister a data object
    irods::error db_unreg_replica_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
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
        if ( !_comm          ||
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
        char cVal[MAX_NAME_LEN];
        int adminMode;
        int trashMode;
        char *theVal;
        char checkPath[MAX_NAME_LEN];

        dataObjNumber[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlUnregDataObj" );
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

        status = splitPathByKey( _data_obj_info->objPath,
                                 logicalDirName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/' );


        if ( adminMode == 0 ) {
            /* Check the access to the dataObj */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlUnregDataObj SQL 1 " );
            }
            status = cmlCheckDataObjOnly( logicalDirName, logicalFileName,
                                          _comm->clientUser.userName,
                                          _comm->clientUser.rodsZone,
                                          ACCESS_DELETE_OBJECT, &icss );
            if ( status < 0 ) {
                _rollback( "chlUnregDataObj" );
                return ERROR( status, "cmlCheckDataObjOnly failed" ); /* convert long to int */
            }
            snprintf( dataObjNumber, sizeof dataObjNumber, "%lld", status );
        }
        else {
            if ( _comm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
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
                    addRErrorMsg( &_comm->rError, 0,
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
                    /* Check for a different replica */
                    snprintf( dataObjNumber, sizeof dataObjNumber, "%lld",
                              _data_obj_info->dataId );
                    snprintf( replNumber, sizeof replNumber, "%d", _data_obj_info->replNum );
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlUnregDataObj SQL 2" );
                    }
                    {
                        std::vector<std::string> bindVars;
                        bindVars.push_back( dataObjNumber );
                        bindVars.push_back( replNumber );
                        status = cmlGetStringValueFromSql(
                                     "select data_repl_num from R_DATA_MAIN where data_id=? and data_repl_num!=?",
                                     cVal, sizeof cVal, bindVars, &icss );
                    }
                    if ( status != 0 ) {
                        addRErrorMsg( &_comm->rError, 0,
                                      "This is the last replica, removal by admin not allowed" );
                        return ERROR( CAT_LAST_REPLICA, "This is the last replica, removal by admin not allowed" );
                    }
                }
                else {
                    addRErrorMsg( &_comm->rError, 0,
                                  "dataId and replNum required" );
                    _rollback( "chlUnregDataObj" );
                    return ERROR( CAT_INVALID_ARGUMENT, "dataId and replNum required" );
                }
            }
        }

        // Get the resource name so we can update its data object count later
        std::string resc_hier;
        if ( strlen( _data_obj_info->rescHier ) == 0 ) {
            if ( _data_obj_info->replNum >= 0 ) {
                snprintf( replNumber, sizeof replNumber, "%d", _data_obj_info->replNum );
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( dataObjNumber );
                    bindVars.push_back( replNumber );
                    status = cmlGetStringValueFromSql(
                                 "select resc_hier from R_DATA_MAIN where data_id=? and data_repl_num=?",
                                 cVal, sizeof cVal, bindVars, &icss );
                }
                if ( status != 0 ) {
                    return ERROR( status, "cmlGetStringValueFromSql failed" );
                }
            }
            else {
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( dataObjNumber );
                    status = cmlGetStringValueFromSql(
                                 "select resc_hier from R_DATA_MAIN where data_id=?",
                                 cVal, sizeof cVal, bindVars, &icss );
                }
                if ( status != 0 ) {
                    return ERROR( status, "cmlGetStringValueFromSql failed" );
                }
            }
            resc_hier = std::string( cVal );
        }
        else {
            resc_hier = std::string( _data_obj_info->rescHier );
        }

        cllBindVars[0] = logicalDirName;
        cllBindVars[1] = logicalFileName;
        if ( _data_obj_info->replNum >= 0 ) {
            snprintf( replNumber, sizeof replNumber, "%d", _data_obj_info->replNum );
            cllBindVars[2] = replNumber;
            cllBindVarCount = 3;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlUnregDataObj SQL 4" );
            }
            snprintf( tSQL, MAX_SQL_SIZE,
                      "delete from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?) and data_name=? and data_repl_num=?" );
        }
        else {
            cllBindVarCount = 2;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlUnregDataObj SQL 5" );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( status, "data object unknown" );
            }
            _rollback( "chlUnregDataObj" );
            return ERROR( status, "cmlExecuteNoAnswerSql failed" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "chlUnRegDataObj - failed in getLocalZone with status [%d]", status );
            return PASS( ret );
        }

        // update the object count in the resource
        if ( ( status = _updateObjCountOfResources( &icss, resc_hier, zone.c_str(), -1 ) ) != 0 ) {
            return ERROR( status, "_updateObjCountOfResources failed" );
        }

        /* delete the access rows, if we just deleted the last replica */
        if ( dataObjNumber[0] != '\0' ) {
            cllBindVars[0] = dataObjNumber;
            cllBindVars[1] = dataObjNumber;
            cllBindVarCount = 2;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlUnregDataObj SQL 3" );
            }
            status = cmlExecuteNoAnswerSql(
                         "delete from R_OBJT_ACCESS where object_id=? and not exists (select * from R_DATA_MAIN where data_id=?)", &icss );
            if ( status == 0 ) {
                removeMetaMapAndAVU( dataObjNumber ); /* remove AVU metadata, if any */
            }
        }

        /* Audit */
        if ( dataObjNumber[0] != '\0' ) {
            status = cmlAudit3( AU_UNREGISTER_DATA_OBJ, dataObjNumber,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone, "", &icss );
        }
        else {
            status = cmlAudit3( AU_UNREGISTER_DATA_OBJ, "0",
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                _data_obj_info->objPath, &icss );
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlUnregDataObj cmlAudit3 failure %d",
                     status );
            _rollback( "chlUnregDataObj" );
            return ERROR( status, "cmlAudit3 failure" );
        }


        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlUnregDataObj cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_unreg_replica_op

    // =-=-=-=-=-=-=-
    //
    irods::error db_reg_rule_exec_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        ruleExecSubmitInp_t*   _re_sub_inp ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm          ||
                !_re_sub_inp ) {
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
        rodsLong_t seqNum;
        char ruleExecIdNum[MAX_NAME_LEN];
        int status;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegRuleExec" );
        }
        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegRuleExec SQL 1 " );
        }
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlRegRuleExec cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlRegRuleExec" );
            return ERROR( seqNum, "cmlGetNextSeqVal failure" );
        }
        snprintf( ruleExecIdNum, MAX_NAME_LEN, "%lld", seqNum );

        /* store as output parameter */
        snprintf( _re_sub_inp->ruleExecId, NAME_LEN, "%s", ruleExecIdNum );

        getNowStr( myTime );

        cllBindVars[0] = ruleExecIdNum;
        cllBindVars[1] = _re_sub_inp->ruleName;
        cllBindVars[2] = _re_sub_inp->reiFilePath;
        cllBindVars[3] = _re_sub_inp->userName;
        cllBindVars[4] = _re_sub_inp->exeAddress;
        cllBindVars[5] = _re_sub_inp->exeTime;
        cllBindVars[6] = _re_sub_inp->exeFrequency;
        cllBindVars[7] = _re_sub_inp->priority;
        cllBindVars[8] = _re_sub_inp->estimateExeTime;
        cllBindVars[9] = _re_sub_inp->notificationAddr;
        cllBindVars[10] = myTime;
        cllBindVars[11] = myTime;

        cllBindVarCount = 12;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegRuleExec SQL 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_EXEC (rule_exec_id, rule_name, rei_file_path, user_name, exe_address, exe_time, exe_frequency, priority, estimated_exe_time, notification_addr, create_ts, modify_ts) values (?,?,?,?,?,?,?,?,?,?,?,?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegRuleExec cmlExecuteNoAnswerSql(insert) failure %d", status );
            _rollback( "chlRegRuleExec" );
            return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );

        }

        /* Audit */
        status = cmlAudit3( AU_REGISTER_DELAYED_RULE,  ruleExecIdNum,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _re_sub_inp->ruleName, &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegRuleExec cmlAudit3 failure %d",
                     status );
            _rollback( "chlRegRuleExec" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegRuleExec cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_reg_rule_exec_op

    // =-=-=-=-=-=-=-
    // unregister a data object
    irods::error db_mod_rule_exec_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _re_id,
        keyValPair_t*          _reg_param ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_re_id  ||
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


        // =-=-=-=-=-=-=-
        //
        int i, j, status;

        char tSQL[MAX_SQL_SIZE];
        char *theVal = 0;

        /* regParamNames has the argument names (in regParam) that this
           routine understands and colNames has the corresponding column
           names; one for one. */
        char *regParamNames[] = {
            RULE_NAME_KW, RULE_REI_FILE_PATH_KW, RULE_USER_NAME_KW,
            RULE_EXE_ADDRESS_KW, RULE_EXE_TIME_KW,
            RULE_EXE_FREQUENCY_KW, RULE_PRIORITY_KW, RULE_ESTIMATE_EXE_TIME_KW,
            RULE_NOTIFICATION_ADDR_KW, RULE_LAST_EXE_TIME_KW,
            RULE_EXE_STATUS_KW,
            "END"
        };
        char *colNames[] = {
            "rule_name", "rei_file_path", "user_name",
            "exe_address", "exe_time", "exe_frequency", "priority",
            "estimated_exe_time", "notification_addr",
            "last_exe_time", "exe_status",
            "create_ts", "modify_ts",
        };

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRuleExec" );
        }

        snprintf( tSQL, MAX_SQL_SIZE, "update R_RULE_EXEC set " );

        for ( i = 0, j = 0; strcmp( regParamNames[i], "END" ); i++ ) {
            theVal = getValByKey( _reg_param, regParamNames[i] );
            if ( theVal != NULL ) {
                if ( j > 0 ) {
                    rstrcat( tSQL, "," , MAX_SQL_SIZE );
                }
                rstrcat( tSQL, colNames[i] , MAX_SQL_SIZE );
                rstrcat( tSQL, "=?", MAX_SQL_SIZE );
                cllBindVars[j++] = theVal;
            }
        }

        if ( j == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid arguement" );
        }

        rstrcat( tSQL, "where rule_exec_id=?", MAX_SQL_SIZE );
        cllBindVars[j++] = _re_id;
        cllBindVarCount = j;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRuleExec SQL 1 " );
        }
        status =  cmlExecuteNoAnswerSql( tSQL, &icss );

        if ( status != 0 ) {
            _rollback( "chlModRuleExec" );
            rodsLog( LOG_NOTICE,
                     "chlModRuleExec cmlExecuteNoAnswer(update) failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswer(update) failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_MODIFY_DELAYED_RULE,  _re_id,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            "", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRuleExec cmlAudit3 failure %d",
                     status );
            _rollback( "chlModRuleExec" );
            return ERROR( status, "cmlAudit3 failure" );
        }


        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRuleExecMeta cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return CODE( status );

    } // db_mod_rule_exec_op

    // =-=-=-=-=-=-=-
    // unregister a data object
    irods::error db_del_rule_exec_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _re_id ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm ||
                !_re_id ) {
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
        char userName[MAX_NAME_LEN + 2];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelRuleExec" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            if ( _comm->proxyUser.authInfo.authFlag == LOCAL_USER_AUTH ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlDelRuleExec SQL 1 " );
                }
                int status;
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( _re_id );
                    status = cmlGetStringValueFromSql(
                                 "select user_name from R_RULE_EXEC where rule_exec_id=?",
                                 userName, MAX_NAME_LEN, bindVars, &icss );
                }
                if ( status != 0 || strncmp( userName, _comm->clientUser.userName, MAX_NAME_LEN )
                        != 0 ) {
                    return ERROR( CAT_NO_ACCESS_PERMISSION, "no access permission" );
                }
            }
            else {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
        }

        cllBindVars[cllBindVarCount++] = _re_id;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelRuleExec SQL 2 " );
        }
        int status =  cmlExecuteNoAnswerSql(
                          "delete from R_RULE_EXEC where rule_exec_id=?",
                          &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelRuleExec delete failure %d",
                     status );
            _rollback( "chlDelRuleExec" );
            return ERROR( status, "delete failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_DELETE_DELAYED_RULE,  _re_id,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            "", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelRuleExec cmlAudit3 failure %d",
                     status );
            _rollback( "chlDelRuleExec" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelRuleExec cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return CODE( status );

    } // db_del_rule_exec_op

    // =-=-=-=-=-=-=-
    //
    irods::error db_add_child_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        std::map<std::string, std::string> *_resc_input ) {

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm || !_resc_input ) {
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
        int result = 0;

        int status;
        static const char* func_name = "chlAddChildResc";
        irods::sql_logger logger( func_name, logSQL );
        std::string new_child_string( resc_input[irods::RESOURCE_CHILDREN] );
        std::string hierarchy, child_resc, root_resc;
        irods::children_parser parser;
        irods::hierarchy_parser hier_parser;
        rodsLong_t obj_count;
        char resc_id[MAX_NAME_LEN];


        logger.log();

        if ( !( result = _canConnectToCatalog( _comm ) ) ) {
            std::string zone;
            if ( !( ret = getLocalZone( _ctx.prop_map(), &icss, zone ) ).ok() ) {
                result = ret.code();

            }
            else if ( resc_input[irods::RESOURCE_ZONE].length() > 0 &&
                      resc_input[irods::RESOURCE_ZONE] != zone ) {
                addRErrorMsg( &_comm->rError, 0,
                              "Currently, resources must be in the local zone" );
                result = CAT_INVALID_ZONE;

            }
            else {

                logger.log();

                resc_id[0] = '\0';
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( resc_input[irods::RESOURCE_NAME] );
                    bindVars.push_back( zone );
                    status = cmlGetStringValueFromSql(
                                 "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                 resc_id, MAX_NAME_LEN, bindVars, &icss );
                }
                if ( status != 0 ) {
                    if ( status == CAT_NO_ROWS_FOUND ) {
                        result = CAT_INVALID_RESOURCE;
                    }
                    else {
                        _rollback( func_name );
                        result = status;
                    }
                }
                else if ( ( ret = _childIsValid( _ctx.prop_map(), new_child_string ) ).code() == 0 ) {
                    if ( ( status = _updateRescChildren(
                                        resc_id,
                                        new_child_string ) ) != 0 ) {
                        result = status;
                    }
                    else if ( ( ret = _updateChildParent(
                                          _ctx.prop_map(),
                                          new_child_string,
                                          resc_input[irods::RESOURCE_NAME] ) ).code() != 0 ) {
                        result = ret.code();
                    }
                    else {

                        // IF CHILD HAZ DATA
                        if ( _childHasData( new_child_string ) ) {

                            // =-=-=-=-=-=-=-
                            // Resolve resource hierarchy
                            status = chlGetHierarchyForResc(
                                         resc_input[irods::RESOURCE_NAME],
                                         zone,
                                         hierarchy );
                            if ( status < 0 ) {
                                std::stringstream ss;
                                ss << func_name
                                   << ": chlGetHierarchyForResc failed, status = "
                                   << status;
                                irods::log( LOG_NOTICE, ss.str() );
                                _rollback( func_name );
                                return ERROR( status, "chlGetHierarchyForResc failed" );
                            }


                            // =-=-=-=-=-=-=-
                            // Update object count for resources up the tree

                            // Get the resource name from the child string
                            parser.set_string( new_child_string );
                            parser.first_child( child_resc );

                            if ( _get_resc_obj_count( child_resc, obj_count ).ok() ) {
                                status = _updateObjCountOfResources( &icss, hierarchy, zone, obj_count );
                            }
                            else {
                                status = CAT_INVALID_OBJ_COUNT;
                            }

                            if ( status < 0 ) {
                                // rollback
                                std::stringstream ss;
                                ss << func_name
                                   << " aborted. Object count update error, status = "
                                   << status;
                                irods::log( LOG_NOTICE, ss.str() );
                                _rollback( func_name );
                                return ERROR( status, "object update error" );
                            }


                            // =-=-=-=-=-=-=-
                            // Update resource hierarchy for objects down the tree

                            // Add child resource to hierarchy for substitution
                            hierarchy += irods::hierarchy_parser::delimiter() + child_resc;

                            // Substitute 'child' with '...;parent;child'
                            status = chlSubstituteResourceHierarchies(
                                         _comm,
                                         child_resc.c_str(),
                                         hierarchy.c_str() );

                            // =-=-=-=-=-=-=-
                            // Update resource name for objects in child
                            // All objects formerly in child resource must now be in hierarchy's root resource

                            // get root resource
                            hier_parser.set_string( hierarchy );
                            hier_parser.first_resc( root_resc );

                            cllBindVars[cllBindVarCount++] = ( char* )root_resc.c_str();
                            cllBindVars[cllBindVarCount++] = ( char* )root_resc.c_str();
                            cllBindVars[cllBindVarCount++] = ( char* )child_resc.c_str();
                            status = cmlExecuteNoAnswerSql(
                                         "update R_DATA_MAIN set resc_name = ?, resc_group_name = ? where resc_name = ?", &icss );

                            if ( status < 0 ) {
                                // rollback
                                std::stringstream ss;
                                ss << func_name
                                   << " aborted. Resource update error, status = "
                                   << status;
                                irods::log( LOG_NOTICE, ss.str() );
                                _rollback( func_name );
                                return ERROR( status, "resource update error" );
                            }

                        }  // IF CHILD HAZ DATA


                        /* Audit */
                        char commentStr[1024]; // this prolly should be better sized
                        snprintf(
                            commentStr,
                            sizeof commentStr,
                            "%s %s",
                            resc_input[irods::RESOURCE_NAME].c_str(),
                            new_child_string.c_str() );
                        if ( ( status = cmlAudit3(
                                            AU_ADD_CHILD_RESOURCE,
                                            resc_id,
                                            _comm->clientUser.userName,
                                            _comm->clientUser.rodsZone,
                                            commentStr,
                                            &icss ) ) != 0 ) {
                            std::stringstream ss;
                            ss << func_name << " cmlAudit3 failure " << status;
                            irods::log( LOG_NOTICE, ss.str() );
                            _rollback( func_name );
                            result = status;
                        }
                        else if ( ( status =  cmlExecuteNoAnswerSql( "commit", &icss ) ) != 0 ) {
                            std::stringstream ss;
                            ss << func_name << " cmlExecuteNoAnswerSql commit failure " << status;
                            irods::log( LOG_NOTICE, ss.str() );
                            result = status;
                        }
                    }
                }
                else {
                    std::string resc_name;
                    irods::children_parser parser;
                    parser.set_string( new_child_string );
                    parser.first_child( resc_name );

                    std::stringstream msg;
                    msg << "Encountered an error adding '" << resc_name << "' as a child resource.";
                    ret = PASSMSG( msg.str(), ret );
                    addRErrorMsg( &_comm->rError, 0, ret.result().c_str() );
                    result = ret.code();
                }
            }
        }

        return CODE( result );

    } // db_add_child_resc_op

    // =-=-=-=-=-=-=-
    //
    irods::error db_reg_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        std::map<std::string, std::string> *_resc_input ) {

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm || !_resc_input ) {
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
        char myTime[50];
        struct hostent *myHostEnt; // JMC - backport 4597

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegResc" );
        }

        // =-=-=-=-=-=-=-
        // error trap empty resc name
        if ( resc_input[irods::RESOURCE_NAME].length() < 1 ) {
            addRErrorMsg( &_comm->rError, 0, "resource name is empty" );
            return ERROR( CAT_INVALID_RESOURCE_NAME, "resource name is empty" );
        }

        // =-=-=-=-=-=-=-
        // error trap empty resc type
        if ( resc_input[irods::RESOURCE_TYPE].length() < 1 ) {
            addRErrorMsg( &_comm->rError, 0, "resource type is empty" );
            return ERROR( CAT_INVALID_RESOURCE_TYPE, "resource type is empty" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlRegResc SQL 1 " );
        }
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlRegResc cmlGetNextSeqVal failure %d",
                     seqNum );
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
                addRErrorMsg( &_comm->rError, 0,
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
            _resolveHostName( _comm, resc_input[irods::RESOURCE_LOCATION].c_str(), myHostEnt );
        }

        getNowStr( myTime );

        cllBindVars[0] = idNum;
        cllBindVars[1] = resc_input[irods::RESOURCE_NAME].c_str();
        cllBindVars[2] = ( char* )zone.c_str();
        cllBindVars[3] = resc_input[irods::RESOURCE_TYPE].c_str();
        cllBindVars[4] = resc_input[irods::RESOURCE_CLASS].c_str();
        cllBindVars[5] = resc_input[irods::RESOURCE_LOCATION].c_str();
        cllBindVars[6] = resc_input[irods::RESOURCE_PATH].c_str();
        cllBindVars[7] = myTime;
        cllBindVars[8] = myTime;
        cllBindVars[9] = resc_input[irods::RESOURCE_CHILDREN].c_str();
        cllBindVars[10] = resc_input[irods::RESOURCE_CONTEXT].c_str();
        cllBindVars[11] = resc_input[irods::RESOURCE_PARENT].c_str();
        cllBindVars[12] = "0";
        cllBindVarCount = 13;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegResc SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, resc_class_name, resc_net, resc_def_path, create_ts, modify_ts, resc_children, resc_context, resc_parent, resc_objcount) values (?,?,?,?,?,?,?,?,?,?,?,?,?)",
                      &icss );

        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegResc cmlExectuteNoAnswerSql(insert) failure %d",
                     status );
            _rollback( "chlRegResc" );
            return ERROR( status, "cmlExectuteNoAnswerSql(insert) failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_REGISTER_RESOURCE,  idNum,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            resc_input[irods::RESOURCE_NAME].c_str(), &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegResc cmlAudit3 failure %d",
                     status );
            _rollback( "chlRegResc" );
            return ERROR( status, "chlRegResc cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegResc cmlExecuteNoAnswerSql commit failure %d", status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return CODE( status );

    } // db_reg_resc_op

    // =-=-=-=-=-=-=-
    //
    irods::error db_del_child_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        std::map<std::string, std::string> *_resc_input ) {

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm || !_resc_input ) {
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
        irods::sql_logger logger( "chlDelChildResc", logSQL );
        irods::error result;
        int status = 0;
        rodsLong_t obj_count = 0;
        char resc_id[MAX_NAME_LEN];
        std::string child_string( resc_input[irods::RESOURCE_CHILDREN] );
        std::string child, hierarchy, partial_hier;
        irods::children_parser parser;

        parser.set_string( child_string );
        parser.first_child( child );

        std::string zone;
        if ( !( status = _canConnectToCatalog( _comm ) ) ) {
            if ( ( result = getLocalZone( _ctx.prop_map(), &icss, zone ) ).ok() ) {
                logger.log();

                resc_id[0] = '\0';
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( resc_input[irods::RESOURCE_NAME] );
                    bindVars.push_back( zone );
                    status = cmlGetStringValueFromSql(
                                 "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                 resc_id, MAX_NAME_LEN, bindVars, &icss );
                }
                if ( status != 0 ) {
                    if ( status == CAT_NO_ROWS_FOUND ) {
                        std::string msg( "invalid resource name " );
                        msg += resc_input[irods::RESOURCE_NAME];
                        result = ERROR( CAT_INVALID_RESOURCE, msg );
                    }
                    else {
                        _rollback( "chlDelChildResc" );
                        result = CODE( status );
                    }

                }
                else if ( !( result = _updateChildParent(
                                          _ctx.prop_map(),
                                          child,
                                          std::string( "" ) ) ).ok() ) {
                    result = PASS( result );
                }
                else if ( ( status = _removeRescChild( resc_id, child ) ) != 0 ) {
                    result = ERROR( status, "_removeRescChild failed" );
                }
                else {

                    // IF CHILD HAZ DATA
                    if ( _childHasData( child_string ) ) {

                        // =-=-=-=-=-=-=-
                        // Resolve resource hierarchy (of parent)
                        status = chlGetHierarchyForResc(
                                     resc_input[irods::RESOURCE_NAME],
                                     zone.c_str(),
                                     hierarchy );
                        if ( status < 0 ) {
                            std::stringstream ss;
                            ss << "chlDelChildResc: chlGetHierarchyForResc failed, status = " << status;
                            irods::log( LOG_NOTICE, ss.str() );
                            _rollback( "chlDelChildResc" );
                            return ERROR( status, "chlGetHierarchyForResc failed" );
                        }

                        // =-=-=-=-=-=-=-
                        // Update object count for resources up the tree

                        if ( _get_resc_obj_count( child, obj_count ).ok() ) {
                            status = _updateObjCountOfResources(
                                         &icss,
                                         hierarchy,
                                         zone.c_str(),
                                         -obj_count );
                        }
                        else {
                            status = CAT_INVALID_OBJ_COUNT;

                        }

                        if ( status < 0 ) {
                            // rollback
                            std::stringstream ss;
                            ss << "chlDelChildResc aborted. Object count update error, status = " << status;
                            irods::log( LOG_NOTICE, ss.str() );
                            _rollback( "chlDelChildResc" );
                            return ERROR( status, "Object count update error" );
                        }


                        // =-=-=-=-=-=-=-
                        // Update resource hierarchy for objects down the tree

                        // Add child resource to hierarchy for substitution
                        hierarchy += irods::hierarchy_parser::delimiter() + child;

                        // Substitute '...;parent;child' with 'child
                        status = chlSubstituteResourceHierarchies( _comm, hierarchy.c_str(), child.c_str() );

                        // =-=-=-=-=-=-=-
                        // Update resource name for objects in child
                        // If B is removed from A, all files whose resc_hier starts with B are now on B
                        partial_hier = child + irods::hierarchy_parser::delimiter() + "%";

                        cllBindVars[cllBindVarCount++] = ( char* )child.c_str();
                        cllBindVars[cllBindVarCount++] = ( char* )child.c_str();
                        cllBindVars[cllBindVarCount++] = ( char* )child.c_str();
                        cllBindVars[cllBindVarCount++] = ( char* )partial_hier.c_str();
                        status = cmlExecuteNoAnswerSql(
                                     "update R_DATA_MAIN set resc_name = ?, resc_group_name = ? where resc_hier = ? or resc_hier like ?", &icss );

                        if ( status < 0 ) {
                            // rollback
                            std::stringstream ss;
                            ss << "chlDelChildResc" << " aborted. Resource update error, status = " << status;
                            irods::log( LOG_NOTICE, ss.str() );
                            _rollback( "chlDelChildResc" );
                            return ERROR( status, "Resource update error" );
                        }

                    }  // IF CHILD HAZ DATA

                    /* Audit */
                    char commentStr[1024]; // this prolly should be better sized
                    snprintf( commentStr, sizeof commentStr, "%s %s", resc_input[irods::RESOURCE_NAME].c_str(), child_string.c_str() );
                    if ( ( status = cmlAudit3( AU_DEL_CHILD_RESOURCE, resc_id, _comm->clientUser.userName, _comm->clientUser.rodsZone,
                                               commentStr, &icss ) ) != 0 ) {
                        std::stringstream ss;
                        ss << "chlDelChildResc cmlAudit3 failure " << status;
                        irods::log( LOG_NOTICE, ss.str() );
                        _rollback( "chlDelChildResc" );
                        result = ERROR( status, "cmlAudit3 failure" );
                    }
                    else if ( ( status =  cmlExecuteNoAnswerSql( "commit", &icss ) ) != 0 ) {
                        std::stringstream ss;
                        ss << "chlDelChildResc cmlExecuteNoAnswerSql commit failure " << status;
                        irods::log( LOG_NOTICE, ss.str() );
                        result = ERROR( status, "cmlExecuteNoAnswerSql failure" );
                    }
                }
            }
        }

        return result;

    } // db_del_child_resc_op

    // =-=-=-=-=-=-=-
    // delete a resource
    irods::error db_del_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
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
        if ( !_comm || !_resc_name ) {
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
        char rescId[MAX_NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelResc" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        // =-=-=-=-=-=-=-
        // JMC - backport 4629
        if ( strncmp( _resc_name, BUNDLE_RESC, strlen( BUNDLE_RESC ) ) == 0 ) {
            char errMsg[155];
            snprintf( errMsg, 150,
                      "%s is a built-in resource needed for bundle operations.",
                      BUNDLE_RESC );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_PSEUDO_RESC_MODIFY_DISALLOWED, "cannot delete bundle resc" );
        }
        // =-=-=-=-=-=-=-

        if ( _rescHasData( _resc_name ) ) {
            char errMsg[105];
            snprintf( errMsg, 100,
                      "resource '%s' contains one or more dataObjects",
                      _resc_name );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_RESOURCE_NOT_EMPTY, "resc not empty" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        /* get rescId for possible audit call; won't be available after delete */
        rescId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelResc SQL 2 " );
        }
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_RESOURCE_NOT_EMPTY, "resource not empty" );
        }

        cllBindVars[cllBindVarCount++] = _resc_name;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelResc SQL 3" );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( status, "resource does not exist" );
            }
            _rollback( "chlDelResc" );
            return ERROR( status, "resource does not exist" );
        }

        /* Remove it from resource groups, if any */
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelResc SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_RESC_GROUP where resc_id=?",
                      &icss );
        if ( status != 0 &&
                status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlDelResc delete from R_RESC_GROUP failure %d",
                     status );
            _rollback( "chlDelResc" );
            return ERROR( status, "chlDelResc delete from R_RESC_GROUP failure" );
        }


        /* Remove associated AVUs, if any */
        removeMetaMapAndAVU( rescId );


        /* Audit */
        status = cmlAudit3( AU_DELETE_RESOURCE,
                            rescId,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _resc_name,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelResc cmlAudit3 failure %d",
                     status );
            _rollback( "chlDelResc" );
            return ERROR( status, "cmlAudi3 failure" );
        }

        if ( _dry_run ) { // JMC
            _rollback( "chlDelResc" );
            return CODE( status );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelResc cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }
        return CODE( status );

    } // db_del_resc_op

    // =-=-=-=-=-=-=-
    // rollback the db
    irods::error db_rollback_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm ) {
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
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRollback - SQL 1 " );
        }

        int status =  cmlExecuteNoAnswerSql( "rollback", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRollback cmlExecuteNoAnswerSql failure %d",
                     status );
            return ERROR( status, "chlRollback cmlExecuteNoAnswerSql failure" );
        }

        return CODE( status );

    } // db_rollback_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_commit_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm ) {
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
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCommit - SQL 1 " );
        }
        int status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlCommit cmlExecuteNoAnswerSql failure %d",
                     status );
            return ERROR( status, "chlCommit cmlExecuteNoAnswerSql failure" );
        }

        return CODE( status );

    } // db_commit_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_del_user_re_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        userInfo_t*            _user_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char userStr[200];
        char userName2[NAME_LEN];
        char zoneName[NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelUserRE" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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

        status = parseUserName( _user_info->userName, userName2, zoneName );
        if ( zoneName[0] != '\0' ) {
            rstrcpy( zoneToUse, zoneName, NAME_LEN );
        }

        if ( strncmp( _comm->clientUser.userName, userName2, sizeof( userName2 ) ) == 0 &&
                strncmp( _comm->clientUser.rodsZone, zoneToUse, sizeof( zoneToUse ) ) == 0 ) {
            addRErrorMsg( &_comm->rError, 0, "Cannot remove your own admin account, probably unintended" );
            return ERROR( CAT_INVALID_USER, "invalid user" );
        }


        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelUserRE SQL 1 " );
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
            addRErrorMsg( &_comm->rError, 0, "Invalid user" );
            return ERROR( CAT_INVALID_USER, "invalid user" );
        }
        if ( status != 0 ) {
            _rollback( "chlDelUserRE" );
            return ERROR( status, "failed getting user from table" );
        }

        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneToUse;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelUserRE SQL 2" );
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
            rodsLog( LOG_SQL, "chlDelUserRE SQL 3" );
        }
        status = cmlExecuteNoAnswerSql(
                     "delete from R_USER_PASSWORD where user_id=?",
                     &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[MAX_NAME_LEN + 40];
            rodsLog( LOG_NOTICE,
                     "chlDelUserRE delete password failure %d",
                     status );
            snprintf( errMsg, sizeof errMsg, "Error removing password entry" );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            _rollback( "chlDelUserRE" );
            return ERROR( status, "Error removing password entry" );
        }

        /* Remove both the special user_id = group_user_id entry and any
           other access entries for this user (or group) */
        cllBindVars[cllBindVarCount++] = iValStr;
        cllBindVars[cllBindVarCount++] = iValStr;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelUserRE SQL 4" );
        }
        status = cmlExecuteNoAnswerSql(
                     "delete from R_USER_GROUP where user_id=? or group_user_id=?",
                     &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[MAX_NAME_LEN + 40];
            rodsLog( LOG_NOTICE,
                     "chlDelUserRE delete user_group entry failure %d",
                     status );
            snprintf( errMsg, sizeof errMsg, "Error removing user_group entry" );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            _rollback( "chlDelUserRE" );
            return ERROR( status, "Error removing user_group entry" );
        }

        /* Remove any R_USER_AUTH rows for this user */
        cllBindVars[cllBindVarCount++] = iValStr;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelUserRE SQL 4" );
        }
        status = cmlExecuteNoAnswerSql(
                     "delete from R_USER_AUTH where user_id=?",
                     &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            char errMsg[MAX_NAME_LEN + 40];
            rodsLog( LOG_NOTICE,
                     "chlDelUserRE delete user_auth entries failure %d",
                     status );
            snprintf( errMsg, sizeof errMsg, "Error removing user_auth entries" );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            _rollback( "chlDelUserRE" );
            return ERROR( status, "Error removing user_auth entries" );
        }

        /* Remove associated AVUs, if any */
        removeMetaMapAndAVU( iValStr );

        /* Audit */
        snprintf( userStr, sizeof userStr, "%s#%s",
                  userName2, zoneToUse );
        status = cmlAudit3( AU_DELETE_USER_RE,
                            iValStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            userStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelUserRE cmlAudit3 failure %d",
                     status );
            _rollback( "chlDelUserRE" );
            return ERROR( status, "chlDelUserRE cmlAudit3 failure" );
        }

        return SUCCESS();

    } // db_del_user_re_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_reg_coll_by_admin_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        collInfo_t*            _coll_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char userName2[NAME_LEN];
        char zoneName[NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegCollByAdmin" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        // =-=-=-=-=-=-=-
        // JMC - backport 4772
        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
                _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            int status2;
            status2  = cmlCheckGroupAdminAccess(
                           _comm->clientUser.userName,
                           _comm->clientUser.rodsZone,
                           "", &icss );
            if ( status2 != 0 ) {
                return ERROR( status2, "no group admin access" );
            }
            if ( creatingUserByGroupAdmin == 0 ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficent privilege" );
            }
            // =-=-=-=-=-=-=-
        }

        status = splitPathByKey( _coll_info->collName,
                                 logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
        }

        /* Check that the parent collection exists */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegCollByAdmin SQL 1 " );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( status, "collection is unknown" );
            }
            _rollback( "chlRegCollByAdmin" );
            return ERROR( status, "collection not found" );
        }

        snprintf( collIdNum, MAX_NAME_LEN, "%d", status );

        /* String to get next sequence item for objects */
        cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegCollByAdmin SQL 2" );
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
        status = parseUserName( _coll_info->collOwnerName, userName2, zoneName );
        if ( zoneName[0] == '\0' ) {
            rstrcpy( zoneName, zone.c_str(), NAME_LEN );
        }

        cllBindVars[cllBindVarCount++] = logicalParentDirName;
        cllBindVars[cllBindVarCount++] = _coll_info->collName;
        cllBindVars[cllBindVarCount++] = userName2;
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
            rodsLog( LOG_SQL, "chlRegCollByAdmin SQL 3" );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
            }

            rodsLog( LOG_NOTICE,
                     "chlRegCollByAdmin cmlExecuteNoAnswerSQL(insert) failure %d"
                     , status );
            _rollback( "chlRegCollByAdmin" );
            return ERROR( status, "cmlExecuteNoAnswerSQL(insert) failure" );
        }

        /* String to get current sequence item for objects */
        cllCurrentValueString( "R_ObjectID", currStr, MAX_NAME_LEN );
        snprintf( currStr2, MAX_SQL_SIZE, " %s ", currStr );

        cllBindVars[cllBindVarCount++] = userName2;
        cllBindVars[cllBindVarCount++] = zoneName;
        cllBindVars[cllBindVarCount++] = ACCESS_OWN;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;

        snprintf( tSQL, MAX_SQL_SIZE,
                  "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                  currStr2 );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegCollByAdmin SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql( tSQL, &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegCollByAdmin cmlExecuteNoAnswerSql(insert access) failure %d",
                     status );
            _rollback( "chlRegCollByAdmin" );
            return ERROR( status, "cmlExecuteNoAnswerSql(insert access) failure" );
        }

        /* Audit */
        status = cmlAudit4( AU_REGISTER_COLL_BY_ADMIN,
                            currStr2,
                            "",
                            userName2,
                            zoneName,
                            _comm->clientUser.userName,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegCollByAdmin cmlAudit4 failure %d",
                     status );
            _rollback( "chlRegCollByAdmin" );
            return ERROR( status, "cmlAudit4 failure" );
        }

        return SUCCESS();

    } // db_reg_coll_by_admin_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_reg_coll_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        collInfo_t*            _coll_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlRegColl" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        status = splitPathByKey( _coll_info->collName,
                                 logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
        }

        /* Check that the parent collection exists and user has write permission,
           and get the collectionID.  Also get the inherit flag */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegColl SQL 1 " );
        }
        status = cmlCheckDirAndGetInheritFlag( logicalParentDirName,
                                               _comm->clientUser.userName,
                                               _comm->clientUser.rodsZone,
                                               ACCESS_MODIFY_OBJECT, &inheritFlag,
                                               mySessionTicket, mySessionClientAddr, &icss );
        if ( status < 0 ) {
            char errMsg[105];
            if ( status == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown",
                          logicalParentDirName );
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( status, "collection is unknown" );
            }
            _rollback( "chlRegColl" );
            return ERROR( status, "cmlCheckDirAndGetInheritFlag failed" );
        }
        snprintf( collIdNum, MAX_NAME_LEN, "%lld", status );

        /* Check that the path is not already a dataObj */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegColl SQL 2" );
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
            return ERROR( CAT_NAME_EXISTS_AS_DATAOBJ, "data obj alread exists" );
        }


        /* String to get next sequence item for objects */
        cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

        getNowStr( myTime );

        cllBindVars[cllBindVarCount++] = logicalParentDirName;
        cllBindVars[cllBindVarCount++] = _coll_info->collName;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone;
        cllBindVars[cllBindVarCount++] = _coll_info->collType;
        cllBindVars[cllBindVarCount++] = _coll_info->collInfo1;
        cllBindVars[cllBindVarCount++] = _coll_info->collInfo2;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegColl SQL 3" );
        }
        snprintf( tSQL, MAX_SQL_SIZE,
                  "insert into R_COLL_MAIN (coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_type, coll_info1, coll_info2, create_ts, modify_ts) values (%s, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                  nextStr );
        status =  cmlExecuteNoAnswerSql( tSQL,
                                         &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegColl cmlExecuteNoAnswerSql(insert) failure %d", status );
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
                rodsLog( LOG_SQL, "chlRegColl SQL 4" );
            }
            snprintf( tSQL, MAX_SQL_SIZE,
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select %s, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
                      currStr2 );
            status =  cmlExecuteNoAnswerSql( tSQL, &icss );

            if ( status == 0 ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlRegColl SQL 5" );
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
                          "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=%s",
                          currStr2 );
                status =  cmlExecuteNoAnswerSql( tSQL, &icss );
#endif
            }
        }
        else {
            cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
            cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone;
            cllBindVars[cllBindVarCount++] = ACCESS_OWN;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = myTime;
            snprintf( tSQL, MAX_SQL_SIZE,
                      "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                      currStr2 );
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRegColl SQL 6" );
            }
            status =  cmlExecuteNoAnswerSql( tSQL, &icss );
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegColl cmlExecuteNoAnswerSql(insert access) failure %d",
                     status );
            _rollback( "chlRegColl" );
            return ERROR( status, "cmlExecuteNoAnswerSql(insert access) failure" );
        }

        /* Audit */
        status = cmlAudit4( AU_REGISTER_COLL,
                            currStr2,
                            "",
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _coll_info->collName,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegColl cmlAudit4 failure %d",
                     status );
            _rollback( "chlRegColl" );
            return ERROR( status, "cmlAudit4 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegColl cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return CODE( status );

    } // db_reg_coll_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_mod_coll_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        collInfo_t*            _coll_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char iValStr[60];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModColl" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        /* Check that collection exists and user has write permission */
        iVal = cmlCheckDir( _coll_info->collName,  _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            ACCESS_MODIFY_OBJECT, &icss );

        if ( iVal < 0 ) {
            char errMsg[105];
            if ( iVal == CAT_UNKNOWN_COLLECTION ) {
                snprintf( errMsg, 100, "collection '%s' is unknown",
                          _coll_info->collName );
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( CAT_UNKNOWN_COLLECTION, "unknown collection" );
            }
            if ( iVal == CAT_NO_ACCESS_PERMISSION ) {
                snprintf( errMsg, 100, "no permission to update collection '%s'",
                          _coll_info->collName );
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return  ERROR( CAT_NO_ACCESS_PERMISSION, "no permission" );
            }
            return ERROR( iVal, "cmlCheckDir failed" );
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
        if ( count == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "count is 0" );
        }
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _coll_info->collName;
        tSQL += ", modify_ts=? where coll_name=?";

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModColl SQL 1" );
        }
        status =  cmlExecuteNoAnswerSql( tSQL.c_str(),
                                         &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModColl cmlExecuteNoAnswerSQL(update) failure %d", status );
            return ERROR( status, "cmlExecuteNoAnswerSQL(update) failure" );
        }

        /* Audit */
        snprintf( iValStr, sizeof iValStr, "%lld", iVal );
        status = cmlAudit3( AU_REGISTER_COLL,
                            iValStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _coll_info->collName,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModColl cmlAudit3 failure %d",
                     status );
            return ERROR( status, "cmlAudit3 failure" );
        }

        return SUCCESS();

    } // db_mod_coll_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_reg_zone_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _zone_name,
        char*                  _zone_type,
        char*                  _zone_conn_info,
        char*                  _zone_comment ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlRegZone" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        if ( strncmp( _zone_type, "remote", 6 ) != 0 ) {
            addRErrorMsg( &_comm->rError, 0,
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

        /* String to get next sequence item for objects */
        cllNextValueString( "R_ObjectID", nextStr, MAX_NAME_LEN );

        getNowStr( myTime );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegZone SQL 1 " );
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
            rodsLog( LOG_NOTICE,
                     "chlRegZone cmlExecuteNoAnswerSql(insert) failure %d", status );
            _rollback( "chlRegZone" );
            return ERROR( status, "cmlExecuteNoAnswerSql(insert) failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_REGISTER_ZONE,  "0",
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            "", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegResc cmlAudit3 failure %d",
                     status );
            return ERROR( status, "cmlAudit3 failure" );
        }


        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegZone cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_reg_zone_op

    // =-=-=-=-=-=-=-
    // modify the zone
    irods::error db_mod_zone_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _zone_name,
        char*                  _option,
        char*                  _option_value ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char commentStr[200];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModZone" );
        }

        if ( *_zone_name == '\0' || *_option == '\0' || *_option_value == '\0' ) {
            return  ERROR( CAT_INVALID_ARGUMENT, "invalid arument value" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        zoneId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModZone SQL 1 " );
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
                rodsLog( LOG_SQL, "chlModZone SQL 3" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_ZONE_MAIN set r_comment = ?, modify_ts=? where zone_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModZone cmlExecuteNoAnswerSql update failure %d",
                         status );
                return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
            }
            OK = 1;
        }
        if ( strcmp( _option, "conn" ) == 0 ) {
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = zoneId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModZone SQL 5" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_ZONE_MAIN set zone_conn_string = ?, modify_ts=? where zone_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModZone cmlExecuteNoAnswerSql update failure %d",
                         status );
                return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
            }
            OK = 1;
        }
        if ( strcmp( _option, "name" ) == 0 ) {
            if ( strcmp( _zone_name, zone.c_str() ) == 0 ) {
                addRErrorMsg( &_comm->rError, 0,
                              "It is not valid to rename the local zone via chlModZone; iadmin should use acRenameLocalZone" );
                return ERROR( CAT_INVALID_ARGUMENT, "cannot rename localzone" );
            }
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = zoneId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModZone SQL 5" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_ZONE_MAIN set zone_name = ?, modify_ts=? where zone_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModZone cmlExecuteNoAnswerSql update failure %d",
                         status );
                return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
            }
            OK = 1;
        }
        if ( OK == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
        }

        /* Audit */
        snprintf( commentStr, sizeof commentStr, "%s %s", _option, _option_value );
        status = cmlAudit3( AU_MOD_ZONE,
                            zoneId,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            commentStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModZone cmlAudit3 failure %d",
                     status );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModZone cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_mod_zone_op

    // =-=-=-=-=-=-=-
    // modify the zone
    irods::error db_rename_coll_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _old_coll,
        char*                  _new_coll ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlRenameColl SQL 1 " );
        }

        status1 = cmlCheckDir( _old_coll,
                               _comm->clientUser.userName,
                               _comm->clientUser.rodsZone,
                               ACCESS_OWN,
                               &icss );

        if ( status1 < 0 ) {
            return ERROR( status1, "cmlCheckDir failed" );
        }

        /* call chlRenameObject to rename */
        status = chlRenameObject( _comm, status1, _new_coll );
        if ( !status ) {
            return ERROR( status, "chlRenameObject failed" );
        }

        return CODE( status );

    } // db_rename_coll_op

    // =-=-=-=-=-=-=-
    // modify the zone
    irods::error db_mod_zone_coll_acl_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _access_level,
        char*                  _user_name,
        char*                  _path_name ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char *cp;
        if ( *_path_name != '/' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid path name" );
        }
        cp = _path_name + 1;
        if ( strstr( cp, PATH_SEPARATOR ) != NULL ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid path name" );
        }
        status =  chlModAccessControl( _comm, 0,
                                       _access_level,
                                       _user_name,
                                       _comm->clientUser.rodsZone,
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
        rsComm_t*              _comm,
        char*                  _old_zone,
        char*                  _new_zone ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char commentStr[200];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 1 " );
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
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 2 " );
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

        /* Audit */
        /* Do this first, before the userName-zone is made invalid;
           it will be rolledback if an error occurs */

        snprintf( commentStr, sizeof commentStr, "renamed local zone %s to %s",
                  _old_zone, _new_zone );
        status = cmlAudit3( AU_MOD_ZONE,
                            "0",
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            commentStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlAudit3 failure %d",
                     status );
            return ERROR( status, "cmlAudit3 failure" );
        }

        /* update coll_owner_zone in R_COLL_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 3 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_COLL_MAIN set coll_owner_zone = ?, modify_ts=? where coll_owner_zone=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* update data_owner_zone in R_DATA_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 4 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_DATA_MAIN set data_owner_zone = ?, modify_ts=? where data_owner_zone=?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* update zone_name in R_RESC_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 5 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_RESC_MAIN set zone_name = ?, modify_ts=? where zone_name=?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* update rule_owner_zone in R_RULE_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 6 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_RULE_MAIN set rule_owner_zone=?, modify_ts=? where rule_owner_zone=?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* update zone_name in R_USER_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 7 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_USER_MAIN set zone_name=?, modify_ts=? where zone_name=?",
                      &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        /* update zone_name in R_ZONE_MAIN */
        cllBindVars[cllBindVarCount++] = _new_zone;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _old_zone;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameLocalZone SQL 8 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_ZONE_MAIN set zone_name=?, modify_ts=? where zone_name=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
        }

        return SUCCESS();

    } // db_rename_local_zone_op

    // =-=-=-=-=-=-=-
    // modify the zone
    irods::error db_del_zone_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _zone_name ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlDelZone" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelZone SQL 1 " );
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
            addRErrorMsg( &_comm->rError, 0,
                          "It is not permitted to remove the local zone" );
            return ERROR( CAT_INVALID_ARGUMENT, "cannot remove local zone" );
        }

        cllBindVars[cllBindVarCount++] = _zone_name;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelZone 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_ZONE_MAIN where zone_name = ?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelZone cmlExecuteNoAnswerSql delete failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql delete failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_DELETE_ZONE,
                            "0",
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _zone_name,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelZone cmlAudit3 failure %d",
                     status );
            _rollback( "chlDelZone" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelZone cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "cmlExecuteNoAnswerSql commit failure" );
        }

        return SUCCESS();

    } // db_del_zone_op

    // =-=-=-=-=-=-=-
    // modify the zone
    irods::error db_simple_query_op_vector(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _sql,
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
        // check the params
        if ( !_comm ) {
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
        int stmtNum, status, nCols, i, needToGet, didGet;
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
        //rodsLog( LOG_NOTICE, "JMC :: sql - %s", sql );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSimpleQuery" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 1 " );
        }
        if ( i == 1 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 2" );
        }
        if ( i == 2 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 3" );
        }
        if ( i == 3 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 4" );
        }
        if ( i == 4 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 5" );
        }
        if ( i == 5 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 6" );
        }
        if ( i == 6 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 7" );
        }
        if ( i == 7 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 8" );
        }
        if ( i == 8 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 9" );
        }
        if ( i == 9 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 10" );
        }
        if ( i == 10 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 11" );
        }
        if ( i == 11 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 12" );
        }
        if ( i == 12 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 13" );
        }
        if ( i == 13 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 14" );
        }
        //if (i==14 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 15");
        //if (i==15 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 16");
        //if (i==16 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 17");
        if ( i == 17 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 18" );
        }
        if ( i == 18 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 19" );
        }
        if ( i == 19 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 20" );
        }
        if ( i == 20 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 21" );
        }
        if ( i == 21 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 22" );
        }
        if ( i == 22 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 23" );
        }
        if ( i == 23 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 24" );
        }
        if ( i == 24 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 25" );
        }
        if ( i == 25 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 26" );
        }
        if ( i == 26 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 27" );
        }
        if ( i == 27 && logSQL ) {
            rodsLog( LOG_SQL, "chlSimpleQuery SQL 28" );
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
                    rodsLog( LOG_NOTICE,
                             "chlSimpleQuery cmlGetFirstRowFromSqlBV failure %d",
                             status );
                }
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
                    return ERROR( status, "cmlGetNextRowFromStatement failed" );
                }
                if ( status < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlSimpleQuery cmlGetNextRowFromStatement failure %d",
                             status );
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

        return SUCCESS();

    } // db_simple_query_op

    irods::error db_simple_query_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _sql,
        char*                  _arg1,
        char*                  _arg2,
        char*                  _arg3,
        char*                  _arg4,
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
        return db_simple_query_op_vector( _ctx, _comm, _sql, bindVars, _format, _control, _out_buf, _max_out_buf );
    }

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_del_coll_by_admin_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        collInfo_t*            _coll_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlDelCollByAdmin" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        status = splitPathByKey( _coll_info->collName,
                                 logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );

        if ( strlen( logicalParentDirName ) == 0 ) {
            snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
            snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _coll_info->collName + 1 );
        }

        /* check that the collection is empty (both subdirs and files) */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelCollByAdmin SQL 1 " );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( CAT_COLLECTION_NOT_EMPTY, "collection not empty" );
            }
            _rollback( "chlDelCollByAdmin" );
            return ERROR( status, "failed to get collection" );
        }

        /* remove any access rows */
        cllBindVars[cllBindVarCount++] = _coll_info->collName;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelCollByAdmin SQL 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_OBJT_ACCESS where object_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
                      &icss );
        if ( status != 0 ) {
            /* error, but let it fall thru to below, probably doesn't exist */
            rodsLog( LOG_NOTICE,
                     "chlDelCollByAdmin delete access failure %d",
                     status );
            _rollback( "chlDelCollByAdmin" );
        }

        /* Remove associated AVUs, if any */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelCollByAdmin SQL 3 " );
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
        removeMetaMapAndAVU( collIdNum );

        /* Audit (before it's deleted) */
        status = cmlAudit4( AU_DELETE_COLL_BY_ADMIN,
                            "select coll_id from R_COLL_MAIN where coll_name=?",
                            _coll_info->collName,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _coll_info->collName,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelCollByAdmin cmlAudit4 failure %d",
                     status );
            _rollback( "chlDelCollByAdmin" );
            return ERROR( status, "cmlAudit4 failure" );
        }


        /* delete the row if it exists */
        cllBindVars[cllBindVarCount++] = _coll_info->collName;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelCollByAdmin SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql( "delete from R_COLL_MAIN where coll_name=?",
                                         &icss );

        if ( status != 0 ) {
            char errMsg[105];
            snprintf( errMsg, 100, "collection '%s' is unknown",
                      _coll_info->collName );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            _rollback( "chlDelCollByAdmin" );
            return ERROR( CAT_UNKNOWN_COLLECTION, "unknown collection" );
        }

        return SUCCESS();

    } // db_del_coll_by_admin_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_del_coll_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        collInfo_t*            _coll_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
            rodsLog( LOG_SQL, "chlDelColl" );
        }

        status = _delColl( _comm, _coll_info );
        if ( status != 0 ) {
            return ERROR( status, "_delColl failed" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelColl cmlExecuteNoAnswerSql commit failure %d",
                     status );
            _rollback( "chlDelColl" );
            return ERROR( status, "commit failed" );
        }

        return SUCCESS();

    } // db_del_coll_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_check_auth_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        const char*            _scheme,
        char*                  _challenge,
        char*                  _response,
        char*                  _user_name,
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
        if ( !_comm || !_challenge || !_response || !_user_name || !_user_priv_level || !_client_priv_level ) {
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
        char *cp = NULL;
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
        rodsLong_t pamMinTime = 0;
        rodsLong_t pamMaxTime = 0;
        char *pSha1 = NULL;
        int hashType = 0;
        char lastPwModTs[MAX_PASSWORD_LEN + 10];
        snprintf( lastPwModTs, sizeof( lastPwModTs ), "0" );
        char *cPwTs = NULL;
        int iTs1 = 0, iTs2 = 0;
        std::vector<char> pwInfoArray( MAX_PASSWORD_LEN * MAX_PASSWORDS * 4 );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCheckAuth" );
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
        pSha1 = strstr( _user_name, SHA1_FLAG_STRING );
        if ( pSha1 != NULL ) {
            *pSha1 = '\0'; // truncate off the :::sha1 string
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
        status = parseUserName( _user_name, userName2, userZone );

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

        if ( _scheme && strlen( _scheme ) > 0 ) {
            irods::error ret = verify_auth_response( _scheme, _challenge, userName2, _response );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
            goto checkLevel;
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCheckAuth SQL 1 " );
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
                rodsLog( LOG_ERROR, "cmlGetIntegerValueFromSql failed in db_check_auth_op with status %d", status );
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
                rodsLog( LOG_ERROR, "cmlGetMultiRowStringValuesFromSql failed in db_check_auth_op with status %d", status );
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
        nowTime = atoll( myTime );

        /* Check for PAM_AUTH type passwords */
        pamMaxTime = atoll( irods_pam_password_max_time );
        pamMinTime = atoll( irods_pam_password_min_time );

        if ( ( strncmp( goodPwExpiry, "9999", 4 ) != 0 ) &&
                expireTime >=  pamMinTime &&
                expireTime <= pamMaxTime ) {
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
                    rodsLog( LOG_SQL, "chlCheckAuth SQL 2" );
                }
                status = cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where rcat_password=? and create_ts=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)", &icss );
                memset( goodPw, 0, sizeof( goodPw ) );
                memset( lastPw, 0, sizeof( lastPw ) );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlCheckAuth cmlExecuteNoAnswerSql delete expired password failure %d",
                             status );
                    return ERROR( status, "delete expired password failure" );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlCheckAuth cmlExecuteNoAnswerSql commit failure %d",
                             status );
                    return ERROR( status, "commit failure" );
                }
                return ERROR( CAT_PASSWORD_EXPIRED, "password expired" );
            }
        }


        if ( expireTime < TEMP_PASSWORD_MAX_TIME ) {

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
                rodsLog( LOG_SQL, "chlCheckAuth SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_USER_PASSWORD where rcat_password=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlCheckAuth cmlExecuteNoAnswerSql delete failure %d",
                         status );
                _rollback( "chlCheckAuth" );
                return ERROR( status, "delete failure" );
            }

            /* Also remove any expired temporary passwords */

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlCheckAuth SQL 3" );
            }
            snprintf( expireStr, sizeof expireStr, "%d", TEMP_PASSWORD_TIME );
            cllBindVars[cllBindVarCount++] = expireStr;

            pwExpireMaxCreateTime = nowTime - TEMP_PASSWORD_TIME;
            /* Not sure if casting to int is correct but seems OK & avoids warning:*/
            snprintf( expireStrCreate, sizeof expireStrCreate, "%011d",
                      ( int )pwExpireMaxCreateTime );
            cllBindVars[cllBindVarCount++] = expireStrCreate;

            status =  cmlExecuteNoAnswerSql(
                          "delete from R_USER_PASSWORD where pass_expiry_ts = ? and create_ts < ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlCheckAuth cmlExecuteNoAnswerSql delete2 failure %d",
                         status );
                _rollback( "chlCheckAuth" );
                return ERROR( status, "delete2 failed" );
            }

            memset( goodPw, 0, MAX_PASSWORD_LEN );
            if ( returnExpired ) {
                return ERROR( CAT_PASSWORD_EXPIRED, "password expired" );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlCheckAuth SQL 4" );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlCheckAuth cmlExecuteNoAnswerSql commit failure %d",
                         status );
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
            rodsLog( LOG_SQL, "chlCheckAuth SQL 5" );
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
            if ( strcmp( _comm->clientUser.userName, userName2 ) == 0 &&
                    strcmp( _comm->clientUser.rodsZone, userZone ) == 0 ) {
                *_client_priv_level = LOCAL_PRIV_USER_AUTH; /* same user, no query req */
            }
            else {
                if ( _comm->clientUser.userName[0] == '\0' ) {
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
                        rodsLog( LOG_SQL, "chlCheckAuth SQL 6" );
                    }
                    {
                        std::vector<std::string> bindVars;
                        bindVars.push_back( _comm->clientUser.userName );
                        bindVars.push_back( _comm->clientUser.rodsZone );
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

        else if ( strcmp( userType, STORAGE_ADMIN_USER_TYPE ) == 0 ) {
            /* Add a bit to the userPrivLevel to indicate that
               this user has the storageadmin role */
            *_user_priv_level = *_user_priv_level | STORAGE_ADMIN_USER;

            /* If the storageadmin is also the client, then we can just
               set the client privilege level without querying again.
               Otherwise, we query for the user to make sure they exist,
               but we don't set any privilege since storageadmin can't
               proxy all API calls. */
            if ( strcmp( _comm->clientUser.userName, userName2 ) == 0 &&
                    strcmp( _comm->clientUser.rodsZone, userZone ) == 0 ) {
                *_client_priv_level = LOCAL_USER_AUTH;
            }
            else {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlCheckAuth xSQL 8" );
                }
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( _comm->clientUser.userName );
                    bindVars.push_back( _comm->clientUser.rodsZone );
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
            }
        }

        prevFailure = 0;
        return SUCCESS();

    } // db_check_auth_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_make_temp_pw_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _pw_value_to_hash,
        char*                  _other_user ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm             ||
                !_pw_value_to_hash ||
                !_other_user ) {
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
            rodsLog( LOG_SQL, "chlMakeTempPw" );
        }

        if ( _other_user != NULL && strlen( _other_user ) > 0 ) {
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
            if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
            useOtherUser = 1;
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeTempPw SQL 1 " );
        }

        snprintf( tSQL, MAX_SQL_SIZE,
                  "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id and pass_expiry_ts != '%d'",
                  TEMP_PASSWORD_TIME );

        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
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

        /* calcuate the temp password (a hash of the user's main pw and
           the hashValue) */
        memset( md5Buf, 0, sizeof( md5Buf ) );
        snprintf( md5Buf, sizeof( md5Buf ), "%s%s", hashValue, password );

        obfMakeOneWayHash( HASH_TYPE_DEFAULT,
                           ( unsigned char * ) md5Buf, 100, ( unsigned char * ) digest );

        hashToStr( digest, newPw );
        /*   printf("newPw=%s\n", newPw); */

        rstrcpy( _pw_value_to_hash, hashValue, MAX_PASSWORD_LEN );


        /* Insert the temporary, one-time password */

        getNowStr( myTime );
        sprintf( myTimeExp, "%d", TEMP_PASSWORD_TIME );  /* seconds from create time
                                                          when it will expire */
        if ( useOtherUser == 1 ) {
            cllBindVars[cllBindVarCount++] = _other_user;
        }
        else {
            cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
        }
        cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone,
                                         cllBindVars[cllBindVarCount++] = newPw;
        cllBindVars[cllBindVarCount++] = myTimeExp;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeTempPw SQL 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlMakeTempPw cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlMakeTempPw" );
            return ERROR( status, "insert failed" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlMakeTempPw cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failed" );
        }

        memset( newPw, 0, MAX_PASSWORD_LEN );
        return SUCCESS();

    } // db_make_temp_pw_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_make_limited_pw_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
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
        if ( !_comm             ||
                !_pw_value_to_hash ) {
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
        int timeToLive;
        rodsLong_t pamMinTime;
        rodsLong_t pamMaxTime;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeLimitedPw" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeLimitedPw SQL 1 " );
        }

        snprintf( tSQL, MAX_SQL_SIZE,
                  "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id and pass_expiry_ts != '%d'",
                  TEMP_PASSWORD_TIME );

        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
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

        /* calcuate the limited password (a hash of the user's main pw and
           the hashValue) */
        memset( md5Buf, 0, sizeof( md5Buf ) );
        snprintf( md5Buf, sizeof( md5Buf ), "%s%s", hashValue, password );

        obfMakeOneWayHash( HASH_TYPE_DEFAULT,
                           ( unsigned char * ) md5Buf, 100, ( unsigned char * ) digest );

        hashToStr( digest, newPw );

        icatScramble( newPw );

        rstrcpy( _pw_value_to_hash, hashValue, MAX_PASSWORD_LEN );

        getNowStr( myTime );

        timeToLive = _ttl * 3600; /* convert input hours to seconds */
        pamMaxTime = atoll( irods_pam_password_max_time );
        pamMinTime = atoll( irods_pam_password_min_time );
        if ( timeToLive < pamMinTime ||
                timeToLive > pamMaxTime ) {
            return ERROR( PAM_AUTH_PASSWORD_INVALID_TTL, "invalid ttl" );
        }

        /* Insert the limited password */
        snprintf( expTime, sizeof expTime, "%d", timeToLive );
        cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone,
                                         cllBindVars[cllBindVarCount++] = newPw;
        cllBindVars[cllBindVarCount++] = expTime;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeLimitedPw SQL 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlMakeLimitedPw cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlMakeLimitedPw" );
            return ERROR( status, "insert failure" );
        }

        /* Also delete any that are expired */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMakeLimitedPw SQL 3" );
        }
        cllBindVars[cllBindVarCount++] = irods_pam_password_min_time;
        cllBindVars[cllBindVarCount++] = irods_pam_password_max_time;
        cllBindVars[cllBindVarCount++] = myTime;
#if MY_ICAT
        status =  cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and (cast(pass_expiry_ts as signed integer) + cast(modify_ts as signed integer) < ?)",
                                         &icss );
#else
        status =  cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and (cast(pass_expiry_ts as integer) + cast(modify_ts as integer) < ?)",
                                         &icss );
#endif

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlMakeLimitedPw cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failed" );
        }

        memset( newPw, 0, MAX_PASSWORD_LEN );

        return SUCCESS();

    } // db_make_limited_pw_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_update_pam_password_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _user_name,
        int                    _ttl,
        char*                  _test_time,
        char**                 _irods_password ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm      ||
                !_user_name ||
                !_irods_password ) {
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
        char rBuf[200];
        size_t i, j;
        char randomPw[50];
        char randomPwEncoded[50];
        int status;
        char passwordInIcat[MAX_PASSWORD_LEN + 2];
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

        /* if ttl is unset, use the default */
        if ( _ttl == 0 ) {
            rstrcpy( expTime, irods_pam_password_default_time, sizeof expTime );
        }
        else {
            /* convert ttl to seconds and make sure ttl is within the limits */
            rodsLong_t pamMinTime, pamMaxTime;
            pamMinTime = atoll( irods_pam_password_min_time );
            pamMaxTime = atoll( irods_pam_password_max_time );
            _ttl = _ttl * 3600;
            if ( _ttl < pamMinTime ||
                    _ttl > pamMaxTime ) {
                return ERROR( PAM_AUTH_PASSWORD_INVALID_TTL, "pam ttl invalid" );
            }
            snprintf( expTime, sizeof expTime, "%d", _ttl );
        }

        /* get user id */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlUpdateIrodsPamPassword SQL 1" );
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
            rodsLog( LOG_SQL, "chlUpdateIrodsPamPassword SQL 2" );
        }
        cllBindVars[cllBindVarCount++] = irods_pam_password_min_time;
        cllBindVars[cllBindVarCount++] = irods_pam_password_max_time;
        cllBindVars[cllBindVarCount++] = myTime;
#if MY_ICAT
        status =  cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and (cast(pass_expiry_ts as signed integer) + cast(modify_ts as signed integer) < ?)",
#else
        status =  cmlExecuteNoAnswerSql( "delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and (cast(pass_expiry_ts as integer) + cast(modify_ts as integer) < ?)",
#endif
                                         & icss );
        if ( logSQL != 0 ) rodsLog( LOG_SQL, "chlUpdateIrodsPamPassword SQL 3" );
        cVal[0] = passwordInIcat;
        iVal[0] = MAX_PASSWORD_LEN;
        cVal[1] = passwordModifyTime;
        iVal[1] = sizeof( passwordModifyTime );
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( selUserId );
            bindVars.push_back( irods_pam_password_min_time );
            bindVars.push_back( irods_pam_password_max_time );
            status = cmlGetStringValuesFromSql(
#if MY_ICAT
                         "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer) >= ? and cast (pass_expiry_ts as signed integer) <= ?",
#else
                         "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer) >= ? and cast (pass_expiry_ts as integer) <= ?",
#endif
                         cVal, iVal, 2, bindVars, &icss );
        }

        if ( status == 0 ) {
            if ( !irods_pam_auth_no_extend ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlUpdateIrodsPamPassword SQL 4" );
                }
                cllBindVars[cllBindVarCount++] = myTime;
                cllBindVars[cllBindVarCount++] = expTime;
                cllBindVars[cllBindVarCount++] = selUserId;
                cllBindVars[cllBindVarCount++] = passwordInIcat;
                status =  cmlExecuteNoAnswerSql( "update R_USER_PASSWORD set modify_ts=?, pass_expiry_ts=? where user_id = ? and rcat_password = ?",
                                                 &icss );
                if ( status ) {
                    return ERROR( status, "password update error" );
                }

                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure %d",
                             status );
                    return ERROR( status, "commit failure" );
                }
            } // if !irods_pam_auth_no_extend
            icatDescramble( passwordInIcat );
            strncpy( *_irods_password, passwordInIcat, irods_pam_password_len );
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // if the resultant scrambled password has a ' in the
        // string, this can cause issues on some systems, notably
        // Suse 12.  if this is the case we will just get another
        // random password.
        bool pw_good = false;
        while ( !pw_good ) {
            j = 0;
            get64RandomBytes( rBuf );
            for ( i = 0; i < 50 && j < irods_pam_password_len - 1; i++ ) {
                char c;
                c = rBuf[i] & 0x7f;
                if ( c < '0' ) {
                    c += '0';
                }
                if ( ( c > 'a' && c < 'z' ) || ( c > 'A' && c < 'Z' ) ||
                        ( c > '0' && c < '9' ) ) {
                    randomPw[j++] = c;
                }
            }
            randomPw[j] = '\0';

            snprintf( randomPwEncoded, sizeof( randomPwEncoded ), "%s", randomPw );
            icatScramble( randomPwEncoded );
            if ( !strstr( randomPwEncoded, "\'" ) ) {
                pw_good = true;

            }
            else {
                rodsLog( LOG_STATUS, "chlUpdateIrodsPamPassword :: getting a new password [%s] has a single quote", randomPwEncoded );

            }

        } // while

        if ( _test_time != NULL && strlen( _test_time ) > 0 ) {
            snprintf( myTime, sizeof( myTime ), "%s", _test_time );
        }

        if ( logSQL != 0 ) rodsLog( LOG_SQL, "chlUpdateIrodsPamPassword SQL 5" );
        cllBindVars[cllBindVarCount++] = selUserId;
        cllBindVars[cllBindVarCount++] = randomPwEncoded;
        cllBindVars[cllBindVarCount++] = expTime;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        status =  cmlExecuteNoAnswerSql( "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values (?, ?, ?, ?, ?)",
                                         &icss );
        if ( status ) return ERROR( status, "insert failure" );

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        strncpy( *_irods_password, randomPw, irods_pam_password_len );
        return SUCCESS();

    } // db_update_pam_password_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_mod_user_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _user_name,
        char*                  _option,
        char*                  _new_value ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm      ||
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

        int auditId;
        char auditComment[110];
        char auditUserName[110];
        int userSettingOwnPassword;
        int groupAdminSettingPassword; // JMC - backport 4772

        char userName2[NAME_LEN];
        char zoneName[NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModUser" );
        }

        if ( *_user_name == '\0' || *_option == '\0' || _new_value == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "parameter is empty" );
        }

        userSettingOwnPassword = 0;
        // =-=-=-=-=-=-=-
        // JMC - backport 4772
        groupAdminSettingPassword = 0;
        if ( _comm->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH && // JMC - backport 4773
                _comm->proxyUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH ) {
            /* user is OK */
        }
        else {
            /* need to check */
            if ( strcmp( _option, "password" ) != 0 ) {
                /* only password (in cases below) is allowed */
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
            if ( strcmp( _user_name, _comm->clientUser.userName ) == 0 )  {
                userSettingOwnPassword = 1;
            }
            else {
                int status2;
                status2  = cmlCheckGroupAdminAccess(
                               _comm->clientUser.userName,
                               _comm->clientUser.rodsZone,
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

        auditComment[0] = '\0';
        snprintf( auditUserName, sizeof( auditUserName ), "%s", _user_name );

        status = parseUserName( _user_name, userName2, zoneName );
        if ( zoneName[0] == '\0' ) {
            rstrcpy( zoneName, zone.c_str(), NAME_LEN );
        }
        if ( status != 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "parseUserName failed" );
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
                rodsLog( LOG_SQL, "chlModUser SQL 2" );
            }
            auditId = AU_MOD_USER_TYPE;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );
        }
        if ( strcmp( _option, "zone" ) == 0 ||
                strcmp( _option, "zone_name" ) == 0 ) {
            snprintf( tSQL, MAX_SQL_SIZE, form1, "zone_name" );
            cllBindVars[cllBindVarCount++] = _new_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModUser SQL 3" );
            }
            auditId = AU_MOD_USER_ZONE;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );
            snprintf( auditUserName, sizeof( auditUserName ), "%s", _user_name );
        }
        if ( strcmp( _option, "addAuth" ) == 0 ) {
            opType = 4;
            rstrcpy( tSQL, form5, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            cllBindVars[cllBindVarCount++] = _new_value;
            cllBindVars[cllBindVarCount++] = myTime;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModUser SQL 4" );
            }
            auditId = AU_ADD_USER_AUTH_NAME;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );
        }
        if ( strcmp( _option, "rmAuth" ) == 0 ) {
            rstrcpy( tSQL, form6, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            cllBindVars[cllBindVarCount++] = _new_value;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModUser SQL 5" );
            }
            auditId = AU_DELETE_USER_AUTH_NAME;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );

        }

        if ( strncmp( _option, "rmPamPw", 9 ) == 0 ) {
            rstrcpy( tSQL, form7, MAX_SQL_SIZE );
            cllBindVars[cllBindVarCount++] = irods_pam_password_min_time;
            cllBindVars[cllBindVarCount++] = irods_pam_password_max_time;
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneName;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModUser SQL 6" );
            }
            auditId = AU_MOD_USER_PASSWORD;
            snprintf( auditComment, sizeof( auditComment ), "%s", "Deleted user iRODS-PAM password (if any)" );
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
                rodsLog( LOG_SQL, "chlModUser SQL 6" );
            }
            auditId = AU_MOD_USER_INFO;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );
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
                rodsLog( LOG_SQL, "chlModUser SQL 7" );
            }
            auditId = AU_MOD_USER_COMMENT;
            snprintf( auditComment, sizeof( auditComment ), "%s", _new_value );
        }
        if ( strcmp( _option, "password" ) == 0 ) {
            int i;
            char userIdStr[MAX_NAME_LEN];
            i = decodePw( _comm, _new_value, decoded );
            int status2 = icatApplyRule( _comm, ( char* )"acCheckPasswordStrength", decoded );
            if ( status2 == NO_RULE_OR_MSI_FUNCTION_FOUND_ERR ) {
                int status3;
                status3 = addRErrorMsg( &_comm->rError, 0,
                                        "acCheckPasswordStrength rule not found" );
            }

            if ( status2 ) {
                return ERROR( status2, "icatApplyRule failed" );
            }

            icatScramble( decoded );

            if ( i ) {
                return ERROR( i, "password scramble failed" );
            }
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModUser SQL 8" );
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
                    rodsLog( LOG_SQL, "chlModUser SQL 9" );
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
                    rodsLog( LOG_SQL, "chlModUser SQL 10" );
                }
            }
            auditId = AU_MOD_USER_PASSWORD;
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
                    rodsLog( LOG_SQL, "chlModUser SQL 11" );
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
                    addRErrorMsg( &_comm->rError, 0, errMsg );

                    rodsLog( LOG_NOTICE,
                             "chlModUser invalid user_type" );
                    return ERROR( CAT_INVALID_USER_TYPE, "invalid user type" );
                }
            }
            if ( opType == 4 ) { /* trying to insert password or auth-name */
                /* check if user exists */
                int status2;
                _rollback( "chlModUser" );
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModUser SQL 12" );
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
                    rodsLog( LOG_NOTICE,
                             "chlModUser invalid user %s zone %s", userName2, zoneName );
                    return ERROR( CAT_INVALID_USER, "invalid user" );
                }
            }
            rodsLog( LOG_NOTICE,
                     "chlModUser cmlExecuteNoAnswerSql failure %d",
                     status );
            return ERROR( status, "get user_id failed" );
        }

        status = cmlAudit1( auditId, _comm->clientUser.userName,
                            ( char* )zone.c_str(), auditUserName, auditComment, &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModUser cmlAudit1 failure %d",
                     status );
            _rollback( "chlModUser" );
            return ERROR( status, "cmlAudit1 failed" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModUser cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failed" );
        }

        return SUCCESS();

    } // db_mod_user_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_mod_group_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _group_name,
        char*                  _option,
        char*                  _user_name,
        char*                  _user_zone ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm       ||
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
        char commentStr[100];
        char zoneToUse[MAX_NAME_LEN];

        char userName2[NAME_LEN];
        char zoneName[NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModGroup" );
        }

        if ( *_group_name == '\0' || *_option == '\0' || _user_name == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "argument is empty" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
                _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            int status2;
            status2  = cmlCheckGroupAdminAccess(
                           _comm->clientUser.userName,
                           _comm->clientUser.rodsZone, _group_name, &icss );
            if ( status2 != 0 ) {
                /* User is not a groupadmin that is a member of this group. */
                /* But if we're doing an 'add' and they are a groupadmin
                    and the group is empty, allow it */
                if ( strcmp( _option, "add" ) == 0 ) {
                    int status3 =  cmlCheckGroupAdminAccess(
                                       _comm->clientUser.userName,
                                       _comm->clientUser.rodsZone, "", &icss );
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

        status = parseUserName( _user_name, userName2, zoneName );
        if ( zoneName[0] != '\0' ) {
            rstrcpy( zoneToUse, zoneName, NAME_LEN );
        }

        userId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModGroup SQL 1 " );
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
            rodsLog( LOG_SQL, "chlModGroup SQL 2" );
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
                rodsLog( LOG_SQL, "chlModGroup SQL 3" );
            }
            cllBindVars[cllBindVarCount++] = groupId;
            cllBindVars[cllBindVarCount++] = userId;
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_USER_GROUP where group_user_id = ? and user_id = ?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModGroup cmlExecuteNoAnswerSql delete failure %d",
                         status );
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
                rodsLog( LOG_SQL, "chlModGroup SQL 4" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_USER_GROUP (group_user_id, user_id , create_ts, modify_ts) values (?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModGroup cmlExecuteNoAnswerSql add failure %d",
                         status );
                _rollback( "chlModGroup" );
                return ERROR( status, "add failure" );
            }
            OK = 1;
        }

        if ( OK == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
        }

        /* Audit */
        snprintf( commentStr, sizeof commentStr, "%s %s", _option, userId );
        status = cmlAudit3( AU_MOD_GROUP,
                            groupId,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            commentStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModGroup cmlAudit3 failure %d",
                     status );
            _rollback( "chlModGroup" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModGroup cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return SUCCESS();

    } // db_mod_group_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_mod_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _resc_name,
        char*                  _option,
        char*                  _option_value ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm       ||
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
        char myTime[50];
        char rescId[MAX_NAME_LEN];
        char rescPath[MAX_NAME_LEN] = "";
        char rescPathMsg[MAX_NAME_LEN + 100];
        char commentStr[200];
        struct hostent *myHostEnt; // JMC - backport 4597

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModResc" );
        }

        if ( *_resc_name == '\0' || *_option == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "argument is empty" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        // =-=-=-=-=-=-=-
        // JMC - backport 4629
        if ( strncmp( _resc_name, BUNDLE_RESC, strlen( BUNDLE_RESC ) ) == 0 ) {
            char errMsg[155];
            snprintf( errMsg, 150,
                      "%s is a built-in resource needed for bundle operations.",
                      BUNDLE_RESC );
            addRErrorMsg( &_comm->rError, 0, errMsg );
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
            rodsLog( LOG_SQL, "chlModResc SQL 1 " );
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

        getNowStr( myTime );
        OK = 0;

        if ( strcmp( _option, "comment" ) == 0 ) {
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 3" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set r_comment = ?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
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

        if ( strcmp( _option, "objcount" ) == 0 ) {
            int amt = atoi( _option_value );
            int ret = _updateRescObjCount(
                          &icss,
                          _resc_name,
                          zone,
                          amt );
            if ( ret != 0 ) {
                rodsLog(
                    LOG_ERROR,
                    "failed in _updateRescObjCount %d",
                    ret );
                return ERROR(
                           ret,
                           "failed in _updateRescObjCount" );
            }

            OK = 1;

        } // objcount

        if ( strcmp( _option, "info" ) == 0 ) {
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_info=?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to update info" );
            }
            OK = 1;
        }


        if ( strcmp( _option, "freespace" ) == 0 ) {
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
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 4" );
            }
            if ( inType == 0 ) {
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
            }
            if ( inType == 1 ) {
#if ORA_ICAT
                /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = cast(free_space as integer) + cast(? as integer), free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#elif MY_ICAT
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = free_space + ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#else
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = cast(free_space as bigint) + cast(? as bigint), free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#endif
            }
            if ( inType == 2 ) {
#if ORA_ICAT
                /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = cast(free_space as integer) - cast(? as integer), free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#elif MY_ICAT
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = free_space - ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#else
                status =  cmlExecuteNoAnswerSql(
                              "update R_RESC_MAIN set free_space = cast(free_space as bigint) - cast(? as bigint), free_space_ts = ?, modify_ts=? where resc_id=?",
                              &icss );
#endif
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to update freespace" );
            }
            OK = 1;
        }

        if ( strcmp( _option, "host" ) == 0 ) {
            // =-=-=-=-=-=-=-
            // JMC - backport 4597
            _resolveHostName( _comm, _option_value, myHostEnt );

            // =-=-=-=-=-=-=-
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 5" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_net = ?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set host" );
            }
            OK = 1;
        }

        if ( strcmp( _option, "type" ) == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 6" );
            }

            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 7" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_type_name = ?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set type" );
            }
            OK = 1;
        }

        if ( strcmp( _option, "path" ) == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 10" );
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( rescId );
                status = cmlGetStringValueFromSql(
                             "select resc_def_path from R_RESC_MAIN where resc_id=?",
                             rescPath, MAX_NAME_LEN, bindVars, &icss );
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlGetStringValueFromSql query failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to get path" );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 11" );
            }

            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_def_path=?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set path" );
            }
            OK = 1;
        }

        if ( strcmp( _option, "status" ) == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 12" );
            }
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_status=?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set status" );
            }
            OK = 1;
        }

        if ( strcmp( _option, "name" ) == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 13" );
            }
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            /*    If the new name is not unique, this will return an error */
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_name=?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set resc name with modify time" );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 14" );
            }
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = _resc_name;
            status =  cmlExecuteNoAnswerSql(
                          "update R_DATA_MAIN set resc_name=? where resc_name=?",
                          &icss );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status = 0;
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to change resc name" );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 15" );
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
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to update server load" );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 16" );
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
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set load digest" );
            }

            // Update resource parent strings that are _resc_name
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 17" );
            }
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = _resc_name;
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN  set resc_parent=? where resc_parent=?",
                          &icss );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status = 0;
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set parent" );
            }

            // Update resource hierarchies that contain _resc_name
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 18" );
            }
            status = _modRescInHierarchies( _resc_name, _option_value );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status = 0;
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc: _modRescInHierarchies error, status = %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to mod resc in hier" );
            }

            // Update resource children lists that contain _resc_name
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModResc SQL 19" );
            }
            status = _modRescInChildren( _resc_name, _option_value );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status = 0;
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc: _modRescInChildren error, status = %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to update children" );
            }

            // =-=-=-=-=-=-=-
            // modify the resource group for all data objects if the resource
            // in question is a root resource
            std::string resc_hier;
            status = chlGetHierarchyForResc( _option_value, zone, resc_hier );
            bool do_resc_grp = false;
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc: chlGetHierarchyForResc error, status = %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to get hierarchy for resource" );

            }

            // =-=-=-=-=-=-=-
            // check if the resc name is at the beginning of the hierarchy
            if ( resc_hier.empty() || 0 == resc_hier.find( _option_value ) ) {
                do_resc_grp = true;
            }


            if ( do_resc_grp ) {
                cllBindVars[cllBindVarCount++] = _option_value;
                cllBindVars[cllBindVarCount++] = _resc_name;
                status =  cmlExecuteNoAnswerSql(
                              "update R_DATA_MAIN set resc_group_name=? where resc_group_name=?",
                              &icss );
                if ( 0 != status && CAT_SUCCESS_BUT_WITH_NO_INFO != status ) {
                    rodsLog( LOG_NOTICE,
                             "chlModResc: failed to set group name error, status = %d",
                             status );
                    _rollback( "chlModResc" );
                    return ERROR( status, "failed to update group name" );
                }
            }

            OK = 1;

        } // if name

        if ( strcmp( _option, "context" ) == 0 ) {
            cllBindVars[cllBindVarCount++] = _option_value;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = rescId;
            status =  cmlExecuteNoAnswerSql(
                          "update R_RESC_MAIN set resc_context=?, modify_ts=? where resc_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModResc cmlExecuteNoAnswerSql update failure for resc context %d",
                         status );
                _rollback( "chlModResc" );
                return ERROR( status, "failed to set context" );
            }
            OK = 1;
        }

        if ( OK == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid option" );
        }

        /* Audit */
        snprintf( commentStr, sizeof commentStr, "%s %s", _option, _option_value );
        status = cmlAudit3( AU_MOD_RESC,
                            rescId,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            commentStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModResc cmlAudit3 failure %d",
                     status );
            _rollback( "chlModResc" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModResc cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        if ( rescPath[0] != '\0' ) {
            /* if the path was gotten, return it */

            snprintf( rescPathMsg, sizeof( rescPathMsg ), "Previous resource path: %s",
                      rescPath );
            addRErrorMsg( &_comm->rError, 0, rescPathMsg );
        }

        return SUCCESS();

    } // db_mod_resc_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_mod_resc_data_paths_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _resc_name,
        char*                  _old_path,
        char*                  _new_path,
        char*                  _user_name ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm      ||
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
        char *cptr;
        //   char userId[NAME_LEN]="";
        char userZone[NAME_LEN];
        char zoneToUse[NAME_LEN];
        char userName2[NAME_LEN];


        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRescDataPaths" );
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

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        std::string zone;
        ret = getLocalZone( _ctx.prop_map(), &icss, zone );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        rescId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRescDataPaths SQL 1 " );
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
            status = parseUserName( _user_name, userName2, userZone );
            if ( userZone[0] != '\0' ) {
                snprintf( zoneToUse, sizeof( zoneToUse ), "%s", userZone );
            }
            else {
                snprintf( zoneToUse, sizeof( zoneToUse ), "%s", zone.c_str() );
            }

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModRescDataPaths SQL 2" );
            }
            cllBindVars[cllBindVarCount++] = _old_path;
            cllBindVars[cllBindVarCount++] = _new_path;
            cllBindVars[cllBindVarCount++] = _resc_name;
            cllBindVars[cllBindVarCount++] = oldPath2;
            cllBindVars[cllBindVarCount++] = userName2;
            cllBindVars[cllBindVarCount++] = zoneToUse;
            status =  cmlExecuteNoAnswerSql(
                          "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_name=? and data_path like ? and data_owner_name=? and data_owner_zone=?",
                          &icss );
        }
        else {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModRescDataPaths SQL 3" );
            }
            cllBindVars[cllBindVarCount++] = _old_path;
            cllBindVars[cllBindVarCount++] = _new_path;
            cllBindVars[cllBindVarCount++] = _resc_name;
            cllBindVars[cllBindVarCount++] = oldPath2;
            status =  cmlExecuteNoAnswerSql(
                          "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_name=? and data_path like ?",
                          &icss );
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRescDataPaths cmlExecuteNoAnswerSql update failure %d",
                     status );
            _rollback( "chlModResc" );
            return ERROR( status, "failed to update path" );
        }

        rows = cllGetRowCount( &icss, -1 );

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModResc cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failed" );
        }

        if ( rows > 0 ) {
            char rowsMsg[100];
            snprintf( rowsMsg, 100, "%d rows updated",
                      rows );
            status = addRErrorMsg( &_comm->rError, 0, rowsMsg );
        }

        return SUCCESS();

    } // db_mod_resc_data_paths_op

    // =-=-=-=-=-=-=-
    // authenticate user
    irods::error db_mod_resc_freespace_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _resc_name,
        int                    _update_value ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm      ||
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
        char myTime[50];
        char updateValueStr[MAX_NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRescFreeSpace" );
        }

        if ( *_resc_name == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "resc name is empty" );
        }

        /* The following checks may not be needed long term, but
           shouldn't hurt, for now.
        */

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }
        if ( _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }

        getNowStr( myTime );

        snprintf( updateValueStr, MAX_NAME_LEN, "%d", _update_value );

        cllBindVars[cllBindVarCount++] = updateValueStr;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _resc_name;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModRescFreeSpace SQL 1 " );
        }
        status =  cmlExecuteNoAnswerSql(
                      "update R_RESC_MAIN set free_space = ?, free_space_ts=? where resc_name=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRescFreeSpace cmlExecuteNoAnswerSql update failure %d",
                     status );
            _rollback( "chlModRescFreeSpace" );
            return ERROR( status, "update freespace error" );
        }

        /* Audit */
        status = cmlAudit4( AU_MOD_RESC_FREE_SPACE,
                            "select resc_id from R_RESC_MAIN where resc_name=?",
                            _resc_name,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            updateValueStr,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRescFreeSpace cmlAudit4 failure %d",
                     status );
            _rollback( "chlModRescFreeSpace" );
            return ERROR( status, "cmlAudit4 failure" );
        }

        return SUCCESS();

    } // db_mod_resc_freespace_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_reg_user_re_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        userInfo_t*            _user_info ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
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
        char auditSQL[MAX_SQL_SIZE];
        char userZone[MAX_NAME_LEN];
        char zoneId[MAX_NAME_LEN];

        int zoneForm;
        char userName2[NAME_LEN];
        char zoneName[NAME_LEN];

        static char lastValidUserType[MAX_NAME_LEN] = "";
        static char userTypeTokenName[MAX_NAME_LEN] = "";

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegUserRE" );
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
        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
                _comm->proxyUser.authInfo.authFlag  < LOCAL_PRIV_USER_AUTH ) {
            int status2;
            status2  = cmlCheckGroupAdminAccess(
                           _comm->clientUser.userName,
                           _comm->clientUser.rodsZone,
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
                rodsLog( LOG_SQL, "chlRegUserRE SQL 1 " );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
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

        status = parseUserName( _user_info->userName, userName2, zoneName );
        if ( zoneName[0] != '\0' ) {
            snprintf( userZone, sizeof( userZone ), "%s", zoneName );
            zoneForm = 2;
        }
        if ( status != 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid argument" );
        }

        if ( zoneForm ) {
            /* check that the zone exists (if not defaulting to local) */
            zoneId[0] = '\0';
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRegUserRE SQL 5 " );
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
                    addRErrorMsg( &_comm->rError, 0, errMsg );
                    return ERROR( CAT_INVALID_ZONE, "invalid zone name" );
                }
                return ERROR( status, "get zone id failure" );
            }
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegUserRE SQL 2" );
        }
        status = cmlGetNextSeqStr( seqStr, MAX_NAME_LEN, &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE, "chlRegUserRE cmlGetNextSeqStr failure %d",
                     status );
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
            rodsLog( LOG_SQL, "chlRegUserRE SQL 3" );
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
                addRErrorMsg( &_comm->rError, 0, errMsg );
            }
            _rollback( "chlRegUserRE" );
            rodsLog( LOG_NOTICE,
                     "chlRegUserRE insert failure %d", status );
            return ERROR( status, "insert failure" );
        }


        cllBindVars[cllBindVarCount++] = seqStr;
        cllBindVars[cllBindVarCount++] = seqStr;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegUserRE SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_USER_GROUP (group_user_id, user_id, create_ts, modify_ts) values (?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegUserRE insert into R_USER_GROUP failure %d", status );
            _rollback( "chlRegUserRE" );
            return ERROR( status, "insert into r_user_group failure" );
        }


        /*
          The case where the caller is specifying an authstring is used in
          some specialized cases.  Using the new table (Aug 12, 2009), this
          is now set via the chlModUser call below.  This is untested, though.
        */
        if ( strlen( _user_info->authInfo.authStr ) > 0 ) {
            status = chlModUser( _comm, _user_info->userName, "addAuth",
                                 _user_info->authInfo.authStr );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRegUserRE chlModUser insert auth failure %d", status );
                _rollback( "chlRegUserRE" );
                return ERROR( status, "insert auth failure" );
            }
        }

        /* Audit */
        snprintf( auditSQL, MAX_SQL_SIZE - 1,
                  "select user_id from R_USER_MAIN where user_name=? and zone_name='%s'",
                  userZone );
        status = cmlAudit4( AU_REGISTER_USER_RE,
                            auditSQL,
                            userName2,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            userZone,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegUserRE cmlAudit4 failure %d",
                     status );
            _rollback( "chlRegUserRE" );
            return ERROR( status, "cmlAudit4 failure" );
        }

        return CODE( status );

    } // db_reg_user_re_op

    // =-=-=-=-=-=-=-
    // commit the transaction
    irods::error db_set_avu_metadata_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _type,
        char*                  _name,
        char*                  _attribute,
        char*                  _new_value,
        char*                  _new_unit ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type   ||
                !_name   ||
                !_attribute ) {
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
            rodsLog( LOG_SQL, "chlSetAVUMetadata" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetAVUMetadata SQL 1 " );
        }
        objId = checkAndGetObjectId( _comm, _ctx.prop_map(), _type, _name, ACCESS_CREATE_METADATA );
        if ( objId < 0 ) {
            return ERROR( objId, "checkAndGetObjectId failed" );
        }
        snprintf( objIdStr, MAX_NAME_LEN, "%lld", objId );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetAVUMetadata SQL 2" );
        }

        /* Treat unspecified unit as empty string */
        if ( _new_unit == NULL ) {
            _new_unit = "";
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
                /* Need to add the metadata */
                status = chlAddAVUMetadata( _comm, 0, _type, _name, _attribute,
                                            _new_value, _new_unit );
            }
            else {
                rodsLog( LOG_NOTICE,
                         "chlSetAVUMetadata cmlGetMultiRowStringValuesFromSql failure %d",
                         status );
            }
            return ERROR( status, "get avu failed" );
        }

        if ( status > 1 ) {
            /* Cannot update AVU in-place, need to do a delete with wildcards then add */
            status = chlDeleteAVUMetadata( _comm, 1, _type, _name, _attribute, "%",
                                           "%", 1 );
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
                status = chlDeleteAVUMetadata( _comm, 1, _type, _name, _attribute, "%",
                                               "%", 1 );
            }
            if ( status != 0 ) {
                _rollback( "chlSetAVUMetadata" );
                return ERROR( status, "delete avu metadata failed" );
            }
            status = chlAddAVUMetadata( _comm, 0, _type, _name, _attribute,
                                        _new_value, _new_unit );
            return ERROR( status, "delete avu metadata failed" );
        }

        /* Only one metaId for this Attribute and Object has been found, and the metaID is not shared */
        rodsLog( LOG_NOTICE, "chlSetAVUMetadata found metaId %s", metaIdStr );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetAVUMetadata SQL 4" );
        }

        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = _new_value;
        cllBindVars[cllBindVarCount++] = _new_unit;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = metaIdStr;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetAVUMetadata SQL 5" );
        }
        status = cmlExecuteNoAnswerSql( "update R_META_MAIN set meta_attr_value=?,meta_attr_unit=?,modify_ts=? where meta_id=?",
                                        &icss );

        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlSetAVUMetadata cmlExecuteNoAnswerSql update failure %d",
                     status );
            _rollback( "chlSetAVUMetadata" );
            return ERROR( status, "set avu failed" );
        }

        /* Audit */
        status = cmlAudit3( AU_ADD_AVU_METADATA,
                            objIdStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _type,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlSetAVUMetadata cmlAudit3 failure %d",
                     status );
            _rollback( "chlSetAVUMetadata" );
            return ERROR( status, "cmlAudit3 failed" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlSetAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                     status );
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
        rsComm_t*              _comm,
        int                    _admin_mode,
        char*                  _type,
        char*                  _name,
        char*                  _attribute,
        char*                  _value,
        char*                  _units ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type   ||
                !_name   ||
                !_attribute ) {
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
        rodsLong_t status, status2;
        rodsLong_t seqNum;
        int numObjects;
        int nAccess = 0;
        static int accessNeeded = ACCESS_MAX;
        rodsLong_t iVal;
        char collection[MAX_NAME_LEN];
        char objectName[MAX_NAME_LEN];
        char myTime[50];
        char seqNumStr[MAX_NAME_LEN];
        int itype;

        itype = convertTypeOption( _type );
        if ( itype != 1 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid type" );    /* only -d for now */
        }

        status = splitPathByKey( _name, collection, MAX_NAME_LEN, objectName, MAX_NAME_LEN, '/' );
        if ( strlen( collection ) == 0 ) {
            snprintf( collection, sizeof( collection ), "%s", PATH_SEPARATOR );
            snprintf( objectName, sizeof( objectName ), "%s", _name );
        }

        /*
          The following SQL is somewhat complicated, but evaluates the access
          permissions in steps to reduce the complexity and so it can scale
          well.  Altho there are multiple SQL calls, including creating two
          views, the scaling burden is placed on the DBMS, so it should
          perform well even for many thousands of objects at a time.
        */

        /* Get the count of the objects to compare with later */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 1" );
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objectName );
            bindVars.push_back( collection );
            status = cmlGetIntegerValueFromSql(
                         "select count(DISTINCT DM.data_id) from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ?",
                         &iVal, bindVars, &icss );
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild get count failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "select failure" );
        }
        numObjects = iVal;
        if ( numObjects == 0 ) {
            return ERROR( CAT_NO_ROWS_FOUND, "no objects found" );
        }

        /*
           Create a view with all the access permissions for this user, or
           groups this user is a member of, for all the matching data-objects.
        */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 2" );
        }
#if ORA_ICAT
        /* For Oracle, we cannot use views with bind-variables, so use a
           table instead. */
        status =  cmlExecuteNoAnswerSql( "purge recyclebin",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (drop table ACCESS_VIEW_ONE) failure %d",
                     status );
        }
        status =  cmlExecuteNoAnswerSql( "drop table ACCESS_VIEW_ONE",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (drop table ACCESS_VIEW_ONE) failure %d",
                     status );
        }

        status =  cmlExecuteNoAnswerSql( "create table ACCESS_VIEW_ONE (access_type_id integer, data_id integer)",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (create table ACCESS_VIEW_ONE) failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "(create table ACCESS_VIEW_ONE) failure" );
        }

        cllBindVars[cllBindVarCount++] = objectName;
        cllBindVars[cllBindVarCount++] = collection;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone;
        status =  cmlExecuteNoAnswerSql(
                      "insert into ACCESS_VIEW_ONE (access_type_id, data_id) (select access_type_id, DM.data_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id)",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_NO_ACCESS_PERMISSION;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "create view failure" );
        }
#else
        cllBindVars[cllBindVarCount++] = objectName;
        cllBindVars[cllBindVarCount++] = collection;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.userName;
        cllBindVars[cllBindVarCount++] = _comm->clientUser.rodsZone;
        status =  cmlExecuteNoAnswerSql(
                      "create view ACCESS_VIEW_ONE as select access_type_id, DM.data_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_NO_ACCESS_PERMISSION;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "create view failure" );
        }
#endif

        /* Create another view for min below (sub select didn't work).  This
           is the set of access permisions per matching data-object, the best
           permision values (for example, if user has write and has
           group-based read, this will be 'write').
        */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 3" );
        }
#if (defined ORA_ICAT || defined MY_ICAT)
        status =  cmlExecuteNoAnswerSql(
                      "create or replace view ACCESS_VIEW_TWO as select max(access_type_id) max from ACCESS_VIEW_ONE group by data_id",
                      &icss );
#else
        status =  cmlExecuteNoAnswerSql(
                      "create or replace view ACCESS_VIEW_TWO as select max(access_type_id) from ACCESS_VIEW_ONE group by data_id",
                      &icss );
#endif
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_NO_ACCESS_PERMISSION;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "(create view) failure" );
        }

        if ( accessNeeded >= ACCESS_MAX ) { /* not initialized yet */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 4" );
            }
            {
                std::vector<std::string> bindVars;
                status = cmlGetIntegerValueFromSql(
                             "select token_id  from R_TOKN_MAIN where token_name = 'modify metadata' and token_namespace = 'access_type'",
                             &iVal, bindVars, &icss );
            }
            if ( status == 0 ) {
                accessNeeded = iVal;
            }
        }

        /* Get the minimum access permissions for the whole set of
         * data-objects that match */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 5" );
        }
        iVal = -1;
        {
            std::vector<std::string> bindVars;
            status = cmlGetIntegerValueFromSql(
                         "select min(max) from ACCESS_VIEW_TWO",
                         &iVal, bindVars, &icss );
        }

        if ( status == CAT_NO_ROWS_FOUND ) {
            status = CAT_NO_ACCESS_PERMISSION;
        }

        if ( status == 0 ) {
            if ( iVal < accessNeeded ) {
                status = CAT_NO_ACCESS_PERMISSION;
            }
        }

        /* Get the count of the access permissions for the set of
         * data-objects, since if there are completely missing access
         * permissions (NULL) they won't show up in the above query */
        if ( status == 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 6" );
            }
            {
                std::vector<std::string> bindVars;
                status = cmlGetIntegerValueFromSql(
                             "select count(*) from ACCESS_VIEW_TWO",
                             &iVal, bindVars, &icss );
            }
            if ( status == 0 ) {
                nAccess = iVal;

                if ( numObjects > nAccess ) {
                    status = CAT_NO_ACCESS_PERMISSION;
                }
            }
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 7" );
        }
#if ORA_ICAT
        status2 =  cmlExecuteNoAnswerSql(
                       "drop table ACCESS_VIEW_ONE",
                       &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
        if ( status2 == 0 ) {
            status2 =  cmlExecuteNoAnswerSql(
                           "drop view ACCESS_VIEW_TWO",
                           &icss );
            if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status2 = 0;
            }
        }
#else
        status2 =  cmlExecuteNoAnswerSql(
                       "drop view ACCESS_VIEW_TWO, ACCESS_VIEW_ONE",
                       &icss );
        if ( status2 == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status2 = 0;
        }
#endif

        if ( status2 != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild cmlExecuteNoAnswerSql (drop view (or table)) failure %d",
                     status2 );
        }

        if ( status != 0 ) {
            return ERROR( status, "(drop view (or table)) failure" );
        }

        /*
           Now the easy part, set up the AVU and associate it with the data-objects
        */
        status = findOrInsertAVU( _attribute, _value, _units );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild findOrInsertAVU failure %d",
                     status );
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
            rodsLog( LOG_SQL, "chlAddAVUMetadataWild SQL 8" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) select DM.data_id, ?, ?, ? from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? group by DM.data_id",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "insert failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_ADD_AVU_WILD_METADATA,
                            seqNumStr,  /* for WILD, record the AVU id */
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _name,       /* and the input wildcard path */
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild cmlAudit3 failure %d",
                     status );
            _rollback( "chlAddAVUMetadataWild" );
            return ERROR( status, "cmlAudit3 failure" );
        }


        /* Commit */
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadataWild cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return CODE( numObjects );

    } // db_add_avu_metadata_wild_op

    irods::error db_add_avu_metadata_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        int                    _admin_mode,
        char*                  _type,
        char*                  _name,
        char*                  _attribute,
        char*                  _value,
        char*                  _units ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type   ||
                !_name   ||
                !_attribute ) {
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
        rodsLong_t seqNum, iVal;
        rodsLong_t objId, status;
        char objIdStr[MAX_NAME_LEN];
        char seqNumStr[MAX_NAME_LEN];
        char userName[NAME_LEN];
        char userZone[NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddAVUMetadata" );
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

        if ( _admin_mode == 1 ) {
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }
        }

        if ( _units == NULL ) {
            _units = "";
        }

        itype = convertTypeOption( _type );
        if ( itype == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "invalid type arguement" );
        }

        if ( itype == 1 ) {
            status = splitPathByKey( _name,
                                     logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
            if ( strlen( logicalParentDirName ) == 0 ) {
                snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
                snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _name );
            }
            if ( _admin_mode == 1 ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 1 " );
                }
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( logicalEndName );
                    bindVars.push_back( logicalParentDirName );
                    status = cmlGetIntegerValueFromSql(
                                 "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where "
                                 "DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
                                 &iVal, bindVars, &icss );
                }
                if ( status == 0 ) {
                    status = iVal;    /*like cmlCheckDataObjOnly, status is objid */
                }
            }
            else {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 2" );
                }
                status = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                              _comm->clientUser.userName,
                                              _comm->clientUser.rodsZone,
                                              ACCESS_CREATE_METADATA, &icss );
            }
            if ( status < 0 ) {
                _rollback( "chlAddAVUMetadata" );
                return ERROR( status, "select data_id failed" );
            }
            objId = status;
        }

        if ( itype == 2 ) {
            if ( _admin_mode == 1 ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 3" );
                }
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( _name );
                    status = cmlGetIntegerValueFromSql(
                                 "select coll_id from R_COLL_MAIN where coll_name=?",
                                 &iVal, bindVars, &icss );
                }
                if ( status == 0 ) {
                    status = iVal;    /*like cmlCheckDir, status is objid*/
                }
            }
            else {
                /* Check that the collection exists and user has create_metadata
                   permission, and get the collectionID */
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 4" );
                }
                status = cmlCheckDir( _name,
                                      _comm->clientUser.userName,
                                      _comm->clientUser.rodsZone,
                                      ACCESS_CREATE_METADATA, &icss );
            }
            if ( status < 0 ) {
                char errMsg[105];
                _rollback( "chlAddAVUMetadata" );
                if ( status == CAT_UNKNOWN_COLLECTION ) {
                    snprintf( errMsg, 100, "collection '%s' is unknown",
                              _name );
                    addRErrorMsg( &_comm->rError, 0, errMsg );
                }
                else {
                    _rollback( "chlAddAVUMetadata" );
                }
                return ERROR( status, "cmlCheckDir failed" );
            }
            objId = status;
        }

        if ( itype == 3 ) {
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }

            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            objId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 5" );
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
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }

            status = parseUserName( _name, userName, userZone );
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
                rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 6" );
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
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL , "insufficient privilege" );
            }

            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            objId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 7" );
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
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata findOrInsertAVU failure %d",
                     status );
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
            rodsLog( LOG_SQL, "chlAddAVUMetadata SQL 7" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) values (?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlAddAVUMetadata" );
            return ERROR( status, "insert failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_ADD_AVU_METADATA,
                            objIdStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _type,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlAudit3 failure %d",
                     status );
            _rollback( "chlAddAVUMetadata" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return CODE( status );

    } // db_add_avu_metadata_op

    irods::error db_mod_avu_metadata_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _type,
        char*                  _name,
        char*                  _attribute,
        char*                  _value,
        char*                  _unitsOrArg0,
        char*                  _arg1,
        char*                  _arg2,
        char*                  _arg3 ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type   ||
                !_name   ||
                !_attribute ) {
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
        int status, atype;
        char myUnits[MAX_NAME_LEN] = "";
        char *addAttr = "", *addValue = "", *addUnits = "";
        int newUnits = 0;
        if ( _unitsOrArg0 == NULL || *_unitsOrArg0 == '\0' ) {
            return ERROR( CAT_INVALID_ARGUMENT, "unitsOrArg0 empty or null" );
        }
        atype = checkModArgType( _unitsOrArg0 );
        if ( atype == 0 ) {
            snprintf( myUnits, sizeof( myUnits ), "%s", _unitsOrArg0 );
        }

        status = chlDeleteAVUMetadata( _comm, 0, _type, _name, _attribute, _value,
                                       myUnits, 1 );
        if ( status != 0 ) {
            _rollback( "chlModAVUMetadata" );
            return ERROR( status, "delete avu metadata failed" );
        }

        if ( atype == 1 ) {
            addAttr = _unitsOrArg0 + 2;
        }
        if ( atype == 2 ) {
            addValue = _unitsOrArg0 + 2;
        }
        if ( atype == 3 ) {
            addUnits = _unitsOrArg0 + 2;
        }

        atype = checkModArgType( _arg1 );
        if ( atype == 1 ) {
            addAttr = _arg1 + 2;
        }
        if ( atype == 2 ) {
            addValue = _arg1 + 2;
        }
        if ( atype == 3 ) {
            addUnits = _arg1 + 2;
        }

        atype = checkModArgType( _arg2 );
        if ( atype == 1 ) {
            addAttr = _arg2 + 2;
        }
        if ( atype == 2 ) {
            addValue = _arg2 + 2;
        }
        if ( atype == 3 ) {
            addUnits = _arg2 + 2;
        }

        atype = checkModArgType( _arg3 );
        if ( atype == 1 ) {
            addAttr = _arg3 + 2;
        }
        if ( atype == 2 ) {
            addValue = _arg3 + 2;
        }
        if ( atype == 3 ) {
            addUnits = _arg3 + 2;
            newUnits = 1;
        }

        if ( *addAttr  == '\0' &&
                *addValue == '\0' &&
                *addUnits == '\0' ) {
            _rollback( "chlModAVUMetadata" );
            return ERROR( CAT_INVALID_ARGUMENT, "arg check failed" );
        }

        if ( *addAttr == '\0' ) {
            addAttr = _attribute;
        }
        if ( *addValue == '\0' ) {
            addValue = _value;
        }
        if ( *addUnits == '\0' && newUnits == 0 ) {
            addUnits = myUnits;
        }

        status = chlAddAVUMetadata( _comm, 0, _type, _name, addAttr, addValue,
                                    addUnits );
        return CODE( status );

    } // db_mod_avu_metadata_op

    irods::error db_del_avu_metadata_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        int                    _option,
        char*                  _type,
        char*                  _name,
        char*                  _attribute,
        char*                  _value,
        char*                  _unit,
        int                    _nocommit ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type   ||
                !_name   ||
                !_attribute ) {
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
            rodsLog( LOG_SQL, "chlDeleteAVUMetadata" );
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
            status = splitPathByKey( _name,
                                     logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
            if ( strlen( logicalParentDirName ) == 0 ) {
                snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
                snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _name );
            }
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 1 " );
            }
            status = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                          _comm->clientUser.userName,
                                          _comm->clientUser.rodsZone,
                                          ACCESS_DELETE_METADATA, &icss );
            if ( status < 0 ) {
                if ( _nocommit != 1 ) {
                    _rollback( "chlDeleteAVUMetadata" );
                }
                return ERROR( status, "delete avu failed" );
            }
            objId = status;
        }

        if ( itype == 2 ) {
            /* Check that the collection exists and user has delete_metadata permission,
               and get the collectionID */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 2" );
            }
            status = cmlCheckDir( _name,
                                  _comm->clientUser.userName,
                                  _comm->clientUser.rodsZone,
                                  ACCESS_DELETE_METADATA, &icss );
            if ( status < 0 ) {
                char errMsg[105];
                if ( status == CAT_UNKNOWN_COLLECTION ) {
                    snprintf( errMsg, 100, "collection '%s' is unknown",
                              _name );
                    addRErrorMsg( &_comm->rError, 0, errMsg );
                }
                return ERROR( status, "cmlCheckDir failed" );
            }
            objId = status;
        }

        if ( itype == 3 ) {
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }

            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            objId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 3" );
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
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }

            status = parseUserName( _name, userName, userZone );
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
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 4" );
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
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
            }

            std::string zone;
            ret = getLocalZone( _ctx.prop_map(), &icss, zone );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            objId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 5" );
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
                rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 9" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_METAMAP where object_id=? and meta_id =?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure %d",
                         status );
                if ( _nocommit != 1 ) {
                    _rollback( "chlDeleteAVUMetadata" );
                }

                return ERROR( status, "delete failure" );
            }

            /* Remove unused AVU rows, if any */
#ifdef METADATA_CLEANUP
            removeAVUs();
#endif

            /* Audit */
            status = cmlAudit3( AU_DELETE_AVU_METADATA,
                                objIdStr,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                _type,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlDeleteAVUMetadata cmlAudit3 failure %d",
                         status );
                if ( _nocommit != 1 ) {
                    _rollback( "chlDeleteAVUMetadata" );
                }

                return ERROR( status, "cmlAudit3 failure" );
            }

            if ( _nocommit != 1 ) {
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                             status );
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
                    rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 5" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and (meta_attr_unit like ? or meta_attr_unit IS NULL) )",
                              &icss );
            }
            else {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 6" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and (meta_attr_unit = ? or meta_attr_unit IS NULL) )",
                              &icss );
            }
        }
        else {
            if ( _option == 1 ) { /* use wildcards ('like') */
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 7" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and meta_attr_unit like ?)",
                              &icss );
            }
            else {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlDeleteAVUMetadata SQL 8" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and meta_attr_unit = ?)",
                              &icss );
            }
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure %d",
                     status );
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }
            return ERROR( status, "delete failure" );
        }

        /* Remove unused AVU rows, if any */
#ifdef METADATA_CLEANUP
        removeAVUs();
#endif

        /* Audit */
        status = cmlAudit3( AU_DELETE_AVU_METADATA,
                            objIdStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _type,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDeleteAVUMetadata cmlAudit3 failure %d",
                     status );
            if ( _nocommit != 1 ) {
                _rollback( "chlDeleteAVUMetadata" );
            }
            return ERROR( status, "cmlAudit3 failure" );
        }

        if ( _nocommit != 1 ) {
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                         status );
                return ERROR( status, "comit failure" );
            }
        }

        return CODE( status );

    } // db_del_avu_metadata_op

    irods::error db_copy_avu_metadata_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _type1,
        char*                  _type2,
        char*                  _name1,
        char*                  _name2 ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm   ||
                !_type1  ||
                !_type2  ||
                !_name1  ||
                !_name2 ) {
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
        int status;
        rodsLong_t objId1, objId2;
        char objIdStr1[MAX_NAME_LEN];
        char objIdStr2[MAX_NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCopyAVUMetadata" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCopyAVUMetadata SQL 1 " );
        }
        objId1 = checkAndGetObjectId( _comm, _ctx.prop_map(), _type1, _name1, ACCESS_READ_METADATA );
        if ( objId1 < 0 ) {
            return ERROR( objId1, "checkAndGetObjectId failure" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCopyAVUMetadata SQL 2" );
        }
        objId2 = checkAndGetObjectId( _comm, _ctx.prop_map(), _type2, _name2, ACCESS_CREATE_METADATA );

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
            rodsLog( LOG_SQL, "chlCopyAVUMetadata SQL 3" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) select ?, meta_id, ?, ? from R_OBJT_METAMAP where object_id=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlCopyAVUMetadata cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlCopyAVUMetadata" );
            return ERROR( status, "insert failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_COPY_AVU_METADATA,
                            objIdStr1,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            objIdStr2,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlCopyAVUMetadata cmlAudit3 failure %d",
                     status );
            _rollback( "chlCopyAVUMetadata" );
            return ERROR( status, "cmlAudit3 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlCopyAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return CODE( status );

    } // db_copy_avu_metadata_op

    irods::error db_mod_access_control_resc_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        int                    _recursive_flag,
        char*                  _access_level,
        char*                  _user_name,
        char*                  _zone,
        char*                  _resc_name ) {
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_INVALID_ARGUMENT, "invalid argument" );
        }

        if ( _comm->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH ) {
            /* admin, so just get the resc_id */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModAccessControlResc SQL 1" );
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
                                   _comm->clientUser.userName,
                                   _comm->clientUser.rodsZone,
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
            rodsLog( LOG_SQL, "chlModAccessControlResc SQL 2" );
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
            rodsLog( LOG_SQL, "chlModAccessControlResc SQL 3" );
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
                rodsLog( LOG_SQL, "chlModAccessControlResc SQL 4" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                          &icss );
            if ( status != 0 ) {
                _rollback( "chlModAccessControlResc" );
                return ERROR( status, "insert failure" );
            }
        }

        /* Audit */
        status = cmlAudit5( AU_MOD_ACCESS_CONTROL_RESOURCE,
                            rescIdStr,
                            userIdStr,
                            myAccessLev,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModAccessControlResc cmlAudit5 failure %d",
                     status );
            _rollback( "chlModAccessControlResc" );
            return ERROR( status, "cmlAudit5 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status < 0 ) {
            return ERROR( status, "commit failure" );
        }

        return SUCCESS();

    } // db_mod_access_control_resc_op

    irods::error db_mod_access_control_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        int                    _recursive_flag,
        char*                  _access_level,
        char*                  _user_name,
        char*                  _zone,
        char*                  _path_name ) {
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        char *myAccessLev = NULL;
        char logicalEndName[MAX_NAME_LEN];
        char logicalParentDirName[MAX_NAME_LEN];
        char collIdStr[MAX_NAME_LEN];
        rodsLong_t objId = 0;
        rodsLong_t status = 0, status1 = 0, status2 = 0, status3 = 0;
        int rmFlag = 0;
        rodsLong_t userId = 0;
        char myTime[50];
        const char *myZone = 0;
        char userIdStr[MAX_NAME_LEN];
        char objIdStr[MAX_NAME_LEN];
        int inheritFlag = 0;
        char myAccessStr[LONG_NAME_LEN];
        int adminMode = 0;
        rodsLong_t iVal = 0;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModAccessControl" );
        }

        if ( strncmp( _access_level, MOD_RESC_PREFIX, strlen( MOD_RESC_PREFIX ) ) == 0 ) {
            ret = db_mod_access_control_resc_op(
                      _ctx,
                      _comm,
                      _recursive_flag,
                      _access_level,
                      _user_name,
                      _zone,
                      _path_name );
            return PASS( ret );
        }

        adminMode = 0;
        if ( strncmp( _access_level, MOD_ADMIN_MODE_PREFIX,
                      strlen( MOD_ADMIN_MODE_PREFIX ) ) == 0 ) {
            if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
                addRErrorMsg( &_comm->rError, 0,
                              "You must be the admin to use the -M admin mode" );
                return ERROR( CAT_NO_ACCESS_PERMISSION, "You must be the admin to use the -M admin mode" );
            }
            snprintf( myAccessStr, sizeof( myAccessStr ), "%s", _access_level + strlen( MOD_ADMIN_MODE_PREFIX ) );
            _access_level = myAccessStr;
            adminMode = 1;
        }

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
        else if ( strcmp( _access_level, AP_OWN ) == 0 ) {
            myAccessLev = ACCESS_OWN;
        }
        else if ( strcmp( _access_level, ACCESS_INHERIT ) == 0 ) {
            inheritFlag = 1;
        }
        else if ( strcmp( _access_level, ACCESS_NO_INHERIT ) == 0 ) {
            inheritFlag = 2;
        }
        else {
            char errMsg[105];
            snprintf( errMsg, 100, "access level '%s' is invalid",
                      _access_level );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_INVALID_ARGUMENT, errMsg );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        if ( adminMode ) {
            /* See if the input path is a collection
               and, if so, get the collectionID */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModAccessControl SQL 14" );
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( _path_name );
                status1 = cmlGetIntegerValueFromSql(
                              "select coll_id from R_COLL_MAIN where coll_name=?",
                              &iVal, bindVars, &icss );
            }
            if ( status1 == CAT_NO_ROWS_FOUND ) {
                status1 = CAT_UNKNOWN_COLLECTION;
            }
            if ( status1 == 0 ) {
                status1 = iVal;
            }
        }
        else {
            /* See if the input path is a collection and the user owns it,
               and, if so, get the collectionID */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModAccessControl SQL 1 " );
            }
            status1 = cmlCheckDir( _path_name,
                                   _comm->clientUser.userName,
                                   _comm->clientUser.rodsZone,
                                   ACCESS_OWN,
                                   &icss );
        }
        if ( status1 >= 0 ) {
            snprintf( collIdStr, MAX_NAME_LEN, "%lld", status1 );
        }

        if ( status1 < 0 && inheritFlag != 0 ) {
            char errMsg[105];
            snprintf( errMsg, 100, "either the collection does not exist or you do not have sufficient access" );
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_NO_ACCESS_PERMISSION, errMsg );
        }

        /* Not a collection (with access for non-Admin) */
        if ( status1 < 0 ) {
            status2 = splitPathByKey( _path_name,
                                      logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
            if ( strlen( logicalParentDirName ) == 0 ) {
                snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
                snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _path_name + 1 );
            }
            if ( adminMode ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModAccessControl SQL 15" );
                }
                {
                    std::vector<std::string> bindVars;
                    bindVars.push_back( logicalEndName );
                    bindVars.push_back( logicalParentDirName );
                    status2 = cmlGetIntegerValueFromSql(
                                  "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
                                  &iVal, bindVars, &icss );
                }
                if ( status2 == CAT_NO_ROWS_FOUND ) {
                    status2 = CAT_UNKNOWN_FILE;
                }
                if ( status2 == 0 ) {
                    status2 = iVal;
                }
            }
            else {
                /* Not a collection with access, so see if the input path dataObj
                   exists and the user owns it, and, if so, get the objectID */
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModAccessControl SQL 2" );
                }
                status2 = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                               _comm->clientUser.userName,
                                               _comm->clientUser.rodsZone,
                                               ACCESS_OWN, &icss );
            }
            if ( status2 > 0 ) {
                objId = status2;
            }
        }

        /* If both failed, it doesn't exist or there's no permission */
        if ( status1 < 0 && status2 < 0 ) {
            char errMsg[205];

            if ( status1 == CAT_UNKNOWN_COLLECTION && status2 == CAT_UNKNOWN_FILE ) {
                snprintf( errMsg, 200,
                          "Input path is not a collection and not a dataObj: %s",
                          _path_name );
                addRErrorMsg( &_comm->rError, 0, errMsg );
                return ERROR( CAT_INVALID_ARGUMENT, "unknown collection or file" );
            }
            if ( status1 != CAT_UNKNOWN_COLLECTION ) {
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModAccessControl SQL 12" );
                }
                status3 = cmlCheckDirOwn( _path_name,
                                          _comm->clientUser.userName,
                                          _comm->clientUser.rodsZone,
                                          &icss );
                if ( status3 < 0 ) {
                    return ERROR( status1, "cmlCheckDirOwn failed" );
                }
                snprintf( collIdStr, MAX_NAME_LEN, "%lld", status3 );
            }
            else {
                if ( status2 == CAT_NO_ACCESS_PERMISSION ) {
                    /* See if this user is the owner (with no access, but still
                       allowed to ichmod) */
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModAccessControl SQL 13" );
                    }
                    status3 = cmlCheckDataObjOwn( logicalParentDirName, logicalEndName,
                                                  _comm->clientUser.userName,
                                                  _comm->clientUser.rodsZone,
                                                  &icss );
                    if ( status3 < 0 ) {
                        _rollback( "chlModAccessControl" );
                        return ERROR( status2, "cmlCheckDataObjOwn failed" );
                    }
                    objId = status3;
                }
                else {
                    return ERROR( status2, "cmlCheckDataObjOnly failed" );
                }
            }
        }

        /* Doing inheritance */
        if ( inheritFlag != 0 ) {
            status = _modInheritance( inheritFlag, _recursive_flag, collIdStr, _path_name );
            return ERROR( status, "_modInheritance failed" );
        }

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
            rodsLog( LOG_SQL, "chlModAccessControl SQL 3" );
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
        snprintf( objIdStr, MAX_NAME_LEN, "%lld", objId );

        rodsLog( LOG_NOTICE, "recursiveFlag %d", _recursive_flag );

        /* non-Recursive mode */
        if ( _recursive_flag == 0 ) {

            /* doing a dataObj */
            if ( objId ) {
                cllBindVars[cllBindVarCount++] = userIdStr;
                cllBindVars[cllBindVarCount++] = objIdStr;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModAccessControl SQL 4" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                              &icss );
                if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return ERROR( status, "delete failure" );
                }
                if ( rmFlag == 0 ) { /* if not just removing: */
                    getNowStr( myTime );
                    cllBindVars[cllBindVarCount++] = objIdStr;
                    cllBindVars[cllBindVarCount++] = userIdStr;
                    cllBindVars[cllBindVarCount++] = myAccessLev;
                    cllBindVars[cllBindVarCount++] = myTime;
                    cllBindVars[cllBindVarCount++] = myTime;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModAccessControl SQL 5" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                                  &icss );
                    if ( status != 0 ) {
                        _rollback( "chlModAccessControl" );
                        return ERROR( status, "insert failure" );
                    }
                }

                /* Audit */
                status = cmlAudit5( AU_MOD_ACCESS_CONTROL_OBJ,
                                    objIdStr,
                                    userIdStr,
                                    myAccessLev,
                                    &icss );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModAccessControl cmlAudit5 failure %d",
                             status );
                    _rollback( "chlModAccessControl" );
                    return ERROR( status, "cmlAudit5 failure" );
                }

                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return ERROR( status, "commit failiure" );
            }

            /* doing a collection, non-recursive */
            cllBindVars[cllBindVarCount++] = userIdStr;
            cllBindVars[cllBindVarCount++] = collIdStr;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModAccessControl SQL 6" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                _rollback( "chlModAccessControl" );
                return ERROR( status, "delete failure" );
            }
            if ( rmFlag ) { /* just removing */
                /* Audit */
                status = cmlAudit5( AU_MOD_ACCESS_CONTROL_COLL,
                                    collIdStr,
                                    userIdStr,
                                    myAccessLev,
                                    &icss );
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModAccessControl cmlAudit5 failure %d",
                             status );
                    _rollback( "chlModAccessControl" );
                    return ERROR( status, "cmlAudit5 failure" );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return ERROR( status, "commit failure" );
            }

            getNowStr( myTime );
            cllBindVars[cllBindVarCount++] = collIdStr;
            cllBindVars[cllBindVarCount++] = userIdStr;
            cllBindVars[cllBindVarCount++] = myAccessLev;
            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = myTime;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModAccessControl SQL 7" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                          &icss );

            if ( status != 0 ) {
                _rollback( "chlModAccessControl" );
                return ERROR( status, "insert failure" );
            }
            /* Audit */
            status = cmlAudit5( AU_MOD_ACCESS_CONTROL_COLL,
                                collIdStr,
                                userIdStr,
                                myAccessLev,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModAccessControl cmlAudit5 failure %d",
                         status );
                _rollback( "chlModAccessControl" );
                return ERROR( status, "cmlAudit5 failure" );
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return ERROR( CAT_INVALID_ARGUMENT, errMsg );
        }


        char escapedPath[MAX_NAME_LEN * 2];
        makeEscapedPath( _path_name, escapedPath, sizeof( escapedPath ) );
        char pathStart[MAX_NAME_LEN * 2];
        snprintf( pathStart, sizeof( pathStart ), "%s/%%", escapedPath );

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
           rodsLog(LOG_SQL, ...) calls (so 'devtest' will verify it is
           called), but since the later postgres SQL depends on this table,
           we can be sure this is exercised if "chlModAccessControl SQL 8" is.
        */
        status =  cmlExecuteNoAnswerSql( "create temporary table R_MOD_ACCESS_TEMP1 (coll_id bigint not null, coll_name varchar(2700) not null) on commit drop",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModAccessControl cmlExecuteNoAnswerSql create temp table failure %d",
                     status );
            _rollback( "chlModAccessControl" );
            return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql create temp table failure" );
        }

        status =  cmlExecuteNoAnswerSql( "create unique index idx_r_mod_access_temp1 on R_MOD_ACCESS_TEMP1 (coll_name)",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModAccessControl cmlExecuteNoAnswerSql create index failure %d",
                     status );
            _rollback( "chlModAccessControl" );
            return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql create index failure" );
        }

        cllBindVars[cllBindVarCount++] = _path_name;
        cllBindVars[cllBindVarCount++] = pathStart;
        status =  cmlExecuteNoAnswerSql( "insert into R_MOD_ACCESS_TEMP1 (coll_id, coll_name) select  coll_id, coll_name from R_COLL_MAIN where coll_name = ? or coll_name like ?",
                                         &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModAccessControl cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlModAccessControl" );
            return ERROR( status, "chlModAccessControl cmlExecuteNoAnswerSql insert failure" );
        }
#endif

        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = _path_name;
        cllBindVars[cllBindVarCount++] = pathStart;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModAccessControl SQL 8" );
        }
        status =  cmlExecuteNoAnswerSql(
#if (defined ORA_ICAT || defined MY_ICAT)
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
        cllBindVars[cllBindVarCount++] = pathStart;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModAccessControl SQL 9" );
        }
        status =  cmlExecuteNoAnswerSql(
#if (defined ORA_ICAT || defined MY_ICAT)
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

            /* Audit */
            status = cmlAudit5( AU_MOD_ACCESS_CONTROL_COLL_RECURSIVE,
                                collIdStr,
                                userIdStr,
                                myAccessLev,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModAccessControl cmlAudit5 failure %d",
                         status );
                _rollback( "chlModAccessControl" );
                return ERROR( status, "cmlAudit5 failure" );
            }

            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return ERROR( status, "commit failure" );
        }

        getNowStr( myTime );
        makeEscapedPath( _path_name, pathStart, sizeof( pathStart ) );
        strncat( pathStart, "/%", sizeof( pathStart ) );
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = myAccessLev;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _path_name;
        cllBindVars[cllBindVarCount++] = pathStart;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModAccessControl SQL 10" );
        }
#if ORA_ICAT
        /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
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
        cllBindVars[cllBindVarCount++] = pathStart;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModAccessControl SQL 11" );
        }
#if ORA_ICAT
        /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
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

        /* Audit */
        status = cmlAudit5( AU_MOD_ACCESS_CONTROL_COLL_RECURSIVE,
                            collIdStr,
                            userIdStr,
                            myAccessLev,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModAccessControl cmlAudit5 failure %d",
                     status );
            _rollback( "chlModAccessControl" );
            return ERROR( status, "cmlAudit5 failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status < 0 ) {
            return ERROR( status, "commit failed" );
        }

        return CODE( status );

    } // db_mod_access_control_op

    irods::error db_rename_object_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        rodsLong_t             _obj_id,
        char*                  _new_name ) {
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
            rodsLog( LOG_SQL, "chlRenameObject" );
        }

        if ( strstr( _new_name, PATH_SEPARATOR ) ) {
            return ERROR( CAT_INVALID_ARGUMENT, "new name invalid" );
        }

        /* See if it's a dataObj and if so get the coll_id
           check the access permission at the same time */
        collId = 0;

        snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameObject SQL 1 " );
        }

        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
            status = cmlGetIntegerValueFromSql(
                         "select coll_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                         &collId, bindVars,  &icss );
        }

        if ( status == 0 ) { /* it is a dataObj and user has access to it */

            /* check that no other dataObj exists with this name in this collection*/
            snprintf( collIdString, MAX_NAME_LEN, "%lld", collId );
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRenameObject SQL 2" );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 3" );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 4" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_DATA_MAIN set data_name = ? where data_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlExecuteNoAnswerSql update1 failure %d",
                         status );
                _rollback( "chlRenameObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update1 failure" );
            }

            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = collIdString;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRenameObject SQL 5" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set modify_ts=? where coll_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlExecuteNoAnswerSql update2 failure %d",
                         status );
                _rollback( "chlRenameObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update2 failure" );
            }

            /* Audit */
            status = cmlAudit3( AU_RENAME_DATA_OBJ,
                                objIdString,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                _new_name,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlAudit3 failure %d",
                         status );
                _rollback( "chlRenameObject" );
                return ERROR( status, "cmlAudit3 failure" );
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
            rodsLog( LOG_SQL, "chlRenameObject SQL 6" );
        }

        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
            status = cmlGetStringValuesFromSql(
                         "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                         cVal, iVal, 2, bindVars, &icss );
        }
        if ( status == 0 ) {
            /* it is a collection and user has access to it */

            /* check that no other dataObj exists with this name in this collection*/
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlRenameObject SQL 7" );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 8" );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 9" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set coll_name = substr(coll_name,1,?) || ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                         status );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 10" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set parent_coll_name = substr(parent_coll_name,1,?) || ? || substr(parent_coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                         status );
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
                rodsLog( LOG_SQL, "chlRenameObject SQL 11" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set coll_name=?, modify_ts=? where coll_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlRenameObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
            }

            /* Audit */
            status = cmlAudit3( AU_RENAME_COLLECTION,
                                objIdString,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                _new_name,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlRenameObject cmlAudit3 failure %d",
                         status );
                _rollback( "chlRenameObject" );
                return ERROR( status, "cmlAudit3 failure" );
            }

            return CODE( status );

        }


        /* Both collection and dataObj failed, go thru the sql in smaller
           steps to return a specific error */

        snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRenameObject SQL 12" );
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
            rodsLog( LOG_SQL, "chlRenameObject SQL 12" );
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
        rsComm_t*              _comm,
        rodsLong_t             _obj_id,
        rodsLong_t             _target_coll_id ) {
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
            rodsLog( LOG_SQL, "chlMoveObject" );
        }

        /* check that the target collection exists and user has write
           permission, and get the names while at it */
        cVal[0] = parentTargetCollName;
        iVal[0] = MAX_NAME_LEN;
        cVal[1] = targetCollName;
        iVal[1] = MAX_NAME_LEN;
        snprintf( objIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMoveObject SQL 1 " );
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
            status = cmlGetStringValuesFromSql(
                         "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'modify object'",
                         cVal, iVal, 2, bindVars, &icss );

        }
        snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
        if ( status != 0 ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlMoveObject SQL 2" );
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
            return ERROR( CAT_UNKNOWN_COLLECTION, "target isnt a collection" );      /* isn't a coll */
        }


        /* See if we're moving a dataObj and if so get the data_name;
           and at the same time check the access permission */
        snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMoveObject SQL 3" );
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
            status = cmlGetStringValueFromSql(
                         "select data_name from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
                         dataObjName, MAX_NAME_LEN, bindVars, &icss );
        }
        snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
        if ( status == 0 ) { /* it is a dataObj and user has access to it */

            /* check that no other dataObj exists with the ObjName in the
               target collection */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlMoveObject SQL 4" );
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
                rodsLog( LOG_SQL, "chlMoveObject SQL 5" );
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
                rodsLog( LOG_SQL, "chlMoveObject SQL 6" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_DATA_MAIN set coll_id=?, modify_ts=? where data_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlExecuteNoAnswerSql update1 failure %d",
                         status );
                _rollback( "chlMoveObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update1 failure" );
            }


            cllBindVars[cllBindVarCount++] = myTime;
            cllBindVars[cllBindVarCount++] = collIdString;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlMoveObject SQL 7" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set modify_ts=? where coll_id=?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlExecuteNoAnswerSql update2 failure %d",
                         status );
                _rollback( "chlMoveObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update2 failure" );
            }

            /* Audit */
            status = cmlAudit3( AU_MOVE_DATA_OBJ,
                                objIdString,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                collIdString,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlAudit3 failure %d",
                         status );
                _rollback( "chlMoveObject" );
                return ERROR( status, "cmlAudit3 failure" );
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
            rodsLog( LOG_SQL, "chlMoveObject SQL 8" );
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( objIdString );
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
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
                rodsLog( LOG_SQL, "chlMoveObject SQL 9" );
            }
            status = cmlCheckDir( parentCollName,  _comm->clientUser.userName,
                                  _comm->clientUser.rodsZone,
                                  ACCESS_MODIFY_OBJECT, &icss );
            if ( status < 0 ) {
                return ERROR( status, "cmlCheckDir failed" );
            }

            /* check that no other dataObj exists with the ObjName in the
               target collection */
            snprintf( collIdString, MAX_NAME_LEN, "%lld", _target_coll_id );
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlMoveObject SQL 10" );
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
            strncat( newCollName, PATH_SEPARATOR, MAX_NAME_LEN );
            strncat( newCollName, endCollName, MAX_NAME_LEN );

            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlMoveObject SQL 11" );
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
                rodsLog( LOG_SQL, "chlMoveObject SQL 12" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set coll_name = ?, parent_coll_name=?, modify_ts=? where coll_id = ?",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlExecuteNoAnswerSql update failure %d",
                         status );
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
                rodsLog( LOG_SQL, "chlMoveObject SQL 13" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "update R_COLL_MAIN set parent_coll_name = ? || substr(parent_coll_name, ?), coll_name = ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name = ?",
                          &icss );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                status = 0;
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlExecuteNoAnswerSql update failure %d",
                         status );
                _rollback( "chlMoveObject" );
                return ERROR( status, "cmlExecuteNoAnswerSql update failure" );
            }

            /* Audit */
            status = cmlAudit3( AU_MOVE_COLL,
                                objIdString,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone,
                                targetCollName,
                                &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlMoveObject cmlAudit3 failure %d",
                         status );
                _rollback( "chlMoveObject" );
                return ERROR( status, "cmlAudit3 failure" );
            }

            return CODE( status );
        }


        /* Both collection and dataObj failed, go thru the sql in smaller
           steps to return a specific error */
        snprintf( objIdString, MAX_NAME_LEN, "%lld", _obj_id );
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlMoveObject SQL 14" );
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
            rodsLog( LOG_SQL, "chlMoveObject SQL 15" );
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
        rsComm_t*              _comm,
        char*                  _name_space,
        char*                  _name,
        char*                  _value,
        char*                  _value2,
        char*                  _value3,
        char*                  _comment ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        rodsLong_t objId;
        char *myValue1, *myValue2, *myValue3, *myComment;
        char myTime[50];
        rodsLong_t seqNum;
        char errMsg[205];
        char seqNumStr[MAX_NAME_LEN];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegToken" );
        }

        if ( _name_space == NULL || strlen( _name_space ) == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "namespace null or 0 len" );
        }
        if ( _name == NULL || strlen( _name ) == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "name null or 0 len" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegToken SQL 1 " );
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return  ERROR( CAT_INVALID_ARGUMENT, "namespace does not exist" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegToken SQL 2" );
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
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
            rodsLog( LOG_SQL, "chlRegToken SQL 3" );
        }
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlRegToken cmlGetNextSeqVal failure %d",
                     seqNum );
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
            rodsLog( LOG_SQL, "chlRegToken SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_TOKN_MAIN values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            _rollback( "chlRegToken" );
            return ERROR( status, "insert failure" );
        }

        /* Audit */
        status = cmlAudit3( AU_REG_TOKEN,
                            seqNumStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _name,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegToken cmlAudit3 failure %d",
                     status );
            _rollback( "chlRegToken" );
            return ERROR( status, "cmlAudit3 failure" );
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
        rsComm_t*              _comm,
        char*                  _name_space,
        char*                  _name ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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


        int status;
        rodsLong_t objId;
        char errMsg[205];
        char objIdStr[60];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelToken" );
        }

        if ( _name_space == NULL || strlen( _name_space ) == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "namespace is null or 0 len" );
        }
        if ( _name == NULL || strlen( _name ) == 0 ) {
            return ERROR( CAT_INVALID_ARGUMENT, "name is null or 0 len" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelToken SQL 1 " );
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
            addRErrorMsg( &_comm->rError, 0, errMsg );
            return  ERROR( CAT_INVALID_ARGUMENT, "token is not in namespace" );
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelToken SQL 2" );
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

        /* Audit */
        snprintf( objIdStr, sizeof objIdStr, "%lld", objId );
        status = cmlAudit3( AU_DEL_TOKEN,
                            objIdStr,
                            _comm->clientUser.userName,
                            _comm->clientUser.rodsZone,
                            _name,
                            &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelToken cmlAudit3 failure %d",
                     status );
            _rollback( "chlDelToken" );
            return ERROR( status, "cmlAudit3 failure" );
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
        rsComm_t*              _comm,
        char*                  _host_name,
        char*                  _resc_name,
        char*                  _cpu_used,
        char*                  _mem_used,
        char*                  _swap_used,
        char*                  _run_q_load,
        char*                  _disk_space,
        char*                  _net_input,
        char*                  _net_output ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        char myTime[50];
        int status;
        int i;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegServerLoad" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlRegServerLoad SQL 1" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_SERVER_LOAD (host_name, resc_name, cpu_used, mem_used, swap_used, runq_load, disk_space, net_input, net_output, create_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegServerLoad cmlExecuteNoAnswerSql failure %d", status );
            _rollback( "chlRegServerLoad" );
            return ERROR( status, "cmlExecuteNoAnswerSql failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegServerLoad cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return SUCCESS();

    } // db_reg_server_load_op

    irods::error db_purge_server_load_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _seconds_ago ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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

        // =-=-=-=-=-=-=-
        // delete from R_LOAD_SERVER where (%i -exe_time) > %i
        int status;
        char nowStr[50];
        static char thenStr[50];
        time_t nowTime;
        time_t thenTime;
        time_t secondsAgoTime;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlPurgeServerLoad" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        getNowStr( nowStr );
        nowTime = atoll( nowStr );
        secondsAgoTime = atoll( _seconds_ago );
        thenTime = nowTime - secondsAgoTime;
        snprintf( thenStr, sizeof thenStr, "%011d", ( uint ) thenTime );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlPurgeServerLoad SQL 1" );
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
        rsComm_t*              _comm,
        char*                  _resc_name,
        char*                  _load_factor ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        char myTime[50];
        int status;
        int i;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlRegServerLoadDigest" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlRegServerLoadDigest SQL 1" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_SERVER_LOAD_DIGEST (resc_name, load_factor, create_ts) values (?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegServerLoadDigest cmlExecuteNoAnswerSql failure %d", status );
            _rollback( "chlRegServerLoadDigest" );
            return ERROR( status, "cmlExecuteNoAnswerSql failure" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlRegServerLoadDigest cmlExecuteNoAnswerSql commit failure %d",
                     status );
            return ERROR( status, "commit failure" );
        }

        return SUCCESS();

    } // db_reg_server_load_digest_op

    irods::error db_purge_server_load_digest_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _seconds_ago ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        // =-=-=-=-=-=-=-
        /* delete from R_SERVER_LOAD_DIGEST where (%i -exe_time) > %i */
        int status;
        char nowStr[50];
        static char thenStr[50];
        time_t nowTime;
        time_t thenTime;
        time_t secondsAgoTime;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlPurgeServerLoadDigest" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        getNowStr( nowStr );
        nowTime = atoll( nowStr );
        secondsAgoTime = atoll( _seconds_ago );
        thenTime = nowTime - secondsAgoTime;
        snprintf( thenStr, sizeof thenStr, "%011d", ( uint ) thenTime );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlPurgeServerLoadDigest SQL 1" );
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

    irods::error db_calc_usage_and_quota_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        char myTime[50];

        status = 0;
        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        rodsLog( LOG_NOTICE,
                 "chlCalcUsageAndQuota called" );


        getNowStr( myTime );

        /* Delete the old rows from R_QUOTA_USAGE */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCalcUsageAndQuota SQL 1" );
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
            rodsLog( LOG_SQL, "chlCalcUsageAndQuota SQL 2" );
        }
        cllBindVars[cllBindVarCount++] = myTime;
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_QUOTA_USAGE (quota_usage, resc_id, user_id, modify_ts) (select sum(R_DATA_MAIN.data_size), R_RESC_MAIN.resc_id, R_USER_MAIN.user_id, ? from R_DATA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_name = R_DATA_MAIN.data_owner_name and R_USER_MAIN.zone_name = R_DATA_MAIN.data_owner_zone and R_RESC_MAIN.resc_name = R_DATA_MAIN.resc_name group by R_RESC_MAIN.resc_id, user_id)",
                      &icss );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            status = 0;    /* no files, OK */
        }
        if ( status != 0 ) {
            _rollback( "chlCalcUsageAndQuota" );
            return ERROR( status, "insert failed" );
        }

        /* Set the over_quota flags where appropriate */
        status = setOverQuota( _comm );
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
        rsComm_t*              _comm,
        char*                  _type,
        char*                  _name,
        char*                  _resc_name,
        char*                  _limit ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
                rodsLog( LOG_SQL, "chlSetQuota SQL 1" );
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


        status = parseUserName( _name, userName, userZone );
        if ( userZone[0] == '\0' ) {
            snprintf( userZone, sizeof( userZone ), "%s", zone.c_str() );
        }

        if ( itype == 1 ) {
            userId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlSetQuota SQL 2" );
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( userName );
                bindVars.push_back( userZone );
                status = cmlGetIntegerValueFromSql(
                             "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
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
        else {
            userId = 0;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlSetQuota SQL 3" );
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
            rodsLog( LOG_SQL, "chlSetQuota SQL 4" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_QUOTA_MAIN where user_id=? and resc_id=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_DEBUG,
                     "chlSetQuota cmlExecuteNoAnswerSql delete failure %d",
                     status );
        }
        if ( atol( _limit ) > 0 ) {
            getNowStr( myTime );
            cllBindVars[cllBindVarCount++] = userIdStr;
            cllBindVars[cllBindVarCount++] = rescIdStr;
            cllBindVars[cllBindVarCount++] = _limit;
            cllBindVars[cllBindVarCount++] = myTime;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlSetQuota SQL 5" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_QUOTA_MAIN (user_id, resc_id, quota_limit, modify_ts) values (?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlSetQuota cmlExecuteNoAnswerSql insert failure %d",
                         status );
                _rollback( "chlSetQuota" );
                return ERROR( status, "cmlExecuteNoAnswerSql insert failure" );
            }
        }

        /* Reset the over_quota flags based on previous usage info.  The
           usage info may take a while to set, but setting the OverQuota
           should be quick.  */
        status = setOverQuota( _comm );
        if ( status != 0 ) {
            _rollback( "chlSetQuota" );
            return ERROR( status, "setOverQuota failed" );
        }

        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        if ( status < 0 ) {
            return ERROR( status, "commit failure" );
        }
        else {
            return SUCCESS();
        }

    } // db_set_quota_op

    irods::error db_check_quota_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _user_name,
        char*                  _resc_name,
        rodsLong_t*            _user_quota,
        int*                   _quota_status ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        /*
           Check on a user's quota status, returning the most-over or
           nearest-over value.

           A single query is done which gets the four possible types of quotas
           for this user on this resource (and ordered so the first row is the
           result).  The types of quotas are: user per-resource, user global,
           group per-resource, and group global.
        */
        int status;
        int statementNum;

        char mySQL[] = "select distinct QM.user_id, QM.resc_id, QM.quota_limit, QM.quota_over from R_QUOTA_MAIN QM, R_USER_MAIN UM, R_RESC_MAIN RM, R_USER_GROUP UG, R_USER_MAIN UM2 where ( (QM.user_id = UM.user_id and UM.user_name = ?) or (QM.user_id = UG.group_user_id and UM2.user_name = ? and UG.user_id = UM2.user_id) ) and ((QM.resc_id = RM.resc_id and RM.resc_name = ?) or QM.resc_id = '0') order by quota_over desc";

        *_user_quota = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlCheckQuota SQL 1" );
        }
        cllBindVars[cllBindVarCount++] = _user_name;
        cllBindVars[cllBindVarCount++] = _user_name;
        cllBindVars[cllBindVarCount++] = _resc_name;

        status = cmlGetFirstRowFromSql( mySQL, &statementNum,
                                        0, &icss );

        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlCheckQuota - CAT_SUCCESS_BUT_WITH_NO_INFO" );
            *_quota_status = QUOTA_UNRESTRICTED;
            return SUCCESS();
        }

        if ( status == CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "chlCheckQuota - CAT_NO_ROWS_FOUND" );
            *_quota_status = QUOTA_UNRESTRICTED;
            return SUCCESS();
        }

        if ( status != 0 ) {
            return ERROR( status, "check quota failed" );
        }

        /* For now, log it */
        rodsLog( LOG_NOTICE, "checkQuota: inUser:%s inResc:%s RescId:%s Quota:%s",
                 _user_name, _resc_name,
                 icss.stmtPtr[statementNum]->resultValue[1],  /* resc_id column */
                 icss.stmtPtr[statementNum]->resultValue[3] ); /* quota_over column */

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

    irods::error db_del_unused_avus_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        /*
           Remove any AVUs that are currently not associated with any object.
           This is done as a separate operation for efficiency.  See
           'iadmin h rum'.
        */
        int status;
        status = removeAVUs();
        if ( status < 0 ) {
            return ERROR( status, "removeAVUs failed" );
        }

        if ( status == 0 ) {
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return SUCCESS();
        }
        else {
            return ERROR( status, "commit failed" );
        }

    } // db_del_unused_avus_op

    irods::error db_ins_rule_table_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _map_priority_str,
        char*                  _rule_name,
        char*                  _rule_head,
        char*                  _rule_condition,
        char*                  _rule_action,
        char*                  _rule_recovery,
        char*                  _rule_id_str,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        int i;
        rodsLong_t seqNum = -1;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsRuleTable" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        /* first check if the  rule already exists */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsRuleTable SQL 1" );
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
            rodsLog( LOG_NOTICE,
                     "chlInsRuleTable cmlGetIntegerValueFromSqlV3 find rule if any failure %d", status );
            return ERROR( status, "cmlGetIntegerValueFromSqlV3 find rule if any failure" );
        }
        if ( seqNum < 0 ) {
            seqNum = cmlGetNextSeqVal( &icss );
            if ( seqNum < 0 ) {
                rodsLog( LOG_NOTICE, "chlInsRuleTable cmlGetNextSeqVal failure %d",
                         seqNum );
                _rollback( "chlInsRuleTable" );
                return ERROR( seqNum, "cmlGetNextSeqVal failure" );
            }
            snprintf( _rule_id_str, MAX_NAME_LEN, "%lld", seqNum );

            i = 0;
            cllBindVars[i++] = _rule_id_str;
            cllBindVars[i++] = _base_name;
            cllBindVars[i++] = _rule_name;
            cllBindVars[i++] = _rule_head;
            cllBindVars[i++] = _rule_condition;
            cllBindVars[i++] = _rule_action;
            cllBindVars[i++] = _rule_recovery;
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsRuleTable SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_RULE_MAIN(rule_id, rule_base_name, rule_name, rule_event, rule_condition, rule_body, rule_recovery, rule_owner_name, rule_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsRuleTable cmlExecuteNoAnswerSql Rule Main Insert failure %d", status );
                return ERROR( status, "cmlExecuteNoAnswerSql Rule Main Insert failure" );
            }
        }
        else {
            snprintf( _rule_id_str, MAX_NAME_LEN, "%lld", seqNum );
        }
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsRuleTable SQL 3" );
        }
        i = 0;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = _map_priority_str;
        cllBindVars[i++] = _rule_id_str;
        cllBindVars[i++] = _comm->clientUser.userName;
        cllBindVars[i++] = _comm->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_BASE_MAP  (map_base_name, map_priority, rule_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlInsRuleTable cmlExecuteNoAnswerSql Rule Map insert failure %d" , status );

            return ERROR( status, "cmlExecuteNoAnswerSql Rule Map insert failure" );
        }

        return SUCCESS();

    } // db_ins_rule_table_op

    irods::error db_ins_dvm_table_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _var_name,
        char*                  _action,
        char*                  _var_2_cmap,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        int i;
        rodsLong_t seqNum = -1;
        char dvmIdStr[MAX_NAME_LEN];
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsDvmTable" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not conncted" );
        }

        /* first check if the DVM already exists */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsDvmTable SQL 1" );
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
            rodsLog( LOG_NOTICE,
                     "chlInsDvmTable cmlGetIntegerValueFromSqlV3 find DVM if any failure %d", status );
            return ERROR( status, "find DVM if any failure" );
        }
        if ( seqNum < 0 ) {
            seqNum = cmlGetNextSeqVal( &icss );
            if ( seqNum < 0 ) {
                rodsLog( LOG_NOTICE, "chlInsDvmTable cmlGetNextSeqVal failure %d",
                         seqNum );
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
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsDvmTable SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_RULE_DVM(dvm_id, dvm_base_name, dvm_ext_var_name, dvm_condition, dvm_int_map_path, dvm_owner_name, dvm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsDvmTable cmlExecuteNoAnswerSql DVM Main Insert failure %d", status );
                return ERROR( status, "cmlExecuteNoAnswerSql DVM Main Insert failure" );
            }
        }
        else {
            snprintf( dvmIdStr, MAX_NAME_LEN, "%lld", seqNum );
        }
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsDvmTable SQL 3" );
        }
        i = 0;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = dvmIdStr;
        cllBindVars[i++] = _comm->clientUser.userName;
        cllBindVars[i++] = _comm->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_DVM_MAP  (map_dvm_base_name, dvm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlInsDvmTable cmlExecuteNoAnswerSql DVM Map insert failure %d" , status );

            return ERROR( status, "DVM Map insert failure" );
        }

        return SUCCESS();

    } // db_ins_dvm_table_op

    irods::error db_ins_fnm_table_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _func_name,
        char*                  _func_2_cmap,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        int i;
        rodsLong_t seqNum = -1;
        char fnmIdStr[MAX_NAME_LEN];
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsFnmTable" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        /* first check if the FNM already exists */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsFnmTable SQL 1" );
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
            rodsLog( LOG_NOTICE,
                     "chlInsFnmTable cmlGetIntegerValueFromSqlV3 find FNM if any failure %d", status );
            return ERROR( status, "find FNM if any failure" );
        }
        if ( seqNum < 0 ) {
            seqNum = cmlGetNextSeqVal( &icss );
            if ( seqNum < 0 ) {
                rodsLog( LOG_NOTICE, "chlInsFnmTable cmlGetNextSeqVal failure %d",
                         seqNum );
                _rollback( "chlInsFnmTable" );
                return ERROR( seqNum, "cmlGetNextSeqVal failure" );
            }
            snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );

            i = 0;
            cllBindVars[i++] = fnmIdStr;
            cllBindVars[i++] = _base_name;
            cllBindVars[i++] = _func_name;
            cllBindVars[i++] = _func_2_cmap;
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsFnmTable SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_RULE_FNM(fnm_id, fnm_base_name, fnm_ext_func_name, fnm_int_func_name, fnm_owner_name, fnm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsFnmTable cmlExecuteNoAnswerSql FNM Main Insert failure %d", status );
                return ERROR( status, "FNM Map insert failure" );
            }
        }
        else {
            snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );
        }
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsFnmTable SQL 3" );
        }
        i = 0;
        cllBindVars[i++] = _base_name;
        cllBindVars[i++] = fnmIdStr;
        cllBindVars[i++] = _comm->clientUser.userName;
        cllBindVars[i++] = _comm->clientUser.rodsZone;
        cllBindVars[i++] = _my_time;
        cllBindVars[i++] = _my_time;
        cllBindVarCount = i;
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RULE_FNM_MAP  (map_fnm_base_name, fnm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlInsFnmTable cmlExecuteNoAnswerSql FNM Map insert failure %d" , status );

            return ERROR( status, "FNM Map insert failure" );
        }

        return SUCCESS();

    } // db_ins_fnm_table_op

    irods::error db_ins_msrvc_table_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _module_name,
        char*                  _msrvc_name,
        char*                  _msrvc_signature,
        char*                  _msrvc_version,
        char*                  _msrvc_host,
        char*                  _msrvc_location,
        char*                  _msrvc_language,
        char*                  _msrvc_type_name,
        char*                  _msrvc_status,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status;
        int i;
        rodsLong_t seqNum = -1;
        char msrvcIdStr[MAX_NAME_LEN];
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsMsrvcTable" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        /* first check if the MSRVC already exists */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 1" );
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
            rodsLog( LOG_NOTICE,
                     "chlInsMsrvcTable cmlGetIntegerValueFromSqlV3 find MSRVC if any failure %d", status );
            return ERROR( status, "cmlGetIntegerValueFromSqlV3 find MSRVC if any failure" );
        }
        if ( seqNum < 0 ) { /* No micro-service found */
            seqNum = cmlGetNextSeqVal( &icss );
            if ( seqNum < 0 ) {
                rodsLog( LOG_NOTICE, "chlInsMsrvcTable cmlGetNextSeqVal failure %d",
                         seqNum );
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
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_MICROSRVC_MAIN(msrvc_id, msrvc_name, msrvc_module_name, msrvc_signature, msrvc_doxygen, msrvc_variations, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?,   'NONE', 'NONE',  ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_MAIN Insert failure %d", status );
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
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 3" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_status, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure %d", status );
                return ERROR( status, "R_MICROSRVC_VER Insert failure" );
            }
        }
        else { /* micro-service already there */
            snprintf( msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum );
            /* Check if same host and location exists - if so no need to insert a new row */
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 4" );
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
                rodsLog( LOG_NOTICE,
                         "chlInsMsrvcTable cmlGetIntegerValueFromSqlV4 find MSRVC_HOST if any failure %d", status );
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
            cllBindVars[i++] = _comm->clientUser.userName;
            cllBindVars[i++] = _comm->clientUser.rodsZone;
            cllBindVars[i++] = _my_time;
            cllBindVars[i++] = _my_time;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 3" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?)",
                          &icss );
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure %d", status );
                return ERROR( status, "cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure" );
            }
        }

        return SUCCESS();

    } // db_ins_msrvc_table_op

    irods::error db_version_rule_base_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int i, status;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlVersionRuleBase" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlVersionRuleBase SQL 1" );
        }

        status =  cmlExecuteNoAnswerSql(
                      "update R_RULE_BASE_MAP set map_version = ?, modify_ts = ? where map_base_name = ? and map_version = '0'", &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlVersionRuleBase cmlExecuteNoAnswerSql Rule Map version update  failure %d" , status );
            return ERROR( status, "Rule Map version update failure" );

        }

        return SUCCESS();

    } // db_version_rule_base_op

    irods::error db_version_dvm_base_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int i, status;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlVersionDvmBase" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlVersionDvmBase SQL 1" );
        }

        status =  cmlExecuteNoAnswerSql(
                      "update R_RULE_DVM_MAP set map_dvm_version = ?, modify_ts = ? where map_dvm_base_name = ? and map_dvm_version = '0'", &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlVersionDvmBase cmlExecuteNoAnswerSql DVM Map version update  failure %d" , status );
            return ERROR( status, "DVM Map version update  failure" );

        }

        return SUCCESS();

    } // db_version_dvm_base_op

    irods::error db_version_fnm_base_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _base_name,
        char*                  _my_time ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int i, status;

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlVersionFnmBase" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
            rodsLog( LOG_SQL, "chlVersionFnmBase SQL 1" );
        }

        status =  cmlExecuteNoAnswerSql(
                      "update R_RULE_FNM_MAP set map_fnm_version = ?, modify_ts = ? where map_fnm_base_name = ? and map_fnm_version = '0'", &icss );
        if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            rodsLog( LOG_NOTICE,
                     "chlVersionFnmBase cmlExecuteNoAnswerSql FNM Map version update  failure %d" , status );
            return ERROR( status, "FNM Map version update  failure" );

        }

        return SUCCESS();

    } // db_version_fnm_base_op

    irods::error db_add_specific_query_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _sql,
        char*                  _alias ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status, i;
        char myTime[50];
        char tsCreateTime[50];
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddSpecificQuery" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
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
                rodsLog( LOG_SQL, "chlAddSpecificQuery SQL 1" );
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( _alias );
                status = cmlGetStringValueFromSql(
                             "select create_ts from R_SPECIFIC_QUERY where alias=?",
                             tsCreateTime, 50, bindVars, &icss );
            }
            if ( status == 0 ) {
                i = addRErrorMsg( &_comm->rError, 0, "Alias is not unique" );
                return ERROR( CAT_INVALID_ARGUMENT, "alias is not unique" );
            }
            i = 0;
            cllBindVars[i++] = _sql;
            cllBindVars[i++] = _alias;
            cllBindVars[i++] = myTime;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlAddSpecificQuery SQL 2" );
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
                rodsLog( LOG_SQL, "chlAddSpecificQuery SQL 3" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_SPECIFIC_QUERY  (sqlStr, create_ts) values (?, ?)",
                          &icss );
        }

        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlAddSpecificQuery cmlExecuteNoAnswerSql insert failure %d",
                     status );
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
        rsComm_t*              _comm,
        char*                  _sql_or_alias ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check the params
        if ( !_comm
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
        int status, i;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelSpecificQuery" );
        }

        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege level" );
        }

        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }

        i = 0;
        cllBindVars[i++] = _sql_or_alias;
        cllBindVarCount = i;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlDelSpecificQuery SQL 1" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_SPECIFIC_QUERY where sqlStr = ?",
                      &icss );

        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlDelSpecificQuery SQL 2" );
            }
            i = 0;
            cllBindVars[i++] = _sql_or_alias;
            cllBindVarCount = i;
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_SPECIFIC_QUERY where alias = ?",
                          &icss );
        }

        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlDelSpecificQuery cmlExecuteNoAnswerSql delete failure %d",
                     status );
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

        int status, statementNum;
        int numOfCols;
        int attriTextLen;
        int totalLen;
        int maxColSize;
        int currentMaxColSize;
        char *tResult, *tResult2;
        char tsCreateTime[50];

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSpecificQuery" );
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
                rodsLog( LOG_SQL, "chlSpecificQuery SQL 1" );
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
                    rodsLog( LOG_SQL, "chlSpecificQuery SQL 2" );
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
                rodsLog( LOG_SQL, "chlSpecificQuery SQL 3" );
            }
            status = cmlGetFirstRowFromSql( combinedSQL, &statementNum,
                                            _spec_query_inp->rowOffset, &icss );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_NOTICE,
                             "chlSpecificQuery cmlGetFirstRowFromSql failure %d",
                             status );
                }
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

    irods::error db_substitute_resource_hierarchies_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        const char*            _old_hier,
        const char*            _new_hier ) {
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
        int status = 0;
        char old_hier_partial[MAX_NAME_LEN];
        irods::sql_logger logger( "chlSubstituteResourceHierarchies", logSQL );

        logger.log();

        // =-=-=-=-=-=-=-
        // Sanity and permission checks
        if ( !icss.status ) {
            return ERROR( CATALOG_NOT_CONNECTED, "catalog not connected" );
        }
        if ( !_comm || !_old_hier || !_new_hier ) {
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "null parameter" );
        }
        if ( _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH || _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return ERROR( CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "insufficient privilege" );
        }

        // =-=-=-=-=-=-=-
        // String to match partial hierarchies
        snprintf( old_hier_partial, MAX_NAME_LEN, "%s%s%%", _old_hier, irods::hierarchy_parser::delimiter().c_str() );

        // =-=-=-=-=-=-=-
        // Update r_data_main
        cllBindVars[cllBindVarCount++] = ( char* )_new_hier;
        cllBindVars[cllBindVarCount++] = ( char* )_old_hier;
        cllBindVars[cllBindVarCount++] = ( char* )_old_hier;
        cllBindVars[cllBindVarCount++] = old_hier_partial;
#if ORA_ICAT // Oracle
        status = cmlExecuteNoAnswerSql( "update R_DATA_MAIN set resc_hier = ? || substr(resc_hier, (length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss );
#else // Postgres and MySQL
        status = cmlExecuteNoAnswerSql( "update R_DATA_MAIN set resc_hier = ? || substring(resc_hier from (char_length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss );
#endif

        // Nothing was modified
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // Roll back if error
        if ( status < 0 ) {
            std::stringstream ss;
            ss << "chlSubstituteResourceHierarchies: cmlExecuteNoAnswerSql update failure " << status;
            irods::log( LOG_NOTICE, ss.str() );
            _rollback( "chlSubstituteResourceHierarchies" );
            return ERROR( status, "update failure" );
        }

        return SUCCESS();

    } // db_substitute_resource_hierarchies_op

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
            return ERROR( status, "cmlGetFirstRowFromSql failed" );
        }

        ( *_count ) = atol( icss.stmtPtr[ statement_num ]->resultValue[0] );

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
                return ERROR( status, "failed to get a row" );
            }

            _results->push_back( atoi( icss.stmtPtr[ statement_num ]->resultValue[0] ) );

        } // for i

        cmlFreeStatement( statement_num, &icss );

        return SUCCESS();

    } // db_get_distinct_data_objs_missing_from_child_given_parent_op

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

    irods::error db_mod_ticket_op(
        irods::plugin_context& _ctx,
        rsComm_t*              _comm,
        char*                  _op_name,
        char*                  _ticket_string,
        char*                  _arg3,
        char*                  _arg4,
        char*                  _arg5 ) {
        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check incoming pointers
        if ( !_comm ) {
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
        int i;
        char myTime[50];

        status = 0;

        /* session ticket */
        if ( strcmp( _op_name, "session" ) == 0 ) {
            if ( strlen( _arg3 ) > 0 ) {
                /* for 2 server hops, arg3 is the original client addr */
                status = chlGenQueryTicketSetup( _ticket_string, _arg3 );
                snprintf( mySessionTicket, sizeof( mySessionTicket ), "%s", _ticket_string );
                snprintf( mySessionClientAddr, sizeof( mySessionClientAddr ), "%s", _arg3 );
            }
            else {
                /* for direct connections, rsComm has the original client addr */
                status = chlGenQueryTicketSetup( _ticket_string, _comm->clientAddr );
                snprintf( mySessionTicket, sizeof( mySessionTicket ), "%s", _ticket_string );
                snprintf( mySessionClientAddr, sizeof( mySessionClientAddr ), "%s", _comm->clientAddr );
            }
            status = cmlAudit3( AU_USE_TICKET, "0",
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone, _ticket_string, &icss );
            if ( status != 0 ) {
                return ERROR( status, "cmlAudit3 ticket string failed" );
            }
            return SUCCESS();
        }

        /* create */
        if ( strcmp( _op_name, "create" ) == 0 ) {
            status = splitPathByKey( _arg4,
                                     logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
            if ( strlen( logicalParentDirName ) == 0 ) {
                snprintf( logicalParentDirName, sizeof( logicalParentDirName ), "%s", PATH_SEPARATOR );
                snprintf( logicalEndName, sizeof( logicalEndName ), "%s", _arg4 + 1 );
            }
            status2 = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                           _comm->clientUser.userName,
                                           _comm->clientUser.rodsZone,
                                           ACCESS_OWN, &icss );
            if ( status2 > 0 ) {
                snprintf( objTypeStr, sizeof( objTypeStr ), "%s", TICKET_TYPE_DATA );
                objId = status2;
            }
            else {
                status3 = cmlCheckDir( _arg4,   _comm->clientUser.userName,
                                       _comm->clientUser.rodsZone,
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
                rodsLog( LOG_SQL, "chlModTicket SQL 1" );
            }
            {
                std::vector<std::string> bindVars;
                bindVars.push_back( _comm->clientUser.userName );
                bindVars.push_back( _comm->clientUser.rodsZone );
                status = cmlGetIntegerValueFromSql(
                             "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                             &userId, bindVars, &icss );
            }
            if ( status != 0 ) {
                return ERROR( CAT_INVALID_USER, "select user_id failed" );
            }

            seqNum = cmlGetNextSeqVal( &icss );
            if ( seqNum < 0 ) {
                rodsLog( LOG_NOTICE, "chlModTicket failure %ld",
                         seqNum );
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
            getNowStr( myTime );
            i = 0;
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
                rodsLog( LOG_SQL, "chlModTicket SQL 2" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "insert into R_TICKET_MAIN (ticket_id, ticket_string, ticket_type, user_id, object_id, object_type, modify_ts, create_ts) values (?, ?, ?, ?, ?, ?, ?, ?)",
                          &icss );

            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql insert failure %d",
                         status );
                return ERROR( status, "insert failure" );
            }
            status = cmlAudit3( AU_CREATE_TICKET, seqNumStr,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone, _ticket_string, &icss );
            if ( status != 0 ) {
                return ERROR( status, "cmlAudit3 ticket string failed" );
            }
            status = cmlAudit3( AU_CREATE_TICKET, seqNumStr,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone, objIdStr, &icss ); /* target obj */
            if ( status != 0 ) {
                return ERROR( status, "cmlAudit3 target obj failed" );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status < 0 ) {
                return ERROR( status, "commit failed" );
            }
            else {
                return SUCCESS();
            }
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModTicket SQL 3" );
        }
        {
            std::vector<std::string> bindVars;
            bindVars.push_back( _comm->clientUser.userName );
            bindVars.push_back( _comm->clientUser.rodsZone );
            status = cmlGetIntegerValueFromSql(
                         "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                         &userId, bindVars, &icss );
        }
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ||
                status == CAT_NO_ROWS_FOUND ) {
            if ( !addRErrorMsg( &_comm->rError, 0, "Invalid user" ) ) {
            }
            return ERROR( CAT_INVALID_USER, _comm->clientUser.userName );
        }
        if ( status < 0 ) {
            return ERROR( status, "failed to select user_id" );
        }
        snprintf( userIdStr, sizeof userIdStr, "%lld", userId );

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModTicket SQL 4" );
        }
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
                rodsLog( LOG_SQL, "chlModTicket SQL 5" );
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

        /* delete */
        if ( strcmp( _op_name, "delete" ) == 0 ) {
            i = 0;
            cllBindVars[i++] = ticketIdStr;
            cllBindVars[i++] = userIdStr;
            cllBindVarCount = i;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlModTicket SQL 6" );
            }
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_TICKET_MAIN where ticket_id = ? and user_id = ?",
                          &icss );
            if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                return CODE( status );
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql delete failure %d",
                         status );
                return ERROR( status, "delete failure" );
            }

            i = 0;
            cllBindVars[i++] = ticketIdStr;
            cllBindVarCount = i;
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_TICKET_ALLOWED_HOSTS where ticket_id = ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql delete 2 failure %d",
                         status );
            }

            i = 0;
            cllBindVars[i++] = ticketIdStr;
            cllBindVarCount = i;
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_TICKET_ALLOWED_USERS where ticket_id = ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql delete 3 failure %d",
                         status );
            }

            i = 0;
            cllBindVars[i++] = ticketIdStr;
            cllBindVarCount = i;
            status =  cmlExecuteNoAnswerSql(
                          "delete from R_TICKET_ALLOWED_GROUPS where ticket_id = ?",
                          &icss );
            if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql delete 4 failure %d",
                         status );
            }
            status = cmlAudit3( AU_DELETE_TICKET, ticketIdStr,
                                _comm->clientUser.userName,
                                _comm->clientUser.rodsZone, _ticket_string, &icss );
            if ( status != 0 ) {
                return ERROR( status, "cmlAudit3 ticket string failure" );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            if ( status < 0 ) {
                return ERROR( status, "commit failed" );
            }
            else {
                return SUCCESS();
            }
        }

        /* mod */
        if ( strcmp( _op_name, "mod" ) == 0 ) {
            if ( strcmp( _arg3, "uses" ) == 0 ) {
                i = 0;
                cllBindVars[i++] = _arg4;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = userIdStr;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 7" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "update R_TICKET_MAIN set uses_limit=? where ticket_id = ? and user_id = ?",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return CODE( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                             status );
                    return ERROR( status, "update failure" );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    _comm->clientUser.userName,
                                    _comm->clientUser.rodsZone, "uses", &icss );
                if ( status != 0 ) {
                    return ERROR( status, "cmlAudit3 uses failed" );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                if ( status < 0 ) {
                    return ERROR( status, "commit failed" );
                }
                else {
                    return SUCCESS();
                }
            }

            if ( strncmp( _arg3, "write", 5 ) == 0 ) {
                if ( strstr( _arg3, "file" ) != NULL ) {
                    i = 0;
                    cllBindVars[i++] = _arg4;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = userIdStr;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 8" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "update R_TICKET_MAIN set write_file_limit=? where ticket_id = ? and user_id = ?",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                                 status );
                        return ERROR( status, "update failure" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "write file",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 write file failure" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
                if ( strstr( _arg3, "byte" ) != NULL ) {
                    i = 0;
                    cllBindVars[i++] = _arg4;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = userIdStr;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 9" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "update R_TICKET_MAIN set write_byte_limit=? where ticket_id = ? and user_id = ?",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                                 status );
                        return ERROR( status, "update failure" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "write byte",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 write byte failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
            }

            if ( strncmp( _arg3, "expir", 5 ) == 0 ) {
                status = checkDateFormat( _arg4 );
                if ( status != 0 ) {
                    return ERROR( status, "date format incorrect" );
                }
                i = 0;
                cllBindVars[i++] = _arg4;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = userIdStr;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 10" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "update R_TICKET_MAIN set ticket_expiry_ts=? where ticket_id = ? and user_id = ?",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return CODE( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                             status );
                    return ERROR( status, "update failure" );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    _comm->clientUser.userName,
                                    _comm->clientUser.rodsZone, "expire",
                                    &icss );
                if ( status != 0 ) {
                    return ERROR( status, "cmlAudit3 expire failed" );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                if ( status < 0 ) {
                    return ERROR( status, "commit failed" );
                }
                else {
                    return SUCCESS();
                }
            }

            if ( strcmp( _arg3, "add" ) == 0 ) {
                if ( strcmp( _arg4, "host" ) == 0 ) {
                    char *hostIp;
                    hostIp = convertHostToIp( _arg5 );
                    if ( hostIp == NULL ) {
                        return ERROR( CAT_HOSTNAME_INVALID, _arg5 );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = hostIp;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 11" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "insert into R_TICKET_ALLOWED_HOSTS (ticket_id, host) values (? , ?)",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql insert host failure %d",
                                 status );
                        return ERROR( status, "insert host failure" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "add host",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
                if ( strcmp( _arg4, "user" ) == 0 ) {
                    status = icatGetTicketUserId( _ctx.prop_map(), _arg5, user2IdStr );
                    if ( status != 0 ) {
                        return ERROR( status, "icatGetTicketUserId failed" );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = _arg5;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 12" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "insert into R_TICKET_ALLOWED_USERS (ticket_id, user_name) values (? , ?)",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql insert user failure %d",
                                 status );
                        return ERROR( status, "insert user failure" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "add user",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
                if ( strcmp( _arg4, "group" ) == 0 ) {
                    status = icatGetTicketGroupId( _ctx.prop_map(), _arg5, user2IdStr );
                    if ( status != 0 ) {
                        return ERROR( status, "icatGetTicketGroupId failed" );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = _arg5;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 13" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "insert into R_TICKET_ALLOWED_GROUPS (ticket_id, group_name) values (? , ?)",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql insert user failure %d",
                                 status );
                        return ERROR( status, "insert failed" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "add group",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
            }
            if ( strcmp( _arg3, "remove" ) == 0 ) {
                if ( strcmp( _arg4, "host" ) == 0 ) {
                    char *hostIp;
                    hostIp = convertHostToIp( _arg5 );
                    if ( hostIp == NULL ) {
                        return ERROR( CAT_HOSTNAME_INVALID, "host name null" );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = hostIp;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 14" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "delete from R_TICKET_ALLOWED_HOSTS where ticket_id=? and host=?",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql delete host failure %d",
                                 status );
                        return ERROR( status, "delete failed" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "remove host",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }

                }
                if ( strcmp( _arg4, "user" ) == 0 ) {
                    status = icatGetTicketUserId( _ctx.prop_map(), _arg5, user2IdStr );
                    if ( status != 0 ) {
                        return ERROR( status, "icatGetTicketUserId failed" );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = _arg5;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 15" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "delete from R_TICKET_ALLOWED_USERS where ticket_id=? and user_name=?",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql delete user failure %d",
                                 status );
                        return ERROR( status, "delete failed" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "remove user",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
                if ( strcmp( _arg4, "group" ) == 0 ) {
                    status = icatGetTicketGroupId( _ctx.prop_map(), _arg5, group2IdStr );
                    if ( status != 0 ) {
                        return ERROR( status, "icatGetTicketGroupId failed" );
                    }
                    i = 0;
                    cllBindVars[i++] = ticketIdStr;
                    cllBindVars[i++] = _arg5;
                    cllBindVarCount = i;
                    if ( logSQL != 0 ) {
                        rodsLog( LOG_SQL, "chlModTicket SQL 16" );
                    }
                    status =  cmlExecuteNoAnswerSql(
                                  "delete from R_TICKET_ALLOWED_GROUPS where ticket_id=? and group_name=?",
                                  &icss );
                    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                        return CODE( status );
                    }
                    if ( status != 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "chlModTicket cmlExecuteNoAnswerSql delete group failure %d",
                                 status );
                        return ERROR( status, "delete group failed" );
                    }
                    status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                        _comm->clientUser.userName,
                                        _comm->clientUser.rodsZone, "remove group",
                                        &icss );
                    if ( status != 0 ) {
                        return ERROR( status, "cmlAudit3 failed" );
                    }
                    status =  cmlExecuteNoAnswerSql( "commit", &icss );
                    if ( status < 0 ) {
                        return ERROR( status, "commit failed" );
                    }
                    else {
                        return SUCCESS();
                    }
                }
            }
        }

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
            rodsLog( LOG_SQL, "chlGetRcs" );
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
    int chl_gen_query_access_control_setup_impl( char*, char*, char*, int, int );

    irods::error db_gen_query_access_control_setup_op(
        irods::plugin_context& _ctx,
        char*                  _user,
        char*                  _zone,
        char*                  _host,
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
    int chl_gen_query_ticket_setup_impl( char*, char* );

    irods::error db_gen_query_ticket_setup_op(
        irods::plugin_context& _ctx,
        char*                  _ticket,
        char*                  _client_addr ) {
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
    int chl_general_update_impl( generalUpdateInp_t );

    irods::error db_general_update_op(
        irods::plugin_context& _ctx,
        generalUpdateInp_t*    _update_inp ) {
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

    // =-=-=-=-=-=-=-
    //
    irods::error db_start_operation( irods::plugin_property_map& _props ) {
#ifdef MY_ICAT
        char cml_res[ 100 ];
        const char sql[] = "select PREG_REPLACE('/failed/i', 'succeeded', 'Call to PREG_REPLACE() failed.')";
        std::vector<std::string> bindVars;
        int status = cmlGetStringValueFromSql( sql, cml_res, sizeof( cml_res ), bindVars, &icss );
        if ( status < 0 ) {
            return ERROR( status, "Failed to call PREG_REPLACE(). See section \"Installing lib_mysqludf_preg\" of iRODS Manual." );
        }

        if ( strcmp( "Call to PREG_REPLACE() succeeded.", cml_res ) ) {
            std::stringstream ss;
            ss << "Call to PREG_REPLACE() returned incorrect result: ["
               << cml_res
               << "].";
            return ERROR( PLUGIN_ERROR, ss.str().c_str() );
        }
        rodsLog( LOG_DEBUG, "db_start_operation :: Call to PREG_REPLACE() succeeded" );
#endif

        return SUCCESS();

    } // db_start_operation


    // =-=-=-=-=-=-=-
    // derive a new tcp network plugin from
    // the network plugin base class for handling
    // tcp communications
    class postgres_database_plugin : public irods::database {
        public:
            postgres_database_plugin(
                const std::string& _nm,
                const std::string& _ctx ) :
                irods::database(
                    _nm,
                    _ctx ) {
                // =-=-=-=-=-=-=-
                // create a property for the icat session
                // which will manage the lifetime of the db
                // connection - use a copy ctor to init
                icatSessionStruct icss;
                bzero( &icss, sizeof( icss ) );
                properties_.set< icatSessionStruct >( ICSS_PROP, icss );

                set_start_operation( "db_start_operation" );

            } // ctor

            ~postgres_database_plugin() {
            }

    }; // class postgres_database_plugin

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
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
        // names to function names
        pg->add_operation( irods::DATABASE_OP_START,                    "db_start_op" );
        pg->add_operation( irods::DATABASE_OP_DEBUG,                    "db_debug_op" );
        pg->add_operation( irods::DATABASE_OP_OPEN,                     "db_open_op" );
        pg->add_operation( irods::DATABASE_OP_CLOSE,                    "db_close_op" );
        pg->add_operation( irods::DATABASE_OP_GET_LOCAL_ZONE,           "db_get_local_zone_op" );
        pg->add_operation( irods::DATABASE_OP_UPDATE_RESC_OBJ_COUNT,    "db_update_resc_obj_count_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_DATA_OBJ_META,        "db_mod_data_obj_meta_op" );
        pg->add_operation( irods::DATABASE_OP_REG_DATA_OBJ,             "db_reg_data_obj_op" );
        pg->add_operation( irods::DATABASE_OP_REG_REPLICA,              "db_reg_replica_op" );
        pg->add_operation( irods::DATABASE_OP_UNREG_REPLICA,            "db_unreg_replica_op" );
        pg->add_operation( irods::DATABASE_OP_REG_RULE_EXEC,            "db_reg_rule_exec_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_RULE_EXEC,            "db_mod_rule_exec_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_RULE_EXEC,            "db_del_rule_exec_op" );
        pg->add_operation( irods::DATABASE_OP_ADD_CHILD_RESC,           "db_add_child_resc_op" );
        pg->add_operation( irods::DATABASE_OP_REG_RESC,                 "db_reg_resc_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_CHILD_RESC,           "db_del_child_resc_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_RESC,                 "db_del_resc_op" );
        pg->add_operation( irods::DATABASE_OP_ROLLBACK,                 "db_rollback_op" );
        pg->add_operation( irods::DATABASE_OP_COMMIT,                   "db_commit_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_USER_RE,              "db_del_user_re_op" );
        pg->add_operation( irods::DATABASE_OP_REG_COLL_BY_ADMIN,        "db_reg_coll_by_admin_op" );
        pg->add_operation( irods::DATABASE_OP_REG_COLL,                 "db_reg_coll_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_COLL,                 "db_mod_coll_op" );
        pg->add_operation( irods::DATABASE_OP_REG_ZONE,                 "db_reg_zone_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_ZONE,                 "db_mod_zone_op" );
        pg->add_operation( irods::DATABASE_OP_RENAME_COLL,              "db_rename_coll_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_ZONE_COLL_ACL,        "db_mod_zone_coll_acl_op" );
        pg->add_operation( irods::DATABASE_OP_RENAME_LOCAL_ZONE,        "db_rename_local_zone_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_ZONE,                 "db_del_zone_op" );
        pg->add_operation( irods::DATABASE_OP_SIMPLE_QUERY,             "db_simple_query_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_COLL_BY_ADMIN,        "db_del_coll_by_admin_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_COLL,                 "db_del_coll_op" );
        pg->add_operation( irods::DATABASE_OP_CHECK_AUTH,               "db_check_auth_op" );
        pg->add_operation( irods::DATABASE_OP_MAKE_TEMP_PW,             "db_make_temp_pw_op" );
        pg->add_operation( irods::DATABASE_OP_UPDATE_PAM_PASSWORD,      "db_update_pam_password_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_USER,                 "db_mod_user_op" );
        pg->add_operation( irods::DATABASE_OP_MAKE_LIMITED_PW,          "db_make_limited_pw_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_GROUP,                "db_mod_group_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_RESC,                 "db_mod_resc_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_RESC_DATA_PATHS,      "db_mod_resc_data_paths_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_RESC_FREESPACE,       "db_mod_resc_freespace_op" );
        pg->add_operation( irods::DATABASE_OP_REG_USER_RE,              "db_reg_user_re_op" );
        pg->add_operation( irods::DATABASE_OP_SET_AVU_METADATA,         "db_set_avu_metadata_op" );
        pg->add_operation( irods::DATABASE_OP_ADD_AVU_METADATA_WILD,    "db_add_avu_metadata_wild_op" );
        pg->add_operation( irods::DATABASE_OP_ADD_AVU_METADATA,         "db_add_avu_metadata_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_AVU_METADATA,         "db_mod_avu_metadata_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_AVU_METADATA,         "db_del_avu_metadata_op" );
        pg->add_operation( irods::DATABASE_OP_COPY_AVU_METADATA,        "db_copy_avu_metadata_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_ACCESS_CONTROL_RESC,  "db_mod_access_control_resc_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_ACCESS_CONTROL,       "db_mod_access_control_op" );
        pg->add_operation( irods::DATABASE_OP_RENAME_OBJECT,            "db_rename_object_op" );
        pg->add_operation( irods::DATABASE_OP_MOVE_OBJECT,              "db_move_object_op" );
        pg->add_operation( irods::DATABASE_OP_REG_TOKEN,                "db_reg_token_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_TOKEN,                "db_del_token_op" );
        pg->add_operation( irods::DATABASE_OP_REG_SERVER_LOAD,          "db_reg_server_load_op" );
        pg->add_operation( irods::DATABASE_OP_PURGE_SERVER_LOAD,        "db_purge_server_load_op" );
        pg->add_operation( irods::DATABASE_OP_REG_SERVER_LOAD_DIGEST,   "db_reg_server_load_digest_op" );
        pg->add_operation( irods::DATABASE_OP_PURGE_SERVER_LOAD_DIGEST, "db_purge_server_load_digest_op" );
        pg->add_operation( irods::DATABASE_OP_CALC_USAGE_AND_QUOTA,     "db_calc_usage_and_quota_op" );
        pg->add_operation( irods::DATABASE_OP_SET_QUOTA,                "db_set_quota_op" );
        pg->add_operation( irods::DATABASE_OP_CHECK_QUOTA,              "db_check_quota_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_UNUSED_AVUS,          "db_del_unused_avus_op" );
        pg->add_operation( irods::DATABASE_OP_INS_RULE_TABLE,           "db_ins_rule_table_op" );
        pg->add_operation( irods::DATABASE_OP_INS_DVM_TABLE,            "db_ins_dvm_table_op" );
        pg->add_operation( irods::DATABASE_OP_INS_FNM_TABLE,            "db_ins_fnm_table_op" );
        pg->add_operation( irods::DATABASE_OP_INS_MSRVC_TABLE,          "db_ins_msrvc_table_op" );
        pg->add_operation( irods::DATABASE_OP_VERSION_RULE_BASE,        "db_version_rule_base_op" );
        pg->add_operation( irods::DATABASE_OP_VERSION_DVM_BASE,         "db_version_dvm_base_op" );
        pg->add_operation( irods::DATABASE_OP_VERSION_FNM_BASE,         "db_version_fnm_base_op" );
        pg->add_operation( irods::DATABASE_OP_ADD_SPECIFIC_QUERY,       "db_add_specific_query_op" );
        pg->add_operation( irods::DATABASE_OP_DEL_SPECIFIC_QUERY,       "db_del_specific_query_op" );
        pg->add_operation( irods::DATABASE_OP_SPECIFIC_QUERY,           "db_specific_query_op" );
        pg->add_operation( irods::DATABASE_OP_GET_HIERARCHY_FOR_RESC,   "db_get_hierarchy_for_resc_op" );
        pg->add_operation( irods::DATABASE_OP_MOD_TICKET,               "db_mod_ticket_op" );
        pg->add_operation( irods::DATABASE_OP_CHECK_AND_GET_OBJ_ID,     "db_check_and_get_object_id_op" );
        pg->add_operation( irods::DATABASE_OP_GET_RCS,                  "db_get_icss_op" );
        pg->add_operation( irods::DATABASE_OP_GEN_QUERY,                "db_gen_query_op" );
        pg->add_operation( irods::DATABASE_OP_GENERAL_UPDATE,           "db_general_update_op" );
        pg->add_operation( irods::DATABASE_OP_GEN_QUERY_ACCESS_CONTROL_SETUP,
                           "db_gen_query_access_control_setup_op" );
        pg->add_operation( irods::DATABASE_OP_GEN_QUERY_TICKET_SETUP,
                           "db_gen_query_ticket_setup_op" );
        pg->add_operation( irods::DATABASE_OP_SUBSTITUTE_RESOURCE_HIERARCHIES,
                           "db_substitute_resource_hierarchies_op" );
        pg->add_operation( irods::DATABASE_OP_GET_DISTINCT_DATA_OBJ_COUNT_ON_RESOURCE,
                           "db_get_distinct_data_obj_count_on_resource_op" );
        pg->add_operation( irods::DATABASE_OP_GET_DISTINCT_DATA_OBJS_MISSING_FROM_CHILD_GIVEN_PARENT,
                           "db_get_distinct_data_objs_missing_from_child_given_parent_op" );

        return pg;

    } // plugin_factory

}; // extern "C"
