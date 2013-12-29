/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**************************************************************************

  This file contains most of the ICAT (iRODS Catalog) high Level
  functions.  These, along with chlGeneralQuery, constitute the API
  between and Server (and microservices) and the ICAT code.  Each of
  the API routine names start with 'chl' for Catalog High Level.
  Others are internal.

  Also see icatGeneralQuery.c for chlGeneralQuery, the other ICAT high
  level API call.

**************************************************************************/

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "irods_zone_info.hpp"
#include "irods_sql_logger.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_log.hpp"
#include "irods_tmp_string.hpp"
#include "irods_children_parser.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_catalog_properties.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"

#include "irods_database_object.hpp"
#include "irods_database_factory.hpp"
#include "irods_database_manager.hpp"
#include "irods_database_constants.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "icatMidLevelRoutines.hpp"
#include "icatMidLevelHelpers.hpp"
#include "icatHighLevelRoutines.hpp"
#include "icatLowLevel.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <boost/regex.hpp>

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

/*
   Legal values for accessLevel in  chlModAccessControl (Access Parameter).
   Defined here since other code does not need them (except for help messages)
*/
#define AP_READ "read"
#define AP_WRITE "write"
#define AP_OWN "own"
#define AP_NULL "null"

#define MAX_PASSWORDS 40
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


#if 0
/* PAM_PASSWORD_DEFAULT_TIME (the default iRODS-PAM password
   lifetime) PAM_PASSWORD_MIN_TIME must be greater than
   TEMP_PASSWORD_TIME to avoid the possibility that the logic for
   temporary passwords would be applied.  This should be fine as
   IRODS-PAM passwords will typically be valid on the order of a
   couple weeks compared to a couple minutes for temporary one-time
   passwords.
 */
#define PAM_PASSWORD_LEN 20

/* The PAM_PASSWORD_MIN_TIME must be greater than
   TEMP_PASSWORD_TIME so the logic can deal with each password type
   differently.  If they overlap, SQL errors can result */
#define PAM_PASSWORD_MIN_TIME "121"  /* must be > TEMP_PASSWORD_TIME */
#define PAM_PASSWORD_MAX_TIME "1209600"    /* two weeks in seconds */
#define TTL_PASSWORD_MIN_TIME 121  /* must be > TEMP_PASSWORD_TIME */
#define TTL_PASSWORD_MAX_TIME 1209600    /* two weeks in seconds */
/* For batch jobs that should run "forever", TTL_PASSWORD_MAX_TIME
   can be set very large, for example to 2147483647 to allow 68 years TTL. */
#ifdef PAM_AUTH_NO_EXTEND
#define PAM_PASSWORD_DEFAULT_TIME "28800"  /* 8 hours in seconds */
#else
#define PAM_PASSWORD_DEFAULT_TIME "1209600" /* two weeks in seconds */
#endif
#endif

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


int logSQL = 0;

static int removeAVUs();

icatSessionStruct icss = {0};
char localZone[MAX_NAME_LEN] = {""};

int creatingUserByGroupAdmin = 0; // JMC - backport 4772

char mySessionTicket[NAME_LEN] = "";
char mySessionClientAddr[NAME_LEN] = "";

// =-=-=-=-=-=-=-
//
int chlDebug(
    char* _mode ) {
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
        chlDebugGenQuery( 1 );
        chlDebugGenUpdate( 1 );
        cmlDebug( 2 );
    }
    else {
        logSQL = 0;
        chlDebugGenQuery( 0 );
        chlDebugGenUpdate( 0 );
        cmlDebug( 0 );
    }

    return( 0 );
}

/*
  Called internally to rollback current transaction after an error.
*/
int
_rollback( const char *functionName ) {
    int status;
#if ORA_ICAT
    status = 0;
#else
    /* This type of rollback is needed for Postgres since the low-level
       now does an automatic 'begin' to create a sql block */
    status =  cmlExecuteNoAnswerSql( "rollback", &icss );
    if ( status == 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s cmlExecuteNoAnswerSql(rollback) succeeded", functionName );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "%s cmlExecuteNoAnswerSql(rollback) failure %d",
                 functionName, status );
    }
#endif

    return( status );
}

/// =-=-=-=-=-=-=-
/// @brief Open a connection to the database.  This has to be called first.
///        The server/agent and Rule-Engine Server call this when initializing.
int chlOpen(
    rodsServerConfig* _cfg ) {
    // =-=-=-=-=-=-=-
    // check incoming params
    if ( !_cfg ) {
        rodsLog(
            LOG_ERROR,
            "null config parameter" );
        return SYS_INVALID_INPUT_PARAM;
    }
    // =-=-=-=-=-=-=-
    // cache the database type for subsequent calls
    strncpy(
        icss.database_plugin_type,
        _cfg->catalog_database_type,
        NAME_LEN );

    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the open operation on the plugin
    ret = db->call< rodsServerConfig* >(
              irods::DATABASE_OP_OPEN,
              ptr,
              _cfg );

    return ret.code();

} // chlOpen

/// =-=-=-=-=-=-=-
///  @breif Close an open connection to the database.
///         Clean up and shutdown the connection.
int chlClose() {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the open operation on the plugin
    ret = db->call(
              irods::DATABASE_OP_CLOSE,
              ptr );

    return ret.code();

} // chlClose

int chlIsConnected() {
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlIsConnected" );
    }
    return( icss.status );
}

/*
  This is used by the icatGeneralUpdate.c functions to get the icss
  structure.  icatGeneralUpdate.c and this (icatHighLevelRoutine.c)
  are actually one module but in two separate source files (as they
  got larger) so this is a 'glue' that binds them together.  So this
  is mostly an 'internal' function too.
*/
icatSessionStruct *
chlGetRcs() {
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlGetRcs" );
    }
    if ( icss.status != 1 ) {
        return( NULL );
    }
    return( &icss );
}

/*
  Internal function to return the local zone (which is the default
  zone).  The first time it's called, it gets the zone from the DB and
  subsequent calls just return that value.
*/
int
getLocalZone() {
    std::string local_zone;
    irods::error e = irods::zone_info::get_instance()->get_local_zone( icss, logSQL, local_zone );
    strncpy( localZone, local_zone.c_str(), MAX_NAME_LEN );
    return e.code();
}

