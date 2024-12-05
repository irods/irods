#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/plugins/api/switch_user_types.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/replica.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/switch_user.h"
#include "irods/ticket_administration.hpp"
#include "irods/transport/default_transport.hpp"
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    SwitchUserInput input{};
    std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
    addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
    REQUIRE(rc_switch_user(conn_ptr, &input) == 0);
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    SwitchUserInput input{};
    std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
    addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
    REQUIRE(rc_switch_user(conn_ptr, &input) == 0);
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
        REQUIRE(rc_switch_user(nullptr, nullptr) == SYS_INVALID_INPUT_PARAM);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        REQUIRE(rc_switch_user(conn_ptr, nullptr) == SYS_INVALID_INPUT_PARAM);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        SwitchUserInput input{};
        REQUIRE(rc_switch_user(nullptr, &input) == SYS_INVALID_INPUT_PARAM);
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

    SwitchUserInput su_input{};
    addKeyVal(&su_input.options, KW_SWITCH_PROXY_USER, "");

    for (auto&& input : bad_inputs) {
        const auto& [name, zone, ec] = input;

        std::strncpy(su_input.username, name.c_str(), sizeof(SwitchUserInput::username));
        std::strncpy(su_input.zone, zone.c_str(), sizeof(SwitchUserInput::zone));

        DYNAMIC_SECTION("username=[" << name << "], zone=[" << zone << ']')
        {
            REQUIRE(rc_switch_user(conn_ptr, &su_input) == ec);
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
            CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
            // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    SwitchUserInput input{};
    std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
    addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
    REQUIRE(rc_switch_user(conn_ptr, &input) == 0);
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
    SwitchUserInput input{};
    std::strncpy(input.username, env.rodsUserName, sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, env.rodsZone, sizeof(SwitchUserInput::zone));
    addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
    REQUIRE(rc_switch_user(conn_ptr, &input) == SYS_PROXYUSER_NO_PRIV);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
}

TEST_CASE("rc_switch_user cannot be invoked when switching to rodsuser and KW_SWITCH_PROXY_USER is set")
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

    // Become the test user and instruct the server to update the proxy user as well.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    SwitchUserInput input{};
    std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
    addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
    REQUIRE(rc_switch_user(conn_ptr, &input) == 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // Show that because the KW_SWITCH_PROXY_USER option is set, iRODS no longer allows
    // the rc_switch_user to be invoked.
    std::strncpy(input.username, env.rodsUserName, sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, env.rodsZone, sizeof(SwitchUserInput::zone));
    REQUIRE(rc_switch_user(conn_ptr, &input) == SYS_PROXYUSER_NO_PRIV);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    CHECK(conn_ptr->clientUser.userName == alice.name);
    CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // Become the administrator so that the test can clean up properly.
    // This can only be achieved by reconnecting to the server due to the test user not
    // being an administrator.
    conn.disconnect();
    conn.connect();
}

TEST_CASE("rc_switch_user can be called multiple times when KW_SWITCH_PROXY_USER is not set")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    // This connection is used for creating the test user and cleaning up.
    irods::experimental::client_connection conn;

    // Create a rodsuser.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));
    irods::at_scope_exit remove_test_user_alice{
        [&conn, &alice] { REQUIRE_NOTHROW(adm::client::remove_user(conn, alice)); }};
    REQUIRE_NOTHROW(adm::client::modify_user(conn, alice, adm::user_password_property{"rods"}));

    // Create a groupadmin.
    const adm::user bob{"test_user_bob", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, bob));
    REQUIRE_NOTHROW(adm::client::modify_user(conn, bob, adm::user_password_property{"rods"}));
    irods::at_scope_exit remove_test_user_bob{[&conn, &bob] { REQUIRE_NOTHROW(adm::client::remove_user(conn, bob)); }};
    REQUIRE_NOTHROW(adm::client::modify_user(conn, bob, adm::user_type_property{adm::user_type::groupadmin}));

    //
    // Call rc_switch_user multiple times and toggle between two rodsusers.
    // It is important that the users being switched to not be rodsadmins because doing this
    // with rodsadmins may bypass important permission checks.
    //

    auto* conn_ptr = static_cast<RcComm*>(conn);
    const auto* current_user = &alice;
    const auto* next_user = &bob;
    SwitchUserInput input{};

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    for (int i = 0; i < 10; ++i) {
        std::strncpy(input.username, current_user->name.c_str(), sizeof(SwitchUserInput::username));
        std::strncpy(input.zone, current_user->zone.c_str(), sizeof(SwitchUserInput::zone));
        REQUIRE(rc_switch_user(static_cast<RcComm*>(conn), &input) == 0);

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(conn_ptr->clientUser.userName == current_user->name);
        CHECK(conn_ptr->clientUser.rodsZone == current_user->zone);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        // Swap the pointers so that rc_switch_user is invoked with a different user on each iteration.
        std::swap(current_user, next_user);
    }

    // Become the administrator so that the test can clean up properly.
    // This can only be achieved by reconnecting to the server due to the test user not
    // being an administrator.
    conn.disconnect();
    conn.connect();
}

