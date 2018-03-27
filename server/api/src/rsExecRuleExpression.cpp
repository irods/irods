#include "irods_re_plugin.hpp"
#include "miscServerFunct.hpp"
#include "rcMisc.h"
#include "packStruct.h"
#include "rsExecRuleExpression.hpp"

#include <string>

extern const packInstruct_t RodsPackTable[];

int rsExecRuleExpression(
    rsComm_t*               _comm,
    exec_rule_expression_t* _exec_rule ) {
    if( !_comm || !_exec_rule ) {
        return SYS_INVALID_INPUT_PARAM;
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
    rei->rsComm->clientUser = *rei->uoic;

    // do doi things?
    if ( rei->doi != NULL ) {
        if ( rei->doi->next != NULL ) {
            free( rei->doi->next );
            rei->doi->next = NULL;
        }
    }

    std::string instance_name;
    if(strlen(rei->pluginInstanceName) > 0) {
        instance_name = rei->pluginInstanceName;
    }
    else {
        irods::error ret = get_default_rule_plugin_instance( instance_name );
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }
    }

    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               rei);

    std::string instance_name_string{instance_name};
    irods::error err = re_ctx_mgr.exec_rule_expression(
                           instance_name_string,
                           my_rule_text,
                           _exec_rule->params_);
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
