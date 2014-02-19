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
// irods includes
#include "icatHighLevelRoutines.hpp"
#include "readServerConfig.hpp"
#include "icatStructs.hpp"
#include "irods_error.hpp"
#include "irods_database_object.hpp"
#include "irods_database_factory.hpp"
#include "irods_database_manager.hpp"
#include "irods_database_constants.hpp"
#include "irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <boost/regex.hpp>

// =-=-=-=-=-=-=-
// debug flag used across all icat fcns
int logSQL = 0;
int logSQLGenQuery = 0;
int logSQLGenUpdate = 0;
int logSQL_CML = 0;
int auditEnabled = 0;

// =-=-=-=-=-=-=-
// holds the flavor of the catalog for the
// lifetime of the agent
static std::string database_plugin_type;

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
        logSQLGenQuery = 1;//chlDebugGenQuery( 1 );
        logSQLGenUpdate = 1;//chlDebugGenUpdate( 1 );
        logSQL_CML = 2;
        auditEnabled = 1;//cmlDebug( 2 );
    }
    else {
        logSQL = 0;
        logSQLGenQuery = 1;//chlDebugGenQuery( 0 );
        logSQLGenUpdate = 1;//chlDebugGenUpdate( 0 );
        logSQL_CML = 0;
        auditEnabled = 0;//cmlDebug( 0 );
    }

    return( 0 );

} // chlDebug

/// =-=-=-=-=-=-=-
/// @brief Open a connection to the database.  This has to be called first.
///        The server/agent and Rule-Engine Server call this when initializing.
int chlOpen() {
//    // =-=-=-=-=-=-=-
//    // check incoming params
//    if ( !_cfg ) {
//        rodsLog(
//            LOG_ERROR,
//            "null config parameter" );
//        return SYS_INVALID_INPUT_PARAM;
//    }

    // =-=-=-=-=-=-=-
    // cache the database type for subsequent calls
//    database_plugin_type = _cfg->catalog_database_type;
    irods::server_properties::getInstance().get_property<std::string>( CATALOG_DATABASE_TYPE_KW, database_plugin_type );

    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
    ret = db->call(
              irods::DATABASE_OP_OPEN,
              ptr );

    // =-=-=-=-=-=-=-
    // call the start operation, as it may have opinions
    // if it fails, close the connection and error out
    ret = db->start_operation();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        chlClose();
        return PLUGIN_ERROR;
    }

    return ret.code();

} // chlOpen

/// =-=-=-=-=-=-=-
///  @brief Close an open connection to the database.
///         Clean up and shutdown the connection.
int chlClose() {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
    // call the stop operation, as it may have opinions
    irods::error ret2 = db->stop_operation();
    if ( !ret2.ok() ) {
        irods::log( PASS( ret2 ) );
        return PLUGIN_ERROR;
    }

    // =-=-=-=-=-=-=-
    // call the close operation on the plugin
    ret = db->call(
              irods::DATABASE_OP_CLOSE,
              ptr );


    return ret.code();

} // chlClose

// =-=-=-=-=-=-=-
//This is used by the icatGeneralUpdate.c functions to get the icss
//structure.  icatGeneralUpdate.c and this (icatHighLevelRoutine.c)
//are actually one module but in two separate source files (as they
//got larger) so this is a 'glue' that binds them together.  So this
//is mostly an 'internal' function too.
int chlGetRcs(
    icatSessionStruct** _icss ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
    ret = db->call< icatSessionStruct** >(
              irods::DATABASE_OP_GET_RCS,
              ptr,
              _icss );

    return ret.code();

} // chlGetRcs

// =-=-=-=-=-=-=-
// External function to return the local zone name.
// Used by icatGeneralQuery.c
int chlGetLocalZone(
    std::string& _zone ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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

    return ret.code();

} // chlGetLocalZone

// =-=-=-=-=-=-=-
//
int chlCheckAndGetObjectID(
    rsComm_t* _comm,
    char*     _type,
    char*     _name,
    char*     _access ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          rsComm_t*,
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_CHECK_AND_GET_OBJ_ID,
              ptr,
              _comm,
              _type,
              _name,
              _access );

    return ret.code();

} // chlCheckAndGetObjectID

