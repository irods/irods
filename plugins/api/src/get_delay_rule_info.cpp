#include "irods/apiHandler.hpp"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rcConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsPackInstruct.h"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

// clang-format off
#include "irods/get_delay_rule_info.h"

#include "irods/catalog.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_re_serialization.hpp"
#include "irods/irods_server_api_call.hpp"
#include "irods/server_utilities.hpp"

#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <string>
// clang-format on

namespace
{
    using log_api = irods::experimental::log::api;

    //
    // Function Prototypes
    //

    auto call_get_delay_rule_info(irods::api_entry*, RsComm*, const char*, BytesBuf**) -> int;

    auto rs_get_delay_rule_info_impl(RsComm*, const char*, BytesBuf**) -> int;

    auto is_valid_input(const char*) -> bool;

    //
    // Function Implementations
    //

    auto call_get_delay_rule_info(irods::api_entry* _api, RsComm* _comm, const char* _rule_id, BytesBuf** _output)
        -> int
    {
        return _api->call_handler<const char*, BytesBuf**>(_comm, _rule_id, _output);
    } // call_get_delay_rule_info

    auto is_valid_input(const char* _rule_id, BytesBuf** _output) -> bool
    {
        if (!_rule_id || !_output) {
            log_api::error("Invalid input: _rule_id or _output is null");
            return false;
        }

        // 64-bit integers can use up to 20 digits.
        // We add 1 to the value to accomodate for the null byte.
        // This size does not consider negative, hexidecimal, or octal values.
        constexpr std::size_t max_number_of_digits = 20 + 1;

        if (!is_non_empty_string(_rule_id, max_number_of_digits)) {
            log_api::error("Invalid input: _rule_id is empty or cannot be represented by a 64-bit integer.");
            return false;
        }

        // Try to parse the string into a valid signed 64-bit integer.
        try {
            if (const auto i = std::stoll(_rule_id); i <= 0) {
                log_api::error("Invalid input: _rule_id [{}] must be greater than zero.", i);
                return false;
            }
        }
        catch (...) {
            log_api::error("Invalid input: could not parse _rule_id [{}] into a signed integer.", _rule_id);
            return false;
        }

        return true;
    } // is_valid_input

    auto rs_get_delay_rule_info_impl(RsComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
    {
        if (!is_valid_input(_rule_id, _output)) {
            return SYS_INVALID_INPUT_PARAM;
        }

        log_api::debug("{}: _rule_id = [{}]", __func__, _rule_id);

        try {
            namespace ic = irods::experimental::catalog;

            if (!ic::connected_to_catalog_provider(*_comm)) {
                log_api::trace("Redirecting request to catalog service provider ...");
                auto* host_info = ic::redirect_to_catalog_provider(*_comm);
                return rc_get_delay_rule_info(host_info->conn, _rule_id, _output);
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            log_api::error(e.what());
            return e.code();
        }

        std::vector<std::string> row;

        if (const auto ec = chl_get_delay_rule_info(*_comm, _rule_id, &row); ec != 0) {
            log_api::error("Could not get delay rule information [rule id=[{}]]", _rule_id);
            return ec;
        }

        try {
            using json = nlohmann::json;

            // clang-format off
            const auto json_string = json{
                {"rule_id", _rule_id},
                {"rule_name", row.at(0)},
                {"rei_file_path", row.at(1)},
                {"user_name", row.at(2)},
                {"exe_address", row.at(3)},
                {"exe_time", row.at(4)},
                {"exe_frequency", row.at(5)},
                {"priority", row.at(6)},
                {"last_exe_time", row.at(7)},
                {"exe_status", row.at(8)},
                {"estimated_exe_time", row.at(9)},
                {"notification_addr", row.at(10)},
                {"exe_context", row.at(11)}
            }.dump();
            // clang-format on

            *_output = irods::to_bytes_buffer(json_string);
        }
        catch (const std::exception& e) {
            log_api::error("An error occurred while serializing the JSON object: {}", e.what());
            return SYS_LIBRARY_ERROR;
        }
        catch (...) {
            log_api::error("An unknown error occurred while serializing the JSON object");
            return SYS_UNKNOWN_ERROR;
        }

        return 0;
    } // rs_get_delay_rule_info_impl

    using operation = std::function<int(RsComm*, const char*, BytesBuf**)>;
    const operation op = rs_get_delay_rule_info_impl;
    auto fn_ptr = reinterpret_cast<funcPtr>(call_get_delay_rule_info);
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const char*, BytesBuf**)>;
    const operation op{};
    funcPtr fn_ptr = nullptr;
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C" auto plugin_factory(
    [[maybe_unused]] const std::string& _instance_name, // NOLINT(bugprone-easily-swappable-parameters)
    [[maybe_unused]] const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{
        GET_DELAY_RULE_INFO_APN,         // API number
        RODS_API_VERSION,                // API version
        LOCAL_PRIV_USER_AUTH,            // Client auth
        LOCAL_PRIV_USER_AUTH,            // Proxy auth
        "STR_PI", 0,                     // In PI / bs flag
        "BinBytesBuf_PI", 0,             // Out PI / bs flag
        op,                              // Operation
        "api_get_delay_rule_info",       // Operation name
        irods::clearInStruct_noop,       // Clear input function
        clearBytesBuffer,                // Clear output function
        fn_ptr
    };
    // clang-format on

    auto* api = new irods::api_entry{def}; // NOLINT(cppcoreguidelines-owning-memory)

    api->in_pack_key = "STR_PI";
    api->in_pack_value = STR_PI;

    api->out_pack_key = "BinBytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
} // plugin_factory
