#include "irods/irods_pack_table.hpp"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/plugins/api/grid_configuration_types.h"
#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"

#include "irods/apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "irods/get_grid_configuration_value.h"
#include "irods/irods_server_api_call.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/irods_logger.hpp"

#include <string>

namespace ic = irods::experimental::catalog;

using log_api = irods::experimental::log::api;

namespace
{
    //
    // Function Prototypes
    //

    auto call_get_grid_configuration_value(irods::api_entry*,
                                           RsComm*,
                                           const GridConfigurationInput*,
                                           GridConfigurationOutput**) -> int;

    auto rs_get_grid_configuration_value(RsComm*,
                                         const GridConfigurationInput*,
                                         GridConfigurationOutput**) -> int;

    //
    // Function Implementations
    //

    auto call_get_grid_configuration_value(irods::api_entry* _api,
                                           RsComm* _comm,
                                           const GridConfigurationInput* _input,
                                           GridConfigurationOutput** _output) -> int
    {
        return _api->call_handler<const GridConfigurationInput*, GridConfigurationOutput**>(_comm, _input, _output);
    }

    auto rs_get_grid_configuration_value(RsComm* _comm,
                                         const GridConfigurationInput* _input,
                                         GridConfigurationOutput** _output) -> int
    {
        // Redirect to the catalog service provider so that we can query the database.
        try {
            if (!ic::connected_to_catalog_provider(*_comm)) {
                log_api::trace("Redirecting request to catalog service provider ...");
                auto* host_info = ic::redirect_to_catalog_provider(*_comm);
                return rc_get_grid_configuration_value(host_info->conn, _input, _output);
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            log_api::error(e.what());
            return e.code();
        }

        *_output = static_cast<GridConfigurationOutput*>(std::malloc(sizeof(GridConfigurationOutput)));
        std::memset(*_output, 0, sizeof(GridConfigurationOutput));

        return chlGetGridConfigurationValue(_comm,
                                            _input->name_space,
                                            _input->option_name,
                                            (*_output)->option_value,
                                            sizeof((*_output)->option_value));
    }

    using operation = std::function<int(RsComm*, const GridConfigurationInput*, GridConfigurationOutput**)>;
    const operation op = rs_get_grid_configuration_value;
    #define CALL_GET_GRID_CONFIGURATION_VALUE call_get_grid_configuration_value
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const GridConfigurationInput*, GridConfigurationOutput**)>;
    const operation op{};
    #define CALL_GET_GRID_CONFIGURATION_VALUE nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{
        GET_GRID_CONFIGURATION_VALUE_APN,       // API number
        RODS_API_VERSION,                       // API version
        LOCAL_PRIV_USER_AUTH,                   // Client auth
        LOCAL_PRIV_USER_AUTH,                   // Proxy auth
        "GridConfigurationInp_PI", 0,           // In PI / bs flag
        "GridConfigurationOut_PI", 0,           // Out PI / bs flag
        op,                                     // Operation
        "api_get_grid_configuration_value",     // Operation name
        irods::clearInStruct_noop,              // Clear input function
        irods::clearOutStruct_noop,             // Clear output function
        (funcPtr) CALL_GET_GRID_CONFIGURATION_VALUE
    };
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "GridConfigurationInp_PI";
    api->in_pack_value = GridConfigurationInp_PI;

    api->out_pack_key = "GridConfigurationOut_PI";
    api->out_pack_value = GridConfigurationOut_PI;

    return api;
} // plugin_factory
