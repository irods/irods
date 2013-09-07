


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




/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline eirods::error round_robin_check_params(
    eirods::resource_plugin_context& _ctx ) { 
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    eirods::error ret = _ctx.valid< DEST_TYPE >();
    if( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }
   
    return SUCCESS();

} // round_robin_check_params

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
template< typename DEST_TYPE >
eirods::error round_robin_get_resc_for_call( 
    eirods::resource_plugin_context& _ctx,
    eirods::resource_ptr&            _resc ) {
    // =-=-=-=-=-=-=-
    // check incoming parameters 
    eirods::error err = round_robin_check_params< DEST_TYPE >( _ctx );
    if( !err.ok() ) {
        return PASSMSG( "round_robin_get_resc_for_call - bad resource context", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's name
    std::string name;
    err = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, name );
    if( !err.ok() ) {
        return PASSMSG( "round_robin_get_resc_for_call - failed to get property 'name'.", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's hier string
    boost::shared_ptr< DEST_TYPE > obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    std::string hier = obj->resc_hier( );
  
    // =-=-=-=-=-=-=-
    // get the next child pointer given our name and the hier string
    err = get_next_child_in_hier( name, hier, _ctx.child_map(), _resc );
    if( !err.ok() ) {
        return PASSMSG( "round_robin_get_resc_for_call - get_next_child_in_hier failed.", err );
    }

    return SUCCESS();

} // round_robin_get_resc_for_call

extern "C" {
    /// =-=-=-=-=-=-=-
    /// @brief token to index the next child property
    const std::string NEXT_CHILD_PROP( "round_robin_next_child" );
    
    /// =-=-=-=-=-=-=-
    /// @brief token to index the vector of children
    const std::string CHILD_VECTOR_PROP( "round_robin_child_vector" );

    /// =-=-=-=-=-=-=-
    /// @brief build a sorted list of children based on hints in the context
    ///        string for them and their positoin in the child map
    // NOTE :: this assumes the order in the icat dictates the order of the RR.
    //         the user can override that behavior with applying an index to the
    //         child.  should the resc id wrap, this should still work as it 
    //         should behave like a circular queue. 
    eirods::error build_sorted_child_vector( 
            eirods::resource_child_map& _cmap, 
            std::vector< std::string >& _child_vector ) {
        // =-=-=-=-=-=-=-
        // vector holding all of the children
        size_t list_size = _cmap.size();
        _child_vector.resize( list_size );

        // =-=-=-=-=-=-=-
        // iterate over the children and look for indicies on the
        // childrens context strings.  use those to build the initial 
        // list. 
        eirods::resource_child_map::iterator itr;
        for( itr  = _cmap.begin();
             itr != _cmap.end();
             ++itr ) {

            std::string           ctx  = itr->second.first;
            eirods::resource_ptr& resc = itr->second.second;
            if( !ctx.empty() ) {
                try {
                    // =-=-=-=-=-=-=-
                    // cast std::string to int index
                    int idx = boost::lexical_cast<int>( ctx );
                    if( idx < 0 || idx >= list_size ) {
                        eirods::log( ERROR( -1, "build_sorted_child_vector - index < 0" ) );
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // make sure the map at this spot is already empty, could have
                    // duplicate indicies on children
                    if( !_child_vector[ idx ].empty() ) {
                        std::stringstream msg;
                        msg << "build_sorted_child_vector - child map list is not empty ";
                        msg << "for index " << idx << " colliding with [";
                        msg << _child_vector[ idx ] << "]";
                        eirods::log( ERROR( -1, msg.str() ) );
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // snag child resource name
                    std::string name;
                    eirods::error ret = resc->get_property< std::string >( eirods::RESOURCE_NAME, name );
                    if( !ret.ok() ) {
                        eirods::log( ERROR( -1, "build_sorted_child_vector - get property for resource name failed." ));
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // finally add child to the list
                    _child_vector[ idx ] = name;

                } catch (boost::bad_lexical_cast const&) {
                    eirods::log( ERROR( -1, "build_sorted_child_vector - lexical cast failed" ));
                }

            } // if ctx != empty

        } // for itr
 

        // =-=-=-=-=-=-=-
        // iterate over the children again and add in any in the holes 
        // left from the first pass
        for( itr  = _cmap.begin();
             itr != _cmap.end();
             ++itr ) {

            std::string           ctx  = itr->second.first;
            eirods::resource_ptr& resc = itr->second.second;

            // =-=-=-=-=-=-=-
            // skip any resource whose context is not empty
            // as they should have places already
            if( !ctx.empty() ) {
                continue;
            }

            // =-=-=-=-=-=-=-
            // iterate over the _child_vector and find a hole to 
            // fill in with this resource name
            bool   filled_flg = false;
            size_t idx        = 0;
            std::vector< std::string >::iterator vitr;
            for( vitr  = _child_vector.begin();
                 vitr != _child_vector.end();
                 ++vitr ) {
                if( vitr->empty() ) {
                    // =-=-=-=-=-=-=-
                    // snag child resource name
                    std::string name;
                    eirods::error ret = resc->get_property< std::string >( eirods::RESOURCE_NAME, name );
                    if( !ret.ok() ) {
                        eirods::log( ERROR( -1, "build_sorted_child_vector - get property for resource name failed." ));
                        idx++;
                        continue;
                    }
               
                    (*vitr) = name;
                    filled_flg = true;
                    break;
                
                } else {
                    idx++;
                }

            } // for vitr

            // =-=-=-=-=-=-=-
            // check to ensure that the resc found its way into the list
            if( false == filled_flg ) {
                eirods::log( ERROR( -1, "build_sorted_child_vector - failed to find an entry in the resc list" ));
            }

        } // for itr


        return SUCCESS();

    } // build_sorted_child_vector

    /// =-=-=-=-=-=-=-
    /// @brief given the property map the properties next_child and child_vector,
    ///        select the next property in the vector to be tapped as the RR resc
    eirods::error update_next_child_resource( 
        eirods::plugin_property_map& _prop_map ) {
        // =-=-=-=-=-=-=-
        // extract next_child, may be empty for new RR node
        std::string next_child; 
        _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child );

        // =-=-=-=-=-=-=-
        // extract child_vector
        std::vector< std::string > child_vector; 
        eirods::error get_err = _prop_map.get( CHILD_VECTOR_PROP, child_vector );
        if( !get_err.ok() ) {
            std::stringstream msg;
            msg << "update_next_child_resource - failed to get child vector";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // if the next_child string is empty then the next in the round robin
        // selection is the first non empty resource in the vector
        if( next_child.empty() ) {
            // =-=-=-=-=-=-=-
            // scan the child vector for the first non empty position
            for( size_t i = 0; i < child_vector.size(); ++i ) {
                if( child_vector[ i ].empty() ) {
                    std::stringstream msg;
                    msg << "update_next_child_resource - chlid vector at ";
                    msg << " posittion " << i; 
                    eirods::log( ERROR( -1, msg.str() ) );

                } else {
                    next_child = child_vector[ i ];
                    break;
                }

            } // for i

        } else {
            // =-=-=-=-=-=-=-
            // scan the child vector for the context string
            // and select the next position in the series
            for( size_t i = 0; i < child_vector.size(); ++i ) {
                if( next_child == child_vector[ i ] ) {
                    size_t idx = ( (i+1) >= child_vector.size() ) ? 0 : i+1;
                    next_child = child_vector[ idx ];
                    break;
                }

           } // for i

        } // else

        // =-=-=-=-=-=-=-
        // if next_child is empty, something went terribly awry
        if( next_child.empty() ) {
            std::stringstream msg;
            msg << "update_next_child_resource - next_child is empty.";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // assign the next_child to the property map
        _prop_map.set< std::string >( NEXT_CHILD_PROP, next_child );

        return SUCCESS();

    } // update_next_child_resource
    
    // =-=-=-=-=-=-=-
    /// @brief Start Up Operation - iterate over children and map into the 
    ///        list from which to pick the next resource for the creation operation
    eirods::error round_robin_start_operation( 
        eirods::plugin_property_map& _prop_map,
        eirods::resource_child_map&  _cmap ) {
        // =-=-=-=-=-=-=-
        // trap case where no children are available
        if( _cmap.empty() ) {
            return ERROR( -1, "round_robin_start_operation - no children specified" );
        }

        // =-=-=-=-=-=-=-
        // build the initial list of children
        std::vector< std::string > child_vector;
        eirods::error err = build_sorted_child_vector( _cmap, child_vector );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_start_operation - failed.", err );
        }
      
        // =-=-=-=-=-=-=-
        // report children to log 
        for( size_t i = 0; i < child_vector.size(); ++i ) {
            rodsLog( LOG_NOTICE, "round_robin_start_operation :: RR Child [%s] at [%d]", 
                     child_vector[i].c_str(), i );    
        }

        // =-=-=-=-=-=-=-
        // add the child list to the property map
        err = _prop_map.set< std::vector< std::string > >( CHILD_VECTOR_PROP, child_vector );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_start_operation - failed.", err );
        }

        // =-=-=-=-=-=-=-
        // if the next_child property is empty then we need to populate it
        // to the first resource in the child vector
        std::string next_child;
        err = _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child );
        if( err.ok() && next_child.empty() && child_vector.size() > 0 ) {
            _prop_map.set< std::string >( NEXT_CHILD_PROP, child_vector[ 0 ] );
        }

        return SUCCESS();

    } // round_robin_start_operation

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    eirods::error round_robin_file_create( 
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call create on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_CREATE, _ctx.fco() );
   
    } // round_robin_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error round_robin_file_open( 
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_OPEN, _ctx.fco() );
 
    } // round_robin_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    eirods::error round_robin_file_read(
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call read on the child 
        return resc->call< void*, int >( _ctx.comm(), eirods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
 
    } // round_robin_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    eirods::error round_robin_file_write( 
        eirods::resource_plugin_context& _ctx,
        void*                               _buf, 
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call write on the child 
        return resc->call< void*, int >( _ctx.comm(), eirods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
 
    } // round_robin_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    eirods::error round_robin_file_close(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call close on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_CLOSE, _ctx.fco() );
 
    } // round_robin_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    eirods::error round_robin_file_unlink(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::data_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call unlink on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_UNLINK, _ctx.fco() );
 
    } // round_robin_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    eirods::error round_robin_file_stat(
        eirods::resource_plugin_context& _ctx,
        struct stat*                     _statbuf ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::data_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stat on the child 
        return resc->call< struct stat* >( _ctx.comm(), eirods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
 
    } // round_robin_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    eirods::error round_robin_file_lseek(
        eirods::resource_plugin_context& _ctx,
        long long                        _offset, 
        int                              _whence ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call lseek on the child 
        return resc->call< long long, int >( _ctx.comm(), eirods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
 
    } // round_robin_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    eirods::error round_robin_file_mkdir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call mkdir on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_MKDIR, _ctx.fco() );

    } // round_robin_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    eirods::error round_robin_file_rmdir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rmdir on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_RMDIR, _ctx.fco() );

    } // round_robin_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    eirods::error round_robin_file_opendir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call opendir on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_OPENDIR, _ctx.fco() );

    } // round_robin_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    eirods::error round_robin_file_closedir(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call closedir on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );

    } // round_robin_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    eirods::error round_robin_file_readdir(
        eirods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::collection_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call readdir on the child 
        return resc->call< struct rodsDirent** >( _ctx.comm(), eirods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );

    } // round_robin_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    eirods::error round_robin_file_rename(
        eirods::resource_plugin_context& _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call< const char* >( _ctx.comm(), eirods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );

    } // round_robin_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    eirods::error round_robin_file_getfs_freespace(
        eirods::resource_plugin_context& _ctx ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call freespace on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_FREESPACE, _ctx.fco() );

    } // round_robin_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    eirods::error round_robin_file_stage_to_cache(
        eirods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child 
        return resc->call< const char* >( _ctx.comm(), eirods::RESOURCE_OP_STAGE, _ctx.fco(), _cache_file_name );

    } // round_robin_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    eirods::error round_robin_file_sync_to_arch(
        eirods::resource_plugin_context& _ctx, 
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call synctoarch on the child 
        return resc->call< const char* >( _ctx.comm(), eirods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

    } // round_robin_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error round_robin_file_registered(
        eirods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_REGISTERED, _ctx.fco() );

    } // round_robin_file_registered
 
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error round_robin_file_unregistered(
        eirods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );

    } // round_robin_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error round_robin_file_modified(
        eirods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        eirods::resource_ptr resc; 
        eirods::error err = round_robin_get_resc_for_call< eirods::file_object >( _ctx, resc );
        if( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call( _ctx.comm(), eirods::RESOURCE_OP_MODIFIED, _ctx.fco() );

    } // round_robin_file_modified
    
    /// =-=-=-=-=-=-=-
    /// @brief find the next valid child resource for create operation
    eirods::error get_next_valid_child_resource( 
        eirods::plugin_property_map& _prop_map,
        eirods::resource_child_map&  _cmap,
        eirods::resource_ptr&        _resc ) {
        // =-=-=-=-=-=-=-
        // counter and flag
        size_t child_ctr   = 0;
        bool   child_found = false;
 
        // =-=-=-=-=-=-=-
        // while we have not found a child and have not
        // exhausted all the children in the map
        while( !child_found &&
               child_ctr < _cmap.size() ) {
            // =-=-=-=-=-=-=-
            // increment child counter
            child_ctr++;

            // =-=-=-=-=-=-=-
            // get the next_child property 
            std::string next_child;
            eirods::error err = _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child ); 
            if( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - get property for 'next_child' failed.", err );
            
            }

            // =-=-=-=-=-=-=-
            // get the next_child resource 
            if( !_cmap.has_entry( next_child ) ) {
                std::stringstream msg;
                msg << "child map has no child by name [";
                msg << next_child << "]";
                return PASSMSG( msg.str(), err );
                    
            } 

            // =-=-=-=-=-=-=-
            // request our child resource to test it
            eirods::resource_ptr resc = _cmap[ next_child ].second;

            // =-=-=-=-=-=-=-
            // get the resource's status
            int resc_status = 0;
            err = resc->get_property<int>( eirods::RESOURCE_STATUS, resc_status ); 
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
                // update the next_child as we do not have a valid child yet
                err = update_next_child_resource( _prop_map );
                if( !err.ok() ) {
                    return PASSMSG( "update_next_child_resource failed", err );

                }

            }

        } // while

        // =-=-=-=-=-=-=-
        // return appropriately
        if( child_found ) {
            return SUCCESS();
        
        } else {
            return ERROR( EIRODS_NEXT_RESC_FOUND, "no valid child found" );
        
        }

    } // get_next_valid_child_resource

    /// =-=-=-=-=-=-=-
    /// @brief used to allow the resource to determine which host
    ///        should provide the requested operation
    eirods::error round_robin_redirect(
        eirods::resource_plugin_context& _ctx, 
        const std::string*               _opr,
        const std::string*               _curr_host,
        eirods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error err = round_robin_check_params< eirods::file_object >( _ctx );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_redirect - bad resource context", err );
        }
        if( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null host" );
        }
        if( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // get the object's hier string
        eirods::file_object_ptr file_obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        std::string hier = file_obj->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, name );
        if( !err.ok() ) {
            return PASSMSG( "failed to get property 'name'.", err );
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
            err = get_next_child_in_hier( name, hier, _ctx.child_map(), resc );
            if( !err.ok() ) {
                return PASSMSG( "get_next_child_in_hier failed.", err );
            }

            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
            return resc->call< const std::string*, 
                               const std::string*, 
                               eirods::hierarchy_parser*, 
                               float* >( 
                       _ctx.comm(), 
                       eirods::RESOURCE_OP_RESOLVE_RESC_HIER, 
                       _ctx.fco(), 
                       _opr, 
                       _curr_host, 
                       _out_parser, 
                       _out_vote );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next available child resource
            eirods::resource_ptr resc;
            eirods::error err = get_next_valid_child_resource( 
                                    _ctx.prop_map(), 
                                    _ctx.child_map(), 
                                    resc );
            if( !err.ok() ) {
                return PASS( err );
            
            }

            // =-=-=-=-=-=-=-
            // forward the 'put' redirect to the appropriate child
            err = resc->call< const std::string*, 
                              const std::string*, 
                              eirods::hierarchy_parser*, 
                              float* >( 
                       _ctx.comm(), 
                       eirods::RESOURCE_OP_RESOLVE_RESC_HIER, 
                       _ctx.fco(), 
                       _opr, 
                       _curr_host, 
                       _out_parser, 
                       _out_vote );
            if( !err.ok() ) {
                return PASSMSG( "forward of put redirect failed", err );
            
            }
            
            std::string new_hier;
            _out_parser->str( new_hier );
            
            // =-=-=-=-=-=-=-
            // update the next_child appropriately as the above succeeded
            err = update_next_child_resource( _ctx.prop_map() );
            if( !err.ok() ) {
                return PASSMSG( "update_next_child_resource failed", err );

            }

            return SUCCESS();
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "round_robin_redirect - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // round_robin_redirect

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class roundrobin_resource : public eirods::resource {

        class roundrobin_pdmo {
             eirods::plugin_property_map& properties_;
            public:
            // =-=-=-=-=-=-=-
            // public - ctor
            roundrobin_pdmo( eirods::plugin_property_map& _prop_map ) : 
                properties_( _prop_map ) {
            }

            roundrobin_pdmo( const roundrobin_pdmo& _rhs ) : 
                properties_( _rhs.properties_ ) {
            }

            roundrobin_pdmo& operator=( const roundrobin_pdmo& _rhs ) {
                properties_ = _rhs.properties_;
                return *this;
            }

            // =-=-=-=-=-=-=-
            // public - ctor
            eirods::error operator()( rcComm_t* _comm ) {
                std::string name;
                properties_.get< std::string >( eirods::RESOURCE_NAME, name );
                
                std::string next_child;
                properties_.get< std::string >( NEXT_CHILD_PROP, next_child );
                generalAdminInp_t inp;
                inp.arg0 = const_cast<char*>( "modify" );
                inp.arg1 = const_cast<char*>( "resource" ); 
                inp.arg2 = const_cast<char*>( name.c_str() );
                inp.arg3 = const_cast<char*>( "context" );
                inp.arg4 = const_cast<char*>( next_child.c_str() );
                inp.arg5 = 0;
                inp.arg6 = 0;
                inp.arg7 = 0;
                inp.arg8 = 0;
                inp.arg9 = 0;

                int status = rcGeneralAdmin( _comm, &inp ); 
                if( status < 0 ) {
                    return ERROR( status, "roundrobin_pdmo - rsGeneralAdmin failed." );
                }

               return SUCCESS();

           } // operator=

        }; // class roundrobin_pdmo

    public:
        roundrobin_resource( const std::string& _inst_name, 
                             const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
            // =-=-=-=-=-=-=-
            // assign context string as the next_child string
            // in the property map.  this is used to keep track
            // of the last used child in the vector
            properties_.set< std::string >( NEXT_CHILD_PROP, context_ );
            rodsLog( LOG_NOTICE, "roundrobin_resource :: next_child [%s]", context_.c_str() );

            set_start_operation( "round_robin_start_operation" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            std::string next_child;
            properties_.get< std::string >( NEXT_CHILD_PROP, next_child );
            if( !next_child.empty() ) {
                _flg = ( next_child != context_ );
            } else {
                _flg = false;
            }

            return SUCCESS();
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _pdmo ) {
            _pdmo = roundrobin_pdmo( properties_ );
            return SUCCESS();
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
        roundrobin_resource* resc = new roundrobin_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "round_robin_file_create" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "round_robin_file_open" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "round_robin_file_read" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "round_robin_file_write" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "round_robin_file_close" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "round_robin_file_unlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "round_robin_file_stat" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "round_robin_file_mkdir" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "round_robin_file_opendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "round_robin_file_readdir" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "round_robin_file_rename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "round_robin_file_getfs_freespace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "round_robin_file_lseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "round_robin_file_rmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "round_robin_file_closedir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "round_robin_file_stage_to_cache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "round_robin_file_sync_to_arch" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "round_robin_file_registered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "round_robin_file_unregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "round_robin_file_modified" );
        
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "round_robin_redirect" );

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



