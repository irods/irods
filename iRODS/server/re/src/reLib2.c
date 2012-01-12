/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"
#include "reAction.h"
#include "reHelpers1.h"

int
executeRuleBody(char *action, char *ruleAction, char *ruleRecovery, 
		char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		ruleExecInfo_t *rei, int reiSaveFlag )
{
  int i,j,k,n;
  char *actionArray[MAX_ACTION_IN_RULE];
  char *recoveryArray[MAX_ACTION_IN_RULE];
  int cutFlag = 0;

  i = replaceVariables(action, ruleAction,args,argc,rei);
  if (i != 0)
    return(i);
  i = replaceVariables(action, ruleRecovery,args,argc,rei);
  if (i != 0)
    return(i);
  

  n = getActionRecoveryList(ruleAction,ruleRecovery,actionArray,recoveryArray);
  if (n <= 0)
    return(n);
  for (i = 0; i < n; i++) {
    if (!strcmp(actionArray[i],"cut")) {
      if (reTestFlag > 0) {
	if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
	  fprintf(stdout,"!!!Processing a cut\n");
	else if (reTestFlag == HTML_TEST_1 || HTML_TEST_MSI)
	    fprintf(stdout,"<FONT COLOR=#FF0000>!!!Processing a cut</FONT><BR>\n");
	else  if (reTestFlag == LOG_TEST_1)
	   if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	     rodsLog (LOG_NOTICE,"!!!Processing a cut\n");
      }
      cutFlag = 1;
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"nop") || !strcmp(actionArray[i],"null")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"succeed")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"fail")) {
      j = FAIL_ACTION_ENCOUNTERED_ERR;
      break;
    }
    j = executeRuleAction(actionArray[i], rei, reiSaveFlag);
    if (j < 0) 
      break;
  }
  if (j == BREAK_ACTION_ENCOUNTERED_ERR)
    return(j);
  if (j == RETRY_WITHOUT_RECOVERY_ERR)
    return(j);
  if (j < 0) {
    sprintf(tmpStr,"executeRuleAction Failed for %s",actionArray[i]);
    rodsLogError(LOG_ERROR,j,tmpStr);
    rodsLog (LOG_NOTICE,"executeRuleBody: Micro-service or Action %s Failed with status %i",actionArray[i],j);
    for ( ; i >= 0 ; i--) {
      if (!strcmp(recoveryArray[i],"nop"))
	continue;
      k = executeRuleRecovery(recoveryArray[i], rei, reiSaveFlag);
      if (k < 0) {
	  sprintf(tmpStr,"executeRuleRecovery Failed for %s",recoveryArray[i]);
	  rodsLogError(LOG_ERROR,k,tmpStr);
	  if (cutFlag== 1)
	    return(CUT_ACTION_PROCESSED_ERR);
	  else
	    return(k);
      }
    }
    if (cutFlag == 1)
      return(CUT_ACTION_PROCESSED_ERR);
    else
      return(j);
  }
  return(0);
}

int
freeActionRecoveryList(char *actionArray[], char *recoveryArray[], int n)
{
  int i;
  for (i = 0; i < n; i++) {
    if (actionArray[i] != NULL) {
      free (actionArray[i]);
    }
    if (recoveryArray[i] != NULL) {
      free (recoveryArray[i]);
    }
  }
  return(0);
}

int
executeRuleBodyNew(char *action, char *ruleAction, char *ruleRecovery, 
		msParamArray_t *inMsParamArray,
		ruleExecInfo_t *rei, int reiSaveFlag )
{
  int i,j,k,n;
  char *actionArray[MAX_ACTION_IN_RULE];
  char *recoveryArray[MAX_ACTION_IN_RULE];
  int cutFlag = 0;

  /*****
  i = replaceVariablesNew(action, ruleAction, inMsParamArray, rei);
  if (i != 0)
    return(i);
  i = replaceVariablesNew(action, ruleRecovery, inMsParamArray, rei);
  if (i != 0)
    return(i);
  ****/
  n = getActionRecoveryList(ruleAction,ruleRecovery,actionArray,recoveryArray);
  if (n <= 0)
    return(n);


  for (i = 0; i < n; i++) {
    if (!strcmp(actionArray[i],"cut")) {
      if (reTestFlag > 0) {
	if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
	  fprintf(stdout,"!!!Processing a cut\n");
	else if (reTestFlag == HTML_TEST_1 || HTML_TEST_MSI)
	    fprintf(stdout,"<FONT COLOR=#FF0000>!!!Processing a cut</FONT><BR>\n");
	else  if (reTestFlag == LOG_TEST_1)
	   if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	     rodsLog (LOG_NOTICE,"!!!Processing a cut\n");
      }
      cutFlag = 1;
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"nop")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"succeed")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"fail")) {
      j = FAIL_ACTION_ENCOUNTERED_ERR;
      break;
    }
    j  = replaceVariablesNew(action, actionArray[i], inMsParamArray, rei);
    if (j != 0) {
      freeActionRecoveryList(actionArray,recoveryArray,n);
      return(j);
    }
    j = executeRuleActionNew(actionArray[i], inMsParamArray, rei, reiSaveFlag);
    if (j < 0) 
      break;
  }
  if (j == BREAK_ACTION_ENCOUNTERED_ERR) {
    freeActionRecoveryList(actionArray,recoveryArray,n);
    return(j);
  }
  if (j == RETRY_WITHOUT_RECOVERY_ERR) {
    freeActionRecoveryList(actionArray,recoveryArray,n);
    return(j);
  }
  if (j < 0) {
    sprintf(tmpStr,"executeRuleAction Failed for %s",actionArray[i]);
    rodsLogError(LOG_ERROR,j,tmpStr);
    rodsLog (LOG_NOTICE,"executeRuleBody: Micro-service or Action %s Failed with status %i",actionArray[i],j);
    for ( ; i >= 0 ; i--) {
      if (!strcmp(recoveryArray[i],"nop") || !strcmp(recoveryArray[i],"null"))
	continue;
      k  = replaceVariablesNew(action, recoveryArray[i], inMsParamArray, rei);
      if (k != 0) {
	freeActionRecoveryList(actionArray,recoveryArray,n);
	return(k);
      }
      k = executeRuleRecoveryNew(recoveryArray[i], inMsParamArray, rei, reiSaveFlag);
      if (k < 0) {
	  sprintf(tmpStr,"executeRuleRecovery Failed for %s",recoveryArray[i]);
	  rodsLogError(LOG_ERROR,k,tmpStr);
	  freeActionRecoveryList(actionArray,recoveryArray,n);
	  if (cutFlag== 1)
	    return(CUT_ACTION_PROCESSED_ERR);
	  else
	    return(k);
      }
    }
    freeActionRecoveryList(actionArray,recoveryArray,n);
    if (cutFlag == 1)
      return(CUT_ACTION_PROCESSED_ERR);
    else
      return(j);
  }
  /* fix mem leak mw 02/07/07 */
  for (i = 0; i < n; i++) {
    if (actionArray[i] != NULL) {
      free (actionArray[i]);
    }
    if (recoveryArray[i] != NULL) {
      free (recoveryArray[i]);
    }
  }
  if (cutFlag == 1)
    return(CUT_ACTION_ON_SUCCESS_PROCESSED_ERR);
  /** this allows use of cut as the last msi in the body so that other rules will not be processed
      even when the current rule succeeds **/
  return(0);
}

