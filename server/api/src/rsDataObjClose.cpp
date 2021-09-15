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

#include "finalize_utilities.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_serialization.hpp"
#include "irods_server_api_call.hpp"
#include "irods_server_properties.hpp"
#include "irods_stacktrace.hpp"
#include "json_serialization.hpp"
#include "key_value_proxy.hpp"
#include "replica_access_table.hpp"
#include "replica_state_table.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "logical_locking.hpp"

#include <memory>
#include <functional>
#include <sys/types.h>
#include <unistd.h>

namespace
{
    namespace ill = irods::logical_locking;
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

        // This section only exists for the compound resource as replication handles its own finalizing logic.
        if (oprType == REPLICATE_DEST) {
            const int srcL1descInx = l1desc.srcL1descInx;
            if (srcL1descInx < 3) {
                THROW(SYS_FILE_DESC_OUT_OF_RANGE, fmt::format(
                    "{}: srcL1descInx {} out of range",
                    __FUNCTION__, srcL1descInx));
            }

            auto source_replica = ir::replica_proxy_t{*L1desc[srcL1descInx].dataObjInfo};
            if (!source_replica.checksum().empty() && STALE_REPLICA != source_replica.replica_status()) {
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

        if (0 == l1desc.chksumFlag) {
            if (destination_replica.checksum().empty()) {
                return {};
            }
            l1desc.chksumFlag = VERIFY_CHKSUM;
        }

        if (VERIFY_CHKSUM == l1desc.chksumFlag) {
            if (!std::string_view{l1desc.chksum}.empty()) {
                return irods::verify_checksum(_comm, *destination_replica.get(), l1desc.chksum);
            }

            // Specifically for compound resources.
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

                if (!destination_replica.checksum().empty()) {
                    destination_replica.cond_input().erase(ORIG_CHKSUM_KW);

                    /* for replication, the chksum in dataObjInfo was duplicated */
                    if (destination_replica.checksum() != checksum_string) {
                        THROW(USER_CHKSUM_MISMATCH, fmt::format(
                            "{}:mismatch chksum for {}.Rcat={},comp {}",
                            __FUNCTION__, destination_replica.logical_path(), destination_replica.checksum(), checksum_string));
                    }
                }

                return checksum_string;
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

                if (!source_replica.checksum().empty()) {
                    destination_replica.cond_input()[ORIG_CHKSUM_KW] = source_replica.checksum();
                }

                if (const int ec = _dataObjChksum(&_comm, destination_replica.get(), &checksum_string ); ec < 0) {
                    THROW(ec, "failed in _dataObjChksum");
                }

                if (!checksum_string) {
                    THROW(SYS_INTERNAL_NULL_INPUT_ERR, "checksum_string is NULL");
                }

                if (!source_replica.checksum().empty()) {
                    destination_replica.cond_input().erase(ORIG_CHKSUM_KW);
                    if (source_replica.checksum() != checksum_string) {
                        THROW(USER_CHKSUM_MISMATCH, fmt::format(
                            "{}:mismatch chksum for {}.Rcat={},comp {}",
                            __FUNCTION__, destination_replica.logical_path(), source_replica.checksum(), checksum_string));
                    }
                }

                return checksum_string;
            }

            return {};
        }

        return irods::register_new_checksum(_comm, *destination_replica.get(), l1desc.chksum);
    } // perform_checksum_operation_for_finalize

    auto finalize_destination_replica_for_replication(RsComm& _comm, const OpenedDataObjInp& _inp, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        auto d = ir::make_replica_proxy(*l1desc.dataObjInfo);

        auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        const auto admin_op = cond_input.contains(ADMIN_KW);

        const int source_fd = l1desc.srcL1descInx;
        if (source_fd < 3) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - source L1 descriptor out of range [fd=[{}], path=[{}]",
                __FUNCTION__, __LINE__, source_fd, d.logical_path()));

