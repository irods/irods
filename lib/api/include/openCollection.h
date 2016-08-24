#ifndef OPEN_COLLECTION_H__
#define OPEN_COLLECTION_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

/* rcOpenCollection - Open a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   openCollInp_t *collInp - generic coll input. Relevant items are:
 *      collName - the collection to be registered.
 *      flags - supports LONG_METADATA_FG, VERY_LONG_METADATA_FG
 *        and VERY_LONG_METADATA_FG.
 *
 * OutPut -
 *   int status - status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcOpenCollection( rcComm_t *conn, collInp_t *openCollInp );

#endif
