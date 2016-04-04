#include "getXmsgTicket.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGetXmsgTicket( rcComm_t *conn, getXmsgTicketInp_t *getXmsgTicketInp, xmsgTicketInfo_t **outXmsgTicketInfo )
 *
 * \brief Get an XMessage ticket.
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
 * \param[in] getXmsgTicketInp
 * \param[out] outXmsgTicketInfo
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcGetXmsgTicket( rcComm_t *conn, getXmsgTicketInp_t *getXmsgTicketInp,
                 xmsgTicketInfo_t **outXmsgTicketInfo ) {
    int status;
    status = procApiRequest( conn, GET_XMSG_TICKET_AN, getXmsgTicketInp, NULL,
                             ( void ** ) outXmsgTicketInfo, NULL );

    return status;
}
