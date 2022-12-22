#include "irods/apiNumber.h"
#include "irods/dataObjClose.h"
#include "irods/dataObjCreate.h"
#include "irods/dataObjGet.h"
#include "irods/dataObjLock.h"
#include "irods/dataObjOpen.h"
#include "irods/dataObjOpr.hpp"
#include "irods/dataObjPut.h"
#include "irods/dataObjRepl.h"
#include "irods/dataObjTrim.h"
#include "irods/fileStageToCache.h"
#include "irods/fileSyncToArch.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/getRescQuota.h"
#include "irods/icatDefines.h"
#include "irods/miscServerFunct.hpp"
#include "irods/objMetaOpr.hpp"
#include "irods/physPath.hpp"
#include "irods/resource.hpp"
#include "irods/rodsLog.h"
#include "irods/rsDataCopy.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsDataObjCreate.hpp"
#include "irods/rsDataObjGet.hpp"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjPut.hpp"
#include "irods/rsDataObjRead.hpp"
#include "irods/rsDataObjRepl.hpp"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsDataObjWrite.hpp"
#include "irods/rsFileStageToCache.hpp"
#include "irods/rsFileSyncToArch.hpp"
#include "irods/rsGetRescQuota.hpp"
#include "irods/rsL3FileGetSingleBuf.hpp"
#include "irods/rsL3FilePutSingleBuf.hpp"
#include "irods/rsUnbunAndRegPhyBunfile.hpp"
#include "irods/rsUnregDataObj.hpp"
#include "irods/rs_replica_close.hpp"
#include "irods/specColl.hpp"
#include "irods/unbunAndRegPhyBunfile.h"

#include "irods/finalize_utilities.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_server_api_call.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/json_serialization.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/replica_access_table.hpp"
#include "irods/replication_utilities.hpp"
#include "irods/voting.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/data_object_proxy.hpp"
#include "irods/replica_proxy.hpp"

#include "irods/logical_locking.hpp"

#include <cstring>
#include <algorithm>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <boost/make_shared.hpp>

namespace
{
    namespace ill = irods::logical_locking;
    namespace ir = irods::experimental::replica;
    namespace irv = irods::experimental::resource::voting;
    namespace rst = irods::replica_state_table;

    auto apply_static_peps(RsComm& _comm, l1desc& _l1desc, const int _operation_status) -> void
    {
        if (_l1desc.openType == CREATE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForCreate");
        }
        else if (_l1desc.openType == OPEN_FOR_WRITE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForOpen");
        }

        // Calling acPostProcForPut after replication is legacy behavior.
        // Keep this here even though it is wrong.
        irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForPut");
    } // apply_static_peps

    auto calculate_checksum(
        RsComm& _comm,
        l1desc& _l1desc,
        ir::replica_proxy_t& _source_replica,
        ir::replica_proxy_t& _destination_replica) -> std::string
    {
        if (!_destination_replica.checksum().empty()) {
            _l1desc.chksumFlag = REG_CHKSUM;
        }

        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        if (!_source_replica.checksum().empty() && STALE_REPLICA != _source_replica.replica_status()) {
            _destination_replica.cond_input()[ORIG_CHKSUM_KW] = _source_replica.checksum();

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - verifying checksum for [{}],source:[{}]",
                __FUNCTION__, __LINE__, _destination_replica.logical_path(), _source_replica.checksum()));

            if (const int ec = _dataObjChksum(&_comm, _destination_replica.get(), &checksum_string); ec < 0) {
                _destination_replica.checksum("");

                if (DIRECT_ARCHIVE_ACCESS == ec) {
                    _destination_replica.checksum(_source_replica.checksum());
                    return _source_replica.checksum().data();
                }

                THROW(ec, fmt::format(
                    "{}: _dataObjChksum error for {}, status = {}",
                    __FUNCTION__, _destination_replica.logical_path(), ec));
            }

            if (!checksum_string) {
                THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
            }

            _destination_replica.checksum(checksum_string);
            rst::update(_destination_replica.data_id(), _destination_replica);

            if (_source_replica.checksum() != checksum_string) {
                THROW(USER_CHKSUM_MISMATCH, fmt::format(
                    "{}: chksum mismatch for {} src [{}] new [{}]",
                    __FUNCTION__, _destination_replica.logical_path(), _source_replica.checksum(), checksum_string));
            }

