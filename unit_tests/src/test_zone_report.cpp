#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/server_report.h"

#include <unistd.h>

#include <tuple>

#include <nlohmann/json.hpp>

TEST_CASE("json zone report")
{
    namespace adm = irods::experimental::administration;

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

    irods::at_scope_exit remove_resources{[&conn, &ufs_info] {
        adm::client::remove_resource(conn, ufs_info.resource_name);
    }};

    REQUIRE(adm::client::add_resource(conn, ufs_info).value() == 0);

    // see issue #5170
    SECTION("verify resource id from json serialization")
    {
        // we need a fresh connection because the cache is probably out of date
        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();

        auto [ec, info] = adm::client::resource_info(conn, ufs_info.resource_name);
        REQUIRE(ec.value() == 0);
        REQUIRE(info->type() == ufs_info.resource_type);

        bytesBuf_t *jbuf{};
        REQUIRE(rcServerReport(static_cast<rcComm_t*>(conn), &jbuf) == 0);
        std::string report_str(static_cast<char*>(jbuf->buf), jbuf->len);

        const nlohmann::json report = nlohmann::json::parse(report_str);
        REQUIRE(report["resources"].size() >= 1);
        std::string id_str;
        for (const nlohmann::json &resource : report["resources"]) {
            const std::string name = resource["name"].get<std::string>();
            if (name == ufs_info.resource_name) {
                id_str = resource["id"].get<std::string>();
            }
        }
        REQUIRE(info->id() == id_str);
    }
}
