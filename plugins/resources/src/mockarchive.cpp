// =-=-=-=-=-=-=-
// irods includes
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_physical_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// =-=-=-=-=-=-=-
// system includes
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <openssl/md5.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/param.h>
#include <sys/file.h>
#include <unistd.h>
#endif

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif


// =-=-=-=-=-=-=-
// 2. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update
//       the file object's physical path with the full path

/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
irods::error mock_archive_generate_full_path(
    irods::plugin_property_map& _prop_map,
    const std::string&          _phy_path,
    std::string&                _ret_string)
{
    // TODO - getting vault path by property will not likely work for coordinating nodes
    std::string vault_path;
    if (const auto err = _prop_map.get<std::string>(irods::RESOURCE_PATH, vault_path); !err.ok()) {
        return PASSMSG("Resource has no vault path.", err);
    }

    if ( _phy_path.compare( 0, 1, "/" ) != 0 &&
            _phy_path.compare( 0, vault_path.size(), vault_path ) != 0 ) {
        _ret_string  = vault_path;
        _ret_string += "/";
        _ret_string += _phy_path;
    }
    else {
        // The physical path already contains the vault path
        _ret_string = _phy_path;
    }

    return SUCCESS();
} // mock_archive_generate_full_path

/// @brief update the physical path in the file object
irods::error mock_archive_check_path(irods::plugin_context& _ctx)
{
    try {
        irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );

        // NOTE: Must do this for all storage resources
        std::string full_path;
        if (const auto err = mock_archive_generate_full_path(_ctx.prop_map(), data_obj->physical_path(), full_path); !err.ok()) {
            return PASSMSG("Failed generating full path for object.", err);
        }

        data_obj->physical_path( full_path );

        return SUCCESS();
    }
    catch (const std::bad_cast&) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "failed to cast fco to data_object");
    }
} // mock_archive_check_path

/// @brief Checks the basic operation parameters and updates the physical path in the file object
template<typename DEST_TYPE>
irods::error mock_archive_check_params_and_path(irods::plugin_context& _ctx)
{
    // verify that the resc context is valid
    if (const auto err = _ctx.valid<DEST_TYPE>(); !err.ok()) {
        return PASSMSG("Resource context is invalid.", err);
    }

    return mock_archive_check_path(_ctx);
} // mock_archive_check_params_and_path

//@brief Recursively make all of the dirs in the path
irods::error mock_archive_mkdir_r(const std::string& path, mode_t mode)
{
    std::string subdir;
    std::size_t pos = 0;
    bool done = false;
    while ( !done ) {
        pos = path.find_first_of( '/', pos + 1 );
        if ( pos > 0 ) {
            subdir = path.substr( 0, pos );
            int status = mkdir( subdir.c_str(), mode );

            // handle error cases
            if (status < 0 && errno != EEXIST) {
                return ERROR(UNIX_FILE_RENAME_ERR - errno, fmt::format(
                            "mkdir error for \"{}\", errno = \"{}\", status = {}.",
                            subdir, strerror(errno), status));
            }
        }
        if ( pos == std::string::npos ) {
            done = true;
        }
    }

    return SUCCESS();
} // mock_archive_mkdir_r


irods::error make_hashed_path(
    irods::plugin_property_map& _prop_map,
    const std::string&          _path,
    std::string&                _hashed)
{
    // hash the physical path to reflect object store behavior
    MD5_CTX context;
    char md5Buf[ MAX_NAME_LEN ];
    unsigned char hash  [ MAX_NAME_LEN ];

    strncpy( md5Buf, _path.c_str(), _path.size() );
    MD5_Init( &context );
    MD5_Update( &context, ( unsigned char* )md5Buf, _path.size() );
    MD5_Final( ( unsigned char* )hash, &context );

    std::stringstream ins;
    for ( int i = 0; i < 16; ++i ) {
        ins << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( int )hash[i];
    }

    // get the vault path for the resource
    std::string path;
    if (const auto err = _prop_map.get<std::string>(irods::RESOURCE_PATH, path); !err.ok()) {
        return PASSMSG("Failed to get vault path for resource.", err);
    }

    // append the hash to the path as the new 'cache file name'
    path += "/";
    path += ins.str();

    _hashed = path;

    return SUCCESS();
} // make_hashed_path

