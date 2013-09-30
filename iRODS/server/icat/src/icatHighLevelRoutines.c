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
// eirods includes
#include "eirods_error.h"
#include "eirods_zone_info.h"
#include "eirods_sql_logger.h"
#include "eirods_string_tokenize.h"
#include "eirods_log.h"
#include "eirods_tmp_string.h"
#include "eirods_children_parser.h"
#include "eirods_stacktrace.h"
#include "eirods_hierarchy_parser.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rods.h"
#include "rcMisc.h"
#include "icatMidLevelRoutines.h"
#include "icatMidLevelHelpers.h"
#include "icatHighLevelRoutines.h"
#include "icatLowLevel.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <boost/regex.hpp>

extern int get64RandomBytes(char *buf);

static char prevChalSig[200]; /* a 'signiture' of the previous
                                 challenge.  This is used as a sessionSigniture on the ICAT server
                                 side.  Also see getSessionSignitureClientside function. */

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
   to be < 1000 to differentiate the row from regular passwords.
   This time, 120 seconds, should be long enough to give the iDrop and
   iDrop-lite applets enough time to download and go through their
   startup sequence.  iDrop and iDrop-lite disconnect when idle to
   reduce the number of open connections and active agents.  */
#define TEMP_PASSWORD_TIME 120
#define TEMP_PASSWORD_MAX_TIME 1000

/* IRODS_PAM_PASSWORD_DEFAULT_TIME (the default iRODS-PAM password
   lifetime) IRODS_PAM_PASSWORD_MIN_TIME must be greater than
   TEMP_PASSWORD_TIME to avoid the possibility that the logic for
   temporary passwords would be applied.  This should be fine as
   IRODS-PAM passwords will typically be valid on the order of a
   couple weeks compared to a couple minutes for temporary one-time
   passwords.
 */
#define IRODS_PAM_PASSWORD_LEN 20

/* The IRODS_PAM_PASSWORD_MIN_TIME must be greater than
   TEMP_PASSWORD_TIME so the logic can deal with each password type
   differently.  If they overlap, SQL errors can result */
#define IRODS_PAM_PASSWORD_MIN_TIME "121"  /* must be > TEMP_PASSWORD_TIME */
#define IRODS_PAM_PASSWORD_MAX_TIME "1209600"    /* two weeks in seconds */
#ifdef PAM_AUTH_NO_EXTEND
#define IRODS_PAM_PASSWORD_DEFAULT_TIME "28800"  /* 8 hours in seconds */
#else
#define IRODS_PAM_PASSWORD_DEFAULT_TIME "1209600" /* two weeks in seconds */
#endif
 
#define PASSWORD_SCRAMBLE_PREFIX ".E_"
#define PASSWORD_KEY_ENV_VAR "irodsPKey"
#define PASSWORD_DEFAULT_KEY "a9_3fker"

#define MAX_HOST_STR 2700

int logSQL=0;

static int _delColl(rsComm_t *rsComm, collInfo_t *collInfo);
static int removeAVUs();

icatSessionStruct icss={0};
char localZone[MAX_NAME_LEN]={""};

int creatingUserByGroupAdmin=0; // JMC - backport 4772

/*
  Enable or disable some debug logging.
  By default this is off.
*/
int chlDebug(char *debugMode) {
    if (strstr(debugMode, "SQL")) {
        logSQL=1;
        chlDebugGenQuery(1);
        chlDebugGenUpdate(1);
        cmlDebug(2);
    }
    else {
        if (strstr(debugMode, "sql")) {
            logSQL=1;
            chlDebugGenQuery(1);
            chlDebugGenUpdate(1);
            cmlDebug(1);
        }
        else {
            logSQL=0;
            chlDebugGenQuery(0);
            chlDebugGenUpdate(0);
            cmlDebug(0);
        }
    }
    return(0);
}

/* 
   Possibly descramble a password (for user passwords stored in the ICAT).
   Called internally, from various chl functions.
*/
static int
icatDescramble(char *pw) {
    char *cp1, *cp2, *cp3;
    int i,len;
    char pw2[MAX_PASSWORD_LEN+10];
    char unscrambled[MAX_PASSWORD_LEN+10];

    len = strlen(PASSWORD_SCRAMBLE_PREFIX);
    cp1=pw;
    cp2=PASSWORD_SCRAMBLE_PREFIX;  /* if starts with this, it is scrambled */
    for (i=0;i<len;i++) {
        if (*cp1++ != *cp2++) {
            return 0;                /* not scrambled, leave as is */
        }
    }
    strncpy(pw2, cp1, MAX_PASSWORD_LEN);
    cp3 = getenv(PASSWORD_KEY_ENV_VAR);
    if (cp3==NULL) {
        cp3 = PASSWORD_DEFAULT_KEY;
    }
    obfDecodeByKey(pw2, cp3, unscrambled);
    strncpy(pw, unscrambled, MAX_PASSWORD_LEN);

    return 0;
}

/* 
   Scramble a password (for user passwords stored in the ICAT).
   Called internally.
*/
static int
icatScramble(char *pw) {
    char *cp1;
    char newPw[MAX_PASSWORD_LEN+10];
    char scrambled[MAX_PASSWORD_LEN+10];

    cp1 = getenv(PASSWORD_KEY_ENV_VAR);
    if (cp1==NULL) {
        cp1 = PASSWORD_DEFAULT_KEY;
    }
    obfEncodeByKey(pw, cp1, scrambled);
    strncpy(newPw, PASSWORD_SCRAMBLE_PREFIX, MAX_PASSWORD_LEN);
    strncat(newPw, scrambled, MAX_PASSWORD_LEN);
    strncpy(pw, newPw, MAX_PASSWORD_LEN);
    return 0;
}

/*
  Open a connection to the database.  This has to be called first.  The
  server/agent and Rule-Engine Server call this when initializing.
*/
int chlOpen(char *DBUser, char *DBpasswd) {
    int i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlOpen");
    strncpy(icss.databaseUsername, DBUser, DB_USERNAME_LEN);
    strncpy(icss.databasePassword, DBpasswd, DB_PASSWORD_LEN);
    i = cmlOpen(&icss);
    if (i != 0) {
        rodsLog(LOG_NOTICE, "chlOpen cmlOpen failure %d",i);
    }
    else {
        icss.status=1;
    }
    return(i);
}

/*
  Close an open connection to the database.  
  Clean up and shutdown the connection.
*/
int chlClose() {
    int i;

    i = cmlClose(&icss);
    if (i == 0) icss.status=0;
    return(i);
}

int chlIsConnected() {
    if (logSQL!=0) rodsLog(LOG_SQL, "chlIsConnected");
    return(icss.status);
}

/*
  This is used by the icatGeneralUpdate.c functions to get the icss
  structure.  icatGeneralUpdate.c and this (icatHighLevelRoutine.c)
  are actually one module but in two separate source files (as they
  got larger) so this is a 'glue' that binds them together.  So this
  is mostly an 'internal' function too.
*/
icatSessionStruct *
chlGetRcs()
{
    if (logSQL!=0) rodsLog(LOG_SQL, "chlGetRcs");
    if (icss.status != 1) {
        return(NULL);
    }
    return(&icss);
}

/*
  Called internally to rollback current transaction after an error.
*/
int
_rollback(const char *functionName) {
    int status;
#if ORA_ICAT
    status = 0; 
#else
    /* This type of rollback is needed for Postgres since the low-level
       now does an automatic 'begin' to create a sql block */
    status =  cmlExecuteNoAnswerSql("rollback", &icss);
    if (status == 0) {
        rodsLog(LOG_NOTICE,
                "%s cmlExecuteNoAnswerSql(rollback) succeeded", functionName);
    }
    else {
        rodsLog(LOG_NOTICE,
                "%s cmlExecuteNoAnswerSql(rollback) failure %d", 
                functionName, status);
    }
#endif

    return(status);
}


/*
  Internal function to return the local zone (which is the default
  zone).  The first time it's called, it gets the zone from the DB and
  subsequent calls just return that value.
*/
int
getLocalZone()
{
    std::string local_zone;
    eirods::error e = eirods::zone_info::get_instance()->get_local_zone(icss, logSQL, local_zone);
    strncpy(localZone, local_zone.c_str(), MAX_NAME_LEN);
    return e.code();
}

/* 
   External function to return the local zone name.
   Used by icatGeneralQuery.c
*/
char *
chlGetLocalZone() {
    getLocalZone();
    return(localZone);
}

/**
 * @brief Updates the specified resources object count by the specified amount
 */
static int
_updateRescObjCount(
    const std::string& _resc_name,
    const std::string& _zone,
    int _amount) {

    int result = 0;
    int status;
    char resc_id[MAX_NAME_LEN];
    char myTime[50];
    eirods::sql_logger logger(__FUNCTION__, logSQL);
    eirods::hierarchy_parser hparse;
    
    resc_id[0] = '\0';
//    logger.log();
    std::stringstream ss;
    if((status = cmlGetStringValueFromSql("select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                          resc_id, MAX_NAME_LEN, _resc_name.c_str(), _zone.c_str(), 0,
                                          &icss)) != 0) {
        if(status == CAT_NO_ROWS_FOUND) {
            result = CAT_INVALID_RESOURCE;
        } else {
            _rollback(__FUNCTION__);
            result = status;
        }
    } else {
        std::stringstream ss;
        ss << "update R_RESC_MAIN set resc_objcount=resc_objcount+";
        ss << _amount;
        ss << ", modify_ts=? where resc_id=?";
        getNowStr(myTime);
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = resc_id;
        if((status = cmlExecuteNoAnswerSql(ss.str().c_str(), &icss)) != 0) {
            std::stringstream ss;
            ss << __FUNCTION__ << " cmlExecuteNoAnswerSql update failure " << status;
            eirods::log(LOG_NOTICE, ss.str());
            _rollback(__FUNCTION__);
            result = status;
        }
    }
    return result;
}

/**
 * @brief Traverses the specified resource hierarchy updating the object counts of each resource
 */
static int
_updateObjCountOfResources(
    const std::string _resc_hier,
    const std::string _zone,
    int _amount) {
    int result = 0;
    eirods::hierarchy_parser hparse;

    hparse.set_string(_resc_hier);
    for(eirods::hierarchy_parser::const_iterator it = hparse.begin();
        result == 0 && it != hparse.end(); ++it) {
        result = _updateRescObjCount(*it, _zone, _amount);
    }
    return result;
}

/**
 * @brief Public function for updating object count on the specified resource by the specified amount
 */
int
chlUpdateRescObjCount(
    const std::string& _resc,
    int _delta) {

    int result = 0;
    int ret;
    if((ret = getLocalZone()) != 0) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to set the local zone.";
        eirods::log(LOG_ERROR, msg.str());
        result = ret;
    } else if((ret = _updateRescObjCount(_resc, localZone, _delta)) != 0) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to update the object count for resource \"" << _resc << "\"";
        eirods::log(LOG_ERROR, msg.str());
        result = ret;
    }
    
    return result;
}

/* 
 * chlModDataObjMeta - Modify the metadata of an existing data object. 
 * Input - rsComm_t *rsComm  - the server handle
 *         dataObjInfo_t *dataObjInfo - contains info about this copy of
 *         a data object.
 *         keyValPair_t *regParam - the keyword/value pair of items to be
 *         modified. Valid keywords are given in char *dataObjCond[] in
 *         rcGlobal.h. 
 *         If the keyword ALL_REPL_STATUS_KW is used
 *         the replStatus of the copy specified by dataObjInfo 
 *         is marked NEWLY_CREATED_COPY and all other copies are
 *         be marked OLD_COPY.  
 */
int chlModDataObjMeta(rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                      keyValPair_t *regParam) {
    int i, j, status, upCols;
    rodsLong_t iVal = 0; // JMC cppcheck - uninit var
    int status2;

    int mode=0;

    char logicalFileName[MAX_NAME_LEN];
    char logicalDirName[MAX_NAME_LEN];
    char *theVal;
    char replNum1[MAX_NAME_LEN];

    char *whereColsAndConds[10];
    char *whereValues[10];
    char idVal[MAX_NAME_LEN];
    int numConditions;
    char oldCopy[NAME_LEN];
    char newCopy[NAME_LEN];
    int adminMode;

    int maxCols=90;
    char *updateCols[90];
    char *updateVals[90];

    /* regParamNames has the argument names (in regParam) that this
       routine understands and colNames has the corresponding column
       names; one for one. */
    int dataTypeIndex=1; /* matches table below for quick check */
    // Using the keyword defines so there is one point of truth - hcj
    char *regParamNames[]={
        REPL_NUM_KW, DATA_TYPE_KW, DATA_SIZE_KW,
        RESC_NAME_KW,FILE_PATH_KW, DATA_OWNER_KW, DATA_OWNER_ZONE_KW, 
        REPL_STATUS_KW, CHKSUM_KW, DATA_EXPIRY_KW,
        DATA_COMMENTS_KW, DATA_CREATE_KW, DATA_MODIFY_KW, RESC_GROUP_NAME_KW,
        DATA_MODE_KW, RESC_HIER_STR_KW, "END"
    };

   /* If you update colNames, be sure to update DATA_EXPIRY_TS_IX if
    * you add items before "data_expiry_ts" and */
    char *colNames[]={
        "data_repl_num", "data_type_name", "data_size",
        "resc_name", "data_path", "data_owner_name", "data_owner_zone",
        "data_is_dirty", "data_checksum", "data_expiry_ts",
        "r_comment", "create_ts", "modify_ts", "resc_group_name",
        "data_mode", "resc_hier"
    };
    int DATA_EXPIRY_TS_IX=9; /* must match index in above colNames table */
    int MODIFY_TS_IX=12;     /* must match index in above colNames table */

    char objIdString[MAX_NAME_LEN];
    char *neededAccess;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta");

    if (regParam == NULL || dataObjInfo == NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    adminMode=0;
    theVal = getValByKey(regParam, IRODS_ADMIN_KW);
    if (theVal != NULL) {
        adminMode=1; 
    }

    /* Set up the updateCols and updateVals arrays */
    for (i=0, j=0; i<maxCols; i++) {
        if (strcmp(regParamNames[i],"END")==0) break;
        theVal = getValByKey(regParam, regParamNames[i]);
        if (theVal != NULL) {
            updateCols[j]=colNames[i];
            updateVals[j]=theVal;
            if (i==DATA_EXPIRY_TS_IX) { /* if data_expiry, make sure it's
                                           in the standard time-stamp
                                           format: "%011d" */
                if (strcmp(colNames[i],"data_expiry_ts") == 0) { /* double check*/
                    if (strlen(theVal) < 11) {
                        static char theVal2[20];
                        time_t myTimeValue;
                        myTimeValue=atoll(theVal);
                        snprintf(theVal2, sizeof theVal2, "%011d", (int)myTimeValue);
                        updateVals[j]=theVal2;
                    }
                }
            }

            if (i==MODIFY_TS_IX) { /* if modify_ts, also make sure it's
                                            in the standard time-stamp
                                            format: "%011d" */
               if (strcmp(colNames[i],"modify_ts") == 0) { /* double check*/
                  if (strlen(theVal) < 11) {
                     static char theVal3[20];
                     time_t myTimeValue;
                     myTimeValue=atoll(theVal);
                     snprintf(theVal3, sizeof theVal3, "%011d", (int)myTimeValue);
                     updateVals[j]=theVal3;
                  }
               }
            }

            j++;

            /* If the datatype is being updated, check that it is valid */
            if (i==dataTypeIndex) {
                status = cmlCheckNameToken("data_type", 
                                           theVal, &icss);
                if (status !=0 ) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Invalid data type specified.";
                    addRErrorMsg (&rsComm->rError, 0, msg.str().c_str());
                    return(CAT_INVALID_DATA_TYPE);
                }
            }
        }
    }
    upCols=j;

    /* If the only field is the chksum then the user only needs read
       access since we can trust that the server-side code is
       calculating it properly and checksum is a system-managed field.
       For example, when doing an irsync the server may calcuate a
       checksum and want to set it in the source copy.
    */
    neededAccess = ACCESS_MODIFY_METADATA;
    if (upCols==1 && strcmp(updateCols[0],"chksum")==0) {
        neededAccess = ACCESS_READ_OBJECT;
    }

    /* If dataExpiry is being updated, user needs to have 
       a greater access permission */
    theVal = getValByKey(regParam, DATA_EXPIRY_KW);
    if (theVal != NULL) {
        neededAccess = ACCESS_DELETE_OBJECT;
    }

    if (dataObjInfo->dataId <= 0) {
        status = splitPathByKey(dataObjInfo->objPath, 
                                logicalDirName, logicalFileName, '/');

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta SQL 1 ");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name=?", &iVal, 
            logicalDirName, 0, 0, 0, 0, &icss);

        if (status != 0) {
            char errMsg[105];
            snprintf(errMsg, 100, "collection '%s' is unknown", 
                     logicalDirName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            _rollback("chlModDataObjMeta");
            return(CAT_UNKNOWN_COLLECTION);
        }
        snprintf(objIdString, MAX_NAME_LEN, "%lld", iVal);

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta SQL 2");
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN where coll_id=? and data_name=?",
            &iVal, objIdString, logicalFileName,  0, 0, 0, &icss);
        if (status != 0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to find file in database by its logical path.";
            addRErrorMsg (&rsComm->rError, 0, msg.str().c_str());
            _rollback("chlModDataObjMeta");
            return(CAT_UNKNOWN_FILE);
        }

        dataObjInfo->dataId = iVal;  /* return it for possible use next time, */
        /* and for use below */
    }

    snprintf(objIdString, MAX_NAME_LEN, "%lld", dataObjInfo->dataId);

    if (adminMode) {
        if (rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }
    else {
        status = cmlCheckDataObjId(objIdString, rsComm->clientUser.userName,
                                   rsComm->clientUser.rodsZone, neededAccess, &icss);
        if (status != 0) {
            theVal = getValByKey(regParam, ACL_COLLECTION_KW);
            if (theVal != NULL && dataObjInfo->objPath != NULL &&
                upCols==1 && strcmp(updateCols[0],"data_path")==0) {
                int len, iVal = 0; // JMC cppcheck - uninit var ( shadows prev decl? )
                /*
                  In this case, the user is doing a 'imv' of a collection but one of the sub-files is not owned by them.  We decided
                  this should be allowed and so we support it via this new ACL_COLLECTION_KW, checking that the ACL_COLLECTION
                  matches the beginning path of the object and that the user has the appropriate access to that collection.  */
                len = strlen(theVal);
                if (strncmp(theVal, dataObjInfo->objPath, len) == 0) {

                    iVal = cmlCheckDir(theVal,
                                       rsComm->clientUser.userName, 
                                       rsComm->clientUser.rodsZone,
                                       ACCESS_OWN, 
                                       &icss);
                }
                if (iVal > 0) status=0; /* Collection was found (id
                                         * returned) & user has access */
            }
            if (status) {
                _rollback("chlModDataObjMeta");
                return(CAT_NO_ACCESS_PERMISSION);
            }
        } 
    }

    whereColsAndConds[0]="data_id=";
    snprintf(idVal, MAX_NAME_LEN, "%lld", dataObjInfo->dataId);
    whereValues[0]=idVal;
    numConditions=1;

    /* This is up here since this is usually called to modify the
     * metadata of a single repl.  If ALL_KW is included, then apply
     * this change to all replicas (by not restricting the update to
     * only one).
     */
    if (getValByKey(regParam, ALL_KW) == NULL) { 
        j = numConditions;
        whereColsAndConds[j]="data_repl_num=";
        snprintf(replNum1, MAX_NAME_LEN, "%d", dataObjInfo->replNum);
        whereValues[j]=replNum1;
        numConditions++;
    }

    mode =0;
    if (getValByKey(regParam, ALL_REPL_STATUS_KW)) { 
        mode=1;
        /* mark this one as NEWLY_CREATED_COPY and others as OLD_COPY */
    }

    status = getLocalZone();
    if( status < 0 ) {
        rodsLog( LOG_ERROR, "chlModObjMeta - failed in getLocalZone with status [%d]", status );
        return status;
    }

    // If we are moving the data object from one resource to another resource, update the object counts for those resources
    // appropriately - hcj
    char* new_resc_hier = getValByKey(regParam, RESC_HIER_STR_KW);
    if(new_resc_hier != NULL) {
        std::stringstream id_stream;
        id_stream << dataObjInfo->dataId;
        std::stringstream repl_stream;
        repl_stream << dataObjInfo->replNum;
        char resc_hier[MAX_NAME_LEN];
        if((status = cmlGetStringValueFromSql("select resc_hier from R_DATA_MAIN where data_id=? and data_repl_num=?",
                                              resc_hier, MAX_NAME_LEN, id_stream.str().c_str(), repl_stream.str().c_str(),
                                              0, &icss)) != 0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the resc hierarchy from object with id: ";
            msg << id_stream.str();
            msg << " and replNum: ";
            msg << repl_stream.str();
            eirods::log(LOG_NOTICE, msg.str());
            return status;
        }
        // TODO - Address this in terms of resource hierarchies
        else if((status = _updateObjCountOfResources(resc_hier, localZone, -1)) != 0) {
            return status;
        } else if((status = _updateObjCountOfResources(new_resc_hier, localZone, +1)) != 0) {
            return status;
        }
    }
    
    if (mode == 0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta SQL 4");
        status = cmlModifySingleTable("R_DATA_MAIN", updateCols, updateVals, 
                                      whereColsAndConds, whereValues, upCols, 
                                      numConditions, &icss);
    } else {
        /* mark this one as NEWLY_CREATED_COPY and others as OLD_COPY */
        j = upCols;
        updateCols[j]="data_is_dirty";
        snprintf(newCopy, NAME_LEN, "%d", NEWLY_CREATED_COPY);
        updateVals[j]=newCopy;
        upCols++;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta SQL 5");
        status = cmlModifySingleTable("R_DATA_MAIN", updateCols, updateVals, 
                                      whereColsAndConds, whereValues, upCols, 
                                      numConditions, &icss);
        if (status == 0) {
            j = numConditions-1;
            whereColsAndConds[j]="data_repl_num!=";
            snprintf(replNum1, MAX_NAME_LEN, "%d", dataObjInfo->replNum);
            whereValues[j]=replNum1;

            updateCols[0]="data_is_dirty";
            snprintf(oldCopy, NAME_LEN, "%d", OLD_COPY);
            updateVals[0]=oldCopy;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModDataObjMeta SQL 6");
            status2 = cmlModifySingleTable("R_DATA_MAIN", updateCols, updateVals,
                                           whereColsAndConds, whereValues, 1,
                                           numConditions, &icss);

            if (status2 != 0 && status2 != CAT_SUCCESS_BUT_WITH_NO_INFO) {
                /* Ignore NO_INFO errors but not others */
                rodsLog(LOG_NOTICE,
                        "chlModDataObjMeta cmlModifySingleTable failure for other replicas %d",
                        status2);
                _rollback("chlModDataObjMeta");
                return(status2);
            }

        }
    }
    if (status != 0) {
        _rollback("chlModDataObjMeta");
        rodsLog(LOG_NOTICE,
                "chlModDataObjMeta cmlModifySingleTable failure %d",
                status);
        return(status);
    }

    if ( !(dataObjInfo->flags & NO_COMMIT_FLAG) ) {
        status =  cmlExecuteNoAnswerSql("commit", &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModDataObjMeta cmlExecuteNoAnswerSql commit failure %d",
                    status);
            return(status);
        }
    }
    return status;
}

/* 
 * chlRegDataObj - Register a new iRODS file (data object)
 * Input - rsComm_t *rsComm  - the server handle
 *         dataObjInfo_t *dataObjInfo - contains info about the data object.
 */
int chlRegDataObj(rsComm_t *rsComm, dataObjInfo_t *dataObjInfo) {
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

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj");
    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 1 ");
    seqNum = cmlGetNextSeqVal(&icss);
    if (seqNum < 0) {
        rodsLog(LOG_NOTICE, "chlRegDataObj cmlGetNextSeqVal failure %d",
                seqNum);
        _rollback("chlRegDataObj");
        return(seqNum);
    }
    snprintf(dataIdNum, MAX_NAME_LEN, "%lld", seqNum);
    dataObjInfo->dataId=seqNum;  /* store as output parameter */

    status = splitPathByKey(dataObjInfo->objPath, 
                            logicalDirName, logicalFileName, '/');


    /* Check that collection exists and user has write permission. 
       At the same time, also get the inherit flag */
    iVal = cmlCheckDirAndGetInheritFlag(logicalDirName, 
                                        rsComm->clientUser.userName,
                                        rsComm->clientUser.rodsZone, 
                                        ACCESS_MODIFY_OBJECT, &inheritFlag, &icss);
    if (iVal < 0) {
        char errMsg[105];
        if (iVal==CAT_UNKNOWN_COLLECTION) {
            snprintf(errMsg, 100, "collection '%s' is unknown", 
                     logicalDirName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }
        if (iVal==CAT_NO_ACCESS_PERMISSION) {
            snprintf(errMsg, 100, "no permission to update collection '%s'", 
                     logicalDirName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }
        return (iVal);
    }
    snprintf(collIdNum, MAX_NAME_LEN, "%lld", iVal);

    /* Make sure no collection already exists by this name */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 4");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where coll_name=?",
        &iVal, 
        dataObjInfo->objPath, 0, 0, 0, 0, &icss);
    if (status == 0) {
        return(CAT_NAME_EXISTS_AS_COLLECTION);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 5");
    status = cmlCheckNameToken("data_type", 
                               dataObjInfo->dataType, &icss);
    if (status !=0 ) {
        return(CAT_INVALID_DATA_TYPE);
    }

    snprintf(dataReplNum, MAX_NAME_LEN, "%d", dataObjInfo->replNum);
    snprintf(dataStatusNum, MAX_NAME_LEN, "%d", dataObjInfo->replStatus);
    snprintf(dataSizeNum, MAX_NAME_LEN, "%lld", dataObjInfo->dataSize);
    getNowStr(myTime);
    cllBindVars[0]=dataIdNum;
    cllBindVars[1]=collIdNum;
    cllBindVars[2]=logicalFileName;
    cllBindVars[3]=dataReplNum;
    cllBindVars[4]=dataObjInfo->version;
    cllBindVars[5]=dataObjInfo->dataType;
    cllBindVars[6]=dataSizeNum;
    cllBindVars[7]=dataObjInfo->rescGroupName;
    cllBindVars[8]=dataObjInfo->rescName;
    cllBindVars[9]=dataObjInfo->rescHier;
    cllBindVars[10]=dataObjInfo->filePath;
    cllBindVars[11]=rsComm->clientUser.userName;
    cllBindVars[12]=rsComm->clientUser.rodsZone;
    cllBindVars[13]=dataStatusNum;
    cllBindVars[14]=dataObjInfo->chksum;
    cllBindVars[15]=dataObjInfo->dataMode;
    cllBindVars[16]=myTime;
    cllBindVars[17]=myTime;
    cllBindVarCount=18;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 6");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_DATA_MAIN (data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_group_name, resc_name, resc_hier, data_path, data_owner_name, data_owner_zone, data_is_dirty, data_checksum, data_mode, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", 
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegDataObj cmlExecuteNoAnswerSql failure %d",status);
        _rollback("chlRegDataObj");
        return(status);
    }

    status = getLocalZone();
    if( status < 0 ) {
        rodsLog( LOG_ERROR, "chlRegDataInfo - failed in getLocalZone with status [%d]", status );
        return status;
    }

    if((status = _updateObjCountOfResources( dataObjInfo->rescHier, localZone, 1)) != 0) {
        return status;
    }
    
    if (inheritFlag) {
        /* If inherit is set (sticky bit), then add access rows for this
           dataobject that match those of the parent collection */
        cllBindVars[0]=dataIdNum;
        cllBindVars[1]=myTime;
        cllBindVars[2]=myTime;
        cllBindVars[3]=collIdNum;
        cllBindVarCount=4;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 7");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select ?, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRegDataObj cmlExecuteNoAnswerSql insert access failure %d",
                    status);
            _rollback("chlRegDataObj");
            return(status);
        }
    }
    else {
        cllBindVars[0]=dataIdNum;
        cllBindVars[1]=rsComm->clientUser.userName;
        cllBindVars[2]=rsComm->clientUser.rodsZone;
        cllBindVars[3]=ACCESS_OWN;
        cllBindVars[4]=myTime;
        cllBindVars[5]=myTime;
        cllBindVarCount=6;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegDataObj SQL 8");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_OBJT_ACCESS values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRegDataObj cmlExecuteNoAnswerSql insert access failure %d",
                    status);
            _rollback("chlRegDataObj");
            return(status);
        }
    }

    status = cmlAudit3(AU_REGISTER_DATA_OBJ, dataIdNum,
                       rsComm->clientUser.userName, 
                       rsComm->clientUser.rodsZone, "", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegDataObj cmlAudit3 failure %d",
                status);
        _rollback("chlRegDataObj");
        return(status);
    }


    if ( !(dataObjInfo->flags & NO_COMMIT_FLAG) ) {
        status =  cmlExecuteNoAnswerSql("commit", &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRegDataObj cmlExecuteNoAnswerSql commit failure %d",
                    status);
            return(status);
        }
    }

    return(0);
}

/* 
 * chlRegReplica - Register a new iRODS replica file (data object)
 * Input - rsComm_t *rsComm  - the server handle
 *         srcDataObjInfo and dstDataObjInfo each contain information
 *         about the object.
 * The src dataId and replNum are used in a query, a few fields are updated
 * from dstDataObjInfo, and a new row inserted.
 */
