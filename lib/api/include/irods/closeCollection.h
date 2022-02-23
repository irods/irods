#ifndef CLOSE_COLLECTION_H__
#define CLOSE_COLLECTION_H__

#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"


/* rcCloseCollection - Close a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   int handleInxInp - the handleInx (collection handle index) to close.
 *
 * Output -
 *   int status - status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcCloseCollection( rcComm_t *conn, int handleInxInp );

#endif
