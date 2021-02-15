#include "rsExecRuleExpression.hpp"

#include "irods_re_plugin.hpp"
#include "irods_re_structs.hpp"
#include "miscServerFunct.hpp"
#include "rcMisc.h"
#include "packStruct.h"
#include "stringOpr.h"

#include <cstring>
#include <string>

extern const packInstruct_t RodsPackTable[];

int rsExecRuleExpression(RsComm* _comm, ExecRuleExpression* _exec_rule)
{
    if (!_comm || !_exec_rule) {
        return SYS_INVALID_INPUT_PARAM;
    }

    ruleExecInfoAndArg_t* rei_and_arg = nullptr;
    const auto status = unpack_struct(_exec_rule->packed_rei_.buf,
                                     reinterpret_cast<void **>(&rei_and_arg),
                                     "ReiAndArg_PI",
                                     RodsPackTable,
                                     NATIVE_PROT,
                                     nullptr);
    if (status < 0) {
        rodsLog(LOG_ERROR, "%s :: unpackStruct error. status = %d", __FUNCTION__, status);
        return status;
    }

    ruleExecInfo_t* rei = rei_and_arg->rei;
    rei->rsComm = _comm;
    rei->rsComm->clientUser = *rei->uoic;

    // Do dataObjectInfo (doi) things?
    if (rei->doi) {
        if (rei->doi->next) {
            free(rei->doi->next);
            rei->doi->next = nullptr;
        }
    }

    std::string instance_name;
    if (is_non_empty_string(rei->pluginInstanceName, sizeof(RuleExecInfo::pluginInstanceName))) {
        instance_name = rei->pluginInstanceName;
    }

    irods::rule_engine_context_manager<irods::unit, ruleExecInfo_t*, irods::AUDIT_RULE>
        re_ctx_mgr(irods::re_plugin_globals->global_re_mgr, rei);

    const auto* my_rule_text = static_cast<char*>(_exec_rule->rule_text_.buf);
    const auto err = re_ctx_mgr.exec_rule_expression(instance_name, my_rule_text, _exec_rule->params_);
    if (!err.ok()) {
        rodsLog(LOG_ERROR, "%s : %d, %s", __FUNCTION__, err.code(), err.result().c_str());
    }

    return err.code();
} // rsExecRuleExpression