int chlRegReplica(rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                  dataObjInfo_t *dstDataObjInfo, keyValPair_t *condInput) {
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
    char theColls[]="data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_group_name, resc_name, resc_hier, data_path, data_owner_name, data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts, data_map_id, data_mode, r_comment, create_ts, modify_ts";
    int IX_DATA_REPL_NUM=3;  /* index of data_repl_num in theColls */
    int IX_RESC_GROUP_NAME=7;/* index into theColls */
    int IX_RESC_NAME=8;      /* index into theColls */
    int IX_RESC_HIER=9;
    int IX_DATA_PATH=10;      /* index into theColls */

    int IX_DATA_MODE=18;
    int IX_CREATE_TS=20;
    int IX_MODIFY_TS=21;
    int IX_RESC_NAME2=22;
    int IX_DATA_PATH2=23;
    int IX_DATA_ID2=24;
    int nColumns=25;

    char objIdString[MAX_NAME_LEN];
    char replNumString[MAX_NAME_LEN];
    int adminMode;
    char *theVal;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegReplica");

    adminMode=0;
    if (condInput != NULL) {
        theVal = getValByKey(condInput, IRODS_ADMIN_KW);
        if (theVal != NULL) {
            adminMode=1;
        }
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    status = splitPathByKey(srcDataObjInfo->objPath, 
                            logicalDirName, logicalFileName, '/');

    if (adminMode) {
        if (rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }
    else {
        /* Check the access to the dataObj */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegReplica SQL 1 ");
        status = cmlCheckDataObjOnly(logicalDirName, logicalFileName,
                                     rsComm->clientUser.userName, 
                                     rsComm->clientUser.rodsZone, 
                                     ACCESS_READ_OBJECT, &icss);
        if (status < 0) {
            _rollback("chlRegReplica");
            return(status);
        }
    }

    /* Get the next replica number */
    snprintf(objIdString, MAX_NAME_LEN, "%lld", srcDataObjInfo->dataId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegReplica SQL 2");
    status = cmlGetIntegerValueFromSql(
        "select max(data_repl_num) from R_DATA_MAIN where data_id = ?",
        &iVal, objIdString, 0, 0, 0, 0, &icss);

    if (status != 0) {
        _rollback("chlRegReplica");
        return(status);
    }

    nextReplNum = iVal+1;
    snprintf(nextRepl, sizeof nextRepl, "%d", nextReplNum);
    dstDataObjInfo->replNum = nextReplNum; /* return new replica number */
    snprintf(replNumString, MAX_NAME_LEN, "%d", srcDataObjInfo->replNum);
    snprintf(tSQL, MAX_SQL_SIZE,
             "select %s from R_DATA_MAIN where data_id = ? and data_repl_num = ?",
             theColls);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegReplica SQL 3");
    status = cmlGetOneRowFromSqlV2(tSQL, cVal, nColumns, 
                                   objIdString, replNumString, &icss);
    if (status < 0) {
        _rollback("chlRegReplica");
        return(status);
    }
    statementNumber = status;

    cVal[IX_DATA_REPL_NUM]=nextRepl;
    cVal[IX_RESC_NAME]=dstDataObjInfo->rescName;
    cVal[IX_RESC_HIER]=dstDataObjInfo->rescHier;
    cVal[IX_RESC_GROUP_NAME]=dstDataObjInfo->rescGroupName;
    cVal[IX_DATA_PATH]=dstDataObjInfo->filePath;
    cVal[IX_DATA_MODE]=dstDataObjInfo->dataMode;


    getNowStr(myTime);
    cVal[IX_MODIFY_TS]=myTime;
    cVal[IX_CREATE_TS]=myTime;

    cVal[IX_RESC_NAME2]=dstDataObjInfo->rescName; // JMC - backport 4669
    cVal[IX_DATA_PATH2]=dstDataObjInfo->filePath; // JMC - backport 4669
    cVal[IX_DATA_ID2]=objIdString; // JMC - backport 4669


    for (i=0;i<nColumns;i++) {
        cllBindVars[i]=cVal[i];
    }
    cllBindVarCount = nColumns;
#if (defined ORA_ICAT || defined MY_ICAT) // JMC - backport 4685
    /* MySQL and Oracle */
    snprintf(tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? from DUAL where not exists (select data_id from R_DATA_MAIN where resc_name=? and data_path=? and data_id=?)",
            theColls);
#else  
    /* Postgres */
    snprintf(tSQL, MAX_SQL_SIZE, "insert into R_DATA_MAIN ( %s ) select ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,? where not exists (select data_id from R_DATA_MAIN where resc_name=? and data_path=? and data_id=?)",
            theColls);

#endif            
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegReplica SQL 4");
    status = cmlExecuteNoAnswerSql(tSQL,  &icss);
    if (status < 0) {
        rodsLog(LOG_NOTICE, 
                "chlRegReplica cmlExecuteNoAnswerSql(insert) failure %d",
                status);
        _rollback("chlRegReplica");
        return(status);
    }

    status = getLocalZone();
    if( status < 0 ) {
        rodsLog( LOG_ERROR, "chlRegReplica - failed in getLocalZone with status [%d]", status );
        return status;
    }

    if((status = _updateObjCountOfResources(dstDataObjInfo->rescHier, localZone, +1)) != 0) {
        return status;
    }
    
    cmlFreeStatement(statementNumber, &icss);
    if (status < 0) {
        rodsLog(LOG_NOTICE, "chlRegReplica cmlFreeStatement failure %d", status);
        return(status);
    }

    status = cmlAudit3(AU_REGISTER_DATA_REPLICA, objIdString,
                       rsComm->clientUser.userName, 
                       rsComm->clientUser.rodsZone, nextRepl, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegDataReplica cmlAudit3 failure %d",
                status);
        _rollback("chlRegReplica");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegReplica cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}

/*
 * removeMetaMapAndAVU - remove AVU (user defined metadata) for an object,
 *   the metadata mapping information, if any.  Optionally, also remove
 *   any unused AVUs, if any, if some mapping information was removed.
 * 
 */
void removeMetaMapAndAVU(char *dataObjNumber) {
    char tSQL[MAX_SQL_SIZE];
    int status;
    cllBindVars[0]=dataObjNumber;
    cllBindVarCount=1;
    if (logSQL!=0) rodsLog(LOG_SQL, "removeMetaMapAndAVU SQL 1 ");
    snprintf(tSQL, MAX_SQL_SIZE, 
             "delete from R_OBJT_METAMAP where object_id=?");
    status =  cmlExecuteNoAnswerSql(tSQL, &icss);

/* Note, the status will be CAT_SUCCESS_BUT_WITH_NO_INFO (not 0) if
   there were no rows deleted from R_OBJT_METAMAP, in which case there
   is no need to do the SQL below.
*/
    if (status == 0) {
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

    if (logSQL!=0) rodsLog(LOG_SQL, "removeAVUs SQL 1 ");
    cllBindVarCount=0;

#if ORA_ICAT
    snprintf(tSQL, MAX_SQL_SIZE, 
             "delete from R_META_MAIN where meta_id in (select meta_id from R_META_MAIN minus select meta_id from R_OBJT_METAMAP)");
#elif MY_ICAT
    /* MYSQL does not have 'minus' or 'except' (to my knowledge) so
     * use previous version of the SQL, which is very slow */
    snprintf(tSQL, MAX_SQL_SIZE, 
             "delete from R_META_MAIN where meta_id not in (select meta_id from R_OBJT_METAMAP)"); 
#else
    /* Postgres */
    snprintf(tSQL, MAX_SQL_SIZE, 
             "delete from R_META_MAIN where meta_id in (select meta_id from R_META_MAIN except select meta_id from R_OBJT_METAMAP)");
#endif 
    status =  cmlExecuteNoAnswerSql(tSQL, &icss);
    rodsLog (LOG_NOTICE, "removeAVUs status=%d\n",status);

    return status;
}

/**
 * @brief Returns true if there is only one repl associated with this data object
 */
bool
_dataIsLastRepl(
    dataObjInfo_t* _dataObjInfo) {

    bool result = true;
    int status;
    eirods::sql_logger logger("_dataIsLastRepl", logSQL);
    static const unsigned int length = 30;
    char cVal[length];
    
    logger.log();
    std::stringstream id_stream;
    id_stream << _dataObjInfo->dataId;
    std::string id_string = id_stream.str();
    std::stringstream repl_stream;
    repl_stream << _dataObjInfo->replNum;
    std::string repl_string = repl_stream.str();
    status = cmlGetStringValueFromSql("select data_repl_num from R_DATA_MAIN where data_id=? and data_repl_num!=?",
                                      cVal, length, id_string.c_str(), repl_string.c_str(), 0, &icss);
    if(status != 0) {
        result = false;
    }
    return result;
}

/*
 * unregDataObj - Unregister a data object
 * Input - rsComm_t *rsComm  - the server handle
 *         dataObjInfo_t *dataObjInfo - contains info about the data object.
 *         keyValPair_t *condInput - used to specify a admin-mode.
 */
int chlUnregDataObj (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, 
                     keyValPair_t *condInput) {
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

    dataObjNumber[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    adminMode=0;
    trashMode=0;
    if (condInput != NULL) {
        theVal = getValByKey(condInput, IRODS_ADMIN_KW);
        if (theVal != NULL) {
            adminMode=1;
        }
        theVal = getValByKey(condInput, IRODS_ADMIN_RMTRASH_KW);
        if (theVal != NULL) {
            adminMode=1;
            trashMode=1;
        }
    }

    status = splitPathByKey(dataObjInfo->objPath, 
                            logicalDirName, logicalFileName, '/');


    if (adminMode==0) {
        /* Check the access to the dataObj */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj SQL 1 ");
        status = cmlCheckDataObjOnly(logicalDirName, logicalFileName,
                                     rsComm->clientUser.userName, 
                                     rsComm->clientUser.rodsZone, 
                                     ACCESS_DELETE_OBJECT, &icss);
        if (status < 0) {
            _rollback("chlUnregDataObj");
            return(status);  /* convert long to int */
        }
        snprintf(dataObjNumber, sizeof dataObjNumber, "%lld", status);
    }
    else {
        if (rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        if (trashMode) {
            int len;
            status = getLocalZone();
            if (status != 0) return(status);
            snprintf(checkPath, MAX_NAME_LEN, "/%s/trash", localZone);
            len = strlen(checkPath);
            if (strncmp(checkPath, logicalDirName, len) != 0) {
                addRErrorMsg (&rsComm->rError, 0, 
                              "TRASH_KW but not zone/trash path");
                return(CAT_INVALID_ARGUMENT);
            }
            if (dataObjInfo->dataId > 0) {
                snprintf(dataObjNumber, sizeof dataObjNumber, "%lld",
                         dataObjInfo->dataId);
            }
        }
        else {
            if (dataObjInfo->replNum >= 0 && dataObjInfo->dataId >= 0) {
                /* Check for a different replica */
                snprintf(dataObjNumber, sizeof dataObjNumber, "%lld", 
                         dataObjInfo->dataId);
                snprintf(replNumber, sizeof replNumber, "%d", dataObjInfo->replNum);
                if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj SQL 2");
                status = cmlGetStringValueFromSql(
                    "select data_repl_num from R_DATA_MAIN where data_id=? and data_repl_num!=?",
                    cVal,
                    sizeof cVal,
                    dataObjNumber,
                    replNumber,
                    0,
                    &icss);
                if (status != 0) {
                    addRErrorMsg (&rsComm->rError, 0, 
                                  "This is the last replica, removal by admin not allowed");
                    return(CAT_LAST_REPLICA);
                }
            }
            else {
                addRErrorMsg (&rsComm->rError, 0, 
                              "dataId and replNum required");
                _rollback("chlUnregDataObj");
                return (CAT_INVALID_ARGUMENT);
            }
        }
    }

    // Get the resource name so we can update its data object count later
    std::string resc_hier;
    if(!dataObjInfo->rescHier || strlen(dataObjInfo->rescHier) == 0) {
        if(dataObjInfo->replNum >= 0) {
            snprintf(replNumber, sizeof replNumber, "%d", dataObjInfo->replNum);
            if((status = cmlGetStringValueFromSql("select resc_hier from R_DATA_MAIN where data_id=? and data_repl_num=?",
                                                  cVal, sizeof cVal, dataObjNumber, replNumber, 0, &icss)) != 0) {
                return status;
            }
        } else {
            if((status = cmlGetStringValueFromSql("select resc_hier from R_DATA_MAIN where data_id=?",
                                                  cVal, sizeof cVal, dataObjNumber, 0, 0, &icss)) != 0) {
                return status;
            }
        }
        resc_hier = std::string(cVal);
    } else {
        resc_hier = std::string(dataObjInfo->rescHier);
    }

    cllBindVars[0]=logicalDirName;
    cllBindVars[1]=logicalFileName;
    if (dataObjInfo->replNum >= 0) {
        snprintf(replNumber, sizeof replNumber, "%d", dataObjInfo->replNum);
        cllBindVars[2]=replNumber;
        cllBindVarCount=3;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj SQL 4");
        snprintf(tSQL, MAX_SQL_SIZE, 
                 "delete from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?) and data_name=? and data_repl_num=?");
    }
    else {
        cllBindVarCount=2;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj SQL 5");
        snprintf(tSQL, MAX_SQL_SIZE, 
                 "delete from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?) and data_name=?");
    }
    status =  cmlExecuteNoAnswerSql(tSQL, &icss);
    if (status != 0) {
        if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
            char errMsg[105];
            status = CAT_UNKNOWN_FILE;  /* More accurate, in this case */
            snprintf(errMsg, 100, "data object '%s' is unknown", 
                     logicalFileName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("chlUnregDataObj");
        return(status);
    }

    status = getLocalZone();
    if( status < 0 ) {
        rodsLog( LOG_ERROR, "chlUnRegDataObj - failed in getLocalZone with status [%d]", status );
        return status;
    }

    // update the object count in the resource
    if((status = _updateObjCountOfResources(resc_hier, localZone, -1)) != 0) {
        return status;
    }
    
    /* delete the access rows, if we just deleted the last replica */
    if (dataObjNumber[0]!='\0') {
        cllBindVars[0]=dataObjNumber;
        cllBindVars[1]=dataObjNumber;
        cllBindVarCount=2;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlUnregDataObj SQL 3");
        status = cmlExecuteNoAnswerSql(
            "delete from R_OBJT_ACCESS where object_id=? and not exists (select * from R_DATA_MAIN where data_id=?)", &icss);
        if (status == 0) {
            removeMetaMapAndAVU(dataObjNumber); /* remove AVU metadata, if any */
        }
    }

    /* Audit */
    if (dataObjNumber[0]!='\0') {
        status = cmlAudit3(AU_UNREGISTER_DATA_OBJ, dataObjNumber,
                           rsComm->clientUser.userName, 
                           rsComm->clientUser.rodsZone, "", &icss);
    }
    else {
        status = cmlAudit3(AU_UNREGISTER_DATA_OBJ, "0",
                           rsComm->clientUser.userName, 
                           rsComm->clientUser.rodsZone,
                           dataObjInfo->objPath, &icss);
    }
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlUnregDataObj cmlAudit3 failure %d",
                status);
        _rollback("chlUnregDataObj");
        return(status);
    }

   
    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlUnregDataObj cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(status);

}

/* 
 * chlRegRuleExec - Register a new iRODS delayed rule execution object
 * Input - rsComm_t *rsComm  - the server handle
 *         ruleExecSubmitInp_t *ruleExecSubmitInp - contains info about the
 *             delayed rule.
 */
int chlRegRuleExec(rsComm_t *rsComm, 
                   ruleExecSubmitInp_t *ruleExecSubmitInp) {
    char myTime[50];
    rodsLong_t seqNum;
    char ruleExecIdNum[MAX_NAME_LEN];
    int status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegRuleExec");
    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegRuleExec SQL 1 ");
    seqNum = cmlGetNextSeqVal(&icss);
    if (seqNum < 0) {
        rodsLog(LOG_NOTICE, "chlRegRuleExec cmlGetNextSeqVal failure %d",
                seqNum);
        _rollback("chlRegRuleExec");
        return(seqNum);
    }
    snprintf(ruleExecIdNum, MAX_NAME_LEN, "%lld", seqNum);

    /* store as output parameter */
    strncpy(ruleExecSubmitInp->ruleExecId,ruleExecIdNum, NAME_LEN); 

    getNowStr(myTime);

    cllBindVars[0]=ruleExecIdNum;
    cllBindVars[1]=ruleExecSubmitInp->ruleName;
    cllBindVars[2]=ruleExecSubmitInp->reiFilePath;
    cllBindVars[3]=ruleExecSubmitInp->userName;
    cllBindVars[4]=ruleExecSubmitInp->exeAddress;
    cllBindVars[5]=ruleExecSubmitInp->exeTime;
    cllBindVars[6]=ruleExecSubmitInp->exeFrequency;
    cllBindVars[7]=ruleExecSubmitInp->priority;
    cllBindVars[8]=ruleExecSubmitInp->estimateExeTime;
    cllBindVars[9]=ruleExecSubmitInp->notificationAddr;
    cllBindVars[10]=myTime;
    cllBindVars[11]=myTime;

    cllBindVarCount=12;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegRuleExec SQL 2");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_RULE_EXEC (rule_exec_id, rule_name, rei_file_path, user_name, exe_address, exe_time, exe_frequency, priority, estimated_exe_time, notification_addr, create_ts, modify_ts) values (?,?,?,?,?,?,?,?,?,?,?,?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegRuleExec cmlExecuteNoAnswerSql(insert) failure %d",status);
        _rollback("chlRegRuleExec");
        return(status);

    }
 
    /* Audit */
    status = cmlAudit3(AU_REGISTER_DELAYED_RULE,  ruleExecIdNum,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       ruleExecSubmitInp->ruleName, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegRuleExec cmlAudit3 failure %d",
                status);
        _rollback("chlRegRuleExec");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegRuleExec cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}

/* 
 * chlModRuleExec - Modify the metadata of an existing (delayed) 
 * Rule Execution object. 
 * Input - rsComm_t *rsComm  - the server handle
 *         char *ruleExecId - the id of the object to change
 *         keyValPair_t *regParam - the keyword/value pair of items to be
 *         modified.
 */
int chlModRuleExec(rsComm_t *rsComm, char *ruleExecId,
                   keyValPair_t *regParam) {
    int i, j, status;

    char tSQL[MAX_SQL_SIZE];
    char *theVal;

    int maxCols=90;

    /* regParamNames has the argument names (in regParam) that this
       routine understands and colNames has the corresponding column
       names; one for one. */   
    char *regParamNames[]={ 
        RULE_NAME_KW, RULE_REI_FILE_PATH_KW, RULE_USER_NAME_KW, 
        RULE_EXE_ADDRESS_KW, RULE_EXE_TIME_KW,
        RULE_EXE_FREQUENCY_KW, RULE_PRIORITY_KW, RULE_ESTIMATE_EXE_TIME_KW,
        RULE_NOTIFICATION_ADDR_KW, RULE_LAST_EXE_TIME_KW, 
        RULE_EXE_STATUS_KW,
        "END"
    };
    char *colNames[]={
        "rule_name", "rei_file_path", "user_name",
        "exe_address", "exe_time", "exe_frequency", "priority",
        "estimated_exe_time", "notification_addr", 
        "last_exe_time", "exe_status",
        "create_ts", "modify_ts", 
    };

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModRuleExec");

    if (regParam == NULL || ruleExecId == NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    snprintf(tSQL, MAX_SQL_SIZE, "update R_RULE_EXEC set ");

    for (i=0, j=0; i<maxCols; i++) {
        if (strcmp(regParamNames[i],"END")==0) break;
        theVal = getValByKey(regParam, regParamNames[i]);
        if (theVal != NULL) {
            if (j>0) rstrcat(tSQL, "," , MAX_SQL_SIZE);
            rstrcat(tSQL, colNames[i] , MAX_SQL_SIZE);
            rstrcat(tSQL, "=?", MAX_SQL_SIZE);
            cllBindVars[j++]=theVal;
        }
    }

    if (j == 0) {
        return (CAT_INVALID_ARGUMENT);
    }

    rstrcat(tSQL, "where rule_exec_id=?", MAX_SQL_SIZE);
    cllBindVars[j++]=ruleExecId;
    cllBindVarCount=j;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModRuleExec SQL 1 ");
    status =  cmlExecuteNoAnswerSql(tSQL, &icss);

    if (status != 0) {
        _rollback("chlModRuleExec");
        rodsLog(LOG_NOTICE,
                "chlModRuleExec cmlExecuteNoAnswer(update) failure %d",
                status);
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_MODIFY_DELAYED_RULE,  ruleExecId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       "", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRuleExec cmlAudit3 failure %d",
                status);
        _rollback("chlModRuleExec");
        return(status);
    }


    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRuleExecMeta cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return status;
}

/* delete a delayed rule execution entry */
int chlDelRuleExec(rsComm_t *rsComm, 
                   char *ruleExecId) {
    int status;
    char userName[MAX_NAME_LEN+2];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelRuleExec");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        if (rsComm->proxyUser.authInfo.authFlag == LOCAL_USER_AUTH) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlDelRuleExec SQL 1 ");
            status = cmlGetStringValueFromSql(
                "select user_name from R_RULE_EXEC where rule_exec_id=?",
                userName, MAX_NAME_LEN, ruleExecId, 0, 0, &icss);
            if (strncmp(userName, rsComm->clientUser.userName, MAX_NAME_LEN)
                != 0) {
                return(CAT_NO_ACCESS_PERMISSION);
            }
        }
        else {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }

    cllBindVars[cllBindVarCount++]=ruleExecId;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelRuleExec SQL 2 ");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_RULE_EXEC where rule_exec_id=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelRuleExec delete failure %d",
                status);
        _rollback("chlDelRuleExec");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_DELETE_DELAYED_RULE,  ruleExecId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       "", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelRuleExec cmlAudit3 failure %d",
                status);
        _rollback("chlDelRuleExec");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelRuleExec cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(status);
}



/*
  This can be called to test other routines, passing in an actual
  rsComm structure.

  For example, in _rsSomething instead of:
  status = chlSomething (SomethingInp);
  change it to this:
  status = chlTest(rsComm, Something->name);
  and it tests the chlRegDataObj function.
*/
int chlTest(rsComm_t *rsComm, char *name) {
    dataObjInfo_t dataObjInfo;

    strcpy(dataObjInfo.objPath, name);
    dataObjInfo.replNum=1;
    strcpy(dataObjInfo.version, "12");
    strcpy(dataObjInfo.dataType, "URL");
    dataObjInfo.dataSize=42;

    strcpy(dataObjInfo.rescName, "resc A");

    strcpy(dataObjInfo.filePath, "/scratch/slocal/test1");

    dataObjInfo.replStatus=5;

    return (chlRegDataObj(rsComm, &dataObjInfo));
}

int
_canConnectToCatalog(
    rsComm_t* _rsComm)
{
    int result = 0;
    if (!icss.status) {
        result = CATALOG_NOT_CONNECTED;
    } else if (_rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        result = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    } else if (_rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        result = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    return result;
}

int
_resolveHostName(
    rsComm_t* _rsComm,
    rescInfo_t* _rescInfo)
{
    int result = 0;
    struct hostent *myHostEnt;

    myHostEnt = gethostbyname(_rescInfo->rescLoc);
    if (myHostEnt <= 0) {
        char errMsg[155];
        snprintf(errMsg, 150, 
                 "Warning, resource host address '%s' is not a valid DNS entry, gethostbyname failed.", 
                 _rescInfo->rescLoc);
        addRErrorMsg (&_rsComm->rError, 0, errMsg);
    }
    if (strcmp(_rescInfo->rescLoc, "localhost") == 0) {
        addRErrorMsg( &_rsComm->rError, 0, 
                      "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client.");
    }

    return result;
}

int
_childIsValid(
    const std::string& _new_child) {

    // Lookup the child resource and make sure its parent field is empty
    int result = 0;
    char parent[MAX_NAME_LEN];
    int status;

    // Get the resource name from the child string
    std::string resc_name;
    eirods::children_parser parser;
    parser.set_string(_new_child);
    parser.first_child(resc_name);

    // Get resources parent
    eirods::sql_logger logger("_childIsValid", logSQL);
    logger.log();
    parent[0] = '\0';
    if((status = cmlGetStringValueFromSql("select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
                                          parent, MAX_NAME_LEN, resc_name.c_str(), localZone, 0, &icss)) != 0) {
        if(status == CAT_NO_ROWS_FOUND) {
            std::stringstream ss;
            ss << "Child resource \"" << resc_name << "\" not found";
            eirods::log(LOG_NOTICE, ss.str());
        } else {
            _rollback("_childIsValid");
        }
        result = status;

    } else if(strlen(parent) != 0) {
        // If the resource already has a parent it cannot be added as a child of another one
        std::stringstream ss;
        ss << "Child resource \"" << resc_name << "\" already has a parent \"" << parent << "\"";
        eirods::log(LOG_NOTICE, ss.str());
        result = EIRODS_CHILD_HAS_PARENT;
    }
    return result;
}

int
_updateRescChildren(
    char* _resc_id,
    const std::string& _new_child_string) {

    int result = 0;
    int status;
    char children[MAX_PATH_ALLOWED];
    char myTime[50];
    eirods::sql_logger logger("_updateRescChildren", logSQL);
    
    if((status = cmlGetStringValueFromSql("select resc_children from R_RESC_MAIN where resc_id=?",
                                          children, MAX_PATH_ALLOWED, _resc_id, 0, 0, &icss)) != 0) {
        _rollback("_updateRescChildren");
        result = status;
    } else {
        std::string children_string(children);
        std::stringstream ss;
        if(children_string.empty()) {
            ss << _new_child_string;
        } else {
            ss << children_string << ";" << _new_child_string;
        }
        std::string combined_children = ss.str();

        // have to do this to avoid const issues
        eirods::tmp_string ts(combined_children.c_str());
        getNowStr(myTime);
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = ts.str();
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = _resc_id;
        logger.log();
        if((status = cmlExecuteNoAnswerSql("update R_RESC_MAIN set resc_children=?, modify_ts=? "
                                           "where resc_id=?", &icss)) != 0) {
            std::stringstream ss;
            ss << "_updateRescChildren cmlExecuteNoAnswerSql update failure " << status;
            eirods::log(LOG_NOTICE, ss.str());
            _rollback("_updateRescChildren");
            result = status;
        }
    }
    return result;
}

int
_updateChildParent(
    const std::string& _new_child,
    const std::string& _parent) {

    int result = 0;
    char resc_id[MAX_NAME_LEN];
    char myTime[50];
    eirods::sql_logger logger("_updateChildParent", logSQL);
    int status;
    
    // Get the resource name from the child string
    eirods::children_parser parser;
    std::string child;
    parser.set_string(_new_child);
    parser.first_child(child);

    // Get the resource id for the child resource
    resc_id[0] = '\0';
    if((status = cmlGetStringValueFromSql("select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                          resc_id, MAX_NAME_LEN, child.c_str(), localZone, 0,
                                          &icss)) != 0) {
        if(status == CAT_NO_ROWS_FOUND) {
            result = CAT_INVALID_RESOURCE;
        } else {
            _rollback("_updateChildParent");
            result = status;
        }
    } else {
        // Update the parent for the child resource
        
        // have to do this to get around const
        eirods::tmp_string ts(_parent.c_str());
        getNowStr(myTime);
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = ts.str();
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = resc_id;
        logger.log();
        if((status = cmlExecuteNoAnswerSql("update R_RESC_MAIN set resc_parent=?, modify_ts=? "
                                           "where resc_id=?", &icss)) != 0) {
            std::stringstream ss;
            ss << "_updateChildParent cmlExecuteNoAnswerSql update failure " << status;
            eirods::log(LOG_NOTICE, ss.str());
            _rollback("_updateChildParent");
            result = status;
        }
    }

    return result;
}

eirods::error chlRescObjCount(
    const std::string& _resc_name,
    rodsLong_t & _rtn_obj_count)
{
    eirods::error result = SUCCESS();
    rodsLong_t obj_count = 0;
    int status;
    
    if((status = cmlGetIntegerValueFromSql("select resc_objcount from R_RESC_MAIN where resc_name=?",
                                           &obj_count, _resc_name.c_str(), 0, 0, 0, 0, &icss)) != 0) {
        _rollback(__FUNCTION__);
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to get object count for resource: \"" << _resc_name << "\"";
        result = ERROR(status, msg.str());
    } else {
        _rtn_obj_count = obj_count;
    }

    return result;
}

/**
 * @brief Returns true if the specified resource has associated data objects
 */
bool
_rescHasData(
    const std::string& _resc_name) {

    bool result = false;
    eirods::sql_logger logger("_rescHasData", logSQL);
    int status;
    static const char* func_name = "_rescHasData";
    rodsLong_t obj_count;
    
    logger.log();
    if((status = cmlGetIntegerValueFromSql("select resc_objcount from R_RESC_MAIN where resc_name=?",
                                           &obj_count, _resc_name.c_str(), 0, 0, 0, 0, &icss)) != 0) {
        _rollback(func_name);
    } else {
        if(obj_count > 0) {
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
    const std::string& _child) {

    bool result = true;
    eirods::children_parser parser;
    parser.set_string(_child);
    std::string child;
    parser.first_child(child);
    result = _rescHasData(child);
    return result;
}

/**
 * @brief Adds the child, with context, to the resource all specified in the rescInfo
 */
int
chlAddChildResc(
    rsComm_t* rsComm,
    rescInfo_t* rescInfo)
{
    int result = 0;
    
    int status;
    static const char* func_name = "chlAddChildResc";
    eirods::sql_logger logger(func_name, logSQL);
    std::string new_child_string(rescInfo->rescChildren);
    char resc_id[MAX_NAME_LEN];
    
    logger.log();

    if(!(result = _canConnectToCatalog(rsComm))) {

        if((status = getLocalZone())) {
            result = status;

        } else if (rescInfo->zoneName != NULL && strlen(rescInfo->zoneName) > 0 && strcmp(rescInfo->zoneName, localZone) !=0) {
            addRErrorMsg (&rsComm->rError, 0, 
                          "Currently, resources must be in the local zone");
            result = CAT_INVALID_ZONE;

        } else if(_childHasData(new_child_string)) {
            char errMsg[105];
            snprintf(errMsg, 100, 
                     "resource '%s' contains one or more dataObjects",
                     rescInfo->rescName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            result = CAT_RESOURCE_NOT_EMPTY;
        } else {

            logger.log();

            resc_id[0] = '\0';
            if((status = cmlGetStringValueFromSql("select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                                  resc_id, MAX_NAME_LEN, rescInfo->rescName, localZone, 0,
                                                  &icss)) != 0) {
                if(status == CAT_NO_ROWS_FOUND) {
                    result = CAT_INVALID_RESOURCE;
                } else {
                    _rollback(func_name);
                    result = status;
                }
            } else if((status =_childIsValid(new_child_string)) == 0) {
                if((status = _updateRescChildren(resc_id, new_child_string)) != 0) {
                    result = status;
                } else if((status = _updateChildParent(new_child_string, std::string(rescInfo->rescName))) != 0) {
                    result = status;
                } else {
                    
                    /* Audit */
                    char commentStr[1024]; // this prolly should be better sized
                    snprintf(commentStr, sizeof commentStr, "%s %s", rescInfo->rescName, new_child_string.c_str()); 
                    if((status = cmlAudit3(AU_ADD_CHILD_RESOURCE, resc_id, rsComm->clientUser.userName, rsComm->clientUser.rodsZone,
                                           commentStr, &icss)) != 0) {
                        std::stringstream ss;
                        ss << func_name << " cmlAudit3 failure " << status;
                        eirods::log(LOG_NOTICE, ss.str());
                        _rollback(func_name);
                        result = status;
                    } else if((status =  cmlExecuteNoAnswerSql("commit", &icss)) != 0) {
                        std::stringstream ss;
                        ss << func_name<< " cmlExecuteNoAnswerSql commit failure " << status;
                        eirods::log(LOG_NOTICE, ss.str());
                        result = status;
                    }
                }                
            } else {
                std::string resc_name;
                eirods::children_parser parser;
                parser.set_string(new_child_string);
                parser.first_child(resc_name);
                
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Resource '" << resc_name << "' already has a parent.";
                addRErrorMsg(&rsComm->rError, 0, msg.str().c_str());
                result = status;
            }
        }
    }
    return result;
}

/* register a Resource */
int chlRegResc(rsComm_t *rsComm, 
               rescInfo_t *rescInfo) {
    rodsLong_t seqNum;
    char idNum[MAX_SQL_SIZE];
    int status;
    char myTime[50];
    struct hostent *myHostEnt; // JMC - backport 4597
 
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegResc");
 
    // =-=-=-=-=-=-=-
    // error trap empty resc name 
    if( strlen( rescInfo->rescName ) < 1 ) { 
        addRErrorMsg( &rsComm->rError, 0, "resource name is empty" );
        return CAT_INVALID_RESOURCE_NAME;
    }

    // =-=-=-=-=-=-=-
    // error trap empty resc type 
    if( strlen( rescInfo->rescType ) < 1 ) { 
        addRErrorMsg( &rsComm->rError, 0, "resource type is empty" );
        return CAT_INVALID_RESOURCE_TYPE;
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegResc SQL 1 ");
    seqNum = cmlGetNextSeqVal(&icss);
    if (seqNum < 0) {
        rodsLog(LOG_NOTICE, "chlRegResc cmlGetNextSeqVal failure %d",
                seqNum);
        _rollback("chlRegResc");
        return(seqNum);
    }
    snprintf(idNum, MAX_SQL_SIZE, "%lld", seqNum);

    status = getLocalZone();
    if (status != 0) return(status);

    if (rescInfo->zoneName != NULL && strlen(rescInfo->zoneName) > 0) {
        if (strcmp(rescInfo->zoneName, localZone) !=0) {
            addRErrorMsg (&rsComm->rError, 0, 
                          "Currently, resources must be in the local zone");
            return(CAT_INVALID_ZONE);
        }
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegResc SQL 2");

// =-=-=-=-=-=-=-
// JMC :: resources may now have an empty location if they
//     :: are coordinating nodes
//    if (strlen(rescInfo->rescLoc)<1) {
//        return(CAT_INVALID_RESOURCE_NET_ADDR);
//    }
    // =-=-=-=-=-=-=-
    // if the resource is not the 'empty resource' test it
    if( eirods::EMPTY_RESC_HOST != rescInfo->rescLoc ) {
        // =-=-=-=-=-=-=-
        // JMC - backport 4597
        myHostEnt = gethostbyname(rescInfo->rescLoc);
        if (myHostEnt <= 0) {
            char errMsg[155];
            snprintf(errMsg, 150, 
                     "Warning, resource host address '%s' is not a valid DNS entry, gethostbyname failed.", 
                     rescInfo->rescLoc);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }
        if (strcmp(rescInfo->rescLoc, "localhost") == 0) { // JMC - backport 4650
            addRErrorMsg( &rsComm->rError, 0, 
                          "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client.");
        }
    
    }
    #if 0
    if (false &&                // hcj - disable checking for vault path. this needs to be checked from the plugins
        (strcmp(rescInfo->rescType, "database") !=0) &&
        (strcmp(rescInfo->rescType, "mso") !=0) ) {
        if (strlen(rescInfo->rescVaultPath)<1) {
            return(CAT_INVALID_RESOURCE_VAULT_PATH);
        }
    }
    #endif
      
    status = getLocalZone();
    if (status != 0) return(status);

    getNowStr(myTime);

    cllBindVars[0]=idNum;
    cllBindVars[1]=rescInfo->rescName;
    cllBindVars[2]=localZone;
    cllBindVars[3]=rescInfo->rescType;
    cllBindVars[4]=rescInfo->rescClass;
    cllBindVars[5]=rescInfo->rescLoc;
    cllBindVars[6]=rescInfo->rescVaultPath;
    cllBindVars[7]=myTime;
    cllBindVars[8]=myTime;
    cllBindVars[9]=rescInfo->rescChildren;
    cllBindVars[10]=rescInfo->rescContext;
    cllBindVars[11]=rescInfo->rescParent;
    cllBindVars[12]="0";
    cllBindVarCount=13;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegResc SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, resc_class_name, resc_net, resc_def_path, create_ts, modify_ts, resc_children, resc_context, resc_parent, resc_objcount) values (?,?,?,?,?,?,?,?,?,?,?,?,?)",
        &icss);

    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegResc cmlExectuteNoAnswerSql(insert) failure %d",
                status);
        _rollback("chlRegResc");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_REGISTER_RESOURCE,  idNum,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       rescInfo->rescName, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegResc cmlAudit3 failure %d",
                status);
        _rollback("chlRegResc");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegResc cmlExecuteNoAnswerSql commit failure %d",status);
        return(status);
    }
    return(status);
}

int
_removeRescChild(
    char* _resc_id,
    const std::string& _child_string) {

    int result = 0;
    int status;
    char children[MAX_PATH_ALLOWED];
    char myTime[50];
    eirods::sql_logger logger("_removeRescChild", logSQL);

    // Get the resources current children string
    if((status = cmlGetStringValueFromSql("select resc_children from R_RESC_MAIN where resc_id=?",
                                          children, MAX_PATH_ALLOWED, _resc_id, 0, 0, &icss)) != 0) {
        _rollback("_updateRescChildren");
        result = status;
    } else {

        // Parse the children string
        eirods::children_parser parser;
        eirods::error ret = parser.set_string(children);
        if(!ret.ok()) {
            std::stringstream ss;
            ss << "_removeChildFromResource resource has invalid children string \"" << children << "\'";
            ss << ret.result();
            eirods::log(LOG_NOTICE, ss.str());
            result = CAT_INVALID_CHILD;
        } else {

            // Remove the specified child from the children
            ret = parser.remove_child(_child_string);
            if(!ret.ok()) {
                std::stringstream ss;
                ss << "_removeChildFromResource parent has no child \"" << _child_string << "\'";
                ss << ret.result();
                eirods::log(LOG_NOTICE, ss.str());
                result = CAT_INVALID_CHILD;
            } else {
                // Update the database with the new children string
                
                // have to do this to avoid const issues
                std::string children_string;
                parser.str(children_string);
                eirods::tmp_string tmp_children(children_string.c_str());
                getNowStr(myTime);
                cllBindVarCount = 0;
                cllBindVars[cllBindVarCount++] = tmp_children.str();
                cllBindVars[cllBindVarCount++] = myTime;
                cllBindVars[cllBindVarCount++] = _resc_id;
                logger.log();
                if((status = cmlExecuteNoAnswerSql("update R_RESC_MAIN set resc_children=?, modify_ts=? "
                                                   "where resc_id=?", &icss)) != 0) {
                    std::stringstream ss;
                    ss << "_removeRescChild cmlExecuteNoAnswerSql update failure " << status;
                    eirods::log(LOG_NOTICE, ss.str());
                    _rollback("_removeRescChild");
                    result = status;
                }
            }
        }
    }
    return result;
}

/**
 * @brief Returns true if the specified resource already has children
 */
static bool
_rescHasChildren(
    const std::string _resc_name) {

    bool result = false;
    int status;
    char children[MAX_NAME_LEN];
    eirods::sql_logger logger("_rescHasChildren", logSQL);

    logger.log();
    if((status = cmlGetStringValueFromSql("select resc_children from R_RESC_MAIN where resc_name=?",
                                          children, MAX_NAME_LEN, _resc_name.c_str(), 0, 0, &icss)) !=0) {
        if(status != CAT_NO_ROWS_FOUND) {
            _rollback("_rescHasChildren");
        }
        result = false;
    } else if(strlen(children) != 0) {
        result = true;
    }
    return result;
}

/**
 * @brief Remove a child from its parent
 */
int
chlDelChildResc(
    rsComm_t* rsComm,
    rescInfo_t* rescInfo) {

    eirods::sql_logger logger("chlDelChildResc", logSQL);
    int result = 0;
    int status;
    char resc_id[MAX_NAME_LEN];
    std::string child_string(rescInfo->rescChildren);
    std::string child;
    eirods::children_parser parser;
    parser.set_string(child_string);
    parser.first_child(child);
    
    if(!(result = _canConnectToCatalog(rsComm))) {
        if((status = getLocalZone())) {
            result = status;
        } else if(_rescHasData(child) || _rescHasChildren(child)) {
            char errMsg[105];
            snprintf(errMsg, 100, 
                     "resource '%s' contains one or more dataObjects",
                     child.c_str());
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            result = CAT_RESOURCE_NOT_EMPTY;
        } else {
            logger.log();

            resc_id[0] = '\0';
            if((status = cmlGetStringValueFromSql("select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
                                                  resc_id, MAX_NAME_LEN, rescInfo->rescName, localZone, 0,
                                                  &icss)) != 0) {
                if(status == CAT_NO_ROWS_FOUND) {
                    result = CAT_INVALID_RESOURCE;
                } else {
                    _rollback("chlDelChildResc");
                    result = status;
                }
            } else if((status = _updateChildParent(child, std::string(""))) != 0) {
                result = status;
            } else if((status = _removeRescChild(resc_id, child)) != 0) {
                result = status;
            } else {

                /* Audit */
                char commentStr[1024]; // this prolly should be better sized
                snprintf(commentStr, sizeof commentStr, "%s %s", rescInfo->rescName, child_string.c_str()); 
                if((status = cmlAudit3(AU_DEL_CHILD_RESOURCE, resc_id, rsComm->clientUser.userName, rsComm->clientUser.rodsZone,
                                       commentStr, &icss)) != 0) {
                    std::stringstream ss;
                    ss << "chlDelChildResc cmlAudit3 failure " << status;
                    eirods::log(LOG_NOTICE, ss.str());
                    _rollback("chlDelChildResc");
                    result = status;
                } else if((status =  cmlExecuteNoAnswerSql("commit", &icss)) != 0) {
                    std::stringstream ss;
                    ss << "chlDelChildResc cmlExecuteNoAnswerSql commit failure " << status;
                    eirods::log(LOG_NOTICE, ss.str());
                    result = status;
                }
            }
        }
    }
    return result;
}

bool
_rescHasParentOrChild(
    char* rescId) {

    bool result = false;
    char parent[MAX_NAME_LEN];
    char children[MAX_NAME_LEN];
    int status;
    eirods::sql_logger logger("_rescHasParentOrChild", logSQL);

    logger.log();
    parent[0] = '\0';
    children[0] = '\0';
    if((status = cmlGetStringValueFromSql("select resc_parent from R_RESC_MAIN where resc_id=?",
                                          parent, MAX_NAME_LEN, rescId, 0, 0, &icss)) != 0) {
        if(status == CAT_NO_ROWS_FOUND) {
            std::stringstream ss;
            ss << "Resource \"" << rescId << "\" not found";
            eirods::log(LOG_NOTICE, ss.str());
        } else {
            _rollback("_rescHasParentOrChild");
        }
        result = false;
    } else if(strlen(parent) != 0) {
        result = true;
    } else if((status = cmlGetStringValueFromSql("select resc_children from R_RESC_MAIN where resc_id=?",
                                                 children, MAX_NAME_LEN, rescId, 0, 0, &icss)) !=0) {
        if(status != CAT_NO_ROWS_FOUND) {
            _rollback("_rescHasParentOrChild");
        }
        result = false;
    } else if(strlen(children) != 0) {
        result = true;
    }
    return result;
    
}

/* delete a Resource */
int chlDelResc(rsComm_t *rsComm, rescInfo_t *rescInfo, int _dryrun ) {
          
    int status;
    char rescId[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelResc");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4629
    if (strncmp(rescInfo->rescName, BUNDLE_RESC, strlen(BUNDLE_RESC))==0) {
        char errMsg[155];
        snprintf(errMsg, 150, 
                 "%s is a built-in resource needed for bundle operations.", 
                 BUNDLE_RESC);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_PSEUDO_RESC_MODIFY_DISALLOWED);
    }
    // =-=-=-=-=-=-=-

    if (_rescHasData(rescInfo->rescName)) {
        char errMsg[105];
        snprintf(errMsg, 100, 
                 "resource '%s' contains one or more dataObjects",
                 rescInfo->rescName);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_RESOURCE_NOT_EMPTY);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    /* get rescId for possible audit call; won't be available after delete */
    rescId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelResc SQL 2 ");
    status = cmlGetStringValueFromSql(
        "select resc_id from R_RESC_MAIN where resc_name=?",
        rescId, MAX_NAME_LEN, rescInfo->rescName, 0, 0, &icss);
    if (status != 0) {
        if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
            char errMsg[105];
            snprintf(errMsg, 100, 
                     "resource '%s' does not exist",
                     rescInfo->rescName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("chlDelResc");
        return(status);
    }

    if(_rescHasParentOrChild(rescId)) {
        char errMsg[105];
        snprintf(errMsg, 100, 
                 "resource '%s' has a parent or child",
                 rescInfo->rescName);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_RESOURCE_NOT_EMPTY);
    }
    
    cllBindVars[cllBindVarCount++]=rescInfo->rescName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelResc SQL 3");
    status = cmlExecuteNoAnswerSql(
        "delete from R_RESC_MAIN where resc_name=?",
        &icss);
    if (status != 0) {
        if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
            char errMsg[105];
            snprintf(errMsg, 100, 
                     "resource '%s' does not exist",
                     rescInfo->rescName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("chlDelResc");
        return(status);
    }

    /* Remove it from resource groups, if any */
    cllBindVars[cllBindVarCount++]=rescId;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelResc SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_RESC_GROUP where resc_id=?",
        &icss);
    if (status != 0 && 
        status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlDelResc delete from R_RESC_GROUP failure %d",
                status);
        _rollback("chlDelResc");
        return(status);
    }


    /* Remove associated AVUs, if any */
    removeMetaMapAndAVU(rescId);


    /* Audit */
    status = cmlAudit3(AU_DELETE_RESOURCE,  
                       rescId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       rescInfo->rescName, 
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelResc cmlAudit3 failure %d",
                status);
        _rollback("chlDelResc");
        return(status);
    }

    if( _dryrun ) { // JMC
        _rollback( "chlDelResc" );
        return status;
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelResc cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(status);
}

