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
#include "replica_proxy.hpp"

#include "replica_state_table.hpp"

#include <chrono>

namespace
{
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
        if (!std::string_view{_info.chksum}.empty()) {
            _l1desc.chksumFlag = REG_CHKSUM;
        }

        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        auto destination_replica = irods::experimental::replica::make_replica_proxy(_info);

        if (!_l1desc.chksumFlag) {
            if (destination_replica.checksum().empty()) {
                return {};
            }
            _l1desc.chksumFlag = VERIFY_CHKSUM;
        }

        if (VERIFY_CHKSUM == _l1desc.chksumFlag) {
            if (!std::string_view{_l1desc.chksum}.empty()) {
                return irods::verify_checksum(_comm, _info, _l1desc.chksum);
            }

            return {};
        }

        return irods::register_new_checksum(_comm, _info, _l1desc.chksum);
    } // calculate_checksum

    auto finalize_on_failure(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> int
    {
        auto replica = irods::experimental::replica::make_replica_proxy(_info);

        replica.replica_status(STALE_REPLICA);
        replica.mtime(std::to_string((int)time(nullptr)));

        const rodsLong_t vault_size = getSizeInVault(&_comm, replica.get());
        if (vault_size < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "{} - getSizeInVault failed [{}]",
                __FUNCTION__, vault_size));
            return vault_size;
        }

        replica.size(vault_size);

        auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);
        auto [reg_param, lm] = irods::experimental::make_key_value_proxy();
        reg_param[DATA_SIZE_KW] = std::to_string(replica.size());
        reg_param[IN_PDMO_KW] = replica.hierarchy();
        reg_param[REPL_STATUS_KW] = std::to_string(STALE_REPLICA);
        reg_param[STALE_ALL_INTERMEDIATE_REPLICAS_KW] = "";
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

        return 0;
    } // finalize_on_failure

    auto finalize_replica(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> int
    {
        auto replica = irods::experimental::replica::make_replica_proxy(_info);

        auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);
        try {
            const bool verify_size = !cond_input.contains(NO_CHK_COPY_LEN_KW);
            const auto size_in_vault = irods::get_size_in_vault(_comm, _info, verify_size, _l1desc.dataSize);
            replica.size(size_in_vault);

            if (verify_size || !replica.checksum().empty()) {
                const auto checksum = calculate_checksum(_comm, _l1desc, _info);
                if (!checksum.empty()) {
                    cond_input[CHKSUM_KW] = checksum;
                }
            }
        }
        catch (const irods::exception& e) {
            if (const int ec = irods::stale_replica(_comm, _l1desc, _info); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "{} - rsModDataObjMeta failed [{}]",
                    __FUNCTION__, ec));
            }
            throw;
        }

        try {
            irods::apply_metadata_from_cond_input(_comm, *_l1desc.dataObjInp);
            irods::apply_acl_from_cond_input(_comm, *_l1desc.dataObjInp);
        }
        catch (const irods::exception& e) {
            rsDataObjUnlink(&_comm, _l1desc.dataObjInp);
            throw;
        }

        cond_input[OPEN_TYPE_KW] = std::to_string(_l1desc.openType);

        // TODO: unlock in RST here (restore replica states)

        // Set target replica to the state it should be
        if (OPEN_FOR_WRITE_TYPE == _l1desc.openType) {
            replica.replica_status(GOOD_REPLICA);
            replica.mtime(SET_TIME_TO_NOW_KW);

            // stale other replicas because the truth has moved
            for (auto& rj : rst::at(replica.data_id())) {
                const auto replica_number = std::stoi(std::string{rj.at("after").at("data_repl_num")});

                if (replica.replica_number() != replica_number) {
                    rst::update(replica.data_id(), replica_number,
                        nlohmann::json{{"data_is_dirty", std::to_string(STALE_REPLICA)}});
                }
            }
        }
        else {
            replica.replica_status(GOOD_REPLICA);
        }

        if (cond_input.contains(CHKSUM_KW)) {
            replica.checksum(cond_input.at(CHKSUM_KW).value());
        }

        // Write it out to the catalog
        rst::update(replica.data_id(), replica);

        const auto update = irods::to_json(cond_input.get());

        if (const int ec = rst::publish_to_catalog(_comm, replica.data_id(), replica.replica_number(), update); ec < 0) {
            THROW(ec, fmt::format("failed to publish to catalog:[{}]", ec));
        }

        if (GOOD_REPLICA == replica.replica_status()) {
            /* update quota overrun */
            updatequotaOverrun(replica.hierarchy().data(), replica.size(), ALL_QUOTA);
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
                "[{}] - failed to open data object [{}]; ec:[{}]",
                __FUNCTION__, _inp.objPath, fd));
            return fd;
        }

        auto& l1desc = L1desc[fd];
        auto opened_replica = irods::experimental::replica::make_replica_proxy(*l1desc.dataObjInfo);
        const std::string hier = opened_replica.hierarchy().data();

        const auto [begin_object, begin_object_lm] = irods::experimental::data_object::duplicate_data_object(*l1desc.dataObjInfo);

        OpenedDataObjInp write_inp{};
        write_inp.len = _bbuf.len;
        write_inp.l1descInx = fd;

        BytesBuf write_bbuf{};
        write_bbuf.buf = _bbuf.buf;
        write_bbuf.len = _bbuf.len;

        const int bytes_written = rsDataObjWrite(&_comm, &write_inp, &write_bbuf);
        if (bytes_written < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "{}: rsDataObjWrite for {} failed with {}",
                __FUNCTION__, opened_replica.physical_path(), bytes_written));
            if (const int ec = dataObjUnlinkS(&_comm, l1desc.dataObjInp, opened_replica.get()); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "dataObjUnlinkS failed for [{}] with [{}]",
                    opened_replica.physical_path(), ec));
            }
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
            auto [final_object, final_object_lm] = irods::experimental::data_object::duplicate_data_object(*l1desc.dataObjInfo);

            struct l1desc l1desc_cache = irods::duplicate_l1_descriptor(l1desc);
            const irods::at_scope_exit free_fd{[&l1desc_cache] { freeL1desc_struct(l1desc_cache); }};

            // close the replica, free L1 descriptor
            if (const int ec = irods::close_replica_without_catalog_update(_comm, fd); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - error closing replica; ec:[{}]",
                    __FUNCTION__, __LINE__, ec));

                return ec;
            }

            if (bytes_written < 0) {
                // finalize object for failure case
                if (const auto ec = finalize_on_failure(_comm, *final_object.get(), l1desc_cache); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}] - failed while finalizing object [{}]; ec:[{}]",
                        __FUNCTION__, _inp.objPath, ec));
                }
                // TODO: continue to finalize
                return bytes_written;
            }
            else {
                // finalize object for successful transfer
                try {
                    irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - finalizing replica", __FUNCTION__, __LINE__));

                    if (const auto ec = finalize_replica(_comm, *final_object.get(), l1desc_cache); ec < 0) {
                        irods::log(LOG_ERROR, fmt::format(
                            "[{}:{}] - error finalizing replica; ec:[{}]",
                            __FUNCTION__, __LINE__, ec));

                        status = ec;
                    }
                }
                catch (const irods::exception& e) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - error finalizing replica; [{}]",
                        __FUNCTION__, __LINE__, e.what()));

                    status = e.code();
                }
            }

            if (l1desc_cache.purgeCacheFlag) {
                irods::purge_cache(_comm, *final_object.get());
            }

            if (status < 0) {
                return status;
            }
            else {
                apply_static_peps(_comm, l1desc_cache, status);
            }
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
        dataObjInp->openFlags |= O_CREAT | O_RDWR;
        int l1descInx = rsDataObjOpen(rsComm, dataObjInp);
        if ( l1descInx < 0 ) {
            return l1descInx;
        }

        L1desc[l1descInx].oprType = PUT_OPR;
        L1desc[l1descInx].dataSize = dataObjInp->dataSize;

        if ( getStructFileType( L1desc[l1descInx].dataObjInfo->specColl ) >= 0 ) { // JMC - backport 4682
            *portalOprOut = ( portalOprOut_t * ) malloc( sizeof( portalOprOut_t ) );
            bzero( *portalOprOut,  sizeof( portalOprOut_t ) );
            ( *portalOprOut )->l1descInx = l1descInx;
            return l1descInx;
        }

        int status = preProcParaPut( rsComm, l1descInx, portalOprOut );

        if ( status < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = l1descInx;
            L1desc[l1descInx].oprStatus = status;
            rsDataObjClose( rsComm, &dataObjCloseInp );
            return status;
        }

        int allFlag = 0;
        if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
            allFlag = 1;
        }

        dataObjInp_t replDataObjInp{};
        if ( allFlag == 1 ) {
            /* need to save dataObjInp. get freed in sendAndRecvBranchMsg */
            rstrcpy( replDataObjInp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
            addKeyVal( &replDataObjInp.condInput, UPDATE_REPL_KW, "" );
            addKeyVal( &replDataObjInp.condInput, ALL_KW, "" );
        }
        /* return portalOprOut to the client and wait for the rcOprComplete
         * call. That is when the parallel I/O is done */
        int retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
                ( void * ) * portalOprOut, NULL );

        if ( retval < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = l1descInx;
            L1desc[l1descInx].oprStatus = retval;
            rsDataObjClose( rsComm, &dataObjCloseInp );
            if ( allFlag == 1 ) {
                clearKeyVal( &replDataObjInp.condInput );
            }
        }
        else if (1 == allFlag) {
            transferStat_t *transStat = NULL;
            status = rsDataObjRepl(rsComm, &replDataObjInp, &transStat);
            free(transStat);
            clearKeyVal(&replDataObjInp.condInput);
            if (status < 0) {
                const auto err{ERROR(status, "rsDataObjRepl failed")};
                irods::log(err);
                return err.code();
            }
        }

        /* already send the client the status */
        return SYS_NO_HANDLER_REPLY_MSG;
    } // parallel_transfer_put

    void throw_if_force_put_to_new_resource(
        dataObjInp_t& data_obj_inp,
        irods::file_object_ptr file_obj)
    {
        char* dst_resc_kw   = getValByKey( &data_obj_inp.condInput, DEST_RESC_NAME_KW );
        char* force_flag_kw = getValByKey( &data_obj_inp.condInput, FORCE_FLAG_KW );
        if (file_obj->replicas().empty()  ||
            !dst_resc_kw   ||
            !force_flag_kw ||
            strlen( dst_resc_kw ) == 0) {
            return;
        }

        const auto hier_match{
            [&dst_resc_kw, &replicas = file_obj->replicas()]()
            {
                return std::any_of(replicas.cbegin(), replicas.cend(),
                [&dst_resc_kw](const auto& r) {
                    return irods::hierarchy_parser{r.resc_hier()}.first_resc() == dst_resc_kw;
                });
            }()
        };
        if (!hier_match) {
            THROW(HIERARCHY_ERROR, fmt::format(
                "cannot force put [{}] to a different resource [{}]",
                data_obj_inp.objPath, dst_resc_kw));
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
                rodsLog(LOG_ERROR, "%s", e.what());
                return e.code();
            }
        }
        if (const char* metadata_string = getValByKey(&dataObjInp->condInput, METADATA_INCLUDED_KW)) {
            try {
                irods::deserialize_metadata( metadata_string );
            }
            catch (const irods::exception& e) {
                rodsLog(LOG_ERROR, "%s", e.what());
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
            irods::log(LOG_ERROR, e.what());
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
            irods::log(e);
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

