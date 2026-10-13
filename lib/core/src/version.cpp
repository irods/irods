#include "irods/rodsDef.h"
#include "irods/rodsLog.h"
#include "irods/version.hpp"

#include <fmt/format.h>

#include <charconv>
#include <regex>
#include <system_error>

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
                std::uint16_t major; // NOLINT(cppcoreguidelines-init-variables)
                const std::string major_str = m[1].str();
                const auto* major_b = major_str.c_str();
                const auto* major_e = major_b + major_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::from_chars_result fc_res = std::from_chars(major_b, major_e, major);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                std::uint16_t minor; // NOLINT(cppcoreguidelines-init-variables)
                const std::string minor_str = m[2].str();
                const auto* minor_b = minor_str.c_str();
                const auto* minor_e = minor_b + minor_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                fc_res = std::from_chars(minor_b, minor_e, minor);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                std::uint16_t patch; // NOLINT(cppcoreguidelines-init-variables)
                const std::string patch_str = m[2].str();
                const auto* patch_b = patch_str.c_str();
                const auto* patch_e = patch_b + patch_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                fc_res = std::from_chars(patch_b, patch_e, patch);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                return irods::version{major, minor, patch};
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
