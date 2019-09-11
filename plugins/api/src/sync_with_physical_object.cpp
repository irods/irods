#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"

#include "apiHandler.hpp"

#include <functional>

// TODO Finalize needs to accept the info from rc_get_file_descriptor_info.
// Finalize also needs to accept a list of AVUs and ACLs.
// dstream calls finalize instead of close.
//
// Finalize accepts a flag that instructs it to update the catalog
// and apply metadata and everything else. If this flag is not set,
// then JUST close the data object.
// {
//   update_catalog: boolean, (default=true)
//   file_descriptor: int,
//   file_descriptor_info: {...},
//   metadata: [{key, value, units}, ...], (optional) (should match the schema from the MDTWG)
//   acl: [{ace}, ...] (optional)
// }
//
// Need two factories:
// - One that returns a stream object with additional context.
// - Another that returns a stream object constructed with info from the first one.
//
// getHostForPut() might be useful.
//
// Finalize will need to redirect.
// Finalize will need to clean up the L1 descriptor table.
//
// streams will not invoke rxDataObjClose at all.
// rxDataObjClose will just invoke finalize.

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "objDesc.hpp"
#include "rsFileClose.hpp"
#include "rsFileStat.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_resource_backport.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_query.hpp"
#include "irods_exception.hpp"
#include "irods_logger.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include "json.hpp"

#include <string>
#include <tuple>

namespace
{
    namespace fs = irods::experimental::filesystem;

