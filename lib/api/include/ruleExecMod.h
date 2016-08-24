#ifndef RULE_EXEC_MOD_H__
#define RULE_EXEC_MOD_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "objInfo.h"

typedef struct {
    char ruleId[NAME_LEN];
    keyValPair_t condInput;
} ruleExecModInp_t;
#define RULE_EXEC_MOD_INP_PI "str ruleId[NAME_LEN];struct KeyValPair_PI;"


#ifdef __cplusplus
extern "C"
#endif
int rcRuleExecMod( rcComm_t *conn, ruleExecModInp_t *ruleExecModInp );

#endif
