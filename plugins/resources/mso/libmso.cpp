// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "rcConnect.h"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

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

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error mso_file_mkdir(
    irods::plugin_context& ) {
    irods::error result = SUCCESS();
    // =-=-=-=-=-=-=-
    // stub out
    return SUCCESS();

} // mso_file_mkdir

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error mso_file_stat(
    irods::plugin_context& ,
    struct stat*                    _statbuf ) {
    irods::error result = SUCCESS();
    // =-=-=-=-=-=-=-
    // manufacture a stat as we do not have a
    // microservice to perform this duty
    _statbuf->st_mode  = S_IFREG;
    _statbuf->st_nlink = 1;
    _statbuf->st_uid   = getuid();
    _statbuf->st_gid   = getgid();
    _statbuf->st_atime = _statbuf->st_mtime = _statbuf->st_ctime = time( 0 );
    _statbuf->st_size  = UNKNOWN_FILE_SZ;
    return SUCCESS();

} // mso_file_stat_plugin

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error mso_file_rename(
    irods::plugin_context&,
    const char* ) {
    // =-=-=-=-=-=-=-
    // stub out
    return SUCCESS();

} // mso_file_rename_plugin

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error mso_file_truncate(
    irods::plugin_context& ) {
    // =-=-=-=-=-=-=-
    // stub out
    return SUCCESS();

} // mso_file_truncate_plugin

// =-=-=-=-=-=-=-
// interface for POSIX Unlink
irods::error mso_file_unlink(
    irods::plugin_context& ) {
    // =-=-=-=-=-=-=-
    // stub out
    return SUCCESS();

} // mso_file_unlink_plugin

// =-=-=-=-=-=-=-
// interface to stage files to cache resource from the archive
irods::error mso_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                     _cache_file_name ) {
    using namespace boost;
    namespace fs = boost::filesystem;

    // =-=-=-=-=-=-=-
    // ensure we have a valid plugin context
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the file_object from the context
    irods::file_object_ptr fco = boost::dynamic_pointer_cast <
                                 irods::file_object > ( _ctx.fco() );

    // =-=-=-=-=-=-=-
    // look for the magic token in the physical path
    std::string phy_path = fco->physical_path();
    size_t      pos      = phy_path.find_first_of( ":" );
    if ( std::string::npos == pos ) {
        std::string msg( "[:] not found in physical path for mso [" );
        msg += phy_path + "]";
        return ERROR(
                   MICRO_SERVICE_OBJECT_TYPE_UNDEFINED,
                   msg );
    }

    // =-=-=-=-=-=-=-
    // remove the first two chars from the phy path
    phy_path = phy_path.substr( 2, std::string::npos );

    // =-=-=-=-=-=-=-
    // substr the phy path to the : per
    // the syntax of an mso registered phy path
    std::string call_code = phy_path.substr( 0, pos - 2 );

    // =-=-=-=-=-=-=-
    // build a string to call the microservice per mso syntax
    std::string call_str( "msiobjget_" );
    call_str += call_code;

    std::list<boost::any> args;
    args.push_back( phy_path );
    args.push_back( lexical_cast< std::string >( fco->mode() ) );
    args.push_back( lexical_cast< std::string >( fco->flags() ) );
    args.push_back( std::string( _cache_file_name ) );

    // =-=-=-=-=-=-=-
    // prepare necessary artifacts for invocation of the rule engine
    ruleExecInfo_t rei;
    memset( &rei, 0, sizeof( ruleExecInfo_t ) );

    rei.rsComm = _ctx.comm();
    if ( _ctx.comm() != NULL ) {
        rei.uoic = &_ctx.comm()->clientUser;
        rei.uoip = &_ctx.comm()->proxyUser;
    }

    // =-=-=-=-=-=-=-
    // call the microservice via the rull engine
    int status = applyRuleWithInOutVars(
                     const_cast<char*>( call_str.c_str() ),
                     args,
                     &rei );

    // =-=-=-=-=-=-=-
    // handle error condition, rei may have more info
    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }

        return ERROR( status,
                      call_str );

    }

    // =-=-=-=-=-=-=-
    // win?
    return SUCCESS();

} // mso_file_stage_to_cache

