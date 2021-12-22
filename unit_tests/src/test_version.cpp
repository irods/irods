#include <catch2/catch.hpp>

#include "version.hpp"

TEST_CASE("version equivalence operators")
{
    constexpr irods::version v4290{4, 2, 9};

    REQUIRE(v4290 == v4290);
    REQUIRE(v4290 <= v4290);
    REQUIRE(v4290 >= v4290);

    constexpr irods::version v4300{4, 3};

    REQUIRE(v4290 != v4300);
    REQUIRE(v4290 <  v4300);
    REQUIRE(v4290 <= v4300);
    REQUIRE(v4300 >  v4290);
    REQUIRE(v4300 >= v4290);
}

