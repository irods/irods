#include "irods/rsDataObjCopy.hpp"

#include "irods/collection.hpp"
#include "irods/dataObjClose.h"
#include "irods/dataObjCopy.h"
#include "irods/dataObjCreate.h"
#include "irods/dataObjOpen.h"
#include "irods/dataObjRepl.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/irods_logger.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/objMetaOpr.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/regDataObj.h"
#include "irods/rodsLog.h"
#include "irods/rsDataObjClose.hpp"
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

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/data_object_proxy.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_redirect.hpp"

#include <boost/format.hpp>

#include <chrono>

namespace
{
    namespace fs = irods::experimental::filesystem;

    using log_api = irods::experimental::log::api;

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

    auto close_source_data_obj(RsComm* _comm, const int _l1_index, const DataObjInp& _inp) -> int
    {
        // The L1 descriptor may not be available later, so we need to use the path from the DataObjInp.
        const std::string_view path = static_cast<const char*>(_inp.objPath);

        log_api::trace("[{}:{}] - closing [{}]", __func__, __LINE__, path);

        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = _l1_index;

        const int ec = rsDataObjClose(_comm, &close_inp);

        if (ec < 0) {
            log_api::error("[{}:{}] - failed closing [{}] with status [{}]", __func__, __LINE__, path, ec);
        }

        return ec;
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

    auto close_destination_data_obj(RsComm* _comm, const int _l1_index, const DataObjInp& _inp) -> int
    {
        // The L1 descriptor may not be available at this point, so we need to use the path from the DataObjInp.
        const std::string_view path = static_cast<const char*>(_inp.objPath);

        log_api::trace("[{}:{}] - closing [{}]", __func__, __LINE__, path);

        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = _l1_index;

        // Declare this here because we need to return an error if the source index is invalid.
        int ec = 0;

        // The L1 descriptor index is const and bounds-checked before this function is called. Therefore, ignore linter.
        //
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto& l1_desc = L1desc[_l1_index];

        const int source_l1_index = l1_desc.srcL1descInx;
        if (source_l1_index < 3 || source_l1_index >= NUM_L1_DESC) {
            log_api::error("[{}:{}] - Source L1 descriptor for [{}] is out of range: [{}]",
                           __func__,
                           __LINE__,
                           path,
                           source_l1_index);

            // Set the error code, but continue to the close call so the destination data object can be finalized. The
            // L1 descriptor oprType is also set so that rsDataObjClose knows that the operation encountered an issue.
            // The data object should be marked stale because the source L1 descriptor has been corrupted and cannot be
            // used for data verification in the finalization process of the new data object.
            ec = SYS_FILE_DESC_OUT_OF_RANGE;
            l1_desc.oprStatus = ec;
        }
        else {
            // The L1 descriptor index is const and confirmed to be in-bounds above. Therefore, ignore linter.
            //
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            close_inp.bytesWritten = L1desc[source_l1_index].dataObjInfo->dataSize;
        }

        if (const int close_ec = rsDataObjClose(_comm, &close_inp); close_ec < 0) {
            ec = close_ec;
        }

        if (ec < 0) {
            log_api::error("[{}:{}] - failed closing [{}] with status [{}]", __func__, __LINE__, path, ec);
        }

        return ec;
    } // close_destination_data_obj

    int rsDataObjCopy_impl(
        rsComm_t *rsComm,
        dataObjCopyInp_t *dataObjCopyInp,
        transferStat_t **transStat)
    {
        dataObjInp_t* srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
        dataObjInp_t* destDataObjInp = &dataObjCopyInp->destDataObjInp;
        try {
            const auto source_exists_and_is_data_object = fs::server::is_data_object(*rsComm, srcDataObjInp->objPath);
            if (!source_exists_and_is_data_object || fs::path{destDataObjInp->objPath}.is_relative()) {
                return USER_INPUT_PATH_ERR;
            }

            // These checks should only be done when the destination data object exists. If not, it does not matter if
            // the force flag was provided or not, or which resource is being targeted for the copy.
            const auto destination_exists_and_is_data_object =
                fs::server::is_data_object(*rsComm, destDataObjInp->objPath);
            if (destination_exists_and_is_data_object) {
                // If destination object exists and the force flag is not being used, an overwrite without the force
                // flag is being attempted. Return an error in this case.
                if (!getValByKey(&destDataObjInp->condInput, FORCE_FLAG_KW)) {
                    log_api::error("[{}]: Overwrite of [{}] requires use of the force flag keyword [{}].",
                                   __func__,
                                   destDataObjInp->objPath,
                                   FORCE_FLAG_KW);
                    return OVERWRITE_WITHOUT_FORCE_FLAG;
                }

                if (const auto* destination_resource = getValByKey(&destDataObjInp->condInput, DEST_RESC_NAME_KW);
                    destination_resource) {
                    // Construct a query to determine whether the data object has any replicas in a resource hierarchy
                    // that starts with the target resource.
                    irods::experimental::query_builder qb;
                    if (const auto zone = fs::zone_name(destDataObjInp->objPath); zone) {
                        qb.zone_hint(*zone);
                    }
                    const auto path = fs::path{destDataObjInp->objPath};
                    const auto qstr = fmt::format("select DATA_ID where COLL_NAME = '{0}' and DATA_NAME = '{1}' and "
                                                  "DATA_RESC_HIER like '{2};%' || = '{2}'",
                                                  path.parent_path().c_str(),
                                                  path.object_name().c_str(),
                                                  destination_resource);

                    // If no results come back from the query, that means no replica exists on the target resource.
                    // Creating new replicas on existing data objects with copy is not allowed, so we throw an error
                    // indicating that an error occurred while resolving the hierarchy.
                    if (auto q = qb.build(*rsComm, qstr); q.empty()) {
                        log_api::error(fmt::format(
                            "[{}]: Cannot overwrite [{}] on resource [{}] because resource does not hold a replica.",
                            __func__,
                            destDataObjInp->objPath,
                            destination_resource));
                        return HIERARCHY_ERROR;
                    }
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            log_api::error(e.what());
            return e.code().value();
        }
        catch (const irods::exception& e) {
            log_api::error(e.client_display_what());
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            return e.code();
        }

        specCollCache_t *specCollCache{};
        resolveLinkedPath(rsComm, srcDataObjInp->objPath, &specCollCache, &srcDataObjInp->condInput);
        resolveLinkedPath(rsComm, destDataObjInp->objPath, &specCollCache, &destDataObjInp->condInput);

        rodsServerHost_t* rodsServerHost = nullptr;
        int remoteFlag = connect_to_remote_zone( rsComm, dataObjCopyInp, &rodsServerHost );
        if (remoteFlag < 0) {
            return remoteFlag;
        }
        else if ( remoteFlag == REMOTE_HOST ) {
            // It is not possible to reach this case with rodsServerHost being nullptr. However, in the event that it
            // ever does occur for some reason, we should return an error.
            if (!rodsServerHost) {
                log_api::error("[{}]: rodsServerHost is nullptr despite remoteFlag being valid.", __func__);
                return SYS_INTERNAL_ERR;
            }

            return _rcDataObjCopy(rodsServerHost->conn, dataObjCopyInp, transStat);
        }

        if (strcmp(srcDataObjInp->objPath, destDataObjInp->objPath) == 0) {
            rodsLog(LOG_ERROR,
                    "%s: same src and dest objPath [%s] not allowed",
                    __FUNCTION__, srcDataObjInp->objPath );
            return USER_INPUT_PATH_ERR;
        }

        const int srcL1descInx = open_source_data_obj(rsComm, *srcDataObjInp);
        if (srcL1descInx < 3 || srcL1descInx >= NUM_L1_DESC) {
            return srcL1descInx < 0 ? srcL1descInx : SYS_FILE_DESC_OUT_OF_RANGE;
        }

        const int createMode = std::atoi(L1desc[srcL1descInx].dataObjInfo->dataMode);
        if (createMode >= 0100) {
            destDataObjInp->createMode = createMode;
        }

        const int destL1descInx = open_destination_data_obj(rsComm, *destDataObjInp);
        if (destL1descInx < 3 || destL1descInx >= NUM_L1_DESC) {
            std::ignore = close_source_data_obj(rsComm, srcL1descInx, *srcDataObjInp);

            return destL1descInx < 0 ? destL1descInx : SYS_FILE_DESC_OUT_OF_RANGE;
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

        if (transStat) {
            // The transferStat_t communicates information back to the client regarding
            // the data transfer such as bytes written and how many threads were used.
            // These must be saved before the L1 descriptor is free'd.
            //
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            *transStat = static_cast<transferStat_t*>(std::malloc(sizeof(transferStat_t)));
            std::memset(*transStat, 0, sizeof(transferStat_t));

            // The L1 descriptor indexes are const and confirmed to be in-bounds above. Therefore, ignore linter.
            //
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            (*transStat)->bytesWritten = L1desc[srcL1descInx].dataObjInfo->dataSize;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            (*transStat)->numThreads = L1desc[destL1descInx].dataObjInp->numThreads;
        }

        const auto close_dest_ec = close_destination_data_obj(rsComm, destL1descInx, *destDataObjInp);
        const auto close_source_ec = close_source_data_obj(rsComm, srcL1descInx, *srcDataObjInp);
        const auto close_ec = 0 == close_dest_ec ? close_source_ec : close_dest_ec;

        return copy_ec < 0 ? copy_ec : close_ec;
    } // rsDataObjCopy_impl

} // anonymous namespace

int rsDataObjCopy(rsComm_t* rsComm,
                  dataObjCopyInp_t* dataObjCopyInp,
                  transferStat_t** transStat)
{
    if (!rsComm || !dataObjCopyInp) {
        return USER__NULL_INPUT_ERR;
    }

    // Do not update the collection mtime here because the open API call for the destination data object does it.
    return rsDataObjCopy_impl(rsComm, dataObjCopyInp, transStat);
}

