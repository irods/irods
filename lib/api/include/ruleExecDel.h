#ifndef RULE_EXEC_DEL_H__
#define RULE_EXEC_DEL_H__

#include "rcConnect.h"
#include "rodsDef.h"

typedef struct {
    char ruleExecId[NAME_LEN];
} ruleExecDelInp_t;

#define RULE_EXEC_DEL_INP_PI "str ruleExecId[NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcRuleExecDel( rcComm_t *conn, ruleExecDelInp_t *ruleExecDelInp );

#endif
