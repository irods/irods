/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobals.h"
#include "initServer.h"
#include "reHelpers1.h"
#include "apiHeaderAll.h"

#ifdef MYMALLOC
# Within reLib1.c here, change back the redefines of malloc back to normal
#define malloc(x) malloc(x)
#define free(x) free(x)
#endif


#if 0
int 
applyActionCall(char *actionCall,  ruleExecInfo_t *rei, int reiSaveFlag)
{
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  char action[MAX_ACTION_SIZE];  
  int i, argc;

  i = parseAction(actionCall,action,args, &argc);
  if (i != 0)
    return(i);

  i  = applyRuleArg(action, args, argc, rei, reiSaveFlag);

  return(i);
}


int
applyRule(char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	  ruleExecInfo_t *rei, int reiSaveFlag)
{
  int ruleInx, i, status;
  char *nextRule;
  ruleInx = -1; /* new rule */
  char ruleCondition[MAX_RULE_LENGTH * 3];
  char ruleAction[MAX_RULE_LENGTH * 3];
  char ruleRecovery[MAX_RULE_LENGTH * 3];
  char ruleHead[MAX_RULE_LENGTH * 3]; 
  char ruleBase[MAX_RULE_LENGTH * 3]; 
  int  first = 0;
  ruleExecInfo_t  *saveRei;
  int reTryWithoutRecovery = 0;
  funcPtr myFunc = NULL;
  int actionInx;
  int numOfStrArgs;
  int ii;

  mapExternalFuncToInternalProc(action);

  i = findNextRule (action,  &ruleInx);
  if (i != 0) {
    /* probably a microservice */
    i = executeMicroService (action,args,argc,rei);
    if (i < 0) {
      rodsLog (LOG_NOTICE,"applyRule Failed for action: %s with status %i",action,i);
    }
    return(i);
  }

  while (i == 0) {
    getRule(ruleInx, ruleBase,ruleHead, ruleCondition,ruleAction, ruleRecovery, MAX_RULE_LENGTH * 3);

    i  = initializeMsParam(ruleHead,args,argc, rei);
    if (i != 0) {
      rodsLog (LOG_NOTICE,"applyRule Failed in  initializeMsParam for action: %s with status %i",action,i);
      return(i);
    }
    /*****
    i = checkRuleHead(ruleHead,args,argc);
    if (i == 0) {
    ******/
    if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Testing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else if (rei != NULL && rei->rsComm != NULL && rei->rsComm->rError != NULL)
	    rodsLog(LOG_NOTICE,&(rei->rsComm->rError),-1,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	}

      i = checkRuleCondition(action,  ruleCondition, args, argc, rei, reiSaveFlag);
      if (i == TRUE) {
	if (reiSaveFlag == SAVE_REI) {
	  if (first == 0 ) {
	    saveRei = (ruleExecInfo_t  *) mallocAndZero(sizeof(ruleExecInfo_t));
	    i = copyRuleExecInfo(rei,saveRei);
	    first = 1;
	  }
	  else if (reTryWithoutRecovery == 0) {
	    i = copyRuleExecInfo(saveRei,rei);
	  }
	}
	if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Executing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else if (rei != NULL && rei->rsComm != NULL && rei->rsComm->rError != NULL)
	    rodsLog (LOG_NOTICE,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	}
	status = 
	   executeRuleBody(action, ruleAction, ruleRecovery, args, argc, rei, reiSaveFlag);
	if ( status == 0) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  finalizeMsParam(ruleHead,args,argc, rei,status);
	  return(status);
	}
	else if ( status == CUT_ACTION_PROCESSED_ERR) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  finalizeMsParam(ruleHead,args,argc, rei,status);
	  rodsLog (LOG_NOTICE,"applyRule Failed for action : %s with status %i",action, status);
	  return(status);
	}
	if ( status == RETRY_WITHOUT_RECOVERY_ERR)
	  reTryWithoutRecovery = 1;
	  finalizeMsParam(ruleHead,args,argc, rei,0);
      }
      /*****
    }
      *****/
    i = findNextRule (action,  &ruleInx);
  }
  finalizeMsParam(ruleHead,args,argc, rei,status);
  if (first == 1) {
    if (reiSaveFlag == SAVE_REI)
      freeRuleExecInfoStruct(saveRei, 0);
  }
  if (i == NO_MORE_RULES_ERR)
    return(i);
  return(status);

}
#endif

int 
applyRuleArg(char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	       ruleExecInfo_t *rei, int reiSaveFlag)
{
  msParamArray_t *inMsParamArray = NULL;
  int i;

  i = applyRuleArgPA(action ,args,  argc, inMsParamArray, rei, reiSaveFlag);
  return(i);
}


int 
applyRuleArgPA(char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		      msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag)
{
  int i;
  int pFlag = 0;
  msParam_t *mP;
  char tmpStr[MAX_ACTION_SIZE];

  if (inMsParamArray == NULL) {
    inMsParamArray = (msParamArray_t*)mallocAndZero(sizeof(msParamArray_t));
    pFlag = 1;
  }
  for (i = 0; i < argc ; i++) {
    if (args[i][0] == '*') { 
      if ((mP = getMsParamByLabel (inMsParamArray, args[i])) == NULL) {
	addMsParam(inMsParamArray, args[i], NULL, NULL,NULL);
      }
    }
    else {
      addMsParam(inMsParamArray, args[i], STR_MS_T, strdup (args[i]),NULL);
    }
  }
  makeAction(tmpStr,action, args,argc, MAX_ACTION_SIZE);
  i = applyRule(tmpStr, inMsParamArray, rei, reiSaveFlag);
#if 0
  /* RAJA ADDED Jul 14, 2008 to get back the changed args */
  if (i == 0) {
    for (i = 0; i < argc ; i++) {
      if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) 
	strcpy(args[i], (char *) mP->inOutStruct);
      /**** DANGER, DANGER: Potential overflow..... ****/
    }
    i = 0;
  }
  /* RAJA ADDED Jul 14, 2008 to get back the changed args */
#endif
  if (pFlag == 1)
    free(inMsParamArray);
  return(i);
  
}

int
initializeMsParamNew(char *ruleHead, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		     msParamArray_t *inMsParamArray,  ruleExecInfo_t *rei)
{
  int i;
  msParam_t *mP;
  char tmpStr[NAME_LEN];
  char *args2[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc2 = 0;
  msParamArray_t *outMsParamArray;
  char *tmparg;

  /* save the old msParamArray in rei */
#ifdef ADDR_64BITS
  snprintf(tmpStr,NAME_LEN, "%lld", (rodsLong_t) rei->msParamArray);  /* pointer stored as long integer */
#else
  snprintf(tmpStr,NAME_LEN, "%d", (uint) rei->msParamArray);  /* pointer stored as long integer */
#endif
  pushStack(&msParamStack,tmpStr);            /* pointer->integer->string stored in stack */

  /* make a new msParamArray in rei */
  rei->msParamArray = (msParamArray_t*)malloc(sizeof(msParamArray_t));
  rei->msParamArray->len = 0;
  rei->msParamArray->msParam = NULL;
  outMsParamArray = rei->msParamArray;


  parseAction(ruleHead, tmpStr,args2, &argc2);
  
  /* stick things into msParamArray in rei */
  /**** changed by RAJA Jul 11, 2007 so that conversion of values in the internal strings also happen ****
  for (i = 0; i < argc ; i++) {
    if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) {
      addMsParam(outMsParamArray,args2[i],mP->type, mP->inOutStruct, mP->inpOutBuf);
    }
    else {
      addMsParam(outMsParamArray, args2[i], NULL, NULL,NULL);
    }
  }
  **** changed by RAJA Jul 11, 2007 so that conversion of values in the internal strings also happen ****/
  for (i = 0; i < argc ; i++) {
    if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) {
      tmparg = NULL;
      if (mP->inOutStruct == NULL || (!strcmp(mP->type, STR_MS_T) 
				      && !strcmp((char*)mP->inOutStruct,(char*)mP->label) )) {
	convertArgWithVariableBinding(args[i],&tmparg,inMsParamArray,rei);
	if (tmparg != NULL)
	  addMsParam(outMsParamArray,args2[i],mP->type, tmparg, mP->inpOutBuf);
	else
	  addMsParam(outMsParamArray,args2[i],mP->type, mP->inOutStruct, mP->inpOutBuf);
      }
      else if (!strcmp(mP->type, STR_MS_T) ) {
	convertArgWithVariableBinding((char*)mP->inOutStruct,&tmparg,inMsParamArray,rei);
	if (tmparg != NULL) 
	  addMsParam(outMsParamArray,args2[i],mP->type, tmparg, mP->inpOutBuf);
	else
	  addMsParam(outMsParamArray,args2[i],mP->type, mP->inOutStruct, mP->inpOutBuf);
      }
      else
	addMsParam(outMsParamArray,args2[i],mP->type, mP->inOutStruct, mP->inpOutBuf);
      if (tmparg != NULL)
	free(tmparg);
    }
    else {
      if( args[i][0] == '*')
	addMsParam(outMsParamArray, args2[i], NULL, NULL,NULL);
      else
	addMsParam(outMsParamArray, args2[i],STR_MS_T, args[i], NULL);
    }
  }
  /* RAJA added July 11 2007 to make sure that ruleExecOut is apassed along */
  if ((mP = getMsParamByLabel (inMsParamArray, "ruleExecOut")) != NULL) 
    /*    if (getMsParamByLabel (outMsParamArray,"ruleExecOut") != NULL)  RAJA CHANGED Sep 25 2007 ***/
    if (getMsParamByLabel (outMsParamArray,"ruleExecOut") == NULL)
      addMsParam(outMsParamArray,"ruleExecOut",mP->type, mP->inOutStruct, mP->inpOutBuf);
  /* RAJA added July 11 2007 to make sure that ruleExecOut is apassed along */

  freeRuleArgs (args2, argc2);
  return (0);
}


