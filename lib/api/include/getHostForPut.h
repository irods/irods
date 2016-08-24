#ifndef GET_HOST_FOR_PUT_H__
#define GET_HOST_FOR_PUT_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

#define THIS_ADDRESS    "thisAddress"   // a returned value for outHost, Just use the address of this conn

#ifdef __cplusplus
extern "C"
#endif
int rcGetHostForPut( rcComm_t *conn, dataObjInp_t *dataObjInp, char **outHost );

#endif
