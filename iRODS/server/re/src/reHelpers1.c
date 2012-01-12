/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reHelpers1.h"

char *breakPoints[100];
int breakPointsInx = 0;
char myHostName[MAX_NAME_LEN];
char waitHdr[HEADER_TYPE_LEN];
char waitMsg[MAX_NAME_LEN];
int myPID;

int
checkRuleCondition(char *action, char *incond, char *args[MAX_NUM_OF_ARGS_IN_ACTION], 
		   int argc, ruleExecInfo_t *rei, int reiSaveFlag )
{
  int i,k;
  /*  char *t1, *t2, *t3;*/
  char cond[MAX_COND_LEN];
  char evaStr[MAX_COND_LEN * 2];
  /*
  dataObjInfo_t *doi;
  rescInfo_t *roi;
  userInfo_t *uoi;
  collInfo_t *coi;

  doi = rei->doi;
  roi = rei->roi;
  uoi = rei->uoi;
  coi = rei->coi;
  i = 0;
  */

  
  rstrcpy (cond,incond,MAX_COND_LEN);
  i  = replaceVariables(action, cond,args,argc,rei);
  if (i < 0)
    return(0); /* FALSE */

  /****
  t1 = cond;
  /  * replace variables with values * /
  while ((t2 = strstr(t1,"$")) != NULL) {
    j =  replaceDollarParam(t2,(int) (MAX_COND_LEN - (t2 - cond) - 1),
			    args, argc, rei);
    if (j < 0) {
      rodsLog (LOG_NOTICE,"replaceDollarParam Failed at %s: %i\n", (char *)t2,j);
      return(0);
    }
    t1 = t2 + 1;
  }
  ****/
  /***
  while(strcmp(dataObjCond[i],"ENDOFLIST")) {
    if ((t2  = strstr(t1,dataObjCond[i])) == NULL) {
      t1 = cond;
      i++;
    }
    else if (*(t2 - 1) != '$') {
      t1 = t2 + 1;
    }
    else {
      t2--;
      j = replaceDataVar(t2,(int) (MAX_COND_LEN - (t2 - cond) - 1), i, doi);
      if (j < 0) {
	rodsLog (LOG_NOTICE,"replaceVar Failed at %s: %i\n", (char *)(t2-1),j);
	return(0);
      }
      t1 = t2;
    }
  }
  ***/
  /* check condition and return */
  k =  computeExpression(cond, rei, reiSaveFlag,evaStr);
  if (k < 0) {
    rodsLog (LOG_NOTICE,"computeExpression Failed: %s: %i\n", cond,k);
  }
  return(k);
}




int
checkRuleConditionNew(char *action, char *incond,  msParamArray_t *inMsParamArray,
		   ruleExecInfo_t *rei, int reiSaveFlag )
{
  int i,k;
  /* char *t1, *t2, *t3;*/
  char cond[MAX_COND_LEN];
  char evaStr[MAX_COND_LEN * 2];
  /*
  dataObjInfo_t *doi;
  rescInfo_t *roi;
  userInfo_t *uoi;
  collInfo_t *coi;

  doi = rei->doi;
  roi = rei->roi;
  uoi = rei->uoi;
  coi = rei->coi;
  i = 0;
  */

  
  rstrcpy (cond,incond,MAX_COND_LEN);
  i  = replaceVariablesAndMsParams(action, cond, inMsParamArray, rei);
  if (i < 0)
    return(0); /* FALSE */

  
  /* check condition and return */
  k =  computeExpression(cond, rei, reiSaveFlag, evaStr);
  if (k < 0) {
    rodsLog (LOG_NOTICE,"computeExpression Failed: %s: %i\n", cond, k);
  }
  return(k);
}


int
convertArgWithVariableBinding(char *inS, char **outS, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei)
{
  char *tS;
  int i;
 


  tS =  (char*)malloc(strlen(inS)  + 4 * MAX_COND_LEN);
  strcpy(tS,inS);

  i = replaceVariablesAndMsParams("", tS, inMsParamArray, rei);
  if (i < 0 || !strcmp(tS, inS)) {
    free(tS);
    *outS = NULL;
  }
  else
    *outS = tS;
  return(i);
}

int
evaluateExpression(char *expr, char *res, ruleExecInfo_t *rei)
{
  int i;

  i  = replaceVariablesAndMsParams("", expr, rei->msParamArray, rei);
  if (i < 0) return(i);
  i =  computeExpression(expr, rei, 0, res);
  /***
  if (i == 1)
    strcpy(res,expr);
  ***/
  return(i);
}

