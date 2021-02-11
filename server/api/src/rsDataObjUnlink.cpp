#include "dataObjUnlink.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "rodsConnect.h"
#include "icatDefines.h"
#include "fileUnlink.h"
#include "unregDataObj.h"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rmColl.h"
#include "dataObjRename.h"
#include "subStructFileUnlink.h"
#include "modDataObjMeta.h"
#include "phyBundleColl.h"
#include "dataObjRepl.h"
#include "regDataObj.h"
#include "physPath.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsModDataObjMeta.hpp"
#include "rsDataObjRepl.hpp"
#include "rsSubStructFileUnlink.hpp"
#include "rsFileUnlink.hpp"
#include "rsDataObjRename.hpp"
#include "rsUnregDataObj.hpp"
#include "rsRmColl.hpp"
#include "rsRegDataObj.hpp"
#include "rcMisc.h"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_logger.hpp"
#include "scoped_privileged_client.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "boost/filesystem/path.hpp"
#include "fmt/format.h"

#include <string>
#include <string_view>
#include <chrono>

using logger = irods::experimental::log;

namespace
{
    namespace ir = irods::experimental::replica;
    namespace id = irods::experimental::data_object;

    int getNumSubfilesInBunfileObj(rsComm_t *rsComm, const char *objPath)
    {
        int status;
        genQueryOut_t *genQueryOut = NULL;
        genQueryInp_t genQueryInp;
        int totalRowCount;
        char condStr[MAX_NAME_LEN];

        bzero( &genQueryInp, sizeof( genQueryInp ) );
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

        // If a replica number was specified, a specific replica is being unlinked - find it.
        if (cond_input.contains(REPL_NUM_KW)) {
            auto obj = id::make_data_object_proxy(**_head);

            const auto repl_num = std::stoi(cond_input.at(REPL_NUM_KW).value().data());

            const auto& replica = id::find_replica(obj, repl_num);

            if (!replica) {
                THROW(SYS_REPLICA_DOES_NOT_EXIST, fmt::format(
                    "replica does not exist; path:[{}],repl_num:[{}]",
                    _inp.objPath, repl_num));
            }

            replica_ptr = replica->get();
        }

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

            if (cond_input.contains(REPL_NUM_KW)) {
                return SYS_CANT_MV_BUNDLE_DATA_BY_COPY;
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

        if (cond_input.contains(REPL_NUM_KW)) {
            auto op = id::make_data_object_proxy(**_head);

            auto replica = *id::find_replica(op, std::stoi(cond_input.at(REPL_NUM_KW).value().data()));

            return dataObjUnlinkS(&_comm, &_inp, replica.get());
        }

        auto op = id::make_data_object_proxy(**_head);

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

        // Deprecation messages must be handled by doing the following.
        // The native rule engine may erase all messages in the rError array.
        // The only way to guarantee that messages are received by the client
        // is to add them to the rError array when the function returns.
        irods::at_scope_exit<std::function<void()>> at_scope_exit{[&] {
            if (getValByKey(&dataObjUnlinkInp->condInput, REPL_NUM_KW)) {
                addRErrorMsg(&rsComm->rError, DEPRECATED_PARAMETER, "-n is deprecated.  Please use itrim instead.");
            }
        }};

        auto* recurse = getValByKey(&dataObjUnlinkInp->condInput, RECURSIVE_OPR__KW);
        auto* replica_number = getValByKey(&dataObjUnlinkInp->condInput, REPL_NUM_KW);
        if (recurse && replica_number) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        ruleExecInfo_t rei;
        int rmTrashFlag = 0;

        specCollCache_t *specCollCache{};
        resolveLinkedPath(rsComm, dataObjUnlinkInp->objPath, &specCollCache, &dataObjUnlinkInp->condInput);
        rodsServerHost_t* rodsServerHost{};
        int status = getAndConnRcatHost(rsComm, MASTER_RCAT, dataObjUnlinkInp->objPath, &rodsServerHost);
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
                std::stringstream msg;
                msg << "failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjUnlinkInp->objPath << "]";
                irods::log(PASS(irods::error(e)));
                // TODO: If the force flag is set, we just continue?
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
                    freeAllDataObjInfo(dataObjInfoHead);
                    return 0;
                }
            }
        }

        if (dataObjUnlinkInp->oprType == UNREG_OPR ||
            getValByKey(&dataObjUnlinkInp->condInput, FORCE_FLAG_KW) ||
            getValByKey(&dataObjUnlinkInp->condInput, REPL_NUM_KW) ||
            getValByKey(&dataObjUnlinkInp->condInput, EMPTY_BUNDLE_ONLY_KW) ||
            dataObjInfoHead->specColl || rmTrashFlag == 1) {
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
                freeAllDataObjInfo(dataObjInfoHead);
                return err.code();
            }

            if (NO_TRASH_CAN != rei.status) {
                status = rsMvDataObjToTrash(rsComm, *dataObjUnlinkInp, &dataObjInfoHead);
                freeAllDataObjInfo(dataObjInfoHead);
                return status;
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
        freeAllDataObjInfo(dataObjInfoHead);
        return status;
    } // rsDataObjUnlink_impl
} // anonymous namespace