/* 
   Issue a rollback command.

   If we don't do a commit, the updates will not be saved in the
   database but will still exist during the current connection.  Since
   iadmin connects once and then can issue multiple commands there are
   situations where we need to rollback.

   For example, if the user's zone is wrong the code will first remove the
   home collection and then fail when removing the user and we need to
   rollback or the next attempt will show the collection as missing.

*/
int chlRollback(rsComm_t *rsComm) {
    int status;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRollback - SQL 1 ");
    status =  cmlExecuteNoAnswerSql("rollback", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRollback cmlExecuteNoAnswerSql failure %d",
                status);
    }
    return(status);
}

/*
  Issue a commit command.
  This is called to commit changes to the database.
  Some of the chl functions also commit changes upon success but some
  do not, having the caller (microservice, perhaps) either commit or
  rollback.
*/
int chlCommit(rsComm_t *rsComm) {
    int status;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlCommit - SQL 1 ");
    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlCommit cmlExecuteNoAnswerSql failure %d",
                status);
    }
    return(status);
}

/* Delete a User, Rule Engine version */
int chlDelUserRE(rsComm_t *rsComm, userInfo_t *userInfo) {
    int status;
    char iValStr[200];
    char zoneToUse[MAX_NAME_LEN];
    char userStr[200];
    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    strncpy(zoneToUse, localZone, MAX_NAME_LEN);
    if (strlen(userInfo->rodsZone)>0) {
        strncpy(zoneToUse, userInfo->rodsZone, MAX_NAME_LEN);
    }

    status = parseUserName(userInfo->userName, userName2, zoneName);
    if (zoneName[0]!='\0') {
        rstrcpy(zoneToUse, zoneName, NAME_LEN);
    }

    if (strncmp(rsComm->clientUser.userName, userName2, sizeof(userName2))==0 &&
        strncmp(rsComm->clientUser.rodsZone, zoneToUse, sizeof(zoneToUse))==0) {
       addRErrorMsg (&rsComm->rError, 0, "Cannot remove your own admin account, probably unintended");
       return(CAT_INVALID_USER);
    }
   

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE SQL 1 ");
    status = cmlGetStringValueFromSql(
        "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
        iValStr, 200, userName2, zoneToUse, 0, &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO ||
        status==CAT_NO_ROWS_FOUND) {
        addRErrorMsg (&rsComm->rError, 0, "Invalid user");
        return(CAT_INVALID_USER); 
    }
    if (status != 0) {
        _rollback("chlDelUserRE");
        return(status);
    }

    cllBindVars[cllBindVarCount++]=userName2;
    cllBindVars[cllBindVarCount++]=zoneToUse;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE SQL 2");
    status = cmlExecuteNoAnswerSql(
        "delete from R_USER_MAIN where user_name=? and zone_name=?",
        &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) return(CAT_INVALID_USER); 
    if (status != 0) {
        _rollback("chlDelUserRE");
        return(status);
    }

    cllBindVars[cllBindVarCount++]=iValStr;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE SQL 3");
    status = cmlExecuteNoAnswerSql(
        "delete from R_USER_PASSWORD where user_id=?",
        &icss);
    if (status!=0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        char errMsg[MAX_NAME_LEN+40];
        rodsLog(LOG_NOTICE,
                "chlDelUserRE delete password failure %d",
                status);
        snprintf(errMsg, sizeof errMsg, "Error removing password entry");
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        _rollback("chlDelUserRE");
        return(status);
    }

    /* Remove both the special user_id = group_user_id entry and any
       other access entries for this user (or group) */
    cllBindVars[cllBindVarCount++]=iValStr;
    cllBindVars[cllBindVarCount++]=iValStr;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE SQL 4");
    status = cmlExecuteNoAnswerSql(
        "delete from R_USER_GROUP where user_id=? or group_user_id=?",
        &icss);
    if (status!=0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        char errMsg[MAX_NAME_LEN+40];
        rodsLog(LOG_NOTICE,
                "chlDelUserRE delete user_group entry failure %d",
                status);
        snprintf(errMsg, sizeof errMsg, "Error removing user_group entry");
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        _rollback("chlDelUserRE");
        return(status);
    }

    /* Remove any R_USER_AUTH rows for this user */
    cllBindVars[cllBindVarCount++]=iValStr;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelUserRE SQL 4");
    status = cmlExecuteNoAnswerSql(
        "delete from R_USER_AUTH where user_id=?",
        &icss);
    if (status!=0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        char errMsg[MAX_NAME_LEN+40];
        rodsLog(LOG_NOTICE,
                "chlDelUserRE delete user_auth entries failure %d",
                status);
        snprintf(errMsg, sizeof errMsg, "Error removing user_auth entries");
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        _rollback("chlDelUserRE");
        return(status);
    }

    /* Remove associated AVUs, if any */
    removeMetaMapAndAVU(iValStr);

    /* Audit */
    snprintf(userStr, sizeof userStr, "%s#%s",
             userName2, zoneToUse);
    status = cmlAudit3(AU_DELETE_USER_RE,  
                       iValStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       userStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelUserRE cmlAudit3 failure %d",
                status);
        _rollback("chlDelUserRE");
        return(status);
    }

    return(0);
}

/*
  Register a Collection by the admin.
  There are cases where the irods admin needs to create collections,
  for a new user, for example; thus the create user rule/microservices
  make use of this.
*/
int chlRegCollByAdmin(rsComm_t *rsComm, collInfo_t *collInfo)
{
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

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegCollByAdmin");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH || 
        rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone, 
            "", &icss);
        if (status2 != 0) return(status2);
        if (creatingUserByGroupAdmin==0) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        // =-=-=-=-=-=-=-
    }

    if (collInfo==0) {
        return(CAT_INVALID_ARGUMENT);
    }

    status = splitPathByKey(collInfo->collName, 
                            logicalParentDirName, logicalEndName, '/');

    if (strlen(logicalParentDirName)==0) {
        strcpy(logicalParentDirName, "/");
        strcpy(logicalEndName, collInfo->collName+1);
    }

    /* Check that the parent collection exists */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegCollByAdmin SQL 1 ");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where coll_name=?",
        &iVal, logicalParentDirName, 0, 0, 0, 0, &icss);
    if (status < 0) {
        char errMsg[MAX_NAME_LEN+40];
        if (status == CAT_NO_ROWS_FOUND) {
            snprintf(errMsg, sizeof errMsg,
                     "collection '%s' is unknown, cannot create %s under it",
                     logicalParentDirName, logicalEndName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("chlRegCollByAdmin");
        return(status);
    }

    snprintf(collIdNum, MAX_NAME_LEN, "%d", status);

    /* String to get next sequence item for objects */
    cllNextValueString("R_ObjectID", nextStr, MAX_NAME_LEN);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegCollByAdmin SQL 2");
    snprintf(tSQL, MAX_SQL_SIZE, 
             "insert into R_COLL_MAIN (coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_type, coll_info1, coll_info2, create_ts, modify_ts) values (%s, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
             nextStr);

    getNowStr(myTime);

    status = getLocalZone();
    if (status != 0) return(status);

    /* Parse input name into user and zone */
    status = parseUserName(collInfo->collOwnerName, userName2, zoneName);
    if (zoneName[0]=='\0') {
        rstrcpy(zoneName, localZone, NAME_LEN);
    }

    cllBindVars[cllBindVarCount++]=logicalParentDirName;
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    cllBindVars[cllBindVarCount++]=userName2;
    if (strlen(collInfo->collOwnerZone)>0) {
        cllBindVars[cllBindVarCount++]=collInfo->collOwnerZone;
    }
    else {
        cllBindVars[cllBindVarCount++]=zoneName;
    }
    if (collInfo->collType != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collType;
    }
    else {
        cllBindVars[cllBindVarCount++]=""; 
    }
    if (collInfo->collInfo1 != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collInfo1;
    }
    else {
        cllBindVars[cllBindVarCount++]="";
    }
    if (collInfo->collInfo2 != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collInfo2;
    }
    else {
        cllBindVars[cllBindVarCount++]="";
    }
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegCollByAdmin SQL 3");
    status =  cmlExecuteNoAnswerSql(tSQL,
                                    &icss);
    if (status != 0) {
        char errMsg[105];
        if (status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
            snprintf(errMsg, 100, "Error %d %s",
                     status,
                     "CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME"
                );
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }

        rodsLog(LOG_NOTICE,
                "chlRegCollByAdmin cmlExecuteNoAnswerSQL(insert) failure %d"
                ,status);
        _rollback("chlRegCollByAdmin");
        return(status);
    }

    /* String to get current sequence item for objects */
    cllCurrentValueString("R_ObjectID", currStr, MAX_NAME_LEN);
    snprintf(currStr2, MAX_SQL_SIZE, " %s ", currStr);

    cllBindVars[cllBindVarCount++]=userName2;
    cllBindVars[cllBindVarCount++]=zoneName;
    cllBindVars[cllBindVarCount++]=ACCESS_OWN;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;

    snprintf(tSQL, MAX_SQL_SIZE, 
             "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
             currStr2);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegCollByAdmin SQL 4");
    status =  cmlExecuteNoAnswerSql(tSQL, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegCollByAdmin cmlExecuteNoAnswerSql(insert access) failure %d",
                status);
        _rollback("chlRegCollByAdmin");
        return(status);
    }

    /* Audit */
    status = cmlAudit4(AU_REGISTER_COLL_BY_ADMIN,  
                       currStr2,
                       "",
                       userName2,
                       zoneName,
                       rsComm->clientUser.userName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegCollByAdmin cmlAudit4 failure %d",
                status);
        _rollback("chlRegCollByAdmin");
        return(status);
    }

    return(0);
}

/* 
 * chlRegColl - register a collection
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   collInfo_t *collInfo - generic coll input. Relevant items are:
 *      collName - the collection to be registered, and optionally
 *      collType, collInfo1 and/or collInfo2.
 *   We may need a kevValPair_t sometime, but currently not used.
 */
int chlRegColl(rsComm_t *rsComm, collInfo_t *collInfo) {
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

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    status = splitPathByKey(collInfo->collName, 
                            logicalParentDirName, logicalEndName, '/');

    if (strlen(logicalParentDirName)==0) {
        strcpy(logicalParentDirName, "/");
        strcpy(logicalEndName, collInfo->collName+1);
    }

    /* Check that the parent collection exists and user has write permission,
       and get the collectionID.  Also get the inherit flag */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 1 ");
    status = cmlCheckDirAndGetInheritFlag(logicalParentDirName, 
                                          rsComm->clientUser.userName,
                                          rsComm->clientUser.rodsZone, 
                                          ACCESS_MODIFY_OBJECT, &inheritFlag, &icss);
    if (status < 0) {
        char errMsg[105];
        if (status == CAT_UNKNOWN_COLLECTION) {
            snprintf(errMsg, 100, "collection '%s' is unknown",
                     logicalParentDirName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("chlRegColl");
        return(status);
    }
    snprintf(collIdNum, MAX_NAME_LEN, "%lld", status);

    /* Check that the path is not already a dataObj */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 2");
    status = cmlGetIntegerValueFromSql(
        "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
        &iVal, logicalEndName, collIdNum, 0, 0, 0, &icss);

    if (status == 0) {
        return(CAT_NAME_EXISTS_AS_DATAOBJ);
    }


    /* String to get next sequence item for objects */
    cllNextValueString("R_ObjectID", nextStr, MAX_NAME_LEN);

    getNowStr(myTime);

    cllBindVars[cllBindVarCount++]=logicalParentDirName;
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.userName;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.rodsZone;
    if (collInfo->collType != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collType;
    }
    else {
        cllBindVars[cllBindVarCount++]=""; 
    }
    if (collInfo->collInfo1 != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collInfo1;
    }
    else {
        cllBindVars[cllBindVarCount++]="";
    }
    if (collInfo->collInfo2 != NULL) {
        cllBindVars[cllBindVarCount++]=collInfo->collInfo2;
    }
    else {
        cllBindVars[cllBindVarCount++]="";
    }
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 3");
    snprintf(tSQL, MAX_SQL_SIZE, 
             "insert into R_COLL_MAIN (coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_type, coll_info1, coll_info2, create_ts, modify_ts) values (%s, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
             nextStr);
    status =  cmlExecuteNoAnswerSql(tSQL,
                                    &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegColl cmlExecuteNoAnswerSql(insert) failure %d",status);
        _rollback("chlRegColl");
        return(status);
    }

    /* String to get current sequence item for objects */
    cllCurrentValueString("R_ObjectID", currStr, MAX_NAME_LEN);
    snprintf(currStr2, MAX_SQL_SIZE, " %s ", currStr);

    if (inheritFlag) {
        /* If inherit is set (sticky bit), then add access rows for this
           collection that match those of the parent collection */
        cllBindVars[0]=myTime;
        cllBindVars[1]=myTime;
        cllBindVars[2]=collIdNum;
        cllBindVarCount=3;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 4");
        snprintf(tSQL, MAX_SQL_SIZE, 
                 "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) (select %s, user_id, access_type_id, ?, ? from R_OBJT_ACCESS where object_id = ?)",
                 currStr2);
        status =  cmlExecuteNoAnswerSql(tSQL, &icss);

        if (status == 0) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 5");
#if ORA_ICAT
            char newCollectionID[MAX_NAME_LEN];
            /* 
               For Oracle, we can't use currStr2 string in a where clause so
               do another query to get the new collection id.
            */
            status = cmlGetCurrentSeqVal(&icss);

            if (status > 0) {
                /* And then use it in the where clause for the update */
                snprintf(newCollectionID, MAX_NAME_LEN, "%lld", status);
                cllBindVars[cllBindVarCount++]="1";
                cllBindVars[cllBindVarCount++]=myTime;
                cllBindVars[cllBindVarCount++]=newCollectionID;
                status =  cmlExecuteNoAnswerSql(
                    "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=?",
                    &icss);
            }
#else
            /*
              For Postgres we can, use the currStr2 to get the current id
              and save a SQL interaction.
            */
            cllBindVars[cllBindVarCount++]="1";
            cllBindVars[cllBindVarCount++]=myTime;
            snprintf(tSQL, MAX_SQL_SIZE, 
                     "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=%s",
                     currStr2);
            status =  cmlExecuteNoAnswerSql(tSQL, &icss);
#endif
        }
    }
    else {
        cllBindVars[cllBindVarCount++]=rsComm->clientUser.userName;
        cllBindVars[cllBindVarCount++]=rsComm->clientUser.rodsZone;
        cllBindVars[cllBindVarCount++]=ACCESS_OWN;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        snprintf(tSQL, MAX_SQL_SIZE, 
                 "insert into R_OBJT_ACCESS values (%s, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                 currStr2);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegColl SQL 6");
        status =  cmlExecuteNoAnswerSql(tSQL, &icss);
    }
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegColl cmlExecuteNoAnswerSql(insert access) failure %d",
                status);
        _rollback("chlRegColl");
        return(status);
    }

    /* Audit */
    status = cmlAudit4(AU_REGISTER_COLL,  
                       currStr2,
                       "",
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       collInfo->collName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegColl cmlAudit4 failure %d",
                status);
        _rollback("chlRegColl");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegColl cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(status);
}

/* 
 * chlModColl - modify attributes of a collection
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   collInfo_t *collInfo - generic coll input. Relevant items are:
 *      collName - the collection to be updated, and one or more of:
 *      collType, collInfo1 and/or collInfo2.
 *   We may need a kevValPair_t sometime, but currently not used.
 */
int chlModColl(rsComm_t *rsComm, collInfo_t *collInfo) {
    char myTime[50];
    rodsLong_t status;
    char tSQL[MAX_SQL_SIZE];
    int count;
    rodsLong_t iVal;
    char iValStr[60];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModColl");

    if( NULL == collInfo ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "chlModColl :: null input parameter collInfo" );
        return -1;
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    /* Check that collection exists and user has write permission */
    iVal = cmlCheckDir(collInfo->collName,  rsComm->clientUser.userName,  
                       rsComm->clientUser.rodsZone, 
                       ACCESS_MODIFY_OBJECT, &icss);

    if (iVal < 0) {
        char errMsg[105];
        if (iVal==CAT_UNKNOWN_COLLECTION) {
            snprintf(errMsg, 100, "collection '%s' is unknown", 
                     collInfo->collName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_UNKNOWN_COLLECTION);
        }
        if (iVal==CAT_NO_ACCESS_PERMISSION) {
            snprintf(errMsg, 100, "no permission to update collection '%s'", 
                     collInfo->collName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return (CAT_NO_ACCESS_PERMISSION);
        }
        return(iVal);
    }

    //if (collInfo==0) { // JMC - cppcheck null ref above
    //   return(CAT_INVALID_ARGUMENT);
    //}

    strncpy(tSQL, "update R_COLL_MAIN set ", MAX_SQL_SIZE);
    count=0;
    if (collInfo->collType != NULL && strlen(collInfo->collType)>0) {
        if (strcmp(collInfo->collType,"NULL_SPECIAL_VALUE")==0) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++]="";
        }
        else {
            cllBindVars[cllBindVarCount++]=collInfo->collType;
        }
        strncat(tSQL, "coll_type=? ", MAX_SQL_SIZE);
        count++;
    }
    if (collInfo->collInfo1 != NULL && strlen(collInfo->collInfo1)>0) {
        if (strcmp(collInfo->collInfo1,"NULL_SPECIAL_VALUE")==0) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++]="";
        } else {
            cllBindVars[cllBindVarCount++]=collInfo->collInfo1;
        }
        if (count>0)  strncat(tSQL, ",", MAX_SQL_SIZE);
        strncat(tSQL, "coll_info1=? ", MAX_SQL_SIZE);
        count++;
    }
    if (collInfo->collInfo2 != NULL && strlen(collInfo->collInfo2)>0) {
        if (strcmp(collInfo->collInfo2,"NULL_SPECIAL_VALUE")==0) {
            /* A special value to indicate NULL */
            cllBindVars[cllBindVarCount++]="";
        } else {
            cllBindVars[cllBindVarCount++]=collInfo->collInfo2;
        }
        if (count>0)  strncat(tSQL, ",", MAX_SQL_SIZE);
        strncat(tSQL, "coll_info2=? ", MAX_SQL_SIZE);
        count++;
    }
    if (count==0) return(CAT_INVALID_ARGUMENT);
    getNowStr(myTime);
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    strncat(tSQL, ", modify_ts=? where coll_name=?", MAX_SQL_SIZE);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModColl SQL 1");
    status =  cmlExecuteNoAnswerSql(tSQL,
                                    &icss);

    /* Audit */
    snprintf(iValStr, sizeof iValStr, "%lld", iVal);
    status = cmlAudit3(AU_REGISTER_COLL,  
                       iValStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       collInfo->collName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModColl cmlAudit3 failure %d",
                status);
        return(status);
    }


    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModColl cmlExecuteNoAnswerSQL(update) failure %d", status);
        return(status);
    }
    return(0);
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
eirods::error validate_zone_name( 
    std::string _zone_name ) {
    std::string::iterator itr = std::find_if( _zone_name.begin(),
                                              _zone_name.end(),
                                              allowed_zone_char );
    if( itr != _zone_name.end() ) {
        std::stringstream msg;
        msg << "validate_zone_name failed for zone [";
        msg << _zone_name;
        msg << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    return SUCCESS();

} // validate_zone_name

/* register a Zone */
int chlRegZone(rsComm_t *rsComm, 
               char *zoneName, char *zoneType, char *zoneConnInfo, 
               char *zoneComment) {
    char nextStr[MAX_NAME_LEN];
    char tSQL[MAX_SQL_SIZE];
    int status;
    char myTime[50];

    if( !rsComm || !zoneName || !zoneType || !zoneConnInfo || !zoneComment ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegZone");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (strncmp(zoneType, "remote", 6) != 0) {
        addRErrorMsg (&rsComm->rError, 0, 
                      "Currently, only zones of type 'remote' are allowed");
        return(CAT_INVALID_ARGUMENT);
    }

    // =-=-=-=-=-=-=-
    // validate the zone name does not include improper characters
    eirods::error ret = validate_zone_name( zoneName );
    if( !ret.ok() ) {
        eirods::log( ret );
        return SYS_INVALID_INPUT_PARAM;
    }

    /* String to get next sequence item for objects */
    cllNextValueString("R_ObjectID", nextStr, MAX_NAME_LEN);

    getNowStr(myTime);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegZone SQL 1 ");
    cllBindVars[cllBindVarCount++]=zoneName;
    cllBindVars[cllBindVarCount++]=zoneConnInfo;
    cllBindVars[cllBindVarCount++]=zoneComment;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;

    snprintf(tSQL, MAX_SQL_SIZE, 
             "insert into R_ZONE_MAIN (zone_id, zone_name, zone_type_name, zone_conn_string, r_comment, create_ts, modify_ts) values (%s, ?, 'remote', ?, ?, ?, ?)",
             nextStr);
    status =  cmlExecuteNoAnswerSql(tSQL,
                                    &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegZone cmlExecuteNoAnswerSql(insert) failure %d",status);
        _rollback("chlRegZone");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_REGISTER_ZONE,  "0",
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       "", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegResc cmlAudit3 failure %d",
                status);
        return(status);
    }


    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegZone cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}


/* Modify a Zone (certain fields) */
int chlModZone(rsComm_t *rsComm, char *zoneName, char *option,
               char *optionValue) {
    int status, OK;
    char myTime[50];
    char zoneId[MAX_NAME_LEN];
    char commentStr[200];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModZone");

    if (zoneName == NULL || option==NULL || optionValue==NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*zoneName == '\0' || *option == '\0' || *optionValue=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    zoneId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModZone SQL 1 ");
    status = cmlGetStringValueFromSql(
        "select zone_id from R_ZONE_MAIN where zone_name=?",
        zoneId, MAX_NAME_LEN, zoneName, "", 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_ZONE);
        return(status);
    }

    getNowStr(myTime);
    OK=0;
    if (strcmp(option, "comment")==0) {
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=zoneId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModZone SQL 3");
        status =  cmlExecuteNoAnswerSql(
            "update R_ZONE_MAIN set r_comment = ?, modify_ts=? where zone_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModZone cmlExecuteNoAnswerSql update failure %d",
                    status);
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "conn")==0) {
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=zoneId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModZone SQL 5");
        status =  cmlExecuteNoAnswerSql(
            "update R_ZONE_MAIN set zone_conn_string = ?, modify_ts=? where zone_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModZone cmlExecuteNoAnswerSql update failure %d",
                    status);
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "name")==0) {
        if (strcmp(zoneName,localZone)==0) {
            addRErrorMsg (&rsComm->rError, 0, 
                          "It is not valid to rename the local zone via chlModZone; iadmin should use acRenameLocalZone");
            return (CAT_INVALID_ARGUMENT);
        }
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=zoneId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModZone SQL 5");
        status =  cmlExecuteNoAnswerSql(
            "update R_ZONE_MAIN set zone_name = ?, modify_ts=? where zone_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModZone cmlExecuteNoAnswerSql update failure %d",
                    status);
            return(status);
        }
        OK=1;
    }
    if (OK==0) {
        return (CAT_INVALID_ARGUMENT);
    }

    /* Audit */
    snprintf(commentStr, sizeof commentStr, "%s %s", option, optionValue);
    status = cmlAudit3(AU_MOD_ZONE,  
                       zoneId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       commentStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModZone cmlAudit3 failure %d",
                status);
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModZone cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(0);
}

/* rename a collection */
int chlRenameColl(rsComm_t *rsComm, char *oldCollName, char *newCollName) {
    int status;
    rodsLong_t status1;

    /* See if the input path is a collection and the user owns it,
       and, if so, get the collectionID */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameColl SQL 1 ");

    status1 = cmlCheckDir(oldCollName,
                          rsComm->clientUser.userName, 
                          rsComm->clientUser.rodsZone, 
                          ACCESS_OWN, 
                          &icss);

    if (status1 < 0) {
        return(status1);
    }

    /* call chlRenameObject to rename */
    status = chlRenameObject(rsComm, status1, newCollName);
    return(status);
}


/* rename the local zone */
int chlRenameLocalZone(rsComm_t *rsComm, char *oldZoneName, char *newZoneName) {
    int status;
    char zoneId[MAX_NAME_LEN];
    char myTime[50];
    char commentStr[200];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 1 ");
    getLocalZone();

    if (strcmp(localZone, oldZoneName) != 0) { /* not the local zone */
        return(CAT_INVALID_ARGUMENT);
    }

    /* check that the new zone does not exist */
    zoneId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 2 ");
    status = cmlGetStringValueFromSql(
        "select zone_id from R_ZONE_MAIN where zone_name=?",
        zoneId, MAX_NAME_LEN, newZoneName, "", 0, &icss);
    if (status != CAT_NO_ROWS_FOUND) return(CAT_INVALID_ZONE);

    getNowStr(myTime);

    /* Audit */
    /* Do this first, before the userName-zone is made invalid;
       it will be rolledback if an error occurs */

    snprintf(commentStr, sizeof commentStr, "renamed local zone %s to %s",
             oldZoneName, newZoneName);
    status = cmlAudit3(AU_MOD_ZONE,  
                       "0",
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       commentStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlAudit3 failure %d",
                status);
        return(status);
    }

    /* update coll_owner_zone in R_COLL_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 3 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_COLL_MAIN set coll_owner_zone = ?, modify_ts=? where coll_owner_zone=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    /* update data_owner_zone in R_DATA_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 4 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_DATA_MAIN set data_owner_zone = ?, modify_ts=? where data_owner_zone=?",
        &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    /* update zone_name in R_RESC_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 5 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_RESC_MAIN set zone_name = ?, modify_ts=? where zone_name=?",
        &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    /* update rule_owner_zone in R_RULE_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 6 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_RULE_MAIN set rule_owner_zone=?, modify_ts=? where rule_owner_zone=?",
        &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    /* update zone_name in R_USER_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 7 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_USER_MAIN set zone_name=?, modify_ts=? where zone_name=?",
        &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    /* update zone_name in R_ZONE_MAIN */
    cllBindVars[cllBindVarCount++]=newZoneName;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=oldZoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameLocalZone SQL 8 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_ZONE_MAIN set zone_name=?, modify_ts=? where zone_name=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRenameLocalZone cmlExecuteNoAnswerSql update failure %d",
                status);
        return(status);
    }

    return(0);
}

/* delete a Zone */
int chlDelZone(rsComm_t *rsComm, char *zoneName) {
    int status;
    char zoneType[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelZone");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelZone SQL 1 ");

    status = cmlGetStringValueFromSql(
        "select zone_type_name from R_ZONE_MAIN where zone_name=?",
        zoneType, MAX_NAME_LEN, zoneName, 0, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_ZONE);
        return(status);
    }

    if (strcmp(zoneType, "remote") != 0) {
        addRErrorMsg (&rsComm->rError, 0, 
                      "It is not permitted to remove the local zone");
        return(CAT_INVALID_ARGUMENT);
    }

    cllBindVars[cllBindVarCount++]=zoneName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelZone 2");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_ZONE_MAIN where zone_name = ?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelZone cmlExecuteNoAnswerSql delete failure %d",
                status);
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_DELETE_ZONE,
                       "0",
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       zoneName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelZone cmlAudit3 failure %d",
                status);
        _rollback("chlDelZone");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelZone cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}


