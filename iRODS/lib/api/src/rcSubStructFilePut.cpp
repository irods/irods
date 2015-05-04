#include "subStructFilePut.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFilePut( rcComm_t *conn, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf )
 *
 * \brief Put a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subFile
 * \param[out] subFilePutOutBBuf
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFilePut( rcComm_t *conn, subFile_t *subFile,
                    bytesBuf_t *subFilePutOutBBuf ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_PUT_AN, subFile,
                             subFilePutOutBBuf, ( void ** ) NULL, NULL );

    return status;
}
