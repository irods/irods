/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See userAdmin.h for a description of this API call.*/

#include "reFuncDefs.hpp"
#include "userAdmin.hpp"
#include "reGlobalsExtern.hpp"
#include "icatHighLevelRoutines.hpp"

int
rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "userAdmin" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsUserAdmin( rsComm, userAdminInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcUserAdmin( rodsServerHost->conn,
                              userAdminInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsUserAdmin: rcUserAdmin failed" );
    }
    return status;
}

#ifdef RODS_CAT
int
_rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp ) {
    int status, status2;

    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    char errString1[] =
        "rsUserAdmin:acPreProcForModifyUser error for %s and option %s,stat=%d";
    char errString2[] =
        "rsUserAdmin:acPreProcForModifyUserGroup error for %s and option %s,stat=%d";
    char errString3[] =
        "rsUserAdmin:acPostProcForModifyUserGroup error for %s and option %s,stat=%d";

    int argc;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }

    rodsLog( LOG_DEBUG,
             "_rsUserAdmin arg0=%s",
             userAdminInp->arg0 );

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
            rodsLog( LOG_ERROR,
                     userAdminInp->arg1, userAdminInp->arg2, status2 );
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
            rodsLog( LOG_ERROR, errString1,
                     userAdminInp->arg1, userAdminInp->arg2, status2 );
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
                rodsLog( LOG_ERROR, errString2, args[0], args[1], status2 );
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
                    rodsLog( LOG_ERROR, errString3, args[0], args[1], status2 );
                    return status2;
                }
            }
            return status;
        }
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if ( strcmp( userAdminInp->arg0, "mkuser" ) == 0 ) {
        /* run the acCreateUser rule */
        ruleExecInfo_t rei;
        userInfo_t userInfo;
        memset( ( char* )&rei, 0, sizeof( rei ) );
        memset( ( char* )&userInfo, 0, sizeof( userInfo ) );
        rei.rsComm = rsComm;
        snprintf( userInfo.userName, sizeof( userInfo.userName ),
                  "%s", userAdminInp->arg1 );
        snprintf( userInfo.userType, sizeof( userInfo.userType ),
                  "%s", "rodsuser" );
        rei.uoio = &userInfo;
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;
        status = applyRuleArg( "acCreateUser", NULL, 0, &rei, SAVE_REI );
        if ( status != 0 ) {
            chlRollback( rsComm );
            return status;
        }
        /* And then the chlModUser function to set the initial password */
        status = chlModUser( rsComm,
                             userAdminInp->arg1,
                             "password",
                             userAdminInp->arg2 );
        if ( status != 0 ) {
            chlRollback( rsComm );
        }
        return status;
    }
    if ( strcmp( userAdminInp->arg0, "mkgroup" ) == 0 ) {
        /* run the acCreateUser rule */
        ruleExecInfo_t rei;
        userInfo_t userInfo;
        memset( ( char* )&rei, 0, sizeof( rei ) );
        rei.rsComm = rsComm;
        snprintf( userInfo.userName, sizeof( userInfo.userName ),
                  "%s", userAdminInp->arg1 );
        snprintf( userInfo.userType, sizeof( userInfo.userType ),
                  "%s", userAdminInp->arg2 );
        snprintf( userInfo.rodsZone, sizeof( userInfo.rodsZone ),
                  "%s", userAdminInp->arg3 );
        snprintf( userInfo.authInfo.authStr, sizeof( userInfo.authInfo.authStr ),
                  "%s", userAdminInp->arg4 );
        rei.uoio = &userInfo;
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;
        status = applyRuleArg( "acCreateUser", NULL, 0, &rei, SAVE_REI );
        if ( status != 0 ) {
            chlRollback( rsComm );
        }
        return status;
    }

    // =-=-=-=-=-=-=-
    return CAT_INVALID_ARGUMENT;
}
#endif

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

    rodsLog( LOG_NOTICE, "icatApplyRule called" );
    memset( ( char* )&rei, 0, sizeof( rei ) );
    args[0] = arg1;
    rei.rsComm = rsComm;
    rei.uoic = &rsComm->clientUser;
    rei.uoip = &rsComm->proxyUser;
    status =  applyRuleArg( ruleName,
                            args, 1, &rei, NO_SAVE_REI );
    return status;
}
