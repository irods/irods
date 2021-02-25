#include "apiNumber.h"
#include "dataObjClose.h"
#include "dataObjLock.h"
#include "dataObjOpr.hpp"
#include "dataObjRepl.h"
#include "dataObjTrim.h"
#include "dataObjUnlink.h"
#include "fileClose.h"
#include "fileStat.h"
#include "getRescQuota.h"
#include "key_value_proxy.hpp"
#include "miscServerFunct.hpp"
#include "modAVUMetadata.h"
#include "modAccessControl.h"
#include "modDataObjMeta.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "physPath.hpp"
#include "rcGlobalExtern.h"
#include "regDataObj.h"
#include "regReplica.h"
#include "resource.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "rsDataObjClose.hpp"
#include "rsDataObjTrim.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsFileClose.hpp"
#include "rsFileStat.hpp"
#include "rsGetRescQuota.hpp"
#include "rsGlobalExtern.hpp"
#include "rsModAVUMetadata.hpp"
#include "rsModAccessControl.hpp"
#include "rsModDataObjMeta.hpp"
#include "rsRegDataObj.hpp"
#include "rsRegReplica.hpp"
#include "rsSubStructFileClose.hpp"
#include "rsSubStructFileStat.hpp"
#include "ruleExecSubmit.h"
#include "subStructFileClose.h"
#include "subStructFileRead.h"
#include "subStructFileStat.h"

// =-=-=-=-=-=-=-
#include "finalize_utilities.hpp"
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_file_object.hpp"
#include "irods_exception.hpp"
#include "irods_serialization.hpp"
#include "irods_server_api_call.hpp"
#include "irods_server_properties.hpp"
#include "irods_at_scope_exit.hpp"
#include "json_serialization.hpp"
#include "key_value_proxy.hpp"
#include "replica_access_table.hpp"
#include "replica_state_table.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include <memory>
#include <functional>

#include <sys/types.h>
#include <unistd.h>

namespace
{
    namespace ir = irods::experimental::replica;
    namespace rst = irods::replica_state_table;

    auto apply_static_peps(RsComm& _comm, openedDataObjInp_t& _close_inp, const int _fd, const int _operation_status) -> void
    {
        auto& l1desc = L1desc[_fd];

        /* note : this may overlap with acPostProcForPut or acPostProcForCopy */
        if ( l1desc.openType == CREATE_TYPE ) {
            irods::apply_static_post_pep(_comm, l1desc, _operation_status, "acPostProcForCreate");
        }
        else if ( l1desc.openType == OPEN_FOR_READ_TYPE ||
                  l1desc.openType == OPEN_FOR_WRITE_TYPE ) {
            irods::apply_static_post_pep(_comm, l1desc, _operation_status, "acPostProcForOpen");
        }
        else if ( l1desc.oprType == REPLICATE_DEST ) {
            irods::apply_static_post_pep(_comm, l1desc, _operation_status, "acPostProcForRepl");
        }

        if ( l1desc.oprType == COPY_DEST ) {
            /* have to put copy first because the next test could
             * trigger put rule for copy operation */
            irods::apply_static_post_pep(_comm, l1desc, _operation_status, "acPostProcForCopy");
        }
        else if ( l1desc.oprType == PUT_OPR ||
                  l1desc.openType == CREATE_TYPE ||
                  ( l1desc.openType == OPEN_FOR_WRITE_TYPE &&
                    ( l1desc.bytesWritten > 0 ||
                      _close_inp.bytesWritten > 0 ) ) ) {
            irods::apply_static_post_pep(_comm, l1desc, _operation_status, "acPostProcForPut");
        }
    } // apply_static_peps

    bool bytes_written_in_operation(const l1desc_t& l1desc)
    {
        return l1desc.bytesWritten >= 0 ||
               REPLICATE_DEST == l1desc.oprType ||
               COPY_DEST == l1desc.oprType;
    } // bytes_written_in_operation

