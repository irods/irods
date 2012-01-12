/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See modAVUMetadata.h for a description of this API call.*/

#include "modAVUMetadata.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsModAVUMetadata (rsComm_t *rsComm, modAVUMetadataInp_t *modAVUMetadataInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;
    char *myHint;

    if (strcmp(modAVUMetadataInp->arg0,"add")==0) {
	myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"adda")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"addw")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"rmw")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"rmi")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"rm")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else if (strcmp(modAVUMetadataInp->arg0,"cp")==0) {
        myHint = modAVUMetadataInp->arg3;
    } else if (strcmp(modAVUMetadataInp->arg0,"mod")==0) {
        myHint = modAVUMetadataInp->arg2;
    } else {
	/* assume local */
	myHint = NULL;
    }
 
    status = getAndConnRcatHost(rsComm, MASTER_RCAT, myHint, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsModAVUMetadata (rsComm, modAVUMetadataInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcModAVUMetadata(rodsServerHost->conn,
			       modAVUMetadataInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsModAVUMetadata: rcModAVUMetadata failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsModAVUMetadata (rsComm_t *rsComm, modAVUMetadataInp_t *modAVUMetadataInp )
{
    int status;

    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }



    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    args[0] = modAVUMetadataInp->arg0; /* option add, adda, rm, rmw, rmi, cp,
					  or mod */
    args[1] = modAVUMetadataInp->arg1; /* item type -d,-d,-c,-C,-r,-R,-u,-U */
    args[2] = modAVUMetadataInp->arg2; /* item name */
    args[3] = modAVUMetadataInp->arg3; /* attr name */
    args[4] = modAVUMetadataInp->arg4; /* attr val */
    args[5] = modAVUMetadataInp->arg5; /* attr unit */
    if (args[5]) {
    	argc = 6;
    } else {
    	argc = 5;
    }
    i =  applyRuleArg("acPreProcForModifyAVUMetadata",args,argc, &rei2, NO_SAVE_REI);
    if (i < 0) {
      if (rei2.status < 0) {
	i = rei2.status;
      }
      rodsLog (LOG_ERROR,
	       "rsModAVUMetadata:acPreProcForModifyAVUMetadata error for %s of type %s and option %s,stat=%d",
	       modAVUMetadataInp->arg2,modAVUMetadataInp->arg1,modAVUMetadataInp->arg0, i);
      return i;
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/


    if (strcmp(modAVUMetadataInp->arg0,"add")==0) {
       status = chlAddAVUMetadata(rsComm, 0,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"adda")==0) {
       status = chlAddAVUMetadata(rsComm, 1,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"addw")==0) {
       status = chlAddAVUMetadataWild(rsComm, 0,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"rmw")==0) {
       status = chlDeleteAVUMetadata(rsComm, 1,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5,
				  0);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"rmi")==0) {
       status = chlDeleteAVUMetadata(rsComm, 2,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5,
				  0);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"rm")==0) {
       status = chlDeleteAVUMetadata(rsComm, 0,
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
			          modAVUMetadataInp->arg5, 
				  0);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"cp")==0) {
       status = chlCopyAVUMetadata(rsComm, 
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4);
    }
    else if (strcmp(modAVUMetadataInp->arg0,"mod")==0) {
       status = chlModAVUMetadata(rsComm, 
				  modAVUMetadataInp->arg1,
				  modAVUMetadataInp->arg2,
				  modAVUMetadataInp->arg3,
				  modAVUMetadataInp->arg4,
				  modAVUMetadataInp->arg5,
				  modAVUMetadataInp->arg6,
				  modAVUMetadataInp->arg7,
				  modAVUMetadataInp->arg8);
    }
    else {
      return(CAT_INVALID_ARGUMENT);
    }      
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    i =  applyRuleArg("acPostProcForModifyAVUMetadata",args,argc, &rei2, NO_SAVE_REI);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "rsModAVUMetadata:acPostProcForModifyAVUMetadata error for %s of type %s and option %s,stat=%d",
               modAVUMetadataInp->arg2,modAVUMetadataInp->arg1,modAVUMetadataInp->arg0, i);
      return i;
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    return(status);

} 
#endif
