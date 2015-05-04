#include "streamRead.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStreamRead( rcComm_t *conn, fileReadInp_t *streamReadInp, bytesBuf_t *streamReadOutBBuf )
 *
 * \brief Read an incoming stream.
 *
 * \user client
 *
 * \ingroup rules
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
*
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] streamReadInp
 * \param[out] streamReadOutBBuf
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcStreamRead( rcComm_t *conn, fileReadInp_t *streamReadInp,
              bytesBuf_t *streamReadOutBBuf ) {
    int status;
    status = procApiRequest( conn, STREAM_READ_AN, streamReadInp, NULL,
                             ( void ** ) NULL, streamReadOutBBuf );

    return status;
}
