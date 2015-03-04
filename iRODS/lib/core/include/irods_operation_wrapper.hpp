#ifdef IRODS_RE_PLUGIN_HPP

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_context.hpp"
#include "irods_error.hpp"
#include "irods_operation_rule_execution_manager.hpp"
#include "irods_operation_rule_execution_manager_no_op.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "reGlobalsExtern.hpp"
#include "dataObjInpOut.hpp"
#else
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

           template< typename... T1 >
            error call(
                plugin_context& _ctx,
                T1            ... _t1 ) {
                if ( operation_ ) {
                    // =-=-=-=-=-=-=-
                    // get vars from fco
                    keyValPair_t kvp;
                    bzero( &kvp, sizeof( kvp ) );
                    _ctx.fco()->get_re_vars( kvp );
                    
                    ruleExecInfo_t rei;
                    memset( ( char* )&rei, 0, sizeof( ruleExecInfo_t ) );
                    // rei.rsComm        = _comm;
                    rei.condInputData = &kvp; // give rule scope to our key value pairs
        

                    // =-=-=-=-=-=-=-
                    // call the pep-rule for this op
                    dynamic_operation_execution_manager<default_re_ctx, default_ms_ctx, DONT_AUDIT_RULE> rex_mgr (std::shared_ptr<rule_engine_context_manager<default_re_ctx, default_ms_ctx, DONT_AUDIT_RULE> > (new rule_engine_context_manager<default_re_ctx, default_ms_ctx, DONT_AUDIT_RULE >(global_re_mgr, &rei)));
                    error op_err = rex_mgr.call(instance_name_, operation_name_, std::function<error(plugin_context&, T1...)>(operation_), _ctx, _t1...);

                    // =-=-=-=-=-=-=-
                    // clean up kvp struct
                    clearKeyVal( &kvp );

                    return op_err;

                }
                else {
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1

        private:
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



#endif
