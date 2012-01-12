/**
 * @file systemMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <string.h>
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"
#include "execMyRule.h"

int
fillSubmitConditions (char *action, char *inDelayCondition, bytesBuf_t *packedReiAndArgBBuf, 
        ruleExecSubmitInp_t *ruleSubmitInfo,  ruleExecInfo_t *rei);

/**
 * \fn assign(msParam_t* var, msParam_t* value, ruleExecInfo_t *rei)
 *
 * \brief  This microservice assigns a value to a variable.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note This microservice can be used to assign values to * and $ variables. 
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest8.ir
 *
 * myTestRule||assign(*A,0)##whileExec( *A < 20 , assign(*A, *A + 4), nop)|nop##nop 
 * *A=1000%*D= (199 * 2) + 200
 * *Action%*Condition%*A%*B%*C%*D%*E%ruleExecOut
 *
 * Also:
 *
 * myTestRule||assign(*A,0)##whileExec(*A < 20, writeLine(stdout, *A)##assign(*A, *A + 4), nop)|nop##nop
 * null
 * ruleExecOut
 * 
 * \param[in] var - var is a msParam of type STR_MS_T which is a variable name or a Dollar Variable.
 * \param[in] value - value is a msParam of type STR_MS_T that is computed and value assigned to variable.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence depends upon the variable being modified
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int assign(msParam_t* var, msParam_t* value, ruleExecInfo_t *rei)
{
  char *varName;
  char *varMap;
  char *varValue;
  char aVal[MAX_COND_LEN * 2];
  int i;
  char eaVal[MAX_COND_LEN * 2];
  varName = (char *)var->label;
  varValue = (char *) var->inOutStruct;
  if ( varValue != NULL && varValue[0] == '$') {
    i = getVarMap("", (char*)var->inOutStruct, &varMap, 0);
    if (i < 0) return(i);
    rstrcpy(aVal,(char*)value->inOutStruct,MAX_COND_LEN * 2);
    i = evaluateExpression(aVal, eaVal,rei);
    if (i < 0 ) 
      return(i);
    /*    if (strcmp(eaVal, aVal))*/
    rodsLog (LOG_NOTICE,"BEFORE:uioc=%s,uiop=%s:cp=%d,pp=%d,eaval=%s\n",rei->uoic->userName,rei->uoip->userName, rei->uoic,rei->uoip,eaVal);
    i = setVarValue(varMap,rei, eaVal);
    rodsLog (LOG_NOTICE,"AFTER:uioc=%s,uiop=%s:cp=%d,pp=%d\n",rei->uoic->userName,rei->uoip->userName, rei->uoic,rei->uoip);
  }
  else { /* it is a *-variable so just copy */
    if ((value->type == NULL && value->label != NULL) ||
	strcmp (value->type, STR_MS_T) == 0 ||
	strcmp (value->type, STR_PI) == 0 ) {
      if (value->inOutStruct != NULL)
	rstrcpy((char*)aVal,(char*)value->inOutStruct,MAX_COND_LEN * 2);
      else
	rstrcpy((char*)aVal,(char*)value->label,MAX_COND_LEN * 2);
      i = evaluateExpression(aVal, eaVal,rei);
      if (i < 0)
	fillMsParam (var, NULL, value->type, value->inOutStruct, value->inpOutBuf);
      else if (!strcmp(eaVal,aVal)) {
	var->inOutStruct = strdup(aVal);
	var->type = strdup(STR_MS_T);
      }
      else {
	/*	fillIntInMsParam(var,j); */
	var->inOutStruct = strdup(eaVal);
	var->type = strdup(STR_MS_T);
      }
    }
    else
      fillMsParam (var, NULL, value->type, value->inOutStruct, value->inpOutBuf);
  }
  return(0);
}


/**
 * \fn whileExec(msParam_t* condition, msParam_t* whileBody, msParam_t* recoverWhileBody, ruleExecInfo_t *rei)
 *
 * \brief  It is a while loop in the rule language. It executes a while loop.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note The first argument is a condition that will be checked on each loop iteration.
 *  The second argument is the body of the while loop, given as a sequence of
 *  microservices, and the third argument is the recoveryBody for recovery
 *  from failures.
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest33.ir
 *
 * myTestRule||assign(*A,0)##whileExec( *A < 20 , ifExec(*A > 10, assign(*A,400)##break,nop,nop,nop)##assign(*A, *A + 4), writeLine(stdout,*A) nop)|nop##nop 
 * *A=1000%*D= (199 * 2) + 200
 * *Action%*Condition%*A%*B%*C%*D%*E%ruleExecOut
 * 
 * Also:
 *
 * myTestRule||assign(*A,0)##whileExec(*A < 20, writeLine(stdout, *A)##assign(*A, *A + 4), nop)|nop##nop
 * null
 * ruleExecOut
 *
 * \param[in] condition - a msParam of type STR_MS_T which is a logical expression computing to TRUE or FALSE.
 * \param[in] whileBody - a msParam of type STR_MS_T which is a statement list - microservices and ruleNames separated by ##
 * \param[in] recoverWhileBody - a msParam of type STR_MS_T which is a statement list  as above.
 * \param[in,out] rei Required - the RuleExecInfo structure.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int whileExec(msParam_t* condition, msParam_t* whileBody,
	      msParam_t* recoverWhileBody, ruleExecInfo_t *rei)
{
  int i,done;
  char *cond, *ruleBody;
  msParamArray_t *inMsParamArray = NULL;
  char eaVal[MAX_COND_LEN * 2];
  done = 0;
  inMsParamArray = rei->msParamArray;
  /*  cond = (char *) malloc(strlen(condition->label) + 10 + MAX_COND_LEN * 2); */
  cond = (char *) malloc(strlen((char*)condition->inOutStruct) + 10 + MAX_COND_LEN * 2);
  ruleBody = (char *) malloc ( strlen((char*)whileBody->inOutStruct) +
			       strlen((char*)recoverWhileBody->inOutStruct) + 10 + MAX_COND_LEN * 2);
  sprintf(ruleBody,"%s|%s",(char *) whileBody->inOutStruct,
	    (char *) recoverWhileBody->inOutStruct + MAX_COND_LEN * 8);
    
   while (done == 0) {
    /* strcpy(cond , condition->label); */
    strcpy(cond , (char*)condition->inOutStruct);
    i = evaluateExpression(cond, eaVal, rei);
    if (i < 0)
      return(i);
    if (i == 0) { /* FALSE */
      done = 1;
      break;
    }    
    i = execMyRuleWithSaveFlag(ruleBody, inMsParamArray, rei, 0);
    if (i == BREAK_ACTION_ENCOUNTERED_ERR) 
      return(0);
    if (i != 0) 
      return(i);
    /*  RECOVERY OF OTHER EXECUTED BODY IS NOT DONE. HOW CAN WE DO THAT */
  }
  return(0);
}


