#include "irods/dataObjUnlink.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/rodsConnect.h"
#include "irods/icatDefines.h"
#include "irods/fileUnlink.h"
#include "irods/unregDataObj.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/specColl.hpp"
#include "irods/resource.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rmColl.h"
#include "irods/dataObjRename.h"
#include "irods/subStructFileUnlink.h"
#include "irods/modDataObjMeta.h"
#include "irods/dataObjRepl.h"
#include "irods/regDataObj.h"
#include "irods/physPath.hpp"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/rsDataObjRepl.hpp"
#include "irods/rsSubStructFileUnlink.hpp"
#include "irods/rsFileUnlink.hpp"
#include "irods/rsDataObjRename.hpp"
#include "irods/rsUnregDataObj.hpp"
#include "irods/rsRmColl.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rcMisc.h"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_logger.hpp"
#include "irods/scoped_privileged_client.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/irods_query.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/data_object_proxy.hpp"

#include "irods/logical_locking.hpp"

#include <boost/filesystem/path.hpp>
#include <fmt/format.h>

#include <cstring>
#include <string>
#include <string_view>
#include <chrono>

namespace logger = irods::experimental::log;

namespace
{
    namespace id = irods::experimental::data_object;
    namespace ir = irods::experimental::replica;
    namespace ill = irods::logical_locking;

    int getNumSubfilesInBunfileObj(rsComm_t *rsComm, const char *objPath)
    {
        int status;
        genQueryOut_t *genQueryOut = NULL;
        genQueryInp_t genQueryInp;
        int totalRowCount;
        char condStr[MAX_NAME_LEN];

        std::memset(&genQueryInp, 0, sizeof(genQueryInp));
        genQueryInp.maxRows = 1;
        genQueryInp.options = RETURN_TOTAL_ROW_COUNT;

        snprintf( condStr, MAX_NAME_LEN, "='%s'", objPath );
        addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );
        snprintf( condStr, MAX_NAME_LEN, "='%s'", BUNDLE_RESC_CLASS );
        addInxVal( &genQueryInp.sqlCondInp, COL_R_CLASS_NAME, condStr );
        addKeyVal( &genQueryInp.condInput, ZONE_KW, objPath );

        addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
        addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
        addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );

        status = rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
        if ( genQueryOut == NULL || status < 0 ) {
            freeGenQueryOut( &genQueryOut );
            clearGenQueryInp( &genQueryInp );
            if ( status == CAT_NO_ROWS_FOUND ) {
                return 0;
            }
            else {
                return status;
            }
        }
        totalRowCount = genQueryOut->totalRowCount;
        freeGenQueryOut( &genQueryOut );
        /* clear result */
        genQueryInp.maxRows = 0;
        rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
        clearGenQueryInp( &genQueryInp );

        return totalRowCount;
    } // getNumSubfilesInBunfileObj

