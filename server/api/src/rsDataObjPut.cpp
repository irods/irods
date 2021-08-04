#include "dataObjPut.h"

#include "dataObjRepl.h"
#include "dataObjUnlink.h"
#include "dataPut.h"
#include "filePut.h"
#include "getRemoteZoneResc.h"
#include "icatHighLevelRoutines.hpp"
#include "modDataObjMeta.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rcGlobalExtern.h"
#include "regDataObj.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "rsApiHandler.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjPut.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsDataObjWrite.hpp"
#include "rsDataPut.hpp"
#include "rsFilePut.hpp"
#include "rsGlobalExtern.hpp"
#include "rsRegDataObj.hpp"
#include "rsSubStructFilePut.hpp"
#include "rsUnregDataObj.hpp"
#include "specColl.hpp"
#include "subStructFilePut.h"
#include "finalize_utilities.hpp"
#include "getRescQuota.h"
#include "json_serialization.hpp"
#include "modAVUMetadata.h"
#include "modAccessControl.h"
#include "rsGetRescQuota.hpp"
#include "rsModAVUMetadata.hpp"
#include "rsModAccessControl.hpp"
#include "rs_replica_close.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_logger.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_serialization.hpp"
#include "irods_server_properties.hpp"
#include "scoped_privileged_client.hpp"
#include "server_utilities.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "logical_locking.hpp"

#include <cstring>
#include <chrono>
#include <string>
#include <tuple>
#include <algorithm>
#include <exception>

namespace
{
    namespace ir = irods::experimental::replica;
    namespace ill = irods::logical_locking;
    namespace rst = irods::replica_state_table;

    auto apply_static_peps(RsComm& _comm, l1desc& _l1desc, const int _operation_status) -> void
    {
        if (_l1desc.openType == CREATE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForCreate");
        }
        else if (_l1desc.openType == OPEN_FOR_WRITE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForOpen");
        }

        irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForPut");
    } // apply_static_peps

    auto calculate_checksum(RsComm& _comm, l1desc& _l1desc, DataObjInfo& _info) -> std::string
    {
        if (REG_CHKSUM == _l1desc.chksumFlag || VERIFY_CHKSUM == _l1desc.chksumFlag) {
            return irods::register_new_checksum(_comm, _info, _l1desc.chksum);
        }

        return {};
    } // calculate_checksum

    auto finalize_on_failure(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> int
    {
        const auto admin_op = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput).contains(ADMIN_KW);

        auto r = ir::make_replica_proxy(_info);

        r.replica_status(STALE_REPLICA);
        r.mtime(SET_TIME_TO_NOW_KW);

        const rodsLong_t vault_size = getSizeInVault(&_comm, r.get());
        if (vault_size < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to get size in vault "
                "[error code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, vault_size,
                r.logical_path(), r.hierarchy()));

            if (const int unlock_ec = ill::unlock_and_publish(_comm, {r, admin_op, 0}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
            }

            return vault_size;
        }

        r.size(vault_size);

        // Write it out to the catalog
        rst::update(r.data_id(), r);

        if (const auto ec = ill::unlock(r.data_id(), r.replica_number(), r.replica_status(), ill::restore_status); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            return ec;
        }

