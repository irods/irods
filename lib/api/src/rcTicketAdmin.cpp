/* This is script-generated code.  */
/* See ticketAdmin.h for a description of this API call.*/

#include "ticketAdmin.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
rcTicketAdmin( rcComm_t *conn, ticketAdminInp_t *ticketAdminInp ) {
    int status;
    status = procApiRequest( conn, TICKET_ADMIN_AN,  ticketAdminInp, nullptr,
                             ( void ** ) nullptr, nullptr );

    return status;
}
