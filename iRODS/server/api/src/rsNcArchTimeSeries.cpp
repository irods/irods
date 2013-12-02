/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncArchTimeSeries.h for a description of this API call.*/

#include "ncArchTimeSeries.h"
#include "rodsLog.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"
#include "miscServerFunct.h"
#include "dataObjCreate.h"
#include "ncGetAggInfo.h"
#include "ncClose.h"
#include "ncInq.h"
#include "regDataObj.h"


int
rsNcArchTimeSeries (rsComm_t *rsComm,
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp) 
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int status;
    dataObjInp_t dataObjInp;
    specCollCache_t *specCollCache = NULL;

    if (getValByKey (&ncArchTimeSeriesInp->condInput, NATIVE_NETCDF_CALL_KW) 
      != NULL) {
        /* must to be called internally */
        if (rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        status = _rsNcArchTimeSeries (rsComm, ncArchTimeSeriesInp);
    } else {
        resolveLinkedPath (rsComm, ncArchTimeSeriesInp->objPath, 
          &specCollCache, &ncArchTimeSeriesInp->condInput);
        bzero (&dataObjInp, sizeof (dataObjInp));
        rstrcpy (dataObjInp.objPath, ncArchTimeSeriesInp->objPath, 
          MAX_NAME_LEN);
        replKeyVal (&ncArchTimeSeriesInp->condInput, &dataObjInp.condInput);
        remoteFlag = getAndConnRemoteZone (rsComm, &dataObjInp, &rodsServerHost,
          REMOTE_CREATE);
        clearKeyVal (&dataObjInp.condInput);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
            status = _rsNcArchTimeSeries (rsComm, ncArchTimeSeriesInp);
        } else {
            if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0)
                return status;
            status = rcNcArchTimeSeries (rodsServerHost->conn, 
              ncArchTimeSeriesInp);
        }
    }
    return status;
}

int
_rsNcArchTimeSeries (rsComm_t *rsComm,
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp) 
{
    int status;
    int dimInx, varInx;
    char *tmpStr;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    dataObjInp_t dataObjInp;
    unsigned int endTime;
    ncOpenInp_t ncOpenInp;
    ncCloseInp_t ncCloseInp;
    int *ncid = NULL;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    ncAggInfo_t *ncAggInfo = NULL;
    rodsLong_t startTimeInx, endTimeInx;
    rodsLong_t fileSizeLimit;

    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, ncArchTimeSeriesInp->aggCollection,
      MAX_NAME_LEN);
    replKeyVal (&ncArchTimeSeriesInp->condInput, &dataObjInp.condInput);
    
    status = getRescGrpForCreate (rsComm, &dataObjInp, &myRescGrpInfo);
    clearKeyVal (&dataObjInp.condInput);
    
    /* pick the first local host */
    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (isLocalHost (tmpRescInfo->rescLoc)) break; 
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    if (tmpRescGrpInfo == NULL) {
        /* we don't have a local resource */
        if (getValByKey (&ncArchTimeSeriesInp->condInput, NATIVE_NETCDF_CALL_KW)
          != NULL) {
            rodsLog (LOG_ERROR,
              "_rsNcArchTimeSeries: No local resc for NATIVE_NETCDF_CALL of %s",
              ncArchTimeSeriesInp->objPath);
            freeRescGrpInfo (myRescGrpInfo);
            return SYS_INVALID_RESC_INPUT;
        } else {
            remoteFlag = resolveHostByRescInfo (myRescGrpInfo->rescInfo,
              &rodsServerHost);
            if (remoteFlag < 0) {
                freeRescGrpInfo (myRescGrpInfo);
                return (remoteFlag);
            } else if (remoteFlag == REMOTE_HOST) {
                freeRescGrpInfo (myRescGrpInfo);
                addKeyVal (&ncArchTimeSeriesInp->condInput, 
                  NATIVE_NETCDF_CALL_KW, "");
                if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
                    return status;
                }
                status = rcNcArchTimeSeries (rodsServerHost->conn,
                  ncArchTimeSeriesInp);
                return status;
            }
        }
    }
    /* get here when tmpRescGrpInfo != NULL. Will do it locally */ 

    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, ncArchTimeSeriesInp->objPath, MAX_NAME_LEN);
