#ifndef IRODS_FILESYSTEM_PERMISSIONS_HPP
#define IRODS_FILESYSTEM_PERMISSIONS_HPP

#include <string>

namespace irods::experimental::filesystem
{
    enum class perms
    {
        null,
        read_metadata,
        read_object,
        read,
        create_metadata,
        modify_metadata,
        delete_metadata,
        create_object,
        modify_object,
        write,
        delete_object,
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
