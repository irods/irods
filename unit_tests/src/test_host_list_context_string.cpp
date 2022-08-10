#include <catch2/catch.hpp>

#include <string_view>
#include <sstream>
#include <vector>

#include "irods/irods_plugin_base.hpp"
#include "irods/filesystem/path.hpp"
#include "irods/private/irods_is_in_host_list.hpp"

TEST_CASE("host list tests", "[detached_mode_host_list]")
{
    const std::string_view resource_hostname = "myhost";
    irods::plugin_property_map prop_map;

    SECTION("not in host list")
    {
        prop_map.set<std::string>("host_list", "host2,host3");
        REQUIRE_FALSE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("not in host list but hostname with common prefix")
    {
        prop_map.set<std::string>("host_list", "myhost1,myhost2,host2,host3,myhost3");
        REQUIRE_FALSE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("at beginning of host list")
    {
        prop_map.set<std::string>("host_list", "myhost,host2,host3");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("at end of host list no path")
    {
        prop_map.set<std::string>("host_list", "host2,host3,myhost");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("in middle of host list no path")
    {
        prop_map.set<std::string>("host_list", "host2,myhost,host3");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("at beginning of host list not case match")
    {
        prop_map.set<std::string>("host_list", "MYHOST,host2,host3");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("at end of host list not case match")
    {
        prop_map.set<std::string>("host_list", "host2,host3,MYHOST");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("in middle of host list not case match")
    {
        prop_map.set<std::string>("host_list", "host2,MYHOST,host3");
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("empty host list")
    {
        prop_map.set<std::string>("host_list", "");
        REQUIRE_FALSE(is_host_in_host_list(prop_map, resource_hostname));
    }

    SECTION("no host list - all hosts in list")
    {
        REQUIRE(is_host_in_host_list(prop_map, resource_hostname));
    }
}
