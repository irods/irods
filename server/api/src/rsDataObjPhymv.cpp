/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

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

namespace {

dataObjInp_t init_destination_replica_input(
    rsComm_t* rsComm,
    const dataObjInp_t& dataObjInp) {
    dataObjInp_t destination_data_obj_inp = dataObjInp;
    replKeyVal(&dataObjInp.condInput, &destination_data_obj_inp.condInput);

    // Remove existing keywords used for source resource
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_NAME_KW );
    rmKeyVal( &destination_data_obj_inp.condInput, RESC_HIER_STR_KW );
    rmKeyVal( &destination_data_obj_inp.condInput, REPL_NUM_KW);

    // Get the destination resource that the client specified, or use the default resource
    const char* hier_kw = getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_HIER_STR_KW);
    if (hier_kw) {
        addKeyVal(&destination_data_obj_inp.condInput, RESC_HIER_STR_KW, hier_kw);
        return destination_data_obj_inp;
    }
    const char* target = getValByKey(&destination_data_obj_inp.condInput, DEST_RESC_NAME_KW);
    if (!target) {
        THROW(USER__NULL_INPUT_ERR, "Destination hierarchy or leaf resource required - none provided.");
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
    source_data_obj_inp.openFlags = O_RDWR;
    int source_l1descInx = rsDataObjOpen(rsComm, &source_data_obj_inp);
    if (source_l1descInx < 0) {
        return source_l1descInx;
    }
    return source_l1descInx;
} // open_source_replica

int open_destination_replica(
    rsComm_t* rsComm,
    dataObjInp_t& destination_data_obj_inp) {
    addKeyVal(&destination_data_obj_inp.condInput, REG_REPL_KW, "");
    addKeyVal(&destination_data_obj_inp.condInput, FORCE_FLAG_KW, "");
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
    int destination_l1descInx = open_destination_replica(rsComm, destination_inp);
    if (destination_l1descInx < 0) {
        close_replica(rsComm, source_l1descInx, source_l1descInx);
        THROW(destination_l1descInx, "Failed opening destination replica");
    }
    L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;

    // Copy data from source to destination
    int status = dataObjCopy(rsComm, destination_l1descInx);
    L1desc[destination_l1descInx].bytesWritten = L1desc[destination_l1descInx].dataObjInfo->dataSize;
    L1desc[source_l1descInx].bytesWritten = 0;
    if (status < 0) {
        rodsLog(LOG_ERROR, "[%s] - dataObjCopy failed for [%s]",
            __FUNCTION__, destination_inp.objPath);
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

int move_replica(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp) {
    // Make sure the requested source and destination resources are valid
    dataObjInp_t destination_inp{};
    dataObjInp_t source_inp{};
    const irods::at_scope_exit free_cond_inputs{[&destination_inp, &source_inp]() {
        clearKeyVal(&destination_inp.condInput);
        clearKeyVal(&source_inp.condInput);
    }};
    destination_inp = init_destination_replica_input(rsComm, dataObjInp);
    source_inp = init_source_replica_input(rsComm, dataObjInp);

    const char* dest_hier = getValByKey(&destination_inp.condInput, RESC_HIER_STR_KW);
    const char* source_hier = getValByKey(&source_inp.condInput, RESC_HIER_STR_KW);
    if (dest_hier && source_hier &&
        std::string{dest_hier} == source_hier) {
        return 0;
    }
    return replicate_data(rsComm, source_inp, destination_inp);
} // move_replica

} // anonymous namespace

int rsDataObjPhymv(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    transferStat_t **transStat) {

    if (!dataObjInp) {
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
} // rsDataObjPhymv