/*
   External function to return the local zone name.
   Used by icatGeneralQuery.c
*/
int
chlGetLocalZone(
    std::string& _zone ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the get local zone operation on the plugin
    ret = db->call <
          const std::string* > (
              irods::DATABASE_OP_GET_LOCAL_ZONE,
              ptr,
              &_zone );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlGetLocalZone


/// =-=-=-=-=-=-=-
/// @brief Public function for updating object count on the specified resource by the specified amount
int chlUpdateRescObjCount(
    const std::string& _resc,
    int                _delta ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the open operation on the plugin
    ret = db->call <
          const std::string*,
          int > (
              irods::DATABASE_OP_UPDATE_RESC_OBJ_COUNT,
              ptr,
              &_resc,
              _delta );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlUpdateRescObjCount

// =-=-=-=-=-=-=-
// chlModDataObjMeta - Modify the metadata of an existing data object.
// Input - rsComm_t *rsComm  - the server handle
//         dataObjInfo_t *dataObjInfo - contains info about this copy of
//         a data object.
//         keyValPair_t *regParam - the keyword/value pair of items to be
//         modified. Valid keywords are given in char *dataObjCond[] in
//         rcGlobal.h.
//         If the keyword ALL_REPL_STATUS_KW is used
//         the replStatus of the copy specified by dataObjInfo
//         is marked NEWLY_CREATED_COPY and all other copies are
//         be marked OLD_COPY.
int chlModDataObjMeta(
    rsComm_t*      _comm,
    dataObjInfo_t* _data_obj_info,
    keyValPair_t*  _reg_param ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the open operation on the plugin
    ret = db->call <
          rsComm_t*,
          dataObjInfo_t*,
          keyValPair_t* > (
              irods::DATABASE_OP_MOD_DATA_OBJ_META,
              ptr,
              _comm,
              _data_obj_info,
              _reg_param );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModDataObjMeta

// =-=-=-=-=-=-=-
// chlRegDataObj - Register a new iRODS file (data object)
// Input - rsComm_t *rsComm  - the server handle
//         dataObjInfo_t *dataObjInfo - contains info about the data object.
int chlRegDataObj(
    rsComm_t*      _comm,
    dataObjInfo_t* _data_obj_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the open operation on the plugin
    ret = db->call <
          rsComm_t*,
          dataObjInfo_t* > (
              irods::DATABASE_OP_REG_DATA_OBJ,
              ptr,
              _comm,
              _data_obj_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegDataObj

// =-=-=-=-=-=-=-
// chlRegReplica - Register a new iRODS replica file (data object)
// Input - rsComm_t *rsComm  - the server handle
//         srcDataObjInfo and dstDataObjInfo each contain information
//         about the object.
// The src dataId and replNum are used in a query, a few fields are updated
// from dstDataObjInfo, and a new row inserted.
int chlRegReplica(
    rsComm_t*      _comm,
    dataObjInfo_t* _src_data_obj_info,
    dataObjInfo_t* _dst_data_obj_info,
    keyValPair_t*  _cond_input ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          dataObjInfo_t*,
          dataObjInfo_t*,
          keyValPair_t* > (
              irods::DATABASE_OP_REG_REPLICA,
              ptr,
              _comm,
              _src_data_obj_info,
              _dst_data_obj_info,
              _cond_input );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegReplica

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

/*
 * unregDataObj - Unregister a data object
 * Input - rsComm_t *rsComm  - the server handle
 *         dataObjInfo_t *dataObjInfo - contains info about the data object.
 *         keyValPair_t *condInput - used to specify a admin-mode.
 */
int chlUnregDataObj(
    rsComm_t*      _comm,
    dataObjInfo_t* _data_obj_info,
    keyValPair_t*  _cond_input ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          dataObjInfo_t*,
          keyValPair_t* > (
              irods::DATABASE_OP_UNREG_REPLICA,
              ptr,
              _comm,
              _data_obj_info,
              _cond_input );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlUnregDataObj

// =-=-=-=-=-=-=-
// chlRegRuleExec - Register a new iRODS delayed rule execution object
// Input - rsComm_t *rsComm  - the server handle
//         ruleExecSubmitInp_t *ruleExecSubmitInp - contains info about the
//             delayed rule.
int chlRegRuleExec(
    rsComm_t*            _comm,
    ruleExecSubmitInp_t* _re_sub_inp ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          ruleExecSubmitInp_t* > (
              irods::DATABASE_OP_REG_RULE_EXEC,
              ptr,
              _comm,
              _re_sub_inp );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegRuleExec

// =-=-=-=-=-=-=-
// chlModRuleExec - Modify the metadata of an existing (delayed)
// Rule Execution object.
// Input - rsComm_t *rsComm  - the server handle
//         char *ruleExecId - the id of the object to change
//         keyValPair_t *regParam - the keyword/value pair of items to be
//         modified.
int chlModRuleExec(
    rsComm_t*     _comm,
    char*         _re_id,
    keyValPair_t* _reg_param ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          keyValPair_t* > (
              irods::DATABASE_OP_MOD_RULE_EXEC,
              ptr,
              _comm,
              _re_id,
              _reg_param );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModRuleExec

/* delete a delayed rule execution entry */
int chlDelRuleExec(
    rsComm_t* _comm,
    char*     _re_id ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char* > (
              irods::DATABASE_OP_DEL_RULE_EXEC,
              ptr,
              _comm,
              _re_id );

    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelRuleExec



/*
  This can be called to test other routines, passing in an actual
  rsComm structure.

  For example, in _rsSomething instead of:
  status = chlSomething (SomethingInp);
  change it to this:
  status = chlTest(rsComm, Something->name);
  and it tests the chlRegDataObj function.
*/
int chlTest( rsComm_t *rsComm, char *name ) {
    dataObjInfo_t dataObjInfo;

    strcpy( dataObjInfo.objPath, name );
    dataObjInfo.replNum = 1;
    strcpy( dataObjInfo.version, "12" );
    strcpy( dataObjInfo.dataType, "URL" );
    dataObjInfo.dataSize = 42;

    strcpy( dataObjInfo.rescName, "resc A" );

    strcpy( dataObjInfo.filePath, "/scratch/slocal/test1" );

    dataObjInfo.replStatus = 5;

    return ( chlRegDataObj( rsComm, &dataObjInfo ) );
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
    rescInfo_t* _rescInfo ) {
    int result = 0;
    struct hostent *myHostEnt;

    myHostEnt = gethostbyname( _rescInfo->rescLoc );
    if ( myHostEnt <= 0 ) {
        char errMsg[155];
        snprintf( errMsg, 150,
                  "Warning, resource host address '%s' is not a valid DNS entry, gethostbyname failed.",
                  _rescInfo->rescLoc );
        addRErrorMsg( &_rsComm->rError, 0, errMsg );
    }
    if ( strcmp( _rescInfo->rescLoc, "localhost" ) == 0 ) {
        addRErrorMsg( &_rsComm->rError, 0,
                      "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client." );
    }

    return result;
}

int
_childIsValid(
    const std::string& _new_child ) {

    // Lookup the child resource and make sure its parent field is empty
    int result = 0;
    char parent[MAX_NAME_LEN];
    int status;

    // Get the resource name from the child string
    std::string resc_name;
    irods::children_parser parser;
    parser.set_string( _new_child );
    parser.first_child( resc_name );

    // Get resources parent
    irods::sql_logger logger( "_childIsValid", logSQL );
    logger.log();
    parent[0] = '\0';
    if ( ( status = cmlGetStringValueFromSql( "select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
                    parent, MAX_NAME_LEN, resc_name.c_str(), localZone, 0, &icss ) ) != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            std::stringstream ss;
            ss << "Child resource \"" << resc_name << "\" not found";
            irods::log( LOG_NOTICE, ss.str() );
        }
        else {
            _rollback( "_childIsValid" );
        }
        result = status;

    }
    else if ( strlen( parent ) != 0 ) {
        // If the resource already has a parent it cannot be added as a child of another one
        std::stringstream ss;
        ss << "Child resource \"" << resc_name << "\" already has a parent \"" << parent << "\"";
        irods::log( LOG_NOTICE, ss.str() );
        result = CHILD_HAS_PARENT;
    }
    return result;
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

    if ( ( status = cmlGetStringValueFromSql( "select resc_children from R_RESC_MAIN where resc_id=?",
                    children, MAX_PATH_ALLOWED, _resc_id, 0, 0, &icss ) ) != 0 ) {
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
        irods::tmp_string ts( combined_children.c_str() );
        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = ts.str();
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
}

int
_updateChildParent(
    const std::string& _new_child,
    const std::string& _parent ) {

    int result = 0;
    char resc_id[MAX_NAME_LEN];
    char myTime[50];
    irods::sql_logger logger( "_updateChildParent", logSQL );
    int status;

    // Get the resource name from the child string
    irods::children_parser parser;
    std::string child;
    parser.set_string( _new_child );
    parser.first_child( child );

    // Get the resource id for the child resource
    resc_id[0] = '\0';
    if ( ( status = cmlGetStringValueFromSql( "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                    resc_id, MAX_NAME_LEN, child.c_str(), localZone, 0,
                    &icss ) ) != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            result = CAT_INVALID_RESOURCE;
        }
        else {
            _rollback( "_updateChildParent" );
            result = status;
        }
    }
    else {
        // Update the parent for the child resource

        // have to do this to get around const
        irods::tmp_string ts( _parent.c_str() );
        getNowStr( myTime );
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = ts.str();
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = resc_id;
        logger.log();
        if ( ( status = cmlExecuteNoAnswerSql( "update R_RESC_MAIN set resc_parent=?, modify_ts=? "
                                               "where resc_id=?", &icss ) ) != 0 ) {
            std::stringstream ss;
            ss << "_updateChildParent cmlExecuteNoAnswerSql update failure " << status;
            irods::log( LOG_NOTICE, ss.str() );
            _rollback( "_updateChildParent" );
            result = status;
        }
    }

    return result;
}

irods::error _get_resc_obj_count(
    const std::string& _resc_name,
    rodsLong_t & _rtn_obj_count ) {
    irods::error result = SUCCESS();
    rodsLong_t obj_count = 0;
    int status;

    if ( ( status = cmlGetIntegerValueFromSql( "select resc_objcount from R_RESC_MAIN where resc_name=?",
                    &obj_count, _resc_name.c_str(), 0, 0, 0, 0, &icss ) ) != 0 ) {
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
    if ( ( status = cmlGetIntegerValueFromSql( "select resc_objcount from R_RESC_MAIN where resc_name=?",
                    &obj_count, _resc_name.c_str(), 0, 0, 0, 0, &icss ) ) != 0 ) {
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

/**
 * @brief Adds the child, with context, to the resource all specified in the rescInfo
 */
int
chlAddChildResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rescInfo_t* > (
              irods::DATABASE_OP_ADD_CHILD_RESC,
              ptr,
              _comm,
              _resc_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlAddChildResc


/// @brief function for validating a resource name
irods::error validate_resource_name( std::string _resc_name ) {

    // Must be between 1 and NAME_LEN-1 characters.
    // Must start and end with a word character.
    // May contain non consecutive dashes.
    boost::regex re( "^(?=.{1,63}$)\\w(\\w*(-\\w+)?)*$" );

    if ( !boost::regex_match( _resc_name, re ) ) {
        std::stringstream msg;
        msg << "validate_resource_name failed for resource [";
        msg << _resc_name;
        msg << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    return SUCCESS();

} // validate_user_name


/* register a Resource */
int chlRegResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rescInfo_t* > (
              irods::DATABASE_OP_REG_RESC,
              ptr,
              _comm,
              _resc_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegResc

/**
 * @brief Returns true if the specified resource already has children
 */
#if 0 // currently unused
static bool
_rescHasChildren(
    const std::string _resc_name ) {

    bool result = false;
    int status;
    char children[MAX_NAME_LEN];
    irods::sql_logger logger( "_rescHasChildren", logSQL );

    logger.log();
    if ( ( status = cmlGetStringValueFromSql( "select resc_children from R_RESC_MAIN where resc_name=?",
                    children, MAX_NAME_LEN, _resc_name.c_str(), 0, 0, &icss ) ) != 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            _rollback( "_rescHasChildren" );
        }
        result = false;
    }
    else if ( strlen( children ) != 0 ) {
        result = true;
    }
    return result;
}
#endif

/**
 * @brief Remove a child from its parent
 */
int
chlDelChildResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rescInfo_t* > (
              irods::DATABASE_OP_DEL_CHILD_RESC,
              ptr,
              _comm,
              _resc_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelChildResc

// =-=-=-=-=-=-=-
// delete a Resource
int chlDelResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info,
    int         _dry_run ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rescInfo_t*,
          int > (
              irods::DATABASE_OP_DEL_RESC,
              ptr,
              _comm,
              _resc_info,
              _dry_run );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelResc

// =-=-=-=-=-=-=-
// Issue a rollback command.
//
// If we don't do a commit, the updates will not be saved in the
// database but will still exist during the current connection.  Since
// iadmin connects once and then can issue multiple commands there are
// situations where we need to rollback.
//
// For example, if the user's zone is wrong the code will first remove the
// home collection and then fail when removing the user and we need to
// rollback or the next attempt will show the collection as missing.
int chlRollback(
    rsComm_t* _comm ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t* > (
              irods::DATABASE_OP_ROLLBACK,
              ptr,
              _comm );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRollback

// =-=-=-=-=-=-=-
// Issue a commit command.
// This is called to commit changes to the database.
// Some of the chl functions also commit changes upon success but some
// do not, having the caller (microservice, perhaps) either commit or
// rollback.
int chlCommit(
    rsComm_t* _comm ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t* > (
              irods::DATABASE_OP_COMMIT,
              ptr,
              _comm );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlCommit

// =-=-=-=-=-=-=-
// Delete a User, Rule Engine version
int chlDelUserRE(
    rsComm_t*   _comm,
    userInfo_t* _user_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          userInfo_t* > (
              irods::DATABASE_OP_DEL_USER_RE,
              ptr,
              _comm,
              _user_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelUserRE

// =-=-=-=-=-=-=-
//  Register a Collection by the admin.
//  There are cases where the irods admin needs to create collections,
//  for a new user, for example; thus the create user rule/microservices
//  make use of this.
int chlRegCollByAdmin(
    rsComm_t*   _comm,
    collInfo_t* _coll_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          collInfo_t* > (
              irods::DATABASE_OP_REG_COLL_BY_ADMIN,
              ptr,
              _comm,
              _coll_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegCollByAdmin

// =-=-=-=-=-=-=-
// chlRegColl - register a collection
// Input -
//   rcComm_t *conn - The client connection handle.
//   collInfo_t *collInfo - generic coll input. Relevant items are:
//      collName - the collection to be registered, and optionally
//      collType, collInfo1 and/or collInfo2.
//   We may need a kevValPair_t sometime, but currently not used.
int chlRegColl(
    rsComm_t*   _comm,
    collInfo_t* _coll_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          collInfo_t* > (
              irods::DATABASE_OP_REG_COLL,
              ptr,
              _comm,
              _coll_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegColl

// =-=-=-=-=-=-=-
// chlModColl - modify attributes of a collection
// Input -
//   rcComm_t *conn - The client connection handle.
//   collInfo_t *collInfo - generic coll input. Relevant items are:
//      collName - the collection to be updated, and one or more of:
//      collType, collInfo1 and/or collInfo2.
//   We may need a kevValPair_t sometime, but currently not used.
int chlModColl(
    rsComm_t*   _comm,
    collInfo_t* _coll_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          collInfo_t* > (
              irods::DATABASE_OP_MOD_COLL,
              ptr,
              _comm,
              _coll_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModColl

// =-=-=-=-=-=-=-
// register a Zone
int chlRegZone(
    rsComm_t* _comm,
    char*     _zone_name,
    char*     _zone_type,
    char*     _zone_conn_info,
    char*     _zone_comment ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_REG_ZONE,
              ptr,
              _comm,
              _zone_name,
              _zone_type,
              _zone_conn_info,
              _zone_comment );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegZone

// =-=-=-=-=-=-=-
// Modify a Zone (certain fields)
int chlModZone(
    rsComm_t* _comm,
    char*     _zone_name,
    char*     _option,
    char*     _option_value ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_ZONE,
              ptr,
              _comm,
              _zone_name,
              _option,
              _option_value );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModZone

// =-=-=-=-=-=-=-
// rename a collection
int chlRenameColl(
    rsComm_t* _comm,
    char*     _old_coll,
    char*     _new_coll ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char* > (
              irods::DATABASE_OP_RENAME_COLL,
              ptr,
              _comm,
              _old_coll,
              _new_coll );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRenameColl

// =-=-=-=-=-=-=-
// Modify a Zone Collection ACL
int chlModZoneCollAcl(
    rsComm_t* _comm,
    char*     _access_level,
    char*     _user_name,
    char*     _path_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_ZONE_COLL_ACL,
              ptr,
              _comm,
              _access_level,
              _user_name,
              _path_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModZoneCollAcl

// =-=-=-=-=-=-=-
// rename the local zone
int chlRenameLocalZone(
    rsComm_t* _comm,
    char*     _old_zone,
    char*     _new_zone ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char* > (
              irods::DATABASE_OP_RENAME_LOCAL_ZONE,
              ptr,
              _comm,
              _old_zone,
              _new_zone );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRenameLocalZone

/* delete a Zone */
int chlDelZone(
    rsComm_t* _comm,
    char*     _zone_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char* > (
              irods::DATABASE_OP_DEL_ZONE,
              ptr,
              _comm,
              _zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelZone


// =-=-=-=-=-=-=-
// Simple query

// This is used in cases where it is easier to do a straight-forward
// SQL query rather than go thru the generalQuery interface.  This is
// used this in the iadmin.c interface as it was easier for me (Wayne)
// to work in SQL for admin type ops as I'm thinking in terms of
// tables and columns and SQL anyway.

// For improved security, this is available only to admin users and
// the code checks that the input sql is one of the allowed forms.

// input: sql, up to for optional arguments (bind variables),
// and requested format, max text to return (maxOutBuf)
// output: text (outBuf) or error return
// input/output: control: on input if 0 request is starting,
// returned non-zero if more rows
// are available (and then it is the statement number);
// on input if positive it is the statement number (+1) that is being
// continued.
// format 1: column-name : column value, and with CR after each column
// format 2: column headings CR, rows and col values with CR
int chlSimpleQuery(
    rsComm_t* _comm,
    char*     _sql,
    char*     _arg1,
    char*     _arg2,
    char*     _arg3,
    char*     _arg4,
    int       _format,
    int*      _control,
    char*     _out_buf,
    int       _max_out_buf ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char*,
          char*,
          int,
          int*,
          char*,
          int > (
              irods::DATABASE_OP_SIMPLE_QUERY,
              ptr,
              _comm,
              _sql,
              _arg1,
              _arg2,
              _arg3,
              _arg4,
              _format,
              _control,
              _out_buf,
              _max_out_buf );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlSimpleQuery

// =-=-=-=-=-=-=-
// Delete a Collection by Administrator,
// if it is empty.
int chlDelCollByAdmin(
    rsComm_t*   _comm,
    collInfo_t* _coll_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          collInfo_t* > (
              irods::DATABASE_OP_DEL_COLL_BY_ADMIN,
              ptr,
              _comm,
              _coll_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelCollByAdmin

// =-=-=-=-=-=-=-
// Delete a Collection
int chlDelColl(
    rsComm_t*   _comm,
    collInfo_t* _coll_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          collInfo_t* > (
              irods::DATABASE_OP_DEL_COLL,
              ptr,
              _comm,
              _coll_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelColl

/* Check an authentication response.

   Input is the challange, response, and username; the response is checked
   and if OK the userPrivLevel is set.  Temporary-one-time passwords are
   also checked and possibly removed.

   The clientPrivLevel is the privilege level for the client in
   the rsComm structure; this is used by servers when setting the
   authFlag.

   Called from rsAuthCheck.
*/
int chlCheckAuth(
    rsComm_t*   _comm,
    const char* _scheme,
    char*       _challenge,
    char*       _response,
    char*       _user_name,
    int*        _user_priv_level,
    int*        _client_priv_level ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          const char*,
          char*,
          char*,
          char*,
          int*,
          int* > (
              irods::DATABASE_OP_CHECK_AUTH,
              ptr,
              _comm,
              _scheme,
              _challenge,
              _response,
              _user_name,
              _user_priv_level,
              _client_priv_level );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlCheckAuth

// =-=-=-=-=-=-=-
// Generate a temporary, one-time password.
// Input is the username from the rsComm structure and an optional otherUser.
// Output is the pattern, that when hashed with the client user's password,
// becomes the temporary password.  The temp password is also stored
// in the database.

// Called from rsGetTempPassword and rsGetTempPasswordForOther.

// If otherUser is non-blank, then create a password for the
// specified user, and the caller must be a local admin.
int chlMakeTempPw(
    rsComm_t* _comm,
    char*     _pw_value_to_hash,
    char*     _other_user ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char* > (
              irods::DATABASE_OP_MAKE_TEMP_PW,
              ptr,
              _comm,
              _pw_value_to_hash,
              _other_user );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlMakeTempPw

int
chlMakeLimitedPw(
    rsComm_t* _comm,
    int       _ttl,
    char*     _pw_value_to_hash ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char* > (
              irods::DATABASE_OP_MAKE_LIMITED_PW,
              ptr,
              _comm,
              _ttl,
              _pw_value_to_hash );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlMakeLimitedPw


// =-=-=-=-=-=-=-
// Add or update passwords for use in the PAM authentication mode.
// User has been PAM-authenticated when this is called.
// This function adds a irods password valid for a few days and returns that.
// If one already exists, the expire time is updated, and it's value is returned.
// Passwords created are pseudo-random strings, unrelated to the PAM password.
// If testTime is non-null, use that as the create-time, as a testing aid.
int chlUpdateIrodsPamPassword(
    rsComm_t* _comm,
    char*     _user_name,
    int       _ttl,
    char*     _test_time,
    char**    _irods_password ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          int,
          char*,
          char** > (
              irods::DATABASE_OP_UPDATE_PAM_PASSWORD,
              ptr,
              _comm,
              _user_name,
              _ttl,
              _test_time,
              _irods_password );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlUpdateIrodsPamPassword

// =-=-=-=-=-=-=-
// Admin Only Fcn -- Modify an existing user
// Called from rsGeneralAdmin which is used by iadmin
int chlModUser(
    rsComm_t* _comm,
    char*     _user_name,
    char*     _option,
    char*     _new_value ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_USER,
              ptr,
              _comm,
              _user_name,
              _option,
              _new_value );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModUser

// =-=-=-=-=-=-=-
// Modify an existing group (membership).
// Groups are also users in the schema, so chlModUser can also
// modify other group attibutes. */
int chlModGroup(
    rsComm_t* _comm,
    char*     _group_name,
    char*     _option,
    char*     _user_name,
    char*     _user_zone ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_GROUP,
              ptr,
              _comm,
              _group_name,
              _option,
              _user_name,
              _user_zone );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModGroup

// =-=-=-=-=-=-=-
// Modify a Resource (certain fields)
int chlModResc(
    rsComm_t* _comm,
    char*     _resc_name,
    char*     _option,
    char*     _option_value ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_RESC,
              ptr,
              _comm,
              _resc_name,
              _option,
              _option_value );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModResc

// =-=-=-=-=-=-=-
// Modify a Resource Data Paths
int chlModRescDataPaths(
    rsComm_t* _comm,
    char*     _resc_name,
    char*     _old_path,
    char*     _new_path,
    char*     _user_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_RESC_DATA_PATHS,
              ptr,
              _comm,
              _resc_name,
              _old_path,
              _new_path,
              _user_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();


} // chlModRescDataPaths

// =-=-=-=-=-=-=-
// Add or substract to the resource free_space
int chlModRescFreeSpace(
    rsComm_t* _comm,
    char*     _resc_name,
    int       _update_value ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          int > (
              irods::DATABASE_OP_MOD_RESC_FREESPACE,
              ptr,
              _comm,
              _resc_name,
              _update_value );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModRescFreeSpace

#if 0
/* Add or remove a resource to/from a Resource Group */
int chlModRescGroup( rsComm_t *rsComm, char *rescGroupName, char *option,
                     char *rescName ) {
    int status, OK;
    char myTime[50];
    char rescId[MAX_NAME_LEN];
    rodsLong_t seqNum;
    char rescGroupId[MAX_NAME_LEN];
    char dataObjNumber[MAX_NAME_LEN];
    char commentStr[200];

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chl-Mod-Resc-Group" );
    }

    if ( rescGroupName == NULL || option == NULL || rescName == NULL ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    if ( *rescGroupName == '\0' || *option == '\0' || *rescName == '\0' ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }
    if ( rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    status = getLocalZone();
    if ( status != 0 ) {
        return( status );
    }

    rescId[0] = '\0';
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 1 " );
    }
    status = cmlGetStringValueFromSql(
                 "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                 rescId, MAX_NAME_LEN, rescName, localZone, 0, &icss );
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return( CAT_INVALID_RESOURCE );
        }
        _rollback( "chlModRescGroup" );
        return( status );
    }

    getNowStr( myTime );
    OK = 0;
    if ( strcmp( option, "add" ) == 0 ) {
        /* First try to look for a resc_group id with the same rescGrpName */
        rescGroupId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 2a " );
        }
        status = cmlGetStringValueFromSql(
                     "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
                     rescGroupId, MAX_NAME_LEN, rescGroupName, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                /* Generate a new id */
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 2b " );
                }
                seqNum = cmlGetNextSeqVal( &icss );
                if ( seqNum < 0 ) {
                    rodsLog( LOG_NOTICE, "chlModRescGroup cmlGetNextSeqVal failure %d",
                             seqNum );
                    _rollback( "chlModRescGroup" );
                    return( seqNum );
                }
                snprintf( rescGroupId, MAX_NAME_LEN, "%lld", seqNum );
            }
            else {
                _rollback( "chlModRescGroup" );
                return( status );
            }
        }

        cllBindVars[cllBindVarCount++] = rescGroupName;
        cllBindVars[cllBindVarCount++] = rescGroupId;
        cllBindVars[cllBindVarCount++] = rescId;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = myTime;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 2" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "insert into R_RESC_GROUP (resc_group_name, resc_group_id, resc_id , create_ts, modify_ts) values (?, ?, ?, ?, ?)",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRescGroup cmlExecuteNoAnswerSql insert failure %d",
                     status );
            _rollback( "chlModRescGroup" );
            return( status );
        }
        OK = 1;
    }

    if ( strcmp( option, "remove" ) == 0 ) {
        /* Step 1 : get the resc_group_id as a dataObjNumber*/
        dataObjNumber[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 3a " );
        }
        status = cmlGetStringValueFromSql(
                     "select distinct resc_group_id from R_RESC_GROUP where resc_id=? and resc_group_name=?",
                     dataObjNumber, MAX_NAME_LEN, rescId, rescGroupName, 0, &icss );
        if ( status != 0 ) {
            _rollback( "chlModRescGroup" );
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_RESOURCE );
            }
            return( status );
        }

        /* Step 2 : remove the (resc_group,resc) couple */
        cllBindVars[cllBindVarCount++] = rescGroupName;
        cllBindVars[cllBindVarCount++] = rescId;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 3b" );
        }
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_RESC_GROUP where resc_group_name=? and resc_id=?",
                      &icss );
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModRescGroup cmlExecuteNoAnswerSql delete failure %d",
                     status );
            _rollback( "chlModRescGroup" );
            return( status );
        }

        /* Step 3 : look if the resc_group_name is still refered to */
        rescGroupId[0] = '\0';
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chl-Mod-Resc-Group S Q L 3c " );
        }
        status = cmlGetStringValueFromSql(
                     "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
                     rescGroupId, MAX_NAME_LEN, rescGroupName, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                /* The resource group exists no more */
                removeMetaMapAndAVU( dataObjNumber ); /* remove AVU metadata, if any */
            }
        }
        OK = 1;
    }

    if ( OK == 0 ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    /* Audit */
    snprintf( commentStr, sizeof commentStr, "%s %s", option, rescGroupName );
    status = cmlAudit3( AU_MOD_RESC_GROUP,
                        rescId,
                        rsComm->clientUser.userName,
                        rsComm->clientUser.rodsZone,
                        commentStr,
                        &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlModRescGroup cmlAudit3 failure %d",
                 status );
        _rollback( "chlModRescGroup" );
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlModRescGroup cmlExecuteNoAnswerSql commit failure %d",
                 status );
        return( status );
    }
    return( 0 );
}

#endif

// =-=-=-=-=-=-=-
// Register a User, RuleEngine version
int chlRegUserRE(
    rsComm_t*   _comm,
    userInfo_t* _user_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          userInfo_t* > (
              irods::DATABASE_OP_REG_USER_RE,
              ptr,
              _comm,
              _user_info );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegUserRE

int
convertTypeOption( char *typeStr ) {
    if ( strcmp( typeStr, "-d" ) == 0 ) {
        return( 1 );    /* dataObj */
    }
    if ( strcmp( typeStr, "-D" ) == 0 ) {
        return( 1 );    /* dataObj */
    }
    if ( strcmp( typeStr, "-c" ) == 0 ) {
        return( 2 );    /* collection */
    }
    if ( strcmp( typeStr, "-C" ) == 0 ) {
        return( 2 );    /* collection */
    }
    if ( strcmp( typeStr, "-r" ) == 0 ) {
        return( 3 );    /* resource */
    }
    if ( strcmp( typeStr, "-R" ) == 0 ) {
        return( 3 );    /* resource */
    }
    if ( strcmp( typeStr, "-u" ) == 0 ) {
        return( 4 );    /* user */
    }
    if ( strcmp( typeStr, "-U" ) == 0 ) {
        return( 4 );    /* user */
    }
#ifdef RESC_GROUP
    if ( strcmp( typeStr, "-g" ) == 0 ) {
        return( 5 );    /* resource group */
    }
    if ( strcmp( typeStr, "-G" ) == 0 ) {
        return( 5 );    /* resource group */
    }
#endif
    return ( 0 );
}

/*
  Check object - get an object's ID and check that the user has access.
  Called internally.
*/
rodsLong_t checkAndGetObjectId( rsComm_t *rsComm, char *type,
                                char *name, char *access ) {
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
        return( CATALOG_NOT_CONNECTED );
    }

    if ( type == NULL || *type == '\0' ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    if ( name == NULL || *name == '\0' ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    itype = convertTypeOption( type );
    if ( itype == 0 ) {
        return( CAT_INVALID_ARGUMENT );
    }

    if ( itype == 1 ) {
        status = splitPathByKey( name,
                                 logicalParentDirName, logicalEndName, '/' );
        if ( strlen( logicalParentDirName ) == 0 ) {
            strcpy( logicalParentDirName, "/" );
            strcpy( logicalEndName, name );
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
            return( status );
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
            return( status );
        }
        objId = status;
    }

    if ( itype == 3 ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
        }

        status = getLocalZone();
        if ( status != 0 ) {
            return( status );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 3" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     &objId, name, localZone, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_RESOURCE );
            }
            _rollback( "checkAndGetObjectId" );
            return( status );
        }
    }

    if ( itype == 4 ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
        }

        status = parseUserName( name, userName, userZone );
        if ( userZone[0] == '\0' ) {
            status = getLocalZone();
            if ( status != 0 ) {
                return( status );
            }
            strncpy( userZone, localZone, NAME_LEN );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId SQL 4" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                     &objId, userName, userZone, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_USER );
            }
            _rollback( "checkAndGetObjectId" );
            return( status );
        }
    }

#ifdef RESC_GROUP
    if ( itype == 5 ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
        }

        status = getLocalZone();
        if ( status != 0 ) {
            return( status );
        }

        objId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "checkAndGetObjectId S Q L 5" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
                     &objId, name, 0, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_RESOURCE );
            }
            _rollback( "checkAndGetObjectId" );
            return( status );
        }
    }
#endif

    return( objId );
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
        status = cmlGetIntegerValueFromSql(
                     "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and meta_attr_unit=?",
                     &iVal, attribute, value, units, 0, 0, &icss );
    }
    else {
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "findAVU SQL 2" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and (meta_attr_unit='' or meta_attr_unit IS NULL)", // JMC - backport 4827
                     &iVal, attribute, value, 0, 0, 0, &icss );
    }
    if ( status == 0 ) {
        status = iVal; /* use existing R_META_MAIN row */
        return( status );
    }
// =-=-=-=-=-=-=-
// JMC - backport 4836
    return( status ); // JMC - backport 4836
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
        return( status );
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
        return( status );
    }
    return( seqNum );
}

// =-=-=-=-=-=-=-
// JMC - backport 4836
/* Add or modify an Attribute-Value pair metadata item of an object*/
int chlSetAVUMetadata(
    rsComm_t* _comm,
    char*     _type,
    char*     _name,
    char*     _attribute,
    char*     _new_value,
    char*     _new_unit ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }
    if ( !_type ) {
        rodsLog( LOG_NOTICE, "XXXX - chlSetAVUMetadata :: type is null" );
    }
    else {
        rodsLog( LOG_NOTICE, "XXXX - chlSetAVUMetadata :: type is not null [%s]", _type );
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_SET_AVU_METADATA,
              ptr,
              _comm,
              _type,
              _name,
              _attribute,
              _new_value,
              _new_unit );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlSetAVUMetadata

// =-=-=-=-=-=-=-
// Add an Attribute-Value [Units] pair/triple metadata item to one or
// more data objects.  This is the Wildcard version, where the
// collection/data-object name can match multiple objects).

// The return value is error code (negative) or the number of objects
// to which the AVU was associated.
int chlAddAVUMetadataWild(
    rsComm_t* _comm,
    int       _admin_mode,
    char*     _type,
    char*     _name,
    char*     _attribute,
    char*     _value,
    char*     _units ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_ADD_AVU_METADATA_WILD,
              ptr,
              _comm,
              _admin_mode,
              _type,
              _name,
              _attribute,
              _value,
              _units );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlAddAVUMetadataWild

// =-=-=-=-=-=-=-
// Add an Attribute-Value [Units] pair/triple metadata item to an object
int chlAddAVUMetadata(
    rsComm_t* _comm,
    int       _admin_mode,
    char*     _type,
    char*     _name,
    char*     _attribute,
    char*     _value,
    char*     _units ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_ADD_AVU_METADATA,
              ptr,
              _comm,
              _admin_mode,
              _type,
              _name,
              _attribute,
              _value,
              _units );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlAddAVUMetadata

/* Modify an Attribute-Value [Units] pair/triple metadata item of an object*/
int chlModAVUMetadata(
    rsComm_t* _comm,
    char*     _type,
    char*     _name,
    char*     _attribute,
    char*     _value,
    char*     _unitsOrArg0,
    char*     _arg1,
    char*     _arg2,
    char*     _arg3 ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_AVU_METADATA,
              ptr,
              _comm,
              _type,
              _name,
              _attribute,
              _value,
              _unitsOrArg0,
              _arg1,
              _arg2,
              _arg3 );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModAVUMetadata

/* Delete an Attribute-Value [Units] pair/triple metadata item from an object*/
/* option is 0: normal, 1: use wildcards, 2: input is id not type,name,units */
/* noCommit: if 1: skip the commit (only used by chlModAVUMetadata) */
int chlDeleteAVUMetadata(
    rsComm_t* _comm,
    int       _option,
    char*     _type,
    char*     _name,
    char*     _attribute,
    char*     _value,
    char*     _units,
    int       _nocommit ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char*,
          char*,
          char*,
          char*,
          char*,
          int > (
              irods::DATABASE_OP_DEL_AVU_METADATA,
              ptr,
              _comm,
              _option,
              _type,
              _name,
              _attribute,
              _value,
              _units,
              _nocommit );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDeleteAVUMetadata

// =-=-=-=-=-=-=-
// Copy an Attribute-Value [Units] pair/triple from one object to another
int chlCopyAVUMetadata(
    rsComm_t* _comm,
    char*     _type1,
    char*     _type2,
    char*     _name1,
    char*     _name2 ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_COPY_AVU_METADATA,
              ptr,
              _comm,
              _type1,
              _type2,
              _name1,
              _name2 );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlCopyAVUMetadata

int chlModAccessControlResc(
    rsComm_t* _comm,
    int       _recursive_flag,
    char*     _access_level,
    char*     _user_name,
    char*     _zone,
    char*     _resc_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_ACCESS_CONTROL_RESC,
              ptr,
              _comm,
              _recursive_flag,
              _access_level,
              _user_name,
              _zone,
              _resc_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModAccessControlResc

/* create a path name with escaped SQL special characters (% and _) */
void
makeEscapedPath( char *inPath, char *outPath, int size ) {
    int i;
    for ( i = 0; i < size - 1; i++ ) {
        if ( *inPath == '%' || *inPath == '_' ) {
            *outPath++ = '\\';
        }
        if ( *inPath == '\0' ) {
            *outPath++ = *inPath++;
            break;
        }
        *outPath++ = *inPath++;
    }
    return;
}

/* Internal routine to modify inheritance */
/* inheritFlag =1 to set, 2 to remove */
int _modInheritance( int inheritFlag, int recursiveFlag, char *collIdStr, char *pathName ) {
    rodsLong_t status;
    char myTime[50];
    char newValue[10];
    char pathStart[MAX_NAME_LEN * 2];
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
        makeEscapedPath( pathName, pathStart, sizeof( pathStart ) );
        strncat( pathStart, "/%", sizeof( pathStart ) );

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
        return( status );
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}

// =-=-=-=-=-=-=-
// chlModAccessControl - Modify the Access Control information
//                       of an existing dataObj or collection.
// "n" (null or none) used to remove access.
int chlModAccessControl(
    rsComm_t* _comm,
    int       _recursive_flag,
    char*     _access_level,
    char*     _user_name,
    char*     _zone,
    char*     _path_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          int,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_MOD_ACCESS_CONTROL,
              ptr,
              _comm,
              _recursive_flag,
              _access_level,
              _user_name,
              _zone,
              _path_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlModAccessControl

// =-=-=-=-=-=-=-
// chlRenameObject - Rename a dataObject or collection.
int chlRenameObject(
    rsComm_t*  _comm,
    rodsLong_t _obj_id,
    char*      _new_name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rodsLong_t,
          char* > (
              irods::DATABASE_OP_RENAME_OBJECT,
              ptr,
              _comm,
              _obj_id,
              _new_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRenameObject

// =-=-=-=-=-=-=-
// chlMoveObject - Move a dataObject or collection to another
// collection.
int chlMoveObject(
    rsComm_t*  _comm,
    rodsLong_t _obj_id,
    rodsLong_t _target_coll_id ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          rodsLong_t,
          rodsLong_t > (
              irods::DATABASE_OP_MOVE_OBJECT,
              ptr,
              _comm,
              _obj_id,
              _target_coll_id );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlMoveObject

// =-=-=-=-=-=-=-
// chlRegToken - Register a new token
int chlRegToken(
    rsComm_t* _comm,
    char*     _name_space,
    char*     _name,
    char*     _value,
    char*     _value2,
    char*     _value3,
    char*     _comment ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char*,
          char*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_REG_TOKEN,
              ptr,
              _comm,
              _name_space,
              _name,
              _value,
              _value2,
              _value3,
              _comment );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlRegToken


// =-=-=-=-=-=-=-
// chlDelToken - Delete a token
int chlDelToken(
    rsComm_t* _comm,
    char*     _name_space,
    char*     _name ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           icss.database_plugin_type,
                           db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve(
              irods::DATABASE_INTERFACE,
              db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log(
            PASSMSG(
                "failed to resolve database interface",
                ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <
                                        irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <
                                       irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <
          rsComm_t*,
          char*,
          char* > (
              irods::DATABASE_OP_DEL_TOKEN,
              ptr,
              _comm,
              _name_space,
              _name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    return ret.code();

} // chlDelToken


/*
 * chlRegServerLoad - Register a new iRODS server load row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlRegServerLoad( rsComm_t *rsComm,
                      char *hostName, char *rescName,
                      char *cpuUsed, char *memUsed, char *swapUsed,
                      char *runqLoad, char *diskSpace, char *netInput,
                      char *netOutput ) {
    char myTime[50];
    int status;
    int i;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlRegServerLoad" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    getNowStr( myTime );

    i = 0;
    cllBindVars[i++] = hostName;
    cllBindVars[i++] = rescName;
    cllBindVars[i++] = cpuUsed;
    cllBindVars[i++] = memUsed;
    cllBindVars[i++] = swapUsed;
    cllBindVars[i++] = runqLoad;
    cllBindVars[i++] = diskSpace;
    cllBindVars[i++] = netInput;
    cllBindVars[i++] = netOutput;
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlRegServerLoad cmlExecuteNoAnswerSql commit failure %d",
                 status );
        return( status );
    }

    return( 0 );
}

/*
 * chlPurgeServerLoad - Purge some rows from iRODS server load table
 * that are older than secondsAgo seconds ago.
 * Input - rsComm_t *rsComm - the server handle,
 *    char *secondsAgo (age in seconds).
 */
int chlPurgeServerLoad( rsComm_t *rsComm, char *secondsAgo ) {

    /* delete from R_LOAD_SERVER where (%i -exe_time) > %i */
    int status;
    char nowStr[50];
    static char thenStr[50];
    time_t nowTime;
    time_t thenTime;
    time_t secondsAgoTime;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlPurgeServerLoad" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    getNowStr( nowStr );
    nowTime = atoll( nowStr );
    secondsAgoTime = atoll( secondsAgo );
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}

/*
 * chlRegServerLoadDigest - Register a new iRODS server load-digest row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlRegServerLoadDigest( rsComm_t *rsComm,
                            char *rescName, char *loadFactor ) {
    char myTime[50];
    int status;
    int i;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlRegServerLoadDigest" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    getNowStr( myTime );

    i = 0;
    cllBindVars[i++] = rescName;
    cllBindVars[i++] = loadFactor;
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlRegServerLoadDigest cmlExecuteNoAnswerSql commit failure %d",
                 status );
        return( status );
    }

    return( 0 );
}

/*
 * chlPurgeServerLoadDigest - Purge some rows from iRODS server LoadDigest table
 * that are older than secondsAgo seconds ago.
 * Input - rsComm_t *rsComm - the server handle,
 *    int secondsAgo (age in seconds).
 */
int chlPurgeServerLoadDigest( rsComm_t *rsComm, char *secondsAgo ) {
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

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    getNowStr( nowStr );
    nowTime = atoll( nowStr );
    secondsAgoTime = atoll( secondsAgo );
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
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
        return( 0 );    /* no quotas, done */
    }
    if ( status != 0 ) {
        return( status );
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
        return( status );
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
            return( status2 );
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
            return( status2 );
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        return( status );
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
            return( status2 );
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }
    if ( status != 0 ) {
        return( status );
    }

    /* To simplify the query, if either of the above group operations
       found some over_quota, will probably want to update and insert rows
       for each user into R_QUOTA_MAIN.  For now tho, this is not done and
       perhaps shouldn't be, to keep it a little less complicated. */

    return( status );
}


int chlCalcUsageAndQuota( rsComm_t *rsComm ) {
    int status;
    char myTime[50];

    status = 0;
    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
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
        return( status );
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
        return( status );
    }

    /* Set the over_quota flags where appropriate */
    status = setOverQuota( rsComm );
    if ( status != 0 ) {
        _rollback( "chlCalcUsageAndQuota" );
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}

int chlSetQuota( rsComm_t *rsComm, char *type, char *name,
                 char *rescName, char* limit ) {
    int status;
    rodsLong_t rescId;
    rodsLong_t userId;
    char userZone[NAME_LEN];
    char userName[NAME_LEN];
    char rescIdStr[60];
    char userIdStr[60];
    char myTime[50];
    int itype = 0;

    if ( strncmp( type, "user", 4 ) == 0 ) {
        itype = 1;
    }
    if ( strncmp( type, "group", 5 ) == 0 ) {
        itype = 2;
    }
    if ( itype == 0 ) {
        return ( CAT_INVALID_ARGUMENT );
    }

    status = getLocalZone();
    if ( status != 0 ) {
        return( status );
    }

    /* Get the resource id; use rescId=0 for 'total' */
    rescId = 0;
    if ( strncmp( rescName, "total", 5 ) != 0 ) {
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetQuota SQL 1" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                     &rescId, rescName, localZone, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_RESOURCE );
            }
            _rollback( "chlSetQuota" );
            return( status );
        }
    }


    status = parseUserName( name, userName, userZone );
    if ( userZone[0] == '\0' ) {
        strncpy( userZone, localZone, NAME_LEN );
    }

    if ( itype == 1 ) {
        userId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetQuota SQL 2" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                     &userId, userName, userZone, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_USER );
            }
            _rollback( "chlSetQuota" );
            return( status );
        }
    }
    else {
        userId = 0;
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSetQuota SQL 3" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name='rodsgroup'",
                     &userId, userName, userZone, 0, 0, 0, &icss );
        if ( status != 0 ) {
            if ( status == CAT_NO_ROWS_FOUND ) {
                return( CAT_INVALID_GROUP );
            }
            _rollback( "chlSetQuota" );
            return( status );
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
    if ( atol( limit ) > 0 ) {
        getNowStr( myTime );
        cllBindVars[cllBindVarCount++] = userIdStr;
        cllBindVars[cllBindVarCount++] = rescIdStr;
        cllBindVars[cllBindVarCount++] = limit;
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
            return( status );
        }
    }

    /* Reset the over_quota flags based on previous usage info.  The
       usage info may take a while to set, but setting the OverQuota
       should be quick.  */
    status = setOverQuota( rsComm );
    if ( status != 0 ) {
        _rollback( "chlSetQuota" );
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}


int
chlCheckQuota( rsComm_t *rsComm, char *userName, char *rescName,
               rodsLong_t *userQuota, int *quotaStatus ) {
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

    *userQuota = 0;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlCheckQuota SQL 1" );
    }
    cllBindVars[cllBindVarCount++] = userName;
    cllBindVars[cllBindVarCount++] = userName;
    cllBindVars[cllBindVarCount++] = rescName;

    status = cmlGetFirstRowFromSql( mySQL, &statementNum,
                                    0, &icss );

    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        rodsLog( LOG_NOTICE,
                 "chlCheckQuota - CAT_SUCCESS_BUT_WITH_NO_INFO" );
        *quotaStatus = QUOTA_UNRESTRICTED;
        return( 0 );
    }

    if ( status == CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "chlCheckQuota - CAT_NO_ROWS_FOUND" );
        *quotaStatus = QUOTA_UNRESTRICTED;
        return( 0 );
    }

    if ( status != 0 ) {
        return( status );
    }

#if 0
    for ( i = 0; i < 4; i++ ) {
        rodsLog( LOG_NOTICE, "checkvalue: %s",
                 icss.stmtPtr[statementNum]->resultValue[i] );
    }
#endif

    /* For now, log it */
    rodsLog( LOG_NOTICE, "checkQuota: inUser:%s inResc:%s RescId:%s Quota:%s",
             userName, rescName,
             icss.stmtPtr[statementNum]->resultValue[1],  /* resc_id column */
             icss.stmtPtr[statementNum]->resultValue[3] ); /* quota_over column */

    *userQuota = atoll( icss.stmtPtr[statementNum]->resultValue[3] );
    if ( atoi( icss.stmtPtr[statementNum]->resultValue[1] ) == 0 ) {
        *quotaStatus = QUOTA_GLOBAL;
    }
    else {
        *quotaStatus = QUOTA_RESOURCE;
    }
    cmlFreeStatement( statementNum, &icss ); /* only need the one row */

    return( status );
}

int
chlDelUnusedAVUs( rsComm_t *rsComm ) {
    /*
       Remove any AVUs that are currently not associated with any object.
       This is done as a separate operation for efficiency.  See
       'iadmin h rum'.
    */
    int status;
    status = removeAVUs();

    if ( status == 0 ) {
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
    }
    return( status );

}


/*
 * chlInsRuleTable - Insert  a new iRODS Rule Base table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsRuleTable( rsComm_t *rsComm,
                 char *baseName, char *mapPriorityStr,  char *ruleName,
                 char *ruleHead, char *ruleCondition, char *ruleAction,
                 char *ruleRecovery, char *ruleIdStr, char *myTime ) {
    int status;
    int i;
    rodsLong_t seqNum = -1;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsRuleTable" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    /* first check if the  rule already exists */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsRuleTable SQL 1" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = ruleName;
    cllBindVars[i++] = ruleHead;
    cllBindVars[i++] = ruleCondition;
    cllBindVars[i++] = ruleAction;
    cllBindVars[i++] = ruleRecovery;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select rule_id from R_RULE_MAIN where  rule_base_name = ? and  rule_name = ? and rule_event = ? and rule_condition = ? and rule_body = ? and  rule_recovery = ?",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "chlInsRuleTable cmlGetIntegerValueFromSqlV3 find rule if any failure %d", status );
        return( status );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlInsRuleTable cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlInsRuleTable" );
            return( seqNum );
        }
        snprintf( ruleIdStr, MAX_NAME_LEN, "%lld", seqNum );

        i = 0;
        cllBindVars[i++] = ruleIdStr;
        cllBindVars[i++] = baseName;
        cllBindVars[i++] = ruleName;
        cllBindVars[i++] = ruleHead;
        cllBindVars[i++] = ruleCondition;
        cllBindVars[i++] = ruleAction;
        cllBindVars[i++] = ruleRecovery;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
        }
    }
    else {
        snprintf( ruleIdStr, MAX_NAME_LEN, "%lld", seqNum );
    }
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsRuleTable SQL 3" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = mapPriorityStr;
    cllBindVars[i++] = ruleIdStr;
    cllBindVars[i++] = rsComm->clientUser.userName;
    cllBindVars[i++] = rsComm->clientUser.rodsZone;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_BASE_MAP  (map_base_name, map_priority, rule_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlInsRuleTable cmlExecuteNoAnswerSql Rule Map insert failure %d" , status );

        return( status );
    }

    return( 0 );
}

/*
 * chlInsDvmTable - Insert  a new iRODS DVM table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsDvmTable( rsComm_t *rsComm,
                char *baseName, char *varName, char *action,
                char *var2CMap, char *myTime ) {
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char dvmIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsDvmTable" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    /* first check if the DVM already exists */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsDvmTable SQL 1" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = varName;
    cllBindVars[i++] = action;
    cllBindVars[i++] = var2CMap;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select dvm_id from R_RULE_DVM where  dvm_base_name = ? and  dvm_ext_var_name = ? and  dvm_condition = ? and dvm_int_map_path = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "chlInsDvmTable cmlGetIntegerValueFromSqlV3 find DVM if any failure %d", status );
        return( status );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlInsDvmTable cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlInsDvmTable" );
            return( seqNum );
        }
        snprintf( dvmIdStr, MAX_NAME_LEN, "%lld", seqNum );

        i = 0;
        cllBindVars[i++] = dvmIdStr;
        cllBindVars[i++] = baseName;
        cllBindVars[i++] = varName;
        cllBindVars[i++] = action;
        cllBindVars[i++] = var2CMap;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
        }
    }
    else {
        snprintf( dvmIdStr, MAX_NAME_LEN, "%lld", seqNum );
    }
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsDvmTable SQL 3" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = dvmIdStr;
    cllBindVars[i++] = rsComm->clientUser.userName;
    cllBindVars[i++] = rsComm->clientUser.rodsZone;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_DVM_MAP  (map_dvm_base_name, dvm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlInsDvmTable cmlExecuteNoAnswerSql DVM Map insert failure %d" , status );

        return( status );
    }

    return( 0 );
}



/*
 * chlInsFnmTable - Insert  a new iRODS FNM table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsFnmTable( rsComm_t *rsComm,
                char *baseName, char *funcName,
                char *func2CMap, char *myTime ) {
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char fnmIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsFnmTable" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    /* first check if the FNM already exists */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsFnmTable SQL 1" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = funcName;
    cllBindVars[i++] = func2CMap;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select fnm_id from R_RULE_FNM where  fnm_base_name = ? and  fnm_ext_func_name = ? and  fnm_int_func_name = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "chlInsFnmTable cmlGetIntegerValueFromSqlV3 find FNM if any failure %d", status );
        return( status );
    }
    if ( seqNum < 0 ) {
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlInsFnmTable cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlInsFnmTable" );
            return( seqNum );
        }
        snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );

        i = 0;
        cllBindVars[i++] = fnmIdStr;
        cllBindVars[i++] = baseName;
        cllBindVars[i++] = funcName;
        cllBindVars[i++] = func2CMap;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
        }
    }
    else {
        snprintf( fnmIdStr, MAX_NAME_LEN, "%lld", seqNum );
    }
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsFnmTable SQL 3" );
    }
    i = 0;
    cllBindVars[i++] = baseName;
    cllBindVars[i++] = fnmIdStr;
    cllBindVars[i++] = rsComm->clientUser.userName;
    cllBindVars[i++] = rsComm->clientUser.rodsZone;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVarCount = i;
    status =  cmlExecuteNoAnswerSql(
                  "insert into R_RULE_FNM_MAP  (map_fnm_base_name, fnm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
                  &icss );
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlInsFnmTable cmlExecuteNoAnswerSql FNM Map insert failure %d" , status );

        return( status );
    }

    return( 0 );
}





/*
 * chlInsMsrvcTable - Insert  a new iRODS MSRVC table row (actually in two tables).
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlInsMsrvcTable( rsComm_t *rsComm,
                      char *moduleName, char *msrvcName,
                      char *msrvcSignature,  char *msrvcVersion,
                      char *msrvcHost, char *msrvcLocation,
                      char *msrvcLanguage,  char *msrvcTypeName, char *msrvcStatus,
                      char *myTime ) {
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char msrvcIdStr[MAX_NAME_LEN];
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsMsrvcTable" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    /* first check if the MSRVC already exists */
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlInsMsrvcTable SQL 1" );
    }
    i = 0;
    cllBindVars[i++] = moduleName;
    cllBindVars[i++] = msrvcName;
    cllBindVarCount = i;
    status =  cmlGetIntegerValueFromSqlV3(
                  "select msrvc_id from R_MICROSRVC_MAIN where  msrvc_module_name = ? and  msrvc_name = ? ",
                  &seqNum,
                  &icss );
    if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "chlInsMsrvcTable cmlGetIntegerValueFromSqlV3 find MSRVC if any failure %d", status );
        return( status );
    }
    if ( seqNum < 0 ) { /* No micro-service found */
        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlInsMsrvcTable cmlGetNextSeqVal failure %d",
                     seqNum );
            _rollback( "chlInsMsrvcTable" );
            return( seqNum );
        }
        snprintf( msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum );
        /* inserting in R_MICROSRVC_MAIN */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = msrvcName;
        cllBindVars[i++] = moduleName;
        cllBindVars[i++] = msrvcSignature;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
        }
        /* inserting in R_MICROSRVC_VER */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = msrvcVersion;
        cllBindVars[i++] = msrvcHost;
        cllBindVars[i++] = msrvcLocation;
        cllBindVars[i++] = msrvcLanguage;
        cllBindVars[i++] = msrvcTypeName;
        cllBindVars[i++] = msrvcStatus;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
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
        cllBindVars[i++] = msrvcHost;
        cllBindVars[i++] = msrvcLocation;
        cllBindVarCount = i;
        status =  cmlGetIntegerValueFromSqlV3(
                      "select msrvc_id from R_MICROSRVC_VER where  msrvc_id = ? and  msrvc_host = ? and  msrvc_location = ? ",
                      &seqNum, &icss );
        if ( status != 0 &&  status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "chlInsMsrvcTable cmlGetIntegerValueFromSqlV4 find MSRVC_HOST if any failure %d", status );
            return( status );
        }
        /* insert a new row into version table */
        i = 0;
        cllBindVars[i++] = msrvcIdStr;
        cllBindVars[i++] = msrvcVersion;
        cllBindVars[i++] = msrvcHost;
        cllBindVars[i++] = msrvcLocation;
        cllBindVars[i++] = msrvcLanguage;
        cllBindVars[i++] = msrvcTypeName;
        cllBindVars[i++] = rsComm->clientUser.userName;
        cllBindVars[i++] = rsComm->clientUser.rodsZone;
        cllBindVars[i++] = myTime;
        cllBindVars[i++] = myTime;
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
            return( status );
        }
    }

    return( 0 );
}


