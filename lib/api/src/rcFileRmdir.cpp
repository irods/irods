#include "fileRmdir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileRmdir( rcComm_t *conn, fileRmdirInp_t *fileRmdirInp )
 *
 * \brief Remove a directory.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileRmdirInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileRmdir( rcComm_t *conn, fileRmdirInp_t *fileRmdirInp ) {
    int status;
    status = procApiRequest( conn, FILE_RMDIR_AN,  fileRmdirInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
