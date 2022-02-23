#include <catch2/catch.hpp>

#include "irods/getRodsEnv.h"
#include "irods/rodsPath.h"
#include "irods/miscUtil.h"

#include "irods/filesystem/path.hpp"

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

TEST_CASE("test getLastPathElement")
{
    using namespace std::string_literals;

    char last_path_element[MAX_NAME_LEN]{};

    SECTION("/path/to/.")
    {
        std::string path = "/path/to/.";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/path/to/..")
    {
        std::string path = "/path/to/..";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/path/to/foo")
    {
        std::string path = "/path/to/foo";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo"s);
    }

    SECTION("/path/to/foo/.")
    {
        std::string path = "/path/to/foo/.";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/path/to/foo/..")
    {
        std::string path = "/path/to/foo/..";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/path/to/foo/")
    {
        std::string path = "/path/to/foo/";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "/path/to/foo"s);
    }

    SECTION("/path/to/foo.")
    {
        std::string path = "/path/to/foo.";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo."s);
    }

    SECTION("/path/to/foo~")
    {
        std::string path = "/path/to/foo~";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo~"s);
    }

    SECTION("/path/to/foo^")
    {
        std::string path = "/path/to/foo^";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo^"s);
    }

    SECTION("/.")
    {
        std::string path = "/.";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/..")
    {
        std::string path = "/..";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/")
    {
        std::string path = "/";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("/foo")
    {
        std::string path = "/foo";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo"s);
    }

    SECTION("/foo/")
    {
        std::string path = "/foo/";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "/foo"s);
    }

    SECTION("foo/")
    {
        std::string path = "foo/";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == "foo"s);
    }

    SECTION(".")
    {
        std::string path = ".";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("..")
    {
        std::string path = "..";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }

    SECTION("")
    {
        std::string path = "";
        REQUIRE(getLastPathElement(path.data(), last_path_element) == 0);
        REQUIRE(last_path_element == ""s);
    }
}

TEST_CASE("has_prefix")
{
    REQUIRE(has_prefix("", "") == 0);
    REQUIRE(has_prefix("/", "") == 0);
    REQUIRE(has_prefix("", "/") == 0);
    REQUIRE(has_prefix("a/b/c", "a/b") == 1);
    REQUIRE(has_prefix("/a/b/c/d", "/") == 1);
    REQUIRE(has_prefix("/a/b/c/d", "////") == 1);
    REQUIRE(has_prefix("/a/b/c/d", "/a/b/c") == 1);
    REQUIRE(has_prefix("/a/b/c/d", "/a/b/c////") == 1);
}

