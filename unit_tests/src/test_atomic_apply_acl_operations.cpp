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
#include "irods/zone_administration.hpp"

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

TEST_CASE("#7338")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn{irods::experimental::defer_authentication};

    char* json_error_string{};

    CHECK(rc_atomic_apply_acl_operations(static_cast<RcComm*>(conn), "", &json_error_string) == SYS_NO_API_PRIV);
    CHECK(json_error_string == nullptr);
}

TEST_CASE("#7408: Atomic acls respect entity zone")
{
    using namespace std::string_literals;

    rodsEnv env;
    _getRodsEnv(env);
    irods::experimental::client_connection conn;
    const auto* const user_home = env.rodsHome;
    const auto* const test_zone_name = "atomic_acl_test_zone";
    const auto* const acl_user_name = "atomic_acl_user";
    const auto* const remote_only_user_name = "atomic_acl_remote_zone_user";

    const auto* const local_zone_name = static_cast<const char* const>(env.rodsZone);

    // add zone for testing
    REQUIRE_NOTHROW(ua::client::add_zone(conn, test_zone_name));

    // clean up zone after end
    irods::at_scope_exit remove_test_zone{[&conn, &test_zone_name] {
        if (ua::client::zone_exists(conn, test_zone_name)) {
            ua::client::remove_zone(conn, test_zone_name);
        }
    }};

    // create 3 users:
    // two users with identical names but differing zones
    ua::user remote_zone_user{acl_user_name, test_zone_name};
    ua::user local_zone_user{acl_user_name};
    // one user with unique name in remote zone only
    ua::user remote_only_user{remote_only_user_name, test_zone_name};

    // clean up users after end
    irods::at_scope_exit remove_users{[&] {
        for (auto&& u : {remote_zone_user, local_zone_user, remote_only_user}) {
            CHECK_NOTHROW(ua::client::remove_user(conn, u));
        }
    }};

    REQUIRE_NOTHROW(ua::client::add_user(conn, remote_zone_user, ua::user_type::rodsuser, ua::zone_type::remote));
    REQUIRE_NOTHROW(ua::client::add_user(conn, remote_only_user, ua::user_type::rodsuser, ua::zone_type::remote));
    REQUIRE_NOTHROW(ua::client::add_user(conn, local_zone_user));

    REQUIRE(ua::client::exists(conn, remote_zone_user));
    REQUIRE(ua::client::exists(conn, remote_only_user));
    REQUIRE(ua::client::exists(conn, local_zone_user));

    // clean up acls after end
    irods::at_scope_exit remove_acls{[&] {
        // clang-format off
        const auto json_input = json{
            {"logical_path", user_home},
            {"operations", json::array({
                {
                    {"entity_name", remote_zone_user.name},
                    {"zone", test_zone_name},
                    {"acl", "null"}
                },
                {
                    {"entity_name", local_zone_user.name},
                    {"acl", "null"}
                },
                {
                    {"entity_name", remote_only_user.name},
                    {"zone", test_zone_name},
                    {"acl", "null"}
                },
            })}
        }.dump();
        // clang-format on

        char* json_error_string{};

        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) ==
                0);
        REQUIRE(json_error_string == "{}"s);

        const auto perms = fs::client::status(conn, user_home).permissions();
        auto b = std::begin(perms);
        auto e = std::end(perms);

        for (auto&& u : {remote_zone_user, local_zone_user, remote_only_user}) {
            REQUIRE(std::find_if(b, e, [&u](const fs::entity_permission& p) { return p.name == u.name; }) == e);
        }
    }};

    // clang-format off
    const auto failing_json_input = json{
        {"logical_path", user_home},
        {"operations", json::array({
            {
                {"entity_name", remote_only_user_name},
                {"acl", "own"}
            }
        })}
    }.dump();

    const auto success_json_input = json{
        {"logical_path", user_home},
        {"operations", json::array({
            {
                {"entity_name", acl_user_name},
                {"zone", test_zone_name},
                {"acl", "write"}
            }
        })}
    }.dump();

    const auto remote_only_success_json_input = json{
        {"logical_path", user_home},
        {"operations", json::array({
            {
                {"entity_name", remote_only_user_name},
                {"zone", test_zone_name},
                {"acl", "own"}
            }
        })}
    }.dump();

    const auto local_success_json_input = json{
        {"logical_path", user_home},
        {"operations", json::array({
            {
                {"entity_name", acl_user_name},
                {"acl", "read"}
            }
        })}
    }.dump();
    // clang-format on

    char* json_error_string{};
    std::vector<char*> err_strs;
    err_strs.reserve(4);
    irods::at_scope_exit free_errstrings{[&err_strs] {
        for (auto&& e : err_strs) {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            std::free(e);
        }
    }};
    // should fail: remote zone only user, but no zone specified
    REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn),
                                           failing_json_input.c_str(),
                                           &json_error_string) == SYS_INVALID_INPUT_PARAM);
    err_strs.push_back(json_error_string);
    json parsed_error_string = json::parse(json_error_string);
    REQUIRE(contains_error_information(json_error_string));
    REQUIRE_NOTHROW(parsed_error_string.at("error_message").get_ref<const std::string&>() ==
                    "SYS_INVALID_INPUT_PARAM: Invalid entity\\n\\n");

    // should succeed: remote zone only user with zone param
    REQUIRE(rc_atomic_apply_acl_operations(
                static_cast<rcComm_t*>(conn), remote_only_success_json_input.c_str(), &json_error_string) == 0);
    err_strs.push_back(json_error_string);
    REQUIRE(json_error_string == "{}"s);

    auto perms = fs::client::status(conn, user_home).permissions();
    auto b = std::begin(perms);
    auto e = std::end(perms);

    // success indicator: should find unique permission "own" with remote zone and remote-only username
    REQUIRE(std::find_if(b, e, [&remote_only_user_name, &test_zone_name](const fs::entity_permission& p) {
                return p.name == remote_only_user_name && p.zone == test_zone_name && p.prms == fs::perms::own;
            }) != e);

    // should succeed: remote zone user (with matching name as a local user) with zone param
    REQUIRE(rc_atomic_apply_acl_operations(
                static_cast<rcComm_t*>(conn), success_json_input.c_str(), &json_error_string) == 0);
    err_strs.push_back(json_error_string);
    REQUIRE(json_error_string == "{}"s);

    perms = fs::client::status(conn, user_home).permissions();
    b = std::begin(perms);
    e = std::end(perms);

    // success indicator: should find unique permission "write" with remote zone and shared username
    REQUIRE(std::find_if(b, e, [&acl_user_name, &test_zone_name](const fs::entity_permission& p) {
                return p.name == acl_user_name && p.zone == test_zone_name && p.prms == fs::perms::write;
            }) != e);

    // should NOT find any permission with shared username and local zone
    REQUIRE(std::find_if(b, e, [&acl_user_name, &local_zone_name](const fs::entity_permission& p) {
                return p.name == acl_user_name && p.zone == local_zone_name;
            }) == e);

    // should succeed: no zone param in json implies local zone
    // add acl for local zone user
    REQUIRE(rc_atomic_apply_acl_operations(
                static_cast<rcComm_t*>(conn), local_success_json_input.c_str(), &json_error_string) == 0);
    err_strs.push_back(json_error_string);
    REQUIRE(json_error_string == "{}"s);

    perms = fs::client::status(conn, user_home).permissions();
    b = std::begin(perms);
    e = std::end(perms);

    // success indicator: should find unique permission "read" in local zone with shared username
    REQUIRE(std::find_if(b, e, [&acl_user_name, &local_zone_name](const fs::entity_permission& p) {
                return p.name == acl_user_name && p.zone == local_zone_name && p.prms == fs::perms::read;
            }) != e);
}

