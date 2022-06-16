#ifndef TICKET_API_HPP
#define TICKET_API_HPP

struct RcComm;

namespace irods::ticket::administration{
    int createReadTicket(RcComm* conn, char* objPath, char* ticketName);
}

#endif