// =-=-=-=-=-=-=-
// interface to sync file to the archive from the cache resource
irods::error mso_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                     _cache_file_name ) {
    using namespace boost;
    namespace fs = boost::filesystem;

    // =-=-=-=-=-=-=-
    // ensure we have a valid plugin context
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the file_object from the context
    irods::file_object_ptr fco = boost::dynamic_pointer_cast <
                                 irods::file_object > ( _ctx.fco() );

    // =-=-=-=-=-=-=-
    // determine if the cache file exists
    fs::path cache_path( _cache_file_name );
    if ( !fs::exists( cache_path ) ||
            !fs::is_regular_file( cache_path ) ) {
        return ERROR(
                   UNIX_FILE_STAT_ERR,
                   _cache_file_name );
    }

    // =-=-=-=-=-=-=-
    // compare data size of cache file to fco
    size_t file_size = fs::file_size( cache_path );
    //if ( fs::file_size( cache_path ) != fco->size() ) {
    //    return ERROR(
    //               SYS_COPY_LEN_ERR,
    //               _cache_file_name );
    //}

    // =-=-=-=-=-=-=-
    // look for the magic token in the physical path
    std::string phy_path = fco->physical_path();
    size_t      pos      = phy_path.find_first_of( ":" );
    if ( std::string::npos == pos ) {
        std::string msg( "[:] not found in physical path for mso [" );
        msg += phy_path + "]";
        return ERROR(
                   MICRO_SERVICE_OBJECT_TYPE_UNDEFINED,
                   msg );
    }

    // =-=-=-=-=-=-=-
    // remove the first two chars from the phy path
    phy_path = phy_path.substr( 2, std::string::npos );

    // =-=-=-=-=-=-=-
    // substr the phy path to the : per
    // the syntax of an mso registered phy path
    std::string call_code = phy_path.substr( 0, pos - 2 );

    // =-=-=-=-=-=-=-
    // build a string to call the microservice per mso syntax
    std::string call_str( "msiobjput_" );
    call_str += call_code;
    
    std::list<boost::any> args;
    args.push_back( phy_path );
    args.push_back(  std::string( _cache_file_name ) );
    args.push_back( lexical_cast< std::string >( file_size ) );

    // =-=-=-=-=-=-=-
    // prepare necessary artifacts for invocation of the rule engine
    ruleExecInfo_t rei;
    memset( &rei, 0, sizeof( ruleExecInfo_t ) );

    rei.rsComm = _ctx.comm();
    if ( _ctx.comm() != NULL ) {
        rei.uoic = &_ctx.comm()->clientUser;
        rei.uoip = &_ctx.comm()->proxyUser;
    }

    // =-=-=-=-=-=-=-
    // call the microservice via the rull engine
    int status = applyRuleWithInOutVars(
                     const_cast<char*>( call_str.c_str() ),
                     args,
                     &rei );

    // =-=-=-=-=-=-=-
    // handle error condition, rei may have more info
    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }

        return ERROR( status,
                      call_str );

    }

    // =-=-=-=-=-=-=-
    // win?
    return SUCCESS();



    return SUCCESS();

} // mso_file_sync_to_arch

// =-=-=-=-=-=-=-
// resolve_hierarchy_create - code to determine resolve_hierarchy for get operation
// Create never gets called on an archive.

// =-=-=-=-=-=-=-
// redirect_get - code to determine redirection for get operation
irods::error mso_file_resolve_hierarchy_open(
    irods::plugin_property_map& _prop_map,
    irods::file_object_ptr      _file_obj,
    const std::string&          _resc_name,
    const std::string&          _curr_host,
    float&                      _out_vote ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // initially set a good default
    _out_vote = 0.0;

    // =-=-=-=-=-=-=-
    // determine if the resource is down
    int resc_status = 0;
    irods::error get_ret = _prop_map.get< int >( irods::RESOURCE_STATUS, resc_status );
    if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"status\" property." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if ( INT_RESC_STATUS_DOWN != resc_status ) {

            // =-=-=-=-=-=-=-
            // get the resource host for comparison to curr host
            std::string host_name;
            get_ret = _prop_map.get< std::string >( irods::RESOURCE_LOCATION, host_name );
            if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"location\" property." ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // set a flag to test if were at the curr host, if so we vote higher
                bool curr_host = ( _curr_host == host_name );

                // =-=-=-=-=-=-=-
                // make some flags to clairify decision making
                bool need_repl = ( _file_obj->repl_requested() > -1 );

                // =-=-=-=-=-=-=-
                // set up variables for iteration
                bool          found     = false;
                std::vector< irods::physical_object > objs = _file_obj->replicas();
                std::vector< irods::physical_object >::iterator itr = objs.begin();

                // =-=-=-=-=-=-=-
                // check to see if the replica is in this resource, if one is requested
                for ( ; !found && itr != objs.end(); ++itr ) {

                    // =-=-=-=-=-=-=-
                    // run the hier string through the parser and get the last
                    // entry.
                    std::string last_resc;
                    irods::hierarchy_parser parser;
                    parser.set_string( itr->resc_hier() );
                    parser.last_resc( last_resc );

                    // =-=-=-=-=-=-=-
                    // more flags to simplify decision making
                    bool repl_us = ( _file_obj->repl_requested() == itr->repl_num() );
                    bool resc_us = ( _resc_name == last_resc );
                    // =-=-=-=-=-=-=-
                    // success - correct resource and dont need a specific
                    //           replication, or the repl nums match
                    if ( resc_us ) {
                        if ( !need_repl || ( need_repl && repl_us ) ) {
                            found = true;
                            if ( curr_host ) {
                                _out_vote = 1.0;
                            }
                            else {
                                _out_vote = 0.5;
                            }
                        }

                    } // if resc_us

                } // for itr
            }
        }
    }

    return result;

} // mso_file_resolve_hierarchy_open

