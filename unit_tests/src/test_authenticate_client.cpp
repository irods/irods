#include <catch2/catch_all.hpp>

#include "irods/authenticate.h"
#include "irods/authentication_plugin_framework.hpp"
#include "irods/client_connection.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_query.hpp"
#include "irods/obf.h"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"

#include <nlohmann/json.hpp>

namespace ia = irods::authentication;

TEST_CASE("rc_authenticate_client: null inputs are invalid")
{
    CHECK(INVALID_INPUT_ARGUMENT_NULL_POINTER == rc_authenticate_client(nullptr, nullptr));
    CHECK(INVALID_INPUT_ARGUMENT_NULL_POINTER == rc_authenticate_client(nullptr, ""));
    CHECK(INVALID_INPUT_ARGUMENT_NULL_POINTER == rc_authenticate_client(nullptr, "{}"));
    CHECK(INVALID_INPUT_ARGUMENT_NULL_POINTER == rc_authenticate_client(nullptr, "{\"scheme\": \"native\"}"));
    RcComm comm;
    CHECK(INVALID_INPUT_ARGUMENT_NULL_POINTER == rc_authenticate_client(&comm, nullptr));
}

TEST_CASE("rc_authenticate_client: _context must be valid JSON")
{
    RcComm comm{};
    CHECK(JSON_VALIDATION_ERROR == rc_authenticate_client(&comm, ""));
    CHECK(JSON_VALIDATION_ERROR == rc_authenticate_client(&comm, "nope"));
    CHECK(JSON_VALIDATION_ERROR == rc_authenticate_client(&comm, "{"));
}

TEST_CASE("rc_authenticate_client: _context must be a JSON object with a \"scheme\" key")
{
    RcComm comm{};

    SECTION("JSON array is not valid")
    {
        const auto ctx = nlohmann::json::array({});
        CHECK(USER_AUTH_SCHEME_ERR == rc_authenticate_client(&comm, ctx.dump().c_str()));
    }

    SECTION("JSON array with 'scheme' in it is not valid")
    {
        static const auto ctx = nlohmann::json::array({ia::scheme_name});
        CHECK(USER_AUTH_SCHEME_ERR == rc_authenticate_client(&comm, ctx.dump().c_str()));
    }

    SECTION("JSON array with expected JSON object in it is not valid")
    {
        static const auto ctx = nlohmann::json::array({ia::scheme_name, "native"});
        CHECK(USER_AUTH_SCHEME_ERR == rc_authenticate_client(&comm, ctx.dump().c_str()));
    }

    SECTION("Empty JSON object is not valid")
    {
        static const auto ctx = nlohmann::json::object({});
        CHECK(USER_AUTH_SCHEME_ERR == rc_authenticate_client(&comm, ctx.dump().c_str()));
    }

    SECTION("JSON object without \"scheme\" key is not valid")
    {
        static const auto ctx = nlohmann::json{{"key", "val"}};
        CHECK(USER_AUTH_SCHEME_ERR == rc_authenticate_client(&comm, ctx.dump().c_str()));
    }
}

TEST_CASE("rc_authenticate_client: minimum viable inputs")
{
    load_client_api_plugins();

    // Use local client environment and assume that an .irodsA file exists.
    RodsEnvironment env;
    _getRodsEnv(env);

    // Connect to the server.
    rErrMsg_t error{};
    RcComm* comm = rcConnect(env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, NO_RECONN, &error);
    REQUIRE(comm);
    const auto disconnect = irods::at_scope_exit{[&comm] { rcDisconnect(comm); }};

    // Show that user cannot perform a query on a protected collection before authenticating.
    const auto query_str =
        fmt::format("select COLL_ID where COLL_NAME = '/{}/home/{}'", env.rodsZone, env.rodsUserName);
    CHECK_THROWS(irods::query{comm, query_str});

    // Authenticate using the minimum required inputs...
    const auto ctx = nlohmann::json{{ia::scheme_name, env.rodsAuthScheme}};
    REQUIRE(0 == rc_authenticate_client(comm, ctx.dump().c_str()));

    // Show that user can now run a query on a protected collection after authenticating.
    CHECK(!irods::query{comm, query_str}.empty());
}
