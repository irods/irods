#include "rsDataObjChksum.hpp"

#include "dataObjChksum.h"
#include "dataObjOpr.hpp"
#include "getRemoteZoneResc.h"
#include "modDataObjMeta.h"
#include "objInfo.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rcMisc.h"
#include "resource.hpp"
#include "rodsError.h"
#include "rodsErrorTable.h"
#include "rsFileStat.hpp"
#include "rsModDataObjMeta.hpp"
#include "specColl.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "key_value_proxy.hpp"
#include "irods_rs_comm_query.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_logger.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

#include "boost/lexical_cast.hpp"
#include "fmt/format.h"
#include <nlohmann/json.hpp>

#include <cstring>
#include <optional>
#include <algorithm>
#include <vector>
#include <iterator>
#include <string>
#include <string_view>

namespace
{
    namespace ix = irods::experimental;
    
    using json = nlohmann::json;

    auto to_malloc_allocated_string(const json& _json) -> char*
    {
        const auto s = _json.dump();

        // Allocate enough space for the string and the null-byte.
        const auto size = s.size() + 1;

        auto* t = static_cast<char*>(std::malloc(sizeof(char) * size));
        std::memset(t, 0, size);

        return std::strcpy(t, s.data());
    } // to_malloc_allocated_string

    auto add_verification_result(json& _vresults,
                                 const std::string_view _severity,
                                 int _error_code,
                                 const std::string_view _message) -> void
    {
        _vresults.push_back(json{
            {"severity", _severity},
            {"error_code", _error_code},
            {"message", _message}
        });
    }

    using log = irods::experimental::log;

    // Assumes zone redirection has already occurred, and that the function is
    // being called on a server in the zone the object belongs to.
    int verifyVaultSizeEqualsDatabaseSize(RsComm& rsComm, DataObjInfo& dataObjInfo)
    {
        fileStatInp_t fileStatInp;
        memset(&fileStatInp, 0, sizeof(fileStatInp));
        rstrcpy(fileStatInp.objPath, dataObjInfo.objPath, sizeof(fileStatInp.objPath));
        rstrcpy(fileStatInp.rescHier, dataObjInfo.rescHier, sizeof(fileStatInp.rescHier));
        rstrcpy(fileStatInp.fileName, dataObjInfo.filePath, sizeof(fileStatInp.fileName));
        rodsStat_t *fileStatOut = nullptr;
        const int status_rsFileStat = rsFileStat(&rsComm, &fileStatInp, &fileStatOut);
        if (status_rsFileStat < 0) {
            rodsLog(LOG_ERROR, "verifyVaultSizeEqualsDatabaseSize: rsFileStat of objPath [%s] rescHier [%s] fileName [%s] failed with [%d]",
                    fileStatInp.objPath, fileStatInp.rescHier, fileStatInp.fileName, status_rsFileStat);
            return status_rsFileStat;
        }

        const rodsLong_t size_in_vault = fileStatOut->st_size;
        free(fileStatOut);
        std::optional<rodsLong_t> size_in_database {};
        auto select_and_condition { fmt::format("select DATA_SIZE where DATA_PATH = '{}'", dataObjInfo.filePath) };
        for (auto&& row : irods::query{&rsComm, select_and_condition}) {
            try {
                size_in_database = boost::lexical_cast<rodsLong_t>(row[0]);
            }
            catch (const boost::bad_lexical_cast&) {
                log::api::error("_rsFileChksum: lexical_cast of [{}] for [{}] [{}] [{}] failed",
                    row[0],
                    dataObjInfo.filePath,
                    dataObjInfo.rescHier,
                    dataObjInfo.objPath);
                return INVALID_LEXICAL_CAST;
            }
        }

        if (!size_in_database.has_value()) { return CAT_NO_ROWS_FOUND; }
        if (size_in_database.value() != size_in_vault) {
            rodsLog(LOG_ERROR, "_rsFileChksum: file size mismatch. resource hierarchy [%s] vault path [%s] size [%ji] object path [%s] size [%ji]",
                    dataObjInfo.rescHier, dataObjInfo.filePath,
                    static_cast<intmax_t>(size_in_vault), dataObjInfo.objPath, static_cast<intmax_t>(size_in_database.value()));
            return USER_FILE_SIZE_MISMATCH;
        }

        return 0;
    } // verifyVaultSizeEqualsDatabaseSize

