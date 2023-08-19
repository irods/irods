#include "irods/rsExecMyRule.hpp"

#include "irods/execMyRule.h"
#include "irods/irods_logger.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/msParam.h"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"

#include <cstdlib>
#include <string>
#include <vector>

using log_api = irods::experimental::log::api;

auto rsExecMyRule(RsComm* _comm, ExecMyRuleInp* _exec_inp, MsParamArray** _out_param_arr) -> int
{
    if (!_exec_inp) { // NOLINT(readability-implicit-bool-conversion)
        log_api::error("Invalid input: null pointer");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (getValByKey(&_exec_inp->condInput, AVAILABLE_KW)) { // NOLINT(readability-implicit-bool-conversion)
        std::vector<std::string> instance_names;
        irods::error ret = list_rule_plugin_instances(instance_names);

        if (!ret.ok()) {
            log_api::error(ret.result());
            return ret.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
        }

        std::string ret_string = "Available rule engine plugin instances:\n";
        for (const auto& name : instance_names) {
            ret_string += "\t";
            ret_string += name;
            ret_string += "\n";
        }

        const auto ec = addRErrorMsg(&_comm->rError, 0, ret_string.c_str());
        if (ec != 0) {
            log_api::error("Failed to add message to rError stack.");
        }

        return ec;
    }

    rodsServerHost_t* remote_host = nullptr;
    const int remoteFlag = resolveHost(&_exec_inp->addr, &remote_host);
    if (remoteFlag < 0) {
        const auto msg = fmt::format(fmt::runtime("Failed to resolve hostname: [{}]"), _exec_inp->addr.hostAddr);
        log_api::error(msg);
        addRErrorMsg(&_comm->rError, remoteFlag, msg.data());
        return remoteFlag;
    }

    if (remoteFlag == REMOTE_HOST) {
        return remoteExecMyRule(_comm, _exec_inp, _out_param_arr, remote_host);
    }

    const char* inst_name_str = getValByKey(&_exec_inp->condInput, irods::KW_CFG_INSTANCE_NAME);
    std::string inst_name;
    if (inst_name_str) { // NOLINT(readability-implicit-bool-conversion)
        inst_name = inst_name_str;
    }

    // Construct and initialize an REI object.
    ruleExecInfo_t rei{};
    rei.rsComm = _comm;
    rei.condInputData = &_exec_inp->condInput;
    if (_comm) { // NOLINT(readability-implicit-bool-conversion)
        rei.uoic = &_comm->clientUser;
        rei.uoip = &_comm->proxyUser;
    }

    // Need to have a valid MsParamArray for execMyRule to work.
    if (!_exec_inp->inpParamArray) { // NOLINT(readability-implicit-bool-conversion)
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        _exec_inp->inpParamArray = static_cast<MsParamArray*>(std::malloc(sizeof(MsParamArray)));
        std::memset(_exec_inp->inpParamArray, 0, sizeof(MsParamArray));
    }

    // This exposes the MsParamArray provided by the client (or recently allocated) to the NREP.
    rei.msParamArray = _exec_inp->inpParamArray;

    rstrcpy(rei.ruleName, EXEC_MY_RULE_KW, NAME_LEN); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    const std::string my_rule_text = _exec_inp->myRule;
    const std::string out_param_desc = _exec_inp->outParamDesc;
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    irods::rule_engine_context_manager<irods::unit, RuleExecInfo*, irods::AUDIT_RULE> re_ctx_mgr(
        irods::re_plugin_globals->global_re_mgr, &rei);
    irods::error err = re_ctx_mgr.exec_rule_text(inst_name, my_rule_text, _exec_inp->inpParamArray, out_param_desc);

    // If the client didn't specify a target REP, clear all error information.
    // Doing this maintains the behavior seen by executing "irule" without specifying a target REP.
    // This is further explained due to the fact that "irule" invokes this API endpoint directly to do its work.
    if (inst_name.empty()) {
        freeRErrorContent(&rei.rsComm->rError);
    }
    else if (!err.ok()) {
        log_api::error("{}: {}, {}", __func__, err.code(), err.result());
        return err.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    trimMsParamArray(rei.msParamArray, _exec_inp->outParamDesc);

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    *_out_param_arr = static_cast<MsParamArray*>(std::malloc(sizeof(MsParamArray)));
    std::memset(*_out_param_arr, 0, sizeof(MsParamArray));
    replMsParamArray(rei.msParamArray, *_out_param_arr);

    // Detach the MsParamArray from the REI.
    rei.msParamArray = nullptr;

    if (!inst_name.empty() && err.code() < 0) {
        log_api::error("{}: Rule execution error for [{}], error code = [{}]", __func__, _exec_inp->myRule, err.code());
        return err.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }

    return err.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
} // rsExecMyRule

auto remoteExecMyRule(RsComm* _comm,
                      ExecMyRuleInp* _exec_inp,
                      MsParamArray** _out_param_arr,
                      rodsServerHost* _remote_host) -> int
{
    if (!_remote_host) { // NOLINT(readability-implicit-bool-conversion)
        log_api::error("{}: Invalid server host.", __func__);
        return SYS_INVALID_SERVER_HOST;
    }

    if (const auto ec = svrToSvrConnect(_comm, _remote_host); ec < 0) {
        return ec;
    }

    return rcExecMyRule(_remote_host->conn, _exec_inp, _out_param_arr);
} // remoteExecMyRule
