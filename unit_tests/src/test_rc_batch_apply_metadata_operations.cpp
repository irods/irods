#include "catch.hpp"

#include "rodsClient.h"
#include "batch_apply_metadata_operations.h"

#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "connection_pool.hpp"
#include "filesystem/path.hpp"

#include "json.hpp"

#include <string>

namespace fs = irods::experimental::filesystem;

using json = nlohmann::json;

auto contains_error_information(const char* _json_error_info) -> bool;

TEST_CASE("parallel transfer engine")
{
    using namespace std::string_literals;

    auto& api_table = irods::get_client_api_table();
    auto& pck_table = irods::get_pack_table();
    init_api_table(api_table, pck_table);

    const auto connection_count = 1;
    const auto refresh_time = 600;

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) >= 0);

    irods::connection_pool conn_pool{connection_count,
                                     env.rodsHost,
                                     env.rodsPort,
                                     env.rodsUserName,
                                     env.rodsZone,
                                     refresh_time};

    auto* conn = &static_cast<rcComm_t&>(conn_pool.get_connection());

    const auto user_home = fs::path{env.rodsHome}.string();

    SECTION("all operations succeed")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "the_attr"},
                {"value", "the_val"},
                {"units", "the_units"}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "name"},
                {"value", "kory draughn"},
                {"units", ""}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "remove"},
                {"attribute", "name"},
                {"value", "kory draughn"},
                {"units", ""}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "occupation"},
                {"value", "software developer"},
                {"units", "C++"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("all operations succeed without specifying 'units'")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "the_attr"},
                {"value", "the_val"}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "name"},
                {"value", "kory draughn"}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "remove"},
                {"attribute", "name"},
                {"value", "kory draughn"}
            },
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "set"},
                {"attribute", "occupation"},
                {"value", "software developer"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) == 0);
        REQUIRE(json_error_string == "{}"s);
    }

    SECTION("incorrect json structure")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"attribute", "the_attr"},
                {"units", "the_units"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) != 0);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("incorrect property name")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"__operation", "set"},
                {"attribte", "the_attr"},
                {"units", "the_units"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) != 0);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("incorrect property value")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"__operation", "add"},
                {"attribte", "the_attr"},
                {"units", "the_units"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) != 0);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("object type does not match the 'entity_type'")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "data_object"},
                {"operation", "set"},
                {"attribute", "the_attr"},
                {"value", "the_val"},
                {"units", "the_units"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) != 0);
        REQUIRE(contains_error_information(json_error_string));
    }

    SECTION("remove non-existent metadata")
    {
        const auto json_input = json::array({
            {
                {"entity_name", user_home},
                {"entity_type", "collection"},
                {"operation", "remove"},
                {"attribute", "non_existent_attr"},
                {"value", "non_existent_value"},
                {"units", "non_existent_units"}
            }
        }).dump();

        char* json_error_string{};

        REQUIRE(rc_batch_apply_metadata_operations(conn, json_input.c_str(), &json_error_string) != 0);
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
