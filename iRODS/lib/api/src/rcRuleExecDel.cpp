/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See rcRuleExecDel.h for a description of this API call.*/

#include "ruleExecDel.h"

int
rcRuleExecDel (rcComm_t *conn, ruleExecDelInp_t *ruleExecDelInp)
{
    int status;
    status = procApiRequest (conn, RULE_EXEC_DEL_AN, ruleExecDelInp, 
     NULL, (void **) NULL, NULL);

    return (status);
}

