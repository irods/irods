#include "dataObjInpOut.h"
#include "dataObjOpen.h"
#include "dataObjOpenAndStat.h"
#include "irods_exception.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_linked_list_iterator.hpp"
#include "irods_resource_types.hpp"
#include "objInfo.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "dataObjCreate.h"
#include "dataObjLock.h"
#include "rsDataObjOpen.hpp"
#include "apiNumber.h"
#include "rsDataObjCreate.hpp"
#include "rsModDataObjMeta.hpp"
#include "rsSubStructFileOpen.hpp"
#include "rsFileOpen.hpp"
#include "rsDataObjRepl.hpp"
#include "rsRegReplica.hpp"
#include "rsDataObjClose.hpp"
#include "rsPhyPathReg.hpp"

#include "fileOpen.h"
#include "subStructFileOpen.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "regReplica.h"
#include "regDataObj.h"
#include "dataObjClose.h"
#include "dataObjRepl.h"
#include "rcMisc.h"

#include "dataObjCreateAndStat.h"
#include "fileCreate.h"
#include "subStructFileCreate.h"
#include "specColl.hpp"
#include "dataObjUnlink.h"
#include "regDataObj.h"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "icatHighLevelRoutines.hpp"
#include "rsObjStat.hpp"
#include "rsRegDataObj.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsSubStructFileCreate.hpp"
#include "rsFileCreate.hpp"
#include "rsGetRescQuota.hpp"
#include "rsUnregDataObj.hpp"
#include "rsModDataObjMeta.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_call.hpp"
#include "irods_at_scope_exit.hpp"
#include "key_value_proxy.hpp"
#include "replica_access_table.hpp"
#include "scoped_privileged_client.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

#include "boost/format.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include <fmt/format.h>

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <stdexcept>

namespace ix = irods::experimental;

