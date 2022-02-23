#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/query_builder.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/rodsGenQuery.h"

#include <cctype>
#include <vector>
#include <string>
#include <algorithm>

namespace ix = irods::experimental;
namespace fs = irods::experimental::filesystem;

TEST_CASE("query builder")
{
    rodsEnv env;
    _getRodsEnv(env);

    load_client_api_plugins();

    ix::client_connection conn{env.rodsHost,
                               env.rodsPort,
                               env.rodsUserName,
                               env.rodsZone};

    const fs::path user_home = env.rodsHome;

    SECTION("general query")
    {
        auto query = ix::query_builder{}
            .zone_hint(env.rodsZone)
            .build<RcComm>(conn, "select count(COLL_NAME) where COLL_NAME = '" + user_home.string() + "'");

        REQUIRE(query.size() > 0);
    }

    SECTION("general query supports additional options")
    {
        std::string path = env.rodsHome;
        std::transform(std::begin(path), std::end(path), std::begin(path),
                       [](unsigned char _ch) { return std::toupper(_ch); });

        auto query = ix::query_builder{}
            .options(UPPER_CASE_WHERE) // Case-insensitive search.
            .build<RcComm>(conn, "select count(COLL_NAME) where COLL_NAME = '" + path + "'");

        REQUIRE(query.size() > 0);
    }

    SECTION("specific query")
    {
        using namespace std::string_literals;

        const std::vector args{user_home.string()};

        ix::query_builder qb;
        const std::string specific_query = "ShowCollAcls";

        auto query = qb.type(ix::query_type::specific)
                       .zone_hint(env.rodsZone)
                       .bind_arguments(args)
                       .build<RcComm>(conn, specific_query);

        REQUIRE(query.size() > 0);

        // Show that the specific query arguments can be cleared.
        REQUIRE_THROWS([&conn, &qb, &specific_query] {
            qb.clear_bound_arguments();

            // This should throw an exception because the specific query arguments
            // vector is not set.
            qb.build<RcComm>(conn, specific_query);
        }());
    }

    SECTION("throw exception on empty query string")
    {
        REQUIRE_THROWS([&conn] {
            ix::query_builder{}.build<RcComm>(conn, "");
        }(), "query string is empty");
    }
}

