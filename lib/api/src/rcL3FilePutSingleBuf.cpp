#include "l3FilePutSingleBuf.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcL3FilePutSingleBuf( rcComm_t *conn, int l1descInx, bytesBuf_t *dataObjInBBuf)
 *
 * \brief Remote call for cross zone single buffer put.
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
 * \param[out] dataObjInBBuf
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcL3FilePutSingleBuf( rcComm_t *conn, int l1descInx,
                      bytesBuf_t *dataObjInBBuf ) {
    int status;
    status = procApiRequest( conn, L3_FILE_PUT_SINGLE_BUF_AN,
                             &l1descInx, dataObjInBBuf, ( void ** ) NULL, NULL );

    return status;
}


