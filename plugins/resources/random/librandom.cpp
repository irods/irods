


// =-=-=-=-=-=-=-
// irods includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "generalAdmin.hpp"

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

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>

    
/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline irods::error random_check_params(
    irods::resource_plugin_context& _ctx ) { 
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }
   
    return SUCCESS();

} // random_check_params

/// =-=-=-=-=-=-=-
/// @brief get the next resource shared pointer given this resources name
///        as well as the object's hierarchy string 
irods::error get_next_child_in_hier( 
                  const std::string&          _name, 
                  const std::string&          _hier, 
                  irods::resource_child_map& _cmap, 
                  irods::resource_ptr&       _resc ) {

    // =-=-=-=-=-=-=-
    // create a parser and parse the string
    irods::hierarchy_parser parse;
    irods::error err = parse.set_string( _hier );
    if( !err.ok() ) {
        return PASSMSG( "get_next_child_in_hier - failed in set_string", err );
    }

    // =-=-=-=-=-=-=-
    // get the next resource in the series
    std::string next;
    err = parse.next( _name, next );
    if( !err.ok() ) {
        return PASSMSG( "get_next_child_in_hier - failed in next", err );
    }

    // =-=-=-=-=-=-=-
    // get the next resource from the child map
    if( !_cmap.has_entry( next ) ) {
        std::stringstream msg;
        msg << "get_next_child_in_hier - child map missing entry [";
        msg << next << "]";
        return ERROR( -1, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // assign resource
    _resc = _cmap[ next ].second;

    return SUCCESS();

} // get_next_child_in_hier

// =-=-=-=-=-=-=-
/// @brief get the resource for the child in the hierarchy
///        to pass on the call
template< typename DEST_TYPE >
irods::error random_get_resc_for_call( 
    irods::resource_plugin_context& _ctx,
    irods::resource_ptr&            _resc ) {
    // =-=-=-=-=-=-=-
    // check incoming parameters 
    irods::error err = random_check_params< DEST_TYPE >( _ctx );
    if( !err.ok() ) {
        return PASSMSG( "random_get_resc_for_call - bad resource context", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's name
    std::string name;
    err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
    if( !err.ok() ) {
        return PASSMSG( "random_get_resc_for_call - failed to get property 'name'.", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's hier string
    boost::shared_ptr< DEST_TYPE > dst_obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    std::string hier = dst_obj->resc_hier( );
  
    // =-=-=-=-=-=-=-
    // get the next child pointer given our name and the hier string
    err = get_next_child_in_hier( name, hier, _ctx.child_map(), _resc );
    if( !err.ok() ) {
        return PASSMSG( "random_get_resc_for_call - get_next_child_in_hier failed.", err );
    }

    return SUCCESS();

} // random_get_resc_for_call
extern "C" {
    // =-=-=-=-=-=-=-
    /// @brief Start Up Operation - initialize the random number generator
    irods::error random_start_operation( 
        irods::plugin_property_map& _prop_map,
        irods::resource_child_map&  _cmap ) {
        srand( time( NULL) );
        return SUCCESS();

    } // random_start_operation

    /// =-=-=-=-=-=-=-
    /// @brief given the property map the properties next_child and child_vector,
    ///        select the next property in the vector to be tapped as the RR resc
    irods::error random_get_next_child_resource( 
        irods::resource_child_map& _cmap,
        std::string&                _next_child ) {
        // =-=-=-=-=-=-=-
        // if the child map is empty then just return
        if( _cmap.size() <= 0 ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // get the size of the map and randomly pick an index into it
        double rand_number  = static_cast<double>( rand() );
        rand_number /= static_cast<double>( RAND_MAX );
        size_t target_index = round( ( _cmap.size() - 1 ) * rand_number );

        // =-=-=-=-=-=-=-
        // child map is keyed by resource name so we need to count out the index
        // and then snag the child name from the key of the hash map
        size_t counter = 0;
        std::string next_child;
        irods::resource_child_map::iterator itr = _cmap.begin();
        for( ; itr != _cmap.end(); ++itr ) {
            if( counter == target_index ) {
                next_child = itr->first;
                break;

            } else {
                ++counter;
            
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // assign the next_child to the out variable
        _next_child = next_child;

        return SUCCESS();

    } // random_get_next_child_resource

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    irods::error random_file_create( 
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call create on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );
   
    } // random_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error random_file_open( 
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
 
    } // random_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    irods::error random_file_read(
        irods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call read on the child 
        return resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
 
    } // random_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    irods::error random_file_write( 
        irods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call write on the child 
        return resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
 
    } // random_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    irods::error random_file_close(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call close on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
 
    } // random_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    irods::error random_file_unlink(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::data_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call unlink on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
 
    } // random_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    irods::error random_file_stat(
        irods::resource_plugin_context& _ctx,
        struct stat*                     _statbuf ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::data_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stat on the child 
        return resc->call< struct stat* >( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
 
    } // random_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    irods::error random_file_lseek(
        irods::resource_plugin_context& _ctx,
        long long                        _offset, 
        int                              _whence ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call lseek on the child 
        return resc->call< long long, int >( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
 
    } // random_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    irods::error random_file_mkdir(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call mkdir on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );

    } // random_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    irods::error random_file_rmdir(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rmdir on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );

    } // random_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    irods::error random_file_opendir(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call opendir on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );

    } // random_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    irods::error random_file_closedir(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call closedir on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );

    } // random_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    irods::error random_file_readdir(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call readdir on the child 
        return resc->call< struct rodsDirent** >( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );

    } // random_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    irods::error random_file_rename(
        irods::resource_plugin_context& _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );

    } // random_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    irods::error random_file_getfs_freespace(
        irods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call freespace on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );

    } // random_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    irods::error random_file_stage_to_cache(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child 
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_STAGE, _ctx.fco(), _cache_file_name );

    } // random_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    irods::error random_file_sync_to_arch(
        irods::resource_plugin_context& _ctx, 
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call synctoarch on the child 
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

    } // random_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error random_file_registered(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );

    } // random_file_registered
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error random_file_unregistered(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );

    } // random_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error random_file_modified(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );

    } // random_file_modified
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify a resource of an operation
    irods::error random_file_notify(
        irods::resource_plugin_context& _ctx,
        const std::string*               _opr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc; 
        irods::error err = random_get_resc_for_call< irods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( 
                   _ctx.comm(), 
                   irods::RESOURCE_OP_NOTIFY, 
                   _ctx.fco(), 
                   _opr );

    } // random_file_notify
 
    /// =-=-=-=-=-=-=-
    /// @brief find the next valid child resource for create operation
    irods::error get_next_valid_child_resource( 
        irods::resource_child_map&  _cmap,
        irods::resource_ptr&        _resc ) {
        // =-=-=-=-=-=-=-
        // flag
        bool   child_found = false;

        // =-=-=-=-=-=-=-
        // the pool of resources (children) to try
        std::vector<irods::resource_ptr> candidate_resources;

        // =-=-=-=-=-=-=-
        // copy children from _cmap
        irods::resource_child_map::iterator itr = _cmap.begin();
        for( ; itr != _cmap.end(); ++itr ) {
        	candidate_resources.push_back(itr->second.second);
        }
 
        // =-=-=-=-=-=-=-
        // while we have not found a child and still have candidates
        while( !child_found && !candidate_resources.empty() ) {

            // =-=-=-=-=-=-=-
            // generate random index
            double rand_number  = static_cast<double>( rand() );
            rand_number /= static_cast<double>( RAND_MAX );
            size_t rand_index = round( ( candidate_resources.size() - 1 ) * rand_number );
            
            // =-=-=-=-=-=-=-
            // pick resource in pool at random index
            irods::resource_ptr resc = candidate_resources[rand_index];

            // =-=-=-=-=-=-=-
            // get the resource's status
            int resc_status = 0;
            irods::error err = resc->get_property<int>( irods::RESOURCE_STATUS, resc_status ); 
            if( !err.ok() ) {
                return PASSMSG( "failed to get property", err );
            
            }

            // =-=-=-=-=-=-=-
            // determine if the resource is up and available
            if( INT_RESC_STATUS_DOWN != resc_status ) {
                // =-=-=-=-=-=-=-
                // we found a valid child, set out variable
                _resc = resc;
                child_found = true;

           } else {
               // =-=-=-=-=-=-=-
               // remove child from pool of candidates
        	   candidate_resources.erase(candidate_resources.begin()+rand_index);
           }

        } // while

        // =-=-=-=-=-=-=-
        // return appropriately
        if( child_found ) {
            return SUCCESS();
        
        } else {
            return ERROR( NO_NEXT_RESC_FOUND, "no valid child found" );
        
        }

    } // get_next_valid_child_resource

    /// =-=-=-=-=-=-=-
    /// @brief used to allow the resource to determine which host
    ///        should provide the requested operation
    irods::error random_redirect(
        irods::resource_plugin_context& _ctx, 
        const std::string*               _opr,
        const std::string*               _curr_host,
        irods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error err = random_check_params< irods::file_object >( _ctx );
        if( !err.ok() ) {
            return PASSMSG( "random_redirect - bad resource context", err );
        }
        if( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "random_redirect - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "random_redirect - null host" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "random_redirect - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "random_redirect - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // get the object's hier string
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        std::string hier = file_obj->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
        if( !err.ok() ) {
            return PASSMSG( "random_redirect - failed to get property 'name'.", err );
        }

        // =-=-=-=-=-=-=-
        // add ourselves into the hierarch before calling child resources
        _out_parser->add_child( name );
     
        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( irods::OPEN_OPERATION   == (*_opr)  || 
            irods::WRITE_OPERATION  == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next child pointer in the hierarchy, given our name and the hier string
            irods::resource_ptr resc; 
            err = get_next_child_in_hier( name, hier, _ctx.child_map(), resc );
            if( !err.ok() ) {
                return PASSMSG( "random_redirect - get_next_child_in_hier failed.", err );
            }

            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
            return resc->call< const std::string*, const std::string*, irods::hierarchy_parser*, float* >( 
                               _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), _opr, _curr_host, _out_parser, _out_vote );

        } else if( irods::CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next_child resource for create 
            irods::resource_ptr resc; 
            err = get_next_valid_child_resource( 
                      _ctx.child_map(), 
                      resc );
            if( !err.ok() ) {
                return PASS( err );

            }

            // =-=-=-=-=-=-=-
            // forward the 'create' redirect to the appropriate child
            err = resc->call< const std::string*, 
                              const std::string*, 
                              irods::hierarchy_parser*, 
                              float* >( 
                      _ctx.comm(), 
                      irods::RESOURCE_OP_RESOLVE_RESC_HIER, 
                      _ctx.fco(), 
                      _opr, 
                      _curr_host, 
                      _out_parser, 
                      _out_vote );
            if( !err.ok() ) {
                return PASS( err );
            
            }

            std::string new_hier;
            _out_parser->str( new_hier );
            
            // =-=-=-=-=-=-=-
            // win!
            return SUCCESS();
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "random_redirect - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // random_redirect

    // =-=-=-=-=-=-=-
    // random_file_rebalance - code which would rebalance the subtree
    irods::error random_file_rebalance(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // forward request for rebalance to children
        irods::error result = SUCCESS();
        irods::resource_child_map::iterator itr = _ctx.child_map().begin();
        for( ; itr != _ctx.child_map().end(); ++itr ) {
            irods::error ret = itr->second.second->call( 
                                    _ctx.comm(), 
                                    irods::RESOURCE_OP_REBALANCE, 
                                    _ctx.fco() );
            if( !ret.ok() ) {
                irods::log( PASS( ret ) );
                result = ret;
            }
        }

        return result;

    } // random_file_rebalancec

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
            //set_start_operation( "random_start_operation" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return ERROR( -1, "nop" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& _pdmo ) {
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
    irods::resource* plugin_factory( const std::string& _inst_name, 
                                      const std::string& _context  ) {
        // =-=-=-=-=-=-=-
        // 4a. create unixfilesystem_resource
        random_resource* resc = new random_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "random_file_create" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "random_file_open" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "random_file_read" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "random_file_write" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "random_file_close" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "random_file_unlink" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "random_file_stat" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "random_file_mkdir" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "random_file_opendir" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "random_file_readdir" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "random_file_rename" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "random_file_getfs_freespace" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "random_file_lseek" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "random_file_rmdir" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "random_file_closedir" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE, "random_file_stage_to_cache" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,   "random_file_sync_to_arch" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "random_file_registered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "random_file_unregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "random_file_modified" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,       "random_file_notify" );
        
        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER,     "random_redirect" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,             "random_file_rebalance" );

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



