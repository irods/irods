/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regReplica.h
 */

#ifndef REG_REPLICA_H
#define REG_REPLICA_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

typedef struct {
    dataObjInfo_t *srcDataObjInfo;
    dataObjInfo_t *destDataObjInfo;
    keyValPair_t condInput;
} regReplica_t;

#define RegReplica_PI "struct *DataObjInfo_PI; struct *DataObjInfo_PI; struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_REG_REPLICA rsRegReplica
/* prototype for the server handler */
int
rsRegReplica (rsComm_t *rsComm, regReplica_t *regReplicaInp);
int
_rsRegReplica (rsComm_t *rsComm, regReplica_t *regReplicaInp);
#else
#define RS_REG_REPLICA NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcRegReplica (rcComm_t *conn, regReplica_t *regReplicaInp);

/* rcRegReplica - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   regReplica_t *regReplicaInp - the dataObj to replicate
 *
 * OutPut -
 *   int status - status of the operation.
 */

int
clearRegReplicaInp (regReplica_t *regReplicaInp);

#ifdef  __cplusplus
}
#endif

#endif	/* REG_REPLICA_H */
