/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"
#include "resource.hpp"
#include "dataObjOpr.hpp"

extern char *rmemmove (void *dest, void *src, int strLen, int maxLen);

static int staticVarNumber = 1;


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

#if 0
int
computeHostAddress(rsComm_t *rsComm, char *inStr, rodsHostAddr_t *addr)
{
  int status;
  dataObjInp_t dataObjInp;
  dataObjInfo_t *dataObjInfoHead = NULL;
  char *path;
  rescGrpInfo_t *rescGrpInfo = NULL;

  if (inStr[0] == '@')
    path = inStr + 1;
  else
    path = inStr;
  if (path[0] == '/') { /* objpath */
    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, path, MAX_NAME_LEN);
    status = getDataObjInfo (rsComm, &dataObjInp, &dataObjInfoHead,
			     ACCESS_READ_OBJECT, 0);
    if (status < 0) {
      rodsLog (LOG_ERROR,
	       "computeHostAddress: getDataObjInfo error for Path %s", path); 
      return (status);
    }

    status = sortObjInfoForOpen (rsComm, &dataObjInfoHead, NULL, 0);
    if (status < 0) {
      rodsLog (LOG_ERROR,
               "computeHostAddress: sortObjInfoForOpen error for Path %s", path);
      return status;
    }
    rstrcpy (addr->zoneName, dataObjInfoHead->rescInfo->zoneName, NAME_LEN);
    rstrcpy (addr->hostAddr, dataObjInfoHead->rescInfo->rescLoc, LONG_NAME_LEN);
    freeAllDataObjInfo (dataObjInfoHead);
  }
  else { /* it is a logical resource (or group) name */
    status = getRescInfo (rsComm, path, NULL, &rescGrpInfo);
    if (status < 0) {
      rodsLog (LOG_ERROR,
               "computeHostAddress: getRescInfo error for Path %s", path);
      return (status);
    }

    status = sortResc(rsComm, &rescGrpInfo, "random");
    if (status < 0) {
      rodsLog (LOG_ERROR,
               "computeHostAddress: sortResc error for Path %s", path);
      return status;
    }
    rstrcpy (addr->zoneName, rescGrpInfo->rescInfo->zoneName, NAME_LEN);
    rstrcpy (addr->hostAddr, rescGrpInfo->rescInfo->rescLoc, LONG_NAME_LEN);
    freeAllRescGrpInfo (rescGrpInfo);
  }
    return(0);
}
#endif