int chkPreProcDeleteRule(
    rsComm_t* rsComm,
    dataObjInp_t& dataObjUnlinkInp,
    dataObjInfo_t* dataObjInfoHead) {

    ruleExecInfo_t rei{};
    initReiWithDataObjInp(&rei, rsComm, &dataObjUnlinkInp);
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);
    dataObjInfo_t* tmpDataObjInfo = dataObjInfoHead;
    int status = 0;
    while (tmpDataObjInfo) {
        /* have to go through the loop to test each copy (resource). */
        rei.doi = tmpDataObjInfo;

        // make resource properties available as rule session variables
        rei.condInputData = (keyValPair_t *)malloc(sizeof(keyValPair_t));
        memset(rei.condInputData, 0, sizeof(keyValPair_t));
        irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

        status = applyRule("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI );
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        if (status < 0 &&
            status != NO_MORE_RULES_ERR &&
            status != SYS_DELETE_DISALLOWED) {
            rodsLog(LOG_ERROR,
                    "%s: acDataDeletePolicy err for %s. stat = %d",
                    __FUNCTION__, dataObjUnlinkInp.objPath, status );
            return status;
        }

        if (rei.status == SYS_DELETE_DISALLOWED) {
            rodsLog(LOG_ERROR,
                    "%s:acDataDeletePolicy disallowed delete of %s",
                    __FUNCTION__, dataObjUnlinkInp.objPath );
            return rei.status;
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return status;
}

int rsMvDataObjToTrash(
    rsComm_t *rsComm,
    dataObjInp_t& dataObjInp,
    dataObjInfo_t **dataObjInfoHead ) {

    if (strstr((*dataObjInfoHead)->dataType, BUNDLE_STR)) {
        return SYS_CANT_MV_BUNDLE_DATA_TO_TRASH;
    }

    char* accessPerm{};
    if (!getValByKey(&dataObjInp.condInput, ADMIN_KW)) {
        accessPerm = ACCESS_DELETE_OBJECT;
    }
    else if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }

    freeAllDataObjInfo(*dataObjInfoHead);

    int status = getDataObjInfo(rsComm, &dataObjInp, dataObjInfoHead, accessPerm, 1);
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s: getDataObjInfo error for %s. status = %d",
                 __FUNCTION__, dataObjInp.objPath, status );
        return status;
    }

    status = chkPreProcDeleteRule(rsComm, dataObjInp, *dataObjInfoHead);
    if ( status < 0 ) {
        return status;
    }

    if (const auto ret = ill::try_lock(**dataObjInfoHead, ill::lock_type::write); ret < 0) {
        irods::log(LOG_NOTICE, fmt::format(
            "[{}:{}] - move to trash not allowed because data object is locked"
            "[error code=[{}], logical path=[{}]",
            __FUNCTION__, __LINE__, ret, (*dataObjInfoHead)->objPath));

        return ret;
    }

    char trashPath[MAX_NAME_LEN]{};
    status = rsMkTrashPath(rsComm, dataObjInp.objPath, trashPath);
    if ( status < 0 ) {
        return status;
    }

    dataObjCopyInp_t dataObjRenameInp{};
    dataObjRenameInp.srcDataObjInp.oprType =
        dataObjRenameInp.destDataObjInp.oprType = RENAME_DATA_OBJ;

    rstrcpy(dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy(dataObjRenameInp.srcDataObjInp.objPath, dataObjInp.objPath, MAX_NAME_LEN);

    status = rsDataObjRename(rsComm, &dataObjRenameInp);
    while (status == CAT_NAME_EXISTS_AS_DATAOBJ ||
           status == CAT_NAME_EXISTS_AS_COLLECTION ||
           status == SYS_PHY_PATH_INUSE ||
           getErrno( status ) == EISDIR) {
        appendRandomToPath(dataObjRenameInp.destDataObjInp.objPath);
        status = rsDataObjRename( rsComm, &dataObjRenameInp );
    }
    if (status < 0) {
        rodsLog(LOG_ERROR,
                "%s: rcDataObjRename error for %s, status = %d",
                __FUNCTION__, dataObjRenameInp.destDataObjInp.objPath, status );
    }
    return status;
}

    int _rsDataObjUnlink(RsComm& _comm, DataObjInp& _inp, DataObjInfo** _head)
    {
        if (!_head || !*_head) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - passed-in data object information list is null",
                __FUNCTION__, __LINE__));

            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        DataObjInfo* replica_ptr = *_head;

        auto replica = ir::make_replica_proxy(*replica_ptr);

        // Determine the policy for data deletion
        int status = chkPreProcDeleteRule(&_comm, _inp, replica.get());
        if ( status < 0 ) {
            return status;
        }

        // Handle bundled data object
        if (replica.type() == BUNDLE_STR) {
            if (_comm.proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
                return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
            }

            if (const int numSubfiles = getNumSubfilesInBunfileObj(&_comm, replica.logical_path().data()); numSubfiles > 0) {
                if (cond_input.contains(EMPTY_BUNDLE_ONLY_KW)) {
                    // There is nothing to do if only empty bundles are allowed
                    return 0;
                }

                auto* info_head = replica.get();
                status = _unbunAndStageBunfileObj(&_comm, &info_head, cond_input.get(), NULL, 1);

                // Unlink the object here if the file does not exist or fails to untar
                if (status < 0 && EEXIST != getErrno(status) &&
                    SYS_TAR_STRUCT_FILE_EXTRACT_ERR != getIrodsErrno(status)) {

                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - _unbunAndStageBunfileObj err for {}",
                        __FUNCTION__, __LINE__, replica.logical_path()));

                    return status;
                }

                /* _head may be outdated */
                *_head = NULL;
                status = getDataObjInfoIncSpecColl(&_comm, &_inp, _head);
                if ( status < 0 ) {
                    return status;
                }
            }
        }

        auto op = id::make_data_object_proxy(**_head);

        if (const auto ret = ill::try_lock(*op.get(), ill::lock_type::write); ret < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - unlink not allowed because data object is locked"
                "[error code=[{}], logical path=[{}]]",
                __FUNCTION__, __LINE__, ret, replica.logical_path()));

            return ret;
        }

        int retVal = 0;

        for (auto& r : op.replicas()) {
            if (const int status = dataObjUnlinkS(&_comm, &_inp, r.get());
                status < 0 && retVal == 0 ) {
                retVal = status;
            }

            if (_inp.specColl) {
                break;
            }
        }

        return retVal;
    } // _rsDataObjUnlink

    int rsDataObjUnlink_impl(rsComm_t* rsComm, dataObjInp_t* dataObjUnlinkInp)
    {
        if (!dataObjUnlinkInp) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        ruleExecInfo_t rei;
        int rmTrashFlag = 0;

        specCollCache_t *specCollCache{};
        resolveLinkedPath(rsComm, dataObjUnlinkInp->objPath, &specCollCache, &dataObjUnlinkInp->condInput);
        rodsServerHost_t* rodsServerHost{};
        int status = getAndConnRcatHost(rsComm, PRIMARY_RCAT, dataObjUnlinkInp->objPath, &rodsServerHost);
        if (status < 0 || !rodsServerHost) { // JMC cppcheck - nullptr
            return status;
        }
        else if (rodsServerHost->rcatEnabled == REMOTE_ICAT) {
            return rcDataObjUnlink(rodsServerHost->conn, dataObjUnlinkInp);
        }

        // determine the resource hierarchy if one is not provided
        if (!getValByKey(&dataObjUnlinkInp->condInput, RESC_HIER_STR_KW)) {
            try {
                auto result = irods::resolve_resource_hierarchy(irods::UNLINK_OPERATION, rsComm, *dataObjUnlinkInp);
                const auto hier = std::get<std::string>(result);
                addKeyVal(&dataObjUnlinkInp->condInput, RESC_HIER_STR_KW, hier.c_str());
            }
            catch (const irods::exception& e) {
                irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));

                // Continuation is intentional. See irods/irods#3154.
                if(!getValByKey(&dataObjUnlinkInp->condInput, FORCE_FLAG_KW)) {
                    return e.code();
                }
            }
        }

        if (getValByKey(&dataObjUnlinkInp->condInput, ADMIN_RMTRASH_KW) ||
            getValByKey(&dataObjUnlinkInp->condInput, RMTRASH_KW))
        {
            if ( isTrashPath( dataObjUnlinkInp->objPath ) == False ) {
                return SYS_INVALID_FILE_PATH;
            }

            rmTrashFlag = 1;
        }

        dataObjInfo_t *dataObjInfoHead{};
        irods::at_scope_exit free_data_obj_infos{[&dataObjInfoHead] { freeAllDataObjInfo(dataObjInfoHead); }};

        dataObjUnlinkInp->openFlags = O_WRONLY;
        status = getDataObjInfoIncSpecColl(rsComm, dataObjUnlinkInp, &dataObjInfoHead);
        if ( status < 0 ) {
            char* sys_error = NULL;
            const char* rods_error = rodsErrorName( status, &sys_error );
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get data objects.";
            msg << " - " << rods_error << " " << sys_error;
            irods::error result = ERROR( status, msg.str() );
            irods::log( result );
            free( sys_error );
            return status;
        }

        if ( rmTrashFlag == 1 ) {
            const char* age_kw = getValByKey(&dataObjUnlinkInp->condInput, AGE_KW);
            if (age_kw) {
                const int age_limit = std::atoi(age_kw) * 60;
                const auto age = time(0) - std::atoi(dataObjInfoHead->dataModify);
                if (age < age_limit) {
                    /* younger than ageLimit. Nothing to do */
                    return 0;
                }
            }
        }

        // clang-format off
        if (dataObjUnlinkInp->oprType == UNREG_OPR ||
            getValByKey(&dataObjUnlinkInp->condInput, FORCE_FLAG_KW) ||
            getValByKey(&dataObjUnlinkInp->condInput, EMPTY_BUNDLE_ONLY_KW) ||
            dataObjInfoHead->specColl ||
            rmTrashFlag == 1)
        // clang-format on
        {
            status = _rsDataObjUnlink(*rsComm, *dataObjUnlinkInp, &dataObjInfoHead);
        }
        else {
            initReiWithDataObjInp(&rei, rsComm, dataObjUnlinkInp);
            status = applyRule("acTrashPolicy", NULL, &rei, NO_SAVE_REI);
            clearKeyVal(rei.condInputData);
            free(rei.condInputData);
            if (status < 0) {
                if (rei.status < 0) {
                    status = rei.status;
                }
                const auto err{ERROR(status, "acTrashPolicy failed")};
                irods::log(err);
                return err.code();
            }

            if (NO_TRASH_CAN != rei.status) {
                return rsMvDataObjToTrash(rsComm, *dataObjUnlinkInp, &dataObjInfoHead);
            }
            else {
                status = _rsDataObjUnlink(*rsComm, *dataObjUnlinkInp, &dataObjInfoHead );
            }
        }

        initReiWithDataObjInp( &rei, rsComm, dataObjUnlinkInp );
        rei.doi = dataObjInfoHead;
        rei.status = status;

        // make resource properties available as rule session variables
        irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

        rei.status = applyRule("acPostProcForDelete", NULL, &rei, NO_SAVE_REI);
        if ( rei.status < 0 ) {
            rodsLog(LOG_NOTICE,
                    "%s: acPostProcForDelete error for %s. status = %d",
                    __FUNCTION__, dataObjUnlinkInp->objPath, rei.status );
        }

        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        return status;
    } // rsDataObjUnlink_impl

    auto get_path_permissions_check_setting(RsComm& _comm, DataObjInp& _inp, DataObjInfo& _info) -> int
    {
        RuleExecInfo rei;
        initReiWithDataObjInp(&rei, &_comm, &_inp);
        rei.doi = &_info;
        rei.status = DO_CHK_PATH_PERM;

        // make resource properties available as rule session variables
        irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

        applyRule("acSetChkFilePathPerm", NULL, &rei, NO_SAVE_REI);
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        return rei.status;
    } // get_path_permissions_check_setting
} // anonymous namespace

