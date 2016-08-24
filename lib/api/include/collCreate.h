#ifndef COLL_CREATE_H__
#define COLL_CREATE_H__

#include "objInfo.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"

/* rcCollCreate - Create a iRODS collection.
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
int rcCollCreate( rcComm_t *conn, collInp_t *collCreateInp );

#endif
