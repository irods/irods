#include "irods/ticketAdmin.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

int rcTicketAdmin(RcComm* conn, TicketAdminInput* ticketAdminInp)
{
    return procApiRequest(conn, TICKET_ADMIN_AN, ticketAdminInp, nullptr, (void**) nullptr, nullptr);
}
