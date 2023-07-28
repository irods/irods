#include <catch2/catch.hpp>

#include "irods/check_auth_credentials.h"
#include "irods/client_connection.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"

#include <cstring>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_check_auth_credentials basic functionality")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    // This connection is used for creating the test user and cleaning up.
    irods::experimental::client_connection conn;

    namespace adm = irods::experimental::administration;

    // Create a test user.
    const adm::user alice{"test_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, alice));
    irods::at_scope_exit remove_test_user{[&conn, &alice] { CHECK_NOTHROW(adm::client::remove_user(conn, alice)); }};

    // Set the test user's password.
    adm::user_password_property password_prop{"apass"};
    REQUIRE_NOTHROW(adm::client::modify_user(conn, alice, password_prop));

    auto* conn_ptr = static_cast<RcComm*>(conn);

    CheckAuthCredentialsInput input{};
    std::strncpy(input.username, alice.name.c_str(), sizeof(CheckAuthCredentialsInput::username));
    std::strncpy(input.zone, env.rodsZone, sizeof(CheckAuthCredentialsInput::zone));

    int* correct{};

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    irods::at_scope_exit free_result{[&correct] { std::free(correct); }};

    SECTION("would_succeed member variable is set to 1 (true) on correct password")
    {
        const auto scrambled_password = adm::obfuscate_password(password_prop);
        std::strncpy(input.password, scrambled_password.c_str(), sizeof(CheckAuthCredentialsInput::password));
        CHECK(rc_check_auth_credentials(conn_ptr, &input, &correct) == 0);
        CHECK(correct);
        CHECK(1 == *correct);
    }

    SECTION("would_succeed member variable is set to 0 (false) on incorrect password")
    {
        password_prop.value = "bogus";
        const auto scrambled_password = adm::obfuscate_password(password_prop);
        std::strncpy(input.password, scrambled_password.c_str(), sizeof(CheckAuthCredentialsInput::password));
        CHECK(rc_check_auth_credentials(conn_ptr, &input, &correct) == 0);
        CHECK(correct);
        CHECK(0 == *correct);
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_check_auth_credentials returns error on invalid input")
{
    //
    // IMPORTANT: This test requires access to a rodsadmin user!
    //

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    // This connection is used for creating the test user and cleaning up.
    irods::experimental::client_connection conn;

    auto* conn_ptr = static_cast<RcComm*>(conn);

    CheckAuthCredentialsInput input{};
    int* correct{};

    SECTION("returns error on nullptr for RcComm")
    {
        CHECK(rc_check_auth_credentials(nullptr, &input, &correct) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }

    SECTION("returns error on nullptr for CheckAuthCredentialsInput")
    {
        CHECK(rc_check_auth_credentials(conn_ptr, nullptr, &correct) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }

    SECTION("returns error on nullptr for CheckAuthCredentialsOutput")
    {
        CHECK(rc_check_auth_credentials(conn_ptr, &input, nullptr) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }

    SECTION("returns error on empty string for username")
    {
        std::strncpy(input.username, "", sizeof(CheckAuthCredentialsInput::username));
        std::strncpy(input.zone, env.rodsZone, sizeof(CheckAuthCredentialsInput::zone));
        std::strncpy(input.password, "rods", sizeof(CheckAuthCredentialsInput::password));

        CHECK(rc_check_auth_credentials(conn_ptr, &input, &correct) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }

    SECTION("returns error on empty string for zone")
    {
        std::strncpy(input.username, env.rodsUserName, sizeof(CheckAuthCredentialsInput::username));
        std::strncpy(input.zone, "", sizeof(CheckAuthCredentialsInput::zone));
        std::strncpy(input.password, "rods", sizeof(CheckAuthCredentialsInput::password));

        CHECK(rc_check_auth_credentials(conn_ptr, &input, &correct) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }

    SECTION("returns error on empty string for password")
    {
        std::strncpy(input.username, env.rodsUserName, sizeof(CheckAuthCredentialsInput::username));
        std::strncpy(input.zone, env.rodsZone, sizeof(CheckAuthCredentialsInput::zone));
        std::strncpy(input.password, "", sizeof(CheckAuthCredentialsInput::password));

        CHECK(rc_check_auth_credentials(conn_ptr, &input, &correct) == SYS_INVALID_INPUT_PARAM);
        CHECK(nullptr == correct);
    }
}
