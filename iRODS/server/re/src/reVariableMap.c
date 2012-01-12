/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"
#include "reVariables.h"
#include "rcMisc.h"

#define RE_INT 1
#define RE_STR 2
#define RE_STRDUP 3
#define RE_LONG 4
#define RE_PTR 5

#ifdef ADDR_64BITS
#define CAST_INT_VOIDPTR (long int)
#else
#define CAST_INT_VOIDPTR  
#endif

int
getSetLeafValue(char **varValue, void *leafPtr, void **leafPtrVal, char *newVarValue, int setType)
{
  int *i; /*, *j;*/
  char *s;
  long int *l; /*, , *k;*/
  /*
  if (newVarValue != NULL)
    rodsLog (LOG_NOTICE,"Enter: leafPtr=%d,leafPtrVal=%d,newVarValue=%s\n",leafPtr,leafPtrVal,newVarValue);
  else
    rodsLog (LOG_NOTICE,"Enter: leafPtr=%d,leafPtrVal=%d,newVarValue=nil\n",leafPtr,leafPtrVal);
  */
  if (setType == RE_INT) {
    i = (int*)leafPtr;
    *varValue = (char*)malloc(sizeof(int) * 2);
    sprintf(*varValue, "%d",*i);
    if (newVarValue != NULL) {
      *i = atoi(newVarValue);

    }
  }
  else if (setType == RE_STR) {
    s= (char*)leafPtr;
    *varValue = strdup(s);
    if (newVarValue != NULL) 
      strcpy(s,(char *) newVarValue);
  }
  else if (setType == RE_STRDUP) {
    s= (char*)leafPtr;
    *varValue = strdup(s);
    if (newVarValue != NULL) {
      if (s != NULL)  free(s);
      s = strdup((char *) newVarValue);
    }
  }
  else if (setType == RE_LONG) {
    l = (long*)leafPtr;
    /*    *varValue = malloc(sizeof(long) * 2);*/
    *varValue = (char*)malloc(34);
    /*    sprintf(*varValue, "%lld",*l);*/
    sprintf(*varValue, "%ld",*l);
    if (newVarValue != NULL) {
      *l = atol(newVarValue);
    }
  }
  else if (setType == RE_PTR) {
    *varValue = (char*)malloc(34);
#ifdef ADDR_64BITS      
    sprintf(*varValue, "%lld", (long long int) (void *) *leafPtrVal);
#else
    /*    sprintf(*varValue, "%p",(void *) *leafPtrVal); */
    sprintf(*varValue, "%ld",(long) *leafPtrVal);
#endif
    if (newVarValue != NULL) {
#ifdef ADDR_64BITS      
      *leafPtrVal = (void *) strtoll(newVarValue, (char **)NULL, 0);
#else
      *leafPtrVal = (void *) strtol(newVarValue, (char **)NULL, 0);
#endif
      /*      leafPtr = newVarValue; */
    }
  }
  else 
    return(INVALID_OBJECT_TYPE);
  /*
  if (newVarValue != NULL)
    rodsLog (LOG_NOTICE,"Exit : leafPtr=%d,leafPtrVal=%d,newVarValue=%s\n",leafPtr,leafPtrVal,newVarValue);
  else
    rodsLog (LOG_NOTICE,"Exit : leafPtr=%d,leafPtrVal=%d,newVarValue=nil\n",leafPtr,leafPtrVal);
  */
  return(0);

}
int
mapExternalFuncToInternalProc( char *funcName)
{
  int i;
  
  for (i = 0; i < appRuleFuncMapDef.MaxNumOfFMaps; i++) {
    if (strstr(appRuleFuncMapDef.funcName[i],funcName ) != NULL) {
	strcpy(funcName, appRuleFuncMapDef.func2CMap[i]);
	return(1);
    }
  }
  for (i = 0; i < coreRuleFuncMapDef.MaxNumOfFMaps; i++) {
    if (strstr(coreRuleFuncMapDef.funcName[i],funcName ) != NULL) {
	strcpy(funcName, coreRuleFuncMapDef.func2CMap[i]);
	return(1);
    }
  }
  return(0);
}

int 
getVarMap(char *action, char *inVarName, char **varMap, int index)
{
  int i;
  char *varName;

  if (inVarName[0] == '$')
    varName = inVarName + 1;
  else
    varName = inVarName;
  if (index < 1000) {
    for (i = index; i < appRuleVarDef.MaxNumOfDVars; i++) {
      if (!strcmp(appRuleVarDef.varName[i],varName )) {
	if (strlen(appRuleVarDef.action[i]) == 0 ||
	    strstr(appRuleVarDef.action[i],action) != NULL ) {
	  *varMap = strdup(appRuleVarDef.var2CMap[i]);
	  return(i);
	}
      }
    }
    index = 1000;
  }
  i = index - 1000;
  for ( ; i < coreRuleVarDef.MaxNumOfDVars; i++) {
    if (!strcmp(coreRuleVarDef.varName[i],varName )) {
      if (strlen(coreRuleVarDef.action[i]) == 0 ||
	  strstr(coreRuleVarDef.action[i],action) != NULL ) {
	*varMap = strdup(coreRuleVarDef.var2CMap[i]);
	return(i + 1000);
      }
    }
  }
  return(UNKNOWN_VARIABLE_MAP_ERR);
}



