#include "ticketAdmin.h"

#include "apiNumber.h"
#include "procApiRequest.h"

int rcTicketAdmin(RcComm* conn, TicketAdminInput* ticketAdminInp)
{
    return procApiRequest(conn, TICKET_ADMIN_AN, ticketAdminInp, nullptr, (void**) nullptr, nullptr);
}
