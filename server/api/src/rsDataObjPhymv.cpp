#include "dataObjCreate.h"
#include "dataObjOpr.hpp"
#include "dataObjPhymv.h"
#include "dataObjRepl.h"
#include "fileClose.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "icatDefines.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rodsLog.h"
#include "rsDataObjClose.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjPhymv.hpp"
#include "rsDataObjPhymv.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjTrim.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsFileClose.hpp"
#include "rsFileOpen.hpp"
#include "rsGetRescQuota.hpp"
#include "rs_replica_close.hpp"
#include "specColl.hpp"

#include "finalize_utilities.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "json_serialization.hpp"
#include "key_value_proxy.hpp"
#include "replica_access_table.hpp"
#include "replication_utilities.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "logical_locking.hpp"

namespace
{
    namespace ill = irods::logical_locking;
    namespace ir = irods::experimental::replica;
    namespace id = irods::experimental::data_object;
    namespace fs = irods::experimental::filesystem;
    namespace rat = irods::experimental::replica_access_table;
    namespace rst = irods::replica_state_table;

    auto apply_static_peps(RsComm& _comm, l1desc& _l1desc, const int _operation_status) -> void
    {
        if (_l1desc.openType == CREATE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForCreate");
        }
        else if (_l1desc.openType == OPEN_FOR_WRITE_TYPE) {
            irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForOpen");
        }

        // Calling acPostProcForPut after phymv is legacy behavior.
        // Keep this here even though it is wrong.
        irods::apply_static_post_pep(_comm, _l1desc, _operation_status, "acPostProcForPut");
    } // apply_static_peps

    void throw_if_no_write_permissions_on_source_replica(
        RsComm& _comm,
        const DataObjInp& _source_inp,
        const irods::physical_object& _replica)
    {
        std::string location;
        if (const irods::error ret = irods::get_loc_for_hier_string(_replica.resc_hier(), location); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }

        // test the source hier to determine if we have write access to the data
        // stored.  if not then we cannot unlink that replica and should throw an
        // error.
        fileOpenInp_t open_inp{};
        open_inp.mode = getDefFileMode();
        open_inp.flags = O_WRONLY;
        rstrcpy(open_inp.resc_name_, _replica.resc_name().data(), MAX_NAME_LEN);
        rstrcpy(open_inp.resc_hier_, _replica.resc_hier().data(), MAX_NAME_LEN);
        rstrcpy(open_inp.objPath, _source_inp.objPath, MAX_NAME_LEN);
        rstrcpy(open_inp.addr.hostAddr, location.c_str(), NAME_LEN);
        rstrcpy(open_inp.fileName, _replica.path().data(), MAX_NAME_LEN);
        if (const auto cond_input = irods::experimental::make_key_value_proxy(_source_inp.condInput);
            cond_input.contains(IN_PDMO_KW)) {
            rstrcpy(open_inp.in_pdmo, cond_input.at(IN_PDMO_KW).value().data(), MAX_NAME_LEN);
        }

        // kv passthru
        copyKeyVal(&_source_inp.condInput, &open_inp.condInput);

        const int l3_index = rsFileOpen(&_comm, &open_inp);
        clearKeyVal(&open_inp.condInput);
        if(l3_index < 0) {
            THROW(l3_index, fmt::format("unable to open {} for unlink", _source_inp.objPath));
        }

        FileCloseInp close_inp{};
        close_inp.fileInx = l3_index;
        if (const int ec = rsFileClose(&_comm, &close_inp); ec < 0) {
            THROW(ec, fmt::format("failed to close {}", _source_inp.objPath));
        }
    } // throw_if_no_write_permissions_on_source_replica

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

