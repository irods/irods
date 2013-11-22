/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncClose.h"
#include "rodsLog.h"
#include "dataObjClose.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"

int
rsNcClose (rsComm_t *rsComm, ncCloseInp_t *ncCloseInp)
{
    int l1descInx;
    ncCloseInp_t myNcCloseInp;
    int status = 0;

    if (getValByKey (&ncCloseInp->condInput, NATIVE_NETCDF_CALL_KW) != NULL) {
	/* just do nc_close */
	status = nc_close (ncCloseInp->ncid);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "rsNcClose: nc_close %d error, status = %d, %s",
              ncCloseInp->ncid, status, nc_strerror(status));
            status = NETCDF_CLOSE_ERR + status;
	}
	return status;
    }
    l1descInx = ncCloseInp->ncid;

    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNcClose: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
	bzero (&myNcCloseInp, sizeof (myNcCloseInp));
	myNcCloseInp.ncid = L1desc[l1descInx].remoteL1descInx;
        /* cross zone operation */
	status = rcNcClose (L1desc[l1descInx].remoteZoneHost->conn,
	  &myNcCloseInp);
	/* the remote zone resc will do the registration */
	freeL1desc (l1descInx);
    } else {
        if (L1desc[l1descInx].oprType == NC_OPEN_GROUP) {
            /* group open. Just free the L1desc */
	    freeL1desc (l1descInx);
	    return 0;
	}
        if (L1desc[l1descInx].openedAggInfo.ncAggInfo != NULL) {
            status = ncCloseColl (rsComm, l1descInx);
        } else {
            status = ncCloseDataObj (rsComm, l1descInx);
        }
    }
    return status;
}

int
ncCloseColl (rsComm_t *rsComm, int l1descInx)
{
    openedDataObjInp_t dataObjCloseInp;
    int status = 0;

    status = closeAggrFiles (rsComm, l1descInx);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
         "ncCloseColl: closeAggrFiles error");
    }
    freeAggInfo (&L1desc[l1descInx].openedAggInfo.ncAggInfo);
    freeNcInqOut (&L1desc[l1descInx].openedAggInfo.ncInqOut0);
    freeNcInqOut (&L1desc[l1descInx].openedAggInfo.ncInqOut);

    bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose (rsComm, &dataObjCloseInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "ncCloseColl: rsDataObjClose %d error, status = %d",
          l1descInx, status);
    }
    return status;
}

int
ncCloseDataObj (rsComm_t *rsComm, int l1descInx)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    ncCloseInp_t myNcCloseInp;
    openedDataObjInp_t dataObjCloseInp;
    int status = 0;

    remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
      L1desc[l1descInx].dataObjInfo, &rodsServerHost);
    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
        status = nc_close (L1desc[l1descInx].l3descInx);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "ncCloseDataObj: nc_close %d for %s error, status = %d, %s",
              L1desc[l1descInx].l3descInx, 
              L1desc[l1descInx].dataObjInfo->objPath, status, 
            nc_strerror(status));
            freeL1desc (l1descInx);
            return (NETCDF_CLOSE_ERR + status);
        }
        L1desc[l1descInx].l3descInx = 0;
    } else {
        /* execute it remotely */
        bzero (&myNcCloseInp, sizeof (myNcCloseInp));
        myNcCloseInp.ncid = L1desc[l1descInx].l3descInx;
        addKeyVal (&myNcCloseInp.condInput, NATIVE_NETCDF_CALL_KW, "");
        status = rcNcClose (rodsServerHost->conn, &myNcCloseInp);
        clearKeyVal (&myNcCloseInp.condInput);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "ncCloseDataObj: rcNcClose %d for %s error, status = %d",
              L1desc[l1descInx].l3descInx,
              L1desc[l1descInx].dataObjInfo->objPath, status);
            freeL1desc (l1descInx);
            return (status);
        }
    }
    bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose (rsComm, &dataObjCloseInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "ncCloseDataObj: rcNcClose %d error, status = %d",
          l1descInx, status);
    }
    return status;
}

int
closeAggrFiles (rsComm_t *rsComm, int l1descInx)
{
    int status;
    openedAggInfo_t *openedAggInfo;
    int savedStatus = 0;

    openedAggInfo = &L1desc[l1descInx].openedAggInfo;
    if (openedAggInfo->aggElemetInx > 0 && openedAggInfo->objNcid >= 0) {
        status = ncCloseDataObj (rsComm, openedAggInfo->objNcid);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "closeAggrFiles: rcNcClose error for objNcid %d", 
              openedAggInfo->objNcid);
            savedStatus = status;
        }
    }
    if (openedAggInfo->objNcid0 >= 0) {
        status = ncCloseDataObj (rsComm,  openedAggInfo->objNcid0);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "closeAggrFiles: rcNcClose error for objNcid0 %d", 
              openedAggInfo->objNcid0);
            savedStatus = status;
        }
    }
    openedAggInfo->aggElemetInx = openedAggInfo->objNcid = 
      openedAggInfo->objNcid0 = -1;
    return savedStatus;
}

