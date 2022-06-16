#include "irods/ticketAPI.hpp"

#include "irods/objInfo.h"
#include "irods/ticketAdmin.h"
#include <string>

namespace irods::ticket::administration{
    int createReadTicket(RcComm* conn, char* objPath, char* ticketName){
        ticketAdminInp_t ticketAdminInp{};

        ticketAdminInp.arg1 = "create";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "read";
        ticketAdminInp.arg4 = strdup(objPath);
        ticketAdminInp.arg5 = strdup(ticketName);
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp);
    }
}