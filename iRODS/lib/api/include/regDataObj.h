/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regDataObj.h
 */

#ifndef REG_DATA_OBJ_H
#define REG_DATA_OBJ_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

#if defined(RODS_SERVER)
#define RS_REG_DATA_OBJ rsRegDataObj
/* prototype for the server handler */
int
rsRegDataObj (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, 
dataObjInfo_t **outDataObjInfo);
int
_rsRegDataObj (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
int
svrRegDataObj (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
#else
#define RS_REG_DATA_OBJ NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcRegDataObj (rcComm_t *conn, dataObjInfo_t *dataObjInfo,
dataObjInfo_t **outDataObjInfo);

/* rcRegDataObj - Register a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInfo_t *dataObjInfo - the dataObjInfo to register
 *   dataObjInfo_t **outDataObjInfo - the output DataObjInfo which has
 *	the dataId from the registration as output
 *
 * OutPut -
 *   int status - status of the operation.
 */

#ifdef  __cplusplus
}
#endif

#endif	/* REG_DATA_OBJ_H */
