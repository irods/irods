#ifndef RS_RULE_EXEC_SUBMIT_HPP
#define RS_RULE_EXEC_SUBMIT_HPP

#include "ruleExecSubmit.h"

int rsRuleExecSubmit( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp, char **ruleExecId );
int _rsRuleExecSubmit( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp );

#endif
