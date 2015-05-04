#include "subStructFileWrite.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileWrite( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileWriteInp, bytesBuf_t *subStructFileWriteOutBBuf )
 *
 * \brief Write to a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileWriteInp
 * \param[out] subStructFileWriteOutBBuf
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileWrite( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileWriteInp,
                      bytesBuf_t *subStructFileWriteOutBBuf ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_WRITE_AN, subStructFileWriteInp,
                             subStructFileWriteOutBBuf, ( void ** ) NULL, NULL );

    return status;
}