int
executeMyRuleBody(char *action, char *ruleAction, char *ruleRecovery, 
		msParamArray_t *inMsParamArray,
		ruleExecInfo_t *rei, int reiSaveFlag )
{
  int i,j,k,n;
  char *actionArray[MAX_ACTION_IN_RULE];
  char *recoveryArray[MAX_ACTION_IN_RULE];
  int cutFlag = 0;


  /*****
  i = replaceVariablesNew(action, ruleAction, inMsParamArray, rei);
  if (i != 0)
    return(i);
  i = replaceVariablesNew(action, ruleRecovery, inMsParamArray,  rei);
  if (i != 0)
    return(i);
  *****/
  n = getActionRecoveryList(ruleAction,ruleRecovery,actionArray,recoveryArray);
  if (n <= 0)
    return(n);



  for (i = 0; i < n; i++) {
    if (!strcmp(actionArray[i],"cut")) {
      if (reTestFlag > 0) {
	if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
	  fprintf(stdout,"!!!Processing a cut\n");
	else if (reTestFlag == HTML_TEST_1 || HTML_TEST_MSI)
	    fprintf(stdout,"<FONT COLOR=#FF0000>!!!Processing a cut</FONT><BR>\n");
	else  if (reTestFlag == LOG_TEST_1)
	   if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	     rodsLog (LOG_NOTICE,"!!!Processing a cut\n");
      }
      cutFlag = 1;
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"nop")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"succeed")) {
      j = 0;
      continue;
    }
    if (!strcmp(actionArray[i],"fail")) {
      j = FAIL_ACTION_ENCOUNTERED_ERR;
      break;
    }
    j  = replaceVariablesNew(action, actionArray[i], inMsParamArray, rei);
    if (j != 0) {
      freeActionRecoveryList(actionArray,recoveryArray,n);
      return(j);    
    }
    /*** RAJA June 20, 2008 -- changed to executeRuleActionNew ***
    j = executeMyRuleAction(actionArray[i], inMsParamArray, rei, reiSaveFlag);
    *** RAJA June 20, 2008 -- changed to executeRuleActionNew ***/
    j = executeRuleActionNew(actionArray[i], inMsParamArray, rei, reiSaveFlag);
    if (j < 0) 
      break;
  }
  if (j == BREAK_ACTION_ENCOUNTERED_ERR) {
    freeActionRecoveryList(actionArray,recoveryArray,n);
    return(j);
  }
  if (j == RETRY_WITHOUT_RECOVERY_ERR) {
    freeActionRecoveryList(actionArray,recoveryArray,n);
    return(j);
  }
  if (j < 0) {
    sprintf(tmpStr,"executeMyRuleBody: executeRuleAction Failed for %s",actionArray[i]);
    rodsLogError(LOG_ERROR,j,tmpStr);
    rodsLog (LOG_NOTICE,"executeMyRuleBody: Micro-service or Action %s Failed with status %i",actionArray[i],j);
    for ( ; i >= 0 ; i--) {
      if (!strcmp(recoveryArray[i],"nop") || !strcmp(recoveryArray[i],"null"))
	continue;
      k  = replaceVariablesNew(action, recoveryArray[i], inMsParamArray, rei);
      if (k != 0) {
	freeActionRecoveryList(actionArray,recoveryArray,n);
	return(k);
      }
      /*** RAJA June 20, 2008 -- changed to executeMyRuleRecovery ***
      k = executeMyRuleRecovery(recoveryArray[i], inMsParamArray, rei, reiSaveFlag);
      *** RAJA June 20, 2008 -- changed to executeMyRuleRecovery ***/
      executeRuleRecoveryNew(recoveryArray[i], inMsParamArray, rei, reiSaveFlag);
      if (k < 0) {
	  sprintf(tmpStr,"executeMyRuleRecovery Failed for %s",recoveryArray[i]);
	  rodsLogError(LOG_ERROR,k,tmpStr);
	  freeActionRecoveryList(actionArray,recoveryArray,n);
	  if (cutFlag== 1)
	    return(CUT_ACTION_PROCESSED_ERR);
	  else
	    return(k);
      }
    }
    freeActionRecoveryList(actionArray,recoveryArray,n);
    if (cutFlag == 1)
      return(CUT_ACTION_PROCESSED_ERR);
    else
      return(j);
  }
  freeActionRecoveryList(actionArray,recoveryArray,n);
  return(0);
}

