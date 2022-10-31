#ifndef IRODS_RS_EXEC_MY_RULE_HPP
#define IRODS_RS_EXEC_MY_RULE_HPP

struct RsComm;
struct ExecMyRuleInp;
struct MsParamArray;
struct rodsServerHost;

auto rsExecMyRule(RsComm* _comm, ExecMyRuleInp* _exec_inp, MsParamArray** _out_param_arr) -> int;

auto remoteExecMyRule(
    RsComm* _comm,
    ExecMyRuleInp* _exec_inp,
    MsParamArray** _out_param_arr,
    rodsServerHost* _remote_host) -> int;

#endif // IRODS_RS_EXEC_MY_RULE_HPP

