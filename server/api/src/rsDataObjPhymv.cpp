#include "dataObjPhymv.h"
#include "dataObjRepl.h"
#include "dataObjOpr.hpp"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "icatDefines.h"
#include "dataObjCreate.h"
#include "getRemoteZoneResc.h"
#include "physPath.hpp"
#include "fileClose.h"
#include "rsDataObjPhymv.hpp"
#include "rsFileOpen.hpp"
#include "rsFileClose.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjPhymv.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataObjTrim.hpp"
#include "rsDataObjUnlink.hpp"

// =-=-=-=-=-=-=-
#include "irods_at_scope_exit.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"
#include "key_value_proxy.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

namespace
{
    using data_object_proxy = irods::experimental::data_object::data_object_proxy<DataObjInfo>;

    // Takes a data_object_proxy and an input struct and determines the specified replica.
    // If the resource hierarchy was provided, get replica information using that. If the
    // replica number was provided, get replica information using that. Otherwise, returns nullptr.
    DataObjInfo* get_replica_information_based_on_input_keyword(data_object_proxy& _obj, const DataObjInp& _inp)
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        if (cond_input.contains(RESC_HIER_STR_KW)) {
            const auto hier = cond_input.at(RESC_HIER_STR_KW).value();

            const auto& replica = irods::experimental::data_object::find_replica(_obj, hier);

            if (!replica) {
                THROW(SYS_REPLICA_DOES_NOT_EXIST, fmt::format(
                    "replica does not exist; path:[[}],hier:[{]]",
                    _inp.objPath, hier));
            }

            return replica->get();
        }
        else if (cond_input.contains(REPL_NUM_KW)) {
            const auto repl_num = std::stoi(cond_input.at(REPL_NUM_KW).value().data());

            const auto& replica = irods::experimental::data_object::find_replica(_obj, repl_num);

            if (!replica) {
                THROW(SYS_REPLICA_DOES_NOT_EXIST, fmt::format(
                    "replica does not exist; path:[[}],repl_num:[{]]",
                    _inp.objPath, repl_num));
            }

            return replica->get();
        }

        return nullptr;
    } // get_replica_information_based_on_input_keyword

    void throw_if_no_write_permissions_on_source_replica(RsComm& _comm, const DataObjInp& _source_inp)
    {
        namespace fs = irods::experimental::filesystem;

        auto [obj, obj_lm] = irods::experimental::data_object::make_data_object_proxy(_comm, fs::path{_source_inp.objPath});

        auto* data_obj_info = get_replica_information_based_on_input_keyword(obj, _source_inp);
        if (!data_obj_info) {
            THROW(SYS_INTERNAL_ERR, "failed to get replica information");
        }

        std::string location;
        if (const irods::error ret = irods::get_loc_for_hier_string(data_obj_info->rescHier, location); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }

        // test the source hier to determine if we have write access to the data
        // stored.  if not then we cannot unlink that replica and should throw an
        // error.
        fileOpenInp_t open_inp{};
        open_inp.mode = getDefFileMode();
        open_inp.flags = O_WRONLY;
        rstrcpy(open_inp.resc_name_, data_obj_info->rescName, MAX_NAME_LEN);
        rstrcpy(open_inp.resc_hier_, data_obj_info->rescHier, MAX_NAME_LEN);
        rstrcpy(open_inp.objPath, data_obj_info->objPath, MAX_NAME_LEN);
        rstrcpy(open_inp.addr.hostAddr, location.c_str(), NAME_LEN);
        rstrcpy(open_inp.fileName, data_obj_info->filePath, MAX_NAME_LEN);
        rstrcpy(open_inp.in_pdmo, data_obj_info->in_pdmo, MAX_NAME_LEN);

        // kv passthru
        copyKeyVal(&data_obj_info->condInput, &open_inp.condInput);

        const int l3_index = rsFileOpen(&_comm, &open_inp);
        clearKeyVal(&open_inp.condInput);
        if(l3_index < 0) {
            const std::string msg = fmt::format("unable to open {} for unlink", data_obj_info->objPath);
            addRErrorMsg(&_comm.rError, l3_index, msg.c_str());
            THROW(l3_index, msg);
        }

        FileCloseInp close_inp{};
        close_inp.fileInx = l3_index;
        if (const int ec = rsFileClose(&_comm, &close_inp); ec < 0) {
            THROW(ec, fmt::format("failed to close {}", data_obj_info->objPath));
        }
    } // throw_if_no_write_permissions_on_source_replica

