/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See ticketAdmin.h for a description of this API call.*/

#include "ticketAdmin.h"
#include "rsTicketAdmin.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"

int
rsTicketAdmin( rsComm_t *rsComm, ticketAdminInp_t *ticketAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "ticketAdmin" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsTicketAdmin( rsComm, ticketAdminInp );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        if ( strcmp( ticketAdminInp->arg1, "session" ) == 0 ) {
            ticketAdminInp->arg3 = rsComm->clientAddr;
        }
        status = rcTicketAdmin( rodsServerHost->conn,
                                ticketAdminInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsTicketAdmin failed, error %d", status );
    }
    return status;
}

int
_rsTicketAdmin( rsComm_t *rsComm, ticketAdminInp_t *ticketAdminInp ) {
    int status;
    if ( strcmp( ticketAdminInp->arg1, "session" ) == 0 ) {
        ruleExecInfo_t rei;

        memset( ( char* )&rei, 0, sizeof( rei ) );
        rei.rsComm = rsComm;
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;
        status = applyRule( "acTicketPolicy", NULL, &rei, NO_SAVE_REI );
        rodsLog( LOG_DEBUG, "debug ticket rule status:%d", status );
        if ( status != 0 ) {
            return status;
        }
    }
    status = chlModTicket( rsComm, ticketAdminInp->arg1,
                           ticketAdminInp->arg2, ticketAdminInp->arg3,
                           ticketAdminInp->arg4, ticketAdminInp->arg5 );
    return status;
}
