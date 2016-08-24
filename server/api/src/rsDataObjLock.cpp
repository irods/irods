/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjLock.h for a description of this API call.*/

#include "rcMisc.h"
#include "dataObjLock.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsDataObjLock.hpp"

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

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        return _rsDataObjLock( dataObjInp );
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

}

int
_rsDataObjLock( dataObjInp_t *dataObjInp ) {

    int cmd, type;
    int status = getLockCmdAndType( &dataObjInp->condInput, &cmd, &type );
    if ( status < 0 ) {
        return status;
    }

    status = fsDataObjLock( dataObjInp->objPath, cmd, type );
    return status;
}

int
getLockCmdAndType( keyValPair_t *condInput, int *cmd, int *type ) {
    char *lockType, *lockCmd;
    int status;

    if ( condInput == NULL || cmd == NULL || type == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    lockType = getValByKey( condInput, LOCK_TYPE_KW );
    if ( lockType == NULL ) {
        return SYS_LOCK_TYPE_INP_ERR;
    }

    if ( strcmp( lockType, READ_LOCK_TYPE ) == 0 ) {
        *type = F_RDLCK;
    }
    else if ( strcmp( lockType, WRITE_LOCK_TYPE ) == 0 ) {
        *type = F_WRLCK;
    }
    else {
        status = SYS_LOCK_TYPE_INP_ERR;
        rodsLogError( LOG_ERROR, status,
                      "getLockCmdAndType: illegal lock type %s", lockType );
        return status;
    }

    lockCmd = getValByKey( condInput, LOCK_CMD_KW );
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
        status = SYS_LOCK_CMD_INP_ERR;
        rodsLogError( LOG_ERROR, status,
                      "getLockCmdAndType: illegal lock cmd %s", lockCmd );
        return status;
    }
    return 0;
}

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

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        return _rsDataObjUnlock( dataObjInp );
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}

int
_rsDataObjUnlock( dataObjInp_t *dataObjInp ) {

    char * fd_string = getValByKey( &dataObjInp->condInput, LOCK_FD_KW );
    if ( fd_string  == NULL ) {
        int status = SYS_LOCK_TYPE_INP_ERR;
        rodsLogError( LOG_ERROR, status,
                      "getLockCmdAndType: LOCK_FD_KW not defined for unlock operation" );
        return status;
    }
    return fsDataObjUnlock( F_SETLK, F_UNLCK, atoi( fd_string ) );

}
// =-=-=-=-=-=-=-
