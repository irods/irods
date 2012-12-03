/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a passthru resource. This resource isn't particularly useful except for testing purposes.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

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

    // Utility functions
    eirods::error passthruGetFirstChildResc(
        eirods::resource_child_map* _cmap,
        resource_ptr& _resc) {
        eirods::error result = SUCCESS();
        std::pair<std::string, resource_ptr> child_pair;
        if(_cmap->size() != 1) {
            result = ERROR(-1, "passthruFileCreatePlugin - Passthru resource can have 1 and only 1 child.");
        } else {
            child_pair = _cmap->begin().second;
            _resc = child_pair.second;
        }
        return result;
    }
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error passthruFileCreatePlugin( eirods::resource_property_map* 
                                            _prop_map,
                                            eirods::resource_child_map* 
                                            _cmap, 
                                            eirods::first_class_object* 
                                            _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileCreatePlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileCreatePlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileCreatePlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileCreatePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("create", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileCreatePlugin - failed calling child create.", ret);
                }
            }
        }
        return result;
    } // passthruFileCreatePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error passthruFileOpenPlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap, 
                                          eirods::first_class_object* 
                                          _object ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileOpenPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileOpenPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileOpenPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileOpenPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("open", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileOpenPlugin - failed calling child open.", ret);
                }
            }
        }
        return result;
    } // passthruFileOpenPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error passthruFileReadPlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap,
                                          eirods::first_class_object* 
                                          _object,
                                          void*               _buf, 
                                          int                 _len ) {
        eirods::error result = SUCCESS();
                                                                          
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileReadPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("read", _object, _buf, _len);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileReadPlugin - failed calling child read.", ret);
                }
            }
        }
        return result;
    } // passthruFileReadPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error passthruFileWritePlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map*
                                           _cmap,
                                           eirods::first_class_object* 
                                           _object,
                                           void*               _buf, 
                                           int                 _len ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileWritePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("write", _object, _buf, _len);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileWritePlugin - failed calling child write.", ret);
                }
            }
        }
        return result;
    } // passthruFileWritePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error passthruFileClosePlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object* 
                                           _object ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result  ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileClosePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("close", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileClosePlugin - failed calling child close.", ret);
                }
            }
        }
        return result;

    } // passthruFileClosePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error passthruFileUnlinkPlugin( eirods::resource_property_map* 
                                            _prop_map, 
                                            eirods::resource_child_map* 
                                            _cmap,
                                            eirods::first_class_object* 
                                            _object ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileUnlinkPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("unlink", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileUnlinkPlugin - failed calling child unlink.", ret);
                }
            }
        }
        return result;
    } // passthruFileUnlinkPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error passthruFileStatPlugin( eirods::resource_property_map* 
                                          _prop_map, 
                                          eirods::resource_child_map* 
                                          _cmap,
                                          eirods::first_class_object* 
                                          _object,
                                          struct stat*        _statbuf ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileStatPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("stat", _object, _statbuf);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileStatPlugin - failed calling child stat.", ret);
                }
            }
        }
        return result;
    } // passthruFileStatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error passthruFileFstatPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map*
                                           _cmap,
                                           eirods::first_class_object* 
                                           _object,
                                           struct stat*        _statbuf ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        } else {
            resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileFstatPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("fstat", _object, _statbuf);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileFstatPlugin - failed calling child fstat.", ret);
                }
            }
        }
        return result;
    } // passthruFileFstatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error passthruFileLseekPlugin( eirods::resource_property_map* 
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
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }
                                                                           
        // =-=-=-=-=-=-=-
        // make the call to lseek       
        size_t status = lseek( _object->file_descriptor(),  _offset, _whence );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_LSEEK_ERR - errno;
 
            std::stringstream msg;
            msg << "passthruFileLseekPlugin: lseek error for ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // passthruFileLseekPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error passthruFileFsyncPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to fsync       
        int status = fsync( _object->file_descriptor() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_LSEEK_ERR - errno;
 
            std::stringstream msg;
            msg << "passthruFileFsyncPlugin: fsync error for ";
            msg << _object->file_descriptor();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // passthruFileFsyncPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileMkdirPlugin( eirods::resource_property_map*
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
                msg << "passthruFileMkdirPlugin: mkdir error for ";
                msg << coll_obj->physical_path();
                msg << ", errno = '";
                msg << strerror( errno );
                msg << "', status = ";
                msg << status;
                                
                return ERROR( status, msg.str() );

            } // if errno != EEXIST

        } // if status < 0 

        return CODE( status );

    } // passthruFileMkdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileChmodPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object,
                                           int                 _mode ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = chmod( _object->physical_path().c_str(), _mode );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_CHMOD_ERR - errno;
 
            std::stringstream msg;
            msg << "passthruFileChmodPlugin: chmod error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        } // if

        return CODE( status );

    } // passthruFileChmodPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileRmdirPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = rmdir( _object->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if( status < 0 ) {
            status = UNIX_FILE_RMDIR_ERR - errno;
 
            std::stringstream msg;
            msg << "passthruFileRmdirPlugin: mkdir error for ";
            msg << _object->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( errno, msg.str() );
        }

        return CODE( status );

    } // passthruFileRmdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error passthruFileOpendirPlugin( eirods::resource_property_map* 
                                             _prop_map, 
                                             eirods::resource_child_map* 
                                             _cmap,
                                             eirods::first_class_object*
                                             _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
            msg << "passthruFileOpendirPlugin: opendir error for ";
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

    } // passthruFileOpendirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error passthruFileClosedirPlugin( eirods::resource_property_map* 
                                              _prop_map, 
                                              eirods::resource_child_map* 
                                              _cmap,
                                              eirods::first_class_object*
                                              _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
            msg << "passthruFileClosedirPlugin: closedir error for ";
            msg << coll_obj->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // passthruFileClosedirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error passthruFileReaddirPlugin( eirods::resource_property_map* 
                                             _prop_map, 
                                             eirods::resource_child_map* 
                                             _cmap,
                                             eirods::first_class_object*
                                             _object,
                                             struct rodsDirent** _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
                msg << "passthruFileReaddirPlugin: closedir error, status = ";
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

    } // passthruFileReaddirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error passthruFileStagePlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }

