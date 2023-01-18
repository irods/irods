#include "irods/rsRuleExecDel.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/genQuery.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/json_deserialization.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/objMetaOpr.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/rsGenQuery.hpp"
#include "irods/ruleExecSubmit.h"

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace
{
    using log_api = irods::experimental::log::api;

    int _rsRuleExecDel(rsComm_t* rsComm, ruleExecDelInp_t* ruleExecDelInp)
    {
        // If the user is not an administrator, then they must be the creator of the delay rule
        // in order to delete it.
        if (!irods::is_privileged_client(*rsComm)) {
            std::vector<std::string> info;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            if (const int ec = chl_get_delay_rule_info(*rsComm, ruleExecDelInp->ruleExecId, &info); ec < 0) {
                log_api::error("{}: chl_get_delay_rule_info failed, status = {}", __func__, ec);
                return ec;
            }

            try {
                constexpr auto exe_context_column = 11;
                auto rei = irods::to_rule_execution_info(info.at(exe_context_column));
                irods::at_scope_exit free_rei_internals{
                    [&rei] { freeRuleExecInfoInternals(&rei, (FREE_MS_PARAM | FREE_DOINP)); }};

                // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                const std::string_view creator = rei.uoic->userName;
                const std::string_view zone = rei.uoic->rodsZone;
                const auto& client = rsComm->clientUser;
                // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                if (client.userName != creator || client.rodsZone != zone) {
                    log_api::error("{}: User [{}#{}] is not allowed to delete delay rule [{}].",
                                   __func__,
                                   client.userName,
                                   client.rodsZone,
                                   ruleExecDelInp->ruleExecId);
                    return USER_ACCESS_DENIED;
                }
            }
            catch (const irods::exception& e) {
                log_api::error(
                    "{}: Caught exception while deleting delay rule. [{}]", __func__, e.client_display_what());
                return static_cast<int>(e.code());
            }
            catch (const std::exception& e) {
                log_api::error("{}: Caught exception while deleting delay rule. [{}]", __func__, e.what());
                return SYS_LIBRARY_ERROR;
            }
        }

        // Delete the delay rule from the catalog.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        const auto ec = chlDelRuleExec(rsComm, ruleExecDelInp->ruleExecId);

        if (ec < 0) {
            log_api::error("{}: chlDelRuleExec for {} error, status = {}", __func__, ruleExecDelInp->ruleExecId, ec);
        }

        return ec;
    } // _rsRuleExecDel
} // anonymous namespace

auto rsRuleExecDel(rsComm_t* rsComm, ruleExecDelInp_t* ruleExecDelInp) -> int
{
    if (!ruleExecDelInp) {
        log_api::error("{}: Invalid input: null pointer", __func__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    namespace ic = irods::experimental::catalog;

    // Redirect to the catalog service provider so that we can query the database.
    try {
        if (!ic::connected_to_catalog_provider(*rsComm)) {
            log_api::trace("{}: Redirecting request to catalog service provider ...", __func__);
            auto* host_info = ic::redirect_to_catalog_provider(*rsComm);
            return rcRuleExecDel(host_info->conn, ruleExecDelInp);
        }

        ic::throw_if_catalog_provider_service_role_is_invalid();
    }
    catch (const irods::exception& e) {
        log_api::error(e.what());
        return static_cast<int>(e.code());
    }

    return _rsRuleExecDel(rsComm, ruleExecDelInp);
} // rsRuleExecDel
