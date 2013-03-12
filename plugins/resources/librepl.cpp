/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a replicating resource. This resource makes sure that all of its data is replicated to all of its children
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "dataObjRepl.h"
#include "dataObjUnlink.h"
#include "rodsLog.h"
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_backport.h"
#include "eirods_plugin_base.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

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

// define some constants
const std::string child_list_prop = "child_list";
const std::string object_list_prop = "object_list";
const std::string need_pdmo_prop = "Need_PDMO";
const std::string hierarchy_prop = "hierarchy";
const std::string write_oper = "write";
const std::string unlink_oper = "unlink";
const std::string create_oper = "create";

// define some types
typedef struct object_oper_s {
    eirods::file_object object_;
    std::string oper_;
} object_oper_t;
typedef std::vector<eirods::hierarchy_parser> child_list_t;
typedef std::vector<object_oper_t> object_list_t;
// define this so we sort children from highest vote to lowest
struct child_comp {
    bool operator()(float _lhs, float _rhs) const { return _lhs > _rhs; }
};
typedef std::multimap<float, eirods::hierarchy_parser, child_comp> redirect_map_t;

extern "C" {

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

    //////////////////////////////////////////////////////////////////////
    // Utility functions

    // TODO - do we need these for the repl? hcj
    
    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error replCheckParams(
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map* _cmap,
        eirods::first_class_object* _object,
        eirods::file_object*& _file_object) {
        
        eirods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "replCheckParams - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "replCheckParams - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "replCheckParams - null first_class_object" );
        } else {
            _file_object = dynamic_cast<eirods::file_object*>(_object);
            if(_file_object == NULL) {
                result = ERROR(-1, "replCheckParams - Unable to cast first_class_object to file_object.");
            }
        }
        return result;
    }

    /**
     * @brief Gets the name of the child of this resource from the hierarchy
     */
    eirods::error replGetNextRescInHier(
        const eirods::hierarchy_parser& _parser,
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map* _cmap,
        eirods::resource_ptr& _ret_resc)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string this_name;
        ret = _prop_map->get<std::string>("name", this_name);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get resource name from property map.";
            result = ERROR(-1, msg.str());
        } else {
            std::string child;
            ret = _parser.next(this_name, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in the hierarchy.";
                result = ERROR(-1, msg.str());
            } else {
                _ret_resc = (*_cmap)[child].second;
            }
        }
        return result;
    }

    /// @brief Returns true if the specified object is in the specified object list
    bool replObjectInList(
        const object_list_t& _object_list,
        const eirods::file_object& _object)
    {
        bool result = false;
        object_list_t::const_iterator it;
        for(it = _object_list.begin(); !result && it != _object_list.end(); ++it) {
            object_oper_t object_oper = *it;
            // not the best way to compare objects but perhaps all we have at this point
            if(object_oper.object_ == _object) {
                result = true;
            }
        }
        return result;
    }

    /// @brief Updates the fields in the resources properties for the object
    eirods::error replUpdateObjectAndOperProperties(
        eirods::resource_property_map* _prop_map,
        eirods::file_object* _object,
        const std::string& _oper)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        object_list_t object_list;

        ret = _prop_map->get<object_list_t>(object_list_prop, object_list);
        if(!ret.ok() || !replObjectInList(object_list, *_object)) {
            object_oper_t object_oper;
            object_oper.object_ = *_object;
            object_oper.oper_ = _oper;
            object_list.push_back(object_oper);
            ret = _prop_map->set<object_list_t>(object_list_prop, object_list);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to set the object list property on the resource.";
                result = PASSMSG(msg.str(), ret);
            }
        } else {
            // make sure the operation on the object in the list matches the passed in operation
            object_list_t::const_iterator it;
            for(it = object_list.begin(); result.ok() && it != object_list.end(); ++it) {
                object_oper_t object_oper = *it;
                if(object_oper.object_ == *_object) {
                    // operations should match. the exception is that it is okay for a write to happen after a create
                    if(object_oper.oper_ != _oper && (object_oper.oper_ != create_oper || _oper != write_oper)) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Existing object operation \"" << object_oper.oper_ << "\" does not match passed in operation \"" << _oper << "\".";
                    }
                }
            }
        }
        
        return result;
    }

    /// @brief 
    eirods::error replSetPDMOFlag(
        eirods::resource_property_map* _prop_map,
        eirods::file_object* _object)
    {
        eirods::error result = SUCCESS();
        if(!_object->in_pdmo()) {
            eirods::error ret = _prop_map->set<bool>(need_pdmo_prop, true);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to update need pdmo flag in properties.";
                result = PASSMSG(msg.str(), ret);
            }
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////
    // Actual operations
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error replFileCreate(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            result = PASS(false, -1, "replFileCreatePlugin - bad params.", ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "create", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = replUpdateObjectAndOperProperties(_prop_map, file_object, create_oper);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the properties with the object and operation.";
                        result = PASSMSG(msg.str(), ret);
                    }
                }
            }
        }
        return result;
    } // replFileCreate

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error replFileOpen(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {

        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "open", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        return result;
    } // replFileOpen

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error replFileRead(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      void*                          _buf, 
                      int                            _len ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_comm, "read", file_object, _buf, _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // have to return the actual code because it contains the number of bytes read
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRead

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error replFileWrite(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      void*                          _buf, 
                      int                            _len ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_comm, "write", file_object, _buf, _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // Have to return the actual code value here because it contains the bytes written
                    result = CODE(ret.code());
                    ret = replUpdateObjectAndOperProperties(_prop_map, file_object, write_oper);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the object and operation properties.";
                        result = PASSMSG(msg.str(), ret);
                    }
                }
            }
        }
        return result;
    } // replFileWrite

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error replFileClose(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "close", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = replSetPDMOFlag(_prop_map, file_object);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the PDMO flag.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        result = CODE(ret.code());
                    }
                }
            }
        }
        return result;
        
    } // replFileClose
    
    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error replFileUnlink(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "unlink", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                    ret = replUpdateObjectAndOperProperties(_prop_map, file_object, unlink_oper);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the object and operation properties.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        ret = replSetPDMOFlag(_prop_map, file_object);
                        if(!ret.ok()) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to update the PDMO flag.";
                            result = PASSMSG(msg.str(), ret);
                        }
                    }
                }
            }
        }
        return result;
    } // replFileUnlink

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error replFileStat(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results, 
                      struct stat*                   _statbuf ) { 
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_comm, "stat", file_object, _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileStat

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error replFileFstat(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results, 
                      struct stat*                   _statbuf ) { 
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_comm, "fstat", file_object, _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.ok());
                }
            }
        }
        return result;
    } // replFileFstat

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error replFileLseek(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      size_t                         _offset, 
                      int                            _whence ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<size_t, int>(_comm, "lseek", file_object, _offset, _whence);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileLseek

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error replFileFsync(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "fsync", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileFsync

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileMkdir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "mkdir", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileMkdir

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileChmod(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;

        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "chmod", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileChmod

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileRmdir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "rmdir", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRmdir

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error replFileOpendir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "opendir", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileOpendir

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error replFileClosedir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "closedir", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileClosedir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileReaddir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      struct rodsDirent**            _dirent_ptr ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<rodsDirent**>(_comm, "readdir", file_object, _dirent_ptr);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileReaddir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileStage(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "stage", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileStage

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileRename(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      const char*                    _new_file_name ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_comm, "rename", file_object, _new_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRename

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error replFileTruncate(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results  ) { 
    
        // =-=-=-=-=-=-=-
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "truncate", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileTruncate

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error replFileGetFsFreeSpace(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
    
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "freespace", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileGetFsFreeSpace

    // =-=-=-=-=-=-=-
    // replStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error replStageToCache(
        rsComm_t*                      _comm,
        eirods::resource_property_map* _prop_map, 
        eirods::resource_child_map*    _cmap,
        eirods::first_class_object*    _object,
        std::string*                   _results,
        const char*                    _cache_file_name )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;

        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_comm, "freespace", file_object);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replStageToCache

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error replSyncToArch(
        rsComm_t*                      _comm,
        eirods::resource_property_map* _prop_map, 
        eirods::resource_child_map*    _cmap,
        eirods::first_class_object*    _object, 
        std::string*                   _results,
        const char*                    _cache_file_name )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        eirods::file_object* file_object;
        ret = replCheckParams(_prop_map, _cmap, _object, file_object);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(file_object->resc_hier());
            eirods::resource_ptr child;
            ret = replGetNextRescInHier(parser, _prop_map, _cmap, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_comm, "synctoarch", file_object, _cache_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replSyncToArch

    /// @brief Adds the current resource to the specified resource hierarchy
    eirods::error replAddSelfToHierarchy(
        eirods::resource_property_map* _prop_map,
        eirods::hierarchy_parser& _parser)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string name;
        ret = _prop_map->get<std::string>("name", name);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the resource name.";
            result = PASSMSG(msg.str(), ret);
        } else {
            ret = _parser.add_child(name);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add resource to hierarchy.";
                result = PASSMSG(msg.str(), ret);
            }
        }
        return result;
    }

    /// @brief Loop through the children and call redirect on each one to populate the hierarchy vector
    eirods::error replRedirectToChildren(
        rsComm_t*                      _comm,
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map*    _cmap,
        eirods::first_class_object*    _object,
        std::string*                   _result,
        const std::string*             _operation,
        const std::string*             _curr_host,
        eirods::hierarchy_parser&      _parser,
        redirect_map_t&                _redirect_map)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::resource_child_map::iterator it;
        float out_vote;
        for(it = _cmap->begin(); result.ok() && it != _cmap->end(); ++it) {
            eirods::hierarchy_parser parser(_parser);
            eirods::resource_ptr child = it->second.second;
            ret = child->call<const std::string*, const std::string*, eirods::hierarchy_parser*, float*>(
                _comm, "redirect", _object, _operation, _curr_host, &parser, &out_vote);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed calling redirect on the child \"" << it->first << "\"";
                result = PASSMSG(msg.str(), ret);
            } else {
                _redirect_map.insert(std::pair<float, eirods::hierarchy_parser>(out_vote, parser));
            }
        }
        return result;
    }

    /// @brief Creates a list of hierarchies to which this operation must be replicated, all children except the one on which we are
    /// operating.
    eirods::error replCreateChildReplList(
        eirods::resource_property_map* _prop_map,
        const redirect_map_t& _redirect_map)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        // Check for an existing child list property. If it exists assume it is correct and do nothing.
        // This assumes that redirect always resolves to the same child. Is that ok? - hcj
        child_list_t repl_vector;
        ret = _prop_map->get<child_list_t>(child_list_prop, repl_vector);
        if(!ret.ok()) {
            
            // loop over all of the children in the map except the first (selected) and add them to a vector
            redirect_map_t::const_iterator it = _redirect_map.begin();
            for(++it; it != _redirect_map.end(); ++it) {
                eirods::hierarchy_parser parser = it->second;
                repl_vector.push_back(parser);
            }
        
            // add the resulting vector as a property of the resource
            eirods::error ret = _prop_map->set<child_list_t>(child_list_prop, repl_vector);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to store the repl child list as a property.";
                result = PASSMSG(msg.str(), ret);
            }
        }
        return result;
    }

    /// @brief Selects a child from the vector of parsers based on host access
    eirods::error replSelectChild(
        eirods::resource_property_map* _prop_map,
        const std::string& _curr_host,
        const redirect_map_t& _redirect_map,
        eirods::hierarchy_parser* _out_parser,
        float* _out_vote)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        // pluck the first entry out of the map. if its vote is non-zero then ship it
        redirect_map_t::const_iterator it;
        it = _redirect_map.begin();
        float vote = it->first;
        eirods::hierarchy_parser parser = it->second;
        if(vote == 0.0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No valid child resource found for file.";
            result = ERROR(-1, msg.str());
        } else {
            *_out_parser = parser;
            *_out_vote = vote;
            ret = replCreateChildReplList(_prop_map, _redirect_map);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add unselected children to the replication list.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = _prop_map->set<eirods::hierarchy_parser>(hierarchy_prop, parser);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to add hierarchy property to resource.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        
        return result;
    }

    /// @brief Determines which child should be used for the specified operation
    eirods::error replRedirect(
        rsComm_t*                      _comm,
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map*    _cmap,
        eirods::first_class_object*    _object,
        std::string*                   _result,
        const std::string*             _operation,
        const std::string*             _curr_host,
        eirods::hierarchy_parser*      _inout_parser,
        float*                         _out_vote ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::hierarchy_parser parser = *_inout_parser;
        redirect_map_t redirect_map;
        
        // add ourselves to the hierarchy parser
        if(!(ret = replAddSelfToHierarchy(_prop_map, parser)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to add ourselves to the resource hierarchy.";
            result = PASSMSG(msg.str(), ret);
        }

        // call redirect on each child with the appropriate parser
        else if(!(ret = replRedirectToChildren(_comm, _prop_map, _cmap, _object, _result, _operation, _curr_host,
                                               parser, redirect_map)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to redirect to all children.";
            result = PASSMSG(msg.str(), ret);
        }
        
        // foreach child parser determine the best to access based on host
        else if(!(ret = replSelectChild(_prop_map, *_curr_host, redirect_map, _inout_parser, _out_vote)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to select an appropriate child.";
            result = PASSMSG(msg.str(), ret);
        }
        
        return result;
    }

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class repl_resource : public eirods::resource {

        // create a class for doing the post disconnect operations. For the repl resource the post disconnect operation will be to
        // replicate the current file object to all children other than the currently selected child
        class repl_pdmo {
        public:
            /// @brief ctor
            repl_pdmo(eirods::resource_property_map& _prop_map, eirods::resource_child_map& _child_map) : \
                properties_(_prop_map), children_(_child_map) {
            }
            
            /// @brief cctor
            repl_pdmo(const repl_pdmo& _rhs) : properties_(_rhs.properties_), children_(_rhs.children_) {
            }

            /// @brief assignment operator
            repl_pdmo& operator=(const repl_pdmo& _rhs) {
                properties_ = _rhs.properties_;
                return *this;
            }

            /// @brief ftor Replicates data object stored in the resource hierarchy selected by redirect to all other resource
            /// hierarchies.
            eirods::error operator()(
                rsComm_t* _comm)
                {
                    eirods::error result = SUCCESS();
                    eirods::error ret;
                    if(pdmo_needed()) {
                        std::string selected_hierarchy;
                        std::string root_resource;
                        ret = get_selected_hierarchy(selected_hierarchy, root_resource);
                        if(!ret.ok()) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to get the selected hierarchy information.";
                            result = PASSMSG(msg.str(), ret);
                        } else {
                            ret = replicate_children(_comm, selected_hierarchy, root_resource);
                            if(!ret.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to replicate the selected object, \"" << selected_hierarchy << "\" to the other children";
                                result = PASSMSG(msg.str(), ret);
                            }
#if COMMENT
                            else {
                                // this is a debugging tool. check to make sure that all children have the same number of data
                                // objects
                                int num_objects = -1;
                                eirods::resource_child_map::iterator it;
                                for(it = children_.begin(); result.ok() && it != children_.end(); ++it) {
                                    int child_num_objects;
                                    eirods::resource_ptr child_resc = it->second.second;
                                    std::string resc_name;
                                    ret = child_resc->get_property<std::string>("name", resc_name);
                                    if(!ret.ok()) {
                                        std::stringstream msg;
                                        msg << __FUNCTION__;
                                        msg << " - Failed to get the name of the child property.";
                                        result = PASSMSG(msg.str(), ret);
                                    } else {
                                        ret = chlRescObjCount(resc_name, child_num_objects);
                                        if(!ret.ok()) {
                                            std::stringstream msg;
                                            msg << __FUNCTION__;
                                            msg << " - Failed to get the object counts for the child resource from the database.";
                                            result = PASSMSG(msg.str(), ret);
                                        } else {
                                            if(num_objects < 0) {
                                                // first child so just set the num objects that we will compare later
                                                num_objects = child_num_objects;
                                            } else {
                                                if(num_objects != child_num_objects) {
                                                    std::stringstream msg;
                                                    msg << __FUNCTION__;
                                                    msg << " - Children of replicating resource: \"" << root_resource << "\" have different object counts.";
                                                    result = ERROR(-1, msg.str());
                                                }
                                            }
                                        }
                                    }
                                }
                            }
#endif
                        }
                    }
                    return result;
                }
            
        private:
            bool pdmo_needed(void)
                {
                    bool result = false;
                    bool need_pdmo;
                    eirods::error ret;
                    ret = properties_.get<bool>(need_pdmo_prop, need_pdmo);
                    if(ret.ok()) {
                        result = need_pdmo;
                    }
                    return result;
                }

            eirods::error get_selected_hierarchy(
                std::string& _hier_string,
                std::string& _root_resc)
                {
                    eirods::error result = SUCCESS();
                    eirods::error ret;
                    eirods::hierarchy_parser selected_parser;
                    ret = properties_.get<eirods::hierarchy_parser>(hierarchy_prop, selected_parser);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to get the parser for the selected resource hierarchy.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        ret = selected_parser.str(_hier_string);
                        if(!ret.ok()) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to get the hierarchy string from the parser.";
                            result = PASSMSG(msg.str(), ret);
                        } else {
                            ret = selected_parser.first_resc(_root_resc);
                            if(!ret.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to get the root resource from the parser.";
                                result = PASSMSG(msg.str(), ret);
                            }
                        }
                    }
                    return result;
                }

            eirods::error replicate_children(
                rsComm_t* _comm,
                const std::string& _selected_hierarchy,
                const std::string& _root_resc)
                {
                    eirods::error result = SUCCESS();
                    eirods::error ret;
                    child_list_t children;
                    ret = properties_.get<child_list_t>(child_list_prop, children);
                    // its okay if there are no other children
                    if(ret.ok()) {
                        child_list_t::const_iterator child_it;
                        for(child_it = children.begin(); result.ok() && child_it != children.end(); ++child_it) {
                            eirods::hierarchy_parser parser = *child_it;
                            std::string hierarchy_string;
                            parser.str(hierarchy_string);
                            object_list_t objects;
                            ret = properties_.get<object_list_t>(object_list_prop, objects);
                            if(!ret.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to get the resources object list.";
                                result = PASSMSG(msg.str(), ret);
                            } else {
                                while(objects.size() != 0) {
                                    object_oper_t object_oper = objects.back();
                                    objects.pop_back();
                                    dataObjInp_t dataObjInp;
                                    bzero(&dataObjInp, sizeof(dataObjInp));
                                    rstrcpy(dataObjInp.objPath, object_oper.object_.logical_path().c_str(), MAX_NAME_LEN);
                                    // do operation specific things
                                    if(object_oper.oper_ == write_oper || object_oper.oper_ == create_oper) {
                                        dataObjInp.createMode = object_oper.object_.mode();
                                        addKeyVal(&dataObjInp.condInput, RESC_HIER_STR_KW, _selected_hierarchy.c_str());
                                        addKeyVal(&dataObjInp.condInput, DEST_RESC_HIER_STR_KW, hierarchy_string.c_str());
                                        addKeyVal(&dataObjInp.condInput, RESC_NAME_KW, _root_resc.c_str());
                                        addKeyVal(&dataObjInp.condInput, DEST_RESC_NAME_KW, _root_resc.c_str());
                                        addKeyVal(&dataObjInp.condInput, IN_PDMO_KW, "");
                                        if(object_oper.oper_ == write_oper) {
                                            addKeyVal(&dataObjInp.condInput, UPDATE_REPL_KW, "");
                                        }
                                        transferStat_t* transfer_stat = NULL;
                                        int status = rsDataObjRepl(_comm, &dataObjInp, &transfer_stat);
                                        if(status < 0) {
                                            char* sys_error;
                                            char* rods_error = rodsErrorName(status, &sys_error);
                                            std::stringstream msg;
                                            msg << __FUNCTION__;
                                            msg << " - Failed to replicate the data object \"" << _selected_hierarchy;
                                            msg << " to sibling \"" << hierarchy_string << "\" - ";
                                            msg << rods_error << " " << sys_error;
                                            eirods::log(LOG_ERROR, msg.str());
                                            result = ERROR(status, msg.str());
                                        }
                                        if(transfer_stat != NULL) {
                                            free(transfer_stat);
                                        }
                                    } else if(object_oper.oper_ == unlink_oper) {
                                        addKeyVal(&dataObjInp.condInput, RESC_HIER_STR_KW, hierarchy_string.c_str());
                                        addKeyVal(&dataObjInp.condInput, FORCE_FLAG_KW, "");
                                        int status = rsDataObjUnlink(_comm, &dataObjInp);
                                        if(status < 0) {
                                            char* sys_error;
                                            char* rods_error = rodsErrorName(status, &sys_error);
                                            std::stringstream msg;
                                            msg << __FUNCTION__;
                                            msg << " - Failed to unlink the data child \"" << hierarchy_string << "\" ";
                                            msg << rods_error << " " << sys_error;
                                            eirods::log(LOG_ERROR, msg.str());
                                            result = ERROR(status, msg.str());
                                        }
                                    } else {
                                        std::stringstream msg;
                                        msg << __FUNCTION__;
                                        msg << " - Unknown replication operation: \"" << object_oper.oper_ << "\"";
                                        result = ERROR(-1, msg.str());
                                    }
                                }
                            }
                        }
                    }
                    return result;
                }
            
            eirods::resource_property_map& properties_;
            eirods::resource_child_map& children_;
        };
            
    public:
        repl_resource(
            const std::string _inst_name,
            const std::string& _context ) : 
            eirods::resource( _inst_name, _context )
            {
                // =-=-=-=-=-=-=-
                // parse context string into property pairs assuming a ; as a separator
                std::vector< std::string > props;
                eirods::string_tokenize( _context, ";", props );
                
                // =-=-=-=-=-=-=-
                // parse key/property pairs using = as a separator and
                // add them to the property list
                std::vector< std::string >::iterator itr = props.begin();
                for( ; itr != props.end(); ++itr ) {
                    // =-=-=-=-=-=-=-
                    // break up key and value into two strings
                    std::vector< std::string > vals;
                    eirods::string_tokenize( *itr, "=", vals );
                    
                    // =-=-=-=-=-=-=-
                    // break up key and value into two strings
                    properties_[ vals[0] ] = vals[1];
                        
                } // for itr 

            } // ctor

        eirods::error post_disconnect_maintenance_operation(
            eirods::pdmo_type& _out_pdmo)
            {
                eirods::error result = SUCCESS();
                _out_pdmo = repl_pdmo(properties_, children_);
                return result;
            }

        eirods::error need_post_disconnect_maintenance_operation(
            bool& _flag)
            {
                eirods::error result = SUCCESS();
                _flag = false;
                bool need_pdmo;
                eirods::error ret;
                // Make sure we actually have children and objects to which we need to replicate and check the need_pdmo flag to see
                // if we performed any operations which would warrant a replication.
                child_list_t children;
                ret = properties_.get<child_list_t>(child_list_prop, children);
                if(ret.ok()) {
                    object_list_t objects;
                    ret = properties_.get<object_list_t>(object_list_prop, objects);
                    if(ret.ok()) {
                        ret = properties_.get<bool>(need_pdmo_prop, need_pdmo);
                        if(ret.ok()) {
                            _flag = need_pdmo;
                        }
                    }
                }
                return result;

            }

    }; // class repl_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create repl_resource
        repl_resource* resc = new repl_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( "create",       "replFileCreate" );
        resc->add_operation( "open",         "replFileOpen" );
        resc->add_operation( "read",         "replFileRead" );
        resc->add_operation( "write",        "replFileWrite" );
        resc->add_operation( "close",        "replFileClose" );
        resc->add_operation( "unlink",       "replFileUnlink" );
        resc->add_operation( "stat",         "replFileStat" );
        resc->add_operation( "fstat",        "replFileFstat" );
        resc->add_operation( "fsync",        "replFileFsync" );
        resc->add_operation( "mkdir",        "replFileMkdir" );
        resc->add_operation( "chmod",        "replFileChmod" );
        resc->add_operation( "opendir",      "replFileOpendir" );
        resc->add_operation( "readdir",      "replFileReaddir" );
        resc->add_operation( "stage",        "replFileStage" );
        resc->add_operation( "rename",       "replFileRename" );
        resc->add_operation( "freespace",    "replFileGetFsFreeSpace" );
        resc->add_operation( "lseek",        "replFileLseek" );
        resc->add_operation( "rmdir",        "replFileRmdir" );
        resc->add_operation( "closedir",     "replFileClosedir" );
        resc->add_operation( "truncate",     "replFileTruncate" );
        resc->add_operation( "stagetocache", "replStageToCache" );
        resc->add_operation( "synctoarch",   "replSyncToArch" );
        resc->add_operation( "redirect",     "replRedirect" );

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



