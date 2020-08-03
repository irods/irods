/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjPut.h for a description of this API call.*/

#include "dataObjPut.h"
#include "rodsLog.h"
#include "dataPut.h"
#include "filePut.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "dataObjOpen.h"
#include "dataObjCreate.h"
#include "dataObjClose.h"
#include "regDataObj.h"
#include "dataObjUnlink.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsApiHandler.hpp"
#include "subStructFilePut.h"
#include "dataObjRepl.h"
#include "getRemoteZoneResc.h"
#include "icatHighLevelRoutines.hpp"
#include "modDataObjMeta.h"
#include "rsDataObjPut.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataPut.hpp"
#include "rsRegDataObj.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsSubStructFilePut.hpp"
#include "rsFilePut.hpp"
#include "rsUnregDataObj.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjWrite.hpp"

#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_logger.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_serialization.hpp"
#include "irods_server_properties.hpp"
#include "irods_logger.hpp"
#include "server_utilities.hpp"
#include "filesystem/filesystem_error.hpp"

namespace {

int parallel_transfer_put(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    portalOprOut_t **portalOprOut)
{

    // Parallel transfer
    dataObjInp->openFlags |= O_CREAT | O_RDWR;
    int l1descInx = rsDataObjOpen(rsComm, dataObjInp);
    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    L1desc[l1descInx].oprType = PUT_OPR;
    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    if ( getStructFileType( L1desc[l1descInx].dataObjInfo->specColl ) >= 0 ) { // JMC - backport 4682
        *portalOprOut = ( portalOprOut_t * ) malloc( sizeof( portalOprOut_t ) );
        bzero( *portalOprOut,  sizeof( portalOprOut_t ) );
        ( *portalOprOut )->l1descInx = l1descInx;
        return l1descInx;
    }


    int status = preProcParaPut( rsComm, l1descInx, portalOprOut );

    if ( status < 0 ) {
        openedDataObjInp_t dataObjCloseInp{};
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = status;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        return status;
    }

    int allFlag = 0;
    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
        allFlag = 1;
    }

    dataObjInp_t replDataObjInp{};
    if ( allFlag == 1 ) {
        /* need to save dataObjInp. get freed in sendAndRecvBranchMsg */
        rstrcpy( replDataObjInp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
        addKeyVal( &replDataObjInp.condInput, UPDATE_REPL_KW, "" );
        addKeyVal( &replDataObjInp.condInput, ALL_KW, "" );
    }
    /* return portalOprOut to the client and wait for the rcOprComplete
     * call. That is when the parallel I/O is done */
    int retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
            ( void * ) * portalOprOut, NULL );

    if ( retval < 0 ) {
        openedDataObjInp_t dataObjCloseInp{};
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = retval;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        if ( allFlag == 1 ) {
            clearKeyVal( &replDataObjInp.condInput );
        }
    }
    else if (1 == allFlag) {
        transferStat_t *transStat = NULL;
        status = rsDataObjRepl(rsComm, &replDataObjInp, &transStat);
        free(transStat);
        clearKeyVal(&replDataObjInp.condInput);
        if (status < 0) {
            const auto err{ERROR(status, "rsDataObjRepl failed")};
            irods::log(err);
            return err.code();
        }
    }

    /* already send the client the status */
    return SYS_NO_HANDLER_REPLY_MSG;
} // parallel_transfer_put

int single_buffer_put(
    rsComm_t* rsComm,
    dataObjInp_t* dataObjInp,
    bytesBuf_t* dataObjInpBBuf)
{

    dataObjInp->openFlags |= (O_CREAT | O_RDWR | O_TRUNC);
    int l1descInx = rsDataObjOpen(rsComm, dataObjInp);
    if (l1descInx <= 2) {
        if ( l1descInx >= 0 ) {
            rodsLog( LOG_ERROR,
                    "%s: rsDataObjOpen of %s error, status = %d",
                    __FUNCTION__,
                    dataObjInp->objPath,
                    l1descInx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }
        return l1descInx;
    }

    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    openedDataObjInp_t dataObjWriteInp{};
    dataObjWriteInp.len = dataObjInpBBuf->len;
    dataObjWriteInp.l1descInx = l1descInx;

    bytesBuf_t dataObjWriteInpBBuf{};
    dataObjWriteInpBBuf.buf = dataObjInpBBuf->buf;
    dataObjWriteInpBBuf.len = dataObjInpBBuf->len;
    int bytesWritten = rsDataObjWrite(rsComm, &dataObjWriteInp, &dataObjWriteInpBBuf);
    if ( bytesWritten < 0 ) {
        rodsLog(LOG_NOTICE,
                "%s: rsDataObjWrite for %s failed with %d",
                __FUNCTION__, L1desc[l1descInx].dataObjInfo->filePath, bytesWritten );
        dataObjInfo_t* data_obj_info = L1desc[l1descInx].dataObjInfo;
        const int unlink_status = dataObjUnlinkS(rsComm, L1desc[l1descInx].dataObjInp, data_obj_info);
        if (unlink_status < 0) {
            irods::log(ERROR(unlink_status,
                (boost::format("dataObjUnlinkS failed for [%s] with [%d]") %
                data_obj_info->filePath % unlink_status).str()));
        }
    }

    if ( bytesWritten == 0 && myDataObjInfo->dataSize > 0 ) {
        /* overwrite with 0 len file */
        L1desc[l1descInx].bytesWritten = 1;
    }
    else {
        L1desc[l1descInx].bytesWritten = bytesWritten;
    }

    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = l1descInx;
    L1desc[l1descInx].oprStatus = bytesWritten;
    L1desc[l1descInx].oprType = PUT_OPR;
    int status = rsDataObjClose(rsComm, &dataObjCloseInp);
    if ( status < 0 ) {
        rodsLog( LOG_DEBUG,
                "%s: rsDataObjClose of %d error, status = %d",
                __FUNCTION__, l1descInx, status );
    }

    if ( bytesWritten < 0 ) {
        return bytesWritten;
    }

    if (status < 0) {
        return status;
    }

    if (getValByKey(&dataObjInp->condInput, ALL_KW)) {
        /* update the rest of copies */
        transferStat_t *transStat{};
        status = rsDataObjRepl( rsComm, dataObjInp, &transStat );
        if (transStat) {
            free(transStat);
        }
    }
    if (status >= 0) {
        status = applyRuleForPostProcForWrite(
                rsComm, dataObjInpBBuf, dataObjInp->objPath);
        if (status >= 0) {
            status = 0;
        }
    }
    return status;
} // single_buffer_put

} // anonymous namespace

