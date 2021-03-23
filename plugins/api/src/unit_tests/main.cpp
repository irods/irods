#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"
#include "apiHandler.hpp"
#include "client_api_whitelist.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

//#include "touch.h"

#include "rcMisc.h"
#include "rodsErrorTable.h"
#include "irods_re_serialization.hpp"
#include "rodsLog.h"

#include "fmt/format.h"
#include "json.hpp"

#include <string>
#include <string_view>

namespace
{
    using json = nlohmann::json;

    //
    // Function Prototypes
    //

    auto rs_unit_test(RsComm* _comm, BytesBuf* _bbuf_input, BytesBuf** _bbuf_output) -> int;

    auto call_unit_test(irods::api_entry* _api,
                        RsComm* _comm,
                        BytesBuf* _bbuf_input,
                        BytesBuf** _bbuf_output) -> int;

    //
    // Function Implementations
    //

    auto parse_json(const BytesBuf* _bbuf) -> json
    {
        if (!_bbuf) {
            THROW(SYS_NULL_INPUT, "Could not parse string (null pointer) into JSON.");
        }

        try {
            const std::string_view json_string(static_cast<const char*>(_bbuf->buf), _bbuf->len);
            return json::parse(json_string);
        }
        catch (const json::exception& e) {
            THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, "Could not parse string into JSON.");
        }
    } // parse_json

    auto rs_unit_test(RsComm* _comm, BytesBuf* _bbuf_input, BytesBuf** _bbuf_output) -> int
    {
        try {
            return 0;
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            addRErrorMsg(&_comm->rError, SYS_UNKNOWN_ERROR, "Cannot process request due to an unexpected error.");
            return SYS_UNKNOWN_ERROR;
        }
    } // rs_unit_test

    auto call_unit_test(irods::api_entry* _api,
                        RsComm* _comm,
                        BytesBuf* _bbuf_input,
                        BytesBuf** _bbuf_output) -> int
    {
        return _api->call_handler<BytesBuf*>(_comm, _bbuf_input, _bbuf_output);
    } // call_unit_test

    using operation_type = std::function<int(RsComm*, BytesBuf*, BytesBuf**)>;
    const operation_type op = rs_unit_test;
    #define CALL_UNIT_TEST call_unit_test
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation_type = std::function<int(RsComm*, BytesBuf*, BytesBuf**)>;
    const operation_type op{};
    #define CALL_UNIT_TEST nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(UNIT_TEST_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{UNIT_TEST_APN,          // API number
                        RODS_API_VERSION,       // API version
                        NO_USER_AUTH,           // Client auth
                        NO_USER_AUTH,           // Proxy auth
                        "BytesBuf_PI", 0,       // In PI / bs flag
                        "BytesBuf_PI", 0,       // Out PI / bs flag
                        op,                     // Operation
                        "api_unit_test",        // Operation name
                        nullptr,                // Clear function
                        (funcPtr) CALL_UNIT_TEST};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