/**
 * \fn forExec(msParam_t* initial, msParam_t* condition, msParam_t* step, msParam_t* forBody, msParam_t* recoverForBody, ruleExecInfo_t *rei)
 *
 * \brief  It is a for loop in rule language.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note  This micro-service provides a looping over an integer *-variable until a condition is met. Similar to for construct in C 
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest9.ir
 *
 * myTestRule||forExec(assign(*A,0), *A < *D , assign(*A,*A + 4), writeLine(stdout,*A),nop)|nop
 * *A=1000%*D= (199 * 2) + 200
 * *Action%*Condition%*A%*D%ruleExecOut
 *
 * \param[in] initial - a msParam of type STR_MS_T which is an nitial assignment statement for the loop variable.
 * \param[in] condition - a msParam of type STR_MS_T which is a logical expression checking a condition.
 * \param[in] step - a msParam of type STR_MS_T which is an increment/decrement of loop variable.
 * \param[in] forBody - a msParam of type STR_MS_T which is a statement list - micro-services and ruleNames separated by ##.
 * \param[in] recoverForBody - a msParam of type STR_MS_T which is a statement list as above.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int forExec(msParam_t* initial, msParam_t* condition, msParam_t* step, 
	    msParam_t* forBody, msParam_t* recoverForBody, ruleExecInfo_t *rei)
{

  int i,done;
  char *cond, *ruleBody, *stepStr;
  char eaVal[MAX_COND_LEN * 2];
  done = 0;
  /*  i = execMyRuleWithSaveFlag(initial->label,rei->msParamArray, rei,0); */
  i = execMyRuleWithSaveFlag((char*)initial->inOutStruct,rei->msParamArray, rei,0); 
  if (i != 0)
    return(i);
  /*  cond = (char *) malloc(strlen(condition->label) + 10 + MAX_COND_LEN * 2); */
  cond = (char *) malloc(strlen((char*)condition->inOutStruct) + 10 + MAX_COND_LEN * 2);
  /* stepStr = (char *) malloc(strlen(step->label) + 10 + MAX_COND_LEN * 2); */
  stepStr = (char *) malloc(strlen((char*)step->inOutStruct) + 10 + MAX_COND_LEN * 2);
  ruleBody = (char *) malloc ( strlen((char*)forBody->inOutStruct) +
                                 strlen((char*)recoverForBody->inOutStruct) + 10 + MAX_COND_LEN * 8);
  
  while (done == 0) {
    /* strcpy(cond , condition->label); */
    strcpy(cond , (char*)condition->inOutStruct);
    i = evaluateExpression(cond, eaVal, rei);
    if (i < 0)
      return(i);
    if (i == 0) { /* FALSE */
      done = 1;
      break;
    }
    sprintf(ruleBody,"%s|%s",(char *) forBody->inOutStruct,
            (char *) recoverForBody->inOutStruct);
    i = execMyRuleWithSaveFlag(ruleBody, rei->msParamArray, rei, 0);
    if (i == BREAK_ACTION_ENCOUNTERED_ERR) 
      return(0);
    if (i != 0)
      return(i);
    /* strcpy(cond , condition->label); */
    strcpy(stepStr,(char*)step->inOutStruct);
    i = execMyRuleWithSaveFlag(stepStr,rei->msParamArray, rei,0);
    if (i != 0)
      return(i);
  }
  return(0);
}

/**
 * \fn ifExec(msParam_t* condition, msParam_t* thenC, msParam_t* recoverThen, msParam_t* elseC, msParam_t* recoverElse, ruleExecInfo_t *rei)
 *
 * \brief  It is an if-then-else construct in the rule language. It executes an
 *    if-then-else statement.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note The first argument is a conditional check.  If the
 *    check is successful (TRUE), the microservice sequence in the second argument
 *    will be executed. If the check fails, then the microservice sequence in the
 *    fourth argument will be executed. The third argument is the recoveryBody for
 *    recovery from failures for the then-part, and the fifth argument is the
 *    recoveryBody for recovery from failures for the else-part.
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest33.ir
 *
 * myTestRule||assign(*A,0)##whileExec( *A < 20 , ifExec(*A > 10, assign(*A,400)##break,nop,nop,nop)##assign(*A, *A + 4), writeLine(stdout,*A) nop)|nop##nop 
 * *A=1000%*D= (199 * 2) + 200
 * *Action%*Condition%*A%*B%*C%*D%*E%ruleExecOut
 *
 * \param[in] condition - a msParam of type STR_MS_T which is a logical expression computing to TRUE or FALSE.
 * \param[in] thenC - a msParam of type STR_MS_T which is a statement list - microservices and ruleNames separated by ##
 * \param[in] recoverThen - a msParam of type STR_MS_T which is a tatement list - microservices and ruleNames separated by ##
 * \param[in] elseC - a msParam of type STR_MS_T which is a statement list - microservices and ruleNames separated by ##.
 * \param[in] recoverElse - a msParam of type STR_MS_T which is a statement list - microservices and ruleNames separated by ##
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int ifExec(msParam_t* condition, msParam_t* thenC, msParam_t* recoverThen, 
	   msParam_t* elseC, msParam_t* recoverElse, ruleExecInfo_t *rei)
{
  int i;  
  char *cond, *thenStr, *elseStr;
  char eaVal[MAX_COND_LEN * 2];

  if (condition->inOutStruct == NULL) {
    /* cond = (char *) malloc(strlen(condition->label) + 10 + MAX_COND_LEN * 2);
       strcpy(cond , condition->label); */
    cond = (char *) malloc(strlen((char*)condition->inOutStruct) + 10 + MAX_COND_LEN * 2);
    strcpy(cond , (char*)condition->inOutStruct);
  }
  else {
    cond = (char *) malloc(strlen((char*)condition->inOutStruct) + 10 + MAX_COND_LEN * 2);
    strcpy(cond , (char*)condition->inOutStruct);
  }

  i = evaluateExpression(cond, eaVal, rei);
  free(cond);
  if (i < 0)
    return(i);
  if (i == 1) { /* TRUE */
    thenStr = (char *) malloc(strlen((char*)thenC->inOutStruct) + strlen((char*)recoverThen->inOutStruct) + 10 + MAX_COND_LEN * 8);
    sprintf(thenStr,"%s|%s",(char *) thenC->inOutStruct,
            (char *) recoverThen->inOutStruct);
    i = execMyRuleWithSaveFlag(thenStr, rei->msParamArray, rei, 0);
    free(thenStr);
  }
  else {
    elseStr = (char *) malloc(strlen((char*)elseC->inOutStruct) + strlen((char*)recoverElse->inOutStruct) + 10 + MAX_COND_LEN * 8);
    sprintf(elseStr,"%s|%s",(char *) elseC->inOutStruct,
            (char *) recoverElse->inOutStruct);
    i = execMyRuleWithSaveFlag(elseStr, rei->msParamArray, rei, 0);
    free(elseStr);
  }
  return(i);

}

