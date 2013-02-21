


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
                    eirods::error ret = resc->get_property< std::string >( "name", name );
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
                    eirods::error ret = resc->get_property< std::string >( "name", name );
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
                      eirods::resource_property_map& _prop_map ) {
        // =-=-=-=-=-=-=-
        // extract next_child, may be empty for new RR node
        std::string next_child; 
        _prop_map.get< std::string >( "next_child", next_child );

        // =-=-=-=-=-=-=-
        // extract child_vector
        std::vector< std::string > child_vector; 
        eirods::error get_err = _prop_map.get( "child_vector", child_vector );
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
        _prop_map.set< std::string >( "next_child", next_child );

        return SUCCESS();

    } // update_next_child_resource
    
    // =-=-=-=-=-=-=-
    /// @brief Start Up Operation - iterate over children and map into the 
    ///        list from which to pick the next resource for the creation operation
    eirods::error round_robin_start_operation( 
                  eirods::resource_property_map& _prop_map,
                  eirods::resource_child_map&    _cmap ) {

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
        err = _prop_map.set< std::vector< std::string > >( "child_vector", child_vector );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_start_operation - failed.", err );
        }

        // =-=-=-=-=-=-=-
        // if the next_child property is empty then we need to populate it
        // to the first resource in the child vector
        std::string next_child;
        err = _prop_map.get< std::string >( "next_child", next_child );
        if( err.ok() && next_child.empty() && child_vector.size() > 0 ) {
            _prop_map.set< std::string >( "next_child", child_vector[ 0 ] );
        }

        return SUCCESS();

    } // round_robin_start_operation

    /// =-=-=-=-=-=-=-
    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error round_robin_check_params(
        eirods::resource_property_map* _prop_map,
        eirods::resource_child_map*    _cmap,
        eirods::first_class_object*    _object ) {

        eirods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            result = ERROR( -1, "round_robin_check_params - null resource_property_map" );
        } else if( !_cmap ) {
            result = ERROR( -1, "round_robin_check_params - null resource_child_map" );
        } else if( !_object ) {
            result = ERROR( -1, "round_robin_check_params - null first_class_object" );
        }
        
        return result;

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

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    eirods::error round_robin_file_create( 
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map,
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_create - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_create - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_create - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call create on the child 
        return resc->call( _comm, "create", _object );
   
    } // round_robin_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error round_robin_file_open( 
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap, 
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_open - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_open - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_open - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call open operation on the child 
        return resc->call( _comm, "open", _object );
 
    } // round_robin_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    eirods::error round_robin_file_read(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      void*                          _buf, 
                      int                            _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_read - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_read - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_read - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call read on the child 
        return resc->call< void*, int >( _comm, "read", _object, _buf, _len );
 
    } // round_robin_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    eirods::error round_robin_file_write( 
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      void*                          _buf, 
                      int                            _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_write - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_write - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_write - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call write on the child 
        return resc->call< void*, int >( _comm, "write", _object, _buf, _len );
 
    } // round_robin_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    eirods::error round_robin_file_close(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results  ) {
                    
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_close - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_close - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_close - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call close on the child 
        return resc->call( _comm, "close", _object );
 
    } // round_robin_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    eirods::error round_robin_file_unlink(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results  ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_unlink - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_unlink - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_unlink - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call unlink on the child 
        return resc->call( _comm, "unlink", _object );
 
    } // round_robin_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    eirods::error round_robin_file_stat(
                      rsComm_t* _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      struct stat*                   _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stat - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stat - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stat - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call stat on the child 
        return resc->call< struct stat* >( _comm, "stat", _object, _statbuf );
 
    } // round_robin_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Fstat
    eirods::error round_robin_file_fstat(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      struct stat*                   _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fstat - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fstat - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fstat - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call fstat on the child 
        return resc->call< struct stat* >( _comm, "fstat", _object, _statbuf );
 
    } // round_robin_file_fstat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    eirods::error round_robin_file_lseek(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      size_t                         _offset, 
                      int                            _whence ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_lseek - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_lseek - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_lseek - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call lseek on the child 
        return resc->call< size_t, int >( _comm, "lseek", _object, _offset, _whence );
 
    } // round_robin_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX fsync
    eirods::error round_robin_file_fsync(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results  ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fsync - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fsync - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_fsync - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call fsync on the child 
        return resc->call( _comm, "fsync", _object );
 
    } // round_robin_file_fsync

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    eirods::error round_robin_file_mkdir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object, 
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_mkdir - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_mkdir - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_mkdir - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call mkdir on the child 
        return resc->call( _comm, "mkdir", _object );

    } // round_robin_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX chmod
    eirods::error round_robin_file_chmod(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results  ) {
                    
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_chmod - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_chmod - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_chmod - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call chmod on the child 
        return resc->call( _comm, "chmod", _object );

    } // round_robin_file_chmod

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    eirods::error round_robin_file_rmdir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object, 
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rmdir - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rmdir - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rmdir - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call rmdir on the child 
        return resc->call( _comm, "rmdir", _object );

    } // round_robin_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    eirods::error round_robin_file_opendir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_opendir - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_opendir - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_opendir - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call opendir on the child 
        return resc->call( _comm, "opendir", _object );

    } // round_robin_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    eirods::error round_robin_file_closedir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_closedir - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_closedir - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_closedir - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call closedir on the child 
        return resc->call( _comm, "closedir", _object );

    } // round_robin_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    eirods::error round_robin_file_readdir(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      struct rodsDirent**            _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_readdir - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_readdir - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_readdir - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call readdir on the child 
        return resc->call< struct rodsDirent** >( _comm, "readdir", _object, _dirent_ptr );

    } // round_robin_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX file_stage
    eirods::error round_robin_file_stage(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child 
        return resc->call( _comm, "stage", _object );

    } // round_robin_file_stage

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    eirods::error round_robin_file_rename(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object, 
                      std::string*                   _results,
                      const char*                    _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rename - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rename - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_rename - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call rename on the child 
        return resc->call< const char* >( _comm, "rename", _object, _new_file_name );

    } // round_robin_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX truncate
    eirods::error round_robin_file_truncate(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_truncate - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_truncate - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_truncate - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call truncate on the child 
        return resc->call( _comm, "truncate", _object );

    } // round_robin_file_truncate

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    eirods::error round_robin_file_getfs_freespace(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_getfs_freespace - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_getfs_freespace - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_getfs_freespace - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call freespace on the child 
        return resc->call( _comm, "freespace", _object );

    } // round_robin_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    eirods::error round_robin_file_stage_to_cache(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      const char*                    _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage_to_cache - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage_to_cache - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_stage_to_cache - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call stage on the child 
        return resc->call< const char* >( _comm, "stage", _object, _cache_file_name );

    } // round_robin_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    eirods::error round_robin_file_sync_to_arch(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object, 
                      std::string*                   _results,
                      const char*                    _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters 
        eirods::error err = round_robin_check_params( _prop_map, _cmap, _object );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_sync_to_arch - bad params.", err );
        }

        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_sync_to_arch - failed to get property 'name'.", err );
        }
      
        // =-=-=-=-=-=-=-
        // get the next child pointer given our name and the hier string
        eirods::resource_ptr resc; 
        err = get_next_child_in_hier( name, hier, *_cmap, resc );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_file_sync_to_arch - get_next_child_in_hier failed.", err );
        }

        // =-=-=-=-=-=-=-
        // call synctoarch on the child 
        return resc->call< const char* >( _comm, "synctoarch", _object, _cache_file_name );

    } // round_robin_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief used to allow the resource to determine which host
    ///        should provide the requested operation
    eirods::error round_robin_redirect(
                      rsComm_t*                      _comm,
                      eirods::resource_property_map* _prop_map, 
                      eirods::resource_child_map*    _cmap,
                      eirods::first_class_object*    _object,
                      std::string*                   _results,
                      const std::string*             _opr,
                      const std::string*             _curr_host,
                      eirods::hierarchy_parser*      _out_parser,
                      float*                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_prop_map ) {
            return ERROR( -1, "round_robin_redirect - null resource_property_map" );
        }
        if( !_cmap ) {
            return ERROR( -1, "round_robin_redirect - null resource_child_map" );
        }
        if( !_opr ) {
            return ERROR( -1, "round_robin_redirect - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "round_robin_redirect - null operation" );
        }
        if( !_object ) {
            return ERROR( -1, "round_robin_redirect - null first_class_object" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "round_robin_redirect - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "round_robin_redirect - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string hier = _object->resc_hier( );
rodsLog( LOG_NOTICE, "XXXX - roundrobin_redirect :: hier [%s]", hier.c_str() );
 
        // =-=-=-=-=-=-=-
        // get the object's hier string
        std::string name;
        eirods::error err = _prop_map->get< std::string >( "name", name );
        if( !err.ok() ) {
            return PASSMSG( "round_robin_redirect - failed to get property 'name'.", err );
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
            err = get_next_child_in_hier( name, hier, *_cmap, resc );
            if( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - get_next_child_in_hier failed.", err );
            }

            // =-=-=-=-=-=-=-
            // forward the redirect call to the child for assertion of the whole operation,
            // there may be more than a leaf beneath us
            return resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                               _comm, "redirect", _object, _opr, _curr_host, _out_parser, _out_vote );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // get the next_child property 
            std::string next_child;
            eirods::error err = _prop_map->get< std::string >( "next_child", next_child ); 
            if( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - get property for 'next_child' failed.", err );
            
            }

            // =-=-=-=-=-=-=-
            // get the next_child resource 
            if( !_cmap->has_entry( next_child ) ) {
                std::stringstream msg;
                msg << "round_robin_redirect - child map has no child by name [";
                msg << next_child << "]";
                return PASSMSG( msg.str(), err );
                    
            } 

            // =-=-=-=-=-=-=-
            // request our child resource to redirect
            eirods::resource_ptr resc = (*_cmap)[ next_child ].second;

            // =-=-=-=-=-=-=-
            // forward the 'put' redirect to the appropriate child
            err = resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                               _comm, "redirect", _object, _opr, _curr_host, _out_parser, _out_vote );
            if( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - forward of put redirect failed", err );
            
            }
            std::string new_hier;
            _out_parser->str( new_hier );
rodsLog( LOG_NOTICE, "XXXX - roundrobin :: redireect for CREATE for obj [%s], hier str [%s]", _object->logical_path().c_str(), new_hier.c_str() );
            // =-=-=-=-=-=-=-
            // update the next_child appropriately as the above succeeded
            err = update_next_child_resource( *_prop_map );
            if( !err.ok() ) {
                return PASSMSG( "round_robin_redirect - update_next_child_resource failed", err );

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
             eirods::resource_property_map& properties_;
            public:
            // =-=-=-=-=-=-=-
            // public - ctor
            roundrobin_pdmo( eirods::resource_property_map& _prop_map ) : 
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
                properties_.get< std::string >( "name", name );
                
                std::string next_child;
                properties_.get< std::string >( "next_child", next_child );
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
            properties_.set< std::string >( "next_child", context_ );
            rodsLog( LOG_NOTICE, "roundrobin_resource :: next_child [%s]", context_.c_str() );

            set_start_operation( "round_robin_start_operation" );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            std::string next_child;
            properties_.get< std::string >( "next_child", next_child );
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
        resc->add_operation( "create",       "round_robin_file_create" );
        resc->add_operation( "open",         "round_robin_file_open" );
        resc->add_operation( "read",         "round_robin_file_read" );
        resc->add_operation( "write",        "round_robin_file_write" );
        resc->add_operation( "close",        "round_robin_file_close" );
        resc->add_operation( "unlink",       "round_robin_file_unlink" );
        resc->add_operation( "stat",         "round_robin_file_stat" );
        resc->add_operation( "fstat",        "round_robin_file_fstat" );
        resc->add_operation( "fsync",        "round_robin_file_fsync" );
        resc->add_operation( "mkdir",        "round_robin_file_mkdir" );
        resc->add_operation( "chmod",        "round_robin_file_chmod" );
        resc->add_operation( "opendir",      "round_robin_file_opendir" );
        resc->add_operation( "readdir",      "round_robin_file_readdir" );
        resc->add_operation( "stage",        "round_robin_file_stage" );
        resc->add_operation( "rename",       "round_robin_file_rename" );
        resc->add_operation( "freespace",    "round_robin_file_getfs_freespace" );
        resc->add_operation( "lseek",        "round_robin_file_lseek" );
        resc->add_operation( "rmdir",        "round_robin_file_rmdir" );
        resc->add_operation( "closedir",     "round_robin_file_closedir" );
        resc->add_operation( "truncate",     "round_robin_file_truncate" );
        resc->add_operation( "stagetocache", "round_robin_file_stage_to_cache" );
        resc->add_operation( "synctoarch",   "round_robin_file_sync_to_arch" );
        
        resc->add_operation( "redirect",     "round_robin_redirect" );

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