/* Simple query

   This is used in cases where it is easier to do a straight-forward
   SQL query rather than go thru the generalQuery interface.  This is
   used this in the iadmin.c interface as it was easier for me (Wayne)
   to work in SQL for admin type ops as I'm thinking in terms of
   tables and columns and SQL anyway.

   For improved security, this is available only to admin users and
   the code checks that the input sql is one of the allowed forms.

   input: sql, up to for optional arguments (bind variables),
   and requested format, max text to return (maxOutBuf)
   output: text (outBuf) or error return
   input/output: control: on input if 0 request is starting, 
   returned non-zero if more rows
   are available (and then it is the statement number);
   on input if positive it is the statement number (+1) that is being
   continued.
   format 1: column-name : column value, and with CR after each column
   format 2: column headings CR, rows and col values with CR

*/
int chlSimpleQuery(rsComm_t *rsComm, char *sql, 
                   char *arg1, char *arg2, char *arg3, char *arg4,
                   int format, int *control,
                   char *outBuf, int maxOutBuf) {
    int stmtNum, status, nCols, i, needToGet, didGet;
    int rowSize;
    int rows;
    int OK;

    char *allowedSQL[]={
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
    if (logSQL!=0) rodsLog(LOG_SQL, "chlSimpleQuery");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    /* check that the input sql is one of the allowed forms */
    OK=0;
    for (i=0;;i++) {
        if (strlen(allowedSQL[i]) < 1) break;
        if (strcasecmp(allowedSQL[i], sql)==0) {
            OK=1;
            break;
        }
    }

    if (OK == 0) {
        return(CAT_INVALID_ARGUMENT);
    }

    /* done with multiple log calls so that each form will be checked
       via checkIcatLog.pl */
    if (i==0 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 1 ");
    if (i==1 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 2");
    if (i==2 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 3");
    if (i==3 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 4");
    if (i==4 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 5");
    if (i==5 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 6");
    if (i==6 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 7");
    if (i==7 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 8");
    if (i==8 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 9");
    if (i==9 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 10");
    if (i==10 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 11");
    if (i==11 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 12");
    if (i==12 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 13");
    if (i==13 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 14");
    //if (i==14 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 15");
    //if (i==15 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 16");
    //if (i==16 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery S Q L 17");
    if (i==17 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 18");
    if (i==18 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 19");
    if (i==19 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 20");
    if (i==20 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 21");
    if (i==21 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 22");
    if (i==22 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 23");
    if (i==23 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 24");
    if (i==24 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 25");
    if (i==25 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 26");
    if (i==26 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 27");
    if (i==27 && logSQL) rodsLog(LOG_SQL, "chlSimpleQuery SQL 28");

    outBuf[0]='\0';
    needToGet=1;
    didGet=0;
    rowSize=0;
    rows=0;
    if (*control==0) {
        status = cmlGetFirstRowFromSqlBV(sql, arg1, arg2, arg3, arg4,
                                         &stmtNum, &icss);
        if (status < 0) {
            if (status != CAT_NO_ROWS_FOUND) {
                rodsLog(LOG_NOTICE,
                        "chlSimpleQuery cmlGetFirstRowFromSqlBV failure %d",
                        status);
            }
            return(status);
        }
        didGet=1;
        needToGet=0;
        *control = stmtNum+1;   /* control is always > 0 */
    } 
    else {
        stmtNum = *control - 1;
    }

    for (;;) {
        if (needToGet) {
            status = cmlGetNextRowFromStatement(stmtNum, &icss);
            if (status == CAT_NO_ROWS_FOUND) {
                *control = 0;
                if (didGet) {
                    if (format == 2) {
                        i = strlen(outBuf);
                        outBuf[i-1]='\0';  /* remove the last CR */
                    }
                    return(0);
                }
                return(status);
            }
            if (status < 0) {
                rodsLog(LOG_NOTICE,
                        "chlSimpleQuery cmlGetNextRowFromStatement failure %d",
                        status);
                return(status);
            }
            *control = stmtNum+1;   /* control is always > 0 */
            didGet=1;
        }
        needToGet=1;
        nCols = icss.stmtPtr[stmtNum]->numOfCols;
        if (rows==0 && format==3) {
            for (i = 0; i < nCols ; i++ ) {
                rstrcat(outBuf, icss.stmtPtr[stmtNum]->resultColName[i],maxOutBuf);
                rstrcat(outBuf, " ", maxOutBuf);
            }
            if (i != nCols-1) { // JMC - backport 4586
                /* add a space except for the last column */
                rstrcat(outBuf, " ", maxOutBuf);
            }

        }
        rows++;
        for (i = 0; i < nCols ; i++ ) {
            if (format==1 || format==3) {
                if (strlen(icss.stmtPtr[stmtNum]->resultValue[i])==0) {
                    rstrcat(outBuf, "- ", maxOutBuf);
                }
                else {
                    rstrcat(outBuf, icss.stmtPtr[stmtNum]->resultValue[i], 
                            maxOutBuf);
                    rstrcat(outBuf, " ", maxOutBuf);
                }
            }
            if (format == 2) {
                rstrcat(outBuf, icss.stmtPtr[stmtNum]->resultColName[i],maxOutBuf);
                rstrcat(outBuf, ": ", maxOutBuf);
                rstrcat(outBuf, icss.stmtPtr[stmtNum]->resultValue[i], maxOutBuf);
                rstrcat(outBuf, "\n", maxOutBuf);
            }
        }
        rstrcat(outBuf, "\n", maxOutBuf);
        if (rowSize==0) rowSize=strlen(outBuf);
        if ((int) strlen(outBuf)+rowSize+20 > maxOutBuf) {
            return(0); /* success so far, but more rows available */
        }
    }
    return 0;
}

/* Delete a Collection by Administrator, */
/* if it is empty. */
int chlDelCollByAdmin(rsComm_t *rsComm, collInfo_t *collInfo) {
    rodsLong_t iVal;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    char collIdNum[MAX_NAME_LEN];
    int status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelCollByAdmin");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (collInfo==0) {
        return(CAT_INVALID_ARGUMENT);
    }

    status = splitPathByKey(collInfo->collName, 
                            logicalParentDirName, logicalEndName, '/');

    if (strlen(logicalParentDirName)==0) {
        strcpy(logicalParentDirName, "/");
        strcpy(logicalEndName, collInfo->collName+1);
    }

    /* check that the collection is empty (both subdirs and files) */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelCollByAdmin SQL 1 ");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where parent_coll_name=? union select coll_id from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
        &iVal, collInfo->collName, collInfo->collName, 0, 0, 0, &icss);
 
    if (status != CAT_NO_ROWS_FOUND) {
        if (status == 0) {
            char errMsg[105];
            snprintf(errMsg, 100, "collection '%s' is not empty",
                     collInfo->collName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_COLLECTION_NOT_EMPTY);
        }
        _rollback("chlDelCollByAdmin");
        return(status);
    }

    /* remove any access rows */
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelCollByAdmin SQL 2");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_OBJT_ACCESS where object_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
        &icss);
    if (status != 0) {  /* error, but let it fall thru to below, 
                           probably doesn't exist */
        rodsLog(LOG_NOTICE,
                "chlDelCollByAdmin delete access failure %d",
                status);
        _rollback("chlDelCollByAdmin");
    }

    /* Remove associated AVUs, if any */
    snprintf(collIdNum, MAX_NAME_LEN, "%lld", iVal);
    removeMetaMapAndAVU(collIdNum);

    /* Audit (before it's deleted) */
    status = cmlAudit4(AU_DELETE_COLL_BY_ADMIN,  
                       "select coll_id from R_COLL_MAIN where coll_name=?",
                       collInfo->collName,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       collInfo->collName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelCollByAdmin cmlAudit4 failure %d",
                status);
        _rollback("chlDelCollByAdmin");
        return(status);
    }


    /* delete the row if it exists */
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelCollByAdmin SQL 3");
    status =  cmlExecuteNoAnswerSql("delete from R_COLL_MAIN where coll_name=?",
                                    &icss);

    if (status != 0) {
        char errMsg[105];
        snprintf(errMsg, 100, "collection '%s' is unknown", 
                 collInfo->collName);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        _rollback("chlDelCollByAdmin");
        return(CAT_UNKNOWN_COLLECTION);
    }

    return(0);
}


/* Delete a Collection */
int chlDelColl(rsComm_t *rsComm, collInfo_t *collInfo) {

    int status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelColl");

    status = _delColl(rsComm, collInfo);
    if (status != 0) return(status);

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelColl cmlExecuteNoAnswerSql commit failure %d",
                status);
        _rollback("chlDelColl");
        return(status);
    }
    return(0);
}

/* delCollection (internally called),
   does not do the commit.
*/
static int _delColl(rsComm_t *rsComm, collInfo_t *collInfo) {
    rodsLong_t iVal;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    char collIdNum[MAX_NAME_LEN];
    char parentCollIdNum[MAX_NAME_LEN];
    rodsLong_t status;

    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    status = splitPathByKey(collInfo->collName, 
                            logicalParentDirName, logicalEndName, '/');

    if (strlen(logicalParentDirName)==0) {
        strcpy(logicalParentDirName, "/");
        strcpy(logicalEndName, collInfo->collName+1);
    }

    /* Check that the parent collection exists and user has write permission,
       and get the collectionID */
    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl SQL 1 ");
    status = cmlCheckDir(logicalParentDirName, 
                         rsComm->clientUser.userName, 
                         rsComm->clientUser.rodsZone, 
                         ACCESS_MODIFY_OBJECT, 
                         &icss);
    if (status < 0) {
        char errMsg[105];
        if (status == CAT_UNKNOWN_COLLECTION) {
            snprintf(errMsg, 100, "collection '%s' is unknown",
                     logicalParentDirName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(status);
        }
        _rollback("_delColl");
        return(status);
    }
    snprintf(parentCollIdNum, MAX_NAME_LEN, "%lld", status);

    /* Check that the collection exists and user has DELETE or better 
       permission */
    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl SQL 2");
    status = cmlCheckDir(collInfo->collName, 
                         rsComm->clientUser.userName, 
                         rsComm->clientUser.rodsZone,
                         ACCESS_DELETE_OBJECT, 
                         &icss);
    if (status < 0) return(status);
    snprintf(collIdNum, MAX_NAME_LEN, "%lld", status);

    /* check that the collection is empty (both subdirs and files) */
    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl SQL 3");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where parent_coll_name=? union select coll_id from R_DATA_MAIN where coll_id=(select coll_id from R_COLL_MAIN where coll_name=?)",
        &iVal, collInfo->collName, collInfo->collName, 0, 0, 0, &icss);
    if (status != CAT_NO_ROWS_FOUND) {
        return(CAT_COLLECTION_NOT_EMPTY);
    }

    /* delete the row if it exists */
    /* The use of coll_id isn't really needed but may add a little safety.
       Previously, we included a check that it was owned by the user but
       the above cmlCheckDir is more accurate (handles group access). */
    cllBindVars[cllBindVarCount++]=collInfo->collName;
    cllBindVars[cllBindVarCount++]=collIdNum;
    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_COLL_MAIN where coll_name=? and coll_id=?",
        &icss);
    if (status != 0) {  /* error, odd one as everything checked above */
        rodsLog(LOG_NOTICE,
                "_delColl cmlExecuteNoAnswerSql delete failure %d",
                status);
        _rollback("_delColl");
        return(status);
    }

    /* remove any access rows */
    cllBindVars[cllBindVarCount++]=collIdNum;
    if (logSQL!=0) rodsLog(LOG_SQL, "_delColl SQL 5");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_OBJT_ACCESS where object_id=?",
        &icss);
    if (status != 0) {  /* error, odd one as everything checked above */
        rodsLog(LOG_NOTICE,
                "_delColl cmlExecuteNoAnswerSql delete access failure %d",
                status);
        _rollback("_delColl");
    }

    /* Remove associated AVUs, if any */
    removeMetaMapAndAVU(collIdNum);

    /* Audit */
    status = cmlAudit3(AU_DELETE_COLL,
                       collIdNum,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       collInfo->collName,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModColl cmlAudit3 failure %d",
                status);
        _rollback("_delColl");
        return(status);
    }

    return(status);
}

/* Check an authentication response.
 
   Input is the challange, response, and username; the response is checked
   and if OK the userPrivLevel is set.  Temporary-one-time passwords are
   also checked and possibly removed.

   The clientPrivLevel is the privilege level for the client in
   the rsComm structure; this is used by servers when setting the
   authFlag.

   Called from rsAuthCheck.
*/
int chlCheckAuth(rsComm_t *rsComm, char *challenge, char *response,
                 char *username, 
                 int *userPrivLevel, int *clientPrivLevel) {

    int status;
    char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
    char digest[RESPONSE_LEN+2];
    MD5_CTX context;
    char *cp;
    int i, OK, k;
    char userType[MAX_NAME_LEN];
    static int prevFailure=0;
    char pwInfoArray[MAX_PASSWORD_LEN*MAX_PASSWORDS*3];
    char goodPw[MAX_PASSWORD_LEN+10];
    char lastPw[MAX_PASSWORD_LEN+10];
    char goodPwExpiry[MAX_PASSWORD_LEN+10];
    char goodPwTs[MAX_PASSWORD_LEN+10];
    char goodPwModTs[MAX_PASSWORD_LEN+10];
    rodsLong_t expireTime;
    char *cpw;
    int nPasswords;
    char myTime[50];
    time_t nowTime;
    time_t pwExpireMaxCreateTime;
    char expireStr[50];
    char expireStrCreate[50];
    char myUserZone[MAX_NAME_LEN];
    char userName2[NAME_LEN+2];
    char userZone[NAME_LEN+2];
   rodsLong_t pamMinTime;
   rodsLong_t pamMaxTime;

#if defined(OS_AUTH)
    int doOsAuthentication = 0;
    char *os_auth_flag;
#endif

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth");

    if (prevFailure > 1) {
        /* Somebody trying a dictionary attack? */
        if (prevFailure > 5) sleep(20);  /* at least, slow it down */
        sleep(2);
    }
    *userPrivLevel = NO_USER_AUTH;
    *clientPrivLevel = NO_USER_AUTH;

    memset(md5Buf, 0, sizeof(md5Buf));
    strncpy(md5Buf, challenge, CHALLENGE_LEN);
    snprintf(prevChalSig,sizeof prevChalSig,
             "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
             (unsigned char)md5Buf[0], (unsigned char)md5Buf[1], 
             (unsigned char)md5Buf[2], (unsigned char)md5Buf[3],
             (unsigned char)md5Buf[4], (unsigned char)md5Buf[5], 
             (unsigned char)md5Buf[6], (unsigned char)md5Buf[7],
             (unsigned char)md5Buf[8], (unsigned char)md5Buf[9], 
             (unsigned char)md5Buf[10],(unsigned char)md5Buf[11],
             (unsigned char)md5Buf[12],(unsigned char)md5Buf[13], 
             (unsigned char)md5Buf[14],(unsigned char)md5Buf[15]);
#if defined(OS_AUTH)
    /* check for the OS_AUTH_FLAG token in the username to see if
     * we should run the OS level authentication. Make sure and
     * strip it from the username string so other operations 
     * don't fail parsing the format.
     */
    os_auth_flag = strstr(username, OS_AUTH_FLAG);
    if (os_auth_flag) {
        *os_auth_flag = 0;
        doOsAuthentication = 1;
    }
#endif

    status = parseUserName(username, userName2, userZone);
    if (userZone[0]=='\0') {
        status = getLocalZone();
        if (status != 0) return(status);
        strncpy(myUserZone, localZone, MAX_NAME_LEN);
    }
    else {
        strncpy(myUserZone, userZone, MAX_NAME_LEN);
    }

#if defined(OS_AUTH)
    if (doOsAuthentication) {
        if ((status = osauthVerifyResponse(challenge, userName2, response))) {
            return status;
        }
        goto checkLevel;
    }
#endif

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 1 ");
    
    status = cmlGetMultiRowStringValuesFromSql(
           "select rcat_password, pass_expiry_ts, R_USER_PASSWORD.create_ts, R_USER_PASSWORD.modify_ts from R_USER_PASSWORD, R_USER_MAIN where user_name=? and zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
           pwInfoArray, MAX_PASSWORD_LEN,
           MAX_PASSWORDS*4,  /* four strings per password returned */
           userName2, myUserZone, &icss);
        
    if (status < 4) {
        if (status == CAT_NO_ROWS_FOUND) {
            status = CAT_INVALID_USER; /* Be a little more specific */
            if (strncmp(ANONYMOUS_USER, userName2, NAME_LEN)==0) {
                /* anonymous user, skip the pw check but do the rest */
                goto checkLevel;
            }
        } 
        return(status);
    }

    nPasswords=status/4;    /* three strings per password returned */
    goodPwExpiry[0]='\0';
    goodPwTs[0]='\0';
    goodPwModTs[0]='\0';

    cpw=pwInfoArray;
    for (k=0;k<MAX_PASSWORDS && k<nPasswords;k++) {
        memset(md5Buf, 0, sizeof(md5Buf));
        strncpy(md5Buf, challenge, CHALLENGE_LEN);
        icatDescramble(cpw);
        strncpy(md5Buf+CHALLENGE_LEN, cpw, MAX_PASSWORD_LEN);

        MD5Init (&context);
        MD5Update (&context, (unsigned char *) md5Buf, CHALLENGE_LEN+MAX_PASSWORD_LEN);
        MD5Final ((unsigned char *) digest, &context);

        for (i=0;i<RESPONSE_LEN;i++) {
            if (digest[i]=='\0') digest[i]++;  /* make sure 'string' doesn't end
                                                  early (this matches client code) */
        }

        cp = response;
        OK=1;
        for (i=0;i<RESPONSE_LEN;i++) {
            if (*cp++ != digest[i]) OK=0;
        }

        memset(md5Buf, 0, sizeof(md5Buf));
        if (OK==1) {
            rstrcpy(goodPw, cpw, MAX_PASSWORD_LEN);
            cpw+=MAX_PASSWORD_LEN;
            rstrcpy(goodPwExpiry, cpw, MAX_PASSWORD_LEN);
            cpw+=MAX_PASSWORD_LEN;
            rstrcpy(goodPwTs, cpw, MAX_PASSWORD_LEN);
            cpw+=MAX_PASSWORD_LEN;
            rstrcpy(goodPwModTs, cpw, MAX_PASSWORD_LEN);
            break;
        }
        cpw+=MAX_PASSWORD_LEN*4;
    }
    memset(pwInfoArray, 0, sizeof(pwInfoArray));

    if (OK==0) {
        prevFailure++;
        return(CAT_INVALID_AUTHENTICATION);
    }

    expireTime=atoll(goodPwExpiry);
    getNowStr(myTime);
    nowTime=atoll(myTime);

    /* Check for PAM_AUTH type passwords */
    pamMaxTime=atoll(IRODS_PAM_PASSWORD_MAX_TIME);
    pamMinTime=atoll(IRODS_PAM_PASSWORD_MIN_TIME);

    if( ( strncmp(goodPwExpiry, "9999",4)!=0) &&
        expireTime >=  pamMinTime &&
        expireTime <= pamMaxTime) {
        time_t modTime;
        /* The used pw is an iRODS-PAM type, so now check if it's expired */
        getNowStr(myTime);
        nowTime=atoll(myTime);
        modTime=atoll(goodPwModTs);

        if (modTime+expireTime < nowTime) {
            /* it is expired, so return the error below and first remove it */
            cllBindVars[cllBindVarCount++]=lastPw;
            cllBindVars[cllBindVarCount++]=goodPwTs;
            cllBindVars[cllBindVarCount++]=userName2;
            cllBindVars[cllBindVarCount++]=myUserZone;
            if (logSQL!=0) 
                rodsLog(LOG_SQL, "chlCheckAuth SQL 2");
            status = cmlExecuteNoAnswerSql("delete from R_USER_PASSWORD where rcat_password=? and create_ts=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)", &icss);
            memset(goodPw, 0, sizeof(goodPw));
            memset(lastPw, 0, sizeof(lastPw));
            if (status != 0) {
                rodsLog(LOG_NOTICE,
                "chlCheckAuth cmlExecuteNoAnswerSql delete expired password failure %d",
                status);
                return(status);
            }
            status =  cmlExecuteNoAnswerSql("commit", &icss);
            if (status != 0) {
                rodsLog(LOG_NOTICE,
                "chlCheckAuth cmlExecuteNoAnswerSql commit failure %d",
                status);
                return(status);
            }
            return(CAT_PASSWORD_EXPIRED);
        }
    }


    if (expireTime < TEMP_PASSWORD_MAX_TIME) {

        /* in the form used by temporary, one-time passwords */

        time_t createTime;
        int returnExpired;

        /* check if it's expired */

        returnExpired=0;
        getNowStr(myTime);
        nowTime=atoll(myTime);
        createTime=atoll(goodPwTs);
        if (createTime==0 || nowTime==0 ) returnExpired=1;
        if (createTime+expireTime < nowTime) returnExpired=1;


        /* Remove this temporary, one-time password */

        cllBindVars[cllBindVarCount++]=goodPw;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "delete from R_USER_PASSWORD where rcat_password=?",
            &icss);
        if (status !=0) {
            rodsLog(LOG_NOTICE,
                    "chlCheckAuth cmlExecuteNoAnswerSql delete failure %d",
                    status);
            _rollback("chlCheckAuth");
            return(status);
        }

        /* Also remove any expired temporary passwords */

        if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 3");
        snprintf(expireStr, sizeof expireStr, "%d", TEMP_PASSWORD_TIME);
        cllBindVars[cllBindVarCount++]=expireStr; 

        pwExpireMaxCreateTime = nowTime-TEMP_PASSWORD_TIME;
        /* Not sure if casting to int is correct but seems OK & avoids warning:*/
        snprintf(expireStrCreate, sizeof expireStrCreate, "%011d", 
                 (int)pwExpireMaxCreateTime); 
        cllBindVars[cllBindVarCount++]=expireStrCreate;

        status =  cmlExecuteNoAnswerSql(
            "delete from R_USER_PASSWORD where pass_expiry_ts = ? and create_ts < ?",
            &icss);
        if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
            rodsLog(LOG_NOTICE,
                    "chlCheckAuth cmlExecuteNoAnswerSql delete2 failure %d",
                    status);
            _rollback("chlCheckAuth");
            return(status);
        }

        memset(goodPw, 0, MAX_PASSWORD_LEN);
        if (returnExpired) return(CAT_PASSWORD_EXPIRED);

        if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 4");
        status =  cmlExecuteNoAnswerSql("commit", &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlCheckAuth cmlExecuteNoAnswerSql commit failure %d",
                    status);
            return(status);
        }
        memset(goodPw, 0, MAX_PASSWORD_LEN);
        if (returnExpired) return(CAT_PASSWORD_EXPIRED);
    }

    /* Get the user type so privilege level can be set */
checkLevel:

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 5");
    status = cmlGetStringValueFromSql(
        "select user_type_name from R_USER_MAIN where user_name=? and zone_name=?",
        userType, MAX_NAME_LEN, userName2, myUserZone, 0, &icss);
    if (status !=0) {
        if (status == CAT_NO_ROWS_FOUND) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback("chlCheckAuth");
        }
        return(status);
    }
    *userPrivLevel = LOCAL_USER_AUTH;
    if (strcmp(userType, "rodsadmin") == 0) {
        *userPrivLevel = LOCAL_PRIV_USER_AUTH;

        /* Since the user is admin, also get the client privilege level */
        if (strcmp( rsComm->clientUser.userName, userName2)==0 &&
            strcmp( rsComm->clientUser.rodsZone, userZone)==0) {
            *clientPrivLevel = LOCAL_PRIV_USER_AUTH; /* same user, no query req */
        }
        else {
            if (rsComm->clientUser.userName[0]=='\0') {
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
                *clientPrivLevel = REMOTE_USER_AUTH;
                prevFailure=0;
                return(0);
            }
            else {
                if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckAuth SQL 6");
                status = cmlGetStringValueFromSql(
                    "select user_type_name from R_USER_MAIN where user_name=? and zone_name=?",
                    userType, MAX_NAME_LEN, rsComm->clientUser.userName,
                    rsComm->clientUser.rodsZone, 0, &icss);
                if (status !=0) {
                    if (status == CAT_NO_ROWS_FOUND) {
                        status = CAT_INVALID_CLIENT_USER; /* more specific */
                    }
                    else {
                        _rollback("chlCheckAuth");
                    }
                    return(status);
                }
                *clientPrivLevel = LOCAL_USER_AUTH;
                if (strcmp(userType, "rodsadmin") == 0) {
                    *clientPrivLevel = LOCAL_PRIV_USER_AUTH;
                }
            }
        }
    }

    prevFailure=0;
    return(0);
}

/* Generate a temporary, one-time password.
   Input is the username from the rsComm structure.  
   Output is the pattern, that when hashed with the user's password,
   becomes the temporary password.  The temp password is also stored
   in the database.

   Called from rsGetTempPassword.
*/
int chlMakeTempPw(rsComm_t *rsComm, char *pwValueToHash) {
    int status;
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN+2];
    MD5_CTX context;
    int i;
    char password[MAX_PASSWORD_LEN+10];
    char newPw[MAX_PASSWORD_LEN+10];
    char myTime[50];
    char myTimeExp[50];
    char rBuf[200];
    char hashValue[50];
    int j=0;
    char tSQL[MAX_SQL_SIZE];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlMakeTempPw");

    if (logSQL!=0) rodsLog(LOG_SQL, "chlMakeTempPw SQL 1 ");

    snprintf(tSQL, MAX_SQL_SIZE, 
             "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id and pass_expiry_ts != '%d'",
             TEMP_PASSWORD_TIME);

    status = cmlGetStringValueFromSql(tSQL,
                                      password, MAX_PASSWORD_LEN, 
                                      rsComm->clientUser.userName, 
                                      rsComm->clientUser.rodsZone, 0, &icss);
    if (status !=0) {
        if (status == CAT_NO_ROWS_FOUND) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback("chlMakeTempPw");
        }
        return(status);
    }

    icatDescramble(password);

    j=0;
    get64RandomBytes(rBuf);
    for (i=0;i<50 && j<MAX_PASSWORD_LEN-1;i++) {
        char c;
        c = rBuf[i] &0x7f;
        if (c < '0') c+='0';
        if ( (c > 'a' && c < 'z') || (c > 'A' && c < 'Z') ||
             (c > '0' && c < '9') ){
            hashValue[j++]=c;
        }
    }
    hashValue[j]='\0';
/*   printf("hashValue=%s\n", hashValue); */

    /* calcuate the temp password (a hash of the user's main pw and
       the hashValue) */
    memset(md5Buf, 0, sizeof(md5Buf));
    strncpy(md5Buf, hashValue, 100);
    strncat(md5Buf, password, 100);

    MD5Init (&context);
    MD5Update (&context, (unsigned char *) md5Buf, 100);
    MD5Final ((unsigned char *) digest, &context);

    md5ToStr(digest, newPw);
/*   printf("newPw=%s\n", newPw); */

    rstrcpy(pwValueToHash, hashValue, MAX_PASSWORD_LEN);


    /* Insert the temporay, one-time password */

    getNowStr(myTime);
    sprintf(myTimeExp, "%d", TEMP_PASSWORD_TIME);  /* seconds from create time
                                                      when it will expire */
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.userName;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.rodsZone,
        cllBindVars[cllBindVarCount++]=newPw;
    cllBindVars[cllBindVarCount++]=myTimeExp;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlMakeTempPw SQL 2");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
        &icss);
    if (status !=0) {
        rodsLog(LOG_NOTICE,
                "chlMakeTempPw cmlExecuteNoAnswerSql insert failure %d",
                status);
        _rollback("chlMakeTempPw");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlMakeTempPw cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    memset(newPw, 0, MAX_PASSWORD_LEN);
    return(0);
}

/*
  de-scramble a password sent from the client.
  This isn't real encryption, but does obfuscate the pw on the network.
  Called internally, from chlModUser.
*/
int decodePw(rsComm_t *rsComm, char *in, char *out) {
    int status;
    char *cp;
    char password[MAX_PASSWORD_LEN];
    char upassword[MAX_PASSWORD_LEN+10];
    char rand[]=
        "1gCBizHWbwIYyWLo";  /* must match clients */
    int pwLen1, pwLen2;

    if (logSQL!=0) rodsLog(LOG_SQL, "decodePw - SQL 1 ");
    status = cmlGetStringValueFromSql(
        "select rcat_password from R_USER_PASSWORD, R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
        password, MAX_PASSWORD_LEN, 
        rsComm->clientUser.userName, 
        rsComm->clientUser.rodsZone,
        0, &icss);
    if (status !=0) {
        if (status == CAT_NO_ROWS_FOUND) {
            status = CAT_INVALID_USER; /* Be a little more specific */
        }
        else {
            _rollback("decodePw");
        }
        return(status);
    }

    icatDescramble(password);

    obfDecodeByKeyV2(in, password, prevChalSig, upassword);

    pwLen1=strlen(upassword);

    memset(password, 0, MAX_PASSWORD_LEN);

    cp = strstr(upassword, rand);
    if (cp !=NULL) *cp='\0';

    pwLen2 = strlen(upassword);

    if (pwLen2 > MAX_PASSWORD_LEN-5 && pwLen2==pwLen1) {
        /* probable failure */
        char errMsg[160];
        snprintf(errMsg, 150, 
                 "Error with password encoding.\nPlease try connecting directly to the ICAT host (setenv irodsHost)");
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_PASSWORD_ENCODING_ERROR);
    }
    strcpy(out, upassword);
    memset(upassword, 0, MAX_PASSWORD_LEN);

    return(0);
}


/*
 Add or update passwords for use in the PAM authentication mode.
 User has been PAM-authenticated when this is called.
 This function adds a irods password valid for a few days and returns that.
 If one already exists, the expire time is updated, and it's value is returned.
 Passwords created are pseudo-random strings, unrelated to the PAM password.
 If testTime is non-null, use that as the create-time, as a testing aid.
 */
int chlUpdateIrodsPamPassword(rsComm_t *rsComm,
			      char *username, int timeToLive,
			      char *testTime,
			      char **irodsPassword) {
   char myTime[50];
   char rBuf[200];
   int i, j;
   char randomPw[50];
   char randomPwEncoded[50];
   int status;
   char passwordInIcat[MAX_PASSWORD_LEN+2];
   char passwordModifyTime[50];
   char *cVal[3];
   int iVal[3];
   char selUserId[MAX_NAME_LEN];
   char expTime[50];

   status = getLocalZone();
   if (status != 0) return(status);

   getNowStr(myTime);

   /* if ttl is unset, use the default */
   if (timeToLive == 0) {
     rstrcpy(expTime, IRODS_PAM_PASSWORD_DEFAULT_TIME, sizeof expTime);
   }
   else {
     /* convert ttl to seconds and make sure ttl is within the limits */
     rodsLong_t pamMinTime, pamMaxTime;
     pamMinTime=atoll(IRODS_PAM_PASSWORD_MIN_TIME);
     pamMaxTime=atoll(IRODS_PAM_PASSWORD_MAX_TIME);
     timeToLive = timeToLive * 3600;
     if (timeToLive < pamMinTime || 
	 timeToLive > pamMaxTime) {
       return PAM_AUTH_PASSWORD_INVALID_TTL;
     }
     snprintf(expTime, sizeof expTime, "%d", timeToLive);
   }

   /* get user id */
   if (logSQL!=0) rodsLog(LOG_SQL, "chlUpdateIrodsPamPassword SQL 1");
   status = cmlGetStringValueFromSql(
      "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name!='rodsgroup'",
      selUserId, MAX_NAME_LEN, username, localZone, 0, &icss);
   if (status==CAT_NO_ROWS_FOUND) return (CAT_INVALID_USER);
   if (status) return(status);

   /* first delete any that are expired */
   if (logSQL!=0) rodsLog(LOG_SQL, "chlUpdateIrodsPamPassword SQL 2");
   cllBindVars[cllBindVarCount++]=IRODS_PAM_PASSWORD_MIN_TIME;
   cllBindVars[cllBindVarCount++]=IRODS_PAM_PASSWORD_MAX_TIME;
   cllBindVars[cllBindVarCount++]=myTime;
#if MY_ICAT
   status =  cmlExecuteNoAnswerSql("delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and (cast(pass_expiry_ts as signed integer) + cast(modify_ts as signed integer) < ?)",
#else
   status =  cmlExecuteNoAnswerSql("delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and (cast(pass_expiry_ts as integer) + cast(modify_ts as integer) < ?)",
#endif
				   &icss);
   if (logSQL!=0) rodsLog(LOG_SQL, "chlUpdateIrodsPamPassword SQL 3");
   cVal[0]=passwordInIcat;
   iVal[0]=MAX_PASSWORD_LEN;
   cVal[1]=passwordModifyTime;
   iVal[1]=sizeof(passwordModifyTime);
   status = cmlGetStringValuesFromSql(
#if MY_ICAT
	    "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer) >= ? and cast (pass_expiry_ts as signed integer) <= ?",
#else
	    "select rcat_password, modify_ts from R_USER_PASSWORD where user_id=? and pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer) >= ? and cast (pass_expiry_ts as integer) <= ?",
#endif
	    cVal, iVal, 2,
	    selUserId, 
            IRODS_PAM_PASSWORD_MIN_TIME,
            IRODS_PAM_PASSWORD_MAX_TIME, &icss);

   if (status==0) {
#ifndef PAM_AUTH_NO_EXTEND
      if (logSQL!=0) rodsLog(LOG_SQL, "chlUpdateIrodsPamPassword SQL 4");
      cllBindVars[cllBindVarCount++]=myTime;
      cllBindVars[cllBindVarCount++]=expTime;
      cllBindVars[cllBindVarCount++]=selUserId;
      cllBindVars[cllBindVarCount++]=passwordInIcat;
      status =  cmlExecuteNoAnswerSql("update R_USER_PASSWORD set modify_ts=?, pass_expiry_ts=? where user_id = ? and rcat_password = ?",
				      &icss);
      if (status) return(status);

      status =  cmlExecuteNoAnswerSql("commit", &icss);
      if (status != 0) {
	 rodsLog(LOG_NOTICE,
		 "chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure %d",
		 status);
	 return(status);
      }
#endif
      icatDescramble(passwordInIcat);
      strncpy(*irodsPassword, passwordInIcat, IRODS_PAM_PASSWORD_LEN);
      return(0);
   }


   // =-=-=-=-=-=-=-
   // if the resultant scrambled password has a ' in the
   // string, this can cause issues on some systems, notably
   // Suse 12.  if this is the case we will just get another
   // random password.
   bool pw_good = false;
   while( !pw_good ) {

       j=0;
       get64RandomBytes(rBuf);
       for (i=0;i<50 && j<IRODS_PAM_PASSWORD_LEN-1;i++) {
          char c;
          c = rBuf[i] & 0x7f;
          if (c < '0') c+='0';
          if ( (c > 'a' && c < 'z') || (c > 'A' && c < 'Z') ||
               (c > '0' && c < '9') ){
             randomPw[j++]=c;
          }
       }
       randomPw[j]='\0';

       strncpy(randomPwEncoded, randomPw, 50);
       icatScramble(randomPwEncoded); 
       if( !strstr( randomPwEncoded, "\'" ) ) {
           pw_good = true; 

       } else {
           rodsLog( LOG_STATUS, "chlUpdateIrodsPamPassword :: getting a new password [%s] has a single quote", randomPwEncoded );

       }

   } // while

   if (testTime!=NULL && strlen(testTime)>0) {
      strncpy(myTime, testTime, sizeof(myTime));
   }

   if (logSQL!=0) rodsLog(LOG_SQL, "chlUpdateIrodsPamPassword SQL 5");
   cllBindVars[cllBindVarCount++]=selUserId;
   cllBindVars[cllBindVarCount++]=randomPwEncoded;
   cllBindVars[cllBindVarCount++]=expTime;
   cllBindVars[cllBindVarCount++]=myTime;
   cllBindVars[cllBindVarCount++]=myTime;
   status =  cmlExecuteNoAnswerSql("insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values (?, ?, ?, ?, ?)", 
				   &icss);
   if (status) return(status);

   status =  cmlExecuteNoAnswerSql("commit", &icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE,
	  "chlUpdateIrodsPamPassword cmlExecuteNoAnswerSql commit failure %d",
	  status);
      return(status);
   }

   strncpy(*irodsPassword, randomPw, IRODS_PAM_PASSWORD_LEN);
   return(0);
}

/* Modify an existing user.
   Admin only.
   Called from rsGeneralAdmin which is used by iadmin */
int chlModUser(rsComm_t *rsComm, char *userName, char *option,
               char *newValue) {
    int status;
    int opType;
    char decoded[MAX_PASSWORD_LEN+20];
    char tSQL[MAX_SQL_SIZE];
    char form1[]="update R_USER_MAIN set %s=?, modify_ts=? where user_name=? and zone_name=?";
    char form2[]="update R_USER_MAIN set %s=%s, modify_ts=? where user_name=? and zone_name=?";
    char form3[]="update R_USER_PASSWORD set rcat_password=?, modify_ts=? where user_id=?";
    char form4[]="insert into R_USER_PASSWORD (user_id, rcat_password, pass_expiry_ts,  create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)";
    char form5[]="insert into R_USER_AUTH (user_id, user_auth_name, create_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?)";
    char form6[]="delete from R_USER_AUTH where user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?) and user_auth_name = ?";
#if MY_ICAT
   char form7[]="delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as signed integer)>=? and cast(pass_expiry_ts as signed integer)<=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)";
#else
   char form7[]="delete from R_USER_PASSWORD where pass_expiry_ts not like '9999%' and cast(pass_expiry_ts as integer)>=? and cast(pass_expiry_ts as integer)<=? and user_id = (select user_id from R_USER_MAIN where user_name=? and zone_name=?)";
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

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser");

    if (userName == NULL || option == NULL || newValue==NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*userName == '\0' || *option == '\0' || newValue=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    userSettingOwnPassword=0;
    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    groupAdminSettingPassword=0;
    if (rsComm->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH && // JMC - backport 4773
        rsComm->proxyUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH) {
        /* user is OK */
    }
    else {
        /* need to check */
        if ( strcmp(option,"password")!=0) {
            /* only password (in cases below) is allowed */
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        if ( strcmp(userName, rsComm->clientUser.userName)==0)  {
            userSettingOwnPassword=1;
        }
        else {
            int status2;
            status2  = cmlCheckGroupAdminAccess(
                rsComm->clientUser.userName,
                rsComm->clientUser.rodsZone, 
                "", &icss);
            if (status2 != 0) return(status2);
            groupAdminSettingPassword=1;
        }
        // =-=-=-=-=-=-=-
        if (userSettingOwnPassword==0 && groupAdminSettingPassword==0) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }

    status = getLocalZone();
    if (status != 0) return(status);

    tSQL[0]='\0';
    opType=0;

    getNowStr(myTime);

    auditComment[0]='\0';
    strncpy(auditUserName,userName,100);

    status = parseUserName(userName, userName2, zoneName);
    if (zoneName[0]=='\0') {
        rstrcpy(zoneName, localZone, NAME_LEN);
    }
    if (status != 0) {
        return (CAT_INVALID_ARGUMENT);
    }

#if 0
    /* no longer allow modifying the user's name since it would 
       require moving the home and trash/home collections too */
    if (strcmp(option,"name" )==0 ||
        strcmp(option,"user_name" )==0) {
        snprintf(tSQL, MAX_SQL_SIZE, form1,
                 "user_name");
        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUserSQLxx1x");
        auditId = AU_MOD_USER_NAME;
        strncpy(auditComment, userName, 100);
        strncpy(auditUserName,newValue,100);
    }
#endif
    if (strcmp(option,"type")==0 ||
        strcmp(option,"user_type_name")==0) {
        char tsubSQL[MAX_SQL_SIZE];
        snprintf(tsubSQL, MAX_SQL_SIZE, "(select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?)");
        cllBindVars[cllBindVarCount++]=newValue;
        snprintf(tSQL, MAX_SQL_SIZE, form2,
                 "user_type_name", tsubSQL);
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        opType=1;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 2");
        auditId = AU_MOD_USER_TYPE;
        strncpy(auditComment, newValue, 100);
    }
    if (strcmp(option,"zone")==0 ||
        strcmp(option,"zone_name")==0) {
        snprintf(tSQL, MAX_SQL_SIZE, form1, "zone_name");
        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 3");
        auditId = AU_MOD_USER_ZONE;
        strncpy(auditComment, newValue, 100);
        strncpy(auditUserName,userName,100);
    }
    if (strcmp(option,"addAuth")==0) {
        opType=4;
        rstrcpy(tSQL, form5, MAX_SQL_SIZE);
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 4");
        auditId = AU_ADD_USER_AUTH_NAME;
        strncpy(auditComment, newValue, 100);
    }
    if (strcmp(option,"rmAuth")==0) {
        rstrcpy(tSQL, form6, MAX_SQL_SIZE);
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        cllBindVars[cllBindVarCount++]=newValue;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 5");
        auditId = AU_DELETE_USER_AUTH_NAME;
        strncpy(auditComment, newValue, 100);

    }

    if (strncmp(option, "rmPamPw", 9)==0) {
        rstrcpy(tSQL, form7, MAX_SQL_SIZE);
        cllBindVars[cllBindVarCount++]=IRODS_PAM_PASSWORD_MIN_TIME;
        cllBindVars[cllBindVarCount++]=IRODS_PAM_PASSWORD_MAX_TIME;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 6");
        auditId = AU_MOD_USER_PASSWORD;
        strncpy(auditComment, "Deleted user iRODS-PAM password (if any)", 100);
    }

    if (strcmp(option,"info")==0 ||
        strcmp(option,"user_info")==0) {
        snprintf(tSQL, MAX_SQL_SIZE, form1,
                 "user_info");
        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 6");
        auditId = AU_MOD_USER_INFO;
        strncpy(auditComment, newValue, 100);
    }
    if (strcmp(option,"comment")==0 ||
        strcmp(option,"r_comment")==0) {
        snprintf(tSQL, MAX_SQL_SIZE, form1,
                 "r_comment");
        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=userName2;
        cllBindVars[cllBindVarCount++]=zoneName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 7");
        auditId = AU_MOD_USER_COMMENT;
        strncpy(auditComment, newValue, 100);
    }
    if (strcmp(option,"password")==0) {
        int i;
        char userIdStr[MAX_NAME_LEN];
        i = decodePw(rsComm, newValue, decoded);

        icatScramble(decoded); 

        if (i) return(i);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 8");
        i = cmlGetStringValueFromSql(
            "select R_USER_PASSWORD.user_id from R_USER_PASSWORD, R_USER_MAIN where R_USER_MAIN.user_name=? and R_USER_MAIN.zone_name=? and R_USER_MAIN.user_id = R_USER_PASSWORD.user_id",
            userIdStr, MAX_NAME_LEN, userName2, zoneName, 0, &icss);
        if (i != 0 && i !=CAT_NO_ROWS_FOUND) return(i);
        if (i == 0) {
            if (groupAdminSettingPassword == 1) { // JMC - backport 4772
                /* Group admin can only set the initial password, not update */
                return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
            }
            rstrcpy(tSQL, form3, MAX_SQL_SIZE);
            cllBindVars[cllBindVarCount++]=decoded;
            cllBindVars[cllBindVarCount++]=myTime;
            cllBindVars[cllBindVarCount++]=userIdStr;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 9");
        }
        else {
            opType=4;
            rstrcpy(tSQL, form4, MAX_SQL_SIZE);
            cllBindVars[cllBindVarCount++]=userName2;
            cllBindVars[cllBindVarCount++]=zoneName;
            cllBindVars[cllBindVarCount++]=decoded;
            cllBindVars[cllBindVarCount++]="9999-12-31-23.59.01";
            cllBindVars[cllBindVarCount++]=myTime;
            cllBindVars[cllBindVarCount++]=myTime;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 10");
        }
        auditId = AU_MOD_USER_PASSWORD;
    }

    if (tSQL[0]=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    status =  cmlExecuteNoAnswerSql(tSQL, &icss);
    memset(decoded, 0, MAX_PASSWORD_LEN);

    if (status != 0 ) {  /* error */
        if (opType==1) { /* doing a type change, check if user_type problem */
            int status2;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 11");
            status2 = cmlGetIntegerValueFromSql(
                "select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?", 
                &iVal, newValue, 0, 0, 0, 0, &icss);
            if (status2 != 0) {
                char errMsg[105];
                snprintf(errMsg, 100, "user_type '%s' is not valid", 
                         newValue);
                addRErrorMsg (&rsComm->rError, 0, errMsg);

                rodsLog(LOG_NOTICE,
                        "chlModUser invalid user_type");
                return(CAT_INVALID_USER_TYPE);
            }
        }
        if (opType==4) { /* trying to insert password or auth-name */
            /* check if user exists */
            int status2;
            _rollback("chlModUser");
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModUser SQL 12");
            status2 = cmlGetIntegerValueFromSql(
                "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
                &iVal, userName2, zoneName, 0, 0, 0, &icss);
            if (status2 != 0) {
                rodsLog(LOG_NOTICE,
                        "chlModUser invalid user %s zone %s", userName2, zoneName);
                return(CAT_INVALID_USER);
            }
        }
        rodsLog(LOG_NOTICE,
                "chlModUser cmlExecuteNoAnswerSql failure %d",
                status);
        return(status);
    }

    status = cmlAudit1(auditId, rsComm->clientUser.userName, 
                       localZone, auditUserName, auditComment, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModUser cmlAudit1 failure %d",
                status);
        _rollback("chlModUser");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModUser cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(0);
}

/* Modify an existing group (membership). 
   Groups are also users in the schema, so chlModUser can also 
   modify other group attibutes. */
int chlModGroup(rsComm_t *rsComm, char *groupName, char *option,
                char *userName, char *userZone) {
    int status, OK;
    char myTime[50];
    char userId[MAX_NAME_LEN];
    char groupId[MAX_NAME_LEN];
    char commentStr[100];
    char zoneToUse[MAX_NAME_LEN];

    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];
      

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModGroup");

    if (groupName == NULL || option == NULL || userName==NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*groupName == '\0' || *option == '\0' || userName=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
        rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone, groupName, &icss);
        if (status2 != 0) return(status2);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    strncpy(zoneToUse, localZone, MAX_NAME_LEN);
    if (userZone != NULL && *userZone != '\0') {
        strncpy(zoneToUse, userZone, MAX_NAME_LEN);
    }

    status = parseUserName(userName, userName2, zoneName);
    if (zoneName[0]!='\0') {
        rstrcpy(zoneToUse, zoneName, NAME_LEN);
    }

    userId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModGroup SQL 1 ");
    status = cmlGetStringValueFromSql(
        "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name !='rodsgroup'",
        userId, MAX_NAME_LEN, userName2, zoneToUse, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) {
            return(CAT_INVALID_USER);
        }
        _rollback("chlModGroup");
        return(status);
    }

    groupId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModGroup SQL 2");
    status = cmlGetStringValueFromSql(
        "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=? and user_type_name='rodsgroup'",
        groupId, MAX_NAME_LEN, groupName, localZone, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) {
            return(CAT_INVALID_GROUP);
        }
        _rollback("chlModGroup");
        return(status);
    }
    OK=0;
    if (strcmp(option, "remove")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModGroup SQL 3");
        cllBindVars[cllBindVarCount++]=groupId;
        cllBindVars[cllBindVarCount++]=userId;
        status =  cmlExecuteNoAnswerSql(
            "delete from R_USER_GROUP where group_user_id = ? and user_id = ?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModGroup cmlExecuteNoAnswerSql delete failure %d",
                    status);
            _rollback("chlModGroup");
            return(status);
        }
        OK=1;
    }

    if (strcmp(option, "add")==0) {
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=groupId;
        cllBindVars[cllBindVarCount++]=userId;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModGroup SQL 4");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_USER_GROUP (group_user_id, user_id , create_ts, modify_ts) values (?, ?, ?, ?)",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModGroup cmlExecuteNoAnswerSql delete failure %d",
                    status);
            _rollback("chlModGroup");
            return(status);
        }
        OK=1;
    }

    if (OK==0) {
        return (CAT_INVALID_ARGUMENT);
    }

    /* Audit */
    snprintf(commentStr, sizeof commentStr, "%s %s", option, userId);
    status = cmlAudit3(AU_MOD_GROUP,  
                       groupId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       commentStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModGroup cmlAudit3 failure %d",
                status);
        _rollback("chlModGroup");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModGroup cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(0);
}

/* Modify a Resource (certain fields) */
int chlModResc(
    rsComm_t* rsComm, 
    char*     rescName, 
    char*     option,
    char*     optionValue ) {
    int status, OK;
    char myTime[50];
    char rescId[MAX_NAME_LEN];
    char rescPath[MAX_NAME_LEN]="";
    char rescPathMsg[MAX_NAME_LEN+100];
    char commentStr[200];
    struct hostent *myHostEnt; // JMC - backport 4597

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc");

    if (rescName == NULL || option==NULL || optionValue==NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*rescName == '\0' || *option == '\0' || *optionValue=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4629
    if (strncmp(rescName, BUNDLE_RESC, strlen(BUNDLE_RESC))==0) {
        char errMsg[155];
        snprintf(errMsg, 150, 
                "%s is a built-in resource needed for bundle operations.", 
                BUNDLE_RESC);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_PSEUDO_RESC_MODIFY_DISALLOWED);
    }
    // =-=-=-=-=-=-=-
    status = getLocalZone();
    if (status != 0) return(status);

    rescId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 1 ");
    status = cmlGetStringValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
            rescId, MAX_NAME_LEN, rescName, localZone, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
        _rollback("chlModResc");
        return(status);
    }

    getNowStr(myTime);
    OK=0;
    if (strcmp(option, "info")==0) {
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 2");
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_info=?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "comment")==0) {
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 3");
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set r_comment = ?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "freespace")==0) {
        int inType=0;      /* regular mode, just set as provided */
        if (*optionValue=='+') {
            inType=1;       /* increment by the input value */
            optionValue++;  /* skip over the + */
        }
        if (*optionValue=='-') {
            inType=2;      /* decrement by the value */
            optionValue++; /* skip over the - */
        }
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 4");
        if (inType==0) {
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
        }
        if (inType==1) {
#if ORA_ICAT
            /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = cast(free_space as integer) + cast(? as integer), free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#elif MY_ICAT
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = free_space + ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#else
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = cast(free_space as bigint) + cast(? as bigint), free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#endif
        }
        if (inType==2) {
#if ORA_ICAT
            /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = cast(free_space as integer) - cast(? as integer), free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#elif MY_ICAT
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = free_space - ?, free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#else
            status =  cmlExecuteNoAnswerSql(
                    "update R_RESC_MAIN set free_space = cast(free_space as bigint) - cast(? as bigint), free_space_ts = ?, modify_ts=? where resc_id=?",
                    &icss);
#endif
        }
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "host")==0) {
        // =-=-=-=-=-=-=-
        // JMC - backport 4597
        myHostEnt = gethostbyname(optionValue);
        if (myHostEnt <= 0) {
            char errMsg[155];
            snprintf(errMsg, 150, 
                    "Warning, resource host address '%s' is not a valid DNS entry, gethostbyname failed.", 
                    optionValue);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }
        if (strcmp(optionValue, "localhost") == 0) { // JMC - backport 4650
            addRErrorMsg (&rsComm->rError, 0, 
                    "Warning, resource host address 'localhost' will not work properly as it maps to the local host from each client.");
        }

        // =-=-=-=-=-=-=-
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 5");
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_net = ?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }
    if (strcmp(option, "type")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 6");
#if 0 // JMC :: resource type is now dynamic
        status = cmlCheckNameToken("resc_type", optionValue, &icss);
        if (status !=0 ) {
            char errMsg[105];
            snprintf(errMsg, 100, "resource_type '%s' is not valid", 
                    optionValue);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_INVALID_RESOURCE_TYPE);
        }
#endif
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 7");
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_type_name = ?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }

#if 0 // JMC - no longer support resource classes
    if (strcmp(option, "class")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc S---Q---L 8");
        status = cmlCheckNameToken("resc_class", optionValue, &icss);
        if (status !=0 ) {
            char errMsg[105];
            snprintf(errMsg, 100, "resource_class '%s' is not valid", 
                    optionValue);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_INVALID_RESOURCE_CLASS);
        }

        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc S---Q---L 9");
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_class_name = ?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }
#endif
    if (strcmp(option, "path")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 10");
        status = cmlGetStringValueFromSql(
                "select resc_def_path from R_RESC_MAIN where resc_id=?",
                rescPath, MAX_NAME_LEN, rescId, 0, 0, &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlGetStringValueFromSql query failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 11");

        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_def_path=?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }

    if (strcmp(option, "status")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 12");
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_status=?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }

    if (strcmp(option, "name")==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 13");
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        /*    If the new name is not unique, this will return an error */
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_name=?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 14");
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=rescName;
        status =  cmlExecuteNoAnswerSql(
                "update R_DATA_MAIN set resc_name=? where resc_name=?",
                &icss);
        if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 15");
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=rescName;
        status =  cmlExecuteNoAnswerSql(
                "update R_SERVER_LOAD set resc_name=? where resc_name=?",
                &icss);
        if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }

        if (logSQL!=0) rodsLog(LOG_SQL, "chlModResc SQL 16");
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=rescName;
        status =  cmlExecuteNoAnswerSql(
                "update R_SERVER_LOAD_DIGEST set resc_name=? where resc_name=?",
                &icss);
        if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }

        OK=1;
    }

    if (strcmp(option, "context")==0) {
        cllBindVars[cllBindVarCount++]=optionValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=rescId;
        status =  cmlExecuteNoAnswerSql(
                "update R_RESC_MAIN set resc_context=?, modify_ts=? where resc_id=?",
                &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModResc cmlExecuteNoAnswerSql update failure for resc context %d",
                    status);
            _rollback("chlModResc");
            return(status);
        }
        OK=1;
    }

    if (OK==0) {
        return (CAT_INVALID_ARGUMENT);
    }

    /* Audit */
    snprintf(commentStr, sizeof commentStr, "%s %s", option, optionValue);
    status = cmlAudit3(AU_MOD_RESC,  
            rescId,
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone,
            commentStr,
            &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModResc cmlAudit3 failure %d",
                status);
        _rollback("chlModResc");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModResc cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    if (rescPath[0]!='\0') {
        /* if the path was gotten, return it */

        snprintf(rescPathMsg, sizeof(rescPathMsg), "Previous resource path: %s", 
                rescPath);
        addRErrorMsg (&rsComm->rError, 0, rescPathMsg);
    }

    return(0);
}