/**
 * \fn breakExec(ruleExecInfo_t *rei)
 *
 * \brief  This microservice is used to break whileExec, forExec and forEachExec loops.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-29
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note This micro-service is used to break the  looping when performing while for or for each operations. Similar to break statement in C.
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest33.ir
 *
 * myTestRule||assign(*A,0)##whileExec( *A < 20 , ifExec(*A > 10, assign(*A,400)##break,nop,nop,nop)##assign(*A, *A + 4), writeLine(stdout,*A) nop)|nop##nop 
 * *A=1000%*D= (199 * 2) + 200
 * *Action%*Condition%*A%*B%*C%*D%*E%ruleExecOut
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval BREAK_ACTION_ENCOUNTERED_ERR (used internally to break the loops)
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int breakExec(ruleExecInfo_t *rei)
{
  return (BREAK_ACTION_ENCOUNTERED_ERR);
}

/**
 * \fn forEachExec(msParam_t* inlist, msParam_t* body, msParam_t* recoverBody, ruleExecInfo_t *rei)
 *
 * \brief Performs a loop over a list of items given in different forms.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note It is a for loop in C-type language looping over a list. It takes a table
 *    (or list of strings, or %-separated string list), and for each item in the list,
 *    executes the corresponding body of the for-loop. The first parameter specifies the
 *    variable that has the list (the same variable name is used in the body of the loop
 *    to denote an item of the list!). The second parameter is the body given as a
 *    sequence of Micro-Services, and the third parameter is the recoveryBody for
 *    recovery from failures.

 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest14.ir
 *
 * myTestRule||forEachExec(*A,writeLine(stdout,*A),nop)|nop
 * *A=123,345,567,aa,bb,678
 * *Action%*Condition%*A%*B%*C%*D%*E%ruleExecOut
 *
 * \param[in] inlist - a msParam of type STR_MS_T which is a comma separated
 *    string or StrArray_MS_T which is an array of string or IntArray_MS_T which
 *    is an array of integers or GenQueryOut_MS_T which is an iCAT query result.
 * \param[in] body - a msParam of type STR_MS_T which is a statement
 *    list - micro-services and ruleNames separated by ##.
 * \param[in] recoverBody - a msParam of type STR_MS_T
 *    which is a statement list as above.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int forEachExec(msParam_t* inlist, msParam_t* body, msParam_t* recoverBody,
	      ruleExecInfo_t *rei)
{

  char *label ;
  int i;
  msParamArray_t *inMsParamArray;
  void *value, *restPtr, *inPtr, *buf, *inStructPtr, *msParamStruct ;
  char *typ;
  char *bodyStr;
  msParam_t *msParam, *list;
  bytesBuf_t *inBufPtr, *msParamBuf;
  int first = 1;
  int inx;
  char *outtyp;
  execCmdOut_t *myExecCmdOut;

  list = (msParam_t *) malloc (sizeof (msParam_t));
  memset (list, 0, sizeof (msParam_t));
  replMsParam (inlist,list);
  typ = strdup(list->type);
  label = list->label;
  inPtr = list->inOutStruct;
  if (!strcmp(typ,ExecCmdOut_MS_T)) {
    free(typ);
    typ = strdup(STR_MS_T);
    myExecCmdOut = (execCmdOut_t *) inPtr;
    inPtr =  myExecCmdOut->stdoutBuf.buf;
  }
  inStructPtr = inPtr;
  inBufPtr = list->inpOutBuf;
  outtyp = typ;
  buf = inBufPtr;
  value = NULL;

  inMsParamArray = rei->msParamArray;
  msParam = getMsParamByLabel(inMsParamArray, label);
  if (msParam != NULL) {
    msParamStruct = msParam->inOutStruct;
    msParamBuf = msParam->inpOutBuf;

  }
  else 
    msParamStruct = NULL;

  i = 0;

  bodyStr = (char *) malloc ( strlen((char*)body->inOutStruct) +
                               strlen((char*)recoverBody->inOutStruct) + 10 + MAX_COND_LEN * 8);
  sprintf(bodyStr,"%s|%s",(char *) body->inOutStruct,
	    (char *) recoverBody->inOutStruct);
    
  restPtr = NULL;
  inx = 0;
  while (inPtr != NULL) {
    i = getNextValueAndBufFromListOrStruct(typ, inPtr, &value, (bytesBuf_t **) &buf, &restPtr, &inx, &outtyp);
    if ( i != 0)
      break;

    if (first == 1 && msParam == NULL) {
      addMsParam(inMsParamArray, label, outtyp ,value, (bytesBuf_t*)buf);
      msParam = getMsParamByLabel(inMsParamArray, label);
      first = 0;
    }
    else {
      msParam->inOutStruct = value;
      msParam->inpOutBuf = (bytesBuf_t*)buf;
      msParam->type = outtyp;
    }


    i = execMyRuleWithSaveFlag(bodyStr, inMsParamArray, rei, 0);
    if ( i != 0)
      break;
    /*  RECOVERY OF OTHER EXECUTED BODY IS NOT DONE. HOW CAN WE DO THAT */
    /***    freeNextValueStruct(&value,outtyp,"internal"); ***/
    inPtr = restPtr;    
  }
  /***  freeNextValueStruct(&value,outtyp,"all");***/
  if (msParamStruct != NULL) {
    /* value in rei->params get replaced back... */
    msParam->inOutStruct = msParamStruct;
    msParam->inpOutBuf = msParamBuf;
    msParam->type = typ;
  }
  /*** RAJA removed commenting out Nov 3, 2009 to stop a memory leak  ***/
       // clearMsParamArray (list,1);  /* Re-commented out on 11/20/09 to prevent segfault until it is fixed. */
       free(list);
  /***/
  if (i == BREAK_ACTION_ENCOUNTERED_ERR) 
      return(0);
  if (i == NO_VALUES_FOUND)
    return(0);
  return(i);
}

