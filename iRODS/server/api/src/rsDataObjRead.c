/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjRead.h for a description of this API call.*/

#include "dataObjRead.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "subStructFileRead.h"  /* XXXXX can be taken out when structFile api done */
#include "reGlobalsExtern.h"

int
applyRuleForPostProcForRead(rsComm_t *rsComm, bytesBuf_t *dataObjReadOutBBuf, char *objPath)
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
    addMsParam(&msParamArray, "*ReadBuf", BUF_LEN_MS_T, 
	       (void *) dataObjReadOutBBuf->len , dataObjReadOutBBuf);
#else
    bzero (&msParamArray, sizeof (msParamArray));
    myInOutStruct = (int*)malloc (sizeof (int));
    *myInOutStruct = dataObjReadOutBBuf->len;
    addMsParamToArray (&msParamArray, "*ReadBuf", BUF_LEN_MS_T, myInOutStruct,
      dataObjReadOutBBuf, 0);
#endif
    i =  applyRule("acPostProcForDataObjRead(*ReadBuf)",&msParamArray, &rei2, 
      NO_SAVE_REI);
    free (rei2.doi);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "rsDataObjRead: acPostProcForDataObjRead error=%d",i);
      clearMsParamArray(&msParamArray,0);
      return i;
    }
    clearMsParamArray(&msParamArray,0);

    return(0);

}

int
rsDataObjRead (rsComm_t *rsComm, openedDataObjInp_t *dataObjReadInp, 
bytesBuf_t *dataObjReadOutBBuf)
{
    int bytesRead;
    int l1descInx = dataObjReadInp->l1descInx;

    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_NOTICE,
          "rsDataObjRead: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        /* cross zone operation */
        dataObjReadInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        bytesRead = rcDataObjRead (L1desc[l1descInx].remoteZoneHost->conn,
          dataObjReadInp, dataObjReadOutBBuf);
	dataObjReadInp->l1descInx = l1descInx;
    } else {
        int i;
        bytesRead = l3Read (rsComm, l1descInx, dataObjReadInp->len,
          dataObjReadOutBBuf);
	/** RAJA ADDED Dec 1 2010 for pre-post processing rule hooks **/
	i = applyRuleForPostProcForRead(rsComm, dataObjReadOutBBuf, 
          L1desc[l1descInx].dataObjInfo->objPath);
	if (i < 0)
	  return(i);  
#if 0	/* XXXXX This is used for msi changing the the return buffer but causes
         * problem for normal read because len is the size of the buffer. 
	 * it out for now */
	bytesRead = dataObjReadOutBBuf->len;
#endif
	/** RAJA ADDED Dec 1 2010 for pre-post processing rule hooks **/
    }

    return (bytesRead);
}

int
l3Read (rsComm_t *rsComm, int l1descInx, int len,
bytesBuf_t *dataObjReadOutBBuf)
{
    int rescTypeInx;
    int bytesRead;

    dataObjInfo_t *dataObjInfo;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
	subStructFileFdOprInp_t subStructFileReadInp;
        memset (&subStructFileReadInp, 0, sizeof (subStructFileReadInp));
        subStructFileReadInp.type = dataObjInfo->specColl->type;
        subStructFileReadInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileReadInp.len = len;
        rstrcpy (subStructFileReadInp.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        bytesRead = rsSubStructFileRead (rsComm, &subStructFileReadInp, dataObjReadOutBBuf);
    } else {
        fileReadInp_t fileReadInp;

        rescTypeInx = L1desc[l1descInx].dataObjInfo->rescInfo->rescTypeInx;

        switch (RescTypeDef[rescTypeInx].rescCat) {
          case FILE_CAT:
            memset (&fileReadInp, 0, sizeof (fileReadInp));
            fileReadInp.fileInx = L1desc[l1descInx].l3descInx;
            fileReadInp.len = len;
            bytesRead = rsFileRead (rsComm, &fileReadInp, dataObjReadOutBBuf);
            break;

          default:
            rodsLog (LOG_NOTICE,
              "l3Read: rescCat type %d is not recognized",
              RescTypeDef[rescTypeInx].rescCat);
            bytesRead = SYS_INVALID_RESC_TYPE;
            break;
	}
    }
    return (bytesRead);
}

int
_l3Read (rsComm_t *rsComm, int rescTypeInx, int l3descInx,
void *buf, int len)
{
    fileReadInp_t fileReadInp;
    bytesBuf_t dataObjReadInpBBuf;
    int bytesRead;

    dataObjReadInpBBuf.len = len;
    dataObjReadInpBBuf.buf = buf;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
        memset (&fileReadInp, 0, sizeof (fileReadInp));
        fileReadInp.fileInx = l3descInx;
        fileReadInp.len = len;
        bytesRead = rsFileRead (rsComm, &fileReadInp,
          &dataObjReadInpBBuf);
        break;
      default:
        rodsLog (LOG_NOTICE,
          "_l3Read: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        bytesRead = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (bytesRead);
}

#ifdef COMPAT_201
int
rsDataObjRead201 (rsComm_t *rsComm, dataObjReadInp_t *dataObjReadInp,
bytesBuf_t *dataObjReadOutBBuf)
{
    openedDataObjInp_t openedDataObjInp;
    int status;

    bzero (&openedDataObjInp, sizeof (openedDataObjInp));

    openedDataObjInp.l1descInx = dataObjReadInp->l1descInx;
    openedDataObjInp.len = dataObjReadInp->len;

    status = rsDataObjRead (rsComm, &openedDataObjInp, dataObjReadOutBBuf);

    return status;
}
#endif