int
finalizeMsParamNew(char *inAction,char *ruleHead, 
		   msParamArray_t *inMsParamArray, msParamArray_t *outMsParamArray,
		   ruleExecInfo_t *rei, int status)
{

  msParamArray_t *oldMsParamArray;
  char tmpStr[NAME_LEN];
  msParam_t *mP;
  int i;
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc = 0;
  char *args2[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc2 = 0;

  parseAction(ruleHead, tmpStr,args, &argc);
  parseAction(inAction, tmpStr,args2, &argc2);

  /* get the old msParamArray  */
  popStack(&msParamStack,tmpStr);   
#ifdef ADDR_64BITS
  oldMsParamArray = (msParamArray_t *) strtoll (tmpStr, 0, 0);
#else
  oldMsParamArray = (msParamArray_t *) atoi(tmpStr);
#endif

  for (i = 0; i < argc2; i++) {
    if ((mP = getMsParamByLabel (outMsParamArray, args[i])) != NULL) {
      rmMsParamByLabel (inMsParamArray, args2[i],0);
      addMsParam(inMsParamArray, args2[i], mP->type, mP->inOutStruct, mP->inpOutBuf);
    }
  }

  /* RAJA added July 11 2007 to make sure that ruleExecOut is apassed along */
  if ((mP = getMsParamByLabel (outMsParamArray, "ruleExecOut")) != NULL) {
    rmMsParamByLabel (inMsParamArray,  "ruleExecOut",0);
    addMsParam(inMsParamArray,"ruleExecOut",mP->type, mP->inOutStruct, mP->inpOutBuf);
  }
  /* RAJA added July 11 2007 to make sure that ruleExecOut is apassed along */

  /* XXXX should use clearMsparamInRei (rei); ? */
  freeRuleArgs (args, argc);
  freeRuleArgs (args2, argc2);
  /* XXXX fix memleak. MW */
  /* free(rei->msParamArray);  no need to free outMsParamArray. It was 
   * set to rei->msParamArray */
  clearMsparamInRei (rei);
  rei->msParamArray = oldMsParamArray;
  return(0);
}


/** this was applyRulePA and got changed to applyRule ***/

int
applyRule(char *inAction, msParamArray_t *inMsParamArray,
	   ruleExecInfo_t *rei, int reiSaveFlag)
{
  int i, ii;

  if (strlen (rei->ruleName) == 0) {
    strncpy (rei->ruleName, inAction, NAME_LEN);
    rei->ruleName[NAME_LEN - 1] = '\0';
  }

  if (strstr(inAction,"##") != NULL) { /* seems to be multiple actions */
    i = execMyRuleWithSaveFlag(inAction,inMsParamArray,rei,reiSaveFlag);
    return(i);
  }

  if (strstr(inAction,"|") != NULL) { /* seems to be multiple actions */
    i = execMyRuleWithSaveFlag(inAction,inMsParamArray,rei,reiSaveFlag);
    return(i);
  }

  if (GlobalAllRuleExecFlag != 0) {
    ii = GlobalAllRuleExecFlag;
    i = applyAllRules(inAction, inMsParamArray, rei, reiSaveFlag, GlobalAllRuleExecFlag);
    GlobalAllRuleExecFlag = ii;
    return(i);
  }

  if (GlobalREAuditFlag) 
    reDebug("ApplyRule", -1, inAction,inMsParamArray,rei); 

  i = _applyRule(inAction, inMsParamArray, rei, reiSaveFlag);

  if (GlobalREAuditFlag) 
    reDebug("DoneApplyRule", -1, inAction,inMsParamArray,rei); 

  return(i);
}

int
_applyRule(char *inAction, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei, int reiSaveFlag)
{
  int ruleInx, argc,i, status;
  /*char *nextRule; */
  char ruleCondition[MAX_RULE_LENGTH * 3];
  char ruleAction[MAX_RULE_LENGTH * 3];
  char ruleRecovery[MAX_RULE_LENGTH * 3];
  char ruleHead[MAX_RULE_LENGTH * 3]; 
  char ruleBase[MAX_RULE_LENGTH * 3]; 
  int  first = 0;
  ruleExecInfo_t  *saveRei;
  int reTryWithoutRecovery = 0;
  /*funcPtr myFunc = NULL;*/
  /*int actionInx;*/
  /*int numOfStrArgs;*/
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  char action[MAX_ACTION_SIZE];  
  msParamArray_t *outMsParamArray;


  ruleInx = -1; /* new rule */


  if( NULL == rei ) { // JMC cppcheck
	  rodsLog( LOG_ERROR, "" );
	  return -1;
  }

  i = parseAction(inAction,action,args, &argc);
  if (i != 0)
    return(i);

  mapExternalFuncToInternalProc(action);

  i = findNextRule (action,  &ruleInx);
  if (i != 0) {
    /* probably a microservice */
#if 0
    i = executeMicroServiceNew(action,inMsParamArray,rei);
#endif
    i = executeMicroServiceNew(inAction,inMsParamArray,rei);
    return(i);
  }

  while (i == 0) {
    getRule(ruleInx, ruleBase,ruleHead, ruleCondition,ruleAction, ruleRecovery, MAX_RULE_LENGTH * 3);
    if (GlobalREAuditFlag) 
      reDebug("  GotRule", ruleInx, inAction,inMsParamArray,rei); 

    i  = initializeMsParamNew(ruleHead,args,argc, inMsParamArray, rei);
    if (i != 0)
      return(i);
    outMsParamArray = rei->msParamArray;

    /*****
    i = checkRuleHead(ruleHead,args,argc);
    freeRuleArgs (args, argc);
    if (i == 0) {
    ******/
    if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Testing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else if (rei != 0 && rei->rsComm != 0 && &(rei->rsComm->rError) != 0)
	    rodsLog (LOG_NOTICE,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	}

      i = checkRuleConditionNew(action,  ruleCondition, outMsParamArray, rei, reiSaveFlag);
      if (i == TRUE) {
	if (reiSaveFlag == SAVE_REI) {
	  if (first == 0 ) {
	    saveRei = (ruleExecInfo_t  *) mallocAndZero(sizeof(ruleExecInfo_t));
	    i = copyRuleExecInfo(rei,saveRei);
	    first = 1;
	  }
	  else if (reTryWithoutRecovery == 0) {
	    i = copyRuleExecInfo(saveRei,rei);
	  }
	}
	if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Executing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else
	    rodsLog (LOG_NOTICE,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	}
	status = 
	   executeRuleBodyNew(action, ruleAction, ruleRecovery, outMsParamArray, rei, reiSaveFlag);
	if ( status == 0  || status == CUT_ACTION_ON_SUCCESS_PROCESSED_ERR) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  finalizeMsParamNew(inAction,ruleHead,inMsParamArray, outMsParamArray, rei,status);
	  return(0);
	}
	else if ( status == CUT_ACTION_PROCESSED_ERR || 
	  status == MSI_OPERATION_NOT_ALLOWED) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,status);
	  return(status);
	}
	if ( status == RETRY_WITHOUT_RECOVERY_ERR)
	  reTryWithoutRecovery = 1;
	finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,0);
        outMsParamArray = NULL;		/* set this since finalizeMsParamNew
					 * freed it.
					 */
      }
      else {/*** ADDED RAJA JUN 20, 2007 ***/
	finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,0);
      }
      /*****
    }
      *****/
    i = findNextRule (action,  &ruleInx);
  }

  if (first == 1) {
    if (reiSaveFlag == SAVE_REI)
      freeRuleExecInfoStruct(saveRei, 0);
  }
  if (i == NO_MORE_RULES_ERR) {
    rodsLog (LOG_NOTICE,"applyRule Failed for action 1: %s with status %i",action, i);
    return(i);
  }

  finalizeMsParamNew(inAction,ruleHead,inMsParamArray, outMsParamArray, rei,status);


  if (status < 0) {
      rodsLog (LOG_NOTICE,"applyRule Failed for action 2: %s with status %i",action, status);
  }
  return(status);
}