    // clang-format off
    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*)>;
    // clang-format on

    //
    // Function Prototypes
    //

    auto call_sync_with_physical_object(irods::api_entry*, rsComm_t*, bytesBuf_t*) -> int;
    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;
    auto rs_sync_with_physical_object(rsComm_t* _comm, bytesBuf_t* _input) -> int;

    //
    // Function Implementations
    //

    auto call_sync_with_physical_object(irods::api_entry* _api, rsComm_t* _comm, bytesBuf_t* _input) -> int
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

    auto close_physical_object(rsComm_t& _comm, int _l3desc_index) -> int
    {
        fileCloseInp_t input{};
        input.fileInx = _l3desc_index;
        return rsFileClose(&_comm, &input);
    }

    auto file_size(rsComm_t& _comm, const json& _fd_info) -> std::int64_t
    {
        const auto& data_object_info = _fd_info["data_object_info"];
        const auto resource_hierarchy = data_object_info["resource_hierarchy"].get<std::string>();

        std::string location;
        if (const auto err = irods::get_loc_for_hier_string(resource_hierarchy, location); !err.ok()) {
            throw std::runtime_error{"Failed to resolve resource hierarchy to hostname."};
        }

        const auto logical_path = data_object_info["object_path"].get<std::string>();
        const auto physical_path = data_object_info["file_path"].get<std::string>();

        fileStatInp_t stat_input{};
        std::strncpy(stat_input.objPath, logical_path.c_str(), MAX_NAME_LEN);
        std::strncpy(stat_input.fileName, physical_path.c_str(), MAX_NAME_LEN);
        std::strncpy(stat_input.rescHier, resource_hierarchy.c_str(), MAX_NAME_LEN);
        std::strncpy(stat_input.addr.hostAddr, location.c_str(), NAME_LEN);
        rodsStat_t* stat_output{};

        if (rsFileStat(&_comm, &stat_input, &stat_output) != 0) {
            throw std::runtime_error{"Cannot stat file."};
        }

        const auto size = stat_output->st_size;
        std::free(stat_output);

        return size;
    }

    auto update_data_object_size(rsComm_t& _comm,
                                 const std::string& _logical_path,
                                 std::int64_t _new_size) -> int
    {
        dataObjInfo_t info{};
        std::strncpy(info.objPath, _logical_path.c_str(), MAX_NAME_LEN);

        const auto new_size_string = std::to_string(_new_size);
        keyValPair_t reg_params{};
        addKeyVal(&reg_params, DATA_SIZE_KW, new_size_string.c_str());

        modDataObjMeta_t input{};
        input.dataObjInfo = &info;
        input.regParam = &reg_params;

        return irods::server_api_call(MOD_DATA_OBJ_META_AN, &_comm, &input);
    }

    auto rs_sync_with_physical_object(rsComm_t* _comm, bytesBuf_t* _input) -> int
    {
        using log = irods::experimental::log;

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            return SYS_INVALID_INPUT_PARAM;
        }

        // TODO The following should happen:
        // 1. Redirect to the correct server.
        // 2. Close the physical file.
        // 3. Clean up (free L1 descriptors)!
        // 4. Stat the physical file to get the actual file size.
        // 5. Update the data object's catalog entry with the correct size.
        // 6. Apply metadata (use modDataObjAvu/Meta).
        // 7. Compute checksum on file at rest and compare to the existing one (verification).
        // 
        // This API plugin needs to do the same things as rsDataObjClose(...).
        // We will revisit redirects at a later time.

        try {
            log::api::trace("Parsing string into JSON ...");
            log::api::trace("Incoming string ==> " + std::string(static_cast<const char*>(_input->buf), _input->len));
            const auto json_input = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
            const auto l1desc_index = json_input["file_descriptor"].get<int>();
            const auto& l1desc = L1desc[l1desc_index];

            log::api::trace("String successfully transformed into JSON.");
            log::api::trace(json_input.dump());

            if (l1desc.inuseFlag != FD_INUSE) {
                // clang-format off
                log::api::error({{"log_message", "Invalid file descriptor."},
                                 {"error_code", std::to_string(BAD_INPUT_DESC_INDEX)}});
                // clang-format on
                return BAD_INPUT_DESC_INDEX;
            }

#if 0
            // Use irods::get_host_for_hier_string(..) and check remote flag.
            // If we are not on the server that holds the target replica, then redirect.
            if (l1desc.remoteZoneHost) {
                log::api::trace("We're on the wrong server. Preparing to redirect to the target server ...");

                // FIXME The file descriptor info should be the same for each server involved.
                // So there's no need to fetch the file descriptor info from the remote server.
                
                // This is no longer needed.
                // TODO Need to fetch the file descriptor info for the object referenced on the remote server
                // and then pass that to the rc_sync call.
                const auto json_input = json{{"fd", l1desc.remoteL1descInx}}.dump();
                char* json_output{};

                log::api::trace("Fetching file descriptor information from remote server ...");
                if (const auto ec = rc_get_file_descriptor_info(l1desc.remoteZoneHost->conn, json_input.c_str(), &json_output); ec != 0) {
                    // clang-format off
                    log::api::error({{"log_message", "Failed to retrieve the remote file descriptor information."},
                                     {"error_code", std::to_string(ec)}});
                    // clang-format on
                    return ec;
                }

                log::api::trace({{"remote_file_descriptor_info", json_output}, {"log_message", "LOOK HERE!!!!!"}});

                //if (const auto ec = rc_sync_with_physical_object(l1desc.remoteZoneHost->conn, static_cast<const char*>(_input->buf)); ec != 0) {
                if (const auto ec = rc_sync_with_physical_object(l1desc.remoteZoneHost->conn, json_output); ec != 0) {
                    // clang-format off
                    log::api::error({{"log_message", "Remote data object sync failed."},
                                     {"error_code", std::to_string(ec)}});
                    // clang-format on
                    return ec;
                }

                freeL1desc(l1desc_index);

                return 0;
            }
#endif

            log::api::trace("Closing the physical object ...");

            // Close the underlying file object.
            if (const auto ec = close_physical_object(*_comm, l1desc.l3descInx); ec != 0) {
                // clang-format off
                log::api::error({{"log_message", "Failed to close file object."},
                                 {"error_code", std::to_string(ec)}});
                // clang-format on
                return ec;
            }

            log::api::trace("Releasing L1 descriptor ...");
            if (const auto ec = freeL1desc(l1desc_index); ec != 0) {
                // clang-format off
                log::api::error({{"log_message", "Failed to release L1 descriptor."},
                                 {"error_code", std::to_string(ec)}});
                // clang-format on
                return ec;
            }

            if (json_input["update_catalog"].get<bool>()) {
                if (std::cend(json_input) != json_input.find("file_descriptor_info")) {
                    log::api::trace("Updating the catalog ...");
                    const auto& fd_info = json_input["file_descriptor_info"];
                    const auto size_on_disk = file_size(*_comm, fd_info);
                    const auto logical_path = fd_info["data_object_info"]["object_path"].get<std::string>();

                    if (const auto ec = update_data_object_size(*_comm, logical_path, size_on_disk); ec != 0) {
                        // clang-format off
                        log::api::error({{"log_message", "Failed to update the data object size in the catalog."},
                                         {"error_code", std::to_string(ec)}});
                        // clang-format on
                        return ec;
                    }

                    // Compute checksum and store it in the catalog.
                    if (fd_info["bytes_written"].get<std::int64_t>() > 0) {
                        const int replica_number = fd_info["data_object_info"]["replica_number"].get<int>();
                        constexpr const auto calculation = fs::verification_calculation::always;
                        fs::server::data_object_checksum(*_comm, logical_path, replica_number, calculation);
                    }
                }
                else {
                    log::api::trace("File descriptor information is missing. Skipping catalog update.");
                }
            }

            // TODO Setting metadata and ACLs doesn't feel right.
            //
            // What error code should be returned when at least one of these operations (close, avus, acls) fails?
            // Even if you return the results of each operation through an out parameter, what is the return code in this case?
            //
            // It feels like this function is doing more than what it should.
            // Users can always just apply metadata and/or ACLs following a successful transfer.
            //
            // TODO Apply metadata.
            // TODO Apply ACLs.

            log::api::trace("It worked!!!");

            return 0;
        }
        catch (const json::parse_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Failed to parse input into JSON."},
                             {"error_message", e.what()}});
            // clang-format on
            return SYS_INTERNAL_ERR;
        }
        catch (const json::type_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Incorrect type used to access JSON object."},
                             {"error_message", e.what()}});
            // clang-format on
            return SYS_INTERNAL_ERR;
        }
        catch (const irods::exception& e) {
            // clang-format off
            log::api::error({{"log_message", "iRODS error."},
                             {"irods_error_code", std::to_string(e.code())},
                             {"error_message", e.what()}});
            // clang-format on
            return e.code();
        }
        catch (const fs::filesystem_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Internal server error occurred while syncing data object with physical object."},
                             {"irods_error_code", std::to_string(e.code().value())},
                             {"error_message", e.what()}});
            // clang-format on
            return e.code().value();
        }
        catch (const std::exception& e) {
            // clang-format off
            log::api::error({{"log_message", "Internal server error occurred while syncing data object with physical object."},
                             {"irods_error_code", std::to_string(SYS_INTERNAL_ERR)},
                             {"error_message", e.what()}});
            // clang-format on
            return SYS_INTERNAL_ERR;
        }
    }

    const operation op = rs_sync_with_physical_object;
    #define CALL_SYNC_WITH_PHYSICAL_OBJECT call_sync_with_physical_object
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rcComm_t*, bytesBuf_t*)>;
    const operation op{};
    #define CALL_SYNC_WITH_PHYSICAL_OBJECT nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{SYNC_WITH_PHYSICAL_OBJECT_APN,              // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        nullptr, 0,                                 // Out PI / bs flag
                        op,                                         // Operation
                        "rs_sync_with_physical_object",             // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_SYNC_WITH_PHYSICAL_OBJECT};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    return api;
}

