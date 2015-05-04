#include "fileOpen.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileOpen( rcComm_t *conn, fileOpenInp_t *fileOpenInp )
 *
 * \brief Open a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileOpenInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileOpen( rcComm_t *conn, fileOpenInp_t *fileOpenInp ) {
    int status;

#if defined(osx_platform)
    if ( fileOpenInp->flags & O_TRUNC ) {
        fileOpenInp->flags = fileOpenInp->flags ^ O_TRUNC;
        fileOpenInp->flags = fileOpenInp->flags | 0x200;
    }
#endif
    status = procApiRequest( conn, FILE_OPEN_AN, fileOpenInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}

