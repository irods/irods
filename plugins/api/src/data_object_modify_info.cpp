#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/rodsPackInstruct.h"
#include "irods/rcMisc.h"
#include "irods/client_api_whitelist.hpp"

#include "irods/apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "irods/objDesc.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_server_api_call.hpp"
#include "irods/irods_re_serialization.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/scoped_privileged_client.hpp"
#include "irods/key_value_proxy.hpp"

#include <string>
#include <tuple>

namespace
{
    //
    // Function Prototypes
    //

    auto call_data_object_modify_info(irods::api_entry*, rsComm_t*, modDataObjMeta_t*) -> int;
    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;
    auto rs_data_object_modify_info(rsComm_t*, modDataObjMeta_t*) -> int;

    //
    // Function Implementations
    //

    auto call_data_object_modify_info(irods::api_entry* api,
                                      rsComm_t* comm,
                                      modDataObjMeta_t* input) -> int
    {
        return api->call_handler<modDataObjMeta_t*>(comm, input);
    }

    auto is_input_valid(const modDataObjMeta_t* input) -> std::tuple<bool, std::string>
    {
        if (!input) {
            return {false, "Input is null"};
        }

        irods::experimental::key_value_proxy proxy{*input->regParam};

        const auto is_invalid_keyword = [](const auto& handle)
        {
            // To allow modification of a column, comment out the condition
            // referencing that column.
            return handle.key() == CHKSUM_KW ||
                   handle.key() == COLL_ID_KW ||
                   //handle.key() == DATA_COMMENTS_KW ||
                   handle.key() == DATA_CREATE_KW ||
                   //handle.key() == DATA_EXPIRY_KW ||
                   handle.key() == DATA_ID_KW ||
                   handle.key() == DATA_MAP_ID_KW ||
                   handle.key() == DATA_MODE_KW ||
                   //handle.key() == DATA_MODIFY_KW ||
                   handle.key() == DATA_NAME_KW ||
                   handle.key() == DATA_OWNER_KW ||
                   handle.key() == DATA_OWNER_ZONE_KW ||
                   //handle.key() == DATA_RESC_GROUP_NAME_KW || // Not defined.
                   handle.key() == DATA_SIZE_KW ||
                   //handle.key() == DATA_TYPE_KW ||
                   handle.key() == FILE_PATH_KW ||
                   handle.key() == REPL_NUM_KW ||
                   handle.key() == REPL_STATUS_KW ||
                   handle.key() == RESC_HIER_STR_KW ||
                   handle.key() == RESC_ID_KW ||
                   handle.key() == RESC_NAME_KW ||
                   handle.key() == STATUS_STRING_KW ||
                   handle.key() == VERSION_KW;
        };

        if (std::any_of(std::begin(proxy), std::end(proxy), is_invalid_keyword)) {
            return {false, "Invalid keyword found"};
        }

        return {true, ""};
    }

    auto rs_data_object_modify_info(rsComm_t* comm, modDataObjMeta_t* input) -> int
    {
        if (auto [valid, msg] = is_input_valid(input); !valid) {
            rodsLog(LOG_ERROR, msg.data());
            return USER_BAD_KEYWORD_ERR;
        }

        irods::experimental::scoped_privileged_client spc{*comm};
        return rsModDataObjMeta(comm, input);
    }

    using operation = std::function<int(rsComm_t*, modDataObjMeta_t*)>;
    const operation op = rs_data_object_modify_info;
    #define CALL_DATA_OBJECT_MODIFY_INFO call_data_object_modify_info
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

#include "irods/modDataObjMeta.h"

namespace
{
    using operation = std::function<int(rsComm_t*, modDataObjMeta_t*)>;
    const operation op{};
    #define CALL_DATA_OBJECT_MODIFY_INFO nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(DATA_OBJECT_MODIFY_INFO_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{DATA_OBJECT_MODIFY_INFO_APN,     // API number
                        RODS_API_VERSION,                // API version
                        NO_USER_AUTH,                    // Client auth
                        NO_USER_AUTH,                    // Proxy auth
                        "ModDataObjMeta_PI", 0,          // In PI / bs flag
                        nullptr, 0,                      // Out PI / bs flag
                        op,                              // Operation
                        "api_data_object_modify_info",   // Operation name
                        clearModDataObjMetaInp,          // Null clear function
                        (funcPtr) CALL_DATA_OBJECT_MODIFY_INFO};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "ModDataObjMeta_PI";
    api->in_pack_value = ModDataObjMeta_PI;

    return api;
}

