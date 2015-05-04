#include "subStructFileRead.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileRead( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReadInp, bytesBuf_t *subStructFileReadOutBBuf )
 *
 * \brief Read a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileReadInp
 * \param[out] subStructFileReadOutBBuf
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileRead( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReadInp,
                     bytesBuf_t *subStructFileReadOutBBuf ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_READ_AN, subStructFileReadInp, NULL,
                             ( void ** ) NULL, subStructFileReadOutBBuf );

    return status;
}
