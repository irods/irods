#ifndef READ_COLLECTION_H__
#define READ_COLLECTION_H__

#include "rcConnect.h"
#include "miscUtil.h"


/* rcReadCollection - Read a iRODS collection. rcOpenCollection must be
 *    called first.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjReadInp_t *readCollInp - generic read input. Relevant items are:
 *      l1descInx - the handleInx from the rcOpenCollection call.
 *
 * OutPut -
 *   collEnt_t **collEnt - the metadata of an object in the collection.
 *   int status - status of the operation. -1 means no more collEnt
 */
#ifdef __cplusplus
extern "C"
#endif
int rcReadCollection( rcComm_t *conn, int handleInxInp, collEnt_t **collEnt );

#endif
