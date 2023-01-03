#include "irods/rsExecRuleExpression.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/packStruct.h"
#include "irods/rcMisc.h"
#include "irods/stringOpr.h"

#include <cstdlib>
#include <cstring>
#include <string>

extern const packInstruct_t RodsPackTable[];

int rsExecRuleExpression(RsComm* _comm, ExecRuleExpression* _exec_rule)
{
    using log_api = irods::experimental::log::api;

    if (!_comm || !_exec_rule) {
        log_api::error("Invalid input argument: received null pointer");
        return SYS_INVALID_INPUT_PARAM;
    }

    ruleExecInfoAndArg_t* rei_and_arg = nullptr;
    irods::at_scope_exit free_rei_struct{[&rei_and_arg] {
        if (rei_and_arg) {
            freeRuleExecInfoStruct(rei_and_arg->rei, (FREE_MS_PARAM | FREE_DOINP));
            std::free(rei_and_arg);
        }
    }};

    const auto ec = unpack_struct(_exec_rule->packed_rei_.buf,
                                  reinterpret_cast<void**>(&rei_and_arg),
                                  "ReiAndArg_PI",
                                  RodsPackTable,
                                  NATIVE_PROT,
                                  nullptr);

    if (ec < 0) {
        log_api::error("Failed to unpack input structure [error_code=[{}]]", ec);
        return ec;
    }

    ruleExecInfo_t* rei = rei_and_arg->rei;
    rei->rsComm = _comm;
    rei->rsComm->clientUser = *rei->uoic;

    // Do dataObjectInfo (doi) things?
    if (rei->doi) {
        if (rei->doi->next) {
            std::free(rei->doi->next);
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
        log_api::error(err.result());
    }

    return err.code();
} // rsExecRuleExpression

