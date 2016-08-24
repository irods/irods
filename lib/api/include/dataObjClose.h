#ifndef DATA_OBJ_CLOSE_H__
#define DATA_OBJ_CLOSE_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"
#include "rodsType.h"

/* prototype for the client call */
/* rcDataObjClose - Close an opened iRODS data object descriptor.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjCloseInp_t *dataObjCloseInp - Relevant items are:
 *      l1descInx - the iRODS data object descriptor to close.
 *
 * OutPut -
 *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjClose( rcComm_t *conn, openedDataObjInp_t *dataObjCloseInp );

#endif
