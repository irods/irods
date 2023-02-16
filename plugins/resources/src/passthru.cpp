
////////////////////////////////////////////////////////////////////////////
// Plugin defining a weighted passthru resource.
//
// Useful for dialing child votes up or down for both read and write.
////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "irods/msParam.h"
#include "irods/miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_lexical_cast.hpp"

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

namespace
{
    using log_resource = irods::experimental::log::resource;

    auto apply_weight_to_object_votes(irods::plugin_context& _ctx,
                                      const std::string& _this_resource_name,
                                      double _weight) -> void
    {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());
        for (auto& r : file_obj->replicas()) {
            // Only apply vote weight to replicas whose hierarchies contain this resource.
            if (!irods::hierarchy_parser{r.resc_hier()}.contains(_this_resource_name)) {
                continue;
            }

            log_resource::debug("[{}]: applying weight [{}] to replica [{}] of [{}] which voted [{}]",
                                _this_resource_name,
                                _weight,
                                r.repl_num(),
                                file_obj->logical_path(),
                                r.vote());

            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            r.vote(r.vote() * _weight);
        }
    } // apply_weight_to_object_votes

} // anonymous namespace

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
    irods::plugin_property_map& _props,
    irods::resource_ptr&        _resc ) {

    irods::resource_child_map* cmap_ref;
    _props.get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    irods::error result = SUCCESS();
    std::pair<std::string, irods::resource_ptr> child_pair;
    if ( cmap_ref->size() != 1 ) {
        std::stringstream msg;
        msg << "passthru_get_first_child_resc - Passthru resource can have 1 and only 1 child. This resource has " << cmap_ref->size();
        result = ERROR( -1, msg.str() );
    }
    else {
        child_pair = cmap_ref->begin()->second;
        _resc = child_pair.second;
    }
    return result;

} // passthru_get_first_child_resc

// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
irods::error passthru_check_params(
    irods::plugin_context& _ctx ) {
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
irods::error passthru_file_create(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_open(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_read(
    irods::plugin_context& _ctx,
    void*                  _buf,
    const int              _len ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
        if ( !ret.ok() ) {
            result = PASSMSG( "failed getting the first child resource pointer.", ret );
        }
        else {
            ret = resc->call<void*, const int>( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
            result = PASSMSG( "passthru_file_read_plugin - failed calling child read.", ret );
        }
    }
    return result;
} // passthru_file_read_plugin

// =-=-=-=-=-=-=-
// interface for POSIX Write
irods::error passthru_file_write(
    irods::plugin_context& _ctx,
    const void*            _buf,
    const int              _len ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
        if ( !ret.ok() ) {
            result = PASSMSG( "failed getting the first child resource pointer.", ret );
        }
        else {
            ret = resc->call<const void*, const int>( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
            result = PASSMSG( "passthru_file_write_plugin - failed calling child write.", ret );
        }
    }
    return result;
} // passthru_file_write_plugin

// =-=-=-=-=-=-=-
// interface for POSIX Close
irods::error passthru_file_close(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_close_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_unlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_unlink_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                        _statbuf ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_stat_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_lseek(
    irods::plugin_context& _ctx,
    const long long        _offset,
    const int              _whence ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_lseek_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_lseek_plugin - failed getting the first child resource pointer.", ret );
        }
        else {
            ret = resc->call<const long long, const int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
            result = PASSMSG( "passthru_file_lseek_plugin - failed calling child lseek.", ret );
        }
    }
    return result;
} // passthru_file_lseek_plugin

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error passthru_file_mkdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_mkdir_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_rmdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_rmdir_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_opendir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_opendir_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_closedir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_closedir_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**                 _dirent_ptr ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_readdir_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_rename(
    irods::plugin_context& _ctx,
    const char*                         _new_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_rename_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_truncate(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_truncate_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_getfs_freespace_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_getfs_freespace_plugin - failed getting the first child resource pointer.", ret );
        }
        else {
            ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
            result = PASSMSG( "passthru_file_getfs_freespace_plugin - failed calling child freespace.", ret );
        }
    }
    return result;
} // passthru_file_getfs_freespace

// =-=-=-=-=-=-=-
// passthruStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error passthru_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                         _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_stage_to_cache_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
irods::error passthru_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                         _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_sync_to_arch_plugin - bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
        if ( !ret.ok() ) {
            result = PASSMSG( "passthru_file_sync_to_arch_plugin - failed getting the first child resource pointer.", ret );
        }
        else {
            ret = resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

            result = PASSMSG( "passthru_file_sync_to_arch_plugin - failed calling child synctoarch.", ret );
        }
    }
    return result;
} // passthru_file_sync_to_arch_plugin

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file registration
irods::error passthru_file_registered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "bad params.", ret );
    }
    else {
        irods::resource_ptr resc;
        ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
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

static irods::error capture_weight( 
    irods::plugin_context& _ctx, 
    const std::string& _type,
    double& _weight) {
    _weight = 1.0;
    std::string results = _ctx.rule_results();
    if (!results.empty()) {

        std::vector< std::string > props;
        irods::kvp_map_t kvp;
        irods::parse_kvp_string(
                results,
                kvp );

        // =-=-=-=-=-=-=-
        // copy the properties from the context to the prop map
        irods::kvp_map_t::iterator itr = kvp.begin();
        for( ; itr != kvp.end(); ++itr ) {
            if (itr->first == _type) {
                irods::error ret = irods::lexical_cast<double>(itr->second, _weight);
                if (!ret.ok()) {
                    irods::log(PASS(ret));
                    _weight = 1.0;
                } else {
                    // Overriding the resource's context string settings
                    return SUCCESS();
                }
            }
        } // for itr
    } 

    if ( _ctx.prop_map().has_entry( _type ) ) {
        irods::error ret = _ctx.prop_map().get<double>(
                               _type,
                               _weight );
        if ( !ret.ok() ) {
            return PASS(ret);
        }
    }

    return SUCCESS();
}

// =-=-=-=-=-=-=-
// unixRedirectPlugin - used to allow the resource to determine which host
//                      should provide the requested operation
irods::error passthru_file_resolve_hierarchy(
    irods::plugin_context& _ctx,
    const std::string*                  _opr,
    const std::string*                  _curr_host,
    irods::hierarchy_parser*           _out_parser,
    float*                              _out_vote )
{
    irods::error result = SUCCESS();
    irods::error ret = passthru_check_params( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "passthru_file_resolve_hierarchy - invalid resource context.", ret );
    }
    if (!_opr || !_curr_host || !_out_parser || !_out_vote) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Invalid input parameter.");
    }

    std::string this_resource_name;
    try {
        this_resource_name = irods::get_resource_name(_ctx);
    }
    catch (const irods::exception& e) {
        return {e};
    }
    _out_parser->add_child(this_resource_name);

    irods::resource_ptr resc;
    ret = passthru_get_first_child_resc( _ctx.prop_map(), resc );
    if ( !ret.ok() ) {
        return PASSMSG( "passthru_file_resolve_hierarchy - failed getting the first child resource pointer.", ret );
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

    if (irods::OPEN_OPERATION == (*_opr)) {
        double read_weight = 1.0;
        if (const auto ret = capture_weight(_ctx, READ_WEIGHT_KW, read_weight); !ret.ok()) {
            irods::log(PASS(ret));
        }
        else {
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            (*_out_vote) *= read_weight;
            apply_weight_to_object_votes(_ctx, this_resource_name, read_weight);
        }
    }
    else if ((irods::CREATE_OPERATION == (*_opr) || irods::WRITE_OPERATION == (*_opr))) {
        double write_weight = 1.0;
        if (const auto ret = capture_weight(_ctx, WRITE_WEIGHT_KW, write_weight); !ret.ok()) {
            irods::log(PASS(ret));
        }
        else {
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            (*_out_vote) *= write_weight;
            apply_weight_to_object_votes(_ctx, this_resource_name, write_weight);
        }
    }

    log_resource::debug("[{}] - resource [{}], operation [{}], original vote [{}], weighted vote [{}]",
                        __func__,
                        this_resource_name,
                        *_opr,
                        orig_vote,
                        *_out_vote);

    return final_ret;
} // passthru_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// passthru_file_rebalance - code which would rebalance the subtree
irods::error passthru_file_rebalance(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // forward request for rebalance to children
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    irods::error result = SUCCESS();
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
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

    return SUCCESS();

} // passthru_file_rebalance