#ifdef NETCDF4_API
    ncOpenInp.mode = NC_NOWRITE|NC_NETCDF4;
#else
    ncOpenInp.mode = NC_NOWRITE;
#endif
    addKeyVal (&ncOpenInp.condInput, NO_STAGING_KW, "");

    status = rsNcOpen (rsComm, &ncOpenInp, &ncid);
    clearKeyVal (&ncOpenInp.condInput);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_rsNcArchTimeSeries: rsNcOpen error for %s", ncOpenInp.objPath);
        freeRescGrpInfo (myRescGrpInfo);
        return status;
    }
    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = *ncid;
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = *ncid;
    free (ncid);
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;
    status = rsNcInq (rsComm, &ncInqInp, &ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_rsNcArchTimeSeries: rcNcInq error for %s", ncOpenInp.objPath);
        rsNcClose (rsComm, &ncCloseInp);
        freeRescGrpInfo (myRescGrpInfo);
        return status;
    }
    for (dimInx = 0; dimInx < ncInqOut->ndims; dimInx++) {
        if (strcasecmp (ncInqOut->dim[dimInx].name, "time") == 0) break;
    }
    if (dimInx >= ncInqOut->ndims) {
        /* no match */
        rodsLog (LOG_ERROR,
          "_rsNcArchTimeSeries: 'time' dim does not exist for %s",
          ncOpenInp.objPath);
        rsNcClose (rsComm, &ncCloseInp);
        freeRescGrpInfo (myRescGrpInfo);
        freeNcInqOut (&ncInqOut);
        return NETCDF_DIM_MISMATCH_ERR;
    }
    for (varInx = 0; varInx < ncInqOut->nvars; varInx++) {
        if (strcmp (ncInqOut->dim[dimInx].name, ncInqOut->var[varInx].name) 
          == 0) {
            break;
        }
    }
    if (varInx >= ncInqOut->nvars) {
        /* no match */
        rodsLog (LOG_ERROR,
          "_rsNcArchTimeSeries: 'time' var does not exist for %s",
          ncOpenInp.objPath);
        rsNcClose (rsComm, &ncCloseInp);
        freeRescGrpInfo (myRescGrpInfo);
        freeNcInqOut (&ncInqOut);
        return NETCDF_DIM_MISMATCH_ERR;
    }

    if (ncInqOut->var[varInx].nvdims != 1) {
        rodsLog (LOG_ERROR,
          "_rsNcArchTimeSeries: 'time' .nvdims = %d is not 1 for %s",
          ncInqOut->var[varInx].nvdims, ncOpenInp.objPath);
        rsNcClose (rsComm, &ncCloseInp);
        freeRescGrpInfo (myRescGrpInfo);
        freeNcInqOut (&ncInqOut);
        return NETCDF_DIM_MISMATCH_ERR;
    }

    if ((tmpStr = getValByKey (&ncArchTimeSeriesInp->condInput, 
      NEW_NETCDF_ARCH_KW)) != NULL) {
        /* this is a new archive */
        startTimeInx = strtoll (tmpStr, 0, 0);
    } else {
        status = readAggInfo (rsComm, ncArchTimeSeriesInp->aggCollection,
          NULL, &ncAggInfo);
        if (status < 0) {
            rsNcClose (rsComm, &ncCloseInp);
            freeRescGrpInfo (myRescGrpInfo);
            freeNcInqOut (&ncInqOut);
            return status;
        }
        endTime = ncAggInfo->ncAggElement[ncAggInfo->numFiles - 1].endTime;

        status = getTimeInxForArch (rsComm, ncInqInp.ncid, ncInqOut, dimInx, 
          varInx, endTime, &startTimeInx);
        if (status < 0) {
            rsNcClose (rsComm, &ncCloseInp);
            freeRescGrpInfo (myRescGrpInfo);
            freeNcInqOut (&ncInqOut);
            freeAggInfo (&ncAggInfo);
            return status;
        }
    }
    endTimeInx = ncInqOut->dim[dimInx].arrayLen - 1;

    if (ncArchTimeSeriesInp->fileSizeLimit > 0) {
        fileSizeLimit = ncArchTimeSeriesInp->fileSizeLimit * ONE_MILLION;
    } else {
        fileSizeLimit = ARCH_FILE_SIZE;
    }
    status = archPartialTimeSeries (rsComm, ncInqOut, ncAggInfo, 
      L1desc[ncInqInp.ncid].l3descInx, varInx, 
      ncArchTimeSeriesInp->aggCollection, tmpRescGrpInfo, 
      startTimeInx, endTimeInx, fileSizeLimit);

    rsNcClose (rsComm, &ncCloseInp);
    freeRescGrpInfo (myRescGrpInfo);
    freeNcInqOut (&ncInqOut);
    freeAggInfo (&ncAggInfo);

    if (status >= 0) {
        /* update agginfo */
        rstrcpy (ncOpenInp.objPath,  ncArchTimeSeriesInp->aggCollection, 
          MAX_NAME_LEN);
        ncOpenInp.mode = NC_WRITE;
        status = rsNcGetAggInfo (rsComm, &ncOpenInp, &ncAggInfo);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "_rsNcArchTimeSeries: rsNcGetAggInfo error for %s", 
            ncOpenInp.objPath);
        } else {
            freeAggInfo (&ncAggInfo);
        }
    }
    return status;
}

