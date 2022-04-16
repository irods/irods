#include <catch2/catch.hpp>

#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_server_properties.hpp"

#include <fmt/format.h>

TEST_CASE("server_properties", "[exceptions]")
{
    SECTION("exceptions include key string")
    {
        CHECK_THROWS(irods::get_server_property<std::string>(irods::KW_CFG_PLUGIN_TYPE_DATABASE),
                     fmt::format("key does not exist [{}].", irods::KW_CFG_PLUGIN_TYPE_DATABASE));
    }

    SECTION("exceptions include key path as string")
    {
        const irods::configuration_parser::key_path_t key_path{
            irods::KW_CFG_PLUGIN_CONFIGURATION,
            irods::KW_CFG_PLUGIN_TYPE_DATABASE,
            irods::KW_CFG_DB_PASSWORD
        };

        CHECK_THROWS(irods::get_server_property<std::string>(key_path),
                     fmt::format("path does not exist [{}.{}.{}].",
                                 irods::KW_CFG_PLUGIN_CONFIGURATION,
                                 irods::KW_CFG_PLUGIN_TYPE_DATABASE,
                                 irods::KW_CFG_DB_PASSWORD));
    }
}
