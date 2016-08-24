#ifndef EXEC_MY_RULE_H__
#define EXEC_MY_RULE_H__

#include "rodsDef.h"
#include "objInfo.h"
#include "msParam.h"
#include "rcConnect.h"

typedef struct ExecMyRuleInp {
    char myRule[META_STR_LEN];
    rodsHostAddr_t addr;
    keyValPair_t condInput;
    char outParamDesc[LONG_NAME_LEN];  // output labels separated by "%"
    msParamArray_t *inpParamArray;
} execMyRuleInp_t;
#define ExecMyRuleInp_PI "str myRule[META_STR_LEN]; struct RHostAddr_PI; struct KeyValPair_PI; str outParamDesc[LONG_NAME_LEN]; struct *MsParamArray_PI;"


#ifdef __cplusplus
extern "C"
#endif
int rcExecMyRule( rcComm_t *conn, execMyRuleInp_t *execMyRuleInp, msParamArray_t **outParamArray );

#endif
