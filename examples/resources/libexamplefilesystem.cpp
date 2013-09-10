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
// 1. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update 
//       the file object's physical path with the full path

// =-=-=-=-=-=-=-
/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
eirods::error example_generate_full_path(
    eirods::plugin_property_map& _prop_map,
    const std::string&           _phy_path,
    std::string&                 _ret_string )
{
    eirods::error result = SUCCESS();
    eirods::error ret;
    std::string vault_path;
    // TODO - getting vault path by property will not likely work for coordinating nodes
    ret = _prop_map.get<std::string>(eirods::RESOURCE_PATH, vault_path);
    if((result = ASSERT_ERROR(ret.ok(), SYS_INVALID_INPUT_PARAM, "resource has no vault path.")).ok()) {
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

} // example_generate_full_path

// =-=-=-=-=-=-=-
/// @brief update the physical path in the file object
eirods::error example_check_path( 
    eirods::resource_plugin_context& _ctx )
{
    eirods::error result = SUCCESS();
    
    // =-=-=-=-=-=-=-
    // try dynamic cast on ptr, throw error otherwise 
    eirods::data_object_ptr data_obj = boost::dynamic_pointer_cast< eirods::data_object >( _ctx.fco() );
    if((result = ASSERT_ERROR(data_obj.get(), SYS_INVALID_INPUT_PARAM, "Failed to cast fco to data_object." )).ok()) {

        // =-=-=-=-=-=-=-
        // NOTE: Must do this for all storage resources
        std::string full_path;
        eirods::error ret = example_generate_full_path( _ctx.prop_map(), 
                                                     data_obj->physical_path(), 
                                                     full_path );
        if((result = ASSERT_PASS(ret, "Failed generating full path for object.")).ok()) {
            data_obj->physical_path(full_path);
        }
    }
    
    return result;

} // example_check_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
template< typename DEST_TYPE >
eirods::error example_check_params_and_path(
    eirods::resource_plugin_context& _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx.valid< DEST_TYPE >(); 
    if((result = ASSERT_PASS(ret, "resource context is invalid." )).ok()) { 
        result = example_check_path( _ctx );
    }
    
    return result;

} // example_check_params_and_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
eirods::error example_check_params_and_path(
    eirods::resource_plugin_context& _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx.valid(); 
    if((result = ASSERT_PASS(ret, "example_check_params_and_path - resource context is invalid")).ok()) { 
        result = example_check_path( _ctx );
    }
    
    return result;

} // example_check_params_and_path

// =-=-=-=-=-=-=- 
//@brief Recursively make all of the dirs in the path
eirods::error example_file_mkdir_r( 
    rsComm_t*                      _comm,
    const std::string&             _results,
    const std::string& path,
    mode_t mode )
{
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
            result = ASSERT_ERROR(status >= 0 || errno == EEXIST, UNIX_FILE_RENAME_ERR - errno, "mkdir error for \"%s\", errno = \"%s\", status = %d.",
                                  subdir.c_str(), strerror(errno), status);
        }
        if(pos == std::string::npos) {
            done = true;
        }
    }
    
    return result;

} // example_file_mkdir_r

