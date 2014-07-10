#ifndef __IRODS_AUTH_PLUGIN_CONTEXT_HPP__
#define __IRODS_AUTH_PLUGIN_CONTEXT_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_context.hpp"

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
            plugin_property_map&    _prop_map,
            first_class_object_ptr  _fco,
            const std::string&      _results ) :
            plugin_context( _prop_map,
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
            // =-=-=-=-=-=-=
            // trap case of non type related checks
            error ret = valid();

            // =-=-=-=-=-=-=
            // trap case of incorrect type for first class object
            try {
                boost::shared_ptr< OBJ_TYPE > ref = boost::dynamic_pointer_cast< OBJ_TYPE >( fco_ );
            }
            catch ( std::bad_cast exp ) {
                ret = PASSMSG( "invalid type for fco cast", ret );
            }

            return ret;

        } // valid

        // =-=-=-=-=-=-=-
        // accessors

    protected:
        // =-=-=-=-=-=-=-
        // attributes

    }; // class auth_plugin_context

}; // namespace irods

#endif // __IRODS_AUTH_PLUGIN_CONTEXT_HPP__



