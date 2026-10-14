#include "irods/rsRuleExecSubmit.hpp"
#include "irods/ruleExecSubmit.h"

#include "irods/catalog_utilities.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/json_serialization.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/server_utilities.hpp"

#include <nlohmann/json.hpp>

#include <charconv>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
    using json = nlohmann::json;
    using log_api = irods::experimental::log::api;

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
        irods::at_scope_exit free_rei_info{[&rei_info] {
            if (rei_info) {
                freeRuleExecInfoStructFull(rei_info->rei);
                std::free(rei_info);
            }
        }};

        if (const auto ec = unpackReiAndArg(rsComm, &rei_info, ruleExecSubmitInp->packedReiAndArgBBuf); ec < 0) {
            rodsLog(LOG_ERROR, "_rsRuleExecSubmit: Could not unpack REI buffer [error_code=%i].", ec);
            return ec;
        }

        // EMPTY_REI_PATH is a note to the DBA that this column is not used and
        // is reserved for future use.
        rstrcpy(ruleExecSubmitInp->reiFilePath, "EMPTY_REI_PATH", sizeof(ruleExecSubmitInp_t::reiFilePath));

        irods::experimental::key_value_proxy kvp{ruleExecSubmitInp->condInput};
        kvp[RULE_EXECUTION_CONTEXT_KW] = irods::to_json(rei_info->rei).dump().data();
        irods::at_scope_exit clear_cond_input{[&kvp] { kvp.clear(); }};

        // Verify that the priority is valid if the client provided one.
        const std::string_view rule_prio_str{ruleExecSubmitInp->priority};
        if (rule_prio_str.empty()) {
            // The client did not provide a priority, use the default value.
            rstrcpy(ruleExecSubmitInp->priority, "5", sizeof(ruleExecSubmitInp_t::priority));
        }
        else {
            std::int_fast8_t rule_prio; // NOLINT(cppcoreguidelines-init-variables)
            const auto* rule_prio_b = rule_prio_str.data();
            const auto* rule_prio_e =
                rule_prio_b + rule_prio_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto [ptr, ec] = std::from_chars(rule_prio_b, rule_prio_e, rule_prio);
            if (std::errc{} != ec || rule_prio < 1 || rule_prio > 9) {
                log_api::error("Delay rule priority must satisfy the following requirement: 1 <= P <= 9.");
                return SYS_INVALID_INPUT_PARAM;
            }
        }

        // Register the request.
        std::string svc_role;
        if (const auto err = get_catalog_service_role(svc_role); !err.ok()) {
            irods::log(PASS(err));
            return err.code();
        }

        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            const auto status = chlRegRuleExec(rsComm, ruleExecSubmitInp);

            if (status < 0) {
                rodsLog(LOG_ERROR, "_rsRuleExecSubmit: chlRegRuleExec error. status = %d", status);
            }

            return status;
        }

        if (irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
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