/// =-=-=-=-=-=-=-
/// @brief Public function for updating object count on the specified resource by the specified amount
int chlUpdateRescObjCount(
    const std::string& _resc,
    int                _delta ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          const std::string*,
          int > (
              irods::DATABASE_OP_UPDATE_RESC_OBJ_COUNT,
              ptr,
              &_resc,
              _delta );

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
                           database_plugin_type,
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
              irods::DATABASE_OP_MOD_DATA_OBJ_META,
              ptr,
              _comm,
              _data_obj_info,
              _reg_param );

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
                           database_plugin_type,
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
          dataObjInfo_t* > (
              irods::DATABASE_OP_REG_DATA_OBJ,
              ptr,
              _comm,
              _data_obj_info );

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
                           database_plugin_type,
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

    return ret.code();

} // chlRegReplica

/// =-=-=-=-=-=-=-
/// @brief unregDataObj - Unregister a data object
///        Input - rsComm_t *rsComm  - the server handle
///                dataObjInfo_t *dataObjInfo - contains info about the data object.
///                keyValPair_t *condInput - used to specify a admin-mode.
int chlUnregDataObj(
    rsComm_t*      _comm,
    dataObjInfo_t* _data_obj_info,
    keyValPair_t*  _cond_input ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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

    return ret.code();

} // chlModRuleExec

// =-=-=-=-=-=-=-
// delete a delayed rule execution entry
int chlDelRuleExec(
    rsComm_t* _comm,
    char*     _re_id ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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

    return ret.code();

} // chlDelRuleExec

/// =-=-=-=-=-=-=-
/// @brief  This can be called to test other routines, passing in an actual
/// sComm structure.

/// for example, in _rsSomething instead of:
/// status = chlSomething (SomethingInp);
/// change it to this:
/// status = chlTest(rsComm, Something->name);
/// and it tests the chlRegDataObj function.
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

/// =-=-=-=-=-=-=-
/// @brief Adds the child, with context, to the resource all specified in the rescInfo
int chlAddChildResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
                           database_plugin_type,
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

    return ret.code();

} // chlRegResc

/// =-=-=-=-=-=-=-
/// @brief Remove a child from its parent
int chlDelChildResc(
    rsComm_t*   _comm,
    rescInfo_t* _resc_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
    irods::error ret = irods::database_factory( database_plugin_type, db_obj_ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve a plugin for that object
    irods::plugin_ptr db_plug_ptr;
    ret = db_obj_ptr->resolve( irods::DATABASE_INTERFACE, db_plug_ptr );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed to resolve database interface", ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cast plugin and object to db and fco for call
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast <irods::first_class_object > ( db_obj_ptr );
    irods::database_ptr           db = boost::dynamic_pointer_cast <irods::database > ( db_plug_ptr );

    // =-=-=-=-=-=-=-
    // call the operation on the plugin
    ret = db->call <rsComm_t*, const char*, char*, char*, char*, int*, int* > ( irods::DATABASE_OP_CHECK_AUTH, ptr, _comm, _scheme, _challenge, _response,
            _user_name, _user_priv_level, _client_priv_level );

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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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

    return ret.code();

} // chlModRescFreeSpace

// =-=-=-=-=-=-=-
// Register a User, RuleEngine version
int chlRegUserRE(
    rsComm_t*   _comm,
    userInfo_t* _user_info ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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

    return ret.code();

} // chlRegUserRE

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
                           database_plugin_type,
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
          char* > (
              irods::DATABASE_OP_SET_AVU_METADATA,
              ptr,
              _comm,
              _type,
              _name,
              _attribute,
              _new_value,
              _new_unit );

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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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

    return ret.code();

} // chlModAccessControlResc

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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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
                           database_plugin_type,
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

    return ret.code();

} // chlDelToken

