/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GET_HOST_FOR_PUT_H__
#define GET_HOST_FOR_PUT_H__

/* This is a Object File I/O call */

#include "rcConnect.h"
#include "dataObjInpOut.h"

#define THIS_ADDRESS	"thisAddress"	/* a returned value for outHost.
* Just use the address of this conn */
#if defined(RODS_SERVER)
#define RS_GET_HOST_FOR_PUT rsGetHostForPut
/* prototype for the server handler */
int
rsGetHostForPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 char **outHost );
#else
#define RS_GET_HOST_FOR_PUT NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
int
rcGetHostForPut( rcComm_t *conn, dataObjInp_t *dataObjInp,
                 char **outHost );
#ifdef __cplusplus
}
#endif
#endif	// GET_HOST_FOR_PUT_H__
