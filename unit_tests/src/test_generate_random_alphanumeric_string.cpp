#include <catch2/catch.hpp>

#include "irods/irods_exception.hpp"
#include "irods/irods_random.hpp"

#include <algorithm>
#include <cctype>
#include <string>

TEST_CASE("negative length throws")
{
    CHECK_THROWS_AS(irods::generate_random_alphanumeric_string(-1), irods::exception);
}

TEST_CASE("length of 0 results in an empty string")
{
    CHECK(irods::generate_random_alphanumeric_string(0).empty());
}

TEST_CASE("length of 1 effectively calls generate_random_alphanumeric_character")
{
    constexpr std::int16_t character_count = 1;
    const auto s = irods::generate_random_alphanumeric_string(character_count);
    REQUIRE(!s.empty());
    CHECK(s.size() == character_count);
    CHECK(std::all_of(
        std::begin(s), std::end(s), [](const auto& c) { return std::isalnum(static_cast<unsigned char>(c)); }));
}

TEST_CASE("only alphanumeric characters appear in longest allowable string")
{
    constexpr std::int16_t character_count = INT16_MAX;
    const auto s = irods::generate_random_alphanumeric_string(character_count);
    REQUIRE(!s.empty());
    CHECK(s.size() == character_count);
    CHECK(std::all_of(
        std::begin(s), std::end(s), [](const auto& c) { return std::isalnum(static_cast<unsigned char>(c)); }));
}
