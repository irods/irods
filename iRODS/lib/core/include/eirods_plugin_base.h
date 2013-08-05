/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_PLUGIN_BASE_H__
#define __EIRODS_PLUGIN_BASE_H__

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
// eirods includes
#include "eirods_error.h"
#include "eirods_lookup_table.h"

namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief current supported plugin interface version
    const double EIRODS_PLUGIN_INTERFACE_VERSION = 1.0;

    /// =-=-=-=-=-=-=-
    /// @brief abstraction for post disconnect functor - plugins can bind
    ///        functors, free functions or member functions as necessary
    typedef boost::function< eirods::error( rcComm_t* ) > pdmo_type;
    typedef lookup_table<boost::any>                      plugin_property_map;
    
    /**
     * \class plugin_base - ABC for E-iRODS Plugins
     * \author Jason M. Coposky 
     * \date   October 2011
     * \brief  This class enforces the delay_load interface necessary for the 
     load_plugin call to load any other non-member symbols from the 
     shared object.  
     reference iRODS/lib/core/include/eirods_load_plugin.h
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
        const std::string& context_string( )     const { return context_;           }
        double             interface_version(  ) const { return interface_version_; }

    protected:
        std::string                       context_;           // context string for this plugin
        std::string                       instance_name_;     // name of this instance of the plugin
        double                            interface_version_; // version of the plugin interface supported

        /// =-=-=-=-=-=-=-
        /// @brief Map holding resource operations
        std::vector< std::pair< std::string, std::string > > ops_for_delay_load_;

        /// =-=-=-=-=-=-=-
        /// @brief heterogeneous key value map of plugin data
        plugin_property_map properties_; 

    }; // class plugin_base

}; // namespace eirods

#endif // __EIRODS_PLUGIN_BASE_H__



