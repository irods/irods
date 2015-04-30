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
#include "irods_plugin_base.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_context.hpp"
#include "irods_error.hpp"
#include "irods_operation_rule_execution_manager.hpp"
#include "irods_operation_rule_execution_manager_no_op.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "reGlobalsExtern.hpp"
#include "dataObjInpOut.h"

#include <iostream>

namespace irods {
// =-=-=-=-=-=-=-
// const signifying failure of operation call for post dynPEP
    static std::string OP_FAILED( "OPERATION_FAILED" );

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class operation_wrapper {
        public:
            // =-=-=-=-=-=-=-
            /// @brief constructors
            operation_wrapper( );
            operation_wrapper(
                oper_rule_exec_mgr_ptr, // rule exec mgr
                const std::string&,     // instance name
                const std::string&,     // operation name
                plugin_operation );     // fcn ptr to loaded operation

            // =-=-=-=-=-=-=-
            /// @brief destructor
            virtual ~operation_wrapper();

            // =-=-=-=-=-=-=-
            /// @brief copy constructor - necessary for stl containers
            operation_wrapper( const operation_wrapper& _rhs );

            // =-=-=-=-=-=-=-
            /// @brief assignment operator - necessary for stl containers
            operation_wrapper& operator=( const operation_wrapper& _rhs );

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
                    error op_err = ( *operation_ )( _ctx );

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

            } // operator() - T1


            // =-=-=-=-=-=-=-
            // public - single parameter template, there will be more...
            template< typename T1 >
            error call(
                plugin_context& _ctx,
                T1              _t1 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1

            // =-=-=-=-=-=-=-
            // public - two parameter template, there will be more...
            template< typename T1, typename T2 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2

            // =-=-=-=-=-=-=-
            // public - three parameter template, there will be more...
            template< typename T1, typename T2, typename T3 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3

            // =-=-=-=-=-=-=-
            // public - four parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4 ) {
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
                    error op_err =  operation_( _ctx, _t1, _t2, _t3, _t4 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4

            // =-=-=-=-=-=-=-
            // public - five parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5

            // =-=-=-=-=-=-=-
            // public - six parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5, T6

            // =-=-=-=-=-=-=-
            // public - seven parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6,
                T7              _t7 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );

                }

            } // operator() - T1, T2, T3, T4, T5, T6, T7

            // =-=-=-=-=-=-=-
            // public - eight parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6,
                T7              _t7,
                T8              _t8 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5, T6, T7, T8

            // =-=-=-=-=-=-=-
            // public - nine parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6,
                T7              _t7,
                T8              _t8,
                T9              _t9 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5, T6, T7, T8, T9

            // =-=-=-=-=-=-=-
            // public - nine parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6,
                T7              _t7,
                T8              _t8,
                T9              _t9,
                T10             _t10 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5, T6, T7, T8, T9, T10

            // =-=-=-=-=-=-=-
            // public - 11 parameter template, there will be more...
            template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11 >
            error call(
                plugin_context& _ctx,
                T1              _t1,
                T2              _t2,
                T3              _t3,
                T4              _t4,
                T5              _t5,
                T6              _t6,
                T7              _t7,
                T8              _t8,
                T9              _t9,
                T10             _t10,
                T11             _t11 ) {
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
                    error op_err = ( *operation_ )( _ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10, _t11 );

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
                    return ERROR( NULL_VALUE_ERR, "null resource operation." );
                }

            } // operator() - T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11

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
            plugin_operation operation_;
            ///

    }; // class operation_wrapper

}; // namespace irods

#endif // __IRODS_OPERATION_WRAPPER_HPP__