// =-=-=-=-=-=-=-
// chlRegServerLoad - Register a new iRODS server load row.
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlRegServerLoad(
    rsComm_t* _comm,
    char*     _host_name,
    char*     _resc_name,
    char*     _cpu_used,
    char*     _mem_used,
    char*     _swap_used,
    char*     _run_q_load,
    char*     _disk_space,
    char*     _net_input,
    char*     _net_output ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char*,
          char* > (
              irods::DATABASE_OP_REG_SERVER_LOAD,
              ptr,
              _comm,
              _host_name,
              _resc_name,
              _cpu_used,
              _mem_used,
              _swap_used,
              _run_q_load,
              _disk_space,
              _net_input,
              _net_output );

    return ret.code();

} // chlRegServerLoad

// =-=-=-=-=-=-=-
// chlPurgeServerLoad - Purge some rows from iRODS server load table
// that are older than secondsAgo seconds ago.
// Input - rsComm_t *rsComm - the server handle,
//    char *secondsAgo (age in seconds).
int chlPurgeServerLoad(
    rsComm_t* _comm,
    char*     _seconds_ago ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_PURGE_SERVER_LOAD,
              ptr,
              _comm,
              _seconds_ago );

    return ret.code();

} // chlPurgeServerLoad

// =-=-=-=-=-=-=-
// chlRegServerLoadDigest - Register a new iRODS server load-digest row.
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlRegServerLoadDigest(
    rsComm_t* _comm,
    char*     _resc_name,
    char*     _load_factor ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_REG_SERVER_LOAD_DIGEST,
              ptr,
              _comm,
              _resc_name,
              _load_factor );

    return ret.code();

} // chlRegServerLoadDigest

// =-=-=-=-=-=-=-
// chlPurgeServerLoadDigest - Purge some rows from iRODS server LoadDigest table
// that are older than secondsAgo seconds ago.
// Input - rsComm_t *rsComm - the server handle,
//    int secondsAgo (age in seconds).
int chlPurgeServerLoadDigest(
    rsComm_t* _comm,
    char*     _seconds_ago ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_PURGE_SERVER_LOAD_DIGEST,
              ptr,
              _comm,
              _seconds_ago );

    return ret.code();

} // chlPurgeServerLoadDigest

int chlCalcUsageAndQuota(
    rsComm_t* _comm ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_CALC_USAGE_AND_QUOTA,
              ptr,
              _comm );

    return ret.code();

} // chlCalcUsageAndQuota

int chlSetQuota(
    rsComm_t* _comm,
    char*     _type,
    char*     _name,
    char*     _resc_name,
    char*     _limit ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_SET_QUOTA,
              ptr,
              _comm,
              _type,
              _name,
              _resc_name,
              _limit );

    return ret.code();

} // chlSetQuota

int chlCheckQuota(
    rsComm_t*   _comm,
    char*       _user_name,
    char*       _resc_name,
    rodsLong_t* _user_quota,
    int*        _quota_status ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          rodsLong_t*,
          int* > (
              irods::DATABASE_OP_CHECK_QUOTA,
              ptr,
              _comm,
              _user_name,
              _resc_name,
              _user_quota,
              _quota_status );

    return ret.code();

} // chlCheckQuota

int
chlDelUnusedAVUs(
    rsComm_t* _comm ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_DEL_UNUSED_AVUS,
              ptr,
              _comm );

    return ret.code();

} // chlDelUnusedAVUs


// =-=-=-=-=-=-=-
// chlInsRuleTable - Insert  a new iRODS Rule Base table row.
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlInsRuleTable(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _map_priority_str,
    char*     _rule_name,
    char*     _rule_head,
    char*     _rule_condition,
    char*     _rule_action,
    char*     _rule_recovery,
    char*     _rule_id_str,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char*,
          char* > (
              irods::DATABASE_OP_INS_RULE_TABLE,
              ptr,
              _comm,
              _base_name,
              _map_priority_str,
              _rule_name,
              _rule_head,
              _rule_condition,
              _rule_action,
              _rule_recovery,
              _rule_id_str,
              _my_time );

    return ret.code();

} // chlInsRuleTable

