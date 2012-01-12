/* This is script-generated code.  */ 
/* See regReplica.h for a description of this API call.*/

#include "regReplica.h"

int
rcRegReplica (rcComm_t *conn, regReplica_t *regReplicaInp)
{
    int status;
    rescInfo_t *srcRescInfo, *destRescInfo;
    dataObjInfo_t *srcNext, *destNext;

    /* don't sent rescInfo and next */
    srcRescInfo = regReplicaInp->srcDataObjInfo->rescInfo;
    destRescInfo = regReplicaInp->destDataObjInfo->rescInfo;
    srcNext = regReplicaInp->srcDataObjInfo->next;
    destNext = regReplicaInp->destDataObjInfo->next;
    regReplicaInp->srcDataObjInfo->rescInfo = NULL;
    regReplicaInp->destDataObjInfo->rescInfo = NULL;
    regReplicaInp->srcDataObjInfo->next = NULL;
    regReplicaInp->destDataObjInfo->next = NULL;
    status = procApiRequest (conn, REG_REPLICA_AN, regReplicaInp, NULL, 
        (void **) NULL, NULL);
    regReplicaInp->srcDataObjInfo->rescInfo = srcRescInfo;
    regReplicaInp->destDataObjInfo->rescInfo = destRescInfo;
    regReplicaInp->srcDataObjInfo->next = srcNext;
    regReplicaInp->destDataObjInfo->next = destNext;

    return (status);
}
