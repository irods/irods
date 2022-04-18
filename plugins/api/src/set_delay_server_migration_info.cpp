#include "irods/plugins/api/api_plugin_number.h"
#include "irods/plugins/api/delay_server_migration_types.h"
#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"

#include "irods/apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "irods/set_delay_server_migration_info.h"
#include "irods/irods_server_api_call.hpp"
#include "irods/catalog.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsErrorTable.h"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/irods_query.hpp"

#include <fmt/format.h>
#include <nanodbc/nanodbc.h>

#include <unistd.h> // For HOST_NAME_MAX

#include <cstring>
#include <string>
#include <string_view>

namespace ic = irods::experimental::catalog;

using log_api = irods::experimental::log::api;

namespace
{
    //
    // Function Prototypes
    //

    auto call_set_delay_server_migration_info(irods::api_entry*, RsComm*, const DelayServerMigrationInput*) -> int;

    auto rs_set_delay_server_migration_info(RsComm*, const DelayServerMigrationInput*) -> int;

    auto is_hostname_valid(RsComm&, const std::string_view _hostname) -> bool;

    //
    // Function Implementations
    //

    auto call_set_delay_server_migration_info(irods::api_entry* _api,
                                              RsComm* _comm,
                                              const DelayServerMigrationInput* _input) -> int
    {
        return _api->call_handler<const DelayServerMigrationInput*>(_comm, _input);
    } // call_set_delay_server_migration_info

    auto is_hostname_valid(RsComm& _comm, const std::string_view _hostname) -> bool
    {
        irods::query q{&_comm, fmt::format("select RESC_LOC where RESC_LOC = '{}'", _hostname)};
        return q.size() > 0;
    } // is_hostname_valid

    auto rs_set_delay_server_migration_info(RsComm* _comm, const DelayServerMigrationInput* _input) -> int
    {
        // Redirect to the catalog service provider so that we can query the database.
        try {
            if (!ic::connected_to_catalog_provider(*_comm)) {
                log_api::trace("Redirecting request to catalog service provider ...");
                auto* host_info = ic::redirect_to_catalog_provider(*_comm);
                return rc_set_delay_server_migration_info(host_info->conn, _input);
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            log_api::error(e.what());
            return e.code();
        }

        if (!_input) {
            log_api::error("Invalid input argument: null pointer");
            return SYS_INVALID_INPUT_PARAM;
        }

        if (std::strncmp(_input->leader, _input->successor, sizeof(DelayServerMigrationInput::leader)) == 0) {
            log_api::debug("Hostname for leader and successor are identical. There's no work to be done.");
            return 0;
        }

        const auto ignore_leader = (std::strncmp(_input->leader, KW_DELAY_SERVER_MIGRATION_IGNORE, sizeof(DelayServerMigrationInput::leader)) == 0);
        const auto ignore_successor = (std::strncmp(_input->successor, KW_DELAY_SERVER_MIGRATION_IGNORE, sizeof(DelayServerMigrationInput::successor)) == 0);

        if (ignore_leader && ignore_successor) {
            log_api::debug("Hostname for leader and successor are set to {}. There's no work to be done.",
                           KW_DELAY_SERVER_MIGRATION_IGNORE);
            return 0;
        }

        //
        // Verify that the size of the hostnames do not exceed the size allowed by the OS.
        //

        if (!ignore_leader && std::strlen(_input->leader) > HOST_NAME_MAX) {
            log_api::error("Hostname for leader [{}] exceeds the maximum size [{}] allowed for hostnames.",
                           _input->leader, HOST_NAME_MAX);
            return SYS_INVALID_INPUT_PARAM;
        }

        if (!ignore_successor && std::strlen(_input->successor) > HOST_NAME_MAX) {
            log_api::error("Hostname for successor [{}] exceeds the maximum size [{}] allowed for hostnames.",
                           _input->successor, HOST_NAME_MAX);
            return SYS_INVALID_INPUT_PARAM;
        }
#if 0
        //
        // Verify that the hostnames refer to valid local iRODS servers.
        //

        if (!ignore_leader && !is_hostname_valid(*_comm, _input->leader)) {
            log_api::error("Hostname for leader [{}] does not reference a known iRODS server.", _input->leader);
            return SYS_INVALID_INPUT_PARAM;
        }

        if (!ignore_successor && !is_hostname_valid(*_comm, _input->successor)) {
            log_api::error("Hostname for successor [{}] does not reference a known iRODS server.", _input->successor);
            return SYS_INVALID_INPUT_PARAM;
        }
#endif
        //
        // Update the database!
        //

        try {
            auto [db_instance_name, db_conn] = ic::new_database_connection();

            return ic::execute_transaction(db_conn, [&_input, ignore_leader, ignore_successor](auto& _trans) -> int {
                try {
                    if (!ignore_leader) {
                        nanodbc::statement stmt{_trans.connection()};
                        nanodbc::prepare(stmt, "update R_GRID_CONFIGURATION set option_value = ? "
                                               "where namespace = 'delay_server' and option_name = 'leader'");

                        stmt.bind(0, _input->leader);

                        nanodbc::execute(stmt);
                    }

                    if (!ignore_successor) {
                        nanodbc::statement stmt{_trans.connection()};
                        nanodbc::prepare(stmt, "update R_GRID_CONFIGURATION set option_value = ? "
                                               "where namespace = 'delay_server' and option_name = 'successor'");

                        stmt.bind(0, _input->successor);

                        nanodbc::execute(stmt);
                    }

                    _trans.commit();

                    return 0;
                }
                catch (const nanodbc::database_error& e) {
                    log_api::error("Caught database exception: {}", e.what());
                    return SYS_LIBRARY_ERROR;
                }
            });
        }
        catch (const irods::exception& e) {
            log_api::error("Caught iRODS exception: {}", e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            log_api::error("Caught exception: {}", e.what());
            return SYS_LIBRARY_ERROR;
        }
        catch (...) {
            log_api::error("Caught unknown error");
            return SYS_UNKNOWN_ERROR;
        }
    } // rs_set_delay_server_migration_info

    using operation = std::function<int(RsComm*, const DelayServerMigrationInput*)>;
    const operation op = rs_set_delay_server_migration_info;
    #define CALL_SET_DELAY_SERVER_MIGRATION_INFO call_set_delay_server_migration_info
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const DelayServerMigrationInput*)>;
    const operation op{};
    #define CALL_SET_DELAY_SERVER_MIGRATION_INFO nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{SET_DELAY_SERVER_MIGRATION_INFO_APN,     // API number
                        RODS_API_VERSION,                        // API version
                        LOCAL_PRIV_USER_AUTH,                    // Client auth
                        LOCAL_PRIV_USER_AUTH,                    // Proxy auth
                        "DelayServerMigrationInp_PI", 0,         // In PI / bs flag
                        nullptr, 0,                              // Out PI / bs flag
                        op,                                      // Operation
                        "api_set_delay_server_migration_info",   // Operation name
                        nullptr,                                 // Null clear function
                        (funcPtr) CALL_SET_DELAY_SERVER_MIGRATION_INFO};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "DelayServerMigrationInp_PI";
    api->in_pack_value = DelayServerMigrationInp_PI;

    return api;
} // plugin_factory

