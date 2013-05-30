/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_physical_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_redirect.h"
#include "eirods_stacktrace.h"

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

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>  
#endif
#include <dirent.h>
 
#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>


// =-=-=-=-=-=-=-
// 2. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update 
//       the file object's physical path with the full path

// =-=-=-=-=-=-=-
/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
eirods::error unix_generate_full_path(
    eirods::resource_property_map&      _prop_map,
    const std::string&                  _phy_path,
    std::string&                        _ret_string )
{
    eirods::error result = SUCCESS();
    eirods::error ret;
    std::string vault_path;
    // TODO - getting vault path by property will not likely work for coordinating nodes
    ret = _prop_map.get<std::string>( eirods::RESOURCE_PATH, vault_path);
    if(!ret.ok()) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - resource has no vault path.";
        result = ERROR(-1, msg.str());
    } else {
        if(_phy_path.compare(0, 1, "/") != 0 &&
           _phy_path.compare(0, vault_path.size(), vault_path) != 0) {
            _ret_string  = vault_path;
            _ret_string += "/";
            _ret_string += _phy_path;
        } else {
            // The physical path already contains the vault path
            _ret_string = _phy_path;
        }
    }
    
    return result;

} // unix_generate_full_path

// =-=-=-=-=-=-=-
/// @brief update the physical path in the file object
eirods::error unix_check_path( 
    eirods::resource_operation_context* _ctx ) {
    // =-=-=-=-=-=-=-
    // NOTE: Must do this for all storage resources
    std::string full_path;
    eirods::error ret = unix_generate_full_path( _ctx->prop_map(), 
                                                 _ctx->fco().physical_path(), 
                                                 full_path );
    if(!ret.ok()) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed generating full path for object.";
        ret = PASSMSG(msg.str(), ret);
    } else {
        _ctx->fco().physical_path(full_path);
    }

    return ret;

} // unix_check_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
template< typename DEST_TYPE >
eirods::error unix_check_params_and_path(
    eirods::resource_operation_context* _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( !_ctx ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - null resource context";
        result = ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    } 
  
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx->valid< DEST_TYPE >(); 
    if( !ret.ok() ) { 
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - resource context is invalid.";
        result = PASSMSG( msg.str(), ret );
    } else {
        result = unix_check_path( _ctx );
    }

    return result;

} // unix_check_params_and_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
eirods::error unix_check_params_and_path(
    eirods::resource_operation_context* _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( !_ctx ) {
        result = ERROR( SYS_INVALID_INPUT_PARAM, "unix_check_params_and_path - null resource_property_map" );
    } 
  
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx->valid(); 
    if( !ret.ok() ) { 
        result = PASSMSG( "unix_check_params_and_path - resource context is invalid", ret );
    } else {
        result = unix_check_path( _ctx );
    }

    return result;

} // unix_check_params_and_path

// =-=-=-=-=-=-=- 
//@brief Recursively make all of the dirs in the path
eirods::error mock_archive_mkdir_r( 
    rsComm_t*                      _comm,
    const std::string&             _results,
    const std::string& path,
    mode_t mode ) {

    eirods::error result = SUCCESS();
    std::string subdir;
    std::size_t pos = 0;
    bool done = false;
    while(!done && result.ok()) {
        pos = path.find_first_of('/', pos + 1);
        if(pos > 0) {
            subdir = path.substr(0, pos);
            int status = mkdir(subdir.c_str(), mode);
            
            // =-=-=-=-=-=-=-
            // handle error cases
            if( status < 0 && errno != EEXIST) {
                status = UNIX_FILE_RENAME_ERR - errno;
                            
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << ": mkdir error for ";
                msg << subdir;
                msg << ", errno = ";
                msg << strerror(errno);
                msg << ", status = ";
                msg << status;
                    
                result = ERROR( status, msg.str() );
            }
        }
        if(pos == std::string::npos) {
            done = true;
        }
    }
    
    return result;

} // mock_archive_mkdir_r

