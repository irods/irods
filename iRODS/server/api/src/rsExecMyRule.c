#include "reGlobalsExtern.h"
#include "execMyRule.h"
#include "miscServerFunct.h"

int
rsExecMyRule (rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
msParamArray_t **outParamArray)
{
    int status;
    ruleExecInfo_t rei;
    char *iFlag;
    int oldReTestFlag, oldReLoopBackFlag;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;

    if (execMyRuleInp == NULL) { 
       rodsLog(LOG_NOTICE,
        "rsExecMyRule error. NULL input");
       return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    remoteFlag = resolveHost (&execMyRuleInp->addr, &rodsServerHost);

    if (remoteFlag == REMOTE_HOST) {
        status = remoteExecMyRule (rsComm, execMyRuleInp, 
	  outParamArray, rodsServerHost);
	return status;
    }

    initReiWithDataObjInp (&rei, rsComm, NULL);
    rei.condInputData = &execMyRuleInp->condInput;
    /* need to have a non zero inpParamArray for execMyRule to work */
    if (execMyRuleInp->inpParamArray == NULL) {
	execMyRuleInp->inpParamArray = 
	  (msParamArray_t *) malloc (sizeof (msParamArray_t));
	memset (execMyRuleInp->inpParamArray, 0, sizeof (msParamArray_t));
    }
    rei.msParamArray = execMyRuleInp->inpParamArray;

    if ((iFlag = getValByKey (rei.condInputData,"looptest")) != NULL && 
      !strcmp(iFlag,"true")) {
        oldReTestFlag = reTestFlag;
        oldReLoopBackFlag = reLoopBackFlag;
        reTestFlag = LOG_TEST_2;
        reLoopBackFlag = LOOP_BACK_1;
    }

    rstrcpy (rei.ruleName, EXEC_MY_RULE_KW, NAME_LEN);

#if defined(RULE_ENGINE_N)
    status = execMyRule (execMyRuleInp->myRule, execMyRuleInp->inpParamArray, execMyRuleInp->outParamDesc,
      &rei);
#else
    status = execMyRule (execMyRuleInp->myRule, execMyRuleInp->inpParamArray,
      &rei);
#endif

    
    if (iFlag != NULL) {
      reTestFlag = oldReTestFlag;
      reLoopBackFlag = oldReLoopBackFlag;
    }

    trimMsParamArray (rei.msParamArray, execMyRuleInp->outParamDesc);

    *outParamArray = rei.msParamArray;
    rei.msParamArray = NULL;

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rsExecMyRule : execMyRule error for %s, status = %d",     
          execMyRuleInp->myRule, status);
        return (status);
    }

    return (status);
}

int
remoteExecMyRule (rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
msParamArray_t **outParamArray, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_ERROR,
          "remoteExecMyRule: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcExecMyRule (rodsServerHost->conn, execMyRuleInp, outParamArray);

    return (status);
}

