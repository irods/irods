#ifndef IRODS_ICOMMANDS_UTILITY_HPP
#define IRODS_ICOMMANDS_UTILITY_HPP

#include <irods/authentication_plugin_framework.hpp>
#include <irods/getRodsEnv.h>
#include <irods/rodsDef.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

#include <nlohmann/json.hpp>

namespace utils
{
    inline auto set_ips_display_name(const std::string_view _display_name) -> void
    {
        // Setting this environment variable is required so that "ips" can display
        // the command name alongside the connection information.
        if (setenv(SP_OPTION, _display_name.data(), /* overwrite */ 1)) {
            std::cout << "Warning: Could not set environment variable [spOption] for ips.";
        }
    }

    inline auto authenticate_client(RcComm* _comm, const RodsEnvironment& _env) -> int
    {
        namespace ia = irods::authentication;
        const auto ctx = nlohmann::json{{ia::scheme_name, _env.rodsAuthScheme}};
        return ia::authenticate_client(*_comm, ctx);
    } // authenticate_client

    inline auto option_specified(std::string_view _option, int argc, char** argv) -> bool
    {
        for (int arg = 0; arg < argc; ++arg) {
            if (!argv[arg]) {
                continue;
            }

            if (_option == argv[arg]) {
                // parseCmdLineOpt is EVIL and requires this. Please don't ask why.
                argv[arg] = "-Z";
                return true;
            }
        }

        return false;
    } // option_specified
} // namespace utils

#endif // IRODS_ICOMMANDS_UTILITY_HPP