int
getActionRecoveryList(char *ruleAction, char *ruleRecovery, 
		      char *actionArray[],char *recoveryArray[]) 
{  
  int i,j,k;
  char *t1, *t2, *t3, *t4;
  char action[MAX_ACTION_SIZE];
  char recovery[MAX_ACTION_SIZE];

  i  = 0; 
  t1 = ruleAction;
  t3 = ruleRecovery;
  action[0] = '\0';
  recovery[0] = '\0';
  j = getNextAction(t1, action, &t2);
  k = getNextAction(t3, recovery, &t4);
  while(j == 0) {
    /*
    actionArray[i] = strdup(action);
    recoveryArray[i] = strdup(recovery);
    */
    actionArray[i] = (char *) malloc(MAX_ACTION_SIZE);
    recoveryArray[i] = (char *) malloc(MAX_ACTION_SIZE);
    strcpy(actionArray[i],action);
    strcpy(recoveryArray[i],recovery);

    i++;
    if (i == MAX_ACTION_IN_RULE)
      return (MAX_NUM_OF_ACTION_IN_RULE_EXCEEDED);
    t1 = t2;
    t3 = t4;
    action[0] = '\0';
    recovery[0] = '\0';
    j = getNextAction(t1, action, &t2);
    if (k == 0)
      k = getNextAction(t3, recovery, &t4);
  }
  return(i);
}

int
executeRuleRecovery(char *ruleRecovery, ruleExecInfo_t *rei, int reiSaveFlag)
{
  if (strlen(ruleRecovery) == 0)
    return(0);
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
      fprintf(stdout,"***RollingBack\n");
    else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"<FONT COLOR=#FF0000>***RollingBack</FONT><BR>\n");
    else  if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"***RollingBack\n");
  }
  return(executeRuleAction(ruleRecovery, rei, reiSaveFlag));
}

int
executeRuleRecoveryNew(char *ruleRecovery, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag)
{
  if (strlen(ruleRecovery) == 0)
    return(0);
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
      fprintf(stdout,"***RollingBack\n");
    else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"<FONT COLOR=#FF0000>***RollingBack</FONT><BR>\n");
    else  if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"***RollingBack\n");
  }
  if (GlobalREAuditFlag)
    reDebug("  RuleRecovery", -2, ruleRecovery,inMsParamArray,rei);

  return(executeRuleActionNew(ruleRecovery, inMsParamArray,  rei, reiSaveFlag));
}

int
executeMyRuleRecovery(char *ruleRecovery, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag)
{
  if (strlen(ruleRecovery) == 0)
    return(0);
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
      fprintf(stdout,"***RollingBack\n");
    else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"<FONT COLOR=#FF0000>***RollingBack</FONT><BR>\n");
    else  if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"***RollingBack\n");
  }
  return(executeMyRuleAction(ruleRecovery, inMsParamArray, rei, reiSaveFlag));
}

/*********
int
parseAction(char *inAction,char *action, char *args[], int *argc)
{
  char  *t, *s, *u;
  int i = 0;



  strcpy(action,inAction);
  if ((t = strstr(action,"(")) != NULL) {
    *t = '\0';
    t++;
    u = t;
    while ((s = strstr(u,",") ) != NULL) {
      if (*(s-1) == '\\') {
	memmove(s-1,s,strlen(s)+1);
	u = s;
	continue;
      }
      *s = '\0';
      if (i == MAX_NUM_OF_ARGS_IN_ACTION)
	return(MAX_NUM_OF_ARGS_IN_ACTION_EXCEEDED);
      args[i] = t;
      i++;
      t = s+1;
      u = t;
    }
    args[i] = t;
    args[i][strlen(args[i])-1] = '\0';
    i++;
  }
  *argc = i;
  return(0);
}
**********/
int
parseAction(char *inAction,char *action, char *args[], int *argc)
{
  char  *t, *s;
  int i = 0;
  /*int j,k;*/
  char *act;
  char *e,  *t1;
  /*char t2;*/
  int found = 0;

  act = strdup(inAction);
  if ((t = strstr(act,"(")) != NULL) {
    *t = '\0';
    t++;
    s = t;
    e = t;
    strcpy(action,act);
    while  (found == 0) {
      if (i == MAX_NUM_OF_ARGS_IN_ACTION)
	return(MAX_NUM_OF_ARGS_IN_ACTION_EXCEEDED);
      if ((t = strstr(s,",")) != NULL ) {
	*t = '\0';
	t1 = t + 1;
	if (goodExpr(e) == 0) {
#if 0
	  args[i] = e;	/* XXXXX this is why act cannot be freed */
	  trimWS(args[i]);
	  trimQuotes(args[i]);
#else
          trimWS(e);
          trimQuotes(e);
          args[i] = strdup (e);
#endif
	  i++;
	  e=t1;
	  s=t1;
	}
	else {
	  *t = ',';
	  s = t1+1;
	}
      }
      else {
#if 0
	args[i] = e;
	args[i][strlen(args[i])-1] = '\0';
#else
	e[strlen(e) - 1]  = '\0';
	trimWS(e);
	trimQuotes(e);
        args[i] = strdup (e);
#endif
	i++;
	found = 1;
      }
    }
    *argc = i;
    /* XXXX This need to be freed */
    free (act);	/* fix mem leak mw 02/07/07 */
    return(0);
  }
  else {
    strcpy(action,act);
    *argc = 0;
    /* XXXX This need to be freed */
    free (act);	/* fix mem leak mw 02/07/07 */
    return(0);
  }
}

