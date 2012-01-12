/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* execMyRule.h
 * Execute my rule
 */

#ifndef EXEC_MY_RULE_H
#define EXEC_MY_RULE_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "dataObjInpOut.h"
#include "msParam.h"

typedef struct {
    char myRule[META_STR_LEN];
    rodsHostAddr_t addr;
    keyValPair_t condInput;
    char outParamDesc[LONG_NAME_LEN];  /* output labels separated by "%" */  
    msParamArray_t *inpParamArray;
} execMyRuleInp_t;

#define ExecMyRuleInp_PI "str myRule[META_STR_LEN]; struct RHostAddr_PI; struct KeyValPair_PI; str outParamDesc[LONG_NAME_LEN]; struct *MsParamArray_PI;"

#if defined(RODS_SERVER)
#define RS_EXEC_MY_RULE rsExecMyRule
/* prototype for the server handler */
int
rsExecMyRule (rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp, 
msParamArray_t **outParamArray);
int
remoteExecMyRule (rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
msParamArray_t **outParamArray, rodsServerHost_t *rodsServerHost);
#else
#define RS_EXEC_MY_RULE NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcExecMyRule (rcComm_t *conn, execMyRuleInp_t *execMyRuleInp, 
msParamArray_t **outParamArray);

#ifdef  __cplusplus
}
#endif

#endif	/* EXEC_MY_RULE_H */
