#include "fileWrite.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileWrite( rcComm_t *conn, fileWriteInp_t *fileWriteInp, bytesBuf_t *fileWriteInpBBuf )
 *
 * \brief Write a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileWriteInp - the file input
 * \param[out] fileWriteInpBBuf - the input buffer
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileWrite( rcComm_t *conn, fileWriteInp_t *fileWriteInp,
             bytesBuf_t *fileWriteInpBBuf ) {
    int status;

    status = procApiRequest( conn, FILE_WRITE_AN, fileWriteInp,
                             fileWriteInpBBuf, ( void ** ) NULL, NULL );

    return status;
}