int rsDataObjPut(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    bytesBuf_t *dataObjInpBBuf,
    portalOprOut_t **portalOprOut)
{
    try {
        if (irods::is_force_flag_required(*rsComm, *dataObjInp)) {
            return OVERWRITE_WITHOUT_FORCE_FLAG;
        }
    }
    catch (const irods::experimental::filesystem::filesystem_error& e) {
        irods::experimental::log::api::error(e.what());
        return e.code().value();
    }

    rodsServerHost_t *rodsServerHost{};
    specCollCache_t *specCollCache{};

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    int remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE );

    if (const char* acl_string = getValByKey( &dataObjInp->condInput, ACL_INCLUDED_KW)) {
        try {
            irods::deserialize_acl(acl_string);
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "%s", e.what());
            return e.code();
        }
    }
    if (const char* metadata_string = getValByKey(&dataObjInp->condInput, METADATA_INCLUDED_KW)) {
        try {
            irods::deserialize_metadata( metadata_string );
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "%s", e.what());
            return e.code();
        }
    }

    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (LOCAL_HOST != remoteFlag) {
        int status = _rcDataObjPut(
            rodsServerHost->conn,
            dataObjInp,
            dataObjInpBBuf,
            portalOprOut );
        if (status < 0 ||
            getValByKey(&dataObjInp->condInput, DATA_INCLUDED_KW)) {
            return status;
        }

        /* have to allocate a local l1descInx to keep track of things
         * since the file is in remote zone. It sets remoteL1descInx,
         * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that
         * rsComplete knows what to do */
        int l1descInx = allocAndSetL1descForZoneOpr(
                        ( *portalOprOut )->l1descInx, dataObjInp, rodsServerHost, NULL );
        if ( l1descInx < 0 ) {
            return l1descInx;
        }
        ( *portalOprOut )->l1descInx = l1descInx;
        return status;
    }

    if (!getValByKey(&dataObjInp->condInput, RESC_HIER_STR_KW)) {
        try {
            std::string hier{};
            std::tie(std::ignore, hier) = irods::resolve_resource_hierarchy(irods::CREATE_OPERATION, rsComm, *dataObjInp);
            addKeyVal(&dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str());
        }
        catch (const irods::exception& e) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log(LOG_ERROR, msg.str());
            irods::log(LOG_ERROR, e.what());
            return e.code();
        }
    }

    int status2 = applyRuleForPostProcForWrite( rsComm, dataObjInpBBuf, dataObjInp->objPath );
    if ( status2 < 0 ) {
        return ( status2 );
    }

    dataObjInp->openFlags = O_RDWR;

    if (getValByKey(&dataObjInp->condInput, DATA_INCLUDED_KW)) {
        return single_buffer_put(rsComm, dataObjInp, dataObjInpBBuf);
    }

    return parallel_transfer_put( rsComm, dataObjInp, portalOprOut );
} // rsDataObjPut

int preProcParaPut(
    rsComm_t *rsComm,
    int l1descInx,
    portalOprOut_t **portalOprOut)
{
    int status;
    dataOprInp_t dataOprInp;

    initDataOprInp( &dataOprInp, l1descInx, PUT_OPR );
    /* add RESC_HIER_STR_KW for getNumThreads */
    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        addKeyVal( &dataOprInp.condInput, RESC_HIER_STR_KW,
                   L1desc[l1descInx].dataObjInfo->rescHier );
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        status =  remoteDataPut( rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost );
    }
    else {
        status =  rsDataPut( rsComm, &dataOprInp, portalOprOut );
    }

    if ( status >= 0 ) {
        ( *portalOprOut )->l1descInx = l1descInx;
        L1desc[l1descInx].bytesWritten = dataOprInp.dataSize;
    }
    clearKeyVal( &dataOprInp.condInput );
    return status;
} // preProcParaPut

