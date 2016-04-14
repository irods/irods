
#include "exec_rule_expression.h"
#include "irods_re_plugin.hpp"
#include "miscServerFunct.hpp"
#include "rcMisc.h"
#include "packStruct.h"

#include <string>

extern const packInstructArray_t RodsPackTable[];

int rsExecRuleExpression(
    rsComm_t*               _comm,
    exec_rule_expression_t* _exec_rule ) {
    if( !_comm || !_exec_rule ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    std::string instance_name;
    irods::error ret = get_default_rule_plugin_instance( instance_name );
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    std::string my_rule_text = (char*)_exec_rule->rule_text_.buf;

    ruleExecInfoAndArg_t* rei_and_arg = NULL;
    int status = unpackStruct(
                     _exec_rule->packed_rei_.buf,
                     ( void ** ) &rei_and_arg,
                     "ReiAndArg_PI",
                     RodsPackTable,
                     NATIVE_PROT );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "%s :: unpackStruct error. status = %d",
            __FUNCTION__,
            status );
        return status;
    }

    ruleExecInfo_t* rei = rei_and_arg->rei;
    rei->rsComm = _comm;

    // do doi things?
    if ( rei->doi != NULL ) {
        if ( rei->doi->next != NULL ) {
            free( rei->doi->next );
            rei->doi->next = NULL;
        }
    }

    std::string out_param_desc = "ruleExecOut";
    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               rei);
    irods::error err = re_ctx_mgr.exec_rule_expression(
                           my_rule_text,
                           instance_name,
                           _exec_rule->params_,
                           &out_param_desc,
                           rei);
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "%s : %d, %s",
            __FUNCTION__,
            err.code(),
            err.result().c_str()
        );
    }

    return err.code();

} // rsExecRuleExpression

