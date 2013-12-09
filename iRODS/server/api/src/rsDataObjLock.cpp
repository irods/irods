/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjLock.h for a description of this API call.*/

#include "dataObjLock.hpp"
#include "rodsLog.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "reDefines.hpp"
#include "getRemoteZoneResc.hpp"

int
rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    int remoteFlag;
    specCollCache_t *specCollCache = NULL;
    rodsServerHost_t *rodsServerHost = NULL;

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );

    remoteFlag = getAndConnRcatHost( rsComm, MASTER_RCAT, dataObjInp->objPath,
                                     &rodsServerHost );
    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        if ( rodsServerHost != NULL ) { // JMC - cppcheck null ref
            status = rcDataObjLock( rodsServerHost->conn, dataObjInp );
            return ( status );
        }
        else {
            status = SYS_NO_RCAT_SERVER_ERR;
        }
    }
    else {
#ifdef RODS_CAT
        status = _rsDataObjLock( rsComm, dataObjInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    return ( status );
}

int
_rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    int cmd, type, fd;

    fd = getLockCmdAndType( &dataObjInp->condInput, &cmd, &type );
    if ( fd < 0 ) {
        return fd;
    }

    status = fsDataObjLock( dataObjInp->objPath, cmd, type, fd );
    return status;
}

int
getLockCmdAndType( keyValPair_t *condInput, int *cmd, int *type ) {
    char *lockType, *lockCmd, *lockFd;
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
    else if ( strcmp( lockType, UNLOCK_TYPE ) == 0 ) {
        *type = F_UNLCK;
        *cmd  = F_SETLK; // JMC - backport 4604
        lockFd = getValByKey( condInput, LOCK_FD_KW );
        if ( lockFd  != NULL ) {
            return ( atoi( lockFd ) );
        }
        else {
            status = SYS_LOCK_TYPE_INP_ERR;
            rodsLogError( LOG_ERROR, status,
                          "getLockCmdAndType: LOCK_FD_KW not defined for UNLOCK_TYPE" );
            return status;
        }
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
rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp, int fd ) {
    char tmpStr[NAME_LEN];
    int status;

    snprintf( tmpStr, NAME_LEN, "%-d", fd );
    addKeyVal( &dataObjInp->condInput, LOCK_FD_KW, tmpStr );
    addKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW, UNLOCK_TYPE );

    status = rsDataObjLock( rsComm, dataObjInp );

    return status;
}
// =-=-=-=-=-=-=-
