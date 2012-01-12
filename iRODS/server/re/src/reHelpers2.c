/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"
extern char *rmemmove (void *dest, void *src, int strLen, int maxLen);

static int staticVarNumber = 1;

int
replaceArgVar( char *start, int size, 
	       char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc)
{
  int i,j;
  char *t1, *t2;

  t1 = start;
  
  if (strstr(t1,"$ARG[") == t1) {
    t2 = t1+5;
    while (isdigit(*t2)) t2++;
    if (*t2 != ']')
      return(INPUT_ARG_NOT_WELL_FORMED_ERR);
    *t2 = '\0';
    j = atoi((char *)(t1+5));
    *t2 = ']';
    if (j >= argc)
      return(INPUT_ARG_OUT_OF_ARGC_RANGE_ERR);
    i = replaceStrValue(start,size,args[j], t2 - start + 1);
    return(i);
  }
  return(UNKNOWN_PARAM_IN_RULE_ERR);
}

int 
replaceSessionVar(char *action,  char *start, int size,  ruleExecInfo_t *rei)
{
  char varName[NAME_LEN];
  char *varMap;
  char *varValue;
  int i,nLen, vinx;
  char *t1;
  /*char tmpVarValue[MAX_NAME_LEN];*/

  /*
  varValue = tmpVarValue;
  */
  varValue = NULL;

  t1 = start + 1;
  while (isalnum(*t1) ||   *t1 == '_') t1++;
  nLen = t1 - (start + 1);
  if (nLen >= NAME_LEN )
    return(VARIABLE_NAME_TOO_LONG_ERR);
  strncpy(varName, (start + 1),nLen);
  varName[nLen] = '\0';

  vinx = getVarMap(action,varName, &varMap, 0);
  while (vinx >= 0) {
    i = getVarValue(varMap, rei, &varValue);
    if (i >= 0) {
      i = replaceStrValue(start, size,varValue, nLen+1);
      free(varMap);
      if (varValue != NULL) free (varValue);
      return(i);
    }
    else if (i == NULL_VALUE_ERR) {
      free(varMap);
      vinx = getVarMap(action,varName, &varMap, vinx+1);
    }
    else {
      free(varMap);
      if (varValue != NULL) free (varValue);
      return(i);
    }
  }
  if (vinx < 0) {
    if (varValue != NULL) free (varValue);
    return(vinx);
  }
  /*
  i = getVarValue(varMap, rei, &varValue);
  if (i != 0) {
    free(varMap);
    if (varValue != NULL) free (varValue);
    return(i);
  }
  i = replaceStrValue(start, size,varValue, nLen+1);
  free(varMap);
  */
  if (varValue != NULL) free (varValue);
  return(i);
}

int
replaceStarVar(char *action,  char *start, int size,   msParamArray_t *inMsParamArray)
{
  char varName[NAME_LEN];
  /*  char *varMap;*/
  char *varValue;
  int i,nLen;
  char *t1;
  char tmpVarValue[MAX_NAME_LEN];
  msParam_t *mP;

  varValue = tmpVarValue;


  t1 = start + 1 ;
  while (isalnum(*t1) ||   *t1 == '_') t1++;
  nLen = t1 - start ;
  if (nLen >= NAME_LEN )
    return(VARIABLE_NAME_TOO_LONG_ERR);
  strncpy(varName, start ,nLen);
  varName[nLen] = '\0';

  mP = getMsParamByLabel (inMsParamArray, varName);
  if (mP == NULL || mP->inOutStruct == NULL)
    return(0);


/*************************** Added by AdT on 09/24/2008 *********************/
if (strcmp(mP->type, INT_MS_T) == 0)
{
	i = replaceIntValue(start, size, *(int *)mP->inOutStruct, nLen);
	return (i);
}
/****************************************************************************/

  if (strcmp(mP->type,STR_MS_T) != 0)
    return(0);
  i = replaceStrValue(start, size, (char *) mP->inOutStruct,  nLen);
  return(i);

}

