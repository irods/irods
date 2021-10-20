#ifndef IRODS_RS_TICKET_ADMIN_HPP
#define IRODS_RS_TICKET_ADMIN_HPP

#include "ticketAdmin.h"

struct RsComm;
struct TicketAdminInput;

int rsTicketAdmin(RsComm* rsComm, TicketAdminInput* ticketAdminInp);

int _rsTicketAdmin(RsComm* rsComm, TicketAdminInput* ticketAdminInp);

#endif // IRODS_RS_TICKET_ADMIN_HPP
