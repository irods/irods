#ifndef ___IRODS_NETWORK_PLUGIN_HPP__
#define ___IRODS_NETWORK_PLUGIN_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_context.hpp"
#include "irods_network_types.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_load_plugin.hpp"
#include <dlfcn.h>

#include <iostream>

extern "C"
void* operation_rule_execution_manager_factory(
        const char* _plugin_name,
        const char* _operation_name );

namespace irods {
    typedef error( *network_maintenance_operation )( plugin_property_map& );

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class network : public plugin_base {
        public:
            // =-=-=-=-=-=-=-
            // public - ctor
            network(
                    const std::string& _inst,
                    const std::string& _ctx ) :
                plugin_base(
                        _inst,
                        _ctx ),
                start_operation_( default_start_operation ),
                stop_operation_( default_stop_operation ) {
                } // ctor

            // =-=-=-=-=-=-=-
            // public - dtor
            virtual ~network( ) {

            } // dtor

            // =-=-=-=-=-=-=-
            // public - cctor
            network(
                    const network& _rhs ) :
                plugin_base( _rhs ) {
                    start_operation_    = _rhs.start_operation_;
                    stop_operation_     = _rhs.stop_operation_;
                    operations_         = _rhs.operations_;
                    ops_for_delay_load_ = _rhs.ops_for_delay_load_;

                    if ( properties_.size() > 0 ) {
                        std::cout << "[!]\tnetwork cctor - properties map is not empty."
                            << __FILE__ << ":" << __LINE__ << std::endl;
                    }
                    properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?
                } // cctor

            // =-=-=-=-=-=-=-
            // public - assignment
            network& operator=(
                    const network& _rhs ) {
                if ( &_rhs == this ) {
                    return *this;
                }

                plugin_base::operator=( _rhs );

                operations_         = _rhs.operations_;
                ops_for_delay_load_ = _rhs.ops_for_delay_load_;

                if ( properties_.size() > 0 ) {
                    std::cout << "[!]\tnetwork cctor - properties map is not empty."
                        << __FILE__ << ":" << __LINE__ << std::endl;
                }

                properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?

                return *this;

            } // operator=

            // =-=-=-=-=-=-=-
            // public - function which pulls all of the symbols out of the shared object and
            //          associates them with their keys in the operations table
            virtual error delay_load(
                    void* _handle ) {
                // =-=-=-=-=-=-=-
                // check params
                if ( ! _handle ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "void handle pointer" );
                }

                if ( ops_for_delay_load_.empty() ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "empty operations list" );
                }

                // =-=-=-=-=-=-=-
                // check if we need to load a start function
                if ( !start_opr_name_.empty() ) {
                    dlerror();
                    network_maintenance_operation start_op = reinterpret_cast< network_maintenance_operation >(
                            dlsym( _handle, start_opr_name_.c_str() ) );
                    if ( !start_op ) {
                        std::stringstream msg;
                        msg  << "failed to load start function ["
                            << start_opr_name_ << "]";
                        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                    }
                    else {
                        start_operation_ = start_op;
                    }
                } // if load start


                // =-=-=-=-=-=-=-
                // check if we need to load a stop function
                if ( !stop_opr_name_.empty() ) {
                    dlerror();
                    network_maintenance_operation stop_op = reinterpret_cast< network_maintenance_operation >(
                            dlsym( _handle, stop_opr_name_.c_str() ) );
                    if ( !stop_op ) {
                        std::stringstream msg;
                        msg << "failed to load stop function ["
                            << stop_opr_name_ << "]";
                        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                    }
                    else {
                        stop_operation_ = stop_op;
                    }
                } // if load stop


