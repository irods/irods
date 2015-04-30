/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GET_REMOTE_ZONE_RESC_HPP
#define GET_REMOTE_ZONE_RESC_HPP

#define REMOTE_CREATE   "remoteCreate"
#define REMOTE_OPEN     "remoteOpen"

/* This is a Object File I/O call */

#include "rods.h"
#include "rcMisc.hpp"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "dataObjInpOut.h"

#if defined(RODS_SERVER)
#define RS_GET_REMOTE_ZONE_RESC rsGetRemoteZoneResc
/* prototype for the server handler */
int
rsGetRemoteZoneResc( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                     rodsHostAddr_t **rescAddr );
#else
#define RS_GET_REMOTE_ZONE_RESC NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
int
rcGetRemoteZoneResc( rcComm_t *conn, dataObjInp_t *dataObjInp,
                     rodsHostAddr_t **rescAddr );

#ifdef __cplusplus
}
#endif
#endif	/* GET_REMOTE_ZONE_RESC_H */
