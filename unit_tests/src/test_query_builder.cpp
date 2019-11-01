#include "catch.hpp"

#include "getRodsEnv.h"
#include "rcConnect.h"
#include "connection_pool.hpp"
#include "query_builder.hpp"
#include "filesystem.hpp"

#include <vector>
#include <string>

namespace fs = irods::experimental::filesystem;

TEST_CASE("query builder")
{
    rodsEnv env;
    REQUIRE(getRodsEnv(&env) == 0);

    const int cp_size = 1;
    const int cp_refresh_time = 600;

    irods::connection_pool conn_pool{cp_size,
                                     env.rodsHost,
                                     env.rodsPort,
                                     env.rodsUserName,
                                     env.rodsZone,
                                     cp_refresh_time};

    const fs::path user_home = env.rodsHome;

    SECTION("general query")
    {
        auto conn = conn_pool.get_connection();

        auto query = irods::experimental::query_builder{}
            .zone_hint(env.rodsZone)
            .build<rcComm_t>(conn, "select count(COLL_NAME) where COLL_NAME = '" + user_home.string() + "'");

        REQUIRE(query.size() > 0);
    }

    SECTION("specific query")
    {
        using namespace std::string_literals;

        auto conn = conn_pool.get_connection();

        const std::vector<std::string> args{
            user_home.string()
        };

        auto query = irods::experimental::query_builder{}
            .type(irods::experimental::query_type::specific)
            .zone_hint(env.rodsZone)
            .bind_arguments(args)
            .build<rcComm_t>(conn, "ShowCollAcls");

        REQUIRE(query.size() > 0);
    }

    SECTION("throw exception on empty query string")
    {
        REQUIRE_THROWS([&conn_pool] {
            auto conn = conn_pool.get_connection();
            irods::experimental::query_builder{}.build<rcComm_t>(conn, "");
        }(), "query string is empty");
    }
}