int
archPartialTimeSeries (rsComm_t *rsComm, ncInqOut_t *ncInqOut,
ncAggInfo_t *ncAggInfo, int srcNcid, int timeVarInx, char *aggCollection, 
rescGrpInfo_t *myRescGrpInfo, rodsLong_t startTimeInx, rodsLong_t endTimeInx,
rodsLong_t archFileSize)
{
    dataObjInp_t dataObjInp;
    int status, l1descInx;
    rodsLong_t curTimeInx = startTimeInx;
    ncVarSubset_t ncVarSubset;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *myDataObjInfo;
    int nextNumber;
    char basePath[MAX_NAME_LEN];
    int inxInterval;
    rodsLong_t timeStepSize;

    timeStepSize = getTimeStepSize (ncInqOut);
    if (timeStepSize < 0) {
        status = timeStepSize;
        rodsLogError (LOG_ERROR, status,
          "archPartialTimeSeries: getTimeStepSize error for %s",
          aggCollection);
        return status;
    }
    inxInterval = archFileSize/timeStepSize + 1;
    bzero (&dataObjInp, sizeof (dataObjInp));
    bzero (&ncVarSubset, sizeof (ncVarSubset));
    ncVarSubset.numVar = 0;
    ncVarSubset.numSubset = 1;
    rstrcpy (ncVarSubset.ncSubset[0].subsetVarName, 
      ncInqOut->var[timeVarInx].name, NAME_LEN);
    ncVarSubset.ncSubset[0].stride = 1;
    addKeyVal (&dataObjInp.condInput, NO_OPEN_FLAG_KW, "");
    if (ncAggInfo == NULL) {
        nextNumber = 0;
        status = getAggBasePath (aggCollection, basePath);
        if (status < 0) return status;
    } else {
        nextNumber = getNextAggEleObjPath (ncAggInfo, aggCollection, basePath);
        if (nextNumber < 0) return nextNumber;
    }
    while (curTimeInx < endTimeInx) {
        rodsLong_t remainingInx;
        snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s%-d", basePath, 
          nextNumber);
        nextNumber++;
        l1descInx = _rsDataObjCreateWithRescInfo (rsComm, &dataObjInp,
          myRescGrpInfo->rescInfo, myRescGrpInfo->rescGroupName);
        if (l1descInx < 0) {
            return l1descInx;
        }
        myDataObjInfo = L1desc[l1descInx].dataObjInfo;
        rstrcpy (myDataObjInfo->dataType, "netcdf", NAME_LEN);
        memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
        dataObjCloseInp.l1descInx = l1descInx;
        ncVarSubset.ncSubset[0].start = curTimeInx;
        /* if it is close enough, just do all of it */
        remainingInx = endTimeInx - curTimeInx + 1;
        if ((inxInterval + inxInterval/2 + 1) >= remainingInx)
          inxInterval = remainingInx;
        if (curTimeInx + inxInterval > endTimeInx) {
            ncVarSubset.ncSubset[0].end = endTimeInx;
        } else {
           ncVarSubset.ncSubset[0].end = curTimeInx + inxInterval - 1;
        }
        curTimeInx = ncVarSubset.ncSubset[0].end + 1;

        mkDirForFilePath (UNIX_FILE_TYPE, rsComm, "/", 
          myDataObjInfo->filePath, getDefDirMode ());
        status = dumpSubsetToFile (NULL, srcNcid, 0, ncInqOut, &ncVarSubset,
          L1desc[l1descInx].dataObjInfo->filePath);
        if (status >= 0) {
            L1desc[l1descInx].bytesWritten = 1;
        } else {
            rodsLogError (LOG_ERROR, status,
              "archPartialTimeSeries: rsRegDataObj for %s failed, status = %d",
              myDataObjInfo->objPath, status);
            L1desc[l1descInx].oprStatus = status;
            rsDataObjClose (rsComm, &dataObjCloseInp);
            return (status);
        }
        status = svrRegDataObj (rsComm, myDataObjInfo);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "archPartialTimeSeries: rsRegDataObj for %s failed, status = %d",
              myDataObjInfo->objPath, status);
            L1desc[l1descInx].oprStatus = status;
            rsDataObjClose (rsComm, &dataObjCloseInp);
            return (status);
        } else {
            myDataObjInfo->replNum = status;
        }
        L1desc[l1descInx].oprStatus = status;
        rsDataObjClose (rsComm, &dataObjCloseInp);
    }

    return status;
}

