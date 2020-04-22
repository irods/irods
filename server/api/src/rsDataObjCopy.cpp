/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCopy.h for a description of this API call.*/

#include "dataObjCopy.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "dataObjOpen.h"
#include "dataObjClose.h"
#include "dataObjCreate.h"
#include "dataObjRepl.h"
#include "regDataObj.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "rsDataObjCopy.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjRepl.hpp"
#include "rsRegDataObj.hpp"
#include "rsDataObjClose.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"


// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"

namespace {

int close_objects(
    rsComm_t* comm,
    const int dest_l1_inx,
    const int opr_status,
    transferStat_t **transStat)
{
    openedDataObjInp_t close_inp{.l1descInx = dest_l1_inx};
    if (opr_status >= 0) {
        int src_l1_inx = L1desc[dest_l1_inx].srcL1descInx;
        dataObjInp_t* dest_data_obj_inp  = L1desc[dest_l1_inx].dataObjInp;
        dataObjInfo_t* src_data_obj_info = L1desc[src_l1_inx].dataObjInfo;
        if (dest_data_obj_inp && src_data_obj_info) {
            *transStat = ( transferStat_t* )malloc( sizeof( transferStat_t ) );
            memset( *transStat, 0, sizeof( transferStat_t ) );
            ( *transStat )->bytesWritten = src_data_obj_info->dataSize;
            ( *transStat )->numThreads = dest_data_obj_inp->numThreads;
            close_inp.bytesWritten = src_data_obj_info->dataSize;
        }
    }
    const auto close_status = rsDataObjClose(comm, &close_inp);
    if (close_status < 0) {
        rodsLog(LOG_ERROR, "[%s] - Failed closing data object:[%d]",
            __FUNCTION__, close_status);
    }
    return close_status;
} // close_objects

int _rsDataObjCopy(
    rsComm_t *rsComm,
    int destL1descInx,
    int existFlag)
{
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int srcL1descInx;
    int status = 0;

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
            status = svrRegDataObj(rsComm, destDataObjInfo);
            if (CAT_UNKNOWN_COLLECTION == status) {
                /* collection does not exist. make one */
                char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
                status = splitPathByKey(destDataObjInfo->objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/');
                if (status < 0) {
                    const auto err{ERROR(status,
                                         (boost::format("splitPathByKey failed for [%s]") %
                                          destDataObjInfo->objPath).str().c_str())};
                    irods::log(err);
                }
                status = rsMkCollR(rsComm, "/", parColl);
                if (status < 0) {
                    const auto err{ERROR(status,
                                         (boost::format("rsMkCollR for [%s] failed") %
                                          parColl).str().c_str())};
                    irods::log(err);
                }
                status = svrRegDataObj(rsComm, destDataObjInfo);
            }
            if (status < 0) {
                irods::log(LOG_NOTICE,
                           (boost::format("[%s] - svrRegDataObj for [%s] failed, status = [%d]") %
                            __FUNCTION__ % destDataObjInfo->objPath % status).str().c_str());
                return status;
            }
        }
    }
    else {
        destDataObjInp->numThreads = getNumThreads( rsComm, srcDataObjInfo->dataSize, destDataObjInp->numThreads, NULL,
                                     destDataObjInfo->rescHier, srcDataObjInfo->rescHier, 0 );
        srcDataObjInp->numThreads = destDataObjInp->numThreads;
        status = dataObjCopy( rsComm, destL1descInx );
    }

    return status;
}

} // anonymous namespace

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

    {
        namespace fs = irods::experimental::filesystem;

        srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
        try {
            if (! fs::server::is_data_object( *rsComm, srcDataObjInp->objPath )) {
                return USER_INPUT_PATH_ERR;
            }

            destDataObjInp = &dataObjCopyInp->destDataObjInp;
            if (fs::path{destDataObjInp->objPath}.is_relative()) {
                return USER_INPUT_PATH_ERR;
            }
        }
        catch (const fs::filesystem_error & err) {
            return err.code().value();
        }
    }

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
        char* sys_error = NULL;
        const char* rods_error = rodsErrorName( srcL1descInx, &sys_error );
        msg << __FUNCTION__;
        msg << " - Failed to open source object: \"";
        msg << srcDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        irods::log( LOG_ERROR, msg.str() );
        free( sys_error );
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

    destDataObjInp->oprType = COPY_DEST;
    destL1descInx = rsDataObjCreate( rsComm, destDataObjInp );
    if ( destL1descInx == CAT_UNKNOWN_COLLECTION ) {
        /* collection does not exist. make one */
        char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
        splitPathByKey( destDataObjInp->objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/' );
        rsMkCollR( rsComm, "/", parColl );
        destL1descInx = rsDataObjCreate( rsComm, destDataObjInp );
    }

    if ( destL1descInx < 0 ) {
        // Close source resource before returning - leaks L1 descriptors
        openedDataObjInp_t close_inp{.l1descInx = srcL1descInx};
        if(const auto close_status = rsDataObjClose(rsComm, &close_inp);
           close_status < 0) {
            rodsLog(LOG_ERROR, "[%s] - Failed closing source replica for [%s]:[%d]",
                __FUNCTION__, srcDataObjInp->objPath, close_status);
        }

        clearKeyVal( &destDataObjInp->condInput );
        std::stringstream msg;
        char* sys_error = NULL;
        const char* rods_error = rodsErrorName( destL1descInx, &sys_error );
        msg << __FUNCTION__;
        msg << " - Failed to create destination object: \"";
        msg << destDataObjInp->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        irods::log( LOG_ERROR, msg.str() );
        free( sys_error );
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

    status = _rsDataObjCopy(rsComm, destL1descInx, existFlag);

    const int close_status = close_objects(rsComm, destL1descInx, status, transStat);

    clearKeyVal( &destDataObjInp->condInput );
    if (status) {
        return status;
    }
    return close_status;
}

