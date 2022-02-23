#include "irods/regColl.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

/**
 * \fn rcRegColl( rcComm_t *conn, collInp_t *regCollInp )
 *
 * \brief Register a collection.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] regCollInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcRegColl( rcComm_t *conn, collInp_t *regCollInp ) {
    int status;
    status = procApiRequest( conn, REG_COLL_AN, regCollInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
