#ifndef RM_COLL_H__
#define RM_COLL_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

/* rcRmColl - Recursively Remove a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   collInp_t *collInp - generic coll input. Relevant items are:
 *      collName - the collection to be registered.
 *      condInput - condition input (optional) - currently not used.
 *
 * OutPut -
 *   int status - status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcRmColl( rcComm_t *conn, collInp_t *rmCollInp, int vFlag );
int _rcRmColl( rcComm_t *conn, collInp_t *rmCollInp, collOprStat_t **collOprStat );

#endif
