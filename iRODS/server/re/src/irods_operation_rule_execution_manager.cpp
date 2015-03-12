// =-=-=-=-=-=-=-
// irods includes
#include "index.hpp"
#include "reFuncDefs.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_operation_rule_execution_manager.hpp"

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
        // determine if rule exists
        RuleIndexListNode* re_node = 0;
        if ( findNextRule2( const_cast<char*>( _name.c_str() ), 0, &re_node ) < 0 ) {
            return ERROR( SYS_RULE_NOT_FOUND, "no rule found" );
        }

        // =-=-=-=-=-=-=-
        // debug message for creating dynPEP rules
        rodsLog(
            LOG_DEBUG,
            "operation_rule_execution_manager exec_op [%s]",
            _name.c_str() );

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

        // =-=-=-=-=-=-=-
        // add the output parameter
        msParamArray_t params;
        memset( &params, 0, sizeof( msParamArray_t ) );
        char out_param[ MAX_NAME_LEN ] = {"EMPTY_PARAM"};
        if ( _res.empty() ) {
            addMsParamToArray( &params, "*OUT", STR_MS_T, out_param, NULL, 0 );
        }
        else {
            addMsParamToArray( &params, "*OUT", STR_MS_T, const_cast<char*>( _res.c_str() ), NULL, 0 );
        }

        // =-=-=-=-=-=-=-
        // rule exists, param array is build.  call the rule.
        std::string arg_name = _name + "(*OUT)";
        int ret = applyRuleUpdateParams(
                      const_cast<char*>( arg_name.c_str() ),
                      &params,
                      &rei,
                      NO_SAVE_REI );
        if ( 0 != ret ) {
            return ERROR( ret, "failed in call to applyRuleUpdateParams" );
        }

        // =-=-=-=-=-=-=-
        // extract the value from the outgoing param to pass out to the operation
        msParam_t* out_ms_param = getMsParamByLabel( &params, "*OUT" );
        if ( out_ms_param ) {
            _res = reinterpret_cast< char* >( out_ms_param->inOutStruct );

        }
        else {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null out parameter" );

        }

        return SUCCESS();

    } // exec_op

}; // namespace irods