int
makeActionFromMParam(char *outAction, char *action, msParam_t *mP[], int argc, int len)
{
  int i,j;
  char *args; 
  char cv[MAX_ACTION_SIZE];
  strncpy(outAction,action,len);
  if (argc == 0)
    return(0);
  strcat(outAction,"(");
  for (i = 0; i < argc; i++) {
    if (i != 0)
      strcat(outAction,",");
    if (mP[i]->type == NULL) {
      args = (char *) mP[i]->label;
    }
    else if (!strcmp(mP[i]->type, STR_MS_T) ) {
      args = (char *) mP[i]->inOutStruct;
    } else if (!strcmp(mP[i]->type, INT_MS_T) || !strcmp(mP[i]->type, BUF_LEN_MS_T) ) {
      sprintf(cv,"%d",(int*)mP[i]->inOutStruct);
      args = cv;
    } else if (!strcmp(mP[i]->type, DOUBLE_MS_T) ) {
      sprintf(cv,"%lld",(rodsLong_t) mP[i]->inOutStruct);
      args = cv;
    } else if (!strcmp(mP[i]->type, DataObjInp_MS_T) ) {
      dataObjInp_t dataObjInp, *myDataObjInp;
      j = parseMspForDataObjInp (mP[i], &dataObjInp, &myDataObjInp, 0);
      if (j < 0)
	args = (char *) mP[i]->label;
      args = (char *) myDataObjInp->objPath;
    } else if (!strcmp(mP[i]->type, CollInp_MS_T )) {
      collInp_t collCreateInp, *myCollCreateInp;
      j = parseMspForCollInp (mP[i], &collCreateInp, &myCollCreateInp, 0);
      if (j < 0)
        args = (char *) mP[i]->label;
      args = (char *) myCollCreateInp->collName;
    } else if (!strcmp(mP[i]->type,  DataObjCopyInp_MS_T)) {
      dataObjCopyInp_t dataObjCopyInp, *myDataObjCopyInp;
      j = parseMspForDataObjCopyInp (mP[i], &dataObjCopyInp, &myDataObjCopyInp);
      if (j < 0)
        args = (char *) mP[i]->label;
      snprintf (cv, MAX_ACTION_SIZE, "COPY(%s,%s)",
		myDataObjCopyInp->srcDataObjInp.objPath, myDataObjCopyInp->destDataObjInp.objPath);
      args = cv;
    } else if (!strcmp(mP[i]->type,  DataObjReadInp_MS_T) 
	       || !strcmp(mP[i]->type, DataObjCloseInp_MS_T)
	       || !strcmp(mP[i]->type, DataObjWriteInp_MS_T) ) {
      openedDataObjInp_t  *myDataObjReadInp;
      myDataObjReadInp = (openedDataObjInp_t  *) mP[i]->inOutStruct;
      snprintf (cv, MAX_ACTION_SIZE, "OPEN(%d)",myDataObjReadInp->len);
      args = cv;
    } else if (!strcmp(mP[i]->type, ExecCmd_MS_T  )) {
      execCmd_t *execCmd;
      execCmd = (execCmd_t *)  mP[i]->inOutStruct;
      args = execCmd->cmd;
    } else {
      args = (char *) mP[i]->label;
    }
    if (args[0] != '"') {
      if (strstr(args, ",") != 0) {
	strcat(outAction,"\"");
	rstrcat(outAction, args,len);
	strcat(outAction,"\"");
      }
      else {
	rstrcat(outAction, args,len);
      }
    }
    else {
      rstrcat(outAction, args,len);
    }
  }
  rstrcat(outAction,")",len);
  return(0);
}


int
makeAction(char *outAction,char *action, char *args[], int argc, int len)
{
  int i;
  strncpy(outAction,action,len);
  if (argc == 0)
    return(0);
  strcat(outAction,"(");
  for (i = 0; i < argc; i++) {
    if (i != 0)
      strcat(outAction,",");
    if (args[i][0] != '"') {
      if (strstr(args[i], ",") != 0) {
	strcat(outAction,"\"");
	rstrcat(outAction, args[i],len);
	strcat(outAction,"\"");
      }
      else
	rstrcat(outAction, args[i],len);
    }
    else
      rstrcat(outAction, args[i],len);
  }
  rstrcat(outAction,")",len);
  return(0);
}


