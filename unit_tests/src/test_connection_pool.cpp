#include "catch.hpp"

#include "getRodsEnv.h"
#include "connection_pool.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"

TEST_CASE("connection pool")
{
    rodsEnv env;
    REQUIRE(getRodsEnv(&env) == 0);

    SECTION("connections are detachable")
    {
        rcComm_t* released_conn_ptr = nullptr;

        irods::at_scope_exit<std::function<void()>> at_scope_exit{[released_conn_ptr] {
            REQUIRE(rcDisconnect(released_conn_ptr) == 0);
        }};

        const int cp_size = 1;
        const int cp_refresh_time = 600;

        irods::connection_pool conn_pool{cp_size,
                                         env.rodsHost,
                                         env.rodsPort,
                                         env.rodsUserName,
                                         env.rodsZone,
                                         cp_refresh_time};

        namespace fs = irods::experimental::filesystem;

        {
            auto conn = conn_pool.get_connection();
            REQUIRE(static_cast<rcComm_t*>(conn) != nullptr);
            REQUIRE(fs::client::exists(conn, env.rodsHome));

            // Show that the connection is no longer being managed by the pool.
            released_conn_ptr = conn.release();
            REQUIRE(released_conn_ptr != nullptr);
            REQUIRE_FALSE(conn);
            REQUIRE(static_cast<rcComm_t*>(conn) == nullptr);
            REQUIRE_THROWS((void) static_cast<rcComm_t&>(conn), "Invalid connection object");
        }

        // Show that the released connection is no longer managed by the
        // connection pool, but is still usable.
        REQUIRE(fs::client::exists(*released_conn_ptr, env.rodsHome));

        // Given that the connection pool contained only one connection.
        // Show that requesting a connection will cause the connection pool
        // to construct a new connection in place of the released connection.
        auto conn = conn_pool.get_connection();
        REQUIRE(static_cast<rcComm_t*>(conn) != nullptr);

        // Show that the released connection and the connection recently
        // created by the connection pool are indeed different connections.
        REQUIRE(released_conn_ptr != static_cast<rcComm_t*>(conn));
    }
}

