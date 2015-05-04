#include "fileGet.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileGet( rcComm_t *conn, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf )
 *
 * \brief Get a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileGetInp
 * \param[out] fileGetOutBBuf - the out buffer
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileGet( rcComm_t *conn, fileOpenInp_t *fileGetInp,
           bytesBuf_t *fileGetOutBBuf ) {
    int status;
    status = procApiRequest( conn, FILE_GET_AN, fileGetInp, NULL,
                             ( void ** ) NULL, fileGetOutBBuf );

    return status;
}
