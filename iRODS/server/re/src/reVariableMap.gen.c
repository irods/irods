
/* this is automatically generated C code */

#include "reVariableMap.gen.h"

#include "reVariableMap.h"



int getValFromRescInfo(char *varMap, rescInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, RescInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "rescName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescId") == 0) {
		
			    i = getLongLeafValue(varValue, rei->rescId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "zoneName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->zoneName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescLoc") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescLoc, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescType") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescTypeInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->rescTypeInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescClassInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->rescClassInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescStatus") == 0) {
		
			    i = getIntLeafValue(varValue, rei->rescStatus, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "paraOpr") == 0) {
		
			    i = getIntLeafValue(varValue, rei->paraOpr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescClass") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescClass, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescVaultPath") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescVaultPath, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescInfo") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescInfo, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescComments") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescComments, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "gateWayAddr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->gateWayAddr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescMaxObjSize") == 0) {
		
			    i = getLongLeafValue(varValue, rei->rescMaxObjSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "freeSpace") == 0) {
		
			    i = getLongLeafValue(varValue, rei->freeSpace, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "freeSpaceTimeStamp") == 0) {
		
			    i = getStrLeafValue(varValue, rei->freeSpaceTimeStamp, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "freeSpaceTime") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "rescCreate") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescCreate, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescModify") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescModify, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rodsServerHost") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "quotaLimit") == 0) {
		
			    i = getLongLeafValue(varValue, rei->quotaLimit, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "quotaOverrun") == 0) {
		
			    i = getLongLeafValue(varValue, rei->quotaOverrun, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromRescInfo(char *varMap, rescInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rescInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "rescName") == 0) {
	  	
			    i = setStrLeafValue(rei->rescName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescId") == 0) {
	  	
			    i = setLongLeafValue(&(rei->rescId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "zoneName") == 0) {
	  	
			    i = setStrLeafValue(rei->zoneName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescLoc") == 0) {
	  	
			    i = setStrLeafValue(rei->rescLoc, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescType") == 0) {
	  	
			    i = setStrLeafValue(rei->rescType, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescTypeInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->rescTypeInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescClassInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->rescClassInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescStatus") == 0) {
	  	
			    i = setIntLeafValue(&(rei->rescStatus), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "paraOpr") == 0) {
	  	
			    i = setIntLeafValue(&(rei->paraOpr), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescClass") == 0) {
	  	
			    i = setStrLeafValue(rei->rescClass, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescVaultPath") == 0) {
	  	
			    i = setStrLeafValue(rei->rescVaultPath, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescInfo") == 0) {
	  	
			    i = setStrLeafValue(rei->rescInfo, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescComments") == 0) {
	  	
			    i = setStrLeafValue(rei->rescComments, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "gateWayAddr") == 0) {
	  	
			    i = setStrLeafValue(rei->gateWayAddr, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescMaxObjSize") == 0) {
	  	
			    i = setLongLeafValue(&(rei->rescMaxObjSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "freeSpace") == 0) {
	  	
			    i = setLongLeafValue(&(rei->freeSpace), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "freeSpaceTimeStamp") == 0) {
	  	
			    i = setStrLeafValue(rei->freeSpaceTimeStamp, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "freeSpaceTime") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "rescCreate") == 0) {
	  	
			    i = setStrLeafValue(rei->rescCreate, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescModify") == 0) {
	  	
			    i = setStrLeafValue(rei->rescModify, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rodsServerHost") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "quotaLimit") == 0) {
	  	
			    i = setLongLeafValue(&(rei->quotaLimit), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "quotaOverrun") == 0) {
	  	
			    i = setLongLeafValue(&(rei->quotaOverrun), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromRescInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(RescInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "rescName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescId") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "zoneName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescLoc") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescType") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescTypeInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescClassInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescStatus") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "paraOpr") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescClass") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescVaultPath") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescInfo") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescComments") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "gateWayAddr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescMaxObjSize") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "freeSpace") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "freeSpaceTimeStamp") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "freeSpaceTime") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescCreate") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescModify") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rodsServerHost") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "quotaLimit") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "quotaOverrun") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromRescGrpInfo(char *varMap, rescGrpInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, RescGrpInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "rescGroupName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescGroupName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescInfo") == 0) {
		
			    i = getValFromRescInfo(varMapCPtr, rei->rescInfo, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "status") == 0) {
		
			    i = getIntLeafValue(varValue, rei->status, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dummy") == 0) {
		
			    i = getIntLeafValue(varValue, rei->dummy, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "cacheNext") == 0) {
		
			    i = getValFromRescGrpInfo(varMapCPtr, rei->cacheNext, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "next") == 0) {
		
			    i = getValFromRescGrpInfo(varMapCPtr, rei->next, varValue, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromRescGrpInfo(char *varMap, rescGrpInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rescGrpInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "rescGroupName") == 0) {
	  	
			    i = setStrLeafValue(rei->rescGroupName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescInfo") == 0) {
	  	
			    i = setValFromRescInfo(varMapCPtr, &(rei->rescInfo), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "status") == 0) {
	  	
			    i = setIntLeafValue(&(rei->status), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dummy") == 0) {
	  	
			    i = setIntLeafValue(&(rei->dummy), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "cacheNext") == 0) {
	  	
			    i = setValFromRescGrpInfo(varMapCPtr, &(rei->cacheNext), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "next") == 0) {
	  	
			    i = setValFromRescGrpInfo(varMapCPtr, &(rei->next), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromRescGrpInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(RescGrpInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "rescGroupName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescInfo") == 0) {
		
			    return getVarTypeFromRescInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "status") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dummy") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "cacheNext") == 0) {
		
			    return getVarTypeFromRescGrpInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "next") == 0) {
		
			    return getVarTypeFromRescGrpInfo(varMapCPtr, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromKeyValPair(char *varMap, keyValPair_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, KeyValPair_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "len") == 0) {
		
			    i = getIntLeafValue(varValue, rei->len, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "keyWord") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "value") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromKeyValPair(char *varMap, keyValPair_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  keyValPair_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "len") == 0) {
	  	
			    i = setIntLeafValue(&(rei->len), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "keyWord") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "value") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromKeyValPair(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(KeyValPair_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "len") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "keyWord") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "value") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromDataObjInfo(char *varMap, dataObjInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, DataObjInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "objPath") == 0) {
		
			    i = getStrLeafValue(varValue, rei->objPath, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescGroupName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rescGroupName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataType") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataSize") == 0) {
		
			    i = getLongLeafValue(varValue, rei->dataSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "chksum") == 0) {
		
			    i = getStrLeafValue(varValue, rei->chksum, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "version") == 0) {
		
			    i = getStrLeafValue(varValue, rei->version, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "filePath") == 0) {
		
			    i = getStrLeafValue(varValue, rei->filePath, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rescInfo") == 0) {
		
			    i = getValFromRescInfo(varMapCPtr, rei->rescInfo, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataOwnerName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataOwnerName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataOwnerZone") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataOwnerZone, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "replNum") == 0) {
		
			    i = getIntLeafValue(varValue, rei->replNum, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "replStatus") == 0) {
		
			    i = getIntLeafValue(varValue, rei->replStatus, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "statusString") == 0) {
		
			    i = getStrLeafValue(varValue, rei->statusString, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataId") == 0) {
		
			    i = getLongLeafValue(varValue, rei->dataId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collId") == 0) {
		
			    i = getLongLeafValue(varValue, rei->collId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataMapId") == 0) {
		
			    i = getIntLeafValue(varValue, rei->dataMapId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "flags") == 0) {
		
			    i = getIntLeafValue(varValue, rei->flags, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataComments") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataComments, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataMode") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataMode, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataExpiry") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataExpiry, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataCreate") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataCreate, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataModify") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataModify, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataAccess") == 0) {
		
			    i = getStrLeafValue(varValue, rei->dataAccess, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataAccessInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->dataAccessInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "writeFlag") == 0) {
		
			    i = getIntLeafValue(varValue, rei->writeFlag, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "destRescName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->destRescName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "backupRescName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->backupRescName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "subPath") == 0) {
		
			    i = getStrLeafValue(varValue, rei->subPath, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "specColl") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "regUid") == 0) {
		
			    i = getIntLeafValue(varValue, rei->regUid, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "otherFlags") == 0) {
		
			    i = getIntLeafValue(varValue, rei->otherFlags, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "condInput") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "next") == 0) {
		
			    i = getValFromDataObjInfo(varMapCPtr, rei->next, varValue, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromDataObjInfo(char *varMap, dataObjInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataObjInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "objPath") == 0) {
	  	
			    i = setStrLeafValue(rei->objPath, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescName") == 0) {
	  	
			    i = setStrLeafValue(rei->rescName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescGroupName") == 0) {
	  	
			    i = setStrLeafValue(rei->rescGroupName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataType") == 0) {
	  	
			    i = setStrLeafValue(rei->dataType, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataSize") == 0) {
	  	
			    i = setLongLeafValue(&(rei->dataSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "chksum") == 0) {
	  	
			    i = setStrLeafValue(rei->chksum, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "version") == 0) {
	  	
			    i = setStrLeafValue(rei->version, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "filePath") == 0) {
	  	
			    i = setStrLeafValue(rei->filePath, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rescInfo") == 0) {
	  	
			    i = setValFromRescInfo(varMapCPtr, &(rei->rescInfo), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataOwnerName") == 0) {
	  	
			    i = setStrLeafValue(rei->dataOwnerName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataOwnerZone") == 0) {
	  	
			    i = setStrLeafValue(rei->dataOwnerZone, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "replNum") == 0) {
	  	
			    i = setIntLeafValue(&(rei->replNum), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "replStatus") == 0) {
	  	
			    i = setIntLeafValue(&(rei->replStatus), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "statusString") == 0) {
	  	
			    i = setStrLeafValue(rei->statusString, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataId") == 0) {
	  	
			    i = setLongLeafValue(&(rei->dataId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collId") == 0) {
	  	
			    i = setLongLeafValue(&(rei->collId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataMapId") == 0) {
	  	
			    i = setIntLeafValue(&(rei->dataMapId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "flags") == 0) {
	  	
			    i = setIntLeafValue(&(rei->flags), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataComments") == 0) {
	  	
			    i = setStrLeafValue(rei->dataComments, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataMode") == 0) {
	  	
			    i = setStrLeafValue(rei->dataMode, SHORT_STR_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataExpiry") == 0) {
	  	
			    i = setStrLeafValue(rei->dataExpiry, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataCreate") == 0) {
	  	
			    i = setStrLeafValue(rei->dataCreate, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataModify") == 0) {
	  	
			    i = setStrLeafValue(rei->dataModify, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataAccess") == 0) {
	  	
			    i = setStrLeafValue(rei->dataAccess, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataAccessInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->dataAccessInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "writeFlag") == 0) {
	  	
			    i = setIntLeafValue(&(rei->writeFlag), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "destRescName") == 0) {
	  	
			    i = setStrLeafValue(rei->destRescName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "backupRescName") == 0) {
	  	
			    i = setStrLeafValue(rei->backupRescName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "subPath") == 0) {
	  	
			    i = setStrLeafValue(rei->subPath, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "specColl") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "regUid") == 0) {
	  	
			    i = setIntLeafValue(&(rei->regUid), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "otherFlags") == 0) {
	  	
			    i = setIntLeafValue(&(rei->otherFlags), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "condInput") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "next") == 0) {
	  	
			    i = setValFromDataObjInfo(varMapCPtr, &(rei->next), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromDataObjInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(DataObjInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "objPath") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescGroupName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataType") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataSize") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "chksum") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "version") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "filePath") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rescInfo") == 0) {
		
			    return getVarTypeFromRescInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataOwnerName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataOwnerZone") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "replNum") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "replStatus") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "statusString") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataId") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collId") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataMapId") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "flags") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataComments") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataMode") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataExpiry") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataCreate") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataModify") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataAccess") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataAccessInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "writeFlag") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "destRescName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "backupRescName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "subPath") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "specColl") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "regUid") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "otherFlags") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInput") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "next") == 0) {
		
			    return getVarTypeFromDataObjInfo(varMapCPtr, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromCollInfo(char *varMap, collInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, CollInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "collId") == 0) {
		
			    i = getLongLeafValue(varValue, rei->collId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collParentName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collParentName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collOwnerName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collOwnerName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collOwnerZone") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collOwnerZone, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collMapId") == 0) {
		
			    i = getIntLeafValue(varValue, rei->collMapId, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collAccessInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->collAccessInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collComments") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collComments, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collInheritance") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collInheritance, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collExpiry") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collExpiry, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collCreate") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collCreate, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collModify") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collModify, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collAccess") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collAccess, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collType") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collInfo1") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collInfo1, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "collInfo2") == 0) {
		
			    i = getStrLeafValue(varValue, rei->collInfo2, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "condInput") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "next") == 0) {
		
			    i = getValFromCollInfo(varMapCPtr, rei->next, varValue, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromCollInfo(char *varMap, collInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  collInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "collId") == 0) {
	  	
			    i = setLongLeafValue(&(rei->collId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collName") == 0) {
	  	
			    i = setStrLeafValue(rei->collName, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collParentName") == 0) {
	  	
			    i = setStrLeafValue(rei->collParentName, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collOwnerName") == 0) {
	  	
			    i = setStrLeafValue(rei->collOwnerName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collOwnerZone") == 0) {
	  	
			    i = setStrLeafValue(rei->collOwnerZone, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collMapId") == 0) {
	  	
			    i = setIntLeafValue(&(rei->collMapId), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collAccessInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->collAccessInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collComments") == 0) {
	  	
			    i = setStrLeafValue(rei->collComments, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collInheritance") == 0) {
	  	
			    i = setStrLeafValue(rei->collInheritance, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collExpiry") == 0) {
	  	
			    i = setStrLeafValue(rei->collExpiry, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collCreate") == 0) {
	  	
			    i = setStrLeafValue(rei->collCreate, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collModify") == 0) {
	  	
			    i = setStrLeafValue(rei->collModify, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collAccess") == 0) {
	  	
			    i = setStrLeafValue(rei->collAccess, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collType") == 0) {
	  	
			    i = setStrLeafValue(rei->collType, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collInfo1") == 0) {
	  	
			    i = setStrLeafValue(rei->collInfo1, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "collInfo2") == 0) {
	  	
			    i = setStrLeafValue(rei->collInfo2, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "condInput") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "next") == 0) {
	  	
			    i = setValFromCollInfo(varMapCPtr, &(rei->next), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromCollInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(CollInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "collId") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collParentName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collOwnerName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collOwnerZone") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collMapId") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collAccessInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collComments") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collInheritance") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collExpiry") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collCreate") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collModify") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collAccess") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collType") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collInfo1") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "collInfo2") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInput") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "next") == 0) {
		
			    return getVarTypeFromCollInfo(varMapCPtr, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromRuleExecInfo(char *varMap, ruleExecInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, RuleExecInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "status") == 0) {
		
			    i = getIntLeafValue(varValue, rei->status, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "statusStr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->statusStr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "ruleName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->ruleName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rsComm") == 0) {
		
			    i = getValFromRsComm(varMapCPtr, rei->rsComm, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "msParamArray") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "inOutMsParamArray") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "l1descInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->l1descInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "doinp") == 0) {
		
			    i = getValFromDataObjInp(varMapCPtr, rei->doinp, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "doi") == 0) {
		
			    i = getValFromDataObjInfo(varMapCPtr, rei->doi, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rgi") == 0) {
		
			    i = getValFromRescGrpInfo(varMapCPtr, rei->rgi, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "uoic") == 0) {
		
			    i = getValFromUserInfo(varMapCPtr, rei->uoic, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "uoip") == 0) {
		
			    i = getValFromUserInfo(varMapCPtr, rei->uoip, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "coi") == 0) {
		
			    i = getValFromCollInfo(varMapCPtr, rei->coi, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "uoio") == 0) {
		
			    i = getValFromUserInfo(varMapCPtr, rei->uoio, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "condInputData") == 0) {
		
			    i = getValFromKeyValPair(varMapCPtr, rei->condInputData, varValue, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "ruleSet") == 0) {
		
			    i = getStrLeafValue(varValue, rei->ruleSet, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "next") == 0) {
		
			    i = getValFromRuleExecInfo(varMapCPtr, rei->next, varValue, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromRuleExecInfo(char *varMap, ruleExecInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  ruleExecInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "status") == 0) {
	  	
			    i = setIntLeafValue(&(rei->status), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "statusStr") == 0) {
	  	
			    i = setStrLeafValue(rei->statusStr, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "ruleName") == 0) {
	  	
			    i = setStrLeafValue(rei->ruleName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rsComm") == 0) {
	  	
			    i = setValFromRsComm(varMapCPtr, &(rei->rsComm), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "msParamArray") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "inOutMsParamArray") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "l1descInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->l1descInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "doinp") == 0) {
	  	
			    i = setValFromDataObjInp(varMapCPtr, &(rei->doinp), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "doi") == 0) {
	  	
			    i = setValFromDataObjInfo(varMapCPtr, &(rei->doi), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rgi") == 0) {
	  	
			    i = setValFromRescGrpInfo(varMapCPtr, &(rei->rgi), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "uoic") == 0) {
	  	
			    i = setValFromUserInfo(varMapCPtr, &(rei->uoic), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "uoip") == 0) {
	  	
			    i = setValFromUserInfo(varMapCPtr, &(rei->uoip), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "coi") == 0) {
	  	
			    i = setValFromCollInfo(varMapCPtr, &(rei->coi), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "uoio") == 0) {
	  	
			    i = setValFromUserInfo(varMapCPtr, &(rei->uoio), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "condInputData") == 0) {
	  	
			    i = setValFromKeyValPair(varMapCPtr, &(rei->condInputData), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "ruleSet") == 0) {
	  	
			    i = setStrLeafValue(rei->ruleSet, RULE_SET_DEF_LENGTH, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "next") == 0) {
	  	
			    i = setValFromRuleExecInfo(varMapCPtr, &(rei->next), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromRuleExecInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(RuleExecInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "status") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "statusStr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "ruleName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rsComm") == 0) {
		
			    return getVarTypeFromRsComm(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "msParamArray") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "inOutMsParamArray") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "l1descInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "doinp") == 0) {
		
			    return getVarTypeFromDataObjInp(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "doi") == 0) {
		
			    return getVarTypeFromDataObjInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rgi") == 0) {
		
			    return getVarTypeFromRescGrpInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "uoic") == 0) {
		
			    return getVarTypeFromUserInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "uoip") == 0) {
		
			    return getVarTypeFromUserInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "coi") == 0) {
		
			    return getVarTypeFromCollInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "uoio") == 0) {
		
			    return getVarTypeFromUserInfo(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInputData") == 0) {
		
			    return getVarTypeFromKeyValPair(varMapCPtr, r);
			
	  }
	
	  	
	  if (strcmp(varName, "ruleSet") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "next") == 0) {
		
			    return getVarTypeFromRuleExecInfo(varMapCPtr, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromRsComm(char *varMap, rsComm_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, RsComm_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "irodsProt") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "sock") == 0) {
		
			    i = getIntLeafValue(varValue, rei->sock, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "connectCnt") == 0) {
		
			    i = getIntLeafValue(varValue, rei->connectCnt, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "localAddr") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "remoteAddr") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "clientAddr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->clientAddr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "proxyUser") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "clientUser") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "myEnv") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "cliVersion") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "option") == 0) {
		
			    i = getStrLeafValue(varValue, rei->option, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "procLogFlag") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "rError") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "portalOpr") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "apiInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->apiInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "status") == 0) {
		
			    i = getIntLeafValue(varValue, rei->status, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "perfStat") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "windowSize") == 0) {
		
			    i = getIntLeafValue(varValue, rei->windowSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnFlag") == 0) {
		
			    i = getIntLeafValue(varValue, rei->reconnFlag, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnSock") == 0) {
		
			    i = getIntLeafValue(varValue, rei->reconnSock, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnPort") == 0) {
		
			    i = getIntLeafValue(varValue, rei->reconnPort, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnectedSock") == 0) {
		
			    i = getIntLeafValue(varValue, rei->reconnectedSock, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnAddr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->reconnAddr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "cookie") == 0) {
		
			    i = getIntLeafValue(varValue, rei->cookie, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnThr") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "lock") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "cond") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "agentState") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "clientState") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnThrState") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "gsiRequest") == 0) {
		
			    i = getIntLeafValue(varValue, rei->gsiRequest, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromRsComm(char *varMap, rsComm_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rsComm_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "irodsProt") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "sock") == 0) {
	  	
			    i = setIntLeafValue(&(rei->sock), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "connectCnt") == 0) {
	  	
			    i = setIntLeafValue(&(rei->connectCnt), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "localAddr") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "remoteAddr") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "clientAddr") == 0) {
	  	
			    i = setStrLeafValue(rei->clientAddr, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "proxyUser") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "clientUser") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "myEnv") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "cliVersion") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "option") == 0) {
	  	
			    i = setStrLeafValue(rei->option, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "procLogFlag") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "rError") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "portalOpr") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "apiInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->apiInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "status") == 0) {
	  	
			    i = setIntLeafValue(&(rei->status), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "perfStat") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "windowSize") == 0) {
	  	
			    i = setIntLeafValue(&(rei->windowSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnFlag") == 0) {
	  	
			    i = setIntLeafValue(&(rei->reconnFlag), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnSock") == 0) {
	  	
			    i = setIntLeafValue(&(rei->reconnSock), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnPort") == 0) {
	  	
			    i = setIntLeafValue(&(rei->reconnPort), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnectedSock") == 0) {
	  	
			    i = setIntLeafValue(&(rei->reconnectedSock), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnAddr") == 0) {
	  	
			    i = setStrDupLeafValue(&(rei->reconnAddr), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "cookie") == 0) {
	  	
			    i = setIntLeafValue(&(rei->cookie), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnThr") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "lock") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "cond") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "agentState") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "clientState") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnThrState") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "gsiRequest") == 0) {
	  	
			    i = setIntLeafValue(&(rei->gsiRequest), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromRsComm(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(RsComm_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "irodsProt") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "sock") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "connectCnt") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "localAddr") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "remoteAddr") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "clientAddr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "proxyUser") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "clientUser") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "myEnv") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "cliVersion") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "option") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "procLogFlag") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rError") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "portalOpr") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "apiInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "status") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "perfStat") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "windowSize") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnFlag") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnSock") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnPort") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnectedSock") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnAddr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "cookie") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnThr") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "lock") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "cond") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "agentState") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "clientState") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnThrState") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "gsiRequest") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromDataObjInp(char *varMap, dataObjInp_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, DataObjInp_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "objPath") == 0) {
		
			    i = getStrLeafValue(varValue, rei->objPath, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "createMode") == 0) {
		
			    i = getIntLeafValue(varValue, rei->createMode, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "openFlags") == 0) {
		
			    i = getIntLeafValue(varValue, rei->openFlags, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "offset") == 0) {
		
			    i = getLongLeafValue(varValue, rei->offset, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataSize") == 0) {
		
			    i = getLongLeafValue(varValue, rei->dataSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "numThreads") == 0) {
		
			    i = getIntLeafValue(varValue, rei->numThreads, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "oprType") == 0) {
		
			    i = getIntLeafValue(varValue, rei->oprType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "specColl") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "condInput") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromDataObjInp(char *varMap, dataObjInp_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataObjInp_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "objPath") == 0) {
	  	
			    i = setStrLeafValue(rei->objPath, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "createMode") == 0) {
	  	
			    i = setIntLeafValue(&(rei->createMode), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "openFlags") == 0) {
	  	
			    i = setIntLeafValue(&(rei->openFlags), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "offset") == 0) {
	  	
			    i = setLongLeafValue(&(rei->offset), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataSize") == 0) {
	  	
			    i = setLongLeafValue(&(rei->dataSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "numThreads") == 0) {
	  	
			    i = setIntLeafValue(&(rei->numThreads), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "oprType") == 0) {
	  	
			    i = setIntLeafValue(&(rei->oprType), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "specColl") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "condInput") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromDataObjInp(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(DataObjInp_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "objPath") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "createMode") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "openFlags") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "offset") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataSize") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "numThreads") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "oprType") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "specColl") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInput") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromDataOprInp(char *varMap, dataOprInp_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, DataOprInp_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "oprType") == 0) {
		
			    i = getIntLeafValue(varValue, rei->oprType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "numThreads") == 0) {
		
			    i = getIntLeafValue(varValue, rei->numThreads, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "srcL3descInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->srcL3descInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "destL3descInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->destL3descInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "srcRescTypeInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->srcRescTypeInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "destRescTypeInx") == 0) {
		
			    i = getIntLeafValue(varValue, rei->destRescTypeInx, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "offset") == 0) {
		
			    i = getLongLeafValue(varValue, rei->offset, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataSize") == 0) {
		
			    i = getLongLeafValue(varValue, rei->dataSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "condInput") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromDataOprInp(char *varMap, dataOprInp_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  dataOprInp_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "oprType") == 0) {
	  	
			    i = setIntLeafValue(&(rei->oprType), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "numThreads") == 0) {
	  	
			    i = setIntLeafValue(&(rei->numThreads), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "srcL3descInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->srcL3descInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "destL3descInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->destL3descInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "srcRescTypeInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->srcRescTypeInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "destRescTypeInx") == 0) {
	  	
			    i = setIntLeafValue(&(rei->destRescTypeInx), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "offset") == 0) {
	  	
			    i = setLongLeafValue(&(rei->offset), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataSize") == 0) {
	  	
			    i = setLongLeafValue(&(rei->dataSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "condInput") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromDataOprInp(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(DataOprInp_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "oprType") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "numThreads") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "srcL3descInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "destL3descInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "srcRescTypeInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "destRescTypeInx") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "offset") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataSize") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInput") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromAuthInfo(char *varMap, authInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, AuthInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "authScheme") == 0) {
		
			    i = getStrLeafValue(varValue, rei->authScheme, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "authFlag") == 0) {
		
			    i = getIntLeafValue(varValue, rei->authFlag, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "flag") == 0) {
		
			    i = getIntLeafValue(varValue, rei->flag, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "ppid") == 0) {
		
			    i = getIntLeafValue(varValue, rei->ppid, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "host") == 0) {
		
			    i = getStrLeafValue(varValue, rei->host, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "authStr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->authStr, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromAuthInfo(char *varMap, authInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  authInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "authScheme") == 0) {
	  	
			    i = setStrLeafValue(rei->authScheme, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "authFlag") == 0) {
	  	
			    i = setIntLeafValue(&(rei->authFlag), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "flag") == 0) {
	  	
			    i = setIntLeafValue(&(rei->flag), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "ppid") == 0) {
	  	
			    i = setIntLeafValue(&(rei->ppid), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "host") == 0) {
	  	
			    i = setStrLeafValue(rei->host, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "authStr") == 0) {
	  	
			    i = setStrLeafValue(rei->authStr, NAME_LEN, newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromAuthInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(AuthInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "authScheme") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "authFlag") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "flag") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "ppid") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "host") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "authStr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromUserOtherInfo(char *varMap, userOtherInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, UserOtherInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "userInfo") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userInfo, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "userComments") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userComments, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "userCreate") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userCreate, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "userModify") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userModify, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromUserOtherInfo(char *varMap, userOtherInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  userOtherInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "userInfo") == 0) {
	  	
			    i = setStrLeafValue(rei->userInfo, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "userComments") == 0) {
	  	
			    i = setStrLeafValue(rei->userComments, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "userCreate") == 0) {
	  	
			    i = setStrLeafValue(rei->userCreate, TIME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "userModify") == 0) {
	  	
			    i = setStrLeafValue(rei->userModify, TIME_LEN, newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromUserOtherInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(UserOtherInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "userInfo") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "userComments") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "userCreate") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "userModify") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromUserInfo(char *varMap, userInfo_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, UserInfo_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "userName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "rodsZone") == 0) {
		
			    i = getStrLeafValue(varValue, rei->rodsZone, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "userType") == 0) {
		
			    i = getStrLeafValue(varValue, rei->userType, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "sysUid") == 0) {
		
			    i = getIntLeafValue(varValue, rei->sysUid, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "authInfo") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "userOtherInfo") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromUserInfo(char *varMap, userInfo_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  userInfo_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "userName") == 0) {
	  	
			    i = setStrLeafValue(rei->userName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "rodsZone") == 0) {
	  	
			    i = setStrLeafValue(rei->rodsZone, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "userType") == 0) {
	  	
			    i = setStrLeafValue(rei->userType, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "sysUid") == 0) {
	  	
			    i = setIntLeafValue(&(rei->sysUid), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "authInfo") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "userOtherInfo") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromUserInfo(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(UserInfo_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "userName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "rodsZone") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "userType") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "sysUid") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "authInfo") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "userOtherInfo") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromVersion(char *varMap, version_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, Version_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "status") == 0) {
		
			    i = getIntLeafValue(varValue, rei->status, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "relVersion") == 0) {
		
			    i = getStrLeafValue(varValue, rei->relVersion, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "apiVersion") == 0) {
		
			    i = getStrLeafValue(varValue, rei->apiVersion, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnPort") == 0) {
		
			    i = getIntLeafValue(varValue, rei->reconnPort, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "reconnAddr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->reconnAddr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "cookie") == 0) {
		
			    i = getIntLeafValue(varValue, rei->cookie, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromVersion(char *varMap, version_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  version_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "status") == 0) {
	  	
			    i = setIntLeafValue(&(rei->status), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "relVersion") == 0) {
	  	
			    i = setStrLeafValue(rei->relVersion, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "apiVersion") == 0) {
	  	
			    i = setStrLeafValue(rei->apiVersion, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnPort") == 0) {
	  	
			    i = setIntLeafValue(&(rei->reconnPort), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "reconnAddr") == 0) {
	  	
			    i = setStrLeafValue(rei->reconnAddr, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "cookie") == 0) {
	  	
			    i = setIntLeafValue(&(rei->cookie), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromVersion(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(Version_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "status") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "relVersion") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "apiVersion") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnPort") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "reconnAddr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "cookie") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromRodsHostAddr(char *varMap, rodsHostAddr_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, RodsHostAddr_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "hostAddr") == 0) {
		
			    i = getStrLeafValue(varValue, rei->hostAddr, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "zoneName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->zoneName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "portNum") == 0) {
		
			    i = getIntLeafValue(varValue, rei->portNum, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dummyInt") == 0) {
		
			    i = getIntLeafValue(varValue, rei->dummyInt, r);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromRodsHostAddr(char *varMap, rodsHostAddr_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  rodsHostAddr_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "hostAddr") == 0) {
	  	
			    i = setStrLeafValue(rei->hostAddr, LONG_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "zoneName") == 0) {
	  	
			    i = setStrLeafValue(rei->zoneName, NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "portNum") == 0) {
	  	
			    i = setIntLeafValue(&(rei->portNum), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dummyInt") == 0) {
	  	
			    i = setIntLeafValue(&(rei->dummyInt), newVarValue);
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromRodsHostAddr(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(RodsHostAddr_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "hostAddr") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "zoneName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "portNum") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dummyInt") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}


int getValFromFileOpenInp(char *varMap, fileOpenInp_t *rei, Res **varValue, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  
  if (varMap == NULL) {
      i = getPtrLeafValue(varValue, (void *) rei, NULL, FileOpenInp_MS_T, r);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	

	  if (strcmp(varName, "fileType") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "otherFlags") == 0) {
		
			    i = getIntLeafValue(varValue, rei->otherFlags, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "addr") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

	  if (strcmp(varName, "fileName") == 0) {
		
			    i = getStrLeafValue(varValue, rei->fileName, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "flags") == 0) {
		
			    i = getIntLeafValue(varValue, rei->flags, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "mode") == 0) {
		
			    i = getIntLeafValue(varValue, rei->mode, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "dataSize") == 0) {
		
			    i = getLongLeafValue(varValue, rei->dataSize, r);
			
		return i;
	  }
	

	  if (strcmp(varName, "condInput") == 0) {
		
				i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
int setValFromFileOpenInp(char *varMap, fileOpenInp_t **inrei, Res *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  fileOpenInp_t *rei;

  rei = *inrei;

  if (varMap == NULL) {
      i = setStructPtrLeafValue((void**)inrei, newVarValue);
      return(i);
  }
  if (rei == NULL)
    return(NULL_VALUE_ERR);

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return(i);

	
	  if (strcmp(varName, "fileType") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "otherFlags") == 0) {
	  	
			    i = setIntLeafValue(&(rei->otherFlags), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "addr") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	
	  if (strcmp(varName, "fileName") == 0) {
	  	
			    i = setStrLeafValue(rei->fileName, MAX_NAME_LEN, newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "flags") == 0) {
	  	
			    i = setIntLeafValue(&(rei->flags), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "mode") == 0) {
	  	
			    i = setIntLeafValue(&(rei->mode), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "dataSize") == 0) {
	  	
			    i = setLongLeafValue(&(rei->dataSize), newVarValue);
			
		return i;
	  }
	
	  if (strcmp(varName, "condInput") == 0) {
	  	
			    i = UNDEFINED_VARIABLE_MAP_ERR;
			
		return i;
	  }
	

    return(UNDEFINED_VARIABLE_MAP_ERR);
}
ExprType *getVarTypeFromFileOpenInp(char *varMap, Region *r)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;

  if (varMap == NULL) {
      return newIRODSType(FileOpenInp_MS_T, r);
  }

  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return newErrorType(i, r);

	
	  	
	  if (strcmp(varName, "fileType") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "otherFlags") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "addr") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	
	  	
	  if (strcmp(varName, "fileName") == 0) {
		
			    return newSimpType(T_STRING, r);
			
	  }
	
	  	
	  if (strcmp(varName, "flags") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "mode") == 0) {
		
			    return newSimpType(T_INT, r);
			
	  }
	
	  	
	  if (strcmp(varName, "dataSize") == 0) {
		
			    return newSimpType(T_DOUBLE, r);
			
	  }
	
	  	
	  if (strcmp(varName, "condInput") == 0) {
		
			    return newErrorType( UNDEFINED_VARIABLE_MAP_ERR, r);
			
	  }
	

    return newErrorType(UNDEFINED_VARIABLE_MAP_ERR, r);
}