extern "C" {
    // =-=-=-=-=-=-=-
    // 2. Define operations which will be called by the file*
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
    eirods::error example_file_registered_plugin(
        eirods::resource_plugin_context& _ctx)
    {
        eirods::error result = SUCCESS();
        
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path(_ctx);
        result = ASSERT_PASS(ret, "Invalid parameters or physical path.");
        
        // NOOP
        return result;
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error example_file_unregistered_plugin(
        eirods::resource_plugin_context& _ctx)
    {
        eirods::error result = SUCCESS();
        
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path(_ctx);
        result = ASSERT_PASS(ret, "Invalid parameters or physical path.");

        // NOOP
        return result;
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error example_file_modified_plugin(
        eirods::resource_plugin_context& _ctx)
    {
        eirods::error result = SUCCESS();
        
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path(_ctx);
        result = ASSERT_PASS(ret, "Invalid parameters or physical path.");

        // NOOP
        return result;
    }
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error example_file_create_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make call to umask & open for create
            mode_t myMask = umask((mode_t) 0000);
            int    fd     = open( fco->physical_path().c_str(), O_RDWR|O_CREAT|O_EXCL, fco->mode() );

            // =-=-=-=-=-=-=-
            // reset the old mask 
            (void) umask((mode_t) myMask);
                
            // =-=-=-=-=-=-=-
            // if we got a 0 descriptor, try again
            if( fd == 0 ) {
        
                close (fd);
                rodsLog( LOG_NOTICE, "example_file_create_plugin: 0 descriptor" );
                open ("/dev/null", O_RDWR, 0);
                fd = open( fco->physical_path().c_str(), O_RDWR|O_CREAT|O_EXCL, fco->mode() );
            }

            // =-=-=-=-=-=-=-
            // cache file descriptor in out-variable
            fco->file_descriptor( fd );
                        
            // =-=-=-=-=-=-=-
            // trap error case with bad fd
            if( fd < 0 ) {
                int status = UNIX_FILE_CREATE_ERR - errno;
                if(!(result = ASSERT_ERROR(fd >= 0, UNIX_FILE_CREATE_ERR - errno, "create error for \"%s\", errno = \"%s\", status = %d",
                                           fco->physical_path().c_str(), strerror(errno), status)).ok()) {

                    // =-=-=-=-=-=-=-
                    // WARNING :: Major Assumptions are made upstream and use the FD also as a
                    //         :: Status, if this is not done EVERYTHING BREAKS!!!!111one
                    fco->file_descriptor( status );
                    result.code(status);
                } else {
                    result.code(fd);
                }
            }
        }
        // =-=-=-=-=-=-=-
        // declare victory!
        return result;

    } // example_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error example_file_open_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // handle OSX weirdness...
            int flags = fco->flags();

#if defined(osx_platform)
            // For osx, O_TRUNC = 0x0400, O_TRUNC = 0x200 for other system 
            if( flags & 0x200) {
                flags = flags ^ 0x200;
                flags = flags | O_TRUNC;
            }
#endif 
            // =-=-=-=-=-=-=-
            // make call to open
            errno = 0;
            int fd = open( fco->physical_path().c_str(), flags, fco->mode() );

            // =-=-=-=-=-=-=-
            // if we got a 0 descriptor, try again
            if( fd == 0 ) {
                close (fd);
                rodsLog( LOG_NOTICE, "example_file_open_plugin: 0 descriptor" );
                open ("/dev/null", O_RDWR, 0);
                fd = open( fco->physical_path().c_str(), flags, fco->mode() );
            }       
                        
            // =-=-=-=-=-=-=-
            // cache status in the file object
            fco->file_descriptor( fd );

            // =-=-=-=-=-=-=-
            // did we still get an error?
            int status = UNIX_FILE_OPEN_ERR - errno;
            if (!(result = ASSERT_ERROR(fd >= 0, status, "Open error for \"%s\", errno = \"%s\", status = %d, flags = %d.",
                                        fco->physical_path().c_str(), strerror(errno), status, flags)).ok()) {
                result.code(status);
            } else {
                result.code(fd);
            }
        }
        
        // =-=-=-=-=-=-=-
        // declare victory!
        return result;

    } // example_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error example_file_read_plugin( 
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len )
    {
        eirods::error result = SUCCESS();
        int status = 0;
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to read
            status = read( fco->file_descriptor(), _buf, _len );

            // =-=-=-=-=-=-=-
            // pass along an error if it was not successful
            int err_status = UNIX_FILE_READ_ERR - errno;
            if(!(result = ASSERT_ERROR(status >= 0, err_status, "Read error for file: \"%s\", errno = \"%s\".",
                                       fco->physical_path().c_str(), strerror(errno))).ok()) {
                result.code(err_status);
            } else {
                result.code(status);
            }
        }
        
        // =-=-=-=-=-=-=-
        // win!
        return result;

    } // example_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error example_file_write_plugin( 
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
         
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
                
            // =-=-=-=-=-=-=-
            // make the call to write
            int status = write( fco->file_descriptor(), _buf, _len );

            // =-=-=-=-=-=-=-
            // pass along an error if it was not successful
            int err_status = UNIX_FILE_WRITE_ERR - errno;
            if (!(result = ASSERT_ERROR(status >= 0, err_status, "Write file: \"%s\", errno = \"%s\", status = %d.",
                                        fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                        
                result.code(err_status);
            } else {
                result.code(status);
            }
        }
        
        // =-=-=-=-=-=-=-
        // win!
        return result;

    } // example_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error example_file_close_plugin(
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-                               
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to close
            int status = close( fco->file_descriptor() );

            // =-=-=-=-=-=-=-
            // log any error
            int err_status = UNIX_FILE_CLOSE_ERR - errno;
            if(!(result = ASSERT_ERROR(status >= 0, err_status, "Close error for file: \"%s\", errno = \"%s\", status = %d.",
                                       fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                result.code(err_status);
            } else {
                result.code(status);
            }
        }
        
        return result;

    } // example_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error example_file_unlink_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::data_object_ptr fco = boost::dynamic_pointer_cast< eirods::data_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to unlink      
            int status = unlink( fco->physical_path().c_str() );
            
            // =-=-=-=-=-=-=-
            // error handling
            int err_status = UNIX_FILE_UNLINK_ERR - errno;
            if(!(result = ASSERT_ERROR(status >= 0, err_status, "Unlink error for \"%s\", errno = \"%s\", status = %d.",
                                       fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                        
                result.code(err_status);
            } else {
                result.code(status);
            }
        }
        
        return result;

    } // example_file_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error example_file_stat_plugin( 
        eirods::resource_plugin_context& _ctx,
        struct stat*                        _statbuf )
    {
        eirods::error result = SUCCESS();
        // =-=-=-=-=-=-=-
        // NOTE:: this function assumes the object's physical path is 
        //        correct and should not have the vault path 
        //        prepended - hcj
         
        eirods::error ret = _ctx.valid(); 
        if((result = ASSERT_PASS(ret, "resource context is invalid.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::data_object_ptr fco = boost::dynamic_pointer_cast< eirods::data_object >( _ctx.fco() );
            
            // =-=-=-=-=-=-=-
            // make the call to stat
            int status = stat( fco->physical_path().c_str(), _statbuf );

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
            int err_status = UNIX_FILE_STAT_ERR - errno;
            if((result = ASSERT_ERROR(status >= 0, err_status, "Stat error for \"%s\", errno = \"%s\", status = %d.",
                                      fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                result.code(status);
            }
        }
        
        return result;

    } // example_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error example_file_lseek_plugin( 
        eirods::resource_plugin_context& _ctx,
        long long                           _offset, 
        int                                 _whence )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to lseek       
            long long status = lseek( fco->file_descriptor(),  _offset, _whence );

            // =-=-=-=-=-=-=-
            // return an error if necessary
            long long err_status = UNIX_FILE_LSEEK_ERR - errno;
            if((result = ASSERT_ERROR(status >= 0, err_status, "Lseek error for \"%s\", errno = \"%s\", status = %ld.",
                                      fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                result.code(status);
            }
        }
        
        return result;

    } // example_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error example_file_mkdir_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=- 
        // NOTE :: this function assumes the object's physical path is correct and 
        //         should not have the vault path prepended - hcj
         
        eirods::error ret = _ctx.valid< eirods::collection_object >(); 
        if((result = ASSERT_PASS(ret, "resource context is invalid.")).ok()) {
 
            // =-=-=-=-=-=-=-
            // cast down the chain to our understood object type
            eirods::collection_object_ptr fco = boost::dynamic_pointer_cast< eirods::collection_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // make the call to mkdir & umask
            mode_t myMask = umask( ( mode_t ) 0000 );
            int    status = mkdir( fco->physical_path().c_str(), fco->mode() );

            // =-=-=-=-=-=-=-
            // reset the old mask 
            umask( ( mode_t ) myMask );

            // =-=-=-=-=-=-=-
            // return an error if necessary
            result.code(status);
            int err_status = UNIX_FILE_MKDIR_ERR - errno;
            if((result = ASSERT_ERROR(status >= 0, err_status, "Mkdir error for \"%s\", errno = \"%s\", status = %d.",
                                      fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                result.code(status);
            }
        }
        return result;

    } // example_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rmdir
    eirods::error example_file_rmdir_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // cast down the chain to our understood object type
            eirods::collection_object_ptr fco = boost::dynamic_pointer_cast< eirods::collection_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to chmod
            int status = rmdir( fco->physical_path().c_str() );

            // =-=-=-=-=-=-=-
            // return an error if necessary
            int err_status = UNIX_FILE_RMDIR_ERR - errno;
            result = ASSERT_ERROR(status >= 0, err_status, "Rmdir error for \"%s\", errno = \"%s\", status = %d.",
                                  fco->physical_path().c_str(), strerror(errno), err_status);
        }
        
        return result;

    } // example_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error example_file_opendir_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = example_check_params_and_path< eirods::collection_object >( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // cast down the chain to our understood object type
            eirods::collection_object_ptr fco = boost::dynamic_pointer_cast< eirods::collection_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // make the callt to opendir
            DIR* dir_ptr = opendir( fco->physical_path().c_str() );

            // =-=-=-=-=-=-=-
            // if the directory can't be accessed due to permission
            // denied try again using root credentials.            
#ifdef RUN_SERVER_AS_ROOT
            if( dir_ptr == NULL && errno == EACCES && isServiceUserSet() ) {
                if (changeToRootUser() == 0) {
                    dir_ptr = opendir ( fco->physical_path().c_str() );
                    changeToServiceUser();
                } // if
#endif

                // =-=-=-=-=-=-=-
                // cache status in out variable
                int err_status = UNIX_FILE_OPENDIR_ERR - errno;

                // =-=-=-=-=-=-=-
                // return an error if necessary
                if((result = ASSERT_ERROR(NULL != dir_ptr, err_status, "Opendir error for \"%s\", errno = \"%s\", status = %d.",
                                          fco->physical_path().c_str(), strerror(errno), err_status)).ok()) {
                    // =-=-=-=-=-=-=-
                    // cache dir_ptr & status in out variables
                    fco->directory_pointer( dir_ptr );
                }
            }
        
            return result;

        } // example_file_opendir_plugin

        // =-=-=-=-=-=-=-
        // interface for POSIX closedir
        eirods::error example_file_closedir_plugin( 
            eirods::resource_plugin_context& _ctx )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path< eirods::collection_object >( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
                // =-=-=-=-=-=-=-
                // cast down the chain to our understood object type
                eirods::collection_object_ptr fco = boost::dynamic_pointer_cast< eirods::collection_object >( _ctx.fco() );

                // =-=-=-=-=-=-=-
                // make the callt to opendir
                int status = closedir( fco->directory_pointer() );
                        
                // =-=-=-=-=-=-=-
                // return an error if necessary
                int err_status = UNIX_FILE_CLOSEDIR_ERR - errno;
                result = ASSERT_ERROR(status >= 0, err_status, "Closedir error for \"%s\", errno = \"%s\", status = %d.",
                                      fco->physical_path().c_str(), strerror(errno), err_status);
            }
        
            return result;

        } // example_file_closedir_plugin

        // =-=-=-=-=-=-=-
        // interface for POSIX readdir
        eirods::error example_file_readdir_plugin( 
            eirods::resource_plugin_context& _ctx,
            struct rodsDirent**                 _dirent_ptr )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path< eirods::collection_object >( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
                // =-=-=-=-=-=-=-
                // cast down the chain to our understood object type
                eirods::collection_object_ptr fco = boost::dynamic_pointer_cast< eirods::collection_object >( _ctx.fco() );

                // =-=-=-=-=-=-=-
                // zero out errno?
                errno = 0;

                // =-=-=-=-=-=-=-
                // make the call to readdir
                struct dirent * tmp_dirent = readdir( fco->directory_pointer() );

                // =-=-=-=-=-=-=-
                // handle error cases
                if((result = ASSERT_ERROR(tmp_dirent != NULL || errno != 0, -1, "End of directory list reached.")).ok()) {
                    // =-=-=-=-=-=-=-
                    // cache status in out variable
                    int status = UNIX_FILE_READDIR_ERR - errno;
                    if((result = ASSERT_ERROR(tmp_dirent != NULL, status, "Readdir error, status = %d, errno= \"%s\".",
                                              status, strerror(errno))).ok()) {
                        // =-=-=-=-=-=-=-
                        // alloc dirent as necessary
                        if( !( *_dirent_ptr ) ) {
                            (*_dirent_ptr ) = new rodsDirent_t;
                        }

                        // =-=-=-=-=-=-=-
                        // convert standard dirent to rods dirent struct
                        int status = direntToRodsDirent( (*_dirent_ptr), tmp_dirent );

#if defined(solaris_platform)
                        rstrcpy( (*_dirent_ptr)->d_name, tmp_dirent->d_name, MAX_NAME_LEN );
#endif
                    }
                }
            }
        
            return result;

        } // example_file_readdir_plugin

        // =-=-=-=-=-=-=-
        // interface for POSIX readdir
        eirods::error example_file_rename_plugin( 
            eirods::resource_plugin_context& _ctx,
            const char*                         _new_file_name )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=- 
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {

                // =-=-=-=-=-=-=- 
                // manufacture a new path from the new file name 
                std::string new_full_path;
                ret = example_generate_full_path( _ctx.prop_map(), _new_file_name, new_full_path );
                if((result = ASSERT_PASS(ret, "Unable to generate full path for destination file: \"%s\".",
                                         _new_file_name)).ok()) {
         
                    // =-=-=-=-=-=-=-
                    // cast down the hierarchy to the desired object
                    eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

                    // =-=-=-=-=-=-=- 
                    // make the directories in the path to the new file
                    std::string new_path = new_full_path;
                    std::size_t last_slash = new_path.find_last_of('/');
                    new_path.erase(last_slash);
                    ret = example_file_mkdir_r( _ctx.comm(), "", new_path.c_str(), 0750 );
                    if((result = ASSERT_PASS(ret, "Mkdir error for \"%s\".", new_path.c_str())).ok()) {

                    }

                    // =-=-=-=-=-=-=-
                    // make the call to rename
                    int status = rename( fco->physical_path().c_str(), new_full_path.c_str() );

                    // =-=-=-=-=-=-=-
                    // handle error cases
                    if( status < 0 ) {
                        status = UNIX_FILE_RENAME_ERR - errno;
                                
                        std::stringstream msg;
                        msg << "example_file_rename_plugin: rename error for ";
                        msg <<  fco->physical_path();
                        msg << " to ";
                        msg << new_full_path;
                        msg << ", errno = ";
                        msg << strerror(errno);
                        msg << ", status = ";
                        msg << status;
                        
                        return ERROR( status, msg.str() );

                        // =-=-=-=-=-=-=-
                        // handle error cases
                        int err_status = UNIX_FILE_RENAME_ERR - errno;
                        if((result = ASSERT_ERROR(status >= 0, err_status, "Rename error for \"%s\" to \"%s\", errno = \"%s\", status = %d.",
                                                  fco->physical_path().c_str(), new_full_path.c_str(), strerror(errno), err_status)).ok()) {
                            result.code(status);
                        }
                    }
                }
            }
        
            return result;

        } // example_file_rename_plugin

        // =-=-=-=-=-=-=-
        // interface to determine free space on a device given a path
        eirods::error example_file_get_fsfreespace_plugin( 
            eirods::resource_plugin_context& _ctx )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
                // =-=-=-=-=-=-=-
                // cast down the hierarchy to the desired object
                eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

                int status = -1;
                rodsLong_t fssize = USER_NO_SUPPORT_ERR;
#if defined(solaris_platform)
                struct statvfs statbuf;
#else
                struct statfs statbuf;
#endif

#if defined(solaris_platform) || defined(sgi_platform)   ||     \
    defined(aix_platform)     || defined(linux_platform) ||     \
    defined(osx_platform)
#if defined(solaris_platform)
                status = statvfs( fco->physical_path().c_str(), &statbuf );
#else
#if defined(sgi_platform)
                status = statfs( fco->physical_path().c_str(), 
                                 &statbuf, sizeof (struct statfs), 0 );
#else
                status = statfs( fco->physical_path().c_str(), 
                                 &statbuf );
#endif
#endif

                // =-=-=-=-=-=-=-
                // handle error, if any
                int err_status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;
                if((result = ASSERT_ERROR(status >= 0, err_status, "Statfs error for \"%s\", status = %d.",
                                          fco->physical_path().c_str(), err_status)).ok()) {

#if defined(sgi_platform)
                    if (statbuf.f_frsize > 0) {
                        fssize = statbuf.f_frsize;
                    } else {
                        fssize = statbuf.f_bsize;
                    }
                    fssize *= statbuf.f_bavail;
#endif

#if defined(aix_platform) || defined(osx_platform) ||   \
    defined(linux_platform)
                    fssize = statbuf.f_bavail * statbuf.f_bsize;
#endif
 
#if defined(sgi_platform)
                    fssize = statbuf.f_bfree * statbuf.f_bsize;
#endif

#endif /* solaris_platform, sgi_platform .... */
                }
            }
        
            return result;

        } // example_file_get_fsfreespace_plugin

        eirods::error
            exampleFileCopyPlugin( int         mode, 
                                const char* srcFileName, 
                                const char* destFileName )
        {
            eirods::error result = SUCCESS();
        
            int inFd, outFd;
            char myBuf[TRANS_BUF_SZ];
            rodsLong_t bytesCopied = 0;
            int bytesRead;
            int bytesWritten;
            int status;
            struct stat statbuf;

            status = stat (srcFileName, &statbuf);
            int err_status = UNIX_FILE_STAT_ERR - errno;
            if ((result = ASSERT_ERROR(status >= 0, err_status, "Stat of \"%s\" error, status = %d",
                                       srcFileName, err_status)).ok()) {

                inFd = open (srcFileName, O_RDONLY, 0);
                err_status = UNIX_FILE_OPEN_ERR - errno;
                if (!(result = ASSERT_ERROR(inFd >= 0 && (statbuf.st_mode & S_IFREG) != 0, err_status, "Open error for srcFileName \"%s\", status = %d",
                                            srcFileName, status)).ok()) {
                    close( inFd ); // JMC cppcheck - resource
                }
                else {
                    outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
                    err_status = UNIX_FILE_OPEN_ERR - errno;
                    if (!(result = ASSERT_ERROR(outFd >= 0, err_status, "Open error for destFileName %s, status = %d",
                                                destFileName, status)).ok()) {
                        close (inFd);
                    }
                    else {
                        while (result.ok() && (bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
                            bytesWritten = write (outFd, (void *) myBuf, bytesRead);
                            err_status = UNIX_FILE_WRITE_ERR - errno;
                            if ((result = ASSERT_ERROR(bytesWritten > 0, err_status, "Write error for srcFileName %s, status = %d",
                                                       destFileName, status)).ok()) {
                                bytesCopied += bytesWritten;
                            }
                        }

                        close (inFd);
                        close (outFd);

                        if(result.ok()) {
                            result = ASSERT_ERROR(bytesCopied == statbuf.st_size, SYS_COPY_LEN_ERR, "Copied size %lld does not match source size %lld of %s",
                                                  bytesCopied, statbuf.st_size, srcFileName);
                        }
                    }
                }
            }
            return result;
        }

        // =-=-=-=-=-=-=-
        // exampleStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
        // Just copy the file from filename to cacheFilename. optionalInfo info
        // is not used.
        eirods::error example_file_stagetocache_plugin( 
            eirods::resource_plugin_context& _ctx,
            const char*                         _cache_file_name )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
                // =-=-=-=-=-=-=-
                // cast down the hierarchy to the desired object
                eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
                ret = exampleFileCopyPlugin( fco->mode(), fco->physical_path().c_str(), _cache_file_name );
                result = ASSERT_PASS(ret, "Failed");
            }
            return result;
        } // example_file_stagetocache_plugin

        // =-=-=-=-=-=-=-
        // exampleSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
        // Just copy the file from cacheFilename to filename. optionalInfo info
        // is not used.
        eirods::error example_file_synctoarch_plugin( 
            eirods::resource_plugin_context& _ctx,
            char*                               _cache_file_name )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // Check the operation parameters and update the physical path
            eirods::error ret = example_check_params_and_path( _ctx );
            if((result = ASSERT_PASS(ret, "Invalid parameters or physical path.")).ok()) {
        
                // =-=-=-=-=-=-=-
                // cast down the hierarchy to the desired object
                eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
                ret = exampleFileCopyPlugin( fco->mode(), _cache_file_name, fco->physical_path().c_str() );
                result = ASSERT_PASS(ret, "Failed");
            }
            return result;
        } // example_file_synctoarch_plugin

        // =-=-=-=-=-=-=-
        // redirect_create - code to determine redirection for create operation
        eirods::error example_file_redirect_create( 
            eirods::plugin_property_map&   _prop_map,
            eirods::file_object_ptr        _file_obj,
            const std::string&             _resc_name, 
            const std::string&             _curr_host, 
            float&                         _out_vote )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // determine if the resource is down 
            int resc_status = 0;
            eirods::error get_ret = _prop_map.get< int >( eirods::RESOURCE_STATUS, resc_status );
            if((result = ASSERT_PASS(get_ret, "Failed to get \"status\" property." )).ok()) {

                // =-=-=-=-=-=-=-
                // if the status is down, vote no.
                if( INT_RESC_STATUS_DOWN == resc_status ) {
                    _out_vote = 0.0;
                }
                else {
                
                    // =-=-=-=-=-=-=-
                    // get the resource host for comparison to curr host
                    std::string host_name;
                    get_ret = _prop_map.get< std::string >( eirods::RESOURCE_LOCATION, host_name );
                    if((result = ASSERT_PASS(get_ret, "Failed to get \"location\" property." )).ok()) {
        
                        // =-=-=-=-=-=-=-
                        // vote higher if we are on the same host
                        if( _curr_host == host_name ) {
                            _out_vote = 1.0;
                        } else {
                            _out_vote = 0.5;
                        }
                    }
                }
            }
            return result;

        } // example_file_redirect_create

        // =-=-=-=-=-=-=-
        // redirect_open - code to determine redirection for open operation
        eirods::error example_file_redirect_open( 
            eirods::plugin_property_map&   _prop_map,
            eirods::file_object_ptr        _file_obj,
            const std::string&             _resc_name, 
            const std::string&             _curr_host, 
            float&                         _out_vote )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // initially set a good default
            _out_vote = 0.0;

            // =-=-=-=-=-=-=-
            // determine if the resource is down 
            int resc_status = 0;
            eirods::error get_ret = _prop_map.get< int >( eirods::RESOURCE_STATUS, resc_status );
            if((result = ASSERT_PASS(get_ret, "Failed to get \"status\" property." )).ok()) {

                // =-=-=-=-=-=-=-
                // if the status is down, vote no.
                if( INT_RESC_STATUS_DOWN != resc_status ) {

                    // =-=-=-=-=-=-=-
                    // get the resource host for comparison to curr host
                    std::string host_name;
                    get_ret = _prop_map.get< std::string >( eirods::RESOURCE_LOCATION, host_name );
                    if((result = ASSERT_PASS(get_ret, "Failed to get \"location\" property." )).ok()) {
        
                        // =-=-=-=-=-=-=-
                        // set a flag to test if were at the curr host, if so we vote higher
                        bool curr_host = ( _curr_host == host_name );

                        // =-=-=-=-=-=-=-
                        // make some flags to clairify decision making
                        bool need_repl = ( _file_obj->repl_requested() > -1 );

                        // =-=-=-=-=-=-=-
                        // set up variables for iteration
                        bool          found     = false;
                        eirods::error final_ret = SUCCESS();
                        std::vector< eirods::physical_object > objs = _file_obj->replicas();
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
                            bool repl_us  = ( _file_obj->repl_requested() == itr->repl_num() ); 
                            bool resc_us  = ( _resc_name == last_resc );
                            bool is_dirty = ( itr->is_dirty() != 1 );

                            // =-=-=-=-=-=-=-
                            // success - correct resource and dont need a specific
                            //           replication, or the repl nums match
                            if( resc_us ) {
                                // =-=-=-=-=-=-=-
                                // if a specific replica is requested then we
                                // ignore all other criteria
                                if( need_repl ) {
                                    if( repl_us ) {
                                        _out_vote = 1.0;
                                    } else {
                                        // =-=-=-=-=-=-=-
                                        // repl requested and we are not it, vote 
                                        // very low
                                        _out_vote = 0.25;
                                    }
                                } else {
                                    // =-=-=-=-=-=-=-
                                    // if no repl is requested consider dirty flag
                                    if( is_dirty ) {
                                        // =-=-=-=-=-=-=-
                                        // repl is dirty, vote very low
                                        _out_vote = 0.25;
                                    } else {
                                        // =-=-=-=-=-=-=-
                                        // if our repl is not dirty then a local copy
                                        // wins, otherwise vote middle of the road
                                        if( curr_host ) {
                                            _out_vote = 1.0;
                                        } else {
                                            _out_vote = 0.5;
                                        }
                                    }
                                }
           
                                found = true;
                                break;
            
                            } // if resc_us

                        } // for itr
                    }
                }
            }                             
            return result;

        } // example_file_redirect_open

        // =-=-=-=-=-=-=-
        // used to allow the resource to determine which host
        // should provide the requested operation
        eirods::error example_file_redirect_plugin( 
            eirods::resource_plugin_context& _ctx,
            const std::string*                  _opr,
            const std::string*                  _curr_host,
            eirods::hierarchy_parser*           _out_parser,
            float*                              _out_vote )
        {
            eirods::error result = SUCCESS();
        
            // =-=-=-=-=-=-=-
            // check the context validity
            eirods::error ret = _ctx.valid< eirods::file_object >(); 
            if((result = ASSERT_PASS(ret, "Invalid resource context.")).ok()) {
 
                // =-=-=-=-=-=-=-
                // check incoming parameters
                if((result = ASSERT_ERROR(_opr && _curr_host && _out_parser && _out_vote, SYS_INVALID_INPUT_PARAM, "Invalid input parameter.")).ok()) {
        
                    // =-=-=-=-=-=-=-
                    // cast down the chain to our understood object type
                    eirods::file_object_ptr file_obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

                    // =-=-=-=-=-=-=-
                    // get the name of this resource
                    std::string resc_name;
                    ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, resc_name );
                    if((result = ASSERT_PASS(ret, "Failed in get property for name." )).ok()) {

                        // =-=-=-=-=-=-=-
                        // add ourselves to the hierarchy parser by default
                        _out_parser->add_child( resc_name );

                        // =-=-=-=-=-=-=-
                        // test the operation to determine which choices to make
                        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
                            // =-=-=-=-=-=-=-
                            // call redirect determination for 'get' operation
                            ret = example_file_redirect_open( _ctx.prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote) );
                            result = ASSERT_PASS(ret, "Failed redirecting for open.");

                        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
                            // =-=-=-=-=-=-=-
                            // call redirect determination for 'create' operation
                            ret = example_file_redirect_create( _ctx.prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );
                            result = ASSERT_PASS(ret, "Failed redirecting for create.");
                        }

                        else {
                            // =-=-=-=-=-=-=-
                            // must have been passed a bad operation 
                            result = ASSERT_ERROR(false, EIRODS_INVALID_OPERATION, "Operation not supported.");
                        }
                    }
                }
            }
            return result;
        } // example_file_redirect_plugin

        // =-=-=-=-=-=-=-
        // 3. create derived class to handle example file system resources
        //    necessary to do custom parsing of the context string to place
        //    any useful values into the property map for reference in later
        //    operations.  semicolon is the preferred delimiter
        class examplefilesystem_resource : public eirods::resource {
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
                    rodsLog( LOG_NOTICE, "examplefilesystem_resource::post_disconnect_maintenance_operation - [%s]", 
                             name_.c_str() );
                    return SUCCESS();
                }

            private:
                std::string name_;

            }; // class maintenance_operation

        public:
            examplefilesystem_resource( const std::string& _inst_name, 
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
            eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _op  )
                {
                    eirods::error result = SUCCESS();
#if 0
                    std::string name;
                    eirods::error err = get_property< std::string >( "name", name );
                    if((result = ASSERT_PASS(err, "post_disconnect_maintenance_operation failed.")).ok()) {
                    
                        _op = maintenance_operation( name );
                    }
                    return result;
#else
                    return ERROR( -1, "nop" );
#endif
                }
        }; // class examplefilesystem_resource
  
        // =-=-=-=-=-=-=-
        // 4. create the plugin factory function which will return a dynamically
        //    instantiated object of the previously defined derived resource.  use
        //    the add_operation member to associate a 'call name' to the interfaces
        //    defined above.  for resource plugins these call names are standardized
        //    as used by the eirods facing interface defined in 
        //    server/drivers/src/fileDriver.c
        eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

            // =-=-=-=-=-=-=-
            // 4a. create examplefilesystem_resource
            examplefilesystem_resource* resc = new examplefilesystem_resource( _inst_name, _context );

            // =-=-=-=-=-=-=-
            // 4b. map function names to operations.  this map will be used to load
            //     the symbols from the shared object in the delay_load stage of 
            //     plugin loading.
            resc->add_operation( eirods::RESOURCE_OP_CREATE,       "example_file_create_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_OPEN,         "example_file_open_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_READ,         "example_file_read_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_WRITE,        "example_file_write_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "example_file_close_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "example_file_unlink_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_STAT,         "example_file_stat_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "example_file_lseek_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "example_file_mkdir_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "example_file_rmdir_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "example_file_opendir_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "example_file_closedir_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_READDIR,      "example_file_readdir_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_RENAME,       "example_file_rename_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "example_file_get_fsfreespace_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "example_file_stagetocache_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "example_file_synctoarch_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "example_file_registered_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "example_file_unregistered_plugin" );
            resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "example_file_modified_plugin" );
        
            resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "example_file_redirect_plugin" );

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



