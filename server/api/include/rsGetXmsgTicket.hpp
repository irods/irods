#ifndef RS_GET_XMSG_TICKET_HPP
#define RS_GET_XMSG_TICKET_HPP

#include "rodsConnect.h"
#include "rodsXmsg.h"

int rsGetXmsgTicket( rsComm_t *rsComm, getXmsgTicketInp_t *getXmsgTicketInp, xmsgTicketInfo_t **outXmsgTicketInfo );

#endif
