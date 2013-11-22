/* This is script-generated code.  */ 
/* See ticketAdmin.h for a description of this API call.*/

#include "ticketAdmin.h"

int
rcTicketAdmin (rcComm_t *conn, ticketAdminInp_t *ticketAdminInp)
{
    int status;
    status = procApiRequest (conn, TICKET_ADMIN_AN,  ticketAdminInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
