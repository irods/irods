/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncGetAggElement.h
 */

#ifndef NC_GET_AGG_ELEMENT_H
#define NC_GET_AGG_ELEMENT_H

/* This is a NETCDF API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "ncOpen.h"
#ifdef NETCDF_API
#include "netcdf.h"
#endif

/* data struct for aggregation of netcdf files. Our first attempt assumes
 * the aggregation is based on the time dimension - time series */ 
typedef struct {
    unsigned int startTime;
    unsigned int endTime;
    char astartTime[NAME_LEN];
    char aendTime[NAME_LEN];
    rodsLong_t arraylen;
    char objPath[MAX_NAME_LEN];
} ncAggElement_t;

#define NcAggElement_PI "int startTime; int endTime; str astartTime[NAME_LEN]; str aendTime[NAME_LEN]; double arraylen; str objPath[MAX_NAME_LEN];"

#if defined(RODS_SERVER) && defined(NETCDF_API)
#define RS_NC_GET_AGG_ELEMENT rsNcGetAggElement
/* prototype for the server handler */
int
rsNcGetAggElement (rsComm_t *rsComm, ncOpenInp_t *ncOpenInp, 
ncAggElement_t **ncAggElement);
int
_rsNcGetAggElement (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, 
ncAggElement_t **ncAggElement);
#else
#define RS_NC_GET_AGG_ELEMENT NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* rcNcGetAggElement - get the ncAggElement of a NETCDF file
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncOpenInp_t *ncOpenInp - generic nc open/create input. Relevant items are:
 *	objPath - the path of the NETCDF data object.
 *	condInput - condition input (not used).
 * OutPut - 
 *   ncAggElement_t **ncAggElement - the ncAggElement of the NETCDF data object.
 */

/* prototype for the client call */
int
rcNcGetAggElement (rcComm_t *conn, ncOpenInp_t *ncOpenInp, 
ncAggElement_t **ncAggElement);

#ifdef  __cplusplus
}
#endif

#endif	/* NC_GET_AGG_ELEMENT_H */
