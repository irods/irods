#ifndef IRODS_ICOMMANDS_UTILITY_HPP
#define IRODS_ICOMMANDS_UTILITY_HPP

#include <irods/authentication_plugin_framework.hpp>
#include <irods/getRodsEnv.h>
#include <irods/irods_version.h>
#include <irods/rodsDef.h>
#include <irods/version.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

namespace utils
{
    inline auto set_ips_display_name(const std::string_view _display_name) -> void
    {
        // Setting this environment variable is required so that "ips" can display
        // the command name alongside the connection information.
        if (setenv(SP_OPTION, _display_name.data(), /* overwrite */ 1)) {
            std::cout << "Warning: Could not set environment variable [spOption] for ips.\n";
        }
    }

    inline auto authenticate_client(RcComm* _comm, const RodsEnvironment& _env) -> int
    {
        namespace ia = irods::authentication;
        const auto ctx = nlohmann::json{{ia::scheme_name, _env.rodsAuthScheme}};
        return ia::authenticate_client(*_comm, ctx);
    } // authenticate_client

    inline auto option_specified(std::string_view _option, int _argc, char** _argv) -> bool
    {
        for (int arg = 0; arg < _argc; ++arg) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (!_argv[arg]) {
                continue;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (_option == _argv[arg]) {
                // parseCmdLineOpt is EVIL and requires this. Please don't ask why.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                _argv[arg] = "-Z";
                return true;
            }
        }

        return false;
    } // option_specified

    // Prints a warning to stderr if the major version number of the iCommands does
    // not match the major version number of the connected server.
    //
    // Prints a warning to stderr if the version of the server cannot be determined.
    // It is recommended that the user verify they are connecting to a trusted server
    // before proceeding.
    //
    // Setting the following environment variable to 0 causes the warning message to
    // be suppressed.
    //
    //     ICOMMANDS_DETECT_SERVER_VERSION_MISMATCH
    //
    inline auto warn_if_connected_to_potentially_incompatible_server(const RcComm& _comm) -> void
    {
        // If not defined or explicitly set to 0, return immediately.
        const char* env_var = std::getenv("ICOMMANDS_DETECT_SERVER_VERSION_MISMATCH");
        if (env_var != nullptr && std::strncmp(env_var, "0", 1) == 0) {
            return;
        }

        const auto server_version = irods::to_version(_comm.svrVersion->relVersion);
        if (!server_version) {
            fmt::print(stderr,
                       "Warning: Could not determine server version. Make sure you are connected to a trusted iRODS "
                       "server.\n\n");
        }
        else if (server_version->major != IRODS_VERSION_MAJOR) {
            fmt::print(stderr,
                       "Warning: Major version number mismatch detected between iCommands and server.\n\n"
                       "   iCommands   : {}\n"
                       "   iRODS server: {}\n\n",
                       IRODS_VERSION_MAJOR,
                       server_version->major);
        }
    } // warn_if_connected_to_potentially_incompatible_server
} // namespace utils

#endif // IRODS_ICOMMANDS_UTILITY_HPP