int
computeExpression( char *expr, ruleExecInfo_t *rei, int reiSaveFlag , char *res)
{

   int i;
   char expr1[MAX_COND_LEN], expr2[MAX_COND_LEN], expr3[MAX_COND_LEN];
   char oper1[MAX_OPER_LEN];
   static int k;
   if (strlen(expr) == 0) {
     strcpy(res,expr);
     return (TRUE);
   }
   strcpy(res,"");
   if ((i = splitExpression(expr,expr1,expr2,oper1)) < 0)
     return(i);
   if (strlen(oper1) == 0) {
     if (strlen(expr2) == 0) {
       /*
       if (expr1[0] == '(' && expr1[strlen(expr1)-1] == ')') {
	 expr1[0] = ' ';
	 expr1[strlen(expr1)-1] ='\0';
	 if (goodExpr(expr1) != 0) {
           strcpy(res,expr);
	   return(TRUE);	   
	 }
	 trimWS(expr1);
         strcpy(expr3,expr1);
	 if ((i = splitExpression(expr3,expr1,expr2,oper1)) < 0)
	   return(i);
	 if (strlen(oper1) == 0) {
	   strcpy(res,expr);
           return(TRUE);
         }
       }
       else {
	 strcpy(res,expr);
	 return(TRUE);
       }
       */
       trimWS(expr1);
       strcpy(expr3,expr1);
       if ((i = splitExpression(expr3,expr1,expr2,oper1)) < 0)
	 return(i);
       if (strlen(oper1) == 0) {
	 strcpy(res,expr);
	 return(TRUE);
       }

     }
     else {
       strcpy(res,expr);
       return(TRUE);
     }
   }
   k++;

   i  = _computeExpression(expr1,expr2,oper1,rei,reiSaveFlag, res );

   k--;
   if (i < 0)
     strcpy(res,expr);
   return(i);
}

