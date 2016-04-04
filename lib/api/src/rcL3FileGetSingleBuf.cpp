#include "l3FileGetSingleBuf.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcL3FileGetSingleBuf( rcComm_t *conn, int l1descInx, bytesBuf_t *dataObjOutBBuf )
 *
 * \brief Remote call for cross zone single buffer get.
 *
 * \user client
 *
 * \ingroup server_datatransfer
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
*
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] l1descInx
 * \param[out] dataObjOutBBuf
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcL3FileGetSingleBuf( rcComm_t *conn, int l1descInx,
                      bytesBuf_t *dataObjOutBBuf ) {
    int status;
    status = procApiRequest( conn, L3_FILE_GET_SINGLE_BUF_AN, &l1descInx, NULL,
                             ( void ** ) NULL, dataObjOutBBuf );

    return status;
}


