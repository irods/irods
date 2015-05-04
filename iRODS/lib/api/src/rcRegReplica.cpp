#include "regReplica.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcRegReplica( rcComm_t *conn, regReplica_t *regReplicaInp )
 *
 * \brief Register a replica.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] regReplicaInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
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
