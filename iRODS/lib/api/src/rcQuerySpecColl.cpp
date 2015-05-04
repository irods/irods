#include "querySpecColl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcQuerySpecColl( rcComm_t *conn, dataObjInp_t *querySpecCollInp, genQueryOut_t **genQueryOut )
 *
 * \brief Query a special collection.
 *
 * \user client
 *
 * \ingroup server_miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] querySpecCollInp
 * \param[out] genQueryOut - the GenQuery output
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcQuerySpecColl( rcComm_t *conn, dataObjInp_t *querySpecCollInp,
                 genQueryOut_t **genQueryOut ) {
    int status;
    status = procApiRequest( conn, QUERY_SPEC_COLL_AN, querySpecCollInp, NULL,
                             ( void ** ) genQueryOut, NULL );

    return status;
}