dataObjInp_t init_destination_replica_input(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp) {
    dataObjInp_t destination_data_obj_inp = dataObjInp;
    replKeyVal(&dataObjInp.condInput, &destination_data_obj_inp.condInput);

    // Remove existing keywords used for source resource
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_NAME_KW );
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_HIER_STR_KW );
    rmKeyVal( &destination_data_obj_inp.condInput, REPL_NUM_KW);

    const char* hier_kw = getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW);
    if (hier_kw) {
        addKeyVal(&destination_data_obj_inp.condInput, RESC_HIER_STR_KW, hier_kw);
        return destination_data_obj_inp;
    }

    const char* target = getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_NAME_KW);
    if (!target) {
        const auto status = USER__NULL_INPUT_ERR;
        const std::string msg{"Destination hierarchy or leaf resource required - none provided."};
        addRErrorMsg(&rsComm->rError, status, msg.c_str());
        THROW(status, msg);
    }

    irods::hierarchy_parser parser{target};
    if (parser.first_resc() == target && resc_mgr.is_coordinating_resource(target)) {
        const auto status = USER_INVALID_RESC_INPUT;
        const std::string msg{"Destination resource must be a leaf - coordinating resource provided."};
        addRErrorMsg(&rsComm->rError, status, msg.c_str());
        THROW(status, msg);
    }

    std::string hier{};
    irods::error ret = resc_mgr.get_hier_to_root_for_resc(parser.last_resc(), hier);
    if (!ret.ok()) {
        THROW(ret.code(), ret.result());
    }
    addKeyVal(&destination_data_obj_inp.condInput, RESC_HIER_STR_KW, hier.c_str());
    return destination_data_obj_inp;
} // init_destination_replica_input

dataObjInp_t init_source_replica_input(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp)
{
    dataObjInp_t source_data_obj_inp = dataObjInp;
    replKeyVal(&dataObjInp.condInput, &source_data_obj_inp.condInput);

    // Remove existing keywords used for destination resource
    rmKeyVal(&source_data_obj_inp.condInput, DEST_RESC_NAME_KW);
    rmKeyVal(&source_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW);
    // Need to be able to unlink the source replica after moving
    addKeyVal(&source_data_obj_inp.condInput, DATA_ACCESS_KW, ACCESS_DELETE_OBJECT);

    // If hierarchy or replica number are specified, that is all that is required
    const char* hier_kw = getValByKey(&source_data_obj_inp.condInput, RESC_HIER_STR_KW);
    if (hier_kw) {
        return source_data_obj_inp;
    }

    const char* target = getValByKey(&source_data_obj_inp.condInput, RESC_NAME_KW);
    const char* repl_num = getValByKey(&source_data_obj_inp.condInput, REPL_NUM_KW);
    if (target && repl_num) {
        const auto status = USER_INCOMPATIBLE_PARAMS;
        const std::string msg{"source resource and replica number cannot both be specified for source replica."};
        addRErrorMsg(&rsComm->rError, status, msg.c_str());
        THROW(status, msg);
    }

    if (!target && !repl_num) {
        const auto status = USER__NULL_INPUT_ERR;
        const std::string msg{"Source hierarchy or leaf resource or replica number required - none provided."};
        addRErrorMsg(&rsComm->rError, status, msg.c_str());
        THROW(status, msg);
    }

    if (repl_num) {
        rodsLog(LOG_DEBUG, "[%s] - repl_num [%s] used", __FUNCTION__, repl_num);
        return source_data_obj_inp;
    }

    irods::hierarchy_parser parser{target};
    if (parser.first_resc() == target && resc_mgr.is_coordinating_resource(target)) {
        const auto status = USER_INVALID_RESC_INPUT;
        const std::string msg{"Source resource must be a leaf - coordinating resource provided."};
        addRErrorMsg(&rsComm->rError, status, msg.c_str());
        THROW(status, msg);
    }
    std::string hier{};
    irods::error ret = resc_mgr.get_hier_to_root_for_resc(parser.last_resc(), hier);
    if (!ret.ok()) {
        THROW(ret.code(), ret.result());
    }
    addKeyVal(&source_data_obj_inp.condInput, RESC_HIER_STR_KW, hier.c_str());
    return source_data_obj_inp;
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
    dataObjInp_t& source_data_obj_inp) {
    source_data_obj_inp.oprType = PHYMV_SRC;
    source_data_obj_inp.openFlags = O_RDONLY;
    int source_l1descInx = rsDataObjOpen(rsComm, &source_data_obj_inp);
    if (source_l1descInx < 0) {
        return source_l1descInx;
    }
    return source_l1descInx;
} // open_source_replica