// =-=-=-=-=-=-=-
// chlInsDvmTable - Insert  a new iRODS DVM table row.
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlInsDvmTable(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _var_name,
    char*     _action,
    char*     _var_2_cmap,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char* > (
              irods::DATABASE_OP_INS_DVM_TABLE,
              ptr,
              _comm,
              _base_name,
              _var_name,
              _action,
              _var_2_cmap,
              _my_time );

    return ret.code();

} // chlInsDVMTable

// =-=-=-=-=-=-=-
// chlInsFnmTable - Insert  a new iRODS FNM table row.
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlInsFnmTable(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _func_name,
    char*     _func_2_cmap,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_INS_FNM_TABLE,
              ptr,
              _comm,
              _base_name,
              _func_name,
              _func_2_cmap,
              _my_time );

    return ret.code();

} // chlInsFnmTable

// =-=-=-=-=-=-=-
// chlInsMsrvcTable - Insert  a new iRODS MSRVC table row (actually in two tables).
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlInsMsrvcTable(
    rsComm_t* _comm,
    char*     _module_name,
    char*     _msrvc_name,
    char*     _msrvc_signature,
    char*     _msrvc_version,
    char*     _msrvc_host,
    char*     _msrvc_location,
    char*     _msrvc_language,
    char*     _msrvc_typeName,
    char*     _msrvc_status,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char*,
          char*,
          char* > (
              irods::DATABASE_OP_INS_MSRVC_TABLE,
              ptr,
              _comm,
              _module_name,
              _msrvc_name,
              _msrvc_signature,
              _msrvc_version,
              _msrvc_host,
              _msrvc_location,
              _msrvc_language,
              _msrvc_typeName,
              _msrvc_status,
              _my_time );

    return ret.code();

} // chlInsMsrvcTable

// =-=-=-=-=-=-=-
// chlVersionRuleBase - Version out the old base maps with timestamp
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlVersionRuleBase(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_VERSION_RULE_BASE,
              ptr,
              _comm,
              _base_name,
              _my_time );

    return ret.code();

} // chlVersionRuleBase


// =-=-=-=-=-=-=-
// chlVersionDvmBase - Version out the old dvm base maps with timestamp
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlVersionDvmBase(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_VERSION_DVM_BASE,
              ptr,
              _comm,
              _base_name,
              _my_time );

    return ret.code();

} // chlVersionDvmBase


// =-=-=-=-=-=-=-
// chlVersionFnmBase - Version out the old fnm base maps with timestamp
// Input - rsComm_t *rsComm  - the server handle,
//    input values.
int chlVersionFnmBase(
    rsComm_t* _comm,
    char*     _base_name,
    char*     _my_time ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_VERSION_FNM_BASE,
              ptr,
              _comm,
              _base_name,
              _my_time );

    return ret.code();

} // chlVersionFnmBase

int chlAddSpecificQuery(
    rsComm_t* _comm,
    char*     _sql,
    char*     _alias ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_ADD_SPECIFIC_QUERY,
              ptr,
              _comm,
              _sql,
              _alias );

    return ret.code();

} // chlAddSpecificQuery

int chlDelSpecificQuery(
    rsComm_t* _comm,
    char*     _sql_or_alias ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_DEL_SPECIFIC_QUERY,
              ptr,
              _comm,
              _sql_or_alias );

    return ret.code();

} // chlDelSpecificQuery

// =-=-=-=-=-=-=-
//  This is the Specific Query, also known as a sql-based query or
//  predefined query.  These are some specific queries (admin
//  defined/allowed) that can be performed.  The caller provides the SQL
//  which must match one that is pre-defined, along with input parameters
//  (bind variables) in some cases.  The output is the same as for a
//  general-query.

int chlSpecificQuery(
    specificQueryInp_t _spec_query_inp,
    genQueryOut_t*     _result ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          specificQueryInp_t*,
          genQueryOut_t* > (
              irods::DATABASE_OP_SPECIFIC_QUERY,
              ptr,
              &_spec_query_inp,
              _result );

    return ret.code();

} // chlSpecificQuery


