#include "fileReaddir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileReaddir( rcComm_t *conn, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut )
 *
 * \brief Reads a file directory.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileReaddirInp
 * \param[out] fileReaddirOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileReaddir( rcComm_t *conn, fileReaddirInp_t *fileReaddirInp,
               rodsDirent_t **fileReaddirOut ) {
    int status;
    status = procApiRequest( conn, FILE_READDIR_AN,  fileReaddirInp, NULL,
                             ( void ** ) fileReaddirOut, NULL );

    return status;
}
