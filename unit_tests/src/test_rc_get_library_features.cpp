#include <catch2/catch.hpp>

#include "irods/get_library_features.h"
#include "irods/client_connection.hpp"
#include "irods/getRodsEnv.h"
#include "irods/library_features.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"

#include <nlohmann/json.hpp>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_get_library_features")
{
    SECTION("good inputs")
    {
        load_client_api_plugins();

        irods::experimental::client_connection conn;

        char* features_string{};
        REQUIRE(rc_get_library_features(static_cast<RcComm*>(conn), &features_string) == 0);
        REQUIRE(features_string);

        const auto features = nlohmann::json::parse(features_string);

#define IRODS_TO_STRING(X) #X // NOLINT(cppcoreguidelines-macro-usage)

        // clang-format off
        CHECK(202209 == features.at(IRODS_TO_STRING(IRODS_HAS_LIBRARY_TICKET_ADMINISTRATION)).get<int>());
        CHECK(202210 == features.at(IRODS_TO_STRING(IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION)).get<int>());
        CHECK(202211 == features.at(IRODS_TO_STRING(IRODS_HAS_API_ENDPOINT_SWITCH_USER)).get<int>());
        CHECK(202301 == features.at(IRODS_TO_STRING(IRODS_HAS_LIBRARY_SYSTEM_ERROR)).get<int>());
        CHECK(202306 == features.at(IRODS_TO_STRING(IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES)).get<int>());
        CHECK(202307 == features.at(IRODS_TO_STRING(IRODS_HAS_API_ENDPOINT_CHECK_AUTH_CREDENTIALS)).get<int>());
        // clang-format on
    }

    SECTION("bad inputs")
    {
        RcComm comm{};
        char* features_string{};

        CHECK(rc_get_library_features(nullptr, &features_string) == SYS_INVALID_INPUT_PARAM);
        CHECK_FALSE(features_string);

        CHECK(rc_get_library_features(&comm, nullptr) == SYS_INVALID_INPUT_PARAM);
    }
}
