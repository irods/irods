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
        /// @brief  resolve a resource from a first_class_object
        error resolve( const first_class_object&, // FCO to resolve against
                       resource_ptr& );           // resource out variable

        // =-=-=-=-=-=-=-
        // @brief  resolve a resource from a key into the resource table
        error resolve( std::string,     // resource key 
                       resource_ptr& ); // resource out variable

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
        
        // =-=-=-=-=-=-=-
		/// @brief print the list of local resources out to stderr
        void print_local_resources(); 
  
        // =-=-=-=-=-=-=-
		/// @brief exec the pdmos ( post disconnect maintenance operations ) in order
        void call_maintenance_operations(); 

        // =-=-=-=-=-=-=-
		/// @brief resolve a resource from a match with a given property 
        template< typename value_type >
		error resolve_from_property( std::string   _prop,    // property key
                                     value_type    _value,   // property value 
                                     resource_ptr& _resc ) { // outgoing resource variable
            // =-=-=-=-=-=-=-
            // simple flag to state a resource matching the prop and value is found
            bool found = false;     
                    
            // =-=-=-=-=-=-=-
            // quick check on the resource table
            if( resources_.empty() ) {
                return ERROR( -1, "resource_manager::resolve_from_property - empty resource table" );
            }
           
            // =-=-=-=-=-=-=-
            // iterate through the map and search for our path
            lookup_table< resource_ptr >::iterator itr = resources_.begin();
            for( ; itr != resources_.end(); ++itr ) {
                // =-=-=-=-=-=-=-
                // query resource for the property value
                value_type value; 
                error ret = itr->second->get_property< value_type >( _prop, value );

                // =-=-=-=-=-=-=-
                // if we get a good parameter 
                if( ret.ok() ) {
                    // =-=-=-=-=-=-=-
                    // compare incoming value and stored value, assumes that the
                    // values support the comparison operator
                    if( _value == value ) {
                        // =-=-=-=-=-=-=-
                        // if we get a match, cache the resource pointer
                        // in the given out variable and bail
                        found = true;
                        _resc = itr->second; 
                        break;
                    }
                } else {
                    std::stringstream msg;
                    msg << "resource_manager::resolve_from_property - ";
                    msg << "failed to get vault parameter from resource";
                    eirods::error err = PASS( false, -1, msg.str(), ret ); 

                }

            } // for itr

            // =-=-=-=-=-=-=-
            // did we find a resource and is the ptr valid?
            if( true == found && _resc.get() ) {
                return SUCCESS();
            } else {
                std::stringstream msg;
                msg <<  "resource_manager::resolve_from_property - ";
                msg << "failed to find resource for property [";
                msg << _prop;
                msg << "] and value [";
                msg << _value; 
                msg << "]";
                return ERROR( -1, msg.str() );
            }

        } // resolve_from_property 
      
    private:
        // =-=-=-=-=-=-=-
        /// @brief take results from genQuery, extract values and create resources
        error process_init_results( genQueryOut_t* );
 
        // =-=-=-=-=-=-=-
        /// @brief Initialize the child map from the resources lookup table
        error init_child_map(void);
 
        // =-=-=-=-=-=-=-
        /// @brief top level function to gather the post disconnect maintenance 
        //         operations from the resources, in breadth first order
        error gather_operations(void);
        
        // =-=-=-=-=-=-=-
        /// @brief lower level recursive call to gather the post disconnect 
        //         maintenance operations from the resources, in breadth first order
        error gather_operations_recursive( const std::string&,           // child string of parent resc
                                           std::vector< std::string >&,  // vector of 'done' resc names
                                           std::vector< pdmo_base* >& ); // vector of ops for this composition


        // =-=-=-=-=-=-=-
        // Attributes
        lookup_table< boost::shared_ptr< resource > > resources_;
        std::vector< std::vector< pdmo_base* > >      maintenance_operations_;

    }; // class resource_manager

}; // namespace eirods


#endif // __EIRODS_RESOURCE_MANAGER_H__