int
replaceDataVar( char *start, int size,  dataObjInfo_t *doi)
{
  int i;
  char *t1;


  t1 = start + 1;
  if (strstr(t1,OBJ_PATH_KW) == t1) {
    if (doi->objPath == NULL)
      return(OBJPATH_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->objPath,strlen(OBJ_PATH_KW)+1);
  }
  else if (strstr(t1,RESC_NAME_KW) == t1) {
    if (doi->rescName == NULL)
      return(RESCNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->rescName,strlen(RESC_NAME_KW)+1);
  }
  else if (strstr(t1,DEST_RESC_NAME_KW) == t1) {
    if (doi->destRescName == NULL)
      return(DESTRESCNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->destRescName,strlen(DEST_RESC_NAME_KW)+1);
  }
  else if (strstr(t1,DEF_RESC_NAME_KW) == t1) {
    if (doi->destRescName == NULL)
      return(DESTRESCNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->destRescName,strlen(DEF_RESC_NAME_KW)+1);
  }
  else if (strstr(t1,BACKUP_RESC_NAME_KW) == t1) {
    if (doi->backupRescName == NULL)
      return(BACKUPRESCNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->backupRescName,strlen(BACKUP_RESC_NAME_KW)+1);
  }
  else if (strstr(t1,DATA_TYPE_KW) == t1) {
    if (doi->dataType == NULL)
      return(DATATYPE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataType,strlen(DATA_TYPE_KW)+1);
  }
  else if (strstr(t1,DATA_SIZE_KW) == t1) {
    if (doi->dataSize == 0)
      return(DATASIZE_EMPTY_IN_STRUCT_ERR);
    i = replaceLongValue(start,size,doi->dataSize,strlen(DATA_SIZE_KW)+1);
  }
  else if (strstr(t1,CHKSUM_KW) == t1) {
    if (doi->chksum == NULL)
      return(CHKSUM_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->chksum,strlen(CHKSUM_KW)+1);
  }
  else if (strstr(t1,VERSION_KW) == t1) {
    if (doi->version == NULL)
      return(VERSION_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->version,strlen(VERSION_KW)+1);
  }
  else if (strstr(t1,FILE_PATH_KW) == t1) {
    if (doi->filePath == NULL)
      return(FILEPATH_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->filePath,strlen(FILE_PATH_KW)+1);
  }
  else if (strstr(t1,REPL_NUM_KW) == t1) {
    if (doi->replNum == 0)
      return(REPLNUM_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,doi->replNum,strlen(REPL_NUM_KW)+1);
  }
  else if (strstr(t1,REPL_STATUS_KW) == t1) {
    if (doi->replStatus == 0)
      return(REPLSTATUS_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,doi->replStatus,strlen(REPL_STATUS_KW)+1);
  }
  else if (strstr(t1,DATA_OWNER_KW) == t1) {
    if (doi->dataOwnerName == NULL)
      return(DATAOWNER_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataOwnerName,strlen(DATA_OWNER_KW)+1);
  }
  else if (strstr(t1,DATA_OWNER_ZONE_KW) == t1) {
    if (doi->dataOwnerZone == NULL)
      return(DATAOWNERZONE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataOwnerZone,strlen(DATA_OWNER_ZONE_KW)+1);
  }
  else if (strstr(t1,DATA_EXPIRY_KW) == t1) {
    if (doi->dataExpiry == NULL)
      return(DATAEXPIRY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataExpiry,strlen(DATA_EXPIRY_KW)+1);
  }
  else if (strstr(t1,DATA_COMMENTS_KW) == t1) {
    if (doi->dataComments == NULL)
      return(DATACOMMENTS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataComments,strlen(DATA_COMMENTS_KW)+1);
  }
  else if (strstr(t1,DATA_CREATE_KW) == t1) {
    if (doi->dataCreate == NULL)
      return(DATACREATE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataCreate,strlen(DATA_CREATE_KW)+1);
  }
  else if (strstr(t1,DATA_MODIFY_KW) == t1) {
    if (doi->dataModify == NULL)
      return(DATAMODIFY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataModify,strlen(DATA_MODIFY_KW)+1);
  }
  else if (strstr(t1,DATA_ACCESS_KW) == t1) {
    if (doi->dataAccess == NULL)
      return(DATAACCESS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->dataAccess,strlen(DATA_ACCESS_KW)+1);
  }

  else if (strstr(t1,DATA_ACCESS_INX_KW) == t1) {
    if (doi->dataAccessInx == 0)
      return(DATAACCESSINX_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,doi->dataAccessInx,strlen(DATA_ACCESS_INX_KW)+1);
  }
  else if (strstr(t1,DATA_ID_KW) == t1) {
    if (doi->dataId == 0)
      return(DATAID_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,doi->dataId,strlen(DATA_ID_KW)+1);
  }
  else if (strstr(t1,COLL_ID_KW) == t1) {
    if (doi->collId == 0)
      return(COLLID_EMPTY_IN_STRUCT_ERR);
#if 0	/* use replaceLongValue because collId is rodsLong_t now. mw */
    i = replaceIntValue(start,size,doi->collId,strlen(COLL_ID_KW)+1);
#endif
    i = replaceLongValue(start,size,doi->collId,strlen(COLL_ID_KW)+1);
  }
  else if (strstr(t1,RESC_GROUP_NAME_KW) == t1) {
    if (doi->rescGroupName == NULL)
      return(RESCGROUPNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->rescGroupName,strlen(RESC_GROUP_NAME_KW)+1);
  }
  else if (strstr(t1,STATUS_STRING_KW) == t1) {
    if (doi->statusString == NULL)
      return(STATUSSTRING_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,doi->statusString,strlen(STATUS_STRING_KW)+1);
  }
  else if (strstr(t1,DATA_MAP_ID_KW) == t1) {
    if (doi->dataMapId == 0)
      return(DATAMAPID_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,doi->dataMapId,strlen(DATA_MAP_ID_KW)+1);
  }
  else
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  return(i);
}

