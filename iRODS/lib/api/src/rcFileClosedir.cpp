#include "fileClosedir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileClosedir( rcComm_t *conn, fileClosedirInp_t *fileClosedirInp )
 *
 * \brief Close a directory.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileClosedirInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileClosedir( rcComm_t *conn, fileClosedirInp_t *fileClosedirInp ) {
    int status;
    status = procApiRequest( conn, FILE_CLOSEDIR_AN,  fileClosedirInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
