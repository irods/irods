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

    /////////////////
    // Utility functions

    /// @brief Returns the first child resource of the specified resource
    eirods::error passthruGetFirstChildResc(
        eirods::resource_child_map* _cmap,
        eirods::resource_ptr& _resc) {
        eirods::error result = SUCCESS();
        std::pair<std::string, eirods::resource_ptr> child_pair;
        if(_cmap->size() != 1) {
            std::stringstream msg;
            msg << "passthruGetFirstChildResc - Passthru resource can have 1 and only 1 child. This resource has " << _cmap->size();
            result = ERROR(-1, msg.str());
        } else {
            child_pair = _cmap->begin()->second;
            _resc = child_pair.second;
        }
        return result;
    }

    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error passthruCheckParams(
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map* _cmap,
        eirods::first_class_object* _object) {

        eirods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "passthruCheckParams - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "passthruCheckParams - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "passthruCheckParams - null first_class_object" );
        }
        return result;
    }

    /// @brief Generates a full path name from the partial physical path and the specified resource's vault path
    eirods::error
    passthruGenerateFullPath(
        const eirods::resource_ptr& resc,
        const std::string& physical_path,
        std::string& ret_string) {

        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string vault_path;
        ret = resc->get_property<std::string>("path", vault_path);
        if(!ret.ok()) {
            // The resource doesn't appear to have a vault path so just return the physical path
            ret_string = physical_path;
        } else {
            ret_string = vault_path;
            ret_string += physical_path;
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
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileCreatePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileCreatePlugin - failed getting the first child resource pointer.", ret);
            } else {
                // we must generate the full path by prepending the child's vault path
                std::string full_path;
                ret = passthruGenerateFullPath(resc, _object->physical_path(), full_path);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileCreatePlugin - failed to generate the full path.", ret);
                } else {
                    _object->physical_path(full_path);
                    ret = resc->call<eirods::first_class_object*>("create", _object);
                    if(!ret.ok()) {
                        result = PASS(false, -1, "passthruFileCreatePlugin - failed calling child create.", ret);
                    }
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileOpenPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileReadPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileReadPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, void*, int>("read", _object, _buf, _len);
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileWritePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileWritePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, void*, int>("write", _object, _buf, _len);
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileClosePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileUnlinkPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileStatPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileStatPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, struct stat*>("stat", _object, _statbuf);
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
        eirods::error ret;

        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileFstatPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileFstatPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, struct stat*>("fstat", _object, _statbuf);
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
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileLseekPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileLseekPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, size_t, int>("lseek", _object, _offset, _whence);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileLseekPlugin - failed calling child lseek.", ret);
                }
            }
        }
        return result;
    } // passthruFileLseekPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error passthruFileFsyncPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileFsyncPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileFsyncPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("fsync", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileFsyncPlugin - failed calling child fsync.", ret);
                }
            }
        }
        return result;
    } // passthruFileFsyncPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileMkdirPlugin( eirods::resource_property_map*
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileMkdirPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileMkdirPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("mkdir", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileMkdirPlugin - failed calling child mkdir.", ret);
                }
            }
        }
        return result;
    } // passthruFileMkdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileChmodPlugin(
        eirods::resource_property_map* _prop_map, 
        eirods::resource_child_map* _cmap,
        eirods::first_class_object* _object) {

        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileChmodPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileChmodPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("chmod", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileChmodPlugin - failed calling child chmod.", ret);
                }
            }
        }
        return result;
    } // passthruFileChmodPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error passthruFileRmdirPlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileRmdirPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileRmdirPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("rmdir", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileRmdirPlugin - failed calling child rmdir.", ret);
                }
            }
        }
        return result;
    } // passthruFileRmdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error passthruFileOpendirPlugin( eirods::resource_property_map* 
                                             _prop_map, 
                                             eirods::resource_child_map* 
                                             _cmap,
                                             eirods::first_class_object*
                                             _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileOpendirPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileOpendirPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("opendir", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileOpendirPlugin - failed calling child opendir.", ret);
                }
            }
        }
        return result;
    } // passthruFileOpendirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error passthruFileClosedirPlugin( eirods::resource_property_map* 
                                              _prop_map, 
                                              eirods::resource_child_map* 
                                              _cmap,
                                              eirods::first_class_object*
                                              _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileClosedirPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileClosedirPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("closedir", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileClosedirPlugin - failed calling child closedir.", ret);
                }
            }
        }
        return result;
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
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileReaddirPlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileReaddirPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, struct rodsDirent**>("readdir", _object, _dirent_ptr);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileReaddirPlugin - failed calling child readdir.", ret);
                }
            }
        }
        return result;
    } // passthruFileReaddirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error passthruFileStagePlugin( eirods::resource_property_map* 
                                           _prop_map, 
                                           eirods::resource_child_map* 
                                           _cmap,
                                           eirods::first_class_object*
                                           _object ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileStagePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileStagePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("stage", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileStagePlugin - failed calling child stage.", ret);
                }
            }
        }
        return result;
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
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileRenamePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileRenamePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*, const char*>("rename", _object, _new_file_name);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileRenamePlugin - failed calling child rename.", ret);
                }
            }
        }
        return result;
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
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileTruncatePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileTruncatePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("truncate", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileTruncatePlugin - failed calling child truncate.", ret);
                }
            }
        }
        return result;
    } // passthruFileTruncatePlugin

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error passthruFileGetFsFreeSpacePlugin( eirods::resource_property_map* 
                                                    _prop_map, 
                                                    eirods::resource_child_map* 
                                                    _cmap,
                                                    eirods::first_class_object*
                                                    _object ) { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = passthruCheckParams(_prop_map, _cmap, _object);
        if(!ret.ok()) {
            result = PASS(false, -1, "passthruFileGetFsFreeSpacePlugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruFileGetFsFreeSpacePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<eirods::first_class_object*>("freespace", _object);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruFileGetFsFreeSpacePlugin - failed calling child freespace.", ret);
                }
            }
        }
        return result;
    } // passthruFileGetFsFreeSpacePlugin

    // =-=-=-=-=-=-=-
    // passthruStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error passthruStageToCachePlugin( eirods::resource_property_map* 
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
        eirods::error result = SUCCESS();
        eirods::error ret;

        if(!_prop_map) {
            result = ERROR(-1, "passthruStageToCachePlugin - Bad property map.");
        } else if(!_cmap) {
            result = ERROR(-1, "passthruStageToCachePlugin - Bad child map.");
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruStageToCachePlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<const char*, const char*, int, int, size_t, keyValPair_t*, int*>
                    ("stagetocache", _file_name, _cache_file_name, _mode, _flags, _data_size, _cond_input, _status);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruStageToCachePlugin - failed calling child stagetocache.", ret);
                }
            }
        }
        return result;
    } // passthruStageToCachePlugin

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error passthruSyncToArchPlugin( eirods::resource_property_map* 
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
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        if(!_prop_map) {
            result = ERROR(-1, "passthruSyncToArchPlugin - Bad property map.");
        } else if(!_cmap) {
            result = ERROR(-1, "passthruSyncToArchPlugin - Bad child map.");
        } else {
            eirods::resource_ptr resc;
            ret = passthruGetFirstChildResc(_cmap, resc);
            if(!ret.ok()) {
                result = PASS(false, -1, "passthruSyncToArchPlugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<const char*, const char*, int, int, size_t, keyValPair_t*, int*>
                    ("synctoarch", _file_name, _cache_file_name, _mode, _flags, _data_size, _cond_input, _status);
                if(!ret.ok()) {
                    result = PASS(false, -1, "passthruSyncToArchPlugin - failed calling child synctoarch.", ret);
                }
            }
        }
        return result;
    } // passthruSyncToArchPlugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class passthru_resource : public eirods::resource {
    public:
        passthru_resource( std::string _context ) : eirods::resource( _context ) {
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

    }; // class passthru_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the e-irods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( std::string _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create passthru_resource
        passthru_resource* resc = new passthru_resource( _context );

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



