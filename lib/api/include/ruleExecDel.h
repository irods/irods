#ifndef IRODS_RULE_EXEC_DEL_H
#define IRODS_RULE_EXEC_DEL_H

#include "rodsDef.h"

struct RcComm;

typedef struct RuleExecDeleteInput {
    char ruleExecId[NAME_LEN];
} ruleExecDelInp_t;

#define RULE_EXEC_DEL_INP_PI "str ruleExecId[NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcRuleExecDel(struct RcComm* conn, ruleExecDelInp_t* ruleExecDelInp);

#endif // IRODS_RULE_EXEC_DEL_H
