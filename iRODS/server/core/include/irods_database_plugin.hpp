#ifndef ___IRODS_DATABASE_PLUGIN_HPP__
#define ___IRODS_DATABASE_PLUGIN_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_plugin_context.hpp"
#include "irods_database_types.hpp"
#include "irods_operation_wrapper.hpp"
#ifndef LINK_NO_OP_RE_MGR
#include "irods_operation_rule_execution_manager_base.hpp"
#endif

#include <iostream>
#include <utility>
#include <boost/any.hpp>

namespace irods {
    typedef error( *database_maintenance_operation )( plugin_property_map& );
// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class database : public plugin_base {
        public:
            // =-=-=-=-=-=-=-
            /// @brief Constructors
            database( const std::string&,   // instance name
                      const std::string& ); // context

            // =-=-=-=-=-=-=-
            /// @brief Destructor
            virtual ~database();

            // =-=-=-=-=-=-=-
            /// @brief copy ctor
            database( const database& _rhs );

            // =-=-=-=-=-=-=-
            /// @brief Assignment Operator - necessary for stl containers
            database& operator=( const database& _rhs );

            // =-=-=-=-=-=-=-
            /// @brief override from parent plugin_base
            virtual error delay_load( void* _h );

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
            /// @brief interface to set start / stop functions
            void set_start_operation( const std::string& );
            void set_stop_operation( const std::string& );

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

            // =-=-=-=-=-=-=-
            /// @brief delegate the call to the operation in question to the operation wrapper
            error call(
                rsComm_t* _comm,
                const std::string& _op,
                irods::first_class_object_ptr _obj ) {
                
                try {
                    typedef std::function<error(plugin_context&)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    plugin_context ctx( _comm, properties_, _obj, "" );
#ifndef LINK_NO_OP_RE_MGR
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
#endif
                    error op_err = fcn( ctx );

#ifndef LINK_NO_OP_RE_MGR
                    std::string rule_results =  ctx.rule_results();
                    rex_mgr->exec_post_op( ctx.comm(), kvp, rule_results );

                    clearKeyVal( &kvp );
#endif

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
            template< typename... types_t >
            error call(
                rsComm_t*                     _comm,
                const std::string&            _op,
                irods::first_class_object_ptr _obj,
                types_t...                    _t ) {

                try {
                    typedef std::function<error(plugin_context&,types_t...)> fcn_t;
                    fcn_t& fcn = boost::any_cast< fcn_t& >( operations_[ _op ] );
                    plugin_context ctx( _comm, properties_, _obj, "" );
#ifndef LINK_NO_OP_RE_MGR
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
#endif
                    error op_err = fcn( ctx, std::forward<types_t>(_t)... );

#ifndef LINK_NO_OP_RE_MGR
                    std::string rule_results =  ctx.rule_results();
                    rex_mgr->exec_post_op( ctx.comm(), kvp, rule_results );

                    clearKeyVal( &kvp );
#endif

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
            database_maintenance_operation start_operation_;

            std::string stop_opr_name_;
            database_maintenance_operation stop_operation_;

    }; // class database

// =-=-=-=-=-=-=-
// given the name of a database, try to load the shared object
    error load_database_plugin(
        database_ptr&,        // plugin
        const std::string&,   // plugin name
        const std::string&,   // instance name
        const std::string& ); // context string

}; // namespace irods


#endif // ___IRODS_DATABASE_PLUGIN_HPP__



