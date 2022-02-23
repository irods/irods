#ifndef RS_GET_REMOTE_ZONE_RESC_HPP
#define RS_GET_REMOTE_ZONE_RESC_HPP

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"

int rsGetRemoteZoneResc( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsHostAddr_t **rescAddr );

#endif
