#include "fileMkdir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileMkdir( rcComm_t *conn, fileMkdirInp_t *fileMkdirInp )
 *
 * \brief Make a directory.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileMkdirInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileMkdir( rcComm_t *conn, fileMkdirInp_t *fileMkdirInp ) {
    int status;
    status = procApiRequest( conn, FILE_MKDIR_AN,  fileMkdirInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