/* Modify a Resource Data Paths */
int chlModRescDataPaths(rsComm_t *rsComm, char *rescName, char *oldPath,
                       char *newPath, char *userName) {
   char rescId[MAX_NAME_LEN];
   int status, len, rows;
   char *cptr;
//   char userId[NAME_LEN]="";
   char userZone[NAME_LEN];
   char zoneToUse[NAME_LEN];
   char userName2[NAME_LEN];

   char oldPath2[MAX_NAME_LEN];

   if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescDataPaths");

   if (rescName == NULL || oldPath==NULL || newPath==NULL) {
      return (CAT_INVALID_ARGUMENT);
   }

   if (*rescName == '\0' || *oldPath == '\0' || *newPath=='\0') {
      return (CAT_INVALID_ARGUMENT);
   }

   /* the paths must begin and end with / */
   if (*oldPath != '/' or *newPath != '/') {
      return (CAT_INVALID_ARGUMENT);
   }
   len = strlen(oldPath);
   cptr = oldPath+len-1;
   if (*cptr != '/') return (CAT_INVALID_ARGUMENT);
   len = strlen(newPath);
   cptr = newPath+len-1;
   if (*cptr != '/') return (CAT_INVALID_ARGUMENT);

   if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   }
   if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   }

   status = getLocalZone();
   if (status != 0) return(status);

   rescId[0]='\0';
   if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescDataPaths SQL 1 ");
   status = cmlGetStringValueFromSql(
       "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
       rescId, MAX_NAME_LEN, rescName, localZone, 0, &icss);
   if (status != 0) {
      if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
      _rollback("chlModRescDataPaths");
      return(status);
   }

   /* This is needed for like clause which is needed to get the
      correct number of rows that were updated (seems like the DBMS will
      return a row count for rows looked at for the replace). */
   strncpy(oldPath2, oldPath, sizeof(oldPath2));
   strncat(oldPath2, "%", sizeof(oldPath2));

   if (userName != NULL && *userName != '\0') {
      strncpy(zoneToUse, localZone, NAME_LEN);
      status = parseUserName(userName, userName2, userZone);
      if (userZone[0]!='\0') {
        rstrcpy(zoneToUse, userZone, NAME_LEN);
      }

      if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescDataPaths SQL 2");
      cllBindVars[cllBindVarCount++]=oldPath;
      cllBindVars[cllBindVarCount++]=newPath;
      cllBindVars[cllBindVarCount++]=rescName;
      cllBindVars[cllBindVarCount++]=oldPath2;
      cllBindVars[cllBindVarCount++]=userName2;
      cllBindVars[cllBindVarCount++]=zoneToUse;
      status =  cmlExecuteNoAnswerSql(
        "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_name=? and data_path like ? and data_owner_name=? and data_owner_zone=?",
        &icss);
   }
   else {
      if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescDataPaths SQL 3");
      cllBindVars[cllBindVarCount++]=oldPath;
      cllBindVars[cllBindVarCount++]=newPath;
      cllBindVars[cllBindVarCount++]=rescName;
      cllBindVars[cllBindVarCount++]=oldPath2;
      status =  cmlExecuteNoAnswerSql(
        "update R_DATA_MAIN set data_path = replace (R_DATA_MAIN.data_path, ?, ?) where resc_name=? and data_path like ?",
        &icss);
   }
   if (status != 0) {
      rodsLog(LOG_NOTICE,
             "chlModRescDataPaths cmlExecuteNoAnswerSql update failure %d",
             status);
      _rollback("chlModResc");
      return(status);
   }

   rows = cllGetRowCount(&icss,-1);

   status =  cmlExecuteNoAnswerSql("commit", &icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE,
             "chlModResc cmlExecuteNoAnswerSql commit failure %d",
             status);
      return(status);
   }

   if (rows > 0) {
      char rowsMsg[100];
      snprintf(rowsMsg, 100, "%d rows updated", 
                 rows);
      status = addRErrorMsg (&rsComm->rError, 0, rowsMsg);
   }

   return(0);
}





/* Add or substract to the resource free_space */
int chlModRescFreeSpace(rsComm_t *rsComm, char *rescName, int updateValue) {
    int status;
    char myTime[50];
    char updateValueStr[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescFreeSpace");

    if (rescName == NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*rescName == '\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    /* The following checks may not be needed long term, but
       shouldn't hurt, for now.
    */

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    getNowStr(myTime);

    snprintf(updateValueStr,MAX_NAME_LEN, "%d", updateValue);

    cllBindVars[cllBindVarCount++]=updateValueStr;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=rescName;
               
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModRescFreeSpace SQL 1 ");
    status =  cmlExecuteNoAnswerSql(
        "update R_RESC_MAIN set free_space = ?, free_space_ts=? where resc_name=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRescFreeSpace cmlExecuteNoAnswerSql update failure %d",
                status);
        _rollback("chlModRescFreeSpace");
        return(status);
    }

    /* Audit */
    status = cmlAudit4(AU_MOD_RESC_FREE_SPACE,  
                       "select resc_id from R_RESC_MAIN where resc_name=?",
                       rescName,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       updateValueStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRescFreeSpace cmlAudit4 failure %d",
                status);
        _rollback("chlModRescFreeSpace");
        return(status);
    }

    return(0);
}

/* Add or remove a resource to/from a Resource Group */
int chlModRescGroup(rsComm_t *rsComm, char *rescGroupName, char *option,
                    char *rescName) {
    int status, OK;
    char myTime[50];
    char rescId[MAX_NAME_LEN];
    rodsLong_t seqNum;
    char rescGroupId[MAX_NAME_LEN];
    char dataObjNumber[MAX_NAME_LEN];
    char commentStr[200];

    if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group");

    if (rescGroupName == NULL || option==NULL || rescName==NULL) {
        return (CAT_INVALID_ARGUMENT);
    }

    if (*rescGroupName == '\0' || *option == '\0' || *rescName=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }
    if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    status = getLocalZone();
    if (status != 0) return(status);

    rescId[0]='\0';
    if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 1 ");
    status = cmlGetStringValueFromSql(
        "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
        rescId, MAX_NAME_LEN, rescName, localZone, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
        _rollback("chlModRescGroup");
        return(status);
    }

    getNowStr(myTime);
    OK=0;
    if (strcmp(option, "add")==0) {
        /* First try to look for a resc_group id with the same rescGrpName */
        rescGroupId[0]='\0';
        if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 2a ");
        status = cmlGetStringValueFromSql(
            "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
            rescGroupId, MAX_NAME_LEN, rescGroupName, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) {
                /* Generate a new id */
                if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 2b ");
                seqNum = cmlGetNextSeqVal(&icss);
                if (seqNum < 0) {
                    rodsLog(LOG_NOTICE, "chlModRescGroup cmlGetNextSeqVal failure %d",
                            seqNum);
                    _rollback("chlModRescGroup");
                    return(seqNum);
                }
                snprintf(rescGroupId, MAX_NAME_LEN, "%lld", seqNum);
            } else {
                _rollback("chlModRescGroup");
                return(status);
            }
        }
      
        cllBindVars[cllBindVarCount++]=rescGroupName;
        cllBindVars[cllBindVarCount++]=rescGroupId;
        cllBindVars[cllBindVarCount++]=rescId;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_RESC_GROUP (resc_group_name, resc_group_id, resc_id , create_ts, modify_ts) values (?, ?, ?, ?, ?)",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModRescGroup cmlExecuteNoAnswerSql insert failure %d",
                    status);
            _rollback("chlModRescGroup");
            return(status);
        }
        OK=1;
    }

    if (strcmp(option, "remove")==0) {
        /* Step 1 : get the resc_group_id as a dataObjNumber*/
        dataObjNumber[0]='\0';
        if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 3a ");
        status = cmlGetStringValueFromSql(
            "select distinct resc_group_id from R_RESC_GROUP where resc_id=? and resc_group_name=?",
            dataObjNumber, MAX_NAME_LEN, rescId, rescGroupName, 0, &icss);
        if (status != 0) {
            _rollback("chlModRescGroup");
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            return(status);
        }
      
        /* Step 2 : remove the (resc_group,resc) couple */
        cllBindVars[cllBindVarCount++]=rescGroupName;
        cllBindVars[cllBindVarCount++]=rescId;
        if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 3b");
        status =  cmlExecuteNoAnswerSql(
            "delete from R_RESC_GROUP where resc_group_name=? and resc_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModRescGroup cmlExecuteNoAnswerSql delete failure %d",
                    status);
            _rollback("chlModRescGroup");
            return(status);
        }
      
        /* Step 3 : look if the resc_group_name is still refered to */
        rescGroupId[0]='\0';
        if (logSQL!=0) rodsLog(LOG_SQL, "chl-Mod-Resc-Group S Q L 3c ");
        status = cmlGetStringValueFromSql(
            "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
            rescGroupId, MAX_NAME_LEN, rescGroupName, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) {
                /* The resource group exists no more */
                removeMetaMapAndAVU(dataObjNumber); /* remove AVU metadata, if any */
            }
        }
        OK=1;
    }

    if (OK==0) {
        return (CAT_INVALID_ARGUMENT);
    }

    /* Audit */
    snprintf(commentStr, sizeof commentStr, "%s %s", option, rescGroupName);
    status = cmlAudit3(AU_MOD_RESC_GROUP,  
                       rescId,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       commentStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRescGroup cmlAudit3 failure %d",
                status);
        _rollback("chlModRescGroup");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModRescGroup cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }
    return(0);
}


/// @brief function for validating a username
eirods::error validate_user_name(std::string _user_name) {

	// Must be between 3 and NAME_LEN-1 characters.
	// Must start and end with a word character.
	// May contain non consecutive dashes and dots.
	boost::regex re("^(?=.{3,63}$)\\w(\\w*([.-]\\w+)?)*$");

	if (!boost::regex_match(_user_name, re)) {
        std::stringstream msg;
        msg << "validate_user_name failed for user [";
        msg << _user_name;
        msg << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    return SUCCESS();

} // validate_user_name


/* Register a User, RuleEngine version */
int chlRegUserRE(rsComm_t *rsComm, userInfo_t *userInfo) {
    char myTime[50];
    int status;
    char seqStr[MAX_NAME_LEN];
    char auditSQL[MAX_SQL_SIZE];
    char userZone[MAX_NAME_LEN];
    char zoneId[MAX_NAME_LEN];

    int zoneForm;
    char userName2[NAME_LEN];
    char zoneName[NAME_LEN];

    static char lastValidUserType[MAX_NAME_LEN]="";
    static char userTypeTokenName[MAX_NAME_LEN]="";

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (!userInfo) {
        return(SYS_INTERNAL_NULL_INPUT_ERR);
    }


    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ||
        rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        int status2;
        status2  = cmlCheckGroupAdminAccess(
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone, 
            "",
            &icss);
        if (status2 != 0) return(status2);
        creatingUserByGroupAdmin=1;
    }
    // =-=-=-=-=-=-=-
    /*
      Check if the user type is valid.
      This check is skipped if this process has already verified this type
      (iadmin doing a series of mkuser subcommands).
    */
    if ( *userInfo->userType=='\0' || 
         strcmp(userInfo->userType, lastValidUserType)!=0 ) {
        char errMsg[105];
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE SQL 1 ");
        status = cmlGetStringValueFromSql(
            "select token_name from R_TOKN_MAIN where token_namespace='user_type' and token_name=?", 
            userTypeTokenName, MAX_NAME_LEN, userInfo->userType, 0, 0, &icss);
        if (status==0) {
            strncpy(lastValidUserType, userInfo->userType, MAX_NAME_LEN);
        }
        else {
            snprintf(errMsg, 100, "user_type '%s' is not valid", 
                     userInfo->userType);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_INVALID_USER_TYPE);
        }
    }

    status = getLocalZone();
    if (status != 0) return(status);

    if (strlen(userInfo->rodsZone)>0) {
        zoneForm=1;
        strncpy(userZone, userInfo->rodsZone, MAX_NAME_LEN);
    }
    else {
        zoneForm=0;
        strncpy(userZone, localZone, MAX_NAME_LEN);
    }

    status = parseUserName(userInfo->userName, userName2, zoneName);
    if (zoneName[0]!='\0') {
        rstrcpy(userZone, zoneName, NAME_LEN);
        zoneForm=2;
    }
    if (status != 0) {
        return (CAT_INVALID_ARGUMENT);
    }


    // =-=-=-=-=-=-=-
    // Validate user name format
    eirods::error ret = validate_user_name(userName2);
    if(!ret.ok()) {
        eirods::log(ret);
        return SYS_INVALID_INPUT_PARAM;
    }
    // =-=-=-=-=-=-=-


    if (zoneForm) {
        /* check that the zone exists (if not defaulting to local) */
        zoneId[0]='\0';
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE SQL 5 ");
        status = cmlGetStringValueFromSql(
            "select zone_id from R_ZONE_MAIN where zone_name=?",
            zoneId, MAX_NAME_LEN, userZone, "", 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) {
                char errMsg[105];
                snprintf(errMsg, 100, 
                         "zone '%s' does not exist",
                         userZone);
                addRErrorMsg (&rsComm->rError, 0, errMsg);
                return(CAT_INVALID_ZONE);
            }
            return(status);
        }
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE SQL 2");
    status = cmlGetNextSeqStr(seqStr, MAX_NAME_LEN, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE, "chlRegUserRE cmlGetNextSeqStr failure %d",
                status);
        return(status);
    }

    getNowStr(myTime);

    cllBindVars[cllBindVarCount++]=seqStr;
    cllBindVars[cllBindVarCount++]=userName2;
    cllBindVars[cllBindVarCount++]=userTypeTokenName;
    cllBindVars[cllBindVarCount++]=userZone;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE SQL 3");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_USER_MAIN (user_id, user_name, user_type_name, zone_name, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
        &icss);

    if (status != 0) {
        if (status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
            char errMsg[105];
            snprintf(errMsg, 100, "Error %d %s",
                     status,
                     "CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME"
                );
            addRErrorMsg (&rsComm->rError, 0, errMsg);
        }
        _rollback("chlRegUserRE");
        rodsLog(LOG_NOTICE,
                "chlRegUserRE insert failure %d",status);
        return(status);
    }


    cllBindVars[cllBindVarCount++]=seqStr;
    cllBindVars[cllBindVarCount++]=seqStr;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegUserRE SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_USER_GROUP (group_user_id, user_id, create_ts, modify_ts) values (?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegUserRE insert into R_USER_GROUP failure %d",status);
        _rollback("chlRegUserRE");
        return(status);
    }


/*
  The case where the caller is specifying an authstring is used in
  some specialized cases.  Using the new table (Aug 12, 2009), this
  is now set via the chlModUser call below.  This is untested tho.
*/
    if (strlen(userInfo->authInfo.authStr) > 0) {
        status = chlModUser(rsComm, userInfo->userName, "addAuth",
                            userInfo->authInfo.authStr);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRegUserRE chlModUser insert auth failure %d",status);
            _rollback("chlRegUserRE");
            return(status);
        }
    }

    /* Audit */
    snprintf(auditSQL, MAX_SQL_SIZE-1,
             "select user_id from R_USER_MAIN where user_name=? and zone_name='%s'",
             userZone);
    status = cmlAudit4(AU_REGISTER_USER_RE,  
                       auditSQL,
                       userName2,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       userZone,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegUserRE cmlAudit4 failure %d",
                status);
        _rollback("chlRegUserRE");
        return(status);
    }


    return(status);
}

int
convertTypeOption(char *typeStr) {
    if (strcmp(typeStr, "-d") == 0) return(1); /* dataObj */
    if (strcmp(typeStr, "-D") == 0) return(1); /* dataObj */
    if (strcmp(typeStr, "-c") == 0) return(2); /* collection */
    if (strcmp(typeStr, "-C") == 0) return(2); /* collection */
    if (strcmp(typeStr, "-r") == 0) return(3); /* resource */
    if (strcmp(typeStr, "-R") == 0) return(3); /* resource */
    if (strcmp(typeStr, "-u") == 0) return(4); /* user */
    if (strcmp(typeStr, "-U") == 0) return(4); /* user */
    if (strcmp(typeStr, "-g") == 0) return(5); /* resource group */
    if (strcmp(typeStr, "-G") == 0) return(5); /* resource group */
    return (0);
}

/*
  Check object - get an object's ID and check that the user has access.
  Called internally.
*/
rodsLong_t checkAndGetObjectId(rsComm_t *rsComm, char *type,
                               char *name, char *access) {
    int itype;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t status;
    rodsLong_t objId;
    char userName[NAME_LEN];
    char userZone[NAME_LEN];


    if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (type == NULL || *type=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (name == NULL || *name=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    itype = convertTypeOption(type);
    if (itype==0) return(CAT_INVALID_ARGUMENT);

    if (itype==1) {
        status = splitPathByKey(name,
                                logicalParentDirName, logicalEndName, '/');
        if (strlen(logicalParentDirName)==0) {
            strcpy(logicalParentDirName, "/");
            strcpy(logicalEndName, name);
        }
        if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId SQL 1 ");
        status = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                     rsComm->clientUser.userName, 
                                     rsComm->clientUser.rodsZone, 
                                     access, &icss);
        if (status < 0) {
            _rollback("checkAndGetObjectId");
            return(status);
        }
        objId=status;
    }

    if (itype==2) {
        /* Check that the collection exists and user has create_metadata permission,
           and get the collectionID */
        if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId SQL 2");
        status = cmlCheckDir(name,
                             rsComm->clientUser.userName, 
                             rsComm->clientUser.rodsZone,
                             access, &icss); 
        if (status < 0) {
            char errMsg[105];
            if (status == CAT_UNKNOWN_COLLECTION) {
                snprintf(errMsg, 100, "collection '%s' is unknown",
                         name);
                addRErrorMsg (&rsComm->rError, 0, errMsg);
            }
            return(status);
        }
        objId=status;
    }

    if (itype==3) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId SQL 3");
        status = cmlGetIntegerValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
            &objId, name, localZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            _rollback("checkAndGetObjectId");
            return(status);
        }
    }

    if (itype==4) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = parseUserName(name, userName, userZone);
        if (userZone[0]=='\0') {
            status = getLocalZone();
            if (status != 0) return(status);
            strncpy(userZone, localZone, NAME_LEN);
        }

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId SQL 4");
        status = cmlGetIntegerValueFromSql(
            "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
            &objId, userName, userZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
            _rollback("checkAndGetObjectId");
            return(status);
        }
    }
   
    if (itype==5) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "checkAndGetObjectId S Q L 5");
        status = cmlGetIntegerValueFromSql(
            "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
            &objId, name, 0, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            _rollback("checkAndGetObjectId");
            return(status);
        }
    }
   
    return(objId);
}
/*
// =-=-=-=-=-=-=-
// JMC - backport 4836
+ Find existing AVU triplet.
+ Return code is error or the AVU ID.
+*/
rodsLong_t
findAVU(char *attribute, char *value, char *units) {
    rodsLong_t status;
// =-=-=-=-=-=-=-

    rodsLong_t iVal;
    iVal=0;
    if (*units!='\0') {
        if (logSQL!=0) rodsLog(LOG_SQL, "findAVU SQL 1"); // JMC - backport 4836
        status = cmlGetIntegerValueFromSql(
            "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and meta_attr_unit=?",
            &iVal, attribute, value, units, 0, 0, &icss);
    }
    else {
        if (logSQL!=0) rodsLog(LOG_SQL, "findAVU SQL 2");
        status = cmlGetIntegerValueFromSql(
            "select meta_id from R_META_MAIN where meta_attr_name=? and meta_attr_value=? and (meta_attr_unit='' or meta_attr_unit IS NULL)", // JMC - backport 4827
            &iVal, attribute, value, 0, 0, 0, &icss);
    }
    if (status == 0) {
        status = iVal; /* use existing R_META_MAIN row */
        return(status);
    }
// =-=-=-=-=-=-=-
// JMC - backport 4836
    return(status); // JMC - backport 4836
}

/*
  Find existing or insert a new AVU triplet.
  Return code is error, or the AVU ID.
*/
int
findOrInsertAVU(char *attribute, char *value, char *units) {
    char nextStr[MAX_NAME_LEN];
    char myTime[50];
    rodsLong_t status, seqNum;
    rodsLong_t iVal;
    iVal = findAVU(attribute, value, units);
    if (iVal > 0) {
        return iVal;
    }
    if (logSQL!=0) rodsLog(LOG_SQL, "findOrInsertAVU SQL 1");
// =-=-=-=-=-=-=-
    status = cmlGetNextSeqVal(&icss);
    if (status < 0) {
        rodsLog(LOG_NOTICE, "findAVU cmlGetNextSeqVal failure %d",
                status);
        return(status);
    }
    seqNum = status; /* the returned status is the next sequence value */

    snprintf(nextStr, sizeof nextStr, "%lld", seqNum);

    getNowStr(myTime);

    cllBindVars[cllBindVarCount++]=nextStr;
    cllBindVars[cllBindVarCount++]=attribute;
    cllBindVars[cllBindVarCount++]=value;
    cllBindVars[cllBindVarCount++]=units;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;

    if (logSQL!=0) rodsLog(LOG_SQL, "findOrInsertAVU SQL 2"); // JMC - backport 4836
    status =  cmlExecuteNoAnswerSql(
        "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
        &icss);
    if (status < 0) {
        rodsLog(LOG_NOTICE, "findOrInsertAVU insert failure %d", status);
        return(status);
    }
    return(seqNum);
}

// =-=-=-=-=-=-=-
// JMC - backport 4836
/* Add or modify an Attribute-Value pair metadata item of an object*/
int chlSetAVUMetadata(rsComm_t *rsComm, char *type, 
                      char *name, char *attribute, char *newValue,
                      char *newUnit) {
    int status;
    char myTime[50];
    rodsLong_t objId;
    char metaIdStr[MAX_NAME_LEN*2]; /* twice as needed to query multiple */
    char objIdStr[MAX_NAME_LEN];

    memset(metaIdStr, 0, sizeof(metaIdStr));
    if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata SQL 1 ");
    objId = checkAndGetObjectId(rsComm, type, name, ACCESS_CREATE_METADATA);
    if (objId < 0) return objId;
    snprintf(objIdStr, MAX_NAME_LEN, "%lld", objId);

    if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata SQL 2");
    /* Query to see if the attribute exists for this object */
    status = cmlGetMultiRowStringValuesFromSql("select meta_id from R_OBJT_METAMAP where meta_id in (select meta_id from R_META_MAIN where meta_attr_name=? AND meta_id in (select meta_id from R_OBJT_METAMAP where object_id=?))",
                                               metaIdStr, MAX_NAME_LEN, 2, attribute, objIdStr, &icss);
   
    if (status <= 0) {
        if (status == CAT_NO_ROWS_FOUND) { 
            /* Need to add the metadata */
            status = chlAddAVUMetadata(rsComm, 0, type, name, attribute, 
                                       newValue, newUnit);
        } else {
            rodsLog(LOG_NOTICE,
                    "chlSetAVUMetadata cmlGetMultiRowStringValuesFromSql failure %d",
                    status);
        }
        return status;
    }

    if (status > 1) {
        /* More than one AVU, need to do a delete with wildcards then add */
        status = chlDeleteAVUMetadata(rsComm, 1, type, name, attribute, "%",
                                      "%", 1);
        if (status != 0) {
            _rollback("chlSetAVUMetadata");
            return(status);
        }
        status = chlAddAVUMetadata(rsComm, 0, type, name, attribute, 
                                   newValue, newUnit);
        return status;
    }

    /* Only one metaId for this Attribute and Object has been found */

    rodsLog(LOG_NOTICE, "chlSetAVUMetadata found metaId %s", metaIdStr);
    /* Check if there are other objects are using this AVU  */
    if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata SQL 4");
    status = cmlGetMultiRowStringValuesFromSql("select meta_id from R_META_MAIN where meta_attr_name=?",
                                               metaIdStr, MAX_NAME_LEN, 2, attribute, objIdStr, &icss);
    if (status <= 0) {
        rodsLog(LOG_NOTICE,
                "chlSetAVUMetadata cmlGetMultiRowStringValueFromSql failure %d",
                status);
        return(status);
    }
    if (status > 1) {
        /* Can't modify in place, need to delete and add,
           which will reuse matching AVUs if they exist.
        */
        status = chlDeleteAVUMetadata(rsComm, 1, type, name, attribute,
                                      "%", "%", 1);
        if (status != 0) {
            _rollback("chlSetAVUMetadata");
            return(status);
        }
        status = chlAddAVUMetadata(rsComm, 0, type, name, attribute,
                                   newValue, newUnit);
    }
    else {
        getNowStr(myTime);
        cllBindVarCount = 0;
        cllBindVars[cllBindVarCount++] = newValue;
        if (newUnit != NULL && *newUnit!='\0') {
            cllBindVars[cllBindVarCount++] = newUnit;
        }
        cllBindVars[cllBindVarCount++] = myTime;
        cllBindVars[cllBindVarCount++] = attribute;
        cllBindVars[cllBindVarCount++] = metaIdStr;
        if (newUnit != NULL && *newUnit!='\0') {
            if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata SQL 5");
            status = cmlExecuteNoAnswerSql(
                "update R_META_MAIN set meta_attr_value=?,meta_attr_unit=?,modify_ts=? where meta_attr_name=? and meta_id=?",
                &icss);
        } 
        else {
            if (logSQL != 0) rodsLog(LOG_SQL, "chlSetAVUMetadata SQL 6");
            status = cmlExecuteNoAnswerSql(
                "update R_META_MAIN set meta_attr_value=?,modify_ts=? where meta_attr_name=? and meta_id=?",
                &icss);
        }
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlSetAVUMetadata cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlSetAVUMetadata");
            return(status);
        }
    }

    /* Audit */
    status = cmlAudit3(AU_ADD_AVU_METADATA,  
                       objIdStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       type,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlSetAVUMetadata cmlAudit3 failure %d",
                status);
        _rollback("chlSetAVUMetadata");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlSetAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(status);
}
// =-=-=-=-=-=-=-

/* Add an Attribute-Value [Units] pair/triple metadata item to one or
   more data objects.  This is the Wildcard version, where the
   collection/data-object name can match multiple objects).

   The return value is error code (negative) or the number of objects
   to which the AVU was associated.
*/
#define ACCESS_MAX 999999  /* A large access value (larger than the
                              maximum used (i.e. for fail safe)) and
                              also indicates not initialized*/
int 
chlAddAVUMetadataWild(rsComm_t *rsComm, int adminMode, char *type, 
                      char *name, char *attribute, char *value,  char *units) {
    rodsLong_t status, status2;
    rodsLong_t seqNum;
    int numObjects;
    int nAccess=0;
    static int accessNeeded=ACCESS_MAX;
    rodsLong_t iVal;
    char collection[MAX_NAME_LEN];
    char objectName[MAX_NAME_LEN];
    char myTime[50];
    char seqNumStr[MAX_NAME_LEN];
    int itype;

    itype = convertTypeOption(type);
    if (itype!=1) return(CAT_INVALID_ARGUMENT);  /* only -d for now */

    status = splitPathByKey(name, collection, objectName, '/');
    if (strlen(collection)==0) {
        strcpy(collection, "/");
        strcpy(objectName, name);
    }

/*
  The following SQL is somewhat complicated, but evaluates the access
  permissions in steps to reduce the complexity and so it can scale
  well.  Altho there are multiple SQL calls, including creating two
  views, the scaling burden is placed on the DBMS, so it should
  perform well even for many thousands of objects at a time.
*/

/* Get the count of the objects to compare with later */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 1");
    status = cmlGetIntegerValueFromSql(
        "select count(DISTINCT DM.data_id) from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ?",
        &iVal, objectName, collection, 0, 0, 0, &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild get count failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }
    numObjects = iVal;
    if (numObjects == 0) {
        return(CAT_NO_ROWS_FOUND);
    }

/* 
   Create a view with all the access permissions for this user, or
   groups this user is a member of, for all the matching data-objects.
*/
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 2");
#if ORA_ICAT
    /* For Oracle, we cannot use views with bind-variables, so use a
       table instead. */
    status =  cmlExecuteNoAnswerSql("purge recyclebin",
                                    &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (drop table ACCESS_VIEW_ONE) failure %d",
                status);
    }
    status =  cmlExecuteNoAnswerSql("drop table ACCESS_VIEW_ONE",
                                    &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (drop table ACCESS_VIEW_ONE) failure %d",
                status);
    }

    status =  cmlExecuteNoAnswerSql("create table ACCESS_VIEW_ONE (access_type_id integer, data_id integer)",
                                    &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (create table ACCESS_VIEW_ONE) failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }

    cllBindVars[cllBindVarCount++]=objectName;
    cllBindVars[cllBindVarCount++]=collection;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.userName;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.rodsZone;
    status =  cmlExecuteNoAnswerSql(
        "insert into ACCESS_VIEW_ONE (access_type_id, data_id) (select access_type_id, DM.data_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id)",
        &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status==CAT_NO_ROWS_FOUND) status=CAT_NO_ACCESS_PERMISSION;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }
