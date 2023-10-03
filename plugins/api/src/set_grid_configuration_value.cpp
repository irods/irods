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

// clang-format off

#include "irods/set_grid_configuration_value.h"
#include "irods/irods_server_api_call.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/irods_logger.hpp"

#include <string>
#include <string_view>
#include <utility> // for std::pair

// clang-format on

namespace ic = irods::experimental::catalog;

using log_api = irods::experimental::log::api;

namespace
{
    //
    // Function Prototypes
    //

    auto call_set_grid_configuration_value(irods::api_entry*, RsComm*, const GridConfigurationInput*) -> int;

    auto rs_set_grid_configuration_value(RsComm*, const GridConfigurationInput*) -> int;

    auto is_grid_configuration_allowed_to_be_modified(const char* _namespace, const char* _option_name) -> bool;

    //
    // Function Implementations
    //

    auto is_grid_configuration_allowed_to_be_modified(const char* _namespace, const char* _option_name) -> bool
    {
        constexpr auto protected_config_list = std::to_array<std::pair<std::string_view, std::string_view>>(
            {{"database", "schema_version"}, {"delay_server", ""}});

        // If none of the namespace/option name combinations in the protected list match the passed in namespace and
        // option name, it is allowed to be modified and so the function should return true. If any instance returns
        // true, that means a match was found in the list of protected configurations, so we should return false in this
        // function because that means the grid configuration is not allowed to be modified.
        return std::none_of(std::begin(protected_config_list),
                            std::end(protected_config_list),
                            [&_namespace, &_option_name](const auto& _pair) {
                                const auto& [protected_namespace, protected_option_name] = _pair;

                                // If this evaluates to true, this is not a protected option and is allowed to be
                                // modified. Return false because this is not a match.
                                if (protected_namespace != _namespace) {
                                    return false;
                                }

                                // Empty string indicates that modifications for all options in the namespace are
                                // forbidden. If this expression evaluates to true, this a protected option and is not
                                // allowed to be modified.
                                return protected_option_name.empty() || protected_option_name == _option_name;
                            });
    } // is_grid_configuration_allowed_to_be_modified

    auto call_set_grid_configuration_value(irods::api_entry* _api,
                                           RsComm* _comm,
                                           const GridConfigurationInput* _input) -> int
    {
        return _api->call_handler<const GridConfigurationInput*>(_comm, _input);
    }

    auto rs_set_grid_configuration_value(RsComm* _comm, const GridConfigurationInput* _input) -> int
    {
        // Redirect to the catalog service provider so that we can query the database.
        try {
            if (!ic::connected_to_catalog_provider(*_comm)) {
                log_api::trace("Redirecting request to catalog service provider ...");
                auto* host_info = ic::redirect_to_catalog_provider(*_comm);
                const auto ec = rc_set_grid_configuration_value(host_info->conn, _input);

                // Replicate the rError stack here so that any messages the API communicates to the client will
                // persist through the redirect. See #6426 for details.
                replErrorStack(host_info->conn->rError, &_comm->rError);

                return ec;
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            log_api::error(e.what());
            return e.code();
        }

        if (!is_grid_configuration_allowed_to_be_modified(_input->name_space, _input->option_name)) {
            constexpr auto ec = SYS_NOT_ALLOWED;
            addRErrorMsg(&_comm->rError, ec, "Specified grid configuration is not allowed to be modified.");
            return ec;
        }

        return chlSetGridConfigurationValue(_comm,
                                            _input->name_space,
                                            _input->option_name,
                                            _input->option_value);
    }

    using operation = std::function<int(RsComm*, const GridConfigurationInput*)>;
    const operation op = rs_set_grid_configuration_value;
    #define CALL_SET_GRID_CONFIGURATION_VALUE call_set_grid_configuration_value
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const GridConfigurationInput*)>;
    const operation op{};
    #define CALL_SET_GRID_CONFIGURATION_VALUE nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{
        SET_GRID_CONFIGURATION_VALUE_APN,       // API number
        RODS_API_VERSION,                       // API version
        LOCAL_PRIV_USER_AUTH,                   // Client auth
        LOCAL_PRIV_USER_AUTH,                   // Proxy auth
        "GridConfigurationInp_PI", 0,           // In PI / bs flag
        nullptr, 0,                             // Out PI / bs flag
        op,                                     // Operation
        "api_set_grid_configuration_value",     // Operation name
        irods::clearInStruct_noop,              // Clear input function
        irods::clearOutStruct_noop,             // Clear output function
        (funcPtr) CALL_SET_GRID_CONFIGURATION_VALUE
    };
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "GridConfigurationInp_PI";
    api->in_pack_value = GridConfigurationInp_PI;

    return api;
} // plugin_factory