/**
 * \fn delayExec(msParam_t *mPA, msParam_t *mPB, msParam_t *mPC, ruleExecInfo_t *rei)
 *
 * \brief  Execute a set of operations later when certain conditions are met. Can be used to perform 
 * periodic operations also.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note  This microservice is a set of statements that will be  delayed in execution until delayCondition
 *    is true. The condition also supports repeating of the body until success or until
 *    some other condition is satisfied. This microservice takes the delayCondition as
 *    the first parameter, the Micro-service/rule chain that needs to be executed as
 *    the second parameter, and the recovery-Micro-service chain as the third parameter.
 *    The delayCondition is given as a tagged condition. In this case, there are two
 *    conditions that are specified.

 *
 * \usage
 *
 * As seen in server/config/reConfigs/rajatest.irb
 *
 * acPostProcForPut||delayExec(msiReplDataObj(demoResc8),nop)|nop
 *
 * \param[in] mPA - mPA is a msParam of type STR_MS_T which is a delayCondition about when to execute the body.
 *  		   These are tagged with the following tags:
 * 			\li EA - execAddress - host where the delayed execution needs to be performed 
 *			\li ET - execTime - absolute time when it needs to be performed. 
 * 			\li PLUSET - relExeTime - relative to current time when it needs to execute 
 * 			\li EF - execFreq - frequency (in time widths) it needs to be performed. 
 * 			The format for EF is quite rich: 
 * 			The EF value is of the format: 
 *			nnnnU <directive>  where 
 *			nnnn is a number, and 
 *     			U is the unit of the number (s-sec,m-min,h-hour,d-day,y-year), 
 * 			The <directive> can be for the form: 
 *     			<empty-directive>    - equal to REPEAT FOR EVER 
 *     			REPEAT FOR EVER 
 *     			REPEAT UNTIL SUCCESS 
 *     			REPEAT nnnn TIMES    - where nnnn is an integer 
 *     			REPEAT UNTIL <time>  - where <time> is of the time format supported by checkDateFormat function below. 
 *     			REPEAT UNTIL SUCCESS OR UNTIL <time> 
 *     			REPEAT UNTIL SUCCESS OR nnnn TIMES 
 *     			DOUBLE FOR EVER 
 *     			DOUBLE UNTIL SUCCESS   - delay is doubled every time. 
 *     			DOUBLE nnnn TIMES 
 *     			DOUBLE UNTIL <time> 
 *     			DOUBLE UNTIL SUCCESS OR UNTIL <time> 
 *     			DOUBLE UNTIL SUCCESS OR nnnn TIMES 
 *     			DOUBLE UNTIL SUCCESS UPTO <time>  
 *			Example: <PLUSET>1m</PLUSET><EF>10m<//EF>  means start after 1 minute and repeat every 20 minutes
 * \param[in] mPB - mPB is a msParam of type STR_MS_T which is a body Micro-service/Rule list separated by ##
 * \param[in] mPC - mPC is a msParam of type STR_MS_T which is a recoveryBody - Micro-service/Rule list  separted by ##                 
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int delayExec(msParam_t *mPA, msParam_t *mPB, msParam_t *mPC, ruleExecInfo_t *rei)
{
  int i;
  char actionCall[MAX_ACTION_SIZE];  
  char recoveryActionCall[MAX_ACTION_SIZE];  
  char delayCondition[MAX_ACTION_SIZE]; 

  rstrcpy(delayCondition, (char *) mPA->inOutStruct,MAX_ACTION_SIZE);
  rstrcpy(actionCall, (char *) mPB->inOutStruct,MAX_ACTION_SIZE);
  rstrcpy(recoveryActionCall, (char *) mPC->inOutStruct,MAX_ACTION_SIZE);
  i = _delayExec(actionCall, recoveryActionCall, delayCondition, rei);
  return(i);
}



int _delayExec(char *inActionCall, char *recoveryActionCall, 
	       char *delayCondition,  ruleExecInfo_t *rei)
{

  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int i, argc;
  ruleExecSubmitInp_t *ruleSubmitInfo;
  char action[MAX_ACTION_SIZE];  
  char tmpStr[NAME_LEN];  
  bytesBuf_t *packedReiAndArgBBuf = NULL;
  char *ruleExecId;
  char *actionCall;


  RE_TEST_MACRO ("    Calling _delayExec");

  actionCall = inActionCall;
  /* Get Arguments */
  if (strstr(actionCall,"##") == NULL && !strcmp(recoveryActionCall,"nop")) {
    /* single call */
    i = parseAction(actionCall,action,args, &argc);
    if (i != 0)
      return(i);
    mapExternalFuncToInternalProc(action);
    argc = 0;
  }
  else {
    actionCall = (char*)malloc(strlen((char*)inActionCall) + strlen((char*)recoveryActionCall) + 3);
    sprintf(actionCall,"%s|%s",inActionCall,recoveryActionCall);
    args[0] = NULL;
    args[1] = NULL;
    argc = 0;
  }
  /* Pack Rei and Args */
  i = packReiAndArg (rei->rsComm, rei, args, argc, &packedReiAndArgBBuf);
  if (i < 0) {
    if (actionCall != inActionCall) 
      free (actionCall);
    return(i);
  }
  /* fill Conditions into Submit Struct */
  ruleSubmitInfo = (ruleExecSubmitInp_t*)mallocAndZero(sizeof(ruleExecSubmitInp_t));
  i  = fillSubmitConditions (actionCall, delayCondition, packedReiAndArgBBuf, ruleSubmitInfo, rei);
  if (actionCall != inActionCall) 
    free (actionCall);
  if (i < 0) {
    free(ruleSubmitInfo);
    return(i);
  }
  
  /* Store ReiArgs Struct in a File */
  i = rsRuleExecSubmit(rei->rsComm, ruleSubmitInfo, &ruleExecId);
  if (packedReiAndArgBBuf != NULL) {
    clearBBuf (packedReiAndArgBBuf);
    free (packedReiAndArgBBuf);
  }
  
  free(ruleSubmitInfo);
  if (i < 0) 
    return(i);
  free (ruleExecId);
  snprintf(tmpStr,NAME_LEN, "%d",i);
  i = pushStack(&delayStack,tmpStr);
  return(i);
}

int recover_delayExec(msParam_t *actionCall, msParam_t *delayCondition,  ruleExecInfo_t *rei)
{

  int i;
  ruleExecDelInp_t ruleExecDelInp;

  RE_TEST_MACRO ("    Calling recover_delayExec");

  i  = popStack(&delayStack,ruleExecDelInp.ruleExecId);
  if (i < 0)
    return(i);

  i = rsRuleExecDel(rei->rsComm, &ruleExecDelInp);
  return(i);

}


