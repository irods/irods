/* This is script-generated code.  */ 
/* See getRemoteZoneRescRecur.h for a description of this API call.*/

#include "getRemoteZoneResc.h"

int
rcGetRemoteZoneResc (rcComm_t *conn, dataObjInp_t *dataObjInp,
rodsHostAddr_t **rescAddr)
{
    int status;
    status = procApiRequest (conn, GET_REMOTE_ZONE_RESC_AN, dataObjInp, NULL, 
        (void **) rescAddr, NULL);

    return (status);
}