int
getVarNameFromVarMap(char *varMap, char *varName, char **varMapCPtr)
{

  char *p;

  if ((p = strstr(varMap,"->")) == NULL) {
    rstrcpy(varName,varMap,NAME_LEN);
    *varMapCPtr=NULL;
  }
  else {
    *p = '\0';
    rstrcpy(varName,varMap,NAME_LEN);
    *p = '-';
    p++;
    p++;
    *varMapCPtr = p;
  }
  trimWS(varName);
  return(0);

}

int 
getVarValue(char *varMap, ruleExecInfo_t *rei, char **varValue)
{

  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  
  if (!strcmp(varName,"rei")) {
    i = getSetValFromRuleExecInfo(varMapCPtr, &rei, varValue,NULL);
    return(i);
  }
  else
    return(UNDEFINED_VARIABLE_MAP_ERR);
}

int
setVarValue(char *varMap, ruleExecInfo_t *rei, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  char *varValue =  NULL;
  int i;

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName,"rei")) {
    i = getSetValFromRuleExecInfo(varMapCPtr, &rei, &varValue,newVarValue);
    if (varValue != NULL) free (varValue);
    return(i);
  }
  else
    return(UNDEFINED_VARIABLE_MAP_ERR);
}

int
getSetValFromRuleExecInfo(char *varMap, ruleExecInfo_t **inrei, 
		       char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  ruleExecInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = getSetLeafValue(varValue, (void**)inrei, (void **) inrei, (char*)newVarValue, RE_PTR);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName,  &varMapCPtr);
  if (i != 0)
    return(i);

  if (!strcmp(varName, "status") ) 
    i = getSetLeafValue(varValue,&(rei->status), (void **)  CAST_INT_VOIDPTR rei->status, (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "statusStr") ) 
    i = getSetLeafValue(varValue,&(rei->statusStr), (void **) rei->statusStr, (char*)newVarValue,RE_STR);
  else  if (!strcmp(varName, "rsComm") )
    i = getSetValFromRsComm(varMapCPtr, &(rei->rsComm), varValue, newVarValue);
  else  if (!strcmp(varName, "l1descInx") )
    i = getSetLeafValue(varValue,&(rei->l1descInx), (void **) CAST_INT_VOIDPTR  rei->l1descInx, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",rei->l1descInx);*/
  else  if (!strcmp(varName, "doinp") )
    i = getSetValFromDataObjInp(varMapCPtr, &(rei->doinp), varValue, newVarValue);
#if 0	/* XXXXX deplicate dinp and finp */
  else  if (!strcmp(varName, "dinp") )
    i = getSetValFromDataOprInp(varMapCPtr, &(rei->dinp), varValue, newVarValue);
  else  if (!strcmp(varName, "finp") )
    i = getSetValFromFileOpenInp(varMapCPtr, &(rei->finp), varValue, newVarValue);
#endif
  else  if (!strcmp(varName, "doi") )
    i = getSetValFromDataObjInfo(varMapCPtr, &(rei->doi), varValue, newVarValue);
  else  if (!strcmp(varName, "rgi") )
    i = getSetValFromRescGrpInfo(varMapCPtr, &(rei->rgi), varValue, newVarValue);
  else  if (!strcmp(varName, "uoic") )
    i = getSetValFromUserInfo(varMapCPtr, &(rei->uoic), varValue, newVarValue);
  else  if (!strcmp(varName, "uoip") )
    i = getSetValFromUserInfo(varMapCPtr, &(rei->uoip), varValue, newVarValue);
  else  if (!strcmp(varName, "coi") )
    i = getSetValFromCollInfo(varMapCPtr, &(rei->coi), varValue, newVarValue);
#if 0	/* XXXXX deplicate doinpo, dinpo, finpo, rgio */
  else  if (!strcmp(varName, "doinpo") )
    i = getSetValFromDataObjInp(varMapCPtr, &(rei->doinpo), varValue, newVarValue);
  else  if (!strcmp(varName, "dinpo") )
    i = getSetValFromDataOprInp(varMapCPtr, &(rei->dinpo), varValue, newVarValue);
  else  if (!strcmp(varName, "finpo") )
    i = getSetValFromFileOpenInp(varMapCPtr, &(rei->finpo), varValue, newVarValue);
  else  if (!strcmp(varName, "rgio") )
    i = getSetValFromRescGrpInfo(varMapCPtr, &(rei->rgio), varValue, newVarValue);
#endif
  else  if (!strcmp(varName, "uoio") )
    i = getSetValFromUserInfo(varMapCPtr, &(rei->uoio), varValue, newVarValue);
  else  if (!strcmp(varName, "condInputData") )
    i = getSetValFromKeyValPair(varMapCPtr, &(rei->condInputData), varValue, newVarValue);
  else  if (!strcmp(varName, "ruleSet") )
    i = getSetLeafValue(varValue,&(rei->ruleSet), (void **) rei->ruleSet , (char*)newVarValue,RE_STR);
  /*    *varValue = rei->ruleSet;*/
  else  if (!strcmp(varName, "next") )
    i = getSetValFromRuleExecInfo(varMapCPtr, &(rei->next), varValue,newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}

int
getSetValFromRsComm(char *varMap, rsComm_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rsComm_t *ptr;
  userInfo_t *userInfo;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "sock") )
    i = getSetLeafValue(varValue,&(ptr->sock), (void **)  CAST_INT_VOIDPTR ptr->sock, (char*)newVarValue, RE_INT);
    /*sprintf(*varValue, "%d",ptr->sock);*/
  else  if (!strcmp(varName, "connectCnt") )
    i = getSetLeafValue(varValue,&(ptr->connectCnt),(void **)  CAST_INT_VOIDPTR ptr->connectCnt, (char*)newVarValue, RE_INT);
    /*sprintf(*varValue, "%d",ptr->connectCnt);*/
  else  if (!strcmp(varName, "proxyUser") )
  {
    userInfo = &(ptr->proxyUser);
    i = getSetValFromUserInfo(varMapCPtr, (userInfo_t**) &userInfo, varValue, newVarValue);
  }
  else  if (!strcmp(varName, "clientUser") ) 
  {
    userInfo = &(ptr->clientUser);
    i = getSetValFromUserInfo(varMapCPtr, &userInfo, varValue, newVarValue);
  }
  else  if (!strcmp(varName, "cliVersion") )
    i = getSetValFromVersion(varMapCPtr, (version_t**)&(ptr->cliVersion), varValue, newVarValue);
  else  if (!strcmp(varName, "option") )
    i = getSetLeafValue(varValue,&(ptr->option), (void **) ptr->option, (char*)newVarValue,RE_STR);
    /* *varValue = ptr->option;*/
  else  if (!strcmp(varName, "status") )
    i = getSetLeafValue(varValue,&(ptr->status), (void **)  CAST_INT_VOIDPTR ptr->status, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->status); */
  else  if (!strcmp(varName, "apiInx") )
    i = getSetLeafValue(varValue,&(ptr->apiInx), (void **)  CAST_INT_VOIDPTR ptr->apiInx, (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "windowSize") ) 
    i = getSetLeafValue(varValue,&(ptr->windowSize), (void **) CAST_INT_VOIDPTR ptr->windowSize , (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "reconnFlag") ) 
    i = getSetLeafValue(varValue,&(ptr->reconnFlag), (void **) CAST_INT_VOIDPTR ptr->reconnFlag , (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "reconnSock") ) 
    i = getSetLeafValue(varValue,&(ptr->reconnSock), (void **) CAST_INT_VOIDPTR ptr->reconnSock , (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "reconnPort") ) 
    i = getSetLeafValue(varValue,&(ptr->reconnPort), (void **) CAST_INT_VOIDPTR ptr->reconnPort , (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "reconnAddr") ) 
    i = getSetLeafValue(varValue,&(ptr->reconnAddr),(void **) ptr->reconnAddr, (char*)newVarValue, RE_STRDUP);
  else  if (!strcmp(varName, "cookie") ) 
    i = getSetLeafValue(varValue,&(ptr->cookie), (void **)  CAST_INT_VOIDPTR ptr->cookie, (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "clientAddr") )
    i = getSetLeafValue(varValue,&(ptr->clientAddr), (void **) ptr->clientAddr, (char*)newVarValue, RE_STR);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromDataObjInfo(char *varMap, dataObjInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataObjInfo_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }

  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "objPath") )
    i = getSetLeafValue(varValue,&(ptr->objPath), (void **) ptr->objPath, (char*)newVarValue,RE_STR);
    /* *varValue = ptr->objPath;*/
  else  if (!strcmp(varName, "rescName") )
    i = getSetLeafValue(varValue,&(ptr->rescName), (void **) ptr->rescName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescName; */
  else  if (!strcmp(varName, "rescGroupName") )
    i = getSetLeafValue(varValue,&(ptr->rescGroupName), (void **) ptr->rescGroupName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescGroupName; */
  else  if (!strcmp(varName, "dataType") )
    i = getSetLeafValue(varValue,&(ptr->dataType), (void **)ptr->dataType  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataType; */
  else  if (!strcmp(varName, "dataSize") )
    i = getSetLeafValue(varValue,&(ptr->dataSize), (void **) (long) ptr->dataSize, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->dataSize); */
  else  if (!strcmp(varName, "chksum") )
    i = getSetLeafValue(varValue,&(ptr->chksum), (void **)ptr->chksum  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->chksum; */
  else  if (!strcmp(varName, "version") )
    i = getSetLeafValue(varValue,&(ptr->version), (void **) ptr->version , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->version; */
  else  if (!strcmp(varName, "filePath") )
    i = getSetLeafValue(varValue,&(ptr->filePath), (void **) ptr->filePath , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->filePath; */
  else  if (!strcmp(varName, "rescInfo") )
    i = getSetValFromRescInfo(varMapCPtr, &(ptr->rescInfo), varValue, newVarValue);
  else  if (!strcmp(varName, "dataOwnerName") )
    i = getSetLeafValue(varValue,&(ptr->dataOwnerName), (void **) ptr->dataOwnerName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataOwnerName; */
  else  if (!strcmp(varName, "dataOwnerZone") )
    i = getSetLeafValue(varValue,&(ptr->dataOwnerZone), (void **) ptr->dataOwnerZone , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataOwnerZone; */
  else  if (!strcmp(varName, "replNum") )
    i = getSetLeafValue(varValue,&(ptr->replNum), (void **)  CAST_INT_VOIDPTR ptr->replNum, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->replNum); */
  else  if (!strcmp(varName, "replStatus") )
    i = getSetLeafValue(varValue,&(ptr->replStatus), (void **)  CAST_INT_VOIDPTR ptr->replStatus, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->replStatus); */
  else  if (!strcmp(varName, "writeFlag") )
    i = getSetLeafValue(varValue,&(ptr->writeFlag), (void **)  CAST_INT_VOIDPTR ptr->writeFlag, (char*)newVarValue, RE_INT);
  else  if (!strcmp(varName, "statusString") )
    i = getSetLeafValue(varValue,&(ptr->statusString), (void **) ptr->statusString , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->statusString; */
  else  if (!strcmp(varName, "dataId") )
    i = getSetLeafValue(varValue,&(ptr->dataId), (void **) (long) ptr->dataId, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->dataId); */
  else  if (!strcmp(varName, "collId") )
    i = getSetLeafValue(varValue,&(ptr->collId), (void **) (long) ptr->collId, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->collId); */
  else  if (!strcmp(varName, "dataMapId") )
    i = getSetLeafValue(varValue,&(ptr->dataMapId), (void **)  CAST_INT_VOIDPTR ptr->dataMapId, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->dataMapId); */
  else  if (!strcmp(varName, "dataComments") )
    i = getSetLeafValue(varValue,&(ptr->dataComments), (void **) ptr->dataComments , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataComments; */
  else  if (!strcmp(varName, "dataExpiry") )
    i = getSetLeafValue(varValue,&( ptr->dataExpiry), (void **)  ptr->dataExpiry , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataExpiry; */
  else  if (!strcmp(varName, "dataCreate") )
    i = getSetLeafValue(varValue,&(ptr->dataCreate), (void **) ptr->dataCreate , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataCreate; */
  else  if (!strcmp(varName, "dataModify") )
    i = getSetLeafValue(varValue,&(ptr->dataModify), (void **) ptr->dataModify , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataModify; */
  else  if (!strcmp(varName, "dataAccess") )
    i = getSetLeafValue(varValue,&(ptr->dataAccess), (void **)ptr->dataAccess  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->dataAccess; */
  else  if (!strcmp(varName, "dataAccessInx") )
    i = getSetLeafValue(varValue,&(ptr->dataAccessInx), (void **) CAST_INT_VOIDPTR ptr->dataAccessInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->dataAccessInx); */
  else  if (!strcmp(varName, "destRescName") )
    i = getSetLeafValue(varValue,&(ptr->destRescName), (void **) ptr->destRescName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->destRescName; */
  else  if (!strcmp(varName, "backupRescName") )
    i = getSetLeafValue(varValue,&(ptr->backupRescName), (void **) ptr->backupRescName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->backupRescName; */
  else  if (!strcmp(varName, "next") )
    i = getSetValFromDataObjInfo(varMapCPtr, &(ptr->next), varValue, newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}

int
getSetValFromDataObjInp(char *varMap, dataObjInp_t  **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataObjInp_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "objPath") )
    i = getSetLeafValue(varValue,&(ptr->objPath), (void **) ptr->objPath , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->objPath; */
  else  if (!strcmp(varName, "createMode") )
    i = getSetLeafValue(varValue,&(ptr->createMode), (void **) CAST_INT_VOIDPTR ptr->createMode , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->createMode); */
  else  if (!strcmp(varName, "openFlags") )
    i = getSetLeafValue(varValue,&(ptr->openFlags), (void **)  CAST_INT_VOIDPTR ptr->openFlags, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->openFlags); */
  else  if (!strcmp(varName, "offset") )
    i = getSetLeafValue(varValue,&(ptr->offset), (void **)(long) ptr->offset , (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->offset); */
  else  if (!strcmp(varName, "dataSize") )
    i = getSetLeafValue(varValue,&(ptr->dataSize), (void **) (long) ptr->dataSize, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->dataSize); */
  else  if (!strcmp(varName, "numThreads") )
    i = getSetLeafValue(varValue,&(ptr->numThreads), (void **) CAST_INT_VOIDPTR ptr->numThreads , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->numThreads); */
  else  if (!strcmp(varName, "oprType") )
    i = getSetLeafValue(varValue,&(ptr->oprType), (void **) CAST_INT_VOIDPTR ptr->oprType , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->oprType); */
  else  if (!strcmp(varName, "condInput") )
    i = getSetValFromKeyValPair(varMapCPtr, (keyValPair_t **) &(ptr->condInput),varValue, (char*)newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}

int
getSetValFromDataOprInp(char *varMap, dataOprInp_t  **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataOprInp_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "oprType") )
    i = getSetLeafValue(varValue,&(ptr->oprType), (void **) CAST_INT_VOIDPTR ptr->oprType , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->oprType); */
  else  if (!strcmp(varName, "offset") )
    i = getSetLeafValue(varValue,&(ptr->offset), (void **)(long)  ptr->offset, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->offset); */
  else  if (!strcmp(varName, "dataSize") )
    i = getSetLeafValue(varValue,&(ptr->dataSize), (void **)(long) ptr->dataSize , (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->dataSize); */
  else  if (!strcmp(varName, "numThreads") )
    i = getSetLeafValue(varValue,&(ptr->numThreads), (void **)  CAST_INT_VOIDPTR ptr->numThreads, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->numThreads); */
  else  if (!strcmp(varName, "srcL3descInx") )
    i = getSetLeafValue(varValue,&(ptr->srcL3descInx), (void **) CAST_INT_VOIDPTR ptr->srcL3descInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->srcL3descInx); */
  else  if (!strcmp(varName, "destL3descInx") )
    i = getSetLeafValue(varValue,&(ptr->destL3descInx), (void **) CAST_INT_VOIDPTR ptr->destL3descInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->destL3descInx); */
  else  if (!strcmp(varName, "srcRescTypeInx") )
    i = getSetLeafValue(varValue,&(ptr->srcRescTypeInx), (void **) CAST_INT_VOIDPTR ptr->srcRescTypeInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->srcRescTypeInx); */
  else  if (!strcmp(varName, "destRescTypeInx") )
    i = getSetLeafValue(varValue,&(ptr->destRescTypeInx), (void **) CAST_INT_VOIDPTR ptr->destRescTypeInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->destRescTypeInx); */
  else  if (!strcmp(varName, "condInput") )
    i = getSetValFromKeyValPair(varMapCPtr, (keyValPair_t**)&(ptr->condInput),varValue, (char*)newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}



int
getSetValFromRescInfo(char *varMap, rescInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rescInfo_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "rescName") )
    i = getSetLeafValue(varValue,&(ptr->rescName), (void **) ptr->rescName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescName; */
  else  if (!strcmp(varName, "rescId") )
    i = getSetLeafValue(varValue,&(ptr->rescId), (void **)(long)  ptr->rescId, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->rescId); */
  else  if (!strcmp(varName, "zoneName") )
    i = getSetLeafValue(varValue,&(ptr->zoneName), (void **) ptr->zoneName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->zoneName; */
  else  if (!strcmp(varName, "rescLoc") )
    i = getSetLeafValue(varValue,&(ptr->rescLoc), (void **)ptr->rescLoc  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescLoc; */
  else  if (!strcmp(varName, "rescType") )
    i = getSetLeafValue(varValue,&(ptr->rescType), (void **) ptr->rescType , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescType; */
  else  if (!strcmp(varName, "rescTypeInx") )
    i = getSetLeafValue(varValue,&(ptr->rescTypeInx), (void **) CAST_INT_VOIDPTR ptr->rescTypeInx , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->rescTypeInx); */
  else  if (!strcmp(varName, "rescClass") )
    i = getSetLeafValue(varValue,&(ptr->rescClass), (void **) ptr->rescClass , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescClass; */
  else  if (!strcmp(varName, "rescClassInx") )
    i = getSetLeafValue(varValue,&(ptr->rescClassInx), (void **)  CAST_INT_VOIDPTR ptr->rescClassInx, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->rescClassInx); */
  else  if (!strcmp(varName, "rescVaultPath") )
    i = getSetLeafValue(varValue,&(ptr->rescVaultPath), (void **)ptr->rescVaultPath  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescVaultPath; */
  else  if (!strcmp(varName, RESC_STATUS_KW) )
    i = getSetLeafValue(varValue,&(ptr->rescStatus), (void **) CAST_INT_VOIDPTR ptr->rescStatus , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->numOpenPorts); */
  else  if (!strcmp(varName, "paraOpr") )
    i = getSetLeafValue(varValue,&(ptr->paraOpr), (void **) CAST_INT_VOIDPTR ptr->paraOpr , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->paraOpr); */
  else  if (!strcmp(varName, "rescInfo") )
    i = getSetLeafValue(varValue,&(ptr->rescInfo), (void **) ptr->rescInfo , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescInfo; */
  else  if (!strcmp(varName, "rescComments") )
    i = getSetLeafValue(varValue,&(ptr->rescComments), (void **) ptr->rescComments , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescComments; */
  else  if (!strcmp(varName, "gateWayAddr") )
    i = getSetLeafValue(varValue,&(ptr->gateWayAddr), (void **) ptr->gateWayAddr , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->gateWayAddr; */
  else  if (!strcmp(varName, "rescMaxObjSize") )
    i = getSetLeafValue(varValue,&(ptr->rescMaxObjSize), (void **)(long)  ptr->rescMaxObjSize, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->rescMaxObjSize); */
  else  if (!strcmp(varName, "freeSpace") )
    i = getSetLeafValue(varValue,&(ptr->freeSpace), (void **)(long) ptr->freeSpace , (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->freeSpace); */
  else  if (!strcmp(varName, "freeSpaceTimeStamp") )
    i = getSetLeafValue(varValue,&(ptr->freeSpaceTimeStamp), (void **) ptr->freeSpaceTimeStamp , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->freeSpaceTimeStamp; */
  else  if (!strcmp(varName, "rescCreate") )
    i = getSetLeafValue(varValue,&(ptr->rescCreate), (void **) ptr->rescCreate , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescCreate; */
  else  if (!strcmp(varName, "rescModify") )
    i = getSetLeafValue(varValue,&(ptr->rescModify), (void **) ptr->rescModify , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescModify; */
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}

int
getSetValFromRescGrpInfo(char *varMap, rescGrpInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rescGrpInfo_t *ptr;

  ptr = *inptr;
  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }

  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "rescGroupName") )
    i = getSetLeafValue(varValue,&(ptr->rescGroupName), (void **) ptr->rescGroupName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rescGroupName; */
  else  if (!strcmp(varName, "rescInfo") )
    i = getSetValFromRescInfo(varMapCPtr, &(ptr->rescInfo), varValue, newVarValue);
  else  if (!strcmp(varName, "cacheNext") )
    i = getSetValFromRescGrpInfo(varMapCPtr, &( ptr->cacheNext), varValue, newVarValue);
  else  if (!strcmp(varName, "next") )
    i = getSetValFromRescGrpInfo(varMapCPtr, &( ptr->next), varValue, newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromUserInfo(char *varMap, userInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  userInfo_t *ptr;
  authInfo_t *authInfo;

  ptr = *inptr;

  if (varMap == NULL) {
    /*     rodsLog (LOG_NOTICE,"gSVUIEnter:%s::%d\n",ptr->userName,ptr);*/
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    /*     rodsLog (LOG_NOTICE,"gSVUIExit :%s::%d\n",ptr->userName,ptr);*/
      return(i);
    /*
      *varValue = (char *) ptr;
      if (newVarValue != NULL)
      *inptr = newVarValue;
      return(0);
    */
  }

  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "userName") )
    i = getSetLeafValue(varValue,&(ptr->userName), (void **) ptr->userName, (char*)newVarValue, RE_STR);
  /* *varValue = ptr->userName;*/
  else  if (!strcmp(varName, "rodsZone") )
    i = getSetLeafValue(varValue,&(ptr->rodsZone), (void **) ptr->rodsZone, (char*)newVarValue, RE_STR);
  /*   *varValue = ptr->rodsZone;*/
  else  if (!strcmp(varName, "userType") )
    i = getSetLeafValue(varValue,&(ptr->userType),(void **)  ptr->userType, (char*)newVarValue, RE_STR);
  /*   *varValue = ptr->userType;*/
  else  if (!strcmp(varName, "sysUid") )
    i = getSetLeafValue(varValue,&(ptr->sysUid), (void **)  CAST_INT_VOIDPTR ptr->sysUid, (char*)newVarValue, RE_INT);
  /*   sprintf(*varValue, "%d",ptr->sysUid);*/
  else  if (!strcmp(varName, "authInfo") )
  {
    authInfo = &(ptr->authInfo);
    i = getSetValFromAuthInfo(varMapCPtr, &authInfo, varValue, 
      newVarValue);
  }
  else  if (!strcmp(varName, "userOtherInfo") )
    i = getSetValFromUserOtherInfo(varMapCPtr, (userOtherInfo_t **) &(ptr->userOtherInfo), varValue, (char*)newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromCollInfo(char *varMap, collInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  collInfo_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "collId") )
    i = getSetLeafValue(varValue,&(ptr->collId), (void **)(long) ptr->collId , (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->collId); */
  else  if (!strcmp(varName, "collName") )
    i = getSetLeafValue(varValue,&(ptr->collName), (void **)ptr->collName  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collName; */
  else  if (!strcmp(varName, "collParentName") )
    i = getSetLeafValue(varValue,&(ptr->collParentName), (void **) ptr->collParentName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collParentName; */
  else  if (!strcmp(varName, "collOwnerName") )
    i = getSetLeafValue(varValue,&(ptr->collOwnerName), (void **)ptr->collOwnerName  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collOwnerName; */
  else  if (!strcmp(varName, "collOwnerZone") )
    i = getSetLeafValue(varValue,&(ptr->collOwnerZone), (void **)  ptr->collOwnerZone, (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collOwnerZone; */
  else  if (!strcmp(varName, "collMapId") )
    i = getSetLeafValue(varValue,&(ptr->collMapId), (void **)  CAST_INT_VOIDPTR ptr->collMapId, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->collMapId); */
  else  if (!strcmp(varName, "collComments") )
    i = getSetLeafValue(varValue,&(ptr->collComments), (void **) ptr->collComments , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collComments; */
  else  if (!strcmp(varName, "collInheritance") )
    i = getSetLeafValue(varValue,&(ptr->collInheritance), (void **) ptr->collInheritance , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collInheritance; */
  else  if (!strcmp(varName, "collExpiry") )
    i = getSetLeafValue(varValue,&(ptr->collExpiry), (void **) ptr->collExpiry , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collExpiry; */
  else  if (!strcmp(varName, "collCreate") )
    i = getSetLeafValue(varValue,&(ptr->collCreate), (void **) ptr->collCreate , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collCreate; */
  else  if (!strcmp(varName, "collModify") )
    i = getSetLeafValue(varValue,&(ptr->collModify), (void **) ptr->collModify , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collModify; */
  else  if (!strcmp(varName, "collAccess") )
    i = getSetLeafValue(varValue,&(ptr->collAccess), (void **) ptr->collAccess , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->collAccess; */
  else  if (!strcmp(varName, "collAccessInx") )
    i = getSetLeafValue(varValue,&(ptr->collAccessInx), (void **)  CAST_INT_VOIDPTR ptr->collAccessInx, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->collAccessInx); */
  else  if (!strcmp(varName, "next") )
    i = getSetValFromCollInfo(varMapCPtr, &(ptr->next), varValue, (char*)newVarValue);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromKeyValPair(char *varMap, keyValPair_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  char *Cptr;
  int i;
  keyValPair_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  Cptr = (char *) getValByKey(ptr,varName);
  /**** may need to do a st value by key ****/
  if (Cptr == NULL)
    return(UNMATCHED_KEY_OR_INDEX);
  *varValue = Cptr;
  return(0);
}


int
getSetValFromVersion(char *varMap, version_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  version_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "status") )
    i = getSetLeafValue(varValue,&(ptr->status), (void **) CAST_INT_VOIDPTR ptr->status , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->status); */
  else  if (!strcmp(varName, "relVersion") )
    i = getSetLeafValue(varValue,&(ptr->relVersion), (void **) ptr->relVersion , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->relVersion; */
  else  if (!strcmp(varName, "apiVersion") )
    i = getSetLeafValue(varValue,&(ptr->apiVersion), (void **) ptr->apiVersion , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->apiVersion; */
  else  if (!strcmp(varName, "apiVersion") )
    i = getSetLeafValue(varValue,&(ptr->reconnAddr), (void **) ptr->reconnAddr , (char*)newVarValue,RE_STR);
  else  if (!strcmp(varName, "apiVersion") )
    i = getSetLeafValue(varValue,&(ptr->reconnPort), (void **)  CAST_INT_VOIDPTR ptr->reconnPort , (char*)newVarValue,RE_INT);
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromAuthInfo(char *varMap, authInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  authInfo_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  
  if (!strcmp(varName, "authScheme") )
    i = getSetLeafValue(varValue,&(ptr->authScheme), (void **)ptr->authScheme  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->authScheme; */
  else  if (!strcmp(varName, "authFlag") )
    i = getSetLeafValue(varValue,&(ptr->authFlag), (void **) CAST_INT_VOIDPTR ptr->authFlag , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->authFlag); */
  else  if (!strcmp(varName, "flag") )
    i = getSetLeafValue(varValue,&(ptr->flag), (void **) CAST_INT_VOIDPTR ptr->flag , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->flag); */
  else  if (!strcmp(varName, "ppid") )
    i = getSetLeafValue(varValue,&(ptr->ppid), (void **) CAST_INT_VOIDPTR ptr->ppid , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->ppid); */
  else  if (!strcmp(varName, "host") )
    i = getSetLeafValue(varValue,&(ptr->host), (void **)ptr->host  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->host; */
  else  if (!strcmp(varName, "authStr") )
    i = getSetLeafValue(varValue,&(ptr->authStr), (void **) ptr->authStr , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->authStr; */
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromUserOtherInfo(char *varMap, userOtherInfo_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  userOtherInfo_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  
  if (!strcmp(varName, "userInfo") )
    i = getSetLeafValue(varValue,&(ptr->userInfo), (void **) ptr->userInfo , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->userInfo; */
  else  if (!strcmp(varName, "userComments") )
    i = getSetLeafValue(varValue,&(ptr->userComments), (void **)ptr->userComments  , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->userComments; */
  else  if (!strcmp(varName, " userCreate") )
    i = getSetLeafValue(varValue,&(ptr-> userCreate), (void **) ptr-> userCreate , (char*)newVarValue,RE_STR);
    /* *varValue = ptr-> userCreate; */
  else  if (!strcmp(varName, "userModify") )
    i = getSetLeafValue(varValue,&(ptr->userModify), (void **) ptr->userModify , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->userModify; */
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromFileOpenInp(char *varMap, fileOpenInp_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  fileOpenInp_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "fileType") )
    i = getSetLeafValue(varValue,&(ptr->fileType), (void **) ptr->fileType, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->fileType);  */
  else  if (!strcmp(varName, "addr") )
    i = getSetValFromRodsHostAddr(varMapCPtr, (rodsHostAddr_t **)&(ptr->addr), varValue, (char*)newVarValue);
  else  if (!strcmp(varName, "fileName") )
    i = getSetLeafValue(varValue,&(ptr->fileName), (void **) ptr->fileName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->fileName; */
  else  if (!strcmp(varName, "flags") )
    i = getSetLeafValue(varValue,&(ptr->flags), (void **) CAST_INT_VOIDPTR ptr->flags , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->flags); */
  else  if (!strcmp(varName, "mode") )
    i = getSetLeafValue(varValue,&(ptr->mode), (void **) CAST_INT_VOIDPTR ptr->mode , (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->mode); */
  else  if (!strcmp(varName, "dataSize") )
    i = getSetLeafValue(varValue,&(ptr->dataSize), (void **)(long)  ptr->dataSize, (char*)newVarValue, RE_LONG);
    /* sprintf(*varValue, "%lld",ptr->dataSize); */
  else  if (!strcmp(varName, "otherFlags") )
    i = getSetLeafValue(varValue,&(ptr->otherFlags), (void **)  CAST_INT_VOIDPTR ptr->otherFlags, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->otherFlags); */
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFromRodsHostAddr(char *varMap, rodsHostAddr_t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rodsHostAddr_t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void **) inptr, (char*)newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  
  if (!strcmp(varName, "hostAddr") )
    i = getSetLeafValue(varValue,&(ptr->hostAddr), (void **) ptr->hostAddr , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->hostAddr; */
  else  if (!strcmp(varName, "rodsZone") )
    i = getSetLeafValue(varValue,&(ptr->zoneName), (void **) ptr->zoneName , (char*)newVarValue,RE_STR);
    /* *varValue = ptr->rodsZone; */
  else  if (!strcmp(varName, "portNum") )
    i = getSetLeafValue(varValue,&(ptr->portNum), (void **)  CAST_INT_VOIDPTR   ptr->portNum, (char*)newVarValue, RE_INT);
    /* sprintf(*varValue, "%d",ptr->portNum); */
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}



/****************
int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *) ptr-> , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *) ptr-> , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr->  , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return(i);
  }
  
  if (ptr == NULL)
    return(NULL_VALUE_ERR);
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else 
    return(UNDEFINED_VARIABLE_MAP_ERR);
  return(i);

}
*****************/

int
getAllSessionVarValue (char *action, ruleExecInfo_t *rei,
keyValPair_t *varKeyVal)
{
  int i, status;
  char *varValue;
  char *lastVar = NULL; 	/* last var that has data */

  if (varKeyVal == NULL || rei == NULL) {
    rodsLog (LOG_ERROR,
      "getAllSessionVarValue: input rei or varKeyVal is NULL");
      return SYS_INTERNAL_NULL_INPUT_ERR;
  }

  for (i = 0; i < coreRuleVarDef.MaxNumOfDVars; i++) {
    if (lastVar == NULL || strcmp (lastVar, coreRuleVarDef.varName[i]) != 0) {
      status = getSessionVarValue ("", coreRuleVarDef.varName[i], rei,
        &varValue);
      if (status >= 0 && varValue != NULL) {
        lastVar = coreRuleVarDef.varName[i];
        addKeyVal (varKeyVal, lastVar, varValue);
	free (varValue);
      }
    }
  }
  return 0;
}

int
getSessionVarValue (char *action, char *varName, ruleExecInfo_t *rei,
char **varValue)
{
  char *varMap;
  int i, vinx;

  vinx = getVarMap (action,varName, &varMap, 0);
  while (vinx >= 0) {
    i = getVarValue (varMap, rei, varValue);
    if (i >= 0) {
      free(varMap);
      return(i);
    } else if (i == NULL_VALUE_ERR) {
      free(varMap);
      vinx = getVarMap (action,varName, &varMap, vinx+1);
    } else {
      free(varMap);
      return(i);
    }
  }
  if (vinx < 0) {
    return(vinx);
  }
  return(i);
}


