/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "generalAdmin.h"
#include "physPath.h"
#include "reIn2p3SysRule.h"

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
#include "eirods_kvp_string_parser.h"

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
/// @ brief constant to reference the operation type for 
///         file modification
const std::string OPERATION_TYPE( "operation_type" );

/// =-=-=-=-=-=-=-
/// @ brief constant to index the cache child resource
const std::string CACHE_CONTEXT_TYPE( "cache" );

/// =-=-=-=-=-=-=-
/// @ brief constant to index the archive child resource
const std::string ARCHIVE_CONTEXT_TYPE( "archive" );

/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline eirods::error compound_check_param(
    eirods::resource_plugin_context& _ctx ) { 
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    eirods::error ret = _ctx.valid< DEST_TYPE >();
    if( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }
   
    return SUCCESS();

} // compound_check_param

// =-=-=-=-=-=-=-
/// @brief helper function to get the next child in the hier string
///        for use when forwarding an operation
template< typename DEST_TYPE >
eirods::error get_next_child( 
    eirods::resource_plugin_context& _ctx,
    eirods::resource_ptr&            _resc ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    eirods::error ret = compound_check_param< DEST_TYPE >(_ctx);
    if(!ret.ok()) {
        return PASSMSG( "invalid resource context", ret);
    }
    // =-=-=-=-=-=-=-
    // get the resource name
    std::string name;
    ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, name );
    if( !ret.ok() ) {
        PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the resource after this resource
    eirods::hierarchy_parser parser;
    boost::shared_ptr< DEST_TYPE > dst_obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    parser.set_string( dst_obj->resc_hier() );

    std::string child;
    ret = parser.next( name, child );
    if( !ret.ok() ) {
        PASS( ret );
    }
    
    // =-=-=-=-=-=-=-
    // extract the next resource from the child map
    if( _ctx.child_map().has_entry( child ) ) {
        std::pair< std::string, eirods::resource_ptr > resc_pair;
        ret = _ctx.child_map().get( child, resc_pair );
        if( !ret.ok() ) {
            return PASS( ret ); 
        } else {
            _resc = resc_pair.second;
            return SUCCESS();
        }
        
    } else {
        std::stringstream msg;
        msg << "child not found [" << child << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

    }

} // get_next_child

/// =-=-=-=-=-=-=-
/// @brief helper function to get the cache resource 
eirods::error get_cache( 
    eirods::resource_plugin_context& _ctx,
    eirods::resource_ptr&            _resc ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
    if(!ret.ok()) {
        return PASSMSG( "invalid resource context", ret);
    }

    // =-=-=-=-=-=-=-
    // get the cache name
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, resc_name );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the resource from the child map
    std::pair< std::string, eirods::resource_ptr > resc_pair;
    ret = _ctx.child_map().get( resc_name, resc_pair );
    if( !ret.ok() ) {
        std::stringstream msg;
        msg << "failed to get child resource [" << resc_name << "]";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // assign the resource to the out variable
    _resc = resc_pair.second;

    return SUCCESS();

} // get_cache

