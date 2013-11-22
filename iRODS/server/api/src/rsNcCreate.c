/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncCreate.h"
#include "rodsLog.h"
#include "dataObjCreate.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "dataObjUnlink.h"
#include "ncClose.h"
#include "physPath.h"
#include "specColl.h"
#include "regDataObj.h"
#include "getRemoteZoneResc.h"

int
rsNcCreate (rsComm_t *rsComm, ncOpenInp_t *ncCreateInp, int **ncid)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    int status;
    dataObjInp_t dataObjInp;
    int l1descInx, myncid;

    if (getValByKey (&ncCreateInp->condInput, NATIVE_NETCDF_CALL_KW) != NULL) {
        /* just do nc_open with objPath as file nc file path */
        /* must to be called internally */
        if (rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        status = nc_create (ncCreateInp->objPath, ncCreateInp->mode, &myncid);
        if (status == NC_NOERR) {
            *ncid = (int *) malloc (sizeof (int));
            *(*ncid) = myncid;
            return 0;
        } else {
            rodsLog (LOG_ERROR,
              "rsNccreate: nc_create %s error, status = %d, %s",
              ncCreateInp->objPath, status, nc_strerror(status));
            return (NETCDF_OPEN_ERR + status);
        }
    }
    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, ncCreateInp->objPath, MAX_NAME_LEN);
    replKeyVal (&ncCreateInp->condInput, &dataObjInp.condInput);
    resolveLinkedPath (rsComm, dataObjInp.objPath, &specCollCache,
      &dataObjInp.condInput);
    remoteFlag = getAndConnRemoteZone (rsComm, &dataObjInp, &rodsServerHost,
      REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
	addKeyVal (&dataObjInp.condInput, NO_OPEN_FLAG_KW, "");
	l1descInx = _rsDataObjCreate (rsComm, &dataObjInp);
	clearKeyVal (&dataObjInp.condInput);
        if (l1descInx < 0) return l1descInx;
	remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
	  L1desc[l1descInx].dataObjInfo, &rodsServerHost);
	if (remoteFlag < 0) {
            return (remoteFlag);
	} else if (remoteFlag == LOCAL_HOST) {
            status = nc_create (L1desc[l1descInx].dataObjInfo->filePath, 
	      ncCreateInp->mode, &myncid);
	    if (status != NC_NOERR) {
		rodsLog (LOG_ERROR,
		  "rsNcCreate: nc_open %s error, status = %d, %s",
		  ncCreateInp->objPath, status, nc_strerror(status));
		freeL1desc (l1descInx);
		return (NETCDF_CREATE_ERR + status);
	    }
	} else {
            /* execute it remotely with dataObjInfo->filePath */
            ncOpenInp_t myNcCreateInp;
            bzero (&myNcCreateInp, sizeof (myNcCreateInp));
            rstrcpy (myNcCreateInp.objPath,
              L1desc[l1descInx].dataObjInfo->filePath, MAX_NAME_LEN);
            addKeyVal (&myNcCreateInp.condInput, NATIVE_NETCDF_CALL_KW, "");
	    status = rcNcCreate (rodsServerHost->conn, &myNcCreateInp, &myncid);
	    clearKeyVal (&myNcCreateInp.condInput);
	    if (status < 0) {
		rodsLog (LOG_ERROR,
                  "rsNcCreate: _rcNcCreate %s error, status = %d",
                  myNcCreateInp.objPath, status);
                freeL1desc (l1descInx);
                return (status);
            }
	} 
        L1desc[l1descInx].l3descInx = myncid;
        /* need to reg here since NO_OPEN_FLAG_KW does not do it */
	if (L1desc[l1descInx].dataObjInfo->specColl == NULL) {
            status = svrRegDataObj (rsComm, L1desc[l1descInx].dataObjInfo);
            if (status < 0) {
		ncCloseInp_t myNcCloseInp;
		bzero (&myNcCloseInp, sizeof (myNcCloseInp));
                myNcCloseInp.ncid = l1descInx;
	        rsNcClose (rsComm, &myNcCloseInp);
                l3Unlink (rsComm, L1desc[l1descInx].dataObjInfo);
                rodsLog (LOG_ERROR,
                  "rsNcCreate: svrRegDataObj for %s failed, status = %d",
                  L1desc[l1descInx].dataObjInfo->objPath, status);
                freeL1desc (l1descInx);
                return (NETCDF_CREATE_ERR + status);
            }
	}
    } else {
	addKeyVal (&dataObjInp.condInput, CROSS_ZONE_CREATE_KW, "");
        status = rcNcCreate (rodsServerHost->conn, ncCreateInp, &myncid);
        /* rm it to avoid confusion */
        rmKeyVal (&dataObjInp.condInput, CROSS_ZONE_CREATE_KW);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "rsNcCreate: _rcNcCreate %s error, status = %d",
              ncCreateInp->objPath, status);
            return (status);
        }
        l1descInx = allocAndSetL1descForZoneOpr (myncid, &dataObjInp,
          rodsServerHost, NULL);
    }
    L1desc[l1descInx].oprType = NC_CREATE;
    *ncid = (int *) malloc (sizeof (int));
    *(*ncid) = l1descInx;

    return 0;
}

