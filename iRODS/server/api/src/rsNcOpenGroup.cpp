/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncOpenGroup.h"
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
rsNcOpenGroup (rsComm_t *rsComm, ncOpenInp_t *ncOpenGroupInp, int **ncid)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int status;
    dataObjInfo_t *dataObjInfo = NULL;
    int rl1descInx, l1descInx, myncid;
    int rootNcid;

    if (getValByKey (&ncOpenGroupInp->condInput, NATIVE_NETCDF_CALL_KW) != NULL) {
	/* just all nc_open with objPath as file nc file path */
	/* must to be called internally */
        if (rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
	}
        status = nc_inq_grp_full_ncid (ncOpenGroupInp->rootNcid, 
          ncOpenGroupInp->objPath, &myncid);
        if (status == NC_NOERR) {
            *ncid = (int *) malloc (sizeof (int));
            *(*ncid) = myncid;
	    return 0;
	} else {
            rodsLog (LOG_ERROR,
              "rsNcOpenGroup: nc_open %s error, status = %d, %s",
              ncOpenGroupInp->objPath, status, nc_strerror(status));
            return (NETCDF_OPEN_ERR + status);
        } 
    }
    rl1descInx = ncOpenGroupInp->rootNcid;
    if (rl1descInx < 2 || rl1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNcClose: rl1descInx %d out of range",
          rl1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[rl1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[rl1descInx].remoteZoneHost != NULL) {
       ncOpenInp_t myNcOpenGroupInp;
        bzero (&myNcOpenGroupInp, sizeof (myNcOpenGroupInp));
        rstrcpy (myNcOpenGroupInp.objPath,
          ncOpenGroupInp->objPath, MAX_NAME_LEN);
        myNcOpenGroupInp.rootNcid = L1desc[rl1descInx].remoteL1descInx;

        status = rcNcOpenGroup (L1desc[rl1descInx].remoteZoneHost->conn, 
          &myNcOpenGroupInp, &myncid);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "rsNcOpenGroup: _rcNcOpenGroup %s error, status = %d",
              ncOpenGroupInp->objPath, status);
            return (status);
        }
        l1descInx = allocAndSetL1descForZoneOpr (myncid, 
          L1desc[rl1descInx].dataObjInp,
          L1desc[rl1descInx].remoteZoneHost, NULL);
    } else {
        /* local zone */
        rootNcid = L1desc[rl1descInx].l3descInx;
        remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
          L1desc[rl1descInx].dataObjInfo, &rodsServerHost);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
            status = nc_inq_grp_full_ncid (rootNcid, 
              ncOpenGroupInp->objPath, &myncid);
	    if (status != NC_NOERR) {
		rodsLog (LOG_ERROR,
		  "rsNcOpenGroup: nc_inq_grp_full_ncid %s err, stat = %d, %s",
		  ncOpenGroupInp->objPath, status, nc_strerror(status));
		return (NETCDF_OPEN_ERR + status);
	    }
	} else {
	    /* execute it remotely */
	    ncOpenInp_t myNcOpenGroupInp;
	    bzero (&myNcOpenGroupInp, sizeof (myNcOpenGroupInp));
	    rstrcpy (myNcOpenGroupInp.objPath, 
	      ncOpenGroupInp->objPath, MAX_NAME_LEN);
            myNcOpenGroupInp.rootNcid = rootNcid;
	    addKeyVal (&myNcOpenGroupInp.condInput, NATIVE_NETCDF_CALL_KW, "");
	    status = rcNcOpenGroup (rodsServerHost->conn, &myNcOpenGroupInp, 
              &myncid);
	    clearKeyVal (&myNcOpenGroupInp.condInput);
	    if (status < 0) {
		rodsLog (LOG_ERROR,
                  "rsNcOpenGroup: rcNcOpenGroup %s error, status = %d",
                  myNcOpenGroupInp.objPath, status);
                return (status);
            }
	} 
	l1descInx = allocL1desc ();
	L1desc[l1descInx].dataObjInfo = dataObjInfo = 
           (dataObjInfo_t *) calloc (1, sizeof (dataObjInfo_t));
	rstrcpy (dataObjInfo->objPath, 
	      ncOpenGroupInp->objPath, MAX_NAME_LEN);
	dataObjInfo->rescInfo = L1desc[rl1descInx].dataObjInfo->rescInfo;
	L1desc[l1descInx].l3descInx = myncid;
    }
    L1desc[l1descInx].oprType = NC_OPEN_GROUP;
    *ncid = (int *) malloc (sizeof (int));
    *(*ncid) = l1descInx;

    return 0;
}

