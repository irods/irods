#include "rsRuleExecDel.hpp"

#include "catalog_utilities.hpp"
#include "genQuery.h"
#include "icatHighLevelRoutines.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_rs_comm_query.hpp"
#include "json_deserialization.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "rsGenQuery.hpp"

#include "fmt/format.h"

#include <string>
#include <string_view>

namespace
{
    int _rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp )
    {
        // If the user is not an administrator, then they must be the creator of the delay rule
        // in order to delete it.
        if (!irods::is_privileged_client(*rsComm)) {
            std::vector<std::string> info;

            if (const int ec = chl_get_delay_rule_info(*rsComm, ruleExecDelInp->ruleExecId, &info); ec < 0) {
                rodsLog(LOG_ERROR, "%s: chl_get_delay_rule_info failed, status = %d", __func__, ec);
                return ec;
            }

            try {
                auto rei = irods::to_rule_execution_info(info.at(11));
                irods::at_scope_exit free_rei_internals{[&rei] {
                    freeRuleExecInfoInternals(&rei, (FREE_MS_PARAM | FREE_DOINP));
                }};

                const std::string_view creator = rei.uoic->userName;
                const std::string_view zone = rei.uoic->rodsZone;
                const auto& client = rsComm->clientUser;

                if (client.userName != creator || client.rodsZone != zone) {
                    rodsLog(LOG_ERROR, "%s: User [%s#%s] is not allowed to delete delay rule [%s].",
                            __func__, client.userName, client.rodsZone, ruleExecDelInp->ruleExecId);
                    return USER_ACCESS_DENIED;
                }
            }
            catch (const irods::exception& e) {
                rodsLog(LOG_ERROR, "%s: Caught exception while deleting delay rule. [%s]",
                        __func__, e.client_display_what());
                return e.code();
            }
            catch (const std::exception& e) {
                rodsLog(LOG_ERROR, "%s: Caught exception while deleting delay rule. [%s]",
                        __func__, e.what());
                return SYS_LIBRARY_ERROR;
            }
        }

        // Delete the delay rule from the catalog.
        const auto ec = chlDelRuleExec(rsComm, ruleExecDelInp->ruleExecId);

        if (ec < 0) {
            rodsLog(LOG_ERROR, "%s: chlDelRuleExec for %s error, status = %d",
                    __func__, ruleExecDelInp->ruleExecId, ec);
        }

        return ec;
    } // _rsRuleExecDel
} // anonymous namespace

int rsRuleExecDel(rsComm_t* rsComm, ruleExecDelInp_t* ruleExecDelInp)
{
    if (!ruleExecDelInp) {
        rodsLog(LOG_ERROR, "%s: Invalid input: null pointer", __func__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    namespace ic = irods::experimental::catalog;

    // Redirect to the catalog service provider so that we can query the database.
    try {
        if (!ic::connected_to_catalog_provider(*rsComm)) {
            rodsLog(LOG_DEBUG, "%s: Redirecting request to catalog service provider ...", __func__);
            auto* host_info = ic::redirect_to_catalog_provider(*rsComm);
            return rcRuleExecDel(host_info->conn, ruleExecDelInp);
        }

        ic::throw_if_catalog_provider_service_role_is_invalid();
    }
    catch (const irods::exception& e) {
        rodsLog(LOG_ERROR, e.what());
        return e.code();
    }

    return _rsRuleExecDel(rsComm, ruleExecDelInp);
} // rsRuleExecDel
