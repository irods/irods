#include "irods/rs_update_replica_access_time.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/update_replica_access_time.h"

auto rs_update_replica_access_time(RsComm* _comm, BytesBuf* _json_input, char** _output) -> int
{
    using log_api = irods::experimental::log::api;

    if (!_comm || !_json_input || !_output) {
        log_api::error("{}: Invalid input: received null pointer.", __func__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rodsServerHost* host_info{};

    try {
        host_info = irods::experimental::catalog::redirect_to_catalog_provider(*_comm, nullptr);
    }
    catch (const irods::exception& e) {
        log_api::error("{}: Catalog provider host resolution error: {}", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }

    const std::string json_input(static_cast<const char*>(_json_input->buf), _json_input->len);

    if (host_info->localFlag != LOCAL_HOST) {
        log_api::trace("{}: Redirecting request to catalog service provider.", __func__);
        return rc_update_replica_access_time(host_info->conn, json_input.c_str(), _output);
    }

    //
    // At this point, we assume we're connected to a catalog service provider in the correct zone.
    //

    try {
        const auto ec = chl_update_replica_access_time(*_comm, json_input.c_str(), _output);

        if (ec < 0) {
            log_api::error("{}: chl_update_replica_access_time failed with error code [{}].", __func__, ec);
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
} // rs_update_replica_access_time
