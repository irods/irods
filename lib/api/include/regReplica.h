#ifndef REG_REPLICA_H__
#define REG_REPLICA_H__

#include "rcConnect.h"
#include "objInfo.h"

typedef struct {
    dataObjInfo_t *srcDataObjInfo;
    dataObjInfo_t *destDataObjInfo;
    keyValPair_t condInput;
} regReplica_t;
#define RegReplica_PI "struct *DataObjInfo_PI; struct *DataObjInfo_PI; struct KeyValPair_PI;"


/* rcRegReplica - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   regReplica_t *regReplicaInp - the dataObj to replicate
 *
 * OutPut -
 *   int status - status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcRegReplica( rcComm_t *conn, regReplica_t *regReplicaInp );

#endif
