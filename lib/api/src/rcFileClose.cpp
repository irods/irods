#include "fileClose.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp )
 *
 * \brief Closes a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileCloseInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp ) {
    int status;
    status = procApiRequest( conn, FILE_CLOSE_AN,  fileCloseInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