namespace {

// Instructs how "update_replica_access_table" should update the
// replica access table.
enum class update_operation
{
    create,
    update
};

void update_replica_access_table(rsComm_t& _conn,
                                 update_operation _op,
                                 int _l1desc_index,
                                 const dataObjInp_t& _input)
{
    const irods::experimental::filesystem::path p = _input.objPath;
    const ix::key_value_proxy kvp{_input.condInput};

    ix::replica_access_table::data_id_type data_id;
    ix::replica_access_table::replica_number_type replica_number;

    try {
        const auto gql = fmt::format("select DATA_ID, DATA_REPL_NUM "
                                     "where"
                                     " COLL_NAME = '{}' and"
                                     " DATA_NAME = '{}' and"
                                     " DATA_RESC_HIER = '{}'",
                                     p.parent_path().c_str(),
                                     p.object_name().c_str(),
                                     kvp[RESC_HIER_STR_KW].value());

        for (auto&& row : irods::query{&_conn, gql}) {
            data_id = std::stoull(row[0]);
            replica_number = std::stoul(row[1]);
        }
    }
    catch (const std::out_of_range&) {
        THROW(SYS_INTERNAL_ERR, "Could not convert string to integer");
    }

    auto& rat = irods::experimental::replica_access_table::instance();
    auto& l1desc = L1desc[_l1desc_index];

    try {
        if (update_operation::create == _op) {
            l1desc.replica_token = rat.create_new_entry(data_id, replica_number, getpid());
        }
        else {
            auto token = kvp[REPLICA_TOKEN_KW].value();
            rat.append_pid(token.data(), data_id, replica_number, getpid());
            l1desc.replica_token = token;
        }
    }
    catch (const ix::replica_access_table_error& e) {
        rodsLog(LOG_ERROR, e.what());
        THROW(SYS_INTERNAL_ERR, e.what());
    }
}

void enable_creation_of_additional_replicas(rsComm_t& _comm)
{
    // rxDataObjOpen has the freedom to create replicas on demand. To enable this,
    // it must always set the following flag. This special flag instructs rsPhyPathReg
    // to register a new replica if an existing replica already exists.
    ix::key_value_proxy{_comm.session_props}[REG_REPL_KW] = "";
}

int register_intermediate_replica(
    rsComm_t* _comm,
    dataObjInp_t* _inp,
    const char* _path) {
    if (!getValByKey(&_inp->condInput, REGISTER_AS_INTERMEDIATE_KW)) {
        addKeyVal(&_inp->condInput, REGISTER_AS_INTERMEDIATE_KW, "");
    }
    addKeyVal(&_inp->condInput, FILE_PATH_KW, _path);
    addKeyVal(&_inp->condInput, DATA_SIZE_KW, "0");
    return rsPhyPathReg(_comm, _inp);
}

int l3CreateByObjInfo(
    rsComm_t* rsComm,
    dataObjInp_t* dataObjInp,
    dataObjInfo_t* dataObjInfo ) {

    int chkType = getchkPathPerm( rsComm, dataObjInp, dataObjInfo );
    if ( chkType == DISALLOW_PATH_REG ) {
        return PATH_REG_NOT_ALLOWED;
    }

    fileCreateInp_t fileCreateInp{};
    rstrcpy(fileCreateInp.resc_name_,    dataObjInfo->rescName, MAX_NAME_LEN);
    rstrcpy(fileCreateInp.resc_hier_,    dataObjInfo->rescHier, MAX_NAME_LEN);
    rstrcpy(fileCreateInp.objPath,       dataObjInfo->objPath,  MAX_NAME_LEN);
    rstrcpy(fileCreateInp.fileName,      dataObjInfo->filePath, MAX_NAME_LEN);
    rstrcpy(fileCreateInp.in_pdmo,       dataObjInfo->in_pdmo,  MAX_NAME_LEN );
    fileCreateInp.mode = getFileMode(dataObjInp);
    copyKeyVal(&dataObjInfo->condInput, &fileCreateInp.condInput);

    if ( chkType == NO_CHK_PATH_PERM ) {
        fileCreateInp.otherFlags |= NO_CHK_PERM_FLAG;
    }

    //loop until we find a valid filename
    int retryCnt = 0;
    int l3descInx;
    do {
        fileCreateOut_t* create_out{};
        l3descInx = rsFileCreate(rsComm, &fileCreateInp, &create_out);

        // update the dataObjInfo with the potential changes made by the resource - hcj
        if (create_out) {
            rstrcpy(dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN);
            rstrcpy(dataObjInfo->filePath, create_out->file_name, MAX_NAME_LEN);
            free(create_out);
        }

        //update the filename in case of a retry
        rstrcpy(fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
        retryCnt++;
    }
    while ( l3descInx < 0 && getErrno( l3descInx ) == EEXIST &&
            resolveDupFilePath( rsComm, dataObjInfo, dataObjInp ) >= 0 &&
            l3descInx <= 2 && retryCnt < 100 );
    clearKeyVal( &fileCreateInp.condInput );
    return l3descInx;
}

int create_sub_struct_file(
    rsComm_t *rsComm,
    const int l1descInx) {
    dataObjInfo_t *dataObjInfo = L1desc[l1descInx].dataObjInfo;
    std::string location{};
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if (!ret.ok()) {
        irods::log(PASSMSG((boost::format("%s - failed in get_loc_for_hier_string") %
            __FUNCTION__).str(), ret));
        return ret.code();
    }

    subFile_t subFile{};
    rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
    rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

    subFile.specColl = dataObjInfo->specColl;
    subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
    return rsSubStructFileCreate( rsComm, &subFile );
}

int l3Create(
    rsComm_t *rsComm,
    const int l1descInx) {
    dataObjInfo_t *dataObjInfo = L1desc[l1descInx].dataObjInfo;
    if (getStructFileType(dataObjInfo->specColl) >= 0) {
        return create_sub_struct_file(rsComm, l1descInx);    
    }
    /* normal or mounted file */
    return l3CreateByObjInfo(rsComm, L1desc[l1descInx].dataObjInp, L1desc[l1descInx].dataObjInfo);
}

int specCollSubCreate(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp) {
    dataObjInfo_t* dataObjInfo{};
    int status = resolvePathInSpecColl( rsComm, dataObjInp.objPath,
                                    WRITE_COLL_PERM, 0, &dataObjInfo );
    if (!dataObjInfo) {
        rodsLog(LOG_ERROR,
                "%s :: dataObjInfo is null",
                __FUNCTION__ );
        return status;
    }
    if (status >= 0) {
        rodsLog(LOG_ERROR,
                "%s: phyPath %s already exist",
                __FUNCTION__, dataObjInfo->filePath );
        freeDataObjInfo( dataObjInfo );
        return SYS_COPY_ALREADY_IN_RESC;
    }
    else if (status != SYS_SPEC_COLL_OBJ_NOT_EXIST) {
        freeDataObjInfo( dataObjInfo );
        return status;
    }

    int l1descInx = allocL1desc();
    if (l1descInx < 0) {
        return l1descInx;
    }

    dataObjInfo->replStatus = INTERMEDIATE_REPLICA;
    fillL1desc(l1descInx, &dataObjInp, dataObjInfo, dataObjInfo->replStatus, dataObjInp.dataSize);

    status = l3Create( rsComm, l1descInx );
    if ( status <= 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s: l3Create of %s failed, status = %d",
                 __FUNCTION__, L1desc[l1descInx].dataObjInfo->filePath, status);
        freeL1desc(l1descInx);
        return status;
    }
    L1desc[l1descInx].l3descInx = status;
    return l1descInx;
}

int create_data_obj(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp)
{
    rodsObjStat_t* rodsObjStatOut{};
    const irods::at_scope_exit free_obj_stat_out{
        [&rodsObjStatOut]() {
            freeRodsObjStat(rodsObjStatOut);
        }
    };

    addKeyVal(&dataObjInp.condInput, SEL_OBJ_TYPE_KW, "dataObj" );
    int status = rsObjStat( rsComm, &dataObjInp, &rodsObjStatOut );
    if (status < 0) {
        rodsLog(LOG_DEBUG, "[%s:%d] - rsObjStat failed with [%d]", __FUNCTION__, __LINE__, status);
    }

    if (rodsObjStatOut) {
        if (COLL_OBJ_T == rodsObjStatOut->objType) {
            return USER_INPUT_PATH_ERR;
        }

        if (rodsObjStatOut->specColl) {
            // Linked collection should have been translated by this point
            if (LINKED_COLL == rodsObjStatOut->specColl->collClass) {
                return SYS_COLL_LINK_PATH_ERR;
            }

            if (UNKNOWN_OBJ_T == rodsObjStatOut->objType) {
                return specCollSubCreate( rsComm, dataObjInp );
            }
        }
    }

    addKeyVal(&dataObjInp.condInput, OPEN_TYPE_KW, std::to_string(CREATE_TYPE).c_str());
    int l1descInx = allocL1desc();
    if (l1descInx < 0) {
        return l1descInx;
    }

    dataObjInfo_t* dataObjInfo = (dataObjInfo_t*)malloc(sizeof(dataObjInfo_t));
    initDataObjInfoWithInp(dataObjInfo, &dataObjInp);

    const char* resc_hier = getValByKey(&dataObjInp.condInput, RESC_HIER_STR_KW);
    if (resc_hier) {
        // we need to favor the results from the PEP acSetRescSchemeForCreate
        std::string root = irods::hierarchy_parser{resc_hier}.first_resc();
        rstrcpy( dataObjInfo->rescName, root.c_str(), NAME_LEN );
        rstrcpy( dataObjInfo->rescHier, resc_hier, MAX_NAME_LEN );
    }

    irods::error ret = resc_mgr.hier_to_leaf_id(dataObjInfo->rescHier, dataObjInfo->rescId);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    dataObjInfo->replStatus = INTERMEDIATE_REPLICA;
    fillL1desc(l1descInx, &dataObjInp, dataObjInfo, dataObjInfo->replStatus, dataObjInp.dataSize);

    status = getFilePathName(rsComm, dataObjInfo, L1desc[l1descInx].dataObjInp);
    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return status;
    }