int
_computeExpression(char *expr1, char *expr2, char *oper1, ruleExecInfo_t *rei, int reiSaveFlag , char *res )
{
   int i,j,k,iii,jjj,kkk;
   double x,y;
   /*   char aaa[MAX_COND_LEN], bbb[MAX_COND_LEN];*/
   char inres1[MAX_COND_LEN * 2];
   char inres2[MAX_COND_LEN * 2];

   if (isLogical(oper1)) {
     if ((i = computeExpression(expr1,rei, reiSaveFlag,inres1)) < 0) 
       return(i);
     if ((j = computeExpression(expr2,rei, reiSaveFlag,inres2)) < 0)
       return(j);
     if (!strcmp(oper1, "&&")) {
       /*
       sprintf(res,"%i", atoi(inres1) && atoi(inres2));
       return( atoi(res));
       */
       sprintf(res,"%i", i && j);
       return( atoi(res));
     }
     else if (!strcmp(oper1, "%%")) {
       /*
       sprintf(res,"%i", atoi(inres1) || atoi(inres2));
       return( atoi(res));
       */
       sprintf(res,"%i", i || j);
       return( atoi(res));
     }
     else
       return(-1);
   }
   else if (isNumber(expr1) && isNumber(expr2)) {
     x = atof(expr1);
     y = atof(expr2);
     if (!strcmp(oper1, "<"))
       sprintf(res,"%d", x < y);
     else if (!strcmp(oper1, ">"))
       sprintf(res,"%d", x > y);
     else if (!strcmp(oper1, "=="))
       sprintf(res,"%d", x == y);
     else if (!strcmp(oper1, "<="))
      sprintf(res,"%d", x <= y);
     else if (!strcmp(oper1, ">="))
       sprintf(res,"%d", x >= y);
     else if (!strcmp(oper1, "!="))
       sprintf(res,"%d", x != y);
     else if (!strcmp(oper1, "++"))
       sprintf(res,"%d",  (int) (x + y));
     else if ( (strchr(expr1,'.') != NULL) || 
	       (strchr(expr2,'.') != NULL) ) {
       if (!strcmp(oper1, "+"))
	 sprintf(res,"%f",  x + y);
       else if (!strcmp(oper1, "-"))
	 sprintf(res,"%f",  x - y);
       else if (!strcmp(oper1, "*"))
	 sprintf(res,"%f",  x * y);
       else if (!strcmp(oper1, "/"))
	 sprintf(res,"%f",  x / y);
       else
	 return(-2);
     }
     else { /* both are integers */
       if (!strcmp(oper1, "+"))
         sprintf(res,"%d", (int) (x + y));
       else if (!strcmp(oper1, "-"))
         sprintf(res,"%d", (int) (x - y));
       else if (!strcmp(oper1, "*"))
         sprintf(res,"%d", (int) (x * y));
       else if (!strcmp(oper1, "/"))
         sprintf(res,"%d", (int) (x / y));
       else
         return(-2);
     }
     return(atoi(res));
   }
   /********RAJA remove May 29 2009 being done later anyway
   else if (isNumber(expr1)) {
     iii = computeExpression(expr2, rei, reiSaveFlag,inres1);
     if (iii < 0)
       return(iii);
     i = _computeExpression(expr1,inres1,oper1, rei, reiSaveFlag,res);
     return(i);
   }
   else if (isNumber(expr2)) {
     iii = computeExpression(expr1, rei, reiSaveFlag,inres1);
     if (iii < 0)
       return(iii);
     i = _computeExpression(inres1,expr2, oper1, rei, reiSaveFlag, res);
     return(i);
   }
   ************/
   else if (isAFunction(expr1) || isAFunction(expr2)) {
     if (isAFunction(expr1)) {
       if (executeRuleAction(expr1, rei, reiSaveFlag) == 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else {
       if (strlen(expr1) == 0)
	 iii = -1;
       else 
	 iii = atoi(expr1);
     }
     if (isAFunction(expr2)) {
       if (executeRuleAction(expr2, rei, reiSaveFlag) == 0)
	 jjj = 1;
       else
	 jjj = 0;
     }
     else {
       if (strlen(expr2) == 0)
	 jjj = -1;
       else 
	 jjj = atoi(expr2);
     }
     if (iii == -1) {
       sprintf(res,"%d",jjj);
       return(atoi(res));
     }
     if (jjj == -1){
       sprintf(res,"%d",iii);
       return(atoi(res));
     }
     if (!strcmp(oper1, "<"))
       sprintf(res,"%i",  iii < jjj);
     else if (!strcmp(oper1, ">"))
       sprintf(res,"%i",  iii > jjj);
     else if (!strcmp(oper1, "=="))
       sprintf(res,"%i",  iii == jjj);
     else if (!strcmp(oper1, "<="))
       sprintf(res,"%i",  iii <= jjj);
     else if (!strcmp(oper1, ">="))
       sprintf(res,"%i",  iii >= jjj);
     else if (!strcmp(oper1, "!="))
       sprintf(res,"%i",  iii != jjj);
     else
       return(-3);
     return(atoi(res));
   }
   else { /* expr1 and expr2 can be string/numeric expressions */
     iii = computeExpression(expr1, rei, reiSaveFlag,inres1);
     if (iii <  0)
       return(iii);
     jjj = computeExpression(expr2, rei, reiSaveFlag,inres2);
     if (jjj < 0)
       return(jjj);
     if (strcmp(expr1,inres1) || strcmp(expr2,inres2)) {
       kkk = _computeExpression(inres1,inres2,oper1, rei, reiSaveFlag,res);
       return(kkk);
     }
     /*
     if (strcmp(expr1,inres1) && strcmp(expr2,inres2)) {
       kkk = _computeExpression(inres1,inres2,oper1, rei, reiSaveFlag,res);
       return(kkk);
     }
     else if (strcmp(expr1,inres1)) {
       kkk = _computeExpression(inres1,expr2,oper1, rei, reiSaveFlag,res);
       return(kkk);
     }
     else if (strcmp(expr2,inres2)) {
       sprintf(bbb,"%d",jjj);
       kkk = _computeExpression(expr1,inres2,oper1, rei, reiSaveFlag,res);
       return(kkk);
     }
     */
     if (!strcmp(oper1, "<")) {
       if (strcmp(expr1,expr2) < 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, ">")){
       if (strcmp(expr1,expr2) > 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, "==")){
       if (strcmp(expr1,expr2) == 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, "<=")){
       if (strcmp(expr1,expr2) <= 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, ">=")){
       if (strcmp(expr1,expr2) >= 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, "!=")){
       if (strcmp(expr1,expr2) != 0)
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, "not like")) {
       k = reREMatch(expr2,expr1);
       if (k == 0) 
	 iii = 1;
       else
	 iii = 0;
     }
     else if (!strcmp(oper1, "like")) {
       iii = reREMatch(expr2,expr1);
     }
     else
       return(-4);
     sprintf(res,"%i",  iii);
     return(iii);
   }
   /*   return(-5); removed RAJA June 13 2008 as it is not reached */
}

int
replaceVariables(char *action, char *inStr, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		   ruleExecInfo_t *rei )
{
  int j;
  char *t1, *t2;

  t1 = inStr;

  while ((t2 = strstr(t1,"$")) != NULL) {
    j =  replaceDollarParam(action, t2,(int) (MAX_COND_LEN - (t2 - inStr) - 1),
			    args, argc, rei);
    if (j < 0) {
      rodsLog (LOG_NOTICE,"replaceDollarParam Failed at %s: %i\n", (char *)t2,j);
      return(j);
    }
    t1 = t2 + 1;
  }
  return(0);
}

int
replaceVariablesNew(char *action, char *inStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei )
{
  int j;
  char *t1, *t2;

  if (strncmp(inStr,"assign(",7) == 0) 
    return(0);

  j = 0;
  t1 = inStr;


  
  while ((t2 = strstr(t1,"$")) != NULL) {
    j = replaceSessionVar(action,t2,(int) (MAX_COND_LEN - (t2 - inStr) - 1),rei);
    if (j < 0) {
      rodsLog (LOG_NOTICE,"replaceSessionVar Failed at %s: %i\n", (char *)t2,j);
      return(j);
    }
    t1 = t2 + 1;
  }
  return(j);
}

int
replaceVariablesAndMsParams(char *action, char *inStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei )
{
  int j;
  char *t1, *t2;

  t1 = inStr;
  j = 0;

  while ((t2 = strstr(t1,"$")) != NULL) {
    j = replaceSessionVar(action,t2,(int) (MAX_COND_LEN - (t2 - inStr) - 1),rei);
    if (j < 0) {
      rodsLog (LOG_NOTICE,"replaceSessionVar Failed at %s: %i\n", (char *)t2,j);
      return(j);
    }
    t1 = t2 + 1;
  }
  j  = replaceMsParams(inStr,inMsParamArray);
  return(j);
}

int
replaceMsParams(char *inStr, msParamArray_t *inMsParamArray)
{
  int j;
  char *t1, *t2;

  t1 = inStr;

  while ((t2 = strstr(t1,"*")) != NULL) {
    if (*(t2 + 1) == ' ') {
      t1 = t2 + 1;
      continue;
    }
    j = replaceStarVar(inStr,t2,(int) (MAX_COND_LEN - (t2 - inStr) - 1), inMsParamArray);
    if (j < 0) {
      rodsLog (LOG_NOTICE,"replaceMsParams Failed at %s: %i\n", (char *)t2,j);
      return(j);
    }
    t1 = t2 + 1;
  }
  return(0);
}



int
replaceDollarParam(char *action, char *dPtr, int len, 
		   char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		   ruleExecInfo_t *rei)
{
  /*char *t1,*t2,*t3;*/
  int i,j;

  i = replaceArgVar(dPtr,len, args, argc);
  if ( i != UNKNOWN_PARAM_IN_RULE_ERR)
    return(i);
  j = replaceSessionVar(action, dPtr,len,rei);
  return(j);


  /*
  if      ((j = replaceArgVar(dPtr,len, args, argc)) == 0)
    return(0);
  else if ((j = replaceDataVar(dPtr,len,rei->doi)) == 0)
    return(0);
  else if ((j = replaceCollVar(dPtr,len,rei->coi)) == 0)
    return(0);
  else if ((j = replaceUserVar(dPtr,len,rei->uoic, rei->uoip)) == 0)
    return(0);
  else if ((j = replaceRescVar(dPtr,len,rei->roi)) == 0)
    return(0);
  else
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  */
  /*
  t1 = dPtr + 1;

  i = 0;
  while(strcmp(dataObjCond[i],"ENDOFLIST")) {
    if (strstr(t1,dataObjCond[i]) == t1) {
      j =  replaceDataVar(dPtr, len,i,rei->doi);
      return (j);
    }
    else
      i++;
  }

  i = 0;
  while(strcmp(rescCond[i],"ENDOFLIST")) {
    if (strstr(t1,rescCond[i]) == t1) {
      j =  replaceRescVar(dPtr, len,i,rei->roi);
      return(j);
    }
    else
      i++;
  }
  i = 0;
  while(strcmp(userCond[i],"ENDOFLIST")) {
    if (strstr(t1,userCond[i]) == t1) {
      j =  replaceUserVar(dPtr, len,i,rei->uoi);
      return(j);
    }
    else
      i++;
  }
  i = 0;
  while(strcmp(collCond[i],"ENDOFLIST")) {
    if (strstr(t1,collCond[i]) == t1) {
      j =  replaceCollVar(dPtr, len,i,rei->coi);
      return(j);
    }
    else
      i++;
  }

  */
}


int
reREMatch(char *pat, char *str)
{
  int i;

  i = match(pat,str);
  if (i == MATCH_VALID)
    return(TRUE);
  else
    return(FALSE);
  /*
    printf("1:%sPPPP%sNNNN\n%i::%i\n",pat,str,strlen(pat),strlen(str));
  i = matche(pat,str);
  if (i == MATCH_PATTERN) {
    is_valid_pattern(pat,&j);
    if(j != MATCH_ABORT && j != MATCH_END) {
      rodsLog (LOG_NOTICE,"reREMatch:Pattern Problems:%i\n",j);
      rodsLog (LOG_NOTICE,"Pat:(%s)Str:(%s)::PatLen=%i::StrLen=%i\n",
	       pat,str,strlen(pat),strlen(str));
    }
    return(0);
  }
  else if (i == MATCH_VALID)
    return(TRUE);
  else {
    if(i != MATCH_ABORT && i != MATCH_END) {
      rodsLog (LOG_NOTICE,"reREMatch:Match Problem:%i\n",i);
      rodsLog (LOG_NOTICE,"Pat:(%s)Str:(%s)::PatLen=%i::StrLen=%i\n",
	       pat,str,strlen(pat),strlen(str));
    }
    return(FALSE);
  }
  */

}


int initializeReDebug(rsComm_t *svrComm, int flag) 
{
  char condRead[NAME_LEN];
  int i, s,m, status;
  char *readhdr = NULL;
  char *readmsg = NULL;
  char *user = NULL;
  char *addr = NULL;

  if (svrComm == NULL)
    return(0);
  if ( GlobalREDebugFlag != 4 ) 
    return(0);

  s=0;
  m=0;
  myPID = (int) getpid();
  myHostName[0] = '\0';
  gethostname (myHostName, MAX_NAME_LEN);
  sprintf(condRead, "(*XUSER  == %s@%s) && (*XHDR == STARTDEBUG)",
	  svrComm->clientUser.userName, svrComm->clientUser.rodsZone);
  status = _readXMsg(GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr);
  if (status >= 0) {
    if ( (readmsg != NULL)  && strlen(readmsg) > 0) {
      GlobalREDebugFlag = atoi(readmsg);
    }
    if (readhdr != NULL)
      free(readhdr);
    if (readmsg != NULL)
      free(readmsg);
    if (user != NULL)
      free(user);
    if (addr != NULL)
      free(addr);
    /* initialize reDebug stack space*/
    for (i = 0; i < REDEBUG_STACK_SIZE_FULL; i++) 
      reDebugStackFull[i] = NULL;
    for (i = 0; i < REDEBUG_STACK_SIZE_CURR; i++)
      reDebugStackCurr[i] = NULL;
    reDebugStackFullPtr = 0;
    reDebugStackCurrPtr = 0;
    snprintf(waitHdr,HEADER_TYPE_LEN - 1,   "idbug:");

    rodsLog (LOG_NOTICE,"reDebugInitialization: Got Debug StreamId:%i\n",GlobalREDebugFlag);
    snprintf(waitMsg, MAX_NAME_LEN, "PROCESS BEGIN at %s:%i. Client connected from %s at port %i\n", 
	     myHostName, myPID, svrComm->clientAddr,ntohs(svrComm->localAddr.sin_port));
    _writeXMsg(GlobalREDebugFlag, "idbug", waitMsg);
    snprintf(waitMsg, MAX_NAME_LEN, "%s:%i is waiting\n", myHostName, myPID);  }
  return(0);
}


int processXMsg(int streamId, int *msgNum, int *seqNum, 
		char * readhdr, char *readmsg, 
		char *callLabel, int flag, char *hdr, char *actionStr, char *seActionStr,
		msParamArray_t *inMsParamArray, int curStat, ruleExecInfo_t *rei)
{
  char cmd;
  char myhdr[HEADER_TYPE_LEN];
  char mymsg[MAX_NAME_LEN];
  char *outStr = NULL;
  char *ptr;
  msParam_t *mP;
  int i,n;
  int iLevel, wCnt;
  int  ruleInx = -1;
  char ruleCondition[MAX_RULE_LENGTH];
  char ruleAction[MAX_RULE_LENGTH];
  char ruleRecovery[MAX_RULE_LENGTH];
  char ruleHead[MAX_ACTION_SIZE];
  char ruleBase[MAX_ACTION_SIZE];
  char *actionArray[MAX_ACTION_IN_RULE];
  char *recoveryArray[MAX_ACTION_IN_RULE];

  snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);
  cmd = readmsg[0];

  switch(cmd) {
  case 'n': /* next */
    return(REDEBUG_NEXT);
    break;
  case 'd': /* discontinue. stop now */
    return(REDEBUG_WAIT);
    break;
  case 'c': /* continue */
    return(REDEBUG_CONTINUE);
    break;
  case 'C': /* continue */
    return(REDEBUG_CONTINUE_VERBOSE);
    break;
  case 'b': /* break point */
    if (breakPointsInx == 99) {
      _writeXMsg(streamId, myhdr, "Maximum Breakpoint Count reached. Breakpoint not set\n");
      return(REDEBUG_WAIT);
    }

    breakPoints[breakPointsInx] = strdup((char *)(readmsg + 1));
    trimWS(breakPoints[breakPointsInx]);
    snprintf(mymsg, MAX_NAME_LEN, "Breakpoint %i  set at %s\n", breakPointsInx,
	   breakPoints[breakPointsInx]);
    _writeXMsg(streamId, myhdr, mymsg);
    breakPointsInx++;
    return(REDEBUG_WAIT);
    break;
  case 'e': /* examine * and $ variables */
  case 'p': /* print * and $ variables */
    ptr = (char *)(readmsg + 1);
    trimWS(ptr);
    snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Printing %s\n",ptr);
    if (*ptr == '*') {
      mymsg[0] = '\n';
      mymsg[1] = '\0';
      if (rei != NULL  && rei->msParamArray != NULL ) {
	if ((mP = getMsParamByLabel (rei->msParamArray, ptr)) == NULL) {
	  snprintf(mymsg, MAX_NAME_LEN - 1 , "Parameter %s not found\n",ptr);
	  _writeXMsg(streamId, myhdr, mymsg);
	}
	else {
	  writeMsParam (mymsg, MAX_NAME_LEN, mP);
	  _writeXMsg(streamId, myhdr, mymsg);
	}
      }
      else {
        snprintf(mymsg, MAX_NAME_LEN, "MsParamArray is Empty\n");
	_writeXMsg(streamId, myhdr, mymsg);
      }

      return(REDEBUG_WAIT);
    }
    else  if (*ptr == '$') {
      ptr++;
      i = getSessionVarValue ("", ptr, rei, &outStr);
      if (outStr != NULL) {
	snprintf(mymsg, MAX_NAME_LEN , "\nVariable $%s = %s\n", ptr, outStr);
	free(outStr);
      }
      else {
	snprintf(mymsg, MAX_NAME_LEN , "\nVariable $%s not found\n", ptr);
      }
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    else {
      snprintf(mymsg, MAX_NAME_LEN, "Unknown Parameter Type");
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    break;
  case 'l': /* list rule and * and $ variables */
    ptr = (char *)(readmsg + 1);
    trimWS(ptr);
    snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",ptr);
    if (*ptr == '*') {
      mymsg[0] = '\n';
      mymsg[1] = '\0';
      if (rei != NULL  && rei->msParamArray != NULL ) {
	
	for (i = 0;  i < rei->msParamArray->len; i++) {
	  if (rei->msParamArray->msParam[i]->type != NULL)
	    snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "%s of type %s\n", 
		     rei->msParamArray->msParam[i]->label, rei->msParamArray->msParam[i]->type);
	  else
	    snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "%s of type NULL \n", rei->msParamArray->msParam[i]->label);
	}
      }
      else {
	snprintf(mymsg, MAX_NAME_LEN, "\n MsParamArray is Empty\n");
      }
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    else  if (*ptr == '$') {
      mymsg[0] = '\n';
      mymsg[1] = '\0';
      for (i= 0; i < coreRuleVarDef .MaxNumOfDVars; i++) {
	snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "$%s\n", coreRuleVarDef.varName[i]);
      }
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    else if (*ptr == 'r') {
      ptr ++;
      trimWS(ptr);
      mymsg[0] = '\n';
      mymsg[1] = '\0';
      snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",ptr);
      while ((n = findNextRule (ptr, &ruleInx)) >= 0) {
	getRule(ruleInx, ruleBase, ruleHead, ruleCondition,ruleAction,ruleRecovery, MAX_RULE_LENGTH);
	n = 0;
	n = getActionRecoveryList(ruleAction,ruleRecovery,actionArray,recoveryArray);
	if (strlen(ruleCondition) != 0) 
	  snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "\n  %i: %s.%s\n      IF (%s) {\n",ruleInx, ruleBase, ruleHead,ruleCondition);
	else 
	  snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "\n  %i: %s.%s\n      {\n",ruleInx, ruleBase, ruleHead);
	for (i = 0; i < n; i++) {
	  if (strlen(actionArray[i]) < 20) {
	    if (recoveryArray[i] == NULL || 
		strlen(recoveryArray[i]) == 0 || 
		!strcmp(recoveryArray[i],"nop") || 
		!strcmp(recoveryArray[i],"null")) 
	      snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "        %-20.20s\n",actionArray[i]);
	    else
	      snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "        %-20.20s[%s]\n",actionArray[i],recoveryArray[i]);
	  }
	  else {
	    if (recoveryArray[i] == NULL || 
		strlen(recoveryArray[i]) == 0 || 
		!strcmp(recoveryArray[i],"nop") || 
		!strcmp(recoveryArray[i],"null")) 
	      snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "        %s\n",actionArray[i]);
	    else
	      snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "        %s       [%s]\n",actionArray[i],recoveryArray[i]);
	  }

	}
	snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "      }\n");
      }
      if (ruleInx == -1) {
	snprintf(mymsg, MAX_NAME_LEN,"\n  Rule %s not found\n", ptr);
      }
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    else {
      snprintf(mymsg, MAX_NAME_LEN, "Unknown Parameter Type");
      _writeXMsg(streamId, myhdr, mymsg);
      return(REDEBUG_WAIT);
    }
    break;
  case 'w': /* where are you now in CURRENT stack*/
    wCnt = 20;
    iLevel = 0;
    ptr = (char *)(readmsg + 1);
    trimWS(ptr);
    if (strlen(ptr) > 0)
      wCnt = atoi(ptr);

    i = reDebugStackCurrPtr - 1;
    while (wCnt > 0 && reDebugStackCurr[i] != NULL) {
      snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Level %3i",iLevel);
      _writeXMsg(streamId,  myhdr, reDebugStackCurr[i] );
      if (strstr(reDebugStackCurr[i] , "ApplyRule") != NULL)
	iLevel++;
      wCnt--;
      i--;
      if (i < 0) 
	i = REDEBUG_STACK_SIZE_CURR - 1;
    }
    return(REDEBUG_WAIT);
    break;
  case 'W': /* where are you now in FULL stack*/
    wCnt = 20;
    iLevel = 0;
    ptr = (char *)(readmsg + 1);
    trimWS(ptr);
    if (strlen(ptr) > 0)
      wCnt = atoi(ptr);

    i = reDebugStackFullPtr - 1;
    while (wCnt > 0 && reDebugStackFull[i] != NULL) {
      snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Step %4i",iLevel);
      _writeXMsg(streamId,  myhdr, reDebugStackFull[i] );
      iLevel++;
      wCnt--;
      i--;
      if (i < 0) 
	i = REDEBUG_STACK_SIZE_CURR - 1;
    }
    return(REDEBUG_WAIT);
    break;
  default:
    snprintf(mymsg, MAX_NAME_LEN, "Unknown Action: %s.", readmsg );
    _writeXMsg(streamId, myhdr, mymsg);
    return(REDEBUG_WAIT);
    break;

  }
  return(curStat);
}

