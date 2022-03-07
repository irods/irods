#ifndef IRODS_ICOMMANDS_UTILITY_HPP
#define IRODS_ICOMMANDS_UTILITY_HPP

#include <irods/rodsDef.h>

#include <cstdlib>
#include <string_view>
#include <iostream>

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
} // namespace utils

#endif // IRODS_ICOMMANDS_UTILITY_HPP

