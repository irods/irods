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
#include <boost/lexical_cast.hpp>

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

extern "C" {
// hpss includes
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <hpss_errno.h>
#include <hpss_api.h>
#include <hpss_Getenv.h>
#include <hpss_limits.h>
}

// =-=-=-=-=-=-=-
// 2. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update 
//       the file object's physical path with the full path

// =-=-=-=-=-=-=-
/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
eirods::error hpss_generate_full_path(
    eirods::resource_property_map&      _prop_map,
    const std::string&                  _phy_path,
    std::string&                        _ret_string )
{
    eirods::error result = SUCCESS();
    eirods::error ret;
    std::string vault_path;
    // TODO - getting vault path by property will not likely work for coordinating nodes
    ret = _prop_map.get<std::string>( eirods::RESOURCE_PATH, vault_path );
    if(!ret.ok()) {
        std::stringstream msg;
        msg << "resource has no vault path.";
        result = ERROR(-1, msg.str());
    } else {
        if(_phy_path.compare(0, 1, "/") != 0 &&
           _phy_path.compare(0, vault_path.size(), vault_path) != 0) {
            _ret_string  = vault_path;
            _ret_string += _phy_path;
        } else {
            // The physical path already contains the vault path
            _ret_string = _phy_path;
        }
    }
    
    return result;

} // hpss_generate_full_path

// =-=-=-=-=-=-=-
/// @brief update the physical path in the file object
eirods::error hpss_check_path( 
    eirods::resource_operation_context* _ctx ) {
    // =-=-=-=-=-=-=-
    // NOTE: Must do this for all storage resources
    std::string full_path;
    eirods::error ret = hpss_generate_full_path( _ctx->prop_map(), 
                                                 _ctx->fco().physical_path(), 
                                                 full_path );
    if(!ret.ok()) {
        std::stringstream msg;
        msg << "Failed generating full path for object.";
        ret = PASSMSG(msg.str(), ret);
    } else {
        _ctx->fco().physical_path(full_path);
    }

    return ret;

} // hpss_check_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
template< typename DEST_TYPE >
eirods::error hpss_check_params_and_path(
    eirods::resource_operation_context* _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( !_ctx ) {
        std::stringstream msg;
        msg << "null resource context";
        result = ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    } 
  
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx->valid< DEST_TYPE >(); 
    if( !ret.ok() ) { 
        std::stringstream msg;
        msg << "resource context is invalid.";
        result = PASSMSG( msg.str(), ret );
    } else {
        result = hpss_check_path( _ctx );
    }

    return result;

} // hpss_check_params_and_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
eirods::error hpss_check_params_and_path(
    eirods::resource_operation_context* _ctx ) {
    
    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( !_ctx ) {
        result = ERROR( SYS_INVALID_INPUT_PARAM, "hpss_check_params_and_path - null resource_property_map" );
    } 
  
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx->valid(); 
    if( !ret.ok() ) { 
        result = PASSMSG( "hpss_check_params_and_path - resource context is invalid", ret );
    } else {
        result = hpss_check_path( _ctx );
    }

    return result;

} // hpss_check_params_and_path

// =-=-=-=-=-=-=- 
//@brief Recursively make all of the dirs in the path
eirods::error hpss_file_mkdir_r( 
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
            int status = hpss_Mkdir( const_cast<char*>( subdir.c_str() ), mode );
            
            // =-=-=-=-=-=-=-
            // handle error cases
            if( status < 0 && (-status) != EEXIST && (-status) != EACCES ) {
                std::stringstream msg;
                msg << "mkdir error for ";
                msg << subdir;
                msg << ", errno = ";
                msg << strerror( -status );
                msg << ", status = ";
                msg << -status;
                    
                result = ERROR( -status, msg.str() );
            }
        }
        if(pos == std::string::npos) {
            done = true;
        }
    }
    
    return result;

} // hpss_file_mkdir_r

