#ifndef GET_REMOTE_ZONE_RESC_H__
#define GET_REMOTE_ZONE_RESC_H__

#define REMOTE_CREATE   "remoteCreate"
#define REMOTE_OPEN     "remoteOpen"

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"

#ifdef __cplusplus
extern "C"
#endif
int rcGetRemoteZoneResc( rcComm_t *conn, dataObjInp_t *dataObjInp, rodsHostAddr_t **rescAddr );

#endif