#ifdef SAMFS_STAGE
        int status = sam_stage (path, "i");

        if (status < 0) {
            _status = UNIX_FILE_STAGE_ERR - errno;
            rodsLog( LOG_NOTICE,"passthruFileStage: sam_stage error, status = %d\n", (*_status) );
            return ERROR( false, errno, "passthruFileStage: sam_stage error" );
        }

        return CODE( 0 );
#else
        return CODE( 0 );
#endif
    } // passthruFileStagePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error passthruFileRenamePlugin( eirods::resource_property_map* 
                                            _prop_map, 
                                            eirods::resource_child_map* 
                                            _cmap,
                                            eirods::first_class_object*
                                            _object, 
                                            const char*         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
        }

        // =-=-=-=-=-=-=-
        // make the call to rename
        int status = rename( _object->physical_path().c_str(), _new_file_name );

        // =-=-=-=-=-=-=-
        // handle error cases
        if( status < 0 ) {
            status = UNIX_FILE_RENAME_ERR - errno;
                                
            std::stringstream msg;
            msg << "passthruFileRenamePlugin: rename error for ";
            msg <<  _object->physical_path();
            msg << " to ";
            msg << _new_file_name;
            msg << ", status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // passthruFileRenamePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error passthruFileTruncatePlugin( eirods::resource_property_map* 
                                              _prop_map, 
                                              eirods::resource_child_map* 
                                              _cmap,
                                              eirods::first_class_object*
                                              _object ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
            msg << "passthruFileTruncatePlugin: rename error for ";
            msg << file_obj->physical_path();
            msg << ", errno = '";
            msg << strerror( errno );
            msg << "', status = ";
            msg << status;
                        
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // passthruFileTruncatePlugin

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error passthruFileGetFsFreeSpacePlugin( eirods::resource_property_map* 
                                                    _prop_map, 
                                                    eirods::resource_child_map* 
                                                    _cmap,
                                                    eirods::first_class_object*
                                                    _object ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "passthruFileReadPlugin - null resource_child_map" );
        }
        if( !_object ) {
            return ERROR( -1, "passthruFileReadPlugin - null first_class_object" );
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
            msg << "passthruFileGetFsFreeSpacePlugin: statfs error for ";
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

    } // passthruFileGetFsFreeSpacePlugin

    int
    passthruFileCopyPlugin( int mode, const char *srcFileName, 
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
                     "passthruFileCopyPlugin: stat of %s error, status = %d",
                     srcFileName, status);
            return status;
        }

        inFd = open (srcFileName, O_RDONLY, 0);
        if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "passthruFileCopyPlugin: open error for srcFileName %s, status = %d",
                     srcFileName, status );
            close( inFd ); // JMC cppcheck - resource
            return status;
        }

        outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
        if (outFd < 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "passthruFileCopyPlugin: open error for destFileName %s, status = %d",
                     destFileName, status);
            close (inFd);
            return status;
        }

        while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
            bytesWritten = write (outFd, (void *) myBuf, bytesRead);
            if (bytesWritten <= 0) {
                status = UNIX_FILE_WRITE_ERR - errno;
                rodsLog (LOG_ERROR,
                         "passthruFileCopyPlugin: write error for srcFileName %s, status = %d",
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
                      "passthruFileCopyPlugin: Copied size %lld does not match source \
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
        (*_status) = passthruFileCopyPlugin( _mode, _file_name, _cache_file_name );
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

        (*_status) = passthruFileCopyPlugin( _mode, _cache_file_name, _file_name );
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
            // =-=-=-=-=-=-=-
            // parse context string into property pairs assuming a ; as a separator
            std::vector< std::string > props;
            eirods::string_tokenize( _context, props, ";" );

            // =-=-=-=-=-=-=-
            // parse key/property pairs using = as a separator and
            // add them to the property list
            std::vector< std::string >::iterator itr = props.begin();
            for( ; itr != props.end(); ++itr ) {
                // =-=-=-=-=-=-=-
                // break up key and value into two strings
                std::vector< std::string > vals;
                eirods::string_tokenize( *itr, vals, "=" );
                                
                // =-=-=-=-=-=-=-
                // break up key and value into two strings
                properties_[ vals[0] ] = vals[1];
                        
            } // for itr 

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
        resc->add_operation( "create",       "passthruFileCreatePlugin" );
        resc->add_operation( "open",         "passthruFileOpenPlugin" );
        resc->add_operation( "read",         "passthruFileReadPlugin" );
        resc->add_operation( "write",        "passthruFileWritePlugin" );
        resc->add_operation( "close",        "passthruFileClosePlugin" );
        resc->add_operation( "unlink",       "passthruFileUnlinkPlugin" );
        resc->add_operation( "stat",         "passthruFileStatPlugin" );
        resc->add_operation( "fstat",        "passthruFileFstatPlugin" );
        resc->add_operation( "fsync",        "passthruFileFsyncPlugin" );
        resc->add_operation( "mkdir",        "passthruFileMkdirPlugin" );
        resc->add_operation( "chmod",        "passthruFileChmodPlugin" );
        resc->add_operation( "opendir",      "passthruFileOpendirPlugin" );
        resc->add_operation( "readdir",      "passthruFileReaddirPlugin" );
        resc->add_operation( "stage",        "passthruFileStagePlugin" );
        resc->add_operation( "rename",       "passthruFileRenamePlugin" );
        resc->add_operation( "freespace",    "passthruFileGetFsFreeSpacePlugin" );
        resc->add_operation( "lseek",        "passthruFileLseekPlugin" );
        resc->add_operation( "rmdir",        "passthruFileRmdirPlugin" );
        resc->add_operation( "closedir",     "passthruFileClosedirPlugin" );
        resc->add_operation( "truncate",     "passthruFileTruncatePlugin" );
        resc->add_operation( "stagetocache", "passthruStageToCachePlugin" );
        resc->add_operation( "synctoarch",   "passthruSyncToArchPlugin" );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