/**
 * \fn remoteExec(msParam_t *mPD, msParam_t *mPA, msParam_t *mPB, msParam_t *mPC, ruleExecInfo_t *rei)
 *
 * \brief  A set of statements to be remotely executed
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note This mico-service takes a  set of micro-services that need to be executed  at a remote iRODS server. The execution is done immediately and synchronously with  the  result returning back from the call.
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest21.ir
 *
 * myTestRule||writeLine(stdout,begin)##remoteExec(andal.sdsc.edu,null,msiSleep(10,0)##writeLine(stdout,open remote write in andal)
 * ##remoteExec(srbbrick14.sdsc.edu,null,msiSleep(10,0)##writeLine(stdout,remote of a remote write in srbbrick1),nop)
 * ##remoteExec(srbbrick14.sdsc.edu,null,remoteExec(andal.sdsc.edu,null,msiSleep(10,0)
 * ##writeLine(stdout,remote of a remote of a remote write in andal),nop),nop)##msiSleep(10,0)
 * ##writeLine(stdout,close * remote write in andal),nop)##writeLine(stdout,end)|nop
 * null
 * ruleExecOut
 *
 * \param[in] mPD - a msParam of type STR_MS_T which is a host name of the server where the body need to be executed.
 * \param[in] mPA - a msParam of type STR_MS_T which is a delayCondition about when to execute the body.
 * \param[in] mPB - a msParam of type STR_MS_T which is a body. A statement list - micro-services and ruleNames separated by ##
 * \param[in] mPC - a msParam of type STR_MS_T which is arecoverBody. A statement list - micro-services and ruleNames separated by ##
 * \param[in,out] rei Required - the RuleExecInfo structure.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int remoteExec(msParam_t *mPD, msParam_t *mPA, msParam_t *mPB, msParam_t *mPC, ruleExecInfo_t *rei)
{
  int i;
  execMyRuleInp_t execMyRuleInp;
  msParamArray_t *tmpParamArray, *aParamArray;
  msParamArray_t *outParamArray = NULL;
  char tmpStr[LONG_NAME_LEN];
  char tmpStr1[LONG_NAME_LEN];
  /*
  char actionCall[MAX_ACTION_SIZE];  
  char recoveryActionCall[MAX_ACTION_SIZE];  
  char delayCondition[MAX_ACTION_SIZE]; 
  char hostName[MAX_ACTION_SIZE]; 
  rstrcpy(hostName, (char *) mPD->inOutStruct,MAX_ACTION_SIZE);
  rstrcpy(delayCondition, (char *) mPA->inOutStruct,MAX_ACTION_SIZE);
  rstrcpy(actionCall, (char *) mPB->inOutStruct,MAX_ACTION_SIZE);
  rstrcpy(recoveryActionCall, (char *) mPC->inOutStruct,MAX_ACTION_SIZE);
  i = _remoteExec(actionCall, recoveryActionCall, delayCondition, hostName, rei);
  */
  memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
  execMyRuleInp.condInput.len=0;
  rstrcpy (execMyRuleInp.outParamDesc, ALL_MS_PARAM_KW, LONG_NAME_LEN);
  /*  rstrcpy (execMyRuleInp.addr.hostAddr, mPD->inOutStruct, LONG_NAME_LEN);*/
  rstrcpy (tmpStr, (char *) mPD->inOutStruct, LONG_NAME_LEN);
#if 0
  i = evaluateExpression(tmpStr, execMyRuleInp.addr.hostAddr, rei);
  if (i < 0)
    return(i);
#else
  i = evaluateExpression(tmpStr, tmpStr1, rei);
  if (i < 0)
    return(i);
  parseHostAddrStr (tmpStr1, &execMyRuleInp.addr);
#endif
  snprintf(execMyRuleInp.myRule, META_STR_LEN, "remExec||%s|%s",  (char*)mPB->inOutStruct,(char*)mPC->inOutStruct);
  addKeyVal(&execMyRuleInp.condInput,"execCondition",(char*)mPA->inOutStruct);
  
  tmpParamArray =  (msParamArray_t *) malloc (sizeof (msParamArray_t));
  memset (tmpParamArray, 0, sizeof (msParamArray_t));
  i = replMsParamArray (rei->msParamArray,tmpParamArray);
  if (i < 0) {
    free(tmpParamArray);
    return(i);
  }
  aParamArray = rei->msParamArray;
  rei->msParamArray = tmpParamArray;
  execMyRuleInp.inpParamArray = rei->msParamArray;
  i = rsExecMyRule (rei->rsComm, &execMyRuleInp,  &outParamArray);
  carryOverMsParam(outParamArray, aParamArray);
  rei->msParamArray = aParamArray;
  clearMsParamArray(tmpParamArray,0);
  free(tmpParamArray);
  return(i);
}


/*****
int _remoteExec(char *inActionCall, char *recoveryActionCall, 
	       char *delayCondition,  char *hostName, ruleExecInfo_t *rei)
{

  char *args[MAX_NUM_OF_ARGS_IN_ACTION];
  int i, argc;
  ruleExecSubmitInp_t *ruleSubmitInfo;
  char action[MAX_ACTION_SIZE];  
  char tmpStr[NAME_LEN];  
  bytesBuf_t *packedReiAndArgBBuf = NULL;
  char *ruleExecId;
  char *actionCall;


  RE_TEST_MACRO ("    Calling _delayExec");

  actionCall = inActionCall;
  if (strstr(actionCall,"##") == NULL && !strcmp(recoveryActionCall,"nop")) {
    i = parseAction(actionCall,action,args, &argc);
    if (i != 0)
      return(i);
    mapExternalFuncToInternalProc(action);
    argc = 0;
  }
  else {
    actionCall = malloc(strlen(inActionCall) + strlen(recoveryActionCall) + 3);
    sprintf(actionCall,"%s|%s",inActionCall,recoveryActionCall);
    args[0] = NULL;
    args[1] = NULL;
    argc = 0;
  }

  i = packReiAndArg (rei->rsComm, rei, args, argc, &packedReiAndArgBBuf);
  if (i < 0) {
    if (actionCall != inActionCall) 
      free (actionCall);
    return(i);
  }

  ruleSubmitInfo = mallocAndZero(sizeof(ruleExecSubmitInp_t));
  i  = fillSubmitConditions (actionCall, delayCondition, packedReiAndArgBBuf, ruleSubmitInfo, rei);
  strncpy(ruleSubmitInfo->exeAddress,hostName,NAME_LEN);
  if (actionCall != inActionCall) 
    free (actionCall);
  if (i < 0) {
    free(ruleSubmitInfo);
    return(i);
  }
  
  i = rsRemoteRuleExecSubmit(rei->rsComm, ruleSubmitInfo, &ruleExecId);
  if (packedReiAndArgBBuf != NULL) {
    clearBBuf (packedReiAndArgBBuf);
    free (packedReiAndArgBBuf);
  }
  
  free(ruleSubmitInfo);
  if (i < 0) 
    return(i);
  free (ruleExecId);
  snprintf(tmpStr,NAME_LEN, "%d",i);
  i = pushStack(&delayStack,tmpStr);
  return(i);
}
******/
int recover_remoteExec(msParam_t *actionCall, msParam_t *delayCondition, char *hostName, ruleExecInfo_t *rei)
{

  int i;
  ruleExecDelInp_t ruleExecDelInp;

  RE_TEST_MACRO ("    Calling recover_delayExec");

  i  = popStack(&delayStack,ruleExecDelInp.ruleExecId);
  if (i < 0)
    return(i);
  /***
  i = rsRemoteRuleExecDel(rei->rsComm, &ruleExecDelInp);
  ***/
  return(i);

}