// =-=-=-=-=-=-=-
// 3. Define operations which will be called by the file*
//    calls declared in server/driver/include/fileDriver.h
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// NOTE :: to access properties in the _prop_map do the
//      :: following :
//      :: double my_var = 0.0;
//      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
// =-=-=-=-=-=-=-

// interface for POSIX mkdir
irods::error mock_archive_file_mkdir(irods::plugin_context& _ctx)
{
    // NOTE :: this function assumes the object's physical path is correct and
    //         should not have the vault path prepended - hcj
    if (const auto err = _ctx.valid<irods::collection_object>(); !err.ok()) {
        return PASSMSG("resource context is invalid.", err);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // make the call to mkdir & umask
    mode_t myMask = umask( ( mode_t ) 0000 );
    int    status = mkdir( fco->physical_path().c_str(), fco->mode() );

    // reset the old mask
    umask( ( mode_t ) myMask );

    // return an error if necessary
    if (status < 0) {
        const int err_status = UNIX_FILE_MKDIR_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "mkdir error for [{}], errno = [{}], status = {}.",
                    fco->physical_path(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code( status );
    return result;
} // mock_archive_file_mkdir

irods::error mock_archive_file_stat(irods::plugin_context&, struct stat* _statbuf)
{
    // manufacture a stat as we do not have a
    // microservice to perform this duty
    _statbuf->st_mode  = S_IFREG;
    _statbuf->st_nlink = 1;
    _statbuf->st_uid   = getuid();
    _statbuf->st_gid   = getgid();
    _statbuf->st_atime = _statbuf->st_mtime = _statbuf->st_ctime = time( 0 );
    _statbuf->st_size  = UNKNOWN_FILE_SZ;
    return SUCCESS();
} // mock_archive_file_stat

// interface for POSIX rename
irods::error mock_archive_file_rename(irods::plugin_context& _ctx,
                                      const char*            _new_file_name)
{
    // Check the operation parameters and update the physical path
    if (const auto err = mock_archive_check_params_and_path<irods::data_object>(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // manufacture a new path from the new file name
    std::string new_full_path;
    if (const auto err = mock_archive_generate_full_path(_ctx.prop_map(), _new_file_name, new_full_path); !err.ok()) {
        return PASSMSG(fmt::format(
                    "Unable to generate full path for destination file: \"{}\".",
                    _new_file_name), err);
    }

    // cast down the hierarchy to the desired object
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // get hashed names for the old path
    std::string new_hash{};
    if (const auto err = make_hashed_path(_ctx.prop_map(), _new_file_name, new_hash); !err.ok()) {
        return PASSMSG("Failed to gen hashed path", err);
    }

    rodsLog(LOG_DEBUG, "[%s:%d] - old path:[%s],new_path:[%s],new_hashed_path:[%s]",
            __FUNCTION__, __LINE__, fco->physical_path().c_str(), new_full_path.c_str(), new_hash.c_str());

    // make the call to rename
    const int status = rename(fco->physical_path().c_str(), new_hash.c_str());

    if (status < 0) {
        const int err_status = UNIX_FILE_RENAME_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Rename error for \"{}\" to \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path(), new_hash, strerror(errno), err_status));
    }

    fco->physical_path( new_hash );

    auto result = SUCCESS();
    result.code( status );
    return SUCCESS();
} // mock_archive_file_rename

// interface for POSIX Truncate
irods::error mock_archive_file_truncate(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = mock_archive_check_params_and_path<irods::file_object>(_ctx); !err.ok()) {
        return PASSMSG("Invalid plugin context.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to unlink
    if (const int status = truncate(fco->physical_path().c_str(), fco->size()); status < 0) {
        const int err_status = UNIX_FILE_UNLINK_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Truncate error for: \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path(), strerror(errno), err_status));
    }

    return SUCCESS();
} // mock_archive_file_truncate

// interface for POSIX Unlink
irods::error mock_archive_file_unlink(irods::plugin_context& _ctx)
{
    if (const auto err = mock_archive_check_params_and_path<irods::file_object>(_ctx); !err.ok()) {
        return PASSMSG("Invalid plugin context.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to unlink
    if (const int status = unlink( fco->physical_path().c_str()); status < 0) {
        const int err_status = UNIX_FILE_UNLINK_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Unlink error for: \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path(), strerror(errno), err_status));
    }

    return SUCCESS();
} // mock_archive_file_unlink

int
mockArchiveCopyPlugin(
    int         mode,
    const char* srcFileName,
    const char* destFileName ) {

    size_t trans_buff_size;
    try {
        trans_buff_size = irods::get_advanced_setting<const int>(irods::CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS) * 1024 * 1024;
    } catch ( const irods::exception& e ) {
        irods::log(e);
        return e.code();
    }

    int inFd, outFd;
    std::vector<char> myBuf( trans_buff_size );
    rodsLong_t bytesCopied = 0;
    int bytesRead;
    int bytesWritten;
    int status;
    struct stat statbuf;


    inFd = open( srcFileName, O_RDONLY, 0 );
    status = stat( srcFileName, &statbuf );
    if ( inFd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "mockArchiveCopyPlugin: open error for srcFileName %s, status = %d",
                 srcFileName, status );
        return status;
    }
    else if ( status < 0 ) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog( LOG_ERROR,
                 "mockArchiveCopyPlugin: stat of %s error, status = %d",
                 srcFileName, status );
        close( inFd ); // JMC cppcheck - resource
        return status;
    }
    else if ( ( statbuf.st_mode & S_IFREG ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "mockArchiveCopyPlugin: open error for srcFileName %s, status = %d",
                 srcFileName, UNIX_FILE_OPEN_ERR );
        close( inFd ); // JMC cppcheck - resource
        return status;
    }

    outFd = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode );
    if ( outFd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "mockArchiveCopyPlugin: open error for destFileName %s, status = %d",
                 destFileName, status );
        close( inFd );
        return status;
    }

    while ( ( bytesRead = read( inFd, ( void * ) myBuf.data(), trans_buff_size ) ) > 0 ) {
        bytesWritten = write( outFd, ( void * ) myBuf.data(), bytesRead );
        if ( bytesWritten <= 0 ) {
            status = UNIX_FILE_WRITE_ERR - errno;
            rodsLog( LOG_ERROR,
                     "mockArchiveCopyPlugin: write error for srcFileName %s, status = %d",
                     destFileName, status );
            close( inFd );
            close( outFd );
            return status;
        }
        bytesCopied += bytesWritten;
    }

    close( inFd );
    close( outFd );

    if ( bytesCopied != statbuf.st_size ) {
        rodsLog( LOG_ERROR,
                 "mockArchiveCopyPlugin: Copied size %lld does not match source \
                         size %lld of %s",
                 bytesCopied, statbuf.st_size, srcFileName );
        return SYS_COPY_LEN_ERR;
    }
    else {
        return 0;
    }

} // mockArchiveCopyPlugin

// unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error mock_archive_file_stage_to_cache(irods::plugin_context& _ctx,
                                              const char*            _cache_file_name)
{
    // Check the operation parameters and update the physical path
    if (const auto err = mock_archive_check_params_and_path<irods::file_object>(_ctx); !err.ok()) {
        return PASSMSG("Invalid plugin context.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // get the vault path for the resource
    std::string path;
    if (const auto err = _ctx.prop_map().get<std::string>(irods::RESOURCE_PATH, path); !err.ok()) {
        return PASSMSG("Failed to retrieve vault path for resource.", err);
    }

    // append the hash to the path as the new 'cache file name'
    path += "/";
    path += fco->physical_path().c_str();

    if (const int status = mockArchiveCopyPlugin(fco->mode(), fco->physical_path().c_str(), _cache_file_name); status < 0) {
        return ERROR(status, fmt::format(
                    "Failed copying archive file: \"{}\" to cache file: \"{}\".",
                    fco->physical_path(), _cache_file_name));
    }

    return SUCCESS();
} // mock_archive_file_stage_to_cache

// unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from cacheFilename to filename. optionalInfo info
// is not used.
irods::error mock_archive_file_sync_to_arch(irods::plugin_context& _ctx,
                                            const char*            _cache_file_name)
{
    if (const auto err = mock_archive_check_params_and_path<irods::file_object>(_ctx); !err.ok()) {
        return PASSMSG("Invalid plugin context.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // get the vault path for the resource
    std::string path;
    if (const auto err = make_hashed_path(_ctx.prop_map(), fco->physical_path(), path); !err.ok()) {
        return PASSMSG("Failed to gen hashed path", err);
    }

    // append the hash to the path as the new 'cache file name'
    rodsLog( LOG_NOTICE, "mock archive :: cache file name [%s]", _cache_file_name );

    rodsLog( LOG_NOTICE, "mock archive :: new hashed file name for [%s] is [%s]",
            fco->physical_path().c_str(), path.c_str() );

    // make the directories in the path to the new file
    std::string new_path = path;
    std::size_t last_slash = new_path.find_last_of( '/' );
    new_path.erase( last_slash );

    if (const auto err = mock_archive_mkdir_r(new_path, 0750); !err.ok()) {
        irods::log(LOG_DEBUG, err.result());
    }

    // make the copy to the 'archive'
    if (const int status = mockArchiveCopyPlugin(fco->mode(), _cache_file_name, path.c_str()); status < 0) {
        return ERROR(status, "Sync to arch failed.");
    }

    fco->physical_path( path );

    return SUCCESS();
} // mock_archive_file_sync_to_arch

// =-=-=-=-=-=-=-
// redirect_create - code to determine redirection for get operation
// Create never gets called on an archive.

// redirect_get - code to determine redirection for get operation
irods::error mock_archive_redirect_open(
    irods::plugin_property_map& _prop_map,
    irods::file_object_ptr      _file_obj,
    const std::string&          _resc_name,
    const std::string&          _curr_host,
    float&                      _out_vote)
{
    // initially set a good default
    _out_vote = 0.0;

    // determine if the resource is down
    int resc_status = 0;
    if (const auto err = _prop_map.get<int>(irods::RESOURCE_STATUS, resc_status); !err.ok()) {
        return PASSMSG("Failed to get \"status\" property.", err);
    }

    // if the status is down, vote no.
    if (INT_RESC_STATUS_DOWN != resc_status) {
        return SUCCESS();
    }

    // get the resource host for comparison to curr host
    std::string host_name;
    if (const auto err = _prop_map.get<std::string>(irods::RESOURCE_LOCATION, host_name); !err.ok()) {
        return PASSMSG("Failed to get \"location\" property.", err);
    }

    // set a flag to test if were at the curr host, if so we vote higher
    bool curr_host = ( _curr_host == host_name );

    // make some flags to clairify decision making
    bool need_repl = ( _file_obj->repl_requested() > -1 );

    // set up variables for iteration
    bool          found     = false;
    std::vector< irods::physical_object > objs = _file_obj->replicas();
    std::vector< irods::physical_object >::iterator itr = objs.begin();

    // check to see if the replica is in this resource, if one is requested
    for ( ; !found && itr != objs.end(); ++itr ) {
        // run the hier string through the parser and get the last entry.
        std::string last_resc;
        irods::hierarchy_parser parser;
        parser.set_string( itr->resc_hier() );
        parser.last_resc( last_resc );

        // more flags to simplify decision making
        bool repl_us = ( _file_obj->repl_requested() == itr->repl_num() );
        bool resc_us = ( _resc_name == last_resc );

        // success - correct resource and dont need a specific
        //           replication, or the repl nums match
        if ( resc_us ) {
            if ( !need_repl || ( need_repl && repl_us ) ) {
                found = true;
                if ( curr_host ) {
                    _out_vote = 1.0;
                }
                else {
                    _out_vote = 0.5;
                }
            }
        } // if resc_us
    } // for itr

    return SUCCESS();
} // mock_archive_redirect_open

// =-=-=-=-=-=-=-
// used to allow the resource to determine which host
// should provide the requested operation
irods::error mock_archive_file_resolve_hierarchy(
    irods::plugin_context& _ctx,
    const std::string*                  _opr,
    const std::string*                  _curr_host,
    irods::hierarchy_parser*           _out_parser,
    float*                              _out_vote ) {

    // =-=-=-=-=-=-=-
    // check the context validity
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( !ret.ok() ) {
        return PASSMSG( "Invalid plugin context.", ret );
    }

    if ( NULL == _opr || NULL == _curr_host || NULL == _out_parser || NULL == _out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "Invalid input parameters." );
    }

    // =-=-=-=-=-=-=-
    // get the name of this resource
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
    if ( !ret.ok() ) {
        return PASSMSG( "Failed to get property for resource name.", ret );
    }

    // =-=-=-=-=-=-=-
    // add ourselves to the hierarchy parser by default
    _out_parser->add_child( resc_name );

    // =-=-=-=-=-=-=-
    // test the operation to determine which choices to make
    if ( irods::OPEN_OPERATION == ( *_opr ) || irods::UNLINK_OPERATION == ( *_opr )) {
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // call redirect determination for 'open' operation
        ret = mock_archive_redirect_open( _ctx.prop_map(), file_obj, resc_name, ( *_curr_host ), ( *_out_vote ) );
    }
    else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
        ret = ERROR( SYS_INVALID_INPUT_PARAM, "Create operation not supported for an archive" );
    }
    else {
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation
        ret = ERROR( SYS_INVALID_INPUT_PARAM, std::string( "Operation not supported: \"" + ( *_opr ) + "\"." ) );
    }

    return ret;

} // mock_archive_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// mock_archive_file_rebalance - code which would rebalance the subtree
irods::error mock_archive_file_rebalance(
    irods::plugin_context& _ctx ) {

    return SUCCESS();

} // mock_archive_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle mock_archive file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class mockarchive_resource : public irods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
            public:
                maintenance_operation( const std::string& _n ) : name_( _n ) {
                }

            maintenance_operation( const maintenance_operation& _rhs ): name_(_rhs.name_) {
                }

                maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                    return *this;
                }

                irods::error operator()( rcComm_t* ) {
                    rodsLog( LOG_NOTICE, "mockarchive_resource::post_disconnect_maintenance_operation - [%s]",
                             name_.c_str() );
                    return SUCCESS();
                }

            private:
                std::string name_;

        }; // class maintenance_operation

    public:
        mockarchive_resource( const std::string& _inst_name,
                              const std::string& _context ) :
            irods::resource( _inst_name, _context ) {
        } // ctor


        irods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            return ERROR( -1, "nop" );
        }

}; // class mockarchive_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create mockarchive_resource
    mockarchive_resource* resc = new mockarchive_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            mock_archive_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            mock_archive_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            mock_archive_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            mock_archive_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            mock_archive_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            mock_archive_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            mock_archive_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            mock_archive_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            mock_archive_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