        if (_source_replica.checksum().length() > 0 && STALE_REPLICA != _source_replica.replica_status()) {
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

    DataObjInp init_source_replica_input(RsComm& _comm, const DataObjInp& _inp)
    {
        DataObjInp source_data_obj_inp = _inp;

        auto cond_input = irods::experimental::make_key_value_proxy(source_data_obj_inp.condInput);

        replKeyVal(&_inp.condInput, &source_data_obj_inp.condInput);

        // Remove existing keywords used for destination resource
        cond_input.erase(DEST_RESC_NAME_KW);
        cond_input.erase(DEST_RESC_HIER_STR_KW);

        // Need to be able to unlink the source replica after moving
        cond_input[DATA_ACCESS_KW] = ACCESS_DELETE_OBJECT;

        if (cond_input.contains(RESC_HIER_STR_KW)) {
            return source_data_obj_inp;
        }

        if (cond_input.contains(REPL_NUM_KW)) {
            return source_data_obj_inp;
        }

        if (!cond_input.contains(RESC_NAME_KW)) {
            THROW(USER__NULL_INPUT_ERR, "Source hierarchy or leaf resource or replica number required - none provided.");
        }

        const auto target = cond_input.at(RESC_NAME_KW).value();
        const irods::hierarchy_parser parser{target.data()};
        const auto full_hierarchy = resc_mgr.get_hier_to_root_for_resc(parser.last_resc());
        if (resc_mgr.is_coordinating_resource(parser.last_resc()) ||
            (parser.num_levels() > 1 && parser.first_resc() != irods::hierarchy_parser{full_hierarchy}.first_resc())) {
            THROW(USER_INVALID_RESC_INPUT, fmt::format(
                "Source resource [{}] must be a leaf or a full hierarchy.",
                target));
        }

        cond_input[RESC_HIER_STR_KW] = resc_mgr.get_hier_to_root_for_resc(parser.last_resc());

        return source_data_obj_inp;
    } // init_source_replica_input

     auto get_source_replica_info(
        RsComm& _comm,
        DataObjInp& _inp,
        const irods::replication::log_errors _log_errors) -> std::tuple<irods::file_object_ptr, irods::physical_object&>
    {
        irods::file_object_ptr obj{new irods::file_object()};
        if (irods::error err = irods::file_object_factory(&_comm, &_inp, obj); !err.ok()) {
            THROW(err.code(), err.result());
        }

        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);
        if (cond_input.contains(RESC_HIER_STR_KW)) {
            auto maybe_source_replica = obj->get_replica(cond_input.at(RESC_HIER_STR_KW).value());
            if (!maybe_source_replica) {
                const std::string msg = "error finding source replica.";

                if (irods::replication::log_errors::yes == _log_errors) {
                    addRErrorMsg(&_comm.rError, SYS_REPLICA_DOES_NOT_EXIST, msg.data());
                }

                THROW(SYS_REPLICA_DOES_NOT_EXIST,
                    fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));
            }

            auto& source_replica = *maybe_source_replica;

            return {obj, std::ref(source_replica)};
        }
        else if (cond_input.contains(REPL_NUM_KW)) {
            auto maybe_source_replica = obj->get_replica(std::stoi(cond_input.at(REPL_NUM_KW).value().data()));
            if (!maybe_source_replica) {
                const std::string msg = "error finding source replica.";

                if (irods::replication::log_errors::yes == _log_errors) {
                    addRErrorMsg(&_comm.rError, SYS_REPLICA_DOES_NOT_EXIST, msg.data());
                }

                THROW(SYS_REPLICA_DOES_NOT_EXIST,
                    fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));
            }

            auto& source_replica = *maybe_source_replica;

