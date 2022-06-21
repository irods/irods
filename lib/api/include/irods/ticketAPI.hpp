#ifndef TICKET_API_HPP
#define TICKET_API_HPP

struct RcComm;

namespace irods::ticket::administration{

    int createReadTicket(RcComm* conn, char* objPath, char* ticketName);
    int createReadTicket(RcComm* conn, char* objPath);

    int createWriteTicket(RcComm* conn, char* objPath, char* ticketName);
    int createWriteTicket(RcComm* conn, char* objPath);

    int removeUsageRestriction(RcComm* conn, char* ticketName);
    int removeUsageRestriction(RcComm* conn, int ticketID);

    int removeWriteFileRestriction(RcComm* conn, char* ticketName);
    int removeWriteFileRestriction(RcComm* conn, int ticketID);

    int removeWriteByteRestriction(RcComm* conn, char* ticketName);
    int removeWriteByteRestriction(RcComm* conn, int ticketID);

    int setUsageRestriction(RcComm* conn, char* ticketName, int numUses);
    int setUsageRestriction(RcComm* conn, int ticketID, int numUses);

    int setWriteFileRestriction(RcComm* conn, char* ticketName, int numUses);
    int setWriteFileRestriction(RcComm* conn, int ticketID, int numUses);

    int setWriteByteRestriction(RcComm* conn, char* ticketName, int numUses);
    int setWriteByteRestriction(RcComm* conn, int ticketID, int numUses);

    

}

#endif