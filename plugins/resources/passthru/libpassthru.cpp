////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a passthru resource. This resource isn't particularly useful except for testing purposes.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_error.hpp"

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
    // 2. Define operations which will be called by the file*
    //    calls declared in server/driver/include/fileDriver.h
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // NOTE :: to access properties in the _prop_map do the
    //      :: following :
    //      :: double my_var = 0.0;
    //      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
    // =-=-=-=-=-=-=-

    /////////////////
    // Utility functions

    // =-=-=-=-=-=-=-
    /// @brief Returns the first child resource of the specified resource
    irods::error pass_thru_get_first_chid_resc(
        irods::resource_child_map& _cmap,
        irods::resource_ptr& _resc ) {

        irods::error result = SUCCESS();
        std::pair<std::string, irods::resource_ptr> child_pair;
        if ( _cmap.size() != 1 ) {
            std::stringstream msg;
            msg << "pass_thru_get_first_chid_resc - Passthru resource can have 1 and only 1 child. This resource has " << _cmap.size();
            result = ERROR( -1, msg.str() );
        }
        else {
            child_pair = _cmap.begin()->second;
            _resc = child_pair.second;
        }
        return result;

    } // pass_thru_get_first_chid_resc

    // =-=-=-=-=-=-=-
    /// @brief Check the general parameters passed in to most plugin functions
    irods::error pass_thru_check_params(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // verify that the resc context is valid
        irods::error ret = _ctx.valid();
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << " - resource context is invalid.";
            return PASSMSG( msg.str(), ret );
        }

        return SUCCESS();

    } // pass_thru_check_params

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    irods::error pass_thru_file_create_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;
        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );
                if ( !ret.ok() ) {
                    result = PASSMSG( "failed calling child create.", ret );
                }
            }
        }
        return result;
    } // pass_thru_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error pass_thru_file_open_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_open_plugin - failed calling child open.", ret );
            }
        }
        return result;
    } // pass_thru_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    irods::error pass_thru_file_read_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
                result = PASSMSG( "pass_thru_file_read_plugin - failed calling child read.", ret );
            }
        }
        return result;
    } // pass_thru_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    irods::error pass_thru_file_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
                result = PASSMSG( "pass_thru_file_write_plugin - failed calling child write.", ret );
            }
        }
        return result;
    } // pass_thru_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    irods::error pass_thru_file_close_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_close_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_close_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_close_plugin - failed calling child close.", ret );
            }
        }
        return result;

    } // pass_thru_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    irods::error pass_thru_file_unlink_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_unlink_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_unlink_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_unlink_plugin - failed calling child unlink.", ret );
            }
        }
        return result;
    } // pass_thru_file_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    irods::error pass_thru_file_stat_plugin(
        irods::resource_plugin_context& _ctx,
        struct stat*                        _statbuf ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_stat_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_stat_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<struct stat*>( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
                result = PASSMSG( "pass_thru_file_stat_plugin - failed calling child stat.", ret );
            }
        }
        return result;
    } // pass_thru_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    irods::error pass_thru_file_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        long long                        _offset,
        int                              _whence ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_lseek_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_lseek_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<long long, int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
                result = PASSMSG( "pass_thru_file_lseek_plugin - failed calling child lseek.", ret );
            }
        }
        return result;
    } // pass_thru_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error pass_thru_file_mkdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_mkdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_mkdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_mkdir_plugin - failed calling child mkdir.", ret );
            }
        }
        return result;
    } // pass_thru_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error pass_thru_file_rmdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_rmdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_rmdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_rmdir_plugin - failed calling child rmdir.", ret );
            }
        }
        return result;
    } // pass_thru_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    irods::error pass_thru_file_opendir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_opendir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_opendir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_opendir_plugin - failed calling child opendir.", ret );
            }
        }
        return result;
    } // pass_thru_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    irods::error pass_thru_file_closedir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_closedir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_closedir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_closedir_plugin - failed calling child closedir.", ret );
            }
        }
        return result;
    } // pass_thru_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error pass_thru_file_readdir_plugin(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_readdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_readdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<struct rodsDirent**>( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );
                result = PASSMSG( "pass_thru_file_readdir_plugin - failed calling child readdir.", ret );
            }
        }
        return result;
    } // pass_thru_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error pass_thru_file_rename_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _new_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_rename_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_rename_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );
                result = PASSMSG( "pass_thru_file_rename_plugin - failed calling child rename.", ret );
            }
        }
        return result;
    } // pass_thru_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    irods::error pass_thru_file_truncate_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_truncate_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_truncate_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_truncate_plugin - failed calling child truncate.", ret );
            }
        }
        return result;
    } // pass_thru_file_truncate_plugin

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    irods::error pass_thru_file_getfsfreespace_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_file_getfsfreespace_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_file_getfsfreespace_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
                result = PASSMSG( "pass_thru_file_getfsfreespace_plugin - failed calling child freespace.", ret );
            }
        }
        return result;
    } // pass_thru_file_getfsfreespace_plugin

    // =-=-=-=-=-=-=-
    // passthruStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    irods::error pass_thru_stage_to_cache_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_stage_to_cache_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_stage_to_cache_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );
                result = PASSMSG( "pass_thru_stage_to_cache_plugin - failed calling child stagetocache.", ret );
            }
        }
        return result;
    } // pass_thru_stage_to_cache_plugin

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    irods::error pass_thru_sync_to_arch_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_sync_to_arch_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "pass_thru_sync_to_arch_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

                result = PASSMSG( "pass_thru_sync_to_arch_plugin - failed calling child synctoarch.", ret );
            }
        }
        return result;
    } // pass_thru_sync_to_arch_plugin

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error pass_thru_file_registered(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );

                result = PASSMSG( "failed calling child registered.", ret );
            }
        }
        return result;
    } // pass_thru_file_registered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error pass_thru_file_unregistered(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );

                result = PASSMSG( "failed calling child unregistered.", ret );
            }
        }
        return result;
    } // pass_thru_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error pass_thru_file_modified(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );

                result = PASSMSG( "failed calling child modified.", ret );
            }
        }
        return result;
    } // pass_thru_file_modified

    // =-=-=-=-=-=-=-
    // unixRedirectPlugin - used to allow the resource to determine which host
    //                      should provide the requested operation
    irods::error pass_thru_redirect_plugin(
        irods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        irods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error result = SUCCESS();
        irods::error ret = pass_thru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "pass_thru_redirect_plugin - invalid resource context.", ret );
        }
        if ( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null operation" );
        }
        if ( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null operation" );
        }
        if ( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null outgoing hier parser" );
        }
        if ( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "pass_thru_redirect_plugin - null outgoing vote" );
        }

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "pass_thru_redirect_plugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        irods::resource_ptr resc;
        ret = pass_thru_get_first_chid_resc( _ctx.child_map(), resc );
        if ( !ret.ok() ) {
            return PASSMSG( "pass_thru_redirect_plugin - failed getting the first child resource pointer.", ret );
        }

        return resc->call < const std::string*,
               const std::string*,
               irods::hierarchy_parser*,
               float* > (
                   _ctx.comm(),
                   irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                   _ctx.fco(),
                   _opr,
                   _curr_host,
                   _out_parser,
                   _out_vote );

    } // pass_thru_redirect_plugin

    // =-=-=-=-=-=-=-
    // pass_thru_file_rebalance - code which would rebalance the subtree
    irods::error pass_thru_file_rebalance(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // forward request for rebalance to children
        irods::error result = SUCCESS();
        irods::resource_child_map::iterator itr = _ctx.child_map().begin();
        for ( ; itr != _ctx.child_map().end(); ++itr ) {
            irods::error ret = itr->second.second->call(
                                   _ctx.comm(),
                                   irods::RESOURCE_OP_REBALANCE,
                                   _ctx.fco() );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                result = ret;
            }
        }
        
        if( !result.ok() ) {
            return PASS( result );
        }

        return update_resource_object_count( 
                   _ctx.comm(),
                   _ctx.prop_map() );

    } // pass_thru_file_rebalancec

    // =-=-=-=-=-=-=-
    // pass_thru_file_rebalance - code which would notify the subtree of a change
    irods::error pass_thru_file_notify(
        irods::resource_plugin_context& _ctx,
        const std::string*               _opr ) {
        // =-=-=-=-=-=-=-
        // forward request for notify to children
        irods::error result = SUCCESS();
        irods::resource_child_map::iterator itr = _ctx.child_map().begin();
        for ( ; itr != _ctx.child_map().end(); ++itr ) {
            irods::error ret = itr->second.second->call(
                                   _ctx.comm(),
                                   irods::RESOURCE_OP_NOTIFY,
                                   _ctx.fco(),
                                   _opr );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                result = ret;
            }
        }

        return result;

    } // pass_thru_file_notify


    // =-=-=-=-=-=-=-
    // 3. create derived class to handle pass_thru file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class passthru_resource : public irods::resource {
    public:
        passthru_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource(
                _inst_name,
                _context ) {
        } // ctor

    }; // class passthru_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the irods facing interface defined in
    //    server/drivers/src/fileDriver.c
    irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

        // =-=-=-=-=-=-=-
        // 4a. create passthru_resource
        passthru_resource* resc = new passthru_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "pass_thru_file_create_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "pass_thru_file_open_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "pass_thru_file_read_plugin" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "pass_thru_file_write_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "pass_thru_file_close_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "pass_thru_file_unlink_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "pass_thru_file_stat_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "pass_thru_file_mkdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "pass_thru_file_opendir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "pass_thru_file_readdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "pass_thru_file_rename_plugin" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "pass_thru_file_getfsfreespace_plugin" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "pass_thru_file_lseek_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "pass_thru_file_rmdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "pass_thru_file_closedir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE, "pass_thru_stage_to_cache_plugin" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,   "pass_thru_sync_to_arch_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "pass_thru_file_registered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "pass_thru_file_unregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "pass_thru_file_modified" );

        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER,     "pass_thru_redirect_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,             "pass_thru_file_rebalance" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,             "pass_thru_file_notify" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     irods::resource pointer
        return dynamic_cast<irods::resource*>( resc );

    } // plugin_factory

}; // extern "C"