        const auto ctx = rst::publish::context{r, admin_op};

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, ctx); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed in finalize step "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, r.data_id(), r.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            return ec;
        }

        return 0;
    } // finalize_on_failure

    auto finalize_replica(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> int
    {
        auto r = ir::make_replica_proxy(_info);

        auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);

        const auto admin_op = cond_input.contains(ADMIN_KW);

        try {
            const bool verify_size = !cond_input.contains(NO_CHK_COPY_LEN_KW);
            const auto size_in_vault = irods::get_size_in_vault(_comm, _info, verify_size, _l1desc.dataSize);
            if (size_in_vault < 0) {
                THROW(size_in_vault, fmt::format(
                    "[{}:{}] - failed to get size in vault; "
                    "[error_code=[{}], path=[{}] hierarchy=[{}]]",
                    __FUNCTION__, __LINE__,
                    size_in_vault, r.logical_path(), r.hierarchy()));
            }
            r.size(size_in_vault);

            // Update the replica proxy's checksum value so that if an exception is thrown,
            // the checksum value will still be stored in the catalog.
            r.checksum(calculate_checksum(_comm, _l1desc, _info));

            rodsLog(LOG_DEBUG,
                    "[%s:%d] Verifying checksum [calculated_checksum=%s, checksum_from_client=%s, path=%s, replica_number=%d] ...",
                    __func__, __LINE__, r.checksum().data(), _l1desc.chksum,
                    r.logical_path().data(), r.replica_number());

            if (VERIFY_CHKSUM == _l1desc.chksumFlag) {
                if (r.checksum() != _l1desc.chksum) {
                    THROW(USER_CHKSUM_MISMATCH, fmt::format("[{}:{}] Mismatch checksum for {}.inp={}, compute {}",
                          __FUNCTION__, __LINE__, r.logical_path(), _l1desc.chksum, r.checksum()));
                }
            }

            // Copy the new checksum value into the conditional input so that file modified is triggered.
            cond_input[CHKSUM_KW] = r.checksum();

            irods::apply_metadata_from_cond_input(_comm, *_l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *_l1desc.dataObjInp);
        }
        catch (const irods::exception& e) {
            // Commit the changes to the replica state table so that unlock_and_publish stores
            // the size and checksum in the catalog.
            rst::update(r.data_id(), r);

            if (const int ec = ill::unlock_and_publish(_comm, {r, admin_op, 0}, STALE_REPLICA, ill::restore_status); ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, ec));
            }

            throw;
        }

        cond_input[OPEN_TYPE_KW] = std::to_string(_l1desc.openType);

        // Set target replica to the state it should be
        if (OPEN_FOR_WRITE_TYPE == _l1desc.openType) {
            r.replica_status(GOOD_REPLICA);
            r.mtime(SET_TIME_TO_NOW_KW);

            // stale other replicas because the truth has moved
            ill::unlock(r.data_id(), r.replica_number(), GOOD_REPLICA, STALE_REPLICA);
        }
        else {
            // No unlock is required here because a put that is not an overwrite implies a brand new
            // data object as put'ing to a new resource for an existing data object is not allowed.
            // Therefore, there should be no other replicas and so there should be nothing to unlock.
            r.replica_status(GOOD_REPLICA);
        }

        // Write it out to the catalog
        rst::update(r.data_id(), r);

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, {r, *cond_input.get()}); ec < 0) {
            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, r.data_id(), r.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }

            THROW(ec, fmt::format(
                "[{}:{}] - failed to finalize replica "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));
        }

        if (GOOD_REPLICA == r.replica_status()) {
            /* update quota overrun */
            updatequotaOverrun(r.hierarchy().data(), r.size(), ALL_QUOTA);
        }

        return 0;
    } // finalize_replica

    int single_buffer_put(RsComm& _comm, DataObjInp& _inp, BytesBuf& _bbuf)
    {
        _inp.openFlags = O_CREAT | O_RDWR | O_TRUNC;
        const int fd = rsDataObjOpen(&_comm, &_inp);
        if (fd < 3) {
            if (fd >= 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "{}: rsDataObjOpen of {} error, status = {}",
                    __FUNCTION__, _inp.objPath, fd));
                return SYS_FILE_DESC_OUT_OF_RANGE;
            }

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to open data object "
                "[error_code=[{}], path=[{}]]",
                __FUNCTION__, __LINE__, fd, _inp.objPath));
            return fd;
        }

        auto& l1desc = L1desc[fd];
        auto opened_replica = irods::experimental::replica::make_replica_proxy(*l1desc.dataObjInfo);
        const std::string hier = opened_replica.hierarchy().data();

        OpenedDataObjInp write_inp{};
        write_inp.len = _bbuf.len;
        write_inp.l1descInx = fd;

        BytesBuf write_bbuf{};
        write_bbuf.buf = _bbuf.buf;
        write_bbuf.len = _bbuf.len;

        const int bytes_written = rsDataObjWrite(&_comm, &write_inp, &write_bbuf);
        if (bytes_written < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - writing to data object failed "
                "[error code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, bytes_written,
                opened_replica.logical_path(), opened_replica.hierarchy()));
        }

        if ( bytes_written == 0 && opened_replica.size() > 0 ) {
            /* overwrite with 0 len file */
            l1desc.bytesWritten = 1;
        }
        else {
            l1desc.bytesWritten = bytes_written;
        }

        l1desc.dataSize = _inp.dataSize;

        int status = 0;
        // special collections are special - just use the old close
        if (l1desc.dataObjInfo->specColl) {
            OpenedDataObjInp close_inp{};
            close_inp.l1descInx = fd;
            l1desc.oprStatus = bytes_written;
            l1desc.oprType = PUT_OPR;
            const int status = rsDataObjClose(&_comm, &close_inp);
            if (status < 0) {
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}]: rsDataObjClose of [{}] error, status = [{}]",
                    __FUNCTION__, __LINE__, fd, status));
            }

            if ( bytes_written < 0 ) {
                return bytes_written;
            }

            if (status < 0) {
                return status;
            }
        }
        else {
            auto [final_replica, final_replica_lm] = ir::duplicate_replica(*l1desc.dataObjInfo);

            struct l1desc l1desc_cache = irods::duplicate_l1_descriptor(l1desc);
            const irods::at_scope_exit free_fd{[&l1desc_cache] { freeL1desc_struct(l1desc_cache); }};
            constexpr auto preserve_rst = true;

            // close the replica, free L1 descriptor
            if (const int ec = irods::close_replica_without_catalog_update(_comm, fd, preserve_rst); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - error closing replica; ec:[{}]",
                    __FUNCTION__, __LINE__, ec));

                return ec;
            }

            if (bytes_written < 0) {
                // finalize object for failure case
                if (const auto ec = finalize_on_failure(_comm, *final_replica.get(), l1desc_cache); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed while finalizing object [{}]; ec:[{}]",
                        __FUNCTION__, __LINE__, _inp.objPath, ec));
                }

                if (rst::contains(final_replica.data_id())) {
                    rst::erase(final_replica.data_id());
                }

                return bytes_written;
            }

            // finalize object for successful transfer
            try {
                irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - finalizing replica", __FUNCTION__, __LINE__));

                if (const auto ec = finalize_replica(_comm, *final_replica.get(), l1desc_cache); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - error finalizing replica; ec:[{}]",
                        __FUNCTION__, __LINE__, ec));

                    status = ec;
                }
            }
            catch (const irods::exception& e) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - error finalizing replica; [{}]",
                    __FUNCTION__, __LINE__, e.client_display_what()));

                status = e.code();
            }

            if (rst::contains(final_replica.data_id())) {
                rst::erase(final_replica.data_id());
            }

            if (l1desc_cache.purgeCacheFlag) {
                irods::purge_cache(_comm, *final_replica.get());
            }

            if (status < 0) {
                return status;
            }

            apply_static_peps(_comm, l1desc_cache, status);
        }

        if (getValByKey(&_inp.condInput, ALL_KW)) {
            /* update the rest of copies */
            transferStat_t *transStat{};
            status = rsDataObjRepl(&_comm, &_inp, &transStat);
            if (transStat) {
                free(transStat);
            }
        }

        if (status >= 0) {
            status = applyRuleForPostProcForWrite(&_comm, &_bbuf, _inp.objPath);
            if (status >= 0) {
                status = 0;
            }
        }

        return status;
    } // single_buffer_put

    int parallel_transfer_put(RsComm *rsComm, DataObjInp *dataObjInp, portalOprOut **portalOprOut)
    {
        // Parallel transfer
        dataObjInp->openFlags = O_CREAT | O_RDWR | O_TRUNC;
        const int l1descInx = rsDataObjOpen(rsComm, dataObjInp);
        if (l1descInx < 0) {
            return l1descInx;
        }

        auto& l1desc = L1desc[l1descInx];
        l1desc.oprType = PUT_OPR;
        l1desc.dataSize = dataObjInp->dataSize;
        l1desc.dataObjInp->dataSize = dataObjInp->dataSize;

        if (getStructFileType(l1desc.dataObjInfo->specColl) >= 0) {
            *portalOprOut = static_cast<portalOprOut_t*>(malloc(sizeof(portalOprOut_t)));
            bzero(*portalOprOut, sizeof(portalOprOut_t));
            (*portalOprOut)->l1descInx = l1descInx;
            return l1descInx;
        }

        int ec = preProcParaPut(rsComm, l1descInx, portalOprOut);

        if (ec < 0) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = l1descInx;
            l1desc.oprStatus = ec;
            rsDataObjClose(rsComm, &dataObjCloseInp);
            return ec;
        }

        const bool all_replicas = getValByKey(&dataObjInp->condInput, ALL_KW);
        dataObjInp_t replDataObjInp{};

        if (all_replicas) {
            // Save the dataObjInp. It gets freed in sendAndRecvBranchMsg.
            rstrcpy(replDataObjInp.objPath, dataObjInp->objPath, MAX_NAME_LEN);
            addKeyVal(&replDataObjInp.condInput, UPDATE_REPL_KW, "");
            addKeyVal(&replDataObjInp.condInput, ALL_KW, "");
        }

        // Return portalOprOut to the client and wait for the rcOprComplete
        // call. That is when the parallel I/O is done. rcOprComplete invokes
        // rcDataObjClose.
        ec = sendAndRecvBranchMsg(rsComm, rsComm->apiInx, ec, static_cast<void*>(*portalOprOut), nullptr);

        if (ec < 0) {
            if (FD_INUSE == l1desc.inuseFlag) {
                openedDataObjInp_t dataObjCloseInp{};
                dataObjCloseInp.l1descInx = l1descInx;
                l1desc.oprStatus = ec;
                rsDataObjClose(rsComm, &dataObjCloseInp);
            }

            if (all_replicas) {
                clearKeyVal(&replDataObjInp.condInput);
            }
        }
        else if (all_replicas) {
            transferStat_t *transStat = nullptr;
            ec = rsDataObjRepl(rsComm, &replDataObjInp, &transStat);
            free(transStat);
            clearKeyVal(&replDataObjInp.condInput);

            if (ec < 0) {
                irods::log(ERROR(ec, "rsDataObjRepl failed"));
                return ec;
            }
        }

        // Already sent the client the status.
        return SYS_NO_HANDLER_REPLY_MSG;
    } // parallel_transfer_put

    void throw_if_force_put_to_new_resource(
        const DataObjInp& _inp,
        const irods::file_object_ptr file_obj)
    {
        const auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        if (file_obj->replicas().empty() || !cond_input.contains(FORCE_FLAG_KW) || !cond_input.contains(DEST_RESC_NAME_KW)) {
            return;
        }

        const auto destination_resource = cond_input.at(DEST_RESC_NAME_KW).value();

        const auto hier_match{
            [&destination_resource, &replicas = file_obj->replicas()]()
            {
                return std::any_of(replicas.cbegin(), replicas.cend(),
                [&destination_resource](const auto& r) {
                    return irods::hierarchy_parser{r.resc_hier()}.first_resc() == destination_resource;
                });
            }()
        };

        if (!hier_match) {
            THROW(HIERARCHY_ERROR, fmt::format(
                "cannot force put [{}] to a different resource [{}]",
                _inp.objPath, destination_resource));
        }
    } // throw_if_force_put_to_new_resource

    int rsDataObjPut_impl(
        rsComm_t *rsComm,
        dataObjInp_t *dataObjInp,
        bytesBuf_t *dataObjInpBBuf,
        portalOprOut_t **portalOprOut)
    {
        try {
            if (irods::is_force_flag_required(*rsComm, *dataObjInp)) {
                return OVERWRITE_WITHOUT_FORCE_FLAG;
            }
        }
        catch (const irods::experimental::filesystem::filesystem_error& e) {
            irods::experimental::log::api::error(e.what());
            return e.code().value();
        }

        rodsServerHost_t *rodsServerHost{};
        specCollCache_t *specCollCache{};

        resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                           &dataObjInp->condInput );
        int remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                           REMOTE_CREATE );

        if (const char* acl_string = getValByKey( &dataObjInp->condInput, ACL_INCLUDED_KW)) {
            try {
                irods::deserialize_acl(acl_string);
            }
            catch (const irods::exception& e) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
                return e.code();
            }
        }
        if (const char* metadata_string = getValByKey(&dataObjInp->condInput, METADATA_INCLUDED_KW)) {
            try {
                irods::deserialize_metadata( metadata_string );
            }
            catch (const irods::exception& e) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
                return e.code();
            }
        }

        if (remoteFlag < 0) {
            return remoteFlag;
        }
        else if (LOCAL_HOST != remoteFlag) {
            int status = _rcDataObjPut( rodsServerHost->conn, dataObjInp, dataObjInpBBuf, portalOprOut );
            if (status < 0 ||
                getValByKey(&dataObjInp->condInput, DATA_INCLUDED_KW)) {
                return status;
            }

            /* have to allocate a local l1descInx to keep track of things
             * since the file is in remote zone. It sets remoteL1descInx,
             * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that
             * rsComplete knows what to do */
            int l1descInx = allocAndSetL1descForZoneOpr(
                            ( *portalOprOut )->l1descInx, dataObjInp, rodsServerHost, NULL );
            if ( l1descInx < 0 ) {
                return l1descInx;
            }
            ( *portalOprOut )->l1descInx = l1descInx;
            return status;
        }

        try {
            dataObjInfo_t* dataObjInfoHead{};
            irods::file_object_ptr file_obj(new irods::file_object());
            file_obj->logical_path(dataObjInp->objPath);
            irods::error fac_err = irods::file_object_factory(rsComm, dataObjInp, file_obj, &dataObjInfoHead);

            throw_if_force_put_to_new_resource(*dataObjInp, file_obj);

            std::string hier{};
            auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);
            if (!cond_input.contains(RESC_HIER_STR_KW)) {
                auto fobj_tuple = std::make_tuple(file_obj, fac_err);

                std::tie(file_obj, hier) = irods::resolve_resource_hierarchy(
                    rsComm, irods::CREATE_OPERATION, *dataObjInp, fobj_tuple);

                cond_input[RESC_HIER_STR_KW] = hier;
            }
            else {
                if (!fac_err.ok() && CAT_NO_ROWS_FOUND != fac_err.code()) {
                    irods::log(fac_err);
                }
                hier = cond_input.at(RESC_HIER_STR_KW).value().data();
            }

            if (irods::hierarchy_has_replica(file_obj, cond_input.at(RESC_HIER_STR_KW).value()) &&
                !cond_input.contains(FORCE_FLAG_KW)) {
                return OVERWRITE_WITHOUT_FORCE_FLAG;
            }
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
            return e.code();
        }

        int status2 = applyRuleForPostProcForWrite( rsComm, dataObjInpBBuf, dataObjInp->objPath );
        if ( status2 < 0 ) {
            return ( status2 );
        }

        dataObjInp->openFlags = O_RDWR;

        try {
            if (getValByKey(&dataObjInp->condInput, DATA_INCLUDED_KW)) {
                return single_buffer_put(*rsComm, *dataObjInp, *dataObjInpBBuf);
            }

            return parallel_transfer_put( rsComm, dataObjInp, portalOprOut );
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
            //addRErrorMsg(&rsComm->rError, e.code(), e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
            return SYS_UNKNOWN_ERROR;
        }
    } // rsDataObjPut_impl
} // anonymous namespace

