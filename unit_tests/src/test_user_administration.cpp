#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rodsClient.h"
#include "irods/user_administration.hpp"
#include "irods/zone_administration.hpp"

#include <algorithm>
#include <iterator>

TEST_CASE("user group administration")
{
    namespace adm = irods::experimental::administration;

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    SECTION("local user operations")
    {
        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            CHECK_NOTHROW(adm::client::remove_user(conn, test_user));
            CHECK_FALSE(adm::client::exists(conn, test_user));
        }};

        REQUIRE_NOTHROW(adm::client::add_user(conn, test_user));
        REQUIRE(adm::client::exists(conn, test_user));
        REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_password_property{"testpassword"}));
        REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_type_property{adm::user_type::rodsadmin}));

        // Show that the user does not have any authentication names.
        auto auth_names = adm::client::auth_names(conn, test_user);
        CHECK(auth_names.empty());

        // Associate an authentication name with the user.
        const auto* const auth_name = "/C=US/O=RENCI/OU=iRODS/CN=Kory Draughn/UID=kory";
        adm::user_authentication_property auth_property{adm::user_authentication_operation::add, auth_name};
        CHECK_NOTHROW(adm::client::modify_user(conn, test_user, auth_property));

        // Show that the user is now associated with an authentication name.
        auth_names = adm::client::auth_names(conn, test_user);
        const auto e = std::end(auth_names);
        CHECK(std::find(std::begin(auth_names), e, auth_name) != e);

        // Remove the authentication name from the user.
        auth_property.op = adm::user_authentication_operation::remove;
        CHECK_NOTHROW(adm::client::modify_user(conn, test_user, auth_property));

        // Show that the user does not have any authentication names.
        CHECK(adm::client::auth_names(conn, test_user).empty());
    }

    SECTION("user types")
    {
        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            CHECK_NOTHROW(adm::client::remove_user(conn, test_user));
            CHECK_FALSE(adm::client::exists(conn, test_user));
        }};

        REQUIRE_NOTHROW(adm::client::add_user(conn, test_user));
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::rodsuser);

        REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_type_property{adm::user_type::rodsadmin}));
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::rodsadmin);

        REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_type_property{adm::user_type::groupadmin}));
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::groupadmin);
    }

    SECTION("remote user operations")
    {
        const auto* const test_zone_name = "unit_test_zone";

        // Create a remote zone.
        REQUIRE_NOTHROW(adm::client::add_zone(conn, test_zone_name));

        irods::at_scope_exit remove_zone{[&conn, test_zone_name] {
            CHECK_NOTHROW(adm::client::remove_zone(conn, test_zone_name));
            CHECK_FALSE(adm::client::zone_exists(conn, test_zone_name));
        }};

        // Create a remote user.
        const adm::user test_user{"unit_test_test_user", test_zone_name};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            CHECK_NOTHROW(adm::client::remove_user(conn, test_user));
            CHECK_FALSE(adm::client::exists(conn, test_user));
        }};

        REQUIRE_NOTHROW(adm::client::add_user(conn, test_user, adm::user_type::rodsuser, adm::zone_type::remote));
        REQUIRE(adm::client::exists(conn, test_user));

        // Change the type of the user to groupadmin.
        CHECK_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_type_property{adm::user_type::groupadmin}));

        // Show that the user does not have any authentication names.
        auto auth_names = adm::client::auth_names(conn, test_user);
        CHECK(auth_names.empty());

        // Associate an authentication name with the user.
        const auto* const auth_name = "/C=US/O=RENCI/OU=iRODS/CN=Kory Draughn/UID=kory";
        adm::user_authentication_property auth_property{adm::user_authentication_operation::add, auth_name};
        CHECK_NOTHROW(adm::client::modify_user(conn, test_user, auth_property));

        // Show that the user is now associated with an authentication name.
        auth_names = adm::client::auth_names(conn, test_user);
        const auto e = std::end(auth_names);
        CHECK(std::find(std::begin(auth_names), e, auth_name) != e);

        // Remove the authentication name from the user.
        auth_property.op = adm::user_authentication_operation::remove;
        CHECK_NOTHROW(adm::client::modify_user(conn, test_user, auth_property));

        // Show that the user does not have any authentication names.
        CHECK(adm::client::auth_names(conn, test_user).empty());
    }

    SECTION("group operations")
    {
        adm::group test_group{"unit_test_test_group"};

        irods::at_scope_exit remove_group{[&conn, &test_group] {
            CHECK_NOTHROW(adm::client::remove_group(conn, test_group));
            CHECK_FALSE(adm::client::exists(conn, test_group));
        }};

        REQUIRE_NOTHROW(adm::client::add_group(conn, test_group));
        REQUIRE(adm::client::exists(conn, test_group));

        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_group, &test_user] {
            CHECK_NOTHROW(adm::client::remove_user_from_group(conn, test_group, test_user));
            CHECK_NOTHROW(adm::client::remove_user(conn, test_user));
            CHECK_FALSE(adm::client::exists(conn, test_user));
        }};

        REQUIRE_NOTHROW(adm::client::add_user(conn, test_user));
        REQUIRE_NOTHROW(adm::client::add_user_to_group(conn, test_group, test_user));
        REQUIRE(adm::client::user_is_member_of_group(conn, test_group, test_user));
    }

    SECTION("list all groups containing a specific user")
    {
        const std::vector groups{
            adm::group{"unit_test_test_group_a"},
            adm::group{"unit_test_test_group_b"},
            adm::group{"unit_test_test_group_c"}
        };

        irods::at_scope_exit remove_groups{[&conn, &groups] {
            CHECK_NOTHROW(adm::client::remove_group(conn, groups[0]));
            CHECK_NOTHROW(adm::client::remove_group(conn, groups[1]));
            CHECK_NOTHROW(adm::client::remove_group(conn, groups[2]));
        }};

        CHECK_NOTHROW(adm::client::add_group(conn, groups[0]));
        CHECK_NOTHROW(adm::client::add_group(conn, groups[1]));
        CHECK_NOTHROW(adm::client::add_group(conn, groups[2]));

        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &groups, &test_user] {
            CHECK_NOTHROW(adm::client::remove_user_from_group(conn, groups[0], test_user));
            CHECK_NOTHROW(adm::client::remove_user_from_group(conn, groups[2], test_user));
            CHECK_NOTHROW(adm::client::remove_user(conn, test_user));
        }};

        REQUIRE_NOTHROW(adm::client::add_user(conn, test_user));
        REQUIRE_NOTHROW(adm::client::add_user_to_group(conn, groups[0], test_user));
        REQUIRE_NOTHROW(adm::client::add_user_to_group(conn, groups[2], test_user));

        auto groups_containing_user = adm::client::groups(conn, test_user);
        std::sort(std::begin(groups_containing_user), std::end(groups_containing_user));

        REQUIRE(groups_containing_user == std::vector{
            adm::group{"public"},
            adm::group{"unit_test_test_group_a"},
            adm::group{"unit_test_test_group_c"}
        });
    }
}
