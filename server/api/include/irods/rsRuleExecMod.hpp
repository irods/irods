#ifndef RS_RULE_EXEC_MOD_HPP
#define RS_RULE_EXEC_MOD_HPP

#include "irods/rcConnect.h"
#include "irods/ruleExecMod.h"

struct RsComm;

int rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp);
int _rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp);

#endif