    bool is_replica_good(const DataObjInfo& _replica) noexcept
    {
        return _replica.replStatus == GOOD_REPLICA;
    } // is_replica_good

    bool is_member_of_bundle_resource(rodsLong_t _resource_id)
    {
        std::string resc_class;
        const auto err = irods::get_resource_property(_resource_id, irods::RESOURCE_CLASS, resc_class);
        if (!err.ok()) {
            log::api::error(err.result());
        }

        return irods::RESOURCE_CLASS_BUNDLE == resc_class;
    } // is_member_of_bundle_resource

    int check_replica_accessibility(const DataObjInfo& _replica)
    {
        if (is_member_of_bundle_resource(_replica.rescId)) {
            return SYS_CANT_CHKSUM_BUNDLED_DATA;
        }

        const auto l = {GOOD_REPLICA, STALE_REPLICA};

        if (std::none_of(begin(l), end(l), [&_replica](int _x) { return _replica.replStatus == _x; })) {
            return SYS_REPLICA_INACCESSIBLE;
        }

        return 0;
    }

    std::vector<DataObjInfo*> get_good_replicas(DataObjInfo& _replicas)
    {
        std::vector<DataObjInfo*> good_replicas;
        good_replicas.reserve(std::distance(begin(&_replicas), end(&_replicas)));

        for (auto& r : &_replicas) {
            if (is_replica_good(r) && !is_member_of_bundle_resource(r.rescId)) {
                good_replicas.push_back(&r);
            }
        }

        good_replicas.shrink_to_fit();

        return good_replicas;
    } // get_good_replicas

    void report_number_of_replicas_skipped(const DataObjInfo& _replicas,
                                           std::size_t _good_replicas,
                                           json& _verification_results)
    {
        // "_good_replicas" should always be less than or equal to the total number of replicas.
        const auto count = std::distance(begin(&_replicas), end(&_replicas)) - _good_replicas;

        if (count > 0) {
            const auto msg = fmt::format("INFO: Number of replicas skipped: {}", count);
            add_verification_result(_verification_results, "info", 0, msg);
        }
    } // report_number_of_replicas_skipped

    bool report_replicas_without_checksums(const std::vector<DataObjInfo*>& _replicas,
                                           json& _verification_results)
    {
        bool found = false;

        for (const auto* r : _replicas) {
            if (std::strlen(r->chksum) == 0) {
                found = true;
                const auto msg = fmt::format("WARNING: No checksum available for replica [{}].", r->replNum);
                add_verification_result(_verification_results, "warning", CAT_NO_CHECKSUM_FOR_REPLICA, msg);
            }
        }

        return found;
    } // report_replicas_without_checksums

    void report_replica_if_mismatch_size_info(RsComm& _comm,
                                              DataObjInfo& _replica,
                                              json& _verification_results)
    {
        if (const auto ec = verifyVaultSizeEqualsDatabaseSize(_comm, _replica); ec < 0) {
            std::string msg;

            if (ec == USER_FILE_SIZE_MISMATCH) {
                msg = fmt::format("ERROR: Physical size does not match size in catalog for replica [{}].", _replica.replNum);
            }
            else {
                msg = fmt::format("ERROR: Could not verify size for replica [{}].", _replica.replNum);
            }

            add_verification_result(_verification_results, "error", ec, msg);
        }
    } // report_replica_if_mismatch_size_info

