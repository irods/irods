// =-=-=-=-=-=-=-
// irods includes
#include "irods/msParam.h"
#include "irods/generalAdmin.h"
#include "irods/miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_physical_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_random.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>


/// @brief Check the general parameters passed in to most plugin functions
template<typename DEST_TYPE>
inline irods::error random_check_params(irods::plugin_context& _ctx)
{
    // ask the context if it is valid
    if (const auto err = _ctx.valid<DEST_TYPE>(); !err.ok()) {
        return PASSMSG("Resource context invalid.", err);
    }

    return SUCCESS();
} // random_check_params

/// @brief get the next resource shared pointer given this resources name
///        as well as the object's hierarchy string
irods::error get_next_child_in_hier(
    const std::string&          _name,
    const std::string&          _hier,
    irods::plugin_property_map& _props,
    irods::resource_ptr&        _resc)
{
    irods::resource_child_map* cmap_ref;
    if (const auto err = _props.get<irods::resource_child_map*>(irods::RESC_CHILD_MAP_PROP, cmap_ref); !err.ok()) {
        return PASSMSG("Failed to get property.", err);
    }

    // create a parser and parse the string
    irods::hierarchy_parser parse;
    if (const auto err = parse.set_string(_hier); !err.ok()) {
        return PASSMSG("Failed in set_string", err);
    }

    // get the next resource in the series
    std::string next;
    if (const auto err = parse.next(_name, next); !err.ok()) {
        return PASSMSG("Failed in next.", err);
    }

    // get the next resource from the child map
    if (!cmap_ref->has_entry(next)) {
        return ERROR(CHILD_NOT_FOUND, fmt::format(
                    "Child map missing entry: hier [{}] name [{}] next [{}]",
                    _hier, _name, next));
    }

    // assign resource
    _resc = (*cmap_ref)[ next ].second;

    return SUCCESS();
} // get_next_child_in_hier

/// =-=-=-=-=-=-=-
/// @brief get the next resource shared pointer given this resources name
///        as well as the file object
irods::error get_next_child_for_open_or_write(
    const std::string&          _name,
    irods::file_object_ptr&     _file_obj,
    irods::plugin_property_map& _props,
    irods::resource_ptr&        _resc ) {
    // =-=-=-=-=-=-=-
    // set up iteration over physical objects
    std::vector< irods::physical_object > objs = _file_obj->replicas();
    std::vector< irods::physical_object >::iterator itr = objs.begin();

    // =-=-=-=-=-=-=-
    // check to see if the replica is in this resource, if one is requested
    for ( ; itr != objs.end(); ++itr ) {
        // =-=-=-=-=-=-=-
        // run the hier string through the parser
        irods::hierarchy_parser parser;
        parser.set_string( itr->resc_hier() );

        // =-=-=-=-=-=-=-
        // find this resource in the hier
        if ( !parser.resc_in_hier( _name ) ) {
            continue;
        }

        // =-=-=-=-=-=-=-
        // if we have a hit, get the resc ptr to the next resc
        return get_next_child_in_hier(
                   _name,
                   itr->resc_hier(),
                   _props,
                   _resc );

    } // for itr

    std::string msg( "no replica found in resc [" );
    msg += _name + "]";
    return ERROR(
               REPLICA_NOT_IN_RESC,
               msg );

} // get_next_child_for_open_or_write

/// @brief get the resource for the child in the hierarchy
///        to pass on the call
template<typename DEST_TYPE>
irods::error random_get_resc_for_call(irods::plugin_context& _ctx,
                                      irods::resource_ptr&   _resc)
{
    // check incoming parameters
    if (const auto err = random_check_params<DEST_TYPE>(_ctx); !err.ok()) {
        return PASSMSG("Bad resource context.", err);
    }

    // get the object's name
    std::string name;
    if (const auto err = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name); !err.ok()) {
        return PASSMSG("Failed to get property.", err);
    }

    // get the object's hier string
    boost::shared_ptr< DEST_TYPE > dst_obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    std::string hier = dst_obj->resc_hier( );

    // get the next child pointer given our name and the hier string
    if (const auto err = get_next_child_in_hier(name, hier, _ctx.prop_map(), _resc); !err.ok()) {
        return PASSMSG("Get next child failed.", err);
    }

    return SUCCESS();
} // random_get_resc_for_call

