/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See ruleExecMod.h for a description of this API call.*/

#include "miscServerFunct.hpp"
#include "ruleExecMod.h"
#include "rsRuleExecMod.hpp"
#include "icatHighLevelRoutines.hpp"
#include "irods_log.hpp"
#include "irods_configuration_keywords.hpp"

int
rsRuleExecMod( rsComm_t *rsComm, ruleExecModInp_t *ruleExecModInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

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
            status = _rsRuleExecMod( rsComm, ruleExecModInp );
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
        status = rcRuleExecMod( rodsServerHost->conn,
                                ruleExecModInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsRuleExecMod: rcRuleExecMod failed" );
    }
    return status;
}

int
_rsRuleExecMod( rsComm_t *rsComm,
                ruleExecModInp_t *ruleExecModInp ) {
    int status;

    status = chlModRuleExec( rsComm,
                             ruleExecModInp->ruleId,
                             &ruleExecModInp->condInput );
    return status;
}
