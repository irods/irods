/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getRemoteZoneResc.h
 */

#ifndef GET_REMOTE_ZONE_RESC_H
#define GET_REMOTE_ZONE_RESC_H

#define REMOTE_CREATE   "remoteCreate"
#define REMOTE_OPEN     "remoteOpen"

/* This is a Object File I/O call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

#if defined(RODS_SERVER)
#define RS_GET_REMOTE_ZONE_RESC rsGetRemoteZoneResc
/* prototype for the server handler */
int
rsGetRemoteZoneResc (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
rodsHostAddr_t **rescAddr);
#else
#define RS_GET_REMOTE_ZONE_RESC NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
/* rcGetRemoteZoneResc - 
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *	condInput - condition input (optional).
 *	    REMOTE_ZONE_OPR_KW - specifies the type of remote zone operation.
 *	    currently, valid operations are REMOTE_CREATE and  REMOTE_OPEN 
 *
 * OutPut - 
 *   rodsHostAddr_t **rescAddr - the address of the resource.   
 */

int
rcGetRemoteZoneResc (rcComm_t *conn, dataObjInp_t *dataObjInp,
rodsHostAddr_t **rescAddr);

#ifdef  __cplusplus
}
#endif

#endif	/* GET_REMOTE_ZONE_RESC_H */
