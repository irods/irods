#include <catch2/catch.hpp>

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_server_properties.hpp"

#include <fmt/format.h>

#include <thread>

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

// The goal here is to ensure that the server_properties object is locked and unlocked
// safely while using the normal api
TEST_CASE("server_properties is threadsafe", "[thread-safety]")
{
    const auto limit = 1000;

    // Start a thread that will generally get in the way a lot by writing
    // to the irods::server_properties object
    std::thread a([]() {
        for (auto i = 0; i < limit; i++) {
            irods::set_server_property(std::to_string(i), i);
        }
    });
    irods::at_scope_exit([&a]() { a.join(); });
    auto i = 0;
    while (i < limit) {
        auto key = std::to_string(i);
        if (irods::server_property_exists(key)) {
            REQUIRE(irods::get_server_property<int>(key) == i);
            i++;
        }
    }
}