            if (const auto unlock_ec = ill::unlock_and_publish(_comm, {d, admin_op}, STALE_REPLICA); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code=[{}]]", __FUNCTION__, __LINE__, unlock_ec));
            }

            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        const auto s = ir::make_replica_proxy(*L1desc[source_fd].dataObjInfo);

        const auto source_replica_original_status = ill::get_original_replica_status(s.data_id(), s.replica_number());
        d.replica_status(source_replica_original_status);
        d.size(s.size());
        d.mtime(SET_TIME_TO_NOW_KW);

        rst::update(d.data_id(), d);

        ill::unlock(d.data_id(), d.replica_number(), d.replica_status(), ill::restore_status);

        cond_input[OPEN_TYPE_KW] = std::to_string(l1desc.openType);

        const auto input_keywords = irods::experimental::make_key_value_proxy(_inp.condInput);
        if (input_keywords.contains(IN_PDMO_KW)) {
            cond_input[IN_PDMO_KW] = input_keywords.at(IN_PDMO_KW).value();
        }

        const auto [ret, ec] = rst::publish::to_catalog(_comm, {d, *cond_input.get()});

        if (CREATE_TYPE == l1desc.openType) {
            updatequotaOverrun(d.hierarchy().data(), d.size(), ALL_QUOTA);
        }

        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to finalize data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, d.logical_path(), d.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, d.data_id(), d.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            l1desc.oprStatus = ec;

            if (CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME != ec) {
                l3Unlink(&_comm, d.get());
            }
        }

        return ec;
    } // finalize_destination_replica_for_replication

    auto finalize_data_object(RsComm& _comm, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        auto r = ir::make_replica_proxy(*l1desc.dataObjInfo);

        auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);

        if (cross_zone_copy_operation(l1desc)) {
            // TODO: unclear whether this can happen - I feel like it cannot
            if (const int ec = svrRegDataObj(&_comm, r.get()); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to register object for cross-zone copy operation "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

                const auto admin_op = cond_input.contains(ADMIN_KW);
                if (const auto unlock_ec = ill::unlock_and_publish(_comm, {r, admin_op}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code=[{}]]", __FUNCTION__, __LINE__, unlock_ec));
                }

                return ec;
            }
        }

        irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
        irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);

        cond_input[OPEN_TYPE_KW] = std::to_string(l1desc.openType);

        // The default sibling replica status on finalize should be stale. This is because
        // reaching this point necessarily means that a replica of the data object was opened
        // with the ability to modify its contents. Therefore, we must assume that the truth
        // has moved unless there is evidence that no bytes were written.
        int sibling_status = STALE_REPLICA;

        if (CREATE_TYPE == l1desc.openType) {
            // If no errors occurred, creating a new replica with no specified operation type
            // should result in staling any sibling replicas. This is because it is not known
            // whether the new replica is consistent with the siblings.
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - created new replica, STALE sibling replicas [path=[{}], hier=[{}]]",
                __FUNCTION__, __LINE__, r.logical_path(), r.hierarchy()));
            r.replica_status(GOOD_REPLICA);
        }
        else if (OPEN_FOR_WRITE_TYPE == l1desc.openType) {
            if (l1desc.bytesWritten > 0) {
                // If no errors occurred, bytes being written to an existing replica should
                // result in staling any sibling replicas. This is because it is not known
                // whether the new replica is consistent with the siblings.
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - MORE THAN 0 bytes written to existing replica, STALE sibling replicas [path=[{}], hier=[{}]]",
                    __FUNCTION__, __LINE__, r.logical_path(), r.hierarchy()));
                r.replica_status(GOOD_REPLICA);
                r.mtime(SET_TIME_TO_NOW_KW);
            }
            else {
                // If no bytes were written to an existing replica after opening for write,
                // the sibling replica statuses should be restored. This is because nothing
                // changed about the "truth" of the data represented by the replicas.
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - NO bytes written to existing replica, RESTORE sibling replicas [path=[{}], hier=[{}]]",
                    __FUNCTION__, __LINE__, r.logical_path(), r.hierarchy()));
                r.replica_status(std::stoi(rst::get_property(r.data_id(), r.replica_number(), "data_is_dirty")));
                sibling_status = ill::restore_status;
            }
        }

        rst::update(r.data_id(), r);

        if (const auto ec = ill::unlock(r.data_id(), r.replica_number(), r.replica_status(), sibling_status); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            return ec;
        }

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, {r, *cond_input.get()}); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to finalize data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, r.data_id(), r.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            return ec;
        }

        if (GOOD_REPLICA == r.replica_status()) {
            updatequotaOverrun(r.hierarchy().data(), r.size(), ALL_QUOTA);
        }

        return 0;
    } // finalize_data_object

    auto update_replica_size_and_throw_on_failure(RsComm& _comm, const int _fd) -> void
    {
        auto& l1desc = L1desc[_fd];
        auto r = ir::make_replica_proxy(*l1desc.dataObjInfo);
        const auto admin_op = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput).contains(ADMIN_KW);

        // Simply get size from the vault without verifying with catalog
        // and determine how to proceed based on what is returned
        constexpr auto verify_size_in_vault = false;
        const auto size_in_vault = irods::get_size_in_vault(_comm, *l1desc.dataObjInfo, verify_size_in_vault, l1desc.dataSize);
        if (size_in_vault < 0) {
            // Failed to get the catalog size. Unlock data object and throw.
            if (const int ec = ill::unlock_and_publish(_comm, {r, admin_op, 0}, STALE_REPLICA, ill::restore_status); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to unlock data object "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));
            }

            THROW(size_in_vault, fmt::format(
                "[{}:{}] - failed to get size in vault "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, size_in_vault, r.logical_path(), r.hierarchy()));
        }

        if (const bool verify_size = !getValByKey(&l1desc.dataObjInp->condInput, NO_CHK_COPY_LEN_KW);
            verify_size && l1desc.dataSize > 0 && size_in_vault != l1desc.dataSize)
        {
            // The size in the vault does not match the expected size in the catalog.
            // Update the size in the RST and unlock the data object and throw.
            r.size(size_in_vault);

            // TODO: update mtime?

            rst::update(r.data_id(), r);

            if (const int ec = ill::unlock_and_publish(_comm, {r, admin_op}, STALE_REPLICA, ill::restore_status); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to unlock data object "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));
            }

            THROW(size_in_vault, fmt::format(
                "[{}:{}] - failed to get size in vault "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, size_in_vault, r.logical_path(), r.hierarchy()));
        }

        r.size(size_in_vault);
    } // update_replica_size_and_throw_on_failure

    auto update_replica_checksum_and_throw_on_failure(RsComm& _comm, const int _fd) -> void
    {
        auto& l1desc = L1desc[_fd];
        auto cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
        auto r = ir::make_replica_proxy(*l1desc.dataObjInfo);

        try {
            const auto size_should_be_verified = !getValByKey(&l1desc.dataObjInp->condInput, NO_CHK_COPY_LEN_KW);

            if (size_should_be_verified) {
                if (PUT_OPR == l1desc.oprType) {
                    if (REG_CHKSUM == l1desc.chksumFlag || VERIFY_CHKSUM == l1desc.chksumFlag) {
                        // Update the replica proxy's checksum value so that if an exception is thrown,
                        // the checksum value will still be stored in the catalog.
                        r.checksum(irods::register_new_checksum(_comm, *r.get(), l1desc.chksum));
                        rst::update(r.data_id(), r);

                        if (VERIFY_CHKSUM == l1desc.chksumFlag) {
                            rodsLog(LOG_DEBUG,
                                    "[%s:%d] Verifying checksum [calculated_checksum=%s, checksum_from_client=%s, path=%s, replica_number=%d] ...",
                                    __func__, __LINE__, r.checksum().data(), l1desc.chksum, r.logical_path().data(), r.replica_number());

                            if (r.checksum() != l1desc.chksum) {
                                THROW(USER_CHKSUM_MISMATCH, fmt::format("[{}:{}] Mismatch checksum for {}.inp={}, compute {}",
                                      __FUNCTION__, __LINE__, r.logical_path(), l1desc.chksum, r.checksum()));
                            }
                        }

                        // Copy the new checksum value into the conditional input so that file modified is triggered.
                        cond_input[CHKSUM_KW] = r.checksum();
                    }

                    return;
                }

                if (const std::string checksum = perform_checksum_operation_for_finalize(_comm, _fd); !checksum.empty()) {
                    cond_input[CHKSUM_KW] = checksum;
                }

                if (cond_input.contains(CHKSUM_KW)) {
                    r.checksum(cond_input.at(CHKSUM_KW).value());
                }
            }
        }
        catch (const irods::exception& e) {
            const auto admin_op = cond_input.contains(ADMIN_KW);
            if (const int unlock_ec = ill::unlock_and_publish(_comm, {r, admin_op}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code=[{}]]", __FUNCTION__, __LINE__, unlock_ec));
            }

            throw;
        }
    } // update_replica_checksum_and_throw_on_failure

    auto close_physical_file_and_throw_on_failure(RsComm& _comm, const int _fd) -> void
    {
        auto& l1desc = L1desc[_fd];

        if (l1desc.l3descInx > 2) {
            if (const auto close_ec = l3Close(&_comm, _fd); close_ec < 0) {
                auto r = ir::make_replica_proxy(*l1desc.dataObjInfo);

                const auto admin_op = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput).contains(ADMIN_KW);

                if (const int unlock_ec = ill::unlock_and_publish(_comm, {r, admin_op}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code=[{}]]", __FUNCTION__, __LINE__, unlock_ec));
                }

                THROW(close_ec, fmt::format(
                    "[{}:{}] - failed to close physical file "
                    "[error_code=[{}], path=[{}], hierarchy=[{}], physical_path=[{}]]",
                    __FUNCTION__, __LINE__, close_ec, r.logical_path(), r.hierarchy(), r.physical_path()));
            }
        }
    } // close_physical_file_and_throw_on_failure

    auto finalize_replica_for_failed_operation(RsComm& _comm, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        auto r = ir::make_replica_proxy(*l1desc.dataObjInfo);

        r.replica_status(STALE_REPLICA);
        r.mtime(SET_TIME_TO_NOW_KW);

        rst::update(r.data_id(), r);

        if (const auto ec = ill::unlock(r.data_id(), r.replica_number(), r.replica_status(), ill::restore_status); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            return ec;
        }

        const auto admin_op = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput).contains(ADMIN_KW);
        const auto ctx = rst::publish::context{r, admin_op};

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, ctx); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to finalize data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, r.data_id(), r.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            return ec;
        }

        return l1desc.oprStatus;
    } // finalize_replica_for_failed_operation

    auto close_replica(RsComm& _comm, OpenedDataObjInp& _inp, const int _fd) -> int
    {
        close_physical_file_and_throw_on_failure(_comm, _fd);

        auto& l1desc = L1desc[_fd];

        if (cross_zone_write_operation(_inp, l1desc)) {
            l1desc.bytesWritten = _inp.bytesWritten;
        }

        if (bytes_written_in_operation(l1desc)) {
            if (O_RDONLY != (l1desc.dataObjInp->openFlags & O_ACCMODE)) {
                auto rp = ir::replica_proxy_t{*l1desc.dataObjInfo};
                rp.checksum("");
                rst::update(rp.data_id(), rp);

                auto file_modified_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
                file_modified_input[CHKSUM_KW] = rp.checksum();
            }
        }
        else if (O_RDONLY == (l1desc.dataObjInp->openFlags & O_ACCMODE)) {
            irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);

            apply_static_peps(_comm, _inp, _fd, l1desc.oprStatus);

            if (L1desc[_fd].purgeCacheFlag) {
                irods::purge_cache(_comm, *l1desc.dataObjInfo);
            }

            return l1desc.oprStatus;
        }

        update_replica_size_and_throw_on_failure(_comm, _fd);
        update_replica_checksum_and_throw_on_failure(_comm, _fd);

        if (l1desc.oprStatus < 0) {
            return finalize_replica_for_failed_operation(_comm, _fd);
        }

        const auto ec = REPLICATE_DEST == l1desc.oprType
                      ? finalize_destination_replica_for_replication(_comm, _inp, _fd)
                      : finalize_data_object(_comm, _fd);

        l1desc.bytesWritten = l1desc.dataObjInfo->dataSize;

        if (L1desc[_fd].purgeCacheFlag) {
            irods::purge_cache(_comm, *l1desc.dataObjInfo);
        }

        apply_static_peps(_comm, _inp, _fd, l1desc.oprStatus);

        return ec < 0 ? ec : l1desc.oprStatus;
    } // close_replica

    auto close_replica_in_special_collection(RsComm& _comm, OpenedDataObjInp& _inp, const int _fd) -> int
    {
        auto& l1desc = L1desc[_fd];

        if (l1desc.l3descInx > 2) {
            if (const auto ec = l3Close(&_comm, _fd); ec < 0) {
                return ec;
            }
        }

        if (cross_zone_write_operation(_inp, l1desc)) {
            l1desc.bytesWritten = _inp.bytesWritten;
        }

        if (l1desc.oprStatus >= 0) {
            if (l1desc.purgeCacheFlag) {
                irods::purge_cache(_comm, *l1desc.dataObjInfo);
            }

            irods::apply_metadata_from_cond_input(_comm, *l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *l1desc.dataObjInp);
        }

        if (l1desc.oprStatus >= 0) {
            apply_static_peps(_comm, _inp, _fd, l1desc.oprStatus);
        }

        return l1desc.oprStatus;
    } // close_replica_in_special_collection
} // anonymous namespace

