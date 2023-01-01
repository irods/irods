#ifndef IRODS_FILESYSTEM_DETAIL_HPP
#define IRODS_FILESYSTEM_DETAIL_HPP

#include "filesystem/path.hpp"
#include "filesystem/filesystem_error.hpp"

#include "rodsDef.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <cstring>
#include <system_error>

namespace irods::experimental::filesystem::detail
{
    struct irods_category : std::error_category {
        virtual auto message(int condition) -> std::string const override {
            char *subName{};
            std::string s{ rodsErrorName(condition, &subName) };
            if (subName && *subName) {
                s += (std::string{" ("} + subName + ")");
            }
            free(subName);
            return s;
        }
    };

    inline auto make_error_code(int _ec) noexcept -> std::error_code
    {
        return {_ec, irods_category()};
    }

    inline auto is_separator(path::value_type _c) noexcept -> bool
    {
        return path::preferred_separator == _c;
    }

    inline auto throw_if_path_length_exceeds_limit(const irods::experimental::filesystem::path& _p) -> void
    {
        if (std::strlen(_p.c_str()) > MAX_NAME_LEN) {
            throw irods::experimental::filesystem::filesystem_error{"path length cannot exceed max path size", _p,
                                                                    make_error_code(USER_PATH_EXCEEDS_MAX)};
        }
    }

    inline auto throw_if_path_is_empty(const irods::experimental::filesystem::path& _p) -> void
    {
        if (_p.empty()) {
            throw irods::experimental::filesystem::filesystem_error{"empty path", _p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }
    }
} // namespace irods::experimental::filesystem::detail

#endif // IRODS_FILESYSTEM_DETAIL_HPP