TEST_CASE("rc_switch_user disassociates ticket from connection")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    //
    // IMPORTANT: This test must be run using a rodsadmin account.
    //

    // clang-format off
    namespace adm = irods::experimental::administration;
    namespace fs  = irods::experimental::filesystem;
    // clang-format on

    irods::experimental::client_connection conn;

    // Create a ticket that allows users to read the rodsadmin's home collection.
    auto ticket = adm::ticket::client::create_ticket(conn, adm::ticket::ticket_type::read, env.rodsHome);
    CHECK_FALSE(ticket.empty());

    // Create a rodsuser.
    const adm::user test_user_1{"rc_switch_user_test_user_1"};

    REQUIRE_NOTHROW(adm::client::add_user(conn, test_user_1));

    irods::at_scope_exit remove_user_1{
        [&conn, &test_user_1] { CHECK_NOTHROW(adm::client::remove_user(conn, test_user_1)); }};

    adm::user_password_property password_prop{"rods"};
    REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user_1, password_prop));

    // Connect to the iRODS server as test_user_1, with the rodsadmin acting as a proxy.
    irods::experimental::fully_qualified_username proxy_user{env.rodsUserName, env.rodsZone};
    irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};
    irods::experimental::client_connection test_user_conn{
        irods::experimental::defer_authentication, env.rodsHost, env.rodsPort, proxy_user, user};
    REQUIRE(clientLoginWithPassword(static_cast<RcComm*>(test_user_conn), password_prop.value.data()) == 0);

    // Enable the ticket and show that test_user_1 can read the rodsadmin's home collection.
    {
        TicketAdminInput input{};
        input.arg1 = "session";
        input.arg2 = ticket.data();
        input.arg3 = "";

        REQUIRE(rcTicketAdmin(static_cast<RcComm*>(test_user_conn), &input) == 0);
        CHECK(fs::client::exists(test_user_conn, env.rodsHome));
    }

    // Create a second rodsuser.
    const adm::user test_user_2{"rc_switch_user_test_user_2"};

    REQUIRE_NOTHROW(adm::client::add_user(conn, test_user_2));

    irods::at_scope_exit remove_user_2{
        [&conn, &test_user_2] { CHECK_NOTHROW(adm::client::remove_user(conn, test_user_2)); }};

    // This isn't necessary, but it is here to catch potential regressions in regard to
    // password management.
    REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user_2, password_prop));

    // Become the second rodsuser, test_user_2.
    // This will cause the ticket on the connection to be disassociated from it.
    SwitchUserInput input{};
    std::strncpy(input.username, test_user_2.name.c_str(), sizeof(SwitchUserInput::username));
    std::strncpy(input.zone, env.rodsZone, sizeof(SwitchUserInput::zone));
    REQUIRE(rc_switch_user(static_cast<RcComm*>(test_user_conn), &input) == 0);

    // Show that test_user_2 cannot read the rodsadmin's home collection.
    CHECK_FALSE(fs::client::exists(test_user_conn, env.rodsHome));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_switch_user and KW_CLOSE_OPEN_REPLICAS")
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
    const auto data_object = sandbox / "foo";

    auto* conn_ptr = static_cast<RcComm*>(conn);

    dataObjInp_t open_inp{};
    std::strncpy(open_inp.objPath, data_object.c_str(), sizeof(DataObjInp::objPath) - 1);
    open_inp.openFlags = O_CREAT | O_WRONLY; // NOLINT(hicpp-signed-bitwise)
    const auto fd = rcDataObjOpen(conn_ptr, &open_inp);
    REQUIRE(fd > 2);
    REQUIRE(fs::client::is_data_object(conn, data_object));

    // Create a test user.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));

    irods::at_scope_exit remove_test_user{[&conn, &alice] { REQUIRE_NOTHROW(adm::client::remove_user(conn, alice)); }};

    SECTION("rc_switch_user returns an error when there are open replicas")
    {
        SwitchUserInput input{};
        std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
        std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
        CHECK(rc_switch_user(conn_ptr, &input) == SYS_NOT_ALLOWED);

        //
        // At this point, the replica is still open and needs to be closed.
        // The connection still represents the administrator too.
        //

        // Show the replica is still in the intermediate state.
        CHECK(irods::experimental::replica::replica_status(*conn_ptr, data_object, 0) == 2);

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(std::strcmp(conn_ptr->clientUser.userName, env.rodsUserName) == 0);
        CHECK(std::strcmp(conn_ptr->clientUser.rodsZone, env.rodsZone) == 0);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        // Close the replica.
        OpenedDataObjInp close_input{};
        close_input.l1descInx = fd;
        REQUIRE(rcDataObjClose(conn_ptr, &close_input) == 0);

        // Show the replica is now marked good.
        CHECK(irods::experimental::replica::replica_status(*conn_ptr, data_object, 0) == 1);
    }

    SECTION("rc_switch_user closes open replicas and marks them stale")
    {
        auto* conn_ptr = static_cast<RcComm*>(conn);

        // Become the test user.
        SwitchUserInput input{};
        std::strncpy(input.username, alice.name.c_str(), sizeof(SwitchUserInput::username));
        std::strncpy(input.zone, alice.zone.c_str(), sizeof(SwitchUserInput::zone));
        addKeyVal(&input.options, KW_CLOSE_OPEN_REPLICAS, ""); // This is the important step!
        REQUIRE(rc_switch_user(conn_ptr, &input) == 0);

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        CHECK(conn_ptr->clientUser.userName == alice.name);
        CHECK(conn_ptr->clientUser.rodsZone == alice.zone);
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        // Become the administrator so the replica status can be verified.
        // This is also required so the test can clean up properly.
        conn.disconnect();
        conn.connect();

        // Show the replica is now marked stale.
        CHECK(irods::experimental::replica::replica_status(*conn_ptr, data_object, 0) == 0);
    }
}