int
applyAllRules(char *inAction, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag)
{
  int i;

  if (strlen (rei->ruleName) == 0) {
    strncpy (rei->ruleName, inAction, NAME_LEN);
    rei->ruleName[NAME_LEN - 1] = '\0';
  }

  GlobalAllRuleExecFlag = allRuleExecFlag;


  if (strstr(inAction,"##") != NULL) { /* seems to be multiple actions */
    i = execMyRuleWithSaveFlag(inAction,inMsParamArray,rei,reiSaveFlag);
    if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
    return(i);
  }

  if (strstr(inAction,"|") != NULL) { /* seems to be multiple actions */
    i = execMyRuleWithSaveFlag(inAction,inMsParamArray,rei,reiSaveFlag);
    if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
    return(i);
  }


  if (GlobalREAuditFlag) 
    reDebug("ApplyAllRules", -1, inAction,inMsParamArray,rei); 

  i = _applyAllRules(inAction, inMsParamArray, rei, reiSaveFlag, allRuleExecFlag);

  if (GlobalREAuditFlag) 
    reDebug("DoneApplyAllRules", -1, inAction,inMsParamArray,rei); 

  return(i);
}

int
_applyAllRules(char *inAction, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag)
{
  int ruleInx, argc,i, status;
  /*char *nextRule;*/
  char ruleCondition[MAX_RULE_LENGTH * 3];
  char ruleAction[MAX_RULE_LENGTH * 3];
  char ruleRecovery[MAX_RULE_LENGTH * 3];
  char ruleHead[MAX_RULE_LENGTH * 3]; 
  char ruleBase[MAX_RULE_LENGTH * 3]; 
  int  first = 0;
  int  success = 0;
  ruleExecInfo_t  *saveRei;
  int reTryWithoutRecovery = 0;
  /*funcPtr myFunc = NULL;*/
  /*int actionInx;*/
  /*int numOfStrArgs;*/
  /*int ii;*/
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  char action[MAX_ACTION_SIZE];  
  msParamArray_t *outMsParamArray;

  ruleInx = -1; /* new rule */
  outMsParamArray =  NULL;

  GlobalAllRuleExecFlag = allRuleExecFlag;

  i = parseAction(inAction,action,args, &argc);
  if (i != 0) {
    if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
    return(i);
  }

  mapExternalFuncToInternalProc(action);

  i = findNextRule (action,  &ruleInx);
  if (i != 0) {
    /* probably a microservice */
#if 0
    i = executeMicroServiceNew(action,inMsParamArray,rei);
#endif
    i = executeMicroServiceNew(inAction,inMsParamArray,rei);
    if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
    return(i);
  }

  while (i == 0) {
    getRule(ruleInx, ruleBase,ruleHead, ruleCondition,ruleAction, ruleRecovery, MAX_RULE_LENGTH * 3);
    if (GlobalREAuditFlag) 
      reDebug("  GotRule", ruleInx, inAction,inMsParamArray,rei); 

    if (outMsParamArray == NULL) {
      i  = initializeMsParamNew(ruleHead,args,argc, inMsParamArray, rei);
      if (i != 0) {
	if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
	return(i);
      }
      outMsParamArray = rei->msParamArray;
      
    }

    /*****
    i = checkRuleHead(ruleHead,args,argc);
    freeRuleArgs (args, argc);
    if (i == 0) {
    ******/
    if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Testing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else if (rei != 0 && rei->rsComm != 0 && &(rei->rsComm->rError) != 0)
	    rodsLog (LOG_NOTICE,"+Testing Rule Number:%i for Action:%s\n",ruleInx,action);
	}

      i = checkRuleConditionNew(action,  ruleCondition, outMsParamArray, rei, reiSaveFlag);
      if (i == TRUE) {
	if (reiSaveFlag == SAVE_REI) {
	  if (first == 0 ) {
	    saveRei = (ruleExecInfo_t  *) mallocAndZero(sizeof(ruleExecInfo_t));
	    i = copyRuleExecInfo(rei,saveRei);
	    first = 1;
	  }
	  else if (reTryWithoutRecovery == 0) {
	    i = copyRuleExecInfo(saveRei,rei);
	  }
	}
	if (reTestFlag > 0) {
	  if (reTestFlag == COMMAND_TEST_1) 
	    fprintf(stdout,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Executing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,action);
	  else
	    rodsLog (LOG_NOTICE,"+Executing Rule Number:%i for Action:%s\n",ruleInx,action);
	}
	status = 
	   executeRuleBodyNew(action, ruleAction, ruleRecovery, outMsParamArray, rei, reiSaveFlag);
	if ( status == 0) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  /**** finalizeMsParamNew(inAction,ruleHead,inMsParamArray, outMsParamArray, rei,status);
		if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
		return(status); ***/
	  success = 1;
	}
	else if ( status == CUT_ACTION_PROCESSED_ERR) {
	  if (reiSaveFlag == SAVE_REI)
	    freeRuleExecInfoStruct(saveRei, 0);
	  finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,status);
	  if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
	  return(status);
	}
        else if ( status == CUT_ACTION_ON_SUCCESS_PROCESSED_ERR) {
          if (reiSaveFlag == SAVE_REI)
            freeRuleExecInfoStruct(saveRei, 0);
          finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,status);
          if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
          return(0);
        }
	else if ( status == RETRY_WITHOUT_RECOVERY_ERR) {
	  reTryWithoutRecovery = 1;
	  /*** finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,0);***/
	}
	/***  outMsParamArray = NULL; 	***/ /* set this since finalizeMsParamNew
					 * freed it.
					 */
      }
      else {/*** ADDED RAJA JUN 20, 2007 ***/
	/*** finalizeMsParamNew(inAction,ruleHead,inMsParamArray,  outMsParamArray, rei,0); ***/
      }
      /*****
    }
      *****/
    i = findNextRule (action,  &ruleInx);
  }

  if (first == 1) {
    if (reiSaveFlag == SAVE_REI)
      freeRuleExecInfoStruct(saveRei, 0);
  }
  if (i == NO_MORE_RULES_ERR) {
    if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
    if (success == 1) {
      finalizeMsParamNew(inAction,ruleHead,inMsParamArray, outMsParamArray, rei,status);
      return(0);
    }
    else {
      rodsLog (LOG_NOTICE,"applyRule Failed for action 3: %s with status %i",action, i);
      return(i);
    }
  }

  finalizeMsParamNew(inAction,ruleHead,inMsParamArray, outMsParamArray, rei,status);

  if (status < 0) {
      rodsLog (LOG_NOTICE,"applyRule Failed for action 4: %s with status %i",action, status);
  }
  if( GlobalAllRuleExecFlag != 9) GlobalAllRuleExecFlag = 0;
  if (success == 1)
    return(0);
  else
    return(status);
}


int
execMyRule(char * ruleDef, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei)
{

  return(execMyRuleWithSaveFlag(ruleDef,inMsParamArray,rei,0));
}

int
execMyRuleWithSaveFlag(char * ruleDef, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei,int reiSaveFlag)

{
  int i;

  if (strstr(ruleDef, "|") == NULL && strstr(ruleDef, "##") == NULL) {
    i = applyRule(ruleDef, inMsParamArray, rei,1);
    return(i);
  }

  if (GlobalREAuditFlag)
    reDebug("ExecMyRule", -1, ruleDef,inMsParamArray,rei);

  i = _execMyRuleWithSaveFlag(ruleDef, inMsParamArray, rei, reiSaveFlag);

  if (GlobalREAuditFlag)
    reDebug("DoneExecMyRule", -1, ruleDef, inMsParamArray,rei);

  return(i);
}



int
_execMyRuleWithSaveFlag(char * ruleDef, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei,int reiSaveFlag)

{
  int i;
  char *inAction;
  char *ruleBase;
  char *ruleHead;
  char *ruleCondition;
  char *ruleAction;
  char *ruleRecovery;
  char buf[MAX_RULE_LENGTH];
  char l0[MAX_RULE_LENGTH];
  char l1[MAX_RULE_LENGTH];
  char l2[MAX_RULE_LENGTH];
  char l3[MAX_RULE_LENGTH];
  int status;
  char action[MAX_ACTION_SIZE];  
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc;


  rstrcpy(buf, ruleDef, MAX_RULE_LENGTH);
  

  rSplitStr(buf, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
  inAction = strdup(l1);  /** action **/
  ruleBase = strdup("MyRule");
  ruleHead = strdup(l1);  /** ruleHead **/
  rSplitStr(l0, l1, MAX_RULE_LENGTH, l3, MAX_RULE_LENGTH,'|');
  ruleCondition = strdup(l1); /** condition **/
  rSplitStr(l3, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|');
  ruleAction = strdup(l1);  /** function calls **/
  ruleRecovery = strdup(l2);  /** rollback calls **/
  if (strlen(ruleAction) == 0 && strlen(ruleRecovery) == 0) {
	free( ruleAction ); // JMC cppcheck - leak
	free( ruleRecovery ); // JMC cppcheck - leak
    ruleRecovery = ruleCondition;
    ruleAction = inAction;
    inAction = strdup("");
    ruleCondition = strdup("");
  }
  else if (strlen(ruleRecovery) == 0) {
    ruleRecovery = ruleAction;
    ruleAction = ruleCondition;
    ruleCondition = inAction;
    inAction = strdup("");
  }

  /*
  rodsLog(LOG_NOTICE,"PPP:%s::%s::%s::%s\n",inAction,ruleCondition,ruleAction,ruleRecovery);
  */

  i = parseAction(inAction,action,args, &argc);
  if (i != 0) {
	free( ruleBase ); // JMC cppcheck - leak
	free( ruleHead ); // JMC cppcheck - leak
    return(i);
  } 
  freeRuleArgs (args, argc);

  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1) 
      fprintf(stdout,"+Testing MyRule Rule for Action:%s\n",inAction);
    else if (reTestFlag == HTML_TEST_1)
      fprintf(stdout,"+Testing MyRule for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",inAction);
    else if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
      rodsLog (LOG_NOTICE,"+Testing MyRule for Action:%s\n",inAction);
  }

  i = checkRuleConditionNew(action,  ruleCondition, inMsParamArray, rei, reiSaveFlag);
  if (i == TRUE) {
    if (reTestFlag > 0) {
      if (reTestFlag == COMMAND_TEST_1) 
	fprintf(stdout,"+Executing MyRule  for Action:%s\n",action);
	  else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"+Executing MyRule for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",action);
	  else if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	    rodsLog (LOG_NOTICE,"+Executing MyRule for Action:%s\n",action);
    }
    status = 
	   executeMyRuleBody(action, ruleAction, ruleRecovery, inMsParamArray, rei, reiSaveFlag);
    if (status < 0) {
      rodsLog (LOG_NOTICE,"execMyRule %s Failed with status %i",ruleDef, status);
    }
	free( ruleBase ); // JMC cppcheck - leak
	free( ruleHead ); // JMC cppcheck - leak
    return(status);
  }
  else {
    rodsLog (LOG_NOTICE,"execMyRule %s Failed  with status %i",ruleDef, i);
	free( ruleBase ); // JMC cppcheck - leak
	free( ruleHead ); // JMC cppcheck - leak
    return (RULE_FAILED_ERR);
  }
}

