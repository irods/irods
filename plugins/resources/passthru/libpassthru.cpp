
////////////////////////////////////////////////////////////////////////////
// Plugin defining a weighted passthru resource.
//
// Useful for dialing child votes up or down for both read and write.
////////////////////////////////////////////////////////////////////////////

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
#include "irods_kvp_string_parser.hpp"
#include "irods_resource_redirect.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
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

#include <string.h>


const std::string WRITE_WEIGHT_KW( "write" );
const std::string READ_WEIGHT_KW( "read" );


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
    irods::error passthru_get_first_child_resc(
        irods::resource_child_map& _cmap,
        irods::resource_ptr& _resc ) {

        irods::error result = SUCCESS();
        std::pair<std::string, irods::resource_ptr> child_pair;
        if ( _cmap.size() != 1 ) {
            std::stringstream msg;
            msg << "passthru_get_first_child_resc - Weighted Passthru resource can have 1 and only 1 child. This resource has " << _cmap.size();
            result = ERROR( -1, msg.str() );
        }
        else {
            child_pair = _cmap.begin()->second;
            _resc = child_pair.second;
        }
        return result;

    } // passthru_get_first_child_resc

    // =-=-=-=-=-=-=-
    /// @brief Check the general parameters passed in to most plugin functions
    irods::error passthru_check_params(
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

    } // passthru_check_params

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    irods::error passthru_file_create_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;
        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
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
    } // passthru_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error passthru_file_open_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
                result = PASSMSG( "passthru_file_open_plugin - failed calling child open.", ret );
            }
        }
        return result;
    } // passthru_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    irods::error passthru_file_read_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
                result = PASSMSG( "passthru_file_read_plugin - failed calling child read.", ret );
            }
        }
        return result;
    } // passthru_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    irods::error passthru_file_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
                result = PASSMSG( "passthru_file_write_plugin - failed calling child write.", ret );
            }
        }
        return result;
    } // passthru_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    irods::error passthru_file_close_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_close_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_close_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
                result = PASSMSG( "passthru_file_close_plugin - failed calling child close.", ret );
            }
        }
        return result;

    } // passthru_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    irods::error passthru_file_unlink_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_unlink_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_unlink_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
                result = PASSMSG( "passthru_file_unlink_plugin - failed calling child unlink.", ret );
            }
        }
        return result;
    } // passthru_file_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    irods::error passthru_file_stat_plugin(
        irods::resource_plugin_context& _ctx,
        struct stat*                        _statbuf ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_stat_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_stat_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<struct stat*>( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
                result = PASSMSG( "passthru_file_stat_plugin - failed calling child stat.", ret );
            }
        }
        return result;
    } // passthru_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    irods::error passthru_file_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        long long                        _offset,
        int                              _whence ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_lseek_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_lseek_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<long long, int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
                result = PASSMSG( "passthru_file_lseek_plugin - failed calling child lseek.", ret );
            }
        }
        return result;
    } // passthru_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error passthru_file_mkdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_mkdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_mkdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );
                result = PASSMSG( "passthru_file_mkdir_plugin - failed calling child mkdir.", ret );
            }
        }
        return result;
    } // passthru_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error passthru_file_rmdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_rmdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_rmdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );
                result = PASSMSG( "passthru_file_rmdir_plugin - failed calling child rmdir.", ret );
            }
        }
        return result;
    } // passthru_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    irods::error passthru_file_opendir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_opendir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_opendir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );
                result = PASSMSG( "passthru_file_opendir_plugin - failed calling child opendir.", ret );
            }
        }
        return result;
    } // passthru_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    irods::error passthru_file_closedir_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_closedir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_closedir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );
                result = PASSMSG( "passthru_file_closedir_plugin - failed calling child closedir.", ret );
            }
        }
        return result;
    } // passthru_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error passthru_file_readdir_plugin(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_readdir_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_readdir_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<struct rodsDirent**>( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );
                result = PASSMSG( "passthru_file_readdir_plugin - failed calling child readdir.", ret );
            }
        }
        return result;
    } // passthru_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error passthru_file_rename_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _new_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_rename_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_rename_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );
                result = PASSMSG( "passthru_file_rename_plugin - failed calling child rename.", ret );
            }
        }
        return result;
    } // passthru_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    irods::error passthru_file_truncate_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_truncate_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_truncate_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );
                result = PASSMSG( "passthru_file_truncate_plugin - failed calling child truncate.", ret );
            }
        }
        return result;
    } // passthru_file_truncate_plugin

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    irods::error passthru_file_getfsfreespace_plugin(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_getfsfreespace_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_file_getfsfreespace_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
                result = PASSMSG( "passthru_file_getfsfreespace_plugin - failed calling child freespace.", ret );
            }
        }
        return result;
    } // passthru_file_getfsfreespace_plugin

    // =-=-=-=-=-=-=-
    // passthruStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    irods::error passthru_stage_to_cache_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_stage_to_cache_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_stage_to_cache_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );
                result = PASSMSG( "passthru_stage_to_cache_plugin - failed calling child stagetocache.", ret );
            }
        }
        return result;
    } // passthru_stage_to_cache_plugin

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    irods::error passthru_sync_to_arch_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_sync_to_arch_plugin - bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "passthru_sync_to_arch_plugin - failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

                result = PASSMSG( "passthru_sync_to_arch_plugin - failed calling child synctoarch.", ret );
            }
        }
        return result;
    } // passthru_sync_to_arch_plugin

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error passthru_file_registered(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );

                result = PASSMSG( "failed calling child registered.", ret );
            }
        }
        return result;
    } // passthru_file_registered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error passthru_file_unregistered(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );

                result = PASSMSG( "failed calling child unregistered.", ret );
            }
        }
        return result;
    } // passthru_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error passthru_file_modified(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "bad params.", ret );
        }
        else {
            irods::resource_ptr resc;
            ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
            if ( !ret.ok() ) {
                result = PASSMSG( "failed getting the first child resource pointer.", ret );
            }
            else {
                ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );

                result = PASSMSG( "failed calling child modified.", ret );
            }
        }
        return result;
    } // passthru_file_modified

    // =-=-=-=-=-=-=-
    // unixRedirectPlugin - used to allow the resource to determine which host
    //                      should provide the requested operation
    irods::error passthru_redirect_plugin(
        irods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        irods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error result = SUCCESS();
        irods::error ret = passthru_check_params( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_redirect_plugin - invalid resource context.", ret );
        }
        if ( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "passthru_redirect_plugin - null operation" );
        }
        if ( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "passthru_redirect_plugin - null operation" );
        }
        if ( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "passthru_redirect_plugin - null outgoing hier parser" );
        }
        if ( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "passthru_redirect_plugin - null outgoing vote" );
        }

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "passthru_redirect_plugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.child_map(), resc );
        if ( !ret.ok() ) {
            return PASSMSG( "passthru_redirect_plugin - failed getting the first child resource pointer.", ret );
        }

        irods::error final_ret = resc->call < 
                                    const std::string*,
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
       double orig_vote = *_out_vote;
       if( irods::OPEN_OPERATION == ( *_opr ) && 
            _ctx.prop_map().has_entry( READ_WEIGHT_KW ) ) {
           double read_weight = 1.0;
           ret = _ctx.prop_map().get<double>( 
                     READ_WEIGHT_KW,
                     read_weight );
           if( !ret.ok() ) {
               irods::log( PASS( ret ) );
           
           } else {
               (*_out_vote) *= read_weight;

           }

       }
       else if( ( irods::CREATE_OPERATION == ( *_opr ) ||
                  irods::WRITE_OPERATION == ( *_opr ) ) &&
            _ctx.prop_map().has_entry( WRITE_WEIGHT_KW ) ) {
           double write_weight = 1.0;
           ret = _ctx.prop_map().get<double>( 
                     WRITE_WEIGHT_KW,
                     write_weight );
           if( !ret.ok() ) {
               irods::log( PASS( ret ) );
           
           } else {
               (*_out_vote) *= write_weight;

           }
       }

       rodsLog( 
           LOG_DEBUG,
           "passthru_redirect_plugin - [%s] : %f - %f", 
           _opr->c_str(),
           orig_vote,
           *_out_vote );

       return final_ret;

    } // passthru_redirect_plugin

    // =-=-=-=-=-=-=-
    // passthru_file_rebalance - code which would rebalance the subtree
    irods::error passthru_file_rebalance(
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

        if ( !result.ok() ) {
            return PASS( result );
        }

        return update_resource_object_count(
                   _ctx.comm(),
                   _ctx.prop_map() );

    } // passthru_file_rebalancec

    // =-=-=-=-=-=-=-
    // passthru_file_rebalance - code which would notify the subtree of a change
    irods::error passthru_file_notify(
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

    } // passthru_file_notify


    // =-=-=-=-=-=-=-
    // 3. create derived class to handle passthru file system resources
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
                irods::kvp_map_t kvp_map;
                if( !_context.empty() ) {
                    irods::error ret = irods::parse_kvp_string(
                                           _context,
                                           kvp_map );
                    if( !ret.ok() ) {
                        irods::log( PASS( ret ) );

                    }

                    if( kvp_map.find( WRITE_WEIGHT_KW ) != kvp_map.end() ) {
                        try {
                            double write_weight = boost::lexical_cast< double >( kvp_map[ WRITE_WEIGHT_KW ] );
                            properties_.set< double >( WRITE_WEIGHT_KW, write_weight );
                        } catch ( const boost::bad_lexical_cast& ) {
                            std::stringstream msg;
                            msg << "failed to cast weight for write ["
                                << kvp_map[ WRITE_WEIGHT_KW ]
                                << "]";
                            irods::log( 
                                ERROR( 
                                    SYS_INVALID_INPUT_PARAM,
                                    msg.str() ) );
                        }
                    }

                    if( kvp_map.find( READ_WEIGHT_KW ) != kvp_map.end() ) {
                        try {
                            double read_weight = boost::lexical_cast< double >( kvp_map[ READ_WEIGHT_KW ] );
                            properties_.set< double >( READ_WEIGHT_KW, read_weight );
                        } catch ( const boost::bad_lexical_cast& ) {
                            std::stringstream msg;
                            msg << "failed to cast weight for read ["
                                << kvp_map[ READ_WEIGHT_KW ]
                                << "]";
                            irods::log( 
                                ERROR( 
                                    SYS_INVALID_INPUT_PARAM,
                                    msg.str() ) );
                        }
                    }

                } // if !empty

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
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "passthru_file_create_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "passthru_file_open_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "passthru_file_read_plugin" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "passthru_file_write_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "passthru_file_close_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "passthru_file_unlink_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "passthru_file_stat_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "passthru_file_mkdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "passthru_file_opendir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "passthru_file_readdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "passthru_file_rename_plugin" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "passthru_file_getfsfreespace_plugin" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "passthru_file_lseek_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "passthru_file_rmdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "passthru_file_closedir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE, "passthru_stage_to_cache_plugin" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,   "passthru_sync_to_arch_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "passthru_file_registered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "passthru_file_unregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "passthru_file_modified" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,     "passthru_file_truncate_plugin" );

        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER,     "passthru_redirect_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,             "passthru_file_rebalance" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,             "passthru_file_notify" );

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



