#ifndef _AUTH_HPP_
#define _AUTH_HPP_

#include "irods_error.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_auth_types.hpp"
#include "irods_auth_plugin_context.hpp"
#include "irods_load_plugin.hpp"
#include "dlfcn.h"

#include <utility>
#include <boost/any.hpp>

namespace irods {

extern "C"
void* operation_rule_execution_manager_factory(
        const char* _plugin_name,
        const char* _operation_name );

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
                            oper_rule_exec_mgr_ptr rex_mgr;
                            rex_mgr.reset(
                                    reinterpret_cast<operation_rule_execution_manager_base*>(
                                        operation_rule_execution_manager_factory(
                                            instance_name_.c_str(),
                                            key.c_str() ) ) );

                            // Add the operation via a wrapper to the operation map
                            operations_[key] = operation_wrapper<void>( rex_mgr, instance_name_, key, res_op_ptr );
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

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function object
            error add_operation(
                    const std::string& _op,
                    std::function<error(auth_plugin_context&)> _f ) {
                // =-=-=-=-=-=-=-
                // check params
                if ( _op.empty() ) {
                    std::stringstream msg;
                    msg << "empty operation [" << _op << "]";
                    return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                }
                operations_[_op] = _f;
                return SUCCESS();

            }

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function object
            template<typename... types_t>
            error add_operation(
                    const std::string& _op,
                    std::function<error(auth_plugin_context&, types_t...)> _f ) {
                // =-=-=-=-=-=-=-
                // check params
                if ( _op.empty() ) {
                    std::stringstream msg;
                    msg << "empty operation [" << _op << "]";
                    return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
                }
                operations_[_op] = _f;
                return SUCCESS();

            }

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj ) {
                
                try{
                    typedef std::function<error(auth_plugin_context&)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    auth_plugin_context ctx( _comm, properties_, _obj, "" );

                    oper_rule_exec_mgr_ptr rex_mgr;
                    rex_mgr.reset(
                            reinterpret_cast<operation_rule_execution_manager_base*>(
                                operation_rule_execution_manager_factory(
                                    instance_name_.c_str(),
                                    _op.c_str() ) ) );

                    keyValPair_t kvp;
                    bzero( &kvp, sizeof( kvp ) );
                    ctx.fco()->get_re_vars( kvp );

                    std::string pre_results;
                    error ret = rex_mgr->exec_pre_op( ctx.comm(), kvp, pre_results );
                    if ( !ret.ok() && ret.code() != SYS_RULE_NOT_FOUND ) {
                        return PASS( ret );
                    }

                    ctx.rule_results( pre_results );
                    error op_err = fcn( ctx );

                    std::string rule_results =  ctx.rule_results();
                    rex_mgr->exec_post_op( ctx.comm(), kvp, rule_results );

                    clearKeyVal( &kvp );

                    return op_err;

                } catch ( const boost::bad_any_cast& ) {
                    std::string msg( "failed for call - " );
                    msg += _op;
                    return ERROR(
                            INVALID_ANY_CAST,
                            msg );
                }
            }

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
            template< typename... types_t >
            error call(
                rsComm_t*                     _comm,
                const std::string&            _op,
                irods::first_class_object_ptr _obj,
                types_t...                    _t ) {
                try{
                    typedef std::function<error(auth_plugin_context&,types_t...)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    auth_plugin_context ctx( _comm, properties_, _obj, "" );

                    oper_rule_exec_mgr_ptr rex_mgr;
                    rex_mgr.reset(
                            reinterpret_cast<operation_rule_execution_manager_base*>(
                                operation_rule_execution_manager_factory(
                                    instance_name_.c_str(),
                                    _op.c_str() ) ) );

                    keyValPair_t kvp;
                    bzero( &kvp, sizeof( kvp ) );
                    ctx.fco()->get_re_vars( kvp );

                    std::string pre_results;
                    error ret = rex_mgr->exec_pre_op( ctx.comm(), kvp, pre_results );
                    if ( !ret.ok() && ret.code() != SYS_RULE_NOT_FOUND ) {
                        return PASS( ret );
                    }

                    ctx.rule_results( pre_results );
                    error op_err = fcn( ctx, std::forward<types_t>(_t)... );

                    std::string rule_results =  ctx.rule_results();
                    rex_mgr->exec_post_op( ctx.comm(), kvp, rule_results );

                    clearKeyVal( &kvp );

                    return op_err;

                } catch ( const boost::bad_any_cast& ) {
                    std::string msg( "failed for call - " );
                    msg += _op;
                    return ERROR(
                            INVALID_ANY_CAST,
                            msg );
                }
            }

        protected:

            // =-=-=-=-=-=-=-
            /// @brief Start / Stop functions - can be overwritten by plugin devs
            //         called when a plugin is loaded or unloaded for init / cleanup
            std::string start_opr_name_;
            auth_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            auth_maintenance_operation stop_operation_;

    };

}; // namespace irods

#endif // _AUTH_HPP_