int
initRuleStruct(rsComm_t *svrComm, char *irbSet, char *dvmSet, char *fnmSet)
{
  int i;
  char r1[NAME_LEN], r2[RULE_SET_DEF_LENGTH], r3[RULE_SET_DEF_LENGTH];
  
  strcpy(r2,irbSet);
  coreRuleStrct.MaxNumOfRules = 0;
  appRuleStrct.MaxNumOfRules = 0;
  GlobalAllRuleExecFlag = 0;

  while (strlen(r2) > 0) {
    i = rSplitStr(r2,r1,NAME_LEN,r3,RULE_SET_DEF_LENGTH,',');
    if (i == 0)
      i = readRuleStructFromFile(r1, &coreRuleStrct);
    if (i != 0)
      return(i);
    strcpy(r2,r3);
  }
  strcpy(r2,dvmSet);
  coreRuleVarDef.MaxNumOfDVars = 0;
  appRuleVarDef.MaxNumOfDVars = 0;
  
  while (strlen(r2) > 0) {
    i = rSplitStr(r2,r1,NAME_LEN,r3,RULE_SET_DEF_LENGTH,',');
    if (i == 0)
      i = readDVarStructFromFile(r1, &coreRuleVarDef);
    if (i != 0)
      return(i);
    strcpy(r2,r3);
  }
  strcpy(r2,fnmSet);
  coreRuleFuncMapDef.MaxNumOfFMaps = 0;
  appRuleFuncMapDef.MaxNumOfFMaps = 0;
  
  while (strlen(r2) > 0) {
    i = rSplitStr(r2,r1,NAME_LEN,r3,RULE_SET_DEF_LENGTH,',');
    if (i == 0)
      i = readFuncMapStructFromFile(r1, &coreRuleFuncMapDef);
    if (i != 0)
      return(i);
    strcpy(r2,r3);
  }

  if (getenv(RETESTFLAG) != NULL) {
    reTestFlag = atoi(getenv(RETESTFLAG));
    if (getenv(RELOOPBACKFLAG) != NULL)
      reLoopBackFlag = atoi(getenv(RELOOPBACKFLAG));
    else
      reLoopBackFlag = 0;
  }
  else {
    reTestFlag = 0;
    reLoopBackFlag = 0;
  }
  if (getenv("GLOBALALLRULEEXECFLAG") != NULL)
    GlobalAllRuleExecFlag = 9;

  if (getenv(GLOBALREDEBUGFLAG) != NULL) {
    GlobalREDebugFlag = atoi(getenv(GLOBALREDEBUGFLAG));
  }
  if (getenv(GLOBALREAUDITFLAG) != NULL) {
    GlobalREAuditFlag = atoi(getenv(GLOBALREAUDITFLAG));
  }
  if (GlobalREAuditFlag == 0 ) {
    GlobalREAuditFlag = GlobalREDebugFlag;
  }
  delayStack.size = NAME_LEN;
  delayStack.len = 0;
  delayStack.value = NULL;

  msParamStack.size = NAME_LEN;
  msParamStack.len = 0;
  msParamStack.value = NULL;

  initializeReDebug(svrComm, GlobalREDebugFlag); 


  return(0);
}


int
readRuleStructFromDB(char *ruleBaseName, char *versionStr, ruleStruct_t *inRuleStrct, ruleExecInfo_t *rei)
{
  int i,l,status;
  genQueryInp_t genQueryInp;
  genQueryOut_t *genQueryOut = NULL;
  char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
  sqlResult_t *r[8];
  memset(&genQueryInp, 0, sizeof(genQueryInp));
  genQueryInp.maxRows = MAX_SQL_ROWS;

  snprintf(condstr, MAX_NAME_LEN, "= '%s'", ruleBaseName);
  addInxVal(&genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_BASE_NAME, condstr);
  snprintf(condstr2, MAX_NAME_LEN, "= '%s'", versionStr);
  addInxVal(&genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_VERSION, condstr2);
  
  addInxIval(&genQueryInp.selectInp, COL_RULE_BASE_MAP_PRIORITY, ORDER_BY);
  addInxIval(&genQueryInp.selectInp, COL_RULE_BASE_MAP_BASE_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_EVENT, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_CONDITION, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_BODY, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_RECOVERY, 1);
  addInxIval(&genQueryInp.selectInp, COL_RULE_ID, 1);
  l = inRuleStrct->MaxNumOfRules;
  status = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);
  while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
    r[0] = getSqlResultByInx (genQueryOut, COL_RULE_BASE_MAP_BASE_NAME);
    r[1] = getSqlResultByInx (genQueryOut, COL_RULE_NAME);
    r[2] = getSqlResultByInx (genQueryOut, COL_RULE_EVENT);
    r[3] = getSqlResultByInx (genQueryOut, COL_RULE_CONDITION);
    r[4] = getSqlResultByInx (genQueryOut, COL_RULE_BODY);
    r[5] = getSqlResultByInx (genQueryOut, COL_RULE_RECOVERY);
    r[6] = getSqlResultByInx (genQueryOut, COL_RULE_ID);
    for (i = 0; i<genQueryOut->rowCnt; i++) {
      inRuleStrct->ruleBase[l] = strdup(&r[0]->value[r[0]->len * i]);
      inRuleStrct->action[l]   = strdup(&r[1]->value[r[1]->len * i]);
      inRuleStrct->ruleHead[l] = strdup(&r[2]->value[r[2]->len * i]);
      inRuleStrct->ruleCondition[l] = strdup(&r[3]->value[r[3]->len * i]);
      inRuleStrct->ruleAction[l]    = strdup(&r[4]->value[r[4]->len * i]);
      inRuleStrct->ruleRecovery[l]  = strdup(&r[5]->value[r[5]->len * i]);
      inRuleStrct->ruleId[l] = atol(&r[6]->value[r[6]->len * i]);
      l++;
    }
    genQueryInp.continueInx =  genQueryOut->continueInx;
    freeGenQueryOut (&genQueryOut);
    if (genQueryInp.continueInx  > 0) {
      /* More to come */
      status =  rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
    }
    else
      break;
  }
  inRuleStrct->MaxNumOfRules = l;
  return(0);
}


int
readDVMapStructFromDB(char *dvmBaseName, char *versionStr, rulevardef_t *inDvmStrct, ruleExecInfo_t *rei)
{
  int i,l,status;
  genQueryInp_t genQueryInp;
  genQueryOut_t *genQueryOut = NULL;
  char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
  sqlResult_t *r[5];
  memset(&genQueryInp, 0, sizeof(genQueryInp));
  genQueryInp.maxRows = MAX_SQL_ROWS;

  snprintf(condstr, MAX_NAME_LEN, "= '%s'", dvmBaseName);
  addInxVal(&genQueryInp.sqlCondInp, COL_DVM_BASE_MAP_BASE_NAME, condstr);
  snprintf(condstr2, MAX_NAME_LEN, "= '%s'", versionStr);
  addInxVal(&genQueryInp.sqlCondInp, COL_DVM_BASE_MAP_VERSION, condstr2);
  
  addInxIval(&genQueryInp.selectInp, COL_DVM_EXT_VAR_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_DVM_CONDITION, 1);
  addInxIval(&genQueryInp.selectInp, COL_DVM_INT_MAP_PATH, 1);
  addInxIval(&genQueryInp.selectInp, COL_DVM_ID, ORDER_BY);
  l = inDvmStrct->MaxNumOfDVars;
  status = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);
  while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
    r[0] = getSqlResultByInx (genQueryOut, COL_DVM_EXT_VAR_NAME);
    r[1] = getSqlResultByInx (genQueryOut, COL_DVM_CONDITION);
    r[2] = getSqlResultByInx (genQueryOut, COL_DVM_INT_MAP_PATH);
    r[3] = getSqlResultByInx (genQueryOut, COL_DVM_ID);
    for (i = 0; i<genQueryOut->rowCnt; i++) {
      inDvmStrct->varName[l]   = strdup(&r[0]->value[r[0]->len * i]);
      inDvmStrct->action[l] = strdup(&r[1]->value[r[1]->len * i]);
      inDvmStrct->var2CMap[l] = strdup(&r[2]->value[r[2]->len * i]);
      inDvmStrct->varId[l] = atol(&r[3]->value[r[3]->len * i]);
      l++;
    }
    genQueryInp.continueInx =  genQueryOut->continueInx;
    freeGenQueryOut (&genQueryOut);
    if (genQueryInp.continueInx  > 0) {
      /* More to come */
      status =  rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
    }
    else
      break;
  }
  inDvmStrct->MaxNumOfDVars = l;
  return(0);
}



