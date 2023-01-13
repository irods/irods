#include "irods/collection.hpp"
#include "irods/dataObjClose.h"
#include "irods/dataObjCopy.h"
#include "irods/dataObjCreate.h"
#include "irods/dataObjOpen.h"
#include "irods/dataObjRepl.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/irods_logger.hpp"
#include "irods/objMetaOpr.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/regDataObj.h"
#include "irods/rodsLog.h"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsDataObjCopy.hpp"
#include "irods/rsDataObjCreate.hpp"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjRepl.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/scoped_privileged_client.hpp"
#include "irods/server_utilities.hpp"
#include "irods/specColl.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include "irods/key_value_proxy.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_redirect.hpp"

#include <boost/format.hpp>

#include <chrono>

namespace ix = irods::experimental;

namespace {

    int connect_to_remote_zone(
        rsComm_t *rsComm,
        dataObjCopyInp_t *dataObjCopyInp,
        rodsServerHost_t **rodsServerHost ) {

        dataObjInp_t* srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
        rodsServerHost_t* srcIcatServerHost{};
        int status = getRcatHost(PRIMARY_RCAT, srcDataObjInp->objPath, &srcIcatServerHost);

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
        status = getRcatHost(PRIMARY_RCAT, destDataObjInp->objPath, &destIcatServerHost);

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

    int open_source_data_obj(rsComm_t *rsComm, dataObjInp_t& inp)
    {
        inp.oprType = COPY_SRC;
        inp.openFlags = O_RDONLY;
        const int srcL1descInx = rsDataObjOpen(rsComm, &inp);
        if (srcL1descInx < 0) {
            char* sys_error{};
            const char* rods_error = rodsErrorName(srcL1descInx, &sys_error);

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to open source object "
                "[irods error=[{}], system error=[{}], path=[{}]]",
                __FUNCTION__, __LINE__, rods_error, sys_error, inp.objPath));

            free(sys_error);

            return srcL1descInx;
        }