/*
 * chlVersionRuleBase - Version out the old base maps with timestamp
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionRuleBase( rsComm_t *rsComm,
                    char *baseName, char *myTime ) {

    int i, status;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionRuleBase" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    i = 0;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = baseName;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionRuleBase SQL 1" );
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_BASE_MAP set map_version = ?, modify_ts = ? where map_base_name = ? and map_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        rodsLog( LOG_NOTICE,
                 "chlVersionRuleBase cmlExecuteNoAnswerSql Rule Map version update  failure %d" , status );
        return( status );

    }

    return( 0 );
}


/*
 * chlVersionDvmBase - Version out the old dvm base maps with timestamp
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionDvmBase( rsComm_t *rsComm,
                   char *baseName, char *myTime ) {
    int i, status;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionDvmBase" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    i = 0;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = baseName;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionDvmBase SQL 1" );
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_DVM_MAP set map_dvm_version = ?, modify_ts = ? where map_dvm_base_name = ? and map_dvm_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        rodsLog( LOG_NOTICE,
                 "chlVersionDvmBase cmlExecuteNoAnswerSql DVM Map version update  failure %d" , status );
        return( status );

    }

    return( 0 );
}


/*
 * chlVersionFnmBase - Version out the old fnm base maps with timestamp
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionFnmBase( rsComm_t *rsComm,
                   char *baseName, char *myTime ) {

    int i, status;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionFnmBase" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    i = 0;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = myTime;
    cllBindVars[i++] = baseName;
    cllBindVarCount = i;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlVersionFnmBase SQL 1" );
    }

    status =  cmlExecuteNoAnswerSql(
                  "update R_RULE_FNM_MAP set map_fnm_version = ?, modify_ts = ? where map_fnm_base_name = ? and map_fnm_version = '0'", &icss );
    if ( status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        rodsLog( LOG_NOTICE,
                 "chlVersionFnmBase cmlExecuteNoAnswerSql FNM Map version update  failure %d" , status );
        return( status );

    }

    return( 0 );
}







int
icatCheckResc( char *rescName ) {
    int status;
    rodsLong_t rescId;
    status = getLocalZone();
    if ( status != 0 ) {
        return( status );
    }

    rescId = 0;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "icatCheckResc SQxL 1" );
    }
    status = cmlGetIntegerValueFromSql(
                 "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                 &rescId, rescName, localZone, 0, 0, 0, &icss );
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return( CAT_INVALID_RESOURCE );
        }
        _rollback( "icatCheckResc" );
    }
    return( status );
}

int
chlAddSpecificQuery( rsComm_t *rsComm, char *sql, char *alias ) {
    int status, i;
    char myTime[50];
    char tsCreateTime[50];
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlAddSpecificQuery" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( strlen( sql ) < 5 ) {
        return( CAT_INVALID_ARGUMENT );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    getNowStr( myTime );

    if ( alias != NULL && strlen( alias ) > 0 ) {
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlAddSpecificQuery SQL 1" );
        }
        status = cmlGetStringValueFromSql(
                     "select create_ts from R_SPECIFIC_QUERY where alias=?",
                     tsCreateTime, 50,
                     alias, "" , "", &icss );
        if ( status == 0 ) {
            i = addRErrorMsg( &rsComm->rError, 0, "Alias is not unique" );
            return( CAT_INVALID_ARGUMENT );
        }
        i = 0;
        cllBindVars[i++] = sql;
        cllBindVars[i++] = alias;
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
        cllBindVars[i++] = sql;
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
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}

int
chlDelSpecificQuery( rsComm_t *rsComm, char *sqlOrAlias ) {
    int status, i;
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlDelSpecificQuery" );
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    i = 0;
    cllBindVars[i++] = sqlOrAlias;
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
        cllBindVars[i++] = sqlOrAlias;
        cllBindVarCount = i;
        status =  cmlExecuteNoAnswerSql(
                      "delete from R_SPECIFIC_QUERY where alias = ?",
                      &icss );
    }

    if ( status != 0 ) {
        rodsLog( LOG_NOTICE,
                 "chlDelSpecificQuery cmlExecuteNoAnswerSql delete failure %d",
                 status );
        return( status );
    }

    status =  cmlExecuteNoAnswerSql( "commit", &icss );
    return( status );
}

/*
    This is the Specific Query, also known as a sql-based query or
    predefined query.  These are some specific queries (admin
    defined/allowed) that can be performed.  The caller provides the SQL
    which must match one that is pre-defined, along with input parameters
    (bind variables) in some cases.  The output is the same as for a
    general-query.
*/
#define MINIMUM_COL_SIZE 50

