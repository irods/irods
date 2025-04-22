//
// IMPORTANT: Some tests in this file require rodsadmin level privileges!
//

#include <catch2/catch.hpp>

#include "irods/authentication_plugin_framework.hpp"
#include "irods/client_connection.hpp"
#include "irods/filesystem.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"
#include "irods/system_error.hpp"

#include <errno.h> // For ECONNREFUSED.

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace ix = irods::experimental;
namespace ia = irods::authentication;

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
    REQUIRE(ia::authenticate_client(*raw_conn_ptr, nlohmann::json{{ia::scheme_name, env.rodsAuthScheme}}) == 0);

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

    rodsEnv env;
    _getRodsEnv(env);

    const auto test_deferred_authentication = [&env](ix::client_connection& _conn, const std::string& _path) {
        // Show that "conn" holds a valid RcComm.
        REQUIRE(_conn);

        // Show that even though the connection is valid, the user cannot execute any
        // operations because they haven't completed authentication.
        REQUIRE_THROWS(ix::filesystem::client::exists(_conn, _path));

        // Show that the previous operation is allowed once the user is authenticated.
        REQUIRE(ia::authenticate_client(
                    static_cast<RcComm&>(_conn), nlohmann::json{{ia::scheme_name, env.rodsAuthScheme}}) == 0);
        REQUIRE(ix::filesystem::client::exists(_conn, _path));
    };

    namespace adm = irods::experimental::administration;

    ix::client_connection admin_conn;

    const adm::user defer_user{"defer_user"};
    REQUIRE_NOTHROW(adm::client::add_user(admin_conn, defer_user));

    irods::at_scope_exit remove_defer_user{
        [&admin_conn, defer_user] { adm::client::remove_user(admin_conn, defer_user); }};

    SECTION("construct using irods_environment.json")
    {
        ix::client_connection conn{ix::defer_authentication};
        test_deferred_authentication(conn, "/");
    }

    SECTION("construct with user-provided arguments")
    {
        ix::fully_qualified_username user{env.rodsUserName, env.rodsZone};
        ix::client_connection conn{ix::defer_authentication, env.rodsHost, env.rodsPort, user};

        test_deferred_authentication(conn, "/");
    }

    SECTION("construct with proxy and user-provided arguments")
    {
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
        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});

        test_deferred_authentication(conn, "/");
    }

    SECTION("connect with proxy and user-provided arguments")
    {
        ix::fully_qualified_username proxy_user{env.rodsUserName, env.rodsZone};
        ix::fully_qualified_username user{defer_user.name, env.rodsZone};

        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, env.rodsHost, env.rodsPort, proxy_user, user);

        test_deferred_authentication(conn, fmt::format("/{}/home/{}", env.rodsZone, defer_user.name));
    }

    SECTION("connect with proxy using irods_environment.json")
    {
        const adm::user proxy_admin{"proxy_admin"};
        REQUIRE_NOTHROW(adm::client::add_user(admin_conn, proxy_admin, adm::user_type::rodsadmin));

        irods::at_scope_exit remove_proxy_admin{
            [&admin_conn, proxy_admin] { adm::client::remove_user(admin_conn, proxy_admin); }};

        const auto password = std::to_array("ppass");
        const adm::user_password_property pwd_property{password.data()};
        REQUIRE_NOTHROW(adm::client::modify_user(admin_conn, proxy_admin, pwd_property));

        ix::client_connection conn{ix::defer_connection};
        conn.connect(ix::defer_authentication, {proxy_admin.name, env.rodsZone});

        // Show that "conn" holds a valid RcComm.
        REQUIRE(conn);

        // Show that even though the connection is valid, the user cannot execute any
        // operations because they haven't completed authentication.
        const auto path = fmt::format("/{}/home/{}", env.rodsZone, env.rodsUserName);
        REQUIRE_THROWS(ix::filesystem::client::exists(conn, path));

        // Show that the previous operation is allowed once the user is authenticated.
        const auto ctx =
            nlohmann::json{{irods::AUTH_PASSWORD_KEY, password.data()}, {ia::scheme_name, env.rodsAuthScheme}};
        REQUIRE(ia::authenticate_client(static_cast<RcComm&>(conn), ctx) == 0);
        REQUIRE(ix::filesystem::client::exists(conn, path));
    }
}

