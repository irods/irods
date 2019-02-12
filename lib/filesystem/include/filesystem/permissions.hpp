#ifndef IRODS_FILESYSTEM_PERMISSIONS_HPP
#define IRODS_FILESYSTEM_PERMISSIONS_HPP

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

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_PERMISSIONS_HPP
