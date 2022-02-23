#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"

namespace ix = irods::experimental;

TEST_CASE("connect using default constructor", "client_connection")
{
    load_client_api_plugins();
    ix::client_connection conn;
    REQUIRE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(conn, "/"));
}

TEST_CASE("connect using multi-argument constructor", "client_connection")
{
    rodsEnv env;
    _getRodsEnv(env);

    load_client_api_plugins();
    ix::client_connection conn{env.rodsHost,
                               env.rodsPort,
                               env.rodsUserName,
                               env.rodsZone};

    REQUIRE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(conn, "/"));
}

TEST_CASE("take ownership of raw connection", "client_connection")
{
    rodsEnv env;
    _getRodsEnv(env);

    load_client_api_plugins();
    rErrMsg_t error{};
    auto* raw_conn_ptr = rcConnect(env.rodsHost,
                                   env.rodsPort,
                                   env.rodsUserName,
                                   env.rodsZone,
                                   NO_RECONN,
                                   &error);

    REQUIRE(raw_conn_ptr);
    REQUIRE(clientLogin(raw_conn_ptr) == 0);

    ix::client_connection conn{*raw_conn_ptr};
    REQUIRE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(conn, "/"));
}

TEST_CASE("defer connection", "client_connection")
{
    load_client_api_plugins();
    ix::client_connection conn{ix::defer_connection};
    REQUIRE_FALSE(conn);

    SECTION("use no-argument connect")
    {
        conn.connect();
        REQUIRE(conn);
    }

    SECTION("use multi-argument connect")
    {
        rodsEnv env;
        _getRodsEnv(env);

        conn.connect(env.rodsHost,
                     env.rodsPort,
                     env.rodsUserName,
                     env.rodsZone);

        REQUIRE(conn);
    }

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(conn, "/"));

    conn.disconnect();
    REQUIRE_FALSE(conn);
}

TEST_CASE("supports move semantics", "client_connection")
{
    load_client_api_plugins();
    ix::client_connection conn;
    REQUIRE(conn);

    auto new_owner = std::move(conn);
    REQUIRE(new_owner);
    REQUIRE_FALSE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(new_owner, "/"));
}

TEST_CASE("supports implicit and explicit casts to underlying type", "client_connection")
{
    load_client_api_plugins();
    ix::client_connection conn;
    REQUIRE(conn);

    REQUIRE_NOTHROW([&conn] {
        RcComm& conn_ref = conn;
        static_cast<void>(conn_ref);
        REQUIRE(conn);

        // Test the connection.
        REQUIRE(ix::filesystem::client::exists(conn_ref, "/"));
    });

    RcComm* conn_ptr = static_cast<RcComm*>(conn);
    REQUIRE(conn_ptr);
    REQUIRE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(*conn_ptr, "/"));
}

