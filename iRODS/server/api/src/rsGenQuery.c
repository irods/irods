/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "genQuery.h"
#include "icatHighLevelRoutines.h"
#include "miscUtil.h"
#include "cache.h"

/* can be used for debug: */
/* extern int printGenQI( genQueryInp_t *genQueryInp); */

int
rsGenQuery (rsComm_t *rsComm, genQueryInp_t *genQueryInp, 
genQueryOut_t **genQueryOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;
    char *zoneHint;

    zoneHint = getZoneHintForGenQuery (genQueryInp);
 
    status = getAndConnRcatHost(rsComm, SLAVE_RCAT, zoneHint,
				&rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGenQuery (rsComm, genQueryInp, genQueryOut);
#else
       rodsLog(LOG_NOTICE, 
	       "rsGenQuery error. RCAT is not configured on this host");
       return (SYS_NO_RCAT_SERVER_ERR);
#endif 
    } else {
        status = rcGenQuery(rodsServerHost->conn,
			   genQueryInp, genQueryOut);
    }
    if (status < 0  && status != CAT_NO_ROWS_FOUND) {
        rodsLog (LOG_NOTICE,
		 "rsGenQuery: rcGenQuery failed, status = %d", status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGenQuery (rsComm_t *rsComm, genQueryInp_t *genQueryInp,
	     genQueryOut_t **genQueryOut)
{
    int status;

    static int ruleExecuted=0;
    ruleExecInfo_t rei;
    static int ruleResult=0;


    static int PrePostProcForGenQueryFlag = -2;    
    int i, argc;
    ruleExecInfo_t rei2;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    
    if (PrePostProcForGenQueryFlag < 0) {
      if (getenv("PREPOSTPROCFORGENQUERYFLAG") != NULL)
	PrePostProcForGenQueryFlag = 1;
      else
	PrePostProcForGenQueryFlag = 0;
    }

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }

    /*  printGenQI(genQueryInp);  for debug */

    *genQueryOut = (genQueryOut_t*)malloc(sizeof(genQueryOut_t));
    memset((char *)*genQueryOut, 0, sizeof(genQueryOut_t));

    if (ruleExecuted==0) {
#if 0
       msParam_t *outMsParam;
#endif
       memset((char*)&rei,0,sizeof(rei));
       rei.rsComm = rsComm;
       if (rsComm != NULL) {
          /* Include the user info for possible use by the rule.  Note
	     that when this is called (as the agent is initializing),
	     this user info is not confirmed yet.  For password
	     authentication though, the agent will soon exit if this
	     is not valid.  But tor GSI, the user information may not
	     be present and/or may be changed when the authentication
	     completes, so it may not be safe to use this in a GSI
	     enabled environment.  This addition of user information
	     was requested by ARCS/IVEC (Sean Fleming) to avoid a
	     local patch.
          */
	  rei.uoic = &rsComm->clientUser;
	  rei.uoip = &rsComm->proxyUser;
       }
#ifdef RULE_ENGINE_N
       if (getRuleEngineStatus() == UNINITIALIZED) { 
          /* Skip the call to run acAclPolicy if the Rule Engine
             hasn't been initialized yet, which happens for a couple
             initial queries made by the agent when starting up.  The
             new RE logs these types of errors and so this avoids that.
          */
          status = -1;
       }
       else 
#endif
       {
          status = applyRule ("acAclPolicy", NULL, &rei, NO_SAVE_REI);
          ruleResult = rei.status;
       }
       if (status==0) {
	  ruleExecuted=1; /* No need to retry next time since it
                             succeeded.  Since this is called at
                             startup, the Rule Engine may not be
                             initialized yet, in which case the
                             default setting is fine and we should
                             retry next time. */
#if 0
	  /* No longer need this as msiAclPolicy calls
	     chlGenQueryAccessControlSetup to set the flag.  Leaving
	     it in the code for now in case needed later. */
	  outMsParam = getMsParamByLabel(&rei.inOutMsParamArray, "STRICT");
	  printf("outMsParam=%x\n",(int)outMsParam);
	  if (outMsParam != NULL) {
	     ruleResult=1;
	  }
#endif
       }
#if 0
       printf("rsGenQuery rule status=%d ruleResult=%d\n",status,ruleResult);
#endif
    }

    chlGenQueryAccessControlSetup(rsComm->clientUser.userName, 
			      rsComm->clientUser.rodsZone,
	 		      rsComm->clientUser.authInfo.authFlag,
			      -1);
#if 0
    rodsLog (LOG_NOTICE, 
	     "_rsGenQuery debug: client %s %d proxy %s %d", 
	     rsComm->clientUser.userName, 
	     rsComm->clientUser.authInfo.authFlag,
	     rsComm->proxyUser.userName, 
	     rsComm->proxyUser.authInfo.authFlag);
#endif
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    if (PrePostProcForGenQueryFlag == 1) {
      args[0] = (char *) malloc(300);
      sprintf(args[0],"%ld",(long) genQueryInp);
      argc = 1;
      i =  applyRuleArg("acPreProcForGenQuery",args,argc, &rei2, NO_SAVE_REI);
      free(args[0]);
      if (i < 0) {
	rodsLog (LOG_ERROR,
		 "rsGenQuery:acPreProcForGenQuery error,stat=%d", i);
        if (i != NO_MICROSERVICE_FOUND_ERR)
	  return i;
      }
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    status = chlGenQuery(*genQueryInp, *genQueryOut);

    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    if (PrePostProcForGenQueryFlag == 1) {
      args[0] = (char *) malloc(300);
      args[1] = (char *) malloc(300);
      args[2] = (char *) malloc(300);
      sprintf(args[0],"%ld",(long) genQueryInp);
      sprintf(args[1],"%ld",(long) *genQueryOut);
      sprintf(args[2],"%d",status);
      argc = 3;
      i =  applyRuleArg("acPostProcForGenQuery",args,argc, &rei2, NO_SAVE_REI);
      free(args[0]);
      free(args[1]);
      free(args[2]);
      if (i < 0) {
        rodsLog (LOG_ERROR,
                 "rsGenQuery:acPostProcForGenQuery error,stat=%d", i);
	if (i != NO_MICROSERVICE_FOUND_ERR)
	  return i;
      }
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    if (status < 0) {
       clearGenQueryOut (*genQueryOut);
       free (*genQueryOut);
       *genQueryOut = NULL;
       if (status != CAT_NO_ROWS_FOUND) {
	  rodsLog (LOG_NOTICE, 
		   "_rsGenQuery: genQuery status = %d", status);
       }
       return (status);
    }
    return (status);
} 
#endif
