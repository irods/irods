#include <catch2/catch.hpp>

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsGenQuery.h"
#include "irods/rodsErrorTable.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

// The original GenQuery1 string-based parser.
// This function is maintained as part of this unit test for backward compatibility reasons.
auto fillGenQueryInpFromStrCond(char* str, genQueryInp_t* genQueryInp) -> int;

// The following functions are only used by the original GenQuery1 parser.
// Therefore, they have been pulled out of the public iRODS interface to keep users from using them.
auto getCondFromString(char* t) -> char*;
auto goodStrExpr(char* expr) -> int;
auto separateSelFuncFromAttr(char* t, char** aggOp, char** colNm) -> int;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("GenQuery1 flex bison parser maintains backward compatibility with original GenQuery1 parser")
{
    // IMPORTANT: This test assumes all queries CAN be parsed into a valid GenQuery1
    // input struct.

    // Open file containing list of GenQuery1 query strings. This list of queries was
    // generated from the unit and core test suites using the following steps:
    //
    //   1. Modify fillGenQueryInpFromStrCond() so that it appends each query string to a local file.
    //      The file is written to a directory which is automatically captured by the testing
    //      environment (e.g. /var/log/irods).
    //   2. If the tests were run in parallel, take each file (now stored on the host) and concatenate them.
    //   3. Clean up file contents (i.e. dedup lines, etc).
    const auto file_path =
        irods::get_irods_home_directory() / "unit_tests/genquery1_backwards_compatibility_queries.txt";
    std::ifstream in{file_path.generic_string()};
    REQUIRE(in);

    std::size_t iteration = 0;
    std::string query;
    genQueryInp_t input1{};
    genQueryInp_t input2{};

    while (std::getline(in, query)) {
        INFO(fmt::format("Query {} = [{}]", iteration++, query));

        irods::at_scope_exit_unsafe clear_input_structs{[&input1, &input2] {
            clearGenQueryInp(&input1);
            clearGenQueryInp(&input2);
        }};

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
}

TEST_CASE("GenQuery1 flex bison parser returns error on invalid input")
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
        INFO(fmt::format("Query {} = [{}]", iteration++, query));
        INFO(fmt::format("Expected error code = [{}]", ec));
        irods::at_scope_exit_unsafe clear_input_struct{[&input] { clearGenQueryInp(&input); }};
        CHECK(parse_genquery1_string(query, &input) == ec);
    }
}

TEST_CASE("GenQuery1 flex bison parser unescapes hex-escaped bytes")
{
    // clang-format off
    const std::vector<const char*> queries{
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion'",
        "select COLL_ID where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collect\\x27ion'",
        "select COLL_INHERITANCE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination/collect\\x27ion'",
        "select COLL_INHERITANCE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion'",
        "select COLL_INHERITANCE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion'",
        "select COLL_INHERITANCE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion'",
        "select COLL_INHERITANCE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'collect\\x27ion' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination'",
        "select DATA_ID where DATA_NAME = 'collect\\x27ion' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source'",
        "select DATA_ID where DATA_NAME = 'collect\\x27ion' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy'",
        "select DATA_ID where DATA_NAME = 'collect\\x27ion' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd'",
        "select DATA_ID where DATA_NAME = 'collect\\x27ion' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collection'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collection'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collection'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collection'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collection'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data\\x27_object' and COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collection'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion'",
        "select DATA_ID where DATA_NAME = 'data_object' and COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collect\\x27ion'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/destination/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:59:50--irods-testing-4071rg3i' and DATA_NAME = 'test_iquery_supports_embedded_single_quotes\\x27__issue_5727' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource1'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collection' and DATA_NAME = 'data\\x27_object' and DATA_RESC_HIER = 'resource2'",
        "select DATA_ID, DATA_REPL_NUM where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:01:46--irods-testing-oelkjfme' and DATA_NAME = 'data\\x27 object' and DATA_RESC_HIER = 'demoResc'",
        "select DATA_NAME where DATA_NAME = 'data\\x27 object'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:54:35--irods-testing-nt363h5o/source/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/destination/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:55:35--irods-testing-z20gce16/source/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:56:54--irods-testing-fom11asy/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z00:57:48--irods-testing-_t5xtppd/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/alice/2024-12-11Z01:03:39--irods-testing-_6pox9bl/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collect\\x27ion' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collect\\x27ion' and DATA_NAME = 'data_object' and DATA_TOKEN_NAMESPACE = 'access_type'",
        "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE where COLL_NAME = '/tempZone/home/otherrods/2024-12-11Z01:02:48--irods-testing-fpemvq6f/collection' and DATA_NAME = 'data\\x27_object' and DATA_TOKEN_NAMESPACE = 'access_type'"
    };
    // clang-format on

    std::size_t iteration = 0;
    std::string query;
    genQueryInp_t input{};

    // Show that the new parser unescapes the hex-escaped single quote.
    {
        INFO(fmt::format("Query 0 = [{}]", query));
        irods::at_scope_exit_unsafe clear_input_struct{[&input] { clearGenQueryInp(&input); }};
        CHECK(parse_genquery1_string(queries[0], &input) == 0);
        CHECK(input.sqlCondInp.len == 1);
        CHECK(input.sqlCondInp.inx[0] == COL_COLL_NAME);
        CHECK(std::string_view{input.sqlCondInp.value[0]}.find('\'') != std::string_view::npos);
    }

    for (auto&& query : queries) {
        INFO(fmt::format("Query {} = [{}]", iteration++, query));
        irods::at_scope_exit_unsafe clear_input_struct{[&input] { clearGenQueryInp(&input); }};
        CHECK(parse_genquery1_string(query, &input) == 0);
    }
}

