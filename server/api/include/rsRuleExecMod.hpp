#ifndef RS_RULE_EXEC_MOD_HPP
#define RS_RULE_EXEC_MOD_HPP

#include "rcConnect.h"
#include "ruleExecMod.h"

struct RsComm;

int rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp);
int _rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp);

#endif