int rsDataObjUnlink(rsComm_t* rsComm, dataObjInp_t* dataObjUnlinkInp)
{
    try {
        namespace fs = irods::experimental::filesystem;

        const auto ec = rsDataObjUnlink_impl(rsComm, dataObjUnlinkInp);
        const auto parent_path = fs::path{dataObjUnlinkInp->objPath}.parent_path();

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
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
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
    // Because this function may change the "oprType", we must make sure that
    // the "oprType" is restored to it's original value before leaving. This is
    // necessary because other replicas may not require adjustments to the "oprType"
    // (i.e. The registered vs non-registered replica case).
    irods::at_scope_exit restore_opr_type{[dataObjUnlinkInp, old_value = dataObjUnlinkInp->oprType] {
        dataObjUnlinkInp->oprType = old_value;
    }};

    // Verify if the replica is in a vault or not. If the replica is not in a vault,
    // then the server must not delete the replica. Instead, the replica must be unregistered
    // to avoid loss of data.
    {
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

    int status = 0;
    unregDataObj_t unregDataObjInp;

    if ( dataObjInfo->specColl == NULL ) {
        if (dataObjUnlinkInp->oprType == UNREG_OPR &&
            rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH)
        {
            ruleExecInfo_t rei;
            initReiWithDataObjInp( &rei, rsComm, dataObjUnlinkInp );
            rei.doi = dataObjInfo;
            rei.status = DO_CHK_PATH_PERM;

            // make resource properties available as rule session variables
            irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

            applyRule( "acSetChkFilePathPerm", NULL, &rei, NO_SAVE_REI );
            clearKeyVal(rei.condInputData);
            free(rei.condInputData);

            if (rei.status != NO_CHK_PATH_PERM) {
                // extract the host location from the resource hierarchy
                std::string location;
                irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
                if ( !ret.ok() ) {
                    irods::log( PASSMSG( "dataObjUnlinkS - failed in get_loc_for_hier_string", ret ) );
                    return ret.code();
                }

                rodsHostAddr_t addr;
                rodsServerHost_t *rodsServerHost = 0;

                memset( &addr, 0, sizeof( addr ) );
                rstrcpy( addr.hostAddr, location.c_str(), NAME_LEN );
                int remoteFlag = resolveHost( &addr, &rodsServerHost );
                if ( remoteFlag < 0 ) {
                    // error condition?
                }

                /* unregistering but not an admin user */
                std::string out_path;
                ret = resc_mgr.validate_vault_path( dataObjInfo->filePath, rodsServerHost, out_path );
                if ( ret.ok() ) {
                    /* in the vault */
                    std::stringstream msg;
                    msg << "unregistering a data object which is in a vault [";
                    msg << dataObjInfo->filePath << "]";
                    irods::log( PASSMSG( msg.str(), ret ) );
                    return CANT_UNREG_IN_VAULT_FILE;
                }
            }
        }

        unregDataObjInp.dataObjInfo = dataObjInfo;
        unregDataObjInp.condInput = &dataObjUnlinkInp->condInput;
        status = rsUnregDataObj( rsComm, &unregDataObjInp );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "dataObjUnlinkS: rsUnregDataObj error for %s. status = %d",
                     dataObjUnlinkInp->objPath, status );
            return status;
        }
    }

    if ( dataObjUnlinkInp->oprType != UNREG_OPR ) {
        // Set the in_pdmo flag
        char* in_pdmo = getValByKey( &dataObjUnlinkInp->condInput, IN_PDMO_KW );
        if ( in_pdmo != NULL ) {
            rstrcpy( dataObjInfo->in_pdmo, in_pdmo, MAX_NAME_LEN );
        }
        else {
            dataObjInfo->in_pdmo[0] = '\0';
        }

        status = l3Unlink( rsComm, dataObjInfo );
        if ( status < 0 ) {
            int myError = getErrno( status );
            rodsLog( LOG_NOTICE,
                     "dataObjUnlinkS: l3Unlink error for %s. status = %d",
                     dataObjUnlinkInp->objPath, status );
            /* allow ENOENT to go on and unregister */
            if ( myError != ENOENT && myError != EACCES ) {
                char orphanPath[MAX_NAME_LEN];
                int status1 = 0;
                rodsLog( LOG_NOTICE,
                         "dataObjUnlinkS: orphan file %s", dataObjInfo->filePath );
                while ( 1 ) {
                    if ( isOrphanPath( dataObjUnlinkInp->objPath ) ==
                            NOT_ORPHAN_PATH ) {
                        /* don't rename orphan path */
                        status1 = rsMkOrphanPath( rsComm, dataObjInfo->objPath,
                                                  orphanPath );
                        if ( status1 < 0 ) {
                            break;
                        }
                        /* reg the orphan path */
                        rstrcpy( dataObjInfo->objPath, orphanPath, MAX_NAME_LEN );
                    }
                    status1 = svrRegDataObj( rsComm, dataObjInfo );
                    if ( status1 == CAT_NAME_EXISTS_AS_DATAOBJ ||
                            status1 == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
                        continue;
                    }
                    else if ( status1 < 0 ) {
                        rodsLogError( LOG_ERROR, status1,
                                      "dataObjUnlinkS: svrRegDataObj of orphan %s error",
                                      dataObjInfo->objPath );
                    }
                    break;
                }
                return status;
            }
            else {
                status = 0;
            }
        }
    }

    return status;
}

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

