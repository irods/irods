/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See rcRuleExecSubmit.h for a description of this API call.*/

#include "ruleExecSubmit.h"

int
rcRuleExecSubmit (rcComm_t *conn, ruleExecSubmitInp_t *ruleExecSubmitInp,
char **ruleExecId)
{
    int status;
    status = procApiRequest (conn, RULE_EXEC_SUBMIT_AN, ruleExecSubmitInp, 
     NULL, (void **) ruleExecId, NULL);

    return (status);
}

