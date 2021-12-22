#include <catch2/catch.hpp>

#include "irods_error_enum_matcher.hpp"
#include "irods_exception.hpp"
#include "irods_hierarchy_parser.hpp"

using parser = irods::hierarchy_parser;

TEST_CASE("test_hierarchy_parser_delimiter", "[delim]") {
    REQUIRE(";" == parser::delimiter());
}

TEST_CASE("test_hierarchy_parser", "[hierarchy]") {
    const std::string str = "a;b;c;d;e";
    parser p{str};

    REQUIRE(5 == p.num_levels());
    REQUIRE("a" == p.first_resc());
    REQUIRE("e" == p.last_resc());

    SECTION("str") {
        REQUIRE(str == p.str());
        REQUIRE("a" == p.str("a"));
        REQUIRE("a;b;c" == p.str("c"));
        REQUIRE(str == p.str("e"));
        REQUIRE(str == p.str("f"));
    }
    SECTION("contains") {
        REQUIRE(p.contains("a"));
        REQUIRE(p.contains("e"));
        REQUIRE(!p.contains("b;c;d"));
        REQUIRE(!p.contains("f"));
    }
    SECTION("next") {
        REQUIRE("b" == p.next("a"));
        REQUIRE("e" == p.next("d"));
        REQUIRE_THROWS_WITH(p.next("e"),
                            Catch::Contains(std::to_string(NO_NEXT_RESC_FOUND)));
        REQUIRE_THROWS_WITH(p.next("f"),
                            Catch::Contains(std::to_string(CHILD_NOT_FOUND)));
    }
    SECTION("add_child") {
        p.add_child("f");
        REQUIRE("f" == p.next("e"));
        REQUIRE_THROWS_WITH(p.next("f"),
                            Catch::Contains(std::to_string(NO_NEXT_RESC_FOUND)));
        REQUIRE_THAT(p.add_child(irods::hierarchy_parser::delimiter()).code(),
                     equals_irods_error(SYS_INVALID_INPUT_PARAM));
    }
    SECTION("add_parent") {
        p.add_parent("z");
        REQUIRE("a" == p.next("z"));
        p.add_parent("c2", "c");
        REQUIRE("c" == p.next("c2"));
        REQUIRE_THROWS_WITH(p.add_parent("nope", "nope"),
                            Catch::Contains(std::to_string(CHILD_NOT_FOUND)));
        REQUIRE_THROWS_WITH(p.add_parent(irods::hierarchy_parser::delimiter()),
                            Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
    }
    SECTION("remove_resource") {
        // invalid inputs
        CHECK_THROWS_WITH(irods::hierarchy_parser{"a"}.remove_resource("a"),
                          Catch::Contains(std::to_string(SYS_NOT_ALLOWED)));
        CHECK_THROWS_WITH(p.remove_resource(""),
                          Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
        CHECK_THROWS_WITH(p.remove_resource(irods::hierarchy_parser::delimiter()),
                          Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
        CHECK_THROWS_WITH(p.remove_resource("z"),
                          Catch::Contains(std::to_string(CHILD_NOT_FOUND)));

        // beginning resource
        REQUIRE_NOTHROW(p.remove_resource("a"));
        REQUIRE(!p.contains("a"));

        // middle resource
        REQUIRE_NOTHROW(p.remove_resource("c"));
        REQUIRE(!p.contains("c"));
        REQUIRE("d" == p.next("b"));

        // end resource
        REQUIRE_NOTHROW(p.remove_resource("e"));
        REQUIRE(!p.contains("e"));
        REQUIRE_THROWS_WITH(p.next("d"),
                            Catch::Contains(std::to_string(NO_NEXT_RESC_FOUND)));
    }
}

TEST_CASE("test_hierarchy_parser_standalone_resource", "[standalone]") {
    const std::string str = "a";
    parser p{str};

    REQUIRE(1 == p.num_levels());
    REQUIRE(str == p.first_resc());
    REQUIRE(str == p.last_resc());

    SECTION("str") {
        REQUIRE(str == p.str());
        REQUIRE(str == p.str("a"));
        REQUIRE(str == p.str("c"));
    }
    SECTION("contains") {
        REQUIRE(p.contains("a"));
        REQUIRE(!p.contains("b"));
    }
    SECTION("next") {
        REQUIRE_THROWS_WITH(p.next("a"),
                            Catch::Contains(std::to_string(NO_NEXT_RESC_FOUND)));
        REQUIRE_THROWS_WITH(p.next("f"),
                            Catch::Contains(std::to_string(CHILD_NOT_FOUND)));
    }
}

TEST_CASE("test_hierarchy_parser_delimiter_nonsense", "[hierarchy][delim][pathological]") {
    SECTION("trailing delimiter") {
        const std::string str = "a;b;c;d;e;";
        const std::string corrected_str = "a;b;c;d;e";
        parser p{str};

        REQUIRE(5 == p.num_levels());
        REQUIRE("a" == p.first_resc());
        REQUIRE("e" == p.last_resc());
        REQUIRE(corrected_str == p.str());
        REQUIRE_THAT(p.add_child(parser::delimiter()).code(),
                     equals_irods_error(SYS_INVALID_INPUT_PARAM));
        REQUIRE(corrected_str == p.str());
        REQUIRE_THROWS_WITH(p.add_parent(parser::delimiter()),
                            Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
        REQUIRE(corrected_str == p.str());
    }
    SECTION("leading delimiter") {
        const std::string str = ";a;b;c;d;e";
        const std::string corrected_str = "a;b;c;d;e";
        parser p{str};

        REQUIRE(5 == p.num_levels());
        REQUIRE("a" == p.first_resc());
        REQUIRE("e" == p.last_resc());
        REQUIRE(corrected_str == p.str());
    }
}

TEST_CASE("test_hierarchy_parser_set_string_empty", "[delim][empty][pathological]") {
    parser p;
    SECTION("delimiter") {
        REQUIRE_THAT(p.set_string(parser::delimiter()).code(),
                     equals_irods_error(SYS_INVALID_INPUT_PARAM));
    }
    SECTION("empty string") {
        REQUIRE_THAT(p.set_string({}).code(),
                     equals_irods_error(SYS_INVALID_INPUT_PARAM));
    }
}

TEST_CASE("test_hierarchy_parser_empty", "[delim][empty][pathological]") {
    REQUIRE_THROWS_WITH(parser{parser::delimiter()},
                        Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
    REQUIRE_THROWS_WITH(parser{""},
                        Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
}
