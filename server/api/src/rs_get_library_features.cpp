#include "irods/rs_get_library_features.hpp"

#include "irods/irods_logger.hpp"
#include "irods/library_features.h"
#include "irods/rodsErrorTable.h"

#include <nlohmann/json.hpp>

#include <cstring> // For strdup.

auto rs_get_library_features(RsComm* _comm, char** _features) -> int
{
    if (!_comm || !_features) {
        using log_api = irods::experimental::log::api;
        log_api::error("{}: Invalid input. Received one or more null pointers.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

#define IRODS_FEATURE_VALUE(FV) FV // NOLINT(cppcoreguidelines-macro-usage)
#define IRODS_FEATURE(F)        {#F, IRODS_FEATURE_VALUE(F)}, // NOLINT(cppcoreguidelines-macro-usage)

    // clang-format off
    *_features = strdup(nlohmann::json{
        IRODS_FEATURE(IRODS_HAS_LIBRARY_TICKET_ADMINISTRATION)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_TICKET_ADMINISTRATION)

        IRODS_FEATURE(IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_ZONE_ADMINISTRATION)

        IRODS_FEATURE(IRODS_HAS_API_ENDPOINT_SWITCH_USER)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_SWITCH_USER)

        IRODS_FEATURE(IRODS_HAS_LIBRARY_SYSTEM_ERROR)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_SYSTEM_ERROR)

        IRODS_FEATURE(IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_CLIENT_CONNECTION_LIBRARIES_SUPPORT_PROXY_USERS)

        IRODS_FEATURE(IRODS_HAS_API_ENDPOINT_CHECK_AUTH_CREDENTIALS)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_CHECK_AUTH_CREDENTIALS)

        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_REPLICA_TRUNCATE)
        IRODS_FEATURE(IRODS_LIBRARY_FEATURE_GENQUERY2)
    }.dump().c_str());
    // clang-format on

    return 0;
} // rs_check_auth_credentials
