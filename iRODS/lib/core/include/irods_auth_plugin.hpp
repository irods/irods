#ifndef _AUTH_HPP_
#define _AUTH_HPP_

#include "irods_error.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_auth_types.hpp"
#include "irods_auth_plugin_context.hpp"

namespace irods {

/// @brief Type of an operation for start / stop
    typedef error( *auth_maintenance_operation )( plugin_property_map& );

    /**
     * @brief Base class for auth plugins
     */
    class auth : public plugin_base {
        public:
            /// @brief Constructor
            auth(
                const std::string& _name, // instance name
                const std::string& _ctx   // context
            );

            /// @brief Copy ctor
            auth( const auth& _rhs );

            virtual ~auth();

            /// @brief Assignment operator
            auth& operator=( const auth& _rhs );

            /// @brief Load operations from the plugin - overrides plugin base
            virtual error delay_load( void* _handle );

            /// @brief get a property from the map if it exists.
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                error ret = properties_.get< T >( _key, _val );
                return ASSERT_PASS( ret, "Failed to get property for auth plugin." );
            } // get_property

            /// @brief set a property in the map
            template< typename T >
            error set_property( const std::string& _key, const T& _val ) {
                error ret = properties_.set< T >( _key, _val );
                return ASSERT_PASS( ret, "Failed to set property in the auth plugin." );
            } // set_property

            /// @brief Interface to set the start function
            error set_start_operation( const std::string& _name );

            /// @brief Interface to set the stop function
            error set_stop_operation( const std::string& _name );

            /// @brief interface to call start / stop functions
            error start_operation( void ) {
                return ( *start_operation_ )( properties_ );
            }
            error stop_operation( void ) {
                return ( *stop_operation_ )( properties_ );
            }

            /// @brief default start operation
            static error default_start_operation( plugin_property_map& ) {
                return SUCCESS();
            }

            /// @brief default stop operation
            static error default_stop_operation( plugin_property_map& ) {
                return SUCCESS();
            }

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 0 param
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj ) {
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
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
                auth_plugin_context ctx( _comm, properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );

            } // call - T1, T2, T3, T4, T5, T6, T7, T8


        protected:

            // =-=-=-=-=-=-=-
            /// @brief Start / Stop functions - can be overwritten by plugin devs
            //         called when a plugin is loaded or unloaded for init / cleanup
            std::string start_opr_name_;
            auth_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            auth_maintenance_operation stop_operation_;

            // =-=-=-=-=-=-=-
            /// @brief operations to be loaded from the plugin
            lookup_table< operation_wrapper > operations_;

    };

/// @brief Given the name of an auth , try to load the shared object
    error load_auth_plugin(
        auth_ptr&,              // plugin
        const std::string&,     // plugin name
        const std::string&,     // instance name
        const std::string& );   // context string

}; // namespace irods

#endif // _AUTH_HPP_
