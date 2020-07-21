#include "dataObjRepl.h"
#include "dataObjOpr.hpp"
#include "dataObjCreate.h"
#include "dataObjOpen.h"
#include "dataObjClose.h"
#include "dataObjPut.h"
#include "dataObjGet.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "icatDefines.h"
#include "getRemoteZoneResc.h"
#include "l3FileGetSingleBuf.h"
#include "l3FilePutSingleBuf.h"
#include "fileSyncToArch.h"
#include "fileStageToCache.h"
#include "unbunAndRegPhyBunfile.h"
#include "dataObjTrim.h"
#include "dataObjLock.h"
#include "miscServerFunct.hpp"
#include "rsDataObjRepl.hpp"
#include "apiNumber.h"
#include "rsDataCopy.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjRead.hpp"
#include "rsDataObjWrite.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsUnregDataObj.hpp"
#include "rsL3FileGetSingleBuf.hpp"
#include "rsDataObjGet.hpp"
#include "rsDataObjPut.hpp"
#include "rsL3FilePutSingleBuf.hpp"
#include "rsFileStageToCache.hpp"
#include "rsFileSyncToArch.hpp"
#include "rsUnbunAndRegPhyBunfile.hpp"

#include "irods_at_scope_exit.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_log.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_call.hpp"
#include "irods_random.hpp"
#include "irods_string_tokenize.hpp"
#include "voting.hpp"
#include "key_value_proxy.hpp"

#include <string_view>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace ix = irods::experimental;

namespace {

using repl_input_tuple = std::tuple<dataObjInp_t, irods::file_object_ptr>;
repl_input_tuple construct_input_tuple(
    rsComm_t* rsComm,
    dataObjInp_t& _inp,
    const char* kw_hier,
    const std::string& _operation) {
    if (kw_hier) {
        std::string hier = kw_hier;
        addKeyVal( &_inp.condInput, RESC_HIER_STR_KW, hier.c_str() );
        irods::file_object_ptr obj{new irods::file_object()};
        irods::error err = irods::file_object_factory(rsComm, &_inp, obj);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }
        return {_inp, obj};
    }

    auto [obj, hier] = irods::resolve_resource_hierarchy(_operation, rsComm, _inp);
    addKeyVal(&_inp.condInput, RESC_HIER_STR_KW, hier.c_str());
    return {_inp, obj};
} // construct_input_tuple

repl_input_tuple init_destination_replica_input(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp) {
    dataObjInp_t destination_data_obj_inp = dataObjInp;
    replKeyVal(&dataObjInp.condInput, &destination_data_obj_inp.condInput);

    // Remove existing keywords used for source resource
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_NAME_KW );
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_HIER_STR_KW );

    std::string replica_number;
    ix::key_value_proxy kvp{destination_data_obj_inp.condInput};

    if (kvp.contains(REPL_NUM_KW)) {
        replica_number = kvp[REPL_NUM_KW].value();

        // This keyword must be removed temporarily so that the voting mechanism does
        // not misinterpret it and change the operation from a CREATE to a WRITE.
        // See server/core/src/irods_resource_redirect.cpp for details.
        rmKeyVal(&destination_data_obj_inp.condInput, REPL_NUM_KW);
    }

    irods::at_scope_exit restore_replica_number_keyword{[&replica_number, &kvp] {
        kvp[REPL_NUM_KW] = replica_number;
    }};

    // Get the destination resource that the client specified, or use the default resource
    if (!getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW) &&
        !getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_NAME_KW)) {
        const char* dest_resc = getValByKey(&destination_data_obj_inp.condInput, DEF_RESC_NAME_KW);
        addKeyVal(&destination_data_obj_inp.condInput, DEST_RESC_NAME_KW, dest_resc);
    }
    return construct_input_tuple(
        rsComm,
        destination_data_obj_inp,
        getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW),
        irods::CREATE_OPERATION);
} // init_destination_replica_input

