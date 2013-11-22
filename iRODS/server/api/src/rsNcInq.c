/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncInq.h for a description of this API call.*/
#include "ncInq.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "getRemoteZoneResc.h"


int
rsNcInq (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int l1descInx;
    int status = 0;

    if (getValByKey (&ncInqInp->condInput, NATIVE_NETCDF_CALL_KW) !=
      NULL) {
        /* just do nc_inq */
        status = _rsNcInq (rsComm, ncInqInp, ncInqOut);
        return status;
    }
    l1descInx = ncInqInp->ncid;
    if (l1descInx < 2 || l1descInx >= NUM_L1_DESC) {
        rodsLog (LOG_ERROR,
          "rsNcInq: l1descInx %d out of range",
          l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE) return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].openedAggInfo.ncAggInfo != NULL) {
        status = rsNcInqColl (rsComm, ncInqInp, ncInqOut);
    } else {
        status = rsNcInqDataObj (rsComm, ncInqInp, ncInqOut);
    }
    return status;
}

int
rsNcInqColl (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int status;
    int l1descInx;
    ncOpenInp_t ncOpenInp;
    ncInqInp_t myNcInqInp;
    int i;

    l1descInx = ncInqInp->ncid;
    /* always use element 0 file aggr collection */
    if (L1desc[l1descInx].openedAggInfo.objNcid0 == -1) {
        return NETCDF_AGG_ELE_FILE_NOT_OPENED;
    }
    myNcInqInp = *ncInqInp;
    myNcInqInp.ncid = L1desc[l1descInx].openedAggInfo.objNcid0;
    bzero (&myNcInqInp.condInput, sizeof (keyValPair_t));
    status = rsNcInqDataObj (rsComm, &myNcInqInp, ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rsNcInqColl: rsNcInqDataObj error for %s", ncOpenInp.objPath);
        return status;
    }
    /* make correction to 'time' dimension */
    for (i = 0; i < (*ncInqOut)->ndims; i++) {
        if (strcasecmp ((*ncInqOut)->dim[i].name, "time") == 0) {
            (*ncInqOut)->dim[i].arrayLen = sumAggElementArraylen (
              L1desc[l1descInx].openedAggInfo.ncAggInfo, 
              L1desc[l1descInx].openedAggInfo.ncAggInfo->numFiles);
            if ((*ncInqOut)->dim[i].arrayLen < 0) {
                status = (*ncInqOut)->dim[i].arrayLen;
                freeNcInqOut (ncInqOut);
            }
            break;
        }
    }
    return status;  
}

int
rsNcInqDataObj (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    int l1descInx;
    ncInqInp_t myNcInqInp;
    int status = 0;

    l1descInx = ncInqInp->ncid;
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        bzero (&myNcInqInp, sizeof (myNcInqInp));
        myNcInqInp.ncid = L1desc[l1descInx].remoteL1descInx;

        /* cross zone operation */
        status = rcNcInq (L1desc[l1descInx].remoteZoneHost->conn,
          &myNcInqInp, ncInqOut);
    } else {
        remoteFlag = resoAndConnHostByDataObjInfo (rsComm,
          L1desc[l1descInx].dataObjInfo, &rodsServerHost);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
            myNcInqInp = *ncInqInp;
            myNcInqInp.ncid = L1desc[l1descInx].l3descInx;
	    bzero (&myNcInqInp.condInput, sizeof (myNcInqInp.condInput));
            status = _rsNcInq (rsComm, &myNcInqInp, ncInqOut);
            if (status < 0) {
                return status;
            }
        } else {
            /* execute it remotely */
	    myNcInqInp = *ncInqInp;
            myNcInqInp.ncid = L1desc[l1descInx].l3descInx;
	    bzero (&myNcInqInp.condInput, sizeof (myNcInqInp.condInput));
            addKeyVal (&myNcInqInp.condInput, NATIVE_NETCDF_CALL_KW, "");
            status = rcNcInq (rodsServerHost->conn, &myNcInqInp,
              ncInqOut);
            clearKeyVal (&myNcInqInp.condInput);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "rsNcInq: rcsNcInq %d for %s error, status = %d",
                  L1desc[l1descInx].l3descInx,
                  L1desc[l1descInx].dataObjInfo->objPath, status);
                return (status);
            }
        }
    }
    return status;
}

