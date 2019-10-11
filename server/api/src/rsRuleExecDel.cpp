#include "rcMisc.h"
#include "ruleExecSubmit.h"
#include "ruleExecDel.h"
#include "objMetaOpr.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "genQuery.h"
#include "rsRuleExecDel.hpp"
#include "rsGenQuery.hpp"

int
getReInfoById( rsComm_t *rsComm, char *ruleExecId, genQueryOut_t **genQueryOut ) {
    genQueryInp_t genQueryInp;
    char tmpStr[NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_REI_FILE_PATH, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_USER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ADDRESS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_FREQUENCY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_PRIORITY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ESTIMATED_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NOTIFICATION_ADDR, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_LAST_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_STATUS, 1 );

    snprintf( tmpStr, NAME_LEN, "='%s'", ruleExecId );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_EXEC_ID, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, &genQueryInp, genQueryOut );

    clearGenQueryInp( &genQueryInp );

    return status;
}

int
rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    if ( ruleExecDelInp == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsRuleExecDel error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    status = getAndConnReHost( rsComm, &rodsServerHost );
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
            status = _rsRuleExecDel( rsComm, ruleExecDelInp );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE,
                     "rsRuleExecDel error. ICAT is not configured on this host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcRuleExecDel( rodsServerHost->conn, ruleExecDelInp );
    }
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsRuleExecDel: rcRuleExecDel failed, status = %d",
                 status );
    }
    return status;
}

int _rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp ) {
    genQueryOut_t *genQueryOut = NULL;
    int status;
    sqlResult_t *reiFilePath;
    sqlResult_t *ruleUserName;

    char reiDir[MAX_NAME_LEN];

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    status = getReInfoById( rsComm, ruleExecDelInp->ruleExecId,
                            &genQueryOut );


    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: getReInfoById failed, status = %d",
                 status );
        /* unregister it anyway */


        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsRuleExecDel: chlDelRuleExec for %s error, status = %d",
                         ruleExecDelInp->ruleExecId, status );
            }
            return status;
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_ERROR,
                     "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            const auto err{ERROR(SYS_SERVICE_ROLE_NOT_SUPPORTED,
                                 (boost::format("role not supported [%s]") %
                                  svc_role.c_str()).str().c_str())};
            irods::log(err);
            return err.code();
        }
    }

    if ( ( reiFilePath = getSqlResultByInx( genQueryOut,
                                            COL_RULE_EXEC_REI_FILE_PATH ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "_rsRuleExecDel: getSqlResultByInx for REI_FILE_PATH failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    /* First check permission (now that API is allowed for non-admin users) */
    if ( rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        if ( rsComm->proxyUser.authInfo.authFlag == LOCAL_USER_AUTH ) {
            if ( ( ruleUserName = getSqlResultByInx( genQueryOut,
                                  COL_RULE_EXEC_USER_NAME ) ) == NULL ) {
                rodsLog( LOG_NOTICE,
                         "_rsRuleExecDel: getSqlResultByInx for COL_RULE_EXEC_USER_NAME failed" );
                return UNMATCHED_KEY_OR_INDEX;
            }
            if ( strncmp( ruleUserName->value,
                          rsComm->clientUser.userName, MAX_NAME_LEN ) != 0 ) {
                return USER_ACCESS_DENIED;
            }
        }
        else {
            return USER_ACCESS_DENIED;
        }
    }

    /* some sanity check */
    snprintf( reiDir, MAX_NAME_LEN,
              "/%-s/%-s.", PACKED_REI_DIR, REI_FILE_NAME );

    if ( strstr( reiFilePath->value, reiDir ) == NULL ) {
        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            int i;
            char errMsg[105];

            rodsLog( LOG_NOTICE,
                     "_rsRuleExecDel: reiFilePath: %s is not a proper rei path",
                     reiFilePath->value );

            /* Try to unregister it anyway */
            status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );

            if ( status ) {
                return ( status );   /* that failed too, report it */
            }

            /* Add a message to the error stack for the client user */
            snprintf( errMsg, sizeof errMsg, "Rule was removed but reiPath was invalid: %s",
                      reiFilePath->value );
            i = addRErrorMsg( &rsComm->rError, 0, errMsg );
            if ( i < 0 ) {
                irods::log( ERROR( i, "addRErrorMsg failed" ) );
            }
            freeGenQueryOut( &genQueryOut );

            return SYS_INVALID_FILE_PATH;
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_ERROR,
                     "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            const auto err{ERROR(SYS_SERVICE_ROLE_NOT_SUPPORTED,
                                 (boost::format("role not supported [%s]") %
                                  svc_role.c_str()).str().c_str())};
            irods::log(err);
            return err.code();
        }
    }
    status = unlink( reiFilePath->value );
    if ( status < 0 ) {
        status = UNIX_FILE_UNLINK_ERR - errno;
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: unlink of %s error, status = %d",
                 reiFilePath->value, status );
        if ( errno != ENOENT ) {
            freeGenQueryOut( &genQueryOut );
            return status;
        }
    }
    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int unlinkStatus = status;

        /* unregister it */
        status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsRuleExecDel: chlDelRuleExec for %s error, status = %d",
                     ruleExecDelInp->ruleExecId, status );
        }
        if ( unlinkStatus ) {
            int i;
            char errMsg[105];
            /* Add a message to the error stack for the client user */
            snprintf( errMsg, sizeof errMsg,
                      "Rule was removed but unlink of rei file failed" );
            i = addRErrorMsg( &rsComm->rError, 0, errMsg );
            if ( i < 0 ) {
                irods::log( ERROR( i, "addRErrorMsg failed" ) );
            }
            snprintf( errMsg, sizeof errMsg,
                      "rei file: %s",
                      reiFilePath->value );
            i = addRErrorMsg( &rsComm->rError, 1, errMsg );
            if (i < 0) {
                irods::log(ERROR(i, "addRErrorMsg failed"));
            }
            if ( status == 0 ) {
                /* return this error if no other error occurred */
                status = unlinkStatus;
            }

        }
        freeGenQueryOut( &genQueryOut );

        return status;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        const auto err{ERROR(SYS_SERVICE_ROLE_NOT_SUPPORTED,
                             (boost::format("role not supported [%s]") %
                              svc_role.c_str()).str().c_str())};
        irods::log(err);
        return err.code();
    }
}
