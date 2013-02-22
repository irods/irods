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





extern "C" {

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // =-=-=-=-=-=-=-
    // 2. Define utility functions that the operations might need
    // =-=-=-=-=-=-=-

    // NOTE: All storage resources must do this on the physical path stored in the file object and then update the file object's
    // physical path with the full path
    
    /// @brief Generates a full path name from the partial physical path and the specified resource's vault path
    eirods::error unixGenerateFullPath(
        eirods::resource_property_map* _prop_map,
        const std::string& physical_path,
        std::string& ret_string)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string vault_path;
        // TODO - getting vault path by property will not likely work for coordinating nodes
        ret = _prop_map->get<std::string>("path", vault_path);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource has no vault path.";
            result = ERROR(-1, msg.str());
        } else {
            if(physical_path.compare(0, 1, "/") != 0 &&
               physical_path.compare(0, vault_path.size(), vault_path) != 0) {
                ret_string = vault_path;
                ret_string += physical_path;
            } else {
                // The physical path already contains the vault path
                ret_string = physical_path;
            }
        }
        return result;
    }

    /// @brief Checks the basic operation parameters and updates the physical path in the file object
    eirods::error unixCheckParamsAndPath(
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map* _cmap,
        eirods::first_class_object* _object)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "unixFileCreatePlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "unixFileCreatePlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "unixFileCreatePlugin - null first_class_object" );
        } else {

            // NOTE: Must do this for all storage resources
            std::string full_path;
            eirods::error ret = unixGenerateFullPath(_prop_map, _object->physical_path(), full_path);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - Failed generating full path for object.";
                result = PASSMSG(msg.str(), ret);
            } else {
                _object->physical_path(full_path);
            }
        }
        return result;
    }

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
    eirods::error unixFileCreatePlugin( rsComm_t*                      _comm,
                                        eirods::resource_property_map* _prop_map,
                                        eirods::resource_child_map*    _cmap, 
                                        eirods::first_class_object*    _object,
                                        std::string*                   _results ) {

        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make call to umask & open for create
        mode_t myMask = umask((mode_t) 0000);
        int    fd     = open( _object->physical_path().c_str(), O_RDWR|O_CREAT|O_EXCL, _object->mode() );

        // =-=-=-=-=-=-=-
        // reset the old mask 
        (void) umask((mode_t) myMask);
                
        // =-=-=-=-=-=-=-
        // if we got a 0 descriptor, try again
        if( fd == 0 ) {
        
            close (fd);
            rodsLog( LOG_NOTICE, "unixFileCreatePlugin: 0 descriptor" );
            open ("/dev/null", O_RDWR, 0);
            fd = open( _object->physical_path().c_str(), O_RDWR|O_CREAT|O_EXCL, _object->mode() );
        }

        // =-=-=-=-=-=-=-
        // cache file descriptor in out-variable
        _object->file_descriptor( fd );
                        
        // =-=-=-=-=-=-=-
        // trap error case with bad fd
        if( fd < 0 ) {
            int status = UNIX_FILE_CREATE_ERR - errno;
                        
            // =-=-=-=-=-=-=-
            // WARNING :: Major Assumptions are made upstream and use the FD also as a
            //         :: Status, if this is not done EVERYTHING BREAKS!!!!111one
            _object->file_descriptor( status );
                        
            std::stringstream msg;
            msg << "unixFileCreatePlugin: create error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
 
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // declare victory!
        return CODE( fd );

    } // unixFileCreatePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error unixFileOpenPlugin( rsComm_t*                      _comm,
                                      eirods::resource_property_map* _prop_map, 
                                      eirods::resource_child_map*    _cmap, 
                                      eirods::first_class_object*    _object,
                                      std::string*                   _results ) {

        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // handle OSX weirdness...
        int flags = _object->flags();

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
        int fd = open( _object->physical_path().c_str(), flags, _object->mode() );

        // =-=-=-=-=-=-=-
        // if we got a 0 descriptor, try again
        if( fd == 0 ) {
            close (fd);
            rodsLog( LOG_NOTICE, "unixFileOpenPlugin: 0 descriptor" );
            open ("/dev/null", O_RDWR, 0);
            fd = open( _object->physical_path().c_str(), flags, _object->mode() );
        }       
                        
        // =-=-=-=-=-=-=-
        // cache status in the file object
        _object->file_descriptor( fd );

        // =-=-=-=-=-=-=-
        // did we still get an error?
        if ( fd < 0 ) {
            fd = UNIX_FILE_OPEN_ERR - errno;
                        
            std::stringstream msg;
            msg << "unixFileOpenPlugin: open error for ";
            msg << _object->physical_path();
            msg << ", errno = ";
            msg << strerror( errno );
            msg << ", status = ";
            msg << fd;
            msg << ", flags = ";
            msg << flags;
 
            return ERROR( fd, msg.str() );
        }
                
        // =-=-=-=-=-=-=-
        // declare victory!
        return CODE( fd );

    } // unixFileOpenPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error unixFileReadPlugin( rsComm_t*                      _comm,
                                      eirods::resource_property_map* _prop_map, 
                                      eirods::resource_child_map*    _cmap,
                                      eirods::first_class_object*    _object,
                                      std::string*                   _results,
                                      void*                          _buf, 
                                      int                            _len ) {
                                                                          
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to read
        int status = read( _object->file_descriptor(), _buf, _len );

        // =-=-=-=-=-=-=-
        // pass along an error if it was not successful
        if( status < 0 ) {
            status = UNIX_FILE_READ_ERR - errno;
                        
            std::stringstream msg;
            msg << "unixFileReadPlugin - read error fd = ";
            msg << _object->file_descriptor();
            msg << ", errno = ";
            msg << strerror( errno );
            return ERROR( status, msg.str() );
        }
                
        // =-=-=-=-=-=-=-
        // win!
        return CODE( status );

    } // unixFileReadPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error unixFileWritePlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map*  _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results,
                                       void*                          _buf, 
                                       int                            _len ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to write
        int status = write( _object->file_descriptor(), _buf, _len );

        // =-=-=-=-=-=-=-
        // pass along an error if it was not successful
        if (status < 0) {
            status = UNIX_FILE_WRITE_ERR - errno;
                        
            std::stringstream msg;
            msg << "unixFileWritePlugin - write fd = ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
                
        // =-=-=-=-=-=-=-
        // win!
        return CODE( status );

    } // unixFileWritePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error unixFileClosePlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map*  _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results ) {
                                       
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to close
        int status = close( _object->file_descriptor() );

        // =-=-=-=-=-=-=-
        // log any error
        if( status < 0 ) {
            status = UNIX_FILE_CLOSE_ERR - errno;
                        
            std::stringstream msg;
            msg << "unixFileClosePlugin: close error, ";
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileClosePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error unixFileUnlinkPlugin( rsComm_t*                      _comm,
                                        eirods::resource_property_map* _prop_map, 
                                        eirods::resource_child_map*    _cmap,
                                        eirods::first_class_object*    _object,
                                        std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to unlink      
        int status = unlink( _object->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // error handling
        if( status < 0 ) {
            status = UNIX_FILE_UNLINK_ERR - errno;
                        
            std::stringstream msg;
            msg << "unixFileUnlinkPlugin: unlink error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileUnlinkPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error unixFileStatPlugin( rsComm_t*                      _comm,
                                      eirods::resource_property_map* _prop_map, 
                                      eirods::resource_child_map*    _cmap,
                                      eirods::first_class_object*    _object,
                                      std::string*                   _results,
                                      struct stat*                   _statbuf ) { 

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_property_map" );
        } else if( !_cmap ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_child_map" );
        } else if( !_object ) {
            return ERROR( -1, "unixFileCreatePlugin - null first_class_object" );
        }

        // NOTE NOTE NOTE this function assumes the object's physical path is correct and should not have the vault path prepended -
        // hcj
        
        // =-=-=-=-=-=-=-
        // make the call to stat
        int status = stat( _object->physical_path().c_str(), _statbuf );

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
            msg << "unixFileStatPlugin: stat error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
                
        return CODE( status );

    } // unixFileStatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error unixFileFstatPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results,
                                       struct stat*                   _statbuf ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to fstat
        int status = fstat( _object->file_descriptor(), _statbuf );

        // =-=-=-=-=-=-=-
        // if the file can't be accessed due to permission denied 
        // try again using root credentials.
#ifdef RUN_SERVER_AS_ROOT
        if (status < 0 && errno == EACCES && isServiceUserSet()) {
            if (changeToRootUser() == 0) {
                status = fstat( _object->file_descriptor(), statbuf );
                changeToServiceUser();
            }
        }
#endif

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_STAT_ERR - errno;
 
            std::stringstream msg;
            msg << "unixFileFstatPlugin: fstat error for ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;

            return ERROR( status, msg.str() );

        } // if
           
        return CODE( status );

    } // unixFileFstatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error unixFileLseekPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results,
                                       size_t                         _offset, 
                                       int                            _whence ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to lseek       
        size_t status = lseek( _object->file_descriptor(),  _offset, _whence );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_LSEEK_ERR - errno;
 
            std::stringstream msg;
            msg << "unixFileLseekPlugin: lseek error for ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileLseekPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error unixFileFsyncPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to fsync       
        int status = fsync( _object->file_descriptor() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_LSEEK_ERR - errno;
 
            std::stringstream msg;
            msg << "unixFileFsyncPlugin: fsync error for ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileFsyncPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error unixFileMkdirPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,  
                                       std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_property_map" );
        } else if( !_cmap ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_child_map" );
        } else if( !_object ) {
            return ERROR( -1, "unixFileCreatePlugin - null first_class_object" );
        }

        // NOTE NOTE NOTE this function assumes the object's physical path is correct and should not have the vault path prepended -
        // hcj
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object* coll_obj = dynamic_cast< eirods::collection_object* >( _object );
        if( !coll_obj ) {
            return ERROR( -1, "failed to cast first_class_object to collection_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to mkdir & umask
        mode_t myMask = umask( ( mode_t ) 0000 );
        int    status = mkdir( coll_obj->physical_path().c_str(), coll_obj->mode() );

        // =-=-=-=-=-=-=-
        // reset the old mask 
        umask( ( mode_t ) myMask );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_MKDIR_ERR - errno;
 
            if (errno != EEXIST) {
                std::stringstream msg;
                msg << "unixFileMkdirPlugin: mkdir error for ";
                msg << coll_obj->physical_path();
                msg << ", errno = '";
                msg << strerror( errno );
                msg << "', status = ";
                msg << status;
                                
                return ERROR( status, msg.str() );

            } // if errno != EEXIST

        } // if status < 0 

        return CODE( status );

    } // unixFileMkdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error unixFileChmodPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = chmod( _object->physical_path().c_str(), _object->mode() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_CHMOD_ERR - errno;
 
            std::stringstream msg;
            msg << "unixFileChmodPlugin: chmod error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        } // if

        return CODE( status );

    } // unixFileChmodPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error unixFileRmdirPlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = rmdir( _object->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_RMDIR_ERR - errno;
 
            std::stringstream msg;
            msg << "unixFileRmdirPlugin: mkdir error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( errno, msg.str() );
        }

        return CODE( status );

    } // unixFileRmdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error unixFileOpendirPlugin( rsComm_t*                      _comm,
                                         eirods::resource_property_map* _prop_map, 
                                         eirods::resource_child_map*    _cmap,
                                         eirods::first_class_object*    _object,
                                         std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object* coll_obj = dynamic_cast< eirods::collection_object* >( _object );
        if( !coll_obj ) {
            return ERROR( -1, "failed to cast first_class_object to collection_object" );
        }

        // =-=-=-=-=-=-=-
        // make the callt to opendir
        DIR* dir_ptr = opendir( coll_obj->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // if the directory can't be accessed due to permission
        // denied try again using root credentials.            
#ifdef RUN_SERVER_AS_ROOT
        if( dir_ptr == NULL && errno == EACCES && isServiceUserSet() ) {
            if (changeToRootUser() == 0) {
                dir_ptr = opendir ( coll_obj->physical_path().c_str() );
                changeToServiceUser();
            } // if
        } // if
#endif

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( NULL == dir_ptr ) {
            // =-=-=-=-=-=-=-
            // cache status in out variable
            int status = UNIX_FILE_OPENDIR_ERR - errno;

            std::stringstream msg;
            msg << "unixFileOpendirPlugin: opendir error for ";
            msg << coll_obj->physical_path();
            msg << ", errno = ";
            msg << strerror( errno );
            msg << ", status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // cache dir_ptr & status in out variables
        coll_obj->directory_pointer( dir_ptr );

        return SUCCESS();

    } // unixFileOpendirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error unixFileClosedirPlugin( rsComm_t*                      _comm,
                                          eirods::resource_property_map* _prop_map, 
                                          eirods::resource_child_map*    _cmap,
                                          eirods::first_class_object*    _object,
                                          std::string*                   _results ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object* coll_obj = dynamic_cast< eirods::collection_object* >( _object );
        if( !coll_obj ) {
            return ERROR( -1, "failed to cast first_class_object to collection_object" );
        }

        // =-=-=-=-=-=-=-
        // make the callt to opendir
        int status = closedir( coll_obj->directory_pointer() );
                        
        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_CLOSEDIR_ERR - errno;

            std::stringstream msg;
            msg << "unixFileClosedirPlugin: closedir error for ";
            msg << coll_obj->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileClosedirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error unixFileReaddirPlugin( rsComm_t*                      _comm,
                                         eirods::resource_property_map* _prop_map, 
                                         eirods::resource_child_map*    _cmap,
                                         eirods::first_class_object*    _object,
                                         std::string*                   _results,
                                         struct rodsDirent**            _dirent_ptr ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::collection_object* coll_obj = dynamic_cast< eirods::collection_object* >( _object );
        if( !coll_obj ) {
            return ERROR( -1, "failed to cast first_class_object to collection_object" );
        }

        // =-=-=-=-=-=-=-
        // zero out errno?
        errno = 0;

        // =-=-=-=-=-=-=-
        // make the call to readdir
        struct dirent * tmp_dirent = readdir( coll_obj->directory_pointer() );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( tmp_dirent == NULL ) {
            if( errno == 0 ) { // just the end 

                // =-=-=-=-=-=-=-
                // cache status in out variable
                return CODE( -1 );
            } else {

                // =-=-=-=-=-=-=-
                // cache status in out variable
                int status = UNIX_FILE_READDIR_ERR - errno;

                std::stringstream msg;
                msg << "unixFileReaddirPlugin: closedir error, status = ";
                msg << status;
                msg << ", errno = '";
                msg << strerror( errno );
                msg << "'";
                                
                return ERROR( status, msg.str() );
            }
        } else {
            // =-=-=-=-=-=-=-
            // alloc dirent as necessary
            if( !( *_dirent_ptr ) ) {
                (*_dirent_ptr ) = new rodsDirent_t;
            }

            // =-=-=-=-=-=-=-
            // convert standard dirent to rods dirent struct
            int status = direntToRodsDirent( (*_dirent_ptr), tmp_dirent );
            if( status < 0 ) {

            }

#if defined(solaris_platform)
            rstrcpy( (*_dirent_ptr)->d_name, tmp_dirent->d_name, MAX_NAME_LEN );
#endif

            return CODE( 0 );
        }

    } // unixFileReaddirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error unixFileStagePlugin( rsComm_t*                      _comm,
                                       eirods::resource_property_map* _prop_map, 
                                       eirods::resource_child_map*    _cmap,
                                       eirods::first_class_object*    _object,
                                       std::string*                   _results  ) {
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
#ifdef SAMFS_STAGE
        int status = sam_stage (path, "i");

        if (status < 0) {
            _status = UNIX_FILE_STAGE_ERR - errno;
            rodsLog( LOG_NOTICE,"unixFileStage: sam_stage error, status = %d\n", (*_status) );
            return ERROR( false, errno, "unixFileStage: sam_stage error" );
        }

        return CODE( 0 );
#else
        return CODE( 0 );
#endif
    } // unixFileStagePlugin

    /**
     * @brief Recursively make all of the dirs in the path
     */
    eirods::error
    unixFileMkDirR(rsComm_t*                      _comm,
                   const std::string&             _results,
                   const std::string& path,
                   mode_t mode) {
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
    }

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error unixFileRenamePlugin( rsComm_t*                      _comm,
                                        eirods::resource_property_map* _prop_map, 
                                        eirods::resource_child_map*    _cmap,
                                        eirods::first_class_object*    _object, 
                                        std::string*                   _results,
                                        const char*                    _new_file_name ) {
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        std::string new_full_path;
        ret = unixGenerateFullPath(_prop_map, _new_file_name, new_full_path);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Unable to generate full path for destinate file: \"" << _new_file_name << "\"";
            return PASSMSG(msg.str(), ret);
        }
        // make the directories in the path to the new file
        std::string new_path = new_full_path;
        std::size_t last_slash = new_path.find_last_of('/');
        new_path.erase(last_slash);
        ret = unixFileMkDirR( _comm, "", new_path.c_str(), 0750 );
        if(!ret.ok()) {

            std::stringstream msg;
            msg << "unixFileRenamePlugin: mkdir error for ";
            msg << new_path;

            return PASSMSG( msg.str(), ret);

        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = rename( _object->physical_path().c_str(), new_full_path.c_str() );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            status = UNIX_FILE_RENAME_ERR - errno;
                                
            std::stringstream msg;
            msg << "unixFileRenamePlugin: rename error for ";
            msg <<  _object->physical_path();
            msg << " to ";
            msg << new_full_path;
            msg << ", errno = ";
            msg << strerror(errno);
            msg << ", status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // unixFileRenamePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error unixFileTruncatePlugin( rsComm_t*                      _comm,
                                          eirods::resource_property_map* _prop_map, 
                                          eirods::resource_child_map*    _cmap,
                                          eirods::first_class_object*    _object, 
                                          std::string*                   _results ) { 
                                         
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::file_object* file_obj = dynamic_cast< eirods::file_object* >( _object );
        if( !file_obj ) {
            return ERROR( -1, "failed to cast first_class_object to file_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = truncate( file_obj->physical_path().c_str(), 
                               file_obj->size() );

        // =-=-=-=-=-=-=-
        // handle any error cases
        if( status < 0 ) {
            // =-=-=-=-=-=-=-
            // cache status in out variable
            status = UNIX_FILE_TRUNCATE_ERR - errno;

            std::stringstream msg;
            msg << "unixFileTruncatePlugin: rename error for ";
            msg << file_obj->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // unixFileTruncatePlugin

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error unixFileGetFsFreeSpacePlugin( rsComm_t*                      _comm,
                                                eirods::resource_property_map* _prop_map, 
                                                eirods::resource_child_map*    _cmap,
                                                eirods::first_class_object*    _object, 
                                                std::string*                   _results ) { 
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
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
        status = statvfs( _object->physical_path().c_str(), &statbuf );
#else
#if defined(sgi_platform)
        status = statfs( _object->physical_path().c_str(), 
                         &statbuf, sizeof (struct statfs), 0 );
#else
        status = statfs( _object->physical_path().c_str(), 
                         &statbuf );
#endif
#endif

        // =-=-=-=-=-=-=-
        // handle error, if any
        if( status < 0 ) {
            status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;

            std::stringstream msg;
            msg << "unixFileGetFsFreeSpacePlugin: statfs error for ";
            msg << _object->physical_path().c_str();
            msg << ", status = ";
            msg << USER_NO_SUPPORT_ERR;

            return ERROR( USER_NO_SUPPORT_ERR, msg.str() );
        }
            
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

        return CODE( fssize );

    } // unixFileGetFsFreeSpacePlugin

    int
    unixFileCopyPlugin( int         mode, 
                        const char* srcFileName, 
                        const char* destFileName )
    {
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
                     "unixFileCopyPlugin: stat of %s error, status = %d",
                     srcFileName, status);
            return status;
        }

        inFd = open (srcFileName, O_RDONLY, 0);
        if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "unixFileCopyPlugin: open error for srcFileName %s, status = %d",
                     srcFileName, status );
            close( inFd ); // JMC cppcheck - resource
            return status;
        }

        outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
        if (outFd < 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "unixFileCopyPlugin: open error for destFileName %s, status = %d",
                     destFileName, status);
            close (inFd);
            return status;
        }

        while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
            bytesWritten = write (outFd, (void *) myBuf, bytesRead);
            if (bytesWritten <= 0) {
                status = UNIX_FILE_WRITE_ERR - errno;
                rodsLog (LOG_ERROR,
                         "unixFileCopyPlugin: write error for srcFileName %s, status = %d",
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
                      "unixFileCopyPlugin: Copied size %lld does not match source \
                             size %lld of %s",
                      bytesCopied, statbuf.st_size, srcFileName );
            return SYS_COPY_LEN_ERR;
        } else {
            return 0;
        }
    }


    // =-=-=-=-=-=-=-
    // unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error unixStageToCachePlugin( rsComm_t*                      _comm,
                                          eirods::resource_property_map* _prop_map, 
                                          eirods::resource_child_map*    _cmap,
                                          eirods::first_class_object*    _object,
                                          std::string*                   _results,
                                          const char*                    _cache_file_name ) { 
        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        int status = unixFileCopyPlugin( _object->mode(), _object->physical_path().c_str(), _cache_file_name );
        if( status < 0 ) {
            return ERROR( status, "unixStageToCachePlugin failed." );
        } else {
            return SUCCESS();
        }
    } // unixStageToCachePlugin

    // =-=-=-=-=-=-=-
    // unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error unixSyncToArchPlugin( rsComm_t*                      _comm,
                                        eirods::resource_property_map* _prop_map, 
                                        eirods::resource_child_map*    _cmap,
                                        eirods::first_class_object*    _object,
                                        std::string*                   _results,
                                        char*                          _cache_file_name ) {

        // Check the operation parameters and update the physical path
        eirods::error ret = unixCheckParamsAndPath(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        
        int status = unixFileCopyPlugin( _object->mode(), _cache_file_name, _object->physical_path().c_str() );
        if( status < 0 ) {
            return ERROR( status, "unixSyncToArchPlugin failed." );
        } else {
            return SUCCESS();
        }

    } // unixSyncToArchPlugin

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error redirect_create( 
                      eirods::resource_property_map* _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map->get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "redirect_open - failed to get 'status' property", get_ret );
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
        get_ret = _prop_map->get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "redirect_open - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // redirect_create

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error redirect_open( 
                      eirods::resource_property_map* _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map->get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "redirect_open - failed to get 'status' property", get_ret );
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
        get_ret = _prop_map->get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "redirect_open - failed to get 'location' property", get_ret );
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
    // unixRedirectPlugin - used to allow the resource to determine which host
    //                      should provide the requested operation
    eirods::error unixRedirectPlugin( rsComm_t*                      _comm,
                                      eirods::resource_property_map* _prop_map, 
                                      eirods::resource_child_map*    _cmap,
                                      eirods::first_class_object*    _object,
                                      std::string*                   _results,
                                      const std::string*             _opr,
                                      const std::string*             _curr_host,
                                      eirods::hierarchy_parser*      _out_parser,
                                      float*                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixRedirectPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixRedirectPlugin - null resource_child_map" );
        }
        if( !_opr ) {
            return ERROR( -1, "unixRedirectPlugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "unixRedirectPlugin - null operation" );
        }
        if( !_object ) {
            return ERROR( -1, "unixRedirectPlugin - null first_class_object" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "unixRedirectPlugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "unixRedirectPlugin - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::file_object* file_obj = dynamic_cast< eirods::file_object* >( _object );
        if( !file_obj ) {
            return ERROR( -1, "failed to cast first_class_object to file_object" );
        }

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        eirods::error ret = _prop_map->get< std::string >( "name", resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "unixRedirectPlugin - failed in get property for name";
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
            return redirect_open( _prop_map, (*file_obj), resc_name, (*_curr_host), (*_out_vote)  );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return redirect_create( _prop_map, (*file_obj), resc_name, (*_curr_host), (*_out_vote)  );
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "unixRedirectPlugin - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // unixRedirectPlugin

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
                rodsLog( LOG_NOTICE, "unixfilesystem_resource::post_disconnect_maintenance_operation - [%s]", name_.c_str() );
                return SUCCESS();
            }

        private:
            std::string name_;

        }; // class maintenance_operation

    public:
        unixfilesystem_resource( const std::string& _inst_name, 
                                 const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {

            if( !context_.empty() ) {
                // =-=-=-=-=-=-=-
                // tokenize context string into key/val pairs assuming a ; as a separator
                std::vector< std::string > key_vals;
                eirods::string_tokenize( _context, key_vals, ";" );

                // =-=-=-=-=-=-=-
                // tokenize each key/val pair using = as a separator and
                // add them to the property list
                std::vector< std::string >::iterator itr = key_vals.begin();
                for( ; itr != key_vals.end(); ++itr ) {

                    if( !itr->empty() ) {
                        // =-=-=-=-=-=-=-
                        // break up key and value into two strings
                        std::vector< std::string > vals;
                        eirods::string_tokenize( *itr, vals, "=" );
                        
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
                return PASSMSG( "unixfilesystem_resource::post_disconnect_maintenance_operation failed.", err );
            }

            _op = maintenance_operation( name );
            return SUCCESS();
#else
            return ERROR( -1, "nop" );
#endif
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
        resc->add_operation( "create",       "unixFileCreatePlugin" );
        resc->add_operation( "open",         "unixFileOpenPlugin" );
        resc->add_operation( "read",         "unixFileReadPlugin" );
        resc->add_operation( "write",        "unixFileWritePlugin" );
        resc->add_operation( "close",        "unixFileClosePlugin" );
        resc->add_operation( "unlink",       "unixFileUnlinkPlugin" );
        resc->add_operation( "stat",         "unixFileStatPlugin" );
        resc->add_operation( "fstat",        "unixFileFstatPlugin" );
        resc->add_operation( "fsync",        "unixFileFsyncPlugin" );
        resc->add_operation( "mkdir",        "unixFileMkdirPlugin" );
        resc->add_operation( "chmod",        "unixFileChmodPlugin" );
        resc->add_operation( "opendir",      "unixFileOpendirPlugin" );
        resc->add_operation( "readdir",      "unixFileReaddirPlugin" );
        resc->add_operation( "stage",        "unixFileStagePlugin" );
        resc->add_operation( "rename",       "unixFileRenamePlugin" );
        resc->add_operation( "freespace",    "unixFileGetFsFreeSpacePlugin" );
        resc->add_operation( "lseek",        "unixFileLseekPlugin" );
        resc->add_operation( "rmdir",        "unixFileRmdirPlugin" );
        resc->add_operation( "closedir",     "unixFileClosedirPlugin" );
        resc->add_operation( "truncate",     "unixFileTruncatePlugin" );
        resc->add_operation( "stagetocache", "unixStageToCachePlugin" );
        resc->add_operation( "synctoarch",   "unixSyncToArchPlugin" );

        resc->add_operation( "redirect",     "unixRedirectPlugin" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( "check_path_perm", 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( "create_path",     1 );//CREATE_PATH );
        resc->set_property< int >( "category",        0 );//FILE_CAT );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



