#ifndef IRODS_FILESYSTEM_DETAIL_HPP
#define IRODS_FILESYSTEM_DETAIL_HPP

#include "filesystem/path.hpp"
#include "filesystem/filesystem_error.hpp"

#include "rodsDef.h"

#include <cstring>

namespace irods {
namespace experimental {
namespace filesystem {
namespace detail {

    inline void throw_if_path_length_exceeds_limit(const irods::experimental::filesystem::path& _p)
    {
        if (std::strlen(_p.c_str()) > MAX_NAME_LEN) {
            throw irods::experimental::filesystem::filesystem_error{"path length cannot exceed max path size"};
        }
    }

    inline auto is_separator(path::value_type _c) noexcept -> bool
    {
        return path::separator == _c;
    }

} // namespace detail
} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_DETAIL_HPP
