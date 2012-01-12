/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See ruleExecMod.h for a description of this API call.*/

#include "ruleExecMod.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsRuleExecMod (rsComm_t *rsComm, ruleExecModInp_t *ruleExecModInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsRuleExecMod (rsComm, ruleExecModInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcRuleExecMod(rodsServerHost->conn,
			       ruleExecModInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsRuleExecMod: rcRuleExecMod failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsRuleExecMod (rsComm_t *rsComm, 
		     ruleExecModInp_t *ruleExecModInp )
{
    int status;

    status = chlModRuleExec(rsComm, 
			    ruleExecModInp->ruleId,
			    &ruleExecModInp->condInput );
    return(status);
} 
#endif
