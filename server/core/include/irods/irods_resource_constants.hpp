#ifndef __IRODS_RESOURCE_CONSTANTS_HPP__
#define __IRODS_RESOURCE_CONSTANTS_HPP__

#include <string>

namespace irods
{
    // =-=-=-=-=-=-=-
    /// @brief delimiter used for parsing resource context strings
    extern const std::string RESOURCE_DELIMITER;

    // =-=-=-=-=-=-=-
    /// @brief key for the replicating resource node which defines the
    ///        maximum number of data objects to rebalance per iteration
    ///        (defaults to 500)
    ///        used in the "resource_rebalance" operation below
    extern const std::string REPL_LIMIT_KEY;

    // =-=-=-=-=-=-=-
    /// @brief key for compound resource cache staging policy
    extern const std::string RESOURCE_STAGE_TO_CACHE_POLICY;

    // =-=-=-=-=-=-=-
    /// @brief value for compound resource cache staging policy :
    ///        prefer the copy in the Archive
    extern const std::string RESOURCE_STAGE_PREFER_ARCHIVE;

    // =-=-=-=-=-=-=-
    /// @brief value for compound resource cache staging policy :
    ///        prefer the copy in the Cache
    extern const std::string RESOURCE_STAGE_PREFER_CACHE;

    // =-=-=-=-=-=-=-
    /// @brief value for a native file system type - this should be
    ///        modified for different platforms ( windows vs linux, etc. )
    extern const std::string RESOURCE_TYPE_NATIVE;

    // =-=-=-=-=-=-=-
    /// @brief value for a bundle file system class
    extern const std::string RESOURCE_CLASS_BUNDLE;

    // =-=-=-=-=-=-=-
    /// @brief value for a cache file system class
    extern const std::string RESOURCE_CLASS_CACHE;

    // =-=-=-=-=-=-=-
    /// @brief constants for resource operation indexing
    extern const std::string RESOURCE_OP_CREATE;
    extern const std::string RESOURCE_OP_OPEN;
    extern const std::string RESOURCE_OP_READ;
    extern const std::string RESOURCE_OP_WRITE;
    extern const std::string RESOURCE_OP_CLOSE;
    extern const std::string RESOURCE_OP_UNLINK;
    extern const std::string RESOURCE_OP_STAT;
    extern const std::string RESOURCE_OP_FSTAT;
    extern const std::string RESOURCE_OP_FSYNC;
    extern const std::string RESOURCE_OP_MKDIR;
    extern const std::string RESOURCE_OP_CHMOD;
    extern const std::string RESOURCE_OP_OPENDIR;
    extern const std::string RESOURCE_OP_READDIR;
    extern const std::string RESOURCE_OP_RENAME;
    extern const std::string RESOURCE_OP_FREESPACE;
    extern const std::string RESOURCE_OP_LSEEK;
    extern const std::string RESOURCE_OP_RMDIR;
    extern const std::string RESOURCE_OP_CLOSEDIR;
    extern const std::string RESOURCE_OP_TRUNCATE;
    extern const std::string RESOURCE_OP_STAGETOCACHE;
    extern const std::string RESOURCE_OP_SYNCTOARCH;
    extern const std::string RESOURCE_OP_REGISTERED;
    extern const std::string RESOURCE_OP_UNREGISTERED;
    extern const std::string RESOURCE_OP_MODIFIED;
    extern const std::string RESOURCE_OP_RESOLVE_RESC_HIER;
    extern const std::string RESOURCE_OP_REBALANCE;
    extern const std::string RESOURCE_OP_NOTIFY;

    // =-=-=-=-=-=-=-
    /// @brief constants for icat resource properties
    extern const std::string RESOURCE_HOST;
    extern const std::string RESOURCE_ID;
    extern const std::string RESOURCE_FREESPACE;
    extern const std::string RESOURCE_QUOTA;
    extern const std::string RESOURCE_ZONE;
    extern const std::string RESOURCE_NAME;
    extern const std::string RESOURCE_LOCATION;
    extern const std::string RESOURCE_TYPE;
    extern const std::string RESOURCE_CLASS;
    extern const std::string RESOURCE_PATH;
    extern const std::string RESOURCE_INFO;
    extern const std::string RESOURCE_COMMENTS;
    extern const std::string RESOURCE_CREATE_TS;
    extern const std::string RESOURCE_MODIFY_TS;
    extern const std::string RESOURCE_STATUS;
    extern const std::string RESOURCE_PARENT;
    extern const std::string RESOURCE_PARENT_CONTEXT;
    extern const std::string RESOURCE_CHILDREN;
    extern const std::string RESOURCE_CONTEXT;
    extern const std::string RESOURCE_CHECK_PATH_PERM;
    extern const std::string RESOURCE_CREATE_PATH;
    extern const std::string RESOURCE_QUOTA_OVERRUN;
    extern const std::string RESOURCE_SKIP_VAULT_PATH_CHECK_ON_UNLINK;
} // namespace irods

#endif // __IRODS_RESOURCE_CONSTANTS_HPP__