// =-=-=-=-=-=-=-
// used to allow the resource to determine which host
// should provide the requested operation
irods::error mso_file_resolve_hierarchy(
    irods::plugin_context& _ctx,
    const std::string*                  _opr,
    const std::string*                  _curr_host,
    irods::hierarchy_parser*           _out_parser,
    float*                              _out_vote ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // check the context validity
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {

        if ( ( result = ASSERT_ERROR( _opr && _curr_host && _out_parser && _out_vote, SYS_INVALID_INPUT_PARAM,
                                      "Invalid input parameters." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // cast down the chain to our understood object type
            irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // get the name of this resource
            std::string resc_name;
            ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
            if ( ( result = ASSERT_PASS( ret, "Failed to get property for resource name." ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // add ourselves to the hierarchy parser by default
                _out_parser->add_child( resc_name );

                // =-=-=-=-=-=-=-
                // test the operation to determine which choices to make
                if ( irods::OPEN_OPERATION == ( *_opr ) ||
                        irods::WRITE_OPERATION == ( *_opr ) ||
                        irods::UNLINK_OPERATION == ( *_opr )) {
                    // =-=-=-=-=-=-=-
                    // call resolve_hierarchy determination for 'get' operation
                    result = mso_file_resolve_hierarchy_open( _ctx.prop_map(), file_obj, resc_name, ( *_curr_host ), ( *_out_vote ) );

                }
                else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
                    // =-=-=-=-=-=-=-
                    // call resolve_hierarchy determination for 'create' operation
                    result = ASSERT_ERROR( false, SYS_INVALID_INPUT_PARAM, "Create operation not supported for an archive" );
                }
                else {

                    // must have been passed a bad operation
                    result = ASSERT_ERROR( false, SYS_INVALID_INPUT_PARAM, "Operation not supported: \"%s\".",
                                           _opr->c_str() );
                }
            }
        }
    }

    return result;
} // mso_file_resolve_hierarchy_plugin

// =-=-=-=-=-=-=-
// mso_file_rebalance - code which would rebalance the subtree
irods::error mso_file_rebalance(
    irods::plugin_context& ) {
    return SUCCESS();

} // mso_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle mock_archive file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class mso_resource : public irods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
            public:
                maintenance_operation( const std::string& _n ) : name_( _n ) {
                }

                maintenance_operation( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                }

                maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                    return *this;
                }

                irods::error operator()( rcComm_t* ) {
                    rodsLog( LOG_NOTICE, "mso_resource::post_disconnect_maintenance_operation - [%s]",
                             name_.c_str() );
                    return SUCCESS();
                }

            private:
                std::string name_;

        }; // class maintenance_operation

    public:
        mso_resource( const std::string& _inst_name,
                      const std::string& _context ) :
            irods::resource( _inst_name, _context ) {
        } // ctor


        irods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            return ERROR( -1, "nop" );
        }

}; // class mso_resource

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
    // 4a. create mso_resource
    mso_resource* resc = new mso_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            mso_file_unlink ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            mso_file_stage_to_cache ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            mso_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            mso_file_mkdir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            mso_file_rename ) );

    resc->add_operation<struct stat*>(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            mso_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            mso_file_truncate ) );

    resc->add_operation<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            mso_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            mso_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory



