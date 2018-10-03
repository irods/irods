#ifndef IRODS_PLUGIN_BASE_HPP
#define IRODS_PLUGIN_BASE_HPP

#include <string>

#include <boost/function.hpp>

#include "irods_re_structs.hpp"
#include "rcConnect.h"


#ifdef ENABLE_RE
#include "irods_re_namespaceshelper.hpp"
#include "irods_re_plugin.hpp"
#include "irods_re_ruleexistshelper.hpp"
#endif
#include "irods_error.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_context.hpp"

static double PLUGIN_INTERFACE_VERSION = 2.0;

irods::error add_global_re_params_to_kvp_for_dynpep( keyValPair_t& );

namespace irods {
    typedef std::function< irods::error( rcComm_t* ) > pdmo_type;
    typedef std::function< irods::error( plugin_property_map& ) > maintenance_operation_t;

    static error default_plugin_start_operation( plugin_property_map& ) {
        return SUCCESS();
    }

    static error default_plugin_stop_operation( plugin_property_map& ) {
        return SUCCESS();
    }

    /**
     * \brief  Abstract Base Class for iRODS Plugins
     This class enforces the delay_load interface necessary for the
     load_plugin call to load any other non-member symbols from the
     shared object.
     reference lib/core/include/irods_load_plugin.hpp
    **/
    class plugin_base {
        public:
            plugin_base(
                    const std::string& _n,
                    const std::string& _c ) :
                context_( _c ),
                instance_name_( _n ),
                interface_version_( PLUGIN_INTERFACE_VERSION ),
                operations_( ),
                start_operation_( default_plugin_start_operation ),
                stop_operation_( default_plugin_stop_operation ) {
            } // ctor

            plugin_base(
                    const plugin_base& _rhs ) :
                context_( _rhs.context_ ),
                instance_name_( _rhs.instance_name_ ),
                interface_version_( _rhs.interface_version_ ),
                operations_( _rhs.operations_ ),
                start_operation_(_rhs.start_operation_),
                stop_operation_(_rhs.stop_operation_) {
            } // cctor

            plugin_base& operator=(
                    const plugin_base& _rhs ) {
                instance_name_     = _rhs.instance_name_;
                context_           = _rhs.context_;
                interface_version_ = _rhs.interface_version_;
                operations_        = _rhs.operations_;
                start_operation_   = _rhs.start_operation_;
                stop_operation_    = _rhs.stop_operation_;
                return *this;

            } // operator=

            virtual ~plugin_base( ) {
            } // dtor


            /// @brief interface to create and register a PDMO
            virtual error post_disconnect_maintenance_operation( pdmo_type& ) {
                return ERROR( NO_PDMO_DEFINED, "no defined operation" );
            }

            /// =-=-=-=-=-=-=-
            /// @brief interface to determine if a PDMO is necessary
            virtual error need_post_disconnect_maintenance_operation( bool& _b ) {
                    _b = false;
                    return SUCCESS();
            }

            /// =-=-=-=-=-=-=-
            /// @brief list all of the operations in the plugin
            error enumerate_operations( std::vector< std::string >& _ops ) {
                for ( size_t i = 0; i < ops_for_delay_load_.size(); ++i ) {
                    _ops.push_back( ops_for_delay_load_[ i ].first );
                }

                return SUCCESS();

            }

            /// =-=-=-=-=-=-=-
            /// @brief accessor for context string
            const std::string& context_string( )     const {
                return context_;
            }
            double             interface_version( ) const {
                return interface_version_;
            }

