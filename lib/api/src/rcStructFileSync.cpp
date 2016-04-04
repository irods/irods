#include "structFileSync.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStructFileSync( rcComm_t *conn, structFileOprInp_t *structFileOprInp )
 *
 * \brief Sync a structured file object with its extracted subfiles.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] structFileOprInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcStructFileSync( rcComm_t *conn, structFileOprInp_t *structFileOprInp ) {
    int status;
    status = procApiRequest( conn, STRUCT_FILE_SYNC_AN, structFileOprInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
