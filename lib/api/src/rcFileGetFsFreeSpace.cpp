#include "fileGetFsFreeSpace.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileGetFsFreeSpace( rcComm_t *conn, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut )
 *
 * \brief Gets filesystem free space.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileGetFsFreeSpaceInp
 * \param[out] fileGetFsFreeSpaceOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileGetFsFreeSpace( rcComm_t *conn,
                      fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
                      fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut ) {
    int status;
    status = procApiRequest( conn, FILE_GET_FS_FREE_SPACE_AN,
                             fileGetFsFreeSpaceInp, NULL, ( void ** ) fileGetFsFreeSpaceOut, NULL );

    return status;
}