    status = register_intermediate_replica(rsComm, L1desc[l1descInx].dataObjInp, dataObjInfo->filePath);
    if (status < 0) {
        freeL1desc(l1descInx);
        return status;
    }

    if (getValByKey(&dataObjInp.condInput, NO_OPEN_FLAG_KW)) {
        return l1descInx;
    }

    status = l3Create(rsComm, l1descInx);
    if (status < 0) {
        rodsLog(LOG_NOTICE,
                "%s: l3Create of %s failed, status = %d",
                __FUNCTION__, L1desc[l1descInx].dataObjInfo->filePath, status );
        dataObjInfo_t* data_obj_info = L1desc[l1descInx].dataObjInfo;
        const int unlink_status = dataObjUnlinkS(rsComm, L1desc[l1descInx].dataObjInp, data_obj_info);
        if (unlink_status < 0) {
            irods::log(ERROR(unlink_status,
                (boost::format("dataObjUnlinkS failed for [%s] with [%d]") %
                data_obj_info->filePath % unlink_status).str()));
        }
        freeL1desc(l1descInx);
        return status;
    }
    L1desc[l1descInx].l3descInx = status;
    return l1descInx;
} // create_data_obj

int stageBundledData( rsComm_t * rsComm, dataObjInfo_t **subfileObjInfoHead ) {
    dataObjInfo_t *dataObjInfoHead = *subfileObjInfoHead;
    char* cacheRescName{};
    int status = unbunAndStageBunfileObj(
                    rsComm,
                    dataObjInfoHead->filePath,
                    &cacheRescName);
    if ( status < 0 ) {
        return status;
    }

    /* query the bundle dataObj */
    dataObjInp_t dataObjInp{};
    addKeyVal( &dataObjInp.condInput, RESC_NAME_KW, cacheRescName );
    rstrcpy( dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN );

    dataObjInfo_t* cacheObjInfo{};
    status = getDataObjInfo( rsComm, &dataObjInp, &cacheObjInfo, NULL, 0 );
    clearKeyVal( &dataObjInp.condInput );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "%s: getDataObjInfo of subfile %s failed.stat=%d",
                 __FUNCTION__, dataObjInp.objPath, status );
        return status;
    }
    /* que the cache copy at the top */
    queDataObjInfo( subfileObjInfoHead, cacheObjInfo, 0, 1 );
    return status;
}