    void report_replicas_with_mismatch_size_info(RsComm& _comm,
                                                 const std::vector<DataObjInfo*>& _replicas,
                                                 json& _verification_results)
    {
        for (auto* r : _replicas) {
            report_replica_if_mismatch_size_info(_comm, *r, _verification_results);
        }
    } // report_replicas_with_mismatch_size_info

    void report_replica_if_mismatch_checksum_info(RsComm& _comm,
                                                  DataObjInfo& _replica,
                                                  json& _verification_results)
    {
        char* ignore_checksum{};

        if (const auto ec = verifyDataObjChksum(&_comm, &_replica, &ignore_checksum); ec < 0) {
            std::string msg;

            if (ec == USER_CHKSUM_MISMATCH) {
                msg = fmt::format("ERROR: Computed checksum does not match what is in the catalog for replica [{}].", _replica.replNum);
            }
            else {
                msg = fmt::format("ERROR: Could not verify checksum for replica [{}].", _replica.replNum);
            }

            add_verification_result(_verification_results, "error", ec, msg);
        }
    }

    int do_verification(RsComm& _comm,
                        DataObjInp& _dataObjInp,
                        DataObjInfo* _replicas,
                        char** _checksum)
    {
        ix::key_value_proxy kvp{_dataObjInp.condInput};

        if (kvp.contains(FORCE_CHKSUM_KW)) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        const auto client_set_replica_number = kvp.contains(REPL_NUM_KW);
        const auto client_set_resource_name = kvp.contains(RESC_NAME_KW);

        if (client_set_replica_number && client_set_resource_name) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        const auto client_targeted_specific_replica = (client_set_replica_number || client_set_resource_name);
        const auto client_set_all_flag = kvp.contains(CHKSUM_ALL_KW);

        if (client_set_all_flag && client_targeted_specific_replica) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        const auto client_set_admin_flag = kvp.contains(ADMIN_KW);

        // Verify that the client is allowed to use the administrative flag.
        if (client_set_admin_flag && !irods::is_privileged_client(_comm)) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        if (const auto ec = getDataObjInfoIncSpecColl(&_comm, &_dataObjInp, &_replicas); ec < 0) {
            return ec;
        }

        if (client_targeted_specific_replica) {
            // The client is allowed to verify checksum information for a single stale replica.
            // Targeting a specific replica supports verifying the following:
            // - The existence of a checksum for that replica.
            // - The physical size of the replicas vs what is recorded in the catalog.
            if (const auto ec = sortObjInfoForOpen(&_replicas, &_dataObjInp.condInput, /* write flag */ 0); ec < 0) {
                return ec;
            }

            if (const auto ec = check_replica_accessibility(*_replicas); ec < 0) {
                return ec;
            }

            auto verification_results = json::array();

            if (client_set_admin_flag) {
                ix::key_value_proxy{_replicas->condInput}[ADMIN_KW] = "";
            }

            report_replica_if_mismatch_size_info(_comm, *_replicas, verification_results);

            if (std::strlen(_replicas->chksum) == 0) {
                const auto msg = fmt::format("WARNING: No checksum available for replica [{}].", _replicas->replNum);
                add_verification_result(verification_results, "warning", CAT_NO_CHECKSUM_FOR_REPLICA, msg);
                *_checksum = to_malloc_allocated_string(verification_results);

                // Return immediately because this function requires that the replica
                // have a checksum after this point.
                return CHECK_VERIFICATION_RESULTS;
            }

            //
            // At this point, the replica is guaranteed to have a checksum.
            //

            if (!kvp.contains(NO_COMPUTE_KW)) {
                report_replica_if_mismatch_checksum_info(_comm, *_replicas, verification_results);
            }

            if (!verification_results.empty()) {
                *_checksum = to_malloc_allocated_string(verification_results);
                return CHECK_VERIFICATION_RESULTS;
            }

            return 0;
        }

        //
        // Process ALL replicas marked good and not a member of the bundle resource.
        //

        auto good_replicas = get_good_replicas(*_replicas);

        if (good_replicas.empty()) {
            return SYS_NO_GOOD_REPLICA;
        }

        auto verification_results = json::array();

        report_number_of_replicas_skipped(*_replicas, good_replicas.size(), verification_results);
        report_replicas_with_mismatch_size_info(_comm, good_replicas, verification_results);

        if (report_replicas_without_checksums(good_replicas, verification_results)) {
            // Return immediately because this function requires that all replicas
            // have a checksum after this point.

            if (!verification_results.empty()) {
                *_checksum = to_malloc_allocated_string(verification_results);
                return CHECK_VERIFICATION_RESULTS;
            }

            return 0;
        }

        //
        // At this point, all replicas are guaranteed to have a checksum.
        //

        if (!kvp.contains(NO_COMPUTE_KW)) {
            // Verify that the checksum in the catalog for each replica is correct.
            // This operation could be time consuming because a checksum will need to be
            // computed for each replica.
            for (auto* r : good_replicas) {
                if (client_set_admin_flag) {
                    ix::key_value_proxy{r->condInput}[ADMIN_KW] = "";
                }

                report_replica_if_mismatch_checksum_info(_comm, *r, verification_results);
            }
        }

        // Verify that all good replicas share the same checksum.
        const auto mismatch_checksums = [checksum = good_replicas[0]->chksum](DataObjInfo* _r) {
            return std::strcmp(_r->chksum, checksum) != 0;
        };

        if (std::any_of(std::next(begin(good_replicas)), end(good_replicas), mismatch_checksums)) {
            const std::string_view msg = "WARNING: Data object has replicas with different checksums.";
            add_verification_result(verification_results, "warning", USER_CHKSUM_MISMATCH, msg);
        }

        if (!verification_results.empty()) {
            *_checksum = to_malloc_allocated_string(verification_results);
            return CHECK_VERIFICATION_RESULTS;
        }

        return 0;
    } // do_verification

