#include "rsRuleExecSubmit.hpp"

#include "rodsErrorTable.h"
#include "rodsConnect.h"
#include "icatHighLevelRoutines.hpp"
#include "rcMisc.h"
#include "miscServerFunct.hpp"

#include "irods_log.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_random.hpp"
#include "irods_configuration_keywords.hpp"
#include "key_value_proxy.hpp"
#include "catalog_utilities.hpp"
#include "ruleExecSubmit.h"
#include "server_utilities.hpp"
#include "json_serialization.hpp"

#include <json.hpp>

#include <cstring>
#include <string>

namespace
{
    using json = nlohmann::json;

    auto is_input_valid(const ruleExecSubmitInp_t* _input) noexcept -> bool
    {
        return _input &&
               _input->packedReiAndArgBBuf &&
               _input->packedReiAndArgBBuf->buf &&
               _input->packedReiAndArgBBuf->len > 0;
    }

    auto _rsRuleExecSubmit(RsComm* rsComm, ruleExecSubmitInp_t* ruleExecSubmitInp) -> int
    {
        // Do not allow clients to schedule delay rules with session variables in them.
        // This function will reject rules that have session variables in comments as well.
        if (irods::contains_session_variables(ruleExecSubmitInp->ruleName)) {
            rodsLog(LOG_ERROR, "Rules cannot contain session variables. Use dynamic PEPs instead.");
            return RE_UNSUPPORTED_SESSION_VAR;
        }

        ruleExecInfoAndArg_t* rei_info{};
        if (const auto ec = unpackReiAndArg(rsComm, &rei_info, ruleExecSubmitInp->packedReiAndArgBBuf); ec < 0) {
            rodsLog(LOG_ERROR, "_rsRuleExecSubmit: Could not unpack REI buffer [error_code=%i].", ec);
            return ec;
        }

        // EMPTY_REI_PATH is a note to the DBA that this column is not used and
        // is reserved for future use.
        rstrcpy(ruleExecSubmitInp->reiFilePath, "EMPTY_REI_PATH", sizeof(ruleExecSubmitInp_t::reiFilePath));

        irods::experimental::key_value_proxy kvp{ruleExecSubmitInp->condInput};
        kvp[RULE_EXECUTION_CONTEXT_KW] = irods::to_json(rei_info->rei).dump().data();

        // Verify that the priority is valid if the client provided one.
        if (std::strlen(ruleExecSubmitInp->priority) > 0) {
            try {
                if (const auto p = std::stoi(ruleExecSubmitInp->priority); p < 1 || p > 9) {
                    rodsLog(LOG_ERROR, "Delay rule priority must satisfy the following requirement: 1 <= P <= 9.");
                    return SYS_INVALID_INPUT_PARAM;
                }
            }
            catch (...) {
                rodsLog(LOG_ERROR, "Delay rule priority is not an integer.");
                return SYS_INVALID_INPUT_PARAM;
            }
        }
        else {
            // The client did not provide a priority, use the default value.
            rstrcpy(ruleExecSubmitInp->priority, "5", sizeof(ruleExecSubmitInp_t::priority));
        }

        // Register the request.
        std::string svc_role;
        if (const auto err = get_catalog_service_role(svc_role); !err.ok()) {
            irods::log(PASS(err));
            return err.code();
        }

        if (irods::CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            const auto status = chlRegRuleExec(rsComm, ruleExecSubmitInp);

            if (status < 0) {
                rodsLog(LOG_ERROR, "_rsRuleExecSubmit: chlRegRuleExec error. status = %d", status);
            }

            return status;
        }

        if (irods::CFG_SERVICE_ROLE_CONSUMER == svc_role) {
            rodsLog(LOG_ERROR, "_rsRuleExecSubmit error. ICAT is not configured on this host");
            return SYS_NO_ICAT_SERVER_ERR;
        }

        rodsLog(LOG_ERROR, "role not supported [%s]", svc_role.c_str());
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
} // anonymous namespace

int rsRuleExecSubmit(RsComm* rsComm,
                     ruleExecSubmitInp_t* ruleExecSubmitInp,
                     char** ruleExecId)
{
    *ruleExecId = nullptr;

    if (!is_input_valid(ruleExecSubmitInp)) {
        rodsLog(LOG_ERROR, "rsRuleExecSubmit: Invalid input (null pointer or empty buffer)");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    namespace ic = irods::experimental::catalog;

    if (!ic::connected_to_catalog_provider(*rsComm)) {
        auto kvp = irods::experimental::make_key_value_proxy(ruleExecSubmitInp->condInput);

        if (kvp.contains(EXEC_LOCALLY_KW)) {
            rodsLog(LOG_ERROR, "rsRuleExecSubmit: ReHost config error. ReServer not running locally.");
            return SYS_CONFIG_FILE_ERR;
        }

        kvp[EXEC_LOCALLY_KW] = "";

        auto* host_info = ic::redirect_to_catalog_provider(*rsComm);

        return rcRuleExecSubmit(host_info->conn, ruleExecSubmitInp, ruleExecId);
    }

    if (const auto ec = _rsRuleExecSubmit(rsComm, ruleExecSubmitInp); ec < 0) {
        return ec;
    }

    *ruleExecId = strdup(ruleExecSubmitInp->ruleExecId);

    return 0;
}