int l3Open(
    rsComm_t *rsComm,
    int l1descInx)
{
    dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;
    if (!dataObjInfo) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    std::string location{};
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Open - failed in get_loc_for_hier_string", ret ) );
        return ret.code();
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile{};
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        subFile.flags = getFileFlags( l1descInx );
        return rsSubStructFileOpen( rsComm, &subFile );
    }

    fileOpenInp_t fileOpenInp{};
    rstrcpy( fileOpenInp.resc_name_, dataObjInfo->rescName, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.objPath,    dataObjInfo->objPath, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    rstrcpy( fileOpenInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    fileOpenInp.mode = getFileMode(L1desc[l1descInx].dataObjInp);
    fileOpenInp.flags = getFileFlags(l1descInx);
    rstrcpy( fileOpenInp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );

    copyKeyVal(&dataObjInfo->condInput, &fileOpenInp.condInput);

    const int l3descInx = rsFileOpen(rsComm, &fileOpenInp);
    clearKeyVal( &fileOpenInp.condInput );
    return l3descInx;
} // l3Open

int open_with_obj_info(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp,
    dataObjInfo_t* dataObjInfo)
{
    int l1descInx = allocL1desc();
    if (l1descInx < 0) {
        return l1descInx;
    }

    copyKeyVal(&dataObjInp.condInput, &dataObjInfo->condInput);

    /* the size was set to -1 because we don't know the target size.
     * For copy and replicate, the calling routine should modify this
     * dataSize */
    fillL1desc(l1descInx, &dataObjInp, dataObjInfo, dataObjInfo->replStatus, -1);

    if (getValByKey(&dataObjInp.condInput, NO_OPEN_FLAG_KW)) {
        /* don't actually physically open the file */
        return l1descInx;
    }

    if (getValByKey(&dataObjInp.condInput, PHYOPEN_BY_SIZE_KW)) {
        int single_buff_sz;
        try {
            single_buff_sz = irods::get_advanced_setting<const int>(irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER) * 1024 * 1024;
        } catch (const irods::exception& e) {
            irods::log(e);
            return e.code();
        }

        /* open for put or get. May do "dataInclude" */
        if (dataObjInfo->dataSize <= single_buff_sz &&
            (getValByKey(&dataObjInp.condInput, DATA_INCLUDED_KW) ||
             dataObjInfo->dataSize != UNKNOWN_FILE_SZ)) {
            return l1descInx;
        }
    }

    int status = l3Open(rsComm, l1descInx);
    if (status <= 0) {
        rodsLog(LOG_NOTICE, "%s: l3Open of %s failed, status = %d",
                __FUNCTION__, dataObjInfo->filePath, status);
        freeL1desc( l1descInx );
        return status;
    }

    auto& fd = L1desc[l1descInx];
    fd.l3descInx = status;

    // Set the size of the data object to zero in the catalog if the file was truncated.
    // It is important that the catalog reflect truncation immediately because operations
    // following the open may depend on the size of the data object.
    if (fd.dataObjInp->openFlags & O_TRUNC) {
        if (const auto access_mode = (fd.dataObjInp->openFlags & O_ACCMODE);
            access_mode == O_WRONLY || access_mode == O_RDWR)
        {
            dataObjInfo_t info{};
            rstrcpy(info.objPath, fd.dataObjInp->objPath, MAX_NAME_LEN);
            rstrcpy(info.rescHier, fd.dataObjInfo->rescHier, MAX_NAME_LEN);

            keyValPair_t kvp{};
            addKeyVal(&kvp, DATA_SIZE_KW, "0");
            if (getValByKey(&dataObjInp.condInput, ADMIN_KW)) {
                addKeyVal(&kvp, ADMIN_KW, "");
            }

            modDataObjMeta_t input{};
            input.dataObjInfo = &info;
            input.regParam = &kvp;

            if (const auto ec = rsModDataObjMeta(rsComm, &input); ec != 0) {
                rodsLog(LOG_ERROR, "dataOpen: Could not update size of data object [status = %d, path = %s]",
                        ec, info.objPath);
                return ec;
            }

            fd.dataSize = 0;

            if (fd.dataObjInfo) {
                fd.dataObjInfo->dataSize = 0;
            }
        }
    }

    return l1descInx;
} // open_with_obj_info

int applyPreprocRuleForOpen(
    rsComm_t* rsComm,
    dataObjInp_t* dataObjInp,
    dataObjInfo_t** dataObjInfoHead)
{
    ruleExecInfo_t rei;
    initReiWithDataObjInp( &rei, rsComm, dataObjInp );
    rei.doi = *dataObjInfoHead;

    // make resource properties available as rule session variables
    irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

    int status = applyRule( "acPreprocForDataObjOpen", NULL, &rei, NO_SAVE_REI );
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);

    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }
        rodsLog( LOG_ERROR,
                 "%s:acPreprocForDataObjOpen error for %s,stat=%d",
                 __FUNCTION__, dataObjInp->objPath, status );
    }
    else {
        *dataObjInfoHead = rei.doi;
    }
    return status;
} // applyPreprocRuleForOpen

