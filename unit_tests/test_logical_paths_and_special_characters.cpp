#include "catch.hpp"

#include "getRodsEnv.h"
#include "rodsPath.h"

#include "filesystem/path.hpp"

#include <cstring>
#include <string>

TEST_CASE("logical paths and special characters")
{
    using namespace std::string_literals;

    namespace fs = irods::experimental::filesystem;

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) == 0);

    rodsPath_t input{};
    
    SECTION("expand tilde")
    {
        const std::string in_path = "~";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(fs::path{env.rodsHome}.string() == input.outPath);
    }

    SECTION("hat should not expand to home collection")
    {
        const std::string in_path = "^";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(fs::path{env.rodsHome} / in_path == input.outPath);
    }

    SECTION("keep leading tilde")
    {
        const std::string in_path = "/zone/~data_object";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }

    SECTION("keep leading period")
    {
        const std::string in_path = "/zone/.data_object";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }

    SECTION("keep leading hat")
    {
        const std::string in_path = "/zone/^data_object";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }

    SECTION("keep trailing tilde")
    {
        const std::string in_path = "/zone/data_object~";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }

    SECTION("keep trailing period")
    {
        const std::string in_path = "/zone/data_object.";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }

    SECTION("keep trailing hat")
    {
        const std::string in_path = "/zone/data_object^";
        std::strncpy(input.inPath, in_path.data(), in_path.size());

        REQUIRE(parseRodsPath(&input, &env) == 0);
        REQUIRE(in_path == input.outPath);
    }
}