/// =-=-=-=-=-=-=-
/// @brief given the property map the properties next_child and child_vector,
///        select the next property in the vector to be tapped as the RR resc
irods::error random_get_next_child_resource(
    irods::resource_child_map& _cmap,
    std::string&               _next_child ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // if the child map is empty then just return
    if ( _cmap.size() > 0 ) {

        // =-=-=-=-=-=-=-
        // get the size of the map and randomly pick an index into it
        size_t target_index = irods::getRandom<unsigned int>() % _cmap.size();

        // =-=-=-=-=-=-=-
        // child map is keyed by resource name so we need to count out the index
        // and then snag the child name from the key of the hash map
        size_t counter = 0;
        std::string next_child;
        irods::resource_child_map::iterator itr = _cmap.begin();
        for ( ; itr != _cmap.end(); ++itr ) {
            if ( counter == target_index ) {
                next_child = itr->first;
                break;

            }
            else {
                ++counter;

            }

        } // for itr

        // =-=-=-=-=-=-=-
        // assign the next_child to the out variable
        _next_child = next_child;
    }

    return result;

} // random_get_next_child_resource

/// @brief interface for POSIX create
irods::error random_file_create(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call create on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling create on child resource.", ret);
    }

    return ret;
} // random_file_create

// interface for POSIX Open
irods::error random_file_open(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call open on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling open on child resource.", ret);
    }

    return ret;
} // random_file_open

/// @brief interface for POSIX Read
irods::error random_file_read(
    irods::plugin_context& _ctx,
    void*                  _buf,
    const int              _len)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call read on the child
    const auto ret = resc->call<void*, const int>(_ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len);
    if (!ret.ok()) {
        return PASSMSG("Failed calling operation on child resource.", ret);
    }

    return ret;
} // random_file_read

/// @brief interface for POSIX Write
irods::error random_file_write(
    irods::plugin_context& _ctx,
    const void*            _buf,
    const int              _len)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call write on the child
    const auto ret = resc->call<const void*, const int>(_ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len);
    if (!ret.ok()) {
        return PASSMSG("Failed calling operation on child resource.", ret);
    }

    return ret;
} // random_file_write

/// @brief interface for POSIX Close
irods::error random_file_close(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call close on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling operation in child.", ret);
    }

    return ret;
} // random_file_close

/// @brief interface for POSIX Unlink
irods::error random_file_unlink(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::data_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call unlink on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed during call to child operation.", ret);
    }

    return ret;
} // random_file_unlink

/// @brief interface for POSIX Stat
irods::error random_file_stat(irods::plugin_context& _ctx, struct stat* _statbuf)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::data_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call stat on the child
    const auto ret = resc->call<struct stat*>(_ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf);
    if (!ret.ok()) {
        return PASSMSG("Failed in call to child operation.", ret);
    }

    return ret;
} // random_file_stat