bool hier_has_replica(
    const std::vector<irods::physical_object>& _replicas,
    const std::string& _hier)
{
    return std::any_of(_replicas.begin(), _replicas.end(),
        [&_hier](const irods::physical_object& _replica) {
            return _replica.resc_hier() == _hier;
        });
} // hier_has_replica

int change_replica_status_to_intermediate(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp,
    dataObjInfo_t* dataObjInfo)
{
    using namespace irods::experimental;
    keyValPair_t kvp{};
    replKeyVal(&dataObjInp.condInput, &kvp);
    key_value_proxy proxy{kvp};
    proxy[REPL_STATUS_KW] = std::to_string(INTERMEDIATE_REPLICA);
    proxy.erase(ALL_KW);
    proxy.erase(OPEN_TYPE_KW);
    //proxy[IN_PDMO_KW] = dataObjInfo->rescHier;

    modDataObjMeta_t inp{};
    inp.dataObjInfo = dataObjInfo;
    inp.regParam = proxy.get();
    const int status = rsModDataObjMeta(rsComm, &inp);
    if (status < 0) {
        rodsLog(LOG_ERROR,
            "[%s] - rsModDataObjMeta failed with [%d] when modifying [%s] replica [%d]",
            __FUNCTION__, status, dataObjInp.objPath, dataObjInfo->replNum);
    }
    return status;
} // change_replica_status_to_intermediate

int change_replica_status(rsComm_t& rsComm, dataObjInp_t& dataObjInp, int new_replica_status)
{
    {
        const auto l = {GOOD_REPLICA, INTERMEDIATE_REPLICA, STALE_REPLICA};
        if (std::none_of(std::begin(l), std::end(l), [x = new_replica_status](auto y) { return x == y; })) {
            return SYS_INVALID_INPUT_PARAM;
        }
    }

    irods::experimental::key_value_proxy src{dataObjInp.condInput};
    auto resc_hier = src[RESC_HIER_STR_KW].value();

    dataObjInfo_t info{};
    rstrcpy(info.objPath, dataObjInp.objPath, MAX_NAME_LEN);
    rstrcpy(info.rescHier, resc_hier.data(), MAX_NAME_LEN);

    keyValPair_t kvp{};
    irods::experimental::key_value_proxy dst{kvp};
    dst[REPL_STATUS_KW] = std::to_string(new_replica_status);

    modDataObjMeta_t inp{};
    inp.dataObjInfo = &info;
    inp.regParam = dst.get();

    return rsModDataObjMeta(&rsComm, &inp);
} // change_replica_status_to_stale

int close_replica(rsComm_t& conn, int l1desc_index)
{
    openedDataObjInp_t input{};
    input.l1descInx = l1desc_index;
    return rsDataObjClose(&conn, &input);
}

