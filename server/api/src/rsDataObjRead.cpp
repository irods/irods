#include "irods/rsDataObjRead.hpp"

#include "irods/dataObjInpOut.h"
#include "irods/dataObjRead.h"
#include "irods/fileLseek.h"
#include "irods/rodsLog.h"
#include "irods/objMetaOpr.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/subStructFileRead.h"  /* XXXXX can be taken out when structFile api done */
#include "irods/rsSubStructFileRead.hpp"
#include "irods/rsFileRead.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/rsDataObjLseek.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <algorithm>
#include <limits>

int
applyRuleForPostProcForRead( rsComm_t *rsComm, bytesBuf_t *dataObjReadOutBBuf, char *objPath ) {
    if ( ReadWriteRuleState != ON_STATE ) {
        return 0;
    }

    ruleExecInfo_t rei2;
    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    msParamArray_t msParamArray;
    memset( ( char* )&msParamArray, 0, sizeof( msParamArray_t ) );

    if ( rsComm != NULL ) {
        rei2.rsComm = rsComm;
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }
    rei2.doi = ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    memset(rei2.doi,0,sizeof(dataObjInfo_t));

    snprintf( rei2.doi->objPath, sizeof( rei2.doi->objPath ), "%s", objPath );

    memset( &msParamArray, 0, sizeof( msParamArray ) );
    int * myInOutStruct = ( int* )malloc( sizeof( int ) );
    *myInOutStruct = dataObjReadOutBBuf->len;
    addMsParamToArray( &msParamArray, "*ReadBuf", BUF_LEN_MS_T, myInOutStruct,
                       dataObjReadOutBBuf, 0 );
    int status = applyRule( "acPostProcForDataObjRead(*ReadBuf)", &msParamArray, &rei2,
                            NO_SAVE_REI );
    free( rei2.doi );
    if ( status < 0 ) {
        if ( rei2.status < 0 ) {
            status = rei2.status;
        }
        rodsLog( LOG_ERROR,
                 "rsDataObjRead: acPostProcForDataObjRead error=%d", status );
        clearMsParamArray( &msParamArray, 0 );
        return status;
    }
    clearMsParamArray( &msParamArray, 0 );

    return 0;

}

int
rsDataObjRead(rsComm_t* rsComm,
              openedDataObjInp_t* dataObjReadInp,
              bytesBuf_t* dataObjReadOutBBuf)
{
    const int l1descInx = dataObjReadInp->l1descInx;

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
        dataObjReadInp->l1descInx = l1desc.remoteL1descInx;
        const auto bytes_read = rcDataObjRead(l1desc.remoteZoneHost->conn, dataObjReadInp, dataObjReadOutBBuf);
        dataObjReadInp->l1descInx = l1descInx;
        return bytes_read;
    }

    auto* dataObjInfo = l1desc.dataObjInfo;

    // If the replica was opened in read-only mode, then don't allow the client
    // to read past the data size in the catalog. This is necessary because the
    // size of the replica in storage could be larger than the data size in the
    // catalog.
    //
    // For all other modes, let the read operation do what it normally does.
    //
    // This code does not apply to objects that are related to special collections.
    if (!dataObjInfo->specColl && O_RDONLY == (l1desc.dataObjInp->openFlags & O_ACCMODE)) {
        OpenedDataObjInp input{};
        input.l1descInx = l1descInx;
        input.whence = SEEK_CUR;

        FileLseekOut* output{};

        irods::at_scope_exit free_output{[&output] {
            if (output) { // NOLINT(readability-implicit-bool-conversion)
                std::free(output); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            }
        }};

        if (const auto ec = rsDataObjLseek(rsComm, &input, &output); ec < 0) {
            rodsLog(LOG_ERROR, "%s: Could not retrieve the current file read position.", __func__, ec);
            return ec;
        }

        // If the file read position is greater than or equal to the data size,
        // then return immediately.
        if (output->offset >= dataObjInfo->dataSize) {
            return 0;
        }

        // At this point, we know the file read position is less than the replica's
        // recorded data size. We now have to adjust the requested number of bytes to
        // read so that the client does not read past the recorded data size.

        // Perform:
        //     Len = min(Len, min(DataSize-Offset,INT_MAX))
        // in a way that doesn't lop off the upper bits to fit a rodsLong_t into the smaller 'int' type
        // needed by a data object read operation.

        const auto buffer_size = std::min<rodsLong_t>(dataObjReadInp->len, dataObjInfo->dataSize - output->offset);
        using limits_type = std::numeric_limits<decltype(dataObjReadInp->len)>;
        dataObjReadInp->len = (buffer_size > static_cast<rodsLong_t>(limits_type::max())) ? limits_type::max() : buffer_size;
    }

    const auto bytes_read = l3Read(rsComm, l1descInx, dataObjReadInp->len, dataObjReadOutBBuf);
    const auto i = applyRuleForPostProcForRead(rsComm, dataObjReadOutBBuf, dataObjInfo->objPath);
    if (i < 0) {
        return i;
    }

    return bytes_read;
}

int
l3Read( rsComm_t *rsComm, int l1descInx, int len,
        bytesBuf_t *dataObjReadOutBBuf ) {
    dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Read - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileFdOprInp_t subStructFileReadInp;
        memset( &subStructFileReadInp, 0, sizeof( subStructFileReadInp ) );
        subStructFileReadInp.type = dataObjInfo->specColl->type;
        subStructFileReadInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileReadInp.len = len;
        rstrcpy( subStructFileReadInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileReadInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        return rsSubStructFileRead( rsComm, &subStructFileReadInp, dataObjReadOutBBuf );
    }

    if (0 == dataObjInfo->dataSize && !(L1desc[l1descInx].bytesWritten > 0)) {
        rodsLog(LOG_DEBUG, "[%s] - empty file - marking bytes buf len as 0", __FUNCTION__);
        dataObjReadOutBBuf->len = 0;
        return 0;
    }

    fileReadInp_t fileReadInp{};
    fileReadInp.fileInx = L1desc[l1descInx].l3descInx;
    fileReadInp.len = len;
    return rsFileRead( rsComm, &fileReadInp, dataObjReadOutBBuf );
}

int
_l3Read( rsComm_t *rsComm, int l3descInx, void *buf, int len ) {
    fileReadInp_t fileReadInp;
    bytesBuf_t dataObjReadInpBBuf;

    dataObjReadInpBBuf.len = len;
    dataObjReadInpBBuf.buf = buf;

    memset( &fileReadInp, 0, sizeof( fileReadInp ) );
    fileReadInp.fileInx = l3descInx;
    fileReadInp.len = len;
    return rsFileRead( rsComm, &fileReadInp, &dataObjReadInpBBuf );
}

