#ifndef __IRODS_AUTH_PLUGIN_CONTEXT_HPP__
#define __IRODS_AUTH_PLUGIN_CONTEXT_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_context.hpp"
#include <boost/pointer_cast.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief forward decl of auth
    class auth;

// =-=-=-=-=-=-=-
// @brief class which holds default values passed to auth plugin
//        operations.  this allows for easy extension in the future
//        without the need to rewrite plugin interfaces as well as
//        pass along references rather than pointers
    class auth_plugin_context : public plugin_context {
        public:
            // =-=-=-=-=-=-=-
            // ctor
            auth_plugin_context(
                rsComm_t*               _comm,
                plugin_property_map&    _prop_map,
                first_class_object_ptr  _fco,
                const std::string&      _results ) :
                plugin_context( 
                    _comm,
                    _prop_map,
                    _fco,
                    _results ) {
            } // ctor

            // =-=-=-=-=-=-=-
            // test to determine if contents are valid
            virtual error valid() {
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

        protected:
            // =-=-=-=-=-=-=-
            // attributes

    }; // class auth_plugin_context

}; // namespace irods

#endif // __IRODS_AUTH_PLUGIN_CONTEXT_HPP__