int
processBreakPoint(int streamId, int *msgNum, int *seqNum,
		  char *callLabel, int flag, char *hdr, char *actionStr, char *seActionStr,
		  msParamArray_t *inMsParamArray, int curStat, ruleExecInfo_t *rei)
{

  int i;
  char myhdr[HEADER_TYPE_LEN];
  char mymsg[MAX_NAME_LEN];

  snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);


  if (breakPointsInx > 0) {
    for (i = 0; i < breakPointsInx; i++) {
      if (strstr(actionStr, breakPoints[i]) ==  actionStr) {
	snprintf(mymsg,MAX_NAME_LEN, "Breaking at BreakPoint %i:%s\n", i , breakPoints[i]);
	_writeXMsg(streamId, myhdr, mymsg);
	snprintf(mymsg,MAX_NAME_LEN, "with values: %s\n", seActionStr);
	_writeXMsg(streamId, myhdr, mymsg);
	curStat = REDEBUG_WAIT;
	return(curStat);
      }
    }
  }
  return(curStat);
}


int
storeInStack(char *hdr, char* step)
{
  char *stackStr;
  char *stackStr2;
  char *s;
  int i;


  stackStr = (char *) malloc(strlen(hdr) + strlen(step) + 5);
  sprintf(stackStr,"%s: %s\n", hdr, step);


  i = reDebugStackFullPtr;
  if (reDebugStackFull[i] != NULL) {
    free(reDebugStackFull[i]);
  }
  reDebugStackFull[i] = stackStr;
  reDebugStackFullPtr = (i + 1 ) % REDEBUG_STACK_SIZE_FULL;
  i = reDebugStackFullPtr;
  if (reDebugStackFull[i] != NULL) {
    free(reDebugStackFull[i]);
    reDebugStackFull[i] = NULL;
  }

  if (strstr(hdr,"Done") == hdr) { /* Pop the stack */
    s = (char *) hdr + 4;
    i = reDebugStackCurrPtr - 1;
    if (i < 0)
      i = REDEBUG_STACK_SIZE_CURR - 1;
    while (reDebugStackCurr[i] != NULL && strstr(reDebugStackCurr[i] , s) != reDebugStackCurr[i]) {
      free(reDebugStackCurr[i]);
      reDebugStackCurr[i] = NULL;
      i = i - 1;
      if (i < 0)
	i = REDEBUG_STACK_SIZE_CURR - 1;
    }
    if (reDebugStackCurr[i] != NULL) {
      free(reDebugStackCurr[i]);
      reDebugStackCurr[i] = NULL;
    }
    reDebugStackCurrPtr = i;
    return(0);
  }

  stackStr2 = strdup(stackStr);
  i = reDebugStackCurrPtr;
  if (reDebugStackCurr[i] != NULL) {
    free(reDebugStackCurr[i]);
  }
  reDebugStackCurr[i] = stackStr2;
  reDebugStackCurrPtr = (i + 1 ) % REDEBUG_STACK_SIZE_CURR;
  i = reDebugStackCurrPtr;
  if (reDebugStackCurr[i] != NULL) {
    free(reDebugStackCurr[i]);
    reDebugStackCurr[i] = NULL;
  }
  return(0);
}


