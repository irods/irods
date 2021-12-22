#include <catch2/catch.hpp>

#include "rodsClient.h"
#include "atomic_apply_acl_operations.h"

#include "connection_pool.hpp"
#include "filesystem.hpp"
#include "user_administration.hpp"
#include "irods_at_scope_exit.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>
#include <iostream>

namespace fs = irods::experimental::filesystem;
namespace ua = irods::experimental::administration;

using json = nlohmann::json;

auto contains_error_information(const char* _json_error_info) -> bool;

TEST_CASE("atomic_apply_acl_operations")
{
    using namespace std::string_literals;

    load_client_api_plugins();

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

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