int rsDataObjUnlink(rsComm_t* rsComm, dataObjInp_t* dataObjUnlinkInp)
{
    try {
        namespace fs = irods::experimental::filesystem;

        const auto ec = rsDataObjUnlink_impl(rsComm, dataObjUnlinkInp);
        const auto parent_path = fs::path{dataObjUnlinkInp->objPath}.parent_path();

        // Do not update the collection mtime if the logical path refers to a remote zone. If the remote zone cares
        // about updating the mtime, the remote API endpoint called above should have updated the collection mtime
        // already, so it is unnecessary. Failing to prevent the remote collection mtime update can lead to errors
        // being returned even though the operation was successful and the collection mtime is the expected value.
        if (const auto zone_hint = fs::zone_name(parent_path); zone_hint && !isLocalZone(zone_hint->data())) {
            return ec;
        }

        // Update the parent collection's mtime.
        if (ec == 0 && fs::server::is_collection_registered(*rsComm, parent_path)) {
            using std::chrono::system_clock;
            using std::chrono::time_point_cast;

            const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

            try {
                irods::experimental::scoped_privileged_client spc{*rsComm};
                fs::server::last_write_time(*rsComm, parent_path, mtime);
            }
            catch (const fs::filesystem_error& e) {
                logger::api::error(e.what());
                return e.code().value();
            }
        }

        return ec;
    }
    catch (const irods::exception& e) {
        irods::log(LOG_ERROR, fmt::format(
            "[{}:{}] - [{}]",
            __FUNCTION__, __LINE__, e.client_display_what()));
        return e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
        return SYS_INTERNAL_ERR;
    }
    catch (...)
    {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - an unknown error occurred", __FUNCTION__, __LINE__));
        return SYS_UNKNOWN_ERROR;
    }
} // rsDataObjUnlink

