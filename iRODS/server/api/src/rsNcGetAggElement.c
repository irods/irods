/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncGetAggElement.h for a description of this API call.*/

#include "ncGetAggElement.h"
#include "ncInq.h"
#include "rodsLog.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"

int
rsNcGetAggElement (rsComm_t *rsComm, ncOpenInp_t *ncOpenInp,
ncAggElement_t **ncAggElement)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    int status;
    dataObjInp_t dataObjInp;
    int l1descInx;

    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, ncOpenInp->objPath, MAX_NAME_LEN);
    replKeyVal (&ncOpenInp->condInput, &dataObjInp.condInput);
    resolveLinkedPath (rsComm, dataObjInp.objPath, &specCollCache,
      &dataObjInp.condInput);
    remoteFlag = getAndConnRemoteZone (rsComm, &dataObjInp, &rodsServerHost,
      REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
	addKeyVal (&dataObjInp.condInput, NO_OPEN_FLAG_KW, "");
	addKeyVal (&dataObjInp.condInput, NO_STAGING_KW, "");

	l1descInx = _rsDataObjOpen (rsComm, &dataObjInp);
	clearKeyVal (&dataObjInp.condInput);
        if (l1descInx < 0) return l1descInx;
	remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
	  L1desc[l1descInx].dataObjInfo, &rodsServerHost);
	if (remoteFlag < 0) {
            return (remoteFlag);
	} else if (remoteFlag == LOCAL_HOST) {
            status = _rsNcGetAggElement (rsComm, 
              L1desc[l1descInx].dataObjInfo, ncAggElement);
            freeL1desc (l1descInx);
	} else {
            freeL1desc (l1descInx);
	    /* execute it remotely with dataObjInfo->filePath */
	    status = rcNcGetAggElement (rodsServerHost->conn, ncOpenInp, 
              ncAggElement);
	} 
    } else {
        status = rcNcGetAggElement (rodsServerHost->conn, ncOpenInp, 
          ncAggElement);
    }

    return status;
}

int
_rsNcGetAggElement (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, 
ncAggElement_t **ncAggElement)
{
    int status;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    int ncid;
    int i, j;

    if (dataObjInfo == NULL || ncAggElement == NULL) 
      return USER__NULL_INPUT_ERR;
    *ncAggElement = NULL;
#ifdef NETCDF4_API
    status = nc_open (dataObjInfo->filePath, NC_NOWRITE|NC_NETCDF4, &ncid);
#else
    status = nc_open (dataObjInfo->filePath, NC_NOWRITE, &ncid);
#endif
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcGetAggElement: nc_open %s error, status = %d, %s",
          dataObjInfo->filePath, status, nc_strerror(status));
        return NETCDF_OPEN_ERR + status;
    }
    /* do the general inq */
    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;

    status = ncInq (&ncInqInp, &ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_rsNcGetAggElement: ncInq error for %s", dataObjInfo->filePath);
        nc_close (ncid);
        return NETCDF_INQ_ERR + status;
    }
    for (i = 0; i < ncInqOut->ndims; i++) {
        if (strcasecmp (ncInqOut->dim[i].name, "time") == 0) break;
    }
    if (i >= ncInqOut->ndims) {
        /* no match */
        rodsLog (LOG_ERROR,
          "_rsNcGetAggElement: 'time' dim does not exist for %s",
          dataObjInfo->filePath);
        nc_close (ncid);
        return NETCDF_DIM_MISMATCH_ERR;
    }
    for (j = 0; j < ncInqOut->nvars; j++) {
        if (strcmp (ncInqOut->dim[i].name, ncInqOut->var[j].name) == 0) {
            break;
        }
    }

    if (j >= ncInqOut->nvars) {
        /* no match */
        rodsLog (LOG_ERROR,
          "_rsNcGetAggElement: 'time' var does not exist for %s",
          dataObjInfo->filePath);
        nc_close (ncid);
        return NETCDF_DIM_MISMATCH_ERR;
    }
    *ncAggElement = (ncAggElement_t *) calloc (1, sizeof (ncAggElement_t));
    (*ncAggElement)->arraylen = ncInqOut->dim[i].arrayLen;

    (*ncAggElement)->startTime = getNcIntVar (ncid, ncInqOut->var[j].id,
      ncInqOut->var[j].dataType, 0);
    (*ncAggElement)->endTime = getNcIntVar (ncid, ncInqOut->var[j].id,
      ncInqOut->var[j].dataType, ncInqOut->dim[i].arrayLen - 1);
    timeToAsci ((*ncAggElement)->startTime, (*ncAggElement)->astartTime);
    timeToAsci ((*ncAggElement)->endTime, (*ncAggElement)->aendTime);
    rstrcpy ((*ncAggElement)->objPath, dataObjInfo->objPath, MAX_NAME_LEN);

    nc_close (ncid);
    return 0;
}