            return _destination_replica.checksum().data();
        }

        if (!_l1desc.chksumFlag) {
            if (_destination_replica.checksum().empty()) {
                return {};
            }
            _l1desc.chksumFlag = VERIFY_CHKSUM;
        }

        if (VERIFY_CHKSUM == _l1desc.chksumFlag) {
            if (!std::string_view{_l1desc.chksum}.empty()) {
                return irods::verify_checksum(_comm, *_destination_replica.get(), _l1desc.chksum);
            }

            if (!_destination_replica.checksum().empty()) {
                _destination_replica.cond_input()[ORIG_CHKSUM_KW] = _destination_replica.checksum();
            }

            if (const int ec = _dataObjChksum(&_comm, _destination_replica.get(), &checksum_string); ec < 0) {
                THROW(ec, "failed in _dataObjChksum");
            }

            if (!checksum_string) {
                THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
            }

            if (!_destination_replica.checksum().empty()) {
                _destination_replica.cond_input().erase(ORIG_CHKSUM_KW);

                /* for replication, the chksum in dataObjInfo was duplicated */
                if (_destination_replica.checksum() != checksum_string) {
                    THROW(USER_CHKSUM_MISMATCH, fmt::format(
                        "{}:mismatch chksum for {}.Rcat={},comp {}",
                        __FUNCTION__, _destination_replica.logical_path(), _destination_replica.checksum(), checksum_string));
                }
            }

            return {checksum_string};
        }

        return irods::register_new_checksum(_comm, *_destination_replica.get(), _l1desc.chksum);
    } // calculate_checksum

    auto finalize_destination_replica(
        RsComm& _comm,
        l1desc& _l1desc,
        ir::replica_proxy_t& _source_replica,
        ir::replica_proxy_t& _destination_replica) -> int
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);

        try {
            constexpr bool verify_size = true;
            const auto size_in_vault = irods::get_size_in_vault(_comm, *_destination_replica.get(), verify_size, _l1desc.dataSize);
            if (size_in_vault < 0) {
                THROW(size_in_vault, fmt::format(
                    "[{}:{}] - failed to get size in vault "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, size_in_vault,
                    _destination_replica.logical_path(),
                    _destination_replica.hierarchy()));
            }

            _destination_replica.size(size_in_vault);

            if ((_source_replica.checksum().empty() && !cond_input.contains(REG_CHKSUM_KW)) ||
                cond_input.contains(NO_COMPUTE_KW)) {
                _destination_replica.checksum("");
                cond_input[CHKSUM_KW] = "";
            }
            else {
                const auto checksum = calculate_checksum(_comm, _l1desc, _source_replica, _destination_replica);
                cond_input[CHKSUM_KW] = checksum;
                _destination_replica.checksum(checksum);
            }
        }
        catch (const irods::exception& e) {
            const auto admin_op = cond_input.contains(ADMIN_KW);

            if (const int unlock_ec = ill::unlock_and_publish(_comm, {_destination_replica, admin_op, 0}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
            }

            throw;
        }

        const auto source_replica_original_status = ill::get_original_replica_status(_source_replica.data_id(), _source_replica.replica_number());
        _destination_replica.replica_status(source_replica_original_status);
        _destination_replica.mtime(SET_TIME_TO_NOW_KW);

        rst::update(_destination_replica.data_id(), _destination_replica);

        const auto unlock_ec = ill::unlock(
            _destination_replica.data_id(),
            _destination_replica.replica_number(),
            _destination_replica.replica_status(),
            ill::restore_status);
        if (unlock_ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, unlock_ec,
                _destination_replica.logical_path(),
                _destination_replica.hierarchy()));

            return unlock_ec;
        }

        // This allows fileModified to trigger, but the resource plugins need to make sure the
        // appropriate PDMO hierarchies are being checked to prevent infinite loops (e.g. replication
        // resource triggering replication while performing a PDMO replication).
        cond_input.erase(IN_REPL_KW);
        cond_input[OPEN_TYPE_KW] = std::to_string(_l1desc.openType);

        const auto [ret, ec] = rst::publish::to_catalog(_comm, {_destination_replica, *cond_input.get()});

        if (CREATE_TYPE == _l1desc.openType) {
            updatequotaOverrun(_destination_replica.hierarchy().data(), _destination_replica.size(), ALL_QUOTA);
        }

        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to finalize data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, _destination_replica.logical_path(), _destination_replica.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, _destination_replica.data_id(), _destination_replica.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            _l1desc.oprStatus = ec;

            if (CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME != ec) {
                l3Unlink(&_comm, _destination_replica.get());
            }
        }

        if (rst::contains(_destination_replica.data_id())) {
            rst::erase(_destination_replica.data_id());
        }

        return ec;
    } // finalize_destination_replica

    auto finalize_destination_replica_on_failure(
        RsComm& _comm,
        l1desc& _l1desc,
        ir::replica_proxy_t& _destination_replica) -> int
    {
        const auto admin_op = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput).contains(ADMIN_KW);
        const rodsLong_t vault_size = getSizeInVault(&_comm, _destination_replica.get());
        if (vault_size < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "{} - getSizeInVault failed [{}]",
                __FUNCTION__, vault_size));

            if (const int unlock_ec = ill::unlock_and_publish(_comm, {_destination_replica, admin_op, 0}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
            }

            return vault_size;
        }

        _destination_replica.replica_status(STALE_REPLICA);
        _destination_replica.mtime(SET_TIME_TO_NOW_KW);
        _destination_replica.size(vault_size);

        // Write it out to the catalog
        rst::update(_destination_replica.data_id(), _destination_replica);

        if (const auto ec = ill::unlock(_destination_replica.data_id(), _destination_replica.replica_number(), _destination_replica.replica_status(), ill::restore_status); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ec,
                _destination_replica.logical_path(), _destination_replica.hierarchy()));

            return ec;
        }

        const auto ctx = rst::publish::context{_destination_replica, admin_op};

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, ctx); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed in finalize step "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec,
                _destination_replica.logical_path(), _destination_replica.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, _destination_replica.data_id(), _destination_replica.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            return ec;
        }

        return 0;
    } // finalize_destination_replica_on_failure

    auto get_replica_with_hierarchy(
        RsComm& _comm,
        irods::file_object_ptr _object,
        const std::string_view _hierarchy,
        const irods::replication::log_errors _log_errors) -> irods::physical_object&
    {
        auto maybe_replica = _object->get_replica(_hierarchy);

        if (!maybe_replica) {
            const std::string msg = fmt::format(
                "no replica found for [{}] with hierarchy [{}]",
                _object->logical_path(), _hierarchy);

            if (irods::replication::log_errors::yes == _log_errors) {
                addRErrorMsg(&_comm.rError, SYS_REPLICA_DOES_NOT_EXIST, msg.data());
            }

            THROW(SYS_REPLICA_DOES_NOT_EXIST, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));
        }

        return maybe_replica->get();
    } // get_replica_with_hierarchy

    auto resolve_hierarchy_and_get_data_object_info(
        RsComm& _comm,
        DataObjInp& _inp,
        const std::string_view _operation) -> irods::file_object_ptr
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        // Open for read operation is for source replica, so use RESC_HIER_STR_KW
        // if that is the passed-in operation.
        const std::string_view keyword = irods::OPEN_OPERATION == _operation ? RESC_HIER_STR_KW : DEST_RESC_HIER_STR_KW;

        if (irods::experimental::keyword_has_a_value(_inp.condInput, keyword)) {
            irods::file_object_ptr obj{new irods::file_object()};

            if (irods::error err = irods::file_object_factory(&_comm, &_inp, obj); !err.ok()) {
                THROW(err.code(), err.result());
            }

            if (irods::OPEN_OPERATION != _operation) {
                cond_input[RESC_HIER_STR_KW] = cond_input.at(DEST_RESC_HIER_STR_KW);
            }

            return obj;
        }

        auto [obj, hier] = irods::resolve_resource_hierarchy(_operation.data(), &_comm, _inp);

        cond_input[RESC_HIER_STR_KW] = hier;
        if (irods::OPEN_OPERATION != _operation) {
            cond_input[DEST_RESC_HIER_STR_KW] = cond_input.at(RESC_HIER_STR_KW);
        }

        return obj;
    } // resolve_hierarchy_and_get_data_object_info

    DataObjInp init_source_replica_input(RsComm& _comm, const DataObjInp& _inp)
    {
        DataObjInp source_data_obj_inp = _inp;
        replKeyVal(&_inp.condInput, &source_data_obj_inp.condInput);
        auto cond_input = irods::experimental::make_key_value_proxy(source_data_obj_inp.condInput);

        // these keywords are used for destination resource, so remove them
        cond_input.erase(DEST_RESC_NAME_KW);
        cond_input.erase(DEST_RESC_HIER_STR_KW);

        if (irods::experimental::keyword_has_a_value(_inp.condInput, RESC_NAME_KW) &&
            irods::hierarchy_parser{cond_input.at(RESC_NAME_KW).value().data()}.num_levels() > 1) {
            THROW(DIRECT_CHILD_ACCESS, fmt::format(
                "specified resource [{}] is not a root resource",
                cond_input.at(RESC_NAME_KW).value()));
        }

        return source_data_obj_inp;
    } // init_source_replica_input

    int open_source_replica(RsComm& _comm, DataObjInp& _inp)
    {
        _inp.oprType = REPLICATE_SRC;
        _inp.openFlags = O_RDONLY;

        return rsDataObjOpen(&_comm, &_inp);
    } // open_source_replica

    DataObjInp init_destination_replica_input(RsComm& _comm, const DataObjInp& _inp)
    {
        DataObjInp destination_inp = _inp;
        replKeyVal(&_inp.condInput, &destination_inp.condInput);
        auto cond_input = irods::experimental::make_key_value_proxy(destination_inp.condInput);

        // these keywords are used for source resource, so remove them
        cond_input.erase(RESC_NAME_KW);
        cond_input.erase(RESC_HIER_STR_KW);
        cond_input.erase(REPL_NUM_KW);

        if (!irods::experimental::keyword_has_a_value(_inp.condInput, DEST_RESC_HIER_STR_KW) &&
            !irods::experimental::keyword_has_a_value(_inp.condInput, DEST_RESC_NAME_KW) &&
            irods::experimental::keyword_has_a_value(_inp.condInput, DEF_RESC_NAME_KW))
        {
            cond_input[DEST_RESC_NAME_KW] = cond_input.at(DEF_RESC_NAME_KW).value();
        }

        if (irods::experimental::keyword_has_a_value(_inp.condInput, DEST_RESC_NAME_KW) &&
            irods::hierarchy_parser{cond_input.at(DEST_RESC_NAME_KW).value().data()}.num_levels() > 1)
        {
            THROW(DIRECT_CHILD_ACCESS, fmt::format(
                "specified resource [{}] is not a root resource",
                cond_input.at(DEST_RESC_NAME_KW).value()));
        }

        return destination_inp;
    } // init_destination_replica_input

    int open_destination_replica(RsComm& _comm, DataObjInp& _inp, const int _source_fd)
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);
        cond_input[REG_REPL_KW] = "";
        cond_input[DATA_ID_KW] = std::to_string(L1desc[_source_fd].dataObjInfo->dataId);
        cond_input.erase(PURGE_CACHE_KW);

        _inp.oprType = REPLICATE_DEST;
        _inp.openFlags = O_CREAT | O_WRONLY | O_TRUNC;

        return rsDataObjOpen(&_comm, &_inp);
    } // open_destination_replica

    int replicate_data(RsComm& _comm, DataObjInp& _source_inp, DataObjInp& _destination_inp, transferStat_t** _stat)
    {
        // Open source replica
        const int source_l1descInx = open_source_replica(_comm, _source_inp);
        if (source_l1descInx < 0) {
            return source_l1descInx;
        }

        // Source replica information is copied here because information from the L1 descriptor is freed before the
        // last time the information is needed in this operation.
        auto [source_replica, source_replica_lm] = ir::duplicate_replica(*L1desc[source_l1descInx].dataObjInfo);

        irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - source:[{}]",
            __FUNCTION__, __LINE__, source_replica.hierarchy()));

        const auto cond_input = irods::experimental::make_key_value_proxy(_destination_inp.condInput);
        if (source_replica.checksum().empty() && cond_input.contains(VERIFY_CHKSUM_KW)) {
            const std::string msg = fmt::format(
                "Requested to verify checksum, but source replica for [{}] on [{}] has no "
                "checksum. Calculate a checksum for the source replica and try again. "
                "Aborting replication.",
                source_replica.logical_path(), source_replica.hierarchy());

            const auto ec = CAT_NO_CHECKSUM_FOR_REPLICA;

            addRErrorMsg(&_comm.rError, ec, msg.data());

            irods::log(LOG_WARNING, fmt::format("[{}:{}] - {}", msg, __func__, __LINE__));

            return ec;
        }

        if (!source_replica.checksum().empty() && cond_input.contains(REG_CHKSUM_KW)) {
            const std::string msg = fmt::format(
                "Requested to register checksum without verifying, but source replica for [{}] "
                "on [{}] has a checksum. This can result in multiple replicas being marked good "
                "with different checksums, which is an inconsistency. Aborting replication.",
                source_replica.logical_path(), source_replica.hierarchy());

            const auto ec = SYS_NOT_ALLOWED;

            addRErrorMsg(&_comm.rError, ec, msg.data());

            irods::log(LOG_WARNING, fmt::format("[{}:{}] - {}", msg, __func__, __LINE__));

            return ec;
        }

        // Open destination replica
        int destination_l1descInx = open_destination_replica(_comm, _destination_inp, source_l1descInx);
        if (destination_l1descInx < 0) {
            constexpr bool preserve_rst = false;
            if (const int ec = irods::close_replica_without_catalog_update(_comm, source_l1descInx, preserve_rst); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to close source replica:[{}]",
                    __FUNCTION__, __LINE__, ec));
            }
            return destination_l1descInx;
        }

        auto& destination_data_obj_info = *L1desc[destination_l1descInx].dataObjInfo;

        irods::log(LOG_DEBUG8, fmt::format(
            "[{}:{}] - destination:[{}]",
            __FUNCTION__, __LINE__, destination_data_obj_info.rescHier));

        L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;
        L1desc[destination_l1descInx].dataSize = source_replica.size();
        L1desc[destination_l1descInx].dataObjInp->dataSize = source_replica.size();

        const int thread_count = getNumThreads(
            &_comm,
            L1desc[source_l1descInx].dataObjInfo->dataSize,
            L1desc[destination_l1descInx].dataObjInp->numThreads,
            NULL,
            L1desc[destination_l1descInx].dataObjInfo->rescHier,
            L1desc[source_l1descInx].dataObjInfo->rescHier,
            0);

        L1desc[destination_l1descInx].dataObjInp->numThreads = thread_count;

        // Copy data from source to destination
        int status = dataObjCopy(&_comm, destination_l1descInx);
        if (status < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - dataObjCopy failed for [{}], src:[{}], dest:[{}]; ec:[{}]",
                __FUNCTION__, __LINE__,
                _destination_inp.objPath,
                source_replica.hierarchy(),
                destination_data_obj_info.rescHier,
                status));

            L1desc[destination_l1descInx].bytesWritten = status;
        }
        else {
            L1desc[destination_l1descInx].bytesWritten = destination_data_obj_info.dataSize;
        }

        // The transferStat_t communicates information back to the client regarding
        // the data transfer such as bytes written and how many threads were used.
        // These must be saved before the L1 descriptor is free'd.
        *_stat = (TransferStat*)malloc(sizeof(TransferStat));
        memset(*_stat, 0, sizeof(TransferStat));
        (*_stat)->bytesWritten = L1desc[destination_l1descInx].dataSize;
        (*_stat)->numThreads = L1desc[destination_l1descInx].dataObjInp->numThreads;

        // Save the token for the replica access table so that it can be removed
        // in the event of a failure in close. On failure, the entry is restored,
        // but this will prevent retries of the operation as the token information
        // is lost by the time we have returned to the caller.
        const auto token = L1desc[destination_l1descInx].replica_token;

        auto source_fd = irods::duplicate_l1_descriptor(L1desc[source_l1descInx]);
        auto destination_fd = irods::duplicate_l1_descriptor(L1desc[destination_l1descInx]);
        irods::at_scope_exit free_fd{[&source_fd, &destination_fd]
            {
                freeL1desc_struct(source_fd);
                freeL1desc_struct(destination_fd);
            }
        };

        auto [destination_replica, destination_replica_lm] = ir::duplicate_replica(destination_data_obj_info);
        constexpr auto preserve_rst = true;

        // Close source replica
        if (const int ec = irods::close_replica_without_catalog_update(_comm, source_l1descInx, preserve_rst); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - closing source replica [{}] failed with [{}]",
                __FUNCTION__, __LINE__, _source_inp.objPath, ec));

            if (status >= 0) {
                status = ec;
            }
        }

        // Close destination replica
        if (const int ec = irods::close_replica_without_catalog_update(_comm, destination_l1descInx, preserve_rst); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - closing destination replica [{}] failed with [{}]",
                __FUNCTION__, __LINE__, _destination_inp.objPath, ec));

            if (status >= 0) {
                status = ec;
            }

            irods::experimental::replica_access_table::erase_pid(token, getpid());
        }

        // finalize source replica
        try {
            irods::apply_metadata_from_cond_input(_comm, *source_fd.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *source_fd.dataObjInp);
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - error finalizing replica; [{}], ec:[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(), e.code()));

            if (status >= 0) {
                status = e.code();
            }
        }

        if (destination_fd.bytesWritten < 0) {
            if (const auto ec = finalize_destination_replica_on_failure(_comm, destination_fd, destination_replica); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}] - failed while finalizing object [{}]; ec:[{}]",
                    __FUNCTION__, destination_replica.logical_path(), ec));
            }

            return destination_fd.bytesWritten;
        }

        // finalize destination replica
        try {
            if (const int ec = finalize_destination_replica(_comm, destination_fd, source_replica, destination_replica); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - closing destination replica [{}] failed with [{}]",
                    __FUNCTION__, __LINE__, destination_replica.logical_path(), ec));

                if (status >= 0) {
                    status = ec;
                }
            }
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - error finalizing replica; [{}], ec:[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(), e.code()));

            if (status >= 0) {
                status = e.code();
            }
        }

        if (source_fd.purgeCacheFlag > 0) {
            irods::purge_cache(_comm, *source_replica.get());
        }

        if (destination_fd.purgeCacheFlag > 0) {
            irods::purge_cache(_comm, *destination_replica.get());
        }

        if (status >= 0) {
            apply_static_peps(_comm, destination_fd, status);
        }

        return status;
    } // replicate_data

    int replicate_data_object(RsComm& _comm, DataObjInp& _inp, transferStat_t** _stat)
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        // get information about source replica
        auto source_inp = init_source_replica_input(_comm, _inp);
        const irods::at_scope_exit free_source_cond_input{[&source_inp]() { clearKeyVal(&source_inp.condInput); }};
        auto source_cond_input = irods::experimental::make_key_value_proxy(source_inp.condInput);
        auto source_obj = resolve_hierarchy_and_get_data_object_info(_comm, source_inp, irods::OPEN_OPERATION);

        // Copy the resolved hierarchy for the source back into the input struct to maintain legacy behavior.
        if (!cond_input.contains(RESC_HIER_STR_KW)) {
            cond_input[RESC_HIER_STR_KW] = source_cond_input.at(RESC_HIER_STR_KW).value();
        }

        auto& source_replica = get_replica_with_hierarchy(
            _comm, source_obj, source_cond_input.at(RESC_HIER_STR_KW).value(),
            irods::replication::log_errors::yes);

        // If the resolved resource hierarchy does not contain the specified
        // resource name or replica number, that means the vote for that resource
        // returned as 0.0. Either the replica is inaccessible or it does not exist.
        if (irods::experimental::keyword_has_a_value(_inp.condInput, RESC_NAME_KW)) {
            const auto resolved_hierarchy = source_cond_input.at(RESC_HIER_STR_KW).value();
            const auto resource_name = source_cond_input.at(RESC_NAME_KW).value();
            if (!irods::hierarchy_parser{resolved_hierarchy.data()}.resc_in_hier(resource_name.data())) {
                THROW(SYS_REPLICA_INACCESSIBLE, fmt::format(
                    "hierarchy descending from specified source resource name [{}] "
                    "does not have a replica or the replica is inaccessible at this time",
                    resource_name));
            }
        }
        else if (irods::experimental::keyword_has_a_value(_inp.condInput, REPL_NUM_KW)) {
            if (const auto replica_number = std::stoi(source_cond_input.at(REPL_NUM_KW).value().data());
                replica_number != source_replica.repl_num()) {
                THROW(SYS_REPLICA_INACCESSIBLE, fmt::format(
                    "specified source replica number [{}] does not exist "
                    "or the replica is inaccessible at this time",
                    replica_number));
            }
        }

        // get information about destination replica
        auto destination_inp = init_destination_replica_input(_comm, _inp);
        const irods::at_scope_exit free_destination_cond_input{[&destination_inp]() { clearKeyVal(&destination_inp.condInput); }};
        auto destination_cond_input = irods::experimental::make_key_value_proxy(destination_inp.condInput);
        auto destination_obj = resolve_hierarchy_and_get_data_object_info(_comm, destination_inp, irods::CREATE_OPERATION);

        // Copy the resolved hierarchy for the destination back into the input struct to maintain legacy behavior.
        if (!cond_input.contains(DEST_RESC_HIER_STR_KW)) {
            cond_input[DEST_RESC_HIER_STR_KW] = destination_cond_input.at(DEST_RESC_HIER_STR_KW).value();
        }

        try {
            if (irods::experimental::keyword_has_a_value(destination_inp.condInput, DEST_RESC_NAME_KW)) {
                const auto resolved_hierarchy = destination_cond_input.at(DEST_RESC_HIER_STR_KW).value();
                const auto resource_name = destination_cond_input.at(DEST_RESC_NAME_KW).value();
                if (!irods::hierarchy_parser{resolved_hierarchy.data()}.resc_in_hier(resource_name.data())) {
                    THROW(SYS_REPLICA_INACCESSIBLE, fmt::format(
                        "hierarchy descending from specified destination resource name [{}] "
                        "does not have a replica or the replica is inaccessible at this time; "
                        "resolved hierarchy:[{}]",
                        resource_name, resolved_hierarchy.data()));
                }
            }

            const auto& destination_replica = get_replica_with_hierarchy(
                _comm, destination_obj, destination_cond_input.at(DEST_RESC_HIER_STR_KW).value(),
                irods::replication::log_errors::no);

            const auto log_errors = cond_input.contains(RECURSIVE_OPR__KW) ? irods::replication::log_errors::no : irods::replication::log_errors::yes;
            if (!irods::replication::is_allowed(_comm, source_replica, destination_replica, log_errors)) {
                return SYS_NOT_ALLOWED;
            }
        }
        catch (const irods::exception& e) {
            // If the destination replica doesn't exist, replication is always allowed.
            if (SYS_REPLICA_DOES_NOT_EXIST != e.code()) {
                throw;
            }
        }

        irods::log(LOG_DEBUG8, fmt::format(
            "[{}:{}] - source:[{}],destination:[{}]",
            __FUNCTION__, __LINE__,
            source_cond_input.at(RESC_HIER_STR_KW).value(),
            destination_cond_input.at(RESC_HIER_STR_KW).value()));

        // replicate!
        const int ec = replicate_data(_comm, source_inp, destination_inp, _stat);

        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to replicate [{}]",
                __FUNCTION__, __LINE__, _inp.objPath));
        }

        return ec;
    } // replicate_data_object

    int update_all_existing_replicas(RsComm& _comm, const dataObjInp_t& _inp, transferStat_t** _stat)
    {
        // get information about source replica
        auto source_inp = init_source_replica_input(_comm, _inp);
        const irods::at_scope_exit free_source_cond_input{[&source_inp]() { clearKeyVal(&source_inp.condInput); }};
        auto source_cond_input = irods::experimental::make_key_value_proxy(source_inp.condInput);
        auto source_obj = resolve_hierarchy_and_get_data_object_info(_comm, source_inp, irods::OPEN_OPERATION);
        auto& source_replica = get_replica_with_hierarchy(
            _comm, source_obj, source_cond_input.at(RESC_HIER_STR_KW).value(),
            irods::replication::log_errors::yes);

        // If the resolved resource hierarchy does not contain the specified
        // resource name or replica number, that means the vote for that resource
        // returned as 0.0. Either the replica is inaccessible or it does not exist.
        if (irods::experimental::keyword_has_a_value(_inp.condInput, RESC_NAME_KW)) {
            const auto resolved_hierarchy = source_cond_input.at(RESC_HIER_STR_KW).value();
            const auto resource_name = source_cond_input.at(RESC_NAME_KW).value();
            if (!irods::hierarchy_parser{resolved_hierarchy.data()}.resc_in_hier(resource_name.data())) {
                THROW(SYS_REPLICA_INACCESSIBLE, fmt::format(
                    "hierarchy descending from specified source resource name [{}] "
                    "does not have a replica or the replica is inaccessible at this time",
                    resource_name));
            }
        }
        else if (irods::experimental::keyword_has_a_value(_inp.condInput, REPL_NUM_KW)) {
            if (const auto replica_number = std::stoi(source_cond_input.at(REPL_NUM_KW).value().data());
                replica_number != source_replica.repl_num()) {
                THROW(SYS_REPLICA_INACCESSIBLE, fmt::format(
                    "specified source replica number [{}] does not exist "
                    "or the replica is inaccessible at this time",
                    replica_number));
            }
        }

        // only good replica can be used as a source to update all other replicas
        if (GOOD_REPLICA != source_replica.replica_status()) {
            THROW(SYS_NOT_ALLOWED, fmt::format(
                "source replica on [{}] has status [{}]; cannot be used to update other replicas.",
                source_replica.resc_hier(), source_replica.replica_status()));
        }

        // get information about existing replicas (destination replicas)
        auto destination_inp = init_destination_replica_input(_comm, _inp);
        const irods::at_scope_exit free_destination_cond_input{[&destination_inp]() { clearKeyVal(&destination_inp.condInput); }};
        auto destination_cond_input = irods::experimental::make_key_value_proxy(destination_inp.condInput);
        auto destination_obj = resolve_hierarchy_and_get_data_object_info(_comm, destination_inp, irods::WRITE_OPERATION);

        // replicate!
        int status = 0;

        for (const auto& destination_replica : destination_obj->replicas()) {
            irods::log(LOG_DEBUG8, fmt::format(
                "[{}:{}] - obj:[{}], source:[{}], destination:[{}], vote:[{}]",
                __FUNCTION__, __LINE__,
                destination_obj->logical_path(),
                source_replica.resc_hier(),
                destination_replica.resc_hier(),
                destination_replica.vote()));

            if (irods::replication::is_allowed(_comm, source_replica, destination_replica, irods::replication::log_errors::no) &&
                destination_replica.vote() > irv::vote::zero)
            {
                destination_cond_input[RESC_HIER_STR_KW] = destination_replica.resc_hier();
                destination_cond_input[DEST_RESC_HIER_STR_KW] = destination_replica.resc_hier();

                if (const int ec = replicate_data(_comm, source_inp, destination_inp, _stat);
                    ec < 0 && status >= 0)
                {
                    status = ec;
                }
            }
        }

        return status;
    } // update_all_existing_replicas

    int singleL1Copy(rsComm_t *rsComm, dataCopyInp_t& dataCopyInp)
    {
        int trans_buff_size;
        try {
            trans_buff_size = irods::get_advanced_setting<const int>(irods::KW_CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS) * 1024 * 1024;
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

int rsDataObjRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t **transStat)
{
    if (!rsComm || !dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);

    // -R and -a are not compatible
    if (cond_input.contains(DEST_RESC_NAME_KW) && cond_input.contains(ALL_KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    // -S and -n are not compatible
    if (cond_input.contains(RESC_NAME_KW) && cond_input.contains(REPL_NUM_KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    const auto compute_and_verify_checksum = cond_input.contains(VERIFY_CHKSUM_KW);
    const auto compute_and_do_not_verify_checksum = cond_input.contains(REG_CHKSUM_KW);
    const auto do_not_compute_checksum = cond_input.contains(NO_COMPUTE_KW);

    const auto compute_checksum = compute_and_verify_checksum ||
                                  compute_and_do_not_verify_checksum;
    if (compute_checksum && do_not_compute_checksum) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    if (compute_and_verify_checksum && compute_and_do_not_verify_checksum) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    // Must be a privileged user to invoke SU
    if (cond_input.contains(SU_CLIENT_USER_KW) &&
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
        cond_input[IN_REPL_KW] = "";

        const irods::at_scope_exit remove_in_repl{[&cond_input] { cond_input.erase(IN_REPL_KW); }};

        if (cond_input.contains(ALL_KW)) {
            status = update_all_existing_replicas(*rsComm, *dataObjInp, transStat);
        }
        else {
            status = replicate_data_object(*rsComm, *dataObjInp, transStat);
        }
    }
    catch (const irods::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
        addRErrorMsg(&rsComm->rError, e.code(), e.client_display_what());
        status = e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
        status = SYS_INTERNAL_ERR;
    }
    catch (...) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
        status = SYS_UNKNOWN_ERROR;
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
    std::memset(&dataObjInp.condInput, 0, sizeof(dataObjInp.condInput));
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
