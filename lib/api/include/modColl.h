#ifndef MOD_COLL_H__
#define MOD_COLL_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

/* rcModColl - Modify the collType, collInfo1 and collInfo2 of a
 * iRODS collection.
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
int rcModColl( rcComm_t *conn, collInp_t *modCollInp );

#endif