int
_rsNcInq (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int status = ncInq (ncInqInp, ncInqOut);
    return status;
}
#if 0	/* moved to the client */
    int ndims, nvars, ngatts, unlimdimid, format;
    int dimType, attType, varType, allFlag;
    int status, i;
    ncGenDimOut_t *dim;
    ncGenVarOut_t *var;
    ncGenAttOut_t *gatt;
    size_t mylong = 0;
    int intArray[NC_MAX_VAR_DIMS];
    int ncid = ncInqInp->ncid;

    *ncInqOut = NULL;

    status = nc_inq (ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInq: nc_inq error.  %s ", nc_strerror(status));
        status = NETCDF_INQ_ERR + status;
        return status;
    }

    if (ncInqInp->paramType == 0) ncInqInp->paramType = NC_ALL_TYPE;
    if ((ncInqInp->paramType & NC_DIM_TYPE) == 0) {
	dimType = ndims = 0;
    } else {
	dimType = 1;
    }
    if ((ncInqInp->paramType & NC_ATT_TYPE) == 0) {
        attType = ngatts = 0;
    } else {
        attType = 1;
    }
    if ((ncInqInp->paramType & NC_VAR_TYPE) == 0) {
        varType = nvars = 0;
    } else {
        varType = 1;
    }

    if ((varType + attType + dimType) > 1) {
	/* inq more than 1 type, ignore name and myid and inq all items of each
	 * type */
	allFlag = NC_ALL_FLAG;
    } else {
	allFlag = ncInqInp->flags & NC_ALL_FLAG;
    }

    if (allFlag == 0) {
	/* indiviudal query. get only a single result */
	if (ndims > 0) ndims = 1;
	else if (ngatts > 0) ngatts = 1;
	else if (nvars > 0) nvars = 1;
    }

    status = nc_inq_format (ncid, &format);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInq: nc_inq_format error.  %s ", nc_strerror(status));
        status = NETCDF_INQ_FORMAT_ERR + status;
        return status;
    }
    initNcInqOut (ndims, nvars, ngatts, unlimdimid, format, ncInqOut);

    /* inq dimension */
    dim = (*ncInqOut)->dim;
    for (i = 0; i < ndims; i++) {
	if (allFlag != 0) {
	    /* inq all dim */
	    dim[i].id = i;
	    status = nc_inq_dim (ncid, i, dim[i].name, &mylong);
	} else {
	    if (*ncInqInp->name != '\0') {
	        /* inq by name */
	        status = nc_inq_dimid (ncid, ncInqInp->name, &dim[i].id);
	        if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "_rsNcInq: nc_inq_dimid error for %s.  %s ", 
	              ncInqInp->name, nc_strerror(status));
                    status = NETCDF_INQ_ID_ERR + status;
                    freeNcInqOut (ncInqOut);
                    return status;
		}
	    } else {
		dim[i].id = ncInqInp->myid;
	    }
	    status =  nc_inq_dim (ncid, dim[i].id, dim[i].name, &mylong);
        }
        if (status == NC_NOERR) {
	     dim[i].arrayLen = mylong;
        } else {
            rodsLog (LOG_ERROR,
              "_rsNcInq: nc_inq_dim error.  %s ", nc_strerror(status));
            status = NETCDF_INQ_DIM_ERR + status;
	    freeNcInqOut (ncInqOut);
            return status;
	}
    }

    /* inq variables */
    var = (*ncInqOut)->var;
    for (i = 0; i < nvars; i++) {
	if (allFlag != 0) {
            var[i].id = i;
	} else {
	    if (*ncInqInp->name != '\0') {
                /* inq by name */
		status = nc_inq_varid (ncid, ncInqInp->name, &var[i].id);
                if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "_rsNcInq: nc_inq_varid error for %s.  %s ",
                      ncInqInp->name, nc_strerror(status));
                    status = NETCDF_INQ_ID_ERR + status;
                    freeNcInqOut (ncInqOut);
                    return status;
                }
	    } else {
		var[i].id = ncInqInp->myid;
	    }
	}
        status = nc_inq_var (ncid, var[i].id, var[i].name, &var[i].dataType, 
	  &var[i].nvdims, intArray, &var[i].natts);
        if (status == NC_NOERR) {
	    /* fill in att */
	    if (var[i].natts > 0) {
		var[i].att = (ncGenAttOut_t *) 
		  calloc (var[i].natts, sizeof (ncGenAttOut_t));
	        status = inqAtt (ncid, i, var[i].natts, NULL, 0, NC_ALL_FLAG,
		  var[i].att);
	        if (status < 0) {
                    freeNcInqOut (ncInqOut);
                    return status;
		}
	    }
	    /* fill in dimId */
	    if (var[i].nvdims > 0) {
		int j;
	        var[i].dimId = (int *) calloc (var[i].nvdims, sizeof (int));
		for (j = 0; j < var[i].nvdims; j++) {
		    var[i].dimId[j] = intArray[j];
		}
	    }
        } else {
            rodsLog (LOG_ERROR,
              "_rsNcInq: nc_inq_var error.  %s ", nc_strerror(status));
            status = NETCDF_INQ_VARS_ERR + status;
            freeNcInqOut (ncInqOut);
            return status;
        }
    }

    /* inq attributes */
    gatt = (*ncInqOut)->gatt;
    status = inqAtt (ncid, NC_GLOBAL, ngatts, ncInqInp->name, ncInqInp->myid,
      allFlag, gatt);

    return status;
}