auto l3Close(RsComm* _comm, const int _fd) -> int
{
    auto& l1desc = L1desc[_fd];

    auto r = ir::replica_proxy_t{*l1desc.dataObjInfo};
    if (getStructFileType(r.special_collection_info()) >= 0) {
        std::string location{};
        if (irods::error ret = irods::get_loc_for_hier_string(r.hierarchy().data(), location); !ret.ok()) {
            irods::log( PASSMSG( "l3Close - failed in get_loc_for_hier_string", ret ) );
            return ret.code();
        }

        subStructFileFdOprInp_t inp{};
        inp.type = r.special_collection_info()->type;
        inp.fd = l1desc.l3descInx;
        rstrcpy(inp.addr.hostAddr, location.c_str(), NAME_LEN);
        rstrcpy(inp.resc_hier, r.hierarchy().data(), MAX_NAME_LEN);
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
            // This must always happen before closing the replica because other operations
            // may attempt to open this replica, but will fail because those operations do
            // not have the replica token.
            if (auto entry = rat::erase_pid(l1desc.replica_token, getpid()); entry) {
                // "entry" should always be populated in normal situations.
                // Because closing a data object triggers a file modified notification, it is
                // important to be able to restore the previously removed replica access table entry.
                // This is required so that the iRODS state is maintained in relation to the client.
                restore_entry.reset(new irods::at_scope_exit<std::function<void()>>{
                    [&l1desc, e = std::move(*entry)] {
                        if (FD_INUSE == l1desc.inuseFlag) {
                            rat::restore(e);
                            rodsLog(LOG_DEBUG, "[%s:%d] Restored replica access table entry [path=%s, replica_number=%d]",
                                    __func__, __LINE__, l1desc.dataObjInfo->objPath, l1desc.dataObjInfo->replNum);
                        }
                    }
                });
            }
        }
        else {
            irods::log(LOG_WARNING, fmt::format("No replica access token in L1 descriptor. Ignoring replica access table. "
                                                "[path={}, resource_hierarchy={}]",
                                                l1desc.dataObjInfo->objPath, l1desc.dataObjInfo->rescHier));
        }
    }

    // ensure that l1 descriptor is in use before closing
    if (l1desc.inuseFlag != FD_INUSE) {
        rodsLog(LOG_ERROR, "rsDataObjClose: l1descInx %d out of range", fd);
        return ec = BAD_INPUT_DESC_INDEX;
    }

    // sanity check for in-flight l1 descriptor
    if (!l1desc.dataObjInp) {
        rodsLog(LOG_ERROR, "rsDataObjClose: invalid dataObjInp for index %d", fd);
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
        irods::at_scope_exit clean_up{[&fd]
        {
            const rst::key_type key = L1desc[fd].dataObjInfo->dataId;

            if (rst::contains(key)) {
                rst::erase(key);
            }

            freeL1desc(fd);
        }};

        if (L1desc[fd].dataObjInfo->specColl) {
            return ec = close_replica_in_special_collection(*rsComm, *dataObjCloseInp, fd);
        }

        return ec = close_replica(*rsComm, *dataObjCloseInp, fd);
    }
    catch (const irods::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
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

