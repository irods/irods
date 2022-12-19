#ifndef IRODS_RS_RULE_EXEC_DEL_HPP
#define IRODS_RS_RULE_EXEC_DEL_HPP

#include "ruleExecDel.h"

struct RsComm;

int rsRuleExecDel(RsComm* rsComm, ruleExecDelInp_t* ruleExecDelInp);

#endif // IRODS_RS_RULE_EXEC_DEL_HPP
