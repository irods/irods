#ifndef RS_RULE_EXEC_SUBMIT_HPP
#define RS_RULE_EXEC_SUBMIT_HPP

#include "irods/ruleExecSubmit.h"

struct RsComm;

int rsRuleExecSubmit(RsComm* rsComm, ruleExecSubmitInp_t* ruleExecSubmitInp, char** ruleExecId);

#endif // RS_RULE_EXEC_SUBMIT_HPP

