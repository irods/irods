#ifndef __IRODS_RESOURCE_MANAGER_HPP__
#define __IRODS_RESOURCE_MANAGER_HPP__

// =-=-=-=-=-=-=-
#include "rods.h"
#include "irods_resource_plugin.hpp"
#include "irods_first_class_object.hpp"

#include <functional>

namespace irods {

// =-=-=-=-=-=-=-
//
    const std::string EMPTY_RESC_HOST( "EMPTY_RESC_HOST" );
    const std::string EMPTY_RESC_PATH( "EMPTY_RESC_PATH" );



// =-=-=-=-=-=-=-
/// @brief definition of the resource interface
    const std::string RESOURCE_INTERFACE( "irods_resource_interface" );

// =-=-=-=-=-=-=-
/// @brief special resource for local file system operations only
    const std::string LOCAL_USE_ONLY_RESOURCE( "LOCAL_USE_ONLY_RESOURCE" );
    const std::string LOCAL_USE_ONLY_RESOURCE_VAULT( "/var/lib/irods/LOCAL_USE_ONLY_RESOURCE_VAULT" );
    const std::string LOCAL_USE_ONLY_RESOURCE_TYPE( "unixfilesystem" );

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
            // @brief  resolve a resource from a key into the resource table
            error resolve( std::string,     // resource key
                           resource_ptr& ); // resource out variable

            // =-=-=-=-=-=-=-
            // @brief  resolve a resource from a key into the resource table
            error resolve( rodsLong_t,      // resource id
                           resource_ptr& ); // resource out variable

            // =-=-=-=-=-=-=-
            // @brief  resolve a resource from a match with a given property
            error validate_vault_path( std::string,       // physical path  of the data object
                                       rodsServerHost_t*, // host for which we find the path
                                       std::string& );    // match vault path

            // =-=-=-=-=-=-=-
            /// @brief  populate resource table from icat database
            error init_from_catalog( rsComm_t* );

            // =-=-=-=-=-=-=-
            /// @brief call shutdown on resources before destruction
            error shut_down_resources( );

            // =-=-=-=-=-=-=-
            /// @brief  load a resource plugin given a resource type
            error init_from_type( std::string,     // resource type
                                  std::string,     // resource name ( key )
                                  std::string,     // instance name
                                  std::string,     // resource context
                                  resource_ptr& ); // resource out variable

            // =-=-=-=-=-=-=-
            /// @brief create a list of resources who do not have parents ( roots )
            error get_root_resources( std::vector< std::string >& );

            // =-=-=-=-=-=-=-
            /// @brief create a partial hier string for a given resource to the root
            error get_hier_to_root_for_resc( const std::string&, std::string& );

            // =-=-=-=-=-=-=-
            /// @brief gather vectors of leaf ids for each child of the given resource
            typedef std::vector<rodsLong_t> leaf_bundle_t;
            error gather_leaf_bundles_for_resc( const std::string&, std::vector<leaf_bundle_t>& );

            // =-=-=-=-=-=-=-
            /// @brief print the list of local resources out to stderr
            void print_local_resources();

            // =-=-=-=-=-=-=-
            /// @brief determine if any pdmos need to run before doing a connection
            bool need_maintenance_operations( );

            // =-=-=-=-=-=-=-
            /// @brief exec the pdmos ( post disconnect maintenance operations ) in order
            int call_maintenance_operations( rcComm_t* );

            // =-=-=-=-=-=-=-
            /// @brief get the resc id of the leaf resource in the hierarchy
            error hier_to_leaf_id( const std::string&, rodsLong_t& );

            // =-=-=-=-=-=-=-
            /// @brief get the resc hier of the resource given an id
            error leaf_id_to_hier( const rodsLong_t&, std::string& );

            // =-=-=-=-=-=-=-
            /// @brief get the resc name of the resource given an id
            error resc_id_to_name( const rodsLong_t&, std::string& );