    int do_lookup_or_update(RsComm& _comm,
                            DataObjInp& _dataObjInp,
                            DataObjInfo* _replicas,
                            char** _computed_checksum)
    {
        ix::key_value_proxy kvp{_dataObjInp.condInput};

        const auto client_set_replica_number = kvp.contains(REPL_NUM_KW);
        const auto client_set_resource_name = kvp.contains(RESC_NAME_KW);

        if (client_set_replica_number && client_set_resource_name) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        const auto client_targeted_specific_replica = (client_set_replica_number || client_set_resource_name);
        const auto client_set_all_flag = kvp.contains(CHKSUM_ALL_KW);

        if (client_set_all_flag && client_targeted_specific_replica) {
            return USER_INCOMPATIBLE_PARAMS;
        }

        const auto client_set_admin_flag = kvp.contains(ADMIN_KW);

        // Verify that the client is allowed to use the administrative flag.
        if (client_set_admin_flag && !irods::is_privileged_client(_comm)) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        if (const auto ec = getDataObjInfoIncSpecColl(&_comm, &_dataObjInp, &_replicas); ec < 0) {
            return ec;
        }

        if (!client_set_all_flag || client_targeted_specific_replica) {
            if (const auto ec = sortObjInfoForOpen(&_replicas, &_dataObjInp.condInput, /* write flag */ 0); ec < 0) {
                return ec;
            }

            if (const auto ec = check_replica_accessibility(*_replicas); ec < 0) {
                return ec;
            }

            // Return the existing checksum if the client did not set the force flag and the
            // replica has a checksum.
            if (!kvp.contains(FORCE_CHKSUM_KW) && std::strlen(_replicas->chksum) > 0) {
                *_computed_checksum = strdup(_replicas->chksum);
                return 0;
            }

            if (client_set_admin_flag) {
                ix::key_value_proxy{_replicas->condInput}[ADMIN_KW] = "";
            }

            return dataObjChksumAndRegInfo(&_comm, _replicas, _computed_checksum);
        }

        //
        // Process ALL replicas marked good and not a member of the bundle resource.
        //

        const auto good_replicas = get_good_replicas(*_replicas);

        if (good_replicas.empty()) {
            return SYS_NO_GOOD_REPLICA;
        }

        bool replicas_share_identical_checksum = true;
        std::string previous_checksum;

        // Compute and update the checksum for each replica. A checksum will be recomputed and
        // updated if the replica does not have a checksum or the client passed the force flag.
        for (auto* r : good_replicas) {
            // Clear the previously computed checksum. This is so memory leaks are avoided.
            // The checksum computed on the last loop iteration will be returned to the client
            // if ALL replicas share identical checksum values.
            if (*_computed_checksum) {
                std::free(*_computed_checksum);
                *_computed_checksum = nullptr;
            }

            // Compute and register a checksum if the client set the force flag or the replica does
            // not have a checksum.
            if (kvp.contains(FORCE_CHKSUM_KW) || std::strlen(r->chksum) == 0) {
                if (client_set_admin_flag) {
                    ix::key_value_proxy{r->condInput}[ADMIN_KW] = "";
                }

                if (const auto ec = dataObjChksumAndRegInfo(&_comm, r, _computed_checksum); ec < 0) {
                    const auto msg = fmt::format("Could not compute/update checksum for replica [{}].", r->replNum);
                    addRErrorMsg(&_comm.rError, ec, msg.data());
                    return ec;
                }
            }
            else {
                *_computed_checksum = strdup(r->chksum);
            }

            if (!previous_checksum.empty()) {
                // Verify that the current replica's checksum matches the previous replica's checksum.
                // This enables the server to detect if sibling replicas share identical checksum values
                // without having to iterate over the replicas and checksums again.
                if (replicas_share_identical_checksum && *_computed_checksum != previous_checksum) {
                    replicas_share_identical_checksum = false;
                }
            }

            previous_checksum = *_computed_checksum;
        }

        // Report if sibling replicas do not share identical checksums.
        if (!replicas_share_identical_checksum) {
            if (*_computed_checksum) {
                std::free(*_computed_checksum);
            }

            *_computed_checksum = nullptr;
            addRErrorMsg(&_comm.rError, USER_CHKSUM_MISMATCH, "WARNING: Data object has replicas with different checksums.");
        }

        return 0;
    } // do_lookup_or_update
} // anonymous namespace

