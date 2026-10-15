#include "irods/rodsDef.h"
#include "irods/rodsLog.h"
#include "irods/version.hpp"

#include <fmt/format.h>

#include <charconv>
#include <cstdint>
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
                std::string v_str = m[1].str();
                const auto* v_b = v_str.c_str();
                const auto* v_e = v_b + v_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::from_chars_result fc_res = std::from_chars(v_b, v_e, major);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                std::uint16_t minor; // NOLINT(cppcoreguidelines-init-variables)
                v_str = m[2].str();
                v_b = v_str.c_str();
                v_e = v_b + v_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                fc_res = std::from_chars(v_b, v_e, minor);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                std::uint16_t patch; // NOLINT(cppcoreguidelines-init-variables)
                v_str = m[2].str();
                v_b = v_str.c_str();
                v_e = v_b + v_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                fc_res = std::from_chars(v_b, v_e, patch);
                if (std::errc{} != fc_res.ec) {
                    throw std::system_error(std::make_error_code(fc_res.ec));
                }

                return irods::version{.major = major, .minor = minor, .patch = patch};
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