        /* have to set L1desc[srcL1descInx].dataSize because open set this to -1 */
        L1desc[srcL1descInx].dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        return srcL1descInx;
    } // open_source_data_obj

    auto close_source_data_obj(RsComm* rsComm, const int _inx) -> int
    {
        openedDataObjInp_t dataObjCloseInp{};
        dataObjCloseInp.l1descInx = _inx;

        rodsLog(LOG_DEBUG8, "[%s:%d] - closing [%s]", __FUNCTION__, __LINE__, L1desc[_inx].dataObjInp->objPath);

        return rsDataObjClose(rsComm, &dataObjCloseInp);
    } // close_source_data_obj

    int open_destination_data_obj(rsComm_t *rsComm, dataObjInp_t& inp)
    {
        inp.oprType = COPY_DEST;
        inp.openFlags = O_CREAT | O_WRONLY | O_TRUNC;

        int destL1descInx = rsDataObjOpen(rsComm, &inp);
        if ( destL1descInx == CAT_UNKNOWN_COLLECTION ) {
            /* collection does not exist. make one */
            char parColl[MAX_NAME_LEN], child[MAX_NAME_LEN];
            splitPathByKey(inp.objPath, parColl, MAX_NAME_LEN, child, MAX_NAME_LEN, '/');
            rsMkCollR( rsComm, "/", parColl );
            destL1descInx = rsDataObjOpen(rsComm, &inp);
        }

        if (destL1descInx < 0) {
            clearKeyVal( &inp.condInput );

            char* sys_error{};
            const char* rods_error = rodsErrorName(destL1descInx, &sys_error);

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to open destination object "
                "[irods error=[{}], system error=[{}], path=[{}]]",
                __FUNCTION__, __LINE__, rods_error, sys_error, inp.objPath));

            free(sys_error);

            return destL1descInx;
        }

        L1desc[destL1descInx].oprType = COPY_DEST;
        return destL1descInx;
    } // open_destination_data_obj

    auto close_destination_data_obj(RsComm* rsComm, const int _inx) -> int
    {
        openedDataObjInp_t dataObjCloseInp{};
        dataObjCloseInp.l1descInx = _inx;
        dataObjCloseInp.bytesWritten = L1desc[L1desc[_inx].srcL1descInx].dataObjInfo->dataSize;

        rodsLog(LOG_DEBUG8, "[%s:%d] - closing [%s]", __FUNCTION__, __LINE__, L1desc[_inx].dataObjInp->objPath);

        return rsDataObjClose(rsComm, &dataObjCloseInp);
    } // close_destination_data_obj

    int rsDataObjCopy_impl(
        rsComm_t *rsComm,
        dataObjCopyInp_t *dataObjCopyInp,
        transferStat_t **transStat)
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

            if (irods::is_force_flag_required(*rsComm, *destDataObjInp)) {
                return OVERWRITE_WITHOUT_FORCE_FLAG;
            }
        }
        catch (const fs::filesystem_error& e) {
            irods::experimental::log::api::error(e.what());
            return e.code().value();
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

        int srcL1descInx{};
        int destL1descInx{};
        const auto close_objects{[&]() -> int {
            int result = 0;

            if (destL1descInx > 3) {
                // The transferStat_t communicates information back to the client regarding
                // the data transfer such as bytes written and how many threads were used.
                // These must be saved before the L1 descriptor is free'd.
                *transStat = (transferStat_t*)malloc(sizeof(transferStat_t));
                memset(*transStat, 0, sizeof(transferStat_t));
                (*transStat)->bytesWritten = L1desc[srcL1descInx].dataObjInfo->dataSize;
                (*transStat)->numThreads = L1desc[destL1descInx].dataObjInp->numThreads;

                if (const int ec = close_destination_data_obj(rsComm, destL1descInx); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed closing [{}] with status [{}]",
                        __FUNCTION__, __LINE__, destDataObjInp->objPath, ec));

                    result = ec;
                }
            }

            if (srcL1descInx > 3) {
                if (const int ec = close_source_data_obj(rsComm, srcL1descInx); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed closing [{}] with status [{}]",
                        __FUNCTION__, __LINE__, srcDataObjInp->objPath, ec));

                    if (!result) {
                        result = ec;
                    }
                }
            }

            return result;
        }};

        srcL1descInx = open_source_data_obj(rsComm, *srcDataObjInp);
        if (srcL1descInx < 0) {
            return srcL1descInx;
        }

        const int createMode = std::atoi(L1desc[srcL1descInx].dataObjInfo->dataMode);
        if (createMode >= 0100) {
            destDataObjInp->createMode = createMode;
        }

        destL1descInx = open_destination_data_obj(rsComm, *destDataObjInp);
        if (destL1descInx < 0) {
            close_objects();

            return destL1descInx;
        }

        L1desc[destL1descInx].srcL1descInx = srcL1descInx;
        L1desc[destL1descInx].dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        rstrcpy(L1desc[destL1descInx].dataObjInfo->dataType, L1desc[srcL1descInx].dataObjInfo->dataType, NAME_LEN);

        // The input dataSize is used by legacy parallel transfer operations to determine
        // the number of threads. Set this to the size of the source object so that the transfer
        // can be set up successfully.
        L1desc[destL1descInx].dataObjInp->dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;

        const int thread_count = getNumThreads(
            rsComm,
            L1desc[srcL1descInx].dataObjInfo->dataSize,
            L1desc[destL1descInx].dataObjInp->numThreads,
            NULL,
            L1desc[destL1descInx].dataObjInfo->rescHier,
            L1desc[srcL1descInx].dataObjInfo->rescHier,
            0);
        L1desc[destL1descInx].dataObjInp->numThreads = thread_count;
        L1desc[srcL1descInx].dataObjInp->numThreads = thread_count;

        const int copy_ec = dataObjCopy( rsComm, destL1descInx );
        if (copy_ec < 0) {
            L1desc[destL1descInx].oprStatus = copy_ec;
        }

        const auto close_ec = close_objects();

        return copy_ec < 0 ? copy_ec : close_ec;
    } // rsDataObjCopy_impl

} // anonymous namespace

int rsDataObjCopy(rsComm_t* rsComm,
                  dataObjCopyInp_t* dataObjCopyInp,
                  transferStat_t** transStat)
{
    // Do not update the collection mtime here because the open API call for the destination data object does it.
    return rsDataObjCopy_impl(rsComm, dataObjCopyInp, transStat);
}

