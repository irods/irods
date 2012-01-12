/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See modAccessControl.h for a description of this API call.*/

#include "modAccessControl.h"
#include "specColl.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsModAccessControl (rsComm_t *rsComm, modAccessControlInp_t *modAccessControlInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;
    specCollCache_t *specCollCache = NULL;
    char newPath[MAX_NAME_LEN];

    rstrcpy (newPath, modAccessControlInp->path, MAX_NAME_LEN);
    resolveLinkedPath (rsComm, newPath, &specCollCache, NULL);
    if (strcmp (newPath, modAccessControlInp->path) != 0) {
	free (modAccessControlInp->path);
	modAccessControlInp->path = strdup (newPath);
    }

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, 
      modAccessControlInp->path, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsModAccessControl (rsComm, modAccessControlInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcModAccessControl(rodsServerHost->conn,
			       modAccessControlInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsModAccessControl: rcModAccessControl failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsModAccessControl (rsComm_t *rsComm, 
		     modAccessControlInp_t *modAccessControlInp )
{
    int status;

    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;
    char rFlag[15];
    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }


    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    sprintf(rFlag,"%d",modAccessControlInp->recursiveFlag);
    args[0] = rFlag;
    args[1] = modAccessControlInp->accessLevel;
    args[2] = modAccessControlInp->userName;
    args[3] = modAccessControlInp->zone;
    args[4] = modAccessControlInp->path;
    argc = 5;
    i =  applyRuleArg("acPreProcForModifyAccessControl",args,argc, &rei2, NO_SAVE_REI);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "rsModAVUMetadata:acPreProcForModifyAccessControl error for %s.%s of level %s for %s,stat=%d",
	       modAccessControlInp->zone, modAccessControlInp->userName, modAccessControlInp->accessLevel, modAccessControlInp->path, i);
      return i;
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/


    status = chlModAccessControl(rsComm, 
				 modAccessControlInp->recursiveFlag,
				 modAccessControlInp->accessLevel,
				 modAccessControlInp->userName,
				 modAccessControlInp->zone,
				 modAccessControlInp->path );

    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    i =  applyRuleArg("acPostProcForModifyAccessControl",args,argc, &rei2, NO_SAVE_REI);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "rsModAVUMetadata:acPostProcForModifyAccessControl error for %s.%s of level %s for %s,stat=%d",
               modAccessControlInp->zone, modAccessControlInp->userName, modAccessControlInp->accessLevel, modAccessControlInp->path, i);
      return i;
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    return(status);
} 
#endif
