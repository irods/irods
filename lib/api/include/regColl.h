#ifndef REG_COLL_H__
#define REG_COLL_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

/* rcRegColl - Register a iRODS collection.
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
int rcRegColl( rcComm_t *conn, collInp_t *regCollInp );

#endif