            // =-=-=-=-=-=-=-
            /// @brief get the resc name of the resource given an id as a string
            error resc_id_to_name( const std::string&, std::string& );

            // =-=-=-=-=-=-=-
            /// @brief check whether the specified resource name is a coordinating resource
            error is_coordinating_resource( const std::string&, bool& );

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
                if ( resource_name_map_.empty() ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "empty resource table" );
                }

                // =-=-=-=-=-=-=-
                // iterate through the map and search for our path
                lookup_table< resource_ptr >::iterator itr = resource_name_map_.begin();
                for ( ; itr != resource_name_map_.end(); ++itr ) {
                    // =-=-=-=-=-=-=-
                    // query resource for the property value
                    value_type value = NULL;
                    error ret = itr->second->get_property< value_type >( _prop, value );

                    // =-=-=-=-=-=-=-
                    // if we get a good parameter
                    if ( ret.ok() ) {
                        // =-=-=-=-=-=-=-
                        // compare incoming value and stored value, assumes that the
                        // values support the comparison operator
                        if ( _value == value ) {
                            // =-=-=-=-=-=-=-
                            // if we get a match, cache the resource pointer
                            // in the given out variable and bail
                            found = true;
                            _resc = itr->second;
                            break;
                        }
                    }
                    else {
                        std::stringstream msg;
                        msg << "resource_manager::resolve_from_property - ";
                        msg << "failed to get vault parameter from resource";
                        irods::error err = PASSMSG( msg.str(), ret );

                    }

                } // for itr

                // =-=-=-=-=-=-=-
                // did we find a resource and is the ptr valid?
                if ( true == found && _resc.get() ) {
                    return SUCCESS();
                }
                else {
                    std::stringstream msg;
                    msg << "failed to find resource for property [";
                    msg << _prop;
                    msg << "] and value [";
                    msg << _value;
                    msg << "]";
                    return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );
                }

            } // resolve_from_property

            typedef lookup_table< resource_ptr >::iterator iterator;
            iterator begin() { return resource_name_map_.begin(); }
            iterator end()   { return resource_name_map_.end();   }

        private:
            // =-=-=-=-=-=-=-
            /// @brief load a resource plugin from a dynamic shared object
            error load_resource_plugin(
                resource_ptr&,
                const std::string,
                const std::string,
                const std::string);

            // =-=-=-=-=-=-=-
            /// @brief take results from genQuery, extract values and create resources
            error process_init_results( genQueryOut_t* );

            // =-=-=-=-=-=-=-
            /// @brief Initialize the child map from the resources lookup table
            error init_child_map( void );

            // =-=-=-=-=-=-=-
            /// @brief top level function to gather the post disconnect maintenance
            //         operations from the resources, in breadth first order
            error gather_operations( void );

            // =-=-=-=-=-=-=-
            /// @brief top level function to call the start operation on the resource
            //         plugins
            error start_resource_plugins( void );

            // =-=-=-=-=-=-=-
            /// @brief lower level recursive call to gather the post disconnect
            //         maintenance operations from the resources, in breadth first order
            error gather_operations_recursive( const std::string&,          // child string of parent resc
                                               std::vector< std::string >&, // vector of 'done' resc names
                                               std::vector<pdmo_type>& );   // vector of ops for this composition

            // =-=-=-=-=-=-=-
            /// @brief helper function for gather_leaf_bundles_for_resc
            error gather_leaf_bundle_for_child( const std::string&, leaf_bundle_t& );

            // =-=-=-=-=-=-=-
            /// @brief given a resource name get the parent name from the id
            error get_parent_name( resource_ptr, std::string& );

            // =-=-=-=-=-=-=-
            // Attributes
            lookup_table< resource_ptr >                        resource_name_map_;
            lookup_table< resource_ptr, long, std::hash<long> > resource_id_map_;
            std::vector< std::vector< pdmo_type > > maintenance_operations_;

    }; // class resource_manager

}; // namespace irods


#endif // __IRODS_RESOURCE_MANAGER_HPP__
