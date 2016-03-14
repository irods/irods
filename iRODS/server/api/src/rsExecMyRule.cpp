#include "reGlobalsExtern.hpp"
#include "execMyRule.h"
#include "reFuncDefs.hpp"
#include "miscServerFunct.hpp"
#include "rcMisc.h"
#include "irods_re_plugin.hpp"

int
rsExecMyRule( rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
              msParamArray_t **outParamArray ) {
    int status;
    ruleExecInfo_t rei;
    char *iFlag;
    int oldReTestFlag = 0, oldReLoopBackFlag = 0;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;

    if ( execMyRuleInp == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsExecMyRule error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remoteFlag = resolveHost( &execMyRuleInp->addr, &rodsServerHost );

    if ( remoteFlag == REMOTE_HOST ) {
        status = remoteExecMyRule( rsComm, execMyRuleInp,
                                   outParamArray, rodsServerHost );
        return status;
    }

    initReiWithDataObjInp( &rei, rsComm, NULL );
    rei.condInputData = &execMyRuleInp->condInput;
    /* need to have a non zero inpParamArray for execMyRule to work */
    if ( execMyRuleInp->inpParamArray == NULL ) {
        execMyRuleInp->inpParamArray =
            ( msParamArray_t * ) malloc( sizeof( msParamArray_t ) );
        memset( execMyRuleInp->inpParamArray, 0, sizeof( msParamArray_t ) );
    }
    rei.msParamArray = execMyRuleInp->inpParamArray;

    if ( ( iFlag = getValByKey( rei.condInputData, "looptest" ) ) != NULL &&
            !strcmp( iFlag, "true" ) ) {
        oldReTestFlag = reTestFlag;
        oldReLoopBackFlag = reLoopBackFlag;
        reTestFlag = LOG_TEST_2;
        reLoopBackFlag = LOOP_BACK_1;
    }

    rstrcpy( rei.ruleName, EXEC_MY_RULE_KW, NAME_LEN );

    std::string my_rule_text   = execMyRuleInp->myRule;
    std::string out_param_desc = execMyRuleInp->outParamDesc;
    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               &rei);
    irods::error err = re_ctx_mgr.exec_rule_text(
                           my_rule_text,
                           execMyRuleInp->inpParamArray,
                           out_param_desc,
                           &rei);
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "%s : %d, %s",
            __FUNCTION__,
            err.code(),
            err.result().c_str()
        );
    }
    return err.code();


    if ( iFlag != NULL ) {
        reTestFlag = oldReTestFlag;
        reLoopBackFlag = oldReLoopBackFlag;
    }

    trimMsParamArray( rei.msParamArray, execMyRuleInp->outParamDesc );

    *outParamArray = rei.msParamArray;
    rei.msParamArray = NULL;

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsExecMyRule : execMyRule error for %s, status = %d",
                 execMyRuleInp->myRule, status );
        return status;
    }

    return status;
}

int
remoteExecMyRule( rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
                  msParamArray_t **outParamArray, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_ERROR,
                 "remoteExecMyRule: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcExecMyRule( rodsServerHost->conn, execMyRuleInp, outParamArray );

    return status;
}

