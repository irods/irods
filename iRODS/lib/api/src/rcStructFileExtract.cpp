#include "structFileExtract.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStructFileExtract( rcComm_t *conn, structFileOprInp_t *structFileOprInp )
 *
 * \brief Extract a structured file object.
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
rcStructFileExtract( rcComm_t *conn, structFileOprInp_t *structFileOprInp ) {
    int status;
    status = procApiRequest( conn, STRUCT_FILE_EXTRACT_AN,
                             structFileOprInp, NULL, ( void ** ) NULL, NULL );

    return status;
}
