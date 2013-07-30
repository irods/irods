/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a wos coordinating resource. This resource handles the wos 
// storage resources.
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
#include "eirods_resource_constants.h"

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


// define some types
typedef std::vector<eirods::hierarchy_parser> child_list_t;
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

    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error wosCoordCheckParams(eirods::resource_operation_context* _ctx){
        eirods::error result = SUCCESS();
        eirods::error ret;

       // =-=-=-=-=-=-=-
       // check incoming parameters
       if( !_ctx ) {
           result = ERROR( SYS_INVALID_INPUT_PARAM, 
                           "wosCoordCheckParams - null resource context" );
       }
    
       // =-=-=-=-=-=-=-
       // verify that the resc context is valid 
       ret = _ctx->valid();
       if( !ret.ok() ) {
           result = 
            PASSMSG( "wosCoordCheckParams - resource context is invalid", ret );
       }

       return result;
    }

    /**
     * @brief Gets the name of the child of this resource from the hierarchy
     */
    eirods::error wosCoordGetNextRescInHier(
        const eirods::hierarchy_parser& _parser,
        eirods::resource_operation_context* _ctx,
        eirods::resource_ptr& _ret_resc)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string this_name;
        eirods::plugin_property_map& prop_map = _ctx->prop_map();
        eirods::resource_child_map&  cmap     = _ctx->child_map();

        rodsLog( LOG_NOTICE, "before getting name");
        ret = prop_map.get<std::string>("name", this_name);
        rodsLog( LOG_NOTICE, "name: %s", this_name.c_str());
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get resource name from property map.";
            result = ERROR(-1, msg.str());
        } else {
            std::string child;
            rodsLog( LOG_NOTICE, "before getting child");
            ret = _parser.next(this_name, child);
            rodsLog( LOG_NOTICE, "child: %s", child.c_str());
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in the hierarchy.";
                result = ERROR(-1, msg.str());
            } else {
                rodsLog( LOG_NOTICE, "before setting _ret_resc");
                if (cmap.find(child) != cmap.end()) { 
                    _ret_resc = cmap[child].second;
                    rodsLog( LOG_NOTICE, "after setting _ret_resc");
                } else {
                    std::stringstream msg;
                    msg << " child not found.";
                    result = ERROR(-1, msg.str());
                }
            }
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////
    // Actual operations
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error wosCoordFileCreate(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            return PASSMSG("wosCoordFileCreatePlugin - bad params.", ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            rodsLog( LOG_NOTICE, "before calling wosCoordGetNextRescInHier");
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            rodsLog( LOG_NOTICE, "after calling wosCoordGetNextRescInHier");
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CREATE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileCreate

    // =-=-=-=-=-=-=-
    // interface to notify of a file registration
    eirods::error wosCoordRegistered(
        eirods::resource_operation_context* _ctx ) {

        eirods::error ret;

        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_REGISTERED, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                }
            }
        }
        return ret;
    } // wosCoordRegistered

    // =-=-=-=-=-=-=-
    // interface to notify of a file unregistration
    eirods::error wosCoordUnregistered(
        eirods::resource_operation_context* _ctx ) { 
        
        eirods::error ret;
            
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__; 
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_UNREGISTERED, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                }
            }   
        }           
        return ret; 
    } // wosCoordRegistered

    // =-=-=-=-=-=-=-
    // interface to notify of a file modification
    eirods::error wosCoordModified(
        eirods::resource_operation_context* _ctx ) {
        
        eirods::error ret;
            
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__; 
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_MODIFIED, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                }   
            }   
        }    
        return ret;
    } // wosCoordModified

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error wosCoordFileOpen(
        eirods::resource_operation_context* _ctx ) {

        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_OPEN, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                }
            }
        }
        return ret;
    } // wosCoordFileOpen

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error wosCoordFileRead(
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_ctx->comm(), 
                                              eirods::RESOURCE_OP_READ,
                                              _ctx->fco(), 
                                              _buf, 
                                              _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileRead

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error wosCoordFileWrite(
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_ctx->comm(), 
                                              eirods::RESOURCE_OP_WRITE,
                                              _ctx->fco(), 
                                              _buf, 
                                              _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileWrite

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error wosCoordFileClose(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CLOSE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                }
            }
        }
        return ret;
        
    } // wosCoordFileClose
    
    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error wosCoordFileUnlink(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_UNLINK, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileUnlink

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error wosCoordFileStat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) { 
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_ctx->comm(), 
                                                eirods::RESOURCE_OP_STAT, 
                                                _ctx->fco(), 
                                                _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileStat

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error wosCoordFileFstat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) { 
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_ctx->comm(), 
                                                eirods::RESOURCE_OP_FSTAT, 
                                                _ctx->fco(), 
                                                _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileFstat

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error wosCoordFileLseek(
        eirods::resource_operation_context* _ctx,
        size_t                              _offset, 
        int                                 _whence ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<size_t, int>(_ctx->comm(), 
                                               eirods::RESOURCE_OP_LSEEK, 
                                               _ctx->fco(), 
                                               _offset, 
                                               _whence);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileLseek

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error wosCoordFileFsync(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_FSYNC,  _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileFsync

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosCoordFileMkdir(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_MKDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileMkdir

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosCoordFileChmod(
        eirods::resource_operation_context* _ctx ) {
    
        eirods::error ret;

        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CHMOD, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileChmod

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosCoordFileRmdir(
        eirods::resource_operation_context* _ctx ) {
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_RMDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileRmdir

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error wosCoordFileOpendir(
        eirods::resource_operation_context* _ctx ) {    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_OPENDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileOpendir

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error wosCoordFileClosedir(
        eirods::resource_operation_context* _ctx ) {
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileClosedir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosCoordFileReaddir(
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<rodsDirent**>(_ctx->comm(), 
                                                eirods::RESOURCE_OP_READDIR,
                                                _ctx->fco(), 
                                                _dirent_ptr);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileReaddir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosCoordFileStage(
        eirods::resource_operation_context* _ctx ) {
 
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_STAGE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileStage

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosCoordFileRename(
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
    
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>( _ctx->comm(), 
                                               eirods::RESOURCE_OP_RENAME,
                                               _ctx->fco(), 
                                               _new_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileRename

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error wosCoordFileTruncate(
        eirods::resource_operation_context* _ctx) {
    
        // =-=-=-=-=-=-=-
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_TRUNCATE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileTruncate

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error wosCoordFileGetFsFreeSpace(
        eirods::resource_operation_context* _ctx ) {    

        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_FREESPACE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordFileGetFsFreeSpace

    // =-=-=-=-=-=-=-
    // wosCoordStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error wosCoordStageToCache(
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name )
    { 
        eirods::error ret;

        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_ctx->comm(), 
                                               eirods::RESOURCE_OP_STAGETOCACHE,  
                                               _ctx->fco(),
                                               _cache_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordStageToCache

    // =-=-=-=-=-=-=-
    // The syncToArch operation
    eirods::error wosCoordSyncToArch(
        eirods::resource_operation_context* _ctx,
        const char*                    _cache_file_name )
    { 
        eirods::error ret;
        
        ret = wosCoordCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            return PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret = wosCoordGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                return PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_ctx->comm(), 
                                               eirods::RESOURCE_OP_SYNCTOARCH, 
                                               _ctx->fco(), 
                                               _cache_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    return PASSMSG(msg.str(), ret);
                } 
            }
        }
        return ret;
    } // wosCoordSyncToArch

    /// @brief Adds the current resource to the specified resource hierarchy
    eirods::error wosCoordAddSelfToHierarchy(
        eirods::plugin_property_map& _prop_map,
        eirods::hierarchy_parser&    _parser)
    {
        eirods::error ret;
        std::string name;
        ret = _prop_map.get<std::string>("name", name);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the resource name.";
            return PASSMSG(msg.str(), ret);
        } else {
            ret = _parser.add_child(name);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add resource to hierarchy.";
                return PASSMSG(msg.str(), ret);
            }
        }
        return ret;
    }

    /// @brief Loop through the children and call redirect on each one to populate the hierarchy vector
    eirods::error wosCoordRedirectToChildren(
        eirods::resource_operation_context* _ctx,
        const std::string*                  _operation,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser&           _parser,
        redirect_map_t&                     _redirect_map)
    {
        eirods::error ret = SUCCESS();
        eirods::resource_child_map::iterator it;
        eirods::resource_child_map& cmap = _ctx->child_map();
        float out_vote;
        for(it = cmap.begin(); ret.ok() && it != cmap.end(); ++it) {
            eirods::hierarchy_parser parser(_parser);
            eirods::resource_ptr child = it->second.second;
            ret = 
              child->call<const std::string*, 
                          const std::string*, 
                          eirods::hierarchy_parser*, 
                          float*>( _ctx->comm(), 
                                   eirods::RESOURCE_OP_RESOLVE_RESC_HIER, 
                                   _ctx->fco(), 
                                   _operation, 
                                   _curr_host, 
                                   &parser, 
                                   &out_vote);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed calling redirect on the child \"" << it->first << "\"";
                return PASSMSG(msg.str(), ret);
            } else {
                _redirect_map.insert(std::pair<float, eirods::hierarchy_parser>(out_vote, parser));
            }
        }
        return ret;
    }

    /// @brief Selects a child from the redirect map. The redirect map
    /// was populated based on the child voting: the highest value vote
    /// is the one we want.  If there is a tie, we don't care so taking
    /// the first one is still fine.
    eirods::error wosCoordSelectChild(
        const std::string& _curr_host,
        const redirect_map_t& _redirect_map,
        eirods::hierarchy_parser* _out_parser)
    {
        eirods::error ret = SUCCESS();

        // pluck the first entry out of the map. Because the map is sorted by
        // vote value, this is the one we want to use.
        redirect_map_t::const_iterator it;
        it = _redirect_map.begin();
        if (it == _redirect_map.end()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No valid child resource found for file.";
            ret = ERROR(-1, msg.str());
        }
        float vote = it->first;
        if(vote == 0.0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No valid child resource found for file.";
            ret = ERROR(-1, msg.str());
        } 
        
        *_out_parser = it->second;
        return ret;
    }

    /// @brief Determines which child should be used for the specified operation
    eirods::error wosCoordRedirect(
        eirods::resource_operation_context* _ctx,
        const std::string*             _operation,
        const std::string*             _curr_host,
        eirods::hierarchy_parser*      _inout_parser,
        float*                         _out_vote ) {
        eirods::hierarchy_parser parser = *_inout_parser;
        redirect_map_t redirect_map;

        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "wosCoordRedirect - invalid resource context" );
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
        if( !_operation ) {
            return ERROR( -1, "wosCoordRedirect - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "wosCoordRedirect - null operation" );
        }
        if( !_inout_parser ) {
            return ERROR( -1, "wosCoordRedirect - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "wosCoordRedirect - null outgoing vote" );
        }
        
        eirods::plugin_property_map& _prop_map = _ctx->prop_map();
        eirods::resource_child_map&  _cmap     = _ctx->child_map();

        // add ourselves to the hierarchy parser
        if(!(ret = wosCoordAddSelfToHierarchy(_prop_map, parser)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to add ourselves to the resource hierarchy.";
            return PASSMSG(msg.str(), ret);
        }

        // call redirect on each child with the appropriate parser
        else if(!(ret = 
                wosCoordRedirectToChildren(_ctx,
                                           _operation, _curr_host,
                                           parser, redirect_map)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to redirect to all children.";
            return PASSMSG(msg.str(), ret);
        }
        
        else if(!(ret = wosCoordSelectChild( *_curr_host, 
                                            redirect_map, 
                                            _inout_parser)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to select an appropriate child.";
            return PASSMSG(msg.str(), ret);
        }
        
        return ret;
    }

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class wosCoord_resource : public eirods::resource {

    public:
        wosCoord_resource(
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
                return result;
            }

        eirods::error need_post_disconnect_maintenance_operation(
            bool& _flag)
            {
                eirods::error result = SUCCESS();
                return result;

            }

    }; // class wosCoord_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create wosCoord_resource
        wosCoord_resource* resc = new wosCoord_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,            "wosCoordFileCreate" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,              "wosCoordFileOpen" );
        resc->add_operation( eirods::RESOURCE_OP_READ,              "wosCoordFileRead" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,             "wosCoordFileWrite" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,             "wosCoordFileClose" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,            "wosCoordFileUnlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,              "wosCoordFileStat" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,             "wosCoordFileFstat" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,             "wosCoordFileFsync" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,             "wosCoordFileMkdir" );
        resc->add_operation( eirods::RESOURCE_OP_CHMOD,             "wosCoordFileChmod" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,           "wosCoordFileOpendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,           "wosCoordFileReaddir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGE,             "wosCoordFileStage" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,            "wosCoordFileRename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,         "wosCoordFileGetFsFreeSpace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,             "wosCoordFileLseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,             "wosCoordFileRmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,          "wosCoordFileClosedir" );
        resc->add_operation( eirods::RESOURCE_OP_TRUNCATE,          "wosCoordFileTruncate" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE,      "wosCoordStageToCache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,        "wosCoordSyncToArch" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,        "wosCoordRegistered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED,      "wosCoordUnregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,          "wosCoordModified" );
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER, "wosCoordRedirect" );

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
