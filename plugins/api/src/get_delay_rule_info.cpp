#include "apiHandler.hpp"
#include "api_plugin_number.h"
#include "client_api_whitelist.hpp"
#include "rcConnect.h"
#include "rodsErrorTable.h"
#include "rodsPackInstruct.h"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "get_delay_rule_info.h"

#include "catalog.hpp"
#include "catalog_utilities.hpp"
#include "icatHighLevelRoutines.hpp"
#include "irods_re_serialization.hpp"
#include "irods_server_api_call.hpp"
#include "rodsLog.h"
#include "server_utilities.hpp"

#include <json.hpp>
#include <fmt/format.h>

#include <string>

namespace
{
    //
    // Function Prototypes
    //

    auto call_get_delay_rule_info(irods::api_entry*, RsComm*, const char*, BytesBuf**) -> int;

    auto rs_get_delay_rule_info_impl(RsComm*, const char*, BytesBuf**) -> int;

    auto is_valid_input(const char*) -> bool;

    //
    // Function Implementations
    //

    auto call_get_delay_rule_info(irods::api_entry* _api, RsComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
    {
        return _api->call_handler<const char*, BytesBuf**>(_comm, _rule_id, _output);
    } // call_get_delay_rule_info

    auto is_valid_input(const char* _rule_id, BytesBuf** _output) -> bool
    {
        if (!_rule_id || !_output) {
            rodsLog(LOG_ERROR, "Invalid input: _rule_id or _output is null");
            return false;
        }

        // 64-bit integers can use up to 20 digits.
        // We add 1 to the value to accomodate for the null byte.
        // This size does not consider negative, hexidecimal, or octal values.
        constexpr std::size_t max_number_of_digits = 20 + 1;

        if (!is_non_empty_string(_rule_id, max_number_of_digits)) {
            rodsLog(LOG_ERROR, "Invalid input: _rule_id is empty or cannot be represented by a 64-bit integer.");
            return false;
        }

        // Try to parse the string into a valid signed 64-bit integer.
        try {
            if (const auto i = std::stoll(_rule_id); i <= 0) {
                rodsLog(LOG_ERROR, "Invalid input: _rule_id [%d] must be greater than zero.", i);
                return false;
            }
        }
        catch (...) {
            rodsLog(LOG_ERROR, "Invalid input: could not parse _rule_id [%s] into a signed integer.", _rule_id);
            return false;
        }

        return true;
    } // is_valid_input

    auto rs_get_delay_rule_info_impl(RsComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
    {
        if (!is_valid_input(_rule_id, _output)) {
            return SYS_INVALID_INPUT_PARAM;
        }

        rodsLog(LOG_DEBUG, "%s: _rule_id = [%s]", __func__, _rule_id);

        try {
            namespace ic = irods::experimental::catalog;

            if (!ic::connected_to_catalog_provider(*_comm)) {
                irods::log(LOG_DEBUG8, "Redirecting request to catalog service provider ...");
                auto* host_info = ic::redirect_to_catalog_provider(*_comm);
                return rc_get_delay_rule_info(host_info->conn, _rule_id, _output);
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            return e.code();
        }

        std::vector<std::string> row;

        if (const auto ec = chl_get_delay_rule_info(*_comm, _rule_id, &row); ec != 0) {
            rodsLog(LOG_ERROR, "Could not get delay rule information [rule id=[%s]]", _rule_id);
            return ec;
        }

        try {
            using json = nlohmann::json;

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

            *_output = irods::to_bytes_buffer(json_string);
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "An error occurred while serializing the JSON object: %s", e.what());
            return SYS_LIBRARY_ERROR;
        }
        catch (...) {
            rodsLog(LOG_ERROR, "An unknown error occurred while serializing the JSON object");
            return SYS_UNKNOWN_ERROR;
        }

        return 0;
    } // rs_get_delay_rule_info_impl

    using operation = std::function<int(RsComm*, const char*, BytesBuf**)>;
    const operation op = rs_get_delay_rule_info_impl;
    #define CALL_GET_DELAY_RULE_INFO call_get_delay_rule_info
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const char*, BytesBuf**)>;
    const operation op{};
    #define CALL_GET_DELAY_RULE_INFO nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{GET_DELAY_RULE_INFO_APN,         // API number
                        RODS_API_VERSION,                // API version
                        LOCAL_PRIV_USER_AUTH,            // Client auth
                        LOCAL_PRIV_USER_AUTH,            // Proxy auth
                        "STR_PI", 0,                     // In PI / bs flag
                        "BinBytesBuf_PI", 0,             // Out PI / bs flag
                        op,                              // Operation
                        "api_get_delay_rule_info",       // Operation name
                        nullptr,                         // Clear function
                        (funcPtr) CALL_GET_DELAY_RULE_INFO};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "STR_PI";
    api->in_pack_value = STR_PI;

    api->out_pack_key = "BinBytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
} // plugin_factory
