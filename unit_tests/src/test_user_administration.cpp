#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rcConnect.h"
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

    // Create a shared, generic test user
    adm::user generic_user{"lagging-train"};
    REQUIRE_NOTHROW(adm::client::add_user(conn, generic_user));
    irods::at_scope_exit generic_user_cleanup{
        [&conn, &generic_user] { CHECK_NOTHROW(adm::client::remove_user(conn, generic_user)); }};

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

    SECTION("#7208: password can be changed without obfGetPw")
    {
        // Create a new rodsadmin.
        const adm::user test_admin{"unit_test_test_admin"};
        REQUIRE_NOTHROW(adm::client::add_user(conn, test_admin, adm::user_type::rodsadmin));
        irods::at_scope_exit remove_admin{
            [&conn, &test_admin] { CHECK_NOTHROW(adm::client::remove_user(conn, test_admin)); }};

        // Set the test admin's password. This will be used later.
        adm::user_password_property admin_prop{"admin_pass"};
        REQUIRE_NOTHROW(adm::client::modify_user(conn, test_admin, admin_prop));

        // Connect to the server and authenticate as the new admin.
        rodsEnv env;
        _getRodsEnv(env);
        irods::experimental::client_connection admin_conn{
            irods::experimental::defer_authentication, env.rodsHost, env.rodsPort, {test_admin.name, env.rodsZone}};
        CHECK(clientLoginWithPassword(static_cast<RcComm*>(admin_conn), admin_prop.value.data()) == 0);

        // Create a rodsuser.
        const adm::user test_user{"unit_test_test_user"};
        REQUIRE_NOTHROW(adm::client::add_user(admin_conn, test_user));
        irods::at_scope_exit remove_user{
            [&admin_conn, &test_user] { CHECK_NOTHROW(adm::client::remove_user(admin_conn, test_user)); }};

        // Show we can set the password of the test user without needing to call obfGetPw.
        // Notice the second argument on the property. This instructs the user administration library
        // to not call obfGetPw and instead, use the given argument as the descrambled password. The
        // password passed as the second argument must be the requester's password.
        adm::user_password_property test_user_prop{"user_pass", admin_prop.value};
        REQUIRE_NOTHROW(adm::client::modify_user(admin_conn, test_user, test_user_prop));

        // Show the test user can connect to the server and authenticate using the new password.
        irods::experimental::client_connection user_conn{
            irods::experimental::defer_authentication, env.rodsHost, env.rodsPort, {test_user.name, env.rodsZone}};
        CHECK(clientLoginWithPassword(static_cast<RcComm*>(user_conn), test_user_prop.value.data()) == 0);
    }

    SECTION("#7576: ensure no limitations on proxied groupadmin")
    {
        rodsEnv env{};
        _getRodsEnv(env);

        REQUIRE_NOTHROW(
            adm::client::modify_user(conn, generic_user, adm::user_type_property{adm::user_type::groupadmin}));
        REQUIRE(adm::client::type(conn, generic_user) == adm::user_type::groupadmin);

        // Connect to iRODS
        irods::experimental::client_connection user_conn{env.rodsHost,
                                                         env.rodsPort,
                                                         {env.rodsUserName, env.rodsZone}, // (proxy) rodsadmin user
                                                         {generic_user.name, env.rodsZone}}; // groupadmin user

        // Try to add a group as the proxied groupadmin user.
        adm::group test_group{"bad_bad_water_group"};
        REQUIRE_NOTHROW(adm::client::add_group(user_conn, test_group));
        REQUIRE(adm::client::exists(user_conn, test_group));
        irods::at_scope_exit group_cleanup{
            [&conn, &test_group] { CHECK_NOTHROW(adm::client::remove_group(conn, test_group)); }};

        // Add self to group
        REQUIRE_NOTHROW(adm::client::add_user_to_group(user_conn, test_group, generic_user));
        REQUIRE(adm::client::user_is_member_of_group(user_conn, test_group, generic_user));
        irods::at_scope_exit groupadmin_self_add_cleanup{[&user_conn, &test_group, &generic_user] {
            CHECK_NOTHROW(adm::client::remove_user_from_group(user_conn, test_group, generic_user));
        }};

        // Create and remove user using groupadmin
        adm::user groupadmin_created_user{"mr_flips"};
        REQUIRE_NOTHROW(adm::client::add_user(user_conn, groupadmin_created_user));
        REQUIRE(adm::client::exists(user_conn, groupadmin_created_user));
        irods::at_scope_exit created_user_cleanup{[&conn, &groupadmin_created_user] {
            CHECK_NOTHROW(adm::client::remove_user(conn, groupadmin_created_user));
        }};

        // Add and remove user from group using groupadmin
        REQUIRE_NOTHROW(adm::client::add_user_to_group(user_conn, test_group, groupadmin_created_user));
        REQUIRE(adm::client::user_is_member_of_group(user_conn, test_group, groupadmin_created_user));

        // Begin cleanup. Previous 'irods::at_scope_exit's will run after the following line
        CHECK_NOTHROW(adm::client::remove_user_from_group(user_conn, test_group, groupadmin_created_user));
    }
}
