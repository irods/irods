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
#include "irods_configuration_keywords.hpp"
#include "irods_exception.hpp"
#include "irods_logger.hpp"
#include "irods_query.hpp"
#include "irods_re_serialization.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_api_call.hpp"
#include "replica_access_table.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"
#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica.hpp"

#include "replica_state_table.hpp"

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
    namespace ix = irods::experimental;
    namespace fs = irods::experimental::filesystem;
    namespace rst = irods::replica_state_table;

    // clang-format off
    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*)>;
    using log       = ix::log;
    // clang-format on

    //
    // Constants
    //

    // clang-format off
    const char* const REPLICA_STATUS_STALE = "0";
    const char* const REPLICA_STATUS_GOOD  = "1";
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

    auto get_file_size(rsComm_t& _comm, const l1desc_t& _l1desc) -> std::int64_t;

    auto current_time_in_seconds() -> std::string;

    auto update_replica_size_and_status(rsComm_t& _comm,
                                        const l1desc_t& _l1desc,
                                        const bool _send_notifications) -> int;

    auto update_replica_size(rsComm_t& _comm,
                             const l1desc_t& _l1desc,
                             const bool _send_notifications) -> int;

    auto update_replica_status(rsComm_t& _comm,
                               const l1desc_t& _l1desc,
                               std::string_view _new_status,
                               const bool _send_notifications) -> int;

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
            log::api::trace("Parsing string into JSON ... [string={}]", json_string);
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
        fileCloseInp_t input{};
        input.fileInx = _l3desc_index;
        return rsFileClose(&_comm, &input);
    }

    auto get_file_size(rsComm_t& _comm, const l1desc_t& _l1desc) -> std::int64_t
    {
        const auto& info = *_l1desc.dataObjInfo;

        std::string location;
        if (const auto err = irods::get_loc_for_hier_string(info.rescHier, location); !err.ok()) {
            log::api::error("Failed to resolve resource hierarchy to hostname [error_code={}]", err.code());
            return err.code();
        }

        fileStatInp_t stat_input{};
        std::strncpy(stat_input.objPath, info.objPath, MAX_NAME_LEN);
        std::strncpy(stat_input.fileName, info.filePath, MAX_NAME_LEN);
        std::strncpy(stat_input.rescHier, info.rescHier, MAX_NAME_LEN);
        std::strncpy(stat_input.addr.hostAddr, location.c_str(), NAME_LEN);

        rodsStat_t* stat_output{};
        if (const auto ec = rsFileStat(&_comm, &stat_input, &stat_output); ec != 0) {
            log::api::error("Could not stat file [error_code={}, logical_path={}, physical_path={}]",
                            ec, info.objPath, info.filePath);
            return ec;
        }

        const auto size = stat_output->st_size;
        std::free(stat_output);

        return size;
    }

    auto current_time_in_seconds() -> std::string
    {
        // clang-format off
        using clock_type    = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;
        // clang-format on

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());

        return fmt::format("{:011}", now.time_since_epoch().count());
    }

    auto update_replica_size_and_status(rsComm_t& _comm, const l1desc_t& _l1desc, const bool _send_notifications) -> int
    {
        const auto size_on_disk = get_file_size(_comm, _l1desc);

        if (size_on_disk < 0) {
            log::api::error("Failed to retrieve the replica's size on disk [error_code={}].", size_on_disk);
            return size_on_disk;
        }

        dataObjInfo_t info{};
        std::strncpy(info.objPath, _l1desc.dataObjInfo->objPath, MAX_NAME_LEN);
        std::strncpy(info.rescHier, _l1desc.dataObjInfo->rescHier, MAX_NAME_LEN);

        keyValPair_t reg_params{};

        if (CREATE_TYPE == _l1desc.openType) {
            addKeyVal(&reg_params, DATA_SIZE_KW, std::to_string(size_on_disk).data());
            addKeyVal(&reg_params, REPL_STATUS_KW, REPLICA_STATUS_GOOD);
        }
        // If the size of the replica has changed since opening it, then update the size.
        else if (_l1desc.dataObjInfo->dataSize != size_on_disk) {
            addKeyVal(&reg_params, DATA_SIZE_KW, std::to_string(size_on_disk).data());
            addKeyVal(&reg_params, DATA_MODIFY_KW, current_time_in_seconds().data());
            addKeyVal(&reg_params, REPL_STATUS_KW, REPLICA_STATUS_GOOD);
        }
        // If the contents of the replica has changed, then update the last modified timestamp.
        else if (_l1desc.bytesWritten > 0) {
            addKeyVal(&reg_params, DATA_MODIFY_KW, current_time_in_seconds().data());
            addKeyVal(&reg_params, REPL_STATUS_KW, REPLICA_STATUS_GOOD);
        }
        // If nothing has been written, the status is restored from the replica state table
        // so that the replica is not mistakenly marked as good when it is in fact stale.
        else {
            const auto previous_status = std::stoi(rst::get_property(_l1desc.dataObjInfo->objPath, _l1desc.dataObjInfo->replNum, "data_is_dirty"));

            addKeyVal(&reg_params, REPL_STATUS_KW, std::to_string(previous_status).data());
            addKeyVal(&reg_params, DATA_MODIFY_KW, current_time_in_seconds().data());
        }

        // Possibly triggers file modified notification.
        if (_send_notifications) {
            addKeyVal(&reg_params, OPEN_TYPE_KW, std::to_string(_l1desc.openType).data());
        }

        modDataObjMeta_t input{};
        input.dataObjInfo = &info;
        input.regParam = &reg_params;

        return rsModDataObjMeta(&_comm, &input);
    }

    auto update_replica_size(rsComm_t& _comm, const l1desc_t& _l1desc, const bool _send_notifications) -> int
    {
        const auto size_on_disk = get_file_size(_comm, _l1desc);

        if (size_on_disk < 0) {
            log::api::error("Failed to retrieve the replica's size on disk [error_code={}].", size_on_disk);
            return size_on_disk;
        }

        dataObjInfo_t info{};
        std::strncpy(info.objPath, _l1desc.dataObjInfo->objPath, MAX_NAME_LEN);
        std::strncpy(info.rescHier, _l1desc.dataObjInfo->rescHier, MAX_NAME_LEN);

        keyValPair_t reg_params{};

        // If the size of the replica has changed since opening it, then update the size.
        if (_l1desc.dataObjInfo->dataSize != size_on_disk) {
            addKeyVal(&reg_params, DATA_SIZE_KW, std::to_string(size_on_disk).data());
            addKeyVal(&reg_params, DATA_MODIFY_KW, current_time_in_seconds().data());
        }
        // If the contents of the replica has changed, then update the last modified timestamp.
        else if (_l1desc.bytesWritten > 0) {
            addKeyVal(&reg_params, DATA_MODIFY_KW, current_time_in_seconds().data());
        }
        else {
            // Return immediately if nothing needs to be updated. Allowing the function
            // to continue past this point results in an SQL update error due to invalid
            // SQL (i.e. no columns were specified for the update).
            return 0;
        }

        // Possibly triggers file modified notification.
        if (_send_notifications) {
            addKeyVal(&reg_params, OPEN_TYPE_KW, std::to_string(_l1desc.openType).data());
        }

        modDataObjMeta_t input{};
        input.dataObjInfo = &info;
        input.regParam = &reg_params;

        return rsModDataObjMeta(&_comm, &input);
    }

    auto update_replica_status(rsComm_t& _comm,
                               const l1desc_t& _l1desc,
                               std::string_view _new_status,
                               const bool _send_notifications) -> int
    {
        dataObjInfo_t info{};
        std::strncpy(info.objPath, _l1desc.dataObjInfo->objPath, MAX_NAME_LEN);
        std::strncpy(info.rescHier, _l1desc.dataObjInfo->rescHier, MAX_NAME_LEN);

        keyValPair_t reg_params{};
        addKeyVal(&reg_params, REPL_STATUS_KW, _new_status.data());

        // Possibly triggers file modified notification.
        if (_send_notifications) {
            addKeyVal(&reg_params, OPEN_TYPE_KW, std::to_string(_l1desc.openType).data());
        }

        modDataObjMeta_t input{};
        input.dataObjInfo = &info;
        input.regParam = &reg_params;

        return rsModDataObjMeta(&_comm, &input);
    }

    auto free_l1_descriptor(int _l1desc_index) -> int
    {
        if (const auto ec = freeL1desc(_l1desc_index); ec != 0) {
            log::api::error("Failed to release L1 descriptor [error_code={}].", ec);
            return ec;
        }

        return 0;
    }

    auto rs_replica_close(rsComm_t* _comm, bytesBuf_t* _input) -> int
    {
        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            return SYS_INVALID_INPUT_PARAM;
        }

        int ec = 0;
        json json_input;
        std::tie(ec, json_input) = parse_json(*_input);

        if (ec != 0) {
            log::api::error("Failed to parse JSON string [error_code={}]", ec);
            return ec;
        }

        int l1desc_index = -1;
        std::tie(ec, l1desc_index) = get_file_descriptor_index(json_input);

        if (ec != 0) {
            log::api::error("Failed to extract the L1 descriptor index from the JSON object [error_code={}]", ec);
            return ec;
        }

        if (l1desc_index < 3 || l1desc_index >= NUM_L1_DESC) {
            log::api::error("L1 descriptor index is out of range [error_code={}, fd={}].", BAD_INPUT_DESC_INDEX, l1desc_index);
            return BAD_INPUT_DESC_INDEX;
        }

        const auto& l1desc = L1desc[l1desc_index];
        const auto send_notifications = !json_input.contains("send_notifications") || json_input.at("send_notifications").get<bool>();

        try {
            if (l1desc.inuseFlag != FD_INUSE) {
                log::api::error("File descriptor is not open [error_code={}, fd={}].", BAD_INPUT_DESC_INDEX, l1desc_index);
                return BAD_INPUT_DESC_INDEX;
            }

            // Redirect to the federated zone if the local L1 descriptor references a remote zone.
            if (l1desc.oprType == REMOTE_ZONE_OPR && l1desc.remoteZoneHost) {
                auto* conn = l1desc.remoteZoneHost->conn;

                auto j_in = json_input;
                j_in["fd"] = l1desc.remoteL1descInx;

                if (const auto ec = rc_replica_close(conn, j_in.dump().data()); ec != 0) {
                    log::api::error("Failed to close remote replica [error_code={}, remote_l1_descriptor={}",
                                    ec, l1desc.remoteL1descInx);
                    return ec;
                }

                return free_l1_descriptor(l1desc_index);
            }

            const auto is_write_operation = (O_RDONLY != (l1desc.dataObjInp->openFlags & O_ACCMODE));

            const std::string logical_path = l1desc.dataObjInfo->objPath;

            // Allow updates to the replica's catalog information if the stream supports
            // write operations (i.e. the stream is opened in write-only or read-write mode).
            if (is_write_operation) {
                const auto update_size = !json_input.contains("update_size") || json_input.at("update_size").get<bool>();
                const auto update_status = !json_input.contains("update_status") || json_input.at("update_status").get<bool>();
                const auto compute_checksum = json_input.contains("compute_checksum") && json_input.at("compute_checksum").get<bool>();
                const auto update_catalog = update_size || update_status || compute_checksum || send_notifications;

                const irods::at_scope_exit remove_from_rst{[&logical_path, &update_catalog]
                    {
                        if (update_catalog && rst::contains(logical_path)) {
                            rst::erase(logical_path);
                        }
                    }
                };

                // Update the replica's information in the catalog if requested.
                if (update_size && update_status) {
                    if (const auto ec = update_replica_size_and_status(*_comm, l1desc, send_notifications); ec != 0) {
                        log::api::error("Failed to update the replica size and status in the catalog [error_code={}].", ec);
                        update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
                        return ec;
                    }
                }
                else if (update_size) {
                    if (const auto ec = update_replica_size(*_comm, l1desc, send_notifications); ec != 0) {
                        log::api::error("Failed to update the replica size in the catalog [error_code={}].", ec);
                        update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
                        return ec;
                    }
                }
                else if (update_status) {
                    // Default to setting new status to good on the condition that things moved in an expected manner.
                    int new_status = GOOD_REPLICA;

                    const auto size_on_disk = get_file_size(*_comm, l1desc);

                    if (size_on_disk < 0) {
                        // Stale replica status on failure to verify size on disk.
                        log::api::error("Failed to retrieve the replica's size on disk [error_code={}].", size_on_disk);
                        new_status = STALE_REPLICA;
                    }
                    else if (size_on_disk == l1desc.dataObjInfo->dataSize &&
                             l1desc.bytesWritten <= 0 &&
                             CREATE_TYPE != l1desc.openType) {
                        // If nothing changed, the replica status should be restored to the original status.
                        new_status = std::stoi(rst::get_property(l1desc.dataObjInfo->objPath, l1desc.dataObjInfo->replNum, "data_is_dirty"));
                    }

                    if (const auto ec = update_replica_status(*_comm, l1desc, std::to_string(new_status), send_notifications); ec != 0) {
                        log::api::error("Failed to update the replica status in the catalog [error_code={}].", ec);
                        update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
                        return ec;
                    }
                }

                // [Re]compute a checksum for the replica if requested.
                if (compute_checksum) {
                    const auto& info = *l1desc.dataObjInfo;
                    constexpr const auto calculation = irods::experimental::replica::verification_calculation::always;
                    irods::experimental::replica::replica_checksum(*_comm, info.objPath, info.replNum, calculation);
                }
            }

            // Remove the agent's PID from the replica access table.
            auto entry = is_write_operation
                ? ix::replica_access_table::erase_pid(l1desc.replica_token, getpid())
                : std::nullopt;

            // Close the underlying file object.
            if (const auto ec = close_physical_object(*_comm, l1desc.l3descInx); ec != 0) {
                if (entry) {
                    ix::replica_access_table::restore(*entry);
                }

                log::api::error("Failed to close file object [error_code={}].", ec);
                update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
                return ec;
            }

            const auto ec = free_l1_descriptor(l1desc_index);

            if (ec != 0) {
                update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
            }

            return ec;
        }
        catch (const json::type_error& e) {
            log::api::error("Failed to extract property from JSON object [error_code={}]", SYS_INTERNAL_ERR);
            update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
            return SYS_INTERNAL_ERR;
        }
        catch (const irods::exception& e) {
            log::api::error("{} [error_code={}]", e.what(), e.code());
            update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
            return e.code();
        }
        catch (const fs::filesystem_error& e) {
            log::api::error("{} [error_code={}]", e.what(), e.code().value());
            update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
            return e.code().value();
        }
        catch (const std::exception& e) {
            log::api::error("An unexpected error occurred while closing the replica. {} [error_code={}]",
                            e.what(), SYS_INTERNAL_ERR);
            update_replica_status(*_comm, l1desc, REPLICA_STATUS_STALE, send_notifications);
            return SYS_INTERNAL_ERR;
        }
    }

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
                        "BytesBuf_PI", 0,                 // In PI / bs flag
                        nullptr, 0,                       // Out PI / bs flag
                        op,                               // Operation
                        "api_replica_close",              // Operation name
                        nullptr,                          // Clear function
                        (funcPtr) CALL_REPLICA_CLOSE};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    return api;
}

