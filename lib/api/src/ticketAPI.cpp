#include "irods/ticketAPI.hpp"

#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/ticketAdmin.h"
#include <string>

namespace irods::ticket::administration{
    int login(RcComm* conn){
        return clientLogin(conn);
    }

    int createReadTicket(RcComm* conn, char* objPath, char* ticketName){
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "create";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "read";
        ticketAdminInp.arg4 = strdup(objPath);
        ticketAdminInp.arg5 = strdup(ticketName);
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp);
    };

    int createWriteTicket(RcComm* conn, char* objPath, char* ticketName){
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "create";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "write";
        ticketAdminInp.arg4 = strdup(objPath);
        ticketAdminInp.arg5 = strdup(ticketName);
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp);
    };
}