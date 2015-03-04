#ifndef __IRODS_RESOURCE_MANAGER_HPP__
#define __IRODS_RESOURCE_MANAGER_HPP__

// =-=-=-=-=-=-=-
#include "rods.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_first_class_object.hpp"

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
            /// @brief print the list of local resources out to stderr
            void print_local_resources();

            // =-=-=-=-=-=-=-
            /// @brief determine if any pdmos need to run before doing a connection
            bool need_maintenance_operations( );

            // =-=-=-=-=-=-=-
            /// @brief exec the pdmos ( post disconnect maintenance operations ) in order
            int call_maintenance_operations( rcComm_t* );

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
                if ( resources_.empty() ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "empty resource table" );
                }

                // =-=-=-=-=-=-=-
                // iterate through the map and search for our path
                lookup_table< resource_ptr >::iterator itr = resources_.begin();
                for ( ; itr != resources_.end(); ++itr ) {
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
            iterator begin() { return resources_.begin(); }
            iterator end()   { return resources_.end();   }

        private:
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
            /// @brief initalize the special local file system resource
            error init_local_file_system_resource( void );

            // =-=-=-=-=-=-=-
            // Attributes
            lookup_table< resource_ptr >            resources_;
            std::vector< std::vector< pdmo_type > > maintenance_operations_;

    }; // class resource_manager

}; // namespace irods


#endif // __IRODS_RESOURCE_MANAGER_HPP__