int _rsDataObjOpen(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjInp,
    dataObjInfo_t* dataObjInfoHead,
    irods::file_object_ptr _obj)
{
    char* lockType = getValByKey(&dataObjInp.condInput, LOCK_TYPE_KW);
    int lockFd = -1;
    if (lockType) {
        lockFd = irods::server_api_call(
                     DATA_OBJ_LOCK_AN,
                     rsComm, &dataObjInp,
                     NULL, (void**)NULL, NULL);
        if ( lockFd > 0 ) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal(&dataObjInp.condInput, LOCK_TYPE_KW);
        }
        else {
            rodsLog(LOG_ERROR,
                    "%s: lock error for %s. lockType = %s",
                    __FUNCTION__, dataObjInp.objPath, lockType );
            return lockFd;
        }
    }

    const auto unlock_data_obj{[&]() {
        char fd_string[NAME_LEN]{};
        snprintf( fd_string, sizeof( fd_string ), "%-d", lockFd );
        addKeyVal( &dataObjInp.condInput, LOCK_FD_KW, fd_string );
        irods::server_api_call(
            DATA_OBJ_UNLOCK_AN,
            rsComm,
            &dataObjInp,
            NULL,
            ( void** ) NULL,
            NULL );
    }};

    // If O_RDWR or O_WRONLY is set in the open flags, then "writeFlag" will be set to "1".
    const int writeFlag = getWriteFlag(dataObjInp.openFlags);

    // Depending on the conditions, this block may create a new replica.
    if (dataObjInp.openFlags & O_CREAT && writeFlag > 0) {
        const std::string resc_hier = getValByKey(&dataObjInp.condInput, RESC_HIER_STR_KW);
        if (!hier_has_replica(_obj->replicas(), resc_hier)) {
            int l1descInx = create_data_obj(rsComm, dataObjInp);

            if ( lockFd >= 0 ) {
                if ( l1descInx > 0 ) {
                    L1desc[l1descInx].lockFd = lockFd;
                }
                else {
                    unlock_data_obj();
                }
            }
            
            if (l1descInx >= 0) {
                try {
                    update_replica_access_table(*rsComm, update_operation::create, l1descInx, dataObjInp);
                }
                catch (const irods::exception& e) {
                    rodsLog(LOG_ERROR, "Could not update replica access table for newly created data object. "
                                       "Closing data object and setting replica status to stale. "
                                       "[error_code=%d, path=%s, exception=%s]",
                                       e.code(), dataObjInp.objPath, e.what());

                    if (const auto ec = close_replica(*rsComm, l1descInx); ec < 0) {
                        auto hier = ix::key_value_proxy{dataObjInp.condInput}[RESC_HIER_STR_KW].value();
                        rodsLog(LOG_ERROR, "Failed to close replica [error_code=%d, path=%s, hierarchy=%s]",
                                           ec, dataObjInp.objPath, hier.data());
                        return ec;
                    }

                    if (const auto ec = change_replica_status(*rsComm, dataObjInp, STALE_REPLICA); ec < 0) {
                        auto hier = ix::key_value_proxy{dataObjInp.condInput}[RESC_HIER_STR_KW].value();
                        rodsLog(LOG_ERROR, "Failed to set the replica's replica status to stale "
                                           "[error_code={}, path={}, hierarchy={}]",
                                           ec, dataObjInp.objPath, hier.data());
                        return ec;
                    }

                    return e.code();
                }
            }

            return l1descInx;
        }

        const std::string hier{getValByKey(&dataObjInp.condInput, RESC_HIER_STR_KW)};
        const std::string root_resc = irods::hierarchy_parser{hier}.first_resc();
        addKeyVal(&dataObjInp.condInput, DEST_RESC_NAME_KW, root_resc.c_str());
        addKeyVal(&dataObjInp.condInput, OPEN_TYPE_KW, std::to_string(OPEN_FOR_WRITE_TYPE).c_str());
    }

    // Shifts the entries in the dataObjInfo list around so that the target
    // replica is at the front.
    int status = sortObjInfoForOpen(&dataObjInfoHead, &dataObjInp.condInput, writeFlag);
    if (status < 0) {
        if (lockFd > 0) {
            unlock_data_obj();
        }
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Unable to select a data obj info matching the resource hierarchy from the keywords.";
        irods::log(ERROR(status, msg.str()));
        return status;
    }

    status = applyPreprocRuleForOpen( rsComm, &dataObjInp, &dataObjInfoHead );
    if (status < 0) {
        if (lockFd > 0) {
            unlock_data_obj();
        }
        return status;
    }

    if ( getStructFileType( dataObjInfoHead->specColl ) < 0 && writeFlag > 0 ) {
        char* rescName{};
        if ((rescName = getValByKey(&dataObjInp.condInput, DEST_RESC_NAME_KW)) ||
            (rescName = getValByKey(&dataObjInp.condInput, BACKUP_RESC_NAME_KW)) ||
            (rescName = getValByKey(&dataObjInp.condInput, DEF_RESC_NAME_KW))) {
            status = requeDataObjInfoByResc(&dataObjInfoHead, rescName, writeFlag, 1);
        }

        if ( status < 0 ) {
            if ( lockFd > 0 ) {
                unlock_data_obj();
            }
            freeAllDataObjInfo( dataObjInfoHead );
            return status;
        }
    }

    // Stage bundled data to cache directory, if necessary
    std::string resc_class{};
    irods::error prop_err = irods::get_resource_property<std::string>(
                                dataObjInfoHead->rescId, irods::RESOURCE_CLASS, resc_class);
    if (prop_err.ok() && resc_class == irods::RESOURCE_CLASS_BUNDLE) {
        status = stageBundledData(rsComm, &dataObjInfoHead);
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "%s: stageBundledData of %s failed stat=%d",
                     __FUNCTION__, dataObjInfoHead->objPath, status );
            freeAllDataObjInfo( dataObjInfoHead );
            if ( lockFd >= 0 ) {
                unlock_data_obj();
            }
            return status;
        }
    }

    dataObjInfo_t* tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo) {
        dataObjInfo_t* nextDataObjInfo = tmpDataObjInfo->next;
        tmpDataObjInfo->next = NULL;
        int l1descInx = open_with_obj_info(rsComm, dataObjInp, tmpDataObjInfo);

        if (l1descInx >= 0) {
            if (writeFlag > 0) {
                const auto old_replica_status = L1desc[l1descInx].replStatus;
                L1desc[l1descInx].openType = OPEN_FOR_WRITE_TYPE;
                status = change_replica_status_to_intermediate(rsComm, dataObjInp, tmpDataObjInfo);
                if (status < 0) {
                    if (const auto ec = close_replica(*rsComm, l1descInx); ec < 0) {
                        return ec;
                    }

                    return status;
                }

                try {
                    // Replica tokens only apply to write operations against intermediate replicas.
                    auto& rat = irods::experimental::replica_access_table::instance();

                    // There is a case where the client wants to open an existing replica for writes
                    // but does not have a replica token because the client is the first one to open
                    // the replica. "update" should be used when the replica is in an intermediate state.
                    if (rat.contains(tmpDataObjInfo->dataId, tmpDataObjInfo->replNum)) {
                        update_replica_access_table(*rsComm, update_operation::update, l1descInx, dataObjInp);
                    }
                    else {
                        update_replica_access_table(*rsComm, update_operation::create, l1descInx, dataObjInp);
                    }
                }
                catch (const irods::exception& e) {
                    rodsLog(LOG_ERROR, "Could not update replica access table for data object. "
                                       "Closing data object and setting replica status to it's original value. "
                                       "[error_code=%d, path=%s, exception=%s]",
                                       dataObjInp.objPath, e.code(), e.what());

                    if (const auto ec = close_replica(*rsComm, l1descInx); ec < 0) {
                        auto hier = ix::key_value_proxy{dataObjInp.condInput}[RESC_HIER_STR_KW].value();
                        rodsLog(LOG_ERROR, "Failed to close replica [error_code=%d, path=%s, hierarchy=%s]",
                                           ec, dataObjInp.objPath, hier.data());
                        return ec;
                    }

                    if (const auto ec = change_replica_status(*rsComm, dataObjInp, old_replica_status); ec < 0) {
                        auto hier = ix::key_value_proxy{dataObjInp.condInput}[RESC_HIER_STR_KW].value();
                        rodsLog(LOG_ERROR, "Failed to restore the replica's replica status "
                                           "[error_code=%d, path=%s, hierarchy=%s, original_replica_status=%d]",
                                           ec, dataObjInp.objPath, hier.data(), old_replica_status);
                        return ec;
                    }

                    return e.code();
                }
                catch (const std::exception& e) {
                    rodsLog(LOG_ERROR, "%s - %s", __FUNCTION__, e.what());
                    return SYS_INTERNAL_ERR;
                }
            }
            else {
                L1desc[l1descInx].openType = OPEN_FOR_READ_TYPE;
            }

            if ( lockFd >= 0 ) {
                L1desc[l1descInx].lockFd = lockFd;
            }

            return l1descInx;
        }

        status = l1descInx;
        tmpDataObjInfo = nextDataObjInfo;
    }

    return status;
} // _rsDataObjOpen

