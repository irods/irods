#include "irods/rs_delay_rule_unlock.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/stringOpr.h"

auto rs_delay_rule_unlock(RsComm* _comm, DelayRuleUnlockInput* _input) -> int
{
    using log_api = irods::experimental::log::api;

    if (!_comm || !_input) {
        log_api::error("{}: Invalid input: received null pointer.", __func__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (!_input->rule_ids) {
        log_api::error("{}: Rule ID must be a non-empty JSON string.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    rodsServerHost* host_info{};

    try {
        host_info = irods::experimental::catalog::redirect_to_catalog_provider(*_comm, nullptr);
    }
    catch (const irods::exception& e) {
        log_api::error("{}: Catalog provider host resolution error: {}", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }

    if (host_info->localFlag != LOCAL_HOST) {
        log_api::trace("{}: Redirecting request to catalog service provider.", __func__);
        return rc_delay_rule_unlock(host_info->conn, _input);
    }

    //
    // At this point, we assume we're connected to the catalog service provider in the correct zone.
    //

    try {
        const auto ec = chl_delay_rule_unlock(*_comm, _input->rule_ids);

        if (ec < 0) {
            log_api::error("{}: chl_delay_rule_unlock failed with error code [{}].", __func__, ec);
        }

        return ec;
    }
    catch (const irods::exception& e) {
        log_api::error("{}: {}", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }
    catch (const std::exception& e) {
        log_api::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // rs_delay_rule_unlock
