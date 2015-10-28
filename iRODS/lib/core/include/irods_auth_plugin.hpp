#ifndef _AUTH_HPP_
#define _AUTH_HPP_

#include "irods_error.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_auth_types.hpp"
#include "irods_auth_plugin_context.hpp"
#include "irods_load_plugin.hpp"
#include "dlfcn.h"

namespace irods {

/// @brief Type of an operation for start / stop
    typedef error( *auth_maintenance_operation )( plugin_property_map& );

    /**
     * @brief Base class for auth plugins
     */
    class auth : public plugin_base {
        public:

            auth(
                const std::string& _inst,
                const std::string& _ctx ) :
                plugin_base( _inst, _ctx ),
                start_operation_( default_start_operation ),
                stop_operation_( default_stop_operation ) {

            }

            virtual ~auth() {
            }

            auth(
                const auth& _rhs ) :
                plugin_base( _rhs ) {
                start_operation_ = _rhs.start_operation_;
                stop_operation_ = _rhs.stop_operation_;
                operations_ = _rhs.operations_;
                ops_for_delay_load_ = _rhs.ops_for_delay_load_;

                if ( properties_.size() > 0 ) {
                    std::cout << "[!]\tauth cctor - properties map is not empty." << __FILE__ << ":" << __LINE__ << std::endl;
                }

                properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers
            }

            auth& operator=(
                const auth& _rhs ) {
                if ( &_rhs == this ) {
                    return *this;
                }

                plugin_base::operator=( _rhs );

                operations_ = _rhs.operations_;
                ops_for_delay_load_ = _rhs.ops_for_delay_load_;

                if ( properties_.size() > 0 ) {
                    std::cout << "[!]\tauth assignment operator - properties map is not empty."
                              << __FILE__ << ":" << __LINE__ << std::endl;
                }

                properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers

                return *this;
            }

            virtual error delay_load(
                void* _handle ) {
                error result = SUCCESS();
                if ( ( result = ASSERT_ERROR( _handle != NULL, SYS_INVALID_INPUT_PARAM, "Void handle pointer." ) ).ok() ) {

                    if ( ( result = ASSERT_ERROR( !ops_for_delay_load_.empty(), SYS_INVALID_INPUT_PARAM, "Empty operations list." ) ).ok() ) {

                        // Check if we need to load a start function
                        if ( !start_opr_name_.empty() ) {
                            dlerror();
                            auth_maintenance_operation start_op = reinterpret_cast<auth_maintenance_operation>( dlsym( _handle, start_opr_name_.c_str() ) );
                            if ( ( result = ASSERT_ERROR( start_op, SYS_INVALID_INPUT_PARAM, "Failed to load start function: \"%s\" - %s.",
                                                          start_opr_name_.c_str(), dlerror() ) ).ok() ) {
                                start_operation_ = start_op;
                            }
                        }

                        // Check if we need to load a stop function
                        if ( result.ok() && !stop_opr_name_.empty() ) {
                            dlerror();
                            auth_maintenance_operation stop_op = reinterpret_cast<auth_maintenance_operation>( dlsym( _handle, stop_opr_name_.c_str() ) );
                            if ( ( result = ASSERT_ERROR( stop_op, SYS_INVALID_INPUT_PARAM, "Failed to load stop function: \"%s\" - %s.",
                                                          stop_opr_name_.c_str(), dlerror() ) ).ok() ) {
                                stop_operation_ = stop_op;
                            }
                        }

                        // Iterate over the list of operation names, load the functions and add it to the map via the wrapper function
                        std::vector<std::pair<std::string, std::string> >::iterator itr = ops_for_delay_load_.begin();
                        for ( ; result.ok() && itr != ops_for_delay_load_.end(); ++itr ) {

                            std::string key = itr->first;
                            std::string fcn = itr->second;

                            // load the function
                            dlerror();  // reset error messages
                            plugin_operation res_op_ptr = reinterpret_cast<plugin_operation>( dlsym( _handle, fcn.c_str() ) );
                            if ( ( result = ASSERT_ERROR( res_op_ptr, SYS_INVALID_INPUT_PARAM, "Failed to load function: \"%s\" for operation: \"%s\" - %s.",
                                                          fcn.c_str(), key.c_str(), dlerror() ) ).ok() ) {
                                #ifdef RODS_SERVER
                                oper_rule_exec_mgr_ptr rex_mgr(
                                    new operation_rule_execution_manager( instance_name_, key ) );
                                #else
                                oper_rule_exec_mgr_ptr rex_mgr(
                                    new operation_rule_execution_manager_no_op( instance_name_, key ) );
                                #endif
                                // Add the operation via a wrapper to the operation map
                                operations_[key] = operation_wrapper( rex_mgr, instance_name_, key, res_op_ptr );
                            }
                        }
                    }
                }
                return result;
            }

            error set_start_operation(
                const std::string& _name ) {
                error result = SUCCESS();
                start_opr_name_ = _name;
                return result;
            }

            error set_stop_operation(
                const std::string& _name ) {
                error result = SUCCESS();
                stop_opr_name_ = _name;
                return result;
            }

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

}; // namespace irods

#endif // _AUTH_HPP_