#else
    cllBindVars[cllBindVarCount++]=objectName;
    cllBindVars[cllBindVarCount++]=collection;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.userName;
    cllBindVars[cllBindVarCount++]=rsComm->clientUser.rodsZone;
    status =  cmlExecuteNoAnswerSql(
        "create view ACCESS_VIEW_ONE as select access_type_id, DM.data_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id",
        &icss);
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status==CAT_NO_ROWS_FOUND) status=CAT_NO_ACCESS_PERMISSION;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }
#endif

/* Create another view for min below (sub select didn't work).  This
   is the set of access permisions per matching data-object, the best
   permision values (for example, if user has write and has
   group-based read, this will be 'write').
*/
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 3");
#if (defined ORA_ICAT || defined MY_ICAT)
    status =  cmlExecuteNoAnswerSql(
        "create or replace view ACCESS_VIEW_TWO as select max(access_type_id) max from ACCESS_VIEW_ONE group by data_id",
        &icss);
#else
    status =  cmlExecuteNoAnswerSql(
        "create or replace view ACCESS_VIEW_TWO as select max(access_type_id) from ACCESS_VIEW_ONE group by data_id",
        &icss);
#endif
    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) status=0;
    if (status==CAT_NO_ROWS_FOUND) status=CAT_NO_ACCESS_PERMISSION;
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql (create view) failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }      

    if (accessNeeded>=ACCESS_MAX) { /* not initialized yet */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 4");
        status = cmlGetIntegerValueFromSql(
            "select token_id  from R_TOKN_MAIN where token_name = 'modify metadata' and token_namespace = 'access_type'",
            &iVal, 0, 0, 0, 0, 0, &icss);
        if (status==0) accessNeeded = iVal;
    }

    /* Get the minimum access permissions for the whole set of
     * data-objects that match */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 5");
    iVal=-1;
    status = cmlGetIntegerValueFromSql(
        "select min(max) from ACCESS_VIEW_TWO",
        &iVal, 0, 0, 0, 0, 0, &icss);

    if (status==CAT_NO_ROWS_FOUND) status=CAT_NO_ACCESS_PERMISSION;

    if (status==0) {
        if (iVal < accessNeeded) { 
            status = CAT_NO_ACCESS_PERMISSION;
        }
    }

/* Get the count of the access permissions for the set of
 * data-objects, since if there are completely missing access
 * permissions (NULL) they won't show up in the above query */
    if (status==0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 6");
        status = cmlGetIntegerValueFromSql(
            "select count(*) from ACCESS_VIEW_TWO",
            &iVal, 0, 0, 0, 0, 0, &icss);
        if (status==0) {
            nAccess = iVal;

            if (numObjects > nAccess) {
                status=CAT_NO_ACCESS_PERMISSION;
            }
        }
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 7");
#if ORA_ICAT
    status2 =  cmlExecuteNoAnswerSql(
        "drop table ACCESS_VIEW_ONE",
        &icss);
    if (status2==CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
    if (status2==0) {
        status2 =  cmlExecuteNoAnswerSql(
            "drop view ACCESS_VIEW_TWO",
            &icss);
        if (status2==CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
    }
#else
    status2 =  cmlExecuteNoAnswerSql(
        "drop view ACCESS_VIEW_TWO, ACCESS_VIEW_ONE",
        &icss);
    if (status2==CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
#endif

    if (status2 != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild cmlExecuteNoAnswerSql (drop view (or table)) failure %d",
                status2);
    }

    if (status != 0) return(status);

/* 
   Now the easy part, set up the AVU and associate it with the data-objects
*/
    status = findOrInsertAVU(attribute, value, units);
    if (status<0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild findOrInsertAVU failure %d",
                status);
        _rollback("chlAddAVUMetadata");
        return(status);
    }
    seqNum = status;

    getNowStr(myTime);
    snprintf(seqNumStr, sizeof seqNumStr, "%lld", seqNum);
    cllBindVars[cllBindVarCount++]=seqNumStr;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=objectName;
    cllBindVars[cllBindVarCount++]=collection;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadataWild SQL 8");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) select DM.data_id, ?, ?, ? from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name like ? and DM.coll_id=CM.coll_id and CM.coll_name like ? group by DM.data_id",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild cmlExecuteNoAnswerSql insert failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_ADD_AVU_WILD_METADATA,  
                       seqNumStr,  /* for WILD, record the AVU id */
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       name,       /* and the input wildcard path */
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild cmlAudit3 failure %d",
                status);
        _rollback("chlAddAVUMetadataWild");
        return(status);
    }


    /* Commit */
    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadataWild cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    if (status != 0) return(status);
    return(numObjects);
}


/* Add an Attribute-Value [Units] pair/triple metadata item to an object */
int chlAddAVUMetadata(rsComm_t *rsComm, int adminMode, char *type, 
                      char *name, char *attribute, char *value,  char *units) {
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

    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (type == NULL || *type=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (name == NULL || *name=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (attribute == NULL || *attribute=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (value == NULL || *value=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (adminMode==1) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }

    if (units == NULL) units="";

    itype = convertTypeOption(type);
    if (itype==0) return(CAT_INVALID_ARGUMENT);

    if (itype==1) {
        status = splitPathByKey(name,
                                logicalParentDirName, logicalEndName, '/');
        if (strlen(logicalParentDirName)==0) {
            strcpy(logicalParentDirName, "/");
            strcpy(logicalEndName, name);
        }
        if (adminMode==1) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 1 ");
            status = cmlGetIntegerValueFromSql(
                "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
                &iVal, logicalEndName, logicalParentDirName, 0, 0, 0, &icss);
            if (status==0) status=iVal; /*like cmlCheckDataObjOnly, status is objid */
        }
        else {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 2");
            status = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                         rsComm->clientUser.userName, 
                                         rsComm->clientUser.rodsZone, 
                                         ACCESS_CREATE_METADATA, &icss);
        }
        if (status < 0) {
            _rollback("chlAddAVUMetadata");
            return(status);
        }
        objId=status;
    }

    if (itype==2) {
        if (adminMode==1) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 3");
            status = cmlGetIntegerValueFromSql(
                "select coll_id from R_COLL_MAIN where coll_name=?",
                &iVal, name, 0, 0, 0, 0, &icss);
            if (status==0) status=iVal;/*like cmlCheckDir, status is objid*/
        }
        else {
            /* Check that the collection exists and user has create_metadata 
               permission, and get the collectionID */
            if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 4");
            status = cmlCheckDir(name,
                                 rsComm->clientUser.userName, 
                                 rsComm->clientUser.rodsZone,
                                 ACCESS_CREATE_METADATA, &icss);
        }
        if (status < 0) {
            char errMsg[105];
            _rollback("chlAddAVUMetadata");
            if (status == CAT_UNKNOWN_COLLECTION) {
                snprintf(errMsg, 100, "collection '%s' is unknown",
                         name);
                addRErrorMsg (&rsComm->rError, 0, errMsg);
            } else {
                _rollback("chlAddAVUMetadata");
            }
            return(status);
        }
        objId=status;
    }

    if (itype==3) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 5");
        status = cmlGetIntegerValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
            &objId, name, localZone, 0, 0, 0, &icss);
        if (status != 0) {
            _rollback("chlAddAVUMetadata");
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            return(status);
        }
    }

    if (itype==4) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = parseUserName(name, userName, userZone);
        if (userZone[0]=='\0') {
            status = getLocalZone();
            if (status != 0) return(status);
            strncpy(userZone, localZone, NAME_LEN);
        }

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 6");
        status = cmlGetIntegerValueFromSql(
            "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
            &objId, userName, userZone, 0, 0, 0, &icss);
        if (status != 0) {
            _rollback("chlAddAVUMetadata");
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
            return(status);
        }
    }

    if (itype==5) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 7");
        status = cmlGetIntegerValueFromSql(
            "select distinct resc_group_id from R_RESC_GROUP where resc_group_name=?",
            &objId, name, 0, 0, 0, 0, &icss);
        if (status != 0) {
            _rollback("chlAddAVUMetadata");
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            return(status);
        }
    }
   
    status = findOrInsertAVU(attribute, value, units);
    if (status<0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata findOrInsertAVU failure %d",
                status);
        _rollback("chlAddAVUMetadata");
        return(status);
    }
    seqNum = status;

    getNowStr(myTime);
    snprintf(objIdStr, sizeof objIdStr, "%lld", objId);
    snprintf(seqNumStr, sizeof seqNumStr, "%lld", seqNum);
    cllBindVars[cllBindVarCount++]=objIdStr;
    cllBindVars[cllBindVarCount++]=seqNumStr;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddAVUMetadata SQL 7");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) values (?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql insert failure %d",
                status);
        _rollback("chlAddAVUMetadata");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_ADD_AVU_METADATA,  
                       objIdStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       type,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlAudit3 failure %d",
                status);
        _rollback("chlAddAVUMetadata");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(status);
}

/*
  check a chlModAVUMetadata argument; returning the type.
*/
int
checkModArgType(char *arg) {
    if (arg == NULL || *arg=='\0') 
        return(CAT_INVALID_ARGUMENT);
    if (*(arg+1)!=':') return(0); /* not one */
    if (*arg=='n') return(1);
    if (*arg=='v') return(2);
    if (*arg=='u') return(3);
    return(0);
}

/* Modify an Attribute-Value [Units] pair/triple metadata item of an object*/
int chlModAVUMetadata(rsComm_t *rsComm, char *type, 
                      char *name, char *attribute, char *value,  
                      char *unitsOrArg0, char *arg1, char *arg2, char *arg3) {
    int status, atype;
    char myUnits[MAX_NAME_LEN]="";
    char *addAttr="", *addValue="", *addUnits="";
    int newUnits=0;
    if (unitsOrArg0 == NULL || *unitsOrArg0=='\0') 
        return(CAT_INVALID_ARGUMENT);
    atype = checkModArgType(unitsOrArg0);
    if (atype==0) strncpy(myUnits, unitsOrArg0, MAX_NAME_LEN);

    status = chlDeleteAVUMetadata(rsComm, 0, type, name, attribute, value, 
                                  myUnits, 1);
    if (status != 0) {
        _rollback("chlModAVUMetadata");
        return(status);
    }

    if (atype==1) {
        addAttr=unitsOrArg0+2;
    }
    if (atype==2) {
        addValue=unitsOrArg0+2;
    }
    if (atype==3) {
        addUnits=unitsOrArg0+2;
    }

    atype = checkModArgType(arg1);
    if (atype==1) {
        addAttr=arg1+2;
    }
    if (atype==2) {
        addValue=arg1+2;
    }
    if (atype==3) {
        addUnits=arg1+2;
    }

    atype = checkModArgType(arg2);
    if (atype==1) {
        addAttr=arg2+2;
    }
    if (atype==2) {
        addValue=arg2+2;
    }
    if (atype==3) {
        addUnits=arg2+2;
    }

    atype = checkModArgType(arg3);
    if (atype==1) {
        addAttr=arg3+2;
    }
    if (atype==2) {
        addValue=arg3+2;
    }
    if (atype==3) {
        addUnits=arg3+2;
        newUnits=1;
    }

    if (*addAttr=='\0' &&
        *addValue=='\0' &&
        *addUnits=='\0') {
        _rollback("chlModAVUMetadata");
        return (CAT_INVALID_ARGUMENT);
    }

    if (*addAttr=='\0') addAttr=attribute;
    if (*addValue=='\0') addValue=value;
    if (*addUnits=='\0' && newUnits==0) addUnits=myUnits;

    status = chlAddAVUMetadata(rsComm, 0, type, name, addAttr, addValue,
                               addUnits);
    return(status);

}

/* Delete an Attribute-Value [Units] pair/triple metadata item from an object*/
/* option is 0: normal, 1: use wildcards, 2: input is id not type,name,units */
/* noCommit: if 1: skip the commit (only used by chlModAVUMetadata) */
int chlDeleteAVUMetadata(rsComm_t *rsComm, int option, char *type, 
                         char *name, char *attribute, char *value,  
                         char *units, int noCommit ) {
    int itype;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    rodsLong_t status;
    rodsLong_t objId;
    char objIdStr[MAX_NAME_LEN];
    int allowNullUnits;
    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (type == NULL || *type=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (name == NULL || *name=='\0') {
        return (CAT_INVALID_ARGUMENT);
    }

    if (option != 2) {
        if (attribute == NULL || *attribute=='\0') {
            return (CAT_INVALID_ARGUMENT);
        }

        if (value == NULL || *value=='\0') {
            return (CAT_INVALID_ARGUMENT);
        }
    }

    if (units == NULL) units="";

    itype = convertTypeOption(type);
    if (itype==0) return(CAT_INVALID_ARGUMENT);

    if (itype==1) {
        status = splitPathByKey(name,
                                logicalParentDirName, logicalEndName, '/');
        if (strlen(logicalParentDirName)==0) {
            strcpy(logicalParentDirName, "/");
            strcpy(logicalEndName, name);
        }
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 1 ");
        status = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                     rsComm->clientUser.userName, 
                                     rsComm->clientUser.rodsZone, 
                                     ACCESS_DELETE_METADATA, &icss);
        if (status < 0) {
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }
        objId=status;
    }

    if (itype==2) {
        /* Check that the collection exists and user has delete_metadata permission,
           and get the collectionID */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 2");
        status = cmlCheckDir(name,
                             rsComm->clientUser.userName, 
                             rsComm->clientUser.rodsZone,
                             ACCESS_DELETE_METADATA, &icss);
        if (status < 0) {
            char errMsg[105];
            if (status == CAT_UNKNOWN_COLLECTION) {
                snprintf(errMsg, 100, "collection '%s' is unknown",
                         name);
                addRErrorMsg (&rsComm->rError, 0, errMsg);
            }
            return(status);
        }
        objId=status;
    }

    if (itype==3) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 3");
        status = cmlGetIntegerValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
            &objId, name, localZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }
    }

    if (itype==4) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = parseUserName(name, userName, userZone);
        if (userZone[0]=='\0') {
            status = getLocalZone();
            if (status != 0) return(status);
            strncpy(userZone, localZone, NAME_LEN);
        }

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 4");
        status = cmlGetIntegerValueFromSql(
            "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
            &objId, userName, userZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }
    }
   
    if (itype==5) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }

        status = getLocalZone();
        if (status != 0) return(status);

        objId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 5");
        status = cmlGetIntegerValueFromSql(
            "select resc_group_id from R_RESC_GROUP where resc_group_name=?",
            &objId, name, 0, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }
    }


    snprintf(objIdStr, MAX_NAME_LEN, "%lld", objId);

    if (option==2) {
        cllBindVars[cllBindVarCount++]=objIdStr;
        cllBindVars[cllBindVarCount++]=attribute; /* attribute is really id */

        if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 9");
        status =  cmlExecuteNoAnswerSql(
            "delete from R_OBJT_METAMAP where object_id=? and meta_id =?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure %d",
                    status);
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }

        /* Remove unused AVU rows, if any */
#ifdef METADATA_CLEANUP
        removeAVUs();
#endif

        /* Audit */
        status = cmlAudit3(AU_DELETE_AVU_METADATA,  
                           objIdStr,
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           type,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlDeleteAVUMetadata cmlAudit3 failure %d",
                    status);
            _rollback("chlDeleteAVUMetadata");
            return(status);
        }

        if (noCommit != 1) {
            status =  cmlExecuteNoAnswerSql("commit", &icss);
            if (status != 0) {
                rodsLog(LOG_NOTICE,
                        "chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                        status);
                return(status);
            }
        }
        return(status);
    }

    cllBindVars[cllBindVarCount++]=objIdStr;
    cllBindVars[cllBindVarCount++]=attribute;
    cllBindVars[cllBindVarCount++]=value;
    cllBindVars[cllBindVarCount++]=units;

    allowNullUnits=0;
    if (*units=='\0') {
        allowNullUnits=1;  /* null or empty-string units */
    }
    if (option==1 && *units=='%' && *(units+1)=='\0') {
        allowNullUnits=1; /* wildcard and just % */
    }

    if (allowNullUnits) {
        if (option==1) {  /* use wildcards ('like') */
            if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 5");
            status =  cmlExecuteNoAnswerSql(
                "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and (meta_attr_unit like ? or meta_attr_unit IS NULL) )",
                &icss);
        }
        else {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 6");
            status =  cmlExecuteNoAnswerSql(
                "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and (meta_attr_unit = ? or meta_attr_unit IS NULL) )",
                &icss);
        }
    }
    else {
        if (option==1) {  /* use wildcards ('like') */
            if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 7");
            status =  cmlExecuteNoAnswerSql(
                "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name like ? and meta_attr_value like ? and meta_attr_unit like ?)",
                &icss);
        }
        else {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlDeleteAVUMetadata SQL 8");
            status =  cmlExecuteNoAnswerSql(
                "delete from R_OBJT_METAMAP where object_id=? and meta_id IN (select meta_id from R_META_MAIN where meta_attr_name = ? and meta_attr_value = ? and meta_attr_unit = ?)",
                &icss);
        }
    }
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDeleteAVUMetadata cmlExecuteNoAnswerSql delete failure %d",
                status);
        _rollback("chlDeleteAVUMetadata");
        return(status);
    }

    /* Remove unused AVU rows, if any */
#ifdef METADATA_CLEANUP
    removeAVUs();
#endif

    /* Audit */
    status = cmlAudit3(AU_DELETE_AVU_METADATA,  
                       objIdStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       type,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDeleteAVUMetadata cmlAudit3 failure %d",
                status);
        _rollback("chlDeleteAVUMetadata");
        return(status);
    }

    if (noCommit != 1) {
        status =  cmlExecuteNoAnswerSql("commit", &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlDeleteAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                    status);
            return(status);
        }
    }

    return(status);
}

/*
  Copy an Attribute-Value [Units] pair/triple from one object to another  */
int chlCopyAVUMetadata(rsComm_t *rsComm, char *type1,  char *type2, 
                       char *name1, char *name2) {
    char myTime[50];
    int status;
    rodsLong_t objId1, objId2;
    char objIdStr1[MAX_NAME_LEN];
    char objIdStr2[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCopyAVUMetadata");

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCopyAVUMetadata SQL 1 ");
    objId1 = checkAndGetObjectId(rsComm, type1, name1, ACCESS_READ_METADATA);
    if (objId1 < 0) return(objId1);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlCopyAVUMetadata SQL 2");
    objId2 = checkAndGetObjectId(rsComm, type2, name2, ACCESS_CREATE_METADATA);

    if (objId2 < 0) return(objId2);

    snprintf(objIdStr1, MAX_NAME_LEN, "%lld", objId1);
    snprintf(objIdStr2, MAX_NAME_LEN, "%lld", objId2);

    getNowStr(myTime);
    cllBindVars[cllBindVarCount++]=objIdStr2;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=objIdStr1;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlCopyAVUMetadata SQL 3");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) select ?, meta_id, ?, ? from R_OBJT_METAMAP where object_id=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlCopyAVUMetadata cmlExecuteNoAnswerSql insert failure %d",
                status);
        _rollback("chlCopyAVUMetadata");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_COPY_AVU_METADATA,  
                       objIdStr1,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       objIdStr2,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlCopyAVUMetadata cmlAudit3 failure %d",
                status);
        _rollback("chlCopyAVUMetadata");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlCopyAVUMetadata cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(status);
}

/* create a path name with escaped SQL special characters (% and _) */
void
makeEscapedPath(char *inPath, char *outPath, int size) {
   int i;
   for (i=0;i<size-1;i++) {
      if (*inPath=='%' || *inPath=='_') {
         *outPath++='\\';
      }
      if (*inPath=='\0') {
         *outPath++=*inPath++;
         break;
      }
      *outPath++=*inPath++;
   }
   return;
}

/* Internal routine to modify inheritance */
/* inheritFlag =1 to set, 2 to remove */
int _modInheritance(int inheritFlag, int recursiveFlag, char *collIdStr, char *pathName) {
    rodsLong_t status;
    char myTime[50];
    char newValue[10];
    char pathStart[MAX_NAME_LEN*2];
    char auditStr[30];

    if (recursiveFlag==0) {
        strcpy(auditStr, "inheritance non-recursive ");
    }
    else {
        strcpy(auditStr, "inheritance recursive ");
    }

    if (inheritFlag==1) {
        newValue[0]='1';
        newValue[1]='\0';
    }
    else {
        newValue[0]='0';
        newValue[1]='\0';
    }
    strcat(auditStr, newValue);

    getNowStr(myTime);

    /* non-Recursive mode */
    if (recursiveFlag==0) {

        if (logSQL!=0) rodsLog(LOG_SQL, "_modInheritance SQL 1");

        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=collIdStr;
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_id=?",
            &icss);
    }
    else {
        /* Recursive mode */
        makeEscapedPath(pathName, pathStart, sizeof(pathStart));
        strncat(pathStart, "/%", sizeof(pathStart));

        cllBindVars[cllBindVarCount++]=newValue;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=pathName;
        cllBindVars[cllBindVarCount++]=pathStart;
        if (logSQL!=0) rodsLog(LOG_SQL, "_modInheritance SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set coll_inheritance=?, modify_ts=? where coll_name = ? or coll_name like ?",
            &icss);
    }
    if (status != 0) {
        _rollback("_modInheritance");
        return(status);
    }

    /* Audit */
    status = cmlAudit5(AU_MOD_ACCESS_CONTROL_COLL,
                       collIdStr,
                       "0",
                       auditStr,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "_modInheritance cmlAudit5 failure %d",
                status);
        _rollback("_modInheritance");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

int chlModAccessControlResc(rsComm_t *rsComm, int recursiveFlag,
                            char* accessLevel, char *userName, char *zone, 
                            char* rescName) {
    char myAccessStr[LONG_NAME_LEN];
    char rescIdStr[MAX_NAME_LEN];
    char *myAccessLev=NULL;
    int rmFlag=0;
    rodsLong_t status;
    char *myZone;
    rodsLong_t userId;
    char userIdStr[MAX_NAME_LEN];
    char myTime[50];
    rodsLong_t iVal;
    int debug=0;
   
    strncpy(myAccessStr,accessLevel+strlen(MOD_RESC_PREFIX),LONG_NAME_LEN);
    myAccessStr[ LONG_NAME_LEN-1 ]='\0'; // JMC cppcheck - dangerous use of strncpy
    if (debug>0) {
        printf("accessLevel: %s\n", accessLevel);
        printf("rescName: %s\n", rescName);
    }

    if (strcmp(myAccessStr, AP_NULL)==0) {myAccessLev=ACCESS_NULL; rmFlag=1;}
    else if (strcmp(myAccessStr,AP_READ)==0) {myAccessLev=ACCESS_READ_OBJECT;}
    else if (strcmp(myAccessStr,AP_WRITE)==0){myAccessLev=ACCESS_MODIFY_OBJECT;}
    else if (strcmp(myAccessStr,AP_OWN)==0) {myAccessLev=ACCESS_OWN;}
    else {
        char errMsg[105];
        snprintf(errMsg, 100, "access level '%s' is invalid for a resource",
                 myAccessStr);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_INVALID_ARGUMENT);
    }

    if (rsComm->clientUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH) {
        /* admin, so just get the resc_id */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControlResc SQL 1");
        status = cmlGetIntegerValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=?",
            &iVal, rescName, 0, 0, 0, 0, &icss);
        if (status==CAT_NO_ROWS_FOUND) return(CAT_UNKNOWN_RESOURCE);
        if (status<0) return(status);
        status = iVal;
    }
    else {
        status = cmlCheckResc(rescName,
                              rsComm->clientUser.userName, 
                              rsComm->clientUser.rodsZone,
                              ACCESS_OWN,
                              &icss);
        if (status<0) return(status);
    }
    snprintf(rescIdStr, MAX_NAME_LEN, "%lld", status);

    /* Check that the receiving user exists and if so get the userId */
    status = getLocalZone();
    if (status != 0) return(status);

    myZone=zone;
    if (zone == NULL || strlen(zone)==0) {
        myZone=localZone;
    }

    userId=0;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControlResc SQL 2");
    status = cmlGetIntegerValueFromSql(
        "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=?",
        &userId, userName, myZone, 0, 0, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
        return(status);
    }

    snprintf(userIdStr, MAX_NAME_LEN, "%lld", userId);

    /* remove any access permissions */
    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=rescIdStr;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControlResc SQL 3");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
        &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        return(status);
    }

    /* If not just removing, add the new value */
    if (rmFlag==0) {
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=rescIdStr;
        cllBindVars[cllBindVarCount++]=userIdStr;
        cllBindVars[cllBindVarCount++]=myAccessLev;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControlResc SQL 4");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
            &icss);
        if (status != 0) {
            _rollback("chlModAccessControlResc");
            return(status);
        }
    }

    /* Audit */
    status = cmlAudit5(AU_MOD_ACCESS_CONTROL_RESOURCE,
                       rescIdStr,
                       userIdStr,
                       myAccessLev,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModAccessControlResc cmlAudit5 failure %d",
                status);
        _rollback("chlModAccessControlResc");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

/* 
 * chlModAccessControl - Modify the Access Control information
 *         of an existing dataObj or collection.
 * "n" (null or none) used to remove access.
 */
int chlModAccessControl(rsComm_t *rsComm, int recursiveFlag,
                        char* accessLevel, char *userName, char *zone, 
                        char* pathName) {
    char *myAccessLev=NULL;
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    char collIdStr[MAX_NAME_LEN];
    rodsLong_t objId=0;
    rodsLong_t status, status1, status2, status3;
    int rmFlag=0;
    rodsLong_t userId;
    char myTime[50];
    char *myZone;
    char userIdStr[MAX_NAME_LEN];
    char objIdStr[MAX_NAME_LEN];
    char pathStart[MAX_NAME_LEN*2];
    int inheritFlag=0;
    char myAccessStr[LONG_NAME_LEN];
    int adminMode=0;
    rodsLong_t iVal;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl");

    if (strncmp(accessLevel, MOD_RESC_PREFIX, strlen(MOD_RESC_PREFIX))==0) {
        return(chlModAccessControlResc(rsComm, recursiveFlag,
                                       accessLevel, userName, zone, pathName));
    }

    adminMode=0;
    if (strncmp(accessLevel, MOD_ADMIN_MODE_PREFIX, 
                strlen(MOD_ADMIN_MODE_PREFIX))==0) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            addRErrorMsg (&rsComm->rError, 0, 
                              "You must be the admin to use the -M admin mode");
            return(CAT_NO_ACCESS_PERMISSION);
        }
        strncpy(myAccessStr,accessLevel+strlen(MOD_ADMIN_MODE_PREFIX),
                LONG_NAME_LEN);
        accessLevel = myAccessStr;
        adminMode=1;
    }

    if (strcmp(accessLevel, AP_NULL)==0) {myAccessLev=ACCESS_NULL; rmFlag=1;}
    else if (strcmp(accessLevel,AP_READ)==0) {myAccessLev=ACCESS_READ_OBJECT;}
    else if (strcmp(accessLevel,AP_WRITE)==0){myAccessLev=ACCESS_MODIFY_OBJECT;}
    else if (strcmp(accessLevel,AP_OWN)==0) {myAccessLev=ACCESS_OWN;}
    else if (strcmp(accessLevel,ACCESS_INHERIT)==0) {
        inheritFlag=1;
    }
    else if (strcmp(accessLevel,ACCESS_NO_INHERIT)==0) {
        inheritFlag=2;
    }
    else {
        char errMsg[105];
        snprintf(errMsg, 100, "access level '%s' is invalid",
                 accessLevel);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_INVALID_ARGUMENT);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    if (adminMode) {
        /* See if the input path is a collection 
           and, if so, get the collectionID */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 14");
        status1 = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name=?",
            &iVal, pathName, 0, 0, 0, 0, &icss);
        if (status1==CAT_NO_ROWS_FOUND) {
            status1=CAT_UNKNOWN_COLLECTION;
        }
        if (status1==0) status1=iVal;
    }
    else {
        /* See if the input path is a collection and the user owns it,
           and, if so, get the collectionID */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 1 ");
        status1 = cmlCheckDir(pathName,
                              rsComm->clientUser.userName, 
                              rsComm->clientUser.rodsZone,
                              ACCESS_OWN, 
                              &icss);
    }
    if (status1 >= 0) {
        snprintf(collIdStr, MAX_NAME_LEN, "%lld", status1);
    }

    if (status1 < 0 && inheritFlag!=0) {
        char errMsg[105];
        snprintf(errMsg, 100, "access level '%s' is valid only for collections",
                 accessLevel);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_INVALID_ARGUMENT);
    }

    /* Not a collection (with access for non-Admin) */
    if (status1 < 0) {
        status2 = splitPathByKey(pathName,
                                 logicalParentDirName, logicalEndName, '/');
        if (strlen(logicalParentDirName)==0) {
            strcpy(logicalParentDirName, "/");
            strcpy(logicalEndName, pathName+1);
        }
        if (adminMode) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 15");
            status2 = cmlGetIntegerValueFromSql(
                "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
                &iVal, logicalEndName, logicalParentDirName, 0, 0, 0, &icss);
            if (status2==CAT_NO_ROWS_FOUND) status2=CAT_UNKNOWN_FILE;
            if (status2==0) status2=iVal;
        }
        else {
            /* Not a collection with access, so see if the input path dataObj 
               exists and the user owns it, and, if so, get the objectID */
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 2");
            status2 = cmlCheckDataObjOnly(logicalParentDirName, logicalEndName,
                                          rsComm->clientUser.userName, 
                                          rsComm->clientUser.rodsZone, 
                                          ACCESS_OWN, &icss);
        }
        if (status2 > 0) objId=status2;
    }

    /* If both failed, it doesn't exist or there's no permission */
    if (status1 < 0 && status2 < 0) {
        char errMsg[205];

        if (status1 == CAT_UNKNOWN_COLLECTION && status2 == CAT_UNKNOWN_FILE) {
            snprintf(errMsg, 200, 
                     "Input path is not a collection and not a dataObj: %s",
                     pathName);
            addRErrorMsg (&rsComm->rError, 0, errMsg);
            return(CAT_INVALID_ARGUMENT);
        }
        if (status1 != CAT_UNKNOWN_COLLECTION) {
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 12");
            status3 = cmlCheckDirOwn(pathName,
                                     rsComm->clientUser.userName, 
                                     rsComm->clientUser.rodsZone, 
                                     &icss);
            if (status3 < 0) return(status1);
            snprintf(collIdStr, MAX_NAME_LEN, "%lld", status3);
        }
        else {
            if (status2 == CAT_NO_ACCESS_PERMISSION) {
                /* See if this user is the owner (with no access, but still
                   allowed to ichmod) */
                if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 13");
                status3 = cmlCheckDataObjOwn(logicalParentDirName, logicalEndName,
                                             rsComm->clientUser.userName,
                                             rsComm->clientUser.rodsZone,
                                             &icss);
                if (status3 < 0) {
                    _rollback("chlModAccessControl");
                    return(status2);
                }
                objId = status3;
            } else {
                return(status2);
            }
        }
    }

    /* Doing inheritance */
    if (inheritFlag!=0) {
        status = _modInheritance(inheritFlag, recursiveFlag, collIdStr, pathName);
        return(status);
    }

    /* Check that the receiving user exists and if so get the userId */
    status = getLocalZone();
    if (status != 0) return(status);

    myZone=zone;
    if (zone == NULL || strlen(zone)==0) {
        myZone=localZone;
    }

    userId=0;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 3");
    status = cmlGetIntegerValueFromSql(
        "select user_id from R_USER_MAIN where user_name=? and R_USER_MAIN.zone_name=?",
        &userId, userName, myZone, 0, 0, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
        return(status);
    }

    snprintf(userIdStr, MAX_NAME_LEN, "%lld", userId);
    snprintf(objIdStr, MAX_NAME_LEN, "%lld", objId);

    rodsLog(LOG_NOTICE, "recursiveFlag %d",recursiveFlag);

    /* non-Recursive mode */
    if (recursiveFlag==0) {

        /* doing a dataObj */
        if (objId) { 
            cllBindVars[cllBindVarCount++]=userIdStr;
            cllBindVars[cllBindVarCount++]=objIdStr;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 4");
            status =  cmlExecuteNoAnswerSql(
                "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
                &icss);
            if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
                return(status);
            }
            if (rmFlag==0) {  /* if not just removing: */
                getNowStr(myTime);
                cllBindVars[cllBindVarCount++]=objIdStr;
                cllBindVars[cllBindVarCount++]=userIdStr;
                cllBindVars[cllBindVarCount++]=myAccessLev;
                cllBindVars[cllBindVarCount++]=myTime;
                cllBindVars[cllBindVarCount++]=myTime;
                if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 5");
                status =  cmlExecuteNoAnswerSql(
                    "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
                    &icss);
                if (status != 0) {
                    _rollback("chlModAccessControl");
                    return(status);
                }
            }

            /* Audit */
            status = cmlAudit5(AU_MOD_ACCESS_CONTROL_OBJ,
                               objIdStr,
                               userIdStr,
                               myAccessLev,
                               &icss);
            if (status != 0) {
                rodsLog(LOG_NOTICE,
                        "chlModAccessControl cmlAudit5 failure %d",
                        status);
                _rollback("chlModAccessControl");
                return(status);
            }

            status =  cmlExecuteNoAnswerSql("commit", &icss);
            return(status);
        }

        /* doing a collection, non-recursive */
        cllBindVars[cllBindVarCount++]=userIdStr;
        cllBindVars[cllBindVarCount++]=collIdStr;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 6");
        status =  cmlExecuteNoAnswerSql(
            "delete from R_OBJT_ACCESS where user_id=? and object_id=?",
            &icss);
        if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
            _rollback("chlModAccessControl");
            return(status);
        }
        if (rmFlag) {  /* just removing */
            /* Audit */
            status = cmlAudit5(AU_MOD_ACCESS_CONTROL_COLL,
                               collIdStr,
                               userIdStr,
                               myAccessLev,
                               &icss);
            if (status != 0) {
                rodsLog(LOG_NOTICE,
                        "chlModAccessControl cmlAudit5 failure %d",
                        status);
                _rollback("chlModAccessControl");
                return(status);
            }
            status =  cmlExecuteNoAnswerSql("commit", &icss);
            return(status);
        }

        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=collIdStr;
        cllBindVars[cllBindVarCount++]=userIdStr;
        cllBindVars[cllBindVarCount++]=myAccessLev;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 7");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  values (?, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ?)",
            &icss);

        if (status != 0) {
            _rollback("chlModAccessControl");
            return(status);
        }
        /* Audit */
        status = cmlAudit5(AU_MOD_ACCESS_CONTROL_COLL,
                           collIdStr,
                           userIdStr,
                           myAccessLev,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModAccessControl cmlAudit5 failure %d",
                    status);
            _rollback("chlModAccessControl");
            return(status);
        }

        status =  cmlExecuteNoAnswerSql("commit", &icss);
        return(status);
    }


    /* Recursive */
    if (objId) {
        char errMsg[205];

        snprintf(errMsg, 200, 
                 "Input path is not a collection and recursion was requested: %s",
                 pathName);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return(CAT_INVALID_ARGUMENT);
    }


    makeEscapedPath(pathName, pathStart, sizeof(pathStart));
    strncat(pathStart, "/%", sizeof(pathStart));

    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=pathName;
    cllBindVars[cllBindVarCount++]=pathStart;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 8");
    status =  cmlExecuteNoAnswerSql(
              "delete from R_OBJT_ACCESS where user_id=? and object_id in (select data_id from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
              &icss);
     
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        _rollback("chlModAccessControl");
        return(status);
    }

    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=pathName;
    cllBindVars[cllBindVarCount++]=pathStart;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 9");
    status =  cmlExecuteNoAnswerSql(
               "delete from R_OBJT_ACCESS where user_id=? and object_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
              &icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        _rollback("chlModAccessControl");
        return(status);
    }
    if (rmFlag) {  /* just removing */

        /* Audit */
        status = cmlAudit5(AU_MOD_ACCESS_CONTROL_COLL_RECURSIVE,
                           collIdStr,
                           userIdStr,
                           myAccessLev,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlModAccessControl cmlAudit5 failure %d",
                    status);
            _rollback("chlModAccessControl");
            return(status);
        }

        status =  cmlExecuteNoAnswerSql("commit", &icss);
        return(status);
    }

    getNowStr(myTime);
    makeEscapedPath(pathName, pathStart, sizeof(pathStart));
    strncat(pathStart, "/%", sizeof(pathStart));
    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=myAccessLev;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=pathName;
    cllBindVars[cllBindVarCount++]=pathStart;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 10");
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
        &icss);