repl_input_tuple init_source_replica_input(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp) {
    dataObjInp_t source_data_obj_inp = dataObjInp;
    replKeyVal(&dataObjInp.condInput, &source_data_obj_inp.condInput);

    // Remove existing keywords used for destination resource
    rmKeyVal(&source_data_obj_inp.condInput, DEST_RESC_NAME_KW);
    rmKeyVal(&source_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW);

    return construct_input_tuple(
        rsComm,
        source_data_obj_inp,
        getValByKey(&source_data_obj_inp.condInput, RESC_HIER_STR_KW),
        irods::OPEN_OPERATION);
} // init_source_replica_input

int close_replica(
    rsComm_t* rsComm,
    const int _inx,
    const int _status) {
    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = _inx;
    L1desc[dataObjCloseInp.l1descInx].oprStatus = _status;
    char* pdmo_kw = getValByKey(&L1desc[_inx].dataObjInp->condInput, IN_PDMO_KW);
    if (pdmo_kw) {
        addKeyVal(&dataObjCloseInp.condInput, IN_PDMO_KW, pdmo_kw);
    }
    const int status = rsDataObjClose( rsComm, &dataObjCloseInp);
    if (status < 0) {
        rodsLog(LOG_ERROR, "[%s] - rsDataObjClose failed with [%d]", __FUNCTION__, status);
    }
    clearKeyVal( &dataObjCloseInp.condInput );
    return status;
} // close_replica

int open_source_replica(
    rsComm_t* rsComm,
    dataObjInp_t& source_data_obj_inp)
{
    source_data_obj_inp.oprType = REPLICATE_SRC;
    source_data_obj_inp.openFlags = O_RDONLY;
    int source_l1descInx = rsDataObjOpen(rsComm, &source_data_obj_inp);
    if (source_l1descInx < 0) {
        return source_l1descInx;
    }
    // TODO: Consider using force flag and making this part of the voting process
    if (GOOD_REPLICA != L1desc[source_l1descInx].dataObjInfo->replStatus) {
        const int status = SYS_NO_GOOD_REPLICA;
        close_replica(rsComm, source_l1descInx, status);
        return status;
    }
    return source_l1descInx;
} // open_source_replica

int open_destination_replica(
    rsComm_t* rsComm,
    dataObjInp_t& destination_data_obj_inp)
{
    addKeyVal(&destination_data_obj_inp.condInput, REG_REPL_KW, "");
    addKeyVal(&destination_data_obj_inp.condInput, FORCE_FLAG_KW, "");
    destination_data_obj_inp.oprType = REPLICATE_DEST;
    destination_data_obj_inp.openFlags = O_CREAT | O_RDWR;
    rodsLog(LOG_DEBUG,
            "[%s:%d] - opening destination replica [%s] on [%s]",
            __FUNCTION__,
            __LINE__,
            destination_data_obj_inp.objPath,
            getValByKey(&destination_data_obj_inp.condInput, RESC_HIER_STR_KW));
    return rsDataObjOpen(rsComm, &destination_data_obj_inp);
} // open_destination_replica

int replicate_data(
    rsComm_t* rsComm,
    dataObjInp_t& source_inp,
    dataObjInp_t& destination_inp)
{
    // Open source replica
    int source_l1descInx = open_source_replica(rsComm, source_inp);
    if (source_l1descInx < 0) {
        THROW(source_l1descInx, "Failed opening source replica");
    }

    // Open destination replica
    int destination_l1descInx = open_destination_replica(rsComm, destination_inp);
    if (destination_l1descInx < 0) {
        close_replica(rsComm, source_l1descInx, source_l1descInx);
        THROW(destination_l1descInx, "Failed opening destination replica");
    }
    L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;

    // Copy data from source to destination
    const int status = dataObjCopy(rsComm, destination_l1descInx);
    if (status < 0) {
        rodsLog(LOG_ERROR, "[%s] - dataObjCopy failed for [%s]", __FUNCTION__, destination_inp.objPath);
    }
    L1desc[destination_l1descInx].bytesWritten = L1desc[destination_l1descInx].dataObjInfo->dataSize;

    // Close destination replica
    int close_status = close_replica(rsComm, destination_l1descInx, status);
    if (close_status < 0) {
        rodsLog(LOG_ERROR,
                "[%s] - closing destination replica [%s] failed with [%d]",
                __FUNCTION__, destination_inp.objPath, close_status);
    }
    // Close source replica
    close_status = close_replica(rsComm, source_l1descInx, status);
    if (close_status < 0) {
        rodsLog(LOG_ERROR,
                "[%s] - closing source replica [%s] failed with [%d]",
                __FUNCTION__, source_inp.objPath, close_status);
    }
    return status;
} // replicate_data