int
inqAtt (int ncid, int varid, int natt, char *name, int id, int allFlag,
ncGenAttOut_t *attOut)
{
    int status, i;
    nc_type dataType;
    size_t length;


    if (natt <= 0) return 0;

    if (attOut == NULL) return USER__NULL_INPUT_ERR;

    for (i = 0; i < natt; i++) {
        if (allFlag != 0) {
            attOut[i].id = i;
	    status = nc_inq_attname (ncid, varid, i, attOut[i].name);
        } else {
            if (*name != '\0') {
                /* inq by name */
		rstrcpy (attOut[i].name, name, 	NAME_LEN);
		status = NC_NOERR;
	    } else {
		/* inq by id */
		attOut[i].id = id;
	        status = nc_inq_attname (ncid, varid, id, attOut[i].name);
 	    }	
        }
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "inqAtt: nc_inq_attname error for ncid %d, varid %d, %s", 
	      ncid, varid, nc_strerror(status));
            status = NETCDF_INQ_ATT_ERR + status;
	    free (attOut);
            return status;
        }
	status = nc_inq_att (ncid, varid, attOut[i].name, &dataType, &length);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "inqAtt: nc_inq_att error for ncid %d, varid %d, %s", 
              ncid, varid, nc_strerror(status));
            status = NETCDF_INQ_ATT_ERR + status;
            free (attOut);
            return status;
        }   
	status = getAttValue (ncid, varid, attOut[i].name, dataType, length,
	  &attOut[i].value);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "inqAtt: getAttValue error for ncid %d, varid %d", ncid, varid);
            free (attOut);
            return status;
        }  
	attOut[i].dataType = dataType;
	attOut[i].length = length;
	attOut[i].id = i;
    }
    
    return 0;
}

int
getAttValue (int ncid, int varid, char *name, int dataType, int length,
ncGetVarOut_t *value)
{
    int status;

    (value)->dataArray = (dataArray_t *) calloc (1, sizeof (dataArray_t));
    (value)->dataArray->len = length;
    (value)->dataArray->type = dataType;
    switch (dataType) {
      case NC_CHAR:
        value->dataArray->buf = calloc (length + 1, sizeof (char));
        rstrcpy (value->dataType_PI, "charDataArray_PI", NAME_LEN);
        status = nc_get_att_text (ncid, varid, name,
          (char *) (value)->dataArray->buf);
	(value)->dataArray->len = length + 1;
        break;
      case NC_BYTE:
      case NC_UBYTE:
        value->dataArray->buf = calloc (length, sizeof (char));
        rstrcpy (value->dataType_PI, "charDataArray_PI", NAME_LEN);
        status = nc_get_att_uchar (ncid, varid, name,
          (unsigned char *) (value)->dataArray->buf);
        break;
      case NC_SHORT:
        /* use int because we can't pack short yet. */
        (value)->dataArray->buf = calloc (length, sizeof (short));
        rstrcpy ((value)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        status = nc_get_att_short (ncid, varid, name, 
          (short *) (value)->dataArray->buf);
        break;
      case NC_USHORT:
        /* use uint because we can't pack short yet. */
        (value)->dataArray->buf = calloc (length, sizeof (short));
        rstrcpy ((value)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        status = nc_get_att_ushort (ncid, varid, name, 
          (unsigned short *) (value)->dataArray->buf);
        break;
      case NC_STRING:
        (value)->dataArray->buf = calloc (length + 1, sizeof (char *));
        rstrcpy ((value)->dataType_PI, "strDataArray_PI", NAME_LEN);
        status = nc_get_att_string (ncid, varid, name,
          (char **) (value)->dataArray->buf);
        break;
      case NC_INT:
       (value)->dataArray->buf = calloc (length, sizeof (int));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_int (ncid, varid, name,
          (int *) (value)->dataArray->buf);
        break;
      case NC_UINT:
       (value)->dataArray->buf = calloc (length, sizeof (unsigned int));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_uint (ncid, varid, name,
          (unsigned int *) (value)->dataArray->buf);
        break;
      case NC_INT64:
        (value)->dataArray->buf = calloc (length, sizeof (long long));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_longlong (ncid, varid, name,
          (long long *) (value)->dataArray->buf);
        break;
      case NC_UINT64:
        (value)->dataArray->buf = calloc (length, sizeof (unsigned long long));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_ulonglong (ncid, varid, name,
          (unsigned long long *) (value)->dataArray->buf);
        break;
      case NC_FLOAT:
        (value)->dataArray->buf = calloc (length, sizeof (float));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_float (ncid, varid, name,
          (float *) (value)->dataArray->buf);
        break;
      case NC_DOUBLE:
        (value)->dataArray->buf = calloc (length, sizeof (double));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_double (ncid, varid, name,
          (double *) (value)->dataArray->buf);
        break;
      default:
        rodsLog (LOG_ERROR,
          "getAttValue: Unknow dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }

    if (status != NC_NOERR) {
        clearNcGetVarOut (value);
        rodsLog (LOG_ERROR,
          "getAttValue:  nc_get_att err varid %d dataType %d. %s ",
          varid, dataType, nc_strerror(status));
        status = NETCDF_GET_ATT_ERR + status;
    }
    return status;
}
#endif
