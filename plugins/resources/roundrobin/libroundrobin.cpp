// =-=-=-=-=-=-=-
// legacy irods includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
//
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "rs_set_round_robin_context.hpp"

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
inline irods::error round_robin_check_params(
    irods::resource_plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }

    return SUCCESS();

} // round_robin_check_params

/// =-=-=-=-=-=-=-
/// @brief get the next resource shared pointer given this resources name
///        as well as the object's hierarchy string
irods::error get_next_child_in_hier(
    const std::string&          _name,
    const std::string&          _hier,
    irods::resource_child_map&  _cmap,
    irods::resource_ptr&        _resc ) {

    // =-=-=-=-=-=-=-
    // create a parser and parse the string
    irods::hierarchy_parser parse;
    irods::error err = parse.set_string( _hier );
    if ( !err.ok() ) {
        std::stringstream msg;
        msg << "get_next_child_in_hier - failed in set_string for [";
        msg << _hier << "]";
        return PASSMSG( msg.str(), err );
    }

    // =-=-=-=-=-=-=-
    // get the next resource in the series
    std::string next;
    err = parse.next( _name, next );
    if ( !err.ok() ) {
        std::stringstream msg;
        msg << "get_next_child_in_hier - failed in next for [";
        msg << _name << "] for hier ["
            << _hier << "]";
        return PASSMSG( msg.str(), err );
    }

    // =-=-=-=-=-=-=-
    // get the next resource from the child map
    if ( !_cmap.has_entry( next ) ) {
        std::stringstream msg;
        msg << "get_next_child_in_hier - child map missing entry [";
        msg << next << "]";
        return ERROR( CHILD_NOT_FOUND, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // assign resource
    _resc = _cmap[ next ].second;

    return SUCCESS();

} // get_next_child_in_hier

/// =-=-=-=-=-=-=-
/// @brief get the next resource shared pointer given this resources name
///        as well as the file object
irods::error get_next_child_for_open_or_write(
    const std::string&          _name,
    irods::file_object_ptr&     _file_obj,
    irods::resource_child_map&  _cmap,
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
                   _cmap,
                   _resc );

    } // for itr

    std::string msg( "no hier found for resc [" );
    msg += _name + "]";
    return ERROR(
               CHILD_NOT_FOUND,
               msg );

} // get_next_child_for_open_or_write