/// =-=-=-=-=-=-=-
/// @brief helper function to get the archive resource 
eirods::error get_archive( 
    eirods::resource_plugin_context& _ctx,
    eirods::resource_ptr&               _resc ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
    if(!ret.ok()) {
        return PASSMSG( "invalid resource context", ret);
    }

    // =-=-=-=-=-=-=-
    // get the archive name
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( ARCHIVE_CONTEXT_TYPE, resc_name );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the resource from the child map
    std::pair< std::string, eirods::resource_ptr > resc_pair;
    ret = _ctx.child_map().get( resc_name, resc_pair );
    if( !ret.ok() ) {
        std::stringstream msg;
        msg << "failed to get child resource [" << resc_name << "]";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // assign the resource to the out variable
    _resc = resc_pair.second;
    return SUCCESS();

} // get_archive

/// =-=-=-=-=-=-=-
/// @brief Returns the cache resource making sure it corresponds to the fco resc hier
template< typename DEST_TYPE >
eirods::error get_cache_resc(
    eirods::resource_plugin_context& _ctx,
    eirods::resource_ptr&            _resc ) {
    eirods::error result = SUCCESS();
    eirods::error ret;
    eirods::resource_ptr next_resc;
    
    // =-=-=-=-=-=-=-
    // get the cache resource
    if( !(ret = get_cache( _ctx, _resc )).ok() ) {
        std::stringstream msg;
        msg << "Failed to get cache resource.";
        result = PASSMSG(msg.str(), ret);
    }

    // get the next child resource in the hierarchy
    else if(!(ret = get_next_child< DEST_TYPE >(_ctx, next_resc)).ok()) {
        std::stringstream msg;
        msg << "Failed to get next child resource.";
        result = PASSMSG(msg.str(), ret);
    }

    // Make sure the file object came from the cache resource
    else if(_resc != next_resc) {
        boost::shared_ptr< DEST_TYPE > obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
        std::stringstream msg;
        msg << "Cannot open data object: \"";
        msg << obj->physical_path();
        msg << "\" It is stored in an archive resource which is not directly accessible.";
        result = ERROR(EIRODS_DIRECT_ARCHIVE_ACCESS, msg.str());
    }

    return result;

} // get_cache_resc

extern "C" {
    // =-=-=-=-=-=-=-
    /// @brief helper function to take a rule result, find a keyword and then
    ///        parse it for the value
    eirods::error get_stage_policy(
        const std::string& _results,
        std::string& _policy ) {
        // =-=-=-=-=-=-=-
        // get a map of the key value pairs
        eirods::kvp_map_t kvp;
        eirods::error kvp_err = eirods::parse_kvp_string( 
                                    _results,
                                    kvp );
        if( !kvp_err.ok() ) {
            return PASS( kvp_err );
        }

        std::string value = kvp[ eirods::RESOURCE_STAGE_TO_CACHE_POLICY ];
        if( value.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "stage policy value not found" );
        }
        _policy = value;

        return SUCCESS();

    } // get_stage_policy

    // =-=-=-=-=-=-=-
    /// @brief start up operation - determine which child is the cache and which is the
    ///        archive.  cache those names in local variables for ease of use
    eirods::error compound_start_operation(
        eirods::plugin_property_map& _prop_map,
        eirods::resource_child_map&  _cmap ) {
        // =-=-=-=-=-=-=-
        // trap invalid number of children
        if( _cmap.size() == 0 || _cmap.size() > 2 ) {
            std::stringstream msg;
            msg << "compound resource: invalid number of children [";
            msg << _cmap.size() << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // child map is indexed by name, get first name
        std::string first_child_name;
        eirods::resource_child_map::iterator itr = _cmap.begin();
        if( itr == _cmap.end() ) {
            return ERROR( -1, "child map is empty" );
        }
        
        std::string           first_child_ctx  = itr->second.first;
        eirods::resource_ptr& first_child_resc = itr->second.second;
        eirods::error get_err = first_child_resc->get_property<std::string>( eirods::RESOURCE_NAME, first_child_name );
        if( !get_err.ok() ) {
            return PASS( get_err );
        }

        // =-=-=-=-=-=-=-
        // get second name
        std::string second_child_name;
        itr++;
        if( itr == _cmap.end() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "child map has only one entry" );
        }

        std::string          second_child_ctx  = itr->second.first;
        eirods::resource_ptr second_child_resc = itr->second.second;
        get_err = second_child_resc->get_property<std::string>( eirods::RESOURCE_NAME, second_child_name );
        if( !get_err.ok() ) {
            return PASS( get_err );
        }
        
        // =-=-=-=-=-=-=-
        // now if first name is a cache, store that name as such
        // otherwise it is the archive
        // cmap is a hash map whose payload is a pair< string, resource_ptr >
        // the first of which is the parent-child context string denoting 
        // that either the child is a cache or archive
        if( first_child_ctx == CACHE_CONTEXT_TYPE ) {
            _prop_map[ CACHE_CONTEXT_TYPE ] = first_child_name;

        } else if( first_child_ctx == ARCHIVE_CONTEXT_TYPE ) {
            _prop_map[ ARCHIVE_CONTEXT_TYPE ] = first_child_name;

        } else {
            return ERROR( EIRODS_INVALID_RESC_CHILD_CONTEXT, first_child_ctx );

        }

        // =-=-=-=-=-=-=-
        // do the same for the second resource
        if( second_child_ctx == CACHE_CONTEXT_TYPE ) {
            _prop_map[ CACHE_CONTEXT_TYPE ] = second_child_name;

        } else if( second_child_ctx == ARCHIVE_CONTEXT_TYPE ) {
            _prop_map[ ARCHIVE_CONTEXT_TYPE ] = second_child_name;

        } else {
            return ERROR( EIRODS_INVALID_RESC_CHILD_CONTEXT, second_child_ctx );

        }

        // =-=-=-=-=-=-=-
        // if both context strings match, this is also an error
        if( first_child_ctx == second_child_ctx ) {
            std::stringstream msg;
            msg << "matching child context strings :: ";
            msg << "[" << first_child_ctx  << "] vs ";
            msg << "[" << second_child_ctx << "]";
            return ERROR( EIRODS_INVALID_RESC_CHILD_CONTEXT, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // and... were done.
        return SUCCESS();

    } // compound_start_operation
    
    /// =-=-=-=-=-=-=-
    /// @brief replicate a given object for either a sync or a stage
    eirods::error repl_object(
        eirods::resource_plugin_context& _ctx,
        const char*                      _stage_sync_kw ) {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // error check incoming params
        if((result = ASSERT_ERROR( _stage_sync_kw || strlen(_stage_sync_kw) != 0, SYS_INVALID_INPUT_PARAM, "Null or empty _stage_sync_kw.")).ok()) {

            // =-=-=-=-=-=-=-
            // get the file object from the fco
            eirods::file_object_ptr obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // get the root resource to pass to the cond input
            eirods::hierarchy_parser parser;
            parser.set_string( obj->resc_hier() );

            std::string resource;
            parser.first_resc( resource );
            // =-=-=-=-=-=-=-
            // get the parent name
            std::string parent_name;
            eirods::error ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_PARENT, parent_name );
            if((result = ASSERT_PASS(ret, "Failed to get the parent name.")).ok() ) {
                 
                // =-=-=-=-=-=-=-
                // get the cache name
                std::string cache_name;
                eirods::error ret = _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, cache_name );
                if((result = ASSERT_PASS(ret, "Failed to get the cache name.")).ok() ) {

                    // =-=-=-=-=-=-=-
                    // get the archive name
                    std::string arch_name;
                    ret = _ctx.prop_map().get< std::string >( ARCHIVE_CONTEXT_TYPE, arch_name );
                    if((result = ASSERT_PASS(ret, "Failed to get the archive name.")).ok() ) {

                        // =-=-=-=-=-=-=-
                        // manufacture a resc hier to either the archive or the cache resc
                        std::string keyword  = _stage_sync_kw;
                        std::string inp_hier = obj->resc_hier();
                        
                        std::string tgt_name, src_name;
                        if( keyword == STAGE_OBJ_KW ) {
                            tgt_name = cache_name;
                            src_name = arch_name;
                        } else if( keyword == SYNC_OBJ_KW ) {
                            tgt_name = arch_name;
                            src_name = cache_name;
                        } else {
                            std::stringstream msg;
                            msg << "stage_sync_kw value is unexpected [" << _stage_sync_kw << "]";
                            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                        }

                        std::string current_name;
                        ret = _ctx.prop_map().get<std::string>( eirods::RESOURCE_NAME, current_name);
                        if((result = ASSERT_PASS(ret, "Failed to get the resource name.")).ok()) {
                            size_t pos = inp_hier.find( parent_name );
                            if( std::string::npos == pos ) {
                                std::stringstream msg;
                                msg << "parent resc ["
                                    << parent_name
                                    << "] not in fco resc hier ["
                                    << inp_hier
                                    << "]";
                                return ERROR( 
                                           SYS_INVALID_INPUT_PARAM,
                                           msg.str() ); 
                            }

                            // =-=-=-=-=-=-=- 
                            // Generate src and tgt hiers
                            std::string dst_hier = inp_hier.substr( 0, pos+parent_name.size() );
                            if( !dst_hier.empty() ) {
                                dst_hier += eirods::hierarchy_parser::delimiter();
                            }
                            dst_hier += current_name + 
                                        eirods::hierarchy_parser::delimiter() + 
                                        tgt_name;
                            
                            std::string src_hier = inp_hier.substr( 0, pos+parent_name.size() );
                            if( !src_hier.empty() ) {
                                src_hier += eirods::hierarchy_parser::delimiter();
                            }
                            src_hier += current_name + 
                                        eirods::hierarchy_parser::delimiter() + 
                                        src_name;
                            
                            // =-=-=-=-=-=-=- 
                            // Generate sub hier to use for pdmo
                            parser.set_string(src_hier);
                            std::string sub_hier;
                            parser.str(sub_hier, current_name);

                            // =-=-=-=-=-=-=-
                            // create a data obj input struct to call rsDataObjRepl which given
                            // the _stage_sync_kw will either stage or sync the data object 
                            dataObjInp_t data_obj_inp;
                            bzero( &data_obj_inp, sizeof( data_obj_inp ) );
                            rstrcpy( data_obj_inp.objPath, obj->logical_path().c_str(), MAX_NAME_LEN );
                            data_obj_inp.createMode = obj->mode();
                            addKeyVal( &data_obj_inp.condInput, RESC_HIER_STR_KW,      src_hier.c_str() );
                            addKeyVal( &data_obj_inp.condInput, DEST_RESC_HIER_STR_KW, dst_hier.c_str() );
                            addKeyVal( &data_obj_inp.condInput, RESC_NAME_KW,          resource.c_str() );
                            addKeyVal( &data_obj_inp.condInput, DEST_RESC_NAME_KW,     resource.c_str() );
                            addKeyVal( &data_obj_inp.condInput, IN_PDMO_KW,            sub_hier.c_str() );
                            addKeyVal( &data_obj_inp.condInput, _stage_sync_kw,        "1" );

                            transferStat_t* trans_stat = NULL;
                            int status = rsDataObjRepl( _ctx.comm(), &data_obj_inp, &trans_stat );
                            if( status < 0 ) {
                                char* sys_error;
                                char* rods_error = rodsErrorName(status, &sys_error);
                                std::stringstream msg;
                                msg << "Failed to replicate the data object [" << obj->logical_path() << "] ";
                                msg << "for operation [" << _stage_sync_kw << "]";
                                return ERROR( status, msg.str() );
                            }
             
                        } // if current_name

                    } // if arch_name

                } // if cache_name
            
            } // if parent name

        } // if stage_sync_kw 
        return result;

    } // repl_object

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    eirods::error compound_file_create( 
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::file_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_CREATE, _ctx.fco() );

    } // compound_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error compound_file_open( 
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr cache_resc;
        if( !(ret = get_cache_resc< eirods::file_object >( _ctx, cache_resc )).ok() ) {
            std::stringstream msg;
            msg << "Failed to get cache resource.";
            return PASSMSG(msg.str(), ret);
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return cache_resc->call( _ctx.comm(), eirods::RESOURCE_OP_OPEN, _ctx.fco() );
        
    } // compound_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    eirods::error compound_file_read(
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr resc;
        ret = get_cache( _ctx, resc );
        if( !ret.ok() ) {
            return PASSMSG("Unable to get cache resource.", ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call< void*, int >( _ctx.comm(), eirods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );

    } // compound_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    eirods::error compound_file_write( 
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr resc;
        ret = get_cache( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call< void*, int >( _ctx.comm(), eirods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
 
    } // compound_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    eirods::error compound_file_close(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr resc;
        ret = get_cache( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        ret = resc->call( _ctx.comm(), eirods::RESOURCE_OP_CLOSE, _ctx.fco() );
        if( !ret.ok() ) {
            return PASS( ret );
        
        }

        return SUCCESS();
          
    } // compound_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    eirods::error compound_file_unlink(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::data_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::data_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_UNLINK, _ctx.fco() );

    } // compound_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    eirods::error compound_file_stat(
        eirods::resource_plugin_context& _ctx,
        struct stat*                     _statbuf ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::data_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::data_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call< struct stat* >( _ctx.comm(), eirods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );

    } // compound_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    eirods::error compound_file_lseek(
        eirods::resource_plugin_context& _ctx,
        long long                        _offset, 
        int                              _whence ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::file_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call< long long, int >( _ctx.comm(), eirods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
 
    } // compound_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    eirods::error compound_file_mkdir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::collection_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::collection_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_MKDIR, _ctx.fco() );

    } // compound_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    eirods::error compound_file_rmdir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::collection_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::collection_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_RMDIR, _ctx.fco() );

    } // compound_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    eirods::error compound_file_opendir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::collection_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::collection_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_OPENDIR, _ctx.fco() );

    } // compound_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    eirods::error compound_file_closedir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::collection_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::collection_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );

    } // compound_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    eirods::error compound_file_readdir(
        eirods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::collection_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::collection_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call< struct rodsDirent** >( _ctx.comm(), eirods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );

    } // compound_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    eirods::error compound_file_rename(
        eirods::resource_plugin_context& _ctx,
        const char*                      _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::data_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::data_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call<const char*>( _ctx.comm(), eirods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );

    } // compound_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    eirods::error compound_file_getfs_freespace(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the context for validity
        eirods::error ret = compound_check_param< eirods::data_object >(_ctx);
        if(!ret.ok()) {
            return PASSMSG( "invalid resource context", ret);
        }

        // =-=-=-=-=-=-=-
        // get the next child resource
        eirods::resource_ptr resc;
        ret = get_next_child< eirods::data_object >( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call
        return resc->call( 
                   _ctx.comm(), 
                   eirods::RESOURCE_OP_FREESPACE, 
                   _ctx.fco() );

    } // compound_file_getfs_freespace


    /// =-=-=-=-=-=-=-
    /// @brief Move data from a the archive leaf node to a local cache leaf node
    ///        This routine is not supported from a coordinating node's perspective
    ///        the coordinating node would be calling this on a leaf when necessary
    eirods::error compound_file_stage_to_cache(
        eirods::resource_plugin_context& _ctx,
        const char*                      _cache_file_name ) { 
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid resource context";
            return PASSMSG(msg.str(), ret);
        }
      
        // =-=-=-=-=-=-=-
        // get the archive resource
        eirods::resource_ptr resc;
        ret = get_archive( _ctx, resc ); 
        if( !ret.ok() ) {
            return PASS( ret );
        }

        eirods::file_object_ptr ptr = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // forward the call to the archive
        return resc->call< const char* >( _ctx.comm(), eirods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );

    } // compound_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief Move data from a the cache leaf node to a local archive leaf node
    ///        This routine is not supported from a coordinating node's perspective
    ///        the coordinating node would be calling this on a leaf when necessary
    eirods::error compound_file_sync_to_arch(
        eirods::resource_plugin_context& _ctx, 
        const char*                      _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid resource context";
            return PASSMSG(msg.str(), ret);
        }
      
        // =-=-=-=-=-=-=-
        // get the archive resource
        eirods::resource_ptr resc;
        get_archive( _ctx, resc ); 
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // forward the call to the archive
        return resc->call< const char* >( _ctx.comm(), eirods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

    } // compound_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error compound_file_registered(
        eirods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid resource context";
            return PASSMSG(msg.str(), ret);
        }

        return SUCCESS();

    } // compound_file_registered
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error compound_file_unregistered(
        eirods::resource_plugin_context& _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = compound_check_param< eirods::file_object >(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid resource context";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();

    } // compound_file_unregistered
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification - this happens
    ///        after the close operation and the icat should be up to date
    ///        at this point
    eirods::error compound_file_modified(
        eirods::resource_plugin_context& _ctx )
    {
       eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=- 
        // Check the operation parameters and update the physical path
        eirods::error ret = compound_check_param< eirods::file_object >( _ctx );
        if(( result = ASSERT_PASS(ret, "Invalid resource context.")).ok()) {
            std::string operation;
            ret = _ctx.prop_map().get< std::string >( OPERATION_TYPE, operation );
            if( ret.ok() ) {
                std::string name;
                ret = _ctx.prop_map().get<std::string>( eirods::RESOURCE_NAME, name);
                if((result = ASSERT_PASS(ret, "Failed to get the resource name.")).ok()) {
                    eirods::file_object_ptr file_obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
                    eirods::hierarchy_parser sub_parser;
                    sub_parser.set_string(file_obj->in_pdmo());
                    if(!sub_parser.resc_in_hier(name)) {
                        result = repl_object( _ctx, SYNC_OBJ_KW );
                    }
                }
            }
        }
        
        return result;

    } // compound_file_modified

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error compound_file_redirect_create( 
        eirods::resource_plugin_context& _ctx,
        const std::string&               _operation,
        const std::string&               _resc_name, 
        const std::string*               _curr_host, 
        eirods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error ret = _ctx.prop_map().get< int >( eirods::RESOURCE_STATUS, resc_status );
        if( !ret.ok() ) {
            return PASSMSG( "failed to get 'status' property", ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            (*_out_vote ) = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr resc;
        ret = get_cache( _ctx, resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // ask the cache if it is willing to accept a new file, politely
        ret = resc->call< const std::string*, const std::string*, 
            eirods::hierarchy_parser*, float* >( 
                _ctx.comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), 
                &_operation, _curr_host, 
                _out_parser, _out_vote );

        // =-=-=-=-=-=-=-
        // set the operation type to signal that we need to do some work
        // in file modified
        _ctx.prop_map().set< std::string >( OPERATION_TYPE, _operation );

        return ret;

    } // compound_file_redirect_create

    // =-=-=-=-=-=-=-
    /// @brief - handler for prefer archive policy
    eirods::error open_for_prefer_archive_policy( 
        eirods::resource_plugin_context& _ctx,
        const std::string*               _curr_host, 
        eirods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
        }
 
        // =-=-=-=-=-=-=-
        // get the archive resource
        eirods::resource_ptr arch_resc;
        eirods::error ret = get_archive( _ctx, arch_resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }
  
        // =-=-=-=-=-=-=-
        // get the archive resource
        eirods::resource_ptr cache_resc;
        ret = get_cache( _ctx, cache_resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }
       
        // =-=-=-=-=-=-=-
        // repave the repl requested temporarily
        eirods::file_object_ptr f_ptr = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        int repl_requested = f_ptr->repl_requested();
        f_ptr->repl_requested( -1 );
       
        // =-=-=-=-=-=-=-
        // ask the archive if it has the data object in question, politely
        float                    arch_check_vote   = 0.0;
        eirods::hierarchy_parser arch_check_parser = (*_out_parser);
        ret = arch_resc->call< const std::string*, const std::string*, 
            eirods::hierarchy_parser*, float* >( 
                _ctx.comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), 
                &eirods::EIRODS_OPEN_OPERATION, _curr_host, 
                &arch_check_parser, &arch_check_vote );
        if( !ret.ok() || 0.0 == arch_check_vote ) {
            rodsLog( 
                LOG_NOTICE, 
                "replica not found in archive for [%s]",
                f_ptr->logical_path().c_str() );
            // =-=-=-=-=-=-=-
            // the archive query redirect failed, something terrible happened
            // or mounted collection hijinks are afoot.  ask the cache if it
            // has the data object in question, politely as a fallback
            float                    cache_check_vote   = 0.0;
            eirods::hierarchy_parser cache_check_parser = (*_out_parser);
            ret = cache_resc->call< const std::string*, const std::string*, 
                eirods::hierarchy_parser*, float* >( 
                    _ctx.comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), 
                    &eirods::EIRODS_OPEN_OPERATION, _curr_host, 
                    &cache_check_parser, &cache_check_vote );
            if( !ret.ok() || 0.0 == cache_check_vote ) {
                return PASS( ret );
            }
            
            // =-=-=-=-=-=-=-
            // set the vote and hier parser
            (*_out_parser) = cache_check_parser;
            (*_out_vote)   = cache_check_vote;

            return SUCCESS();

        }
            
        // =-=-=-=-=-=-=-
        // repave the resc hier with the archive hier which guarantees that
        // we are in the hier for the repl to do its magic. this is a hack,
        // and will need refactored later with an improved object model
        std::string arch_hier;
        arch_check_parser.str( arch_hier );
        f_ptr->resc_hier( arch_hier );

        // =-=-=-=-=-=-=-
        // if the vote is 0 then we do a wholesale stage, not an update
        // otherwise it is an update operation for the stage to cache
        ret = repl_object( _ctx, STAGE_OBJ_KW );
        if( !ret.ok() ) {
            return PASS( ret );    
        }
       
        // =-=-=-=-=-=-=-
        // restore repl requested
        f_ptr->repl_requested( repl_requested );
 
        // =-=-=-=-=-=-=-
        // get the parent name
        std::string parent_name;
        ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_PARENT, parent_name );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // get this resc name
        std::string current_name;
        ret = _ctx.prop_map().get<std::string>( eirods::RESOURCE_NAME, current_name);
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // get the cache name
        std::string cache_name;
        ret = _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, cache_name );
        if( !ret.ok() ) {
            return PASS( ret );
        }
                        
                        
        // =-=-=-=-=-=-=-
        // get the current hier
        eirods::file_object_ptr obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        std::string inp_hier = obj->resc_hier();
        
        // =-=-=-=-=-=-=-
        // find the parent in the hier
        size_t pos = inp_hier.find( parent_name );
        if( std::string::npos == pos ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "parent resc not in fco resc hier" ); 
        }

        // =-=-=-=-=-=-=-
        // create the new resc hier
        std::string dst_hier = inp_hier.substr( 0, pos+parent_name.size() );
        if( !dst_hier.empty() ) {
            dst_hier += eirods::hierarchy_parser::delimiter();
        }
        dst_hier += current_name + 
                    eirods::hierarchy_parser::delimiter() + 
                    cache_name;

        // =-=-=-=-=-=-=-
        // set the vote and hier parser
        _out_parser->set_string( dst_hier );
        (*_out_vote) = arch_check_vote;

        return SUCCESS();

    } // open_for_prefer_archive_policy

    // =-=-=-=-=-=-=-
    /// @brief - handler for prefer cache policy
    eirods::error open_for_prefer_cache_policy( 
        eirods::resource_plugin_context& _ctx,
        const std::string*               _curr_host, 
        eirods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
        }
         
        // =-=-=-=-=-=-=-
        // get the cache resource
        eirods::resource_ptr cache_resc;
        eirods::error ret = get_cache( _ctx, cache_resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // get the archive resource
        eirods::resource_ptr arch_resc;
        ret = get_archive( _ctx, arch_resc );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // ask the cache if it has the data object in question, politely
        float                    cache_check_vote   = 0.0;
        eirods::hierarchy_parser cache_check_parser = (*_out_parser);
        ret = cache_resc->call< const std::string*, const std::string*, 
            eirods::hierarchy_parser*, float* >( 
                _ctx.comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), 
                &eirods::EIRODS_OPEN_OPERATION, _curr_host, 
                &cache_check_parser, &cache_check_vote );

        // =-=-=-=-=-=-=-
        // if the vote is 0 then the cache doesnt have it so it will need be staged
        if( 0.0 == cache_check_vote ) {
            // =-=-=-=-=-=-=-
            // repave the repl requested temporarily
            eirods::file_object_ptr f_ptr = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
            int repl_requested = f_ptr->repl_requested();
            f_ptr->repl_requested( -1 );

            // =-=-=-=-=-=-=-
            // ask the archive if it has the data object in question, politely
            float                    arch_check_vote   = 0.0;
            eirods::hierarchy_parser arch_check_parser = (*_out_parser);
            ret = arch_resc->call< const std::string*, const std::string*, 
                eirods::hierarchy_parser*, float* >( 
                    _ctx.comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), 
                    &eirods::EIRODS_OPEN_OPERATION, _curr_host, 
                    &arch_check_parser, &arch_check_vote );
            if( !ret.ok() || 0.0 == arch_check_vote ) {
                return PASS( ret );    
            }
            
            // =-=-=-=-=-=-=-
            // repave the resc hier with the archive hier which guarantees that
            // we are in the hier for the repl to do its magic. this is a hack,
            // and will need refactored later with an improved object model
            std::string arch_hier;
            arch_check_parser.str( arch_hier );
            f_ptr->resc_hier( arch_hier );

            // =-=-=-=-=-=-=-
            // if the archive has it, then replicate
            ret = repl_object( _ctx, STAGE_OBJ_KW );
            if( !ret.ok() ) {
                return PASS( ret );    
            }

            // =-=-=-=-=-=-=-
            // restore repl requested
            f_ptr->repl_requested( repl_requested );

            // =-=-=-=-=-=-=-
            // now that the repl happend, we will assume that the
            // object is in the cache as to not hit the DB again
            (*_out_parser) = cache_check_parser;
            (*_out_vote  ) = arch_check_vote;

        } else {
            // =-=-=-=-=-=-=-
            // else it is in the cache so assign the parser 
            (*_out_vote)   = cache_check_vote;
            (*_out_parser) = cache_check_parser; 
        }

        return SUCCESS();

    } // open_for_prefer_cache_policy

    // =-=-=-=-=-=-=-
    /// @brief - code to determine redirection for the open operation
    ///          determine the user set policy for staging to cache
    ///          otherwise the default is to compare checsum
    eirods::error compound_file_redirect_open( 
        eirods::resource_plugin_context& _ctx,
        const std::string*               _curr_host, 
        eirods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
        }

        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error ret = _ctx.prop_map().get< int >( eirods::RESOURCE_STATUS, resc_status );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            (*_out_vote ) = 0.0;
            return SUCCESS(); 
        }
        
        // =-=-=-=-=-=-=-
        // acquire the value of the stage policy from the results string
        std::string policy;
        ret = get_stage_policy( _ctx.rule_results(), policy );

        // =-=-=-=-=-=-=-
        // if the policy is prefer cache then if the cache has the object 
        // return an upvote
        if( policy.empty() || eirods::RESOURCE_STAGE_PREFER_CACHE == policy ) {
            return open_for_prefer_cache_policy( _ctx, _curr_host, _out_parser, _out_vote );
        
        } 
        
        // =-=-=-=-=-=-=-
        // if the policy is always, then if the archive has it 
        // stage from archive to cache and return an upvote
        else if( eirods::RESOURCE_STAGE_PREFER_ARCHIVE == policy ) {
            return open_for_prefer_archive_policy( _ctx, _curr_host, _out_parser, _out_vote );

        } else {
            std::stringstream msg;
            msg << "invalid stage policy [" << policy << "]"; 
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        return SUCCESS();

    } // compound_file_redirect_open

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    eirods::error compound_file_redirect( 
        eirods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx.valid< eirods::file_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
        }
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed in get property for name";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'get' operation
            return compound_file_redirect_open( _ctx, _curr_host, _out_parser, _out_vote );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ||
                   eirods::EIRODS_WRITE_OPERATION  == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return compound_file_redirect_create( _ctx, (*_opr), resc_name, _curr_host, _out_parser, _out_vote );

        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // compound_file_redirect

    // =-=-=-=-=-=-=-
    // compound_file_rebalance - code which would rebalance the subtree
    eirods::error compound_file_rebalance(
        eirods::resource_plugin_context& _ctx ) {
        return SUCCESS();

    } // compound_file_rebalancec


    // =-=-=-=-=-=-=-
    // 3. create derived class to handle universal mss resources
    //    context string will hold the script to be called.
    class compound_resource : public eirods::resource {
    public:
        compound_resource( const std::string& _inst_name, 
                           const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
            // =-=-=-=-=-=-=-
            // set the start operation to identify the cache and archive children 
            set_start_operation( "compound_start_operation" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return SUCCESS();
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _pdmo ) {
            return ERROR( -1, "nop" );
        }

    }; // class compound_resource

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
        // 4a. create compound_resource object
        compound_resource* resc = new compound_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "compound_file_create" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "compound_file_open" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "compound_file_read" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "compound_file_write" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "compound_file_close" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "compound_file_unlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "compound_file_stat" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "compound_file_mkdir" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "compound_file_opendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "compound_file_readdir" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "compound_file_rename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "compound_file_getfs_freespace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "compound_file_lseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "compound_file_rmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "compound_file_closedir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "compound_file_stage_to_cache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "compound_file_sync_to_arch" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "compound_file_registered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "compound_file_unregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "compound_file_modified" );

        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "compound_file_redirect" );
        resc->add_operation( eirods::RESOURCE_OP_REBALANCE,             "compound_file_rebalance" );

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