int
doForkExec(char *prog, char *arg1)
{
   int pid, i;

   i = checkFilePerms(prog);
   if (i) return(i);

   i = checkFilePerms(arg1);
   if (i) return(i);

#ifndef windows_platform
   pid = fork();
   if (pid == -1) return -1;

   if (pid) {
      /*  This is still the parent.  */
   }
   else {
      /* child */
      for (i=0;i<100;i++) {
	 close(i);
      }
      i = execl(prog, prog, arg1, (char *) 0);
      printf("execl failed %d\n",i);
      return(0);
   }
#else
   /* windows platform */
   if(_spawnl(_P_NOWAIT, prog, prog, arg1) == -1)
   {
	   return -1;
   }
#endif

   return(0);
}


/**
 * \fn msiGoodFailure(ruleExecInfo_t *rei)
 *
 * \brief  This microservice performs no operations but fails the current rule application
 *    immediately even if the body still has some more microservices. But other definitions
 *    of the rule are not retried upon this failure. It is useful when you want to fail
 *    but no recovery initiated.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2006-06
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note useful when you want to fail a rule without retries.  
 *
 * \usage None. 
 *
 * \param[in,out] rei Required - the RuleExecInfo structure.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval negative number
 * \pre none
 * \post none
 * \sa fail
 * \bug  no known bugs
**/
int
msiGoodFailure(ruleExecInfo_t *rei)
{
  
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   Calling msiGoodFailure So that It will Retry Other Rules Without Recovery\n");
    return(RETRY_WITHOUT_RECOVERY_ERR);
  }
  /**** This is Just a Test Stub  ****/
  return(RETRY_WITHOUT_RECOVERY_ERR);
}



/* check that a file exists and is not writable by group or other */ 
int
checkFilePerms(char *fileName) {
   struct stat buf;
   if (stat (fileName, &buf) == -1) {
      printf ("Stat failed for file %s\n", 
              fileName);
      return(-1);
   }

   if (buf.st_mode & 022) {
      printf ("File is writable by group or other: %s.\n",
              fileName);
      return(-2);
   }
   return(0);
}

/**
 * \fn msiFreeBuffer(msParam_t* memoryParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice frees a named buffer, including stdout and stderr
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajaseka
 * \date  2006
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note Can be used to free a buffer which was allocated previously.
 *
 * \usage
 *
 * As seen in server/config/reConfigs/core.fnm
 *
 * freeBuf|msiFreeBuffer
 *
 * \param[in] memoryParam - the buffer to free
 * \param[in,out] rei Required - the RuleExecInfo structure.
 *
 * \DolVarDependence  none
 * \DolVarModified  none
 * \iCatAttrDependence  none
 * \iCatAttrModified  none
 * \sideeffect  none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int 
msiFreeBuffer(msParam_t* memoryParam, ruleExecInfo_t *rei)
{

  msParamArray_t *inMsParamArray;
  msParam_t *mP;
  execCmdOut_t *myExecCmdOut;

  RE_TEST_MACRO ("Loopback on msiFreeBuffer");

  if (!strcmp(memoryParam->type,"STR_PI") && 
      ( !strcmp((char*)memoryParam->inOutStruct,"stdout") || 
	!strcmp((char*)memoryParam->inOutStruct,"stderr")
      )
     ) 
    {
      mP = NULL;
      inMsParamArray = rei->msParamArray;
      if (((mP = getMsParamByLabel (inMsParamArray, "ruleExecOut")) != NULL) &&
	  (mP->inOutStruct != NULL)) {
	myExecCmdOut = (execCmdOut_t *) mP->inOutStruct;
	if (!strcmp((char*)memoryParam->inOutStruct,"stdout")) {
	  if ( myExecCmdOut->stdoutBuf.buf != NULL) {
	    free ( myExecCmdOut->stdoutBuf.buf);
	    myExecCmdOut->stdoutBuf.buf=NULL;
	    myExecCmdOut->stdoutBuf.len = 0;
	  }
	}
	if (!strcmp((char*)memoryParam->inOutStruct,"stderr")) {
	  if ( myExecCmdOut->stderrBuf.buf != NULL) {
	    free ( myExecCmdOut->stderrBuf.buf);
	    myExecCmdOut->stderrBuf.buf = NULL;
	    myExecCmdOut->stderrBuf.len = 0;
	  }
	}
      }
      return(0);
    }

  if (memoryParam->inpOutBuf != NULL)
    free(memoryParam->inpOutBuf);
  memoryParam->inpOutBuf = NULL;
  return(0);
  
}

/**
 * \fn msiSleep(msParam_t* secPtr, msParam_t* microsecPtr,  ruleExecInfo_t *rei)
 *
 * \brief  Sleep for some amount of time
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date 2008-05
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note similar t sleep in C
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest23.ir
 *
 * myTestRule||acGetIcatResults(*Action,*Condition,*B)##forEachExec(*B,msiGetValByKey(*B,RESC_LOC,*R)##remoteExec(*R,null,msiSleep(20,0)
 * ##msiDataObjChksum(*B,*Operation,*C),nop)##msiGetValByKey(*B,DATA_NAME,*D)##msiGetValByKey(*B,COLL_NAME,*E)
 * ##writeLine(stdout,CheckSum of *E\*D at *R is *C),nop)|nop##nop
 * *Action=chksumRescLoc%*Condition=COLL_NAME = '/tempZone/home/rods/loopTest'%*Operation=ChksumAll
 * *Action%*Condition%*Operation%*C%ruleExecOut
 * (Note that the \ should be / but was changed to avoid a compiler warning
 * about a slash * in a comment.)
 * 
 * \param[in] secPtr - secPtr is a msParam of type STR_MS_T which is seconds
 * \param[in] microsecPtr - microsecPrt is a msParam of type STR_MS_T which is microseconds
 * \param[in,out] rei Required - the RuleExecInfo structure.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre  none
 * \post none
 * \sa  none
 * \bug  no known bugs
**/
int
msiSleep(msParam_t* secPtr, msParam_t* microsecPtr,  ruleExecInfo_t *rei)
{

  int sec, microsec;

  sec = atoi((char*)secPtr->inOutStruct);
  microsec = atoi((char*)microsecPtr->inOutStruct);

  rodsSleep (sec, microsec);
  return(0);
}

