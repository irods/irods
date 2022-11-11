#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"

#include <unistd.h>

#include <string_view>
#include <tuple>

TEST_CASE("resource administration")
{
    namespace adm = irods::experimental::administration;

    adm::resource_registration_info repl_info;
    repl_info.resource_name = "unit_test_repl";
    repl_info.resource_type = adm::resource_type::replication;

    char hostname[HOST_NAME_MAX + 1]{};
    REQUIRE(gethostname(hostname, sizeof(hostname)) == 0);

    adm::resource_registration_info ufs_info;
    ufs_info.resource_name = "unit_test_ufs0";
    ufs_info.resource_type = adm::resource_type::unixfilesystem;
    ufs_info.host_name = hostname;
    ufs_info.vault_path = "/tmp";

    load_client_api_plugins();

    irods::experimental::client_connection conn;

    irods::at_scope_exit remove_resources{[&conn, &repl_info, &ufs_info] {
        CHECK_NOTHROW(adm::client::remove_resource(conn, repl_info.resource_name));
        CHECK_NOTHROW(adm::client::remove_resource(conn, ufs_info.resource_name));
    }};

    REQUIRE_NOTHROW(adm::client::add_resource(conn, repl_info));
    REQUIRE_NOTHROW(adm::client::add_resource(conn, ufs_info));

    SECTION("add / remove child resource")
    {
        {
            // Because the resource manager caches the resource state at agent startup,
            // we must spawn a new agent so that the resource manager sees the changes.
            irods::experimental::client_connection conn;
            REQUIRE_NOTHROW(adm::client::add_child_resource(conn, repl_info.resource_name, ufs_info.resource_name));

            // Show that the unixfilesystem resource has a parent.
            auto info = adm::client::resource_info(conn, ufs_info.resource_name);
            REQUIRE(info);
            REQUIRE_FALSE(info->parent_id().empty());

            // Convert the parent ID to a resource name.
            auto parent_resc_name = adm::client::resource_name(conn, info->parent_id());
            REQUIRE(*parent_resc_name == repl_info.resource_name);
        }

        {
            irods::experimental::client_connection conn;
            REQUIRE_NOTHROW(adm::client::remove_child_resource(conn, repl_info.resource_name, ufs_info.resource_name));

            // Show that the unixfilesystem resource no longer has a parent.
            auto info = adm::client::resource_info(conn, ufs_info.resource_name);
            REQUIRE(info);
            REQUIRE(info->parent_id().empty());
        }

        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->name() == ufs_info.resource_name);
        REQUIRE(info->type() == ufs_info.resource_type);
        REQUIRE(info->host_name() == ufs_info.host_name);
    }

    SECTION("modify resource type")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->type() == ufs_info.resource_type);

        const adm::resource_type_property property{adm::resource_type::replication};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->type() == adm::resource_type::replication);
    }

    SECTION("modify resource host name")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->host_name() == ufs_info.host_name);

        const adm::host_name_property property{"example.org"};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->host_name() == property.value);
    }

    SECTION("modify resource vault path")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->vault_path() == ufs_info.vault_path);

        const adm::vault_path_property property{"/tmp"};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->vault_path() == property.value);
    }

    SECTION("modify resource status")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->status() == adm::resource_status::unknown);

        const adm::resource_status_property property{adm::resource_status::up};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->status() == property.value);
    }

    SECTION("modify resource comments")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->comments().empty());

        const adm::resource_comments_property property{"This is a test comment."};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->comments() == property.value);
    }

    SECTION("modify resource information")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->information().empty());

        const adm::resource_info_property property{"This is a test info string."};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->information() == property.value);
    }

    SECTION("modify resource free space")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->free_space().empty());
        const auto mtime = info->free_space_last_modified();

        // Set the initial free space value.
        adm::free_space_property property{"1000"};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->free_space() == property.value);

        // Verify that the mtime of the resource has been updated as well.
        REQUIRE(info->free_space_last_modified() > mtime);

        // Show that the "+" and "-" operators work too.

        // Increment the value by 1000.
        property.value = "+1000";
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->free_space() == "2000");

        // Decrement the value by 500.
        property.value = "-500";
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->free_space() == "1500");
    }

    SECTION("modify resource context string")
    {
        auto info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->context_string().empty());

        const adm::context_string_property property{"This resource is for testing only!"};
        REQUIRE_NOTHROW(adm::client::modify_resource(conn, ufs_info.resource_name, property));
        info = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(info);
        REQUIRE(info->context_string() == property.value);
    }

    SECTION("rebalance resource")
    {
        irods::experimental::client_connection conn;
        REQUIRE_NOTHROW(adm::client::rebalance_resource(conn, ufs_info.resource_name));
    }

    SECTION("resource_info returns nullopt on non-existent resource")
    {
        auto info = adm::client::resource_info(conn, "does_not_exist");
        REQUIRE_FALSE(info);
    }
}

