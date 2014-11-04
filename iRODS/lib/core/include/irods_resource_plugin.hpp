#ifndef ___IRODS_RESC_PLUGIN_HPP__
#define ___IRODS_RESC_PLUGIN_HPP__

// =-=-=-=-=-=-=-
#include "irods_resource_constants.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_resource_plugin_context.hpp"

#include <iostream>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief typedef for resource maintenance operation for start / stop operations
    typedef error( *resource_maintenance_operation )(
        plugin_property_map&,
        resource_child_map& );

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class resource : public plugin_base {
        public:

            // =-=-=-=-=-=-=-
            /// @brief Constructors
            resource( const std::string&,   // instance name
                      const std::string& ); // context

            // =-=-=-=-=-=-=-
            /// @brief Destructor
            virtual ~resource();

            // =-=-=-=-=-=-=-
            /// @brief copy ctor
            resource( const resource& _rhs );

            // =-=-=-=-=-=-=-
            /// @brief Assignment Operator - necessary for stl containers
            resource& operator=( const resource& _rhs );

            // =-=-=-=-=-=-=-
            /// @brief override from parent plugin_base
            virtual error delay_load( void* _h );

            // =-=-=-=-=-=-=-
            /// @brief get a property from the map if it exists.  catch the exception in the case where
            // the template types may not match and return success/fail
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                error ret = properties_.get< T >( _key, _val );
                return PASSMSG( "resource::get_property", ret );
            } // get_property

            // =-=-=-=-=-=-=-
            /// @brief set a property in the map
            template< typename T >
            error set_property( const std::string& _key, const T& _val ) {
                error ret = properties_.set< T >( _key, _val );
                return PASSMSG( "resource::set_property", ret );
            } // set_property

            // =-=-=-=-=-=-=-
            /// @brief interface to add and remove children using the zone_name::resource_name
            virtual error add_child( const std::string&, const std::string&, resource_ptr );
            virtual error remove_child( const std::string& );
            virtual int   num_children() {
                return children_.size();
            }
            virtual bool has_child(
                const std::string& _name ) {
                return children_.has_entry( _name );
            }

            // =-=-=-=-=-=-=-
            /// @brief interface to get and set a resource's parent pointer
            virtual error set_parent( const resource_ptr& );
            virtual error get_parent( resource_ptr& );

            // =-=-=-=-=-=-=-
            /// @brief interface to set start / stop functions
            void set_start_operation( const std::string& );
            void set_stop_operation( const std::string& );

            // =-=-=-=-=-=-=-
            /// @brief interface to call start / stop functions
            error start_operation( void ) {
                return ( *start_operation_ )( properties_, children_ );
            }
            error stop_operation( void ) {
                return ( *stop_operation_ )( properties_, children_ );
            }

            // =-=-=-=-=-=-=-
            /// @brief default start operation
            static error default_start_operation( plugin_property_map&,
                                                  resource_child_map& ) {
                return SUCCESS();
            };

            // =-=-=-=-=-=-=-
            /// @brief default stop operation
            static error default_stop_operation( plugin_property_map&,
                                                 resource_child_map& ) {
                return SUCCESS();
            };

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 0 param
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call( ctx );

            } // call -

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
            template< typename T1 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1 >( ctx, _t1 );

            } // call - T1

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 2 params
            template< typename T1, typename T2 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2 >( ctx, _t1, _t2 );

            } // call - T1, T2

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 3 params
            template< typename T1, typename T2, typename T3 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2, T3 >(
                           ctx, _t1, _t2, _t3 );

            } // call - T1, T2, T3

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 4 params
            template< typename T1, typename T2, typename T3, typename T4 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return  operations_[ _op ].call< T1, T2, T3, T4 >(
                            ctx, _t1, _t2, _t3, _t4 );

            } // call - T1, T2, T3, T4

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 5 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2, T3, T4, T5 >(
                           ctx, _t1, _t2, _t3, _t4, _t5 );

            } // call - T1, T2, T3, T4, T5

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 6 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5,
                T6 _t6 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6 );

            } // call - T1, T2, T3, T4, T5, T6

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 7 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5,
                T6 _t6,
                T7 _t7 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7 );

            } // call - T1, T2, T3, T4, T5, T6, T7

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 8 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5,
                T6 _t6,
                T7 _t7,
                T8 _t8 ) {
                resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );

            } // call - T1, T2, T3, T4, T5, T6, T7, T8

        protected:

            // =-=-=-=-=-=-=-
            /// @brief Start / Stop functions - can be overwritten by plugin devs
            //         called when a plugin is loaded or unloaded for init / cleanup
            std::string start_opr_name_;
            resource_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            resource_maintenance_operation stop_operation_;

            // =-=-=-=-=-=-=-
            /// @brief Pointers to Child and Parent Resources
            resource_child_map  children_;
            resource_ptr        parent_;

            // =-=-=-=-=-=-=-
            /// @brief operations to be loaded from the plugin
            lookup_table< operation_wrapper > operations_;

    }; // class resource

// =-=-=-=-=-=-=-
// given the name of a resource, try to load the shared object
    error load_resource_plugin( resource_ptr&,       // plugin
                                const std::string,   // plugin name
                                const std::string,   // instance name
                                const std::string ); // context string


}; // namespace irods


#endif // ___IRODS_RESC_PLUGIN_HPP__