int repl_data_obj(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp)
{
    namespace irv = irods::experimental::resource::voting;

    // Make sure the requested source and destination resources are valid
    dataObjInp_t destination_inp{};
    dataObjInp_t source_inp{};
    const irods::at_scope_exit free_cond_inputs{[&destination_inp, &source_inp]() {
        clearKeyVal(&destination_inp.condInput);
        clearKeyVal(&source_inp.condInput);
    }};
    auto dest_inp_tuple = init_destination_replica_input(rsComm, dataObjInp);
    destination_inp = std::get<dataObjInp_t>(dest_inp_tuple);
    auto file_obj = std::get<irods::file_object_ptr>(dest_inp_tuple);

    auto source_inp_tuple = init_source_replica_input(rsComm, dataObjInp);
    source_inp = std::get<dataObjInp_t>(source_inp_tuple);

    int status{};
    if (getValByKey(&dataObjInp.condInput, ALL_KW)) {
        for (const auto& r : file_obj->replicas()) {
            rodsLog(LOG_DEBUG, "[%s:%d] - hier:[%s],status:[%d],vote:[%f]",
                __FUNCTION__, __LINE__,
                r.resc_hier().c_str(),
                r.replica_status(),
                r.vote());
            if (GOOD_REPLICA == (r.replica_status() & 0x0F)) {
                continue;
            }
            if (r.vote() > irv::vote::zero) {
                addKeyVal(&destination_inp.condInput, RESC_HIER_STR_KW, r.resc_hier().c_str());
                status = replicate_data(rsComm, source_inp, destination_inp);
            }
        }
    }
    else {
        const char* dest_hier = getValByKey(&destination_inp.condInput, RESC_HIER_STR_KW);
        for (const auto& r : file_obj->replicas()) {
            // TODO: #4010 - This short-circuits resource logic for handling good replicas
            if (r.resc_hier() == dest_hier && (r.replica_status() & 0x0F) == GOOD_REPLICA) {
                return 0;
            }
        }
        status = replicate_data(rsComm, source_inp, destination_inp);
    }
    return status;
} // repl_data_obj

int singleL1Copy(
    rsComm_t *rsComm,
    dataCopyInp_t& dataCopyInp) {

    int trans_buff_size;
    try {
        trans_buff_size = irods::get_advanced_setting<const int>(irods::CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS) * 1024 * 1024;
    } catch ( const irods::exception& e ) {
        irods::log(e);
        return e.code();
    }

    dataOprInp_t* dataOprInp = &dataCopyInp.dataOprInp;
    int destL1descInx = dataCopyInp.portalOprOut.l1descInx;
    int srcL1descInx = L1desc[destL1descInx].srcL1descInx;


    openedDataObjInp_t dataObjReadInp{};
    dataObjReadInp.l1descInx = srcL1descInx;
    dataObjReadInp.len = trans_buff_size;

    bytesBuf_t dataObjReadInpBBuf{};
    dataObjReadInpBBuf.buf = malloc(dataObjReadInp.len);
    dataObjReadInpBBuf.len = dataObjReadInp.len;
    const irods::at_scope_exit free_data_obj_read_inp_bbuf{[&dataObjReadInpBBuf]() {
        free(dataObjReadInpBBuf.buf);
    }};

    openedDataObjInp_t dataObjWriteInp{};
    dataObjWriteInp.l1descInx = destL1descInx;

    bytesBuf_t dataObjWriteInpBBuf{};
    dataObjWriteInpBBuf.buf = dataObjReadInpBBuf.buf;
    dataObjWriteInpBBuf.len = 0;

    int bytesRead{};
    rodsLong_t totalWritten = 0;
    while ((bytesRead = rsDataObjRead(rsComm, &dataObjReadInp, &dataObjReadInpBBuf)) > 0) {
        dataObjWriteInp.len = bytesRead;
        dataObjWriteInpBBuf.len = bytesRead;
        int bytesWritten = rsDataObjWrite(rsComm, &dataObjWriteInp, &dataObjWriteInpBBuf);
        if (bytesWritten != bytesRead) {
            rodsLog(LOG_ERROR,
                    "%s: Read %d bytes, Wrote %d bytes.\n ",
                    __FUNCTION__, bytesRead, bytesWritten );
            return SYS_COPY_LEN_ERR;
        }
        totalWritten += bytesWritten;
    }

    if (dataOprInp->dataSize > 0 &&
        !getValByKey(&dataOprInp->condInput, NO_CHK_COPY_LEN_KW) &&
        totalWritten != dataOprInp->dataSize) {
        rodsLog(LOG_ERROR,
                "%s: totalWritten %lld dataSize %lld mismatch",
                __FUNCTION__, totalWritten, dataOprInp->dataSize);
        return SYS_COPY_LEN_ERR;
    }
    return 0;
} // singleL1Copy

} // anonymous namespace

