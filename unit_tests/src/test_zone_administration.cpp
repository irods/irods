#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rodsClient.h"
#include "irods/zone_administration.hpp"

TEST_CASE("basic zone administration operations")
{
    load_client_api_plugins();

    const auto* const test_zone_name_1 = "unit_test_test_zone_1";
    const auto* const test_zone_name_2 = "unit_test_test_zone_2";
    const auto* const conn_info_1 = "localhost:1247";
    const auto* const comment_1 = "original comment.";

    irods::experimental::client_connection conn;

    namespace adm = irods::experimental::administration;

    SECTION("create zone without connection information or comments")
    {
        REQUIRE_NOTHROW(adm::client::add_zone(conn, test_zone_name_1));

        const auto zone_info = adm::client::zone_info(conn, test_zone_name_1);
        REQUIRE(zone_info.has_value());
        CHECK(zone_info->type == adm::zone_type::remote);
        CHECK(zone_info->name == test_zone_name_1);
        CHECK(zone_info->connection_info.empty());
        CHECK(zone_info->comment.empty());
    }

    SECTION("create zone with connection information")
    {
        REQUIRE_NOTHROW(adm::client::add_zone(conn, test_zone_name_1, {.connection_info = conn_info_1}));

        const auto zone_info = adm::client::zone_info(conn, test_zone_name_1);
        REQUIRE(zone_info.has_value());
        CHECK(zone_info->type == adm::zone_type::remote);
        CHECK(zone_info->name == test_zone_name_1);
        CHECK(zone_info->connection_info == conn_info_1);
        CHECK(zone_info->comment.empty());
    }

    SECTION("create zone with comment")
    {
        REQUIRE_NOTHROW(adm::client::add_zone(conn, test_zone_name_1, {.comment = comment_1}));

        const auto zone_info = adm::client::zone_info(conn, test_zone_name_1);
        REQUIRE(zone_info.has_value());
        CHECK(zone_info->type == adm::zone_type::remote);
        CHECK(zone_info->name == test_zone_name_1);
        CHECK(zone_info->connection_info.empty());
        CHECK(zone_info->comment == comment_1);
    }

    SECTION("create zone with connection information and comment")
    {
        REQUIRE_NOTHROW(adm::client::add_zone(conn, test_zone_name_1, {conn_info_1, comment_1}));

        const auto zone_info = adm::client::zone_info(conn, test_zone_name_1);
        REQUIRE(zone_info.has_value());
        CHECK(zone_info->type == adm::zone_type::remote);
        CHECK(zone_info->name == test_zone_name_1);
        CHECK(zone_info->connection_info == conn_info_1);
        CHECK(zone_info->comment == comment_1);
    }

    // Make sure to remove zone(s) on completion (this covers exceptions too).
    irods::at_scope_exit remove_test_zones{[&conn, &test_zone_name_1, &test_zone_name_2] {
        if (adm::client::zone_exists(conn, test_zone_name_1)) {
            adm::client::remove_zone(conn, test_zone_name_1);
        }

        if (adm::client::zone_exists(conn, test_zone_name_2)) {
            adm::client::remove_zone(conn, test_zone_name_2);
        }
    }};

    // Show that zones can be renamed.
    CHECK_FALSE(adm::client::zone_exists(conn, test_zone_name_2));
    CHECK_NOTHROW(adm::client::modify_zone(conn, test_zone_name_1, adm::zone_name_property{test_zone_name_2}));
    CHECK_FALSE(adm::client::zone_exists(conn, test_zone_name_1));

    CHECK_NOTHROW(adm::client::modify_zone(conn, test_zone_name_2, adm::zone_name_property{test_zone_name_1}));
    CHECK(adm::client::zone_exists(conn, test_zone_name_1));

    // Show that the connection information can be adjusted.
    auto zone_info = adm::client::zone_info(conn, test_zone_name_1);
    REQUIRE(zone_info.has_value());
    CHECK(zone_info->type == adm::zone_type::remote);
    CHECK(zone_info->name == test_zone_name_1);

    const auto* const conn_info_2 = "unit-test.example.org:1247";
    CHECK_NOTHROW(adm::client::modify_zone(conn, test_zone_name_1, adm::connection_info_property{conn_info_2}));

    zone_info = adm::client::zone_info(conn, test_zone_name_1);
    REQUIRE(zone_info.has_value());
    CHECK(zone_info->type == adm::zone_type::remote);
    CHECK(zone_info->name == test_zone_name_1);
    CHECK(zone_info->connection_info == conn_info_2);

    // Show that the comments can be adjusted.
    const auto* const comment_2 = "updated comment.";
    CHECK_NOTHROW(adm::client::modify_zone(conn, test_zone_name_1, adm::comment_property{comment_2}));

    zone_info = adm::client::zone_info(conn, test_zone_name_1);
    REQUIRE(zone_info.has_value());
    CHECK(zone_info->type == adm::zone_type::remote);
    CHECK(zone_info->name == test_zone_name_1);
    CHECK(zone_info->connection_info == conn_info_2);
    CHECK(zone_info->comment == comment_2);

    // Show that the handling of the zone collection in regards to strict-acls can be modified.
    const adm::zone_collection_acl_property prop{adm::zone_collection_acl::read, "rods"};
    CHECK_NOTHROW(adm::client::modify_zone(conn, test_zone_name_1, prop));

    // Enable the following lines to show that the constraints applied to "modify_zone()" via
    // the ZoneProperty concept, are honored and result in a compile-time error rather than a
    // run-time error.
#if 0
    struct unsupported_type {};
    adm::client::modify_zone(conn, test_zone_name_1, unsupported_type{});
#endif
}

TEST_CASE("exceptions")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;

    namespace adm = irods::experimental::administration;

    SECTION("empty zone name")
    {
        CHECK_THROWS_AS(adm::client::add_zone(conn, ""), irods::exception);
        CHECK_THROWS_AS(adm::client::remove_zone(conn, ""), irods::exception);
        CHECK_THROWS_AS(adm::client::zone_exists(conn, ""), irods::exception);
        CHECK_THROWS_AS(adm::client::zone_info(conn, ""), irods::exception);
        CHECK_THROWS_AS(adm::client::modify_zone(conn, "", adm::zone_name_property{"otherZone"}), irods::exception);
    }

    SECTION("non-existent zone name")
    {
        CHECK_THROWS_AS(adm::client::remove_zone(conn, "doesnotexist"), irods::exception);
        CHECK_THROWS_AS(
            adm::client::modify_zone(conn, "doesnotexist", adm::zone_name_property{"otherZone"}), irods::exception);
    }
}