TEST_CASE("#7913: Atomic acls accept all acls")
{
    using namespace std::string_literals;

    rodsEnv env;
    _getRodsEnv(env);
    irods::experimental::client_connection conn;
    const auto* const user_home = env.rodsHome;
    const auto* const acl_user_name = "atomic_acl_user";

    ua::user acl_user{acl_user_name};

    // clean up users after end
    irods::at_scope_exit remove_users{[&] { CHECK_NOTHROW(ua::client::remove_user(conn, acl_user)); }};

    REQUIRE_NOTHROW(ua::client::add_user(conn, acl_user));

    REQUIRE(ua::client::exists(conn, acl_user));

    // create test collection to modify ACLs on
    const auto test_collection = fs::path{user_home} / "acl_test_collection_issue_7913";
    REQUIRE_NOTHROW(fs::client::create_collection(conn, test_collection));
    irods::at_scope_exit remove_test_collection{
        [&] { REQUIRE(fs::client::remove_all(conn, test_collection, fs::remove_options::no_trash)); }};

    const std::array new_acls_and_perms{std::make_pair("read_metadata", fs::perms::read_metadata),
                                        std::make_pair("read", fs::perms::read),
                                        std::make_pair("read_object", fs::perms::read),
                                        std::make_pair("create_metadata", fs::perms::create_metadata),
                                        std::make_pair("modify_metadata", fs::perms::modify_metadata),
                                        std::make_pair("delete_metadata", fs::perms::delete_metadata),
                                        std::make_pair("create_object", fs::perms::create_object),
                                        std::make_pair("write", fs::perms::write),
                                        std::make_pair("modify_object", fs::perms::write),
                                        std::make_pair("delete_object", fs::perms::delete_object),
                                        std::make_pair("own", fs::perms::own)};

    for (const auto& acl_and_perm : new_acls_and_perms) {
        // clang-format off
        const auto json_input = json{
            {"logical_path", test_collection.c_str()},
            {"operations", json::array({
                {
                    {"entity_name", acl_user_name},
                    {"acl", acl_and_perm.first}
                }
            })}
        }.dump();
        // clang-format on

        // clean up acl after each iteration
        irods::at_scope_exit remove_acls{[&] {
            // clang-format off
            const auto json_input = json{
                {"logical_path", test_collection.c_str()},
                {"operations", json::array({
                    {
                        {"entity_name", acl_user.name},
                        {"acl", "null"}
                    },
                })}
            }.dump();
            // clang-format on

            char* json_error_string{};

            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

            REQUIRE(rc_atomic_apply_acl_operations(
                        static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) == 0);
            REQUIRE(json_error_string == "{}"s);

            const auto perms = fs::client::status(conn, test_collection.c_str()).permissions();
            auto b = std::begin(perms);
            auto e = std::end(perms);

            REQUIRE(
                std::none_of(b, e, [&acl_user](const fs::entity_permission& p) { return p.name == acl_user.name; }));
        }};

        char* json_error_string{nullptr};

        irods::at_scope_exit free_errstring{[&json_error_string] {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            std::free(json_error_string);
        }};

        REQUIRE(rc_atomic_apply_acl_operations(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_error_string) ==
                0);
        REQUIRE(json_error_string == "{}"s);

        const auto perms = fs::client::status(conn, test_collection.c_str()).permissions();
        auto b = std::begin(perms);
        auto e = std::end(perms);

        // success indicator: should find permission
        REQUIRE(std::any_of(b, e, [&acl_user_name, &acl_and_perm](const fs::entity_permission& p) {
            return p.name == acl_user_name && p.prms == acl_and_perm.second;
        }));
    }
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
