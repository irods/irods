#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"
#include "client_api_whitelist.hpp"

#include "apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "replica_close.h"

#include "objDesc.hpp"
#include "rsFileClose.hpp"
#include "rsFileStat.hpp"
#include "rsModDataObjMeta.hpp"
#include "finalize_utilities.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_exception.hpp"
#include "replica_access_table.hpp"
#include "rodsLog.h"
#include "irods_query.hpp"
#include "irods_re_serialization.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_api_call.hpp"
#include "json_serialization.hpp"
#include "replica_access_table.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica.hpp"

#include "logical_locking.hpp"

#include "fmt/format.h"
#include "json.hpp"

#include <cstring>
#include <string>
#include <string_view>
#include <tuple>
#include <chrono>
#include <optional>

#include <sys/types.h>
#include <unistd.h>

namespace
{
    // clang-format off
    namespace fs  = irods::experimental::filesystem;
    namespace ill = irods::logical_locking;
    namespace ir  = irods::experimental::replica;
    namespace rst = irods::replica_state_table;

    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*)>;
    // clang-format on

    //
    // Function Prototypes
    //

    auto call_replica_close(irods::api_entry*, rsComm_t*, bytesBuf_t*) -> int;

    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;

    auto parse_json(const bytesBuf_t& _bbuf) -> std::tuple<int, json>;

    auto get_file_descriptor_index(const json& _json) -> std::tuple<int, int>;

    auto rs_replica_close(rsComm_t* _comm, bytesBuf_t* _input) -> int;

    auto close_physical_object(rsComm_t& _comm, int _l3desc_index) -> int;

    auto unlock_and_publish_replica(rsComm_t& _comm,
                                    const ir::replica_proxy_t& _replica,
                                    const l1desc& _l1desc,
                                    const bool _send_notifications,
                                    const int _unlock_statuses = ill::restore_status) -> int;

    auto update_replica_size_and_status(rsComm_t& _comm,
                                        const l1desc_t& _l1desc,
                                        const bool _send_notifications) -> int;

    auto update_replica_size(rsComm_t& _comm,
                             const l1desc_t& _l1desc,
                             const bool _send_notifications) -> int;

    auto update_replica_status(rsComm_t& _comm,
                               const l1desc_t& _l1desc,
                               const repl_status_t _new_status,
                               const bool _send_notifications,
                               const int _unlock_statuses = ill::restore_status) -> int;

    auto update_replica_status_on_error(rsComm_t& _comm, const l1desc_t& _l1desc) -> int;

    auto free_l1_descriptor(int _l1desc_index) -> int;

    //
    // Function Implementations
    //

    auto call_replica_close(irods::api_entry* _api, rsComm_t* _comm, bytesBuf_t* _input) -> int
    {
        return _api->call_handler<bytesBuf_t*>(_comm, _input);
    }

    auto is_input_valid(const bytesBuf_t* _input) -> std::tuple<bool, std::string>
    {
        if (!_input) {
            return {false, "Missing logical path"};
        }

        if (_input->len <= 0) {
            return {false, "Length of buffer must be greater than zero"};
        }

        if (!_input->buf) {
            return {false, "Missing input buffer"};
        }

        return {true, ""};
    }

    auto parse_json(const bytesBuf_t& _bbuf) -> std::tuple<int, json>
    {
        try {
            std::string_view json_string(static_cast<const char*>(_bbuf.buf), _bbuf.len);
            rodsLog(LOG_DEBUG, "Parsing string into JSON ... [string=%s]", json_string.data());
            return {0, json::parse(json_string)};
        }
        catch (const json::parse_error& e) {
            return {SYS_INTERNAL_ERR, {}};
        }
    }

    auto get_file_descriptor_index(const json& _json) -> std::tuple<int, int>
    {
        try {
            return {0, _json.at("fd").get<int>()};
        }
        catch (const json::type_error& e) {
            return {SYS_INTERNAL_ERR, -1};
        }
    }

    auto close_physical_object(rsComm_t& _comm, int _l3desc_index) -> int
    {
        // The L3 descriptor index is set to -1 when the physical data has already been closed so that it can be
        // detected on subsequent calls to replica_close. Return 0 here so that the rest of the close operations
        // (i.e. finalizing) can complete since we do not need to close the data again.
        if (_l3desc_index < 0) {
            return 0;
        }

        fileCloseInp_t input{};
        input.fileInx = _l3desc_index;
        return rsFileClose(&_comm, &input);
    } // close_physical_object

    auto unlock_and_publish_replica(rsComm_t& _comm,
                                    const ir::replica_proxy_t& _replica,
                                    const l1desc& _l1desc,
                                    const bool _send_notifications,
                                    const int _unlock_statuses) -> int
    {
        // Unlock the data object but do not publish to catalog because we may want to trigger file_modified.
        if (const int ec =
                ill::unlock(_replica.data_id(), _replica.replica_number(), _replica.replica_status(), _unlock_statuses);
            ec < 0)
        {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unlock object [{}]:[{}]",
                __FUNCTION__, __LINE__, _replica.logical_path(), ec));

            return ec;
        }

        // Send an empty JSON structure if we do not want to trigger file_modified
        // as this is the way the RST interface determines whether it should trigger.
        const auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);

        const auto admin_op = cond_input.contains(ADMIN_KW);
        const auto ctx = _send_notifications ? rst::publish::context{_replica, *cond_input.get()}
                                             : rst::publish::context{_replica, admin_op};

        // Publish to catalog and trigger file_modified as appropriate. The updates
        // made above will be reflected in the RST and stamped out to the catalog here.
        const auto [ret, ec] = rst::publish::to_catalog(_comm, ctx);
        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to finalize data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, _replica.logical_path(), _replica.hierarchy()));

            if (!ret.at("database_updated")) {
                if (const int unlock_ec = irods::stale_target_replica_and_publish(_comm, _replica.data_id(), _replica.replica_number()); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }
            }
        }

        return ec;
    } // unlock_and_publish_replica

    auto update_replica_size_and_status(rsComm_t& _comm, const l1desc_t& _l1desc, const bool _send_notifications) -> int
    {
        constexpr auto verify_size = false;
        const auto size_on_disk = irods::get_size_in_vault(_comm, *_l1desc.dataObjInfo, verify_size, _l1desc.dataSize);

        if (size_on_disk < 0) {
            rodsLog(LOG_ERROR, "Failed to retrieve the replica's size on disk [error_code=%d].", size_on_disk);
            return size_on_disk;
        }

        auto repl = ir::make_replica_proxy(*_l1desc.dataObjInfo);

        if (CREATE_TYPE == _l1desc.openType) {
            repl.size(size_on_disk);
            repl.replica_status(GOOD_REPLICA);
            repl.mtime(SET_TIME_TO_NOW_KW);
        }
        // If the size of the replica has changed since opening it, then update the size.
        else if (_l1desc.dataObjInfo->dataSize != size_on_disk) {
            repl.size(size_on_disk);
            repl.replica_status(GOOD_REPLICA);
            repl.mtime(SET_TIME_TO_NOW_KW);
            repl.checksum("");
        }
        // If the contents of the replica has changed, then update the last modified timestamp.
        else if (_l1desc.bytesWritten > 0) {
            repl.replica_status(GOOD_REPLICA);
            repl.mtime(SET_TIME_TO_NOW_KW);
            repl.checksum("");
        }
        // If nothing has been written, the status is restored from the replica state table
        // so that the replica is not mistakenly marked as good when it is in fact stale.
        // The mtime is also not updated so as to signal that nothing changed in the data.
        else {
            repl.replica_status(ill::get_original_replica_status(repl.data_id(), repl.replica_number()));
        }

        // Update the RST with the changes made above to the replica.
        rst::update(repl.data_id(), repl);

        return unlock_and_publish_replica(_comm, repl, _l1desc, _send_notifications, STALE_REPLICA);
    } // update_replica_size_and_status

    auto update_replica_size(rsComm_t& _comm, const l1desc_t& _l1desc, const bool _send_notifications) -> int
    {
        constexpr auto verify_size = false;
        const auto size_on_disk = irods::get_size_in_vault(_comm, *_l1desc.dataObjInfo, verify_size, _l1desc.dataSize);

        if (size_on_disk < 0) {
            rodsLog(LOG_ERROR, "Failed to retrieve the replica's size on disk [error_code=%d].", size_on_disk);
            return size_on_disk;
        }

        auto repl = ir::make_replica_proxy(*_l1desc.dataObjInfo);

        repl.checksum("");

        // If the size of the replica has changed since opening it, then update the size.
        if (_l1desc.dataObjInfo->dataSize != size_on_disk) {
            repl.size(size_on_disk);
            repl.mtime(SET_TIME_TO_NOW_KW);
        }
        // If the contents of the replica has changed, then update the last modified timestamp.
        else if (_l1desc.bytesWritten > 0) {
            repl.mtime(SET_TIME_TO_NOW_KW);
        }

        // Update the RST with the changes made above to the replica.
        rst::update(repl.data_id(), repl);

        return unlock_and_publish_replica(_comm, repl, _l1desc, _send_notifications, STALE_REPLICA);
    } // update_replica_size

    auto update_replica_status(rsComm_t& _comm,
                               const l1desc_t& _l1desc,
                               const repl_status_t _new_status,
                               const bool _send_notifications,
                               const int _unlock_statuses) -> int
    {
        auto repl = ir::make_replica_proxy(*_l1desc.dataObjInfo);

        repl.replica_status(_new_status);

        // Set target replica status in RST as logical locking will not set
        // the status of the target replica.
        rst::update(
            repl.data_id(), repl.replica_number(), json{{"data_is_dirty", std::to_string(repl.replica_status())}});

        return unlock_and_publish_replica(_comm, repl, _l1desc, _send_notifications, _unlock_statuses);
    } // update_replica_status

    auto update_replica_status_on_error(rsComm_t& _comm, const l1desc_t& _l1desc) -> int
    {
        return update_replica_status(_comm, _l1desc, STALE_REPLICA, false);
    } // update_replica_status_on_error

    auto free_l1_descriptor(int _l1desc_index) -> int
    {
        if (const auto ec = freeL1desc(_l1desc_index); ec != 0) {
            rodsLog(LOG_ERROR, "Failed to release L1 descriptor [error_code=%d].", ec);
            return ec;
        }

        return 0;
    }

    auto rs_replica_close(rsComm_t* _comm, bytesBuf_t* _input) -> int
    {
        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            rodsLog(LOG_ERROR, msg.data());
            return SYS_INVALID_INPUT_PARAM;
        }

        int ec = 0;
        json json_input;
        std::tie(ec, json_input) = parse_json(*_input);

        if (ec != 0) {
            rodsLog(LOG_ERROR, "Failed to parse JSON string [error_code=%d]", ec);
            return ec;
        }

        int l1desc_index = -1;
        std::tie(ec, l1desc_index) = get_file_descriptor_index(json_input);

        if (ec != 0) {
            rodsLog(LOG_ERROR, "Failed to extract the L1 descriptor index from the JSON object [error_code=%d]", ec);
            return ec;
        }

        if (l1desc_index < 3 || l1desc_index >= NUM_L1_DESC) {
            rodsLog(LOG_ERROR, "L1 descriptor index is out of range [error_code=%d, fd=%d].", BAD_INPUT_DESC_INDEX, l1desc_index);
            return BAD_INPUT_DESC_INDEX;
        }

        const auto& l1desc = L1desc[l1desc_index];
        const auto is_write_operation = (O_RDONLY != (l1desc.dataObjInp->openFlags & O_ACCMODE));
        const auto send_notifications = !json_input.contains("send_notifications") || json_input.at("send_notifications").get<bool>();
        const auto update_status = !json_input.contains("update_status") || json_input.at("update_status").get<bool>();

        try {
            if (l1desc.inuseFlag != FD_INUSE) {
                rodsLog(LOG_ERROR, "File descriptor is not open [error_code=%d, fd=%d].", BAD_INPUT_DESC_INDEX, l1desc_index);
                return BAD_INPUT_DESC_INDEX;
            }

            // Redirect to the federated zone if the local L1 descriptor references a remote zone.
            if (l1desc.oprType == REMOTE_ZONE_OPR && l1desc.remoteZoneHost) {
                auto* conn = l1desc.remoteZoneHost->conn;

                auto j_in = json_input;
                j_in["fd"] = l1desc.remoteL1descInx;

                if (const auto ec = rc_replica_close(conn, j_in.dump().data()); ec != 0) {
                    rodsLog(LOG_ERROR, "Failed to close remote replica [error_code=%d, remote_l1_descriptor=%d",
                                    ec, l1desc.remoteL1descInx);
                    return ec;
                }

                return free_l1_descriptor(l1desc_index);
            }

            const auto data_id = l1desc.dataObjInfo->dataId;

            const irods::at_scope_exit remove_from_rst{[&json_input, &data_id] {
                const char* const prop = "preserve_replica_state_table";
                if (!json_input.contains(prop) || !json_input.at(prop).get<bool>()) {
                    if (rst::contains(data_id)) {
                        rst::erase(data_id);
                    }
                }
            }};

            const auto update_size = !json_input.contains("update_size") || json_input.at("update_size").get<bool>();
            const auto compute_checksum =
                json_input.contains("compute_checksum") && json_input.at("compute_checksum").get<bool>();

            // Close the underlying file object.
            if (const auto ec = close_physical_object(*_comm, l1desc.l3descInx); ec != 0) {
                irods::log(LOG_ERROR, fmt::format("Failed to close file object [error_code={}].", ec));
                if (is_write_operation && update_status) {
                    update_replica_status_on_error(*_comm, l1desc);
                    free_l1_descriptor(l1desc_index);
                }
                return ec;
            }

            // Set the L3 descriptor index to -1 here so that any subsequent calls to replica_close will not attempt to
            // close the physical object again.
            L1desc[l1desc_index].l3descInx = -1; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

            // Allow updates to the replica's catalog information if the stream supports
            // write operations (i.e. the stream is opened in write-only or read-write mode).
            if (is_write_operation) {
                // Update the replica's information in the catalog if requested.
                if (update_size && update_status) {
                    if (const auto ec = update_replica_size_and_status(*_comm, l1desc, send_notifications); ec != 0) {
                        rodsLog(LOG_ERROR, "Failed to update the replica size and status in the catalog [error_code=%d].", ec);
                        update_replica_status_on_error(*_comm, l1desc);
                        return ec;
                    }
                }
                else if (update_size) {
                    if (const auto ec = update_replica_size(*_comm, l1desc, send_notifications); ec != 0) {
                        // Intentionally do not update the replica status here. The client explicitly disabled updating
                        // the status, so it is not updated. This leaves the data object in a locked status and the
                        // caller is now responsible for unlocking/finalizing the data object. Note that the physical
                        // data has been closed at this point.
                        rodsLog(LOG_ERROR, "Failed to update the replica size in the catalog [error_code=%d].", ec);
                        return ec;
                    }
                }
                else if (update_status) {
                    // Default to setting new status to good on the condition that things moved in an expected manner.
                    int new_status = GOOD_REPLICA;

                    constexpr auto verify_size = false;
                    const auto size_on_disk = irods::get_size_in_vault(*_comm, *l1desc.dataObjInfo, verify_size, l1desc.dataSize);

                    if (size_on_disk < 0) {
                        // Stale replica status on failure to verify size on disk.
                        rodsLog(LOG_ERROR, "Failed to update the replica status in the catalog [error_code=%d].", ec);
                        new_status = STALE_REPLICA;
                    }
                    else if (size_on_disk == l1desc.dataObjInfo->dataSize &&
                             l1desc.bytesWritten <= 0 &&
                             CREATE_TYPE != l1desc.openType) {
                        // If nothing changed, the replica status should be restored to the original status.
                        new_status = ill::get_original_replica_status(l1desc.dataObjInfo->dataId, l1desc.dataObjInfo->replNum);
                    }

                    if (const auto ec =
                            update_replica_status(*_comm, l1desc, new_status, send_notifications, STALE_REPLICA);
                        ec != 0) {
                        rodsLog(LOG_ERROR, "Failed to update the replica status in the catalog [error_code=%d].", ec);
                        update_replica_status_on_error(*_comm, l1desc);
                        return ec;
                    }
                }

                // [Re]compute a checksum for the replica if requested.
                if (compute_checksum) {
                    const auto& info = *l1desc.dataObjInfo;
                    constexpr const auto calculation = irods::experimental::replica::verification_calculation::always;
                    irods::experimental::replica::replica_checksum(*_comm, info.objPath, info.replNum, calculation);
                }

                // Remove the agent's PID from the replica access table for this replica. The replica access table entry
                // is only erased when everythng else is successful.
                irods::experimental::replica_access_table::erase_pid(l1desc.replica_token, getpid());
            }

            return free_l1_descriptor(l1desc_index);
        }
        catch (const json::type_error& e) {
            rodsLog(LOG_ERROR, "Failed to extract property from JSON object [error_code=%d]", SYS_INTERNAL_ERR);
            if (is_write_operation && update_status) {
                update_replica_status_on_error(*_comm, l1desc);
            }
            return SYS_INTERNAL_ERR;
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "%s [error_code=%d]", e.what(), e.code());
            if (is_write_operation && update_status) {
                update_replica_status_on_error(*_comm, l1desc);
            }
            return e.code();
        }
        catch (const fs::filesystem_error& e) {
            rodsLog(LOG_ERROR, "%s [error_code=%d]", e.what(), e.code().value());
            if (is_write_operation && update_status) {
                update_replica_status_on_error(*_comm, l1desc);
            }
            return e.code().value();
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "An unexpected error occurred while closing the replica. %s [error_code=%d]",
                            e.what(), SYS_INTERNAL_ERR);
            if (is_write_operation && update_status) {
                update_replica_status_on_error(*_comm, l1desc);
            }
            return SYS_INTERNAL_ERR;
        }
    } // rs_replica_close

    const operation op = rs_replica_close;
    #define CALL_REPLICA_CLOSE call_replica_close
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rcComm_t*, bytesBuf_t*)>;
    const operation op{};
    #define CALL_REPLICA_CLOSE nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(REPLICA_CLOSE_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{REPLICA_CLOSE_APN,                // API number
                        RODS_API_VERSION,                 // API version
                        NO_USER_AUTH,                     // Client auth
                        NO_USER_AUTH,                     // Proxy auth
                        "BinBytesBuf_PI", 0,              // In PI / bs flag
                        nullptr, 0,                       // Out PI / bs flag
                        op,                               // Operation
                        "api_replica_close",              // Operation name
                        nullptr,                          // Clear function
                        (funcPtr) CALL_REPLICA_CLOSE};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BinBytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    return api;
}

