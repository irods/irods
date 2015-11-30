#ifndef __IRODS_RESOURCE_PLUGIN_CONTEXT_HPP__
#define __IRODS_RESOURCE_PLUGIN_CONTEXT_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_context.hpp"
#include "irods_resource_types.hpp"

#include <boost/pointer_cast.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief typedef for resource child map
    typedef lookup_table< std::pair< std::string, resource_ptr > > resource_child_map;

// =-=-=-=-=-=-=-
// @brief class which holds default values passed to resource plugin
//        operations.  this allows for easy extension in the future
//        without the need to rewrite plugin interfaces as well as
//        pass along references rather than pointers
    class resource_plugin_context : public plugin_context {
        public:
            // =-=-=-=-=-=-=-
            // ctor
            resource_plugin_context(
                plugin_property_map&    _prop_map,
                first_class_object_ptr  _fco,
                const std::string&      _results,
                rsComm_t*               _comm,
                resource_child_map&     _child_map )  :
                plugin_context( _comm,
                                _prop_map,
                                _fco,
                                _results ),
                comm_( _comm ),
                child_map_( _child_map ) {

            } // ctor

            // =-=-=-=-=-=-=-
            // test to determine if contents are valid
            virtual error valid() {
                // =-=-=-=-=-=-=
                // trap case of bad comm pointer
                if ( 0 == comm_ ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "bad comm pointer" );
                }

                return SUCCESS();

            } // valid

            // =-=-=-=-=-=-=-
            // test to determine if contents are valid
            template < typename OBJ_TYPE >
            error valid() {
                // trap case of incorrect type for first class object
                return boost::dynamic_pointer_cast< OBJ_TYPE >( fco_.get() ) == NULL ?
                       ERROR( INVALID_DYNAMIC_CAST, "invalid type for fco cast" ) :
                       valid();

            } // valid

            // =-=-=-=-=-=-=-
            // accessors
            virtual resource_child_map&  child_map() {
                return child_map_;
            }

        protected:
            // =-=-=-=-=-=-=-
            // attributes
            rsComm_t*             comm_;      // server side comm context
            resource_child_map&   child_map_; // resource child map

    }; // class resource_plugin_context

}; // namespace irods

#endif // __IRODS_RESOURCE_PLUGIN_CONTEXT_HPP__