TEST_CASE("#7192: all connections store unique session signatures", "client_connection")
{
    //
    // IMPORTANT: This test relies on client_connection to verify session signatures are handled
    //            correctly. However, this applies to the connection_pool implementation as well.
    //            Meaning, if it works here, it should work for anything else built on top of
    //            rcConnect-based facilities.
    //

    load_client_api_plugins();

    std::array<ix::client_connection, 3> conns;

    // Show that all connections are in a good state.
    for (auto&& c : conns) {
        CHECK(c);
    }

    //
    // Show that all connections have unique session signatures.
    //

    // clang-format off
    const auto pairs = {
        std::pair{0, 1},
        std::pair{0, 2},
        std::pair{1, 2}
    };
    // clang-format on

    const auto has_unique_session_signatures = [&conns](auto&& p) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        std::string_view sig_a = static_cast<RcComm*>(conns.at(p.first))->session_signature;
        INFO("sig_a: [" << sig_a << ']');

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        std::string_view sig_b = static_cast<RcComm*>(conns.at(p.second))->session_signature;
        INFO("sig_b: [" << sig_b << ']');

        return !sig_a.empty() && !sig_b.empty() && sig_a != sig_b;
    };

    CHECK(std::all_of(std::begin(pairs), std::end(pairs), has_unique_session_signatures));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("#7155: report underlying error info from server on failed connection")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    SECTION("invalid host")
    {
        try {
            ix::client_connection{"example.org", env.rodsPort, {env.rodsUserName, env.rodsZone}};
        }
        catch (const irods::exception& e) {
            CHECK(e.code() == USER_SOCK_CONNECT_TIMEDOUT);
            CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("_rcConnect: connectToRhost failed"));
        }
    }

    SECTION("invalid port")
    {
        try {
            ix::client_connection{env.rodsHost, 8080, {env.rodsUserName, env.rodsZone}};
        }
        catch (const irods::exception& e) {
            const auto ec = ix::make_error_code(e.code());
            CHECK(ix::get_irods_error_code(ec) == USER_SOCK_CONNECT_ERR);
            CHECK(ix::get_errno(ec) == ECONNREFUSED);
            CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("_rcConnect: connectToRhost failed"));
        }
    }

    SECTION("invalid username, not proxied")
    {
        try {
            ix::client_connection{env.rodsHost, env.rodsPort, {"invalid", env.rodsZone}};
        }
        catch (const irods::exception& e) {
            CHECK(e.code() == CAT_INVALID_USER);
            CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Client login error"));
        }
    }

    SECTION("invalid user zone, not proxied")
    {
        try {
            ix::client_connection{env.rodsHost, env.rodsPort, {env.rodsUserName, "invalid"}};
        }
        catch (const irods::exception& e) {
            CHECK(e.code() == CAT_INVALID_USER);
            CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Client login error"));
        }
    }

    SECTION("deferred connection with empty zone")
    {
        try {
            ix::client_connection conn{ix::defer_connection};
            conn.connect(env.rodsHost, env.rodsPort, {env.rodsUserName, ""});
        }
        catch (const irods::exception& e) {
            CHECK(e.code() == SYS_INVALID_INPUT_PARAM);
            CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Empty zone not allowed"));
        }
    }

    SECTION("proxy tests")
    {
        ix::client_connection admin_conn;

        namespace adm = irods::experimental::administration;

        const adm::user other_user{"other_user"};
        REQUIRE_NOTHROW(adm::client::add_user(admin_conn, other_user, adm::user_type::rodsadmin));

        irods::at_scope_exit remove_proxy_admin{
            [&admin_conn, other_user] { adm::client::remove_user(admin_conn, other_user); }};

        const auto password = std::to_array("rods");
        const adm::user_password_property pwd_property{password.data()};
        REQUIRE_NOTHROW(adm::client::modify_user(admin_conn, other_user, pwd_property));

        SECTION("proxied user has empty zone name")
        {
            try {
                ix::client_connection{
                    env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone}, {other_user.name, ""}};
            }
            catch (const irods::exception& e) {
                CHECK(e.code() == SYS_INVALID_INPUT_PARAM);
                CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Empty zone not allowed"));
            }
        }

        SECTION("proxy user has empty zone name")
        {
            try {
                ix::client_connection{
                    env.rodsHost, env.rodsPort, {env.rodsUserName, ""}, {other_user.name, env.rodsZone}};
            }
            catch (const irods::exception& e) {
                CHECK(e.code() == SYS_INVALID_INPUT_PARAM);
                CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Empty zone not allowed"));
            }
        }
    }
}