#elif MY_ICAT
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
        &icss);
#else
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct data_id, cast(? as bigint), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_DATA_MAIN where coll_id in (select coll_id from R_COLL_MAIN where coll_name = ? or coll_name like ?))",
        &icss);
#endif
    if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) status=0; /* no files, OK */
    if (status != 0) {
        _rollback("chlModAccessControl");
        return(status);
    }


    /* Now set the collections */
    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=myAccessLev;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=pathName;
    cllBindVars[cllBindVarCount++]=pathStart;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlModAccessControl SQL 11");
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, cast(? as integer), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
        &icss);
#elif MY_ICAT
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, ?, (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
        &icss);
#else
    status =  cmlExecuteNoAnswerSql(
              "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts)  (select distinct coll_id, cast(? as bigint), (select token_id from R_TOKN_MAIN where token_namespace = 'access_type' and token_name = ?), ?, ? from R_COLL_MAIN where coll_name = ? or coll_name like ?)",
        &icss);
#endif
    if (status != 0) {
        _rollback("chlModAccessControl");
        return(status);
    }

    /* Audit */
    status = cmlAudit5(AU_MOD_ACCESS_CONTROL_COLL_RECURSIVE,
                       collIdStr,
                       userIdStr,
                       myAccessLev,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlModAccessControl cmlAudit5 failure %d",
                status);
        _rollback("chlModAccessControl"); 
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

/* 
 * chlRenameObject - Rename a dataObject or collection.
 */
int chlRenameObject(rsComm_t *rsComm, rodsLong_t objId,
                    char* newName) {
    int status;
    rodsLong_t collId;
    rodsLong_t otherDataId;
    rodsLong_t otherCollId;
    char myTime[50];

    char parentCollName[MAX_NAME_LEN]="";
    char collName[MAX_NAME_LEN]="";
    char *cVal[3];
    int iVal[3];
    int pLen, cLen, len;
    int isRootDir=0;
    char objIdString[MAX_NAME_LEN];
    char collIdString[MAX_NAME_LEN];
    char collNameTmp[MAX_NAME_LEN];

    char pLenStr[MAX_NAME_LEN];
    char cLenStr[MAX_NAME_LEN];
    char collNameSlash[MAX_NAME_LEN];
    char collNameSlashLen[20];
    char slashNewName[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject");

    if (strstr(newName, "/")) {
        return(CAT_INVALID_ARGUMENT);
    }

/* See if it's a dataObj and if so get the coll_id
   check the access permission at the same time */
    collId=0;

    snprintf(objIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 1 ");

    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
        &collId, objIdString, rsComm->clientUser.userName, rsComm->clientUser.rodsZone,
        0, 0,  &icss);

    if (status == 0) {  /* it is a dataObj and user has access to it */

        /* check that no other dataObj exists with this name in this collection*/
        snprintf(collIdString, MAX_NAME_LEN, "%lld", collId);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 2");
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
            &otherDataId, 
            newName, collIdString, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_DATAOBJ);
        }

        /* check that no subcoll exists in this collection,
           with the newName */
        snprintf(collNameTmp, MAX_NAME_LEN, "/%s", newName);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 3");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name = ( select coll_name from R_COLL_MAIN where coll_id=? ) || ?",
            &otherCollId, collIdString, collNameTmp, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_COLLECTION);
        }

        /* update the tables */
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=newName;
        cllBindVars[cllBindVarCount++]=objIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 4");
        status =  cmlExecuteNoAnswerSql(
            "update R_DATA_MAIN set data_name = ? where data_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlExecuteNoAnswerSql update1 failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=collIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 5");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set modify_ts=? where coll_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlExecuteNoAnswerSql update2 failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        /* Audit */
        status = cmlAudit3(AU_RENAME_DATA_OBJ,  
                           objIdString,
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           newName,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlAudit3 failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        return(status);
    }

    /* See if it's a collection, and get the parentCollName and
       collName, and check permission at the same time */

    cVal[0]=parentCollName;
    iVal[0]=MAX_NAME_LEN;
    cVal[1]=collName;
    iVal[1]=MAX_NAME_LEN;

    snprintf(objIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 6");

    status = cmlGetStringValuesFromSql(
        "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
        cVal, iVal, 2, objIdString, 
        rsComm->clientUser.userName, rsComm->clientUser.rodsZone, &icss);
    if (status == 0) { 
        /* it is a collection and user has access to it */

        /* check that no other dataObj exists with this name in this collection*/
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 7");
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN where data_name=? and coll_id= (select coll_id from R_COLL_MAIN  where coll_name = ?)",
            &otherDataId, newName, parentCollName, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_DATAOBJ);
        }

        /* check that no subcoll exists in the parent collection,
           with the newName */
        snprintf(collNameTmp, MAX_NAME_LEN, "%s/%s", parentCollName, newName);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 8");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name = ?",
            &otherCollId, collNameTmp, 0, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_COLLECTION);
        }

        /* update the table */
        pLen = strlen(parentCollName);
        cLen=strlen(collName);
        if (pLen<=0 || cLen <=0) return(CAT_INVALID_ARGUMENT);  /* invalid 
                                                                   argument is not really right, but something is really wrong */

        if (pLen==1) {
            if (strncmp(parentCollName, "/", 20) == 0) { /* just to be sure */
                isRootDir=1;  /* need to treat a little special below */
            }
        }

        /* set any collection names that are under this collection to
           the new name, putting the string together from the the old upper
           part, newName string, and then (if any for each row) the
           tailing part of the name. 
           (In the sql substr function, the index for sql is 1 origin.) */
        snprintf(pLenStr,MAX_NAME_LEN, "%d", pLen); /* formerly +1 but without is
                                                       correct, makes a difference in Oracle, and works
                                                       in postgres too. */
        snprintf(cLenStr,MAX_NAME_LEN, "%d", cLen+1);
        snprintf(collNameSlash, MAX_NAME_LEN, "%s/", collName);
        len = strlen(collNameSlash);
        snprintf(collNameSlashLen, 10, "%d", len);
        snprintf(slashNewName, MAX_NAME_LEN, "/%s", newName);
        if (isRootDir) {
            snprintf(slashNewName, MAX_NAME_LEN, "%s", newName);
        }
        cllBindVars[cllBindVarCount++]=pLenStr;
        cllBindVars[cllBindVarCount++]=slashNewName;
        cllBindVars[cllBindVarCount++]=cLenStr;
        cllBindVars[cllBindVarCount++]=collNameSlashLen;
        cllBindVars[cllBindVarCount++]=collNameSlash;
        cllBindVars[cllBindVarCount++]=collName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 9");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set coll_name = substr(coll_name,1,?) || ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
            &icss);
        if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        /* like above, but for the parent_coll_name's */
        cllBindVars[cllBindVarCount++]=pLenStr;
        cllBindVars[cllBindVarCount++]=slashNewName;
        cllBindVars[cllBindVarCount++]=cLenStr;
        cllBindVars[cllBindVarCount++]=collNameSlashLen;
        cllBindVars[cllBindVarCount++]=collNameSlash;
        cllBindVars[cllBindVarCount++]=collName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 10");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set parent_coll_name = substr(parent_coll_name,1,?) || ? || substr(parent_coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name  = ?",
            &icss);
        if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        /* And now, update the row for this collection */
        getNowStr(myTime);
        snprintf(collNameTmp, MAX_NAME_LEN, "%s/%s", parentCollName, newName);
        if (isRootDir) {
            snprintf(collNameTmp, MAX_NAME_LEN, "%s%s", parentCollName, newName);
        }
        cllBindVars[cllBindVarCount++]=collNameTmp;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=objIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 11");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set coll_name=?, modify_ts=? where coll_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        /* Audit */
        status = cmlAudit3(AU_RENAME_COLLECTION,  
                           objIdString,
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           newName,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlRenameObject cmlAudit3 failure %d",
                    status);
            _rollback("chlRenameObject");
            return(status);
        }

        return(status);

    }


    /* Both collection and dataObj failed, go thru the sql in smaller
       steps to return a specific error */

    snprintf(objIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 12");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_DATA_MAIN where data_id=?",
        &otherDataId, objIdString, 0, 0, 0, 0, &icss);
    if (status == 0) {
        /* it IS a data obj, must be permission error */
        return (CAT_NO_ACCESS_PERMISSION);
    }

    snprintf(collIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRenameObject SQL 12");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where coll_id=?",
        &otherDataId, collIdString, 0, 0, 0, 0, &icss);
    if (status == 0) {
        /* it IS a collection, must be permission error */
        return (CAT_NO_ACCESS_PERMISSION);
    }

    return(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION);
}

/* 
 * chlMoveObject - Move a dataObject or collection to another
 * collection.
 */
int chlMoveObject(rsComm_t *rsComm, rodsLong_t objId,
                  rodsLong_t targetCollId) {
    int status;
    rodsLong_t collId;
    rodsLong_t otherDataId;
    rodsLong_t otherCollId;
    char myTime[50];

    char dataObjName[MAX_NAME_LEN]="";
    char *cVal[3];
    int iVal[3];

    char parentCollName[MAX_NAME_LEN]="";
    char oldCollName[MAX_NAME_LEN]="";
    char endCollName[MAX_NAME_LEN]="";  /* for example: d1 portion of
                                           /tempZone/home/d1  */

    char targetCollName[MAX_NAME_LEN]="";
    char parentTargetCollName[MAX_NAME_LEN]="";
    char newCollName[MAX_NAME_LEN]="";
    int pLen, ocLen;
    int i, OK, len;
    char *cp;
    char objIdString[MAX_NAME_LEN];
    char collIdString[MAX_NAME_LEN];
    char nameTmp[MAX_NAME_LEN];
    char ocLenStr[MAX_NAME_LEN];
    char collNameSlash[MAX_NAME_LEN];
    char collNameSlashLen[20];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject");

    /* check that the target collection exists and user has write
       permission, and get the names while at it */
    cVal[0]=parentTargetCollName;
    iVal[0]=MAX_NAME_LEN;
    cVal[1]=targetCollName;
    iVal[1]=MAX_NAME_LEN;
    snprintf(objIdString, MAX_NAME_LEN, "%lld", targetCollId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 1 ");
    status = cmlGetStringValuesFromSql(
        "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
        cVal, iVal, 2, objIdString, 
        rsComm->clientUser.userName, 
        rsComm->clientUser.rodsZone, &icss);

    snprintf(collIdString, MAX_NAME_LEN, "%lld", targetCollId);
    if (status != 0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 2");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_id=?",
            &collId, collIdString, 0, 0, 0, 0, &icss);
        if (status==0) {
            return (CAT_NO_ACCESS_PERMISSION);  /* does exist, must be
                                                   permission error */
        }
        return(CAT_UNKNOWN_COLLECTION);        /* isn't a coll */
    }


/* See if we're moving a dataObj and if so get the data_name;
   and at the same time check the access permission */
    snprintf(objIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 3");
    status = cmlGetStringValueFromSql(
        "select data_name from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where DM.data_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
        dataObjName, MAX_NAME_LEN, objIdString, 
        rsComm->clientUser.userName,
        rsComm->clientUser.rodsZone, &icss);
    snprintf(collIdString, MAX_NAME_LEN, "%lld", targetCollId);
    if (status == 0) {  /* it is a dataObj and user has access to it */

        /* check that no other dataObj exists with the ObjName in the
           target collection */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 4");
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
            &otherDataId, dataObjName, collIdString, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_DATAOBJ);
        }

        /* check that no subcoll exists in the target collection, with
           the name of the object */
/* //not needed, I think   snprintf(collIdString, MAX_NAME_LEN, "%d", collId); */
        snprintf(nameTmp, MAX_NAME_LEN, "/%s", dataObjName);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 5");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name = ( select coll_name from R_COLL_MAIN where coll_id=? ) || ?",
            &otherCollId, collIdString, nameTmp, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_COLLECTION);
        }

        /* update the table */
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=collIdString;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=objIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 6");
        status =  cmlExecuteNoAnswerSql(
            "update R_DATA_MAIN set coll_id=?, modify_ts=? where data_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlExecuteNoAnswerSql update1 failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }


        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=collIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 7");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set modify_ts=? where coll_id=?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlExecuteNoAnswerSql update2 failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }

        /* Audit */
        status = cmlAudit3(AU_MOVE_DATA_OBJ,  
                           objIdString,
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           collIdString,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlAudit3 failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }

        return(status);
    }

    /* See if it's a collection, and get the parentCollName and
       oldCollName, and check permission at the same time */
    cVal[0]=parentCollName;
    iVal[0]=MAX_NAME_LEN;
    cVal[1]=oldCollName;
    iVal[1]=MAX_NAME_LEN;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 8");
    status = cmlGetStringValuesFromSql(
        "select parent_coll_name, coll_name from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and TM.token_namespace ='access_type' and TM.token_name = 'own'",
        cVal, iVal, 2, objIdString, rsComm->clientUser.userName, 
        rsComm->clientUser.rodsZone, &icss);
    if (status == 0) { 
        /* it is a collection and user has access to it */

        pLen = strlen(parentCollName);

        ocLen=strlen(oldCollName);
        if (pLen<=0 || ocLen <=0) return(CAT_INVALID_ARGUMENT);  /* invalid
                                                                    argument is not really the right error code, but something
                                                                    is really wrong */
        OK=0;
        for (i=ocLen;i>0;i--) {
            if (oldCollName[i]=='/') {
                OK=1;
                strncpy(endCollName, (char*)&oldCollName[i+1], MAX_NAME_LEN);
                break;
            }
        }
        if (OK==0) return (CAT_INVALID_ARGUMENT); /* not really, but...*/

        /* check that no other dataObj exists with the ObjName in the
           target collection */
        snprintf(collIdString, MAX_NAME_LEN, "%lld", targetCollId);
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 9");
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN where data_name=? and coll_id=?",
            &otherDataId, endCollName, collIdString, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_DATAOBJ);
        }

        /* check that no subcoll exists in the target collection, with
           the name of the object */
        strncpy(newCollName, targetCollName, MAX_NAME_LEN);
        strncat(newCollName, "/", MAX_NAME_LEN);
        strncat(newCollName, endCollName, MAX_NAME_LEN);

        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 10");
        status = cmlGetIntegerValueFromSql(
            "select coll_id from R_COLL_MAIN where coll_name = ?",
            &otherCollId, newCollName, 0, 0, 0, 0, &icss);
        if (status!=CAT_NO_ROWS_FOUND) {
            return( CAT_NAME_EXISTS_AS_COLLECTION);
        }


        /* Check that we're not moving the coll down into it's own
           subtree (which would create a recursive loop) */
        cp = strstr(targetCollName, oldCollName);
        if (cp == targetCollName && 
            (targetCollName[strlen(oldCollName)] == '/' || 
             targetCollName[strlen(oldCollName)] == '\0')) {
            return(CAT_RECURSIVE_MOVE);
        }


        /* Update the table */

        /* First, set the collection name and parent collection to the
        new strings, and update the modify-time */
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=newCollName;
        cllBindVars[cllBindVarCount++]=targetCollName;
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=objIdString;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 11");
        status =  cmlExecuteNoAnswerSql(
                  "update R_COLL_MAIN set coll_name = ?, parent_coll_name=?, modify_ts=? where coll_id = ?",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }

        /* Now set any collection names that are under this collection to
           the new name, putting the string together from the the new upper
           part, endCollName string, and then (if any for each row) the
           tailing part of the name. 
           (In the sql substr function, the index for sql is 1 origin.) */
        snprintf(ocLenStr, MAX_NAME_LEN, "%d", ocLen+1);
        snprintf(collNameSlash, MAX_NAME_LEN, "%s/", oldCollName);
        len = strlen(collNameSlash);
        snprintf(collNameSlashLen, 10, "%d", len);
        cllBindVars[cllBindVarCount++]=newCollName;
        cllBindVars[cllBindVarCount++]=ocLenStr;
        cllBindVars[cllBindVarCount++]=newCollName;
        cllBindVars[cllBindVarCount++]=ocLenStr;
        cllBindVars[cllBindVarCount++]=collNameSlashLen;
        cllBindVars[cllBindVarCount++]=collNameSlash;
        cllBindVars[cllBindVarCount++]=oldCollName;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 12");
        status =  cmlExecuteNoAnswerSql(
            "update R_COLL_MAIN set parent_coll_name = ? || substr(parent_coll_name, ?), coll_name = ? || substr(coll_name, ?) where substr(parent_coll_name,1,?) = ? or parent_coll_name = ?",
            &icss);
        if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) status = 0;
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlExecuteNoAnswerSql update failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }

        /* Audit */
        status = cmlAudit3(AU_MOVE_COLL,  
                           objIdString,
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           targetCollName,
                           &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlMoveObject cmlAudit3 failure %d",
                    status);
            _rollback("chlMoveObject");
            return(status);
        }

        return(status);
    }


    /* Both collection and dataObj failed, go thru the sql in smaller
       steps to return a specific error */
    snprintf(objIdString, MAX_NAME_LEN, "%lld", objId);
    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 13");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_DATA_MAIN where data_id=?",
        &otherDataId, objIdString, 0, 0, 0, 0, &icss);
    if (status == 0) {
        /* it IS a data obj, must be permission error */
        return (CAT_NO_ACCESS_PERMISSION);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlMoveObject SQL 14");
    status = cmlGetIntegerValueFromSql(
        "select coll_id from R_COLL_MAIN where coll_id=?",
        &otherDataId, objIdString, 0, 0, 0, 0, &icss);
    if (status == 0) {
        /* it IS a collection, must be permission error */
        return (CAT_NO_ACCESS_PERMISSION);
    }

    return(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION);
}

/* 
 * chlRegToken - Register a new token
 */
int chlRegToken(rsComm_t *rsComm, char *nameSpace, char *name, char *value,
                char *value2, char *value3, char *comment)
{
    int status;
    rodsLong_t objId;
    char *myValue1, *myValue2, *myValue3, *myComment;
    char myTime[50];
    rodsLong_t seqNum;
    char errMsg[205];
    char seqNumStr[MAX_NAME_LEN];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegToken");

    if (nameSpace==NULL || strlen(nameSpace)==0) return (CAT_INVALID_ARGUMENT);
    if (name==NULL || strlen(name)==0) return (CAT_INVALID_ARGUMENT);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegToken SQL 1 ");
    status = cmlGetIntegerValueFromSql(
        "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
        &objId, "token_namespace", nameSpace, 0, 0, 0, &icss);
    if (status != 0) {
        snprintf(errMsg, 200, 
                 "Token namespace '%s' does not exist",
                 nameSpace);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return (CAT_INVALID_ARGUMENT);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegToken SQL 2");
    status = cmlGetIntegerValueFromSql(
        "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
        &objId, nameSpace, name, 0, 0, 0, &icss);
    if (status == 0) {
        snprintf(errMsg, 200, 
                 "Token '%s' already exists in namespace '%s'",
                 name, nameSpace);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return (CAT_INVALID_ARGUMENT);
    }

    myValue1=value;
    if (myValue1==NULL) myValue1="";
    myValue2=value2;
    if (myValue2==NULL) myValue2="";
    myValue3=value3;
    if (myValue3==NULL) myValue3="";
    myComment=comment;
    if (myComment==NULL) myComment="";

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegToken SQL 3");
    seqNum = cmlGetNextSeqVal(&icss);
    if (seqNum < 0) {
        rodsLog(LOG_NOTICE, "chlRegToken cmlGetNextSeqVal failure %d",
                seqNum);
        return(seqNum);
    }

    getNowStr(myTime);
    snprintf(seqNumStr, sizeof seqNumStr, "%lld", seqNum);
    cllBindVars[cllBindVarCount++]=nameSpace;
    cllBindVars[cllBindVarCount++]=seqNumStr;
    cllBindVars[cllBindVarCount++]=name;
    cllBindVars[cllBindVarCount++]=myValue1;
    cllBindVars[cllBindVarCount++]=myValue2;
    cllBindVars[cllBindVarCount++]=myValue3;
    cllBindVars[cllBindVarCount++]=myComment;
    cllBindVars[cllBindVarCount++]=myTime;
    cllBindVars[cllBindVarCount++]=myTime;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegToken SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_TOKN_MAIN values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        _rollback("chlRegToken");
        return(status);
    }

    /* Audit */
    status = cmlAudit3(AU_REG_TOKEN,  
                       seqNumStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       name,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegToken cmlAudit3 failure %d",
                status);
        _rollback("chlRegToken");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}


/* 
 * chlDelToken - Delete a token
 */
int chlDelToken(rsComm_t *rsComm, char *nameSpace, char *name)
{
    int status;
    rodsLong_t objId;
    char errMsg[205];
    char objIdStr[60];

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelToken");

    if (nameSpace==NULL || strlen(nameSpace)==0) return (CAT_INVALID_ARGUMENT);
    if (name==NULL || strlen(name)==0) return (CAT_INVALID_ARGUMENT);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelToken SQL 1 ");
    status = cmlGetIntegerValueFromSql(
        "select token_id from R_TOKN_MAIN where token_namespace=? and token_name=?",
        &objId, nameSpace, name, 0, 0, 0, &icss);
    if (status != 0) {
        snprintf(errMsg, 200, 
                 "Token '%s' does not exist in namespace '%s'",
                 name, nameSpace);
        addRErrorMsg (&rsComm->rError, 0, errMsg);
        return (CAT_INVALID_ARGUMENT);
    }

    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelToken SQL 2");
    cllBindVars[cllBindVarCount++]=nameSpace;
    cllBindVars[cllBindVarCount++]=name;
    status =  cmlExecuteNoAnswerSql(
        "delete from R_TOKN_MAIN where token_namespace=? and token_name=?",
        &icss);
    if (status != 0) {
        _rollback("chlDelToken");
        return(status);
    }

    /* Audit */
    snprintf(objIdStr, sizeof objIdStr, "%lld", objId);
    status = cmlAudit3(AU_DEL_TOKEN,  
                       objIdStr,
                       rsComm->clientUser.userName,
                       rsComm->clientUser.rodsZone,
                       name,
                       &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelToken cmlAudit3 failure %d",
                status);
        _rollback("chlDelToken");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}


/* 
 * chlRegServerLoad - Register a new iRODS server load row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlRegServerLoad(rsComm_t *rsComm, 
                     char *hostName, char *rescName,
                     char *cpuUsed, char *memUsed, char *swapUsed, 
                     char *runqLoad, char *diskSpace, char *netInput, 
                     char *netOutput) {
    char myTime[50];
    int status;
    int i;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegServerLoad");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    getNowStr(myTime);

    i=0;
    cllBindVars[i++]=hostName;
    cllBindVars[i++]=rescName;
    cllBindVars[i++]=cpuUsed;
    cllBindVars[i++]=memUsed;
    cllBindVars[i++]=swapUsed;
    cllBindVars[i++]=runqLoad;
    cllBindVars[i++]=diskSpace;
    cllBindVars[i++]=netInput;
    cllBindVars[i++]=netOutput;
    cllBindVars[i++]=myTime;
    cllBindVarCount=i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegServerLoad SQL 1");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_SERVER_LOAD (host_name, resc_name, cpu_used, mem_used, swap_used, runq_load, disk_space, net_input, net_output, create_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", 
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegServerLoad cmlExecuteNoAnswerSql failure %d",status);
        _rollback("chlRegServerLoad");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegServerLoad cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}

/* 
 * chlPurgeServerLoad - Purge some rows from iRODS server load table
 * that are older than secondsAgo seconds ago.  
 * Input - rsComm_t *rsComm - the server handle, 
 *    char *secondsAgo (age in seconds).
 */
int chlPurgeServerLoad(rsComm_t *rsComm, char *secondsAgo) {

    /* delete from R_LOAD_SERVER where (%i -exe_time) > %i */
    int status;
    char nowStr[50];
    static char thenStr[50];
    time_t nowTime;
    time_t thenTime;
    time_t secondsAgoTime;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlPurgeServerLoad");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    getNowStr(nowStr);
    nowTime=atoll(nowStr);
    secondsAgoTime=atoll(secondsAgo);
    thenTime = nowTime - secondsAgoTime;
    snprintf(thenStr, sizeof thenStr, "%011d", (uint) thenTime);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlPurgeServerLoad SQL 1");

    cllBindVars[cllBindVarCount++]=thenStr;
    status =  cmlExecuteNoAnswerSql(
        "delete from R_SERVER_LOAD where create_ts <?",
        &icss);
    if (status != 0) {
        _rollback("chlPurgeServerLoad");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

/* 
 * chlRegServerLoadDigest - Register a new iRODS server load-digest row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlRegServerLoadDigest(rsComm_t *rsComm, 
                           char *rescName, char *loadFactor) {
    char myTime[50];
    int status;
    int i;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegServerLoadDigest");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    getNowStr(myTime);

    i=0;
    cllBindVars[i++]=rescName;
    cllBindVars[i++]=loadFactor;
    cllBindVars[i++]=myTime;
    cllBindVarCount=i;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlRegServerLoadDigest SQL 1");
    status =  cmlExecuteNoAnswerSql(
        "insert into R_SERVER_LOAD_DIGEST (resc_name, load_factor, create_ts) values (?, ?, ?)", 
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegServerLoadDigest cmlExecuteNoAnswerSql failure %d", status);
        _rollback("chlRegServerLoadDigest");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlRegServerLoadDigest cmlExecuteNoAnswerSql commit failure %d",
                status);
        return(status);
    }

    return(0);
}

/* 
 * chlPurgeServerLoadDigest - Purge some rows from iRODS server LoadDigest table
 * that are older than secondsAgo seconds ago.  
 * Input - rsComm_t *rsComm - the server handle, 
 *    int secondsAgo (age in seconds).
 */
int chlPurgeServerLoadDigest(rsComm_t *rsComm, char *secondsAgo) {
    /* delete from R_SERVER_LOAD_DIGEST where (%i -exe_time) > %i */
    int status;
    char nowStr[50];
    static char thenStr[50];
    time_t nowTime;
    time_t thenTime;
    time_t secondsAgoTime;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlPurgeServerLoadDigest");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    getNowStr(nowStr);
    nowTime=atoll(nowStr);
    secondsAgoTime=atoll(secondsAgo);
    thenTime = nowTime - secondsAgoTime;
    snprintf(thenStr, sizeof thenStr, "%011d", (uint) thenTime);

    if (logSQL!=0) rodsLog(LOG_SQL, "chlPurgeServerLoadDigest SQL 1");

    cllBindVars[cllBindVarCount++]=thenStr;
    status =  cmlExecuteNoAnswerSql(
        "delete from R_SERVER_LOAD_DIGEST where create_ts <?",
        &icss);
    if (status != 0) {
        _rollback("chlPurgeServerLoadDigest");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

/*
  Set the over_quota values (if any) using the limits and 
  and the current usage; handling the various types: per-user per-resource,
  per-user total-usage, group per-resource, and group-total.

  The over_quota column is positive if over_quota and the negative value
  indicates how much space is left before reaching the quota.  
*/
int setOverQuota(rsComm_t *rsComm) {
    int status;
    int rowsFound;
    int statementNum;
    char myTime[50];

    /* For each defined group limit (if any), get a total usage on that
     * resource for all users in that group: */
    char mySQL1[]="select sum(quota_usage), UM1.user_id, R_QUOTA_USAGE.resc_id from R_QUOTA_USAGE, R_QUOTA_MAIN, R_USER_MAIN UM1, R_USER_GROUP, R_USER_MAIN UM2 where R_QUOTA_MAIN.user_id = UM1.user_id and UM1.user_type_name = 'rodsgroup' and R_USER_GROUP.group_user_id = UM1.user_id and UM2.user_id = R_USER_GROUP.user_id and R_QUOTA_USAGE.user_id = UM2.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id group by UM1.user_id, R_QUOTA_USAGE.resc_id";

    /* For each defined group limit on total usage (if any), get a
     * total usage on any resource for all users in that group: */
    char mySQL2a[]="select sum(quota_usage), R_QUOTA_MAIN.quota_limit, UM1.user_id from R_QUOTA_USAGE, R_QUOTA_MAIN, R_USER_MAIN UM1, R_USER_GROUP, R_USER_MAIN UM2 where R_QUOTA_MAIN.user_id = UM1.user_id and UM1.user_type_name = 'rodsgroup' and R_USER_GROUP.group_user_id = UM1.user_id and UM2.user_id = R_USER_GROUP.user_id and R_QUOTA_USAGE.user_id = UM2.user_id and R_QUOTA_USAGE.resc_id != %s and R_QUOTA_MAIN.resc_id = %s group by UM1.user_id,  R_QUOTA_MAIN.quota_limit";
    char mySQL2b[MAX_SQL_SIZE];

    char mySQL3a[]="update R_QUOTA_MAIN set quota_over= %s - ?, modify_ts=? where user_id=? and %s - ? > quota_over";
    char mySQL3b[MAX_SQL_SIZE];


    /* Initialize over_quota values (if any) to the no-usage value
       which is the negative of the limit.  */
    if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 1");
    status =  cmlExecuteNoAnswerSql(
        "update R_QUOTA_MAIN set quota_over = -quota_limit", &icss);
    if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) return(0); /* no quotas, done */
    if (status != 0) return(status);

    /* Set the over_quota values for per-resource, if any */
    if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 2");
    status =  cmlExecuteNoAnswerSql(
#if ORA_ICAT
        "update R_QUOTA_MAIN set quota_over = (select R_QUOTA_USAGE.quota_usage - R_QUOTA_MAIN.quota_limit from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id) where exists (select 1 from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id)",
#elif MY_ICAT
        "update R_QUOTA_MAIN, R_QUOTA_USAGE set R_QUOTA_MAIN.quota_over = R_QUOTA_USAGE.quota_usage - R_QUOTA_MAIN.quota_limit where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id",
#else
        "update R_QUOTA_MAIN set quota_over = quota_usage - quota_limit from R_QUOTA_USAGE where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = R_QUOTA_USAGE.resc_id",
#endif
        &icss);
    if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) status=0; /* none */
    if (status != 0) return(status);

    /* Set the over_quota values for irods-total, if any, and only if
       the this over_quota value is higher than the previous.  Do it in
       two steps to keep it simplier (there may be a better way tho).
    */
    if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 3");
    getNowStr(myTime);
    for (rowsFound=0;;rowsFound++) {
        int status2;
        if (rowsFound==0) {
            status = cmlGetFirstRowFromSql("select sum(quota_usage), R_QUOTA_MAIN.user_id from R_QUOTA_USAGE, R_QUOTA_MAIN where R_QUOTA_MAIN.user_id = R_QUOTA_USAGE.user_id and R_QUOTA_MAIN.resc_id = '0' group by R_QUOTA_MAIN.user_id",
                                           &statementNum, 0, &icss);
        }
        else {
            status = cmlGetNextRowFromStatement(statementNum, &icss);
        }
        if (status != 0) break;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 4");
        status2 = cmlExecuteNoAnswerSql("update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and resc_id='0'",
                                        &icss);
        if (status2 == CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
        if (status2 != 0) return(status2);
    }

    /* Handle group quotas on resources */
    if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 5");
    for (rowsFound=0;;rowsFound++) {
        int status2;
        if (rowsFound==0) {
            status = cmlGetFirstRowFromSql(mySQL1, &statementNum, 
                                           0, &icss);
        }
        else {
            status = cmlGetNextRowFromStatement(statementNum, &icss);
        }
        if (status != 0) break;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[2];
        if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 6");
        status2 = cmlExecuteNoAnswerSql("update R_QUOTA_MAIN set quota_over=?-quota_limit, modify_ts=? where user_id=? and ?-quota_limit > quota_over and R_QUOTA_MAIN.resc_id=?",
                                        &icss);
        if (status2 == CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
        if (status2 != 0) return(status2);
    }
    if (status==CAT_NO_ROWS_FOUND) status=0;
    if (status != 0) return(status);

    /* Handle group quotas on total usage */
#if ORA_ICAT
    /* For Oracle cast is to integer, for Postgres to bigint,for MySQL no cast*/
    snprintf(mySQL2b, sizeof mySQL2b, mySQL2a, 
             "cast('0' as integer)", "cast('0' as integer)");
    snprintf(mySQL3b, sizeof mySQL3b, mySQL3a, 
             "cast(? as integer)", "cast(? as integer)");
#elif MY_ICAT
    snprintf(mySQL2b, sizeof mySQL2b, mySQL2a, "'0'", "'0'");
    snprintf(mySQL3b, sizeof mySQL3b, mySQL3a, "?", "?");
#else
    snprintf(mySQL2b, sizeof mySQL2b, mySQL2a,
             "cast('0' as bigint)", "cast('0' as bigint)");
    snprintf(mySQL3b, sizeof mySQL3b, mySQL3a,
             "cast(? as bigint)", "cast(? as bigint)");
#endif
    if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 7");
    getNowStr(myTime);
    for (rowsFound=0;;rowsFound++) {
        int status2;
        if (rowsFound==0) {
            status = cmlGetFirstRowFromSql(mySQL2b, &statementNum, 
                                           0, &icss);
        }
        else {
            status = cmlGetNextRowFromStatement(statementNum, &icss);
        }
        if (status != 0) break;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[1];
        cllBindVars[cllBindVarCount++]=myTime;
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[2];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[0];
        cllBindVars[cllBindVarCount++]=icss.stmtPtr[statementNum]->resultValue[1];
        if (logSQL!=0) rodsLog(LOG_SQL, "setOverQuota SQL 8");
        status2 = cmlExecuteNoAnswerSql(mySQL3b,
                                        &icss);
        if (status2 == CAT_SUCCESS_BUT_WITH_NO_INFO) status2=0;
        if (status2 != 0) return(status2);
    }
    if (status==CAT_NO_ROWS_FOUND) status=0;
    if (status != 0) return(status);

/* To simplify the query, if either of the above group operations
   found some over_quota, will probably want to update and insert rows
   for each user into R_QUOTA_MAIN.  For now tho, this is not done and
   perhaps shouldn't be, to keep it a little less complicated. */

    return(status);
}


int chlCalcUsageAndQuota(rsComm_t *rsComm) {
    int status;
    char myTime[50];

    status = 0;
    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    rodsLog(LOG_NOTICE,
            "chlCalcUsageAndQuota called");


    getNowStr(myTime);

    /* Delete the old rows from R_QUOTA_USAGE */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlCalcUsageAndQuota SQL 1");
    cllBindVars[cllBindVarCount++]=myTime;
    status =  cmlExecuteNoAnswerSql(
        "delete from R_QUOTA_USAGE where modify_ts < ?", &icss);
    if (status !=0 && status !=CAT_SUCCESS_BUT_WITH_NO_INFO) {
        _rollback("chlCalcUsageAndQuota");
        return(status);
    }

    /* Add a row to R_QUOTA_USAGE for each user's usage on each resource */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlCalcUsageAndQuota SQL 2");
    cllBindVars[cllBindVarCount++]=myTime;
    status =  cmlExecuteNoAnswerSql(
        "insert into R_QUOTA_USAGE (quota_usage, resc_id, user_id, modify_ts) (select sum(R_DATA_MAIN.data_size), R_RESC_MAIN.resc_id, R_USER_MAIN.user_id, ? from R_DATA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_name = R_DATA_MAIN.data_owner_name and R_USER_MAIN.zone_name = R_DATA_MAIN.data_owner_zone and R_RESC_MAIN.resc_name = R_DATA_MAIN.resc_name group by R_RESC_MAIN.resc_id, user_id)",
        &icss);
    if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) status=0; /* no files, OK */
    if (status != 0) {
        _rollback("chlCalcUsageAndQuota");
        return(status);
    }

    /* Set the over_quota flags where appropriate */
    status = setOverQuota(rsComm);
    if (status != 0) {
        _rollback("chlCalcUsageAndQuota");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

int chlSetQuota(rsComm_t *rsComm, char *type, char *name, 
                char *rescName, char* limit) {
    int status;
    rodsLong_t rescId;
    rodsLong_t userId;
    char userZone[NAME_LEN];
    char userName[NAME_LEN];
    char rescIdStr[60];
    char userIdStr[60];
    char myTime[50];
    int itype=0;

    if (strncmp(type, "user",4)==0) itype=1;
    if (strncmp(type, "group",5)==0) itype=2;
    if (itype==0) return (CAT_INVALID_ARGUMENT);

    status = getLocalZone();
    if (status != 0) return(status);

    /* Get the resource id; use rescId=0 for 'total' */
    rescId=0;
    if (strncmp(rescName,"total",5)!=0) { 
        if (logSQL!=0) rodsLog(LOG_SQL, "chlSetQuota SQL 1");
        status = cmlGetIntegerValueFromSql(
            "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
            &rescId, rescName, localZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
            _rollback("chlSetQuota");
            return(status);
        }
    }


    status = parseUserName(name, userName, userZone);
    if (userZone[0]=='\0') {
        strncpy(userZone, localZone, NAME_LEN);
    }

    if (itype==1) {
        userId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlSetQuota SQL 2");
        status = cmlGetIntegerValueFromSql(
            "select user_id from R_USER_MAIN where user_name=? and zone_name=?",
            &userId, userName, userZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_USER);
            _rollback("chlSetQuota");
            return(status);
        }
    }
    else {
        userId=0;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlSetQuota SQL 3");
        status = cmlGetIntegerValueFromSql(
            "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name='rodsgroup'",
            &userId, userName, userZone, 0, 0, 0, &icss);
        if (status != 0) {
            if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_GROUP);
            _rollback("chlSetQuota");
            return(status);
        }
    }

    snprintf(userIdStr, sizeof userIdStr, "%lld", userId);
    snprintf(rescIdStr, sizeof rescIdStr, "%lld", rescId);

    /* first delete previous one, if any */
    cllBindVars[cllBindVarCount++]=userIdStr;
    cllBindVars[cllBindVarCount++]=rescIdStr;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlSetQuota SQL 4");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_QUOTA_MAIN where user_id=? and resc_id=?",
        &icss);
    if (status != 0) {
        rodsLog(LOG_DEBUG,
                "chlSetQuota cmlExecuteNoAnswerSql delete failure %d",
                status);
    }
    if (atol(limit)>0) {
        getNowStr(myTime);
        cllBindVars[cllBindVarCount++]=userIdStr;
        cllBindVars[cllBindVarCount++]=rescIdStr;
        cllBindVars[cllBindVarCount++]=limit;
        cllBindVars[cllBindVarCount++]=myTime;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlSetQuota SQL 5");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_QUOTA_MAIN (user_id, resc_id, quota_limit, modify_ts) values (?, ?, ?, ?)",
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlSetQuota cmlExecuteNoAnswerSql insert failure %d",
                    status);
            _rollback("chlSetQuota");
            return(status);
        }
    }

    /* Reset the over_quota flags based on previous usage info.  The
       usage info may take a while to set, but setting the OverQuota
       should be quick.  */
    status = setOverQuota(rsComm);
    if (status != 0) {
        _rollback("chlSetQuota");
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}


int 
chlCheckQuota(rsComm_t *rsComm, char *userName, char *rescName,
              rodsLong_t *userQuota, int *quotaStatus) {
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

    char mySQL[]="select distinct QM.user_id, QM.resc_id, QM.quota_limit, QM.quota_over from R_QUOTA_MAIN QM, R_USER_MAIN UM, R_RESC_MAIN RM, R_USER_GROUP UG, R_USER_MAIN UM2 where ( (QM.user_id = UM.user_id and UM.user_name = ?) or (QM.user_id = UG.group_user_id and UM2.user_name = ? and UG.user_id = UM2.user_id) ) and ((QM.resc_id = RM.resc_id and RM.resc_name = ?) or QM.resc_id = '0') order by quota_over desc";

    *userQuota = 0;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlCheckQuota SQL 1");
    cllBindVars[cllBindVarCount++]=userName;
    cllBindVars[cllBindVarCount++]=userName;
    cllBindVars[cllBindVarCount++]=rescName;

    status = cmlGetFirstRowFromSql(mySQL, &statementNum, 
                                   0, &icss);
   
    if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlCheckQuota - CAT_SUCCESS_BUT_WITH_NO_INFO");
        *quotaStatus = QUOTA_UNRESTRICTED;
        return(0);
    }

    if (status == CAT_NO_ROWS_FOUND) {
        rodsLog(LOG_NOTICE,
                "chlCheckQuota - CAT_NO_ROWS_FOUND");
        *quotaStatus = QUOTA_UNRESTRICTED;
        return(0);
    }

    if (status != 0) return(status);

#if 0
    for (i=0;i<4;i++) {
        rodsLog(LOG_NOTICE, "checkvalue: %s", 
                icss.stmtPtr[statementNum]->resultValue[i]);
    }
#endif

    /* For now, log it */
    rodsLog(LOG_NOTICE, "checkQuota: inUser:%s inResc:%s RescId:%s Quota:%s", 
            userName, rescName, 
            icss.stmtPtr[statementNum]->resultValue[1],  /* resc_id column */
            icss.stmtPtr[statementNum]->resultValue[3]); /* quota_over column */

    *userQuota = atoll(icss.stmtPtr[statementNum]->resultValue[3]);
    if (atoi(icss.stmtPtr[statementNum]->resultValue[1])==0) {
        *quotaStatus=QUOTA_GLOBAL;
    }
    else {
        *quotaStatus=QUOTA_RESOURCE;
    }
    cmlFreeStatement(statementNum, &icss); /* only need the one row */

    return(status);
}

