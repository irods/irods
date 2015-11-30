#ifndef ___IRODS_RESC_PLUGIN_HPP__
#define ___IRODS_RESC_PLUGIN_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_resource_constants.hpp"
#include "irods_operation_wrapper.hpp"
#include "irods_resource_plugin_context.hpp"
#include "irods_operation_rule_execution_manager_base.hpp"

#include <iostream>
#include <utility>
#include <boost/any.hpp>

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

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function object
            error add_operation(
                    const std::string& _op,
                    std::function<error(resource_plugin_context&)> _f ) {
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
                    std::function<error(resource_plugin_context&, types_t...)> _f ) {
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
                rsComm_t*                     _comm,
                const std::string&            _op,
                irods::first_class_object_ptr _obj ) {

                try {
                    typedef std::function<error(resource_plugin_context&)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );

                    oper_rule_exec_mgr_ptr rex_mgr(
                        new operation_rule_execution_manager(
                                instance_name_, _op ) );
                    
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
            /// @brief delegate the call to the operation in question to the operation wrapper
            template< typename... types_t>
            error call(
                rsComm_t*                     _comm,
                const std::string&            _op,
                irods::first_class_object_ptr _obj,
                types_t... _t ) {

                try {
                    typedef std::function<error(resource_plugin_context&,types_t...)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    resource_plugin_context ctx( properties_, _obj, "", _comm, children_ );

                    oper_rule_exec_mgr_ptr rex_mgr(
                        new operation_rule_execution_manager(
                                instance_name_, _op ) );
                    
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
            resource_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            resource_maintenance_operation stop_operation_;

            // =-=-=-=-=-=-=-
            /// @brief Pointers to Child and Parent Resources
            resource_child_map  children_;
            resource_ptr        parent_;

    }; // class resource

// =-=-=-=-=-=-=-
// given the name of a resource, try to load the shared object
    error load_resource_plugin( resource_ptr&,       // plugin
                                const std::string,   // plugin name
                                const std::string,   // instance name
                                const std::string ); // context string


}; // namespace irods


#endif // ___IRODS_RESC_PLUGIN_HPP__



