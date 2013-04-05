/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCopy.h for a description of this API call.*/

#include "dataObjCopy.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "collection.h"
#include "specColl.h"
#include "dataObjOpen.h"
#include "dataObjCreate.h"
#include "dataObjRepl.h"
#include "regDataObj.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_redirect.h"

int
rsDataObjCopy250 (rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp,
                  transStat_t **transStat)
{
    int status;
    transferStat_t *transferStat = NULL;

    status = rsDataObjCopy (rsComm, dataObjCopyInp, &transferStat);

    if (transStat != NULL && status >= 0 && transferStat != NULL) {
        *transStat = (transStat_t *) malloc (sizeof (transStat_t));
        (*transStat)->numThreads = transferStat->numThreads;
        (*transStat)->bytesWritten = transferStat->bytesWritten;
        free (transferStat);
    }
    return status;
}

int
rsDataObjCopy (rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp,
               transferStat_t **transStat)
{
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

    resolveLinkedPath (rsComm, srcDataObjInp->objPath, &specCollCache, &srcDataObjInp->condInput);
    resolveLinkedPath (rsComm, destDataObjInp->objPath, &specCollCache, &destDataObjInp->condInput);

    remoteFlag = getAndConnRemoteZoneForCopy (rsComm, dataObjCopyInp, &rodsServerHost);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
        status = _rcDataObjCopy (rodsServerHost->conn, dataObjCopyInp,
                                 transStat);
        return status;
    }

    // =-=-=-=-=-=-=-
    // pre-determine hier strings for the source 
    std::string hier;
    eirods::error ret = eirods::resolve_resource_hierarchy( eirods::EIRODS_OPEN_OPERATION, rsComm, 
                                                   srcDataObjInp, hier );
    if( !ret.ok() ) { 
        std::stringstream msg;
        msg << "rsDataObjCopy :: failed in eirods::resolve_resource_hierarchy for [";
        msg << srcDataObjInp->objPath << "]";
        eirods::log( PASSMSG( msg.str(), ret ) );
        return ret.code();
    }
   

    if(true) {
        std::stringstream msg;
        msg << "qqq - Source resource hierarchy: \"";
        msg << hier;
        msg << "\"";
        DEBUGMSG(msg.str());
    }

    // =-=-=-=-=-=-=-
    // we resolved the hier str for subsequent api calls, etc.
    addKeyVal( &srcDataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    // =-=-=-=-=-=-=-
    // determine the hier string for the dest data obj inp
    ret = eirods::resolve_resource_hierarchy( eirods::EIRODS_CREATE_OPERATION, rsComm, 
                                     destDataObjInp, hier );
    if( !ret.ok() ) { 
        std::stringstream msg;
        msg << "rsDataObjCopy :: failed in eirods::resolve_resource_hierarchy for [";
        msg << destDataObjInp->objPath << "]";
        eirods::log( PASSMSG( msg.str(), ret ) );
        return ret.code();
    }
   

    if(true) {
        std::stringstream msg;
        msg << "qqq - Destination resource hierarchy: \"";
        msg << hier;
        msg << "\"";
        DEBUGMSG(msg.str());
    }

    // =-=-=-=-=-=-=-
    // we resolved the hier str for subsequent api calls, etc.
    addKeyVal( &destDataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

#if 0
    *transStat = malloc (sizeof (transferStat_t));
    memset (*transStat, 0, sizeof (transferStat_t));
#endif

    if (strcmp (srcDataObjInp->objPath, destDataObjInp->objPath) == 0) {
        rodsLog (LOG_ERROR,
                 "rsDataObjCopy: same src and dest objPath %s not allowed",
                 srcDataObjInp->objPath);
        return (USER_INPUT_PATH_ERR);
    }
        
    addKeyVal (&srcDataObjInp->condInput, PHYOPEN_BY_SIZE_KW, "");

    srcL1descInx = rsDataObjOpen (rsComm, srcDataObjInp);

    if (srcL1descInx < 0) {
        std::stringstream msg;
        char* sys_error;
        char* rods_error = rodsErrorName(srcL1descInx, &sys_error);
        msg << __FUNCTION__;
        msg << " - Failed to open source object: \"";
        msg << srcDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        eirods::log(LOG_ERROR, msg.str());
        return srcL1descInx;
    }

    /* have to set L1desc[srcL1descInx].dataSize because open set this to -1 */
    destDataObjInp->dataSize = L1desc[srcL1descInx].dataSize =
        L1desc[srcL1descInx].dataObjInfo->dataSize;

    createMode = atoi (L1desc[srcL1descInx].dataObjInfo->dataMode);
    
    if (createMode >= 0100)
        destDataObjInp->createMode = createMode; 

    L1desc[srcL1descInx].oprType = COPY_SRC;

    if (L1desc[srcL1descInx].l3descInx <= 2) {
        /* dataSingleBuf */
        addKeyVal (&destDataObjInp->condInput, NO_OPEN_FLAG_KW, "");
    }

    destL1descInx = rsDataObjCreate (rsComm, destDataObjInp);
    if (destL1descInx == CAT_UNKNOWN_COLLECTION) {
        /* collection does not exist. make one */
        char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
        splitPathByKey (destDataObjInp->objPath, parColl, child, '/');
        rsMkCollR (rsComm, "/", parColl);
        destL1descInx = rsDataObjCreate (rsComm, destDataObjInp);
    }

    if (destL1descInx < 0) {
        std::stringstream msg;
        char* sys_error;
        char* rods_error = rodsErrorName(destL1descInx, &sys_error);
        msg << __FUNCTION__;
        msg << " - Failed to create destination object: \"";
        msg << destDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        eirods::log(LOG_ERROR, msg.str());
        return (destL1descInx);
    }

    if (L1desc[destL1descInx].replStatus == NEWLY_CREATED_COPY) {
        existFlag = 0;
    } else {
        existFlag = 1;
    }
        
    L1desc[destL1descInx].oprType = COPY_DEST;

    L1desc[destL1descInx].srcL1descInx = srcL1descInx;

    rstrcpy (L1desc[destL1descInx].dataObjInfo->dataType, 

             L1desc[srcL1descInx].dataObjInfo->dataType, NAME_LEN);
    /* set dataSize for verification in _rsDataObjClose */

    L1desc[destL1descInx].dataSize = 
        L1desc[srcL1descInx].dataObjInfo->dataSize;

#if 0
    (*transStat)->bytesWritten = L1desc[srcL1descInx].dataObjInfo->dataSize;
#endif
    status = _rsDataObjCopy (rsComm, destL1descInx, existFlag, transStat);

#if 0
    if (status >= 0) {
        (*transStat)->numThreads = destDataObjInp->numThreads;
    }
#endif

    return (status);
}

int
_rsDataObjCopy (rsComm_t *rsComm, int destL1descInx, int existFlag,
                transferStat_t **transStat)
{
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int srcL1descInx;
    int status = 0, status2;
    char *destRescName, *srcRescName;

    destDataObjInp  = L1desc[destL1descInx].dataObjInp;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;
    srcL1descInx    = L1desc[destL1descInx].srcL1descInx;

    srcDataObjInp  = L1desc[srcL1descInx].dataObjInp;
    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;

    if( destDataObjInp == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: destDataObjInp is NULL" );
        return -1;
    }
    if( destDataObjInfo == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: destDataObjInfo is NULL" );
        return -1;
    }
    if( srcDataObjInp == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: srcDataObjInp is NULL" );
        return -1;
    }
    if( srcDataObjInfo == NULL ) { // JMC cppcheck - null ptr ref
        rodsLog( LOG_ERROR, "_rsDataObjCopy: :: srcDataObjInfo is NULL" );
        return -1;
    }

    if (L1desc[srcL1descInx].l3descInx <= 2) {

        /* no physical file was opened */
        status = l3DataCopySingleBuf (rsComm, destL1descInx);

        /* has not been registered yet because of NO_OPEN_FLAG_KW */
        if( status    >= 0                    && 
            existFlag == 0                    && 
            destDataObjInfo->specColl == NULL &&
            L1desc[destL1descInx].remoteZoneHost == NULL) {
            /* If the dest is in remote zone, register in _rsDataObjClose there */
            status = svrRegDataObj (rsComm, destDataObjInfo);
            if (status == CAT_UNKNOWN_COLLECTION) {
                /* collection does not exist. make one */
                char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
                splitPathByKey (destDataObjInfo->objPath, parColl, child, '/');
                status = svrRegDataObj (rsComm, destDataObjInfo);
                rsMkCollR (rsComm, "/", parColl);
                status = svrRegDataObj (rsComm, destDataObjInfo);
            }
            if (status < 0) {    
                rodsLog (LOG_NOTICE,
                         "_rsDataObjCopy: svrRegDataObj for %s failed, status = %d",
                         destDataObjInfo->objPath, status);
                return (status);
            }
        }

    } else {
        if (destDataObjInfo != NULL && destDataObjInfo->rescInfo != NULL)
            destRescName = destDataObjInfo->rescInfo->rescName;
        else
            destRescName = NULL;

        if (srcDataObjInfo != NULL && srcDataObjInfo->rescInfo != NULL)
            srcRescName = srcDataObjInfo->rescInfo->rescName;
        else
            srcRescName = NULL;

        if( srcDataObjInfo != NULL ) {
            destDataObjInp->numThreads = getNumThreads( rsComm, srcDataObjInfo->dataSize, destDataObjInp->numThreads, NULL,
                                                        srcDataObjInfo->rescHier, destDataObjInfo->rescHier );//destRescName, srcRescName);

        }
                
        srcDataObjInp->numThreads = destDataObjInp->numThreads;
#if 0
        /* XXXX can't handle numThreads == 0 && size > MAX_SZ_FOR_SINGLE_BUF */
        if (destDataObjInp->numThreads == 0 && 
            srcDataObjInfo->dataSize > MAX_SZ_FOR_SINGLE_BUF) {
            destDataObjInp->numThreads = 1;
        }
#endif

        status = dataObjCopy (rsComm, destL1descInx);
    }

    memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = destL1descInx;
    if (status >= 0) {
        *transStat = (transferStat_t*)malloc (sizeof (transferStat_t));
        memset (*transStat, 0, sizeof (transferStat_t));
        (*transStat)->bytesWritten = srcDataObjInfo->dataSize;
        (*transStat)->numThreads = destDataObjInp->numThreads;
        dataObjCloseInp.bytesWritten = srcDataObjInfo->dataSize;
    }

    status2 = rsDataObjClose (rsComm, &dataObjCloseInp);

    if (status) return (status);
    return(status2);
}