int
executeRuleAction(char *inAction, ruleExecInfo_t *rei, int reiSaveFlag)
{
  int i,ii,j;
  /*funcPtr myFunc = NULL;*/
  int actionInx;
  /*int numOfStrArgs;*/
  char action[MAX_ACTION_SIZE];  
  char oldaction[MAX_ACTION_SIZE];  
  /*char  *t, *s;*/
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc;

  i = parseAction(inAction,action,args, &argc);
  if (i != 0)
    return(i);
  strcpy(oldaction,action);
  j = mapExternalFuncToInternalProc(action);

  actionInx = actionTableLookUp(action);
  /*
    if (actionInx < 0)
      return(actionInx);
  */

  if (actionInx < 0) {
    /* check if this a rule */
    if (reTestFlag > 0) {
      if (reTestFlag == COMMAND_TEST_1) 
	fprintf(stdout,"...Performing Action:%s\n",inAction);
      else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"...Performing Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",inAction);
      else if (reTestFlag == LOG_TEST_1)
	 if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	   rodsLog (LOG_NOTICE,"...Performing Action:%s\n",inAction);
      if (j == 1) {
	if (reTestFlag == COMMAND_TEST_1) 
	  fprintf(stdout,"------Mapping Action:%s  To %s\n", oldaction,action);
	else if (reTestFlag == HTML_TEST_1)
	  fprintf(stdout,"------Mapping Action:<FONT COLOR=#FF0000>%s</FONT> To <FONT COLOR=#0000FF>%s</FONT><BR>\n", 
		  oldaction,action);
	else if (reTestFlag == LOG_TEST_1)
	   if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	     rodsLog (LOG_NOTICE,"------Mapping Action:%s  To %s\n",  oldaction,action);
      }
    }
    actionInx = applyRuleArg(action,args,argc, rei, reiSaveFlag);
    freeRuleArgs (args, argc);
    if (actionInx  == NO_RULE_FOUND_ERR)
      return (NO_RULE_OR_MSI_FUNCTION_FOUND_ERR);
    else if (actionInx  == NO_MORE_RULES_ERR)
      return (ACTION_FAILED_ERR);
    else
      return(actionInx);
  }
    
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
      fprintf(stdout,"...Performing Function:%s\n",inAction);
    else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
      fprintf(stdout,"...Performing Function:<FONT COLOR=#5500FF>%s</FONT><BR>\n",inAction);
    else if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"...Performing Function:%s\n",inAction);
    if (j == 1) {
      if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
	fprintf(stdout,"------Mapping Function:%s  To %s\n", oldaction,action);
      else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
	fprintf(stdout,"------Mapping Function:<FONT COLOR=#FF0000>%s</FONT> To <FONT COLOR=#0000FF>%s</FONT><BR>\n", 
		oldaction,action);
      else if (reTestFlag == LOG_TEST_1)
	 if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	   rodsLog (LOG_NOTICE,"------Mapping Function/Action:%s  To %s\n", oldaction,action);
    }

  }

  ii = executeMicroService(action, args, argc, rei);
  /******
  myFunc =  MicrosTable[actionInx].callAction;
  numOfStrArgs =  MicrosTable[actionInx].numberOfStringArgs;
  if (argc != numOfStrArgs) {
    freeRuleArgs (args, argc);
    return(ACTION_ARG_COUNT_MISMATCH);
  } 
  if (numOfStrArgs == 0)
    ii = (*myFunc) (rei) ;
  else if (numOfStrArgs == 1)
    ii = (*myFunc) (args[0],rei);
  else if (numOfStrArgs == 2)
    ii = (*myFunc) (args[0],args[1],rei);
  else if (numOfStrArgs == 3)
    ii = (*myFunc) (args[0],args[1],args[2],rei);
  else if (numOfStrArgs == 4)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],rei);
  else if (numOfStrArgs == 5)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],rei);
  else if (numOfStrArgs == 6)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],args[5],rei);
  else if (numOfStrArgs == 7)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],args[5],args[6],rei);
  else if (numOfStrArgs == 8)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],rei);
  else if (numOfStrArgs == 9)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],rei);
  else if (numOfStrArgs == 10)
    ii = (*myFunc) (args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],rei);
  if (ii < 0)
    rei->status = ii;
  ******/
  freeRuleArgs (args, argc);
  return(ii);
}


