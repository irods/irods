#ifndef __IRODS_PLUGIN_CONTEXT_HPP__
#define __IRODS_PLUGIN_CONTEXT_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_first_class_object.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// base context class for communicating to plugins
    class plugin_context {
        public:
            // =-=-=-=-=-=-=-
            // ctor
            plugin_context(
                plugin_property_map&   _prop_map,
                first_class_object_ptr _fco,
                const std::string&     _results )  :
                comm_( 0 ),
                prop_map_( _prop_map ),
                fco_( _fco ),
                results_( _results )  {

            } // ctor

            // =-=-=-=-=-=-=-
            // ctor
            plugin_context(
                rsComm_t*              _comm,
                plugin_property_map&   _prop_map,
                first_class_object_ptr _fco,
                const std::string&     _results )  :
                comm_( _comm ),
                prop_map_( _prop_map ),
                fco_( _fco ),
                results_( _results )  {

            } // ctor

            // =-=-=-=-=-=-=-
            // dtor
            virtual ~plugin_context() {

            } // dtor

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
                OBJ_TYPE* ref = dynamic_cast< OBJ_TYPE* >( fco_.get() );
                if ( ref == NULL ) {
                    ret = ERROR( INVALID_DYNAMIC_CAST, "invalid type for fco cast" );
                }

                return ret;

            } // valid

            // =-=-=-=-=-=-=-
            // accessors
            virtual rsComm_t* comm() {
                return comm_;
            }

            virtual plugin_property_map&   prop_map()     {
                return prop_map_;
            }
            virtual first_class_object_ptr fco()          {
                return fco_;
            }
            virtual const std::string      rule_results() {
                return results_;
            }

            // =-=-=-=-=-=-=-
            // mutators
            virtual void comm( rsComm_t* _c ) {
                comm_ = _c;
            }

            virtual void rule_results( const std::string& _s ) {
                results_ = _s;
            }

        protected:
            // =-=-=-=-=-=-=-
            // attributes
            rsComm_t*              comm_;      // server connection handle
            plugin_property_map&   prop_map_;  // resource property map
            first_class_object_ptr fco_;       // first class object in question
            std::string            results_;   // results from the pre op rule call

    }; // class plugin_context

/// =-=-=-=-=-=-=-
/// @brief type for the generic plugin operation
    typedef error( *plugin_operation )( plugin_context&, ... );

}; // namespace irods

#endif // __IRODS_PLUGIN_CONTEXT_HPP__



