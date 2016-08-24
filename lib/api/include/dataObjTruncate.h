#ifndef DATA_OBJ_TRUNCATE_H__
#define DATA_OBJ_TRUNCATE_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

/* rcDataObjTruncate - Truncate a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      dataSize - the size to truncate to
 *
 * OutPut -
 *   return value - The status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjTruncate( rcComm_t *conn, dataObjInp_t *dataObjInp );

#endif