// =-=-=-=-=-=-=-
// passthru_file_notify - code which would notify the subtree of a change
irods::error passthru_file_notify(
    irods::plugin_context& _ctx,
    const std::string*     _opr ) {

    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // forward request for notify to children
    irods::error result = SUCCESS();
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
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
            if ( !_context.empty() ) {
                irods::error ret = irods::parse_kvp_string(
                                       _context,
                                       kvp_map );
                if ( !ret.ok() ) {
                    rodsLog(
                        LOG_ERROR,
                        "invalid context for [%s:passthru] : [%s] : %d",
                        _inst_name.c_str(),
                        _context.c_str(),
                        _context.size() );
                    return;
                }

                if ( kvp_map.find( WRITE_WEIGHT_KW ) != kvp_map.end() ) {
                    try {
                        double write_weight = boost::lexical_cast< double >( kvp_map[ WRITE_WEIGHT_KW ] );
                        properties_.set< double >( WRITE_WEIGHT_KW, write_weight );
                    }
                    catch ( const boost::bad_lexical_cast& ) {
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

                if ( kvp_map.find( READ_WEIGHT_KW ) != kvp_map.end() ) {
                    try {
                        double read_weight = boost::lexical_cast< double >( kvp_map[ READ_WEIGHT_KW ] );
                        properties_.set< double >( READ_WEIGHT_KW, read_weight );
                    }
                    catch ( const boost::bad_lexical_cast& ) {
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
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create passthru_resource
    passthru_resource* resc = new passthru_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            passthru_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            passthru_file_open ) );

    resc->add_operation(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,const int)>(
                passthru_file_read ) );

    resc->add_operation(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,const void*,const int)>(
            passthru_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            passthru_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            passthru_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            passthru_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            passthru_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            passthru_file_opendir ) );

    resc->add_operation(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            passthru_file_readdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            passthru_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            passthru_file_getfs_freespace ) );

    resc->add_operation(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, const long long, const int)>(
            passthru_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            passthru_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            passthru_file_closedir ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            passthru_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            passthru_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            passthru_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            passthru_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            passthru_file_modified ) );

    resc->add_operation(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            passthru_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            passthru_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            passthru_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            passthru_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory


