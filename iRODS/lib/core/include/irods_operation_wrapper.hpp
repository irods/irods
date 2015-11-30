#ifndef ___IRODS_OPERATION_WRAPPER_HPP__
#define ___IRODS_OPERATION_WRAPPER_HPP__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <utility>

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_context.hpp"
#include "irods_operation_rule_execution_manager.hpp"
#include "irods_operation_rule_execution_manager_no_op.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcMisc.h"
#include "reGlobalsExtern.hpp"
#include "dataObjInpOut.h"

#include <iostream>
#include <functional>
#include <utility>

namespace irods {

    // =-=-=-=-=-=-=-
    // const signifying failure of operation call for post dynPEP
    static std::string OP_FAILED( "OPERATION_FAILED" );

    template< typename... types_t >
    class operation_wrapper {
        public:
            // =-=-=-=-=-=-=-
            /// @brief constructors
            operation_wrapper( ) : operation_( 0 ) {
            } // default ctor

            // =-=-=-=-=-=-=-
            // public - ctor with opreation
            operation_wrapper(
                    oper_rule_exec_mgr_ptr _rule_exec,
                    const std::string&     _inst_name,
                    const std::string&     _op_name,
                    std::function<error(plugin_context&,types_t...)> _op ) :
                rule_exec_mgr_( _rule_exec ),
                instance_name_( _inst_name ),
                operation_name_( _op_name ),
                operation_( _op ) {
                } // ctor

            // =-=-=-=-=-=-=-
            /// @brief destructor
            virtual ~operation_wrapper( ) {
            } // dtor

    
            // =-=-=-=-=-=-=-
            /// @brief copy constructor - necessary for stl containers
            operation_wrapper(
                    const operation_wrapper& _rhs ) {
                rule_exec_mgr_  = _rhs.rule_exec_mgr_;
                operation_      = _rhs.operation_;
                instance_name_  = _rhs.instance_name_;
                operation_name_ = _rhs.operation_name_;
            } // cctor

            // =-=-=-=-=-=-=-
            /// @brief assignment operator - necessary for stl containers
            operation_wrapper& operator=(
                    const operation_wrapper& _rhs ) {
                rule_exec_mgr_  = _rhs.rule_exec_mgr_;
                instance_name_  = _rhs.instance_name_;
                operation_name_ = _rhs.operation_name_;
                operation_      = _rhs.operation_;
                return *this;
            } // operator=

            // =-=-=-=-=-=-=-
            // operator call that will git 'er done. this clearly must match the
            // plugin_operation type signature
            // NOTE :: we are taking the old, long route to handle multiple template
            //         params.  when we can use C++11 this will be MUCH cleaner.

            // =-=-=-=-=-=-=-
            // public - single parameter template, there will be more...
            error call(
                plugin_context& _ctx,
                types_t...      _t) {
                if ( operation_ ) {
                    // =-=-=-=-=-=-=-
                    // get vars from fco
                    keyValPair_t kvp;
                    bzero( &kvp, sizeof( kvp ) );
                    _ctx.fco()->get_re_vars( kvp );

                    // =-=-=-=-=-=-=-
                    // call the pre-rule for this op
                    std::string pre_results;
                    error ret = rule_exec_mgr_->exec_pre_op( _ctx.comm(), kvp, pre_results );
                    if ( !ret.ok() && ret.code() != SYS_RULE_NOT_FOUND ) {
                        return PASS( ret );
                    }

                    // =-=-=-=-=-=-=-
                    // call the actual operation
                    _ctx.rule_results( pre_results );
                    error op_err = operation_( _ctx, std::forward<types_t>(_t)... );

                    // =-=-=-=-=-=-=-
                    // if the op failed, notify the post rule
                    if ( !op_err.ok() ) {
                        _ctx.rule_results( OP_FAILED );
                    }

                    // =-=-=-=-=-=-=-
                    // call the post-rule for this op
                    std::string rule_results =  _ctx.rule_results();
                    rule_exec_mgr_->exec_post_op( _ctx.comm(), kvp, rule_results );

                    // =-=-=-=-=-=-=-
                    // clean up kvp struct
                    clearKeyVal( &kvp );

                    return op_err;

                }
                else {
                    return ERROR( NULL_VALUE_ERR,  "null resource operation" );
                }

            } // operator()

        private:
            /// @brief rule execution context
            oper_rule_exec_mgr_ptr rule_exec_mgr_;
            /// =-=-=-=-=-=-=-
            /// @brief instance name used for calling rules
            std::string instance_name_;
            /// =-=-=-=-=-=-=-
            /// @brief name of this operation, use for calling rules
            std::string operation_name_;
            /// =-=-=-=-=-=-=-=-=-=-=-=-=-=-
            /// @brief function pointer to actual operation
            std::function<error(plugin_context&,types_t...)> operation_;

    }; // class operation_wrapper

    template<>
    class operation_wrapper<void> {
        public:
            // =-=-=-=-=-=-=-
            /// @brief constructors
            operation_wrapper( ) : operation_( 0 ) {
            } // default ctor

            // =-=-=-=-=-=-=-
            // public - ctor with opreation
            operation_wrapper(
                    oper_rule_exec_mgr_ptr _rule_exec,
                    const std::string&     _inst_name,
                    const std::string&     _op_name,
                    std::function<error(plugin_context&)> _op ) :
                rule_exec_mgr_( _rule_exec ),
                instance_name_( _inst_name ),
                operation_name_( _op_name ),
                operation_( _op ) {
                } // ctor

            // =-=-=-=-=-=-=-
            /// @brief destructor
            virtual ~operation_wrapper( ) {
            } // dtor

    
            // =-=-=-=-=-=-=-
            /// @brief copy constructor - necessary for stl containers
            operation_wrapper(
                    const operation_wrapper& _rhs ) {
                rule_exec_mgr_  = _rhs.rule_exec_mgr_;
                operation_      = _rhs.operation_;
                instance_name_  = _rhs.instance_name_;
                operation_name_ = _rhs.operation_name_;
            } // cctor

            // =-=-=-=-=-=-=-
            /// @brief assignment operator - necessary for stl containers
            operation_wrapper& operator=(
                    const operation_wrapper& _rhs ) {
                rule_exec_mgr_  = _rhs.rule_exec_mgr_;
                instance_name_  = _rhs.instance_name_;
                operation_name_ = _rhs.operation_name_;
                operation_      = _rhs.operation_;
                return *this;
            } // operator=

            // =-=-=-=-=-=-=-
            // operator call that will git 'er done. this clearly must match the
            // plugin_operation type signature
            // NOTE :: we are taking the old, long route to handle multiple template
            //         params.  when we can use C++11 this will be MUCH cleaner.

            // =-=-=-=-=-=-=-
            // public - single parameter template, there will be more...
            error call(
                plugin_context& _ctx ) {
                if ( operation_ ) {
                    // =-=-=-=-=-=-=-
                    // get vars from fco
                    keyValPair_t kvp;
                    bzero( &kvp, sizeof( kvp ) );
                    _ctx.fco()->get_re_vars( kvp );

                    // =-=-=-=-=-=-=-
                    // call the pre-rule for this op
                    std::string pre_results;
                    error ret = rule_exec_mgr_->exec_pre_op( _ctx.comm(), kvp, pre_results );
                    if ( !ret.ok() && ret.code() != SYS_RULE_NOT_FOUND ) {
                        return PASS( ret );
                    }

                    // =-=-=-=-=-=-=-
                    // call the actual operation
                    _ctx.rule_results( pre_results );
                    error op_err = operation_( _ctx );

                    // =-=-=-=-=-=-=-
                    // if the op failed, notify the post rule
                    if ( !op_err.ok() ) {
                        _ctx.rule_results( OP_FAILED );
                    }

                    // =-=-=-=-=-=-=-
                    // call the post-rule for this op
                    std::string rule_results =  _ctx.rule_results();
                    rule_exec_mgr_->exec_post_op( _ctx.comm(), kvp, rule_results );

                    // =-=-=-=-=-=-=-
                    // clean up kvp struct
                    clearKeyVal( &kvp );

                    return op_err;

                }
                else {
                    return ERROR( NULL_VALUE_ERR,  "null resource operation" );
                }

            } // operator()

        private:
            /// @brief rule execution context
            oper_rule_exec_mgr_ptr rule_exec_mgr_;
            /// =-=-=-=-=-=-=-
            /// @brief instance name used for calling rules
            std::string instance_name_;
            /// =-=-=-=-=-=-=-
            /// @brief name of this operation, use for calling rules
            std::string operation_name_;
            /// =-=-=-=-=-=-=-=-=-=-=-=-=-=-
            /// @brief function pointer to actual operation
            std::function<error(plugin_context&)> operation_;
            ///

    }; // class operation_wrapper

}; // namespace irods

#endif // __IRODS_OPERATION_WRAPPER_HPP__