// =-=-=-=-=-=-=-
/// @brief get the resource for the child in the hierarchy
///        to pass on the call
template< typename DEST_TYPE >
irods::error round_robin_get_resc_for_call(
    irods::resource_plugin_context& _ctx,
    irods::resource_ptr&            _resc ) {
    // =-=-=-=-=-=-=-
    // check incoming parameters
    irods::error err = round_robin_check_params< DEST_TYPE >( _ctx );
    if ( !err.ok() ) {
        return PASSMSG( "round_robin_get_resc_for_call - bad resource context", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's name
    std::string name;
    err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
    if ( !err.ok() ) {
        return PASSMSG( "round_robin_get_resc_for_call - failed to get property 'name'.", err );
    }

    // =-=-=-=-=-=-=-
    // get the object's hier string
    boost::shared_ptr< DEST_TYPE > obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    std::string hier = obj->resc_hier( );

    // =-=-=-=-=-=-=-
    // get the next child pointer given our name and the hier string
    err = get_next_child_in_hier( name, hier, _ctx.child_map(), _resc );
    if ( !err.ok() ) {
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
    irods::error build_sorted_child_vector(
        irods::resource_child_map& _cmap,
        std::vector< std::string >& _child_vector ) {
        // =-=-=-=-=-=-=-
        // vector holding all of the children
        size_t list_size = _cmap.size();
        _child_vector.resize( list_size );

        // =-=-=-=-=-=-=-
        // iterate over the children and look for indicies on the
        // childrens context strings.  use those to build the initial
        // list.
        irods::resource_child_map::iterator itr;
        for ( itr  = _cmap.begin();
                itr != _cmap.end();
                ++itr ) {

            std::string           ctx  = itr->second.first;
            irods::resource_ptr& resc = itr->second.second;
            if ( !ctx.empty() ) {
                try {
                    // =-=-=-=-=-=-=-
                    // cast std::string to int index
                    size_t idx = boost::lexical_cast<size_t>( ctx );
                    if ( idx >= list_size ) {
                        irods::log( ERROR( -1, "build_sorted_child_vector - index out of bounds" ) );
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // make sure the map at this spot is already empty, could have
                    // duplicate indicies on children
                    if ( !_child_vector[ idx ].empty() ) {
                        std::stringstream msg;
                        msg << "build_sorted_child_vector - child map list is not empty ";
                        msg << "for index " << idx << " colliding with [";
                        msg << _child_vector[ idx ] << "]";
                        irods::log( ERROR( -1, msg.str() ) );
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // snag child resource name
                    std::string name;
                    irods::error ret = resc->get_property< std::string >( irods::RESOURCE_NAME, name );
                    if ( !ret.ok() ) {
                        irods::log( ERROR( -1, "build_sorted_child_vector - get property for resource name failed." ) );
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // finally add child to the list
                    _child_vector[ idx ] = name;

                }
                catch ( const boost::bad_lexical_cast& ) {
                    irods::log( ERROR( -1, "build_sorted_child_vector - lexical cast to size_t failed" ) );
                }

            } // if ctx != empty

        } // for itr


        // =-=-=-=-=-=-=-
        // iterate over the children again and add in any in the holes
        // left from the first pass
        for ( itr  = _cmap.begin();
                itr != _cmap.end();
                ++itr ) {

            std::string           ctx  = itr->second.first;
            irods::resource_ptr& resc = itr->second.second;

            // =-=-=-=-=-=-=-
            // skip any resource whose context is not empty
            // as they should have places already
            if ( !ctx.empty() ) {
                continue;
            }

            // =-=-=-=-=-=-=-
            // iterate over the _child_vector and find a hole to
            // fill in with this resource name
            bool   filled_flg = false;
            size_t idx        = 0;
            std::vector< std::string >::iterator vitr;
            for ( vitr  = _child_vector.begin();
                    vitr != _child_vector.end();
                    ++vitr ) {
                if ( vitr->empty() ) {
                    // =-=-=-=-=-=-=-
                    // snag child resource name
                    std::string name;
                    irods::error ret = resc->get_property< std::string >( irods::RESOURCE_NAME, name );
                    if ( !ret.ok() ) {
                        irods::log( ERROR( -1, "build_sorted_child_vector - get property for resource name failed." ) );
                        idx++;
                        continue;
                    }

                    ( *vitr ) = name;
                    filled_flg = true;
                    break;

                }
                else {
                    idx++;
                }

            } // for vitr

            // =-=-=-=-=-=-=-
            // check to ensure that the resc found its way into the list
            if ( false == filled_flg ) {
                irods::log( ERROR( -1, "build_sorted_child_vector - failed to find an entry in the resc list" ) );
            }

        } // for itr


        return SUCCESS();

    } // build_sorted_child_vector

    /// =-=-=-=-=-=-=-
    /// @brief given the property map the properties next_child and child_vector,
    ///        select the next property in the vector to be tapped as the RR resc
    irods::error update_next_child_resource(
        irods::plugin_property_map& _prop_map ) {
        // =-=-=-=-=-=-=-
        // extract next_child, may be empty for new RR node
        std::string next_child;
        _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child );

        // =-=-=-=-=-=-=-
        // extract child_vector
        std::vector< std::string > child_vector;
        irods::error get_err = _prop_map.get( CHILD_VECTOR_PROP, child_vector );
        if ( !get_err.ok() ) {
            std::stringstream msg;
            msg << "update_next_child_resource - failed to get child vector";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // if the next_child string is empty then the next in the round robin
        // selection is the first non empty resource in the vector
        if ( next_child.empty() ) {
            // =-=-=-=-=-=-=-
            // scan the child vector for the first non empty position
            for ( size_t i = 0; i < child_vector.size(); ++i ) {
                if ( child_vector[ i ].empty() ) {
                    std::stringstream msg;
                    msg << "update_next_child_resource - chlid vector at ";
                    msg << " posittion " << i;
                    irods::log( ERROR( -1, msg.str() ) );

                }
                else {
                    next_child = child_vector[ i ];
                    break;
                }

            } // for i

        }
        else {
            // =-=-=-=-=-=-=-
            // scan the child vector for the context string
            // and select the next position in the series
            for ( size_t i = 0; i < child_vector.size(); ++i ) {
                if ( next_child == child_vector[ i ] ) {
                    size_t idx = ( ( i + 1 ) >= child_vector.size() ) ? 0 : i + 1;
                    next_child = child_vector[ idx ];
                    break;
                }

            } // for i

        } // else

        // =-=-=-=-=-=-=-
        // if next_child is empty, something went terribly awry
        if ( next_child.empty() ) {
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
    irods::error round_robin_start_operation(
        irods::plugin_property_map& _prop_map,
        irods::resource_child_map&  _cmap ) {
        // =-=-=-=-=-=-=-
        // trap case where no children are available
        if ( _cmap.empty() ) {
            return ERROR( -1, "round_robin_start_operation - no children specified" );
        }

        // =-=-=-=-=-=-=-
        // build the initial list of children
        std::vector< std::string > child_vector;
        irods::error err = build_sorted_child_vector( _cmap, child_vector );
        if ( !err.ok() ) {
            return PASSMSG( "round_robin_start_operation - failed.", err );
        }

        // =-=-=-=-=-=-=-
        // report children to log
        for ( size_t i = 0; i < child_vector.size(); ++i ) {
            rodsLog( LOG_DEBUG, "round_robin_start_operation :: RR Child [%s] at [%d]",
                     child_vector[i].c_str(), i );
        }

        // =-=-=-=-=-=-=-
        // add the child list to the property map
        err = _prop_map.set< std::vector< std::string > >( CHILD_VECTOR_PROP, child_vector );
        if ( !err.ok() ) {
            return PASSMSG( "round_robin_start_operation - failed.", err );
        }

        // =-=-=-=-=-=-=-
        // if the next_child property is empty then we need to populate it
        // to the first resource in the child vector
        std::string next_child;
        err = _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child );
        if ( err.ok() && next_child.empty() && child_vector.size() > 0 ) {
            _prop_map.set< std::string >( NEXT_CHILD_PROP, child_vector[ 0 ] );
        }

        return SUCCESS();

    } // round_robin_start_operation

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    irods::error round_robin_file_create(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call create on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );

    } // round_robin_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error round_robin_file_open(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );

    } // round_robin_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    irods::error round_robin_file_read(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call read on the child
        return resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );

    } // round_robin_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    irods::error round_robin_file_write(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call write on the child
        return resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );

    } // round_robin_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    irods::error round_robin_file_close(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call close on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );

    } // round_robin_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    irods::error round_robin_file_unlink(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::data_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call unlink on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );

    } // round_robin_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    irods::error round_robin_file_stat(
        irods::resource_plugin_context& _ctx,
        struct stat*                     _statbuf ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::data_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stat on the child
        return resc->call< struct stat* >( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );

    } // round_robin_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    irods::error round_robin_file_lseek(
        irods::resource_plugin_context& _ctx,
        long long                        _offset,
        int                              _whence ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call lseek on the child
        return resc->call< long long, int >( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );

    } // round_robin_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    irods::error round_robin_file_mkdir(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call mkdir on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );

    } // round_robin_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    irods::error round_robin_file_rmdir(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rmdir on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );

    } // round_robin_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    irods::error round_robin_file_opendir(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call opendir on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );

    } // round_robin_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    irods::error round_robin_file_closedir(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call closedir on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );

    } // round_robin_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    irods::error round_robin_file_readdir(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::collection_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call readdir on the child
        return resc->call< struct rodsDirent** >( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );

    } // round_robin_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    irods::error round_robin_file_rename(
        irods::resource_plugin_context& _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );

    } // round_robin_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX truncate
    irods::error round_robin_file_truncate(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // call truncate on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );

    } // round_robin_file_truncate

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    irods::error round_robin_file_getfs_freespace(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call freespace on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );

    } // round_robin_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    irods::error round_robin_file_stage_to_cache(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );

    } // round_robin_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    irods::error round_robin_file_sync_to_arch(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call synctoarch on the child
        return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

    } // round_robin_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error round_robin_file_registered(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call registered on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );

    } // round_robin_file_registered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error round_robin_file_unregistered(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg <<  __FUNCTION__;
            msg << " - failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call unregistered on the child
        return resc->call( _ctx.comm(), irods::RESOURCE_OP_UNREGISTERED, _ctx.fco() );

    } // round_robin_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error round_robin_file_modified(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // call modified on the child
        err = resc->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // if file modified is successful then we will update the next
        // child in the round robin within the database
        std::string name;
        _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );

        std::string next_child;
        _ctx.prop_map().get< std::string >( NEXT_CHILD_PROP, next_child );

        setRoundRobinContextInp_t inp;
        snprintf( inp.resc_name_, sizeof( inp.resc_name_ ), "%s", name.c_str() );
        snprintf( inp.context_, sizeof( inp.context_ ), "%s", next_child.c_str() );
        int status = irods::server_api_call(
                         SET_RR_CTX_AN,
                         _ctx.comm(),
                         &inp,
                         NULL,
                         ( void** ) NULL,
                         NULL );

        if ( status < 0 ) {
            std::stringstream msg;
            msg << "failed to update round robin context for [";
            msg << name << "] with context [" << next_child << "]";
            return ERROR(
                       status,
                       msg.str() );

        }
        else {
            return SUCCESS();

        }

    } // round_robin_file_modified

    /// =-=-=-=-=-=-=-
    /// @brief find the next valid child resource for create operation
    irods::error get_next_valid_child_resource(
        irods::plugin_property_map& _prop_map,
        irods::resource_child_map&  _cmap,
        irods::resource_ptr&        _resc ) {
        // =-=-=-=-=-=-=-
        // counter and flag
        int child_ctr   = 0;
        bool   child_found = false;

        // =-=-=-=-=-=-=-
        // while we have not found a child and have not
        // exhausted all the children in the map
        while ( !child_found &&
                child_ctr < _cmap.size() ) {
            // =-=-=-=-=-=-=-
            // increment child counter
            child_ctr++;

            // =-=-=-=-=-=-=-
            // get the next_child property
            std::string next_child;
            irods::error err = _prop_map.get< std::string >( NEXT_CHILD_PROP, next_child );
            if ( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - get property for 'next_child' failed.", err );

            }

            // =-=-=-=-=-=-=-
            // get the next_child resource
            if ( !_cmap.has_entry( next_child ) ) {
                std::stringstream msg;
                msg << "child map has no child by name [";
                msg << next_child << "]";
                return PASSMSG( msg.str(), err );

            }

            // =-=-=-=-=-=-=-
            // request our child resource to test it
            irods::resource_ptr resc = _cmap[ next_child ].second;

            // =-=-=-=-=-=-=-
            // get the resource's status
            int resc_status = 0;
            err = resc->get_property<int>( irods::RESOURCE_STATUS, resc_status );
            if ( !err.ok() ) {
                return PASSMSG( "failed to get property", err );

            }

            // =-=-=-=-=-=-=-
            // determine if the resource is up and available
            if ( INT_RESC_STATUS_DOWN != resc_status ) {
                // =-=-=-=-=-=-=-
                // we found a valid child, set out variable
                _resc = resc;
                child_found = true;

            }
            else {
                // =-=-=-=-=-=-=-
                // update the next_child as we do not have a valid child yet
                err = update_next_child_resource( _prop_map );
                if ( !err.ok() ) {
                    return PASSMSG( "update_next_child_resource failed", err );

                }

            }

        } // while

        // =-=-=-=-=-=-=-
        // return appropriately
        if ( child_found ) {
            return SUCCESS();

        }
        else {
            return ERROR( NO_NEXT_RESC_FOUND, "no valid child found" );

        }

    } // get_next_valid_child_resource

    /// =-=-=-=-=-=-=-
    /// @brief used to allow the resource to determine which host
    ///        should provide the requested operation
    irods::error round_robin_redirect(
        irods::resource_plugin_context& _ctx,
        const std::string*               _opr,
        const std::string*               _curr_host,
        irods::hierarchy_parser*        _out_parser,
        float*                           _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error err = round_robin_check_params< irods::file_object >( _ctx );
        if ( !err.ok() ) {
            return PASSMSG( "round_robin_redirect - bad resource context", err );
        }
        if ( !_opr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null operation" );
        }
        if ( !_curr_host ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null host" );
        }
        if ( !_out_parser ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null outgoing hier parser" );
        }
        if ( !_out_vote ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "round_robin_redirect - null outgoing vote" );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        std::string hier = file_obj->resc_hier( );

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
        if ( !err.ok() ) {
            return PASSMSG( "failed to get property 'name'.", err );
        }

        // =-=-=-=-=-=-=-
        // add ourselves into the hierarch before calling child resources
        _out_parser->add_child( name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if ( irods::OPEN_OPERATION  == ( *_opr )  ||
                irods::WRITE_OPERATION == ( *_opr ) ) {
            // =-=-=-=-=-=-=-
            // get the next child pointer in the hierarchy, given our name and the hier string
            irods::resource_ptr resc;
            err = get_next_child_for_open_or_write(
                      name,
                      file_obj,
                      _ctx.child_map(),
                      resc );
            if ( !err.ok() ) {
                ( *_out_vote ) = 0.0;
                return PASS( err );
            }

            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
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
        }
        else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
            // =-=-=-=-=-=-=-
            // get the next available child resource
            irods::resource_ptr resc;
            irods::error err = get_next_valid_child_resource(
                                   _ctx.prop_map(),
                                   _ctx.child_map(),
                                   resc );
            if ( !err.ok() ) {
                return PASS( err );

            }

            // =-=-=-=-=-=-=-
            // forward the 'put' redirect to the appropriate child
            err = resc->call < const std::string*,
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
            if ( !err.ok() ) {
                return PASSMSG( "forward of put redirect failed", err );

            }

            std::string hier;
            _out_parser->str( hier );
            rodsLog(
                LOG_DEBUG,
                "round robin - create :: resc hier [%s] vote [%f]",
                hier.c_str(),
                _out_vote );

            std::string new_hier;
            _out_parser->str( new_hier );

            // =-=-=-=-=-=-=-
            // update the next_child appropriately as the above succeeded
            err = update_next_child_resource( _ctx.prop_map() );
            if ( !err.ok() ) {
                return PASSMSG( "update_next_child_resource failed", err );

            }

            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // must have been passed a bad operation
        std::stringstream msg;
        msg << "round_robin_redirect - operation not supported [";
        msg << ( *_opr ) << "]";
        return ERROR( -1, msg.str() );

    } // round_robin_redirect

    // =-=-=-=-=-=-=-
    // round_robin_file_rebalance - code which would rebalance the subtree
    irods::error round_robin_file_rebalance(
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

    } // round_robin_file_rebalancec

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error round_robin_file_notify(
        irods::resource_plugin_context& _ctx,
        const std::string*               _opr ) {
        // =-=-=-=-=-=-=-
        // get the child resc to call
        irods::resource_ptr resc;
        irods::error err = round_robin_get_resc_for_call< irods::file_object >( _ctx, resc );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed.";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child
        return resc->call< const std::string* >(
                   _ctx.comm(),
                   irods::RESOURCE_OP_NOTIFY,
                   _ctx.fco(),
                   _opr );

    } // round_robin_file_open

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle round_robin file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class roundrobin_resource : public irods::resource {
        public:
            roundrobin_resource( const std::string& _inst_name,
                                 const std::string& _context ) :
                irods::resource( _inst_name, _context ) {
                // =-=-=-=-=-=-=-
                // assign context string as the next_child string
                // in the property map.  this is used to keep track
                // of the last used child in the vector
                properties_.set< std::string >( NEXT_CHILD_PROP, context_ );
                rodsLog( LOG_DEBUG, "roundrobin_resource :: next_child [%s]", context_.c_str() );

                set_start_operation( "round_robin_start_operation" );
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
                                     const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // 4a. create round_robinfilesystem_resource
        roundrobin_resource* resc = new roundrobin_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "round_robin_file_create" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "round_robin_file_open" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "round_robin_file_read" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "round_robin_file_write" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "round_robin_file_close" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "round_robin_file_unlink" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "round_robin_file_stat" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "round_robin_file_mkdir" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "round_robin_file_opendir" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "round_robin_file_readdir" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "round_robin_file_rename" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,     "round_robin_file_truncate" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "round_robin_file_getfs_freespace" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "round_robin_file_lseek" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "round_robin_file_rmdir" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "round_robin_file_closedir" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE, "round_robin_file_stage_to_cache" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,   "round_robin_file_sync_to_arch" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "round_robin_file_registered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "round_robin_file_unregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "round_robin_file_modified" );

        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER,     "round_robin_redirect" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,             "round_robin_file_rebalance" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,             "round_robin_file_notify" );

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



