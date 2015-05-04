#include "modColl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcModColl( rcComm_t *conn, collInp_t *modCollInp )
 *
 * \brief Modify a collection.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] modCollInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcModColl( rcComm_t *conn, collInp_t *modCollInp ) {
    int status;
    status = procApiRequest( conn, MOD_COLL_AN, modCollInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