int
executeRuleActionNew(char *inAction,  msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag)
{
  int i,ii,j;
  /*funcPtr myFunc = NULL;*/
  int actionInx;
  /*int numOfStrArgs;*/
  char action[MAX_ACTION_SIZE];  
  char oldaction[MAX_ACTION_SIZE];  
  /*char  *t, *s;*/
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc;

  i = parseAction(inAction,action,args, &argc);
  if (i != 0)
    return(i);
  strcpy(oldaction,action);
  j = mapExternalFuncToInternalProc(action);

  actionInx = actionTableLookUp(action);
  /*
    if (actionInx < 0)
      return(actionInx);
  */

  if (actionInx < 0) {
    /* check if this a rule */
    if (reTestFlag > 0) {
      if (reTestFlag == COMMAND_TEST_1) 
	fprintf(stdout,"...Performing Action:%s\n",inAction);
      else if (reTestFlag == HTML_TEST_1)
	    fprintf(stdout,"...Performing Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",inAction);
      else if (reTestFlag == LOG_TEST_1)
	 if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	   rodsLog (LOG_NOTICE,"...Performing Action:%s\n",inAction);
      if (j == 1) {
	if (reTestFlag == COMMAND_TEST_1) 
	  fprintf(stdout,"------Mapping Action:%s  To %s\n", oldaction,action);
	else if (reTestFlag == HTML_TEST_1)
	  fprintf(stdout,"------Mapping Action:<FONT COLOR=#FF0000>%s</FONT> To <FONT COLOR=#0000FF>%s</FONT><BR>\n", 
		  oldaction,action);
	else if (reTestFlag == LOG_TEST_1)
	   if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	     rodsLog (LOG_NOTICE,"------Mapping Action:%s  To %s\n",  oldaction,action);
      }
    }
    actionInx = applyRuleArgPA(action,args, argc, inMsParamArray, rei, reiSaveFlag);
    freeRuleArgs (args, argc);
    if (actionInx  == NO_RULE_FOUND_ERR)
      return (NO_RULE_OR_MSI_FUNCTION_FOUND_ERR);
    else if (actionInx  == NO_MORE_RULES_ERR)
      return (ACTION_FAILED_ERR);
    else
      return(actionInx);
  }
    
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
      fprintf(stdout,"...Performing Function:%s\n",inAction);
    else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
      fprintf(stdout,"...Performing Function:<FONT COLOR=#5500FF>%s</FONT><BR>\n",inAction);
    else if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"...Performing Function:%s\n",inAction);
    if (j == 1) {
      if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
	fprintf(stdout,"------Mapping Function:%s  To %s\n", oldaction,action);
      else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
	fprintf(stdout,"------Mapping Function:<FONT COLOR=#FF0000>%s</FONT> To <FONT COLOR=#0000FF>%s</FONT><BR>\n", 
		oldaction,action);
      else if (reTestFlag == LOG_TEST_1)
	 if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	   rodsLog (LOG_NOTICE,"------Mapping Function/Action:%s  To %s\n", oldaction,action);
    }

  }

  freeRuleArgs (args, argc);
  ii = executeMicroServiceNew(inAction, inMsParamArray, rei);
  return(ii);
}

int
executeMyRuleAction(char *inAction,  msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag)
{
  int ii,j = 0;
  /*funcPtr myFunc = NULL;*/
  /*int actionInx;*/
  /*int numOfStrArgs;*/
  char action[MAX_ACTION_SIZE];  
  char oldaction[MAX_ACTION_SIZE];  
  /*char  *t, *s;*/
  /*char *args[MAX_NUM_OF_ARGS_IN_ACTION];*/
  /*int argc;*/
  /****
  i = parseAction(inAction,action,args, &argc);
  if (i != 0)
    return(i);
  strcpy(oldaction,action);
  j = mapExternalFuncToInternalProc(action);
  ***/
  if (reTestFlag > 0) {
    if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
      fprintf(stdout,"...Performing Function:%s\n",inAction);
    else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
      fprintf(stdout,"...Performing Function:<FONT COLOR=#5500FF>%s</FONT><BR>\n",inAction);
    else if (reTestFlag == LOG_TEST_1)
       if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	 rodsLog (LOG_NOTICE,"...Performing Function:%s\n",inAction);
    if (j == 1) {
      if (reTestFlag == COMMAND_TEST_1 || reTestFlag == COMMAND_TEST_MSI) 
	fprintf(stdout,"------Mapping Function:%s  To %s\n", oldaction,action);
      else if (reTestFlag == HTML_TEST_1 || reTestFlag == HTML_TEST_MSI)
	fprintf(stdout,"------Mapping Function:<FONT COLOR=#FF0000>%s</FONT> To <FONT COLOR=#0000FF>%s</FONT><BR>\n", 
		oldaction,action);
      else if (reTestFlag == LOG_TEST_1)
	 if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
	   rodsLog (LOG_NOTICE,"------Mapping Function/Action:%s  To %s\n", oldaction,action);
    }

  }
  ii = executeMicroServiceNew(inAction, inMsParamArray,  rei);
  return(ii);
}



int
actionTableLookUp (char *action)
{
    int i;

    for (i = 0; i < NumOfAction; i++) {
        if (!strcmp(MicrosTable[i].action,action))
            return (i);
    }

    return (UNMATCHED_ACTION_ERR);
}


/*
int
getNextAction(char *listOfAction, char *action, char **restPtr)
{

  char *t, *s;

  if (listOfAction== NULL || strlen(listOfAction) == 0) {
    return(-1);
  }

  s = strstr(listOfAction,"##");
  if (s == NULL) {
    rstrcpy(action,listOfAction,MAX_ACTION_SIZE);
    *restPtr = listOfAction + strlen(listOfAction);
  }
  else {
    *s = '\0';
    rstrcpy(action,listOfAction,MAX_ACTION_SIZE);
    *restPtr = s + 2;
    *s = '#';
  }
  return(0);
}
*/

int
getNextAction(char *listOfAction, char *action, char **restPtr)
{

  /*  char *t, *s;*/

  if (listOfAction== NULL || strlen(listOfAction) == 0) {
    return(-1);
  }

  splitActions(listOfAction,action, restPtr);
  return(0);
}

int
splitActions(char *expr, char *expr1, char **expr2)
{

  int found;
  char *e, *t, *t1;
  found = 0;
  e = expr;
  while  (found == 0) {
    if ((t = strstr(e,"##")) != NULL ) {
      *t = '\0';
      t1 = t + 2;
      if (goodExpr(expr) == 0) {
	rstrcpy(expr1,expr,MAX_ACTION_SIZE);
        *expr2  = t1;
	found = 1;
      }
      else {
	*t = '#';
	e = t1;
      }
    }
    else {
      rstrcpy(expr1,expr,MAX_ACTION_SIZE);
      *expr2 = NULL;
      found = 1;
    }
  }
  trimWS(expr1);
  return(0);
}

