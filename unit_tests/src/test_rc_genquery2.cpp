#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/genquery2.h"
#include "irods/getRodsEnv.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <string>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_genquery2")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;

    SECTION("execute a simple query")
    {
        auto query_string = fmt::format("select COLL_NAME where COLL_NAME = '{}'", env.rodsHome);

        Genquery2Input input{};
        input.query_string = query_string.data();
        input.sql_only = 0;
        input.zone = nullptr;

        char* output{};
        REQUIRE(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        REQUIRE_FALSE(output == nullptr);

        REQUIRE_NOTHROW([&env, &output] {
            const auto result = nlohmann::json::parse(output);
            CHECK_FALSE(result.empty());
            CHECK(result.at(0).at(0).get_ref<const std::string&>() == env.rodsHome);
        }());
    }

    SECTION("bad inputs")
    {
        RcComm comm{};
        Genquery2Input input{};
        char* output{};

        CHECK(rc_genquery2(nullptr, &input, &output) == SYS_INVALID_INPUT_PARAM);
        CHECK_FALSE(output);

        CHECK(rc_genquery2(&comm, nullptr, &output) == SYS_INVALID_INPUT_PARAM);
        CHECK_FALSE(output);

        CHECK(rc_genquery2(&comm, &input, nullptr) == SYS_INVALID_INPUT_PARAM);
        CHECK_FALSE(output);
    }

    SECTION("supports nested, multi-argument functions in projection list")
    {
        auto query_string = fmt::format(
            "select concat(COLL_NAME, concat('--', COLL_ACCESS_USER_NAME)) where COLL_NAME = '{}'", env.rodsHome);

        Genquery2Input input{};
        input.query_string = query_string.data();

        char* output{};
        REQUIRE(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        REQUIRE_FALSE(output == nullptr);

        REQUIRE_NOTHROW([&env, &output] {
            const auto result = nlohmann::json::parse(output);
            CHECK_FALSE(result.empty());
            const auto expected = fmt::format("{}--{}", env.rodsHome, env.rodsUserName);
            CHECK(result.at(0).at(0).get_ref<const std::string&>() == expected);
        }());
    }

    SECTION("supports nested, multi-argument functions in where clause")
    {
        auto query_string =
            fmt::format("select COLL_NAME where concat(COLL_NAME, concat('--', COLL_ACCESS_USER_NAME)) = '{}--{}'",
                        env.rodsHome,
                        env.rodsUserName);

        Genquery2Input input{};
        input.query_string = query_string.data();

        char* output{};
        REQUIRE(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        REQUIRE_FALSE(output == nullptr);

        REQUIRE_NOTHROW([&env, &output] {
            const auto result = nlohmann::json::parse(output);
            CHECK_FALSE(result.empty());
            CHECK(result.at(0).at(0).get_ref<const std::string&>() == env.rodsHome);
        }());
    }

    SECTION("supports nested, multi-argument functions in order-by clause")
    {
        auto query_string = fmt::format("select concat(COLL_NAME, concat('--', COLL_ACCESS_USER_NAME)) where COLL_NAME "
                                        "= '{}' order by concat(COLL_NAME, concat('--', COLL_ACCESS_USER_NAME))",
                                        env.rodsHome);

        Genquery2Input input{};
        input.query_string = query_string.data();

        char* output{};
        REQUIRE(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        REQUIRE_FALSE(output == nullptr);

        REQUIRE_NOTHROW([&env, &output] {
            const auto result = nlohmann::json::parse(output);
            CHECK_FALSE(result.empty());
            const auto expected = fmt::format("{}--{}", env.rodsHome, env.rodsUserName);
            CHECK(result.at(0).at(0).get_ref<const std::string&>() == expected);
        }());
    }
}