    auto cross_zone_write_operation(const openedDataObjInp_t& inp, const l1desc_t& l1desc) -> bool
    {
        return inp.bytesWritten > 0 && l1desc.bytesWritten <= 0;
    } // cross_zone_write_operation

    auto cross_zone_copy_operation(const l1desc_t& l1desc) -> bool
    {
        const auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        return l1desc.l3descInx < 3 &&
               cond_input.contains(CROSS_ZONE_CREATE_KW) &&
               INTERMEDIATE_REPLICA == l1desc.replStatus;
    } // cross_zone_copy_operation

    auto perform_checksum_operation_for_finalize(RsComm& _comm, const int _fd) -> std::string
    {
        auto& l1desc = L1desc[_fd];
        int oprType = l1desc.oprType;

        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        auto destination_replica = ir::replica_proxy_t{*l1desc.dataObjInfo};
        if (oprType == REPLICATE_DEST) {
            const int srcL1descInx = l1desc.srcL1descInx;
            if (srcL1descInx < 3) {
                THROW(SYS_FILE_DESC_OUT_OF_RANGE, fmt::format(
                    "{}: srcL1descInx {} out of range",
                    __FUNCTION__, srcL1descInx));
            }

            auto source_replica = ir::replica_proxy_t{*L1desc[srcL1descInx].dataObjInfo};
            if (source_replica.checksum().length() > 0 && STALE_REPLICA != source_replica.replica_status()) {
                destination_replica.cond_input()[ORIG_CHKSUM_KW] = source_replica.checksum();

                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - verifying checksum for [{}],source:[{}]",
                    __FUNCTION__, __LINE__, destination_replica.logical_path(), source_replica.checksum()));

                if (const int ec = _dataObjChksum(&_comm, destination_replica.get(), &checksum_string); ec < 0) {
                    destination_replica.checksum("");

                    if (DIRECT_ARCHIVE_ACCESS == ec) {
                        destination_replica.checksum(source_replica.checksum());
                        return source_replica.checksum().data();
                    }

                    THROW(ec, fmt::format(
                        "{}: _dataObjChksum error for {}, status = {}",
                        __FUNCTION__, destination_replica.logical_path(), ec));
                }

                if (!checksum_string) {
                    THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
                }

                destination_replica.checksum(checksum_string);

                if (source_replica.checksum() != checksum_string) {
                    THROW(USER_CHKSUM_MISMATCH, fmt::format(
                        "{}: chksum mismatch for {} src [{}] new [{}]",
                        __FUNCTION__, destination_replica.logical_path(), source_replica.checksum(), checksum_string));
                }

                return destination_replica.checksum().data();
            }
        }

        if (!l1desc.chksumFlag) {
            if (destination_replica.checksum().empty()) {
                return {};
            }
            l1desc.chksumFlag = VERIFY_CHKSUM;
        }

        if (VERIFY_CHKSUM == l1desc.chksumFlag) {
            if (!std::string_view{l1desc.chksum}.empty()) {
                return irods::verify_checksum(_comm, *destination_replica.get(), l1desc.chksum);
            }

            if (REPLICATE_DEST == oprType) {
                if (!destination_replica.checksum().empty()) {
                    destination_replica.cond_input()[ORIG_CHKSUM_KW] = destination_replica.checksum();
                }

                if (const int ec = _dataObjChksum(&_comm, destination_replica.get(), &checksum_string); ec < 0) {
                    THROW(ec, "failed in _dataObjChksum");
                }

                if (!checksum_string) {
                    THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
                }

                if (destination_replica.checksum().length() > 0) {
                    destination_replica.cond_input().erase(ORIG_CHKSUM_KW);

                    /* for replication, the chksum in dataObjInfo was duplicated */
                    if (destination_replica.checksum() != checksum_string) {
                        THROW(USER_CHKSUM_MISMATCH, fmt::format(
                            "{}:mismach chksum for {}.Rcat={},comp {}",
                            __FUNCTION__, destination_replica.logical_path(), destination_replica.checksum(), checksum_string));
                    }
                }

                return {checksum_string};
            }

            // TODO come back to this case...
            if (COPY_DEST == oprType) {
                const int srcL1descInx = l1desc.srcL1descInx;
                if (srcL1descInx < 3) {
                    irods::log(LOG_DEBUG, fmt::format(
                        "{}: invalid srcL1descInx {} for copy",
                        __FUNCTION__, srcL1descInx));
                    // just register it for now
                    return {};
                }

                auto source_replica = ir::replica_proxy_t{*L1desc[srcL1descInx].dataObjInfo};

                if (source_replica.checksum().length() > 0) {
                    destination_replica.cond_input()[ORIG_CHKSUM_KW] = source_replica.checksum();
                }

                if (const int ec = _dataObjChksum(&_comm, destination_replica.get(), &checksum_string ); ec < 0) {
                    THROW(ec, "failed in _dataObjChksum");
                }

                if (!checksum_string) {
                    THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
                }

                if (source_replica.checksum().length() > 0) {
                    destination_replica.cond_input().erase(ORIG_CHKSUM_KW);
                    if (source_replica.checksum() != checksum_string) {
                        THROW(USER_CHKSUM_MISMATCH, fmt::format(
                            "{}:mismach chksum for {}.Rcat={},comp {}",
                            __FUNCTION__, destination_replica.logical_path(), source_replica.checksum(), checksum_string));
                    }
                }

                return {checksum_string};
            }

            return {};
        }

        return irods::register_new_checksum(_comm, *destination_replica.get(), l1desc.chksum);
    } // perform_checksum_operation_for_finalize

    auto finalize_destination_replica_for_replication(RsComm& _comm, const openedDataObjInp_t& _inp, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        const int srcL1descInx = l1desc.srcL1descInx;
        if (srcL1descInx < 3) {
            THROW(SYS_FILE_DESC_OUT_OF_RANGE, fmt::format(
                "{}: srcL1descInx {} out of range",
                __FUNCTION__, srcL1descInx));
        }

        const auto source_replica = ir::replica_proxy_t{*L1desc[srcL1descInx].dataObjInfo};
        auto destination_replica = ir::replica_proxy_t{*l1desc.dataObjInfo};

        auto [reg_param, lm] = irods::experimental::make_key_value_proxy({{OPEN_TYPE_KW, std::to_string(l1desc.openType)}});
        reg_param[REPL_STATUS_KW] = std::to_string(source_replica.replica_status());
        reg_param[DATA_SIZE_KW] = std::to_string(source_replica.size());
        reg_param[DATA_MODIFY_KW] = std::to_string((int)time(nullptr));
        reg_param[FILE_PATH_KW] = destination_replica.physical_path();

        const auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        if (cond_input.contains(ADMIN_KW)) {
            reg_param[ADMIN_KW] = cond_input.at(ADMIN_KW);
        }
        if (const char* pdmo_kw = getValByKey(&_inp.condInput, IN_PDMO_KW); pdmo_kw) {
            reg_param[IN_PDMO_KW] = pdmo_kw;
        }
        if (cond_input.contains(SYNC_OBJ_KW)) {
            reg_param[SYNC_OBJ_KW] = cond_input.at(SYNC_OBJ_KW);
        }
        if (cond_input.contains(CHKSUM_KW)) {
            reg_param[CHKSUM_KW] = cond_input.at(CHKSUM_KW);
        }

        irods::log(LOG_DEBUG8, fmt::format(
            "[{}:{}] - modifying [{}] on [{}]",
            __FUNCTION__, __LINE__,
            destination_replica.logical_path(),
            destination_replica.hierarchy()));

        modDataObjMeta_t mod_inp{};
        mod_inp.dataObjInfo = destination_replica.get();
        mod_inp.regParam = reg_param.get();
        const int status = rsModDataObjMeta(&_comm, &mod_inp);

        if (CREATE_TYPE == l1desc.openType) {
            updatequotaOverrun(destination_replica.hierarchy().data(), destination_replica.size(), ALL_QUOTA);
        }

        if (status < 0) {
            l1desc.oprStatus = status;

            if (CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME != status) {
                l3Unlink(&_comm, destination_replica.get());
            }

            irods::log(LOG_NOTICE, fmt::format(
                    "{}: RegReplica/ModDataObjMeta {} err. stat = {}",
                    __FUNCTION__, destination_replica.logical_path(), status));
        }

        return status;
    } // finalize_destination_replica_for_replication

    auto finalize_data_object(RsComm& _comm, const openedDataObjInp_t& _inp, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        if (cross_zone_copy_operation(l1desc)) {
            if (const int ec = svrRegDataObj(&_comm, l1desc.dataObjInfo); ec < 0) {
                l1desc.oprStatus = ec;
                THROW(ec, fmt::format(
                    "{}: svrRegDataObj for {} failed, ec = {}",
                    __FUNCTION__, l1desc.dataObjInfo->objPath, ec));
            }
        }

        try {
            irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);
        }
        catch (const irods::exception& e) {
            if (l1desc.dataObjInp->oprType == PUT_OPR ) {
                rsDataObjUnlink(&_comm, l1desc.dataObjInp);
            }
            throw;
        }

        auto replica = ir::replica_proxy_t{*l1desc.dataObjInfo};

        auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        cond_input[OPEN_TYPE_KW] = std::to_string(l1desc.openType);

        // TODO: unlock in RST here (restore replica states)

        // Set target replica to the state it should be
        if (OPEN_FOR_WRITE_TYPE == l1desc.openType) {
            replica.replica_status(GOOD_REPLICA);
            replica.mtime(SET_TIME_TO_NOW_KW);

            // stale other replicas because the truth has moved
            for (auto& rj : rst::at(replica.logical_path())) {
                const auto replica_number = std::stoi(std::string{rj.at("after").at("data_repl_num")});

                if (replica.replica_number() != replica_number) {
                    const nlohmann::json update{{"data_is_dirty", std::to_string(STALE_REPLICA)}};

                    rst::update(replica.logical_path(), replica_number,
                        nlohmann::json{{"replicas", update}});
                }
            }
        }
        else {
            replica.replica_status(GOOD_REPLICA);
        }

        if (cond_input.contains(CHKSUM_KW)) {
            replica.checksum(cond_input.at(CHKSUM_KW).value());
        }

        replica.cond_input()[FILE_MODIFIED_KW] = irods::to_json(cond_input.get()).dump();

        // Write it out to the catalog
        rst::update(replica.logical_path(), replica);

        if (const int ec = rst::publish_to_catalog(_comm, replica.logical_path(), rst::trigger_file_modified::yes); ec < 0) {
            THROW(ec, fmt::format("failed to publish to catalog:[{}]", ec));
        }

        if (GOOD_REPLICA == replica.replica_status()) {
            updatequotaOverrun(replica.hierarchy().data(), replica.size(), ALL_QUOTA);
        }

        return 0;
    } // finalize_data_object

    auto finalize_for_put_or_copy(RsComm& _comm, const openedDataObjInp_t& _inp, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        if (cross_zone_copy_operation(l1desc)) {
            if (const int ec = svrRegDataObj(&_comm, l1desc.dataObjInfo); ec < 0) {
                l1desc.oprStatus = ec;
                THROW(ec, fmt::format(
                    "{}: svrRegDataObj for {} failed, status = {}",
                    __FUNCTION__, l1desc.dataObjInfo->objPath, ec));
            }
        }

        try {
            irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);
        }
        catch (const irods::exception& e) {
            if (l1desc.dataObjInp->oprType == PUT_OPR ) {
                rsDataObjUnlink(&_comm, l1desc.dataObjInp);
            }
            throw;
        }

        const auto replica = ir::replica_proxy_t{*l1desc.dataObjInfo};
        auto [reg_param, lm] = irods::experimental::make_key_value_proxy({{OPEN_TYPE_KW, std::to_string(l1desc.openType)}});

        reg_param[DATA_SIZE_KW] = std::to_string(replica.size());

        if (OPEN_FOR_WRITE_TYPE == l1desc.openType) {
            reg_param[ALL_REPL_STATUS_KW] = "";
            reg_param[DATA_MODIFY_KW] = std::to_string((int)time(nullptr));
        }
        else {
            reg_param[REPL_STATUS_KW] = std::to_string(GOOD_REPLICA);
        }

        const auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        if (cond_input.contains(CHKSUM_KW)) {
            reg_param[CHKSUM_KW] = cond_input.at(CHKSUM_KW);
        }

        // adding FILE_PATH_KW for decoupled naming in S3
        reg_param[FILE_PATH_KW] = l1desc.dataObjInfo->filePath;

        modDataObjMeta_t mod_inp{};
        mod_inp.dataObjInfo = l1desc.dataObjInfo;
        mod_inp.regParam = reg_param.get();
        const int status = rsModDataObjMeta(&_comm, &mod_inp);
        if (status < 0) {
            THROW(status,
                fmt::format("{}: rsModDataObjMeta failed with {}",
                __FUNCTION__, status));
        }

        if (GOOD_REPLICA == replica.replica_status()) {
            updatequotaOverrun(replica.hierarchy().data(), replica.size(), ALL_QUOTA);
        }

        return status;
    } // finalize_for_put_or_copy

    auto finalize_replica_after_failed_operation(RsComm& _comm, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        auto replica = ir::replica_proxy_t{*l1desc.dataObjInfo};

        const auto accmode = l1desc.dataObjInp->openFlags & O_ACCMODE;
        if (O_RDONLY != accmode) {
            replica.replica_status(STALE_REPLICA);
            //replica.mtime(SET_TIME_TO_NOW_KW);
            replica.mtime(std::to_string((int)time(nullptr)));
        }

        const rodsLong_t vault_size = getSizeInVault(&_comm, replica.get());
        if (vault_size < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "{} - getSizeInVault failed [{}]",
                __FUNCTION__, vault_size));
            return vault_size;
        }

        replica.size(vault_size);

        auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        auto [reg_param, lm] = irods::experimental::make_key_value_proxy();
        reg_param[DATA_SIZE_KW] = std::to_string(replica.size());
        reg_param[IN_PDMO_KW] = replica.hierarchy();
        if (cond_input.contains(ADMIN_KW)) {
            reg_param[ADMIN_KW] = "";
        }

        modDataObjMeta_t mod_inp{};
        mod_inp.dataObjInfo = replica.get();
        mod_inp.regParam = reg_param.get();
        if (const int ec = rsModDataObjMeta(&_comm, &mod_inp); ec < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "{}: rsModDataObjMeta failed, dataSize [{}] status = {}",
                __FUNCTION__, replica.size(), ec));
            return ec;
        }

        // return error code - this is a failure mode
        return L1desc[_fd].oprStatus;
    } // finalize_replica_after_failed_operation

    auto finalize_replica_with_no_bytes_written(RsComm& _comm, const int _fd) -> void
    {
        l1desc_t& l1desc = L1desc[_fd];

        if (L1desc[_fd].purgeCacheFlag) {
            irods::purge_cache(_comm, *l1desc.dataObjInfo);
        }

        try {
            irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);
        }
        catch ( const irods::exception& e ) {
            if ( l1desc.dataObjInp->oprType == PUT_OPR ) {
                rsDataObjUnlink(&_comm, l1desc.dataObjInp );
            }
            throw;
        }

        // TODO: will need to restore original status for open-for-read replicas...
        if (const auto accmode = l1desc.dataObjInp->openFlags & O_ACCMODE; O_RDONLY == accmode) {
            return;
        }

        auto replica = ir::replica_proxy_t{*l1desc.dataObjInfo};

        auto [reg_param, lm] = irods::experimental::make_key_value_proxy({{OPEN_TYPE_KW, std::to_string(l1desc.openType)}});

        if (PUT_OPR == l1desc.oprType && l1desc.chksumFlag) {
            const std::string checksum = perform_checksum_operation_for_finalize(_comm, _fd);
            if (checksum.empty()) {
                return;
            }

            reg_param[CHKSUM_KW] = checksum;
        }

        if (l1desc.openType == CREATE_TYPE) {
            replica.replica_status(GOOD_REPLICA);
            reg_param[REPL_STATUS_KW] = std::to_string(replica.replica_status());
        }
        else if (l1desc.openType == OPEN_FOR_WRITE_TYPE) {
            replica.replica_status(std::stoi(rst::get_property(replica.logical_path(), replica.replica_number(), "data_is_dirty")));
            reg_param[REPL_STATUS_KW] = std::to_string(replica.replica_status());
            reg_param[DATA_MODIFY_KW] = std::to_string((int)time(nullptr));
        }

        if (CREATE_TYPE == l1desc.openType) {
            // TODO: need to restore replica status for OPEN_FOR_WRITE
            // a newly created replica should be marked as good if no bytes were written
            // as it is created in the intermediate state.
            reg_param[REPL_STATUS_KW] = std::to_string(GOOD_REPLICA);
        }

        modDataObjMeta_t mod_inp{};
        mod_inp.dataObjInfo = replica.get();
        mod_inp.regParam = reg_param.get();
        if (const int ec = rsModDataObjMeta(&_comm, &mod_inp); ec < 0) {
            irods::log(LOG_ERROR, fmt::format("[{}] - rsModDataObjMeta failed with [{}]", __FUNCTION__, ec));
        }
    } // finalize_replica_with_no_bytes_written

    auto close_physical_file(RsComm& _comm, const int _fd) -> void
    {
        const int l3descInx = L1desc[_fd].l3descInx;
        if (l3descInx < 3) {
            // This message will appear a lot for single buffer gets -- it is not necessarily an error
            irods::log(LOG_DEBUG, fmt::format("invalid l3 descriptor index [{}]", l3descInx));
            return;
        }

        if (const int ec = l3Close(&_comm, _fd); ec < 0) {
            THROW(ec, fmt::format("l3Close of {} failed, status = {}", l3descInx, ec));
        }
    } // close_physical_file

    auto update_checksum_if_needed(RsComm& _comm, const int _fd) -> std::string
    {
        l1desc_t& l1desc = L1desc[_fd];

        bool update_checksum = !getValByKey(&l1desc.dataObjInp->condInput, NO_CHK_COPY_LEN_KW);
        if ((OPEN_FOR_WRITE_TYPE == l1desc.openType || CREATE_TYPE == l1desc.openType) &&
            !std::string_view{l1desc.dataObjInfo->chksum}.empty()) {
            l1desc.chksumFlag = REG_CHKSUM;
            update_checksum = true;
        }

        if (!update_checksum) {
            return "";
        }

        try{
            return perform_checksum_operation_for_finalize(_comm, _fd);
        }
        catch (const irods::exception& e) {
            if (const int ec = irods::stale_replica(_comm, l1desc, *l1desc.dataObjInfo); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "{} - rsModDataObjMeta failed [{}]",
                    __FUNCTION__, ec));
            }

            throw;
        }
    } // update_checksum_if_needed

    int finalize_replica(RsComm& _comm, const int _fd, openedDataObjInp_t& _inp)
    {
        auto& l1desc = L1desc[_fd];

        auto opened_replica = ir::replica_proxy_t{*l1desc.dataObjInfo};

        if (l1desc.oprStatus < 0) {
            return finalize_replica_after_failed_operation(_comm, _fd);
        }

        if (cross_zone_write_operation(_inp, l1desc)) {
            l1desc.bytesWritten = _inp.bytesWritten;
        }

        try {
            if (!bytes_written_in_operation(l1desc)) {
                finalize_replica_with_no_bytes_written(_comm, _fd);
                return 0;
            }

            const bool verify_size = !getValByKey(&l1desc.dataObjInp->condInput, NO_CHK_COPY_LEN_KW);
            const auto size_in_vault = irods::get_size_in_vault(_comm, *l1desc.dataObjInfo, verify_size, l1desc.dataSize);
            opened_replica.size(size_in_vault);
        }
        catch (const irods::exception& e) {
            if (const int ec = irods::stale_replica(_comm, l1desc, *l1desc.dataObjInfo); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "{} - rsModDataObjMeta failed [{}]",
                    __FUNCTION__, ec));
            }

            throw;
        }

        const auto checksum = update_checksum_if_needed(_comm, _fd);
        // TODO: shouldn't this only happen if the string is not empty?
        irods::experimental::key_value_proxy{l1desc.dataObjInp->condInput}[CHKSUM_KW] = checksum;

        int status = 0;
        if (!opened_replica.special_collection_info()) {
            switch (l1desc.oprType) {
                case PUT_OPR:
                case COPY_DEST:
                    status = finalize_for_put_or_copy(_comm, _inp, _fd);
                    break;

                case REPLICATE_DEST:
                    status = finalize_destination_replica_for_replication(_comm, _inp, _fd);
                    break;

                default:
                    status = finalize_data_object(_comm, _inp, _fd);
                    break;
            }
        }

        l1desc.bytesWritten = l1desc.dataObjInfo->dataSize;
        opened_replica.size(l1desc.dataObjInfo->dataSize); // no-op?

        if (L1desc[_fd].purgeCacheFlag) {
            irods::purge_cache(_comm, *l1desc.dataObjInfo);
        }

        return status;
    } // finalize_replica

    void unlock_file_descriptor(rsComm_t* comm, const int l1descInx)
    {
        char fd_string[NAME_LEN]{};
        snprintf(fd_string, sizeof( fd_string ), "%-d", L1desc[l1descInx].lockFd);
        addKeyVal(&L1desc[l1descInx].dataObjInp->condInput, LOCK_FD_KW, fd_string);
        irods::server_api_call(
            DATA_OBJ_UNLOCK_AN,
            comm,
            L1desc[l1descInx].dataObjInp,
            nullptr, (void**)nullptr, nullptr);
        L1desc[l1descInx].lockFd = -1;
    } // unlock_file_descriptor
} // anonymous namespace