int dataObjUnlinkS(rsComm_t* rsComm,
                   dataObjInp_t* dataObjUnlinkInp,
                   dataObjInfo_t* dataObjInfo)
{
    auto info = ir::make_replica_proxy(*dataObjInfo);

    // Because this function may change the "oprType", we must make sure that
    // the "oprType" is restored to its original value before leaving. This is
    // necessary because other replicas may not require adjustments to the "oprType"
    // (i.e. The registered vs non-registered replica case).
    irods::at_scope_exit restore_opr_type{[dataObjUnlinkInp, old_value = dataObjUnlinkInp->oprType] {
        dataObjUnlinkInp->oprType = old_value;
    }};

    // Verify if the replica is in a vault or not. If the replica is not in a vault,
    // then the server must not delete the replica. Instead, the replica must be unregistered
    // to avoid loss of data.
    if (!dataObjInfo->specColl) {
        bool skip_vault_path_check = false;
        irods::error err = irods::get_resource_property<bool>(dataObjInfo->rescId,
                                                              irods::RESOURCE_SKIP_VAULT_PATH_CHECK_ON_UNLINK,
                                                              skip_vault_path_check);
        if (!err.ok()) {
            if (err.code() != KEY_NOT_FOUND) {
                rodsLog(LOG_NOTICE, "lookup RESOURCE_SKIP_VAULT_PATH_CHECK_ON_UNLINK returned error status=%d msg=%s",
                        err.code(), err.result().c_str());
            }
            skip_vault_path_check = false;
        }

        if (!skip_vault_path_check) {
            std::string vault_path;
            if (const auto err = irods::get_vault_path_for_hier_string(dataObjInfo->rescHier, vault_path); !err.ok()) {
                return err.code();
            }

            if (!has_prefix(dataObjInfo->filePath, vault_path.data())) {
                dataObjUnlinkInp->oprType = UNREG_OPR;

                logger::api::info("Replica is not in a vault. Unregistering replica and leaving it on "
                                  "disk as-is [data_object={}, physical_object={}, vault_path={}].",
                                  dataObjUnlinkInp->objPath,
                                  dataObjInfo->filePath,
                                  vault_path);
            }
        }
    }

    // Ensure that the authenticated user has permission to perform a pure unregister on the replica.
    // Non-admin users do not have permission to unregister replicas from the catalog without unlinking
    // the data if the replica in question resides inside of an iRODS resource vault. Data objects in
    // special collections do not apply to the unregistering piece of the operation.  If the operation
    // is not configured to check the permissions on the path, skip this step.
    if (!dataObjInfo->specColl &&
        UNREG_OPR == dataObjUnlinkInp->oprType &&
        LOCAL_PRIV_USER_AUTH != rsComm->clientUser.authInfo.authFlag &&
        NO_CHK_PATH_PERM != get_path_permissions_check_setting(*rsComm, *dataObjUnlinkInp, *info.get()))
    {
        std::string location;
        if (const auto ret = irods::get_loc_for_hier_string(info.hierarchy().data(), location); !ret.ok()) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed in get_loc_for_hier_string: [{}] "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ret.result(),
                ret.code(), info.logical_path(), info.hierarchy()));

            return ret.code();
        }

        RodsHostAddress addr{};
        rodsServerHost* host = nullptr;
        rstrcpy(addr.hostAddr, location.c_str(), NAME_LEN);
        if (const auto remote_flag = resolveHost(&addr, &host); remote_flag < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - failed in resolveHost [error_code=[{}]]",
                __FUNCTION__, __LINE__, remote_flag));

            return remote_flag;
        }

        // If the vault path is validated (ret.ok() is true), that indicates that the replica
        // resides in an iRODS resource vault. If the user is not a rodsadmin, unregistering
        // a replica from inside an iRODS resource vault is not allowed.
        std::string out_path;
        if (const auto ret = resc_mgr.validate_vault_path(info.physical_path().data(), host, out_path); ret.ok()) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - non-admin users cannot unregister a replica in a vault [path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, info.logical_path(), info.hierarchy()));

            return CANT_UNREG_IN_VAULT_FILE;
        }
    }

    // The UNREG_OPR type is used to indicate that the replica should not be physically unlinked,
    // but only unregistered. Therefore, only attempt physical unlink if this is not an UNREG_OPR.
    if (UNREG_OPR != dataObjUnlinkInp->oprType) {
        const auto cond_input = irods::experimental::make_key_value_proxy(dataObjUnlinkInp->condInput);
        info.in_pdmo(cond_input.contains(IN_PDMO_KW) ? cond_input.at(IN_PDMO_KW).value() : "");

        if (const auto ec = l3Unlink(rsComm, info.get()); ec < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - failed to physically unlink replica "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ec, info.logical_path(), info.hierarchy()));

            // TODO: why do we allow NOENT and EACCES to unregister?
            if (const auto error_number = getErrno(ec);
                ENOENT != error_number && EACCES != error_number) {
                return ec;
            }
        }
    }

    // Special collections have their own logic regarding registration which is not handled
    // in this implementation. Just return success.
    if (dataObjInfo->specColl) {
        return 0;
    }

    unregDataObj_t unreg_inp{};
    unreg_inp.dataObjInfo = info.get();
    unreg_inp.condInput = &dataObjUnlinkInp->condInput;
    if (const auto ec = rsUnregDataObj(rsComm, &unreg_inp); ec < 0) {
        irods::log(LOG_NOTICE, fmt::format(
            "[{}:{}] - rsUnregDataObj failed "
            "[error_code=[{}], path=[{}], hierarchy=[{}]",
            __FUNCTION__, __LINE__, ec, info.logical_path(), info.hierarchy()));

        // TODO: Should the replica be marked stale if the unregister itself fails?

        return ec;
    }

    return 0;
} // dataObjUnlinkS

