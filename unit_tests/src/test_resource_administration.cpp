#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
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
    repl_info.resource_type = adm::resource_type::replication.data();

    char host_name[64] {};
    REQUIRE(gethostname(host_name, sizeof(host_name)) == 0);

    adm::resource_registration_info ufs_info;
    ufs_info.resource_name = "unit_test_ufs0";
    ufs_info.resource_type = adm::resource_type::unixfilesystem.data();
    ufs_info.host_name = host_name;
    ufs_info.vault_path = "/tmp";

    load_client_api_plugins();

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

    irods::at_scope_exit remove_resources{[&conn, &repl_info, &ufs_info] {
        adm::client::remove_resource(conn, repl_info.resource_name);
        adm::client::remove_resource(conn, ufs_info.resource_name);
    }};

    REQUIRE(adm::client::add_resource(conn, repl_info).value() == 0);
    REQUIRE(adm::client::add_resource(conn, ufs_info).value() == 0);

    SECTION("add / remove child resource")
    {
        {
            // Because the resource manager caches the resource state at agent startup,
            // We must spawn a new agent so that the resource manager sees the changes.
            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();
            REQUIRE(adm::client::add_child_resource(conn, repl_info.resource_name, ufs_info.resource_name).value() == 0);

            // Show that the unixfilesystem resource has a parent.
            auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
            REQUIRE(ec.value() == 0);
            REQUIRE_FALSE(info->parent_id().empty());
            
            {
                // Convert the parent ID to a resource name.
                const auto [ec, parent_resc_name] = adm::client::resource_name(conn, info->parent_id());
                REQUIRE(ec.value() == 0);
                REQUIRE(parent_resc_name == repl_info.resource_name);
            }
        }

        {
            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();
            REQUIRE(adm::client::remove_child_resource(conn, repl_info.resource_name, ufs_info.resource_name).value() == 0);

            // Show that the unixfilesystem resource no longer has a parent.
            auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
            REQUIRE(ec.value() == 0);
            REQUIRE(info->parent_id().empty());
        }

        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->name() == ufs_info.resource_name);
        REQUIRE(info->type() == ufs_info.resource_type);
        REQUIRE(info->host_name() == ufs_info.host_name);
    }

    SECTION("modify resource type")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->type() == ufs_info.resource_type);

        REQUIRE(adm::client::set_resource_type(conn, ufs_info.resource_name, adm::resource_type::replication).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->type() == adm::resource_type::replication);
    }

    SECTION("modify resource host name")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->host_name() == ufs_info.host_name);

        const std::string_view new_host_name = "irods.org";
        REQUIRE(adm::client::set_resource_host_name(conn, ufs_info.resource_name, new_host_name).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->host_name() == new_host_name);
    }

    SECTION("modify resource vault path")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->vault_path() == ufs_info.vault_path);

        const std::string_view new_vault_path = "/tmp";
        REQUIRE(adm::client::set_resource_vault_path(conn, ufs_info.resource_name, new_vault_path).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->vault_path() == new_vault_path);
    }

    SECTION("modify resource status")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->status() == adm::resource_status::unknown);

        const auto new_status = adm::resource_status::up;
        REQUIRE(adm::client::set_resource_status(conn, ufs_info.resource_name, new_status).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->status() == new_status);
    }

    SECTION("modify resource comments")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->comments().empty());

        const std::string_view new_comments = "This is a test comment.";
        REQUIRE(adm::client::set_resource_comments(conn, ufs_info.resource_name, new_comments).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->comments() == new_comments);
    }

    SECTION("modify resource information")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->information().empty());

        const std::string_view new_info = "This is a test info string.";
        REQUIRE(adm::client::set_resource_info_string(conn, ufs_info.resource_name, new_info).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->information() == new_info);
    }

    SECTION("modify resource free space")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->free_space().empty());
        const auto mtime = info->free_space_last_modified();

        // Set the initial free space value.
        std::string_view new_free_space = "1000";
        REQUIRE(adm::client::set_resource_free_space(conn, ufs_info.resource_name, new_free_space).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->free_space() == new_free_space);

        // Verify that the mtime of the resource has been updated as well.
        REQUIRE(info->free_space_last_modified() > mtime);

        // Show that the "+" and "-" operators work too.

        // Increment the value by 1000.
        new_free_space = "+1000";
        REQUIRE(adm::client::set_resource_free_space(conn, ufs_info.resource_name, new_free_space).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->free_space() == "2000");

        // Decrement the value by 500.
        new_free_space = "-500";
        REQUIRE(adm::client::set_resource_free_space(conn, ufs_info.resource_name, new_free_space).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->free_space() == "1500");
    }

    SECTION("modify resource context string")
    {
        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->context_string().empty());

        const std::string_view new_context = "This resource is for testing only!";
        REQUIRE(adm::client::set_resource_context_string(conn, ufs_info.resource_name, new_context).value() == 0);
        std::tie(ec, info) = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->context_string() == new_context);
    }

    SECTION("rebalance resource")
    {
        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();
        REQUIRE(adm::client::rebalance_resource(conn, ufs_info.resource_name).value() == 0);
    }
}

