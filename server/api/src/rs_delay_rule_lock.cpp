#include "irods/rs_delay_rule_lock.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/stringOpr.h"

auto rs_delay_rule_lock(RsComm* _comm, DelayRuleLockInput* _input) -> int
{
    using log_api = irods::experimental::log::api;

    if (!_comm || !_input) {
        log_api::error("{}: Invalid input: received null pointer.", __func__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (!is_non_empty_string(_input->rule_id, sizeof(DelayRuleLockInput::rule_id))) {
        log_api::error("{}: Rule ID must be a non-empty string.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    if (!is_non_empty_string(_input->lock_host, sizeof(DelayRuleLockInput::lock_host))) {
        log_api::error("{}: FQDN must be a non-empty string.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    if (_input->lock_host_pid < 0) {
        log_api::error("{}: Delay server PID must be greater than or equal to 0.", __func__);
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
        return rc_delay_rule_lock(host_info->conn, _input);
    }

    //
    // At this point, we assume we're connected to the catalog service provider in the correct zone.
    //

    try {
        const auto ec = chl_delay_rule_lock(*_comm, _input->rule_id, _input->lock_host, _input->lock_host_pid);

        if (ec < 0) {
            log_api::error("{}: chl_delay_rule_lock failed with error code [{}].", __func__, ec);
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
} // rs_delay_rule_lock
