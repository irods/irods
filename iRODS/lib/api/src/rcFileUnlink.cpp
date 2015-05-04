#include "fileUnlink.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileUnlink( rcComm_t *conn, fileUnlinkInp_t *fileUnlinkInp )
 *
 * \brief Unlink a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileUnlinkInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileUnlink( rcComm_t *conn, fileUnlinkInp_t *fileUnlinkInp ) {
    int status;
    status = procApiRequest( conn, FILE_UNLINK_AN,  fileUnlinkInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
