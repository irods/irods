#include "irods/rs_genquery2.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#include "irods/genquery2_driver.hpp"
#pragma clang diagnostic pop

#include "irods/genquery2_sql.hpp"

#include "irods/apiHandler.hpp"
#include "irods/catalog_utilities.hpp"
#include "irods/genquery2_table_column_mappings.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_version.h"
#include "irods/rodsConnect.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/icatHighLevelRoutines.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring> // For strdup.
#include <string_view>
#include <vector>

namespace
{
    // clang-format off
    using json    = nlohmann::json;
    using log_api = irods::experimental::log::api;
    // clang-format on
} // anonymous namespace

auto rs_genquery2(RsComm* _comm, Genquery2Input* _input, char** _output) -> int
{
    if (!_comm || !_input || !_output) {
        log_api::error("{}: Invalid input: received null pointer.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    // Redirect to the catalog service provider based on the user-provided zone.
    // This allows clients to query federated zones.
    //
    // If the client did not provide a zone, getAndConnRcatHost() will operate as if the
    // client provided the local zone's name.
    if (_input->zone) {
        log_api::trace("{}: Received: query_string=[{}], zone=[{}]", __func__, _input->query_string, _input->zone);
    }
    else {
        log_api::trace("{}: Received: query_string=[{}], zone=[nullptr]", __func__, _input->query_string);
    }

    *_output = nullptr;

    rodsServerHost* host_info{};

    try {
        host_info = irods::experimental::catalog::redirect_to_catalog_provider(*_comm, _input->zone);
    }
    catch (const irods::exception& e) {
        log_api::error("{}: Catalog provider host resolution error: {}", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }

    // Return an error if the host information does not point to the zone of interest.
    // getAndConnRcatHost() returns the local zone if the target zone does not exist. We must catch
    // this situation to avoid querying the wrong catalog.
    if (_input->zone) {
        const std::string_view resolved_zone = static_cast<zoneInfo*>(host_info->zoneInfo)->zoneName;

        if (resolved_zone != _input->zone) {
            log_api::error("{}: Could not find zone [{}].", __func__, _input->zone);
            return SYS_INVALID_ZONE_NAME;
        }
    }

    if (host_info->localFlag != LOCAL_HOST) {
        log_api::trace("{}: Redirecting request to remote zone [{}].", __func__, _input->zone);
        return rc_genquery2(host_info->conn, _input, _output);
    }

    //
    // At this point, we assume we're connected to the catalog service provider.
    //

    namespace gq = irods::experimental::api::genquery2;

    // Generating the column mappings must happen after a redirect to guarantee the correct
    // mappings are returned. Remember, querying the catalog only happens on a server with direct
    // database access.
    if (1 == _input->column_mappings) {
        try {
            json mappings;
            std::for_each(
                std::begin(gq::column_name_mappings), std::end(gq::column_name_mappings), [&mappings](auto&& _v) {
                    mappings[std::string{_v.first}] = json{{_v.second.table, _v.second.name}};
                });
            *_output = strdup(mappings.dump().c_str());
            return 0;
        }
        catch (const std::exception& e) {
            log_api::error("{}: Could not generate column mappings: {}", __func__, e.what());
            return SYS_LIBRARY_ERROR;
        }
    }

    if (!_input->query_string) {
        log_api::error("{}: Invalid input: received null pointer.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    try {
        gq::options opts;

        {
            // Get the database type string from server_config.json.
            const auto handle = irods::server_properties::instance().map();
            const auto& config = handle.get_json();
            const auto& db = config.at(nlohmann::json::json_pointer{"/plugin_configuration/database"});
            opts.database = std::begin(db).key();
        }

        opts.user_name = _comm->clientUser.userName;
        opts.user_zone = _comm->clientUser.rodsZone;
        opts.admin_mode = irods::is_privileged_client(*_comm);
        opts.default_number_of_rows = 256; // Default to MAX_SQL_ROWS like GenQuery1 for now.

        irods::experimental::genquery2::driver driver;

        if (const auto ec = driver.parse(_input->query_string); ec != 0) {
            log_api::error("{}: Failed to parse GenQuery2 string. [error code=[{}]]", __func__, ec);
            return SYS_LIBRARY_ERROR;
        }

        const auto [sql, values] = gq::to_sql(driver.select, opts);

        log_api::trace("{}: GenQuery2 SQL: [{}]", __func__, sql);

        if (1 == _input->sql_only) {
            *_output = strdup(sql.c_str());
            return 0;
        }

        if (sql.empty()) {
            log_api::error("{}: Could not generate SQL from GenQuery.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        return chl_execute_genquery2_sql(*_comm, sql.c_str(), &values, _output);
    }
    catch (const irods::exception& e) {
        log_api::error("{}: GenQuery2 error: {}", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    }
    catch (const std::exception& e) {
        log_api::error("{}: GenQuery2 error: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // rs_check_auth_credentials
