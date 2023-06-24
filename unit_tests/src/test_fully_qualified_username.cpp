#include <catch2/catch.hpp>

#include "irods/fully_qualified_username.hpp"
#include "irods/rodsErrorTable.h"

TEST_CASE("fully_qualified_username")
{
    irods::experimental::fully_qualified_username fqun{"rods", "tempZone"};

    CHECK(fqun.name() == "rods");
    CHECK(fqun.zone() == "tempZone");
    CHECK(fqun.full_name() == "rods#tempZone");
}

TEST_CASE("fully_qualified_username throws exception on empty name")
{
    try {
        irods::experimental::fully_qualified_username{"", "tempZone"};
    }
    catch (const irods::exception& e) {
        CHECK(e.code() == SYS_INVALID_INPUT_PARAM);
        CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Empty name not allowed."));
    }
}

TEST_CASE("fully_qualified_username throws exception on empty zone")
{
    try {
        irods::experimental::fully_qualified_username{"rods", ""};
    }
    catch (const irods::exception& e) {
        CHECK(e.code() == SYS_INVALID_INPUT_PARAM);
        CHECK_THAT(e.client_display_what(), Catch::Matchers::Contains("Empty zone not allowed."));
    }
}
