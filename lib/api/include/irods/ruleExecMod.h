#ifndef RULE_EXEC_MOD_H__
#define RULE_EXEC_MOD_H__

#include "irods/rodsDef.h"
#include "irods/objInfo.h"

struct RcComm;

typedef struct RuleExecModifyInput {
    char ruleId[NAME_LEN];
    keyValPair_t condInput;
} ruleExecModInp_t;

#define RULE_EXEC_MOD_INP_PI "str ruleId[NAME_LEN];struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C"
#endif
int rcRuleExecMod(struct RcComm* _comm, struct RuleExecModifyInput* _ruleExecModInp);

#endif // RULE_EXEC_MOD_H__
