#ifndef REG_DATA_OBJ_H__
#define REG_DATA_OBJ_H__

#include "rcConnect.h"
#include "objInfo.h"


/* rcRegDataObj - Register a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInfo_t *dataObjInfo - the dataObjInfo to register
 *   dataObjInfo_t **outDataObjInfo - the output DataObjInfo which has
 *      the dataId from the registration as output
 *
 * OutPut -
 *   int status - status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcRegDataObj( rcComm_t *conn, dataObjInfo_t *dataObjInfo, dataObjInfo_t **outDataObjInfo );

#endif
