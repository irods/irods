#ifndef IRODS_FILESYSTEM_PERMISSIONS_HPP
#define IRODS_FILESYSTEM_PERMISSIONS_HPP

#include <string>

namespace irods::experimental::filesystem
{
    enum class perms
    {
        null,
        read,
        write,
        own
    };

    struct entity_permission
    {
        std::string name;
        std::string zone;
        perms prms;
        std::string type;
    };
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_PERMISSIONS_HPP
