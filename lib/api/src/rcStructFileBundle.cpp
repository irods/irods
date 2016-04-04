#include "structFileBundle.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStructFileBundle( rcComm_t *conn, structFileExtAndRegInp_t *structFileBundleInp )
 *
 * \brief Bundle a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] structFileBundleInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcStructFileBundle( rcComm_t *conn,
                    structFileExtAndRegInp_t *structFileBundleInp ) {
    int status;
    status = procApiRequest( conn, STRUCT_FILE_BUNDLE_AN, structFileBundleInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}
