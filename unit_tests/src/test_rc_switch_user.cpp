#include <catch2/catch.hpp>

#include "irods/switch_user.h"
#include "irods/client_connection.hpp"
#include "irods/transport/default_transport.hpp"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"
#include "irods/zone_administration.hpp"

#include <cstring>
#include <vector>
#include <string>
#include <tuple>

// clang-format off
namespace adm = irods::experimental::administration;
namespace fs  = irods::experimental::filesystem;
namespace io  = irods::experimental::io;
// clang-format on

TEST_CASE("rc_switch_user basic usage")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{"/"} / env.rodsZone / "home/public/unit_testing_sandbox";
    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    // As the administrator, create a data object.
    auto data_object = sandbox / "foo";
    io::client::native_transport tp{conn};
    io::odstream{tp, data_object} << "some data";
    REQUIRE(fs::client::is_data_object(conn, data_object));

    // Create a test user.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));

    irods::at_scope_exit remove_test_user{[&conn, &alice] { REQUIRE_NOTHROW(adm::client::remove_user(conn, alice)); }};

    // Give the test user permission to see the admin's data object and the contents of
    // the sandbox collection.
    fs::client::permissions(conn, sandbox, alice.name, fs::perms::write);
    fs::client::permissions(conn, data_object, alice.name, fs::perms::read);

    // Become the test user.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    REQUIRE(rc_switch_user(conn_ptr, alice.name.c_str(), alice.zone.c_str()) == 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // As the test user, create another data object in the sandbox.
    data_object = sandbox / "bar";
    io::odstream{tp, data_object} << "other data";
    REQUIRE(fs::client::is_data_object(conn, data_object));

    // Show that there are two data objects in the collection.
    CHECK(std::distance(fs::client::collection_iterator{conn, sandbox}, fs::client::collection_iterator{}) == 2);

    // Give the administrator OWN permissions on the data object created by the test user.
    // This allows the administrator to remove the sandbox collection without issues.
    fs::client::permissions(conn, data_object, env.rodsUserName, fs::perms::own);

    // Become the administrator so that the test can clean up properly.
    // This can only be achieved by reconnecting to the server due to the test user not
    // being an administrator.
    conn.disconnect();
    conn.connect();
}

TEST_CASE("rc_switch_user honors permission model following successful invocation")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{static_cast<const char*>(env.rodsHome)} / "unit_testing_sandbox";
    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    // As the administrator, create a data object.
    const auto data_object = sandbox / "foo";
    io::client::native_transport tp{conn};
    io::odstream{tp, data_object} << "data";
    REQUIRE(fs::client::is_data_object(conn, data_object));

    // Create a test user.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));

    irods::at_scope_exit remove_test_user{[&conn, &alice] { REQUIRE_NOTHROW(adm::client::remove_user(conn, alice)); }};

    // Become the test user.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    REQUIRE(rc_switch_user(conn_ptr, alice.name.c_str(), alice.zone.c_str()) == 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // Show that the test user cannot see the administrator's collection or data object.
    CHECK_FALSE(fs::client::exists(conn, sandbox));
    CHECK_FALSE(fs::client::exists(conn, data_object));

    // Become the administrator so that the test can clean up properly.
    // This can only be achieved by reconnecting to the server due to the test user not
    // being an administrator.
    conn.disconnect();
    conn.connect();
}

