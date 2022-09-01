#include "irods/administration_utilities.hpp"
#include "irods/userAdmin.h"
#include "irods/rsUserAdmin.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_configuration_keywords.hpp"

using log_api = irods::experimental::log::api;

int
rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    log_api::debug("userAdmin");

    status = getAndConnRcatHost(rsComm, PRIMARY_RCAT, (const char*) NULL, &rodsServerHost);
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

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsUserAdmin( rsComm, userAdminInp );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            log_api::error("role not supported [{}]", svc_role.c_str());
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        status = rcUserAdmin( rodsServerHost->conn,
                              userAdminInp );
    }

    if ( status < 0 ) {
        log_api::info("rsUserAdmin: rcUserAdmin failed");
    }
    return status;
}

int
_rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp ) {
    int status, status2;

    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];

    int argc;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }

    log_api::debug("_rsUserAdmin arg0={}", userAdminInp->arg0);

    if ( strcmp( userAdminInp->arg0, "userpw" ) == 0 ) {
        args[0] = userAdminInp->arg1; /* username */
        args[1] = userAdminInp->arg2; /* option */
        args[2] = userAdminInp->arg3; /* newValue */
        argc = 3;
        status2 = applyRuleArg( "acPreProcForModifyUser",
                                args, argc, &rei2, NO_SAVE_REI );
        if ( status2 < 0 ) {
            if ( rei2.status < 0 ) {
                status2 = rei2.status;
            }
            log_api::error(
                "_rsUserAdmin: acPreProcForModifyUser failed [{}] [{}] [{}]",
                userAdminInp->arg1,
                userAdminInp->arg2,
                status2);
            return status2;
        }
        status = chlModUser( rsComm,
                             userAdminInp->arg1,
                             userAdminInp->arg2,
                             userAdminInp->arg3 );
        if ( status != 0 ) {
            chlRollback( rsComm );
        }

        status2 = applyRuleArg( "acPostProcForModifyUser", args, argc,
                                &rei2, NO_SAVE_REI );
        if ( status2 < 0 ) {
            if ( rei2.status < 0 ) {
                status2 = rei2.status;
            }
            log_api::error(
                "rsUserAdmin:acPreProcForModifyUser error for {} and option {},stat={}",
                userAdminInp->arg1,
                userAdminInp->arg2,
                status2);
            return status2;
        }
        return status;
    }
    if ( strcmp( userAdminInp->arg0, "modify" ) == 0 ) {
        if ( strcmp( userAdminInp->arg1, "group" ) == 0 ) {
            args[0] = userAdminInp->arg2; /* groupname */
            args[1] = userAdminInp->arg3; /* option */
            args[2] = userAdminInp->arg4; /* username */
            args[3] = userAdminInp->arg5; /* zonename */
            argc = 4;
            status2 = applyRuleArg( "acPreProcForModifyUserGroup",
                                    args, argc, &rei2, NO_SAVE_REI );
            if ( status2 < 0 ) {
                if ( rei2.status < 0 ) {
                    status2 = rei2.status;
                }
                log_api::error(
                    "rsUserAdmin:acPreProcForModifyUserGroup error for {} and option {},stat={}",
                    args[0],
                    args[1],
                    status2);
                return status2;
            }

            status = chlModGroup( rsComm, userAdminInp->arg2,
                                  userAdminInp->arg3, userAdminInp->arg4,
                                  userAdminInp->arg5 );
            if ( status == 0 ) {
                status2 = applyRuleArg( "acPostProcForModifyUserGroup", args, argc,
                                        &rei2, NO_SAVE_REI );
                if ( status2 < 0 ) {
                    if ( rei2.status < 0 ) {
                        status2 = rei2.status;
                    }
                    log_api::error(
                        "rsUserAdmin:acPostProcForModifyUserGroup error for {} and option {},stat={}",
                        args[0],
                        args[1],
                        status2);
                    return status2;
                }
            }
            return status;
        }
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if ( strcmp( userAdminInp->arg0, "mkuser" ) == 0 ) {
        return irods::create_user(*rsComm,
                                  userAdminInp->arg1 ? userAdminInp->arg1 : "",
                                  "rodsuser", "",
                                  userAdminInp->arg3 ? userAdminInp->arg3 : "");
    }
    if ( strcmp( userAdminInp->arg0, "mkgroup" ) == 0 ) {
        return irods::create_user(*rsComm,
                                  userAdminInp->arg1 ? userAdminInp->arg1 : "",
                                  userAdminInp->arg2 ? userAdminInp->arg2 : "",
                                  "",
                                  userAdminInp->arg3 ? userAdminInp->arg3 : "");
    }

    // =-=-=-=-=-=-=-
    return CAT_INVALID_ARGUMENT;
}

/*
   This is the function used by chlModUser to run a rule to check a
   password.  It is in this source file instead of
   icatHighLevelRoutines.c so that test programs and define a dummy
   version of this and avoid linking with, and possibly trying to use,
   the Rule Engine.
*/

int
icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 ) {
    ruleExecInfo_t rei;
    int status;
    const char *args[2];

    log_api::debug("icatApplyRule called");
    memset( ( char* )&rei, 0, sizeof( rei ) );
    args[0] = arg1;
    rei.rsComm = rsComm;
    rei.uoic = &rsComm->clientUser;
    rei.uoip = &rsComm->proxyUser;
    status =  applyRuleArg( ruleName,
                            args, 1, &rei, NO_SAVE_REI );
    return status;
}
