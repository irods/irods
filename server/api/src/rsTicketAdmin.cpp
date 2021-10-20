#include "rsTicketAdmin.hpp"

#include "rcConnect.h"
#include "ticketAdmin.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"

int rsTicketAdmin(rsComm_t* rsComm, ticketAdminInp_t* ticketAdminInp)
{
    rodsLog(LOG_DEBUG, "ticketAdmin");

    rodsServerHost_t* rodsServerHost{};
    int status = getAndConnRcatHost(rsComm, MASTER_RCAT, nullptr, &rodsServerHost);
    if (status < 0) {
        return status;
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
        std::string svc_role;

        if (const auto ret = get_catalog_service_role(svc_role); !ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if (irods::CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            status = _rsTicketAdmin(rsComm, ticketAdminInp);
        }
        else if (irods::CFG_SERVICE_ROLE_CONSUMER == svc_role) {
            status = SYS_NO_RCAT_SERVER_ERR;
        }
        else {
            rodsLog(LOG_ERROR, "role not supported [%s]", svc_role.c_str());
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        if (strcmp(ticketAdminInp->arg1, "session") == 0) {
            ticketAdminInp->arg3 = rsComm->clientAddr;
        }

        status = rcTicketAdmin(rodsServerHost->conn, ticketAdminInp);
    }

    if (status < 0) {
        rodsLog(LOG_NOTICE, "rsTicketAdmin failed, error %d", status);
    }

    return status;
} // rsTicketAdmin

int _rsTicketAdmin(rsComm_t* rsComm, ticketAdminInp_t* ticketAdminInp)
{
    if (strcmp(ticketAdminInp->arg1, "session") == 0) {
        ruleExecInfo_t rei{};

        rei.rsComm = rsComm;
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;

        const auto ec = applyRule("acTicketPolicy", nullptr, &rei, NO_SAVE_REI);
        rodsLog(LOG_DEBUG, "debug ticket rule status:%d", ec);

        if (ec != 0) {
            return ec;
        }
    }

    return chlModTicket(rsComm, ticketAdminInp->arg1,
                        ticketAdminInp->arg2, ticketAdminInp->arg3,
                        ticketAdminInp->arg4, ticketAdminInp->arg5,
                        &ticketAdminInp->condInput);
} // _rsTicketAdmin

