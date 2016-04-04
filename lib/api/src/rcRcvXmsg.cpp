#include "rcvXmsg.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcRcvXmsg( rcComm_t *conn, rcvXmsgInp_t *rcvXmsgInp, rcvXmsgOut_t **rcvXmsgOut )
 *
 * \brief Receive an XMessage.
 *
 * \user client
 *
 * \ingroup xmessage
 *
 * \since 1.0
 *
 * \author Arcot Rajasekar
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] rcvXmsgInp
 * \param[out] rcvXmsgOut
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcRcvXmsg( rcComm_t *conn, rcvXmsgInp_t *rcvXmsgInp,
           rcvXmsgOut_t **rcvXmsgOut ) {
    int status;
    status = procApiRequest( conn, RCV_XMSG_AN, rcvXmsgInp, NULL,
                             ( void ** ) rcvXmsgOut, NULL );

    return status;
}