// =-=-=-=-=-=-=-
// chlSubstituteResourceHierarchies - Given an old resource hierarchy string and a new one,
// replaces all r_data_main.resc_hier rows that match the old string with the new one.
int chlSubstituteResourceHierarchies(
    rsComm_t*   _comm,
    const char* _old_hier,
    const char* _new_hier ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          const char* > (
              irods::DATABASE_OP_SUBSTITUTE_RESOURCE_HIERARCHIES,
              ptr,
              _comm,
              _old_hier,
              _new_hier );

    return ret.code();

} // chlSubstituteResourceHierarchies


/// =-=-=-=-=-=-=-
/// @brief return the distinct object count of a resource in a hierarchy
int chlGetDistinctDataObjCountOnResource(
    const std::string&   _resc_name,
    long long&           _count ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          const char*,
          long long* > (
              irods::DATABASE_OP_GET_DISTINCT_DATA_OBJ_COUNT_ON_RESOURCE,
              ptr,
              _resc_name.c_str(),
              &_count );

    return ret.code();

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
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          const std::string*,
          const std::string*,
          int,
          dist_child_result_t* > (
              irods::DATABASE_OP_GET_DISTINCT_DATA_OBJS_MISSING_FROM_CHILD_GIVEN_PARENT,
              ptr,
              &_parent,
              &_child,
              _limit,
              &_results );

    return ret.code();

} // chlGetDistinctDataObjsMissingFromChildGivenParent

/// =-=-=-=-=-=-=-
/// @brief Given a resource, resolves the hierarchy down to said resource
int chlGetHierarchyForResc(
    const std::string& _resc_name,
    const std::string& _zone_name,
    std::string&       _hierarchy ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          const std::string*,
          const std::string*,
          std::string* > (
              irods::DATABASE_OP_GET_HIERARCHY_FOR_RESC,
              ptr,
              &_resc_name,
              &_zone_name,
              &_hierarchy );

    return ret.code();

} // chlGetHierarchyForResc

/// =-=-=-=-=-=-=-
/// @brief Administrative operations on a ticket.
///        create, modify, and remove.
///        ticketString is either the ticket-string or ticket-id.
int chlModTicket(
    rsComm_t* _comm,
    char*     _op_name,
    char*     _ticket_string,
    char*     _arg3,
    char*     _arg4,
    char*     _arg5 ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
              irods::DATABASE_OP_MOD_TICKET,
              ptr,
              _comm,
              _op_name,
              _ticket_string,
              _arg3,
              _arg4,
              _arg5 );

    return ret.code();

} // chlModTicket

int chlGenQuery(
    genQueryInp_t  _gen_query_inp,
    genQueryOut_t* _result ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          genQueryInp_t*,
          genQueryOut_t* > (
              irods::DATABASE_OP_GEN_QUERY,
              ptr,
              &_gen_query_inp,
              _result );

    return ret.code();

} // chlGenQuery

int chlGenQueryAccessControlSetup(
    char* _user,
    char* _zone,
    char* _host,
    int   _priv,
    int   _control_flag ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char*,
          char*,
          char*,
          int,
          int > (
              irods::DATABASE_OP_GEN_QUERY_ACCESS_CONTROL_SETUP,
              ptr,
              _user,
              _zone,
              _host,
              _priv,
              _control_flag );

    return ret.code();

} // chlGenQueryAccessControlSetup

int chlGenQueryTicketSetup(
    char* _ticket,
    char* _client_addr ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          char*,
          char* > (
              irods::DATABASE_OP_GEN_QUERY_TICKET_SETUP,
              ptr,
              _ticket,
              _client_addr );

    return ret.code();


} // chlGenQueryTicketSetup

int chlGeneralUpdate(
    generalUpdateInp_t _update_inp ) {
    // =-=-=-=-=-=-=-
    // call factory for database object
    irods::database_object_ptr db_obj_ptr;
    irods::error ret = irods::database_factory(
                           database_plugin_type,
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
          generalUpdateInp_t* > (
              irods::DATABASE_OP_GENERAL_UPDATE,
              ptr,
              &_update_inp );

    return ret.code();

} // chlGeneralUpdate

