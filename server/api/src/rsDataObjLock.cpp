/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjLock.h for a description of this API call.*/

#include "irods/rcMisc.h"
#include "irods/dataObjLock.h"
#include "irods/rodsLog.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/physPath.hpp"
#include "irods/specColl.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/rsDataObjLock.hpp"

namespace {

int
getLockCmdAndType(const keyValPair_t& condInput, int *cmd, int *type ) {
    if (!cmd || !type) {
        return USER__NULL_INPUT_ERR;
    }

    char* lockType = getValByKey(&condInput, LOCK_TYPE_KW );
    if (!lockType) {
        rodsLog(LOG_ERROR, "%s: lockType cannot be null", __FUNCTION__);
        return SYS_LOCK_TYPE_INP_ERR;
    }

    if ( strcmp( lockType, READ_LOCK_TYPE ) == 0 ) {
        *type = F_RDLCK;
    }
    else if ( strcmp( lockType, WRITE_LOCK_TYPE ) == 0 ) {
        *type = F_WRLCK;
    }
    else {
        constexpr int status = SYS_LOCK_TYPE_INP_ERR;
        rodsLogError( LOG_ERROR, status,
                      "%s: illegal lock type %s", __FUNCTION__, lockType );
        return status;
    }

    char* lockCmd = getValByKey(&condInput, LOCK_CMD_KW );
    if ( lockCmd  == NULL ) {
        /* default to F_SETLKW */
        *cmd = F_SETLKW;
        return 0;
    }

    if ( strcmp( lockCmd, SET_LOCK_CMD ) == 0 ) {
        *cmd = F_SETLK;
    }
    else if ( strcmp( lockCmd, SET_LOCK_WAIT_CMD ) == 0 ) {
        *cmd = F_SETLKW;
    }
    else if ( strcmp( lockCmd, GET_LOCK_CMD ) == 0 ) {
        *cmd = F_GETLK;
    }
    else {
        constexpr int status = SYS_LOCK_CMD_INP_ERR;
        rodsLogError( LOG_ERROR, status,
                      "%s: illegal lock cmd %s", __FUNCTION__, lockCmd );
        return status;
    }
    return 0;
} // getLockCmdAndType

} // anonymous namespace

int
rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {

    specCollCache_t *specCollCache = NULL;
    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );

    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag = getAndConnRcatHost(
                         rsComm,
                         MASTER_RCAT,
                         ( const char* )dataObjInp->objPath,
                         &rodsServerHost );
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        if ( rodsServerHost != NULL ) { // JMC - cppcheck null ref
            return rcDataObjLock( rodsServerHost->conn, dataObjInp );
        }
        else {
            return SYS_NO_RCAT_SERVER_ERR;
        }
    }
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int cmd, type;
        if (const int status = getLockCmdAndType(dataObjInp->condInput, &cmd, &type);
            status < 0) {
            return status;
        }
        return fsDataObjLock( dataObjInp->objPath, cmd, type );
    }
    else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    }
    else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

} // rsDataObjLock


// =-=-=-=-=-=-=-
// JMC - backport 4604
int
rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {

    specCollCache_t *specCollCache = NULL;
    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );

    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag = getAndConnRcatHost(
                         rsComm,
                         MASTER_RCAT,
                         ( const char* )dataObjInp->objPath,
                         &rodsServerHost );
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        if ( rodsServerHost != NULL ) { // JMC - cppcheck null ref
            return rcDataObjUnlock( rodsServerHost->conn, dataObjInp );
        }
        else {
            return SYS_NO_RCAT_SERVER_ERR;
        }
    }
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        char * fd_string = getValByKey( &dataObjInp->condInput, LOCK_FD_KW );
        if (!fd_string) {
            int status = SYS_LOCK_TYPE_INP_ERR;
            rodsLogError( LOG_ERROR, status,
                          "%s: LOCK_FD_KW not defined for unlock operation", __FUNCTION__ );
            return status;
        }
        return fsDataObjUnlock( F_SETLK, F_UNLCK, atoi( fd_string ) );
    }
    else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    }
    else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
} // rsDataObjUnlock

