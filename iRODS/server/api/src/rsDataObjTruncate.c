/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjTruncate.h for a description of this API call.*/

#include "dataObjTruncate.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "fileTruncate.h"
#include "unregDataObj.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "rmColl.h"
#include "modDataObjMeta.h"
#include "subStructFileTruncate.h"
#include "getRemoteZoneResc.h"
#include "phyBundleColl.h"

int
rsDataObjTruncate (rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    remoteFlag = getAndConnRemoteZone (rsComm, dataObjTruncateInp, 
      &rodsServerHost, REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
        status = rcDataObjTruncate (rodsServerHost->conn, dataObjTruncateInp);
        return status;
    }

    dataObjTruncateInp->openFlags = O_WRONLY;  /* set the permission checking */
    status = getDataObjInfoIncSpecColl (rsComm, dataObjTruncateInp, 
      &dataObjInfoHead);

    if (status < 0) return (status);

    status = _rsDataObjTruncate (rsComm, dataObjTruncateInp, dataObjInfoHead);

    return (status);

}

int
_rsDataObjTruncate (rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp,
dataObjInfo_t *dataObjInfoHead)
{
    int status;
    int retVal = 0;
    dataObjInfo_t *tmpDataObjInfo;

    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
	status = dataObjTruncateS (rsComm, dataObjTruncateInp, tmpDataObjInfo);
	if (status < 0) {
	    if (retVal == 0) {
	        retVal = status;
	    }
	}
        if (dataObjTruncateInp->specColl != NULL) 	/* do only one */
	    break;
	tmpDataObjInfo = tmpDataObjInfo->next;
    }

    freeAllDataObjInfo (dataObjInfoHead);  

    return (retVal);
}

int
dataObjTruncateS (rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp,
dataObjInfo_t *dataObjInfo)
{
    int status;
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    char tmpStr[MAX_NAME_LEN];

    if (dataObjInfo->dataSize == dataObjTruncateInp->dataSize) return 0;

    /* don't do anything for BUNDLE_RESC for now */
    if (strcmp (dataObjInfo->rescInfo->rescName, BUNDLE_RESC) == 0) return 0;

    status = l3Truncate (rsComm, dataObjTruncateInp, dataObjInfo);

    if (status < 0) {
	int myError = getErrno (status);
        rodsLog (LOG_NOTICE,
          "dataObjTruncateS: l3Truncate error for %s. status = %d",
          dataObjTruncateInp->objPath, status);
	/* allow ENOENT to go on and unregister */
	if (myError != ENOENT && myError != EACCES) {
	    return (status);
	}
    }

    if (dataObjInfo->specColl == NULL) {
	/* reigister the new size */

	memset (&regParam, 0, sizeof (regParam));
	memset (&modDataObjMetaInp, 0, sizeof (modDataObjMetaInp));

        snprintf (tmpStr, MAX_NAME_LEN, "%lld", dataObjTruncateInp->dataSize);
        addKeyVal (&regParam, DATA_SIZE_KW, tmpStr);
	addKeyVal (&regParam, CHKSUM_KW, "");

        modDataObjMetaInp.dataObjInfo = dataObjInfo;
        modDataObjMetaInp.regParam = &regParam;
        status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);
	clearKeyVal (&regParam);
        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "dataObjTruncateS: rsModDataObjMeta error for %s. status = %d",
              dataObjTruncateInp->objPath, status);
	}
    }
    return (status);
}

int
l3Truncate (rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp,
dataObjInfo_t *dataObjInfo)
{
    int rescTypeInx;
    fileOpenInp_t fileTruncateInp;
    int status;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy (subFile.subFilePath, dataObjInfo->subPath,
          MAX_NAME_LEN);
        rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        subFile.specColl = dataObjInfo->specColl;
	subFile.offset = dataObjTruncateInp->dataSize;
        status = rsSubStructFileTruncate (rsComm, &subFile);
    } else {
        rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

        switch (RescTypeDef[rescTypeInx].rescCat) {
          case FILE_CAT:
            memset (&fileTruncateInp, 0, sizeof (fileTruncateInp));
            fileTruncateInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
            rstrcpy (fileTruncateInp.fileName, dataObjInfo->filePath, 
	      MAX_NAME_LEN);
            rstrcpy (fileTruncateInp.addr.hostAddr, 
	      dataObjInfo->rescInfo->rescLoc, NAME_LEN);
	    fileTruncateInp.dataSize = dataObjTruncateInp->dataSize;
            status = rsFileTruncate (rsComm, &fileTruncateInp);
            break;

          default:
            rodsLog (LOG_NOTICE,
              "l3Truncate: rescCat type %d is not recognized",
              RescTypeDef[rescTypeInx].rescCat);
            status = SYS_INVALID_RESC_TYPE;
            break;
	}
    }
    return (status);
}

