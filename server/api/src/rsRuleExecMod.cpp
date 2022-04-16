#include "irods/rsRuleExecMod.hpp"

#include "irods/irods_configuration_keywords.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_log.hpp"

int rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp)
{
    rodsServerHost_t* rodsServerHost{};

    int status = getAndConnRcatHost(_rsComm,
                                    MASTER_RCAT,
                                    static_cast<const char*>(nullptr),
                                    &rodsServerHost);
    if (status < 0) {
        return status;
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if (!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            status = _rsRuleExecMod(_rsComm, _ruleExecModInp);
        }
        else if (irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
            status = SYS_NO_RCAT_SERVER_ERR;
        }
        else {
            rodsLog(LOG_ERROR, "role not supported [%s]", svc_role.c_str());
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcRuleExecMod(rodsServerHost->conn, _ruleExecModInp);
    }

    if (status < 0) {
        rodsLog(LOG_NOTICE, "rsRuleExecMod: rcRuleExecMod failed");
    }

    return status;
}

int _rsRuleExecMod(RsComm* _rsComm, RuleExecModifyInput* _ruleExecModInp)
{
    return chlModRuleExec(_rsComm,
                          _ruleExecModInp->ruleId,
                          &_ruleExecModInp->condInput);
}

