#include "fileChmod.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileChmod( rcComm_t *conn, fileChmodInp_t *fileChmodInp )
 *
 * \brief Changes mode on a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileChmodInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileChmod( rcComm_t *conn, fileChmodInp_t *fileChmodInp ) {
    int status;
    status = procApiRequest( conn, FILE_CHMOD_AN,  fileChmodInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