int 
chlDelUnusedAVUs(rsComm_t *rsComm) {
/* 
   Remove any AVUs that are currently not associated with any object.
   This is done as a separate operation for efficiency.  See 
   'iadmin h rum'.
*/
    int status;
    status = removeAVUs();

    if (status == 0) {
        status =  cmlExecuteNoAnswerSql("commit", &icss);
    }
    return(status);

}


/* 
 * chlInsRuleTable - Insert  a new iRODS Rule Base table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsRuleTable(rsComm_t *rsComm, 
                char *baseName, char *mapPriorityStr,  char *ruleName,
                char *ruleHead, char *ruleCondition, char *ruleAction, 
                char *ruleRecovery, char *ruleIdStr, char *myTime) {
    int status;
    int i;
    rodsLong_t seqNum = -1;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsRuleTable");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    /* first check if the  rule already exists */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsRuleTable SQL 1");
    i=0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=ruleName;
    cllBindVars[i++]=ruleHead;
    cllBindVars[i++]=ruleCondition;
    cllBindVars[i++]=ruleAction;
    cllBindVars[i++]=ruleRecovery;
    cllBindVarCount=i;
    status =  cmlGetIntegerValueFromSqlV3(
        "select rule_id from R_RULE_MAIN where  rule_base_name = ? and  rule_name = ? and rule_event = ? and rule_condition = ? and rule_body = ? and  rule_recovery = ?", 
        &seqNum,
        &icss);
    if (status != 0 &&  status != CAT_NO_ROWS_FOUND) {
        rodsLog(LOG_NOTICE,
                "chlInsRuleTable cmlGetIntegerValueFromSqlV3 find rule if any failure %d",status);
        return(status);
    }
    if (seqNum < 0) {
        seqNum = cmlGetNextSeqVal(&icss);
        if (seqNum < 0) {
            rodsLog(LOG_NOTICE, "chlInsRuleTable cmlGetNextSeqVal failure %d",
                    seqNum);
            _rollback("chlInsRuleTable");
            return(seqNum);
        }
        snprintf(ruleIdStr, MAX_NAME_LEN, "%lld", seqNum);

        i=0;
        cllBindVars[i++]=ruleIdStr;
        cllBindVars[i++]=baseName;
        cllBindVars[i++]=ruleName;
        cllBindVars[i++]=ruleHead;
        cllBindVars[i++]=ruleCondition;
        cllBindVars[i++]=ruleAction;
        cllBindVars[i++]=ruleRecovery;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsRuleTable SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_RULE_MAIN(rule_id, rule_base_name, rule_name, rule_event, rule_condition, rule_body, rule_recovery, rule_owner_name, rule_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsRuleTable cmlExecuteNoAnswerSql Rule Main Insert failure %d",status);
            return(status);
        }
    }
    else {
        snprintf(ruleIdStr, MAX_NAME_LEN, "%lld", seqNum);
    }
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsRuleTable SQL 3");
    i = 0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=mapPriorityStr;
    cllBindVars[i++]=ruleIdStr;
    cllBindVars[i++]=rsComm->clientUser.userName;
    cllBindVars[i++]=rsComm->clientUser.rodsZone;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVarCount=i;
    status =  cmlExecuteNoAnswerSql(
        "insert into R_RULE_BASE_MAP  (map_base_name, map_priority, rule_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlInsRuleTable cmlExecuteNoAnswerSql Rule Map insert failure %d" , status);

        return(status);
    }

    return(0);
}

/* 
 * chlInsDvmTable - Insert  a new iRODS DVM table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsDvmTable(rsComm_t *rsComm, 
               char *baseName, char *varName, char *action, 
               char *var2CMap, char *myTime) {
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char dvmIdStr[MAX_NAME_LEN];
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsDvmTable");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    /* first check if the DVM already exists */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsDvmTable SQL 1");
    i=0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=varName;
    cllBindVars[i++]=action;
    cllBindVars[i++]=var2CMap;
    cllBindVarCount=i;
    status =  cmlGetIntegerValueFromSqlV3(
        "select dvm_id from R_RULE_DVM where  dvm_base_name = ? and  dvm_ext_var_name = ? and  dvm_condition = ? and dvm_int_map_path = ? ", 
        &seqNum,
        &icss);
    if (status != 0 &&  status != CAT_NO_ROWS_FOUND) {
        rodsLog(LOG_NOTICE,
                "chlInsDvmTable cmlGetIntegerValueFromSqlV3 find DVM if any failure %d",status);
        return(status);
    }
    if (seqNum < 0) {
        seqNum = cmlGetNextSeqVal(&icss);
        if (seqNum < 0) {
            rodsLog(LOG_NOTICE, "chlInsDvmTable cmlGetNextSeqVal failure %d",
                    seqNum);
            _rollback("chlInsDvmTable");
            return(seqNum);
        }
        snprintf(dvmIdStr, MAX_NAME_LEN, "%lld", seqNum);

        i=0;
        cllBindVars[i++]=dvmIdStr;
        cllBindVars[i++]=baseName;
        cllBindVars[i++]=varName;
        cllBindVars[i++]=action;
        cllBindVars[i++]=var2CMap;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsDvmTable SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_RULE_DVM(dvm_id, dvm_base_name, dvm_ext_var_name, dvm_condition, dvm_int_map_path, dvm_owner_name, dvm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsDvmTable cmlExecuteNoAnswerSql DVM Main Insert failure %d",status);
            return(status);
        }
    }
    else {
        snprintf(dvmIdStr, MAX_NAME_LEN, "%lld", seqNum);
    }
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsDvmTable SQL 3");
    i = 0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=dvmIdStr;
    cllBindVars[i++]=rsComm->clientUser.userName;
    cllBindVars[i++]=rsComm->clientUser.rodsZone;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVarCount=i;
    status =  cmlExecuteNoAnswerSql(
        "insert into R_RULE_DVM_MAP  (map_dvm_base_name, dvm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlInsDvmTable cmlExecuteNoAnswerSql DVM Map insert failure %d" , status);

        return(status);
    }

    return(0);
}



/* 
 * chlInsFnmTable - Insert  a new iRODS FNM table row.
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlInsFnmTable(rsComm_t *rsComm, 
               char *baseName, char *funcName, 
               char *func2CMap, char *myTime) {
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char fnmIdStr[MAX_NAME_LEN];
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsFnmTable");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    /* first check if the FNM already exists */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsFnmTable SQL 1");
    i=0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=funcName;
    cllBindVars[i++]=func2CMap;
    cllBindVarCount=i;
    status =  cmlGetIntegerValueFromSqlV3(
        "select fnm_id from R_RULE_FNM where  fnm_base_name = ? and  fnm_ext_func_name = ? and  fnm_int_func_name = ? ", 
        &seqNum,
        &icss);
    if (status != 0 &&  status != CAT_NO_ROWS_FOUND) {
        rodsLog(LOG_NOTICE,
                "chlInsFnmTable cmlGetIntegerValueFromSqlV3 find FNM if any failure %d",status);
        return(status);
    }
    if (seqNum < 0) {
        seqNum = cmlGetNextSeqVal(&icss);
        if (seqNum < 0) {
            rodsLog(LOG_NOTICE, "chlInsFnmTable cmlGetNextSeqVal failure %d",
                    seqNum);
            _rollback("chlInsFnmTable");
            return(seqNum);
        }
        snprintf(fnmIdStr, MAX_NAME_LEN, "%lld", seqNum);

        i=0;
        cllBindVars[i++]=fnmIdStr;
        cllBindVars[i++]=baseName;
        cllBindVars[i++]=funcName;
        cllBindVars[i++]=func2CMap;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsFnmTable SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_RULE_FNM(fnm_id, fnm_base_name, fnm_ext_func_name, fnm_int_func_name, fnm_owner_name, fnm_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsFnmTable cmlExecuteNoAnswerSql FNM Main Insert failure %d",status);
            return(status);
        }
    }
    else {
        snprintf(fnmIdStr, MAX_NAME_LEN, "%lld", seqNum);
    }
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsFnmTable SQL 3");
    i = 0;
    cllBindVars[i++]=baseName;
    cllBindVars[i++]=fnmIdStr;
    cllBindVars[i++]=rsComm->clientUser.userName;
    cllBindVars[i++]=rsComm->clientUser.rodsZone;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVarCount=i;
    status =  cmlExecuteNoAnswerSql(
        "insert into R_RULE_FNM_MAP  (map_fnm_base_name, fnm_id, map_owner_name,map_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?)",
        &icss);
    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlInsFnmTable cmlExecuteNoAnswerSql FNM Map insert failure %d" , status);

        return(status);
    }

    return(0);
}





/* 
 * chlInsMsrvcTable - Insert  a new iRODS MSRVC table row (actually in two tables).
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int chlInsMsrvcTable(rsComm_t *rsComm,
                     char *moduleName, char *msrvcName,
                     char *msrvcSignature,  char *msrvcVersion,
                     char *msrvcHost, char *msrvcLocation,
                     char *msrvcLanguage,  char *msrvcTypeName, char *msrvcStatus,
                     char *myTime) 
{
    int status;
    int i;
    rodsLong_t seqNum = -1;
    char msrvcIdStr[MAX_NAME_LEN];
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    /* first check if the MSRVC already exists */
    if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable SQL 1");
    i=0;
    cllBindVars[i++]=moduleName;
    cllBindVars[i++]=msrvcName;
    cllBindVarCount=i;
    status =  cmlGetIntegerValueFromSqlV3(
        "select msrvc_id from R_MICROSRVC_MAIN where  msrvc_module_name = ? and  msrvc_name = ? ", 
        &seqNum,
        &icss);
    if (status != 0 &&  status != CAT_NO_ROWS_FOUND) {
        rodsLog(LOG_NOTICE,
                "chlInsMsrvcTable cmlGetIntegerValueFromSqlV3 find MSRVC if any failure %d",status);
        return(status);
    }
    if (seqNum < 0) { /* No micro-service found */
        seqNum = cmlGetNextSeqVal(&icss);
        if (seqNum < 0) {
            rodsLog(LOG_NOTICE, "chlInsMsrvcTable cmlGetNextSeqVal failure %d",
                    seqNum);
            _rollback("chlInsMsrvcTable");
            return(seqNum);
        }
        snprintf(msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum);
        /* inserting in R_MICROSRVC_MAIN */
        i=0;
        cllBindVars[i++]=msrvcIdStr;
        cllBindVars[i++]=msrvcName;
        cllBindVars[i++]=moduleName;
        cllBindVars[i++]=msrvcSignature;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_MICROSRVC_MAIN(msrvc_id, msrvc_name, msrvc_module_name, msrvc_signature, msrvc_doxygen, msrvc_variations, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?,   'NONE', 'NONE',  ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_MAIN Insert failure %d",status);
            return(status);
        }
        /* inserting in R_MICROSRVC_VER */
        i=0;
        cllBindVars[i++]=msrvcIdStr;
        cllBindVars[i++]=msrvcVersion;
        cllBindVars[i++]=msrvcHost;
        cllBindVars[i++]=msrvcLocation;
        cllBindVars[i++]=msrvcLanguage;
        cllBindVars[i++]=msrvcTypeName;
        cllBindVars[i++]=msrvcStatus;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable SQL 3");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_status, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure %d",status);
            return(status);
        }
    }
    else { /* micro-service already there */
        snprintf(msrvcIdStr, MAX_NAME_LEN, "%lld", seqNum);
        /* Check if same host and location exists - if so no need to insert a new row */
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable SQL 4");
        i=0;
        cllBindVars[i++]=msrvcIdStr;
        cllBindVars[i++]=msrvcHost;
        cllBindVars[i++]=msrvcLocation;
        cllBindVarCount=i;
        status =  cmlGetIntegerValueFromSqlV3(
            "select msrvc_id from R_MICROSRVC_VER where  msrvc_id = ? and  msrvc_host = ? and  msrvc_location = ? ",
            &seqNum, &icss);
        if (status != 0 &&  status != CAT_NO_ROWS_FOUND) {
            rodsLog(LOG_NOTICE,
                    "chlInsMsrvcTable cmlGetIntegerValueFromSqlV4 find MSRVC_HOST if any failure %d",status);
            return(status);
        }
        /* insert a new row into version table */
        i=0;
        cllBindVars[i++]=msrvcIdStr;
        cllBindVars[i++]=msrvcVersion;
        cllBindVars[i++]=msrvcHost;
        cllBindVars[i++]=msrvcLocation;
        cllBindVars[i++]=msrvcLanguage;
        cllBindVars[i++]=msrvcTypeName;
        cllBindVars[i++]=rsComm->clientUser.userName;
        cllBindVars[i++]=rsComm->clientUser.rodsZone;
        cllBindVars[i++]=myTime;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlInsMsrvcTable SQL 3");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_MICROSRVC_VER(msrvc_id, msrvc_version, msrvc_host, msrvc_location, msrvc_language, msrvc_type_name, msrvc_owner_name, msrvc_owner_zone, create_ts, modify_ts) values (?, ?, ?, ?, ?, ?,  ?, ?, ?, ?)", 
            &icss);
        if (status != 0) {
            rodsLog(LOG_NOTICE,
                    "chlInsMsrvcTable cmlExecuteNoAnswerSql R_MICROSRVC_VER Insert failure %d",status);
            return(status);
        }
    }

    return(0);
}


/*
 * chlVersionRuleBase - Version out the old base maps with timestamp 
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionRuleBase(rsComm_t *rsComm,
                   char *baseName, char *myTime) {

    int i, status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionRuleBase");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    i=0;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=baseName;
    cllBindVarCount=i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionRuleBase SQL 1");
  
    status =  cmlExecuteNoAnswerSql(
        "update R_RULE_BASE_MAP set map_version = ?, modify_ts = ? where map_base_name = ? and map_version = '0'",&icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlVersionRuleBase cmlExecuteNoAnswerSql Rule Map version update  failure %d" , status);
        return(status);

    }

    return(0);
}


/*
 * chlVersionDvmBase - Version out the old dvm base maps with timestamp 
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionDvmBase(rsComm_t *rsComm,
                  char *baseName, char *myTime) {
    int i, status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionDvmBase");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    i=0;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=baseName;
    cllBindVarCount=i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionDvmBase SQL 1");
  
    status =  cmlExecuteNoAnswerSql(
        "update R_RULE_DVM_MAP set map_dvm_version = ?, modify_ts = ? where map_dvm_base_name = ? and map_dvm_version = '0'",&icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlVersionDvmBase cmlExecuteNoAnswerSql DVM Map version update  failure %d" , status);
        return(status);

    }

    return(0);
}


/*
 * chlVersionFnmBase - Version out the old fnm base maps with timestamp 
 * Input - rsComm_t *rsComm  - the server handle,
 *    input values.
 */
int
chlVersionFnmBase(rsComm_t *rsComm,
                  char *baseName, char *myTime) {

    int i, status;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionFnmBase");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    i=0;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=myTime;
    cllBindVars[i++]=baseName;
    cllBindVarCount=i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlVersionFnmBase SQL 1");
  
    status =  cmlExecuteNoAnswerSql(
        "update R_RULE_FNM_MAP set map_fnm_version = ?, modify_ts = ? where map_fnm_base_name = ? and map_fnm_version = '0'",&icss);
    if (status != 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO) {
        rodsLog(LOG_NOTICE,
                "chlVersionFnmBase cmlExecuteNoAnswerSql FNM Map version update  failure %d" , status);
        return(status);

    }

    return(0);
}







int
icatCheckResc(char *rescName) {
    int status;
    rodsLong_t rescId;
    status = getLocalZone();
    if (status != 0) return(status);

    rescId=0;
    if (logSQL!=0) rodsLog(LOG_SQL, "icatCheckResc SQxL 1");
    status = cmlGetIntegerValueFromSql(
        "select resc_id from R_RESC_MAIN where resc_name=? and zone_name=?",
        &rescId, rescName, localZone, 0, 0, 0, &icss);
    if (status != 0) {
        if (status==CAT_NO_ROWS_FOUND) return(CAT_INVALID_RESOURCE);
        _rollback("icatCheckResc");
    }
    return(status);
}

int
chlAddSpecificQuery(rsComm_t *rsComm, char *sql, char *alias) {
    int status, i;
    char myTime[50];
    char tsCreateTime[50];
    if (logSQL!=0) rodsLog(LOG_SQL, "chlAddSpecificQuery");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (strlen(sql) < 5) {
        return(CAT_INVALID_ARGUMENT);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    getNowStr(myTime);

    if (alias != NULL && strlen(alias)>0) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddSpecificQuery SQL 1");
        status = cmlGetStringValueFromSql(
            "select create_ts from R_SPECIFIC_QUERY where alias=?",
            tsCreateTime, 50,
            alias, "" , "", &icss);
        if (status==0) {
            i = addRErrorMsg (&rsComm->rError, 0, "Alias is not unique");
            return(CAT_INVALID_ARGUMENT);
        }
        i=0;
        cllBindVars[i++]=sql;
        cllBindVars[i++]=alias;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddSpecificQuery SQL 2");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_SPECIFIC_QUERY  (sqlStr, alias, create_ts) values (?, ?, ?)",
            &icss);
    }
    else {
        i=0;
        cllBindVars[i++]=sql;
        cllBindVars[i++]=myTime;
        cllBindVarCount=i;
        if (logSQL!=0) rodsLog(LOG_SQL, "chlAddSpecificQuery SQL 3");
        status =  cmlExecuteNoAnswerSql(
            "insert into R_SPECIFIC_QUERY  (sqlStr, create_ts) values (?, ?)",
            &icss);
    }

    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlAddSpecificQuery cmlExecuteNoAnswerSql insert failure %d",
                status);
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
}

int
chlDelSpecificQuery(rsComm_t *rsComm, char *sqlOrAlias) {
    int status, i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelSpecificQuery");

    if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
    }

    if (!icss.status) {
        return(CATALOG_NOT_CONNECTED);
    }

    i=0;
    cllBindVars[i++]=sqlOrAlias;
    cllBindVarCount=i;
    if (logSQL!=0) rodsLog(LOG_SQL, "chlDelSpecificQuery SQL 1");
    status =  cmlExecuteNoAnswerSql(
        "delete from R_SPECIFIC_QUERY where sqlStr = ?", 
        &icss);

    if (status==CAT_SUCCESS_BUT_WITH_NO_INFO) {
        if (logSQL!=0) rodsLog(LOG_SQL, "chlDelSpecificQuery SQL 2");
        i=0;
        cllBindVars[i++]=sqlOrAlias;
        cllBindVarCount=i;
        status =  cmlExecuteNoAnswerSql(
            "delete from R_SPECIFIC_QUERY where alias = ?", 
            &icss);
    }

    if (status != 0) {
        rodsLog(LOG_NOTICE,
                "chlDelSpecificQuery cmlExecuteNoAnswerSql delete failure %d",
                status);
        return(status);
    }

    status =  cmlExecuteNoAnswerSql("commit", &icss);
    return(status);
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
chlSpecificQuery(specificQueryInp_t specificQueryInp, genQueryOut_t *result) {
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

    int debug=0;

    icatSessionStruct *icss;

    if (logSQL!=0) rodsLog(LOG_SQL, "chlSpecificQuery");

    result->attriCnt=0;
    result->rowCnt=0;
    result->totalRowCount = 0;

    currentMaxColSize=0;

    icss = chlGetRcs();
    if (icss==NULL) return(CAT_NOT_OPEN);
#ifdef ADDR_64BITS
    if (debug) printf("icss=%ld\n",(long int)icss);
#else
    if (debug) printf("icss=%d\n",(int)icss);
#endif

    if (specificQueryInp.continueInx == 0) {
        if (specificQueryInp.sql == NULL) {
            return(CAT_INVALID_ARGUMENT);
        }
/*
  First check that this SQL is one of the allowed forms.
*/
        if (logSQL!=0) rodsLog(LOG_SQL, "chlSpecificQuery SQL 1");
        status = cmlGetStringValueFromSql(
            "select create_ts from R_SPECIFIC_QUERY where sqlStr=?",
            tsCreateTime, 50,
            specificQueryInp.sql, "" , "", icss);
        if (status == CAT_NO_ROWS_FOUND) {
            int status2;
            if (logSQL!=0) rodsLog(LOG_SQL, "chlSpecificQuery SQL 2");
            status2 = cmlGetStringValueFromSql(
                "select sqlStr from R_SPECIFIC_QUERY where alias=?",
                combinedSQL, sizeof(combinedSQL),
                specificQueryInp.sql, "" , "", icss);
            if (status2 == CAT_NO_ROWS_FOUND) return(CAT_UNKNOWN_SPECIFIC_QUERY);
            if (status2 != 0) return(status2);
        }
        else {
            if (status != 0) return(status);
            strncpy(combinedSQL, specificQueryInp.sql, sizeof(combinedSQL));
        }

        i=0;
        while (specificQueryInp.args[i]!=NULL && strlen(specificQueryInp.args[i])>0) {
            cllBindVars[cllBindVarCount++]=specificQueryInp.args[i++];
        }

        if (logSQL!=0) rodsLog(LOG_SQL, "chlSpecificQuery SQL 3");
        status = cmlGetFirstRowFromSql(combinedSQL, &statementNum, 
                                       specificQueryInp.rowOffset, icss);
        if (status < 0) {
            if (status != CAT_NO_ROWS_FOUND) {
                rodsLog(LOG_NOTICE,
                        "chlSpecificQuery cmlGetFirstRowFromSql failure %d",
                        status);
            }
            return(status);
        }

        result->continueInx = statementNum+1;
        if (debug) printf("statement number =%d\n", statementNum);
        needToGetNextRow = 0;
    }
    else {
        statementNum = specificQueryInp.continueInx-1;
        needToGetNextRow = 1;
        if (specificQueryInp.maxRows<=0) {  /* caller is closing out the query */
            status = cmlFreeStatement(statementNum, icss);
            return(status);
        }
    }
    for (i=0;i<specificQueryInp.maxRows;i++) {
        if (needToGetNextRow) {
            status = cmlGetNextRowFromStatement(statementNum, icss);
            if (status == CAT_NO_ROWS_FOUND) {
                cmlFreeStatement(statementNum, icss);
                result->continueInx=0;
                if (result->rowCnt==0) return(status); /* NO ROWS; in this 
                                                          case a continuation call is finding no more rows */
                return(0);
            }
            if (status < 0) return(status);
        }
        needToGetNextRow = 1;

        result->rowCnt++;
        if (debug) printf("result->rowCnt=%d\n", result->rowCnt);
        numOfCols = icss->stmtPtr[statementNum]->numOfCols;
        if (debug) printf("numOfCols=%d\n",numOfCols);
        result->attriCnt=numOfCols;
        result->continueInx = statementNum+1;

        maxColSize=0;

        for (k=0;k<numOfCols;k++) {
            j = strlen(icss->stmtPtr[statementNum]->resultValue[k]);
            if (maxColSize <= j) maxColSize=j;
        }
        maxColSize++; /* for the null termination */
        if (maxColSize < MINIMUM_COL_SIZE) {
            maxColSize=MINIMUM_COL_SIZE;  /* make it a reasonable size */
        }
        if (debug) printf("maxColSize=%d\n",maxColSize);

        if (i==0) {  /* first time thru, allocate and initialize */
            attriTextLen= numOfCols * maxColSize;
            if (debug) printf("attriTextLen=%d\n",attriTextLen);
            totalLen = attriTextLen * specificQueryInp.maxRows;
            for (j=0;j<numOfCols;j++) {
                tResult = (char *) malloc(totalLen);
                if (tResult==NULL) return(SYS_MALLOC_ERR);
                memset(tResult, 0, totalLen);
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
        if (maxColSize > currentMaxColSize) {
            maxColSize += MINIMUM_COL_SIZE; /* bump it up to try to avoid
                                               some multiple resizes */
            if (debug) printf("Bumping %d to %d\n",
                              currentMaxColSize, maxColSize);
            attriTextLen= numOfCols * maxColSize;
            if (debug) printf("attriTextLen=%d\n",attriTextLen);
            totalLen = attriTextLen * specificQueryInp.maxRows;
            for (j=0;j<numOfCols;j++) {
                char *cp1, *cp2;
                int k;
                tResult = (char *) malloc(totalLen);
                if (tResult==NULL) return(SYS_MALLOC_ERR);
                memset(tResult, 0, totalLen);
                cp1 = result->sqlResult[j].value;
                cp2 = tResult;
                for (k=0;k<result->rowCnt;k++) {
                    strncpy(cp2, cp1, result->sqlResult[j].len);
                    cp1 += result->sqlResult[j].len;
                    cp2 += maxColSize;
                }
                free(result->sqlResult[j].value);
                result->sqlResult[j].len = maxColSize;
                result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }

        /* Store the current row values into the appropriate spots in
           the attribute string */
        for (j=0;j<numOfCols;j++) {
            tResult2 = result->sqlResult[j].value; /* ptr to value str */
            tResult2 += currentMaxColSize*(result->rowCnt-1);  /* skip forward 
                                                                  for this row */
            strncpy(tResult2, icss->stmtPtr[statementNum]->resultValue[j],
                    currentMaxColSize); /* copy in the value text */
        }

    }

    result->continueInx=statementNum+1;  /* the statementnumber but
                                            always >0 */
    return(0);

}


/*
 * chlSubstituteResourceHierarchies - Given an old resource hierarchy string and a new one,
 * replaces all r_data_main.resc_hier rows that match the old string with the new one.
 *
 */
int chlSubstituteResourceHierarchies(rsComm_t *rsComm, char *oldHier, char *newHier) {
	int status = 0;
	char oldHier_partial[MAX_NAME_LEN];
    eirods::sql_logger logger("chlSubstituteResourceHierarchies", logSQL);

    logger.log();

    // =-=-=-=-=-=-=-
    // Sanity and permission checks
	if (!icss.status) {
		return(CATALOG_NOT_CONNECTED);
	}
    if (!rsComm || !oldHier || !newHier) {
    	return(SYS_INTERNAL_NULL_INPUT_ERR);
    }
	if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH || rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
		return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
	}

	// =-=-=-=-=-=-=-
	// String to match partial hierarchies
	snprintf(oldHier_partial, MAX_NAME_LEN, "%s;%%", oldHier);

	// =-=-=-=-=-=-=-
	// Update r_data_main
	cllBindVars[cllBindVarCount++]=newHier;
	cllBindVars[cllBindVarCount++]=oldHier;
	cllBindVars[cllBindVarCount++]=oldHier;
	cllBindVars[cllBindVarCount++]=oldHier_partial;
#if ORA_ICAT // Oracle
	status = cmlExecuteNoAnswerSql("update R_DATA_MAIN set resc_hier = ? || substr(resc_hier, (length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss);
#else // Postgres and MySQL
	status = cmlExecuteNoAnswerSql("update R_DATA_MAIN set resc_hier = ? || substring(resc_hier from (char_length(?)+1)) where resc_hier = ? or resc_hier like ?", &icss);
#endif

	// =-=-=-=-=-=-=-
	// Roll back if error
	if (status) {
		std::stringstream ss;
		ss << "chlSubstituteResourceHierarchies: cmlExecuteNoAnswerSql update failure " << status;
		eirods::log(LOG_NOTICE, ss.str());
		_rollback("chlSubstituteResourceHierarchies");
	}
	else {
		status =  cmlExecuteNoAnswerSql("commit", &icss);
	}

	return status;
}

/// =-=-=-=-=-=-=-
/// @brief return a map of children who do not have a repl count meeting
///        the num_children limit based on object path whose payload is a
///        vector of the valid repl resource hierarchies
int chlGetDataObjsOnResourceForLimitAndNumChildren( 
    const std::string&   _resc_name,
    int                  _num_children,
    int                  _limit,
    repl_query_result_t& _results ) {
    // =-=-=-=-=-=-=-
    // clear the results
    _results.clear();

    // =-=-=-=-=-=-=-
    // the basic query string
    char query[ MAX_NAME_LEN ];
    std::string base_query = "select data_name, coll_name, resc_hier, data_mode from r_data_main join r_coll_main using(coll_id) where data_id in (select data_id from r_data_main where resc_hier like '%s;%s;%s' or resc_hier like '%s%s;%s' group by data_id having (count(data_id)<%d)) limit %d";
    sprintf( 
        query, 
        base_query.c_str(), 
        "%", _resc_name.c_str(), 
        "%", "%", 
        _resc_name.c_str(), 
        "%", _num_children, 
        _limit );

    // =-=-=-=-=-=-=-
    // snag the first row from the resulting query
    int statement_num = 0;

    // =-=-=-=-=-=-=-
    // iterate over resulting rows
    for( int i = 0; ; i++ ) {
        // =-=-=-=-=-=-=-
        // extract either the first or next row
        int status = 0;
        if( 0 == i ) {
            status = cmlGetFirstRowFromSql( 
                             query, 
                             &statement_num, 
                             0, &icss );
        } else {
            status = cmlGetNextRowFromStatement( statement_num, &icss );
        }

        if ( status != 0 ) {
            return status; 
        }

        // =-=-=-=-=-=-=-
        // extract the results from the row
        std::string data_name( icss.stmtPtr[ statement_num ]->resultValue[0] );
        std::string coll_name( icss.stmtPtr[ statement_num ]->resultValue[1] );
        std::string resc_hier( icss.stmtPtr[ statement_num ]->resultValue[2] );
        int mode = atoi( icss.stmtPtr[ statement_num ]->resultValue[3] );

        // =-=-=-=-=-=-=-
        // build the obj path and add to the result set
        coll_name += "/";
        coll_name += data_name;

        _results[ coll_name ].push_back( std::make_pair( resc_hier, mode ) );

    } // for i
               
    cmlFreeStatement( statement_num, &icss );
     
    return 0;
     
} // chlGetDataObjsOnResourceForLimit


/*
 * @brief Given a resource, resolves the hierarchy down to said resource
 */
int chlResolveResourceHierarchy(const std::string& resc_name, const std::string& zone_name, std::string& hierarchy) {
	char *current_node;
	char parent[MAX_NAME_LEN];
	int status;


    eirods::sql_logger logger("chlResolveResourceHierarchy", logSQL);
    logger.log();

	if (!icss.status) {
		return(CATALOG_NOT_CONNECTED);
	}

	hierarchy = resc_name; // Initialize hierarchy string with resource

	current_node = (char *)resc_name.c_str();
	while (current_node) {
		// Ask for parent of current node
		status = cmlGetStringValueFromSql("select resc_parent from R_RESC_MAIN where resc_name=? and zone_name=?",
				parent, MAX_NAME_LEN, current_node, zone_name.c_str(), NULL, &icss);

		if (status == CAT_NO_ROWS_FOUND) { // Resource doesn't exist
			return CAT_INVALID_RESOURCE;
		}

		if (status < 0) { // Other error
			return status;
		}

		if (parent && strlen(parent)) {
			hierarchy = parent + eirods::hierarchy_parser::delimiter() + hierarchy;	// Add parent to hierarchy string
			current_node = parent;
		}
		else {
			current_node = NULL;
		}
	}

	return 0;
}


/*
 * @brief Updates object count for resource and its parents
 */
int chlUpdateObjCountForRescAndUp(rsComm_t *rsComm, const std::string& resc_name, const std::string& zone_name, int delta) {
	std::string resc_hier;
	int status;

    eirods::sql_logger logger("chlUpdateObjCountForRescAndUp", logSQL);
    logger.log();

    // =-=-=-=-=-=-=-
    // Sanity and permission checks
	if (!icss.status) {
		return(CATALOG_NOT_CONNECTED);
	}
    if (!rsComm) {
    	return(SYS_INTERNAL_NULL_INPUT_ERR);
    }
	if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH || rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
		return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
	}

    // =-=-=-=-=-=-=-
    // Resolve resource hierarchy
    status = chlResolveResourceHierarchy(resc_name, zone_name, resc_hier);

    if (status < 0) {
    	return status;
    }

    // =-=-=-=-=-=-=-
    // Update object count for hierarchy
    status = _updateObjCountOfResources(resc_hier, zone_name, delta);

	// =-=-=-=-=-=-=-
	// commit for _updateObjCountOfResources()
	if (status < 0) {
		std::stringstream ss;
		ss << "chlUpdateObjCountForRescAndUp: _updateObjCountOfResources failed " << status;
		eirods::log(LOG_NOTICE, ss.str());
		_rollback("chlUpdateObjCountForRescAndUp");
	}
	else {
		status =  cmlExecuteNoAnswerSql("commit", &icss);
	}

	return status;
}














