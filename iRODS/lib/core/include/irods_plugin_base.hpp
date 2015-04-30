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

/// =-=-=-=-=-=-=-
/// @brief interface plugin loader will used to load the
///        plugin interface version.  this is linked directly
///        into the plugins
extern "C" double get_plugin_interface_version();

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief abstraction for post disconnect functor - plugins can bind
///        functors, free functions or member functions as necessary
    typedef boost::function< irods::error( rcComm_t* ) > pdmo_type;
    typedef lookup_table<boost::any>                      plugin_property_map;

    /**
     * \author Jason M. Coposky
     * \brief  Abstract Base Class for iRODS Plugins
     This class enforces the delay_load interface necessary for the
     load_plugin call to load any other non-member symbols from the
     shared object.
     reference iRODS/lib/core/include/irods_load_plugin.hpp
    **/
    class plugin_base {
        public:
            // =-=-=-=-=-=-=-
            /// @brief Constructors
            plugin_base( const std::string&,   // instance name
                         const std::string& ); // context
            plugin_base( const plugin_base& );

            // =-=-=-=-=-=-=-
            /// @brief assignment operator
            plugin_base& operator=( const plugin_base& );

            /// =-=-=-=-=-=-=-
            /// @brief Destructor
            virtual ~plugin_base();

            /// =-=-=-=-=-=-=-
            /// @brief interface to load operations from the shared object
            virtual error delay_load( void* ) = 0;

            /// =-=-=-=-=-=-=-
            /// @brief interface to create and register a PDMO
            virtual error post_disconnect_maintenance_operation( pdmo_type& );

            /// =-=-=-=-=-=-=-
            /// @brief interface to determine if a PDMO is necessary
            virtual error need_post_disconnect_maintenance_operation( bool& );

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function name
            virtual error add_operation( std::string, std::string );

            /// =-=-=-=-=-=-=-
            /// @brief list all of the operations in the plugin
            error enumerate_operations( std::vector< std::string >& );

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