/**
 * \fn msiApplyAllRules(msParam_t *actionParam, msParam_t* reiSaveFlagParam, msParam_t* allRuleExecFlagParam, ruleExecInfo_t *rei)
 *
 * \brief  This micro-service executes all applicable rules for a given action name.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \remark Ketan Palshikar - msi documentation, 2009-06-26
 * \remark Terrell Russell - reviewed msi documentation, 2009-06-30
 * 
 * \note Normal operations of the rule engine is to stop after a rule (one of the alternates) completes successfully. But in some cases, one may want the rule engine to try all alternatives and succeed in as many as possible. Then by firing that rule under this micro-service all alternatives are tried. 
 *
 * \usage
 *
 * As seen in clients/icommands/test/ruleTest25.ir
 *
 * myTestRule||msiAdmAddAppRuleStruct(*B,,)##writeLine(stdout,a test for all rule application - begin)##applyAllRules(aa,0,*DepthAllRuleBinaryFlag)
 * ##writeLine(stdout,a test for all rule application - end)|nop
 * *B=core4%*DepthAllRuleBinaryFlag=$1
 * ruleExecOut
 *
 * \param[in] actionParam - a msParam of type STR_MS_T which is the name of an action to be executed.
 * \param[in] reiSaveFlagParam - a msParam of type STR_MS_T which is 0 or 1 value used to
 *    check if the rei structure needs to be saved at every rule invocation inside the
 *    execution. This helps to save time if the rei structure is known not to be
 *    changed when executing the underlying rules.
 * \param[in] allRuleExecFlagParam - allRuleExecFlagParam is a msParam of type STR_MS_T which
 *    is 0 or 1 whether the "apply all rule" condition applies only to the actionParam
 *    invocation or is recursively done at all levels of invocation of every rule inside the execution.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiApplyAllRules(msParam_t *actionParam, msParam_t* reiSaveFlagParam, 
		 msParam_t* allRuleExecFlagParam, ruleExecInfo_t *rei)
{
  int i;
  char *action;
  int reiSaveFlag;
  int allRuleExecFlag;

  action = (char*)actionParam->inOutStruct;
  reiSaveFlag = atoi((char*)reiSaveFlagParam->inOutStruct);
  allRuleExecFlag = atoi((char*)allRuleExecFlagParam->inOutStruct);
  i = applyAllRules(action, rei->msParamArray, rei, reiSaveFlag,allRuleExecFlag);
  return(i);

}


/**
 * \fn msiGetDiffTime(msParam_t* inpParam1, msParam_t* inpParam2, msParam_t* inpParam3, msParam_t* outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice returns the difference between two system times
 *
 * \module framework
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2007-09-19
 *
 * \remark Terrell Russell - msi documentation, 2009-06-23
 *
 * \note If we have arithmetic MSs in the future we should use DOUBLE_MS_T instead of strings.
 *       Default output format is in seconds, use 'human' as the third input param for human readable format.
 *
 * \usage
 * As seen in modules/ERA/test/getDiffTime.ir
 * 
 * testrule||msiGetSystemTime(*Start_time, null)##msiSleep(3, 0)##msiGetSystemTime(*End_time, null)##msiGetDiffTime(*Start_time, *End_time, human, *Duration)##writeLine(stdout, *Duration)|nop
 * null
 * ruleExecOut
 *
 * \param[in] inpParam1 - a STR_MS_T containing the start date (system time in seconds)
 * \param[in] inpParam2 - a STR_MS_T containing the end date (system time in seconds)
 * \param[in] inpParam3 - Optional - a STR_MS_T containing the desired output format
 * \param[out] outParam - a STR_MS_T containing the time elapsed between the two dates
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiGetDiffTime(msParam_t* inpParam1, msParam_t* inpParam2, msParam_t* inpParam3, msParam_t* outParam, ruleExecInfo_t *rei)
{
	long hours, minutes, seconds;
	char timeStr[TIME_LEN];
	char *format;
	int status;

	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiGetDiffTime")


	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetDiffTime: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	
	/* Check for proper input */
	if ((parseMspForStr(inpParam1) == NULL)) {
		rodsLog (LOG_ERROR, "msiGetDiffTime: input inpParam1 is NULL");
		return (USER__NULL_INPUT_ERR);
	}

	if ((parseMspForStr (inpParam2) == NULL)) {
		rodsLog (LOG_ERROR, "msiGetDiffTime: input inpParam2 is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* get time values from strings */
	seconds = atol((char*)inpParam2->inOutStruct) - atol((char*)inpParam1->inOutStruct);
	
	/* get desired output format */
	format = (char*)inpParam3->inOutStruct;

	
	/* did they ask for human readable format? */
	if (format && !strcmp(format, "human")) {
		/* get hours-minutes-seconds */
		hours = seconds / 3600;
		seconds = seconds % 3600;
		minutes = seconds / 60;
		seconds = seconds % 60;
		
		snprintf(timeStr, TIME_LEN, "%ldh %ldm %lds", hours, minutes, seconds);
	}
	else {
		snprintf(timeStr, TIME_LEN, "%011ld", seconds);
	}
	
	
	/* result goes to outParam */
	status = fillStrInMsParam (outParam, timeStr);	
	
	return(status);
}


/**
 * \fn msiGetSystemTime(msParam_t* outParam, msParam_t* inpParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice returns the local system time
 *
 * \module framework
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2007-09-19
 *
 * \remark Terrell Russell - msi documentation, 2009-06-23
 *
 * \note Default output format is system time in seconds, use 'human' as input param for human readable format.
 *
 * \usage
 * As seen in modules/ERA/test/getSystemTime.ir
 * 
 * testrule||msiGetSystemTime(*Time, human)##writeLine(stdout, *Time)|nop
 * null
 * ruleExecOut
 *
 * \param[out] outParam - a STR_MS_T containing the time
 * \param[in] inpParam - Optional - a STR_MS_T containing the desired output format
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiGetSystemTime(msParam_t* outParam, msParam_t* inpParam, ruleExecInfo_t *rei)
{
	char *format;
	char tStr0[TIME_LEN],tStr[TIME_LEN];
	int status;
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiGetSystemTime")


	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetSystemTime: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	

	format = (char*)inpParam->inOutStruct;
	
	if (!format || strcmp(format, "human")) {
		getNowStr(tStr);
	}
	else {
		getNowStr(tStr0);
		getLocalTimeFromRodsTime(tStr0,tStr);
	}
	
	status = fillStrInMsParam (outParam,tStr);

	return(status);
}


/**
 * \fn msiHumanToSystemTime(msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei)
 *
 * \brief Converts a human readable date to a system timestamp
 *
 * \module framework
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-03-11
 *
 * \remark Terrell Russell - msi documentation, 2009-06-23
 *
 * \note Expects an input date in the form: YYYY-MM-DD-hh.mm.ss
 *
 * \usage
 * As seen in modules/ERA/test/convertDateToTimestamp.ir
 * 
 * testrule||msiHumanToSystemTime(*date, *timestamp)##writeString(stdout,*timestamp)##writeLine(stdout,"")|nop
 * *date=2008-03-14-15.13.01
 * ruleExecOut
 *
 * \param[in] inpParam - a STR_MS_T containing the input date
 * \param[out] outParam - a STR_MS_T containing the timestamp
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiHumanToSystemTime(msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei)
{
	char sys_time[TIME_LEN], *hr_time;
	int status;
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiHumanToSystemTime")


	/* microservice check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiHumanToSystemTime: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}


	/* parse inpParam (date input string) */
	if ((hr_time = parseMspForStr (inpParam)) == NULL) {
		rodsLog (LOG_ERROR, "msiHumanToSystemTime: parseMspForStr error for input param.");
		return (rei->status);
	}


	/* left padding with a zero for DB format. Might change that later */
	memset(sys_time, '\0', TIME_LEN);
	sys_time[0] = '0';


	/* conversion */
	if ((status = localToUnixTime (hr_time, &sys_time[1]))  == 0) {
		status = fillStrInMsParam (outParam, sys_time);
	}


	return(status);
}