int
chlSpecificQuery( specificQueryInp_t specificQueryInp, genQueryOut_t *result ) {
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

    int debug = 0;

    icatSessionStruct *icss;

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlSpecificQuery" );
    }

    result->attriCnt = 0;
    result->rowCnt = 0;
    result->totalRowCount = 0;

    currentMaxColSize = 0;

    icss = chlGetRcs();
    if ( icss == NULL ) {
        return( CAT_NOT_OPEN );
    }
#ifdef ADDR_64BITS
    if ( debug ) {
        printf( "icss=%ld\n", ( long int )icss );
    }
#else
    if ( debug ) {
        printf( "icss=%d\n", ( int )icss );
    }
#endif

    if ( specificQueryInp.continueInx == 0 ) {
        if ( specificQueryInp.sql == NULL ) {
            return( CAT_INVALID_ARGUMENT );
        }
        /*
          First check that this SQL is one of the allowed forms.
        */
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSpecificQuery SQL 1" );
        }
        status = cmlGetStringValueFromSql(
                     "select create_ts from R_SPECIFIC_QUERY where sqlStr=?",
                     tsCreateTime, 50,
                     specificQueryInp.sql, "" , "", icss );
        if ( status == CAT_NO_ROWS_FOUND ) {
            int status2;
            if ( logSQL != 0 ) {
                rodsLog( LOG_SQL, "chlSpecificQuery SQL 2" );
            }
            status2 = cmlGetStringValueFromSql(
                          "select sqlStr from R_SPECIFIC_QUERY where alias=?",
                          combinedSQL, sizeof( combinedSQL ),
                          specificQueryInp.sql, "" , "", icss );
            if ( status2 == CAT_NO_ROWS_FOUND ) {
                return( CAT_UNKNOWN_SPECIFIC_QUERY );
            }
            if ( status2 != 0 ) {
                return( status2 );
            }
        }
        else {
            if ( status != 0 ) {
                return( status );
            }
            strncpy( combinedSQL, specificQueryInp.sql, sizeof( combinedSQL ) );
        }

        i = 0;
        while ( specificQueryInp.args[i] != NULL && strlen( specificQueryInp.args[i] ) > 0 ) {
            cllBindVars[cllBindVarCount++] = specificQueryInp.args[i++];
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlSpecificQuery SQL 3" );
        }
        status = cmlGetFirstRowFromSql( combinedSQL, &statementNum,
                                        specificQueryInp.rowOffset, icss );
        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                rodsLog( LOG_NOTICE,
                         "chlSpecificQuery cmlGetFirstRowFromSql failure %d",
                         status );
            }
            return( status );
        }

        result->continueInx = statementNum + 1;
        if ( debug ) {
            printf( "statement number =%d\n", statementNum );
        }
        needToGetNextRow = 0;
    }
    else {
        statementNum = specificQueryInp.continueInx - 1;
        needToGetNextRow = 1;
        if ( specificQueryInp.maxRows <= 0 ) { /* caller is closing out the query */
            status = cmlFreeStatement( statementNum, icss );
            return( status );
        }
    }
    for ( i = 0; i < specificQueryInp.maxRows; i++ ) {
        if ( needToGetNextRow ) {
            status = cmlGetNextRowFromStatement( statementNum, icss );
            if ( status == CAT_NO_ROWS_FOUND ) {
                cmlFreeStatement( statementNum, icss );
                result->continueInx = 0;
                if ( result->rowCnt == 0 ) {
                    return( status );
                } /* NO ROWS; in this
                                                          case a continuation call is finding no more rows */
                return( 0 );
            }
            if ( status < 0 ) {
                return( status );
            }
        }
        needToGetNextRow = 1;

        result->rowCnt++;
        if ( debug ) {
            printf( "result->rowCnt=%d\n", result->rowCnt );
        }
        numOfCols = icss->stmtPtr[statementNum]->numOfCols;
        if ( debug ) {
            printf( "numOfCols=%d\n", numOfCols );
        }
        result->attriCnt = numOfCols;
        result->continueInx = statementNum + 1;

        maxColSize = 0;

        for ( k = 0; k < numOfCols; k++ ) {
            j = strlen( icss->stmtPtr[statementNum]->resultValue[k] );
            if ( maxColSize <= j ) {
                maxColSize = j;
            }
        }
        maxColSize++; /* for the null termination */
        if ( maxColSize < MINIMUM_COL_SIZE ) {
            maxColSize = MINIMUM_COL_SIZE; /* make it a reasonable size */
        }
        if ( debug ) {
            printf( "maxColSize=%d\n", maxColSize );
        }

        if ( i == 0 ) { /* first time thru, allocate and initialize */
            attriTextLen = numOfCols * maxColSize;
            if ( debug ) {
                printf( "attriTextLen=%d\n", attriTextLen );
            }
            totalLen = attriTextLen * specificQueryInp.maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                tResult = ( char * ) malloc( totalLen );
                if ( tResult == NULL ) {
                    return( SYS_MALLOC_ERR );
                }
                memset( tResult, 0, totalLen );
                result->sqlResult[j].attriInx = 0;
                /* In Gen-query this would be set to specificQueryInp.selectInp.inx[j]; */

                result->sqlResult[j].len = maxColSize;
                result->sqlResult[j].value = tResult;
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
            if ( debug ) printf( "Bumping %d to %d\n",
                                     currentMaxColSize, maxColSize );
            attriTextLen = numOfCols * maxColSize;
            if ( debug ) {
                printf( "attriTextLen=%d\n", attriTextLen );
            }
            totalLen = attriTextLen * specificQueryInp.maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                char *cp1, *cp2;
                int k;
                tResult = ( char * ) malloc( totalLen );
                if ( tResult == NULL ) {
                    return( SYS_MALLOC_ERR );
                }
                memset( tResult, 0, totalLen );
                cp1 = result->sqlResult[j].value;
                cp2 = tResult;
                for ( k = 0; k < result->rowCnt; k++ ) {
                    strncpy( cp2, cp1, result->sqlResult[j].len );
                    cp1 += result->sqlResult[j].len;
                    cp2 += maxColSize;
                }
                free( result->sqlResult[j].value );
                result->sqlResult[j].len = maxColSize;
                result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }

        /* Store the current row values into the appropriate spots in
           the attribute string */
        for ( j = 0; j < numOfCols; j++ ) {
            tResult2 = result->sqlResult[j].value; /* ptr to value str */
            tResult2 += currentMaxColSize * ( result->rowCnt - 1 );  /* skip forward
                                                                  for this row */
            strncpy( tResult2, icss->stmtPtr[statementNum]->resultValue[j],
                     currentMaxColSize ); /* copy in the value text */
        }

    }

    result->continueInx = statementNum + 1;  /* the statementnumber but
                                            always >0 */
    return( 0 );

}