int rsDataObjRepl(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    transferStat_t **transStat) {
    if (!dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // -S and -n are not compatible
    if (getValByKey(&dataObjInp->condInput, RESC_NAME_KW) &&
        getValByKey(&dataObjInp->condInput, REPL_NUM_KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    // -a and -R are not compatible
    if (getValByKey(&dataObjInp->condInput, ALL_KW) &&
        getValByKey(&dataObjInp->condInput, DEST_RESC_NAME_KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    // Must be a privileged user to invoke SU
    if (getValByKey(&dataObjInp->condInput, SU_CLIENT_USER_KW) &&
        rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }

    // Resolve path in linked collection if applicable
    dataObjInfo_t *dataObjInfo{};
    const irods::at_scope_exit free_data_obj_info{[dataObjInfo]() {
        freeAllDataObjInfo(dataObjInfo);
    }};
    int status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                    READ_COLL_PERM, 0, &dataObjInfo );
    if (status == DATA_OBJ_T && dataObjInfo && dataObjInfo->specColl) {
        if (dataObjInfo->specColl->collClass != LINKED_COLL) {
            return SYS_REG_OBJ_IN_SPEC_COLL;
        }
        rstrcpy(dataObjInp->objPath, dataObjInfo->objPath, MAX_NAME_LEN);
    }

    rodsServerHost_t *rodsServerHost{};
    const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if (remoteFlag == REMOTE_HOST) {
        *transStat = (transferStat_t*)malloc(sizeof(transferStat_t));
        memset(*transStat, 0, sizeof(transferStat_t));
        status = _rcDataObjRepl(rodsServerHost->conn, dataObjInp, transStat);
        return status;
    }

    try {
        addKeyVal(&dataObjInp->condInput, IN_REPL_KW, "");
        status = repl_data_obj(rsComm, *dataObjInp);
        rmKeyVal(&dataObjInp->condInput, IN_REPL_KW);
    }
    catch (const irods::exception& e) {
        irods::log(e);
        status = e.code();
    }

    if (status < 0 && status != DIRECT_ARCHIVE_ACCESS) {
        rodsLog(LOG_NOTICE, "%s - Failed to replicate data object. status:[%d]",
                __FUNCTION__, status);
    }
    return (status == DIRECT_ARCHIVE_ACCESS) ? 0 : status;
} // rsDataObjRepl

int dataObjCopy(
    rsComm_t* rsComm,
    int _destination_l1descInx) {

    int srcRemoteFlag{};
    int source_l1descInx = L1desc[_destination_l1descInx].srcL1descInx;
    int srcL3descInx = L1desc[source_l1descInx].l3descInx;

    // copy data size from src catalog to destination
    L1desc[_destination_l1descInx].dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;
    //L1desc[source_l1descInx].dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;

    if (L1desc[source_l1descInx].remoteZoneHost) {
        srcRemoteFlag = REMOTE_ZONE_HOST;
    }
    else {
        srcRemoteFlag = FileDesc[srcL3descInx].rodsServerHost->localFlag;
    }
    int destRemoteFlag{};
    int destL3descInx = L1desc[_destination_l1descInx].l3descInx;
    if (L1desc[_destination_l1descInx].remoteZoneHost) {
        destRemoteFlag = REMOTE_ZONE_HOST;
    }
    else {
        destRemoteFlag = FileDesc[destL3descInx].rodsServerHost->localFlag;
    }

    dataCopyInp_t dataCopyInp{};
    const irods::at_scope_exit clear_cond_input{[&dataCopyInp]() {
        clearKeyVal(&dataCopyInp.dataOprInp.condInput);
    }};

    portalOprOut_t* portalOprOut{};
    if (srcRemoteFlag == REMOTE_ZONE_HOST &&
        destRemoteFlag == REMOTE_ZONE_HOST) {
        // Destination: remote zone
        // Source: remote zone
        initDataOprInp(&dataCopyInp.dataOprInp, _destination_l1descInx, COPY_TO_REM_OPR);
        L1desc[_destination_l1descInx].dataObjInp->numThreads = 0;
        dataCopyInp.portalOprOut.l1descInx = _destination_l1descInx;
        return singleL1Copy(rsComm, dataCopyInp);
    }

    if (srcRemoteFlag != REMOTE_ZONE_HOST &&
        destRemoteFlag != REMOTE_ZONE_HOST &&
        FileDesc[srcL3descInx].rodsServerHost == FileDesc[destL3descInx].rodsServerHost) {
        // Destination: local zone
        // Source: local zone
        // Source and destination host are the same
        initDataOprInp( &dataCopyInp.dataOprInp, _destination_l1descInx, SAME_HOST_COPY_OPR );
        dataCopyInp.portalOprOut.numThreads = dataCopyInp.dataOprInp.numThreads;
        if ( srcRemoteFlag == LOCAL_HOST ) {
            addKeyVal(&dataCopyInp.dataOprInp.condInput, EXEC_LOCALLY_KW, "");
        }
    }
    else if (destRemoteFlag == REMOTE_ZONE_HOST ||
             (srcRemoteFlag == LOCAL_HOST && destRemoteFlag != LOCAL_HOST)) {
        // Destination: remote zone OR different host in local zone
        // Source: local zone
        initDataOprInp( &dataCopyInp.dataOprInp, _destination_l1descInx, COPY_TO_REM_OPR );
        if ( L1desc[_destination_l1descInx].dataObjInp->numThreads > 0 ) {
            int status = preProcParaPut( rsComm, _destination_l1descInx, &portalOprOut );
            if (status < 0 || !portalOprOut) {
                rodsLog(LOG_NOTICE,
                        "%s: preProcParaPut error for %s",
                        __FUNCTION__,
                        L1desc[source_l1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = _destination_l1descInx;
        }
        if ( srcRemoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataCopyInp.dataOprInp.condInput, EXEC_LOCALLY_KW, "" );
        }
    }
    else if (srcRemoteFlag == REMOTE_ZONE_HOST ||
             (srcRemoteFlag != LOCAL_HOST && destRemoteFlag == LOCAL_HOST)) {
        // Destination: local zone
        // Source: remote zone OR different host in local zone
        initDataOprInp( &dataCopyInp.dataOprInp, _destination_l1descInx, COPY_TO_LOCAL_OPR );
        if ( L1desc[_destination_l1descInx].dataObjInp->numThreads > 0 ) {
            int status = preProcParaGet( rsComm, source_l1descInx, &portalOprOut );
            if (status < 0 || !portalOprOut) {
                rodsLog(LOG_NOTICE,
                        "%s: preProcParaGet error for %s",
                        __FUNCTION__,
                        L1desc[source_l1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = source_l1descInx;
        }
        if ( destRemoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataCopyInp.dataOprInp.condInput, EXEC_LOCALLY_KW, "" );
        }
    }
    else {
        /* remote to remote */
        initDataOprInp( &dataCopyInp.dataOprInp, _destination_l1descInx, COPY_TO_LOCAL_OPR );
        if (L1desc[_destination_l1descInx].dataObjInp->numThreads > 0) {
            int status = preProcParaGet(rsComm, source_l1descInx, &portalOprOut);
            if (status < 0 || !portalOprOut) {
                rodsLog(LOG_NOTICE,
                        "%s: preProcParaGet error for %s",
                        __FUNCTION__,
                        L1desc[source_l1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = source_l1descInx;
        }
    }

    if (getValByKey(&L1desc[_destination_l1descInx].dataObjInp->condInput, NO_CHK_COPY_LEN_KW)) {
        addKeyVal(&dataCopyInp.dataOprInp.condInput, NO_CHK_COPY_LEN_KW, "");
        if (L1desc[_destination_l1descInx].dataObjInp->numThreads > 1) {
            L1desc[_destination_l1descInx].dataObjInp->numThreads = 1;
            L1desc[source_l1descInx].dataObjInp->numThreads = 1;
            dataCopyInp.portalOprOut.numThreads = 1;
        }
    }
    int status = rsDataCopy(rsComm, &dataCopyInp);
    if (status >= 0 && portalOprOut && L1desc[_destination_l1descInx].dataObjInp) {
        /* update numThreads since it could be changed by remote server */
        L1desc[_destination_l1descInx].dataObjInp->numThreads = portalOprOut->numThreads;
    }
    free(portalOprOut);
    return status;
} // dataObjCopy

int unbunAndStageBunfileObj(
    rsComm_t* rsComm,
    char* bunfileObjPath,
    char** outCacheRescName) {

    /* query the bundle dataObj */
    dataObjInp_t dataObjInp{};
    rstrcpy( dataObjInp.objPath, bunfileObjPath, MAX_NAME_LEN );

    dataObjInfo_t *bunfileObjInfoHead{};
    int status = getDataObjInfo( rsComm, &dataObjInp, &bunfileObjInfoHead, NULL, 1 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unbunAndStageBunfileObj: getDataObjInfo of bunfile %s failed.stat=%d",
                 dataObjInp.objPath, status );
        return status;
    }
    status = _unbunAndStageBunfileObj( rsComm, &bunfileObjInfoHead, &dataObjInp.condInput,
                                       outCacheRescName, 0 );

    freeAllDataObjInfo( bunfileObjInfoHead );

    return status;
} // unbunAndStageBunfileObj

int _unbunAndStageBunfileObj(
    rsComm_t* rsComm,
    dataObjInfo_t** bunfileObjInfoHead,
    keyValPair_t * condInput,
    char** outCacheRescName,
    const int rmBunCopyFlag) {

    dataObjInp_t dataObjInp{};
    bzero( &dataObjInp.condInput, sizeof( dataObjInp.condInput ) );
    rstrcpy( dataObjInp.objPath, ( *bunfileObjInfoHead )->objPath, MAX_NAME_LEN );
    int status = sortObjInfoForOpen( bunfileObjInfoHead, condInput, 0 );

    addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, ( *bunfileObjInfoHead )->rescHier );
    if ( status < 0 ) {
        return status;
    }

    if (outCacheRescName) {
        *outCacheRescName = ( *bunfileObjInfoHead )->rescName;
    }

    addKeyVal(&dataObjInp.condInput, BUN_FILE_PATH_KW, (*bunfileObjInfoHead)->filePath);
    if ( rmBunCopyFlag > 0 ) {
        addKeyVal( &dataObjInp.condInput, RM_BUN_COPY_KW, "" );
    }
    if (!std::string_view{(*bunfileObjInfoHead)->dataType}.empty()) {
        addKeyVal(&dataObjInp.condInput, DATA_TYPE_KW, (*bunfileObjInfoHead)->dataType);
    }
    status = _rsUnbunAndRegPhyBunfile(rsComm, &dataObjInp, (*bunfileObjInfoHead)->rescName);

    return status;
} // _unbunAndStageBunfileObj
