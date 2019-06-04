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

#include "boost/format.hpp"

namespace {

int connect_to_remote_zone(
    rsComm_t *rsComm,
    dataObjCopyInp_t *dataObjCopyInp,
    rodsServerHost_t **rodsServerHost ) {

    dataObjInp_t* srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    rodsServerHost_t* srcIcatServerHost{};
    int status = getRcatHost( MASTER_RCAT, srcDataObjInp->objPath,
                          &srcIcatServerHost );

    if (status < 0 || !srcIcatServerHost) {
        rodsLog( LOG_ERROR,
                 "%s: getRcatHost error for %s",
                 __FUNCTION__, srcDataObjInp->objPath );
        return status;
    }
    if ( srcIcatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. nothing to do */
        return LOCAL_HOST;
    }

    dataObjInp_t* destDataObjInp = &dataObjCopyInp->destDataObjInp;
    rodsServerHost_t *destIcatServerHost{};
    status = getRcatHost( MASTER_RCAT, destDataObjInp->objPath,
                          &destIcatServerHost );

    if ( status < 0 || !destIcatServerHost ) {
        rodsLog( LOG_ERROR,
                 "%s: getRcatHost error for %s",
                 __FUNCTION__, destDataObjInp->objPath );
        return status;
    }

    if ( destIcatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. nothing to do */
        return LOCAL_HOST;
    }

    /* remote zone to different remote zone copy. Have to handle it
     * locally because of proxy admin user privilege issue */
    if ( srcIcatServerHost != destIcatServerHost ) {
        return LOCAL_HOST;
    }

    /* from the same remote zone. do it in the remote zone */
    status = getAndConnRemoteZone(rsComm, destDataObjInp, rodsServerHost, REMOTE_CREATE);
    return status;
} // connect_to_remote_zone

int open_source_data_obj(
    rsComm_t *rsComm,
    dataObjInp_t& _inp) {
    _inp.oprType = COPY_SRC;
    _inp.openFlags = O_RDONLY;
    const int srcL1descInx = rsDataObjOpen(rsComm, &_inp);
    if (srcL1descInx < 0) {
        char* sys_error{};
        const char* rods_error = rodsErrorName(srcL1descInx, &sys_error);
        const std::string error_msg = (boost::format(
            "%s -  - Failed to open source object: \"%s\" - %s %s") %
            __FUNCTION__ % _inp.objPath % rods_error % sys_error).str();
        free(sys_error);
        THROW(srcL1descInx, error_msg);
    }

    /* have to set L1desc[srcL1descInx].dataSize because open set this to -1 */
    L1desc[srcL1descInx].dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
    return srcL1descInx;
} // open_source_data_obj

void close_source_data_obj(
    rsComm_t *rsComm,
    const int _inx) {
    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = _inx;
    const int close_status = rsDataObjClose(rsComm, &dataObjCloseInp);
    if (close_status < 0) {
        rodsLog(LOG_NOTICE, "%s - failed closing [%s] with status [%d]",
                __FUNCTION__,
                __LINE__,
                L1desc[_inx].dataObjInp->objPath,
                close_status);
    }
} // close_source_data_obj

int open_destination_data_obj(
    rsComm_t *rsComm,
    dataObjInp_t& _inp) {
    _inp.oprType = COPY_DEST;
    _inp.openFlags = O_CREAT | O_RDWR;
    int destL1descInx = rsDataObjOpen(rsComm, &_inp);
    if ( destL1descInx == CAT_UNKNOWN_COLLECTION ) {
        /* collection does not exist. make one */
        char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
        splitPathByKey(_inp.objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/');
        rsMkCollR( rsComm, "/", parColl );
        destL1descInx = rsDataObjOpen(rsComm, &_inp);
    }

    if (destL1descInx < 0) {
        clearKeyVal( &_inp.condInput );
        char* sys_error = NULL;
        const char* rods_error = rodsErrorName( destL1descInx, &sys_error );
        const std::string error_msg = (boost::format(
            "%s -  - Failed to create destination object: \"%s\" - %s %s") %
            __FUNCTION__ % _inp.objPath % rods_error % sys_error).str();
        free(sys_error);
        THROW(destL1descInx, error_msg);
    }

    L1desc[destL1descInx].oprType = COPY_DEST;
    return destL1descInx;
} // open_destination_data_obj

void close_destination_data_obj(
    rsComm_t *rsComm,
    const int _inx,
    transferStat_t **transStat)
{
    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = _inx;

    *transStat = (transferStat_t*)malloc(sizeof(transferStat_t));
    memset(*transStat, 0, sizeof(transferStat_t));
    const int srcL1descInx = L1desc[_inx].srcL1descInx;
    (*transStat)->bytesWritten = L1desc[srcL1descInx].dataObjInfo->dataSize;
    (*transStat)->numThreads = L1desc[_inx].dataObjInp->numThreads;
    dataObjCloseInp.bytesWritten = L1desc[srcL1descInx].dataObjInfo->dataSize;
    rodsLog(LOG_NOTICE, "[%s:%d] - closing [%s]", __FUNCTION__, __LINE__, L1desc[_inx].dataObjInp->objPath);
    const int close_status = rsDataObjClose(rsComm, &dataObjCloseInp);
    if (close_status < 0) {
        rodsLog(LOG_NOTICE, "%s - failed closing [%s] with status [%d]",
                __FUNCTION__,
                __LINE__,
                L1desc[_inx].dataObjInp->objPath,
                close_status);
    }
} // close_destination_data_obj

} // anonymous namespace

int rsDataObjCopy(
    rsComm_t *rsComm,
    dataObjCopyInp_t *dataObjCopyInp,
    transferStat_t **transStat )
{
    namespace fs = irods::experimental::filesystem;

    if (!dataObjCopyInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    dataObjInp_t* srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    dataObjInp_t* destDataObjInp = &dataObjCopyInp->destDataObjInp;
    try {
        if (!fs::server::is_data_object(*rsComm, srcDataObjInp->objPath) ||
            fs::path{destDataObjInp->objPath}.is_relative()) {
            return USER_INPUT_PATH_ERR;
        }
    }
    catch (const fs::filesystem_error& err) {
        return err.code().value();
    }

    specCollCache_t *specCollCache{};
    resolveLinkedPath(rsComm, srcDataObjInp->objPath, &specCollCache, &srcDataObjInp->condInput);
    resolveLinkedPath(rsComm, destDataObjInp->objPath, &specCollCache, &destDataObjInp->condInput);

    rodsServerHost_t *rodsServerHost;
    int remoteFlag = connect_to_remote_zone( rsComm, dataObjCopyInp, &rodsServerHost );
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        return _rcDataObjCopy(rodsServerHost->conn, dataObjCopyInp, transStat);
    }

    if (strcmp(srcDataObjInp->objPath, destDataObjInp->objPath) == 0) {
        rodsLog(LOG_ERROR,
                "%s: same src and dest objPath [%s] not allowed",
                __FUNCTION__, srcDataObjInp->objPath );
        return USER_INPUT_PATH_ERR;
    }

    try {
        int srcL1descInx{};
        int destL1descInx{};
        const irods::at_scope_exit close_objects{[&]() {
            if (destL1descInx > 3) {
                close_destination_data_obj(rsComm, destL1descInx, transStat);
            }
            if (srcL1descInx > 3) {
                close_source_data_obj(rsComm, srcL1descInx);
            }
        }};

        srcL1descInx = open_source_data_obj(rsComm, *srcDataObjInp);

        const int createMode = std::atoi(L1desc[srcL1descInx].dataObjInfo->dataMode);
        if (createMode >= 0100) {
            destDataObjInp->createMode = createMode;
        }
        destL1descInx = open_destination_data_obj(rsComm, *destDataObjInp);

        L1desc[destL1descInx].srcL1descInx = srcL1descInx;
        L1desc[destL1descInx].dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        rstrcpy(L1desc[destL1descInx].dataObjInfo->dataType, L1desc[srcL1descInx].dataObjInfo->dataType, NAME_LEN);

        const int thread_count = getNumThreads(
            rsComm,
            L1desc[srcL1descInx].dataObjInfo->dataSize,
            L1desc[destL1descInx].dataObjInp->numThreads,
            NULL,
            L1desc[destL1descInx].dataObjInfo->rescHier,
            L1desc[srcL1descInx].dataObjInfo->rescHier,
            0);
        L1desc[srcL1descInx].dataObjInp->numThreads = thread_count;

        const int status = dataObjCopy( rsComm, destL1descInx );
        if (status < 0) {
            L1desc[destL1descInx].oprStatus = status;
        }
        return status;
    }
    catch (const irods::exception& e) {
        irods::log(e);
        return e.code();
    }
    return 0;
} // rsDataObjCopy
