/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncInqGrps.h"
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
rsNcInqGrps (rsComm_t *rsComm, ncInqGrpsInp_t *ncInqGrpsInp,
ncInqGrpsOut_t **ncInqGrpsOut)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int status;
    int l1descInx;
    int ncid;

    if (getValByKey (&ncInqGrpsInp->condInput, NATIVE_NETCDF_CALL_KW) != NULL) {
	/* just all nc_open with objPath as file nc file path */
	/* must to be called internally */
        if (rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
	}
	status = _rsNcInqGrps (ncInqGrpsInp->ncid, ncInqGrpsOut);
	return status;
#if 0
        status = nc_inq_grp_full_ncid (ncInqGrpsInp->ncid, 
          ncInqGrpsInp->objPath, &myncid);
        if (status == NC_NOERR) {
            *ncid = (int *) malloc (sizeof (int));
            *(*ncid) = myncid;
	    return 0;
	} else {
            rodsLog (LOG_ERROR,
              "rsNcInqGrps: nc_open %s error, status = %d, %s",
              ncInqGrpsInp->objPath, status, nc_strerror(status));
            return (NETCDF_OPEN_ERR - status);
        } 
#endif
    }
    l1descInx = ncInqGrpsInp->ncid;
    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNcClose: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
	ncInqGrpsInp_t myNcInqGrpsInp;
        bzero (&myNcInqGrpsInp, sizeof (myNcInqGrpsInp));
        myNcInqGrpsInp.ncid = L1desc[l1descInx].remoteL1descInx;

        status = rcNcInqGrps (L1desc[l1descInx].remoteZoneHost->conn, 
          &myNcInqGrpsInp, ncInqGrpsOut);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "rsNcInqGrps: rcNcInqGrps, status = %d", status);
        }
    } else {
        /* local zone */
        ncid = L1desc[l1descInx].l3descInx;
        remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
          L1desc[l1descInx].dataObjInfo, &rodsServerHost);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
#if 0
            status = nc_inq_grp_full_ncid (ncid, 
              ncInqGrpsInp->objPath, &myncid);
	    if (status != NC_NOERR) {
		rodsLog (LOG_ERROR,
		  "rsNcInqGrps: nc_inq_grp_full_ncid %s err, stat = %d, %s",
		  ncInqGrpsInp->objPath, status, nc_strerror(status));
		return (NETCDF_OPEN_ERR - status);
	    }
#endif
            status = _rsNcInqGrps (ncid, ncInqGrpsOut);
	} else {
	    /* execute it remotely */
	    ncInqGrpsInp_t myNcInqGrpsInp;
	    bzero (&myNcInqGrpsInp, sizeof (myNcInqGrpsInp));
            myNcInqGrpsInp.ncid = ncid;
	    addKeyVal (&myNcInqGrpsInp.condInput, NATIVE_NETCDF_CALL_KW, "");
	    status = rcNcInqGrps (rodsServerHost->conn, &myNcInqGrpsInp, 
              ncInqGrpsOut);
	    clearKeyVal (&myNcInqGrpsInp.condInput);
	    if (status < 0) {
		rodsLog (LOG_ERROR,
                  "rsNcInqGrps: rcNcInqGrps, status = %d", status);
            }
	} 
    }
    return status;
}

int
_rsNcInqGrps (int ncid, ncInqGrpsOut_t **ncInqGrpsOut)
{
    int numgrps = 0;
    ncInqGrpsOut_t *myNInqGrpsOut = NULL;
    int *grpNcid;
    int i, status;

    status = nc_inq_grps (ncid, &numgrps, NULL);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInqGrps: nc_inq_grps error ncid = %d.  %s ", 
          ncid, nc_strerror(status));
        status = NETCDF_INQ_ERR + status;
        return status;
    }
    myNInqGrpsOut = *ncInqGrpsOut = (ncInqGrpsOut_t *) 
      calloc (1, sizeof (ncInqGrpsOut_t));

    if (numgrps <= 0) return 0;

    grpNcid = (int *) calloc (1, numgrps * sizeof (int));

    status = nc_inq_grps (ncid, &numgrps, grpNcid);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInqGrps: nc_inq_grps error.  %s ", nc_strerror(status));
	free (grpNcid);
        status = NETCDF_INQ_ERR + status;
        return status;
    }
    myNInqGrpsOut->grpName = (char **) calloc (1, numgrps * sizeof (char *));
    for (i = 0; i < numgrps; i++) {
	size_t len;
	status = nc_inq_grpname_len (grpNcid[i], &len);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "_rsNcInqGrps: c_inq_grpname_len error.  %s ", 
	      nc_strerror(status));
	    freeNcInqGrpsOut (ncInqGrpsOut);	
	    free (grpNcid);
            status = NETCDF_INQ_ERR + status;
            return status;
	}
	myNInqGrpsOut->grpName[i] = (char *) malloc (len + 1);
        myNInqGrpsOut->ngrps++;
	status = nc_inq_grpname_full (grpNcid[i], &len, 
          myNInqGrpsOut->grpName[i]);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "_rsNcInqGrps: nc_inq_grpname_full error.  %s ",
              nc_strerror(status));
            freeNcInqGrpsOut (ncInqGrpsOut);
            free (grpNcid);
            status = NETCDF_INQ_ERR + status;
            return status;
        }
    }
    free (grpNcid);
    return 0;
}