            return {obj, std::ref(source_replica)};
        }

        THROW(SYS_INTERNAL_NULL_INPUT_ERR,
            "no resource hierarchy or replica number provided - "
            "source replica information cannot be retrieved.");
    } // get_source_replica_info

    int open_source_replica(RsComm& _comm, DataObjInp& source_data_obj_inp)
    {
        source_data_obj_inp.oprType = PHYMV_SRC;
        source_data_obj_inp.openFlags = O_RDONLY;

        return rsDataObjOpen(&_comm, &source_data_obj_inp);
    } // open_source_replica

    DataObjInp init_destination_replica_input(RsComm& _comm, const DataObjInp& _inp)
    {
        DataObjInp destination_data_obj_inp = _inp;
        replKeyVal(&_inp.condInput, &destination_data_obj_inp.condInput);
        auto cond_input = irods::experimental::make_key_value_proxy(destination_data_obj_inp.condInput);

        // Remove existing keywords used for source resource
        cond_input.erase(RESC_NAME_KW);
        cond_input.erase(RESC_HIER_STR_KW);
        cond_input.erase(REPL_NUM_KW);

        if (cond_input.contains(DEST_RESC_HIER_STR_KW)) {
            cond_input[RESC_HIER_STR_KW] = cond_input.at(DEST_RESC_HIER_STR_KW);
            return destination_data_obj_inp;
        }

        if (!cond_input.contains(DEST_RESC_NAME_KW)) {
            THROW(USER__NULL_INPUT_ERR,
                "Destination hierarchy or leaf resource required - none provided.");
        }

        const auto target = cond_input.at(DEST_RESC_NAME_KW).value();
        const irods::hierarchy_parser parser{target.data()};
        const auto full_hierarchy = resc_mgr.get_hier_to_root_for_resc(parser.last_resc());
        if (resc_mgr.is_coordinating_resource(parser.last_resc()) ||
            (parser.num_levels() > 1 && parser.first_resc() != irods::hierarchy_parser{full_hierarchy}.first_resc())) {
            THROW(USER_INVALID_RESC_INPUT, fmt::format(
                "Destination resource [{}] must be a leaf or a full hierarchy.",
                target));
        }

        cond_input[DEST_RESC_HIER_STR_KW] = full_hierarchy;
        cond_input[RESC_HIER_STR_KW] = full_hierarchy;

        return destination_data_obj_inp;
    } // init_destination_replica_input

    int open_destination_replica(RsComm& _comm, DataObjInp& _inp, const int _source_fd)
    {
        auto kvp = irods::experimental::make_key_value_proxy(_inp.condInput);
        kvp[REG_REPL_KW] = "";
        kvp[DATA_ID_KW] = std::to_string(L1desc[_source_fd].dataObjInfo->dataId);
        kvp.erase(PURGE_CACHE_KW);

        _inp.oprType = PHYMV_DEST;
        _inp.openFlags = O_CREAT | O_WRONLY | O_TRUNC;

        return rsDataObjOpen(&_comm, &_inp);
    } // open_destination_replica

    auto finalize_destination_replica_on_failure(
        RsComm& _comm,
        l1desc& _l1desc,
        ir::replica_proxy_t& _destination_replica) -> int
    {
        const auto admin_op = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput).contains(ADMIN_KW);

        const rodsLong_t vault_size = getSizeInVault(&_comm, _destination_replica.get());

        if (vault_size < 0) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - getSizeInVault failed [{}]", __FUNCTION__, __LINE__, vault_size));

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
                "[{}:{}] - failed to finalize data object "
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

    auto finalize_destination_replica(
        RsComm& _comm,
        l1desc& _source_l1desc,
        l1desc& _destination_l1desc,
        ir::replica_proxy_t& _source_replica,
        ir::replica_proxy_t& _destination_replica) -> int
    {
        auto [source_replica, source_replica_lm] = ir::make_replica_proxy(
            _source_replica.logical_path(),
            rst::at(_source_replica.data_id(), _source_replica.replica_number(), rst::state_type::after));

        auto cond_input = irods::experimental::make_key_value_proxy(_destination_l1desc.dataObjInp->condInput);

        const auto admin_op = cond_input.contains(ADMIN_KW);

        try {
            constexpr bool verify_size = true;
            const auto size_in_vault = irods::get_size_in_vault(_comm, *_destination_replica.get(), verify_size, _destination_l1desc.dataSize);
            if (size_in_vault < 0) {
                THROW(size_in_vault, fmt::format(
                    "[{}:{}] - failed to get size in vault "
                    "[ec=[{}], path=[{}], repl num=[{}]]",
                    __FUNCTION__, __LINE__, size_in_vault,
                    _destination_replica.logical_path(),
                    _destination_replica.replica_number()));
            }

            _destination_replica.size(size_in_vault);

            if (!_destination_replica.checksum().empty()) {
                const auto checksum = calculate_checksum(_comm, _destination_l1desc, _source_replica, _destination_replica);
                if (!checksum.empty()) {
                    cond_input[CHKSUM_KW] = checksum;
                }
            }
        }
        catch (const irods::exception& e) {
            rst::update(_destination_replica.data_id(), _destination_replica);

            if (const int unlock_ec = ill::unlock_and_publish(_comm, {_destination_replica, admin_op, 0}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
            }

            throw;
        }

        const auto source_replica_original_status = ill::get_original_replica_status(_source_replica.data_id(), _source_replica.replica_number());
        const auto source_replica_number = _source_replica.replica_number();

        const int trim_ec = dataObjUnlinkS(&_comm, _source_l1desc.dataObjInp, _source_replica.get());

        if (trim_ec < 0) {
            irods::log(LOG_WARNING, fmt::format(
                "[{}:{}] - unlinking source replica failed "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, trim_ec,
                source_replica.logical_path(), source_replica.hierarchy()));

            // If unlinking the source replica failed, move it to the next available replica number
            // and mark it as a STALE_REPLICA when the object is unlocked.
            const auto query_string = fmt::format(
                "select max(DATA_REPL_NUM) where DATA_ID = '{}'", source_replica.data_id()
            );

            source_replica.replica_number(std::stoi(irods::query{&_comm, query_string}.front().at(0)) + 1);
            source_replica.mtime(SET_TIME_TO_NOW_KW);
            source_replica.status(nlohmann::json{{"original_status", std::to_string(STALE_REPLICA)}}.dump());

            // The source replica information must be published to the catalog before unlocking because
            // the destination replica is going to take on the replica number of the source replica.
            // If the source replica has not been updated in the catalog at the point of publishing,
            // a duplicate key constraint violation will occur. The last step of the unlinking
            // process is unregistering from the catalog, so it is unlikely that the replica was
            // successfully unregistered in the catalog. However, if the replica has been unregistered
            // in the catalog, the replica_state_table (and subsequently the logical locking system)
            // no longer reflects the state of the catalog and may lead to undesirable results when
            // updating the catalog on unlock.
            rst::update(source_replica.data_id(), source_replica);

            if (const auto [ret, ec] = rst::publish::to_catalog(_comm, {source_replica.data_id(), admin_op}); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to update catalog for source object unlink failure "
                    "[error code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, ec,
                    source_replica.logical_path(), source_replica.hierarchy()));

                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, _source_replica.data_id(), _source_replica.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed to unlock data object [error_code={}]",
                        __FUNCTION__, __LINE__, unlock_ec));
                }

                return ec;
            }
        }
        else {
            // The replica was unlinked and unregistered successfully, so remove the replica entry
            // from the replica_state_table.
            rst::erase(source_replica.data_id(), source_replica_number);
        }

        const auto original_destination_replica_number = _destination_replica.replica_number();

        _destination_replica.replica_status(source_replica_original_status);
        _destination_replica.replica_number(source_replica_number);
        _destination_replica.ctime(_source_replica.ctime());
        _destination_replica.mtime(_source_replica.mtime());

        rst::update(_destination_replica.data_id(), _destination_replica);

        const auto unlock_ec = ill::unlock(
            _destination_replica.data_id(),
            original_destination_replica_number,
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

        const auto ctx = rst::publish::context{_destination_replica.data_id(),
                                               original_destination_replica_number,
                                               irods::to_json(cond_input.get()),
                                               cond_input.contains(ADMIN_KW),
                                               _destination_replica.size()};

        const auto [ret, ec] = rst::publish::to_catalog(_comm, ctx);

        if (CREATE_TYPE == _destination_l1desc.openType) {
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
        }

        if (rst::contains(_destination_replica.data_id())) {
            rst::erase(_destination_replica.data_id());
        }

        return trim_ec < 0 ? trim_ec : ec;
    } // finalize_destination_replica

    int replicate_data(RsComm& _comm, DataObjInp& source_inp, DataObjInp& destination_inp, TransferStat** _stat)
    {
        // Open source replica
        int source_l1descInx = open_source_replica(_comm, source_inp);
        if (source_l1descInx < 0) {
            THROW(source_l1descInx, "Failed opening source replica");
        }

        // Open destination replica
        int destination_l1descInx = open_destination_replica(_comm, destination_inp, source_l1descInx);
        if (destination_l1descInx < 0) {
            constexpr auto preserve_rst = false;
            if (const int ec = irods::close_replica_without_catalog_update(_comm, source_l1descInx, preserve_rst); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to close source replica:[{}]",
                    __FUNCTION__, __LINE__, ec));
            }
            return destination_l1descInx;
        }
        L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;
        L1desc[destination_l1descInx].dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;
        L1desc[destination_l1descInx].dataObjInp->dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;

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
                "[{}:{}] - dataObjCopy failed "
                "[error_code=[{}], path=[{}], source=[{}], destination=[{}]]",
                __FUNCTION__, __LINE__, status,
                destination_inp.objPath,
                L1desc[source_l1descInx].dataObjInfo->rescHier,
                L1desc[destination_l1descInx].dataObjInfo->rescHier));

            L1desc[source_l1descInx].oprStatus = status;

            L1desc[destination_l1descInx].bytesWritten = status;
            L1desc[destination_l1descInx].oprStatus = status;
        }
        else {
            L1desc[destination_l1descInx].bytesWritten = L1desc[destination_l1descInx].dataObjInfo->dataSize;
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

        auto [source_replica, source_replica_lm] = ir::duplicate_replica(*L1desc[source_l1descInx].dataObjInfo);
        auto [destination_replica, destination_replica_lm] = ir::duplicate_replica(*L1desc[destination_l1descInx].dataObjInfo);
        constexpr auto preserve_rst = true;

        // Close source replica
        if (const int ec = irods::close_replica_without_catalog_update(_comm, source_l1descInx, preserve_rst); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - closing source replica [{}] failed with [{}]",
                __FUNCTION__, __LINE__, source_inp.objPath, ec));

            if (status >= 0) {
                status = ec;
            }
        }

        // Close destination replica
        if (const int ec = irods::close_replica_without_catalog_update(_comm, destination_l1descInx, preserve_rst); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - closing destination replica [{}] failed with [{}]",
                __FUNCTION__, __LINE__, destination_inp.objPath, ec));

            if (status >= 0) {
                status = ec;
            }

            rat::erase_pid(token, getpid());
        }

        if (destination_fd.bytesWritten < 0) {
            if (const auto ec = finalize_destination_replica_on_failure(_comm, destination_fd, destination_replica); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}] - failed while finalizing object [{}]; ec:[{}]",
                    __FUNCTION__, destination_replica.logical_path(), ec));
            }
            return destination_fd.bytesWritten;
        }

        // Finalizing the destination replica is wrapped up in finalizing the source replica.
        // If something goes wrong with finalizing the destination replica, the source replica
        // will not be unlinked and if something goes wrong with unlinking the source replica
        // this will affect how the data object is finalized.
        try {
            if (const int ec = finalize_destination_replica(_comm, source_fd, destination_fd, source_replica, destination_replica); ec < 0) {
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

        return status;
    } // replicate_data

    int move_replica(RsComm& _comm, DataObjInp& _inp, TransferStat** _stat)
    {
        const auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);
        const auto log_errors = cond_input.contains(RECURSIVE_OPR__KW) ? irods::replication::log_errors::no : irods::replication::log_errors::yes;

        try {
            // get information about source replica
            auto source_inp = init_source_replica_input(_comm, _inp);
            const irods::at_scope_exit free_source_cond_input{[&source_inp] { clearKeyVal(&source_inp.condInput); }};
            auto [source_obj, source_replica] = get_source_replica_info(_comm, source_inp, log_errors);

            // Need to make sure the physical data of the source replica can be unlinked before
            // beginning the phymv operation as it will be unlinked after the data movement has
            // taken place. Failing to do so may result in unintentionally unregistered data in
            // the resource vault.
            try {
                throw_if_no_write_permissions_on_source_replica(_comm, source_inp, source_replica);
            }
            catch (const irods::exception& e) {
                irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));

                if (irods::replication::log_errors::yes == log_errors) {
                    addRErrorMsg(&_comm.rError, e.code(), e.client_display_what());
                }

                return e.code();
            }

            // get information about destination replica
            auto destination_inp = init_destination_replica_input(_comm, _inp);
            const irods::at_scope_exit free_destination_cond_input{[&destination_inp] { clearKeyVal(&destination_inp.condInput); }};
            irods::file_object_ptr destination_obj{new irods::file_object()};
            if (irods::error err = irods::file_object_factory(&_comm, &destination_inp, destination_obj); !err.ok()) {
                irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, err.result()));

                if (irods::replication::log_errors::yes == log_errors) {
                    addRErrorMsg(&_comm.rError, err.code(), err.result().data());
                }

                return err.code();
            }

            // Verify that replication is allowed
            const auto destination_hierarchy = irods::experimental::key_value_proxy{destination_inp.condInput}.at(RESC_HIER_STR_KW).value();
            auto maybe_destination_replica = destination_obj->get_replica(destination_hierarchy);
            if (maybe_destination_replica) {
                const auto& destination_replica = *maybe_destination_replica;

                if (!irods::replication::is_allowed(_comm, source_replica, destination_replica, log_errors)) {
                    return SYS_NOT_ALLOWED;
                }
            }

            return replicate_data(_comm, source_inp, destination_inp, _stat);
        }
        catch (const irods::exception& e) {
            if (irods::replication::log_errors::no == log_errors) {
                return e.code();
            }

            throw;
        }
    } // move_replica
} // anonymous namespace