int
checkRuleHead(char *ruleHead, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc)
{

  int i;
  char *largs[MAX_NUM_OF_ARGS_IN_ACTION];
  int largc;
  char action[MAX_ACTION_SIZE];  

  i =  parseAction(ruleHead,action,largs, &largc);
  if (i != 0)
    return(i);
  if (argc < largc) {
    freeRuleArgs (args, argc);
    return(INSUFFICIENT_INPUT_ARG_ERR);
  }

  for (i = 0; i < largc; i++) {
    if (strcmp(largs[i],"$") && strcmp(args[i],largs[i])) {
      freeRuleArgs (args, argc);
      return(INPUT_ARG_DOES_NOT_MATCH_ERR);
    }
  }
  freeRuleArgs (args, argc);
  return(0);
}
    

int
executeMicroService (char *inAction, char *largs[MAX_NUM_OF_ARGS_IN_ACTION], int largc,
	  ruleExecInfo_t *rei)
{
  funcPtr myFunc = NULL;
  int actionInx;
  int numOfStrArgs;
  int i, ii;
  void *myArgv[MAX_NUM_OF_ARGS_IN_ACTION];
  msParam_t *mP;
  /*  char tmpStr[NAME_LEN];*/
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc;
  int argcToFree;
  char action[MAX_ACTION_SIZE];  

  if (strstr(inAction,"(") != NULL) {
    i =  parseAction(inAction,action,args, &argc);
    if (i != 0)
      return(i);
    argcToFree = argc;
  }
  else {
    for (i = 0 ; i < largc; i++) {
      args[i] = largs[i];
    }
    argcToFree = 0;
    argc = largc;
    rstrcpy(action,inAction,MAX_ACTION_SIZE);
  }


  actionInx = actionTableLookUp(action);
  if (actionInx < 0)
    return(NO_MICROSERVICE_FOUND_ERR);
  myFunc =  MicrosTable[actionInx].callAction;
  numOfStrArgs =  MicrosTable[actionInx].numberOfStringArgs;
  if (argc != numOfStrArgs)
    return(ACTION_ARG_COUNT_MISMATCH);

  for(i = 0; i < numOfStrArgs; i++) {
    if (args[i][0] == '*') { 
      if ((mP = getMsParamByLabel (rei->msParamArray, args[i])) != NULL) {
	myArgv[i] = mP;
      }
      else {
	/*create an entry in the array */
	/* this is an output send the args as is */
	addMsParam(rei->msParamArray, args[i], NULL, NULL,NULL);
	mP = getMsParamByLabel (rei->msParamArray, args[i]);
	myArgv[i] = mP;
      }
    }
    else {/* this is not a msParam */
      /* so we create a new param  */
      addMsParam(rei->msParamArray, args[i], STR_MS_T, strdup (args[i]),NULL);
      myArgv[i] = strdup(args[i]);
    }
  }


  freeRuleArgs (args, argcToFree);

  if (numOfStrArgs == 0)
    ii = (*myFunc) (rei) ;
  else if (numOfStrArgs == 1)
    ii = (*myFunc) (myArgv[0],rei);
  else if (numOfStrArgs == 2)
    ii = (*myFunc) (myArgv[0],myArgv[1],rei);
  else if (numOfStrArgs == 3)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],rei);
  else if (numOfStrArgs == 4)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],rei);
  else if (numOfStrArgs == 5)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],rei);
  else if (numOfStrArgs == 6)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],rei);
  else if (numOfStrArgs == 7)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],rei);
  else if (numOfStrArgs == 8)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],rei);
  else if (numOfStrArgs == 9)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],myArgv[8],rei);
  else if (numOfStrArgs == 10)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],
		    myArgv[8],myArgv [9],rei);
  rei->status = ii;
  return(ii);
}

int
executeMicroServiceNew(char *inAction,  msParamArray_t *inMsParamArray, 
		       ruleExecInfo_t *rei)
{
  funcPtr myFunc = NULL;
  int actionInx;
  int numOfStrArgs;
  int i,j, ii;
  void *myArgv[MAX_NUM_OF_ARGS_IN_ACTION];
  msParam_t *mP;
  /*char tmpStr[NAME_LEN];*/
  char tmpVarName[MAX_ACTION_SIZE];
  char action[MAX_ACTION_SIZE];  
  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int argc;
  char *tmparg;
  char tmpActStr[MAX_ACTION_SIZE];


  i = parseAction(inAction,action,args, &argc);
  if (i != 0)
    return(i);
  j = mapExternalFuncToInternalProc(action);


  actionInx = actionTableLookUp(action);
  if (actionInx < 0)
    return(NO_MICROSERVICE_FOUND_ERR);
  myFunc =  MicrosTable[actionInx].callAction;
  numOfStrArgs =  MicrosTable[actionInx].numberOfStringArgs;
  if (argc != numOfStrArgs)
    return(ACTION_ARG_COUNT_MISMATCH);



#ifdef AAAAA
  for(i = 0; i < numOfStrArgs; i++) {
    if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) {
      myArgv[i] = mP;
    }
    else {
      if (args[i][0] == '*') { 
	/*create an entry in the array */
	/* this is an output send the args as is */
	addMsParam(inMsParamArray, args[i], NULL, NULL,NULL);
	mP = getMsParamByLabel (inMsParamArray, args[i]);
	myArgv[i] = mP;
      }
      else {/* this is not a msParam */
	/* so we create a new param  */

	addMsParam(inMsParamArray, args[i], STR_MS_T, strdup (args[i]),
	  NULL);
	mP = getMsParamByLabel (inMsParamArray, args[i]);
	myArgv[i] = mP;
      }
    }
  }

