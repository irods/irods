/* This is script-generated code.  */
/* See regReplica.h for a description of this API call.*/

#include "regReplica.hpp"

int
rcRegReplica( rcComm_t *conn, regReplica_t *regReplicaInp ) {
    int status;
    dataObjInfo_t *srcNext, *destNext;

    /* don't send next */
    srcNext = regReplicaInp->srcDataObjInfo->next;
    destNext = regReplicaInp->destDataObjInfo->next;
    regReplicaInp->srcDataObjInfo->next = NULL;
    regReplicaInp->destDataObjInfo->next = NULL;
    status = procApiRequest( conn, REG_REPLICA_AN, regReplicaInp, NULL,
                             ( void ** ) NULL, NULL );
    regReplicaInp->srcDataObjInfo->next = srcNext;
    regReplicaInp->destDataObjInfo->next = destNext;

    return status;
}
