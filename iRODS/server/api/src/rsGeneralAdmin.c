/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See generalAdmin.h for a description of this API call.*/

#include "generalAdmin.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsGeneralAdmin (rsComm_t *rsComm, generalAdminInp_t *generalAdminInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog(LOG_DEBUG, "generalAdmin");

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGeneralAdmin (rsComm, generalAdminInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcGeneralAdmin(rodsServerHost->conn,
			       generalAdminInp);
       if (status < 0) {
	  replErrorStack (rodsServerHost->conn->rError, &rsComm->rError);
       }
    }
    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsGeneralAdmin: rcGeneralAdmin error %d", status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGeneralAdmin(rsComm_t *rsComm, generalAdminInp_t *generalAdminInp )
{
    int status;
    userInfo_t userInfo;
    collInfo_t collInfo;
    rescInfo_t rescInfo;
    ruleExecInfo_t rei;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }

    rodsLog (LOG_DEBUG,
	     "_rsGeneralAdmin arg0=%s", 
	     generalAdminInp->arg0);

    if (strcmp(generalAdminInp->arg0,"pvacuum")==0) {
       char *args[2];
       char argStr[100];    /* argument string */
       memset((char*)&rei,0,sizeof(rei));
       rei.rsComm = rsComm;
       rei.uoic = &rsComm->clientUser;
       rei.uoip = &rsComm->proxyUser;
       rstrcpy(argStr,"",sizeof argStr);
       if (atoi(generalAdminInp->arg1) > 0) {
	  snprintf(argStr,sizeof argStr,"<ET>%s</ET>",generalAdminInp->arg1);
       }
       if (atoi(generalAdminInp->arg2) > 0) {
	  strncat(argStr,"<EF>",100);
	  strncat(argStr,generalAdminInp->arg2,100);
	  strncat(argStr,"</EF>",100);
       }
       args[0]=argStr;
       status = applyRuleArg("acVacuum", args, 1, &rei, SAVE_REI);
       return(status);
    }

    if (strcmp(generalAdminInp->arg0,"add")==0) {
       if (strcmp(generalAdminInp->arg1,"user")==0) { 
	  /* run the acCreateUser rule */
	  char *args[2];
	  memset((char*)&rei,0,sizeof(rei));
	  rei.rsComm = rsComm;
	  strncpy(userInfo.userName, generalAdminInp->arg2, 
		  sizeof userInfo.userName);
	  strncpy(userInfo.userType, generalAdminInp->arg3, 
		  sizeof userInfo.userType);
	  strncpy(userInfo.rodsZone, generalAdminInp->arg4, 
		  sizeof userInfo.rodsZone);
	  strncpy(userInfo.authInfo.authStr, generalAdminInp->arg5, 
		  sizeof userInfo.authInfo.authStr);
	  rei.uoio = &userInfo;
	  rei.uoic = &rsComm->clientUser;
	  rei.uoip = &rsComm->proxyUser;
	  status = applyRuleArg("acCreateUser", args, 0, &rei, SAVE_REI);
	  if (status != 0) chlRollback(rsComm);
          return(status);
       }
       if (strcmp(generalAdminInp->arg1,"dir")==0) {
	  memset((char*)&collInfo,0,sizeof(collInfo));
	  strncpy(collInfo.collName, generalAdminInp->arg2, 
		  sizeof collInfo.collName);
	  if (strlen(generalAdminInp->arg3) > 0) {
	     strncpy(collInfo.collOwnerName, generalAdminInp->arg3,
		     sizeof collInfo.collOwnerName);
	     status = chlRegCollByAdmin(rsComm, &collInfo);
	     if (status == 0) {
		int status2;
		status2 = chlCommit(rsComm);
	     }
	  }
	  else {
	     status = chlRegColl(rsComm, &collInfo);
	  }
	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"zone")==0) {
	  status = chlRegZone(rsComm, generalAdminInp->arg2,
			      generalAdminInp->arg3, 
			      generalAdminInp->arg4,
			      generalAdminInp->arg5);
	  if (status == 0) {
	     if (strcmp(generalAdminInp->arg3,"remote")==0) {
		memset((char*)&collInfo,0,sizeof(collInfo));
		strncpy(collInfo.collName, "/", sizeof collInfo.collName);
		strncat(collInfo.collName, generalAdminInp->arg2,
			sizeof collInfo.collName);
		strncpy(collInfo.collOwnerName, rsComm->proxyUser.userName,
			sizeof collInfo.collOwnerName);
		status = chlRegCollByAdmin(rsComm, &collInfo);
		if (status == 0) {
		   chlCommit(rsComm);
		}
	     }
	  }
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"resource")==0) {
	  strncpy(rescInfo.rescName,  generalAdminInp->arg2, 
		  sizeof rescInfo.rescName);
	  strncpy(rescInfo.rescType,  generalAdminInp->arg3, 
		  sizeof rescInfo.rescType);
	  strncpy(rescInfo.rescClass, generalAdminInp->arg4, 
		  sizeof rescInfo.rescClass);
	  strncpy(rescInfo.rescLoc,   generalAdminInp->arg5, 
		  sizeof rescInfo.rescLoc);
	  strncpy(rescInfo.rescVaultPath, generalAdminInp->arg6, 
		  sizeof rescInfo.rescVaultPath);
	  strncpy(rescInfo.zoneName,  generalAdminInp->arg7, 
		  sizeof rescInfo.zoneName);
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  args[0] = rescInfo.rescName;
	  args[1] = rescInfo.rescType;
	  args[2] = rescInfo.rescClass;
	  args[3] = rescInfo.rescLoc;
	  args[4] = rescInfo.rescVaultPath;
	  args[5] = rescInfo.zoneName;
	  argc = 6;
	  i =  applyRuleArg("acPreProcForCreateResource", args, argc, &rei2, NO_SAVE_REI);
	  if (i < 0) {
	    if (rei2.status < 0) {
	      i = rei2.status;
	    }
	    rodsLog (LOG_ERROR,
		     "rsGeneralAdmin:acPreProcForCreateResource error for %s,stat=%d",
		     rescInfo.rescName,i);
	    return i;
	  }
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlRegResc(rsComm, &rescInfo);
	  if (status != 0) {
	    chlRollback(rsComm);
	    return(status);
	  }
	  
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  i =  applyRuleArg("acPostProcForCreateResource",args,argc, &rei2, NO_SAVE_REI);
	  if (i < 0) {
	    if (rei2.status < 0) {
	      i = rei2.status;
	    }
	    rodsLog (LOG_ERROR,
		     "rsGeneralAdmin:acPostProcForCreateResource error for %s,stat=%d",
                     rescInfo.rescName,i);
            return i;
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"token")==0) {
	 
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  args[0] = generalAdminInp->arg2;
	  args[1] = generalAdminInp->arg3;
	  args[2] = generalAdminInp->arg4;
	  args[3] = generalAdminInp->arg5;
	  args[4] = generalAdminInp->arg6;
	  args[5] = generalAdminInp->arg7;
	  argc = 6;
	  i =  applyRuleArg("acPreProcForCreateToken", args, argc, &rei2, NO_SAVE_REI);
	  if (i < 0) {
	    if (rei2.status < 0) {
	      i = rei2.status;
	    }
	    rodsLog (LOG_ERROR,
		     "rsGeneralAdmin:acPreProcForCreateToken error for %s.%s=%s,stat=%d",
		     args[0],args[1],args[2],i);
	    return i;
	  }
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlRegToken(rsComm, generalAdminInp->arg2,
			       generalAdminInp->arg3, 
			       generalAdminInp->arg4,
			       generalAdminInp->arg5,
			       generalAdminInp->arg6,
			       generalAdminInp->arg7);
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
 	  if (status == 0) {
	    i =  applyRuleArg("acPostProcForCreateToken", args, argc, &rei2, NO_SAVE_REI);
	    if (i < 0) {
	      if (rei2.status < 0) {
		i = rei2.status;
	      }
	      rodsLog (LOG_ERROR,
		       "rsGeneralAdmin:acPostProcForCreateToken error for %s.%s=%s,stat=%d",
		       args[0],args[1],args[2],i);
	      return i;
	    }
	  }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
 
	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"specificQuery")==0) {
	  status = chlAddSpecificQuery(rsComm, generalAdminInp->arg2,
				       generalAdminInp->arg3);
	  return(status);
       }
    }
    if (strcmp(generalAdminInp->arg0,"modify")==0) {
       if (strcmp(generalAdminInp->arg1,"user")==0) {
	 /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	 args[0] = generalAdminInp->arg2; /* username */
	 args[1] = generalAdminInp->arg3; /* option */
#if 0
	 args[2] = generalAdminInp->arg4; /* newValue */
#else
/* Since the obfuscated password might contain commas, single or
   double quotes, etc, it's hard to escape for processing (often
   causing a seg fault), so for now just pass in a dummy string.  
   It is also unlikely the microservice actually needs the obfuscated
   password. */
	 args[2] = "obfuscatedPw";
#endif
	 argc = 3;
	 i =  applyRuleArg("acPreProcForModifyUser",args,argc, &rei2, NO_SAVE_REI);
	 if (i < 0) {
	   if (rei2.status < 0) {
	     i = rei2.status;
	   }
	   rodsLog (LOG_ERROR,
		    "rsGeneralAdmin:acPreProcForModifyUser error for %s and option %s,stat=%d",
		    args[0],args[1], i);
	   return i;
	 }
	 /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlModUser(rsComm, generalAdminInp->arg2, 
			      generalAdminInp->arg3, generalAdminInp->arg4);

	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  if (status == 0) {
 	    i =  applyRuleArg("acPostProcForModifyUser",args,argc, &rei2, NO_SAVE_REI);
	    if (i < 0) {
	      if (rei2.status < 0) {
		i = rei2.status;
	      }
	      rodsLog (LOG_ERROR,
		       "rsGeneralAdmin:acPostProcForModifyUser error for %s and option %s,stat=%d",
		       args[0],args[1], i);
	      return i;
	    }
	  }
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"group")==0) {
         /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
         args[0] = generalAdminInp->arg2; /* groupname */
         args[1] = generalAdminInp->arg3; /* option */
         args[2] = generalAdminInp->arg4; /* username */
	 args[3] = generalAdminInp->arg5; /* zonename */
         argc = 4;
         i =  applyRuleArg("acPreProcForModifyUserGroup",args,argc, &rei2, NO_SAVE_REI);
         if (i < 0) {
           if (rei2.status < 0) {
             i = rei2.status;
           }
           rodsLog (LOG_ERROR,
                    "rsGeneralAdmin:acPreProcForModifyUserGroup error for %s and option %s,stat=%d",
                    args[0],args[1], i);
           return i;
         }
         /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlModGroup(rsComm, generalAdminInp->arg2, 
			       generalAdminInp->arg3, generalAdminInp->arg4,
			       generalAdminInp->arg5);
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
          if (status == 0) {
            i =  applyRuleArg("acPostProcForModifyUserGroup",args,argc, &rei2, NO_SAVE_REI);
            if (i < 0) {
              if (rei2.status < 0) {
                i = rei2.status;
              }
              rodsLog (LOG_ERROR,
                       "rsGeneralAdmin:acPostProcForModifyUserGroup error for %s and option %s,stat=%d",
                       args[0],args[1], i);
              return i;
            }
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"zone")==0) {
	  status = chlModZone(rsComm, generalAdminInp->arg2, 
			      generalAdminInp->arg3, generalAdminInp->arg4);
	  if (status != 0) chlRollback(rsComm);
	  if (status == 0 &&
	      strcmp(generalAdminInp->arg3, "name")==0) {
	     char oldName[MAX_NAME_LEN];
	     char newName[MAX_NAME_LEN];
	     strncpy(oldName, "/", sizeof oldName);
	     strncat(oldName, generalAdminInp->arg2, sizeof oldName);
	     strncpy(newName, generalAdminInp->arg4, sizeof newName);
	     status = chlRenameColl(rsComm, oldName, newName);
	     if (status==0) chlCommit(rsComm);
	  }
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"localzonename")==0) {
	  /* run the acRenameLocalZone rule */
	  char *args[2];
	  memset((char*)&rei,0,sizeof(rei));
	  rei.rsComm = rsComm;
	  memset((char*)&rei,0,sizeof(rei));
	  rei.rsComm = rsComm;
	  rei.uoic = &rsComm->clientUser;
	  rei.uoip = &rsComm->proxyUser;
	  args[0]=generalAdminInp->arg2;
	  args[1]=generalAdminInp->arg3;
	  status = applyRuleArg("acRenameLocalZone", args, 2, &rei, 
				NO_SAVE_REI);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"resource")==0) {
         /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
         args[0] = generalAdminInp->arg2; /* rescname */
         args[1] = generalAdminInp->arg3; /* option */
         args[2] = generalAdminInp->arg4; /* newvalue */
         argc = 3;
         i =  applyRuleArg("acPreProcForModifyResource",args,argc, &rei2, NO_SAVE_REI);
         if (i < 0) {
           if (rei2.status < 0) {
             i = rei2.status;
           }
           rodsLog (LOG_ERROR,
                    "rsGeneralAdmin:acPreProcForModifyResource error for %s and option %s,stat=%d",
                    args[0],args[1], i);
           return i;
         }
         /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlModResc(rsComm, generalAdminInp->arg2, 
			      generalAdminInp->arg3, generalAdminInp->arg4);

	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
          if (status == 0) {
            i =  applyRuleArg("acPostProcForModifyResource",args,argc, &rei2, NO_SAVE_REI);
            if (i < 0) {
              if (rei2.status < 0) {
                i = rei2.status;
              }
              rodsLog (LOG_ERROR,
                       "rsGeneralAdmin:acPostProcForModifyResource error for %s and option %s,stat=%d",
                       args[0],args[1], i);
              return i;
            }
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"resourcegroup")==0) {
	 /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
         args[0] = generalAdminInp->arg2; /* rescgroupname */
         args[1] = generalAdminInp->arg3; /* option */
         args[2] = generalAdminInp->arg4; /* rescname */
         argc = 3;
         i =  applyRuleArg("acPreProcForModifyResourceGroup",args,argc, &rei2, NO_SAVE_REI);
         if (i < 0) {
           if (rei2.status < 0) {
             i = rei2.status;
           }
           rodsLog (LOG_ERROR,
                    "rsGeneralAdmin:acPreProcForModifyResourceGroup error for %s and option %s,stat=%d",
                    args[0],args[1], i);
           return i;
         }
         /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlModRescGroup(rsComm, generalAdminInp->arg2, 
			      generalAdminInp->arg3, generalAdminInp->arg4);

	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
          if (status == 0) {
            i =  applyRuleArg("acPostProcForModifyResourceGroup",args,argc, &rei2, NO_SAVE_REI);
            if (i < 0) {
              if (rei2.status < 0) {
                i = rei2.status;
              }
              rodsLog (LOG_ERROR,
                       "rsGeneralAdmin:acPostProcForModifyResourceGroup error for %s and option %s,stat=%d",
                       args[0],args[1], i);
              return i;
            }
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
    }
    if (strcmp(generalAdminInp->arg0,"rm")==0) {
       if (strcmp(generalAdminInp->arg1,"user")==0) { 
	  /* run the acDeleteUser rule */
	  char *args[2];
	  memset((char*)&rei,0,sizeof(rei));
	  rei.rsComm = rsComm;
	  strncpy(userInfo.userName, generalAdminInp->arg2, 
		  sizeof userInfo.userName);
	  strncpy(userInfo.rodsZone, generalAdminInp->arg3, 
		  sizeof userInfo.rodsZone);
	  rei.uoio = &userInfo;
	  rei.uoic = &rsComm->clientUser;
	  rei.uoip = &rsComm->proxyUser;
	  status = applyRuleArg("acDeleteUser", args, 0, &rei, SAVE_REI);
	  if (status != 0) chlRollback(rsComm);
          return(status);
       }
       if (strcmp(generalAdminInp->arg1,"dir")==0) {
	  memset((char*)&collInfo,0,sizeof(collInfo));
	  strncpy(collInfo.collName, generalAdminInp->arg2, 
		  sizeof collInfo.collName);
	  status = chlDelColl(rsComm, &collInfo);
	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"resource")==0) {
	  strncpy(rescInfo.rescName,  generalAdminInp->arg2, 
		  sizeof rescInfo.rescName);

	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	  args[0] = rescInfo.rescName;
	  argc = 1;
          i =  applyRuleArg("acPreProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI);
          if (i < 0) {
            if (rei2.status < 0) {
              i = rei2.status;
            }
            rodsLog (LOG_ERROR,
                     "rsGeneralAdmin:acPreProcForDeleteResource error for %s,stat=%d",
                     rescInfo.rescName,i);
            return i;
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlDelResc(rsComm, &rescInfo);
	  if (status == 0) {
	    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	    i =  applyRuleArg("acPostProcForDeleteResource",args,argc, &rei2, NO_SAVE_REI);
	    if (i < 0) {
	      if (rei2.status < 0) {
		i = rei2.status;
	      }
	      rodsLog (LOG_ERROR,
		       "rsGeneralAdmin:acPostProcForDeleteResource error for %s,stat=%d",
		       rescInfo.rescName,i);
	      return i;
	    }
	  }
	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  if (status != 0) chlRollback(rsComm);
	  return(status); 
       }
       if (strcmp(generalAdminInp->arg1,"zone")==0) {
	  status = chlDelZone(rsComm, generalAdminInp->arg2);
	  if (status == 0) {
	     memset((char*)&collInfo,0,sizeof(collInfo));
	     strncpy(collInfo.collName, "/", sizeof collInfo.collName);
	     strncat(collInfo.collName, generalAdminInp->arg2,
		     sizeof collInfo.collName);
	     status = chlDelCollByAdmin(rsComm, &collInfo);
	  }
	  if (status == 0) {
	     status = chlCommit(rsComm);
	  }
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"token")==0) {

	 /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
	 args[0] = generalAdminInp->arg2;
	 args[1] = generalAdminInp->arg3;
	 argc = 2;
	 i =  applyRuleArg("acPreProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI);
	 if (i < 0) {
	   if (rei2.status < 0) {
	     i = rei2.status;
	   }
	   rodsLog (LOG_ERROR,
		    "rsGeneralAdmin:acPreProcForDeleteToken error for %s.%s,stat=%d",
		    args[0],args[1],i);
	   return i;
	 }
	 /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  status = chlDelToken(rsComm, generalAdminInp->arg2,
			       generalAdminInp->arg3);

	  /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
          if (status == 0) {
            i =  applyRuleArg("acPostProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI);
            if (i < 0) {
              if (rei2.status < 0) {
                i = rei2.status;
              }
              rodsLog (LOG_ERROR,
                       "rsGeneralAdmin:acPostProcForDeleteToken error for %s.%s,stat=%d",
                       args[0],args[1],i);
              return i;
            }
          }
          /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

	  if (status != 0) chlRollback(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"unusedAVUs")==0) {
	  status = chlDelUnusedAVUs(rsComm);
	  return(status);
       }
       if (strcmp(generalAdminInp->arg1,"specificQuery")==0) {
	  status = chlDelSpecificQuery(rsComm, generalAdminInp->arg2);
	  return(status);
       }
    }
    if (strcmp(generalAdminInp->arg0,"calculate-usage")==0) {
       status = chlCalcUsageAndQuota(rsComm);
       return(status);
    }
    if (strcmp(generalAdminInp->arg0,"set-quota")==0) {
       status = chlSetQuota(rsComm,
			    generalAdminInp->arg1, 
			    generalAdminInp->arg2, 
			    generalAdminInp->arg3, 
			    generalAdminInp->arg4);

       return(status);
    }
    return(CAT_INVALID_ARGUMENT);
} 
#endif
