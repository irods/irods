#include "catch.hpp"

#include "irods_configuration_keywords.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_server_properties.hpp"

#include "fmt/format.h"

TEST_CASE("server_properties", "[exceptions]")
{
    SECTION("exceptions include key string")
    {
        CHECK_THROWS(irods::get_server_property<std::string>(irods::PLUGIN_TYPE_DATABASE),
                     fmt::format("key does not exist [{}].", irods::PLUGIN_TYPE_DATABASE));
    }

    SECTION("exceptions include key path as string")
    {
        const irods::configuration_parser::key_path_t key_path{
            irods::CFG_PLUGIN_CONFIGURATION_KW,
            irods::PLUGIN_TYPE_DATABASE,
            irods::CFG_DB_PASSWORD_KW
        };

        CHECK_THROWS(irods::get_server_property<std::string>(key_path),
                     fmt::format("path does not exist [{}.{}.{}].",
                                 irods::CFG_PLUGIN_CONFIGURATION_KW,
                                 irods::PLUGIN_TYPE_DATABASE,
                                 irods::CFG_DB_PASSWORD_KW));
    }
}
