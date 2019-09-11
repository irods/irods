#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"

#include "apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "objDesc.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_logger.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include <string>
#include <tuple>
#include <system_error>

#include "json.hpp"

namespace
{
    // clang-format off
    namespace fs    = irods::experimental::filesystem;

    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    // clang-format on

    //
    // Function Prototypes
    //

    auto call_batch_apply_metadata_operations(irods::api_entry*, rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;
    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;
    auto get_file_descriptor(const bytesBuf_t& _buf) -> int;
    auto rs_batch_apply_metadata_operations(rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    //
    // Function Implementations
    //

    auto call_batch_apply_metadata_operations(irods::api_entry* _api,
                                              rsComm_t* _comm,
                                              bytesBuf_t* _input,
                                              bytesBuf_t** _output) -> int
    {
        return _api->call_handler<bytesBuf_t*, bytesBuf_t**>(_comm, _input, _output);
    }

    auto is_input_valid(const bytesBuf_t* _input) -> std::tuple<bool, std::string>
    {
        if (!_input) {
            return {false, "Missing JSON input"};
        }

        if (_input->len <= 0) {
            return {false, "Length of buffer must be greater than zero"};
        }

        if (!_input->buf) {
            return {false, "Missing input buffer"};
        }

        return {true, ""};
    }

    auto to_bytes_buffer(const std::string& _s) -> bytesBuf_t*
    {
        const auto buf_size = _s.length() + 1;

        char* buf = new char[buf_size]{};
        std::strncpy(buf, _s.c_str(), _s.length());

        bytesBuf_t* bbp = new bytesBuf_t{};
        bbp->len = buf_size;
        bbp->buf = buf;

        return bbp;
    }

    auto throw_if_object_type_is_incorrect(rsComm_t& _comm, const json& _operation) -> void
    {
        const auto entity_name = _operation.at("entity_name").template get<std::string>();
        const auto entity_type = _operation.at("entity_type").template get<std::string>();
        const auto status = fs::server::status(_comm, entity_name);

        switch (status.type()) {
            case fs::object_type::data_object:
                if ("data_object" != entity_type) {
                    throw std::system_error{SYS_INVALID_INPUT_PARAM, std::generic_category(), "Incorrect entity type"};
                }
                break;

            case fs::object_type::collection:
                if ("collection" != entity_type) {
                    throw std::system_error{SYS_INVALID_INPUT_PARAM, std::generic_category(), "Incorrect entity type"};
                }
                break;

            default:
                throw std::system_error{SYS_INVALID_INPUT_PARAM, std::generic_category(), "Entity type not supported"};
        }
    }

    auto make_error_object(const json& _op, int _op_index, const std::string& _error_msg) -> json
    {
        return json{
            {"operation", _op},
            {"operation_index", _op_index},
            {"error_message", _error_msg}
        };
    }

    auto rs_batch_apply_metadata_operations(rsComm_t* _comm, bytesBuf_t* _input, bytesBuf_t** _output) -> int
    {
        using log = irods::experimental::log;

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            return 0;
        }

        json operations;

        try {
            operations = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
        }
        catch (const json::parse_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Failed to parse input into JSON"},
                             {"error_message", e.what()}});
            // clang-format on

            const auto err_info = make_error_object(json{}, 0, e.what());
            *_output = to_bytes_buffer(err_info.dump());

            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        for (json::size_type i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];

            try {
                using func_type = std::function<bool(rsComm_t&, const fs::path&, const fs::metadata&)>;

                func_type func; 

                if (const std::string op_code = op.at("operation").get<std::string>(); op_code == "set") {
                    func = fs::server::set_metadata;
                }
                else if (op_code == "remove") {
                    func = fs::server::remove_metadata;
                }
                else {
                    // clang-format off
                    log::api::error({{"log_message", "Invalid metadata operation"},
                                     {"metadata_operation", op.dump()}});
                    // clang-format on

                    const auto err_info = make_error_object(op, i, "Invalid metadata operation.");
                    *_output = to_bytes_buffer(err_info.dump());

                    return INVALID_OPERATION;
                }

                throw_if_object_type_is_incorrect(*_comm, op);

                std::string units; // Units are optional.

                if (op.count("units") > 0) {
                    units = op["units"].get<std::string>();
                }

                func(*_comm, op.at("entity_name").get<std::string>(), {
                    op.at("attribute").get<std::string>(),
                    op.at("value").get<std::string>(),
                    units
                });
            }
            catch (const fs::filesystem_error& e) {
                // clang-format off
                log::api::error({{"log_message", e.what()},
                                 {"metadata_operation", op.dump()}});
                // clang-format on

                const auto err_info = make_error_object(op, i, e.what());
                *_output = to_bytes_buffer(err_info.dump());

                return e.code().value();
            }
            catch (const json::out_of_range& e) {
                // clang-format off
                log::api::error({{"log_message", e.what()},
                                 {"metadata_operation", op.dump()}});
                // clang-format on

                const auto err_info = make_error_object(op, i, e.what());
                *_output = to_bytes_buffer(err_info.dump());

                return SYS_INTERNAL_ERR;
            }
            catch (const json::type_error& e) {
                // clang-format off
                log::api::error({{"log_message", e.what()},
                                 {"metadata_operation", op.dump()}});
                // clang-format on

                const auto err_info = make_error_object(op, i, e.what());
                *_output = to_bytes_buffer(err_info.dump());

                return SYS_INTERNAL_ERR;
            }
            catch (const std::system_error& e) {
                // clang-format off
                log::api::error({{"log_message", e.what()},
                                 {"metadata_operation", op.dump()}});
                // clang-format on

                const auto err_info = make_error_object(op, i, e.what());
                *_output = to_bytes_buffer(err_info.dump());

                return e.code().value();
            }
        }

        *_output = to_bytes_buffer(json::object().dump());

        return 0;
    }

    const operation op = rs_batch_apply_metadata_operations;
    #define CALL_BATCH_APPLY_METADATA_OPERATIONS call_batch_apply_metadata_operations
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_BATCH_APPLY_METADATA_OPERATIONS nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{BATCH_APPLY_METADATA_OPERATIONS_APN,        // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        "BytesBuf_PI", 0,                           // Out PI / bs flag
                        op,                                         // Operation
                        "rs_batch_apply_metadata_operations",       // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_BATCH_APPLY_METADATA_OPERATIONS};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