            /// =-=-=-=-=-=-=-
            /// @brief interface to add operations - key, function object
            error add_operation(
                    const std::string& _op,
                    std::function<error(plugin_context&)> _f ) {
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
                    std::function<error(plugin_context&, types_t...)> _f ) {
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

            template< typename... types_t >
            error call(
                rsComm_t*                     _comm,
                const std::string&            _operation_name,
                irods::first_class_object_ptr _fco,
                types_t...                    _t ) {
                using namespace std;
                try{
                    plugin_context ctx( _comm, properties_, _fco, "" );
                    std::function<error(plugin_context&, std::string*, types_t...)> adapted_fcn =
                        [this,&_operation_name]
                        (plugin_context& _ctx, std::string* _out_param, types_t... _t) {
                        _ctx.rule_results( *_out_param );
                        typedef std::function<error(plugin_context&,types_t...)> fcn_t;
                        fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _operation_name ] );
                        error ret = fcn( _ctx, _t... );
                        *_out_param = _ctx.rule_results();
                        return ret;
                    };

                    std::string out_param;
                    #ifdef ENABLE_RE
                    error to_return_op_err = SUCCESS();
                    ruleExecInfo_t rei;
                    memset( &rei, 0, sizeof( rei ) );
                    if (_comm) {
                        rei.rsComm        = _comm;
                        rei.uoic          = &_comm->clientUser;
                        rei.uoip          = &_comm->proxyUser;
                    }

                    rule_engine_context_manager<
                        unit,
                        ruleExecInfo_t*,
                        AUDIT_RULE > re_ctx_mgr(
                                        re_plugin_globals->global_re_mgr,
                                        &rei );

                    // invoke the pre-pep for this operation
                    error pre_err = invoke_policy_enforcement_point(
                                        re_ctx_mgr,
                                        ctx,
                                        &out_param,
                                        _operation_name,
                                        "pre",
                                        std::forward<types_t>(_t)...);
                    if(!pre_err.ok()) {
                        out_param = "error="+std::to_string(pre_err.code()) + ";message="+pre_err.result();
                        // if the pre-pep fails, invoke the exception pep
                        error except_err = invoke_policy_enforcement_point(
                                           re_ctx_mgr,
                                           ctx,
                                           &out_param,
                                           _operation_name,
                                           "except",
                                           std::forward<types_t>(_t)...);
                        if(!except_err.ok()) {
                            irods::log(PASS(except_err));
                        }
                        return pre_err;
                    }

                    to_return_op_err = adapted_fcn(
                                           ctx,
                                           &out_param,
                                           forward<types_t>(_t)...);
                    if(!to_return_op_err.ok()) {
                        // if the operation fails, invoke the exception pep
                        out_param = "error="+std::to_string(to_return_op_err.code()) + ";message="+to_return_op_err.result();
                        error except_err = invoke_policy_enforcement_point(
                                               re_ctx_mgr,
                                               ctx,
                                               &out_param,
                                               _operation_name,
                                               "except",
                                               forward<types_t>(_t)...);
                        if(!except_err.ok()) {
                            irods::log(PASS(except_err));
                        }
                        return to_return_op_err;
                    }


                    // invoke the post-pep for this operation
                    error post_err = invoke_policy_enforcement_point(
                                         re_ctx_mgr,
                                         ctx,
                                         &out_param,
                                         _operation_name,
                                         "post",
                                         forward<types_t>(_t)...);
                    if(!post_err.ok()) {
                        out_param = "error="+std::to_string(post_err.code()) + ";message="+post_err.result();
                        // if the post-pep fails, invoke the exception pep
                        error except_err = invoke_policy_enforcement_point(
                                               re_ctx_mgr,
                                               ctx,
                                               &out_param,
                                               _operation_name,
                                               "except",
                                               forward<types_t>(_t)...);
                        if(!except_err.ok()) {
                            irods::log(PASS(except_err));
                        }
                        return post_err;
                    }

                    return to_return_op_err;
                    #else
                    return adapted_fcn( ctx, &out_param, forward<types_t>(_t)... );
                    #endif
                } catch ( const boost::bad_any_cast& ) {
                    std::string msg( "failed for call - " );
                    msg += _operation_name;
                    return ERROR(
                            INVALID_ANY_CAST,
                            msg );
                }

            } // call

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

            void set_start_operation( maintenance_operation_t _op ) {
                start_operation_ = _op;
            }

            void set_stop_operation( maintenance_operation_t _op ) {
                stop_operation_ = _op;
            }

            error start_operation() {
                return start_operation_( properties_ );
            }

            error stop_operation() {
                return stop_operation_( properties_ );
            }

        protected:
            std::string context_;           // context string for this plugin
            std::string instance_name_;     // name of this instance of the plugin
            double      interface_version_; // version of the plugin interface supported

            /// =-=-=-=-=-=-=-
            /// @brief heterogeneous key value map of plugin data
            plugin_property_map properties_;

            /// =-=-=-=-=-=-=-
            /// @brief Map holding resource operations
            std::vector< std::pair< std::string, std::string > > ops_for_delay_load_;

            // =-=-=-=-=-=-=-
            /// @brief operations to be loaded from the plugin
            lookup_table< boost::any > operations_;

            maintenance_operation_t start_operation_;
            maintenance_operation_t stop_operation_;

        private:
            #ifdef ENABLE_RE
            template<typename... types_t>
            error invoke_policy_enforcement_point(
                rule_engine_context_manager<
                                    unit,
                                    ruleExecInfo_t*,
                                    AUDIT_RULE > _re_ctx_mgr,
                plugin_context      _ctx,
                std::string*        _out_param,
                const std::string&  _operation_name,
                const std::string&  _class,
                types_t...          _t) {
                bool ret = false;
                error saved_op_err = SUCCESS();
                for ( auto& ns : NamespacesHelper::Instance()->getNamespaces() ) {
                    std::string rule_name = ns + "pep_" + _operation_name + "_" + _class;

                    if (RuleExistsHelper::Instance()->checkOperation( rule_name ) ) {
                        if (_re_ctx_mgr.rule_exists(rule_name, ret).ok() && ret) {
                            error op_err = _re_ctx_mgr.exec_rule(
                                               rule_name,
                                               instance_name_,
                                               _ctx,
                                               _out_param,
                                               std::forward<types_t>(_t)...);
                            if (!op_err.ok()) {
                                rodsLog(
                                    LOG_DEBUG,
                                    "%s-pep rule [%s] failed with error code [%d]",
                                    _class.c_str(),
                                    rule_name.c_str(),
                                    op_err.code());
                                saved_op_err = op_err;
                            }
                        } else {
                            rodsLog(
                                LOG_DEBUG10,
                                "Rule [%s] passes regex test, but does not exist",
                                rule_name.c_str() );
                        }
                    }
                }
                return saved_op_err;

            } // invoke_policy_enforcement_point
            #endif

    }; // class plugin_base

    // =-=-=-=-=-=-=-
    // helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr<plugin_base> plugin_ptr;

}; // namespace irods

#endif // IRODS_PLUGIN_BASE_HPP
