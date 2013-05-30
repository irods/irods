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
#include "eirods_hierarchy_parser.h"
#include "eirods_error.h"

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

    // =-=-=-=-=-=-=-
    /// @brief Returns the first child resource of the specified resource
    eirods::error pass_thru_get_first_chid_resc(
        eirods::resource_child_map& _cmap,
        eirods::resource_ptr& _resc) {

        eirods::error result = SUCCESS();
        std::pair<std::string, eirods::resource_ptr> child_pair;
        if(_cmap.size() != 1) {
            std::stringstream msg;
            msg << "pass_thru_get_first_chid_resc - Passthru resource can have 1 and only 1 child. This resource has " << _cmap.size();
            result = ERROR(-1, msg.str());
        } else {
            child_pair = _cmap.begin()->second;
            _resc = child_pair.second;
        }
        return result;

    } // pass_thru_get_first_chid_resc

    // =-=-=-=-=-=-=-
    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error pass_thru_check_params(
        eirods::resource_operation_context* _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_ctx ) {
            std::stringstream msg;
            msg << " - null resource context";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        } 
  
        // =-=-=-=-=-=-=-
        // verify that the resc context is valid 
        eirods::error ret = _ctx->valid(); 
        if( !ret.ok() ) { 
            std::stringstream msg;
            msg << " - resource context is invalid.";
            return PASSMSG( msg.str(), ret );
        }     

        return SUCCESS();

    } // pass_thru_check_params

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error pass_thru_file_create_plugin( 
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG("bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_CREATE, _ctx->fco());
                if(!ret.ok()) {
                    result = PASSMSG("failed calling child create.", ret);
                } 
            }
        }
        return result;
    } // pass_thru_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error pass_thru_file_open_plugin( 
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_OPEN, _ctx->fco());
                result = PASSMSG("pass_thru_file_open_plugin - failed calling child open.", ret);
            }
        }
        return result;
    } // pass_thru_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error pass_thru_file_read_plugin(  
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<void*, int>( _ctx->comm(), eirods::RESOURCE_OP_READ, _ctx->fco(), _buf, _len);
                result = PASSMSG("pass_thru_file_read_plugin - failed calling child read.", ret);
            }
        }
        return result;
    } // pass_thru_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error pass_thru_file_write_plugin( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<void*, int>( _ctx->comm(), eirods::RESOURCE_OP_WRITE, _ctx->fco(), _buf, _len);
                result = PASSMSG("pass_thru_file_write_plugin - failed calling child write.", ret);
            }
        }
        return result;
    } // pass_thru_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error pass_thru_file_close_plugin(  
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_close_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_close_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_CLOSE, _ctx->fco());
                result = PASSMSG("pass_thru_file_close_plugin - failed calling child close.", ret);
            }
        }
        return result;

    } // pass_thru_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error pass_thru_file_unlink_plugin(  
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_unlink_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_unlink_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_UNLINK, _ctx->fco());
                result = PASSMSG("pass_thru_file_unlink_plugin - failed calling child unlink.", ret);
            }
        }
        return result;
    } // pass_thru_file_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error pass_thru_file_stat_plugin(  
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_stat_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_stat_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<struct stat*>( _ctx->comm(), eirods::RESOURCE_OP_STAT, _ctx->fco(), _statbuf);
                result = PASSMSG("pass_thru_file_stat_plugin - failed calling child stat.", ret);
            }
        }
        return result;
    } // pass_thru_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error pass_thru_file_fstat_plugin(  
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_fstat_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_fstat_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<struct stat*>( _ctx->comm(), eirods::RESOURCE_OP_FSTAT, _ctx->fco(), _statbuf);
                result = PASSMSG("pass_thru_file_fstat_plugin - failed calling child fstat.", ret);
            }
        }
        return result;
    } // pass_thru_file_fstat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error pass_thru_file_lseek_plugin(
        eirods::resource_operation_context* _ctx,
        size_t                              _offset, 
        int                                 _whence ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_lseek_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_lseek_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<size_t, int>( _ctx->comm(), eirods::RESOURCE_OP_LSEEK, _ctx->fco(), _offset, _whence);
                result = PASSMSG("pass_thru_file_lseek_plugin - failed calling child lseek.", ret);
            }
        }
        return result;
    } // pass_thru_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error pass_thru_file_fsync_plugin(  
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_fsync_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_fsync_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_FSYNC, _ctx->fco());
                result = PASSMSG("pass_thru_file_fsync_plugin - failed calling child fsync.", ret);
            }
        }
        return result;
    } // pass_thru_file_fsync_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error pass_thru_file_mkdir_plugin(  
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_mkdir_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_mkdir_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_MKDIR, _ctx->fco());
                result = PASSMSG("pass_thru_file_mkdir_plugin - failed calling child mkdir.", ret);
            }
        }
        return result;
    } // pass_thru_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error pass_thru_file_rmdir_plugin( 
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_rmdir_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_rmdir_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_RMDIR, _ctx->fco());
                result = PASSMSG("pass_thru_file_rmdir_plugin - failed calling child rmdir.", ret);
            }
        }
        return result;
    } // pass_thru_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error pass_thru_file_opendir_plugin( 
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_opendir_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_opendir_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_OPENDIR, _ctx->fco());
                result = PASSMSG("pass_thru_file_opendir_plugin - failed calling child opendir.", ret);
            }
        }
        return result;
    } // pass_thru_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error pass_thru_file_closedir_plugin(  
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_closedir_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_closedir_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx->fco());
                result = PASSMSG("pass_thru_file_closedir_plugin - failed calling child closedir.", ret);
            }
        }
        return result;
    } // pass_thru_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error pass_thru_file_readdir_plugin(  
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_readdir_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_readdir_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<struct rodsDirent**>( _ctx->comm(), eirods::RESOURCE_OP_READDIR, _ctx->fco(), _dirent_ptr);
                result = PASSMSG("pass_thru_file_readdir_plugin - failed calling child readdir.", ret);
            }
        }
        return result;
    } // pass_thru_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error pass_thru_file_rename_plugin(  
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_rename_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_rename_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<const char*>( _ctx->comm(), eirods::RESOURCE_OP_RENAME, _ctx->fco(), _new_file_name);
                result = PASSMSG("pass_thru_file_rename_plugin - failed calling child rename.", ret);
            }
        }
        return result;
    } // pass_thru_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error pass_thru_file_getfsfreespace_plugin( 
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_file_getfsfreespace_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_file_getfsfreespace_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_FREESPACE, _ctx->fco());
                result = PASSMSG("pass_thru_file_getfsfreespace_plugin - failed calling child freespace.", ret);
            }
        }
        return result;
    } // pass_thru_file_getfsfreespace_plugin

    // =-=-=-=-=-=-=-
    // passthruStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error pass_thru_stage_to_cache_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        eirods::error result = SUCCESS();
        eirods::error ret;
 
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_stage_to_cache_plugin - bad params.", ret);
        } else { 
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_stage_to_cache_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<const char*>( _ctx->comm(), eirods::RESOURCE_OP_STAGETOCACHE, _ctx->fco(), _cache_file_name );
                result = PASSMSG("pass_thru_stage_to_cache_plugin - failed calling child stagetocache.", ret);
            }
        }
        return result;
    } // pass_thru_stage_to_cache_plugin

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error pass_thru_sync_to_arch_plugin( 
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_sync_to_arch_plugin - bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "pass_thru_sync_to_arch_plugin - failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call<const char*>( _ctx->comm(), eirods::RESOURCE_OP_SYNCTOARCH, _ctx->fco(), _cache_file_name );
                    
                result = PASSMSG("pass_thru_sync_to_arch_plugin - failed calling child synctoarch.", ret);
            }
        }
        return result;
    } // pass_thru_sync_to_arch_plugin

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error pass_thru_file_registered(
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_REGISTERED, _ctx->fco() );
                    
                result = PASSMSG("failed calling child registered.", ret);
            }
        }
        return result;
    } // pass_thru_file_registered
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error pass_thru_file_unregistered(
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_UNREGISTERED, _ctx->fco() );
                    
                result = PASSMSG("failed calling child unregistered.", ret);
            }
        }
        return result;
    } // pass_thru_file_unregistered
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error pass_thru_file_modified(
        eirods::resource_operation_context* _ctx ) {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "bad params.", ret);
        } else {
            eirods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
            if(!ret.ok()) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret);
            } else {
                ret = resc->call( _ctx->comm(), eirods::RESOURCE_OP_MODIFIED, _ctx->fco() );
                    
                result = PASSMSG("failed calling child modified.", ret);
            }
        }
        return result;
    } // pass_thru_file_modified
 
    // =-=-=-=-=-=-=-
    // unixRedirectPlugin - used to allow the resource to determine which host
    //                      should provide the requested operation
    eirods::error pass_thru_redirect_plugin( 
        eirods::resource_operation_context* _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error result = SUCCESS();
        eirods::error ret = pass_thru_check_params( _ctx );
        if(!ret.ok()) {
            result = PASSMSG( "pass_thru_redirect_plugin - invalid resource context.", ret );
        }
        if( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null operation" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx->prop_map().get< std::string >( eirods::RESOURCE_NAME, resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "pass_thru_redirect_plugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        eirods::resource_ptr resc; 
        ret = pass_thru_get_first_chid_resc(_ctx->child_map(), resc);
        if(!ret.ok()) {
            return PASSMSG( "pass_thru_redirect_plugin - failed getting the first child resource pointer.", ret);
        } 

        return resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                         _ctx->comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx->fco(), _opr, _curr_host, _out_parser, _out_vote );

    } // pass_thru_redirect_plugin








    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class passthru_resource : public eirods::resource {
    public:
        passthru_resource( const std::string _inst_name, const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
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

    }; // class passthru_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create passthru_resource
        passthru_resource* resc = new passthru_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "pass_thru_file_create_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "pass_thru_file_open_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "pass_thru_file_read_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "pass_thru_file_write_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "pass_thru_file_close_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "pass_thru_file_unlink_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "pass_thru_file_stat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "pass_thru_file_fstat_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "pass_thru_file_fsync_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "pass_thru_file_mkdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "pass_thru_file_opendir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "pass_thru_file_readdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "pass_thru_file_rename_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "pass_thru_file_getfsfreespace_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "pass_thru_file_lseek_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "pass_thru_file_rmdir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "pass_thru_file_closedir_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "pass_thru_stage_to_cache_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "pass_thru_sync_to_arch_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "pass_thru_file_registered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "pass_thru_file_unregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "pass_thru_file_modified" );
        
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "pass_thru_redirect_plugin" );

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



