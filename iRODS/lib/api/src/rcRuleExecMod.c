/* This is script-generated code.  */ 
/* See ruleExecMod.h for a description of this API call.*/

#include "ruleExecMod.h"

int
rcRuleExecMod (rcComm_t *conn, ruleExecModInp_t *ruleExecModInp)
{
    int status;
    status = procApiRequest (conn, RULE_EXEC_MOD_AN, ruleExecModInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
