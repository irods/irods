/* This file handles the libcf subsetting. */

#include "nccfGetVara.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"

int
rsNccfGetVara (rsComm_t *rsComm,  nccfGetVarInp_t * nccfGetVarInp,
nccfGetVarOut_t ** nccfGetVarOut)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    int l1descInx;
    nccfGetVarInp_t myNccfGetVarInp;
    int status = 0;

    if (getValByKey (&nccfGetVarInp->condInput, NATIVE_NETCDF_CALL_KW) !=
      NULL) {
        status = _rsNccfGetVara (nccfGetVarInp->ncid, nccfGetVarInp,
          nccfGetVarOut);
        return status;
    }
    l1descInx = nccfGetVarInp->ncid;
    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNccfGetVara: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        myNccfGetVarInp = *nccfGetVarInp;
        myNccfGetVarInp.ncid = L1desc[l1descInx].remoteL1descInx;

        /* cross zone operation */
        status = rcNccfGetVara (L1desc[l1descInx].remoteZoneHost->conn,
          &myNccfGetVarInp, nccfGetVarOut);
    } else {
        remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
          L1desc[l1descInx].dataObjInfo, &rodsServerHost);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
            status = _rsNccfGetVara (L1desc[l1descInx].l3descInx,
              nccfGetVarInp, nccfGetVarOut);
            if (status < 0) {
                return status;
            }
        } else {
            /* execute it remotely */
            myNccfGetVarInp = *nccfGetVarInp;
            myNccfGetVarInp.ncid = L1desc[l1descInx].l3descInx;
            addKeyVal (&myNccfGetVarInp.condInput, NATIVE_NETCDF_CALL_KW, "");
            status = rcNccfGetVara (rodsServerHost->conn, &myNccfGetVarInp,
              nccfGetVarOut);
            clearKeyVal (&myNccfGetVarInp.condInput);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "rsNccfGetVara: rcNccfGetVara %d for %s error, status = %d",
                  L1desc[l1descInx].l3descInx,
                  L1desc[l1descInx].dataObjInfo->objPath, status);
                return (status);
            }
        }
    }
    return status;
}

int
_rsNccfGetVara (int ncid,  nccfGetVarInp_t * nccfGetVarInp,
nccfGetVarOut_t ** nccfGetVarOut)
{
    int status;
    nc_type xtypep;
    int nlat = 0;
    int nlon = 0;
    void *data = NULL;
    char *dataType_PI;
    int typeSize;
    int dataLen;

    if (nccfGetVarInp == NULL || nccfGetVarOut == NULL) 
        return USER__NULL_INPUT_ERR;

    *nccfGetVarOut = NULL;
    status = nc_inq_vartype (ncid, nccfGetVarInp->varid, &xtypep);
    if (status != NC_NOERR) return NETCDF_INQ_VARS_ERR + status;

    switch (xtypep) {
      case NC_CHAR:
      case NC_BYTE:
      case NC_UBYTE:
	typeSize = sizeof (char);
	dataType_PI = "charDataArray_PI";
	break;
      case NC_STRING:
	typeSize = sizeof (char *);
	dataType_PI = "strDataArray_PI";
	break;
      case NC_INT:
      case NC_UINT:
      case NC_FLOAT:
	dataType_PI = "intDataArray_PI";
	typeSize = sizeof (int);
	break;
      case NC_SHORT:
      case NC_USHORT:
        dataType_PI = "int16DataArray_PI";
        typeSize = sizeof (short);
        break;
      case NC_INT64:
      case NC_UINT64:
      case NC_DOUBLE:
	typeSize = sizeof (double);
	dataType_PI = "int64DataArray_PI";
	break;
      default:
        rodsLog (LOG_ERROR,
          "_rsNccfGetVara: Unknow dataType %d", xtypep);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    if (nccfGetVarInp->maxOutArrayLen <= 0) {
	dataLen = DEF_OUT_ARRAY_BUF_SIZE;
    } else {
	dataLen = typeSize * nccfGetVarInp->maxOutArrayLen;
	if (dataLen < MIN_OUT_ARRAY_BUF_SIZE) {
	    dataLen = MIN_OUT_ARRAY_BUF_SIZE;
	} else if (dataLen > MAX_OUT_ARRAY_BUF_SIZE) {
            rodsLog (LOG_ERROR,
              "_rsNccfGetVara: dataLen %d larger than MAX_OUT_ARRAY_BUF_SIZE", 
	      dataLen);
	    return NETCDF_VARS_DATA_TOO_BIG;
	}
    }
    *nccfGetVarOut = (nccfGetVarOut_t *) calloc (1, sizeof (nccfGetVarOut_t));
    (*nccfGetVarOut)->dataArray = (dataArray_t *) calloc
      (1, sizeof (dataArray_t));

    (*nccfGetVarOut)->dataArray->buf = data = calloc (1, dataLen);

    status = nccf_get_vara (ncid, nccfGetVarInp->varid,
      nccfGetVarInp->latRange, &nlat, nccfGetVarInp->lonRange, &nlon,
      nccfGetVarInp->lvlIndex,  nccfGetVarInp->timestep, data);

    if (status == NC_NOERR) {
	(*nccfGetVarOut)->dataArray->len = nlat * nlon;
	/* sanity check. It's too late */
	if ((*nccfGetVarOut)->dataArray->len * typeSize > dataLen) {
            rodsLog (LOG_ERROR,
              "_rsNccfGetVara:  nccf_get_vara outlen %d > alloc len %d.",
	      (*nccfGetVarOut)->dataArray->len, dataLen);
            freeNccfGetVarOut (nccfGetVarOut);
	    return NETCDF_VARS_DATA_TOO_BIG;
	}
	(*nccfGetVarOut)->nlat = nlat;
	(*nccfGetVarOut)->nlon = nlon;
	rstrcpy ((*nccfGetVarOut)->dataType_PI, dataType_PI, NAME_LEN);
        (*nccfGetVarOut)->dataArray->type = xtypep;
    } else {
        freeNccfGetVarOut (nccfGetVarOut);
        rodsLog (LOG_ERROR,
          "_rsNccfGetVara:  nccf_get_vara err varid %d dataType %d. %s ",
          nccfGetVarInp->varid, xtypep, nc_strerror(status));
        status = NETCDF_GET_VARS_ERR - status;
    }
    return status;
}

