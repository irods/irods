/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncInqId.h
 */

#ifndef NC_INQ_WITH_ID_H
#define NC_INQ_WITH_ID_H

/* This is a NETCDF API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "ncInqId.h"

typedef struct {
    rodsLong_t mylong;	/* an int output.content depends on paramType.For 
			 * NC_DIM_T, this is arrayLen. not used for NC_VAR_T */
    int dataType;		/* data type for NC_VAR_T. */
    int natts;		/* no. of attrs for NC_VAR_T */
    char name[MAX_NAME_LEN];
    int myint;		/* not used */
    int ndim;		/* no. of dimension */
    int *intArray;	/* int array of ndim length */
} ncInqWithIdOut_t;
    
#define NcInqWithIdOut_PI "double mylong; int dataType; int natts; str name[MAX_NAME_LEN];int myint; int ndim; int *intArray(ndim);"
#if defined(RODS_SERVER) && defined(NETCDF_API)
#define RS_NC_INQ_WITH_ID rsNcInqWithId
/* prototype for the server handler */
int
rsNcInqWithId (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp, 
ncInqWithIdOut_t **ncInqWithIdOut);
int
_rsNcInqWithId (int type, int ncid, int myid, char *name, 
ncInqWithIdOut_t **ncInqWithIdOut);
int
rsNcInqWithIdColl (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp, 
ncInqWithIdOut_t **ncInqWithIdOut);
int
rsNcInqWithIdDataObj (rsComm_t *rsComm, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut);
#else
#define RS_NC_INQ_WITH_ID NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* rcNcInqWithId - general netcdf inq with id (equivalent to nc_inq_dim,
 *    ....
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncInqIdInp_t struct:
 *     paramType - parameter type - NC_DIM_T, ....
 *     ncid - int ncid.   
 *     myid - depends on paramType.  dimid for NC_DIM_T,
 * OutPut - ncInqWithIdOut_t
 *		mylong- content depends on paramType. length for NC_DIM_T
 *		name - content depends on paramType. name of dim for NC_DIM_T
 */
/* prototype for the client call */
int
rcNcInqWithId (rcComm_t *conn, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut);

#ifdef  __cplusplus
}
#endif

#endif	/* NC_INQ_WITH_ID_H */
