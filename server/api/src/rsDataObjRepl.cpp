#include "apiNumber.h"
#include "dataObjClose.h"
#include "dataObjCreate.h"
#include "dataObjGet.h"
#include "dataObjLock.h"
#include "dataObjOpen.h"
#include "dataObjOpr.hpp"
#include "dataObjPut.h"
#include "dataObjRepl.h"
#include "dataObjTrim.h"
#include "fileStageToCache.h"
#include "fileSyncToArch.h"
#include "getRemoteZoneResc.h"
#include "icatDefines.h"
#include "l3FileGetSingleBuf.h"
#include "l3FilePutSingleBuf.h"
#include "miscServerFunct.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "resource.hpp"
#include "rodsLog.h"
#include "rsDataCopy.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjGet.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjPut.hpp"
#include "rsDataObjRead.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsDataObjWrite.hpp"
#include "rsFileStageToCache.hpp"
#include "rsFileSyncToArch.hpp"
#include "rsL3FileGetSingleBuf.hpp"
#include "rsL3FilePutSingleBuf.hpp"
#include "rsUnbunAndRegPhyBunfile.hpp"
#include "rsUnregDataObj.hpp"
#include "specColl.hpp"
#include "unbunAndRegPhyBunfile.h"

#include "irods_at_scope_exit.hpp"
#include "irods_log.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_call.hpp"
#include "irods_random.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_server_api_call.hpp"
#include "irods_server_properties.hpp"
#include "irods_stacktrace.hpp"
#include "irods_string_tokenize.hpp"
#include "key_value_proxy.hpp"
#include "key_value_proxy.hpp"
#include "replica_access_table.hpp"
#include "voting.hpp"

#include <string_view>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "fmt/format.h"

namespace {

namespace ix = irods::experimental;

using repl_input_tuple = std::tuple<dataObjInp_t, irods::file_object_ptr>;
repl_input_tuple init_destination_replica_input(RsComm& _comm, const DataObjInp& _inp)
{
    DataObjInp destination_data_obj_inp = _inp;
    replKeyVal(&_inp.condInput, &destination_data_obj_inp.condInput);
    auto cond_input = irods::experimental::make_key_value_proxy(destination_data_obj_inp.condInput);

    // Remove existing keywords used for source resource
    cond_input.erase(RESC_NAME_KW);
    cond_input.erase(RESC_HIER_STR_KW);

    if (cond_input.contains(DEST_RESC_HIER_STR_KW)) {
        cond_input[RESC_HIER_STR_KW] = cond_input.at(DEST_RESC_HIER_STR_KW).value();
        irods::file_object_ptr obj{new irods::file_object()};
        irods::error err = irods::file_object_factory(&_comm, &destination_data_obj_inp, obj);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }
        return {destination_data_obj_inp, obj};
    }

    std::string replica_number;
    if (cond_input.contains(REPL_NUM_KW)) {
        replica_number = cond_input[REPL_NUM_KW].value();

        // This keyword must be removed temporarily so that the voting mechanism does
        // not misinterpret it and change the operation from a CREATE to a WRITE.
        // See server/core/src/irods_resource_redirect.cpp for details.
        rmKeyVal(&destination_data_obj_inp.condInput, REPL_NUM_KW);
    }

    irods::at_scope_exit restore_replica_number_keyword{[&replica_number, &cond_input] {
        cond_input[REPL_NUM_KW] = replica_number;
    }};

    // Get the destination resource that the client specified, or use the default resource
    if (!cond_input.contains(DEST_RESC_HIER_STR_KW) &&
        !cond_input.contains(DEST_RESC_NAME_KW) &&
        cond_input.contains(DEF_RESC_NAME_KW)) {
        cond_input[DEST_RESC_NAME_KW] = cond_input.at(DEF_RESC_NAME_KW).value();
    }

    auto [obj, hier] = irods::resolve_resource_hierarchy(
        irods::CREATE_OPERATION, &_comm, destination_data_obj_inp);

    cond_input[RESC_HIER_STR_KW] = hier;
    cond_input[DEST_RESC_HIER_STR_KW] = hier;

    return {destination_data_obj_inp, obj};
} // init_destination_replica_input

repl_input_tuple init_source_replica_input(RsComm& _comm, const DataObjInp& _inp)
{
    DataObjInp source_data_obj_inp = _inp;
    replKeyVal(&_inp.condInput, &source_data_obj_inp.condInput);
    auto cond_input = irods::experimental::make_key_value_proxy(source_data_obj_inp.condInput);

    // Remove existing keywords used for destination resource
    cond_input.erase(DEST_RESC_NAME_KW);
    cond_input.erase(DEST_RESC_HIER_STR_KW);

    if (cond_input.contains(RESC_HIER_STR_KW)) {
        irods::file_object_ptr obj{new irods::file_object()};
        irods::error err = irods::file_object_factory(&_comm, &source_data_obj_inp, obj);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }
        return {source_data_obj_inp, obj};
    }

    auto [obj, hier] = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, &_comm, source_data_obj_inp);

    cond_input[RESC_HIER_STR_KW] = hier;

    return {source_data_obj_inp, obj};
} // init_source_replica_input