int open_destination_replica(
    rsComm_t* rsComm,
    dataObjInp_t& destination_data_obj_inp,
    const int source_l1desc_inx)
{
    auto kvp = irods::experimental::make_key_value_proxy(destination_data_obj_inp.condInput);
    kvp[REG_REPL_KW] = "";
    kvp[DATA_ID_KW] = std::to_string(L1desc[source_l1desc_inx].dataObjInfo->dataId);
    kvp[SOURCE_L1_DESC_KW] = std::to_string(source_l1desc_inx);
    kvp.erase(PURGE_CACHE_KW);

    destination_data_obj_inp.oprType = PHYMV_DEST;
    destination_data_obj_inp.openFlags = O_CREAT | O_RDWR;
    int destination_l1descInx = rsDataObjOpen(rsComm, &destination_data_obj_inp);
    if (destination_l1descInx < 0) {
        return destination_l1descInx;
    }
    return destination_l1descInx;
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
        close_replica(rsComm, source_l1descInx, source_l1descInx);
        THROW(destination_l1descInx, "Failed opening destination replica");
    }
    L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;

    // Copy data from source to destination
    int status = dataObjCopy(rsComm, destination_l1descInx);
    L1desc[destination_l1descInx].bytesWritten = L1desc[destination_l1descInx].dataObjInfo->dataSize;
    if (status < 0) {
        rodsLog(LOG_ERROR, "[%s] - dataObjCopy failed for [%s]",
            __FUNCTION__, destination_inp.objPath);
        L1desc[destination_l1descInx].bytesWritten = status;
    }
    else {
        const int trim_status = dataObjUnlinkS(rsComm, &source_inp, L1desc[source_l1descInx].dataObjInfo);
        if (trim_status < 0) {
            rodsLog(LOG_ERROR, "[%s] - unlinking source replica failed with [%d]",
                __FUNCTION__, trim_status);
            status = trim_status;
        }
    }

    // Close destination replica
    const int close_status = close_replica(rsComm, destination_l1descInx, status);
    if (close_status < 0) {
        rodsLog(LOG_ERROR, "[%s] - closing destination replica failed with [%d]",
            __FUNCTION__, close_status);
        status = close_status;
    }
    return status;
} // replicate_data

int move_replica(rsComm_t* rsComm, dataObjInp_t& dataObjInp)
{
    // Make sure the requested source and destination resources are valid
    dataObjInp_t destination_inp{};
    dataObjInp_t source_inp{};
    const irods::at_scope_exit free_cond_inputs{[&destination_inp, &source_inp]() {
        clearKeyVal(&destination_inp.condInput);
        clearKeyVal(&source_inp.condInput);
    }};
    destination_inp = init_destination_replica_input(rsComm, dataObjInp);
    source_inp = init_source_replica_input(rsComm, dataObjInp);

    // Need to make sure the physical data of the source replica can be unlinked before
    // beginning the phymv operation as it will be unlinked after the data movement has
    // taken place. Failing to do so may result in unintentionally unregistered data in
    // the resource vault.
    throw_if_no_write_permissions_on_source_replica(*rsComm, source_inp);

    const char* dest_hier = getValByKey(&destination_inp.condInput, RESC_HIER_STR_KW);
    const char* source_hier = getValByKey(&source_inp.condInput, RESC_HIER_STR_KW);

    if (dest_hier && source_hier &&
        std::string{dest_hier} == source_hier) {
        return 0;
    }
    return replicate_data(rsComm, source_inp, destination_inp);
} // move_replica

} // anonymous namespace

int rsDataObjPhymv(rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t **transStat)
{
    if (!rsComm || !dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    specCollCache_t *specCollCache{};
    resolveLinkedPath(rsComm, dataObjInp->objPath, &specCollCache, &dataObjInp->condInput);

    rodsServerHost_t *rodsServerHost{};
    const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (REMOTE_HOST == remoteFlag) {
        return _rcDataObjPhymv(rodsServerHost->conn, dataObjInp, transStat);
    }

    try {
        int status = move_replica(rsComm, *dataObjInp);
        if (status < 0) {
            rodsLog(LOG_NOTICE, "%s - Failed to physically move replica. status:[%d]",
                __FUNCTION__, status);
        }
        return status;
    }
    catch (const irods::exception& _e) {
        irods::log(_e);
        return _e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}] - [{}]", __FUNCTION__, e.what()));
        return SYS_LIBRARY_ERROR;
    }
    catch (...) {
        irods::log(LOG_ERROR, fmt::format("[{}] - unknown error occurred", __FUNCTION__));
        return SYS_UNKNOWN_ERROR;
    }
} // rsDataObjPhymv