int rsDataObjChksum(RsComm* rsComm, DataObjInp* dataObjChksumInp, char** outChksum)
{
    int status;
    DataObjInfo *dataObjInfoHead;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = nullptr;

    resolveLinkedPath( rsComm, dataObjChksumInp->objPath, &specCollCache, &dataObjChksumInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjChksumInp, &rodsServerHost, REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }

    if ( remoteFlag == REMOTE_HOST ) {
        return rcDataObjChksum( rodsServerHost->conn, dataObjChksumInp, outChksum );
    }

    // If the client specified a leaf resource, then discover the hierarchy and
    // store it in the keyValPair_t. This instructs the iRODS server to create
    // the replica at the specified resource if it does not exist.
    auto kvp = ix::make_key_value_proxy(dataObjChksumInp->condInput);
    if (kvp.contains(LEAF_RESOURCE_NAME_KW)) {
        std::string hier;
        auto leaf = kvp[LEAF_RESOURCE_NAME_KW].value();
        bool is_coord_resc = false;

        if (const auto err = resc_mgr.is_coordinating_resource(leaf.data(), is_coord_resc); !err.ok()) {
            log::api::error(err.result());
            return err.code();
        }

        // Leaf resources cannot be coordinating resources. This essentially checks
        // if the resource has any child resources which is exactly what we're interested in.
        if (is_coord_resc) {
            log::api::error("[{}] is not a leaf resource.", leaf);
            return USER_INVALID_RESC_INPUT;
        }

        if (const auto err = resc_mgr.get_hier_to_root_for_resc(leaf.data(), hier); !err.ok()) {
            log::api::error(err.result());
            return err.code();
        }

        kvp[RESC_HIER_STR_KW] = hier;
    }

    // =-=-=-=-=-=-=-
    // determine the resource hierarchy if one is not provided
    if (!kvp.contains(RESC_HIER_STR_KW)) {
        try {
            auto result = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, rsComm, *dataObjChksumInp);
            kvp[RESC_HIER_STR_KW] = std::get<std::string>(result);
        }
        catch (const irods::exception& e) {
            log::api::error(e.what());
            return e.code();
        }
    } // if keyword
    status = _rsDataObjChksum(rsComm, dataObjChksumInp, outChksum, &dataObjInfoHead);

    freeAllDataObjInfo( dataObjInfoHead );
    rodsLog( LOG_DEBUG, "rsDataObjChksum - returning status %d", status );

    return status;
}

