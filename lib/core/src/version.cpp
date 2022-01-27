#include "rodsDef.h"
#include "rodsLog.h"
#include "version.hpp"

#include <fmt/format.h>

#include <regex>

extern int ProcessType;

namespace irods
{
    auto to_version(const std::string& _version) -> std::optional<irods::version>
    {
        if (_version.empty()) {
            return std::nullopt;
        }

        // Expects _version to be formatted like "rodsX.Y.Z" where X, Y, and Z
        // can represent any integer in the range of std::uint16_t.
        static const std::regex pattern{R"_(\s*rods(\d+)\.(\d+)\.(\d+)\s*)_"};

        if (std::smatch m; std::regex_match(_version, m, pattern)) {
            // At this point, it is guaranteed that "m" will contain the following:
            //
            //   m = [<_version>, <major>, <minor>, <patch>]
            //
            // Where <_version> represents the string that was searched.
            if (m.size() == 4) {
                return irods::version{
                    static_cast<std::uint16_t>(std::stoi(m[1].str())), // Major
                    static_cast<std::uint16_t>(std::stoi(m[2].str())), // Minor
                    static_cast<std::uint16_t>(std::stoi(m[3].str()))  // Patch
                };
            }
            else if (CLIENT_PT != ProcessType) {
                rodsLog(LOG_NOTICE,
                        "Could not extract major, minor, and patch information from string [_version=%s].",
                        _version.data());
            }
        }

        return std::nullopt;
    } // to_version
} // namespace irods
