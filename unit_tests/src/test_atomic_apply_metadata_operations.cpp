#include "catch.hpp"

#include "client_connection.hpp"
#include "atomic_apply_metadata_operations.h"
#include "filesystem/path.hpp"
#include "irods_at_scope_exit.hpp"
#include "rodsErrorTable.h"
#include "getRodsEnv.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

using json = nlohmann::json;

namespace fs = irods::experimental::filesystem;

extern "C" auto load_client_api_plugins() -> void;

auto contains_error_information(const char* _json_error_info) -> bool;
auto remove_all_metadata(RcComm& _conn, const rodsEnv& _env, const fs::path& _user_home) -> void;

TEST_CASE("atomic_apply_metadata_operations")
{
    using namespace std::string_literals;

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;
    auto* conn_ptr = static_cast<RcComm*>(conn);

    const auto user_home = fs::path{env.rodsHome}.string();

    irods::at_scope_exit clean_up{[&] {
        remove_all_metadata(conn, env, user_home);
    }};

    SECTION("all operations succeed")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"},
                    {"units", "the_units"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "name"},
                    {"value", "john doe"},
                    {"units", ""}
                },
                {
                    {"operation", "add"},
                    {"attribute", "a0"},
                    {"value", "v0"},
                    {"units", "u0"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "a1"},
                    {"value", "v1"},
                    {"units", "u1"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "a2"},
                    {"value", "v2"},
                    {"units", "u2"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "a3"},
                    {"value", "v3"},
                    {"units", "u3"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"},
                    {"units", ""}
                },
                {
                    {"operation", "add"},
                    {"attribute", "occupation"},
                    {"value", "software developer"},
                    {"units", "C++"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("all operations succeed without specifying 'units'")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "name"},
                    {"value", "john doe"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "occupation"},
                    {"value", "software developer"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("incorrect json structure")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"attribute", "the_attr"},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == JSON_VALIDATION_ERROR);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("incorrect property name")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"__operation", "add"},
                    {"attribte", "the_attr"},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == JSON_VALIDATION_ERROR);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("invalid operation value")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"operation", "update"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == INVALID_OPERATION);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("object type does not match the 'entity_type'")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "data_object"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == SYS_INVALID_INPUT_PARAM);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("remove non-existent metadata")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"operation", "remove"},
                    {"attribute", "non_existent_attr"},
                    {"value", "non_existent_value"},
                    {"units", "non_existent_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("users")
    {
        const auto json_input = json{
            {"entity_name", env.rodsUserName},
            {"entity_type", "user"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "irods_user"},
                    {"value", env.rodsUserName},
                    {"units", "rodsadmin"}
                },
                {
                    {"operation", "add"},
                    {"attribute", "renci_position"},
                    {"value", "research_developer"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name_does_not_exist"},
                    {"value", "value_does_not_exist"},
                    {"units", "units_does_not_exist"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("resources")
    {
        const auto json_input = json{
            {"entity_name", env.rodsDefResource},
            {"entity_type", "resource"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "this_was_set_atomically"},
                    {"value", "it worked!"},
                },
                {
                    {"operation", "add"},
                    {"attribute", "the_atomic_attribute_name"},
                    {"value", "the_atomic_value"},
                    {"units", "the_atomic_units"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "this_was_set_atomically"},
                    {"value", "it worked!"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("do not accept empty attribute names")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "data_object"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", ""},
                    {"value", "the_val"},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == SYS_INVALID_INPUT_PARAM);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("do not accept empty attribute values")
    {
        const auto json_input = json{
            {"entity_name", user_home},
            {"entity_type", "data_object"},
            {"operations", json::array({
                {
                    {"operation", "add"},
                    {"attribute", "the_attr"},
                    {"value", ""},
                    {"units", "the_units"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == SYS_INVALID_INPUT_PARAM);
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

auto remove_all_metadata(RcComm& _conn, const rodsEnv& _env, const fs::path& _user_home) -> void
{
    using namespace std::string_literals;

    {
        const auto json_input = json{
            {"entity_name", _user_home},
            {"entity_type", "collection"},
            {"operations", json::array({
                {
                    {"operation", "remove"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"},
                    {"units", "the_units"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"},
                    {"units", ""}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "a0"},
                    {"value", "v0"},
                    {"units", "u0"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "a1"},
                    {"value", "v1"},
                    {"units", "u1"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "a2"},
                    {"value", "v2"},
                    {"units", "u2"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "a3"},
                    {"value", "v3"},
                    {"units", "u3"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"},
                    {"units", ""}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "occupation"},
                    {"value", "software developer"},
                    {"units", "C++"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "the_attr"},
                    {"value", "the_val"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name"},
                    {"value", "john doe"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "occupation"},
                    {"value", "software developer"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(&_conn, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    {
        const auto json_input = json{
            {"entity_name", _env.rodsUserName},
            {"entity_type", "user"},
            {"operations", json::array({
                {
                    {"operation", "remove"},
                    {"attribute", "irods_user"},
                    {"value", _env.rodsUserName},
                    {"units", "rodsadmin"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "renci_position"},
                    {"value", "research_developer"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "name_does_not_exist"},
                    {"value", "value_does_not_exist"},
                    {"units", "units_does_not_exist"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(&_conn, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    {
        const auto json_input = json{
            {"entity_name", _env.rodsDefResource},
            {"entity_type", "resource"},
            {"operations", json::array({
                {
                    {"operation", "remove"},
                    {"attribute", "this_was_set_atomically"},
                    {"value", "it worked!"},
                },
                {
                    {"operation", "remove"},
                    {"attribute", "the_atomic_attribute_name"},
                    {"value", "the_atomic_value"},
                    {"units", "the_atomic_units"}
                },
                {
                    {"operation", "remove"},
                    {"attribute", "this_was_set_atomically"},
                    {"value", "it worked!"}
                }
            })}
        }.dump();

        char* json_error_string{};
        irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

        REQUIRE(rc_atomic_apply_metadata_operations(&_conn, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }
}

