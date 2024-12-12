#include <catch2/catch.hpp>

//#include "boost/algorithm/string/replace.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsGenQuery.h"
#include "irods/rodsErrorTable.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>

#include <cstring>
#include <fstream>
#include <string> // For std::getline
#include <utility>
#include <vector>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("genquery1 flex bison parser maintains backward compatibility")
{
    // IMPORTANT: This test assumes all queries CAN be parsed into a valid GenQuery1
    // input struct.

    // Open file containing list of GenQuery1 query strings. This list of queries was
    // produced from the test suite.
    const auto file_path = irods::get_irods_home_directory() / "unit_tests/genquery1_backwards_compatibility_queries.txt";
    std::ifstream in{file_path.generic_string()};
    REQUIRE(in);

    std::size_t iteration = 0;
    std::size_t skipped = 0;
    std::string query;
    genQueryInp_t input1{};
    genQueryInp_t input2{};

    while (std::getline(in, query)) {
        INFO(fmt::format("Query {} = [{}]", iteration++, query))

        if (query.find("\\x27") != std::string::npos) {
            ++skipped;
            continue;
        }

        irods::at_scope_exit_unsafe clear_input_structs{[&input1, &input2] {
            clearGenQueryInp(&input1);
            clearGenQueryInp(&input2);
        }};

        // The query may contain escaped single quotes. This has to do with how the queries
        // were captured. The original GenQuery1 parser does not understand hex-encoded bytes
        // so the test must unescape them. The new parser will convert the hex-encoded bytes
        // to their unescaped form automatically. Without this, the string-based comparisons
        // involving embedded single quotes would always fail.
        //auto unescaped_query = boost::replace_all_copy(query, "\\x27", "'");

        // Parse the query string into a genQueryInp_t struct.
        const auto ec1 = fillGenQueryInpFromStrCond(query.data(), &input1);
        const auto ec2 = parse_genquery1_string(query.c_str(), &input2);
        REQUIRE(ec1 == ec2);

        // Show that both parsers produce the same result. This proves the new flex/bison
        // parser doesn't break backwards compatibility.
        CHECK(input1.continueInx == input2.continueInx);
        CHECK(input1.maxRows == input2.maxRows);
        CHECK(input1.options == input2.options);
        CHECK(input1.rowOffset == input2.rowOffset);

        CHECK(input1.selectInp.len == input2.selectInp.len);
        CHECK(input1.sqlCondInp.len == input2.sqlCondInp.len);

        for (int i = 0; i < input1.selectInp.len; ++i) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            INFO("input1.selectInp.inx[" << i << "] = [" << input1.selectInp.inx[i] << ']');
            INFO("input2.selectInp.inx[" << i << "] = [" << input2.selectInp.inx[i] << ']');
            CHECK(input1.selectInp.inx[i] == input2.selectInp.inx[i]);
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            INFO("input1.selectInp.value[" << i << "] = [" << input1.selectInp.value[i] << ']');
            INFO("input2.selectInp.value[" << i << "] = [" << input2.selectInp.value[i] << ']');
            CHECK(input1.selectInp.value[i] == input2.selectInp.value[i]);
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        for (int i = 0; i < input1.sqlCondInp.len; ++i) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            INFO("input1.sqlCondInp.inx[" << i << "] = [" << input1.sqlCondInp.inx[i] << ']');
            INFO("input2.sqlCondInp.inx[" << i << "] = [" << input2.sqlCondInp.inx[i] << ']');
            CHECK(input1.sqlCondInp.inx[i] == input2.sqlCondInp.inx[i]);
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            INFO("input1.sqlCondInp.value[" << i << "] = [" << input1.sqlCondInp.value[i] << ']');
            INFO("input2.sqlCondInp.value[" << i << "] = [" << input2.sqlCondInp.value[i] << ']');
            CHECK(boost::iequals(input1.sqlCondInp.value[i], input2.sqlCondInp.value[i]));
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }

    SUCCEED("Queries: total=[" << iteration << "], skipped=[" << skipped << ']');
}

TEST_CASE("genquery1 flex bison parser returns error on invalid input")
{
    // clang-format off
    const std::vector<std::pair<const char*, int>> invalid_queries{
        {"bad formatting", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_GROUP_NAME where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z00:41:48--irods-testing-zzdtv1n3' and DATA_NAME = 'test_modifying_restricted_columns'", NO_COLUMN_NAME_FOUND},
        {"select bad_aggregate_func(COLL_NAME)", INVALID_OPERATION},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER  'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER  'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER == 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER == 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER lik 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER lik 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER like not like 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER like not like 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not lik 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not lik 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not like like 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not like like 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not not like 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER not not like 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER notlike 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER notlike 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER ot like 'root;mid;leaf'", INPUT_ARG_NOT_WELL_FORMED_ERR},
        {"select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER ot like 'root;mid;leaf1' || like 'root;mid;leaf1'", INPUT_ARG_NOT_WELL_FORMED_ERR}
    };
    // clang-format on

    std::size_t iteration = 0;
    std::string query;
    genQueryInp_t input{};

    for (auto&& [query, ec] : invalid_queries) {
        INFO(fmt::format("Query {} = [{}]", iteration++, query))
        INFO(fmt::format("Expected error code = [{}]", ec))
        irods::at_scope_exit_unsafe clear_input_struct{[&input] { clearGenQueryInp(&input); }};
        CHECK(parse_genquery1_string(query, &input) == ec);
    }
}