int preProcParaPut(
    rsComm_t *rsComm,
    int l1descInx,
    portalOprOut_t **portalOprOut)
{
    int status;
    dataOprInp_t dataOprInp;

    initDataOprInp( &dataOprInp, l1descInx, PUT_OPR );
    /* add RESC_HIER_STR_KW for getNumThreads */
    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        addKeyVal( &dataOprInp.condInput, RESC_HIER_STR_KW,
                   L1desc[l1descInx].dataObjInfo->rescHier );
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        status =  remoteDataPut( rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost );
    }
    else {
        status =  rsDataPut( rsComm, &dataOprInp, portalOprOut );
    }

    if ( status >= 0 ) {
        ( *portalOprOut )->l1descInx = l1descInx;
        L1desc[l1descInx].bytesWritten = dataOprInp.dataSize;
    }
    clearKeyVal( &dataOprInp.condInput );
    return status;
} // preProcParaPut

int rsDataObjPut(rsComm_t* rsComm,
                 dataObjInp_t* dataObjInp,
                 bytesBuf_t* dataObjInpBBuf,
                 portalOprOut_t** portalOprOut)
{
    namespace ix = irods::experimental;
    namespace fs = ix::filesystem;

    const auto ec = rsDataObjPut_impl(rsComm, dataObjInp, dataObjInpBBuf, portalOprOut);

    // Do not update the collection mtime if the logical path refers to a remote
    // zone. There is a known limitation in the iRODS protocol which does not allow
    // for further communication over the server port with the remote server in
    // parallel transfer situations once initiated. Further, if the remote zone
    // cares about updating the mtime, the remote API endpoint called should have
    // updated the collection mtime already, so it is unnecessary.
    if (const auto zone_hint = fs::zone_name(fs::path{dataObjInp->objPath});
        zone_hint && !isLocalZone(zone_hint->data())) {
        return ec;
    }

    const auto parent_path = fs::path{dataObjInp->objPath}.parent_path();

    // Update the parent collection's mtime.
    if (ec == 0 && fs::server::is_collection_registered(*rsComm, parent_path)) {
        using std::chrono::system_clock;
        using std::chrono::time_point_cast;

        const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

        try {
            ix::scoped_privileged_client spc{*rsComm};
            fs::server::last_write_time(*rsComm, parent_path, mtime);
        }
        catch (const fs::filesystem_error& e) {
            ix::log::api::error(e.what());
            return e.code().value();
        }
    }

    return ec;
} // rsDataObjPut

