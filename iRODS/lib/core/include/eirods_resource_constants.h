


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

}; // namespace eirods

#endif // ___EIRODS_RESOURCE_CONSTANTS_H__