auto fillGenQueryInpFromStrCond(char* str, genQueryInp_t* genQueryInp) -> int
{
    int n, m;
    char *p, *t, *f, *u, *a, *c;
    char* s;
    s = strdup(str);
    if ((t = strstr(s, "select")) != NULL || (t = strstr(s, "SELECT")) != NULL) {
        if ((f = strstr(t, "where")) != NULL || (f = strstr(t, "WHERE")) != NULL) {
            /* Where Condition Found*/
            *f = '\0';
        }
        t = t + 7;
        while ((u = strchr(t, ',')) != NULL) {
            *u = '\0';
            trimWS(t);
            separateSelFuncFromAttr(t, &a, &c);
            m = getSelVal(a);
            n = getAttrIdFromAttrName(c);
            if (n < 0) {
                free(s);
                return n;
            }
            addInxIval(&genQueryInp->selectInp, n, m);
            t = u + 1;
        }
        trimWS(t);
        separateSelFuncFromAttr(t, &a, &c);
        m = getSelVal(a);
        n = getAttrIdFromAttrName(c);
        if (n < 0) {
            free(s);
            return n;
        }
        addInxIval(&genQueryInp->selectInp, n, m);
        if (f == NULL) {
            free(s);
            return 0;
        }
    }
    else {
        free(s);
        return INPUT_ARG_NOT_WELL_FORMED_ERR;
    }
    t = f + 6;
    while ((u = getCondFromString(t)) != NULL) {
        *u = '\0';
        trimWS(t);
        if ((p = strchr(t, ' ')) == NULL) {
            free(s);
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }
        *p = '\0';
        n = getAttrIdFromAttrName(t);
        if (n < 0) {
            free(s);
            return n;
        }
        addInxVal(&genQueryInp->sqlCondInp, n, p + 1);
        t = u + 5;
    }
    trimWS(t);
    if ((p = strchr(t, ' ')) == NULL) {
        free(s);
        return INPUT_ARG_NOT_WELL_FORMED_ERR;
    }
    *p = '\0';
    n = getAttrIdFromAttrName(t);
    if (n < 0) {
        free(s);
        return n;
    }
    addInxVal(&genQueryInp->sqlCondInp, n, p + 1);
    free(s);
    return 0;
} // fillGenQueryInpFromStrCond

auto getCondFromString(char* t) -> char*
{
    char* u;
    char *u1, *u2;
    char* s;

    s = t;
    for (;;) {
        /* Search for an 'and' string, either case, and use the one
           that appears first. */
        u1 = strstr(s, " and ");
        u2 = strstr(s, " AND ");
        u = u1;
        if (u1 == NULL) {
            u = u2;
        }
        if (u1 != NULL && u2 != NULL) {
            if (strlen(u2) > strlen(u1)) {
                u = u2; /* both are present, use the first */
            }
        }

        if (u != NULL) {
            *u = '\0';
            if (goodStrExpr(t) == 0) {
                *u = ' ';
                return u;
            }
            *u = ' ';
            s = u + 1;
        }
        else {
            break;
        }
    }
    return NULL;
} // getCondFromString

auto goodStrExpr(char* expr) -> int
{
    int qcnt = 0;
    int qqcnt = 0;
    int bcnt = 0;
    int i = 0;
    int inq = 0;
    int inqq = 0;
    while (expr[i] != '\0') {
        if (inq) {
            if (expr[i] == '\'') {
                inq--;
                qcnt++;
            }
        }
        else if (inqq) {
            if (expr[i] == '"') {
                inqq--;
                qqcnt++;
            }
        }
        else if (expr[i] == '\'') {
            inq++;
            qcnt++;
        }
        else if (expr[i] == '"') {
            inqq++;
            qqcnt++;
        }
        else if (expr[i] == '(') {
            bcnt++;
        }
        else if (expr[i] == ')')
            if (bcnt > 0) {
                bcnt--;
            }
        i++;
    }
    if (bcnt != 0 || qcnt % 2 != 0 || qqcnt % 2 != 0) {
        return -1;
    }
    return 0;
} // goodStrExpr

auto separateSelFuncFromAttr(char* t, char** aggOp, char** colNm) -> int
{
    char* s;

    if ((s = strchr(t, '(')) == NULL) {
        *colNm = t;
        *aggOp = NULL;
        return 0;
    }
    *aggOp = t;
    *s = '\0';
    s++;
    *colNm = s;
    if ((s = strchr(*colNm, ')')) == NULL) {
        return NO_COLUMN_NAME_FOUND;
    }
    *s = '\0';
    return 0;
} // separateSelFuncFromAttr