int rsDataObjPhymv(RsComm *rsComm, DataObjInp *dataObjInp, TransferStat **transStat)
{
    if (!rsComm || !dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);

    // -S and -n are not compatible
    if (cond_input.contains(RESC_NAME_KW) && cond_input.contains(REPL_NUM_KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    SpecCollCache *specCollCache{};
    resolveLinkedPath(rsComm, dataObjInp->objPath, &specCollCache, &dataObjInp->condInput);

    rodsServerHost *rodsServerHost{};
    const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (REMOTE_HOST == remoteFlag) {
        return _rcDataObjPhymv(rodsServerHost->conn, dataObjInp, transStat);
    }

    try {
        const int status = move_replica(*rsComm, *dataObjInp, transStat);
        if (status < 0) {
            rodsLog(LOG_NOTICE, "%s - Failed to physically move replica. status:[%d]",
                __FUNCTION__, status);
        }
        return status;
    }
    catch (const irods::exception& e) {
        irods::log(e);
        addRErrorMsg(&rsComm->rError, e.code(), e.client_display_what());
        return e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}] - [{}]", __FUNCTION__, e.what()));
        return SYS_INTERNAL_ERR;
    }
    catch (...) {
        irods::log(LOG_ERROR, fmt::format("[{}] - unknown error occurred", __FUNCTION__));
        return SYS_UNKNOWN_ERROR;
    }
} // rsDataObjPhymv