/*
 * chlSubstituteResourceHierarchies - Given an old resource hierarchy string and a new one,
 * replaces all r_data_main.resc_hier rows that match the old string with the new one.
 *
 */
int chlSubstituteResourceHierarchies( rsComm_t *rsComm, const char *old_hier, const char *new_hier ) {
    int status = 0;
    char old_hier_partial[MAX_NAME_LEN];
    irods::sql_logger logger( "chlSubstituteResourceHierarchies", logSQL );

    logger.log();

    // =-=-=-=-=-=-=-
    // Sanity and permission checks
    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }
    if ( !rsComm || !old_hier || !new_hier ) {
        return( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH || rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
    }

    // =-=-=-=-=-=-=-
    // String to match partial hierarchies
    snprintf( old_hier_partial, MAX_NAME_LEN, "%s%s%%", old_hier, irods::hierarchy_parser::delimiter().c_str() );

    // =-=-=-=-=-=-=-
    // Update r_data_main
    cllBindVars[cllBindVarCount++] = ( char* )new_hier;
    cllBindVars[cllBindVarCount++] = ( char* )old_hier;
    cllBindVars[cllBindVarCount++] = ( char* )old_hier;
    cllBindVars[cllBindVarCount++] = old_hier_partial;
#if ORA_ICAT // Oracle
    status = cmlExecuteNoAnswerSql( "update R_DATA_MAIN set resc_hier = ? || substr(resc_hier, (length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss );
#else // Postgres and MySQL
    status = cmlExecuteNoAnswerSql( "update R_DATA_MAIN set resc_hier = ? || substring(resc_hier from (char_length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss );
#endif

    // Nothing was modified
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // Roll back if error
    if ( status < 0 ) {
        std::stringstream ss;
        ss << "chlSubstituteResourceHierarchies: cmlExecuteNoAnswerSql update failure " << status;
        irods::log( LOG_NOTICE, ss.str() );
        _rollback( "chlSubstituteResourceHierarchies" );
    }

    return status;
}


/// =-=-=-=-=-=-=-
/// @brief return the distinct object count of a resource in a hierarchy
int chlGetDistinctDataObjCountOnResource(
    const std::string&   _resc_name,
    long long&           _count ) {
    // =-=-=-=-=-=-=-
    // the basic query string
    char query[ MAX_NAME_LEN ];
    std::string base_query = "select count(distinct data_id) from r_data_main where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s';";
    sprintf(
        query,
        base_query.c_str(),
        _resc_name.c_str(), "%",      // root node
        "%", _resc_name.c_str(), "%", // mid node
        "%", _resc_name.c_str() );    // leaf node

    // =-=-=-=-=-=-=-
    // invoke the query
    int statement_num = 0;
    int status = cmlGetFirstRowFromSql(
                     query,
                     &statement_num,
                     0, &icss );
    if ( status != 0 ) {
        return status;
    }

    _count = atol( icss.stmtPtr[ statement_num ]->resultValue[0] );

    return 0;

} // chlGetDistinctDataObjCountOnResource

/// =-=-=-=-=-=-=-
/// @brief return a map of data object who do not
///        appear on a given child resource but who are a
///        member of a given parent resource node
int chlGetDistinctDataObjsMissingFromChildGivenParent(
    const std::string&   _parent,
    const std::string&   _child,
    int                  _limit,
    dist_child_result_t& _results ) {
    // =-=-=-=-=-=-=-
    // the basic query string
    char query[ MAX_NAME_LEN ];
    std::string base_query = "select distinct data_id from r_data_main where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' except ( select distinct data_id from r_data_main where resc_hier like '%s;%s' or resc_hier like '%s;%s;%s' or resc_hier like '%s;%s' ) limit %d;";
    sprintf(
        query,
        base_query.c_str(),
        _parent.c_str(), "%",      // root
        "%", _parent.c_str(), "%", // mid tier
        "%", _parent.c_str(),      // leaf
        _child.c_str(), "%",       // root
        "%", _child.c_str(), "%",  // mid tier
        "%", _child.c_str(),       // leaf
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
            return status;
        }

        _results.push_back( atoi( icss.stmtPtr[ statement_num ]->resultValue[0] ) );

    } // for i

    cmlFreeStatement( statement_num, &icss );

    return 0;

} // chlGetDistinctDataObjsMissingFromChildGivenParent

/*
 * @brief Given a resource, resolves the hierarchy down to said resource
 */
int chlGetHierarchyForResc( const std::string& resc_name, const std::string& zone_name, std::string& hierarchy ) {
    char *current_node;
    char parent[MAX_NAME_LEN];
    int status;


    irods::sql_logger logger( "chlGetHierarchyForResc", logSQL );
    logger.log();

    if ( !icss.status ) {
        return( CATALOG_NOT_CONNECTED );
    }

    hierarchy = resc_name; // Initialize hierarchy string with resource

    current_node = ( char * )resc_name.c_str();
    while ( current_node ) {
        // Ask for parent of current node
        status = cmlGetStringValueFromSql( "select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
                                           parent, MAX_NAME_LEN, current_node, zone_name.c_str(), NULL, &icss );

        if ( status == CAT_NO_ROWS_FOUND ) { // Resource doesn't exist
            return CAT_UNKNOWN_RESOURCE;
        }

        if ( status < 0 ) { // Other error
            return status;
        }

        if ( strlen( parent ) ) {
            hierarchy = parent + irods::hierarchy_parser::delimiter() + hierarchy;        // Add parent to hierarchy string
            current_node = parent;
        }
        else {
            current_node = NULL;
        }
    }

    return 0;
}

int
icatGetTicketUserId( char *userName, char *userIdStr ) {
    char userId[NAME_LEN];
    char userZone[NAME_LEN];
    char zoneToUse[NAME_LEN];
    char userName2[NAME_LEN];
    int status;

    status = getLocalZone();
    if ( status != 0 ) {
        return( status );
    }

    strncpy( zoneToUse, localZone, NAME_LEN );
    status = parseUserName( userName, userName2, userZone );
    if ( userZone[0] != '\0' ) {
        rstrcpy( zoneToUse, userZone, NAME_LEN );
    }

    userId[0] = '\0';
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "icatGetTicketUserId SQL 1 " );
    }
    status = cmlGetStringValueFromSql(
                 "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name!='rodsgroup'",
                 userId, NAME_LEN, userName2, zoneToUse, 0, &icss );
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return( CAT_INVALID_USER );
        }
        return( status );
    }
    strncpy( userIdStr, userId, NAME_LEN );
    return( 0 );
}

