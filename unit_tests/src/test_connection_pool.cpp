#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rodsClient.h"

TEST_CASE("connection pool")
{
    rodsEnv env;
    _getRodsEnv(env);

    load_client_api_plugins();

    SECTION("connections are detachable")
    {
        rcComm_t* released_conn_ptr = nullptr;

        irods::at_scope_exit at_scope_exit{[&released_conn_ptr] {
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

    SECTION("connection_proxies are default constructible")
    {
        // This will not compile if the connection proxy class is not
        // default constructible. This allows connection proxies to be
        // placed inside of classes.
        irods::connection_pool::connection_proxy conn;

        // The connection proxy does not reference an actual connection.
        // Therefore, it should evaluate to false in a boolean context.
        REQUIRE_FALSE(conn);

        // Default constructed connection proxies can be moved to.
        auto conn_pool = irods::make_connection_pool();
        conn = conn_pool->get_connection();
        REQUIRE(conn);

        // The following structure will not compile if the connection_proxy
        // is not default constructible.
        struct conn_wrapper
        {
            irods::connection_pool::connection_proxy conn;
        };
    }

    SECTION("releasing a connection does not cause an error when the pool destructs")
    {
        rcComm_t* released_conn_ptr = nullptr;

        irods::at_scope_exit disconnect{[&released_conn_ptr] {
            REQUIRE(rcDisconnect(released_conn_ptr) == 0);
        }};

        {
            auto conn_pool = irods::make_connection_pool();

            // After this call, the pool will not have any more connections.
            // The released connections will be replaced on the next call to
            // get_connection().
            released_conn_ptr = conn_pool->get_connection().release();

            // When the connection pool goes out of scope, the empty connection
            // slot should not cause an error or the program to crash.
        }

        REQUIRE(released_conn_ptr);
    }
}

