#ifndef IRODS_FILESYSTEM_PERMISSIONS_HPP
#define IRODS_FILESYSTEM_PERMISSIONS_HPP

namespace irods::experimental::filesystem
{
    enum class perms
    {
        null,
        read,
        write,
        own,
        inherit,
        noinherit
    };
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_PERMISSIONS_HPP