                // =-=-=-=-=-=-=-
                // iterate over list and load function. then add it to the map via wrapper functor
                std::vector< std::pair< std::string, std::string > >::iterator itr = ops_for_delay_load_.begin();
                for ( ; itr != ops_for_delay_load_.end(); ++itr ) {
                    // =-=-=-=-=-=-=-
                    // cache values in obvious variables
                    std::string& key = itr->first;
                    std::string& fcn = itr->second;

                    // =-=-=-=-=-=-=-
                    // check key and fcn name before trying to load
                    if ( key.empty() ) {
                        std::cout << "[!]\tirods::network::delay_load - empty op key for ["
                            << fcn << "], skipping." << std::endl;
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // check key and fcn name before trying to load
                    if ( fcn.empty() ) {
                        std::cout << "[!]\tirods::network::delay_load - empty function name for ["
                            << key << "], skipping." << std::endl;
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // call dlsym to load and check results
                    dlerror();
                    plugin_operation res_op_ptr = reinterpret_cast< plugin_operation >(
                                                      dlsym(
                                                          _handle,
                                                          fcn.c_str() ) );
                    if ( !res_op_ptr ) {
                        std::cout << "[!]\tirods::network::delay_load - failed to load ["
                            << fcn << "].  error - " << dlerror() << std::endl;
                        continue;
                    }

                    // =-=-=-=-=-=-=-
                    // add the operation via a wrapper to the operation map
                    oper_rule_exec_mgr_ptr rex_mgr;
                    rex_mgr.reset(
                            reinterpret_cast<operation_rule_execution_manager_base*>(
                                operation_rule_execution_manager_factory(
                                    instance_name_.c_str(),
                                    key.c_str() ) ) );

                    // =-=-=-=-=-=-=-
                    // add the operation via a wrapper to the operation map
                    operations_[ key ] = operation_wrapper(
                            rex_mgr,
                            instance_name_,
                            key,
                            res_op_ptr );
                } // for itr


                // =-=-=-=-=-=-=-
                // see if we loaded anything at all
                if ( operations_.size() < 0 ) {
                    return ERROR( SYS_INVALID_INPUT_PARAM, "operations map is emtpy" );
                }

                return SUCCESS();

            } // delay_load

            // =-=-=-=-=-=-=-
            // public - set a name for the developer provided start op
            void set_start_operation(
                    const std::string& _op ) {
                start_opr_name_ = _op;
            } // set_start_operation

            // =-=-=-=-=-=-=-
            // public - set a name for the developer provided stop op
            void set_stop_operation(
                    const std::string& _op ) {
                stop_opr_name_ = _op;
            } // set_stop_operation

            // =-=-=-=-=-=-=-
            /// @brief get a property from the map if it exists.  catch the exception in the case where
            // the template types may not match and return sucess/fail
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                error ret = properties_.get< T >( _key, _val );
                return PASS( ret );
            } // get_property

            // =-=-=-=-=-=-=-
            /// @brief set a property in the map
            template< typename T >
            error set_property( const std::string& _key, const T& _val ) {
                error ret = properties_.set< T >( _key, _val );
                return PASS( ret );
            } // set_property

            // =-=-=-=-=-=-=-
            /// @brief interface to call start / stop functions
            error start_operation( void ) {
                return ( *start_operation_ )( properties_ );
            }
            error stop_operation( void ) {
                return ( *stop_operation_ )( properties_ );
            }

            // =-=-=-=-=-=-=-
            /// @brief default start operation
            static error default_start_operation(
                plugin_property_map& ) {
                return SUCCESS();
            };

            // =-=-=-=-=-=-=-
            /// @brief default stop operation
            static error default_stop_operation(
                plugin_property_map& ) {
                return SUCCESS();
            };

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call( ctx );

            } // call -

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
            template< typename T1 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1 >( ctx, _t1 );

            } // call - T1

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 2 params
            template< typename T1, typename T2 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2 >(
                           ctx, _t1, _t2 );

            } // call - T1, T2

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 3 params
            template< typename T1, typename T2, typename T3 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3 >(
                           ctx, _t1, _t2, _t3 );

            } // call - T1, T2, T3

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 4 params
            template< typename T1, typename T2, typename T3, typename T4 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4 ) {
                plugin_context ctx( properties_, _obj, "" );
                return  operations_[ _op ].call< T1, T2, T3, T4 >(
                            ctx, _t1, _t2, _t3, _t4 );

            } // call - T1, T2, T3, T4

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 5 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3, T4, T5 >(
                           ctx, _t1, _t2, _t3, _t4, _t5 );

            } // call - T1, T2, T3, T4, T5

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 6 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5,
                T6 _t6 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6 );

            } // call - T1, T2, T3, T4, T5, T6

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 7 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
            error call(
                const std::string& _op,
                irods::first_class_object_ptr _obj,
                T1 _t1,
                T2 _t2,
                T3 _t3,
                T4 _t4,
                T5 _t5,
                T6 _t6,
                T7 _t7 ) {
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7 );

            } // call - T1, T2, T3, T4, T5, T6, T7

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 8 params
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
            error call(
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
                plugin_context ctx( properties_, _obj, "" );
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >(
                           ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );

            } // call - T1, T2, T3, T4, T5, T6, T7, T8

        protected:

            // =-=-=-=-=-=-=-
            /// @brief Start / Stop functions - can be overwritten by plugin devs
            //         called when a plugin is loaded or unloaded for init / cleanup
            std::string start_opr_name_;
            network_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            network_maintenance_operation stop_operation_;

            // =-=-=-=-=-=-=-
            /// @brief operations to be loaded from the plugin
            lookup_table< operation_wrapper > operations_;

    }; // class network

}; // namespace irods


#endif // ___IRODS_NETWORK_PLUGIN_HPP__