/// @brief interface for POSIX lseek
irods::error random_file_lseek(
    irods::plugin_context& _ctx,
    const long long        _offset,
    const int              _whence)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call lseek on the child
    const auto ret = resc->call<const long long, const int>(_ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_lseek

/// @brief interface for POSIX mkdir
irods::error random_file_mkdir(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::collection_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call mkdir on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_mkdir

/// @brief interface for POSIX rmdir
irods::error random_file_rmdir(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::collection_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call rmdir on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_rmdir

/// @brief interface for POSIX opendir
irods::error random_file_opendir(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::collection_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call opendir on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_opendir

/// @brief interface for POSIX closedir
irods::error random_file_closedir(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::collection_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call closedir on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_closedir

/// @brief interface for POSIX readdir
irods::error random_file_readdir(irods::plugin_context& _ctx,
                                 struct rodsDirent**    _dirent_ptr)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::collection_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call readdir on the child
    const auto ret = resc->call<struct rodsDirent**>(_ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_readdir

/// @brief interface for POSIX rename
irods::error random_file_rename(irods::plugin_context& _ctx,
                                const char*            _new_file_name)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call rename on the child
    const auto ret = resc->call<const char*>(_ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_rename

/// @brief interface to truncate a file
irods::error random_file_truncate(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call truncate on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_truncate

/// @brief interface to determine free space on a device given a path
irods::error random_file_getfs_freespace(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call freespace on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_getfs_freespace

/// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
///        Just copy the file from filename to cacheFilename. optionalInfo info
///        is not used.
irods::error random_file_stage_to_cache(irods::plugin_context& _ctx,
                                        const char*            _cache_file_name)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call stagetocache on the child
    const auto ret = resc->call<const char*>(_ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_stage_to_cache

/// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
///        Just copy the file from cacheFilename to filename. optionalInfo info
///        is not used.
irods::error random_file_sync_to_arch(irods::plugin_context& _ctx,
                                      const char*            _cache_file_name)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call synctoarch on the child
    const auto ret = resc->call<const char*>(_ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_sync_to_arch

/// @brief interface to notify of a file registration
irods::error random_file_registered(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call file registered on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_registered

/// @brief interface to notify of a file unregistration
irods::error random_file_unregistered(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call file unregistered on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_unregistered

/// @brief interface to notify of a file modification
irods::error random_file_modified(irods::plugin_context& _ctx)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call file modified on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco());
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_modified

/// @brief interface to notify a resource of an operation
irods::error random_file_notify(irods::plugin_context& _ctx,
                                const std::string*     _opr)
{
    // get the child resc to call
    irods::resource_ptr resc;
    if (const auto err = random_get_resc_for_call<irods::file_object>(_ctx, resc); !err.ok()) {
        return PASSMSG("Failed to select random resource.", err);
    }

    // call file notify on the child
    const auto ret = resc->call(_ctx.comm(), irods::RESOURCE_OP_NOTIFY, _ctx.fco(), _opr);
    if (!ret.ok()) {
        return PASSMSG("Failed calling child operation.", ret);
    }

    return ret;
} // random_file_notify

/// =-=-=-=-=-=-=-
/// @brief find the next valid child resource for create operation
irods::error get_next_valid_child_resource(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {
    irods::error result = SUCCESS();
    irods::error ret;

    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // flag
    bool   child_found = false;

    // =-=-=-=-=-=-=-
    // the pool of resources (children) to try
    std::vector<irods::resource_ptr> candidate_resources;

    // =-=-=-=-=-=-=-
    // copy children from _cmap
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        candidate_resources.push_back( itr->second.second );
    }

    // =-=-=-=-=-=-=-
    // while we have not found a child and still have candidates
    while ( result.ok() && ( !child_found && !candidate_resources.empty() ) ) {

        // =-=-=-=-=-=-=-
        // generate random index
        size_t rand_index = irods::getRandom<unsigned int>() % candidate_resources.size();

        // =-=-=-=-=-=-=-
        // pick resource in pool at random index
        irods::resource_ptr resc = candidate_resources[rand_index];

        irods::hierarchy_parser hierarchy_copy(*_out_parser);

        // =-=-=-=-=-=-=-
        // forward the 'create' redirect to the appropriate child
        ret = resc->call< const std::string*, const std::string*, irods::hierarchy_parser*, float* >( _ctx.comm(),
                irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                _ctx.fco(), _opr, _curr_host, &hierarchy_copy,
                _out_vote );
        // Found a valid child
        if ( ret.ok() && *_out_vote != 0 ) {
            child_found = true;
            *_out_parser = hierarchy_copy;
        }
        else {
            // =-=-=-=-=-=-=-
            // remove child from pool of candidates
            candidate_resources.erase( candidate_resources.begin() + rand_index );
        }
    } // while

    if (result.ok() && !child_found) {
        return ERROR(NO_NEXT_RESC_FOUND, "No valid child found.");
    }

    return result;
} // get_next_valid_child_resource

/// =-=-=-=-=-=-=-
/// @brief used to allow the resource to determine which host
///        should provide the requested operation
irods::error random_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {

    // =-=-=-=-=-=-=-
    // check incoming parameters
    irods::error ret = random_check_params< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "Invalid resource context.", ret );
    }

    if ( NULL == _opr || NULL == _curr_host || NULL == _out_parser || NULL == _out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "Invalid parameters." );
    }

    // =-=-=-=-=-=-=-
    // get the name of this resource
    std::string name;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
    if ( !ret.ok() ) {
        return PASSMSG( std::string( "Failed to get property: \"" + irods::RESOURCE_NAME + "\"." ), ret );
    }

    // =-=-=-=-=-=-=-
    // add ourselves into the hierarchy before calling child resources
    _out_parser->add_child( name );

    // =-=-=-=-=-=-=-
    // test the operation to determine which choices to make
    if ( irods::OPEN_OPERATION   == ( *_opr )  ||
         irods::WRITE_OPERATION  == ( *_opr ) ||
         irods::UNLINK_OPERATION == ( *_opr )) {
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // get the next child pointer in the hierarchy, given our name and the hier string
        irods::resource_ptr resc;
        ret = get_next_child_for_open_or_write( name, file_obj, _ctx.prop_map(), resc );
        if ( ret.ok() ) {
            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
            ret = resc->call< const std::string*, const std::string*, irods::hierarchy_parser*, float* >( _ctx.comm(),
                    irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                    _ctx.fco(), _opr, _curr_host, _out_parser,
                    _out_vote );
            if ( !ret.ok() ) {
                ret = PASSMSG( "Failed calling child operation.", ret );
            }
        }
        else if ( REPLICA_NOT_IN_RESC == ret.code() ) {
            *_out_vote = 0;
            ret = SUCCESS();
        }
    }
    else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
        // =-=-=-=-=-=-=-
        // get the next_child resource for create
        ret = get_next_valid_child_resource( _ctx, _opr, _curr_host, _out_parser, _out_vote );
        if ( !ret.ok() ) {
            ret = PASSMSG( "Failed getting next valid child.", ret );
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation
        ret = ERROR( INVALID_OPERATION, std::string( "Operation not supported: \"" + ( *_opr )  + "\"." ) );
    }

    return ret;

} // random_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// random_file_rebalance - code which would rebalance the subtree
irods::error random_file_rebalance(
    irods::plugin_context& _ctx ) {
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // forward request for rebalance to children
    irods::error result = SUCCESS();
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        if (const auto err = itr->second.second->call(_ctx.comm(), irods::RESOURCE_OP_REBALANCE, _ctx.fco()); !err.ok()) {
            result = PASSMSG("Failed calling child operation.", err);
            irods::log(PASS(result));
        }
    }

    if ( !result.ok() ) {
        return PASS( result );
    }

    return SUCCESS();

} // random_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle unix file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class random_resource : public irods::resource {
    public:
        random_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource( _inst_name, _context ) {
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return ERROR( -1, "nop" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            return ERROR( -1, "nop" );
        }

}; // class

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name,
                                 const std::string& _context ) {
    // =-=-=-=-=-=-=-
    // 4a. create unixfilesystem_resource
    random_resource* resc = new random_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.

    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            random_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            random_file_open ) );

    resc->add_operation(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,const int)>(
                random_file_read ) );

    resc->add_operation(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,const void*,const int)>(
            random_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            random_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            random_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            random_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            random_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            random_file_opendir ) );

    resc->add_operation(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            random_file_readdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            random_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            random_file_getfs_freespace ) );

    resc->add_operation(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, const long long, const int)>(
            random_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            random_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            random_file_closedir ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            random_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            random_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            random_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            random_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            random_file_modified ) );

    resc->add_operation(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            random_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            random_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            random_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            random_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
