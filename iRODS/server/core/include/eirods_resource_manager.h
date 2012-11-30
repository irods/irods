/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_RESOURCE_MANAGER_H__
#define __EIRODS_RESOURCE_MANAGER_H__

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_resource_plugin.h"
#include "eirods_first_class_object.h"

namespace eirods {

    class resource_manager {
    public:
                
        // =-=-=-=-=-=-=-
        /// @brief constructors
        resource_manager();
        resource_manager( const resource_manager& );
                
        // =-=-=-=-=-=-=-
        // @brief  destructor
        virtual ~resource_manager();
        
        // =-=-=-=-=-=-=-
        // @brief  resolve a resource from a first_class_object
        error resolve( const first_class_object&, // FCO to resolve against
                       resource_ptr& );           // resource out variable

        // =-=-=-=-=-=-=-
        // @brief  resolve a resource from a key into the resource table
        error resolve( std::string,     // resource key 
                       resource_ptr& ); // resource out variable

        // =-=-=-=-=-=-=-
        // @brief  resolve a resource from a match with a given property 
        error resolve_from_property( std::string,     // property key
                                     std::string,     // property value 
                                     resource_ptr& ); // resource our variable
 
        // =-=-=-=-=-=-=-
        // @brief  resolve a resource from a match with a given property 
        error resolve_from_physical_path( std::string,     // physical path
                                          resource_ptr& ); // out variable

        // =-=-=-=-=-=-=-
        /// @brief  populate resource table from icat database
        error init_from_catalog( rsComm_t* );
 
        // =-=-=-=-=-=-=-
        /// @brief  load a resource plugin given a resource type
        error init_from_type( std::string,     // resource type
                              std::string,     // resource name ( key )
                              std::string,     // resource context 
                              resource_ptr& ); // resource out variable

        private:
        // =-=-=-=-=-=-=-
        /// @brief  take results from genQuery, extract values and create resources
        error process_init_results( genQueryOut_t* );

        // =-=-=-=-=-=-=-
        /// @brief Initialize the child map from the resources lookup table
        error init_child_map(void);


        // =-=-=-=-=-=-=-
        // Attributes
        lookup_table< boost::shared_ptr< resource > > resources_;

    }; // class resource_manager

}; // namespace eirods


#endif // __EIRODS_RESOURCE_MANAGER_H__



