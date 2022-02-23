#include "irods/irods_resource_constants.hpp"

namespace irods
{
    const std::string RESOURCE_DELIMITER( ";" );

    const std::string REPL_LIMIT_KEY( "replication_rebalance_limit" );

    const std::string RESOURCE_STAGE_TO_CACHE_POLICY( "compound_resource_cache_refresh_policy" );
    const std::string RESOURCE_STAGE_PREFER_ARCHIVE( "always" );
    const std::string RESOURCE_STAGE_PREFER_CACHE( "when_necessary" );

    const std::string RESOURCE_TYPE_NATIVE( "unixfilesystem" );

    const std::string RESOURCE_CLASS_BUNDLE( "bundle" );
    const std::string RESOURCE_CLASS_CACHE( "cache" );

    const std::string RESOURCE_OP_CREATE( "resource_create" );
    const std::string RESOURCE_OP_OPEN( "resource_open" );
    const std::string RESOURCE_OP_READ( "resource_read" );
    const std::string RESOURCE_OP_WRITE( "resource_write" );
    const std::string RESOURCE_OP_CLOSE( "resource_close" );
    const std::string RESOURCE_OP_UNLINK( "resource_unlink" );
    const std::string RESOURCE_OP_STAT( "resource_stat" );
    const std::string RESOURCE_OP_FSTAT( "resource_fstat" );
    const std::string RESOURCE_OP_FSYNC( "resource_fsync" );
    const std::string RESOURCE_OP_MKDIR( "resource_mkdir" );
    const std::string RESOURCE_OP_CHMOD( "resource_chmod" );
    const std::string RESOURCE_OP_OPENDIR( "resource_opendir" );
    const std::string RESOURCE_OP_READDIR( "resource_readdir" );
    const std::string RESOURCE_OP_RENAME( "resource_rename" );
    const std::string RESOURCE_OP_FREESPACE( "resource_freespace" );
    const std::string RESOURCE_OP_LSEEK( "resource_lseek" );
    const std::string RESOURCE_OP_RMDIR( "resource_rmdir" );
    const std::string RESOURCE_OP_CLOSEDIR( "resource_closedir" );
    const std::string RESOURCE_OP_TRUNCATE( "resource_truncate" );
    const std::string RESOURCE_OP_STAGETOCACHE( "resource_stagetocache" );
    const std::string RESOURCE_OP_SYNCTOARCH( "resource_synctoarch" );
    const std::string RESOURCE_OP_REGISTERED( "resource_registered" );
    const std::string RESOURCE_OP_UNREGISTERED( "resource_unregistered" );
    const std::string RESOURCE_OP_MODIFIED( "resource_modified" );
    const std::string RESOURCE_OP_RESOLVE_RESC_HIER( "resource_resolve_hierarchy" );
    const std::string RESOURCE_OP_REBALANCE( "resource_rebalance" );
    const std::string RESOURCE_OP_NOTIFY( "resource_notify" );

    const std::string RESOURCE_HOST( "resource_property_host" );
    const std::string RESOURCE_ID( "resource_property_id" );
    const std::string RESOURCE_FREESPACE( "resource_property_freespace" );
    const std::string RESOURCE_QUOTA( "resource_property_quota" );
    const std::string RESOURCE_ZONE( "resource_property_zone" );
    const std::string RESOURCE_NAME( "resource_property_name" );
    const std::string RESOURCE_LOCATION( "resource_property_location" );
    const std::string RESOURCE_TYPE( "resource_property_type" );
    const std::string RESOURCE_CLASS( "resource_property_class" );
    const std::string RESOURCE_PATH( "resource_property_path" );
    const std::string RESOURCE_INFO( "resource_property_info" );
    const std::string RESOURCE_COMMENTS( "resource_property_comments" );
    const std::string RESOURCE_CREATE_TS( "resource_property_create_ts" );
    const std::string RESOURCE_MODIFY_TS( "resource_property_modify_ts" );
    const std::string RESOURCE_STATUS( "resource_property_status" );
    const std::string RESOURCE_PARENT( "resource_property_parent" );
    const std::string RESOURCE_PARENT_CONTEXT( "resource_property_parent_context" );
    const std::string RESOURCE_CHILDREN( "resource_property_children" );
    const std::string RESOURCE_CONTEXT( "resource_property_context" );
    const std::string RESOURCE_CHECK_PATH_PERM( "resource_property_check_path_perm" );
    const std::string RESOURCE_CREATE_PATH( "resource_property_create_path" );
    const std::string RESOURCE_QUOTA_OVERRUN( "resource_property_quota_overrun" );
    const std::string RESOURCE_SKIP_VAULT_PATH_CHECK_ON_UNLINK( "resource_skip_vault_path_check_on_unlink" );
} // namespace irods

