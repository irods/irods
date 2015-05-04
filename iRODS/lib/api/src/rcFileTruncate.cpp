#include "fileTruncate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileTruncate( rcComm_t *conn, fileOpenInp_t *fileTruncateInp )
 *
 * \brief Truncate a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileTruncateInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileTruncate( rcComm_t *conn, fileOpenInp_t *fileTruncateInp ) {
    int status;
    status = procApiRequest( conn, FILE_TRUNCATE_AN, fileTruncateInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}

