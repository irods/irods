/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

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

#define NB_READ_TOUT_SEC	60	/* 60 sec timeout */
#define NB_WRITE_TOUT_SEC	60	/* 60 sec timeout */

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

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

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error unixFileCreatePlugin( eirods::resource_property_map* 
                                        _prop_map,
                                        eirods::resource_child_map* 
                                        _cmap, 
                                        eirods::first_class_object* 
                                        _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileCreatePlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileCreatePlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make call to umask & open for create
        mode_t myMask = umask((mode_t) 0000);
        int    fd     = open( _object->physical_path().c_str(), O_RDWR|O_CREAT|O_EXCL, _object->mode() );

        std::cerr << "qqq - opened the file: \"" << _object->physical_path() << "\"" << std::endl;
        
        // =-=-=-=-=-=-=-
        // reset the old mask 
        (void) umask((mode_t) myMask);
		
        // =-=-=-=-=-=-=-
        // if we got a 0 descriptor, try again
        if( fd == 0 ) {
            std::cerr << "qqq - zero descriptor the file: \"" << _object->physical_path() << "\"" << std::endl;
        
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
			
            std::cerr << "qqq - error descriptor the file: \"" << _object->physical_path() << "\"" << std::endl;
        
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
    eirods::error unixFileOpenPlugin( eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap, 
                                      eirods::first_class_object* 
                                      _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileOpenPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileOpenPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileOpenPlugin - null first_class_object" );
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
    eirods::error unixFileReadPlugin( eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object,
                                      void*               _buf, 
                                      int                 _len ) {
									  
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileWritePlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map*
                                       _cmap,
                                       eirods::first_class_object* 
                                       _object,
                                       void*               _buf, 
                                       int                 _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileClosePlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object* 
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileUnlinkPlugin( eirods::resource_property_map* 
                                        _prop_map, 
                                        eirods::resource_child_map* 
                                        _cmap,
                                        eirods::first_class_object* 
                                        _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileStatPlugin( eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object,
                                      struct stat*        _statbuf ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
        }

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
    eirods::error unixFileFstatPlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map*
                                       _cmap,
                                       eirods::first_class_object* 
                                       _object,
                                       struct stat*        _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileLseekPlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object* 
                                       _object,
                                       size_t              _offset, 
                                       int                 _whence ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileFsyncPlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object*
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileMkdirPlugin( eirods::resource_property_map*
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object*
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
        }

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
    eirods::error unixFileChmodPlugin(
        eirods::resource_property_map* _prop_map, 
        eirods::resource_child_map* _cmap,
        eirods::first_class_object* _object) {

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileRmdirPlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object*
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileOpendirPlugin( eirods::resource_property_map* 
                                         _prop_map, 
                                         eirods::resource_child_map* 
                                         _cmap,
                                         eirods::first_class_object*
                                         _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileClosedirPlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap,
                                          eirods::first_class_object*
                                          _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileReaddirPlugin( eirods::resource_property_map* 
                                         _prop_map, 
                                         eirods::resource_child_map* 
                                         _cmap,
                                         eirods::first_class_object*
									                         _object,
										 struct rodsDirent** _dirent_ptr ) {
		// =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileStagePlugin( eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object*
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
		}
		if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
		}
		if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error unixFileRenamePlugin( eirods::resource_property_map* 
                                        _prop_map, 
                                        eirods::resource_child_map* 
                                        _cmap,
                                        eirods::first_class_object*
                                        _object, 
                                        const char*         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = rename( _object->physical_path().c_str(), _new_file_name );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            status = UNIX_FILE_RENAME_ERR - errno;
				
            std::stringstream msg;
            msg << "unixFileRenamePlugin: rename error for ";
            msg <<  _object->physical_path();
            msg << " to ";
            msg << _new_file_name;
            msg << ", status = ";
            msg << status;
			
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // unixFileRenamePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error unixFileTruncatePlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap,
                                          eirods::first_class_object*
                                          _object ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    eirods::error unixFileGetFsFreeSpacePlugin( eirods::resource_property_map* 
                                                _prop_map, 
                                                eirods::resource_child_map* 
                                                _cmap,
                                                eirods::first_class_object*
                                                _object ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "unixFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "unixFileReadPlugin - null first_class_object" );
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
    unixFileCopyPlugin( int mode, const char *srcFileName, 
                        const char *destFileName )
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
    eirods::error unixStageToCachePlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap,
                                          const char*         _file_name, 
                                          const char*         _cache_file_name, 
                                          int                 _mode, 
                                          int                 _flags,  
                                          size_t              _data_size, 
                                          keyValPair_t*       _cond_input, 
                                          int*                _status ) {
        (*_status) = unixFileCopyPlugin( _mode, _file_name, _cache_file_name );
        if( (*_status) < 0 ) {
            return ERROR( *_status, "unixStageToCachePlugin failed." );
        } else {
            return SUCCESS();
        }
    } // unixStageToCachePlugin

    // =-=-=-=-=-=-=-
    // unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error unixSyncToArchPlugin( eirods::resource_property_map* 
                                        _prop_map, 
                                        eirods::resource_child_map* 
                                        _cmap,
                                        const char*         _file_name, 
                                        char*               _cache_file_name, 
                                        int                 _mode,
                                        int                 _flags,  
                                        rodsLong_t          _data_size, 
                                        keyValPair_t*       _cond_input, 
                                        int*                _status ) {

        (*_status) = unixFileCopyPlugin( _mode, _cache_file_name, _file_name );
        if( (*_status) < 0 ) {
            return ERROR( (*_status), "unixSyncToArchPlugin failed." );
        } else {
            return SUCCESS();
        }

    } // unixSyncToArchPlugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class unixfilesystem_resource : public eirods::resource {
    public:
        unixfilesystem_resource( std::string _context ) : eirods::resource( _context ) {
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

    }; // class unixfilesystem_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the e-irods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( std::string _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create unixfilesystem_resource
        unixfilesystem_resource* resc = new unixfilesystem_resource( _context );

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

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



