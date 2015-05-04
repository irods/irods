/* This is script-generated code.  */
/* See collRepl.h for a description of this API call.*/

#include "collRepl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
_rcCollRepl( rcComm_t *conn, collInp_t *collReplInp,
             collOprStat_t **collOprStat ) {
    int status;

    collReplInp->oprType = REPLICATE_OPR;

    status = procApiRequest( conn, COLL_REPL_AN, collReplInp, NULL,
                             ( void ** ) collOprStat, NULL );

    return status;
}

/**
 * \fn rcCollRepl (rcComm_t *conn, collInp_t *collReplInp, int vFlag)
 *
 * \brief Replicate a collection.
 *
 * \user client
 *
 * \ingroup collection_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] collReplInp - Collection to replicate
 * \param[in] vFlag - verbose flag
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcCollRepl( rcComm_t *conn, collInp_t *collReplInp, int vFlag ) {
    int status, retval;
    collOprStat_t *collOprStat = NULL;

    retval = _rcCollRepl( conn, collReplInp, &collOprStat );

    status = cliGetCollOprStat( conn, collOprStat, vFlag, retval );

    return status;
}

