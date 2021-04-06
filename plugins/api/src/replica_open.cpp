#include "api_plugin_number.h"
#include "dataObjInpOut.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsErrorTable.h"
#include "rodsPackInstruct.h"
#include "rcMisc.h"
#include "client_api_whitelist.hpp"

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
#include "rsDataObjOpen.hpp"
#include "rsDataObjClose.hpp"
#include "rs_get_file_descriptor_info.hpp"
#include "rs_replica_close.hpp"
#include "rodsLog.h"

#include <json.hpp>

#include <string>

namespace
{
    //
    // Function Prototypes
    //

    auto call_replica_open(irods::api_entry*, rsComm_t*, dataObjInp_t*, bytesBuf_t**) -> int;

    auto rs_replica_open(rsComm_t*, dataObjInp_t*, bytesBuf_t**) -> int;

    //
    // Function Implementations
    //

    auto call_replica_open(irods::api_entry* _api, rsComm_t* _comm, dataObjInp_t* _input, bytesBuf_t** _output) -> int
    {
        return _api->call_handler<dataObjInp_t*, bytesBuf_t**>(_comm, _input, _output);
    }

    auto rs_replica_open(rsComm_t* _comm, dataObjInp_t* _input, bytesBuf_t** _output) -> int
    {
        if (!_input || !_output) {
            rodsLog(LOG_ERROR, "Invalid input detected.");
            return SYS_INVALID_INPUT_PARAM;
        }

        rodsLog(LOG_DEBUG, "Opening data object ...");

        const auto fd = rsDataObjOpen(_comm, _input);

        rodsLog(LOG_DEBUG, "Opened L1 descriptor = %d", fd);

        if (fd < 3) {
            rodsLog(LOG_DEBUG, "Could not open replica [error_code=%d]", fd);
            return fd;
        }

        rodsLog(LOG_DEBUG, "Constructing JSON ...");

        const auto json_input = nlohmann::json{{"fd", fd}}.dump();
        char* json_output{};

        rodsLog(LOG_DEBUG, "JSON = %s", json_input.data());
        rodsLog(LOG_DEBUG, "Retrieving L1 descriptor information ...");

        if (const auto ec = rs_get_file_descriptor_info(_comm, json_input.data(), &json_output); ec != 0) {
            rodsLog(LOG_ERROR, "Could not get L1 descriptor information for replica [error_code=%d]", ec);
            rodsLog(LOG_DEBUG, "Closing replica ...");

            // Although the JSON input does not include the "update_catalog" option, the
            // catalog will be updated because the option defaults to "true".
            if (const auto ec0 = rs_replica_close(_comm, json_input.data()); ec0 != 0) {
                rodsLog(LOG_ERROR, "Could not close replica [error_code=%d]", ec0);
            }

            return ec;
        }

        rodsLog(LOG_DEBUG, "L1 Descriptor Info = %s", json_output);
        rodsLog(LOG_DEBUG, "Storing L1 descriptor information in the output bytesBuf_t ...");

        *_output = static_cast<bytesBuf_t*>(std::malloc(sizeof(bytesBuf_t)));
        (*_output)->buf = json_output;
        (*_output)->len = std::strlen(json_output) + 1;

        rodsLog(LOG_DEBUG, "Returning opened L1 descriptor ...");

        return fd;
    }

    using operation = std::function<int(rsComm_t*, dataObjInp_t*, bytesBuf_t**)>;
    const operation op = rs_replica_open;
    #define CALL_REPLICA_OPEN call_replica_open
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, dataObjInp_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_REPLICA_OPEN nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(REPLICA_OPEN_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{REPLICA_OPEN_APN,                // API number
                        RODS_API_VERSION,                // API version
                        NO_USER_AUTH,                    // Client auth
                        NO_USER_AUTH,                    // Proxy auth
                        "DataObjInp_PI", 0,              // In PI / bs flag
                        "BinBytesBuf_PI", 0,             // Out PI / bs flag
                        op,                              // Operation
                        "api_replica_open",              // Operation name
                        clearDataObjInp,                 // Clear function
                        (funcPtr) CALL_REPLICA_OPEN};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "DataObjInp_PI";
    api->in_pack_value = DataObjInp_PI;

    api->out_pack_key = "BinBytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