/**
 * \fn msiStrToBytesBuf(msParam_t* str_msp, msParam_t* buf_msp, ruleExecInfo_t *rei)
 *
 * \brief Converts a string to a bytesBuf_t
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-09-16
 *
 * \remark Terrell Russell - msi documentation, 2009-06-15
 *
 * \note Following discussion on iRODS_Chat. For easily passing parameters to
 *    microservices that require a BUF_LEN_MS_T
 *
 * \usage
 * 
 * testrule||msiStrToBytesBuf(*A, *Buff)##writeLine(stdout,"")|nop
 * *A=Astring
 * ruleExecOut
 *
 * \param[in] str_msp - a STR_MS_T
 * \param[out] buf_msp - a BUF_LEN_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiStrToBytesBuf(msParam_t* str_msp, msParam_t* buf_msp, ruleExecInfo_t *rei)
{
	char *inStr;
	bytesBuf_t *outBuf;

	/* parse str_msp */
	if ((inStr = parseMspForStr(str_msp)) == NULL)  
	{
		rodsLog (LOG_ERROR, "msiStrToBytesBuf: input str_msp is NULL.");
		return (USER__NULL_INPUT_ERR);
	}

	/* buffer init */
	outBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (outBuf, 0, sizeof (bytesBuf_t));

	/* fill string in buffer */
	outBuf->len = strlen(inStr);
	outBuf->buf = inStr;

	/* fill buffer in buf_msp */
	fillBufLenInMsParam (buf_msp, outBuf->len, outBuf);
	
	return 0;
}



/**
 * \fn msiListEnabledMS(msParam_t *outKVPairs, ruleExecInfo_t *rei)
 *
 * \brief Returns the list of compiled microservices on the local iRODS server
 *
 * \module framework
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2009-02-12
 *
 * \remark Terrell Russell - msi documentation, 2009-06-23
 *
 * \note This microservice looks at reAction.h and returns the list of compiled 
 *  microservices on the local iRODS server.
 *      The results are written to a KeyValPair_MS_T. For each pair the keyword is the MS name 
 *  while the value is the module where the microservice belongs. 
 *  Standard non-module microservices are listed as "core".
 *
 * \usage
 * As seen in clients/icommands/test/listMS.ir
 * 
 * testrule||msiListEnabledMS(*KVPairs)##writeKeyValPairs(stdout, *KVPairs, ": ")|nop
 * null
 * ruleExecOut
 *
 * \param[out] outKVPairs - A KeyValPair_MS_T containing the results.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/
int
msiListEnabledMS(msParam_t *outKVPairs, ruleExecInfo_t *rei)
{
        FILE *radh;						/* reAction(dot)h */

        keyValPair_t *results;			/* the output data structure */

        char lineStr[LONG_NAME_LEN];	/* for line and string parsing */
        char modName[NAME_LEN];
        char *begPtr, *endPtr;


        /* For testing mode when used with irule --test */
        RE_TEST_MACRO ("    Calling msiEnabledMS")


        /* Sanity test */
        if (rei == NULL || rei->rsComm == NULL) {
                rodsLog (LOG_ERROR, "msiListEnabledMS: input rei or rsComm is NULL.");
                return (SYS_INTERNAL_NULL_INPUT_ERR);
        }


        /* Open reAction.h for reading */
        radh = fopen ("../re/include/reAction.h", "r");
        if(!radh) {
                rodsLog (LOG_ERROR, "msiListEnabledMS: unable to open reAction.h for reading.");
                return (UNIX_FILE_READ_ERR);
        }


        /* Skip the first part of the file */
        while (fgets(lineStr, LONG_NAME_LEN, radh) != NULL)
        {
                if (strstr(lineStr, "microsdef_t MicrosTable[]") == lineStr)
                {
                        break;
                }
        }


        /* Default microservices come first, will be listed are "core" */
        strncpy(modName, "core", NAME_LEN); /* Pad with null chars in the process */


        /* Allocate memory for our result struct */
        results = (keyValPair_t*)malloc(sizeof(keyValPair_t));
        memset (results, 0, sizeof (keyValPair_t));


        /* Scan microservice table one line at a time*/
        while (fgets(lineStr, LONG_NAME_LEN, radh) != NULL)
        {
                /* End of the table? */
                if (strstr(lineStr, "};") == lineStr)
                {
                        break;
                }

                /* Get microservice name */
                if ( (begPtr = strchr(lineStr, '\"')) && (endPtr = strrchr(lineStr, '\"')) )
                {
                        endPtr[0] = '\0';
                        addKeyVal(results, &begPtr[1], modName);
                }
                else 
                {
                        /* New Module? */
                        if (strstr(lineStr, "module microservices") )
                        {
                                /* Get name of module (between the first two spaces) */
                                begPtr = strchr(lineStr, ' ');
                                endPtr = strchr(++begPtr, ' ');
                                endPtr[0] = '\0';

                                strncpy(modName, begPtr, NAME_LEN-1);
                        }
                }
        }


        /* Done */
        fclose(radh);


        /* Send results out to outKVPairs */
        fillMsParam (outKVPairs, NULL, KeyValPair_MS_T, results, NULL);


        return 0;
}




