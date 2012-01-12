/* This is script-generated code.  */ 
/* See getXmsgTicket.h for a description of this API call.*/

#include "getXmsgTicket.h"

int
rcGetXmsgTicket (rcComm_t *conn, getXmsgTicketInp_t *getXmsgTicketInp, 
xmsgTicketInfo_t **outXmsgTicketInfo)
{
    int status;
    status = procApiRequest (conn, GET_XMSG_TICKET_AN, getXmsgTicketInp, NULL, 
        (void **) outXmsgTicketInfo, NULL);

    return (status);
}
