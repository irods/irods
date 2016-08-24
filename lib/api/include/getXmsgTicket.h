#ifndef GET_XMSG_TICKET_H__
#define GET_XMSG_TICKET_H__

#include "rcConnect.h"
#include "rodsXmsg.h"


#ifdef __cplusplus
extern "C"
#endif
int rcGetXmsgTicket( rcComm_t *conn, getXmsgTicketInp_t *getXmsgTicketInp, xmsgTicketInfo_t **outXmsgTicketInfo );

#endif