#endif

  /* #ifdef AAAAA */
  for(i = 0; i < numOfStrArgs; i++) {
    if (!strcmp(action,"whileExec")||
	!strcmp(action,"remoteExec")||
	!strcmp(action,"delayExec")||
	!strcmp(action,"assign") || 
	!strcmp(action,"forExec") || 
	!strcmp(action,"ifExec") || 
	!strcmp(action,"forEachExec") ||
	!strcmp(action,"msiCollectionSpider")) {
      if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) {
	myArgv[i] = mP;
      }
      else {
	/* RAJA Sep 19 2007 ***
	addMsParam(inMsParamArray, args[i], STR_MS_T, strdup (args[i]),
		   NULL);
	mP = getMsParamByLabel (inMsParamArray, args[i]);
	myArgv[i] = mP;
	***/
	if (isStarVariable(args[i]) == TRUE ) { 
	  addMsParam(inMsParamArray, args[i], STR_MS_T, strdup (args[i]), NULL);
	  mP = getMsParamByLabel (inMsParamArray, args[i]);
	  myArgv[i] = mP;
	}
	else {
	  getNewVarName(tmpVarName,inMsParamArray);
	  addMsParam(inMsParamArray, tmpVarName, STR_MS_T, args[i], NULL);
	  mP = getMsParamByLabel (inMsParamArray, tmpVarName);
	  myArgv[i] = mP;
	}
      }
    }
    else if ((mP = getMsParamByLabel (inMsParamArray, args[i])) != NULL) {
      myArgv[i] = mP;
      tmparg = NULL;
      if (mP->type != NULL) {
	if (mP->inOutStruct == NULL || (!strcmp(mP->type, STR_MS_T) 
					&& !strcmp((char*)mP->inOutStruct,(char*)mP->label) )) {
	  convertArgWithVariableBinding(args[i],&tmparg,inMsParamArray,rei);
	  if (tmparg != NULL) 
	    mP->inOutStruct = tmparg;
	}
	else if (!strcmp(mP->type, STR_MS_T) ) {
	  convertArgWithVariableBinding((char*)mP->inOutStruct,&tmparg,inMsParamArray,rei);
	  if (tmparg != NULL) 
	    mP->inOutStruct = tmparg;
	}
      }
    }
    else {
      if (isStarVariable(args[i]) == TRUE ) { 
	/*create an entry in the array */
	/* this is an output send the args as is */
	tmparg = NULL;
	convertArgWithVariableBinding(args[i],&tmparg,inMsParamArray,rei);
	addMsParam(inMsParamArray, args[i], NULL, tmparg,NULL);
	if (tmparg != NULL)
	  free(tmparg);
	mP = getMsParamByLabel (inMsParamArray, args[i]);
	myArgv[i] = mP;
      }
      else {/* this is not a msParam */
	/* so we create a new param  */
	tmparg = NULL;
	getNewVarName(tmpVarName,inMsParamArray);
	convertArgWithVariableBinding(args[i],&tmparg,inMsParamArray,rei);
	if (tmparg != NULL) {
	  /* addMsParam(inMsParamArray, args[i], STR_MS_T, tmparg, NULL); */
	  addMsParam(inMsParamArray, tmpVarName, STR_MS_T, tmparg, NULL);
	  free(tmparg);
	}
	else
	  /* addMsParam(inMsParamArray, args[i], STR_MS_T, args[i], NULL); */
	  addMsParam(inMsParamArray, tmpVarName, STR_MS_T, args[i], NULL);
	/* mP = getMsParamByLabel (inMsParamArray, args[i]); */
	mP = getMsParamByLabel (inMsParamArray, tmpVarName);
	myArgv[i] = mP;
      }
    }
  }
  /* #endif  */ 


  if (GlobalREAuditFlag) {
    makeActionFromMParam(tmpActStr,action, (msParam_t **) myArgv,numOfStrArgs, MAX_ACTION_SIZE);
    reDebug("    ExecMicroSrvc", -4, tmpActStr, inMsParamArray,rei);
  }

  freeRuleArgs (args, argc);

  if (numOfStrArgs == 0)
    ii = (*myFunc) (rei) ;
  else if (numOfStrArgs == 1)
    ii = (*myFunc) (myArgv[0],rei);
  else if (numOfStrArgs == 2)
    ii = (*myFunc) (myArgv[0],myArgv[1],rei);
  else if (numOfStrArgs == 3)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],rei);
  else if (numOfStrArgs == 4)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],rei);
  else if (numOfStrArgs == 5)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],rei);
  else if (numOfStrArgs == 6)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],rei);
  else if (numOfStrArgs == 7)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],rei);
  else if (numOfStrArgs == 8)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],rei);
  else if (numOfStrArgs == 9)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],myArgv[8],rei);
  else if (numOfStrArgs == 10)
    ii = (*myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],
		    myArgv[8],myArgv [9],rei);
  rei->status = ii;
  /*  removeTmpVarName(inMsParamArray);*/
  return(ii);
}

int
freeRuleArgs (char *args[], int argc)
{
    int i;

    for (i = 0; i < argc; i++) {
	free (args[i]);
	args[i] = NULL;
    }
    return (0);
}

