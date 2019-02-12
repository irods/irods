#ifndef IRODS_FILESYSTEM_PATH_TRAITS_HPP
#define IRODS_FILESYSTEM_PATH_TRAITS_HPP

#include <string>
#include <vector>
#include <list>

namespace irods {
namespace experimental {
namespace filesystem {

    class collection_entry;
        
    namespace path_traits
    {
        template <typename>
        constexpr bool is_pathable = false;

        // clang-format off
        template <> constexpr bool is_pathable<char*>             = true;
        template <> constexpr bool is_pathable<const char*>       = true;
        template <> constexpr bool is_pathable<std::string>       = true;
        template <> constexpr bool is_pathable<std::vector<char>> = true;
        template <> constexpr bool is_pathable<std::list<char>>   = true;
        template <> constexpr bool is_pathable<collection_entry>  = true;
        // clang-format on
    } // namespace path_traits

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_PATH_TRAITS_HPP
