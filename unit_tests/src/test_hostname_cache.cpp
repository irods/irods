#include <catch2/catch.hpp>

#include "irods/hostname_cache.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <string_view>
#include <chrono>
#include <thread>

namespace hnc = irods::experimental::net::hostname_cache;

using namespace std::chrono_literals;

TEST_CASE("hostname_cache")
{
    hnc::init("irods_hostname_cache_test", 100'000);
    irods::at_scope_exit cleanup{[] { hnc::deinit(); }};

    SECTION("insert / update / expiration")
    {
        REQUIRE(hnc::insert_or_assign("foo", "foo.irods.org", 3s));
        auto alias = hnc::lookup("foo");
        REQUIRE(alias);
        REQUIRE(alias.value() == "foo.irods.org");

        REQUIRE_FALSE(hnc::insert_or_assign("foo", "foobar.irods.org", 3s));
        alias = hnc::lookup("foo");
        REQUIRE(alias);
        REQUIRE(alias.value() == "foobar.irods.org");

        std::this_thread::sleep_for(3s);
        REQUIRE_FALSE(hnc::lookup("foo"));
    }

    SECTION("erasure operations")
    {
        const auto initial_available_memory = hnc::available_memory();

        REQUIRE(hnc::insert_or_assign("foo", "foo.irods.org", 3s));
        REQUIRE(hnc::insert_or_assign("bar", "bar.irods.org", 3s));
        REQUIRE(hnc::insert_or_assign("baz", "baz.irods.org", 3s));
        REQUIRE(hnc::insert_or_assign("jar", "jar.irods.org", 3s));

        REQUIRE(hnc::size() == 4);
        REQUIRE(hnc::available_memory() < initial_available_memory);

        // Erase the "baz" entry.
        const char* key = "baz";
        REQUIRE(hnc::lookup(key));
        hnc::erase(key);
        REQUIRE_FALSE(hnc::lookup(key));

        REQUIRE(hnc::size() == 3);

        // Show that the other entries still exist.
        REQUIRE(hnc::lookup("foo"));
        REQUIRE(hnc::lookup("bar"));
        REQUIRE(hnc::lookup("jar"));

        // Show that all expired entries were erased from the cache.
        std::this_thread::sleep_for(3s);
        hnc::erase_expired_entries();
        REQUIRE_FALSE(hnc::lookup("foo"));
        REQUIRE_FALSE(hnc::lookup("bar"));
        REQUIRE_FALSE(hnc::lookup("jar"));

        REQUIRE(hnc::size() == 0);
        REQUIRE(hnc::available_memory() == initial_available_memory);
    }
}

