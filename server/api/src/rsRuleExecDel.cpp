#include "rsRuleExecDel.hpp"

#include "genQuery.h"
#include "icatHighLevelRoutines.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_configuration_keywords.hpp"
#include "miscServerFunct.hpp"
#include "objMetaOpr.hpp"
#include "rcMisc.h"
#include "rsGenQuery.hpp"
#include "ruleExecSubmit.h"

#include "fmt/format.h"

namespace
{
    int get_username_of_delay_rule( rsComm_t *rsComm, char *ruleExecId, genQueryOut_t **genQueryOut )
    {
        genQueryInp_t genQueryInp{};

        addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_USER_NAME, 1 );

        char tmpStr[NAME_LEN];
        snprintf( tmpStr, NAME_LEN, "='%s'", ruleExecId );
        addInxVal( &genQueryInp.sqlCondInp, COL_RULE_EXEC_ID, tmpStr );

        genQueryInp.maxRows = MAX_SQL_ROWS;

        const auto status = rsGenQuery( rsComm, &genQueryInp, genQueryOut );

        clearGenQueryInp( &genQueryInp );

        return status;
    } // get_username_of_delay_rule

    int _rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp )
    {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if (!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        // First check permission (now that API is allowed for non-admin users).
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            if (rsComm->clientUser.authInfo.authFlag == LOCAL_USER_AUTH) {
                genQueryOut_t *genQueryOut = nullptr;
                irods::at_scope_exit free_gen_query_output{[&genQueryOut] { freeGenQueryOut(&genQueryOut); }};

                // Get the username of the user who created the rule.
                const int ec = get_username_of_delay_rule(rsComm, ruleExecDelInp->ruleExecId, &genQueryOut);
                if (ec < 0) {
                    rodsLog(LOG_ERROR, "_rsRuleExecDel: get_username_of_delay_rule failed, status = %d", ec);
                    return ec;
                }

                auto* ruleUserName = getSqlResultByInx(genQueryOut, COL_RULE_EXEC_USER_NAME);
                if (!ruleUserName) {
                    rodsLog(LOG_ERROR, "_rsRuleExecDel: getSqlResultByInx for COL_RULE_EXEC_USER_NAME failed");
                    return UNMATCHED_KEY_OR_INDEX;
                }

                if (strncmp(ruleUserName->value, rsComm->clientUser.userName, MAX_NAME_LEN)!= 0) {
                    return USER_ACCESS_DENIED;
                }
            }
            else {
                return USER_ACCESS_DENIED;
            }
        }

        if (irods::CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            // Unregister rule (i.e. remove the entry from the catalog).
            const auto ec = chlDelRuleExec(rsComm, ruleExecDelInp->ruleExecId);

            if (ec < 0) {
                rodsLog(LOG_ERROR, "_rsRuleExecDel: chlDelRuleExec for %s error, status = %d",
                        ruleExecDelInp->ruleExecId, ec);
            }

            return ec;
        }

        if (irods::CFG_SERVICE_ROLE_CONSUMER == svc_role) {
            rodsLog(LOG_ERROR, "_rsRuleExecDel: chlDelRuleExec must be invoked on the catalog provider host");
            return SYS_NO_RCAT_SERVER_ERR;
        }
        
        const auto err = ERROR(SYS_SERVICE_ROLE_NOT_SUPPORTED,
                               fmt::format("role not supported [{}]", svc_role));
        irods::log(err);

        return err.code();
    } // _rsRuleExecDel
} // anonymous namespace

int rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp )
{
    rodsServerHost_t *rodsServerHost = nullptr;

    if ( ruleExecDelInp == NULL ) {
        rodsLog( LOG_NOTICE, "rsRuleExecDel error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    int status = getAndConnReHost( rsComm, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if (!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsRuleExecDel( rsComm, ruleExecDelInp );
        }
        else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE, "rsRuleExecDel error. ICAT is not configured on this host" );
            return SYS_NO_RCAT_SERVER_ERR;
        }
        else {
            rodsLog( LOG_ERROR, "role not supported [%s]", svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcRuleExecDel( rodsServerHost->conn, ruleExecDelInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rsRuleExecDel: rcRuleExecDel failed, status = %d", status );
    }

    return status;
}

