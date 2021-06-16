// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "genQuery.h"
#include "generalAdmin.h"
#include "miscServerFunct.hpp"
#include "rsGenQuery.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_kvp_string_parser.hpp"

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

#define MAX_ELAPSE_TIME 1800

/// =-=-=-=-=-=-=-
/// @brief Key to deferral policy requested
const std::string DEFER_POLICY_KEY( "defer_policy" );

/// =-=-=-=-=-=-=-
/// @brief string specifying the prefer localhost deferral policy
const std::string DEFER_POLICY_LOCALHOST( "localhost_defer_policy" );

/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline irods::error load_balanced_check_params(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    result = ASSERT_PASS( ret, "Resource context invalid." );

    return result;

} // load_balanced_check_params

/// =-=-=-=-=-=-=-
/// @brief get the next resource shared pointer given this resources name
///        as well as the object's hierarchy string
irods::error get_next_child_in_hier(
    const std::string&          _name,
    const std::string&          _hier,
    irods::plugin_property_map& _props,
    irods::resource_ptr&       _resc ) {
    irods::error result = SUCCESS();

    irods::resource_child_map* cmap_ref;
    _props.get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // create a parser and parse the string
    irods::hierarchy_parser parse;
    irods::error err = parse.set_string( _hier );
    if ( ( result = ASSERT_PASS( err, "Failed in set_string" ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get the next resource in the series
        std::string next;
        err = parse.next( _name, next );

        if ( ( result = ASSERT_PASS( err, "Failed in next." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // get the next resource from the child map
            if ( ( result = ASSERT_ERROR( cmap_ref->has_entry( next ), CHILD_NOT_FOUND, "Child map missing entry: \"%s\"",
                                          next.c_str() ) ).ok() ) {
                // =-=-=-=-=-=-=-
                // assign resource
                _resc = (*cmap_ref)[ next ].second;
            }
        }
    }

    return result;

} // get_next_child_in_hier

// =-=-=-=-=-=-=-
/// @brief get the resource for the child in the hierarchy
///        to pass on the call
template< typename DEST_TYPE >
irods::error load_balanced_get_resc_for_call(
    irods::plugin_context& _ctx,
    irods::resource_ptr&            _resc ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // check incoming parameters
    irods::error err = load_balanced_check_params< DEST_TYPE >( _ctx );
    if ( ( result = ASSERT_PASS( err, "Bad resource context." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get the object's name
        std::string name;
        err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
        if ( ( result = ASSERT_PASS( err, "Failed to get property." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // get the object's hier string
            boost::shared_ptr< DEST_TYPE > dst_obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
            std::string hier = dst_obj->resc_hier( );

            // =-=-=-=-=-=-=-
            // get the next child pointer given our name and the hier string
            err = get_next_child_in_hier( name, hier, _ctx.prop_map(), _resc );
            result = ASSERT_PASS( err, "Get next child failed." );
        }
    }

    return result;

} // load_balanced_get_resc_for_call

// =-=-=-=-=-=-=-
/// @brief Start Up Operation - initialize the load_balanced number generator
irods::error load_balanced_start_operation(
    irods::plugin_property_map&,
    irods::resource_child_map& ) {
    srand( time( NULL ) );
    return SUCCESS();

} // load_balanced_start_operation

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX create
irods::error load_balanced_file_create(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Invalid resource context." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call create on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling create on child resource." );
    }

    return result;
} // load_balanced_file_create

// =-=-=-=-=-=-=-
// interface for POSIX Open
irods::error load_balanced_file_open(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed in file open." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call open operation on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling open on the child." );
    }

    return result;
} // load_balanced_file_open

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Read
irods::error load_balanced_file_read(
    irods::plugin_context& _ctx,
    void*                               _buf,
    int                                 _len ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed finding resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call read on the child
        err = resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
        result = ASSERT_PASS( err, "Failed calling operation on child resource." );
    }

    return result;

} // load_balanced_file_read

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Write
irods::error load_balanced_file_write(
    irods::plugin_context& _ctx,
    void*                               _buf,
    int                                 _len ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed choosing child resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call write on the child
        err = resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
        result = ASSERT_PASS( err, "Failed calling operation on child resource." );
    }

    return result;
} // load_balanced_file_write

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Close
irods::error load_balanced_file_close(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call close on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling operation in child." );
    }

    return result;
} // load_balanced_file_close

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Unlink
irods::error load_balanced_file_unlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::data_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call unlink on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed during call to child operation." );
    }

    return result;
} // load_balanced_file_unlink

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Stat
irods::error load_balanced_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                     _statbuf ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::data_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced child resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call stat on the child
        err = resc->call< struct stat* >( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
        result = ASSERT_PASS( err, "Failed in call to child operation." );
    }

    return result;
} // load_balanced_file_stat

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX lseek
irods::error load_balanced_file_lseek(
    irods::plugin_context& _ctx,
    long long                        _offset,
    int                              _whence ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced child." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call lseek on the child
        err = resc->call< long long, int >( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_lseek

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX mkdir
irods::error load_balanced_file_mkdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::collection_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced child resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call mkdir on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_mkdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX rmdir
irods::error load_balanced_file_rmdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::collection_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rmdir on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_rmdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX opendir
irods::error load_balanced_file_opendir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::collection_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call opendir on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_opendir

// =-=-=-=-=-=-=-
/// @brief interface for POSIX closedir
irods::error load_balanced_file_closedir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::collection_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call closedir on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_closedir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX readdir
irods::error load_balanced_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**              _dirent_ptr ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::collection_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call readdir on the child
        err = resc->call< struct rodsDirent** >( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_readdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX rename
irods::error load_balanced_file_rename(
    irods::plugin_context& _ctx,
    const char*                         _new_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rename on the child
        err = resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_rename

/// =-=-=-=-=-=-=-
/// @brief interface to truncate a file
irods::error load_balanced_file_truncate(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call freespace on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_truncate

/// =-=-=-=-=-=-=-
/// @brief interface to determine free space on a device given a path
irods::error load_balanced_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call freespace on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_getfs_freespace

/// =-=-=-=-=-=-=-
/// @brief This routine copies data from the archive resource to the cache resource
///        in a compound resource composition
irods::error load_balanced_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                         _cache_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed to select load_balanced resource." ) ).ok() ) {
        // =-=-=-=-=-=-=-
        // call stage on the child
        err = resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_stage_to_cache

/// =-=-=-=-=-=-=-
/// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
///        Just copy the file from cacheFilename to filename. optionalInfo info
///        is not used.
irods::error load_balanced_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                         _cache_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call synctoarch on the child
        err = resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_sync_to_arch

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file registration
irods::error load_balanced_file_registered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rename on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_registered

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file unregistration
irods::error load_balanced_file_unregistered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rename on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_unregistered

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file modification
irods::error load_balanced_file_modified(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rename on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;
} // load_balanced_file_modified

/// =-=-=-=-=-=-=-
/// @brief interface to notify a resource of an operation
irods::error load_balanced_file_notify(
    irods::plugin_context& _ctx,
    const std::string*               _opr ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // get the child resc to call
    irods::resource_ptr resc;
    irods::error err = load_balanced_get_resc_for_call< irods::file_object >( _ctx, resc );
    if ( ( result = ASSERT_PASS( err, "Failed selecting load_balanced resource." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // call rename on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_NOTIFY, _ctx.fco(), _opr );
        result = ASSERT_PASS( err, "Failed calling child operation." );
    }

    return result;

} // load_balanced_file_notify

/// =-=-=-=-=-=-=-
/// @brief get the loads, times and names from the resource monitoring table
irods::error get_load_lists(
    irods::plugin_context& _ctx,
    std::vector< std::string >&     _resc_names,
    std::vector< int >&             _resc_loads,
    std::vector< int >&             _resc_times ) {
    // =-=-=-=-=-=-=-
    //
    int i = 0, j = 0, nresc = 0, status = 0;
    char* tResult = 0;
    genQueryInp_t  genQueryInp;
    genQueryOut_t* genQueryOut = NULL;

    // =-=-=-=-=-=-=-
    // query the database in order to retrieve the information on the
    // resources' load
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_SLD_RESC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_SLD_LOAD_FACTOR, 1 );
    addInxIval( &genQueryInp.selectInp, COL_SLD_CREATE_TIME, SELECT_MAX );

    // =-=-=-=-=-=-=-
    // a tmp fix to increase no. of resource to 256
    genQueryInp.maxRows = MAX_SQL_ROWS * 10;
    status =  rsGenQuery( _ctx.comm(), &genQueryInp, &genQueryOut );
    if ( status == 0 ) {
        nresc = genQueryOut->rowCnt;
        // =-=-=-=-=-=-=-
        // vectors should be sized to number of resources
        // these vectors will be indexed directly, not built
        _resc_names.resize( nresc );
        _resc_loads.resize( nresc );
        _resc_times.resize( nresc );

        for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
            for ( j = 0; j < nresc; j++ ) {
                tResult = genQueryOut->sqlResult[i].value;
                tResult += j * genQueryOut->sqlResult[i].len;
                switch ( i ) {
                case 0:
                    _resc_names[j] = tResult;

                case 1:
                    _resc_loads[j] = atoi( tResult );
                    break;
                case 2:
                    _resc_times[j] = atoi( tResult );
                    break;
                }
            }
        }
    }
    else {
        clearGenQueryInp( &genQueryInp );
        freeGenQueryOut( &genQueryOut );
        return ERROR( status, "genquery failed" );
    }

    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );

    return SUCCESS();

} // get_load_lists



/// =-=-=-=-=-=-=-
/// @brief
irods::error load_balanced_redirect_for_create_operation(
    irods::plugin_context& _ctx,
    const std::string*              _opr,
    const std::string*              _curr_host,
    irods::hierarchy_parser*        _out_parser,
    float*                          _out_vote ) {
    // =-=-=-=-=-=-=-
    // capture the name, time and load lists from the DB
    std::vector< std::string > names;
    std::vector< int >         loads;
    std::vector< int >         times;
    irods::error ret = get_load_lists(
                           _ctx,
                           names,
                           loads,
                           times );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // retrieve local time in order to check if the load information is up
    // to date, ie less than MAX_ELAPSE_TIME seconds old
    int    min_load = 100;
    time_t time_now = 0;
    time( &time_now );

    // =-=-=-=-=-=-=-
    // iterate over children and find them in the lists
    //
    //
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    bool resc_found = false;

    irods::resource_ptr selected_resource;
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        // =-=-=-=-=-=-=-
        // cache resc ptr for ease of use
        irods::resource_ptr resc = itr->second.second;

        // =-=-=-=-=-=-=-
        // get the resource name for comparison
        std::string resc_name;
        ret = resc->get_property< std::string >( irods::RESOURCE_NAME, resc_name );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // scan the list for a match
        for ( size_t i = 0; i < names.size(); ++i ) {
            if ( resc_name == names[ i ] ) {
                if ( loads[ i ] >= 0 &&
                        min_load > loads[ i ] &&
                        ( time_now - times[ i ] ) < MAX_ELAPSE_TIME ) {
                    resc_found = true;
                    min_load = loads[i];
                    selected_resource = resc;
                }

            } // if match

        } // for i

    } // for itr

    // =-=-=-=-=-=-=-
    // if we did not find a resource, this is definitely an error
    if ( !resc_found ) {
        return ERROR(
                   CHILD_NOT_FOUND,
                   "failed to find child resc in load list" );
    }

    // =-=-=-=-=-=-=-
    // forward the redirect call to the child for assertion of the whole operation,
    // there may be more than a leaf beneath us
    float                   vote   = 0.0;
    irods::hierarchy_parser parser = ( *_out_parser );
    irods::error err = selected_resource->call <
                       const std::string*,
                       const std::string*,
                       irods::hierarchy_parser*,
                       float* > (
                           _ctx.comm(),
                           irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                           _ctx.fco(),
                           _opr,
                           _curr_host,
                           &parser,
                           &vote );
    std::string hier;
    parser.str( hier );
    rodsLog(
        LOG_DEBUG10,
        "load_balanced node - hier : [%s], vote %f",
        hier.c_str(),
        vote );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
    }

    // =-=-=-=-=-=-=-
    // set the out variables
    ( *_out_parser ) = parser;
    ( *_out_vote )   = vote;

    return SUCCESS();

} // load_balanced_redirect_for_create_operation

/// =-=-=-=-=-=-=-
/// @brief
irods::error load_balanced_redirect_for_open_operation(
    irods::plugin_context& _ctx,
    const std::string*              _opr,
    const std::string*              _curr_host,
    irods::hierarchy_parser*        _out_parser,
    float*                          _out_vote ) {

    *_out_vote = 0.0;

    // =-=-=-=-=-=-=-
    // data struct to hold parser and vote from the search
    std::map< float, irods::hierarchy_parser > result_map;

    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    if(cmap_ref->empty()) {
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // iterate over all the children and pick the highest vote
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        // =-=-=-=-=-=-=-
        // cache resc ptr for ease of use
        irods::resource_ptr resc = itr->second.second;

        // =-=-=-=-=-=-=-
        // temp results from open redirect call - init parser with incoming
        // hier parser
        float                   vote   = 0.0;
        irods::hierarchy_parser parser = ( *_out_parser );

        // =-=-=-=-=-=-=-
        // forward the redirect call to the child for assertion of the whole operation,
        // there may be more than a leaf beneath us
        irods::error err = resc->call <
                           const std::string*,
                           const std::string*,
                           irods::hierarchy_parser*,
                           float* > (
                               _ctx.comm(),
                               irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                               _ctx.fco(),
                               _opr,
                               _curr_host,
                               &parser,
                               &vote );
        std::string hier;
        parser.str( hier );
        rodsLog( LOG_DEBUG10, "load_balanced node - hier : [%s], vote %f", hier.c_str(), vote );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
        else {
            result_map[ vote ] = parser;
        }

    } // for

    if( result_map.empty() ) {
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // now that we have collected all of the results the map
    // will have put the largest one at the end of the map
    // so grab that one and return the result if it is non zero
    float high_vote = result_map.rbegin()->first;
    if(0.0 == high_vote) {
        return SUCCESS();
    }
    else if ( high_vote > 0.0 ) {
        ( *_out_parser ) = result_map.rbegin()->second;
        ( *_out_vote )   = high_vote;
        return SUCCESS();

    }
    else {
        return ERROR( -1, "no valid data object found to open" );
    }

} // load_balanced_redirect_for_open_operation


/// =-=-=-=-=-=-=-
/// @brief used to allow the resource to determine which host
///        should provide the requested operation
irods::error load_balanced_file_resolve_hierarchy(
    irods::plugin_context& _ctx,
    const std::string*              _opr,
    const std::string*              _curr_host,
    irods::hierarchy_parser*        _out_parser,
    float*                          _out_vote ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // check incoming parameters
    irods::error err = load_balanced_check_params< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( err, "Invalid resource context." ) ).ok() ) {
        if ( ( result = ASSERT_ERROR( _opr && _curr_host && _out_parser && _out_vote, SYS_INVALID_INPUT_PARAM,
                                      "Invalid parameters." ) ).ok() ) {
            // =-=-=-=-=-=-=-
            // get the object's hier string
            irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
            std::string hier = file_obj->resc_hier( );

            // =-=-=-=-=-=-=-
            // get the object's hier string
            std::string name;
            err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
            if ( ( result = ASSERT_PASS( err, "Failed to get property: \"%s\".", irods::RESOURCE_NAME.c_str() ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // add ourselves into the hierarch before calling child resources
                _out_parser->add_child( name );

                // =-=-=-=-=-=-=-
                // test the operation to determine which choices to make
                if ( irods::OPEN_OPERATION   == ( *_opr )  ||
                        irods::WRITE_OPERATION  == ( *_opr ) ||
                        irods::UNLINK_OPERATION == ( *_opr )) {
                    std::string err_msg = "failed in resolve hierarchy for [" + ( *_opr ) + "]";
                    err = load_balanced_redirect_for_open_operation( _ctx, _opr, _curr_host, _out_parser, _out_vote );
                    result = ASSERT_PASS( err, err_msg );

                }
                else if ( irods::CREATE_OPERATION == ( *_opr ) ) {

                    // =-=-=-=-=-=-=-
                    // get the next_child resource for create
                    irods::resource_ptr resc;
                    std::string err_msg = "failed in resolve hierarchy for [" + ( *_opr ) + "]";
                    err = load_balanced_redirect_for_create_operation( _ctx, _opr, _curr_host, _out_parser, _out_vote );
                    result = ASSERT_PASS( err, err_msg );
                }
                else {

                    // =-=-=-=-=-=-=-
                    // must have been passed a bad operation
                    result = ASSERT_ERROR( false, INVALID_OPERATION, "Operation not supported: \"%s\".",
                                           _opr->c_str() );
                }
            }
        }
    }

    return result;
} // load_balanced_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// load_balanced_file_rebalance - code which would rebalance the subtree
irods::error load_balanced_file_rebalance(
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
        irods::error ret = itr->second.second->call( _ctx.comm(), irods::RESOURCE_OP_REBALANCE, _ctx.fco() );
        if ( !( result = ASSERT_PASS( ret, "Failed calling child operation." ) ).ok() ) {
            irods::log( PASS( result ) );
        }
    }

    if ( !result.ok() ) {
        return PASS( result );
    }

    return SUCCESS();

} // load_balanced_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle unix file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class load_balanced_resource : public irods::resource {
    public:
        load_balanced_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource( _inst_name, _context ) {
            // =-=-=-=-=-=-=-
            // extract the defer policy from the context string
            irods::kvp_map_t kvp;
            irods::error ret = irods::parse_kvp_string(
                                   _context,
                                   kvp );
            if(!ret.ok()) {
                rodsLog(
                    LOG_ERROR,
                    "libload_balanced: invalid context [%s]",
                    _context.c_str() );
                return;
            }

            if ( kvp.end() != kvp.find( DEFER_POLICY_KEY ) ) {
                properties_.set< std::string >(
                    DEFER_POLICY_KEY,
                    kvp[ DEFER_POLICY_KEY ] );
            }
            else {
                properties_.set< std::string >(
                    DEFER_POLICY_KEY,
                    DEFER_POLICY_LOCALHOST );
                rodsLog(
                    LOG_DEBUG,
                    "load_balanced_resource :: using localhost policy, none specified" );
            }

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
    load_balanced_resource* resc = new load_balanced_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            load_balanced_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            load_balanced_file_open ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,int)>(
                load_balanced_file_read ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,void*,int)>(
            load_balanced_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            load_balanced_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            load_balanced_file_unlink ) );

    resc->add_operation<struct stat*>(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            load_balanced_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            load_balanced_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            load_balanced_file_opendir ) );

    resc->add_operation<struct rodsDirent**>(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            load_balanced_file_readdir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            load_balanced_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            load_balanced_file_getfs_freespace ) );

    resc->add_operation<long long, int>(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, long long, int)>(
            load_balanced_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            load_balanced_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            load_balanced_file_closedir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            load_balanced_file_stage_to_cache ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            load_balanced_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            load_balanced_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            load_balanced_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            load_balanced_file_modified ) );

    resc->add_operation<const std::string*>(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            load_balanced_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            load_balanced_file_truncate ) );

    resc->add_operation<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            load_balanced_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            load_balanced_file_rebalance ) );


    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
