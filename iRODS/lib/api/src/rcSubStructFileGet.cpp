#include "subStructFileGet.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileGet( rcComm_t *conn, subFile_t *subFile, bytesBuf_t *subFileGetOutBBuf )
 *
 * \brief Get a subfile of a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subFile
 * \param[out] subFileGetOutBBuf
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileGet( rcComm_t *conn, subFile_t *subFile,
                    bytesBuf_t *subFileGetOutBBuf ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_GET_AN, subFile, NULL,
                             ( void ** ) NULL, subFileGetOutBBuf );

    return status;
}
