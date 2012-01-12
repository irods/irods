/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjWrite.h for a description of this API call.*/

#include "dataObjWrite.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "subStructFileWrite.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "subStructFileRead.h"  /* XXXXX can be taken out when structFile api done */
#include "reGlobalsExtern.h"

int
applyRuleForPostProcForWrite(rsComm_t *rsComm, bytesBuf_t *dataObjWriteInpBBuf, char *objPath)
{
    int i;
    ruleExecInfo_t rei2;
    msParamArray_t msParamArray;
    int *myInOutStruct;

    if (ReadWriteRuleState != ON_STATE) return 0;

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    memset ((char*)&msParamArray, 0, sizeof(msParamArray_t));

    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }
    rei2.doi = (dataObjInfo_t*)mallocAndZero(sizeof(dataObjInfo_t));
    strcpy(rei2.doi->objPath,objPath);

#if 0
    addMsParam(&msParamArray, "*WriteBuf", BUF_LEN_MS_T, 
	       (void *) dataObjWriteInpBBuf->len , dataObjWriteInpBBuf);
#else
    bzero (&msParamArray, sizeof (msParamArray));
    myInOutStruct = (int*)malloc (sizeof (int));
    *myInOutStruct = dataObjWriteInpBBuf->len;
    addMsParamToArray (&msParamArray, "*WriteBuf", BUF_LEN_MS_T, myInOutStruct,
      dataObjWriteInpBBuf, 0);
#endif
    i =  applyRule("acPostProcForDataObjWrite(*WriteBuf)",&msParamArray, &rei2, NO_SAVE_REI);
    free (rei2.doi);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "rsDataObjWrite: acPostProcForDataObjWrite error=%d",i);
      clearMsParamArray(&msParamArray,0);
      return i;
    }
    clearMsParamArray(&msParamArray,0);

    return(0);

}

int
rsDataObjWrite (rsComm_t *rsComm, openedDataObjInp_t *dataObjWriteInp, 
bytesBuf_t *dataObjWriteInpBBuf)
{
    int bytesWritten;

    int l1descInx = dataObjWriteInp->l1descInx;

    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
	rodsLog (LOG_NOTICE,
	  "rsDataObjWrite: l1descInx %d out of range",
	  l1descInx);
	return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        /* cross zone operation */
        dataObjWriteInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        bytesWritten = rcDataObjWrite (L1desc[l1descInx].remoteZoneHost->conn,
          dataObjWriteInp, dataObjWriteInpBBuf);
        dataObjWriteInp->l1descInx = l1descInx;
    } else {
	/** RAJA ADDED Dec 1 2010 for pre-post processing rule hooks **/
        int i;
        i = applyRuleForPostProcForWrite(rsComm, dataObjWriteInpBBuf, 
           L1desc[l1descInx].dataObjInfo->objPath);
	if (i < 0)   
	  return(i);  
	dataObjWriteInp->len = dataObjWriteInpBBuf->len;
	/** RAJA ADDED Dec 1 2010 for pre-post processing rule hooks **/
	bytesWritten = l3Write (rsComm, l1descInx, dataObjWriteInp->len,
			      dataObjWriteInpBBuf);
    }

    return (bytesWritten);
}

int
l3Write (rsComm_t *rsComm, int l1descInx, int len,
bytesBuf_t *dataObjWriteInpBBuf)
{
    int rescTypeInx;
    fileWriteInp_t fileWriteInp;
    int bytesWritten;

    dataObjInfo_t *dataObjInfo;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subStructFileFdOprInp_t subStructFileWriteInp;
        memset (&subStructFileWriteInp, 0, sizeof (subStructFileWriteInp));
        subStructFileWriteInp.type = dataObjInfo->specColl->type;
        subStructFileWriteInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileWriteInp.len = len;
        rstrcpy (subStructFileWriteInp.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        bytesWritten = rsSubStructFileWrite (rsComm, &subStructFileWriteInp, 
	  dataObjWriteInpBBuf);
    } else {
        rescTypeInx = L1desc[l1descInx].dataObjInfo->rescInfo->rescTypeInx;

        switch (RescTypeDef[rescTypeInx].rescCat) {
          case FILE_CAT:
	    memset (&fileWriteInp, 0, sizeof (fileWriteInp));
	    fileWriteInp.fileInx = L1desc[l1descInx].l3descInx;
	    fileWriteInp.len = len;
	    bytesWritten = rsFileWrite (rsComm, &fileWriteInp, 
	      dataObjWriteInpBBuf);
	    if (bytesWritten > 0) {
	        L1desc[l1descInx].bytesWritten+=bytesWritten;
	    }
	    break;

          default:
            rodsLog (LOG_NOTICE,
              "l3Write: rescCat type %d is not recognized",
              RescTypeDef[rescTypeInx].rescCat);
            bytesWritten = SYS_INVALID_RESC_TYPE;
            break;
	}
    }
    return (bytesWritten);
}

int
_l3Write (rsComm_t *rsComm, int rescTypeInx, int l3descInx, 
void *buf, int len)
{
    fileWriteInp_t fileWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;
    int bytesWritten;

    dataObjWriteInpBBuf.len = len;
    dataObjWriteInpBBuf.buf = buf;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
	memset (&fileWriteInp, 0, sizeof (fileWriteInp));
	fileWriteInp.fileInx = l3descInx;
	fileWriteInp.len = len;
	bytesWritten = rsFileWrite (rsComm, &fileWriteInp, 
	  &dataObjWriteInpBBuf);
	break;
      default:
        rodsLog (LOG_NOTICE,
          "_l3Write: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        bytesWritten = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (bytesWritten);
}

#ifdef COMPAT_201
int
rsDataObjWrite201 (rsComm_t *rsComm, dataObjWriteInp_t *dataObjWriteInp,
bytesBuf_t *dataObjWriteInpBBuf)
{
    openedDataObjInp_t openedDataObjInp;
    int status;

    bzero (&openedDataObjInp, sizeof (openedDataObjInp));

    openedDataObjInp.l1descInx = dataObjWriteInp->l1descInx;
    openedDataObjInp.len = dataObjWriteInp->len;

    status = rsDataObjWrite (rsComm, &openedDataObjInp, dataObjWriteInpBBuf);

    return status;
}
#endif

