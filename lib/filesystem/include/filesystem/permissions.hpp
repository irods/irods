#ifndef IRODS_FILESYSTEM_PERMISSIONS_HPP
#define IRODS_FILESYSTEM_PERMISSIONS_HPP

#include <string>

namespace irods {
namespace experimental {
namespace filesystem {

    enum class perms
    {
        null,
        read,
        write,
        own,
        inherit,
        noinherit
    };

    struct entity_permission
    {
        std::string name;
        perms prms;
    };

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_PERMISSIONS_HPP