int close_replica(RsComm& _comm, const int _inx, const int _status)
{
    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = _inx;
    L1desc[dataObjCloseInp.l1descInx].oprStatus = _status;
    char* pdmo_kw = getValByKey(&L1desc[_inx].dataObjInp->condInput, IN_PDMO_KW);
    if (pdmo_kw) {
        addKeyVal(&dataObjCloseInp.condInput, IN_PDMO_KW, pdmo_kw);
    }
    const int status = rsDataObjClose(&_comm, &dataObjCloseInp);
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
    const auto* info = L1desc[source_l1descInx].dataObjInfo;
    irods::log(LOG_DEBUG, fmt::format("[{}:{}] - opened source replica [{}] on [{}] (repl [{}])",
        __FUNCTION__, __LINE__, info->objPath, info->rescHier, info->replNum));
    // TODO: Consider using force flag and making this part of the voting process
    if (GOOD_REPLICA != L1desc[source_l1descInx].dataObjInfo->replStatus) {
        const int status = SYS_NO_GOOD_REPLICA;
        close_replica(*rsComm, source_l1descInx, status);
        return status;
    }
    return source_l1descInx;
} // open_source_replica

int open_destination_replica(
    rsComm_t* rsComm,
    dataObjInp_t& destination_data_obj_inp,
    const int source_l1desc_inx)
{
    auto kvp = ix::make_key_value_proxy(destination_data_obj_inp.condInput);
    kvp[REG_REPL_KW] = "";
    kvp[DATA_ID_KW] = std::to_string(L1desc[source_l1desc_inx].dataObjInfo->dataId);
    kvp[SOURCE_L1_DESC_KW] = std::to_string(source_l1desc_inx);
    kvp.erase(PURGE_CACHE_KW);
    destination_data_obj_inp.oprType = REPLICATE_DEST;
    destination_data_obj_inp.openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - opening destination replica for [{}] (id:[{}]) on [{}]",
            __FUNCTION__,
            __LINE__,
            destination_data_obj_inp.objPath,
            kvp.at(DATA_ID_KW).value(),
            kvp.at(RESC_HIER_STR_KW).value()));
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
    int destination_l1descInx = open_destination_replica(rsComm, destination_inp, source_l1descInx);
    if (destination_l1descInx < 0) {
        close_replica(*rsComm, source_l1descInx, source_l1descInx);
        THROW(destination_l1descInx, "Failed opening destination replica");
    }
    L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;
    L1desc[destination_l1descInx].dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;

    // Copy data from source to destination
    int status = dataObjCopy(rsComm, destination_l1descInx);
    if (status < 0) {
        rodsLog(LOG_ERROR, "[%s] - dataObjCopy failed for [%s]", __FUNCTION__, destination_inp.objPath);
    }
    else {
        L1desc[destination_l1descInx].bytesWritten = L1desc[destination_l1descInx].dataObjInfo->dataSize;
    }

    // Save the token for the replica access table so that it can be removed
    // in the event of a failure in close. On failure, the entry is restored,
    // but this will prevent retries of the operation as the token information
    // is lost by the time we have returned to the caller.
    const auto token = L1desc[destination_l1descInx].replica_token;

    // Close destination replica
    int close_status = close_replica(*rsComm, destination_l1descInx, status);
    if (close_status < 0) {
        irods::log(LOG_ERROR, fmt::format(
            "[{}] - closing destination replica [{}] failed with [{}]",
            __FUNCTION__, destination_inp.objPath, close_status));

        if (status >= 0) {
            status = close_status;
        }

        auto& rat = irods::experimental::replica_access_table::instance();
        rat.erase_pid(token, getpid());
    }
    // Close source replica
    close_status = close_replica(*rsComm, source_l1descInx, 0);
    if (close_status < 0) {
        irods::log(LOG_ERROR, fmt::format(
            "[{}] - closing source replica [{}] failed with [{}]",
            __FUNCTION__, source_inp.objPath, close_status));

        if (status >= 0) {
            status = close_status;
        }
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
    auto dest_inp_tuple = init_destination_replica_input(*rsComm, dataObjInp);
    destination_inp = std::get<dataObjInp_t>(dest_inp_tuple);
    auto file_obj = std::get<irods::file_object_ptr>(dest_inp_tuple);

    auto source_inp_tuple = init_source_replica_input(*rsComm, dataObjInp);
    source_inp = std::get<dataObjInp_t>(source_inp_tuple);

    int status{};
    if (getValByKey(&dataObjInp.condInput, ALL_KW)) {
        for (const auto& r : file_obj->replicas()) {
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - hier:[{}],status:[{}],vote:[{}]",
                __FUNCTION__, __LINE__,
                r.resc_hier(),
                r.replica_status(),
                r.vote()));
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
            if (r.resc_hier() == dest_hier) {
                if (GOOD_REPLICA == r.replica_status()) {
                    const char* source_hier = getValByKey(&source_inp.condInput, RESC_HIER_STR_KW);
                    irods::log(LOG_DEBUG, fmt::format(
                        "[{}:{}] - hierarchy contains good replica already, source:[{}],dest:[{}]",
                        __FUNCTION__, __LINE__, dest_hier, source_hier));
                    return 0;
                }
                break;
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

    // -S and -r are not compatible
    if (getValByKey(&dataObjInp->condInput, RESC_NAME_KW) &&
        getValByKey(&dataObjInp->condInput, RECURSIVE_OPR__KW)) {
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

int dataObjCopy(rsComm_t* rsComm, int _destination_l1descInx)
{
    int source_l1descInx = L1desc[_destination_l1descInx].srcL1descInx;
    if (source_l1descInx < 3) {
        irods::log(LOG_ERROR, fmt::format(
            "[{}] - source l1 descriptor out of range:[{}]",
            __FUNCTION__, source_l1descInx));
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    int srcRemoteFlag{};
    int srcL3descInx = L1desc[source_l1descInx].l3descInx;
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

int unbunAndStageBunfileObj(rsComm_t* rsComm, const char* bunfileObjPath, char** outCacheRescName) {

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
