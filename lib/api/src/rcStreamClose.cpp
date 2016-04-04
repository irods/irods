#include "streamClose.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStreamClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp )
 *
 * \brief Close an existing stream.
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
 * \param[in] fileCloseInp
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcStreamClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp ) {
    int status;
    status = procApiRequest( conn, STREAM_CLOSE_AN, fileCloseInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