int
readFNMapStructFromDB(char *fnmBaseName, char *versionStr, fnmapStruct_t *inFnmStrct, ruleExecInfo_t *rei)
{
  int i,l,status;
  genQueryInp_t genQueryInp;
  genQueryOut_t *genQueryOut = NULL;
  char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
  sqlResult_t *r[5];
  memset(&genQueryInp, 0, sizeof(genQueryInp));
  genQueryInp.maxRows = MAX_SQL_ROWS;

  snprintf(condstr, MAX_NAME_LEN, "= '%s'", fnmBaseName);
  addInxVal(&genQueryInp.sqlCondInp, COL_FNM_BASE_MAP_BASE_NAME, condstr);
  snprintf(condstr2, MAX_NAME_LEN, "= '%s'", versionStr);
  addInxVal(&genQueryInp.sqlCondInp, COL_FNM_BASE_MAP_VERSION, condstr2);
  
  addInxIval(&genQueryInp.selectInp, COL_FNM_EXT_FUNC_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_FNM_INT_FUNC_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_FNM_ID, ORDER_BY);

  l = inFnmStrct->MaxNumOfFMaps;
  status = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);
  while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
    r[0] = getSqlResultByInx (genQueryOut, COL_FNM_EXT_FUNC_NAME);
    r[1] = getSqlResultByInx (genQueryOut, COL_FNM_INT_FUNC_NAME);
    r[2] = getSqlResultByInx (genQueryOut, COL_FNM_ID);
    for (i = 0; i<genQueryOut->rowCnt; i++) {
      inFnmStrct->funcName[l]   = strdup(&r[0]->value[r[0]->len * i]);
      inFnmStrct->func2CMap[l] = strdup(&r[1]->value[r[1]->len * i]);
      inFnmStrct->fmapId[l] = atol(&r[2]->value[r[2]->len * i]);
      l++;
    }
    genQueryInp.continueInx =  genQueryOut->continueInx;
    freeGenQueryOut (&genQueryOut);
    if (genQueryInp.continueInx  > 0) {
      /* More to come */
      status =  rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
    }
    else
      break;
  }
  inFnmStrct->MaxNumOfFMaps = l;
  return(0);
}



int
readMsrvcStructFromDB(int inStatus, msrvcStruct_t *inMsrvcStrct, ruleExecInfo_t *rei)
{
  int i,l,status;
  genQueryInp_t genQueryInp;
  genQueryOut_t *genQueryOut = NULL;
  sqlResult_t *r[10];
  memset(&genQueryInp, 0, sizeof(genQueryInp));
  genQueryInp.maxRows = MAX_SQL_ROWS;
  char condstr[MAX_NAME_LEN];

  snprintf(condstr, MAX_NAME_LEN, "= '%i'", inStatus);
  addInxVal(&genQueryInp.sqlCondInp, COL_MSRVC_STATUS, condstr);

  addInxIval(&genQueryInp.selectInp, COL_MSRVC_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_MODULE_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_SIGNATURE, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_VERSION, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_HOST, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_LOCATION, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_LANGUAGE, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_TYPE_NAME, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_STATUS, 1);
  addInxIval(&genQueryInp.selectInp, COL_MSRVC_ID, ORDER_BY);
  
  l = inMsrvcStrct->MaxNumOfMsrvcs;
  status = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);
  while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
    r[0] = getSqlResultByInx (genQueryOut, COL_MSRVC_MODULE_NAME);
    r[1] = getSqlResultByInx (genQueryOut, COL_MSRVC_NAME);
    r[2] = getSqlResultByInx (genQueryOut, COL_MSRVC_SIGNATURE);
    r[3] = getSqlResultByInx (genQueryOut, COL_MSRVC_VERSION);
    r[4] = getSqlResultByInx (genQueryOut, COL_MSRVC_HOST);
    r[5] = getSqlResultByInx (genQueryOut, COL_MSRVC_LOCATION);
    r[6] = getSqlResultByInx (genQueryOut, COL_MSRVC_LANGUAGE);
    r[7] = getSqlResultByInx (genQueryOut, COL_MSRVC_TYPE_NAME);
    r[8] = getSqlResultByInx (genQueryOut, COL_MSRVC_STATUS);
    r[9] = getSqlResultByInx (genQueryOut, COL_MSRVC_ID);
    for (i = 0; i<genQueryOut->rowCnt; i++) {
      inMsrvcStrct->moduleName[l] = strdup(&r[0]->value[r[0]->len * i]);
      inMsrvcStrct->msrvcName[l]   = strdup(&r[1]->value[r[1]->len * i]);
      inMsrvcStrct->msrvcSignature[l] = strdup(&r[2]->value[r[2]->len * i]);
      inMsrvcStrct->msrvcVersion[l] = strdup(&r[3]->value[r[3]->len * i]);
      inMsrvcStrct->msrvcHost[l]    = strdup(&r[4]->value[r[4]->len * i]);
      inMsrvcStrct->msrvcLocation[l]  = strdup(&r[5]->value[r[5]->len * i]);
      inMsrvcStrct->msrvcLanguage[l]  = strdup(&r[6]->value[r[6]->len * i]);
      inMsrvcStrct->msrvcTypeName[l]  = strdup(&r[7]->value[r[7]->len * i]);
      inMsrvcStrct->msrvcStatus[l]  = atol(&r[8]->value[r[8]->len * i]);
      inMsrvcStrct->msrvcId[l]  = atol(&r[9]->value[r[9]->len * i]);
      l++;
    }
    genQueryInp.continueInx =  genQueryOut->continueInx;
    freeGenQueryOut (&genQueryOut);
    if (genQueryInp.continueInx  > 0) {
      /* More to come */
      status =  rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
    }
    else
      break;
  }
  inMsrvcStrct->MaxNumOfMsrvcs = l;
  return(0);
}

