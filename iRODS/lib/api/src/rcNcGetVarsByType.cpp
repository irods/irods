/**
 * @file  rcNcGetVarsByType.c
 *
 */

/* This is script-generated code.  */ 
/* See ncGetVarsByType.h for a description of this API call.*/

#include "ncGetVarsByType.h"

/**
 * \fn rcNcGetVarsByType (rcComm_t *conn,  ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut)
 *
 * \brief General NETCDF variable subsetting function (equivalent to nc_get_vars_type API).
 *
 * \user client
 *
 * \category NETCDF operations
 *
 * \since 3.1
 *
 * \author  Mike Wan
 * \date    2012
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Example for this API can be rather involved. See the getSingleNcVarData function in rcNcInq.c on the usage of this API. 
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncGetVarInp - Elements of ncGetVarInp_t used :
 *    \li int \b dataType - valid values are defined in ncGetVarsByType.h - NC_BYTE, NC_CHAR, NC_SHORT, NC_INT, NC_FLOAT, NC_DOUBLE, NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64 or NC_STRING.
 *    \li int \b ncid - the ncid from ncNcOpen.
 *    \li int \b varid - the variable Id from rcNcInq or rcNcInqId.
 *    \li int \b ndim - number of dimensions (rank).
 *    \li rodsLong_t \b *start - A vector of rodsLong_t with ndim length specifying the index in the variable where the first of the data values will be read.
 *    \li rodsLong_t \b *count - A vector of rodsLong_t with ndim length specifying the number of indices selected along each dimension.
 *    \li rodsLong_t \b *stride - A vector of rodsLong_t with ndim length specifying for each dimension, the interval between selected indices.
 * \param[out] ncGetVarOut - a ncGetVarOut_t. Elements of ncGetVarOut_t:
 *    \li char \b dataType_PI[NAME_LEN] - Packing instruction of the dataType.
 *    \li dataArray_t \b *dataArray - returned values of the variable. dataArray->type gives the var type; dataArray->len gives the var length; dataArray->buf contains the var values.
 *
 * \return integer
 * \retval status of the call. success if greater or equal 0. error if negative.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcNcGetVarsByType (rcComm_t *conn,  ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut)
{
    int status;
    status = procApiRequest (conn, NC_GET_VARS_BY_TYPE_AN, ncGetVarInp, NULL, 
        (void **) ncGetVarOut, NULL);

    return (status);
}

int
freeNcGetVarOut (ncGetVarOut_t **ncGetVarOut)
{
    if (ncGetVarOut == NULL || *ncGetVarOut == NULL) return 
      USER__NULL_INPUT_ERR;

    clearNcGetVarOut (*ncGetVarOut);
    free (*ncGetVarOut);
    *ncGetVarOut = NULL;
    return 0;
}

int
clearNcGetVarOut (ncGetVarOut_t *ncGetVarOut)
{
    if (ncGetVarOut == NULL) return USER__NULL_INPUT_ERR;

    if ((ncGetVarOut)->dataArray != NULL) {
        if ((ncGetVarOut)->dataArray->buf != NULL) {
            free ((ncGetVarOut)->dataArray->buf);
        }
        free ((ncGetVarOut)->dataArray);
    }
    return 0;
}

int
clearNcGetVarInp (ncGetVarInp_t *ncGetVarInp)
{
    if (ncGetVarInp == NULL) return USER__NULL_INPUT_ERR;
    if (ncGetVarInp->start != NULL) {
	free (ncGetVarInp->start);
	ncGetVarInp->start = NULL;
    }
    if (ncGetVarInp->count != NULL) {
        free (ncGetVarInp->count);
        ncGetVarInp->count = NULL;
    }
    if (ncGetVarInp->stride != NULL) {
        free (ncGetVarInp->stride);
        ncGetVarInp->stride = NULL;
    }
    clearKeyVal (&ncGetVarInp->condInput);
    return 0;
}

#ifdef NETCDF_API
/* _rsNcGetVarsByType - moved from server to client because it can be
 * called by client too
 */
