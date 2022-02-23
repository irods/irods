#include "irods/rsDataObjLseek.hpp"

#include "irods/dataObjLseek.h"
#include "irods/rodsLog.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/subStructFileLseek.h"
#include "irods/objMetaOpr.hpp"
#include "irods/subStructFileUnlink.h"
#include "irods/rsSubStructFileLseek.hpp"
#include "irods/rsFileLseek.hpp"
#include "irods/irods_resource_backport.hpp"

#include <cstring>

int rsDataObjLseek(rsComm_t* rsComm,
                   openedDataObjInp_t* dataObjLseekInp,
                   fileLseekOut_t** dataObjLseekOut)
{
    const int l1descInx = dataObjLseekInp->l1descInx;

    if (l1descInx <= 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog(LOG_ERROR, "%s: l1descInx %d out of range", __func__, l1descInx);
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    auto& l1desc = L1desc[l1descInx];

    if (l1desc.inuseFlag != FD_INUSE) {
        return BAD_INPUT_DESC_INDEX;
    }

    if (l1desc.remoteZoneHost) {
        // Cross zone operation.
        dataObjLseekInp->l1descInx = l1desc.remoteL1descInx;
        const auto ec = rcDataObjLseek(l1desc.remoteZoneHost->conn, dataObjLseekInp, dataObjLseekOut);
        dataObjLseekInp->l1descInx = l1descInx;
        return ec;
    }

    const int l3descInx = l1desc.l3descInx;

    if (l3descInx <= 2) {
        rodsLog(LOG_ERROR, "%s: l3descInx %d out of range", __func__, l3descInx);
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    auto* dataObjInfo = l1desc.dataObjInfo;

    // Extract the host location from the resource hierarchy.
    std::string location;
    if (const auto ret = irods::get_loc_for_hier_string(dataObjInfo->rescHier, location); !ret.ok()) {
        irods::log(PASSMSG("rsDataObjLseek: failed in get_loc_for_hier_string", ret));
        return ret.code();
    }

    if (getStructFileType(dataObjInfo->specColl) >= 0) {
        subStructFileLseekInp_t subStructFileLseekInp{};
        subStructFileLseekInp.type = dataObjInfo->specColl->type;
        subStructFileLseekInp.fd = l1desc.l3descInx;
        subStructFileLseekInp.offset = dataObjLseekInp->offset;
        subStructFileLseekInp.whence = dataObjLseekInp->whence;
        rstrcpy(subStructFileLseekInp.addr.hostAddr, location.c_str(), NAME_LEN);
        rstrcpy(subStructFileLseekInp.resc_hier, dataObjInfo->rescHier, NAME_LEN);
        return rsSubStructFileLseek(rsComm, &subStructFileLseekInp, dataObjLseekOut);
    }

    // If the replica was opened in read-only mode, then don't allow the client
    // to seek past the data size in the catalog. This is necessary because the
    // size of the replica in storage could be larger than the data size in the
    // catalog.
    //
    // For all other modes, let the seek operation do what it normally does.
    //
    // This code does not apply to objects that are related to special collections.
    const auto offset = (O_RDONLY == (l1desc.dataObjInp->openFlags & O_ACCMODE))
        ? std::min(dataObjLseekInp->offset, dataObjInfo->dataSize)
        : dataObjLseekInp->offset;

    *dataObjLseekOut = static_cast<fileLseekOut_t*>(malloc(sizeof(fileLseekOut_t)));
    std::memset(*dataObjLseekOut, 0, sizeof(fileLseekOut_t));

    (*dataObjLseekOut)->offset = _l3Lseek(rsComm, l3descInx, offset, dataObjLseekInp->whence);

    if ((*dataObjLseekOut)->offset >= 0) {
        return 0;
    }

    return (*dataObjLseekOut)->offset;
}

rodsLong_t _l3Lseek(rsComm_t* rsComm, int l3descInx, rodsLong_t offset, int whence)
{
    fileLseekInp_t fileLseekInp{};
    fileLseekInp.fileInx = l3descInx;
    fileLseekInp.offset = offset;
    fileLseekInp.whence = whence;

    fileLseekOut_t* fileLseekOut = nullptr;

    if (const auto ec = rsFileLseek(rsComm, &fileLseekInp, &fileLseekOut); ec < 0) {
        return ec;
    }

    const auto off = fileLseekOut->offset;
    std::free(fileLseekOut);

    return off;
}

