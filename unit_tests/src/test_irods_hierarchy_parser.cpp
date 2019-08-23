#include "catch.hpp"

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
    SECTION("resc_in_hier") {
        REQUIRE(p.resc_in_hier("a"));
        REQUIRE(p.resc_in_hier("e"));
        REQUIRE(!p.resc_in_hier("b;c;d"));
        REQUIRE(!p.resc_in_hier("f"));
    }
    SECTION("next") {
        REQUIRE("b" == p.next("a"));
        REQUIRE("e" == p.next("d"));
        REQUIRE_THROWS_WITH(p.next("e"),
                            Catch::Contains(std::to_string(NO_NEXT_RESC_FOUND)));
        REQUIRE_THROWS_WITH(p.next("f"),
                            Catch::Contains(std::to_string(CHILD_NOT_FOUND)));
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
    SECTION("resc_in_hier") {
        REQUIRE(p.resc_in_hier("a"));
        REQUIRE(!p.resc_in_hier("b"));
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
    SECTION("delimiter") {
        parser p;
        irods::error e = p.set_string(parser::delimiter());
        REQUIRE_THAT(e.code(), equals_irods_error(SYS_INVALID_INPUT_PARAM));
    }
    SECTION("empty string") {
        parser p;
        irods::error e = p.set_string("");
        REQUIRE_THAT(e.code(), equals_irods_error(SYS_INVALID_INPUT_PARAM));
    }
}

TEST_CASE("test_hierarchy_parser_empty", "[delim][empty][pathological]") {
    REQUIRE_THROWS_WITH(parser{parser::delimiter()},
                        Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
    REQUIRE_THROWS_WITH(parser{""},
                        Catch::Contains(std::to_string(SYS_INVALID_INPUT_PARAM)));
}