extern "C" {

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;


    // =-=-=-=-=-=-=-
    // 3. Define operations which will be called by the file*
    //    calls declared in server/driver/include/fileDriver.h
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // NOTE :: to access properties in the _prop_map do the 
    //      :: following :
    //      :: double my_var = 0.0;
    //      :: eirods::error ret = _prop_map.get< double >( "my_key", my_var ); 
    // =-=-=-=-=-=-=-

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error mock_archive_registered_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error mock_archive_unregistered_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error mock_archive_modified_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error mock_archive_create_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "create not supported" );

    } // mock_archive_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error mock_archive_open_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "open not supported" );

    } // mock_archive_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error mock_archive_read_plugin( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "read not supported" );

    } // mock_archive_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error mock_archive_write_plugin( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "write not supported" );

    } // mock_archive_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error mock_archive_close_plugin(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "close not supported" );

    } // mock_archive_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error mock_archive_unlink_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to unlink      
        int status = unlink( fco.physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // error handling
        if( status < 0 ) {
            status = UNIX_FILE_UNLINK_ERR - errno;
                        
            std::stringstream msg;
            msg << "mock_archive_unlink_plugin: unlink error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // mock_archive_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error mock_archive_stat_plugin( 
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) { 
        // =-=-=-=-=-=-=-
        // NOTE:: this function assumes the object's physical path is 
        //        correct and should not have the vault path 
        //        prepended - hcj
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "mock_archive_stat_plugin - invalid resource context" );
        }
         
        eirods::error ret = _ctx->valid(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to stat
        int status = stat( fco.physical_path().c_str(), _statbuf );

        // =-=-=-=-=-=-=-
        // if the file can't be accessed due to permission denied 
        // try again using root credentials.
#ifdef RUN_SERVER_AS_ROOT
        if( status < 0 && errno == EACCES && isServiceUserSet() ) {
            if (changeToRootUser() == 0) {
                status = stat (filename, statbuf);
                changeToServiceUser();
            }
        }
#endif

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_STAT_ERR - errno;
 
            std::stringstream msg;
            msg << "mock_archive_stat_plugin: stat error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
                
        return CODE( status );

    } // mock_archive_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error mock_archive_fstat_plugin( 
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "fstat not supported" );

    } // mock_archive_fstat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error mock_archive_lseek_plugin( 
        eirods::resource_operation_context* _ctx,
        size_t                              _offset, 
        int                                 _whence ) {
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "lseek not supported" );

    } // mock_archive_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error mock_archive_fsync_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "fsync not supported" );

    } // mock_archive_fsync_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error mock_archive_mkdir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=- 
        // NOTE :: this function assumes the object's physical path is correct and 
        //         should not have the vault path prepended - hcj
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "mock_archive_mkdir_plugin - invalid resource context" );
        }
         
        eirods::error ret = _ctx->valid< eirods::collection_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object& coll_obj = dynamic_cast< eirods::collection_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // make the call to mkdir & umask
        mode_t myMask = umask( ( mode_t ) 0000 );
        int    status = mkdir( coll_obj.physical_path().c_str(), coll_obj.mode() );

        // =-=-=-=-=-=-=-
        // reset the old mask 
        umask( ( mode_t ) myMask );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_MKDIR_ERR - errno;
 
            if (errno != EEXIST) {
                std::stringstream msg;
                msg << "mock_archive_mkdir_plugin: mkdir error for ";
                msg << coll_obj.physical_path();
                msg << ", errno = '";
                msg << strerror( errno );
                msg << "', status = ";
                msg << status;
                                
                return ERROR( status, msg.str() );

            } // if errno != EEXIST

        } // if status < 0 

        return CODE( status );

    } // mock_archive_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rmdir
    eirods::error mock_archive_rmdir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "rmdir not supported" );

    } // mock_archive_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error mock_archive_opendir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "opendir not supported" );

    } // mock_archive_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error mock_archive_closedir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "closedir not supported" );

    } // mock_archive_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error mock_archive_readdir_plugin( 
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "readdir not supported" );

    } // mock_archive_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error mock_archive_rename_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        // =-=-=-=-=-=-=- 
        // manufacture a new path from the new file name 
        std::string new_full_path;
        ret = unix_generate_full_path( _ctx->prop_map(), _new_file_name, new_full_path );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Unable to generate full path for destinate file: \"" << _new_file_name << "\"";
            return PASSMSG(msg.str(), ret);
        }
         
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();

        // =-=-=-=-=-=-=- 
        // make the directories in the path to the new file
        std::string new_path = new_full_path;
        std::size_t last_slash = new_path.find_last_of('/');
        new_path.erase(last_slash);
        ret = mock_archive_mkdir_r( _ctx->comm(), "", new_path.c_str(), 0750 );
        if(!ret.ok()) {

            std::stringstream msg;
            msg << "mock_archive_rename_plugin: mkdir error for ";
            msg << new_path;

            return PASSMSG( msg.str(), ret);

        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = rename( fco.physical_path().c_str(), new_full_path.c_str() );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            status = UNIX_FILE_RENAME_ERR - errno;
                                
            std::stringstream msg;
            msg << "mock_archive_rename_plugin: rename error for ";
            msg <<  fco.physical_path();
            msg << " to ";
            msg << new_full_path;
            msg << ", errno = ";
            msg << strerror(errno);
            msg << ", status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // mock_archive_rename_plugin

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error mock_archive_get_fsfreespace_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // operation not supported
        return ERROR( SYS_NOT_SUPPORTED, "getfsfreespace not supported" );

    } // mock_archive_get_fsfreespace_plugin

    int
    mockArchiveCopyPlugin(
        int         mode, 
        const char* srcFileName, 
        const char* destFileName )
    {
        rodsLog( LOG_NOTICE, "XXXX - mockArchiveCopyPlugin copy from src [%s] to dst [%s]", srcFileName, destFileName );

        int inFd, outFd;
        char myBuf[TRANS_BUF_SZ];
        rodsLong_t bytesCopied = 0;
        int bytesRead;
        int bytesWritten;
        int status;
        struct stat statbuf;

        status = stat (srcFileName, &statbuf);

        if (status < 0) {
            status = UNIX_FILE_STAT_ERR - errno;
            rodsLog (LOG_ERROR, 
                     "mockArchiveCopyPlugin: stat of %s error, status = %d",
                     srcFileName, status);
            return status;
        }

        inFd = open (srcFileName, O_RDONLY, 0);
        if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "mockArchiveCopyPlugin: open error for srcFileName %s, status = %d",
                     srcFileName, status );
            close( inFd ); // JMC cppcheck - resource
            return status;
        }

        outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
        if (outFd < 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "mockArchiveCopyPlugin: open error for destFileName %s, status = %d",
                     destFileName, status);
            close (inFd);
            return status;
        }

        while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
            bytesWritten = write (outFd, (void *) myBuf, bytesRead);
            if (bytesWritten <= 0) {
                status = UNIX_FILE_WRITE_ERR - errno;
                rodsLog (LOG_ERROR,
                         "mockArchiveCopyPlugin: write error for srcFileName %s, status = %d",
                         destFileName, status);
                close (inFd);
                close (outFd);
                return status;
            }
            bytesCopied += bytesWritten;
        }

        close (inFd);
        close (outFd);

        if (bytesCopied != statbuf.st_size) {
            rodsLog ( LOG_ERROR,
                      "mockArchiveCopyPlugin: Copied size %lld does not match source \
                             size %lld of %s",
                      bytesCopied, statbuf.st_size, srcFileName );
            return SYS_COPY_LEN_ERR;
        } else {
            return 0;
        }

    } // mockArchiveCopyPlugin

    // =-=-=-=-=-=-=-
    // unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error mock_archive_stagetocache_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // get the vault path for the resource
        std::string path;
        ret = _ctx->prop_map().get< std::string >( "path", path ); 
        if( !ret.ok() ) {
            return PASS( ret );
        }
       
        // =-=-=-=-=-=-=-
        // append the hash to the path as the new 'cache file name'
        path += "/";
        path += fco.physical_path().c_str();
        
        int status = mockArchiveCopyPlugin( fco.mode(), fco.physical_path().c_str(), _cache_file_name );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "mock_archive_stagetocache_plugin failed copying archive file: \"";
            msg << fco.physical_path();
            msg << "\" to cache file: \"";
            msg << _cache_file_name;
            msg << "\"";
            return ERROR( status, msg.str());
        } else {
            return SUCCESS();
        }
    } // mock_archive_stagetocache_plugin

    // =-=-=-=-=-=-=-
    // unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error mock_archive_synctoarch_plugin( 
        eirods::resource_operation_context* _ctx,
        char*                               _cache_file_name ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
       
        // =-=-=-=-=-=-=-
        // hash the physical path to reflect object store behavior
        MD5_CTX context;
        char md5Buf[ MAX_NAME_LEN ];
        unsigned char hash  [ MAX_NAME_LEN ];

        strncpy( md5Buf, fco.physical_path().c_str(), fco.physical_path().size() );
        MD5Init( &context );
        MD5Update( &context, (unsigned char*)md5Buf, fco.physical_path().size() );
        MD5Final( (unsigned char*)hash, &context );
       

        std::stringstream ins;
        for(int i = 0; i < 16; ++i) {
            ins << std::setfill('0') << std::setw(2) << std::hex << (int)hash[i];
        }

        rodsLog( LOG_NOTICE, "XXXX - mock_archive :: buf [%s]   hash [%s]", md5Buf, ins.str().c_str() );


        // =-=-=-=-=-=-=-
        // get the vault path for the resource
        std::string path;
        ret = _ctx->prop_map().get< std::string >( eirods::RESOURCE_PATH, path ); 
        if( !ret.ok() ) {
            return PASS( ret );
        }
       
        // =-=-=-=-=-=-=-
        // append the hash to the path as the new 'cache file name'
        path += "/";
        path += ins.str();

        rodsLog( LOG_NOTICE, "mock archive :: cache file name [%s]", _cache_file_name );

        rodsLog( LOG_NOTICE, "mock archive :: new hashed file name for [%s] is [%s]",
                 fco.physical_path().c_str(), path.c_str() );
        
        // =-=-=-=-=-=-=-
        // make the copy to the 'archive'
        int status = mockArchiveCopyPlugin( fco.mode(), _cache_file_name, path.c_str() );
        if( status < 0 ) {
            return ERROR( status, "mock_archive_synctoarch_plugin failed." );
        } else {
            fco.physical_path( ins.str() );
            return SUCCESS();
        }

    } // mock_archive_synctoarch_plugin

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error mock_archive_redirect_create( 
        eirods::resource_property_map& _prop_map,
        eirods::file_object&           _file_obj,
        const std::string&             _resc_name, 
        const std::string&             _curr_host, 
        float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( eirods::RESOURCE_STATUS, resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "mock_archive_redirect_create - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( eirods::RESOURCE_LOCATION, host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "mock_archive_redirect_create - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // mock_archive_redirect_create

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error mock_archive_redirect_open( 
        eirods::resource_property_map& _prop_map,
        eirods::file_object&           _file_obj,
        const std::string&             _resc_name, 
        const std::string&             _curr_host, 
        float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // initially set a good default
        _out_vote = 0.0;

        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( eirods::RESOURCE_STATUS, resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "mock_archive_redirect_open - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( eirods::RESOURCE_LOCATION, host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "mock_archive_redirect_open - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // set a flag to test if were at the curr host, if so we vote higher
        bool curr_host = ( _curr_host == host_name );

        // =-=-=-=-=-=-=-
        // make some flags to clairify decision making
        bool need_repl = ( _file_obj.repl_requested() > -1 );

        // =-=-=-=-=-=-=-
        // set up variables for iteration
        bool          found     = false;
        eirods::error final_ret = SUCCESS();
        std::vector< eirods::physical_object > objs = _file_obj.replicas();
        std::vector< eirods::physical_object >::iterator itr = objs.begin();
        
        // =-=-=-=-=-=-=-
        // check to see if the replica is in this resource, if one is requested
        for( ; itr != objs.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // run the hier string through the parser and get the last
            // entry.
            std::string last_resc;
            eirods::hierarchy_parser parser;
            parser.set_string( itr->resc_hier() );
            parser.last_resc( last_resc ); 
          
            // =-=-=-=-=-=-=-
            // more flags to simplify decision making
            bool repl_us = ( _file_obj.repl_requested() == itr->repl_num() ); 
            bool resc_us = ( _resc_name == last_resc );

            // =-=-=-=-=-=-=-
            // success - correct resource and dont need a specific
            //           replication, or the repl nums match
            if( resc_us ) {
                if( !need_repl || ( need_repl && repl_us ) ) {
                    found = true;
                    if( curr_host ) {
                        _out_vote = 1.0;
                    } else {
                        _out_vote = 0.5;
                    }
                    break; 
                }

            } // if resc_us

        } // for itr
                             
        return SUCCESS();

    } // mock_archive_redirect_open

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    eirods::error mock_archive_redirect_plugin( 
        eirods::resource_operation_context* _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {

        // =-=-=-=-=-=-=-
        // check the context pointer
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "mock_archive_mkdir_plugin - invalid resource context" );
        }
         
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx->valid< eirods::file_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_opr ) {
            return ERROR( -1, "mock_archive_redirect_plugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "mock_archive_redirect_plugin - null operation" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "mock_archive_redirect_plugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "mock_archive_redirect_plugin - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::file_object& file_obj = dynamic_cast< eirods::file_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx->prop_map().get< std::string >( eirods::RESOURCE_NAME, resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "mock_archive_redirect_plugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'get' operation
            return mock_archive_redirect_open( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote) );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return mock_archive_redirect_create( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "mock_archive_redirect_plugin - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // mock_archive_redirect_plugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class mockarchive_resource : public eirods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
        public:
            maintenance_operation( const std::string& _n ) : name_(_n) {
            }

            maintenance_operation( const maintenance_operation& _rhs ) {
                name_ = _rhs.name_;    
            }

            maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                name_ = _rhs.name_;    
                return *this;
            }

            eirods::error operator()( rcComm_t* ) {
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
            eirods::resource( _inst_name, _context ) {

            if( !context_.empty() ) {
                // =-=-=-=-=-=-=-
                // tokenize context string into key/val pairs assuming a ; as a separator
                std::vector< std::string > key_vals;
                eirods::string_tokenize( _context, ";", key_vals );

                // =-=-=-=-=-=-=-
                // tokenize each key/val pair using = as a separator and
                // add them to the property list
                std::vector< std::string >::iterator itr = key_vals.begin();
                for( ; itr != key_vals.end(); ++itr ) {

                    if( !itr->empty() ) {
                        // =-=-=-=-=-=-=-
                        // break up key and value into two strings
                        std::vector< std::string > vals;
                        eirods::string_tokenize( *itr, "=", vals );
                        
                        // =-=-=-=-=-=-=-
                        // break up key and value into two strings
                        if( vals.size() == 2 ) {
                            properties_[ vals[0] ] = vals[1];
                        } else {
                            // this would be an error case  
                        }

                    } // if key_val not empty
                
                } // for itr 
            
            } // if context not empty

        } // ctor


        eirods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _op  ) {
#if 0
            std::string name;
            eirods::error err = get_property< std::string >( "name", name );
            if( !err.ok() ) {
                return PASSMSG( "mockarchive_resource::post_disconnect_maintenance_operation failed.", err );
            }

            _op = maintenance_operation( name );
            return SUCCESS();
#else
            return ERROR( -1, "nop" );
#endif
        }

    }; // class mockarchive_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create mockarchive_resource
        mockarchive_resource* resc = new mockarchive_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "mock_archive_create_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "mock_archive_open_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "mock_archive_read_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "mock_archive_write_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "mock_archive_close_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "mock_archive_unlink_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "mock_archive_stat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "mock_archive_fstat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "mock_archive_lseek_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "mock_archive_fsync_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "mock_archive_mkdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "mock_archive_rmdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "mock_archive_opendir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "mock_archive_closedir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "mock_archive_readdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "mock_archive_rename_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "mock_archive_get_fsfreespace_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "mock_archive_stagetocache_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "mock_archive_synctoarch_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "mock_archive_registered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "mock_archive_unregistered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "mock_archive_modified_plugin" );
        
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "mock_archive_redirect_plugin" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( eirods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( eirods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