int
readRuleStructFromFile(char *ruleBaseName, ruleStruct_t *inRuleStrct)
{
  int i = 0;
  char l0[MAX_RULE_LENGTH];
  char l1[MAX_RULE_LENGTH];
  char l2[MAX_RULE_LENGTH];
  char l3[MAX_RULE_LENGTH];

   char rulesFileName[MAX_NAME_LEN];
   FILE *file;
   char buf[MAX_RULE_LENGTH];
   char *configDir;
   char *t;
   i = inRuleStrct->MaxNumOfRules;

   if (ruleBaseName[0] == '/' || ruleBaseName[0] == '\\' ||
       ruleBaseName[1] == ':') {
     snprintf (rulesFileName,MAX_NAME_LEN, "%s",ruleBaseName);
   }
   else {
     configDir = getConfigDir ();
     snprintf (rulesFileName,MAX_NAME_LEN, "%s/reConfigs/%s.irb", configDir,ruleBaseName);
   }     
   file = fopen(rulesFileName, "r");
   if (file == NULL) {
     rodsLog(LOG_NOTICE,
	     "readRuleStructFromFile() could not open rules file %s\n",
	     rulesFileName);
     return(RULES_FILE_READ_ERROR);
   }
   buf[MAX_RULE_LENGTH-1]='\0';
   while (fgets (buf, MAX_RULE_LENGTH-1, file) != NULL) {
     if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
     if (buf[0] == '#' || strlen(buf) < 4) 
       continue;
     rSplitStr(buf, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inRuleStrct->action[i] = strdup(l1);  /** action **/
     inRuleStrct->ruleHead[i] = strdup(l1);
     if ((t = strstr(inRuleStrct->action[i],"(")) != NULL) {
       *t = '\0';
     }
     /*****     rSplitStr(l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|'); 
		inRuleStrct->ruleHead[i] = strdup(l1);
		rSplitStr(l2, l1, MAX_RULE_LENGTH, l3, MAX_RULE_LENGTH,'|');
      *****/
     inRuleStrct->ruleBase[i] = strdup(ruleBaseName);
     rSplitStr(l0, l1, MAX_RULE_LENGTH, l3, MAX_RULE_LENGTH,'|');
     inRuleStrct->ruleCondition[i] = strdup(l1); /** condition **/
     rSplitStr(l3, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|');
     inRuleStrct->ruleAction[i] = strdup(l1);  /** function calls **/
     rSplitStr(l2, l1, MAX_RULE_LENGTH, l3, MAX_RULE_LENGTH,'|');
     inRuleStrct->ruleRecovery[i] = strdup(l1);  /** rollback calls **/
     if (strlen(l3) > 0)
       inRuleStrct->ruleId[i] = atoll(l3);   /** ruleId **/
     else
       inRuleStrct->ruleId[i] = (long int) i;  /** ruleId **/
     i++;
   }
   fclose (file);
   inRuleStrct->MaxNumOfRules = i;
   return(0);
}

int
clearRuleStruct(ruleStruct_t *inRuleStrct)
{
  int i;
  for (i = 0 ; i < inRuleStrct->MaxNumOfRules ; i++) {
    if (inRuleStrct->ruleBase[i]  != NULL)
      free(inRuleStrct->ruleBase[i]);
    if (inRuleStrct->ruleHead[i]  != NULL)
      free(inRuleStrct->ruleHead[i]);
    if (inRuleStrct->ruleCondition[i]  != NULL)
      free(inRuleStrct->ruleCondition[i]);
    if (inRuleStrct->ruleAction[i]  != NULL)
      free(inRuleStrct->ruleAction[i]);
    if (inRuleStrct->ruleRecovery[i]  != NULL)
      free(inRuleStrct->ruleRecovery[i]);
    
  }
  inRuleStrct->MaxNumOfRules  = 0;
  return(0);
}

int clearDVarStruct(rulevardef_t *inRuleVarDef) 
{
  int i;
  for (i = 0 ; i < inRuleVarDef->MaxNumOfDVars; i++) {
    if (inRuleVarDef->varName[i] != NULL)
      free(inRuleVarDef->varName[i]);
    if (inRuleVarDef->action[i] != NULL)
      free(inRuleVarDef->action[i]);
    if (inRuleVarDef->var2CMap[i] != NULL)
      free(inRuleVarDef->var2CMap[i]);
  }
  inRuleVarDef->MaxNumOfDVars =  0;
  return(0);
}

int clearFuncMapStruct( rulefmapdef_t* inRuleFuncMapDef)
{
  int i;
  for (i = 0 ; i < inRuleFuncMapDef->MaxNumOfFMaps; i++) {
    if (inRuleFuncMapDef->funcName[i] != NULL)
      free(inRuleFuncMapDef->funcName[i]);
    if (inRuleFuncMapDef->func2CMap[i] != NULL)
      free(inRuleFuncMapDef->func2CMap[i]);
  }
  inRuleFuncMapDef->MaxNumOfFMaps = 0;
  return(0);
}


int
readDVarStructFromFile(char *dvarBaseName,rulevardef_t *inRuleVarDef)
{
  int i = 0;
  char l0[MAX_DVAR_LENGTH];
  char l1[MAX_DVAR_LENGTH];
  char l2[MAX_DVAR_LENGTH];
  char l3[MAX_DVAR_LENGTH];
   char dvarsFileName[MAX_NAME_LEN];
   FILE *file;
   char buf[MAX_DVAR_LENGTH];
   char *configDir;

   i = inRuleVarDef->MaxNumOfDVars;

   if (dvarBaseName[0] == '/' || dvarBaseName[0] == '\\' ||
       dvarBaseName[1] == ':') {
     snprintf (dvarsFileName,MAX_NAME_LEN, "%s",dvarBaseName);
   }
   else {
     configDir = getConfigDir ();
     snprintf (dvarsFileName,MAX_NAME_LEN, "%s/reConfigs/%s.dvm", configDir,dvarBaseName);
   }
   file = fopen(dvarsFileName, "r");
   if (file == NULL) {
     rodsLog(LOG_NOTICE,
	     "readDvarStructFromFile() could not open dvm file %s\n",
	     dvarsFileName);
     return(DVARMAP_FILE_READ_ERROR);
   }
   buf[MAX_DVAR_LENGTH-1]='\0';
   while (fgets (buf, MAX_DVAR_LENGTH-1, file) != NULL) {
     if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
     if (buf[0] == '#' || strlen(buf) < 4) 
       continue;
     rSplitStr(buf, l1, MAX_DVAR_LENGTH, l0, MAX_DVAR_LENGTH, '|');
     inRuleVarDef->varName[i] = strdup(l1);  /** varName **/
     rSplitStr(l0, l1, MAX_DVAR_LENGTH, l3, MAX_DVAR_LENGTH,'|');
     inRuleVarDef->action[i] = strdup(l1); /** action **/
     rSplitStr(l3, l1, MAX_DVAR_LENGTH, l2, MAX_DVAR_LENGTH,'|');
     inRuleVarDef->var2CMap[i] = strdup(l1);  /** var2CMap **/
     if (strlen(l2) > 0)
       inRuleVarDef->varId[i] = atoll(l2);   /** varId **/
     else
       inRuleVarDef->varId[i] = i; /** ruleId **/
     i++;
   }
   fclose (file);
   inRuleVarDef->MaxNumOfDVars = (long int)  i;
   return(0);
}

int
readFuncMapStructFromFile(char *fmapBaseName, rulefmapdef_t* inRuleFuncMapDef)
{
  int i = 0;
  char l0[MAX_FMAP_LENGTH];
  char l1[MAX_FMAP_LENGTH];
  char l2[MAX_FMAP_LENGTH];
  /*  char l3[MAX_FMAP_LENGTH];*/
   char fmapsFileName[MAX_NAME_LEN];
   FILE *file;
   char buf[MAX_FMAP_LENGTH];
   char *configDir;

   i = inRuleFuncMapDef->MaxNumOfFMaps;

   if (fmapBaseName[0] == '/' || fmapBaseName[0] == '\\' ||
       fmapBaseName[1] == ':') {
     snprintf (fmapsFileName,MAX_NAME_LEN, "%s",fmapBaseName);
   }
   else {
     configDir = getConfigDir ();
     snprintf (fmapsFileName,MAX_NAME_LEN, "%s/reConfigs/%s.fnm", configDir,fmapBaseName);
   }
   file = fopen(fmapsFileName, "r");
   if (file == NULL) {
     rodsLog(LOG_NOTICE,
	     "readFmapStructFromFile() could not open fnm file %s\n",
	     fmapsFileName);
     return(FMAP_FILE_READ_ERROR);
   }
   buf[MAX_FMAP_LENGTH-1]='\0';
   while (fgets (buf, MAX_FMAP_LENGTH-1, file) != NULL) {
     if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
     if (buf[0] == '#' || strlen(buf) < 4) 
       continue;
     rSplitStr(buf, l1, MAX_FMAP_LENGTH, l0, MAX_FMAP_LENGTH, '|');
     inRuleFuncMapDef->funcName[i] = strdup(l1);  /** funcName **/
     rSplitStr(l0,l1, MAX_FMAP_LENGTH, l2, MAX_FMAP_LENGTH,'|');
     inRuleFuncMapDef->func2CMap[i] = strdup(l1);  /** func2CMap **/
     if (strlen(l2) > 0)
       inRuleFuncMapDef->fmapId[i] = atoll(l2);   /** fmapId **/
     else
       inRuleFuncMapDef->fmapId[i] = i;  /** fmapId **/
     i++;
   }
   fclose (file);
   inRuleFuncMapDef->MaxNumOfFMaps = (long int) i;
   return(0);
}

int
readMsrvcStructFromFile(char *msrvcFileName, msrvcStruct_t* inMsrvcStruct)
{
  int i = 0;
  char l0[MAX_RULE_LENGTH];
  char l1[MAX_RULE_LENGTH];
  char l2[MAX_RULE_LENGTH];
  char mymsrvcFileName[MAX_NAME_LEN];
  FILE *file;
  char buf[MAX_RULE_LENGTH];
  char *configDir;


   i = inMsrvcStruct->MaxNumOfMsrvcs;

   if (msrvcFileName[0] == '/' || msrvcFileName[0] == '\\' ||
       msrvcFileName[1] == ':') {
     snprintf (mymsrvcFileName,MAX_NAME_LEN, "%s",msrvcFileName);
   }
   else {
     configDir = getConfigDir ();
     snprintf (mymsrvcFileName,MAX_NAME_LEN, "%s/reConfigs/%s.msi", configDir,msrvcFileName);
   }
   file = fopen(mymsrvcFileName, "r");
   if (file == NULL) {
     rodsLog(LOG_NOTICE,
	     "readMservcStructFromFile() could not open msrvc file %s\n",
	     mymsrvcFileName);
     return(MSRVC_FILE_READ_ERROR);
   }
  buf[MAX_RULE_LENGTH-1]='\0';
   while (fgets (buf, MAX_RULE_LENGTH-1, file) != NULL) {
     if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
     if (buf[0] == '#' || strlen(buf) < 4) 
       continue;
     rSplitStr(buf, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inMsrvcStruct->moduleName[i] = strdup(l1);  

     rSplitStr(l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH,'|');
     inMsrvcStruct->msrvcName[i] = strdup(l1);
     rSplitStr(l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inMsrvcStruct->msrvcSignature[i] = strdup(l1);  

     rSplitStr(l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH,'|');
     inMsrvcStruct->msrvcVersion[i] = strdup(l1);
     rSplitStr(l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inMsrvcStruct->msrvcHost[i] = strdup(l1);

     rSplitStr(l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH,'|');
     inMsrvcStruct->msrvcLocation[i] = strdup(l1);
     rSplitStr(l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inMsrvcStruct->msrvcLanguage[i] = strdup(l1);

     rSplitStr(l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH,'|');
     inMsrvcStruct->msrvcTypeName[i] = strdup(l1);
     rSplitStr(l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|');
     inMsrvcStruct->msrvcStatus[i] = atol(l1);
     if (strlen(l0) > 0)  
       inMsrvcStruct->msrvcId[i] = atol(l1);
     else 
       inMsrvcStruct->msrvcId[i] = (long int) i;
     i++;
   }
   fclose (file);
   inMsrvcStruct->MaxNumOfMsrvcs = i;
   return(0);
}

int
findNextRule (char *action,  int *ruleInx)
{
  int i;
   i = *ruleInx;
   i++;
   if (i < 0)
     i = 0;
   if (i < 1000) {
     for( ; i < appRuleStrct.MaxNumOfRules; i++) {
       if (!strcmp( appRuleStrct.action[i],action)) {
	 *ruleInx = i;
	 return(0);
       }
     }
     i = 1000;
   }
   i  = i - 1000;
   for( ; i < coreRuleStrct.MaxNumOfRules; i++) {
     if (!strcmp( coreRuleStrct.action[i],action)) {
       *ruleInx = i + 1000;
       return(0);
     }
   }
   return(NO_MORE_RULES_ERR);
}



int
getRule(int ri, char *ruleBase, char *ruleHead, char *ruleCondition, 
	char *ruleAction, char *ruleRecovery, int rSize)
{

  if (ri < 1000) {
    rstrcpy( ruleBase , appRuleStrct.ruleBase[ri], rSize);
    rstrcpy( ruleHead , appRuleStrct.ruleHead[ri], rSize);
    rstrcpy( ruleCondition , appRuleStrct.ruleCondition[ri], rSize);
    rstrcpy( ruleAction , appRuleStrct.ruleAction[ri], rSize);
    rstrcpy( ruleRecovery , appRuleStrct.ruleRecovery[ri], rSize);
  }
  else {
    ri = ri - 1000;
    rstrcpy( ruleBase , coreRuleStrct.ruleBase[ri], rSize);
    rstrcpy( ruleHead , coreRuleStrct.ruleHead[ri], rSize);
    rstrcpy( ruleCondition , coreRuleStrct.ruleCondition[ri], rSize);
    rstrcpy( ruleAction , coreRuleStrct.ruleAction[ri], rSize);
    rstrcpy( ruleRecovery , coreRuleStrct.ruleRecovery[ri], rSize);
  }
  return(0);
}








int
initializeMsParam(char *ruleHead, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	  ruleExecInfo_t *rei)
{

  msParam_t *mP;
  char tmpStr[NAME_LEN];
  char *args2[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc2 = 0;
  int i;

  /* save the old msParamArray in rei */
#ifdef ADDR_64BITS
  snprintf(tmpStr,NAME_LEN, "%lld", (rodsLong_t) rei->msParamArray);  /* pointer stored as long integer */
#else
  snprintf(tmpStr,NAME_LEN, "%d", (uint) rei->msParamArray);  /* pointer stored as long integer */
#endif
  pushStack(&msParamStack,tmpStr);            /* pointer->integer->string stored in stack */
  
  /* make a new msParamArray in rei */
  rei->msParamArray = (msParamArray_t*)malloc(sizeof(msParamArray_t));
  rei->msParamArray->len = 0;
  rei->msParamArray->msParam = NULL;

  parseAction(ruleHead, tmpStr,args2, &argc2);
  
  if (argc < argc2)
    return(INSUFFICIENT_INPUT_ARG_ERR);

  /* stick things into msParamArray in rei */
  for (i = 0; i < argc2 ; i++) {
    if (args[i][0] == '*') {
      /* this is an output and hence an empty struct is added here */
      addMsParam(rei->msParamArray, args2[i], "", NULL,NULL);
    }
    else {
      mP = (msParam_t *) args[i];
      addMsParam(rei->msParamArray, args2[i], "int *msParam", mP, NULL);
    }
  }
  freeRuleArgs (args2, argc2);

  return(0);
  
}

int
finalizeMsParam(char *ruleHead, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	  ruleExecInfo_t *rei, int status)
{

  msParamArray_t *oldMsParamArray;
  char tmpStr[NAME_LEN];
  msParam_t *mP;
  int i;
  char *args2[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc2 = 0;

  parseAction(ruleHead, tmpStr,args2, &argc2);

  /* get the old msParamArray  */
  popStack(&msParamStack,tmpStr);   
#ifdef ADDR_64BITS
  oldMsParamArray = (msParamArray_t *) strtoll (tmpStr, 0, 0);
#else
  oldMsParamArray = (msParamArray_t *) atoi(tmpStr);
#endif

  for (i = 0; i < argc; i++) {
    if (args[i][0] == '*') { /* it has not been bound  and hence an output */
      mP = getMsParamByLabel (rei->msParamArray, args2[i]);
      rmMsParamByLabel (oldMsParamArray, args[i], 0);
      addMsParam(oldMsParamArray, args[i], mP->type, mP->inOutStruct, mP->inpOutBuf);
    }
    else {
      /* currently nothing, should I also copy fromnew to old
	 because something changed. for the present it is considered a constant */
    }
  }
  freeRuleArgs (args2, argc2);

  free(rei->msParamArray);
  rei->msParamArray = oldMsParamArray;
  return(0);
}




int
insertRulesIntoDB(char * baseName, ruleStruct_t *coreRuleStruct,
		  ruleExecInfo_t *rei)
{
  generalRowInsertInp_t generalRowInsertInp;
  char ruleIdStr[MAX_NAME_LEN];
  int rc1, i;
  int  mapPriorityInt = 1;
  char mapPriorityStr[50];
  endTransactionInp_t endTransactionInp;
  char myTime[50];

  memset (&endTransactionInp, 0, sizeof (endTransactionInp_t));
  getNowStr(myTime);

  /* Before inserting rules and its base map, we need to first version out the base map */
  generalRowInsertInp.tableName = "versionRuleBase";
  generalRowInsertInp.arg1 = baseName;
  generalRowInsertInp.arg2 = myTime;

  rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
  if (rc1 < 0) {
    endTransactionInp.arg0 = "rollback";
    rsEndTransaction(rei->rsComm, &endTransactionInp);
    return(rc1);
  }

  for (i = 0; i < coreRuleStruct->MaxNumOfRules; i++) {
    generalRowInsertInp.tableName = "ruleTable";
    generalRowInsertInp.arg1 = baseName;
    sprintf(mapPriorityStr, "%i", mapPriorityInt);
    mapPriorityInt++;
    generalRowInsertInp.arg2 = mapPriorityStr;
    generalRowInsertInp.arg3 = coreRuleStruct->action[i];
    generalRowInsertInp.arg4 = coreRuleStruct->ruleHead[i];
    generalRowInsertInp.arg5 = coreRuleStruct->ruleCondition[i];
    generalRowInsertInp.arg6 = coreRuleStruct->ruleAction[i];
    generalRowInsertInp.arg7= coreRuleStruct->ruleRecovery[i];
    generalRowInsertInp.arg8 = ruleIdStr;
    generalRowInsertInp.arg9 = myTime;

    rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
    if (rc1 < 0) {
      endTransactionInp.arg0 = "rollback";
      rsEndTransaction(rei->rsComm, &endTransactionInp);
      return(rc1);
    }
  }

  endTransactionInp.arg0 = "commit";
  rc1 = rsEndTransaction(rei->rsComm, &endTransactionInp);
  return(rc1);
}


int
insertDVMapsIntoDB(char * baseName, dvmStruct_t *coreDVMStruct,
		  ruleExecInfo_t *rei)
{
  generalRowInsertInp_t generalRowInsertInp;
  int rc1, i;
  endTransactionInp_t endTransactionInp;
  char myTime[50];

  memset (&endTransactionInp, 0, sizeof (endTransactionInp_t));
  getNowStr(myTime);

  /* Before inserting rules and its base map, we need to first version out the base map */
  generalRowInsertInp.tableName = "versionDVMBase";
  generalRowInsertInp.arg1 = baseName;
  generalRowInsertInp.arg2 = myTime;

  rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
  if (rc1 < 0) {
    endTransactionInp.arg0 = "rollback";
    rsEndTransaction(rei->rsComm, &endTransactionInp);
    return(rc1);
  }

  for (i = 0; i < coreDVMStruct->MaxNumOfDVars; i++) {
    generalRowInsertInp.tableName = "dvmTable";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = coreDVMStruct->varName[i];
    generalRowInsertInp.arg3 = coreDVMStruct->action[i];
    generalRowInsertInp.arg4 = coreDVMStruct->var2CMap[i];
    generalRowInsertInp.arg5 = myTime;

    rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
    if (rc1 < 0) {
      endTransactionInp.arg0 = "rollback";
      rsEndTransaction(rei->rsComm, &endTransactionInp);
      return(rc1);
    }
  }

  endTransactionInp.arg0 = "commit";
  rc1 = rsEndTransaction(rei->rsComm, &endTransactionInp);
  return(rc1);
}

int
insertFNMapsIntoDB(char * baseName, fnmapStruct_t *coreFNMStruct,
		  ruleExecInfo_t *rei)
{
  generalRowInsertInp_t generalRowInsertInp;
  int rc1, i;
  endTransactionInp_t endTransactionInp;
  char myTime[50];

  memset (&endTransactionInp, 0, sizeof (endTransactionInp_t));
  getNowStr(myTime);

  /* Before inserting rules and its base map, we need to first version out the base map */
  generalRowInsertInp.tableName = "versionFNMBase";
  generalRowInsertInp.arg1 = baseName;
  generalRowInsertInp.arg2 = myTime;

  rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
  if (rc1 < 0) {
    endTransactionInp.arg0 = "rollback";
    rsEndTransaction(rei->rsComm, &endTransactionInp);
    return(rc1);
  }

  for (i = 0; i < coreFNMStruct->MaxNumOfFMaps; i++) {
    generalRowInsertInp.tableName = "fnmTable";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = coreFNMStruct->funcName[i];
    generalRowInsertInp.arg3 = coreFNMStruct->func2CMap[i];
    generalRowInsertInp.arg4 = myTime;

    rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
    if (rc1 < 0) {
      endTransactionInp.arg0 = "rollback";
      rsEndTransaction(rei->rsComm, &endTransactionInp);
      return(rc1);
    }
  }

  endTransactionInp.arg0 = "commit";
  rc1 = rsEndTransaction(rei->rsComm, &endTransactionInp);
  return(rc1);
}



int
insertMSrvcsIntoDB(msrvcStruct_t *coreMsrvcStruct,
		  ruleExecInfo_t *rei)
{

  generalRowInsertInp_t generalRowInsertInp;
  int rc1, i;
  endTransactionInp_t endTransactionInp;
  char myTime[100];
  char myStatus[100]; 
  memset (&endTransactionInp, 0, sizeof (endTransactionInp_t));
  getNowStr(myTime);

  for (i = 0; i < coreMsrvcStruct->MaxNumOfMsrvcs; i++) {
    generalRowInsertInp.tableName = "msrvcTable";
    generalRowInsertInp.arg1 = coreMsrvcStruct->moduleName[i];
    generalRowInsertInp.arg2 = coreMsrvcStruct->msrvcName[i];
    generalRowInsertInp.arg3 = coreMsrvcStruct->msrvcSignature[i];
    generalRowInsertInp.arg4 = coreMsrvcStruct->msrvcVersion[i];
    generalRowInsertInp.arg5 = coreMsrvcStruct->msrvcHost[i];
    generalRowInsertInp.arg6 = coreMsrvcStruct->msrvcLocation[i];
    generalRowInsertInp.arg7 = coreMsrvcStruct->msrvcLanguage[i];
    generalRowInsertInp.arg8 = coreMsrvcStruct->msrvcTypeName[i];
    snprintf(myStatus,100, "%ld", coreMsrvcStruct->msrvcStatus[i]);
    generalRowInsertInp.arg9 = myStatus;
    generalRowInsertInp.arg10 = myTime;


    rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
    if (rc1 < 0) {
      endTransactionInp.arg0 = "rollback";
      rsEndTransaction(rei->rsComm, &endTransactionInp);
      return(rc1);
    }
  }

  endTransactionInp.arg0 = "commit";
  rc1 = rsEndTransaction(rei->rsComm, &endTransactionInp);
  return(rc1);

}


int
writeRulesIntoFile(char * inFileName, ruleStruct_t *myRuleStruct,
                  ruleExecInfo_t *rei)
{
  int i;
  FILE *file;
  char fileName[MAX_NAME_LEN];
  char *configDir;

  if (inFileName[0] == '/' || inFileName[0] == '\\' ||
      inFileName[1] == ':') {
    snprintf (fileName,MAX_NAME_LEN, "%s",inFileName);
  }
  else {
    configDir = getConfigDir ();
    snprintf (fileName,MAX_NAME_LEN, "%s/reConfigs/%s.irb", configDir,inFileName);
  }

  file = fopen(fileName, "w");
  if (file == NULL) {
    rodsLog(LOG_NOTICE,
	    "writeRulesIntoFile() could not open rules file %s for writing\n",
	    fileName);
    return(FILE_OPEN_ERR);
  }
  for( i = 0; i < myRuleStruct->MaxNumOfRules; i++) {
    fprintf(file, "%s|%s|%s|%s|%ld\n", myRuleStruct->ruleHead[i],
	    myRuleStruct->ruleCondition[i],
	    myRuleStruct->ruleAction[i],
	    myRuleStruct->ruleRecovery[i],
	    myRuleStruct->ruleId[i]);
            
  }
  fclose (file);
  return(0);
}

int
writeDVMapsIntoFile(char * inFileName, dvmStruct_t *myDVMapStruct,
                  ruleExecInfo_t *rei)
{
  int i;
  FILE *file;
  char fileName[MAX_NAME_LEN];
  char *configDir;

  if (inFileName[0] == '/' || inFileName[0] == '\\' ||
      inFileName[1] == ':') {
    snprintf (fileName,MAX_NAME_LEN, "%s",inFileName);
  }
  else {
    configDir = getConfigDir ();
    snprintf (fileName,MAX_NAME_LEN, "%s/reConfigs/%s.dvm", configDir,inFileName);
  }

  file = fopen(fileName, "w");
  if (file == NULL) {
    rodsLog(LOG_NOTICE,
	    "writeDVMapsIntoFile() could not open rules file %s for writing\n",
	    fileName);
    return(FILE_OPEN_ERR);
  }
  for( i = 0; i < myDVMapStruct->MaxNumOfDVars; i++) {
    fprintf(file, "%s|%s|%s|%ld\n", myDVMapStruct->varName[i],
	    myDVMapStruct->action[i],
	    myDVMapStruct->var2CMap[i],
	    myDVMapStruct->varId[i]);
  }
  fclose (file);
  return(0);
}


int
writeFNMapsIntoFile(char * inFileName, fnmapStruct_t *myFNMapStruct,
                  ruleExecInfo_t *rei)
{
  int i;
  FILE *file;
  char fileName[MAX_NAME_LEN];
  char *configDir;

  if (inFileName[0] == '/' || inFileName[0] == '\\' ||
      inFileName[1] == ':') {
    snprintf (fileName,MAX_NAME_LEN, "%s",inFileName);
  }
  else {
    configDir = getConfigDir ();
    snprintf (fileName,MAX_NAME_LEN, "%s/reConfigs/%s.fnm", configDir,inFileName);
  }

  file = fopen(fileName, "w");
  if (file == NULL) {
    rodsLog(LOG_NOTICE,
	    "writeFNMapsIntoFile() could not open rules file %s for writing\n",
	    fileName);
    return(FILE_OPEN_ERR);
  }
  for( i = 0; i < myFNMapStruct->MaxNumOfFMaps; i++) {
    fprintf(file, "%s|%s|%ld\n", myFNMapStruct->funcName[i],
	    myFNMapStruct->func2CMap[i],
	    myFNMapStruct->fmapId[i]);
  }
  fclose (file);
  return(0);
}



int
writeMSrvcsIntoFile(char * inFileName, msrvcStruct_t *myMsrvcStruct,
                  ruleExecInfo_t *rei)
{
  int i;
  FILE *file;
  char fileName[MAX_NAME_LEN];
  char *configDir;

  if (inFileName[0] == '/' || inFileName[0] == '\\' ||
      inFileName[1] == ':') {
    snprintf (fileName,MAX_NAME_LEN, "%s",inFileName);
  }
  else {
    configDir = getConfigDir ();
    snprintf (fileName,MAX_NAME_LEN, "%s/reConfigs/%s.msi", configDir,inFileName);
  }

  file = fopen(fileName, "w");
  if (file == NULL) {
    rodsLog(LOG_NOTICE,
	    "writeMsrvcsIntoFile() could not open microservics file %s for writing\n",
	    fileName);
    return(FILE_OPEN_ERR);
  }
  for( i = 0; i < myMsrvcStruct->MaxNumOfMsrvcs; i++) {
    fprintf(file, "%s|%s|%s|%s|%s|%s|%s|%s|%ld|%ld\n", 
	    myMsrvcStruct->moduleName[i],
	    myMsrvcStruct->msrvcName[i],
	    myMsrvcStruct->msrvcSignature[i],
	    myMsrvcStruct->msrvcVersion[i],
	    myMsrvcStruct->msrvcHost[i],
	    myMsrvcStruct->msrvcLocation[i],
	    myMsrvcStruct->msrvcLanguage[i],
	    myMsrvcStruct->msrvcTypeName[i],
	    myMsrvcStruct->msrvcStatus[i],
	    myMsrvcStruct->msrvcId[i]);
  }
  fclose (file);
  return(0);
}

int
finalzeRuleEngine(rsComm_t *rsComm)
{
  if ( GlobalREDebugFlag > 5 ) {
    _writeXMsg(GlobalREDebugFlag, "idbug", "PROCESS END");
  }
  return(0);
}


