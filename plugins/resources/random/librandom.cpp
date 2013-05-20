


// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "generalAdmin.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_physical_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_redirect.h"
#include "eirods_stacktrace.h"

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


extern "C" {

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;
 
    // =-=-=-=-=-=-=-
    /// @brief Start Up Operation - initialize the random number generator
    eirods::error random_start_operation( 
                  eirods::resource_property_map& _prop_map,
                  eirods::resource_child_map&    _cmap ) {
        srand( time( NULL) );
        return SUCCESS();

    } // random_start_operation

    /// =-=-=-=-=-=-=-
    /// @brief given the property map the properties next_child and child_vector,
    ///        select the next property in the vector to be tapped as the RR resc
    eirods::error random_get_next_child_resource( 
                      eirods::resource_child_map& _cmap,
                      std::string& _next_child ) {
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
        eirods::resource_child_map::iterator itr = _cmap.begin();
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
    /// @brief Check the general parameters passed in to most plugin functions
    inline eirods::error random_check_params(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the resource context
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "resource context is null" );
        }

        // =-=-=-=-=-=-=-
        // ask the context if it is valid
        eirods::error ret = _ctx->valid();
        if( !ret.ok() ) {
            return PASSMSG( "resource context is invalid", ret );

        }
       
        return SUCCESS();
 
    } // random_check_params

    /// =-=-=-=-=-=-=-
    /// @brief get the next resource shared pointer given this resources name
    ///        as well as the object's hierarchy string 
    eirods::error get_next_child_in_hier( 
                      const std::string&          _name, 
                      const std::string&          _hier, 
                      eirods::resource_child_map& _cmap, 
                      eirods::resource_ptr&       _resc ) {

        // =-=-=-=-=-=-=-
        // create a parser and parse the string
        eirods::hierarchy_parser parse;
        eirods::error err = parse.set_string( _hier );
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
    eirods::error random_get_resc_for_call( 
        eirods::resource_operation_context* _ctx,
        eirods::resource_ptr&               _resc ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = random_check_params( _ctx );
        if( !err.ok() ) {
            return PASSMSG( "random_get_resc_for_call - bad resource context", err );
        }
 
        // =-=-=-=-=-=-=-
        // get the object's name
        std::string name;
        err = _ctx->prop_map().get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "random_get_resc_for_call - failed to get property 'name'.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _ctx->fco().resc_hier( );
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        err = get_next_child_in_hier( name, hier, _ctx->child_map(), _resc );
        if( !err.ok() ) {
            return PASSMSG( "random_get_resc_for_call - get_next_child_in_hier failed.", err );
        }

        return SUCCESS();

    } // random_get_resc_for_call

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    eirods::error random_file_create( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call create on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_CREATE, _ctx->fco() );
   
    } // random_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error random_file_open( 
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_OPEN, _ctx->fco() );
 
    } // random_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    eirods::error random_file_read(
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call read on the child 
        return resc->call< void*, int >( _ctx->comm(), eirods::RESOURCE_OP_READ, _ctx->fco(), _buf, _len );
 
    } // random_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    eirods::error random_file_write( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call write on the child 
        return resc->call< void*, int >( _ctx->comm(), eirods::RESOURCE_OP_WRITE, _ctx->fco(), _buf, _len );
 
    } // random_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    eirods::error random_file_close(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call close on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_CLOSE, _ctx->fco() );
 
    } // random_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    eirods::error random_file_unlink(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call unlink on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_UNLINK, _ctx->fco() );
 
    } // random_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    eirods::error random_file_stat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stat on the child 
        return resc->call< struct stat* >( _ctx->comm(), eirods::RESOURCE_OP_STAT, _ctx->fco(), _statbuf );
 
    } // random_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Fstat
    eirods::error random_file_fstat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call fstat on the child 
        return resc->call< struct stat* >( _ctx->comm(), eirods::RESOURCE_OP_FSTAT, _ctx->fco(), _statbuf );
 
    } // random_file_fstat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    eirods::error random_file_lseek(
        eirods::resource_operation_context* _ctx,
        size_t                              _offset, 
        int                                 _whence ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call lseek on the child 
        return resc->call< size_t, int >( _ctx->comm(), eirods::RESOURCE_OP_LSEEK, _ctx->fco(), _offset, _whence );
 
    } // random_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX fsync
    eirods::error random_file_fsync(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call fsync on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_FSYNC, _ctx->fco() );
 
    } // random_file_fsync

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    eirods::error random_file_mkdir(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call mkdir on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_MKDIR, _ctx->fco() );

    } // random_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    eirods::error random_file_rmdir(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rmdir on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_RMDIR, _ctx->fco() );

    } // random_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    eirods::error random_file_opendir(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call opendir on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_OPENDIR, _ctx->fco() );

    } // random_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    eirods::error random_file_closedir(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call closedir on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx->fco() );

    } // random_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    eirods::error random_file_readdir(
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call readdir on the child 
        return resc->call< struct rodsDirent** >( _ctx->comm(), eirods::RESOURCE_OP_READDIR, _ctx->fco(), _dirent_ptr );

    } // random_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    eirods::error random_file_rename(
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call< const char* >( _ctx->comm(), eirods::RESOURCE_OP_RENAME, _ctx->fco(), _new_file_name );

    } // random_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    eirods::error random_file_getfs_freespace(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call freespace on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_FREESPACE, _ctx->fco() );

    } // random_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    eirods::error random_file_stage_to_cache(
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child 
        return resc->call< const char* >( _ctx->comm(), eirods::RESOURCE_OP_STAGE, _ctx->fco(), _cache_file_name );

    } // random_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    eirods::error random_file_sync_to_arch(
        eirods::resource_operation_context* _ctx, 
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call synctoarch on the child 
        return resc->call< const char* >( _ctx->comm(), eirods::RESOURCE_OP_SYNCTOARCH, _ctx->fco(), _cache_file_name );

    } // random_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error random_file_registered(
        eirods::resource_operation_context* _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_REGISTERED, _ctx->fco() );

    } // random_file_registered
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error random_file_unregistered(
        eirods::resource_operation_context* _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_UNREGISTERED, _ctx->fco() );

    } // random_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error random_file_modified(
        eirods::resource_operation_context* _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = random_get_resc_for_call( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx->comm(), eirods::RESOURCE_OP_MODIFIED, _ctx->fco() );

    } // random_file_modified

    /// =-=-=-=-=-=-=-
    /// @brief used to allow the resource to determine which host
    ///        should provide the requested operation
    eirods::error random_redirect(
        eirods::resource_operation_context* _ctx, 
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error err = random_check_params( _ctx );
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
        std::string hier = _ctx->fco().resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _ctx->prop_map().get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "random_redirect - failed to get property 'name'.", err );
        }

        // =-=-=-=-=-=-=-
        // add ourselves into the hierarch before calling child resources
        _out_parser->add_child( name );
     
        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next child pointer in the hierarchy, given our name and the hier string
            eirods::resource_ptr resc; 
            err = get_next_child_in_hier( name, hier, _ctx->child_map(), resc );
            if( !err.ok() ) {
                return PASSMSG( "random_redirect - get_next_child_in_hier failed.", err );
            }

            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
            return resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                               _ctx->comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx->fco(), _opr, _curr_host, _out_parser, _out_vote );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next_child resource for create 
            std::string next_child;
            err = random_get_next_child_resource( _ctx->child_map(), next_child );
            if( !err.ok() ) {
                return PASSMSG( "random_redirect - random_get_next_child_resource failed", err );

            }

            // =-=-=-=-=-=-=-
            // determine if the next child exists
            if( !_ctx->child_map().has_entry( next_child ) ) {
                std::stringstream msg;
                msg << "random_redirect - child map has no child by name [";
                msg << next_child << "]";
                return PASSMSG( msg.str(), err );
                    
            } 

            // =-=-=-=-=-=-=-
            // get the next_child resource 
            eirods::resource_ptr resc = _ctx->child_map()[ next_child ].second;

            // =-=-=-=-=-=-=-
            // forward the 'put' redirect to the appropriate child
            err = resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                               _ctx->comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx->fco(), _opr, _curr_host, _out_parser, _out_vote );
            if( !err.ok() ) {
                return PASSMSG( "random_redirect - forward of put redirect failed", err );
            
            }

            std::string new_hier;
            _out_parser->str( new_hier );
            
            // =-=-=-=-=-=-=-
            // update the next_child appropriately as the above succeeded
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
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class random_resource : public eirods::resource {
    public:
        random_resource( const std::string& _inst_name, 
                             const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
            //set_start_operation( "random_start_operation" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return ERROR( -1, "nop" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _pdmo ) {
            return ERROR( -1, "nop" );
        }

    }; // class 

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, 
                                      const std::string& _context  ) {
        // =-=-=-=-=-=-=-
        // 4a. create unixfilesystem_resource
        random_resource* resc = new random_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "random_file_create" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "random_file_open" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "random_file_read" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "random_file_write" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "random_file_close" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "random_file_unlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "random_file_stat" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "random_file_fstat" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "random_file_fsync" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "random_file_mkdir" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "random_file_opendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "random_file_readdir" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "random_file_rename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "random_file_getfs_freespace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "random_file_lseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "random_file_rmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "random_file_closedir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "random_file_stage_to_cache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "random_file_sync_to_arch" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "random_file_registered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "random_file_unregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "random_file_modified" );
        
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "random_redirect" );

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