extern "C" {
    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // =-=-=-=-=-=-=-
    /// @brief used for authentication to the hpss server
    ///        for this agent / session
    eirods::error hpss_start_operation( 
        eirods::resource_property_map& _prop_map,
        eirods::resource_child_map&    _cmap ) {
        // =-=-=-=-=-=-=-
        // get the keytab property
        std::string keytab;
        eirods::error err = _prop_map.get< std::string >( "keytab", keytab );
        if( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // get the user property
        std::string user;
        err = _prop_map.get< std::string >( "user", user );
        if( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // call the login
	int status = hpss_SetLoginCred( const_cast<char*>( user.c_str() ),
                                        hpss_authn_mech_unix,
                                        hpss_rpc_cred_client,
                                        hpss_rpc_auth_type_keytab,
                                        const_cast<char*>( keytab.c_str() ) );
        if( status < 0 ) {
	    std::stringstream msg;
            msg << "Could not authenticate [";
            msg << user;
            msg << "] using keytab [ ";
            msg << keytab;
            msg << "]";
            return ERROR( status, msg.str() );
	}

        // =-=-=-=-=-=-=-
        // win
        return SUCCESS();

    } // hpss_start_operation

    // =-=-=-=-=-=-=-
    /// @brief used for releasing authentication to the hpss server
    ///        for this agent / session
    eirods::error hpss_stop_operation( 
                  eirods::resource_property_map& _prop_map,
                  eirods::resource_child_map&    _cmap ) {
        //hpss_PurgeLoginCred();
        return SUCCESS();

    } // hpss_stop_operation

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

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error hpss_file_create_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        hpss_cos_hints_t hints_in, hints_out;
        hpss_cos_priorities_t hints_pri;

        memset(&hints_in,  0, sizeof(hints_in));
        memset(&hints_out, 0, sizeof(hints_out));
        memset(&hints_pri, 0, sizeof(hints_pri));
#if 0
        // =-=-=-=-=-=-=-
        // extract the rule results, this could possibly be a desired
        // Class Of Service for the HPSS system.
        std::string rule_results = _ctx->rule_results();
        if( !rule_results.empty() ) {
	    hints_in.COSId = boost::lexical_cast<unsigned int>( rule_results );
	    rodsLog( LOG_NOTICE, "HPSS - COS Hint for Create :: [%d]", hints_in.COSId );
        }
#endif
        // =-=-=-=-=-=-=-
        // make call to umask & open for create
        mode_t myMask = umask((mode_t) 0000);
        rodsLog( LOG_NOTICE, "XXXX - hpss_file_create_plugin :: A" );
        char* phypath = strdup(fco.physical_path().c_str());
        int    fd     = hpss_Open( phypath, O_RDWR|O_CREAT|O_EXCL,
                                   fco.mode(), &hints_in, &hints_pri, &hints_out );
        rodsLog( LOG_NOTICE, "XXXX - hpss_file_create_plugin :: B" );

        // =-=-=-=-=-=-=-
        // reset the old mask 
        (void) umask((mode_t) myMask);
                
        // =-=-=-=-=-=-=-
        // cache file descriptor in out-variable
        fco.file_descriptor( fd );
                        
        // =-=-=-=-=-=-=-
        // trap error case with bad fd
        if( fd < 0 ) {
            int status = - fd;
                        
            // =-=-=-=-=-=-=-
            // WARNING :: Major Assumptions are made upstream and use the FD also as a
            //         :: Status, if this is not done EVERYTHING BREAKS!!!!111one
            fco.file_descriptor( status );
                        
            std::stringstream msg;
            msg << "hpss_file_create_plugin: create error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( -fd );
            msg << "', status = ";
            msg << status;
 
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // declare victory!
        return CODE( fd );

    } // hpss_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error hpss_file_open_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // extract COS Hint from the open rule as a user defined
        // policy
        hpss_cos_hints_t hints_in; 
        hpss_cos_hints_t hints_out; 
        hpss_cos_priorities_t hints_pri;

        memset(&hints_in, 0, sizeof(hints_in));
        memset(&hints_out, 0, sizeof(hints_out));
        memset(&hints_pri, 0, sizeof(hints_pri));


        // =-=-=-=-=-=-=-
        // make call to open
        int fd = hpss_Open( const_cast<char*>( fco.physical_path().c_str() ), O_RDWR|O_EXCL,
                            fco.mode(), &hints_in, &hints_pri, &hints_out );
                        
        // =-=-=-=-=-=-=-
        // cache status in the file object
        fco.file_descriptor( fd );

        // =-=-=-=-=-=-=-
        // did we still get an error?
        if ( fd < 0 ) {
            //fd = UNIX_FILE_OPEN_ERR - errno;
                        
            std::stringstream msg;
            msg << "hpss_file_open_plugin: open error for ";
            msg << fco.physical_path();
            msg << ", errno = ";
            msg << strerror( -fd );
            msg << ", status = ";
            msg << fd;
 
            return ERROR( fd, msg.str() );
        }
        
        // =-=-=-=-=-=-=-
        // declare victory!
        return CODE( fd );

    } // hpss_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error hpss_file_read_plugin( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to read
        ssize_t status = hpss_Read( fco.file_descriptor(), _buf, _len );

        // =-=-=-=-=-=-=-
        // pass along an error if it was not successful
        if( status < 0 ) {
            status = UNIX_FILE_READ_ERR - errno;
                        
            std::stringstream msg;
            msg << "hpssFileReadPlugin - read error fd = ";
            msg << fco.file_descriptor();
            msg << ", errno = ";
            msg << strerror( -status );
            return ERROR( status, msg.str() );
        }
                
        // =-=-=-=-=-=-=-
        // win!
        return CODE( status );

    } // hpss_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error hpss_file_write_plugin( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
         
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
                
        // =-=-=-=-=-=-=-
        // make the call to write
        ssize_t status = hpss_Write( fco.file_descriptor(), _buf, _len );

        // =-=-=-=-=-=-=-
        // pass along an error if it was not successful
        if (status < 0) {
            status = UNIX_FILE_WRITE_ERR - errno;
                        
            std::stringstream msg;
            msg << "hpss_file_write_plugin - write fd = ";
            msg << fco.file_descriptor();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
                
        // =-=-=-=-=-=-=-
        // win!
        return CODE( status );

    } // hpss_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error hpss_file_close_plugin(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-                               
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to close
        int status = hpss_Close( fco.file_descriptor() );

        // =-=-=-=-=-=-=-
        // log any error
        if( status != 0 ) {
            status = UNIX_FILE_CLOSE_ERR - errno;
                        
            std::stringstream msg;
            msg << "hpssFileClosePlugin: close error, ";
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error hpss_file_unlink_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to unlink      
        int status = hpss_Unlink( const_cast<char*>( fco.physical_path().c_str() ) );

        // =-=-=-=-=-=-=-
        // error handling
        if( status < 0 ) {
            status = UNIX_FILE_UNLINK_ERR - errno;
                        
            std::stringstream msg;
            msg << "hpss_file_unlink_plugin: unlink error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_unlink_plugin

    // =-=-=-=-=-=-=-
    //
    eirods::error hpss_stat_to_stat( struct stat* _stat, hpss_stat_t* _hpss_stat ) {

        if( !_stat ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "invalid stat" );
        }

        if( !_hpss_stat ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "invalid hpss stat" );
        }

        _stat->st_dev     = _hpss_stat->st_dev;
        _stat->st_ino     = _hpss_stat->st_ino;
        _stat->st_mode    = _hpss_stat->st_mode; // ????
        _stat->st_nlink   = _hpss_stat->st_nlink;
        _stat->st_uid     = _hpss_stat->st_uid;
        _stat->st_gid     = _hpss_stat->st_gid;
        _stat->st_rdev    = _hpss_stat->st_rdev;
        _stat->st_size    = _hpss_stat->st_size;
        _stat->st_blksize = _hpss_stat->st_blksize;
        _stat->st_blocks  = _hpss_stat->st_blocks;
        _stat->st_atime   = _hpss_stat->st_atime_n;
        _stat->st_mtime   = _hpss_stat->st_mtime_n;
        _stat->st_ctime   = _hpss_stat->st_ctime_n;

        return SUCCESS();

    } // hpss_stat_to_stat

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error hpss_file_stat_plugin( 
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) { 
        // =-=-=-=-=-=-=-
        // NOTE:: this function assumes the object's physical path is 
        //        correct and should not have the vault path 
        //        prepended - hcj
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "hpss_file_stat_plugin - invalid resource context" );
        }
         
        eirods::error ret = _ctx->valid(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to stat
        hpss_stat_t hpss_stat;
        memset( &hpss_stat, 0, sizeof( hpss_stat ) );
        int status = hpss_Stat( const_cast<char*>( fco.physical_path().c_str() ), &hpss_stat );
        hpss_stat_to_stat( _statbuf, &hpss_stat );
 
        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_stat_plugin: stat error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
                
        return CODE( status );

    } // hpss_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error hpss_file_fstat_plugin( 
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to fstat
        hpss_stat_t hpss_stat;
        int status = hpss_Fstat( fco.file_descriptor(), &hpss_stat );
        hpss_stat_to_stat( _statbuf, &hpss_stat );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_fstat_plugin: fstat error for ";
            msg << fco.file_descriptor();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;

            return ERROR( status, msg.str() );

        } // if
           
        return CODE( status );

    } // hpss_file_fstat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error hpss_file_lseek_plugin( 
        eirods::resource_operation_context* _ctx,
        hpss_off_t                          _offset, 
        int                                 _whence ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to lseek       
        hpss_off_t status = hpss_Lseek( fco.file_descriptor(),  _offset, _whence );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_lseek_plugin: lseek error for ";
            msg << fco.file_descriptor();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error hpss_file_fsync_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to fsync   
        // This is a no-op that will return 0 as long
        // as the hpss file descriptor is valid.    
        int status = hpss_Fsync( fco.file_descriptor() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_fsync_plugin: fsync error for ";
            msg << fco.file_descriptor();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_fsync_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error hpss_file_mkdir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=- 
        // NOTE :: this function assumes the object's physical path is correct and 
        //         should not have the vault path prepended - hcj
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "hpss_file_mkdir_plugin - invalid resource context" );
        }
         
        eirods::error ret = _ctx->valid< eirods::collection_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object& coll_obj = dynamic_cast< eirods::collection_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // make the call to mkdir & umask
        mode_t myMask = umask( ( mode_t ) 0000 );
        int    status =  ( hpss_Mkdir( const_cast<char*>( coll_obj.physical_path().c_str() ), coll_obj.mode() ) );

        // =-=-=-=-=-=-=-
        // reset the old mask 
        umask( ( mode_t ) myMask );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            if (-status != EEXIST) {
                std::stringstream msg;
                msg << "hpss_file_mkdir_plugin: mkdir error for ";
                msg << coll_obj.physical_path();
                msg << ", errno = '";
                msg << strerror( -status );
                msg << "', status = ";
                msg << status;
                                
                return ERROR( status, msg.str() );

            } // if errno != EEXIST

        } // if status < 0 

        return CODE( status );

    } // hpss_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rmdir
    eirods::error hpss_file_rmdir_plugin( 
        eirods::resource_operation_context* _ctx ) { 

        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& fco = _ctx->fco();
        
        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = hpss_Rmdir( const_cast<char*>( fco.physical_path().c_str() ) );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_rmdir_plugin: mkdir error for ";
            msg << fco.physical_path();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << -status;
                        
            return ERROR( -status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error hpss_file_opendir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path< eirods::collection_object >( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object& coll_obj = dynamic_cast< eirods::collection_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // make the callt to opendir
        int fd = hpss_Opendir( const_cast< char* > ( coll_obj.physical_path().c_str() ));

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( fd < 0 ) {
            // =-=-=-=-=-=-=-
            // cache status in out variable

            std::stringstream msg;
            msg << "hpss_file_opendir_plugin: opendir error for ";
            msg << coll_obj.physical_path();
            msg << ", errno = ";
            msg << strerror( -fd );
            msg << ", status = ";
            msg << fd;
                        
            return ERROR( -fd, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // cache dir_ptr & status in out variables
        coll_obj.directory_pointer( reinterpret_cast < DIR* > ( fd ));

        return SUCCESS();

    } // hpss_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error hpss_file_closedir_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path< eirods::collection_object >( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object& coll_obj = dynamic_cast< eirods::collection_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // make the callt to opendir
        int status = hpss_Closedir( reinterpret_cast< long >( coll_obj.directory_pointer() ) );
                        
        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_closedir_plugin: closedir error for ";
            msg << coll_obj.physical_path();
            msg << ", errno = '";
            msg << strerror( -status );
            msg << "', status = ";
            msg << -status;
                        
            return ERROR( -status, msg.str() );
        }

        return CODE( status );

    } // hpss_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // convert an hpss dirent to a rods dirent
    eirods::error hpss_dirent_to_rods_dirent( 
        hpss_dirent_t&     _hpss_dirent, 
        struct rodsDirent& _rods_dirent ) {
        // =-=-=-=-=-=-=-=-
        // 
        _rods_dirent.d_offset = low32m( _hpss_dirent.d_offset ); /* offset after this entry */
        memcpy( &_rods_dirent.d_handle, &_hpss_dirent.d_handle, sizeof( ns_ObjHandle_t ) ); // NOTE:: unholy hack.
        _rods_dirent.d_reclen = _hpss_dirent.d_reclen;           /* length of this record */
        _rods_dirent.d_namlen = _hpss_dirent.d_namelen;          /* length of d_name */
        strncpy( _hpss_dirent.d_name, _rods_dirent.d_name, strlen( _hpss_dirent.d_name ) );

        return SUCCESS();

    } // hpss_dirent_to_rods_dirent

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error hpss_file_readdir_plugin( 
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _rods_dirent ) {
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path< eirods::collection_object >( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object& coll_obj = dynamic_cast< eirods::collection_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // zero out errno?
        errno = 0;

        // =-=-=-=-=-=-=-
        // make the call to readdir
        hpss_dirent_t hpss_dirent;
        int status = hpss_Readdir( reinterpret_cast<long>( coll_obj.directory_pointer() ), &hpss_dirent );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            if( 0 == strlen( hpss_dirent.d_name ) ) {
                // =-=-=-=-=-=-=-
                // we have hit the end of directory listing
                return CODE( -1 );

            } else {
                // =-=-=-=-=-=-=-
                // cache status in out variable
                std::stringstream msg;
                msg << "hpss_file_readdir_plugin: closedir error, status = ";
                msg << status;
                msg << ", errno = '";
                msg << strerror( -status );
                msg << "'";
                                
                return ERROR( -status, msg.str() );
            }
        } else {
            // =-=-=-=-=-=-=-
            // alloc dirent as necessary
            if( !( *_rods_dirent ) ) {
                (*_rods_dirent ) = new rodsDirent_t;
            }

            // =-=-=-=-=-=-=-
            // convert standard dirent to rods dirent struct
            hpss_dirent_to_rods_dirent( hpss_dirent, *(*_rods_dirent) );

            return CODE( 0 );
        }

    } // hpss_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rename
    eirods::error hpss_file_rename_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        // =-=-=-=-=-=-=- 
        // manufacture a new path from the new file name 
        std::string new_full_path;
        ret = hpss_generate_full_path( _ctx->prop_map(), _new_file_name, new_full_path );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Unable to generate full path for destinate file: \"" << _new_file_name << "\"";
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
        ret = hpss_file_mkdir_r( _ctx->comm(), "", new_path.c_str(), 0750 );
        if(!ret.ok()) {

            std::stringstream msg;
            msg << "hpss_file_rename_plugin: mkdir error for ";
            msg << new_path;

            return PASSMSG( msg.str(), ret);

        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = hpss_Rename( const_cast<char*>( fco.physical_path().c_str() ), 
                                  const_cast<char*>( new_full_path.c_str() ) );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            std::stringstream msg;
            msg << "hpss_file_rename_plugin: rename error for ";
            msg <<  fco.physical_path();
            msg << " to ";
            msg << new_full_path;
            msg << ", errno = ";
            msg << strerror( -status );
            msg << ", status = ";
            msg <<  -status ;
                        
            return ERROR( -status, msg.str() );

        }

        return CODE( status );

    } // hpss_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error hpss_file_get_fsfreespace_plugin( 
        eirods::resource_operation_context* _ctx ) { 
        return CODE( SYS_NOT_SUPPORTED );

    } // hpss_file_get_fsfreespace_plugin

    // =-=-=-=-=-=-=-
    // unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error hpss_file_stagetocache_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        return CODE( SYS_NOT_SUPPORTED );

    } // hpss_file_stagetocache_plugin

    // =-=-=-=-=-=-=-
    // unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error hpss_file_synctoarch_plugin( 
        eirods::resource_operation_context* _ctx,
        char*                               _cache_file_name ) {
        return CODE( SYS_NOT_SUPPORTED );

    } // hpss_file_synctoarch_plugin

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error hpss_file_redirect_create( 
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
            return PASSMSG( "hpss_file_redirect_create - failed to get 'status' property", get_ret );
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
            return PASSMSG( "hpss_file_redirect_create - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // hpss_file_redirect_create

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error hpss_file_redirect_open( 
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
            return PASSMSG( "hpss_file_redirect_open - failed to get 'status' property", get_ret );
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
            return PASSMSG( "hpss_file_redirect_open - failed to get 'location' property", get_ret );
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
        // initially set vote to 0.0
        _out_vote = 0.0;

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

    } // redirect_get

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    eirods::error hpss_file_redirect_plugin( 
        eirods::resource_operation_context* _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {

        // =-=-=-=-=-=-=-
        // check the context pointer
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "hpss_file_mkdir_plugin - invalid resource context" );
        }
         
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx->valid< eirods::file_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_opr ) {
            return ERROR( -1, "hpss_file_redirect_plugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "hpss_file_redirect_plugin - null operation" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "hpss_file_redirect_plugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "hpss_file_redirect_plugin - null outgoing vote" );
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
            msg << "hpss_file_redirect_plugin - failed in get property for name";
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
            return hpss_file_redirect_open( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return hpss_file_redirect_create( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "hpss_file_redirect_plugin - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // hpss_file_redirect_plugin

    eirods::error hpss_file_registered_plugin(
        eirods::resource_operation_context* _ctx)
    {
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    eirods::error hpss_file_unregistered_plugin(
        eirods::resource_operation_context* _ctx)
    {
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    eirods::error hpss_file_modified_plugin(
        eirods::resource_operation_context* _ctx)
    {
        // Check the operation parameters and update the physical path
        eirods::error ret = hpss_check_params_and_path(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class unixfilesystem_resource : public eirods::resource {
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
                rodsLog( LOG_NOTICE, "unixfilesystem_resource::post_disconnect_maintenance_operation - [%s]", 
                name_.c_str() );
                return SUCCESS();
            }

        private:
            std::string name_;

        }; // class maintenance_operation

    public:
        unixfilesystem_resource( const std::string& _inst_name, 
                                 const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
           
            // =-=-=-=-=-=-=-
            // set the start op for authentication 
            set_start_operation( "hpss_start_operation" );
            set_stop_operation( "hpss_stop_operation" );

            // =-=-=-=-=-=-=-
            // parse out key value pairs into the property list
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
            return ERROR( -1, "nop" );
        }

    }; // class unixfilesystem_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create unixfilesystem_resource
        unixfilesystem_resource* resc = new unixfilesystem_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "hpss_file_create_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "hpss_file_open_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "hpss_file_read_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "hpss_file_write_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "hpss_file_close_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "hpss_file_unlink_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "hpss_file_stat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "hpss_file_fstat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "hpss_file_lseek_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "hpss_file_fsync_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "hpss_file_mkdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "hpss_file_rmdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "hpss_file_opendir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "hpss_file_closedir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "hpss_file_readdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "hpss_file_rename_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "hpss_file_get_fsfreespace_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "hpss_file_stagetocache_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "hpss_file_synctoarch_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "hpss_file_registered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "hpss_file_unregistered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "hpss_file_modified_plugin" );

        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "hpss_file_redirect_plugin" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