int sendWaitXMsg (int streamId) {
  _writeXMsg(streamId, waitHdr, waitMsg);
  return(0);
}
int cleanUpDebug(int streamId) {
  int i;
  for (i = 0 ; i < REDEBUG_STACK_SIZE_CURR; i++) {
    if (reDebugStackCurr[i] != NULL) {
      free(reDebugStackCurr[i]);
      reDebugStackCurr[i] = NULL;
    }
  }
  for (i = 0 ; i < REDEBUG_STACK_SIZE_FULL; i++) {
    if (reDebugStackFull[i] != NULL) {
      free(reDebugStackFull[i]);
      reDebugStackFull[i] = NULL;
    }
  }
  reDebugStackCurrPtr = 0;
  reDebugStackFullPtr = 0;
  GlobalREDebugFlag = -1;
  return(0);
}

int
reDebug(char *callLabel, int flag, char *actionStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei)
{
  int i, m, s, status, sleepT, j;
  int processedBreakPoint = 0;
  char hdr[HEADER_TYPE_LEN];
  char *readhdr = NULL;
  char *readmsg = NULL;
  char *user = NULL;
  char *addr = NULL;
  static int mNum = 0;
  static int sNum = 0;
  static int curStat = 0; 
  char condRead[MAX_NAME_LEN];
  char myActionStr[10][MAX_NAME_LEN + 10];
  int aNum = 0;
  char seActionStr[10 * MAX_NAME_LEN + 100];
  rsComm_t *svrComm;
  int waitCnt = 0;
  sleepT = 1;
  svrComm = rei->rsComm;

  snprintf(hdr, HEADER_TYPE_LEN - 1,   "iaudit:%s",callLabel);
  condRead[0] = '\0'; 
  rodsLog (LOG_NOTICE,"PPP:%s\n",hdr);
  strcpy(seActionStr, actionStr);
  if (GlobalREAuditFlag > 0) {
    if (flag == -4) {
      if (rei->uoic != NULL && rei->uoic->userName != NULL && rei->uoic->rodsZone != NULL) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  USER:%s@%s", rei->uoic->userName, rei->uoic->rodsZone);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->objPath != NULL && strlen(rei->doi->objPath) > 0 ) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  DATA:%s", rei->doi->objPath);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->filePath != NULL && strlen(rei->doi->filePath) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  FILE:%s", rei->doi->filePath);
	aNum++;
      }
      if (rei->doinp != NULL && rei->doinp->objPath != NULL && strlen(rei->doinp->objPath) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  DATAIN:%s",rei->doinp->objPath);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->rescName != NULL && strlen(rei->doi->rescName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESC:%s",rei->doi->rescName);
	aNum++;
      }
      if (rei->rgi != NULL && rei->rgi->rescInfo->rescName != NULL && strlen(rei->rgi->rescInfo->rescName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESC:%s",rei->rgi->rescInfo->rescName);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->rescGroupName != NULL && strlen(rei->doi->rescGroupName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESCGRP:%s",rei->doi->rescGroupName);
	aNum++;
      }
      if (rei->rgi != NULL && rei->rgi->rescGroupName != NULL && strlen(rei->rgi->rescGroupName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESCGRP:%s",rei->rgi->rescGroupName);
	aNum++;
      }
      if (rei->coi != NULL && rei->coi->collName != NULL) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  COLL:%s", rei->coi->collName);
	aNum++;
      }
      for (j = 0; j < aNum; j++) {
	strncat(seActionStr, myActionStr[j], 10 * MAX_NAME_LEN + 100);
      }
    }
  }

  /* Write audit trail */
  if (GlobalREAuditFlag == 3) {
    i = _writeXMsg(GlobalREAuditFlag, hdr, actionStr);
  }

  /* Send current position for debugging */
  if ( GlobalREDebugFlag > 5 ) {
    if (curStat != REDEBUG_CONTINUE) {
      snprintf(hdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);
      i = _writeXMsg(GlobalREDebugFlag, hdr, actionStr);
    }
  }

  /* store in stack */
  storeInStack(callLabel, actionStr);

  while ( GlobalREDebugFlag > 5 ) {
    s = sNum;
    m = mNum;
    /* what should be the condition */
    sprintf(condRead, "(*XSEQNUM >= %d) && (*XADDR != %s:%i) && (*XUSER  == %s@%s) && ((*XHDR == CMSG:ALL) %%%% (*XHDR == CMSG:%s:%i))",
	    s, myHostName, myPID, svrComm->clientUser.userName, svrComm->clientUser.rodsZone, myHostName, myPID);

    /*
    sprintf(condRead, "(*XSEQNUM >= %d)  && ((*XHDR == CMSG:ALL) %%%% (*XHDR == CMSG:%s:%i))",
	    s,  myHostName, getpid());
    */
    status = _readXMsg(GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr);
    if (status == SYS_UNMATCHED_XMSG_TICKET) {
      cleanUpDebug(GlobalREDebugFlag);
      return(0);
    }
    if (status  >= 0) {
      rodsLog (LOG_NOTICE,"Getting XMsg:%i:%s:%s\n", s,readhdr, readmsg);
      curStat = processXMsg(GlobalREDebugFlag, &m, &s, readhdr, readmsg,
			    callLabel, flag, hdr,  actionStr, seActionStr, 
			    inMsParamArray, curStat, rei);
      if (readhdr != NULL)
	free(readhdr);
      if (readmsg != NULL)
	free(readmsg);
      if (user != NULL)
	free(user);
      if (addr != NULL)
	free(addr);

      mNum = m;
      sNum = s + 1;
      if (curStat == REDEBUG_WAIT) {
	sendWaitXMsg(GlobalREDebugFlag);
      }
      if (curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE) {
	if (processedBreakPoint == 1)
	  break;
	curStat = processBreakPoint(GlobalREDebugFlag, &m, &s, 
				   callLabel, flag, hdr,  actionStr, seActionStr,
				   inMsParamArray, curStat, rei);
	processedBreakPoint = 1;
	if (curStat == REDEBUG_WAIT) {
	  sendWaitXMsg(GlobalREDebugFlag);
	  continue;
	}
	else
	  break;
      }
      else if (curStat == REDEBUG_NEXT || curStat == REDEBUG_STEP )
	break;
    }
    else {
      if (curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE) {
	
	rodsLog(LOG_NOTICE, "Calling 2: %s,%p",actionStr, &actionStr);
	curStat = processBreakPoint(GlobalREDebugFlag, &m, &s, 
				    callLabel, flag, hdr,  actionStr, seActionStr,
				    inMsParamArray, curStat, rei);
	processedBreakPoint = 1;
	if (curStat == REDEBUG_WAIT) {
	  sendWaitXMsg(GlobalREDebugFlag);
	  continue;
	}
	else
	  break;
      }
      sleep(sleepT);
      sleepT = 2 * sleepT;
      /* if (sleepT > 10) sleepT = 10;*/
      if (sleepT > 1){
	sleepT = 1;
	waitCnt++;
	if (waitCnt > 60) {
	  sendWaitXMsg(GlobalREDebugFlag);
	  waitCnt = 0;
	}
      }
    }
  }


  return(0);
}
