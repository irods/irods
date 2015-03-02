/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCopy.h for a description of this API call.*/

#include "dataObjCopy.hpp"
#include "rodsLog.hpp"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "dataObjOpen.hpp"
#include "dataObjCreate.hpp"
#include "dataObjRepl.hpp"
#include "regDataObj.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "getRemoteZoneResc.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"

int
rsDataObjCopy( rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp,
               transferStat_t **transStat ) {
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    int srcL1descInx, destL1descInx;
    int status;
    int existFlag;
    uint createMode;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    destDataObjInp = &dataObjCopyInp->destDataObjInp;

    resolveLinkedPath( rsComm, srcDataObjInp->objPath, &specCollCache, &srcDataObjInp->condInput );
    resolveLinkedPath( rsComm, destDataObjInp->objPath, &specCollCache, &destDataObjInp->condInput );

    remoteFlag = getAndConnRemoteZoneForCopy( rsComm, dataObjCopyInp, &rodsServerHost );
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = _rcDataObjCopy( rodsServerHost->conn, dataObjCopyInp,
                                 transStat );
        return status;
    }

    if ( strcmp( srcDataObjInp->objPath, destDataObjInp->objPath ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "rsDataObjCopy: same src and dest objPath %s not allowed",
                 srcDataObjInp->objPath );
        return USER_INPUT_PATH_ERR;
    }

    addKeyVal( &srcDataObjInp->condInput, PHYOPEN_BY_SIZE_KW, "" );

    srcL1descInx = rsDataObjOpen( rsComm, srcDataObjInp );

    if ( srcL1descInx < 0 ) {
        std::stringstream msg;
        char* sys_error;
        char* rods_error = rodsErrorName( srcL1descInx, &sys_error );
        msg << __FUNCTION__;
        msg << " - Failed to open source object: \"";
        msg << srcDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        irods::log( LOG_ERROR, msg.str() );
        return srcL1descInx;
    }

    /* have to set L1desc[srcL1descInx].dataSize because open set this to -1 */
    destDataObjInp->dataSize = L1desc[srcL1descInx].dataSize =
                                   L1desc[srcL1descInx].dataObjInfo->dataSize;

    createMode = atoi( L1desc[srcL1descInx].dataObjInfo->dataMode );

    if ( createMode >= 0100 ) {
        destDataObjInp->createMode = createMode;
    }

    L1desc[srcL1descInx].oprType = COPY_SRC;

    if ( L1desc[srcL1descInx].l3descInx <= 2 ) {
        /* dataSingleBuf */
        addKeyVal( &destDataObjInp->condInput, NO_OPEN_FLAG_KW, "" );
    }

    destL1descInx = rsDataObjCreate( rsComm, destDataObjInp );
    if ( destL1descInx == CAT_UNKNOWN_COLLECTION ) {
        /* collection does not exist. make one */
        char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
        splitPathByKey( destDataObjInp->objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/' );
        rsMkCollR( rsComm, "/", parColl );
        destL1descInx = rsDataObjCreate( rsComm, destDataObjInp );
    }

    if ( destL1descInx < 0 ) {
        clearKeyVal( &destDataObjInp->condInput );
        std::stringstream msg;
        char* sys_error;
        char* rods_error = rodsErrorName( destL1descInx, &sys_error );
        msg << __FUNCTION__;
        msg << " - Failed to create destination object: \"";
        msg << destDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        irods::log( LOG_ERROR, msg.str() );
        return destL1descInx;
    }

    if ( L1desc[destL1descInx].replStatus == NEWLY_CREATED_COPY ) {
        existFlag = 0;
    }
    else {
        existFlag = 1;
    }

    L1desc[destL1descInx].oprType = COPY_DEST;

    L1desc[destL1descInx].srcL1descInx = srcL1descInx;

    rstrcpy( L1desc[destL1descInx].dataObjInfo->dataType,

             L1desc[srcL1descInx].dataObjInfo->dataType, NAME_LEN );
    /* set dataSize for verification in _rsDataObjClose */

    L1desc[destL1descInx].dataSize =
        L1desc[srcL1descInx].dataObjInfo->dataSize;

    status = _rsDataObjCopy( rsComm, destL1descInx, existFlag, transStat );

    clearKeyVal( &destDataObjInp->condInput );

    return status;
}

int
_rsDataObjCopy( rsComm_t *rsComm, int destL1descInx, int existFlag,
                transferStat_t **transStat ) {
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int srcL1descInx;
    int status = 0, status2;

    destDataObjInp  = L1desc[destL1descInx].dataObjInp;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;
    srcL1descInx    = L1desc[destL1descInx].srcL1descInx;

    srcDataObjInp  = L1desc[srcL1descInx].dataObjInp;
    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;

    if ( destDataObjInp == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: destDataObjInp is NULL" );
        return -1;
    }
    if ( destDataObjInfo == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: destDataObjInfo is NULL" );
        return -1;
    }
    if ( srcDataObjInp == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: srcDataObjInp is NULL" );
        return -1;
    }
    if ( srcDataObjInfo == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: srcDataObjInfo is NULL" );
        return -1;
    }

    if ( L1desc[srcL1descInx].l3descInx <= 2 ) {

        /* no physical file was opened */
        status = l3DataCopySingleBuf( rsComm, destL1descInx );

        /* has not been registered yet because of NO_OPEN_FLAG_KW */
        if ( status    >= 0                    &&
                existFlag == 0                    &&
                destDataObjInfo->specColl == NULL &&
                L1desc[destL1descInx].remoteZoneHost == NULL ) {
            /* If the dest is in remote zone, register in _rsDataObjClose there */
            status = svrRegDataObj( rsComm, destDataObjInfo );
            if ( status == CAT_UNKNOWN_COLLECTION ) {
                /* collection does not exist. make one */
                char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
                splitPathByKey( destDataObjInfo->objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/' );
                status = svrRegDataObj( rsComm, destDataObjInfo );
                rsMkCollR( rsComm, "/", parColl );
                status = svrRegDataObj( rsComm, destDataObjInfo );
            }
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "_rsDataObjCopy: svrRegDataObj for %s failed, status = %d",
                         destDataObjInfo->objPath, status );
                return status;
            }
        }

    }
    else {
        if ( srcDataObjInfo != NULL ) {
            destDataObjInp->numThreads = getNumThreads( rsComm, srcDataObjInfo->dataSize, destDataObjInp->numThreads, NULL,
                                         srcDataObjInfo->rescHier, destDataObjInfo->rescHier );//destRescName, srcRescName);

        }

        srcDataObjInp->numThreads = destDataObjInp->numThreads;

        status = dataObjCopy( rsComm, destL1descInx );
    }

    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = destL1descInx;
    if ( status >= 0 ) {
        *transStat = ( transferStat_t* )malloc( sizeof( transferStat_t ) );
        memset( *transStat, 0, sizeof( transferStat_t ) );
        ( *transStat )->bytesWritten = srcDataObjInfo->dataSize;
        ( *transStat )->numThreads = destDataObjInp->numThreads;
        dataObjCloseInp.bytesWritten = srcDataObjInfo->dataSize;
    }

    status2 = rsDataObjClose( rsComm, &dataObjCloseInp );

    if ( status ) {
        return status;
    }
    return status2;
}

