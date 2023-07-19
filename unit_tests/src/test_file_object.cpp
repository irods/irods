#include <catch2/catch.hpp>

#include "irods/irods_file_object.hpp"

TEST_CASE("file_object")
{
    SECTION("test file object size can be negative issue 7154")
    {
        irods::file_object obj{};
        REQUIRE(obj.size() == 0);

        obj.size(-1);
        REQUIRE(obj.size() < 0);
    }
}

