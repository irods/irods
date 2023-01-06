#ifndef IRODS_FILESYSTEM_PATH_UTILITIES_HPP
#define IRODS_FILESYSTEM_PATH_UTILITIES_HPP

#include "irods/filesystem/path.hpp"
#include "irods/filesystem/filesystem_error.hpp"

#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/system_error.hpp"

#include <cstring>

namespace irods::experimental::filesystem
{
    /// Returns whether the path element \p _c represents the preferred path separator.
    ///
    /// \since 4.2.12
    inline auto is_separator(path::value_type _c) noexcept -> bool
    {
        return path::preferred_separator == _c;
    }

    /// Throws a filesystem_error if the length of path \p _p exceeds the maximum length
    /// allowed for a logical path.
    ///
    /// \since 4.2.12
    inline auto throw_if_path_length_exceeds_limit(const path& _p) -> void
    {
        if (strnlen(_p.c_str(), MAX_NAME_LEN) == MAX_NAME_LEN) {
            throw filesystem_error{
                "path length cannot exceed max path size", _p, make_error_code(USER_PATH_EXCEEDS_MAX)};
        }
    }

    /// Throws a filesystem_error if the path, \p _p, is empty.
    ///
    /// \since 4.2.12
    inline auto throw_if_path_is_empty(const path& _p) -> void
    {
        if (_p.empty()) {
            throw filesystem_error{"empty path", _p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }
    }
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_PATH_UTILITIES_HPP
