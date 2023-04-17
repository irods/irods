#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/filesystem.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("connection pool")
{
    rodsEnv env;
    _getRodsEnv(env);

    load_client_api_plugins();

    constexpr int cp_size = 1;
    constexpr int cp_refresh_time = 600;

    SECTION("connections are detachable")
    {
        rcComm_t* released_conn_ptr = nullptr;

        irods::at_scope_exit at_scope_exit{[&released_conn_ptr] {
            REQUIRE(rcDisconnect(released_conn_ptr) == 0);
        }};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        irods::connection_pool conn_pool{cp_size,
                                         env.rodsHost,
                                         env.rodsPort,
                                         env.rodsUserName,
                                         env.rodsZone,
                                         cp_refresh_time};
#pragma clang diagnostic pop

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

    SECTION("connections are detachable (updated interface)")
    {
        rcComm_t* released_conn_ptr = nullptr;

        irods::at_scope_exit at_scope_exit{[&released_conn_ptr] { REQUIRE(rcDisconnect(released_conn_ptr) == 0); }};

        irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};
        irods::connection_pool conn_pool{cp_size, env.rodsHost, env.rodsPort, user, cp_refresh_time};

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
        } //namespace irods::experimental::filesystem;

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

    SECTION("connection_proxy is default constructible")
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

    SECTION("connection pools allow the default authentication method to be overridden")
    {
        REQUIRE_NOTHROW([&env] {
            const auto auth_func = [](auto& _comm) {
                if (clientLogin(&_comm) != 0) {
                    THROW(SYS_SOCK_CONNECT_ERR, ".irodsA authentication error");
                }
            };

            irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};
            irods::connection_pool conn_pool{cp_size, env.rodsHost, env.rodsPort, user, cp_refresh_time, auth_func};
        }());
    }

    SECTION("connection pools support proxied connections")
    {
        irods::experimental::client_connection admin_conn;

        namespace ia = irods::experimental::administration;

        const ia::user proxy_admin{"proxy_admin"};
        REQUIRE_NOTHROW(ia::client::add_user(admin_conn, proxy_admin, ia::user_type::rodsadmin));

        irods::at_scope_exit remove_proxy_admin{
            [&admin_conn, proxy_admin] { ia::client::remove_user(admin_conn, proxy_admin); }};

        auto password = std::to_array("ppass");
        const ia::user_password_property pwd_property{password.data()};
        REQUIRE_NOTHROW(ia::client::modify_user(admin_conn, proxy_admin, pwd_property));

        const auto auth_func = [&password](auto& _comm) {
            if (clientLoginWithPassword(&_comm, password.data()) != 0) {
                THROW(SYS_SOCK_CONNECT_ERR, "Password authentication error");
            }
        };

        irods::experimental::fully_qualified_username proxy_user{proxy_admin.name, env.rodsZone};
        irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};

        irods::connection_pool conn_pool{
            cp_size, env.rodsHost, env.rodsPort, proxy_user, user, cp_refresh_time, auth_func};

        auto conn = conn_pool.get_connection();

        // Show that the proxied connection works by accessing the home collection of
        // the proxied user.
        namespace fs = irods::experimental::filesystem;
        REQUIRE(fs::client::exists(conn, fmt::format("/{}/home/{}", env.rodsZone, env.rodsUserName)));
    }

    SECTION("connection pool throws exception on incorrect alternative authentication method")
    {
        const auto auth_func = [](auto& _comm) {
            // This will cause the connection pool to throw an exception on construction.
            if (clientLoginWithPassword(&_comm, "nope") != 0) {
                THROW(SYS_SOCK_CONNECT_ERR, "Password authentication error");
            }
        };

        try {
            irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};
            irods::connection_pool conn_pool{cp_size, env.rodsHost, env.rodsPort, user, cp_refresh_time, auth_func};
        }
        catch (const irods::exception& e) {
            REQUIRE(e.code() == SYS_SOCK_CONNECT_ERR);
        }
    }

    SECTION("connection pool with proxy throws exception on incorrect alternative authentication method")
    {
        irods::experimental::client_connection admin_conn;

        namespace ia = irods::experimental::administration;

        const ia::user proxy_admin{"proxy_admin"};
        REQUIRE_NOTHROW(ia::client::add_user(admin_conn, proxy_admin, ia::user_type::rodsadmin));

        irods::at_scope_exit remove_proxy_admin{
            [&admin_conn, proxy_admin] { ia::client::remove_user(admin_conn, proxy_admin); }};

        // The property is mutable so that we can use it for clientLoginWithPassword()
        // later in the test.
        auto password = std::to_array("ppass");
        const ia::user_password_property pwd_property{password.data()};
        REQUIRE_NOTHROW(ia::client::modify_user(admin_conn, proxy_admin, pwd_property));

        const auto auth_func = [](auto& _comm) {
            // This will cause the connection pool to throw an exception on construction.
            if (clientLoginWithPassword(&_comm, "nope") != 0) {
                THROW(SYS_SOCK_CONNECT_ERR, "Password authentication error");
            }
        };

        try {
            irods::experimental::fully_qualified_username proxy_user{proxy_admin.name, env.rodsZone};
            irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};

            irods::connection_pool conn_pool{
                cp_size, env.rodsHost, env.rodsPort, proxy_user, user, cp_refresh_time, auth_func};
        }
        catch (const irods::exception& e) {
            REQUIRE(e.code() == SYS_SOCK_CONNECT_ERR);
        }
    }
}