int l3Unlink(
    rsComm_t *rsComm,
    dataObjInfo_t *dataObjInfo ) {
    std::string resc_class;
    irods::error prop_err = irods::get_resource_property<std::string>(
                                dataObjInfo->rescId,
                                irods::RESOURCE_CLASS,
                                resc_class );
    if (!prop_err.ok() ) {
        std::stringstream msg;
        msg << "failed to get property [class] for resource [";
        msg << dataObjInfo->rescName;
        msg << "]";
        irods::log( PASSMSG( msg.str(), prop_err ) );
        return prop_err.code();
    }
    else if (resc_class == irods::RESOURCE_CLASS_BUNDLE) {
        return 0;
    }

    std::string location{};
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Unlink - failed in get_loc_for_hier_string", ret ) );
        return ret.code();
    }

    irods::error resc_err = irods::is_hier_live(dataObjInfo->rescHier);
    if (!resc_err.ok()) {
        return resc_err.code();
    }

    if (getStructFileType(dataObjInfo->specColl) >= 0) {
        subFile_t subFile{};
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        return rsSubStructFileUnlink( rsComm, &subFile );
    }

    fileUnlinkInp_t fileUnlinkInp{};
    rstrcpy( fileUnlinkInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( fileUnlinkInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileUnlinkInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( fileUnlinkInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
    rstrcpy( fileUnlinkInp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );
    return rsFileUnlink( rsComm, &fileUnlinkInp );
}

