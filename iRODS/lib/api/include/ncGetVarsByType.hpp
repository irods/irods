/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncGetVarsByType.h
 */

#ifndef NC_GET_VARS_BY_TYPE_H
#define NC_GET_VARS_BY_TYPE_H

/* This is a NETCDF API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "ncInqId.h"

#ifndef NETCDF_API
#include <stddef.h>
/* NETCDF lib is not installed. Define some of the netcdf.h stuff */
#define NC_BYTE     1   /* signed 1 byte integer */
#define NC_CHAR     2   /* ISO/ASCII character   - uchar, text */
#define NC_SHORT    3   /* signed 2 byte integer */
#define NC_INT      4   /* signed 4 byte integer */
#define NC_FLOAT    5   /* single precision floating point number */
#define NC_DOUBLE   6   /* double precision floating point number */
#define NC_UBYTE    7   /* unsigned 1 byte int */
#define NC_USHORT   8   /* unsigned 2-byte int */
#define NC_UINT     9   /* unsigned 4-byte int */
#define NC_INT64    10  /* signed 8-byte int */
#define NC_UINT64   11  /* unsigned 8-byte int */
#define NC_STRING   12  /* string - only for netcdf librarysupports HDF */
#define NC_NOWRITE  0x00000
#define NC_WRITE    0x00001
#define NC_MAX_DIMS 1024
#define NC_FORMAT_CLASSIC	(1)
#define NC_FORMAT_64BIT		(2)
#define NC_FORMAT_NETCDF4	(3)
#define NC_FORMAT_NETCDF4_CLASSIC	(4)
#define NC_CLASSIC_MODEL	0x0100
#define NC_64BIT_OFFSET		0x0200
#define NC_NETCDF4		0x1000
#endif

 
/* valid dataType are:
NC_BYTE     1   signed 1 byte integer 
NC_CHAR     2   ISO/ASCII character   - uchar, text
NC_SHORT    3   signed 2 byte integer
NC_INT      4   signed 4 byte integer 
NC_FLOAT    5   single precision floating point number 
NC_DOUBLE   6   double precision floating point number
NC_UBYTE    7   unsigned 1 byte int
NC_USHORT   8   unsigned 2-byte int
NC_UINT     9   unsigned 4-byte int
NC_INT64    10  signed 8-byte int
NC_UINT64   11  unsigned 8-byte int
NC_STRING   12  string - only for netcdf librarysupports HDF
*/

typedef struct {
    int dataType;
    int ncid;
    int varid;	
    int ndim;		/* no. of dimension */
    rodsLong_t *start;  /* array of ndim length */
    rodsLong_t *count;  /* array of ndim length */
    rodsLong_t *stride; /* array of ndim length */
    keyValPair_t condInput;
} ncGetVarInp_t;
    
#define NcGetVarInp_PI "int varType; int ncid; int varid; int ndim; double *start(ndim); double *count(ndim); double *stride(ndim); struct KeyValPair_PI;"

typedef struct {
    char dataType_PI[NAME_LEN];	  /* the packing instruction of the dataType */
    dataArray_t *dataArray;
} ncGetVarOut_t;

#define NcGetVarOut_PI "piStr dataType_PI[NAME_LEN]; ?dataType_PI *dataArray;"

#if defined(RODS_SERVER) && defined (NETCDF_API)
#define RS_NC_GET_VARS_BY_TYPE rsNcGetVarsByType
/* prototype for the server handler */
int
rsNcGetVarsByType (rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp, 
ncGetVarOut_t **ncGetVarOut);
int
rsNcGetVarsByTypeForColl (rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut);
int
rsNcGetVarsByTypeForObj (rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut);
#else
#define RS_NC_GET_VARS_BY_TYPE NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* rcNcGetVarsByType - general netcdf subsetting function nc_get_vars_type
 *    ....
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncGetVarInp_t struct:
 *    int dataType;
 *   int ncid;
 *   int varid;
 *   int ndim;           no. of dimension 
 *   rodsLong_t *start;  array of ndim length
 *   rodsLong_t *count;  array of ndim length
 *   rodsLong_t *stride; array of ndim length 
 * OutPut - ncGetVarOut_t
 *   char dataType_PI;   the packing instruction of the dataType 
 *   int type;		 the type of data
 *   int len;            array length of the data
 *   void *data;         data array of dataLen length
 */
/* prototype for the client call */
int
rcNcGetVarsByType (rcComm_t *conn,  ncGetVarInp_t *ncGetVarInp, 
ncGetVarOut_t **ncGetVarOut);
int
freeNcGetVarOut (ncGetVarOut_t **ncGetVarOut);
int
clearNcGetVarOut (ncGetVarOut_t *ncGetVarOut);
int
clearNcGetVarInp (ncGetVarInp_t *ncGetVarInp);
int
_rsNcGetVarsByType (int ncid, ncGetVarInp_t *ncGetVarInp,
ncGetVarOut_t **ncGetVarOut);
int
getSizeForGetVars (ncGetVarInp_t *ncGetVarInp);
int
getDataTypeSize (int dataType);
#ifdef  __cplusplus
}
#endif

#endif	/* NC_GET_VARS_BY_TYPE_H */
