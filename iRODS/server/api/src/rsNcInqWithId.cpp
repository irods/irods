/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncInqWithId.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"

int
rsNcInqWithId (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut)
{
    int l1descInx;
    ncInqIdInp_t myNcInqWithIdInp;
    int status = 0;

    if (getValByKey (&ncInqWithIdInp->condInput, NATIVE_NETCDF_CALL_KW) != 
      NULL) {
	/* just do nc_inq_YYYY */
	status = _rsNcInqWithId (ncInqWithIdInp->paramType, 
	  ncInqWithIdInp->ncid, ncInqWithIdInp->myid, ncInqWithIdInp->name,
	  ncInqWithIdOut);
        return status;
    }
    l1descInx = ncInqWithIdInp->ncid;
    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNcInqWithId: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
	bzero (&myNcInqWithIdInp, sizeof (myNcInqWithIdInp));
        myNcInqWithIdInp.paramType = ncInqWithIdInp->paramType;
        myNcInqWithIdInp.myid = ncInqWithIdInp->myid;
	myNcInqWithIdInp.ncid = L1desc[l1descInx].remoteL1descInx;
        rstrcpy (myNcInqWithIdInp.name, ncInqWithIdInp->name, MAX_NAME_LEN);

        /* cross zone operation */
	status = rcNcInqWithId (L1desc[l1descInx].remoteZoneHost->conn,
	  &myNcInqWithIdInp, ncInqWithIdOut);
    } else {
        if (L1desc[l1descInx].openedAggInfo.ncAggInfo != NULL) {
            status = rsNcInqWithIdColl (rsComm, ncInqWithIdInp, ncInqWithIdOut);
        } else {
            status = rsNcInqWithIdDataObj (rsComm, ncInqWithIdInp, 
              ncInqWithIdOut);
        }
    }
    return status;
}

int
_rsNcInqWithId (int paramType, int ncid, int myid, char *name,
ncInqWithIdOut_t **ncInqWithIdOut)
{
    int status;
    char myname[MAX_NAME_LEN];
    size_t mylong = 0;
    int mytype = 0;
    int mynatts = 0;
    int myndim = 0;
    int intArray[NC_MAX_VAR_DIMS];

    myname[0] = '\0';
    if (name == NULL || ncInqWithIdOut == NULL) return USER__NULL_INPUT_ERR;

    switch (paramType) {
      case NC_DIM_T:
        status = nc_inq_dim (ncid, myid, myname, &mylong);
	break;
      case NC_VAR_T:
	status = nc_inq_var (ncid, myid, myname, &mytype, &myndim, intArray,
	  &mynatts);
	break;
      default:
        rodsLog (LOG_ERROR,
          "_rsNcInqWithId: Unknow paramType %d for %s ", paramType, myname);
        return (NETCDF_INVALID_PARAM_TYPE);
    }

    if (status == NC_NOERR) {
	*ncInqWithIdOut = (ncInqWithIdOut_t *) calloc (1, sizeof 
	  (ncInqWithIdOut_t)); 
	(*ncInqWithIdOut)->mylong = mylong;
	(*ncInqWithIdOut)->dataType = mytype;
	(*ncInqWithIdOut)->natts = mynatts;
        if (myndim > 0) {
	    int len = sizeof (int) * myndim;
	    (*ncInqWithIdOut)->ndim = myndim;
	    (*ncInqWithIdOut)->intArray = (int *) calloc (1, len);
	    memcpy ((*ncInqWithIdOut)->intArray, intArray, len);
	}
	rstrcpy ((*ncInqWithIdOut)->name, myname, MAX_NAME_LEN);
    } else {
        rodsLog (LOG_ERROR,
          "_rsNcInqWithId: nc_inq error paramType %d for %s. %s ", 
	  paramType, name, nc_strerror(status));
	*ncInqWithIdOut = NULL;
        status = NETCDF_INQ_ID_ERR + status;
    }
    return status;
}

int
rsNcInqWithIdDataObj (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    int l1descInx;
    ncInqIdInp_t myNcInqWithIdInp;
    int status = 0;

    l1descInx = ncInqWithIdInp->ncid;
    remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
      L1desc[l1descInx].dataObjInfo, &rodsServerHost);
    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
        status = _rsNcInqWithId (ncInqWithIdInp->paramType,
          L1desc[l1descInx].l3descInx,  ncInqWithIdInp->myid,
          ncInqWithIdInp->name, ncInqWithIdOut);
        if (status < 0) {
            return status;
        }
    } else {
        /* execute it remotely */
        bzero (&myNcInqWithIdInp, sizeof (myNcInqWithIdInp));
        myNcInqWithIdInp.paramType = ncInqWithIdInp->paramType;
        myNcInqWithIdInp.ncid = L1desc[l1descInx].l3descInx;
        myNcInqWithIdInp.myid = ncInqWithIdInp->myid;
        rstrcpy (myNcInqWithIdInp.name, ncInqWithIdInp->name, MAX_NAME_LEN);
        addKeyVal (&myNcInqWithIdInp.condInput, NATIVE_NETCDF_CALL_KW, "");
        status = rcNcInqWithId (rodsServerHost->conn, &myNcInqWithIdInp,
          ncInqWithIdOut);
        clearKeyVal (&myNcInqWithIdInp.condInput);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "rsNcInqWithIdDataObj: rcNcInqWithId %d for %s error, status=%d",
              L1desc[l1descInx].l3descInx,
              L1desc[l1descInx].dataObjInfo->objPath, status);
            return (status);
        }
    }
    return status;
}

int
rsNcInqWithIdColl (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut)
{
    int status;
    int l1descInx;
    ncInqIdInp_t myNcInqWithIdInp;

    l1descInx = ncInqWithIdInp->ncid;
    /* always use element 0 file aggr collection */
    if (L1desc[l1descInx].openedAggInfo.objNcid0 == -1) {
        return NETCDF_AGG_ELE_FILE_NOT_OPENED;
    }
    myNcInqWithIdInp = *ncInqWithIdInp;
    myNcInqWithIdInp.ncid = L1desc[l1descInx].openedAggInfo.objNcid0;
    bzero (&myNcInqWithIdInp.condInput, sizeof (keyValPair_t));
    status = rsNcInqWithIdDataObj (rsComm, &myNcInqWithIdInp, ncInqWithIdOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rsNcInqWithIdColl: rsNcInqWithIdDataObj error for l1descInx %d", 
          l1descInx);
    }
    return status;
}

