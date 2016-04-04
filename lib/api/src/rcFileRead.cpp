#include "fileRead.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileRead( rcComm_t *conn, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf )
 *
 * \brief Read a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileReadInp
 * \param[out] fileReadOutBBuf - the file buffer
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileRead( rcComm_t *conn, fileReadInp_t *fileReadInp,
            bytesBuf_t *fileReadOutBBuf ) {
    int status;

    status = procApiRequest( conn, FILE_READ_AN, fileReadInp,
                             NULL, ( void ** ) NULL, fileReadOutBBuf );

    return status;
}

