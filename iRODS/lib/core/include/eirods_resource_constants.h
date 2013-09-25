


#ifndef ___EIRODS_RESOURCE_CONSTANTS_H__
#define ___EIRODS_RESOURCE_CONSTANTS_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {

    // =-=-=-=-=-=-=-
    /// @brief delimiter used for parsing resource context strings
    const std::string RESOURCE_DELIMITER(";");

    // =-=-=-=-=-=-=-
    /// @brief key for compound resource cache staging policy
    const std::string RESOURCE_STAGE_TO_CACHE_POLICY("stage_to_cache_policy");

    // =-=-=-=-=-=-=-
    /// @brief value for compound resource cache staging policy : 
    ///        prefer the copy in the Archive
    const std::string RESOURCE_STAGE_PREFER_ARCHIVE("prefer_archive");

    // =-=-=-=-=-=-=-
    /// @brief value for compound resource cache staging policy : 
    ///        prefer the copy in the Cache
    const std::string RESOURCE_STAGE_PREFER_CACHE("prefer_cache");

    // =-=-=-=-=-=-=-
    /// @brief value for a native file system type - this should be
    ///        modified for different platforms ( windows vs linux, etc. )
    const std::string RESOURCE_TYPE_NATIVE( "unix file system" );

    // =-=-=-=-=-=-=-
    /// @brief value for a bundle file system class 
    const std::string RESOURCE_CLASS_BUNDLE( "bundle" );

    // =-=-=-=-=-=-=-
    /// @brief value for a cache file system class 
    const std::string RESOURCE_CLASS_CACHE( "cache" );

    // =-=-=-=-=-=-=-
    /// @brief constants for resource operation indexing
    const std::string RESOURCE_OP_CREATE( "eirods_resource_create" );
    const std::string RESOURCE_OP_OPEN( "eirods_resource_open" );
    const std::string RESOURCE_OP_READ( "eirods_resource_read" );
    const std::string RESOURCE_OP_WRITE( "eirods_resource_write" );
    const std::string RESOURCE_OP_CLOSE( "eirods_resource_close" );
    const std::string RESOURCE_OP_UNLINK( "eirods_resource_unlink" );
    const std::string RESOURCE_OP_STAT( "eirods_resource_stat" );
    const std::string RESOURCE_OP_FSTAT( "eirods_resource_fstat" );
    const std::string RESOURCE_OP_FSYNC( "eirods_resource_fsync" );
    const std::string RESOURCE_OP_MKDIR( "eirods_resource_mkdir" );
    const std::string RESOURCE_OP_CHMOD( "eirods_resource_chmod" );
    const std::string RESOURCE_OP_OPENDIR( "eirods_resource_opendir" );
    const std::string RESOURCE_OP_READDIR( "eirods_resource_readdir" );
    const std::string RESOURCE_OP_STAGE( "eirods_resource_stage" );
    const std::string RESOURCE_OP_RENAME( "eirods_resource_rename" );
    const std::string RESOURCE_OP_FREESPACE( "eirods_resource_freespace" );
    const std::string RESOURCE_OP_LSEEK( "eirods_resource_lseek" );
    const std::string RESOURCE_OP_RMDIR( "eirods_resource_rmdir" );
    const std::string RESOURCE_OP_CLOSEDIR( "eirods_resource_closedir" );
    const std::string RESOURCE_OP_TRUNCATE( "eirods_resource_truncate" );
    const std::string RESOURCE_OP_STAGETOCACHE( "eirods_resource_stagetocache" );
    const std::string RESOURCE_OP_SYNCTOARCH( "eirods_resource_synctoarch" );
    
    const std::string RESOURCE_OP_REGISTERED( "eirods_resource_registered" );
    const std::string RESOURCE_OP_UNREGISTERED( "eirods_resource_unregistered" );
    const std::string RESOURCE_OP_MODIFIED( "eirods_resource_modified" );
    const std::string RESOURCE_OP_RESOLVE_RESC_HIER( "eirods_resource_resolve_resc_hier" );
    const std::string RESOURCE_OP_REBALANCE( "eirods_resource_rebalance" );

    // =-=-=-=-=-=-=-
    /// @brief constants for icat resource properties
    const std::string RESOURCE_HOST( "eirods_resource_property_host" );
    const std::string RESOURCE_ID( "eirods_resource_property_id" );
    const std::string RESOURCE_FREESPACE( "eirods_resource_property_freespace" );
    const std::string RESOURCE_QUOTA( "eirods_resource_property_quota" );
    const std::string RESOURCE_ZONE( "eirods_resource_property_zone" );
    const std::string RESOURCE_NAME( "eirods_resource_property_name" );
    const std::string RESOURCE_LOCATION( "eirods_resource_property_location" );
    const std::string RESOURCE_TYPE( "eirods_resource_property_type" );
    const std::string RESOURCE_CLASS( "eirods_resource_property_class" );
    const std::string RESOURCE_PATH( "eirods_resource_property_path" );
    const std::string RESOURCE_INFO( "eirods_resource_property_info" );
    const std::string RESOURCE_COMMENTS( "eirods_resource_property_comments" );
    const std::string RESOURCE_CREATE_TS( "eirods_resource_property_create_ts" );
    const std::string RESOURCE_MODIFY_TS( "eirods_resource_property_modify_ts" );
    const std::string RESOURCE_STATUS( "eirods_resource_property_status" );
    const std::string RESOURCE_PARENT( "eirods_resource_property_parent" );
    const std::string RESOURCE_CHILDREN( "eirods_resource_property_children" );
    const std::string RESOURCE_CONTEXT( "eirods_resource_property_context" );
    const std::string RESOURCE_CHECK_PATH_PERM( "eirods_resource_property_check_path_perm" );
    const std::string RESOURCE_CREATE_PATH( "eirods_resource_property_create_path" );


}; // namespace eirods

#endif // ___EIRODS_RESOURCE_CONSTANTS_H__