int
replaceUserVar( char *start, int size,  userInfo_t *uoic,  userInfo_t *uoip)
{
  int i;
  char *t1;


  t1 = start + 1;
  if (strstr(t1,USER_NAME_CLIENT_KW) == t1) {
    if (uoic->userName == NULL)
      return(USERNAMECLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userName,strlen(USER_NAME_CLIENT_KW)+1);
  }
  else if (strstr(t1,RODS_ZONE_CLIENT_KW) == t1) {
    if (uoic->rodsZone == NULL)
      return(RODSZONECLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->rodsZone,strlen(RODS_ZONE_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_TYPE_CLIENT_KW) == t1) {
    if (uoic->userType == NULL)
      return(USERTYPECLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userType,strlen(USER_TYPE_CLIENT_KW)+1);
  }
  else if (strstr(t1,HOST_CLIENT_KW) == t1) {
    if (uoic->authInfo.host == NULL)
      return(HOSTCLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->authInfo.host,strlen(HOST_CLIENT_KW)+1);
  }
  else if (strstr(t1,AUTH_STR_CLIENT_KW) == t1) {
    if (uoic->authInfo.host == NULL)
      return(AUTHSTRCLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->authInfo.authStr,strlen(AUTH_STR_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_AUTH_SCHEME_CLIENT_KW) == t1) {
    if (uoic->authInfo.authScheme == 0)
      return(USERAUTHSCHEMECLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->authInfo.authScheme,
			strlen(USER_AUTH_SCHEME_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_INFO_CLIENT_KW) == t1) {
    if (uoic->userOtherInfo.userInfo == NULL)
      return(USERINFOCLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userOtherInfo.userInfo,strlen(USER_INFO_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_COMMENT_CLIENT_KW) == t1) {
    if (uoic->userOtherInfo.userComments == NULL)
      return(USERCOMMENTCLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userOtherInfo.userComments,
			strlen(USER_COMMENT_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_CREATE_CLIENT_KW) == t1) {
    if (uoic->userOtherInfo.userCreate == NULL)
      return(USERCREATECLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userOtherInfo.userCreate,
			strlen(USER_CREATE_CLIENT_KW)+1);
  }
  else if (strstr(t1,USER_MODIFY_CLIENT_KW) == t1) {
    if (uoic->userOtherInfo.userModify == NULL)
      return(USERMODIFYCLIENT_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoic->userOtherInfo.userModify,
			strlen(USER_MODIFY_CLIENT_KW)+1);
  }
  else   if (strstr(t1,USER_NAME_PROXY_KW) == t1) {
    if (uoip->userName == NULL)
      return(USERNAMEPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userName,strlen(USER_NAME_PROXY_KW)+1);
  }
  else if (strstr(t1,RODS_ZONE_PROXY_KW) == t1) {
    if (uoip->rodsZone == NULL)
      return(RODSZONEPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->rodsZone,strlen(RODS_ZONE_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_TYPE_PROXY_KW) == t1) {
    if (uoip->userType == NULL)
      return(USERTYPEPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userType,strlen(USER_TYPE_PROXY_KW)+1);
  }
  else if (strstr(t1,HOST_PROXY_KW) == t1) {
    if (uoip->authInfo.host == NULL)
      return(HOSTPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->authInfo.host,strlen(HOST_PROXY_KW)+1);
  }
  else if (strstr(t1,AUTH_STR_PROXY_KW) == t1) {
    if (uoip->authInfo.authStr == NULL)
      return(AUTHSTRPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->authInfo.authStr,strlen(AUTH_STR_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_AUTH_SCHEME_PROXY_KW) == t1) {
    if (uoip->authInfo.authScheme == 0)
      return(USERAUTHSCHEMEPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->authInfo.authScheme,
			strlen(USER_AUTH_SCHEME_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_INFO_PROXY_KW) == t1) {
    if (uoip->userOtherInfo.userInfo == NULL)
      return(USERINFOPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userOtherInfo.userInfo,strlen(USER_INFO_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_COMMENT_PROXY_KW) == t1) {
    if (uoip->userOtherInfo.userComments == NULL)
      return(USERCOMMENTPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userOtherInfo.userComments,
			strlen(USER_COMMENT_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_CREATE_PROXY_KW) == t1) {
    if (uoip->userOtherInfo.userCreate == NULL)
      return(USERCREATEPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userOtherInfo.userCreate,
			strlen(USER_CREATE_PROXY_KW)+1);
  }
  else if (strstr(t1,USER_MODIFY_PROXY_KW) == t1) {
    if (uoip->userOtherInfo.userModify == NULL)
      return(USERMODIFYPROXY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,uoip->userOtherInfo.userModify,
			strlen(USER_MODIFY_PROXY_KW)+1);
  }
  else
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  return(i);
}



int
replaceCollVar( char *start, int size,  collInfo_t *coi)
{
  int i;
  char *t1;


  t1 = start + 1;
  if (strstr(t1,COLL_NAME_KW) == t1) {
    if (coi->collName == NULL)
      return(COLLNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collName,strlen(COLL_NAME_KW)+1);
  }
  else if (strstr(t1,COLL_PARENT_NAME_KW) == t1) {
    if (coi->collParentName == NULL)
      return(COLLPARENTNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collParentName,strlen(COLL_PARENT_NAME_KW)+1);
  }
  else if (strstr(t1,COLL_OWNER_NAME_KW) == t1) {
    if (coi->collOwnerName == NULL)
      return(COLLOWNERNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collOwnerName,strlen(COLL_OWNER_NAME_KW)+1);
  }
  else if (strstr(t1,COLL_OWNER_ZONE_KW) == t1) {
    if (coi->collOwnerZone == NULL)
      return(COLLOWNERZONE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collOwnerZone,strlen(COLL_OWNER_ZONE_KW)+1);
  }
  else if (strstr(t1,COLL_EXPIRY_KW) == t1) {
    if (coi->collExpiry == NULL)
      return(COLLEXPIRY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collExpiry,strlen(COLL_EXPIRY_KW)+1);
  }
  else if (strstr(t1,COLL_COMMENTS_KW) == t1) {
    if (coi->collComments == NULL)
      return(COLLCOMMENTS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collComments,strlen(COLL_COMMENTS_KW)+1);
  }
  else if (strstr(t1,COLL_CREATE_KW) == t1) {
    if (coi->collCreate == NULL)
      return(COLLCREATE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collCreate,strlen(COLL_CREATE_KW)+1);
  }
  else if (strstr(t1,COLL_MODIFY_KW) == t1) {
    if (coi->collModify == NULL)
      return(COLLMODIFY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collModify,strlen(COLL_MODIFY_KW)+1);
  }
  else if (strstr(t1,COLL_ACCESS_KW) == t1) {
    if (coi->collAccess == NULL)
      return(COLLACCESS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collAccess,strlen(COLL_ACCESS_KW)+1);
  }
  else if (strstr(t1,COLL_ACCESS_INX_KW) == t1) {
    if (coi->collAccessInx == 0)
      return(COLLACCESSINX_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,coi->collAccessInx,strlen(COLL_ACCESS_INX_KW)+1);
  }
  else if (strstr(t1,COLL_ID_KW) == t1) {
    if (coi->collId == 0)
      return(COLLID_EMPTY_IN_STRUCT_ERR);
#if 0	/* use replaceLongValue because collId is rodsLong_t now. mw */
    i = replaceIntValue(start,size,coi->collId,strlen(COLL_ID_KW)+1);
#endif
    i = replaceLongValue(start,size,coi->collId,strlen(COLL_ID_KW)+1);
  }
  else if (strstr(t1,COLL_MAP_ID_KW) == t1) {
    if (coi->collMapId == 0)
      return(COLLMAPID_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,coi->collMapId,strlen(COLL_MAP_ID_KW)+1);
  }
  else if (strstr(t1,COLL_INHERITANCE_KW) == t1) {
    if (coi->collInheritance == NULL)
      return(COLLINHERITANCE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,coi->collInheritance,strlen(COLL_INHERITANCE_KW)+1);
  }
  
  else
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  return(i);
}



int
replaceRescVar( char *start, int size,  rescInfo_t *roi)
{
  int i;
  char *t1;


  t1 = start + 1;

  if (strstr(t1,RESC_NAME_KW) == t1) {
    if (roi->rescName == NULL)
      return(RESCNAME_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescName,strlen(RESC_NAME_KW)+1);
  }
  else if (strstr(t1,RESC_ZONE_KW) == t1) {
    if (roi->zoneName == NULL)
      return(RESCZONE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->zoneName,strlen(RESC_ZONE_KW)+1);
  }
  else if (strstr(t1,RESC_LOC_KW) == t1) {
    if (roi->rescLoc == NULL)
      return(RESCLOC_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescLoc,strlen(RESC_LOC_KW)+1);
  }
  else if (strstr(t1,RESC_TYPE_KW) == t1) {
    if (roi->rescType == NULL)
      return(RESCTYPE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescType,strlen(RESC_TYPE_KW)+1);
  }
  else if (strstr(t1,RESC_TYPE_INX_KW) == t1) {
    if (roi->rescTypeInx == 0)
      return(RESCTYPEINX_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,roi->rescTypeInx,strlen(RESC_TYPE_INX_KW)+1);
  }
  else if (strstr(t1,RESC_CLASS_KW) == t1) {
    if (roi->rescClass == NULL)
      return(RESCCLASS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescClass,strlen(RESC_CLASS_KW)+1);
  }
  else if (strstr(t1,RESC_CLASS_INX_KW) == t1) {
    if (roi->rescClassInx == 0)
      return(RESCCLASSINX_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,roi->rescClassInx,strlen(RESC_CLASS_INX_KW)+1);
  }
  else if (strstr(t1,RESC_VAULT_PATH_KW) == t1) {
    if (roi->rescVaultPath == NULL)
      return(RESCVAULTPATH_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescVaultPath,strlen(RESC_VAULT_PATH_KW)+1);
  }
  else if (strstr(t1,RESC_STATUS_KW) == t1) {
    i = replaceIntValue(start,size,roi->rescStatus,strlen(RESC_STATUS_KW)+1);
  }
  else if (strstr(t1,PARA_OPR_KW) == t1) {
    if (roi->paraOpr == 0)
      return(PARAOPR_EMPTY_IN_STRUCT_ERR);
    i = replaceIntValue(start,size,roi->paraOpr,strlen(PARA_OPR_KW)+1);
  }
  else if (strstr(t1,RESC_ID_KW) == t1) {
    if (roi->rescId == 0)
      return(RESCID_EMPTY_IN_STRUCT_ERR);
#if 0	/* XXXXX rescId is srbLong_t now */
    i = replaceIntValue(start,size,roi->rescId,strlen(RESC_ID_KW)+1);
#endif
    i = replaceLongValue(start,size,roi->rescId,strlen(RESC_ID_KW)+1);
  }
  else if (strstr(t1,GATEWAY_ADDR_KW) == t1) {
    if (roi->gateWayAddr == NULL)
      return(GATEWAYADDR_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->gateWayAddr,strlen(GATEWAY_ADDR_KW)+1);
  }
  else if (strstr(t1,RESC_MAX_OBJ_SIZE_KW) == t1) {
    if (roi->rescMaxObjSize == 0)
      return(RESCMAX_BJSIZE_EMPTY_IN_STRUCT_ERR);
    i = replaceLongValue(start,size,roi->rescMaxObjSize,strlen(RESC_MAX_OBJ_SIZE_KW)+1);
  }
  else if (strstr(t1,FREE_SPACE_KW) == t1) {
    if (roi->freeSpace == 0)
      return(FREESPACE_EMPTY_IN_STRUCT_ERR);
    i = replaceLongValue(start,size,roi->freeSpace,strlen(FREE_SPACE_KW)+1);
  }
  else if (strstr(t1,FREE_SPACE_TIME_KW) == t1) {
    if (roi->freeSpaceTime == 0)
      return(FREESPACETIME_EMPTY_IN_STRUCT_ERR);
    i = replaceLongValue(start,size,roi->freeSpaceTime,strlen(FREE_SPACE_TIME_KW)+1);
  }
  else if (strstr(t1,FREE_SPACE_TIMESTAMP_KW) == t1) {
    if (roi->freeSpaceTimeStamp == NULL)
      return(FREESPACETIMESTAMP_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->freeSpaceTimeStamp,strlen(FREE_SPACE_TIMESTAMP_KW)+1);
  }
  else if (strstr(t1,RESC_INFO_KW) == t1) {
    if (roi->rescInfo == NULL)
      return(RESCINFO_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescInfo,strlen(RESC_INFO_KW)+1);
  }
  else if (strstr(t1,RESC_COMMENTS_KW) == t1) {
    if (roi->rescComments == NULL)
      return(RESCCOMMENTS_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescComments,strlen(RESC_COMMENTS_KW)+1);
  }
  else if (strstr(t1,RESC_CREATE_KW) == t1) {
    if (roi->rescCreate == NULL)
      return(RESCCREATE_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescCreate,strlen(RESC_CREATE_KW)+1);
  }
  else if (strstr(t1,RESC_MODIFY_KW) == t1) {
    if (roi->rescModify == NULL)
      return(RESCMODIFY_EMPTY_IN_STRUCT_ERR);
    i = replaceStrValue(start,size,roi->rescModify,strlen(RESC_MODIFY_KW)+1); 
  }
  else
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  return(i);
}

int
isNumber(char *s)
{
  int i;

  for (i = 0; i < strlen(s); i++) {
    if (isdigit(s[i]) == 0 && s[i] != '.' && s[i] != '-'  && s[i] != '+')
      return(0);
  }
  return(1);

}

int
isLogical(char *s)
{
  if (!strcmp(s,"&&") || !strcmp(s,"%%"))
    return(1);
  else
    return(0);
}

int
beginOpr(char *e,char **eoper) {

    if (strncmp(e," && ",4) == 0 ||
	strncmp(e," %% ",4) == 0 ||
	strncmp(e," >= ",4) == 0 ||
	strncmp(e," <= ",4) == 0 ||
	strncmp(e," == ",4) == 0 ||
        strncmp(e," ++ ",4) == 0 ||
	strncmp(e," != ",4) == 0 ) {
      *eoper = e + 4;
      return(1);
    }
    if (strncmp(e," < ",3) == 0 ||
	strncmp(e," > ",3) == 0 ||
	strncmp(e," + ",3) == 0 ||
	strncmp(e," - ",3) == 0 ||
	strncmp(e," * ",3) == 0 ||
	strncmp(e," / ",3) == 0 ) {
      *eoper = e + 3;
      return(1);
    }
    if (strncmp(e," not like ",10) == 0 ) {
      *eoper = e + 10;
      return(1);
    }
    if (strncmp(e," like ",6) == 0) {
      *eoper = e + 6;
      return(1);
    }
    *eoper = NULL;
    return(0);
}

int
splitExpression(char *expr, char *expr1, char *expr2, char *oper)
{

  int found;
  char *e, *t, *t1;
  char t2;
  found = 0;
  e = expr;
  while  (found == 0) {
    /*
    if ((t = strstr(e," && ")) != NULL ||
	(t = strstr(e," || ")) != NULL ||
	(t = strstr(e," < ")) != NULL ||
	(t = strstr(e," > ")) != NULL ||
	(t = strstr(e," >= ")) != NULL ||
	(t = strstr(e," <= ")) != NULL ||
	(t = strstr(e," == ")) != NULL ||
	(t = strstr(e," != ")) != NULL ||
	(t = strstr(e," + ")) != NULL ||
	(t = strstr(e," - ")) != NULL ||
	(t = strstr(e," * ")) != NULL ||
	(t = strstr(e," / ")) != NULL ||
	(t = strstr(e," not like ")) != NULL ||
	(t = strstr(e," like ")) != NULL) {

      *t = '\0';
      t1 = t + 1;
      while (*t1 != ' ') t1++;
      t2 = *t1;
      *t1 = '\0';
   */
    t  = e;
    while (beginOpr(t,&t1) == 0) {
      t++;
      if( *t == '\0')
	break;
    }
    if (*t != '\0') {
      *t = '\0';
      t1--;
      t2 = *t1;
      *t1 = '\0';
      if (goodExpr(expr) == 0 && goodExpr(t1+1) == 0) {
	strcpy(expr1,expr);
	strcpy(expr2,t1+1);
	strcpy(oper,t+1);
	*t = ' ';
	*t1 = t2;
	found = 1;
      }
      else {
	*t = ' ';
	*t1 = t2;
	e = t1+1;
      }
    }
    else {
      strcpy(expr1,expr);
      strcpy(expr2,"");
      strcpy(oper,"");
      found = 1;
    }
  }
  trimWS(expr1);
  if (expr1[0] == '(' && expr1[strlen(expr1)-1] == ')') {
    expr1[0] = ' ';
    expr1[strlen(expr1)-1] ='\0';
    if (goodExpr(expr1) != 0) {
      expr1[0] = '(';
      expr1[strlen(expr1)] =')';
    }
    trimWS(expr1);
  }
  trimWS(expr2);
  if (expr2[0] == '(' && expr2[strlen(expr2)-1] == ')') {
    expr2[0] = ' ';
    expr2[strlen(expr2)-1] ='\0';
    if (goodExpr(expr2) != 0) {
      expr2[0] = '(';
      expr2[strlen(expr2)] =')';
    }
    trimWS(expr2);
  }
  trimWS(oper);
  return(0);
}

int
goodExpr(char *expr)
{
  int qcnt = 0;
  int qqcnt = 0;
  int bcnt = 0;
  int i = 0;
  int inq =  0;
  int inqq =  0;
  while(expr[i] != '\0') {
    if (inq) {
      if (expr[i] == '\'') { inq--; qcnt++;}
    }
    else if (inqq) {
      if (expr[i] == '"') { inqq--; qqcnt++;}
    }
    else if (expr[i] == '\'') { inq++; qcnt++; }
    else if (expr[i] == '"') { inqq++; qqcnt++; }
    else if (expr[i] == '(') bcnt++;
    else if (expr[i] == ')') 
      if (bcnt > 0) bcnt--;
    i++;
  }
  if (bcnt != 0 || qcnt % 2 != 0 || qqcnt % 2 != 0 )
    return(-1);
  return(0);

}

/****** OLD
int
goodExpr(char *expr)
{
  int qcnt = 0;
  int qqcnt = 0;
  int bcnt = 0;
  int i = 0;

  while(expr[i] != '\0') {
    if (expr[i] == '\'') qcnt++;
    else if (expr[i] == '"') qqcnt++;
    else if (expr[i] == '(') bcnt++;
    else if (expr[i] == ')') bcnt--;
    i++;
  }
  if (bcnt != 0 || qcnt % 2 != 0 || qqcnt % 2 != 0 )
    return(-1);
  return(0);

}
 OLD *******/
int
replaceStrValue(char *start, int size, char *val, int paramLen) 
{
  int vl,sl;
  
  vl = strlen(val);
  sl = strlen(start);

  if (rmemmove((char*)(start+ vl),(char *) (start+paramLen),
	       sl-paramLen+1,size - vl) == NULL)
    return(SYS_COPY_LEN_ERR);
  if (rmemmove(start,val,vl,vl) == NULL)
    return(SYS_COPY_LEN_ERR);
  return(0);
}
int
replaceIntValue(char *start, int size, int inval, int paramLen) 
{
  char val[MAX_NAME_LEN];

  sprintf(val,"%i",inval);
  return(replaceStrValue(start,size,val,paramLen));
}

int
replaceLongValue(char *start, int size, rodsLong_t inval, int paramLen) 
{
  char val[MAX_NAME_LEN];

  sprintf(val,"%lld",inval);
  return(replaceStrValue(start,size,val,paramLen));
}

int
replaceULongValue(char *start, int size, rodsULong_t inval, int paramLen) 
{
  char val[MAX_NAME_LEN];

  sprintf(val,"%llu",inval);
  return(replaceStrValue(start,size,val,paramLen));
}

int
isAFunction(char *s) 
{
  char *t;
  int i;
  if ((t = strstr(s,"(")) != NULL) {
    *t = '\0';
    i = actionTableLookUp(s);
    *t = '(';
    if (i < 0)
      return(0);
    else
      return(1);
  }
  return(0);

}

int
getNextValueAndBufFromListOrStruct(char *typ, void *inPtr, 
				   void **value,  bytesBuf_t **buf, void **restPtr, int *inx, char **outtyp)
{
  int i,j;

  if (!strcmp(typ,STR_MS_T)) {
    /* Taken as a comma separated string */
    char *s;
    s =  (char *) inPtr;
    while (*s != '\0' && (*s != ',' || (*(s-1) == '\\' && *(s-2) != '\\')) ) s++;
    if (*s != '\0') {
      *s = '\0';
      *value = strdup((char*)inPtr);
      *s = ',';
      *restPtr = s+1;
    }
    else {
      *value = strdup((char*)inPtr);
      *restPtr = NULL;
    }
    /** Raja Added July 22 2010 */
    trimWS((char*)*value);
    return(0);
  }
  else if (!strcmp(typ,StrArray_MS_T)) {
    strArray_t *strA;

    strA = (strArray_t  *) inPtr;
    if (*inx >= strA->len) {
      *restPtr = NULL;
      return(NO_VALUES_FOUND);
    }
    *value = (void *) strdup( (char *) &strA->value[(*inx) * strA->size]);
    (*inx) = (*inx) + 1;
    *restPtr = inPtr;
    return(0);
  }
  else if (!strcmp(typ,IntArray_MS_T)) {
    intArray_t *intA;
    int *ii;
    intA = (intArray_t *) inPtr;
    if (*inx >= intA->len) {
      *restPtr = NULL;
      return(NO_VALUES_FOUND);
    }
    ii = (int*)malloc(sizeof(int));
    *ii = intA->value[(*inx)];
    *value = ii;
    (*inx) = (*inx) + 1;
    *restPtr = inPtr;
    return(0);
  }
  else if (!strcmp(typ,GenQueryOut_MS_T)) {
    /* takes a tuple and gives it as KeyValPair struct... */
    /*  allocates only the first time */
    keyValPair_t *k;
    genQueryOut_t *g;
    char *cname, *aval;
    sqlResult_t *v;
    g = (genQueryOut_t *) inPtr;
    if (g->rowCnt == 0) {
      *restPtr = NULL;
      return(NO_VALUES_FOUND);
    }
    if (*inx >= g->rowCnt) {
      *restPtr = NULL;
      return(NO_VALUES_FOUND);
    }

    if (*inx == 0) { /* first time  */
      k =  (keyValPair_t*)malloc(sizeof(keyValPair_t));
      memset (k, 0, sizeof (keyValPair_t));
      *outtyp = strdup(KeyValPair_MS_T);
      for (i = 0; i < g->attriCnt; i++) {
	v = &g->sqlResult[i];
	cname = (char *) getAttrNameFromAttrId(v->attriInx);
	aval = v->value;
        j  = addKeyVal (k, cname,aval);
	if (j < 0)
	  return(j);
      }
      *value = k;
    }
    else {
      k  = (keyValPair_t*)*value;
      for (i = 0; i < g->attriCnt; i++) {
	v = &g->sqlResult[i];
	/* assumes same ordering :-) */
	aval = &v->value[v->len * (*inx)];
	k->value[i] = (char *) strdup(aval);
      }
    }
    (*inx) = (*inx) + 1;
    *restPtr = inPtr;
    return(0);
  }
  else if (!strcmp(typ,DataObjInp_MS_T)) {
    return(0);
  }
  else
    return(USER_PARAM_TYPE_ERR);
}

int
freeNextValueStruct(void **value,char *typ, char *flag)
{

  int i;

  if (*value == NULL)
    return(0);

  if (!strcmp(typ,STR_MS_T) ||
      !strcmp(typ,StrArray_MS_T) ||
      !strcmp(typ,IntArray_MS_T) ){
    free(*value);
    *value = NULL;
  }
  else if (!strcmp(typ,GenQueryOut_MS_T)) {
    keyValPair_t *k;
    k  = (keyValPair_t*)*value;
    for ( i = 0; i < k->len; i++)
      free(k->value[i]);
    if (!strcmp(flag,"all")) {
      for ( i = 0; i < k->len; i++)
	free(k->keyWord[i]);
      free(*value);
      *value = NULL;
    }
  }
  else
    return(USER_PARAM_TYPE_ERR);
  return(0);
}

int
getNewVarName(char *v, msParamArray_t *msParamArray)
{
  /*  msParam_t *mP;*/

  sprintf(v,"*RNDVAR%i",staticVarNumber);
  staticVarNumber++;

  while  (getMsParamByLabel (msParamArray, v) != NULL) {
    sprintf(v,"*RNDVAR%i",staticVarNumber);
    staticVarNumber++;
  }


  return(0);
}

int
removeTmpVarName(msParamArray_t *msParamArray)
{

  int i;

  for (i = 0; i < msParamArray->len; i++) {
    if (strncmp(msParamArray->msParam[i]->label, "*RNDVAR",7) == 0)
      rmMsParamByLabel (msParamArray,msParamArray->msParam[i]->label, 1);
  }
  return(0);

}

int
isStarVariable(char *str)
{
  char *t;

  if (str[0] != '*')
    return(FALSE);

  t = str+1;
  while (isalnum(*t) ||   *t == '_') t++;
  while(isspace(*t)) t++;
  if (*t != '\0')
    return(FALSE);
  else
    return(TRUE);
}

int
carryOverMsParam(msParamArray_t *sourceMsParamArray,msParamArray_t *targetMsParamArray)
{

  int i;
    msParam_t *mP, *mPt;
    char *a, *b;
    if (sourceMsParamArray == NULL)
      return(0);
    /****
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL) 
	rmMsParamByLabel (targetMsParamArray, mPt->label, 1);
      addMsParam(targetMsParamArray, mPt->label, mPt->type, NULL,NULL);
      j = targetMsParamArray->len - 1;
      free(targetMsParamArray->msParam[j]->label);
      free(targetMsParamArray->msParam[j]->type);
      replMsParam(mPt,targetMsParamArray->msParam[j]);
    }
    ****/
    /****
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL) 
	rmMsParamByLabel (targetMsParamArray, mPt->label, 1);
      addMsParamToArray(targetMsParamArray, 
			mPt->label, mPt->type, mPt->inOutStruct, mPt->inpOutBuf, 1);
    }
    ****/
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL) {
	a = mP->label;
        b = mP->type;
	mP->label = NULL;
	mP->type = NULL;
	/** free(mP->inOutStruct);**/
	free(mP->inpOutBuf);
	replMsParam(mPt,mP);
	free(mP->label);
	mP->label = a;
	free(mP->type);
	mP->type = b;
      }
      else 
	addMsParamToArray(targetMsParamArray, 
			mPt->label, mPt->type, mPt->inOutStruct, mPt->inpOutBuf, 1);
    }
    /****
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL) {
	if (mP->inOutStruct == mP->inOutStruct) {
	  if (mP->type != mPt->type) {
	    free(mP->type );
	    mP->type = mPt->type;
	  }
	  if (mP->label != mPt->label) {
	    free(mP->label);
	    mP->label= mPt->label;
	  }
	}
	else {
	  if (mP->type == mPt->type)
	    a = mP->type;
	  else {
	    free(mP->type );
	    a = NULL;
	  }
	  if (mP->label == mPt->label)
	    b = mP->label;
	  else {
	    free(mP->label );
	    b = NULL;
	  }
	  b = mP->label;
	  free(mP->inOutStruct);
	  free(mPt->inpOutBuf);
	  replMsParam(mPt,mP);
	  if (a != NULL) {
	    free(mP->type);
	    mP->type =  a;
	  }
	  if (b != NULL) {
	    free(mP->label);
	    mP->label =  b;
	  }
	}
      }
      else {
	addMsParamToArray(targetMsParamArray, 
			  mPt->label, mPt->type, mPt->inOutStruct, mPt->inpOutBuf, 1);
      }
    }
    ***/

  return(0);
}

