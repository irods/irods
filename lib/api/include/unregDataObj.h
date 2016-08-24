#ifndef UNREG_DATA_OBJ_H__
#define UNREG_DATA_OBJ_H__

#include "rcConnect.h"
#include "objInfo.h"

typedef struct {
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput;
} unregDataObj_t;

#define UnregDataObj_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"

/* rcUnregDataObj - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   unregDataObj_t *unregDataObjInp - the dataObjInfo to unregister
 *
 * OutPut -
 *   int status - status of the operation.
 */

int rcUnregDataObj( rcComm_t *conn, unregDataObj_t *unregDataObjInp );

#endif
