#include <catch2/catch.hpp>

#include "connection_pool.hpp"
#include "user_administration.hpp"
#include "irods_at_scope_exit.hpp"

#include <algorithm>
#include <iterator>

TEST_CASE("user group administration")
{
    namespace adm = irods::experimental::administration;

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

    SECTION("local user operations")
    {
        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            if (adm::client::exists(conn, test_user)) {
                adm::client::remove_user(conn, test_user);
            }
        }};

        REQUIRE(adm::client::add_user(conn, test_user).value() == 0);
        REQUIRE(adm::client::exists(conn, test_user));
        REQUIRE(adm::client::set_user_password(conn, test_user, "testpassword").value() == 0);
        REQUIRE(adm::client::set_user_type(conn, test_user, adm::user_type::rodsadmin).value() == 0);

        REQUIRE(adm::client::remove_user(conn, test_user).value() == 0);
        REQUIRE_FALSE(adm::client::exists(conn, test_user));
    }

    SECTION("user types")
    {
        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            if (adm::client::exists(conn, test_user)) {
                adm::client::remove_user(conn, test_user);
            }
        }};

        REQUIRE(adm::client::add_user(conn, test_user).value() == 0);
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::rodsuser);

        REQUIRE(adm::client::set_user_type(conn, test_user, adm::user_type::rodsadmin).value() == 0);
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::rodsadmin);

        REQUIRE(adm::client::set_user_type(conn, test_user, adm::user_type::groupadmin).value() == 0);
        REQUIRE(adm::client::type(conn, test_user) == adm::user_type::groupadmin);

        REQUIRE(adm::client::remove_user(conn, test_user).value() == 0);
    }

#ifdef IRODS_TEST_REMOTE_USER_ADMINISTRATION_OPERATIONS
    SECTION("remote user operations")
    {
        adm::user test_user{"unit_test_test_user", "otherZone"};

        irods::at_scope_exit remove_user{[&conn, &test_user] {
            if (adm::client::exists(conn, test_user)) {
                adm::client::remove_user(conn, test_user);
            }
        }};

        REQUIRE(adm::client::add_user(conn, test_user, adm::user_type::rodsuser, adm::zone_type::remote).value() == 0);
        REQUIRE(adm::client::exists(conn, test_user));

        REQUIRE(adm::client::remove_user(conn, test_user).value() == 0);
        REQUIRE_FALSE(adm::client::exists(conn, test_user));
    }
#endif // IRODS_TEST_REMOTE_USER_ADMINISTRATION_OPERATIONS

    SECTION("group operations")
    {
        adm::group test_group{"unit_test_test_group"};

        irods::at_scope_exit remove_group{[&conn, &test_group] {
            if (adm::client::exists(conn, test_group)) {
                adm::client::remove_group(conn, test_group);
            }
        }};

        REQUIRE(adm::client::add_group(conn, test_group).value() == 0);
        REQUIRE(adm::client::exists(conn, test_group));

        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &test_group, &test_user] {
            if (adm::client::exists(conn, test_user)) {
                adm::client::remove_user_from_group(conn, test_group, test_user);
                adm::client::remove_user(conn, test_user);
            }
        }};

        REQUIRE(adm::client::add_user(conn, test_user).value() == 0);
        REQUIRE(adm::client::add_user_to_group(conn, test_group, test_user).value() == 0);
        REQUIRE(adm::client::user_is_member_of_group(conn, test_group, test_user));

        REQUIRE(adm::client::remove_user_from_group(conn, test_group, test_user).value() == 0);
        REQUIRE(adm::client::remove_user(conn, test_user).value() == 0);
        REQUIRE_FALSE(adm::client::user_is_member_of_group(conn, test_group, test_user));

        REQUIRE(adm::client::remove_group(conn, test_group).value() == 0);
        REQUIRE_FALSE(adm::client::exists(conn, test_group));
    }

    SECTION("list all groups containing a specific user")
    {
        const std::vector groups{
            adm::group{"unit_test_test_group_a"},
            adm::group{"unit_test_test_group_b"},
            adm::group{"unit_test_test_group_c"}
        };

        irods::at_scope_exit remove_groups{[&conn, &groups] {
            for (auto&& g : groups) {
                if (adm::client::exists(conn, g)) {
                    adm::client::remove_group(conn, g);
                }
            }
        }};

        for (auto&& g : groups) {
            adm::client::add_group(conn, g);
        }

        adm::user test_user{"unit_test_test_user"};

        irods::at_scope_exit remove_user{[&conn, &groups, &test_user] {
            if (adm::client::exists(conn, test_user)) {
                for (auto&& g : groups) {
                    adm::client::remove_user_from_group(conn, g, test_user);
                }

                adm::client::remove_user(conn, test_user);
            }
        }};

        REQUIRE(adm::client::add_user(conn, test_user).value() == 0);
        REQUIRE(adm::client::add_user_to_group(conn, groups[0], test_user).value() == 0);
        REQUIRE(adm::client::add_user_to_group(conn, groups[2], test_user).value() == 0);

        auto groups_containing_user = adm::client::groups(conn, test_user);
        std::sort(std::begin(groups_containing_user), std::end(groups_containing_user));

        REQUIRE(groups_containing_user == std::vector{
            adm::group{"public"},
            adm::group{"unit_test_test_group_a"},
            adm::group{"unit_test_test_group_c"}
        });
    }
}

