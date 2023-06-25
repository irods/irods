#include <catch2/catch.hpp>

#include "irods/rcConnect.h"
#include "irods/rodsClient.h"
#include "irods/atomic_apply_acl_operations.h"

#include "irods/client_connection.hpp"
#include "irods/filesystem.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/transport/default_transport.hpp"
#include "irods/dstream.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>
#include <iostream>
#include <utility>

namespace fs = irods::experimental::filesystem;
namespace ua = irods::experimental::administration;
namespace io = irods::experimental::io;

using json = nlohmann::json;

auto contains_error_information(const char* _json_string) -> bool;

TEST_CASE("atomic_apply_acl_operations")
{
    using namespace std::string_literals;

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    rodsEnv env;
    _getRodsEnv(env);
    const auto user_home = fs::path{env.rodsHome}.string();

    SECTION("set and remove ACLs")
    {
        ua::user user_a{"nintendo_link"};
        ua::user user_b{"sony_kratos"};
        ua::user user_c{"microsoft_master_chief"};
        ua::group group{"sega_sonic"};

        irods::at_scope_exit remove_users{[&] {
            for (auto&& u : {user_a, user_b, user_c}) {
                ua::client::remove_user(conn, u);
            }

            ua::client::remove_group(conn, group);
        }};

        for (auto&& u : {user_a, user_b, user_c}) {
            if (!ua::client::exists(conn, u)) {
                ua::client::add_user(conn, u);
            }

            if (!ua::client::exists(conn, group)) {
                ua::client::add_group(conn, group);
            }
        }

        REQUIRE(ua::client::exists(conn, user_a));
        REQUIRE(ua::client::exists(conn, user_b));
        REQUIRE(ua::client::exists(conn, user_c));
        REQUIRE(ua::client::exists(conn, group));

        irods::at_scope_exit remove_acls{[&] {
            const auto json_input = json{
                {"logical_path", user_home},
                {"operations", json::array({
                    {
                        {"entity_name", user_a.name},
                        {"acl", "null"}
                    },
                    {
                        {"entity_name", user_b.name},
                        {"acl", "null"}
                    },
                    {
                        {"entity_name", user_c.name},
                        {"acl", "null"}
                    },
                    {
                        {"entity_name", group.name},
                        {"acl", "null"}
                    }
                })}
            }.dump();

            char* json_error_string{};
            irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

            REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) == 0);
            REQUIRE(json_error_string == "{}"s);

            const auto perms = fs::client::status(conn, user_home).permissions();
            auto b = std::begin(perms);
            auto e = std::end(perms);

            REQUIRE(std::find_if(b, e, [&user_a](const fs::entity_permission& p) { return p.name == user_a.name && p.prms == fs::perms::read; }) == e);
            REQUIRE(std::find_if(b, e, [&user_b](const fs::entity_permission& p) { return p.name == user_b.name && p.prms == fs::perms::write; }) == e);
            REQUIRE(std::find_if(b, e, [&user_c](const fs::entity_permission& p) { return p.name == user_c.name && p.prms == fs::perms::own; }) == e);
            REQUIRE(std::find_if(b, e, [&group](const fs::entity_permission& p) { return p.name == group.name && p.prms == fs::perms::read; }) == e);
        }};

        const auto json_input = json{
            {"logical_path", user_home},
            {"operations", json::array({
                {
                    {"entity_name", user_a.name},
                    {"acl", "read"}
                },
                {
                    {"entity_name", user_b.name},
                    {"acl", "write"}
                },
                {
                    {"entity_name", user_c.name},
                    {"acl", "own"}
                },
                {
                    {"entity_name", group.name},
                    {"acl", "read"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);

        const auto perms = fs::client::status(conn, user_home).permissions();
        auto b = std::begin(perms);
        auto e = std::end(perms);

        REQUIRE(std::find_if(b, e, [&user_a](const fs::entity_permission& p) { return p.name == user_a.name && p.prms == fs::perms::read; }) != e);
        REQUIRE(std::find_if(b, e, [&user_b](const fs::entity_permission& p) { return p.name == user_b.name && p.prms == fs::perms::write; }) != e);
        REQUIRE(std::find_if(b, e, [&user_c](const fs::entity_permission& p) { return p.name == user_c.name && p.prms == fs::perms::own; }) != e);
        REQUIRE(std::find_if(b, e, [&group](const fs::entity_permission& p) { return p.name == group.name && p.prms == fs::perms::read; }) != e);
    }

    SECTION("None existent path generates an error")
    {
        const auto json_input = json{
            {"logical_path", "/tempZone/home/does_not_exist123"}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) == OBJ_PATH_DOES_NOT_EXIST);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("Invalid ACL value generates an error")
    {
        const auto json_input = json{
            {"logical_path", user_home},
            {"operations", json::array({
                {
                    {"entity_name", env.rodsUserName},
                    {"acl", "bad_input"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE_FALSE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) == 0);
        REQUIRE(contains_error_information(json_error_string));
    }
}

TEST_CASE("Administrators are allowed to use the admin_mode option")
{
    //
    // This test requires that the environment represent a rodsadmin user.
    //

    using namespace std::string_literals;

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    // Create a test rodsuser.
    ua::user test_user{"unit_test_test_user"};
    irods::at_scope_exit remove_user{[&conn, &test_user] { ua::client::remove_user(conn, test_user); }};
    REQUIRE_NOTHROW(ua::client::add_user(conn, test_user));

    // Create a test rodsgroup.
    ua::group test_group{"unit_test_test_group"};
    irods::at_scope_exit remove_group{[&conn, &test_group] { ua::client::remove_group(conn, test_group); }};
    REQUIRE_NOTHROW(ua::client::add_group(conn, test_group));

    // Capture the home collection of the test user.
    rodsEnv env;
    _getRodsEnv(env);
    const auto test_user_home = fs::path{"/"} / env.rodsZone / "home" / test_user.name;

    // Show that the administrator (identified by "conn") cannot manipulate metadata on the
    // test user's home collection.
    const fs::metadata avu{"attr_name_6198", "attr_value_6198", "attr_units_6198"};
    CHECK_THROWS(fs::client::add_metadata(conn, test_user_home, avu));

    // clang-format off
    // Show that the admin can grant permissions to themself via the admin mode option.
    const auto json_input = json{
        {"admin_mode", true},
        {"logical_path", test_user_home.c_str()},
        {"operations", json::array({
            {
                {"entity_name", env.rodsUserName},
                {"acl", "write"}
            },
            {
                {"entity_name", test_group.name},
                {"acl", "read"}
            }
        })}
    }.dump();
    // clang-format on

    char* json_error_string{};
    irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

    CHECK(rc_atomic_apply_acl_operations(static_cast<RcComm*>(conn), json_input.c_str(), &json_error_string) == 0);
    CHECK(json_error_string == "{}"s);

    // Show that the permissions were updated successfully.
    const auto perms = fs::client::status(conn, test_user_home).permissions();
    auto b = std::begin(perms);
    auto e = std::end(perms);

    const auto check_rodsadmin_perms = [&env](const fs::entity_permission& _p) {
        return _p.name == env.rodsUserName && _p.prms == fs::perms::write;
    };
    CHECK(std::find_if(b, e, check_rodsadmin_perms) != e);

    const auto check_test_group_perms = [&test_group](const fs::entity_permission& _p) {
        return _p.name == test_group.name && _p.prms == fs::perms::read;
    };
    CHECK(std::find_if(b, e, check_test_group_perms) != e);

    // Show that the administrator can now manipulate metadata on the test user's home collection.
    CHECK_NOTHROW(fs::client::add_metadata(conn, test_user_home, avu));
    const auto metadata = fs::client::get_metadata(conn, test_user_home);
    REQUIRE(!metadata.empty());
    CHECK(metadata[0].attribute == avu.attribute);
    CHECK(metadata[0].value == avu.value);
    CHECK(metadata[0].units == avu.units);
}

TEST_CASE("Non-admin users are not allowed to use the admin_mode option")
{
    //
    // This test requires that the environment represent a rodsadmin user.
    //

    using namespace std::string_literals;

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    // Create a test rodsuser.
    ua::user test_user{"unit_test_test_user"};
    irods::at_scope_exit remove_user{[&conn, &test_user] {
        conn.disconnect();
        conn.connect();
        ua::client::remove_user(conn, test_user);
    }};
    CHECK_NOTHROW(ua::client::add_user(conn, test_user));
    REQUIRE_NOTHROW(ua::client::modify_user(conn, test_user, ua::user_password_property{"rods"}));

    // Connect to server as the test user.
    conn.disconnect();
    rodsEnv env;
    _getRodsEnv(env);
    conn.connect(env.rodsHost, env.rodsPort, {test_user.name.c_str(), env.rodsZone});

    // Capture the home collection of the test user.
    const auto test_user_home = fs::path{"/"} / env.rodsZone / "home" / test_user.name;

    // Show that the test user cannot use the admin_mode option to grant permissions
    // even on their own collection.

    // clang-format off
    const auto test_input = {
        std::pair<std::string_view, int>{env.rodsHome, OBJ_PATH_DOES_NOT_EXIST},
        std::pair<std::string_view, int>{test_user_home.c_str(), CAT_INSUFFICIENT_PRIVILEGE_LEVEL}
    };
    // clang-format on

    for (auto&& [path, ec] : test_input) {
        // clang-format off
        const auto json_input = json{
            {"admin_mode", true},
            {"logical_path", path},
            {"operations", json::array({
                {
                    {"entity_name", env.rodsUserName},
                    {"acl", "read"}
                }
            })}
        }.dump();
        // clang-format on

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        CHECK(rc_atomic_apply_acl_operations(static_cast<RcComm*>(conn), json_input.c_str(), &json_error_string) == ec);
        CHECK(contains_error_information(json_error_string));
    }

    // Show that the permissions have not changed.
    CHECK(fs::client::status(conn, test_user_home).permissions().size() == 1);
}

TEST_CASE("Non-admin users can modify ACLs of collections and data objects")
{
    //
    // This test requires that the environment represent a rodsadmin user.
    //

    using namespace std::string_literals;

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    // Create a test rodsuser.
    ua::user test_user{"unit_test_test_user"};
    irods::at_scope_exit remove_user{[&conn, &test_user] {
        conn.disconnect();
        conn.connect();
        ua::client::remove_user(conn, test_user);
    }};
    CHECK_NOTHROW(ua::client::add_user(conn, test_user));
    REQUIRE_NOTHROW(ua::client::modify_user(conn, test_user, ua::user_password_property{"rods"}));

    // Capture the local user's environment information.
    rodsEnv env;
    _getRodsEnv(env);

    // Connect to server as the test user.
    rErrMsg_t error;
    auto* conn_ptr = rcConnect(env.rodsHost, env.rodsPort, test_user.name.c_str(), env.rodsZone, 0, &error);
    REQUIRE(conn_ptr);
    irods::at_scope_exit disconnect_test_user{[conn_ptr] { rcDisconnect(conn_ptr); }};
    char password[] = "rods";
    REQUIRE(clientLoginWithPassword(conn_ptr, password) == 0);

    // Capture the home collection of the test user.
    const auto test_user_home = fs::path{"/"} / env.rodsZone / "home" / test_user.name;

    // Create a test data object.
    const auto test_data_object = test_user_home / "unit_test_test_object.txt";

    {
        io::client::default_transport tp{*conn_ptr};
        io::odstream{tp, test_data_object} << "this is some test data.";
        REQUIRE(fs::client::exists(*conn_ptr, test_data_object));
    }

    irods::at_scope_exit delete_data_object{[conn_ptr, &test_data_object] {
        fs::client::remove(*conn_ptr, test_data_object, fs::remove_options::no_trash);
    }};

    // Show that the test user can modify permissions on collections and data objects they
    // have access to.

    // clang-format off
    const auto test_input = {
        std::pair<fs::path, std::string_view>{test_user_home, "read"},
        std::pair<fs::path, std::string_view>{test_data_object, "write"}
    };
    // clang-format on

    for (auto&& [path, permission] : test_input) {
        // clang-format off
        const auto json_input = json{
            {"logical_path", path},
            {"operations", json::array({
                {
                    {"entity_name", env.rodsUserName},
                    {"acl", permission}
                }
            })}
        }.dump();
        // clang-format on

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        CHECK(rc_atomic_apply_acl_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        CHECK(json_error_string == "{}"s);
    }

    // Show that the permissions were updated successfully.
    const auto has_expected_permissions = [conn_ptr, &env](const auto& _path, const auto& _perm) {
        const auto perms = fs::client::status(*conn_ptr, _path).permissions();

        // clang-format off
        return std::find_if(std::begin(perms), std::end(perms), [&env, _perm](const fs::entity_permission& _p) {
            return _p.name == env.rodsUserName && _p.prms == _perm;
        }) != std::end(perms);
        // clang-format on
    };

    CHECK(has_expected_permissions(test_user_home, fs::perms::read));
    CHECK(has_expected_permissions(test_data_object, fs::perms::write));
}

auto contains_error_information(const char* _json_string) -> bool
{
    try {
        const auto err_info = json::parse(_json_string);

        return (err_info.count("operation") > 0 &&
                err_info.count("operation_index") > 0 &&
                err_info.count("error_message") > 0);
    }
    catch (...) {
        return false;
    }
}

