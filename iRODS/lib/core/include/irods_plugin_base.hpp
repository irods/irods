#ifndef __IRODS_PLUGIN_BASE_HPP__
#define __IRODS_PLUGIN_BASE_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/function.hpp>

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "irods_lookup_table.hpp"

static double PLUGIN_INTERFACE_VERSION = 1.0;

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief abstraction for post disconnect functor - plugins can bind
///        functors, free functions or member functions as necessary
    typedef boost::function< irods::error( rcComm_t* ) > pdmo_type;
    typedef lookup_table<boost::any>                     plugin_property_map;

    /**
     * \brief  Abstract Base Class for iRODS Plugins
     This class enforces the delay_load interface necessary for the
     load_plugin call to load any other non-member symbols from the
     shared object.
     reference iRODS/lib/core/include/irods_load_plugin.hpp
    **/
    class plugin_base {
        public:
            plugin_base(
                    const std::string& _n,
                    const std::string& _c ) :
                context_( _c ),
                instance_name_( _n ),
                interface_version_( PLUGIN_INTERFACE_VERSION ) {
            } // ctor

            plugin_base(
                    const plugin_base& _rhs ) :
                context_( _rhs.context_ ),
                instance_name_( _rhs.instance_name_ ),
                interface_version_( _rhs.interface_version_ ) {
            } // cctor

            plugin_base& operator=(
                    const plugin_base& _rhs ) {
                instance_name_     = _rhs.instance_name_;
                context_           = _rhs.context_;
                interface_version_ = _rhs.interface_version_;

                return *this;

            } // operator=
    
            virtual ~plugin_base( ) {
            } // dtor

            /// =-=-=-=-=-=-=-
            /// @brief interface to load operations from the shared object
            virtual error delay_load( void* ) = 0;

            /// =-=-=-=-=-=-=-
            /// @brief interface to create and register a PDMO
            virtual error post_disconnect_maintenance_operation( pdmo_type& ) {
                return ERROR( NO_PDMO_DEFINED, "no defined operation" );
            }

            /// =-=-=-=-=-=-=-
            /// @brief interface to determine if a PDMO is necessary
            virtual error need_post_disconnect_maintenance_operation( bool& _b ) {
                    _b = false;
                    return SUCCESS();
            }

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function name
            virtual error add_operation( std::string _op,
                    std::string _fcn_name ) {
                // =-=-=-=-=-=-=-
                // check params
                if ( _op.empty() ) {
                    std::stringstream msg;
                    msg << "empty operation [" << _op << "]";
                    return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                }

                if ( _fcn_name.empty() ) {
                    std::stringstream msg;
                    msg << "empty function name [" << _fcn_name << "]";
                    return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                }

                // =-=-=-=-=-=-=-
                // add operation string to the vector
                ops_for_delay_load_.push_back( std::pair< std::string, std::string >( _op, _fcn_name ) );

                return SUCCESS();

            }

            /// =-=-=-=-=-=-=-
            /// @brief list all of the operations in the plugin
            error enumerate_operations( std::vector< std::string >& _ops ) {
                for ( size_t i = 0; i < ops_for_delay_load_.size(); ++i ) {
                    _ops.push_back( ops_for_delay_load_[ i ].first );
                }

                return SUCCESS();

            }

            /// =-=-=-=-=-=-=-
            /// @brief accessor for context string
            const std::string& context_string( )     const {
                return context_;
            }
            double             interface_version( ) const {
                return interface_version_;
            }

        protected:
            std::string context_;           // context string for this plugin
            std::string instance_name_;     // name of this instance of the plugin
            double      interface_version_; // version of the plugin interface supported

            /// =-=-=-=-=-=-=-
            /// @brief heterogeneous key value map of plugin data
            plugin_property_map properties_;

            /// =-=-=-=-=-=-=-
            /// @brief Map holding resource operations
            std::vector< std::pair< std::string, std::string > > ops_for_delay_load_;

    }; // class plugin_base

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< plugin_base > plugin_ptr;

}; // namespace irods

#endif // __IRODS_PLUGIN_BASE_HPP__