int
_rsNcGetVarsByType (int ncid, ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut)
{
    int status;
    size_t start[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ptrdiff_t stride[NC_MAX_DIMS];
    int i;
    int len = 1;
    int hasStride = 0;

    if (ncGetVarInp == NULL || ncGetVarOut == NULL) return USER__NULL_INPUT_ERR;

    bzero (start, sizeof (start));
    bzero (count, sizeof (count));
    bzero (stride, sizeof (stride));
    for (i = 0; i < ncGetVarInp->ndim; i++) {
        start[i] = ncGetVarInp->start[i];
        count[i] = ncGetVarInp->count[i];
        stride[i] = ncGetVarInp->stride[i];
    }
    len = getSizeForGetVars (ncGetVarInp);
    if (len <= 0) return len;
    *ncGetVarOut = (ncGetVarOut_t *) calloc (1, sizeof (ncGetVarOut_t));
    (*ncGetVarOut)->dataArray = (dataArray_t *) calloc (1, sizeof (dataArray_t));
    (*ncGetVarOut)->dataArray->len = len;
    (*ncGetVarOut)->dataArray->type = ncGetVarInp->dataType;

    switch (ncGetVarInp->dataType) {
      case NC_CHAR:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (char) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "charDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_text (ncid, ncGetVarInp->varid, start, count,
              stride, (char *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_text (ncid, ncGetVarInp->varid, start, count,
              (char *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_BYTE:
      case NC_UBYTE:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (char) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "charDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_uchar (ncid, ncGetVarInp->varid, start, count,
              stride, (unsigned char *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_uchar (ncid, ncGetVarInp->varid, start, count,
              (unsigned char *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_STRING:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (char *) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "strDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_string (ncid, ncGetVarInp->varid, start, count,
              stride, (char **) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_string (ncid, ncGetVarInp->varid, start, count,
              (char **) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_INT:
       (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (int) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "intDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_int (ncid, ncGetVarInp->varid, start, count,
              stride, (int *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_int (ncid, ncGetVarInp->varid, start, count,
              (int *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_UINT:
       (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (unsigned int) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "intDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_uint (ncid, ncGetVarInp->varid, start, count,
              stride, (unsigned int *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_uint (ncid, ncGetVarInp->varid, start, count,
              (unsigned int *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_SHORT:
       (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (short) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_short (ncid, ncGetVarInp->varid, start, count,
              stride, (short *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_short (ncid, ncGetVarInp->varid, start, count,
              (short *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_USHORT:
       (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (short) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_ushort (ncid, ncGetVarInp->varid, start, count,
              stride, (unsigned short*) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_ushort (ncid, ncGetVarInp->varid, start, count,
              (unsigned short*) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_INT64:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (long long) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_longlong (ncid, ncGetVarInp->varid, start,
              count, stride, (long long *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_longlong (ncid, ncGetVarInp->varid, start,
              count, (long long *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_UINT64:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (unsigned long long) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_ulonglong (ncid, ncGetVarInp->varid,
              start, count, stride,
              (unsigned long long *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_ulonglong (ncid, ncGetVarInp->varid, start,
              count, (unsigned long long *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_FLOAT:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (float) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "intDataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_float (ncid, ncGetVarInp->varid, start, count,
              stride, (float *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_float (ncid, ncGetVarInp->varid, start, count,
              (float *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      case NC_DOUBLE:
        (*ncGetVarOut)->dataArray->buf = calloc (1, sizeof (double) * len);
        rstrcpy ((*ncGetVarOut)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        if (hasStride != 0) {
            status = nc_get_vars_double (ncid, ncGetVarInp->varid, start, count,
              stride, (double *) (*ncGetVarOut)->dataArray->buf);
        } else {
            status = nc_get_vara_double (ncid, ncGetVarInp->varid, start, count,
              (double *) (*ncGetVarOut)->dataArray->buf);
        }
        break;
      default:
        rodsLog (LOG_ERROR,
          "_rsNcGetVarsByType: Unknow dataType %d", ncGetVarInp->dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }

    if (status != NC_NOERR) {
        freeNcGetVarOut (ncGetVarOut);
        rodsLog (LOG_ERROR,
          "_rsNcGetVarsByType:  nc_get_vars err varid %d dataType %d. %s ",
          ncGetVarInp->varid, ncGetVarInp->dataType, nc_strerror(status));
        status = NETCDF_GET_VARS_ERR - status;
    }
    return status;
}
#endif		/* NETCDF_API */

int
getSizeForGetVars (ncGetVarInp_t *ncGetVarInp)
{
    int i;
    int len = 1;
    int hasStride = 0;

    for (i = 0; i < ncGetVarInp->ndim; i++) {
        if (ncGetVarInp->count[i] <= 0) return NETCDF_VAR_COUNT_OUT_OF_RANGE;
        /* cal dataArray->len */
        if (ncGetVarInp->stride[i] <= 0) {
            ncGetVarInp->stride[i] = 1;
        } else if (ncGetVarInp->stride[i] > 1) {
            hasStride = 1;
        }
        len = len * ((ncGetVarInp->count[i] - 1) / ncGetVarInp->stride[i] + 1);
    }
    return len;
}

int
getDataTypeSize (int dataType)
{
    int size;

    switch (dataType) {
      case NC_CHAR:
      case NC_BYTE:
      case NC_UBYTE:
        size = sizeof (char);
        break;
      case NC_STRING:
        size = sizeof (char *);
        break;
      case NC_INT:
      case NC_UINT:
        size = sizeof (int);
        break;
      case NC_SHORT:
      case NC_USHORT:
        size = sizeof (short);
        break;
      case NC_INT64:
      case NC_UINT64:
        size = sizeof (rodsLong_t);
        break;
      case NC_FLOAT:
        size = sizeof (float);
        break;
      case NC_DOUBLE:
        size = sizeof (double);
        break;
        rodsLog (LOG_ERROR,
          "getDataTypeSize: Unknow dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    return size;
}





 
