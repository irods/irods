#include "catch.hpp"

#include "getRodsEnv.h"
#include "rcConnect.h"
#include "connection_pool.hpp"
#include "query_builder.hpp"
#include "filesystem.hpp"

#include <vector>
#include <string>

namespace ix = irods::experimental;
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

        auto query = ix::query_builder{}
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

        ix::query_builder qb;
        const std::string specific_query = "ShowCollAcls";

        auto query = qb.type(ix::query_type::specific)
                       .zone_hint(env.rodsZone)
                       .bind_arguments(args)
                       .build<rcComm_t>(conn, specific_query);

        REQUIRE(query.size() > 0);

        // Show that the specific query arguments can be cleared.
        REQUIRE_THROWS([&conn, &qb, &specific_query] {
            qb.clear_bound_arguments();

            // This should throw an exception because the specific query arguments
            // vector is not set.
            qb.build<rcComm_t>(conn, specific_query);
        }());
    }

    SECTION("throw exception on empty query string")
    {
        REQUIRE_THROWS([&conn_pool] {
            auto conn = conn_pool.get_connection();
            ix::query_builder{}.build<rcComm_t>(conn, "");
        }(), "query string is empty");
    }
}