auto l3Close(RsComm* _comm, const int _fd) -> int
{
    auto& l1desc = L1desc[_fd];

    auto replica = ir::replica_proxy_t{*l1desc.dataObjInfo};
    if (getStructFileType(replica.special_collection_info()) >= 0) {
        std::string location{};
        if (irods::error ret = irods::get_loc_for_hier_string(replica.hierarchy().data(), location); !ret.ok()) {
            irods::log( PASSMSG( "l3Close - failed in get_loc_for_hier_string", ret ) );
            return ret.code();
        }

        subStructFileFdOprInp_t inp{};
        inp.type = replica.special_collection_info()->type;
        inp.fd = l1desc.l3descInx;
        rstrcpy(inp.addr.hostAddr, location.c_str(), NAME_LEN);
        rstrcpy(inp.resc_hier, replica.hierarchy().data(), MAX_NAME_LEN);
        return rsSubStructFileClose(_comm, &inp);
    }

    fileCloseInp_t inp{};
    inp.fileInx = l1desc.l3descInx;
    rstrcpy(inp.in_pdmo, l1desc.in_pdmo, MAX_NAME_LEN);
    return rsFileClose(_comm, &inp);
} // l3Close

int rsDataObjClose(rsComm_t* rsComm, openedDataObjInp_t* dataObjCloseInp)
{
    if (!rsComm) {
        irods::log(LOG_ERROR, fmt::format("{}: rsComm is null", __FUNCTION__));
        return USER__NULL_INPUT_ERR;
    }

    if (!dataObjCloseInp) {
        irods::log(LOG_ERROR, fmt::format("{}: dataObjCloseInp is null", __FUNCTION__));
        return USER__NULL_INPUT_ERR;
    }

    const auto fd = dataObjCloseInp->l1descInx;
    if (fd < 3 || fd >= NUM_L1_DESC) {
        irods::log(LOG_NOTICE, fmt::format("{}: l1descInx {} out of range", __FUNCTION__, fd));
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    const auto& l1desc = L1desc[fd];

    if(!l1desc.dataObjInp) {
        irods::log(LOG_ERROR, fmt::format("{}: dataObjInp for index {} is null", __FUNCTION__, fd));
        return SYS_INVALID_INPUT_PARAM;
    }

    if (!l1desc.dataObjInfo) {
        irods::log(LOG_ERROR, fmt::format("[{}] - dataObjInfo for index {} is null", __FUNCTION__, fd));
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    std::unique_ptr<irods::at_scope_exit<std::function<void()>>> restore_entry;

    int ec = 0;

    // Replica access tokens only apply to write operations.
    if ((l1desc.dataObjInp->openFlags & O_ACCMODE) != O_RDONLY) {
        if (!l1desc.replica_token.empty()) {
            namespace rat = irods::experimental::replica_access_table;

            // Capture the replica token and erase the PID from the replica access table.
            // This must always happen before calling "irsDataObjClose" because other operations
            // may attempt to open this replica, but will fail because those operations do not
            // have the replica token.
            if (auto entry = rat::erase_pid(l1desc.replica_token, getpid()); entry) {
                // "entry" should always be populated in normal situations.
                // Because closing a data object triggers a file modified notification, it is
                // important to be able to restore the previously removed replica access table entry.
                // This is required so that the iRODS state is maintained in relation to the client.
                restore_entry.reset(new irods::at_scope_exit<std::function<void()>>{
                    [&ec, e = std::move(*entry)] {
                        if (ec != 0) {
                            rat::restore(e);
                        }
                    }
                });
            }
        }
        else {
            irods::experimental::log::api::warn("No replica access token in L1 descriptor. Ignoring replica access table. "
                                                "[path={}, resource_hierarchy={}]",
                                                l1desc.dataObjInfo->objPath, l1desc.dataObjInfo->rescHier);
        }
    }
    // Capture the error code so that the at_scope_exit handler can respond to it.

    // ensure that l1 descriptor is in use before closing
    if (l1desc.inuseFlag != FD_INUSE) {
        rodsLog(LOG_ERROR, "rsDataObjClose: l1descInx %d out of range", fd);
        return ec = BAD_INPUT_DESC_INDEX;
    }

    // sanity check for in-flight l1 descriptor
    if(!l1desc.dataObjInp) {
        rodsLog(LOG_ERROR,
            "rsDataObjClose: invalid dataObjInp for index %d", fd);
        return ec = SYS_INVALID_INPUT_PARAM;
    }

    if (l1desc.remoteZoneHost) {
        dataObjCloseInp->l1descInx = l1desc.remoteL1descInx;
        ec = rcDataObjClose( l1desc.remoteZoneHost->conn, dataObjCloseInp );
        dataObjCloseInp->l1descInx = fd;
        freeL1desc(fd);
        return ec;
    }

    try {
        irods::at_scope_exit clean_up{[&fd, &rsComm]
        {
            auto& l1desc = L1desc[fd];

            if (l1desc.lockFd > 0) {
                unlock_file_descriptor(rsComm, fd);
            }

            const std::string logical_path = l1desc.dataObjInfo->objPath;

            freeL1desc(fd);

            if (rst::contains(logical_path)) {
                rst::erase(logical_path);
            }
        }};

        close_physical_file(*rsComm, fd);

        ec = finalize_replica(*rsComm, fd, *dataObjCloseInp);
        if (ec < 0 || l1desc.oprStatus < 0) {
            return ec;
        }

        apply_static_peps(*rsComm, *dataObjCloseInp, fd, ec);

        return ec;
    }
    catch (const irods::exception& e) {
        irods::log(e);
        return ec = e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
        return ec = SYS_INTERNAL_ERR;
    }
    catch (...) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - an unknown error occurred", __FUNCTION__, __LINE__));
        return ec = SYS_UNKNOWN_ERROR;
    }
} // rsDataObjClose

