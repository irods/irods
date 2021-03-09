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
#include "server_utilities.hpp"
#include "irods_logger.hpp"
#include "scoped_privileged_client.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"

#include "boost/format.hpp"

#include <chrono>

namespace ix = irods::experimental;

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
    dataObjInp_t& inp) {
    inp.oprType = COPY_SRC;
    inp.openFlags = O_RDONLY;
    const int srcL1descInx = rsDataObjOpen(rsComm, &inp);
    if (srcL1descInx < 0) {
        char* sys_error{};
        const char* rods_error = rodsErrorName(srcL1descInx, &sys_error);
        const std::string error_msg = (boost::format(
            "%s -  - Failed to open source object: \"%s\" - %s %s") %
            __FUNCTION__ % inp.objPath % rods_error % sys_error).str();
        free(sys_error);
        THROW(srcL1descInx, error_msg);
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

int open_destination_data_obj(
    rsComm_t *rsComm,
    dataObjInp_t& inp)
{
    inp.oprType = COPY_DEST;
    inp.openFlags = O_CREAT | O_WRONLY | O_TRUNC;

    irods::file_object_ptr file_obj(new irods::file_object());
    std::string hier{};
    const char* h{getValByKey(&inp.condInput, RESC_HIER_STR_KW)};
    if (!h) {
        std::tie(file_obj, hier) = irods::resolve_resource_hierarchy(irods::CREATE_OPERATION, rsComm, inp);
        addKeyVal(&inp.condInput, RESC_HIER_STR_KW, hier.c_str());
    }
    else {
        hier = h;
        irods::file_object_ptr obj(new irods::file_object());
        dataObjInfo_t* dataObjInfoHead{};
        irods::error fac_err = irods::file_object_factory(rsComm, &inp, obj, &dataObjInfoHead);
        if (!fac_err.ok()) {
            irods::log(fac_err);
        }
        file_obj.swap(obj);
    }

    const auto hier_has_replica{[&hier, &replicas = file_obj->replicas()]() {
        return std::any_of(replicas.begin(), replicas.end(),
            [&](const irods::physical_object& replica) {
                return replica.resc_hier() == hier;
            });
        }()};

    if (hier_has_replica && !getValByKey(&inp.condInput, FORCE_FLAG_KW)) {
        THROW(OVERWRITE_WITHOUT_FORCE_FLAG, "force flag required to overwrite data object for copy");
    }

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
        char* sys_error = NULL;
        const char* rods_error = rodsErrorName( destL1descInx, &sys_error );
        const std::string error_msg = (boost::format(
            "%s -  - Failed to create destination object: \"%s\" - %s %s") %
            __FUNCTION__ % inp.objPath % rods_error % sys_error).str();
        free(sys_error);
        THROW(destL1descInx, error_msg);
    }

    L1desc[destL1descInx].oprType = COPY_DEST;
    return destL1descInx;
} // open_destination_data_obj

    auto close_destination_data_obj(RsComm* rsComm, const int _inx) -> int
    {
        openedDataObjInp_t dataObjCloseInp{};
        dataObjCloseInp.l1descInx = _inx;
        dataObjCloseInp.bytesWritten = L1desc[_inx].dataSize;

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

    try {
        int srcL1descInx{};
        int destL1descInx{};
        const irods::at_scope_exit close_objects{[&]() {
            if (destL1descInx > 3) {
                // The transferStat_t communicates information back to the client regarding
                // the data transfer such as bytes written and how many threads were used.
                // These must be saved before the L1 descriptor is free'd.
                *transStat = (transferStat_t*)malloc(sizeof(transferStat_t));
                memset(*transStat, 0, sizeof(transferStat_t));
                (*transStat)->bytesWritten = L1desc[destL1descInx].dataSize;
                (*transStat)->numThreads = L1desc[destL1descInx].dataObjInp->numThreads;

                if (const int ec = close_destination_data_obj(rsComm, destL1descInx); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed closing [{}] with status [{}]",
                        __FUNCTION__, __LINE__, destDataObjInp->objPath, ec));
                }
            }

            if (srcL1descInx > 3) {
                if (const int ec = close_source_data_obj(rsComm, srcL1descInx); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed closing [{}] with status [{}]",
                        __FUNCTION__, __LINE__, srcDataObjInp->objPath, ec));
                }
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
        L1desc[destL1descInx].dataObjInp->numThreads = thread_count;
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

} // anonymous namespace

int rsDataObjCopy(rsComm_t* rsComm,
                  dataObjCopyInp_t* dataObjCopyInp,
                  transferStat_t** transStat)
{
    namespace fs = ix::filesystem;

    const auto ec = rsDataObjCopy_impl(rsComm, dataObjCopyInp, transStat);
    const auto parent_path = fs::path{dataObjCopyInp->destDataObjInp.objPath}.parent_path();

    // Update the parent collection's mtime.
    if (ec == 0 && fs::server::is_collection_registered(*rsComm, parent_path)) {
        using std::chrono::system_clock;
        using std::chrono::time_point_cast;

        const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

        try {
            ix::scoped_privileged_client spc{*rsComm};
            fs::server::last_write_time(*rsComm, parent_path, mtime);
        }
        catch (const fs::filesystem_error& e) {
            ix::log::api::error(e.what());
            return e.code().value();
        }
    }

    return ec;
}