int rsDataObjOpen_impl(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp)
{
    if (!dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (has_trailing_path_separator(dataObjInp->objPath)) {
        return USER_INPUT_PATH_ERR;
    }

    if ((dataObjInp->openFlags & O_ACCMODE) == O_RDONLY && (dataObjInp->openFlags & O_TRUNC)) {
        return USER_INCOMPATIBLE_OPEN_FLAGS;
    }

    rodsServerHost_t* rodsServerHost{};
    int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (REMOTE_HOST == remoteFlag) {
        openStat_t* stat{};
        const int status = rcDataObjOpenAndStat(rodsServerHost->conn, dataObjInp, &stat);
        if (status < 0) {
            return status;
        }
        const int l1descInx = allocAndSetL1descForZoneOpr(status, dataObjInp, rodsServerHost, stat);
        if (stat) {
            free(stat);
        }
        return l1descInx;
    }

    using irods::experimental::key_value_proxy;

    key_value_proxy kvp{dataObjInp->condInput};

    // If the client specified a leaf resource, then discover the hierarchy and
    // store it in the keyValPair_t. This instructs the iRODS server to create
    // the replica at the specified resource if it does not exist.
    if (kvp.contains(LEAF_RESOURCE_NAME_KW)) {
        std::string hier;
        auto leaf = kvp[LEAF_RESOURCE_NAME_KW].value();
        bool is_coord_resc = false;

        if (const auto err = resc_mgr.is_coordinating_resource(leaf.data(), is_coord_resc); !err.ok()) {
            rodsLog(LOG_ERROR, err.result().data());
            return err.code();
        }

        // Leaf resources cannot be coordinating resources. This essentially checks
        // if the resource has any child resources which is exactly what we're interested in.
        if (is_coord_resc) {
            rodsLog(LOG_ERROR, "[%s] is not a leaf resource.", leaf.data());
            return USER_INVALID_RESC_INPUT;
        }

        if (const auto err = resc_mgr.get_hier_to_root_for_resc(leaf.data(), hier); !err.ok()) {
            rodsLog(LOG_ERROR, err.result().data());
            return err.code();
        }

        kvp[RESC_HIER_STR_KW] = hier;
    }

    enable_creation_of_additional_replicas(*rsComm);

    dataObjInfo_t* dataObjInfoHead{};

    if (!kvp.contains(RESC_HIER_STR_KW)) {
        try {
            const auto operation = (dataObjInp->openFlags & O_CREAT) ?
                irods::CREATE_OPERATION : irods::OPEN_OPERATION;
            auto [file_obj, hier] = irods::resolve_resource_hierarchy(operation, rsComm, *dataObjInp, &dataObjInfoHead);
            kvp[RESC_HIER_STR_KW] = hier;
            return _rsDataObjOpen(rsComm, *dataObjInp, dataObjInfoHead, file_obj);
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "[%s] - resolve_resource_hierarchy failed with [%d] when opening [%s]",
                    __FUNCTION__, e.code(), dataObjInp->objPath);
            return e.code();
        }
    }
    else {
        irods::file_object_ptr file_obj(new irods::file_object());
        irods::error fac_err = irods::file_object_factory(rsComm, dataObjInp, file_obj, &dataObjInfoHead);
        if (!fac_err.ok()) {
            irods::log(fac_err);
        }
        return _rsDataObjOpen(rsComm, *dataObjInp, dataObjInfoHead, file_obj);
    }
} // rsDataObjOpen_impl

} // anonymous namespace

int rsDataObjOpen(rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    namespace fs = ix::filesystem;

    const auto data_object_exists = fs::server::exists(*rsComm, dataObjInp->objPath);
    const auto fd = rsDataObjOpen_impl(rsComm, dataObjInp);

    constexpr auto minimum_valid_file_descriptor = 3;

    // Update the parent collection's mtime.
    if (fd >= minimum_valid_file_descriptor && !data_object_exists) {
        const auto parent_path = fs::path{dataObjInp->objPath}.parent_path();

        if (fs::server::is_collection_registered(*rsComm, parent_path)) {
            using std::chrono::system_clock;
            using std::chrono::time_point_cast;

            const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

            try {
                ix::scoped_privileged_client spc{*rsComm};
                fs::server::last_write_time(*rsComm, parent_path, mtime);
            }
            catch (const fs::filesystem_error& e) {
                rodsLog(LOG_ERROR, e.what());
                return e.code().value();
            }
        }
    }

    return fd;
}

