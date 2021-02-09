#include "catch.hpp"

#include "dns_cache.hpp"
#include "irods_at_scope_exit.hpp"

#include <string_view>
#include <chrono>
#include <thread>
#include <regex>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

namespace dnsc = irods::experimental::net::dns_cache;

using namespace std::chrono_literals;

using gai_deleter = void (*)(addrinfo*);

auto resolve_canonical_hostname(const std::string_view _node) -> std::unique_ptr<addrinfo, gai_deleter>;

auto resolve_ip(const std::string_view _node) -> std::unique_ptr<addrinfo, gai_deleter>;

TEST_CASE("dns_cache")
{
    dnsc::init("irods_dns_cache_test", 100'000);
    irods::at_scope_exit cleanup{[] { dnsc::deinit(); }};

    SECTION("basic functionality")
    {
        const auto initial_available_memory = dnsc::available_memory();

        const char* google = "google.com";
        {
            auto gai_info = resolve_canonical_hostname(google);
            REQUIRE(gai_info);
            REQUIRE(dnsc::insert_or_assign(google, *gai_info, 3s));

            auto dnsc_entry = dnsc::lookup(google);
            REQUIRE(dnsc_entry);
        }

        const char* yahoo = "yahoo.com";
        {
            auto gai_info = resolve_ip(yahoo);
            REQUIRE(gai_info);
            REQUIRE(dnsc::insert_or_assign(yahoo, *gai_info, 3s));

            auto dnsc_entry = dnsc::lookup(yahoo);
            REQUIRE(dnsc_entry);

            const auto* src = &reinterpret_cast<sockaddr_in*>(dnsc_entry->ai_addr)->sin_addr;
            char dst[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, src, dst, sizeof(dst));

            const std::regex pattern{R"_(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})_"};
            REQUIRE(std::regex_match(dst, pattern));
        }

        const char* github = "github.com";
        {
            auto gai_info = resolve_canonical_hostname(github);
            REQUIRE(gai_info);
            REQUIRE(dnsc::insert_or_assign(github, *gai_info, 3s));

            auto dnsc_entry = dnsc::lookup(github);
            REQUIRE(dnsc_entry);
        }

        REQUIRE(dnsc::size() == 3);

        std::this_thread::sleep_for(4s);
        REQUIRE_FALSE(dnsc::lookup(google));
        REQUIRE_FALSE(dnsc::lookup(yahoo));
        REQUIRE_FALSE(dnsc::lookup(github));

        REQUIRE(dnsc::available_memory() < initial_available_memory);

        SECTION("test dnsc_erase")
        {
            dnsc::erase(google);
            dnsc::erase(yahoo);
            dnsc::erase(github);
        }

        SECTION("test dnsc_erase_expired_entries")
        {
            dnsc::erase_expired_entries();
        }

        SECTION("test dnsc_clear")
        {
            dnsc::clear();
        }

        REQUIRE(dnsc::size() == 0);
        REQUIRE(dnsc::available_memory() == initial_available_memory);
    }
}

auto resolve_canonical_hostname(const std::string_view _node) -> std::unique_ptr<addrinfo, gai_deleter>
{
    addrinfo hints{};
    hints.ai_flags = AI_CANONNAME;
    addrinfo* res{};
    const auto ec = getaddrinfo(_node.data(), nullptr, &hints, &res);
    REQUIRE(ec == 0);
    return {res, freeaddrinfo};
}

auto resolve_ip(const std::string_view _node) -> std::unique_ptr<addrinfo, gai_deleter>
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* res{};
    const auto ec = getaddrinfo(_node.data(), nullptr, &hints, &res);
    REQUIRE(ec == 0);
    return {res, freeaddrinfo};
}

