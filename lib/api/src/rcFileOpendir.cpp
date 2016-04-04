#include "fileOpendir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileOpendir( rcComm_t *conn, fileOpendirInp_t *fileOpendirInp )
 *
 * \brief Open a directory.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileOpendirInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileOpendir( rcComm_t *conn, fileOpendirInp_t *fileOpendirInp ) {
    int status;
    status = procApiRequest( conn, FILE_OPENDIR_AN,  fileOpendirInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