TEST_CASE("rc_switch_user enforces API requirements")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;

    auto* conn_ptr = static_cast<RcComm*>(conn);

    SECTION("null pointers")
    {
        REQUIRE(rc_switch_user(conn_ptr, nullptr, nullptr) == SYS_INVALID_INPUT_PARAM);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        REQUIRE(rc_switch_user(conn_ptr, nullptr, "") == SYS_INVALID_INPUT_PARAM);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        REQUIRE(rc_switch_user(conn_ptr, "", nullptr) == SYS_INVALID_INPUT_PARAM);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    }

    constexpr const char* const very_long_string = "_________________________64_byte_string_________________________";

    // clang-format off
    const std::vector<std::tuple<std::string, std::string, int>> bad_inputs{
        {""                                                  , ""              , SYS_INVALID_INPUT_PARAM}, // Empty strings
        {env.rodsUserName                                    , ""              , SYS_INVALID_INPUT_PARAM}, // Empty zone
        {""                                                  , env.rodsZone    , SYS_INVALID_INPUT_PARAM}, // Empty username
        {fmt::format("{}#{}", env.rodsUserName, env.rodsZone), env.rodsZone    , CAT_INVALID_USER       }, // Zone in username
        {"does_not_exist"                                    , env.rodsZone    , CAT_INVALID_USER       }, // Non-existent username
        {env.rodsUserName                                    , "does_not_exist", CAT_INVALID_USER       }, // Non-existent zone
        {very_long_string                                    , env.rodsZone    , SYS_INVALID_INPUT_PARAM}, // Username string is too long
        {env.rodsUserName                                    , very_long_string, SYS_INVALID_INPUT_PARAM}  // Zone string is too long
    };
    // clang-format on

    for (auto&& input : bad_inputs) {
        const auto& [name, zone, ec] = input;

        DYNAMIC_SECTION("username=[" << name << "], zone=[" << zone << ']')
        {
            REQUIRE(rc_switch_user(conn_ptr, name.c_str(), zone.c_str()) == ec);
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
            CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
            // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }
    }
}

TEST_CASE("rc_switch_user supports remote zones")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;

    // Create a remote zone.
    const auto* zone_name = "switch_user_zone";
    REQUIRE_NOTHROW(adm::client::add_zone(conn, zone_name));

    // Make sure to the remove zone on completion.
    irods::at_scope_exit remove_test_zones{[zone_name] {
        irods::experimental::client_connection conn;

        if (adm::client::zone_exists(conn, zone_name)) {
            REQUIRE_NOTHROW(adm::client::remove_zone(conn, zone_name));
        }
    }};

    // Create a test user for the remote zone.
    const adm::user alice{"test_user_alice", zone_name};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice, adm::user_type::rodsuser, adm::zone_type::remote));

    irods::at_scope_exit remove_test_user{[&alice] {
        irods::experimental::client_connection conn;
        REQUIRE_NOTHROW(adm::client::remove_user(conn, alice));
    }};

    // Become the test user.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    REQUIRE(rc_switch_user(conn_ptr, alice.name.c_str(), alice.zone.c_str()) == 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    const auto unique_name = adm::client::local_unique_name(conn, alice);
    CHECK(fs::client::exists(conn, (fs::path{env.rodsHome} / ".." / unique_name).lexically_normal()));
}

TEST_CASE("rc_switch_user cannot be invoked by non-admins")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    // This connection is used for creating the test user and cleaning up.
    irods::experimental::client_connection conn;

    // Create a test user.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));
    REQUIRE_NOTHROW(adm::client::modify_user(conn, alice, adm::user_password_property{"rods"}));

    irods::at_scope_exit remove_test_user{[&conn, &alice] { REQUIRE_NOTHROW(adm::client::remove_user(conn, alice)); }};

    // Connect to the server as the test user.
    rErrMsg_t error;
    auto* conn_ptr = rcConnect(env.rodsHost, env.rodsPort, alice.name.c_str(), env.rodsZone, 0, &error);
    REQUIRE(conn_ptr);
    irods::at_scope_exit close_alice_connection{[conn_ptr] { REQUIRE(rcDisconnect(conn_ptr) == 0); }};
    REQUIRE(clientLoginWithPassword(conn_ptr, "rods") == 0);

    // Show that the test user is not allowed to invoke rc_switch_user due to them being a rodsuser.
    REQUIRE(rc_switch_user(conn_ptr, env.rodsUserName, env.rodsZone) == SYS_PROXYUSER_NO_PRIV);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
}