int _rsDataObjChksum(RsComm* rsComm,
                     DataObjInp* dataObjInp,
                     char** outChksumStr,
                     DataObjInfo** dataObjInfoHead)
{
    *dataObjInfoHead = nullptr;
    *outChksumStr = nullptr;

    if (ix::key_value_proxy{dataObjInp->condInput}.contains(VERIFY_CHKSUM_KW)) {
        return do_verification(*rsComm, *dataObjInp, *dataObjInfoHead, outChksumStr);
    }

    return do_lookup_or_update(*rsComm, *dataObjInp, *dataObjInfoHead, outChksumStr);
}

int dataObjChksumAndRegInfo(RsComm* rsComm,
                            DataObjInfo* dataObjInfo,
                            char** outChksumStr)
{
    int status = _dataObjChksum( rsComm, dataObjInfo, outChksumStr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "%s: _dataObjChksum error for %s, status = %d",
                 __FUNCTION__, dataObjInfo->objPath, status );
        return status;
    }

    if ( dataObjInfo->specColl != nullptr ) {
        return status;
    }

    keyValPair_t regParam{};
    addKeyVal( &regParam, CHKSUM_KW, *outChksumStr );
    // set pdmo flag so that chksum doesn't trigger file operations
    addKeyVal( &regParam, IN_PDMO_KW, "" );
    // Make sure admin flag is set as appropriate
    if ( nullptr != getValByKey( &dataObjInfo->condInput, ADMIN_KW ) ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        addKeyVal( &regParam, ADMIN_KW, "" );
    }
    modDataObjMeta_t modDataObjMetaInp{};
    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
    clearKeyVal( &regParam );

    return status;
}

int verifyDataObjChksum(RsComm* rsComm,
                        DataObjInfo* dataObjInfo,
                        char** outChksumStr)
{
    addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, dataObjInfo->chksum );
    int status = _dataObjChksum( rsComm, dataObjInfo, outChksumStr );
    rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "verifyDataObjChksum:_dataObjChksum error for %s, stat=%d",
                 dataObjInfo->objPath, status );
        return status;
    }

    if ( *outChksumStr == nullptr ) {
        rodsLog( LOG_ERROR, "verifyDataObjChksum: outChkSumStr is null." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( *outChksumStr, dataObjInfo->chksum ) != 0 ) {
        const auto msg = fmt::format("verifyDataObjChksum: computed checksum [{}] != catalog value [{}] "
                                     "for [{}] hierarchy [{}] replica number [{}]",
                                     *outChksumStr, dataObjInfo->chksum, dataObjInfo->objPath,
                                     dataObjInfo->rescHier, dataObjInfo->replNum);
        rodsLog(LOG_ERROR, msg.data());
        addRErrorMsg(&rsComm->rError, USER_CHKSUM_MISMATCH, msg.data());
        return USER_CHKSUM_MISMATCH;
    }

    return status;
}

int verifyDatObjChksum(RsComm* rsComm,
                       DataObjInfo* dataObjInfo,
                       char** outChksumStr)
{
    return verifyDataObjChksum(rsComm, dataObjInfo, outChksumStr);
}

