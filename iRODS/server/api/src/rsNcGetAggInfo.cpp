/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncGetAggInfo.h for a description of this API call.*/

#include "ncGetAggInfo.h"
#include "ncInq.h"
#include "rodsLog.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "openCollection.h"
#include "readCollection.h"
#include "closeCollection.h"
#include "closeCollection.h"
#include "dataObjPut.h"
#include "dataObjGet.h"

int
rsNcGetAggInfo (rsComm_t *rsComm, ncOpenInp_t *ncOpenInp,
ncAggInfo_t **ncAggInfo)
{
    specCollCache_t *specCollCache = NULL;
    collInp_t collInp;
    int handleInx;
    collEnt_t *collEnt;
    int status = 0;
    int status2 = 0;
    int savedStatus = 0;
    ncOpenInp_t childNcOpenInp;
    ncAggElement_t *ncAggElement = NULL;
    bytesBuf_t *packedBBuf = NULL;

    bzero (&collInp, sizeof (collInp));
    rstrcpy (collInp.collName, ncOpenInp->objPath, MAX_NAME_LEN);
    resolveLinkedPath (rsComm, collInp.collName, &specCollCache,
      &ncOpenInp->condInput);
    collInp.flags = VERY_LONG_METADATA_FG; 
    handleInx = rsOpenCollection (rsComm, &collInp);
    if (handleInx < 0) {
        rodsLog (LOG_ERROR,
          "rsNcGetAggInfo: rsOpenCollection of %s error. status = %d",
          collInp.collName, handleInx);
        return (handleInx);
    }
    bzero (&childNcOpenInp, sizeof (childNcOpenInp));
    *ncAggInfo = (ncAggInfo_t *) calloc (1, sizeof (ncAggInfo_t));
    rstrcpy ((*ncAggInfo)->ncObjectName, ncOpenInp->objPath, MAX_NAME_LEN);
    while ((status2 = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
        if (collEnt->objType != DATA_OBJ_T) {
            free (collEnt);
            continue;
        } else if (strcmp (collEnt->dataType, "netcdf") != 0) {
            if (strcmp (collEnt->dataName, NC_AGG_INFO_FILE_NAME) != 0) {
                rodsLog (LOG_NOTICE,
                  "rsNcGetAggInfo: dataType of %s in %s is not 'netcdf' type",
                  collEnt->dataName, collInp.collName);
                savedStatus = NETCDF_INVALID_DATA_TYPE;
            }
            free (collEnt);
            continue;
        } 
        snprintf (childNcOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
          collInp.collName, collEnt->dataName);
        status = rsNcGetAggElement (rsComm, &childNcOpenInp, &ncAggElement);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rsNcGetAggInfo: rsNcGetAggElement of %s error.",
              childNcOpenInp.objPath);
            free (collEnt);
            break;
        } else {
            status = addNcAggElement (ncAggElement, *ncAggInfo);
            free (ncAggElement);
            if (status < 0) {
                free (collEnt);
                break;
            }
        }
        free (collEnt);
    }
    rsCloseCollection (rsComm, &handleInx);
    if (status2 < 0 && status2 != CAT_NO_ROWS_FOUND && status >= 0) {
        status = status2;
    }
    if (status >= 0 && (ncOpenInp->mode & NC_WRITE) != 0) {
        dataObjInp_t dataObjInp;
        portalOprOut_t *portalOprOut = NULL;
        status = packStruct ((void *) *ncAggInfo, &packedBBuf, "NcAggInfo_PI",
          RodsPackTable, 0, XML_PROT);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rsNcGetAggInfo: packStruct error for %s",
              childNcOpenInp.objPath);
          return status;
        }
        /* write it */
        bzero (&dataObjInp, sizeof (dataObjInp));
        replKeyVal (&ncOpenInp->condInput, &dataObjInp.condInput);
        snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
          collInp.collName, NC_AGG_INFO_FILE_NAME);
        dataObjInp.dataSize = packedBBuf->len;
        dataObjInp.oprType = PUT_OPR;
        addKeyVal (&dataObjInp.condInput, DATA_INCLUDED_KW, "");
        addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");
        status = rsDataObjPut (rsComm, &dataObjInp, packedBBuf, &portalOprOut);
        clearBBuf (packedBBuf);
        clearKeyVal (&dataObjInp.condInput);
        if (portalOprOut != NULL) free (portalOprOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rsNcGetAggInfo: rsDataObjPut error for %s", dataObjInp.objPath);
        }
    }
    if (status < 0) {
        return status;
    } else {
        return savedStatus;
    }
}

int
readAggInfo (rsComm_t *rsComm, char *aggColl, keyValPair_t *condInput,
ncAggInfo_t **ncAggInfo)
{
    int status;
    dataObjInp_t dataObjInp;
    bytesBuf_t packedBBuf;
    portalOprOut_t *portalOprOut = NULL;

    bzero (&dataObjInp, sizeof (dataObjInp));
    bzero (&packedBBuf, sizeof (packedBBuf));
    if (condInput != NULL)
        replKeyVal (condInput, &dataObjInp.condInput);
    snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
      aggColl, NC_AGG_INFO_FILE_NAME);
    dataObjInp.oprType = GET_OPR;
    status = rsDataObjGet (rsComm, &dataObjInp, &portalOprOut, &packedBBuf);
    clearKeyVal (&dataObjInp.condInput);
    if (portalOprOut != NULL) free (portalOprOut);
    if (status < 0) {
        if (status == CAT_NO_ROWS_FOUND) status = NETCDF_AGG_INFO_FILE_ERR;
        rodsLogError (LOG_ERROR, status,
          "readAggInfo: rsDataObjGet error for %s", dataObjInp.objPath);
        return status;
    }
    status = unpackStruct (packedBBuf.buf, (void **) ncAggInfo,
      "NcAggInfo_PI", RodsPackTable, XML_PROT);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "readAggInfo: unpackStruct error for %s", dataObjInp.objPath);
        return NETCDF_AGG_INFO_FILE_ERR;
    }
    if (*ncAggInfo == NULL || (*ncAggInfo)->numFiles == 0) {
        rodsLog (LOG_ERROR,
          "readAggInfo: No ncAggInfo for %s", dataObjInp.objPath);
        return NETCDF_AGG_INFO_FILE_ERR;
    }
    return status;
}