int
getTimeInxForArch (rsComm_t *rsComm, int ncid, ncInqOut_t *ncInqOut,
int dimInx, int varInx, unsigned int prevEndTime, rodsLong_t *startTimeInx)
{
    rodsLong_t start[1], count[1], stride[1];
    rodsLong_t timeArrayLen, timeArrayRemain, readCount;
    ncGetVarInp_t ncGetVarInp;
    ncGetVarOut_t *ncGetVarOut = NULL;
    void *bufPtr;
    int i, status;
    unsigned int myTime;


    /* read backward, READ_TIME_SIZE at a time until it is <= prevEndTime */
    timeArrayLen = timeArrayRemain = ncInqOut->dim[dimInx].arrayLen;
    if (timeArrayRemain <= READ_TIME_SIZE) {
        readCount = timeArrayRemain;
    } else {
        readCount = READ_TIME_SIZE;
    }
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = ncInqOut->var[varInx].dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid =  ncInqOut->var[varInx].id;
    ncGetVarInp.ndim =  ncInqOut->var[varInx].nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    while (timeArrayRemain > 0) {
        int goodInx = -1;
        timeArrayRemain -= readCount;
        start[0] = timeArrayRemain;
        count[0] = readCount;
        stride[0] = 1;

        status = rsNcGetVarsByType (rsComm, &ncGetVarInp, &ncGetVarOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "dumpNcInqOut: rcNcGetVarsByType error for %s",
              ncInqOut->var[varInx].name);
              return status;
        }
        bufPtr = ncGetVarOut->dataArray->buf;
        
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            myTime = ncValueToInt (ncGetVarOut->dataArray->type, &bufPtr);
            if (myTime < 0) {
                /* XXXX close and clear */
                return myTime;
            }
            if (myTime > prevEndTime) break;
            goodInx = i;
        }
        if (goodInx >= 0) {
            *startTimeInx = timeArrayRemain + goodInx + 1;
            return 0;
        }
        if (timeArrayRemain <= READ_TIME_SIZE) {
            readCount = timeArrayRemain;
        } else {
            readCount = READ_TIME_SIZE;
        }
    }
    *startTimeInx = 0;
    return NETCDF_DIM_MISMATCH_ERR;
}

