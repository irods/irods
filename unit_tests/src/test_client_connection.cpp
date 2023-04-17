#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/filesystem.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/user_administration.hpp"

#include <array>

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
    ix::client_connection conn{env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone}};

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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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

        conn.connect(env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});

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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    }());

    auto* conn_ptr = static_cast<RcComm*>(conn);
    REQUIRE(conn_ptr);
    REQUIRE(conn);

    // Test the connection.
    REQUIRE(ix::filesystem::client::exists(*conn_ptr, "/"));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("defer authentication", "client_connection")
{
    load_client_api_plugins();

    const auto test_deferred_authentication = [](ix::client_connection& _conn, const std::string& _path) {
        // Show that "conn" holds a valid RcComm.
        REQUIRE(_conn);

        // Show that even though the connection is valid, the user cannot execute any
        // operations because they haven't completed authentication.
        REQUIRE_THROWS(ix::filesystem::client::exists(_conn, _path));

        // Show that the previous operation is allowed once the user is authenticated.
        REQUIRE(clientLogin(static_cast<RcComm*>(_conn)) == 0);
        REQUIRE(ix::filesystem::client::exists(_conn, _path));
    };

    namespace ia = irods::experimental::administration;

    ix::client_connection admin_conn;

    const ia::user defer_user{"defer_user"};
    REQUIRE_NOTHROW(ia::client::add_user(admin_conn, defer_user));

    irods::at_scope_exit remove_defer_user{
        [&admin_conn, defer_user] { ia::client::remove_user(admin_conn, defer_user); }};

    SECTION("construct using irods_environment.json")
    {
        ix::client_connection conn{ix::defer_authentication};
        test_deferred_authentication(conn, "/");
    }

    SECTION("construct with user-provided arguments")
    {
        rodsEnv env;
        _getRodsEnv(env);

        ix::fully_qualified_username user{env.rodsUserName, env.rodsZone};
        ix::client_connection conn{ix::defer_authentication, env.rodsHost, env.rodsPort, user};

        test_deferred_authentication(conn, "/");
    }

    SECTION("construct with proxy and user-provided arguments")
    {
        rodsEnv env;
        _getRodsEnv(env);

        ix::fully_qualified_username proxy_user{env.rodsUserName, env.rodsZone};
        ix::fully_qualified_username user{defer_user.name, env.rodsZone};

        ix::client_connection conn{ix::defer_authentication, env.rodsHost, env.rodsPort, proxy_user, user};

        test_deferred_authentication(conn, fmt::format("/{}/home/{}", env.rodsZone, defer_user.name));
    }

    SECTION("connect using irods_environment.json")
    {
        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication);
        test_deferred_authentication(conn, "/");
    }

    SECTION("connect with user-provided arguments")
    {
        rodsEnv env;
        _getRodsEnv(env);

        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});

        test_deferred_authentication(conn, "/");
    }

    SECTION("connect with proxy and user-provided arguments")
    {
        rodsEnv env;
        _getRodsEnv(env);

        ix::fully_qualified_username proxy_user{env.rodsUserName, env.rodsZone};
        ix::fully_qualified_username user{defer_user.name, env.rodsZone};

        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, env.rodsHost, env.rodsPort, proxy_user, user);

        test_deferred_authentication(conn, fmt::format("/{}/home/{}", env.rodsZone, defer_user.name));
    }

    SECTION("connect with proxy using irods_environment.json")
    {
        const ia::user proxy_admin{"proxy_admin"};
        REQUIRE_NOTHROW(ia::client::add_user(admin_conn, proxy_admin, ia::user_type::rodsadmin));

        irods::at_scope_exit remove_proxy_admin{
            [&admin_conn, proxy_admin] { ia::client::remove_user(admin_conn, proxy_admin); }};

        // The property is mutable so that we can use it for clientLoginWithPassword()
        // later in the test.
        auto password = std::to_array("ppass");
        const ia::user_password_property pwd_property{password.data()};
        REQUIRE_NOTHROW(ia::client::modify_user(admin_conn, proxy_admin, pwd_property));

        rodsEnv env;
        _getRodsEnv(env);

        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, {proxy_admin.name, env.rodsZone});

        // Show that "conn" holds a valid RcComm.
        REQUIRE(conn);

        // Show that even though the connection is valid, the user cannot execute any
        // operations because they haven't completed authentication.
        const auto path = fmt::format("/{}/home/{}", env.rodsZone, env.rodsUserName);
        REQUIRE_THROWS(ix::filesystem::client::exists(conn, path));

        // Show that the previous operation is allowed once the user is authenticated.
        REQUIRE(clientLoginWithPassword(static_cast<RcComm*>(conn), password.data()) == 0);
        REQUIRE(ix::filesystem::client::exists(conn, path));
    }
}
