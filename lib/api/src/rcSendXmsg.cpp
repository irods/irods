#include "sendXmsg.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSendXmsg( rcComm_t *conn, sendXmsgInp_t *sendXmsgInp )
 *
 * \brief Send an XMessage.
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
 * \param[in] sendXmsgInp
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSendXmsg( rcComm_t *conn, sendXmsgInp_t *sendXmsgInp ) {
    int status;
    status = procApiRequest( conn, SEND_XMSG_AN, sendXmsgInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
