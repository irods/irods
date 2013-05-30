


#ifndef ___EIRODS_RESOURCE_CONSTANTS_H__
#define ___EIRODS_RESOURCE_CONSTANTS_H__

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
    const std::string RESOURCE_OP_CREATE( "create" );
    const std::string RESOURCE_OP_OPEN( "open" );
    const std::string RESOURCE_OP_READ( "read" );
    const std::string RESOURCE_OP_WRITE( "write" );
    const std::string RESOURCE_OP_CLOSE( "close" );
    const std::string RESOURCE_OP_UNLINK( "unlink" );
    const std::string RESOURCE_OP_STAT( "stat" );
    const std::string RESOURCE_OP_FSTAT( "fstat" );
    const std::string RESOURCE_OP_FSYNC( "fsync" );
    const std::string RESOURCE_OP_MKDIR( "mkdir" );
    const std::string RESOURCE_OP_CHMOD( "chmod" );
    const std::string RESOURCE_OP_OPENDIR( "opendir" );
    const std::string RESOURCE_OP_READDIR( "readdir" );
    const std::string RESOURCE_OP_STAGE( "stage" );
    const std::string RESOURCE_OP_RENAME( "rename" );
    const std::string RESOURCE_OP_FREESPACE( "freespace" );
    const std::string RESOURCE_OP_LSEEK( "lseek" );
    const std::string RESOURCE_OP_RMDIR( "rmdir" );
    const std::string RESOURCE_OP_CLOSEDIR( "closedir" );
    const std::string RESOURCE_OP_TRUNCATE( "truncate" );
    const std::string RESOURCE_OP_STAGETOCACHE( "stagetocache" );
    const std::string RESOURCE_OP_SYNCTOARCH( "synctoarch" );
    
    const std::string RESOURCE_OP_REGISTERED( "registered" );
    const std::string RESOURCE_OP_UNREGISTERED( "unregistered" );
    const std::string RESOURCE_OP_MODIFIED( "modified" );
    const std::string RESOURCE_OP_RESOLVE_RESC_HIER( "resolve_resc_hier" );

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



