// =-=-=-=-=-=-=-
// irods includes
#include "index.hpp"
#include "reFuncDefs.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_operation_rule_execution_manager.hpp"

#include "irods_re_plugin.hpp"
namespace irods {
// =-=-=-=-=-=-=-
// public - Constructor
    operation_rule_execution_manager::operation_rule_execution_manager(
        const std::string& _instance,
        const std::string& _op_name ) :
        operation_rule_execution_manager_base(
            _instance,
            _op_name ) {
        rule_name_ = "pep_" + op_name_;

    } // ctor

// =-=-=-=-=-=-=-
// public - execute rule for pre operation
    error operation_rule_execution_manager::exec_pre_op(
        rsComm_t*     _comm,
        keyValPair_t& _kvp,
        std::string&  _res ) {
        // =-=-=-=-=-=-=-
        // manufacture pre rule name
        std::string pre_name = rule_name_ + "_pre";

        // =-=-=-=-=-=-=-
        // execute the rule
        return exec_op( _comm, _kvp, pre_name, _res );

    } // exec_pre_op

// =-=-=-=-=-=-=-
// public - execute rule for post operation
    error operation_rule_execution_manager::exec_post_op(
        rsComm_t*     _comm,
        keyValPair_t& _kvp,
        std::string&  _res ) {
        // =-=-=-=-=-=-=-
        // manufacture pre rule name
        std::string post_name = rule_name_ + "_post";

        // =-=-=-=-=-=-=-
        // execute the rule
        return exec_op( _comm, _kvp, post_name, _res );

    } // exec_post_op

// =-=-=-=-=-=-=-
// private - execute rule for pre operation
    error operation_rule_execution_manager::exec_op(
        rsComm_t*          _comm,
        keyValPair_t&      _kvp,
        const std::string& _name,
        std::string&       _res ) {
        
        // =-=-=-=-=-=-=-
        // add additional global re params
        error err = add_global_re_params_to_kvp_for_dynpep( _kvp );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // manufacture an rei for the applyRule
        
        ruleExecInfo_t rei;
        memset( ( char* )&rei, 0, sizeof( ruleExecInfo_t ) );
        rei.rsComm        = _comm;
        rei.condInputData = &_kvp; // give rule scope to our key value pairs
        rstrcpy( rei.pluginInstanceName, instance_.c_str(), MAX_NAME_LEN );

        rule_engine_context_manager<unit, ruleExecInfo_t*, AUDIT_RULE> re_ctx_mgr = rule_engine_context_manager<unit, ruleExecInfo_t*, AUDIT_RULE>(global_re_mgr, &rei);
        
        // =-=-=-=-=-=-=-
        // determine if rule exists
        bool ret;
        if ( !(err = re_ctx_mgr.rule_exists(_name, ret)).ok() ) {
            return err;
        } else if( !ret ) {
            return ERROR( SYS_RULE_NOT_FOUND, "no rule found" );
        }

        // =-=-=-=-=-=-=-
        // debug message for creating dynPEP rules
        rodsLog(
            LOG_DEBUG,
            "operation_rule_execution_manager exec_op [%s]",
            _name.c_str() );

        // just pass "" instead of "EMPTY_PARAM" because the return value need to be in well formed kvp format
        // the rule has to manually set it to ""
        // rule exists, call the rule.
        err = re_ctx_mgr.exec_rule(_name, &_res);
        if ( ! err.ok() ) {
            return ERROR( ret, "failed in call to exec_rule" );
        }

        return SUCCESS();

    } // exec_op

}; // namespace irods



