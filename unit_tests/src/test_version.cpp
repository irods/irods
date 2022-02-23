#include <catch2/catch.hpp>

#include "irods/version.hpp"

static constexpr irods::version v4290{4, 2, 9};
static constexpr irods::version v4300{4, 3};

TEST_CASE("version equivalence operators")
{
    REQUIRE(v4290 == v4290);
    REQUIRE(v4290 <= v4290);
    REQUIRE(v4290 >= v4290);

    REQUIRE(v4290 != v4300);
    REQUIRE(v4290 <  v4300);
    REQUIRE(v4290 <= v4300);
    REQUIRE(v4300 >  v4290);
    REQUIRE(v4300 >= v4290);
}

TEST_CASE("test to_version")
{
    // The string must be in the form "^rodsX.Y.Z$"
    CHECK(*irods::to_version("rods4.2.9") == v4290);
    CHECK(*irods::to_version("rods4.3.0") == v4300);

    // If it's not, std::nullopt is returned
    CHECK(!irods::to_version("irods4.2.9"));
    CHECK(!irods::to_version("rods4.2.9i"));
    CHECK(!irods::to_version("rodsrods4.2.9"));
    CHECK(!irods::to_version("rods4.2.9.0"));
    CHECK(!irods::to_version("rods429"));
    CHECK(!irods::to_version("rods4\\.2\\.9"));
    CHECK(!irods::to_version("rods4.3"));
    CHECK(!irods::to_version("4.2.9"));
    CHECK(!irods::to_version("rods4.two.9"));
    CHECK(!irods::to_version("rods4.-2.9"));

    // to_version uses std::stoi, so overflow will throw std::out_of_range
    CHECK_THROWS(irods::to_version("rods400000000000000.200000000000000.900000000000000"));
}