int
icatGetTicketGroupId( char *groupName, char *groupIdStr ) {
    char groupId[NAME_LEN];
    char groupZone[NAME_LEN];
    char zoneToUse[NAME_LEN];
    char groupName2[NAME_LEN];
    int status;

    status = getLocalZone();
    if ( status != 0 ) {
        return( status );
    }

    strncpy( zoneToUse, localZone, NAME_LEN );
    status = parseUserName( groupName, groupName2, groupZone );
    if ( groupZone[0] != '\0' ) {
        rstrcpy( zoneToUse, groupZone, NAME_LEN );
    }

    groupId[0] = '\0';
    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "icatGetTicketGroupId SQL 1 " );
    }
    status = cmlGetStringValueFromSql(
                 "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name='rodsgroup'",
                 groupId, NAME_LEN, groupName2, zoneToUse, 0, &icss );
    if ( status != 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            return( CAT_INVALID_GROUP );
        }
        return( status );
    }
    strncpy( groupIdStr, groupId, NAME_LEN );
    return( 0 );
}

char *
convertHostToIp( char *inputName ) {
    struct hostent *myHostent;
    static char ipAddr[50];
    myHostent = gethostbyname( inputName );
    if ( myHostent == NULL || myHostent->h_addrtype != AF_INET ) {
        printf( "unknown hostname: %s\n", inputName );
        return( NULL );
    }
    snprintf( ipAddr, sizeof( ipAddr ), "%s",
              ( char * )inet_ntoa( *( struct in_addr* )( myHostent->h_addr_list[0] ) ) );
    return( ipAddr );
}


/* Administrative operations on a ticket.
   create, modify, and remove.
   ticketString is either the ticket-string or ticket-id.
*/
int chlModTicket( rsComm_t *rsComm, char *opName, char *ticketString,
                  char *arg3, char *arg4, char *arg5 ) {
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
    if ( strcmp( opName, "session" ) == 0 ) {
        if ( strlen( arg3 ) > 0 ) {
            /* for 2 server hops, arg3 is the original client addr */
            status = chlGenQueryTicketSetup( ticketString, arg3 );
            strncpy( mySessionTicket, ticketString, sizeof( mySessionTicket ) );
            strncpy( mySessionClientAddr, arg3, sizeof( mySessionClientAddr ) );
        }
        else {
            /* for direct connections, rsComm has the original client addr */
            status = chlGenQueryTicketSetup( ticketString, rsComm->clientAddr );
            strncpy( mySessionTicket, ticketString, sizeof( mySessionTicket ) );
            strncpy( mySessionClientAddr, rsComm->clientAddr,
                     sizeof( mySessionClientAddr ) );
        }
        status = cmlAudit3( AU_USE_TICKET, "0",
                            rsComm->clientUser.userName,
                            rsComm->clientUser.rodsZone, ticketString, &icss );
        if ( status != 0 ) {
            return( status );
        }
        return( 0 );
    }

    /* create */
    if ( strcmp( opName, "create" ) == 0 ) {
        status = splitPathByKey( arg4,
                                 logicalParentDirName, logicalEndName, '/' );
        if ( strlen( logicalParentDirName ) == 0 ) {
            strcpy( logicalParentDirName, "/" );
            strcpy( logicalEndName, arg4 + 1 );
        }
        status2 = cmlCheckDataObjOnly( logicalParentDirName, logicalEndName,
                                       rsComm->clientUser.userName,
                                       rsComm->clientUser.rodsZone,
                                       ACCESS_OWN, &icss );
        if ( status2 > 0 ) {
            strncpy( objTypeStr, TICKET_TYPE_DATA, sizeof( objTypeStr ) );
            objId = status2;
        }
        else {
            status3 = cmlCheckDir( arg4,   rsComm->clientUser.userName,
                                   rsComm->clientUser.rodsZone,
                                   ACCESS_OWN, &icss );
            if ( status3 == CAT_NO_ROWS_FOUND && status2 == CAT_NO_ROWS_FOUND ) {
                return( CAT_UNKNOWN_COLLECTION );
            }
            if ( status3 < 0 ) {
                return( status3 );
            }
            strncpy( objTypeStr, TICKET_TYPE_COLL, sizeof( objTypeStr ) );
            objId = status3;
        }

        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModTicket SQL 1" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                     &userId, rsComm->clientUser.userName, rsComm->clientUser.rodsZone,
                     0, 0, 0, &icss );
        if ( status != 0 ) {
            return( CAT_INVALID_USER );
        }

        seqNum = cmlGetNextSeqVal( &icss );
        if ( seqNum < 0 ) {
            rodsLog( LOG_NOTICE, "chlModTicket failure %ld",
                     seqNum );
            return( seqNum );
        }
        snprintf( seqNumStr, NAME_LEN, "%lld", seqNum );
        snprintf( objIdStr, NAME_LEN, "%lld", objId );
        snprintf( userIdStr, NAME_LEN, "%lld", userId );
        if ( strncmp( arg3, "write", 5 ) == 0 ) {
            strncpy( ticketType, "write", sizeof( ticketType ) );
        }
        else {
            strncpy( ticketType, "read", sizeof( ticketType ) );
        }
        getNowStr( myTime );
        i = 0;
        cllBindVars[i++] = seqNumStr;
        cllBindVars[i++] = ticketString;
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
            return( status );
        }
        status = cmlAudit3( AU_CREATE_TICKET, seqNumStr,
                            rsComm->clientUser.userName,
                            rsComm->clientUser.rodsZone, ticketString, &icss );
        if ( status != 0 ) {
            return( status );
        }
        status = cmlAudit3( AU_CREATE_TICKET, seqNumStr,
                            rsComm->clientUser.userName,
                            rsComm->clientUser.rodsZone, objIdStr, &icss ); /* target obj */
        if ( status != 0 ) {
            return( status );
        }
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        return( status );
    }

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlModTicket SQL 3" );
    }
    status = cmlGetIntegerValueFromSql(
                 "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                 &userId, rsComm->clientUser.userName, rsComm->clientUser.rodsZone,
                 0, 0, 0, &icss );
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ||
            status == CAT_NO_ROWS_FOUND ) {
        if ( !addRErrorMsg( &rsComm->rError, 0, "Invalid user" ) ) {
        }
        return( CAT_INVALID_USER );
    }
    if ( status < 0 ) {
        return( status );
    }
    snprintf( userIdStr, sizeof userIdStr, "%lld", userId );

    if ( logSQL != 0 ) {
        rodsLog( LOG_SQL, "chlModTicket SQL 4" );
    }
    status = cmlGetIntegerValueFromSql(
                 "select ticket_id from R_TICKET_MAIN where user_id=? and ticket_string=?",
                 &ticketId, userIdStr, ticketString,
                 0, 0, 0, &icss );
    if ( status != 0 ) {
        if ( logSQL != 0 ) {
            rodsLog( LOG_SQL, "chlModTicket SQL 5" );
        }
        status = cmlGetIntegerValueFromSql(
                     "select ticket_id from R_TICKET_MAIN where user_id=? and ticket_id=?",
                     &ticketId, userIdStr, ticketString,
                     0, 0, 0, &icss );
        if ( status != 0 ) {
            return( CAT_TICKET_INVALID );
        }
    }
    snprintf( ticketIdStr, NAME_LEN, "%lld", ticketId );

    /* delete */
    if ( strcmp( opName, "delete" ) == 0 ) {
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
            return( status );
        }
        if ( status != 0 ) {
            rodsLog( LOG_NOTICE,
                     "chlModTicket cmlExecuteNoAnswerSql delete failure %d",
                     status );
            return( status );
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
                            rsComm->clientUser.userName,
                            rsComm->clientUser.rodsZone, ticketString, &icss );
        if ( status != 0 ) {
            return( status );
        }
        status =  cmlExecuteNoAnswerSql( "commit", &icss );
        return( status );
    }

    /* mod */
    if ( strcmp( opName, "mod" ) == 0 ) {
        if ( strcmp( arg3, "uses" ) == 0 ) {
            i = 0;
            cllBindVars[i++] = arg4;
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
                return( status );
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                         status );
                return( status );
            }
            status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                rsComm->clientUser.userName,
                                rsComm->clientUser.rodsZone, "uses", &icss );
            if ( status != 0 ) {
                return( status );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return( status );
        }

        if ( strncmp( arg3, "write", 5 ) == 0 ) {
            if ( strstr( arg3, "file" ) != NULL ) {
                i = 0;
                cllBindVars[i++] = arg4;
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
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "write file",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
            if ( strstr( arg3, "byte" ) != NULL ) {
                i = 0;
                cllBindVars[i++] = arg4;
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
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "write byte",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
        }

        if ( strncmp( arg3, "expir", 5 ) == 0 ) {
            status = checkDateFormat( arg4 );
            if ( status != 0 ) {
                return( status );
            }
            i = 0;
            cllBindVars[i++] = arg4;
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
                return( status );
            }
            if ( status != 0 ) {
                rodsLog( LOG_NOTICE,
                         "chlModTicket cmlExecuteNoAnswerSql update failure %d",
                         status );
                return( status );
            }
            status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                rsComm->clientUser.userName,
                                rsComm->clientUser.rodsZone, "expire",
                                &icss );
            if ( status != 0 ) {
                return( status );
            }
            status =  cmlExecuteNoAnswerSql( "commit", &icss );
            return( status );
        }

        if ( strcmp( arg3, "add" ) == 0 ) {
            if ( strcmp( arg4, "host" ) == 0 ) {
                char *hostIp;
                hostIp = convertHostToIp( arg5 );
                if ( hostIp == NULL ) {
                    return( CAT_HOSTNAME_INVALID );
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
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql insert host failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "add host",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
            if ( strcmp( arg4, "user" ) == 0 ) {
                status = icatGetTicketUserId( arg5, user2IdStr );
                if ( status != 0 ) {
                    return( status );
                }
                i = 0;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = arg5;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 12" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "insert into R_TICKET_ALLOWED_USERS (ticket_id, user_name) values (? , ?)",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql insert user failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "add user",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
            if ( strcmp( arg4, "group" ) == 0 ) {
                status = icatGetTicketGroupId( arg5, user2IdStr );
                if ( status != 0 ) {
                    return( status );
                }
                i = 0;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = arg5;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 13" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "insert into R_TICKET_ALLOWED_GROUPS (ticket_id, group_name) values (? , ?)",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql insert user failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "add group",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
        }
        if ( strcmp( arg3, "remove" ) == 0 ) {
            if ( strcmp( arg4, "host" ) == 0 ) {
                char *hostIp;
                hostIp = convertHostToIp( arg5 );
                if ( hostIp == NULL ) {
                    return( CAT_HOSTNAME_INVALID );
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
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql delete host failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "remove host",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
            if ( strcmp( arg4, "user" ) == 0 ) {
                status = icatGetTicketUserId( arg5, user2IdStr );
                if ( status != 0 ) {
                    return( status );
                }
                i = 0;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = arg5;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 15" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_TICKET_ALLOWED_USERS where ticket_id=? and user_name=?",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql delete user failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "remove user",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
            if ( strcmp( arg4, "group" ) == 0 ) {
                status = icatGetTicketGroupId( arg5, group2IdStr );
                if ( status != 0 ) {
                    return( status );
                }
                i = 0;
                cllBindVars[i++] = ticketIdStr;
                cllBindVars[i++] = arg5;
                cllBindVarCount = i;
                if ( logSQL != 0 ) {
                    rodsLog( LOG_SQL, "chlModTicket SQL 16" );
                }
                status =  cmlExecuteNoAnswerSql(
                              "delete from R_TICKET_ALLOWED_GROUPS where ticket_id=? and group_name=?",
                              &icss );
                if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
                    return( status );
                }
                if ( status != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "chlModTicket cmlExecuteNoAnswerSql delete group failure %d",
                             status );
                    return( status );
                }
                status = cmlAudit3( AU_MOD_TICKET, ticketIdStr,
                                    rsComm->clientUser.userName,
                                    rsComm->clientUser.rodsZone, "remove group",
                                    &icss );
                if ( status != 0 ) {
                    return( status );
                }
                status =  cmlExecuteNoAnswerSql( "commit", &icss );
                return( status );
            }
        }
    }

    return ( CAT_INVALID_ARGUMENT );
}
