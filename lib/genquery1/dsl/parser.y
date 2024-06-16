%skeleton "lalr1.cc" /* Enables C++ */
%require "3.0" /* position.hh and stack.hh are deprecated in 3.2 and later. */
%language "C++" /* Redundant? */
%defines /* Doesn't work in version 3.0 - %header */

/*
Request that symbols be handled as a whole (type, value, and possibly location)
in the scanner. In C++, only works when variant-based semantic values are enabled.
This option causes make_* functions to be generated for each token kind.
*/
%define api.token.constructor 

/* Needed when application uses multiple parsers within the same program. */
/* %define api.prefix {gq1} */

/* %define api.parser.class {gq1_parser} */
%define api.namespace {gq1}

/* The type used for semantic values. */
%define api.value.type variant

/* Enables runtime assertions to catch invalid uses. In C++, detects improper use of variants. */
%define parse.assert

%define parse.trace
%define parse.error verbose /* Can produce incorrect information if LAC is not enabled. */

/* An argument passed to both the lexer and the parser. */
%param {irods::experimental::genquery1::driver& drv}

%locations

/* Code to be included in the parser's header and implementation file. */
%code provides {
    #undef YY_DECL

    #define YY_DECL \
        gq1::parser::symbol_type irods::experimental::genquery1::scanner::yylex(irods::experimental::genquery1::driver& drv)
}

/* Code to be included in the parser's implementation file. */
%code requires {
    #include "irods/irods_exception.hpp"
    #include "irods/rcMisc.h"
    #include "irods/rodsErrorTable.h"
    #include "irods/rodsGenQueryNames.h"

    #include <fmt/format.h>

    #include <string>

    namespace irods::experimental::genquery1
    {
        class driver;
    } // namespace irods::experimental::genquery1
}

%code {
    #include "irods/private/genquery1_driver.hpp"

    #include <boost/algorithm/string.hpp>

    #include <algorithm>
    #include <optional>
    #include <string_view>

    auto yylex(irods::experimental::genquery1::driver&) -> gq1::parser::symbol_type;

    auto find_column_id_by_name(std::string_view _column) -> int;

    auto add_column_to_projection_list(irods::experimental::genquery1::driver& _drv,
                                       std::string_view _column,
                                       std::optional<std::string_view> _fn = std::nullopt) -> void;

    auto add_condition(irods::experimental::genquery1::driver& _drv,
                       std::string_view _column,
                       std::string_view _condition) -> void;
}

%define api.token.prefix {IRODS_GENQUERY1_TOKEN_}
%token
    AND
    BEGIN_OF
    BETWEEN
    COMMA ","
    DOUBLE_AMPERSANDS "&&"
    DOUBLE_VERTICAL_BARS "||"
    EQUAL
    GREATER_THAN
    GREATER_THAN_OR_EQUAL_TO
    IN
    LESS_THAN
    LESS_THAN_OR_EQUAL_TO
    LIKE
    NOT
    NOT_EQUAL
    NULL
    PARENT_OF
    PAREN_CLOSE ")"
    PAREN_OPEN "("
    SELECT
    WHERE
;
%token <std::string>
    IDENTIFIER
    STRING_LITERAL
;
%token END_OF_INPUT 0

%nterm projection_list;
%nterm projection;
%nterm condition_list;
%nterm condition;
%nterm <std::string> condition_expression_list;
%nterm <std::string> condition_expression;
%nterm <std::string> string_literal_list;

%start genquery1 /* Defines where grammar starts */

%%

genquery1:
  SELECT projection_list
| SELECT projection_list WHERE condition_list
;

projection_list:
  projection
| projection_list "," projection
;

projection:
  IDENTIFIER { add_column_to_projection_list(drv, $1); }
| IDENTIFIER "(" IDENTIFIER ")" { add_column_to_projection_list(drv, $3, $1); }
;

condition_list:
  condition
| condition_list AND condition
;

condition:
  IDENTIFIER condition_expression_list { add_condition(drv, $1, $2); }
;

condition_expression_list:
  condition_expression
| condition_expression_list "||" condition_expression { $$ = fmt::format("{} || {}", $1, $3); }
| condition_expression_list "&&" condition_expression { $$ = fmt::format("{} && {}", $1, $3); }
;

condition_expression:
  LIKE STRING_LITERAL { $$ = fmt::format("like '{}'", $2); }
| NOT LIKE STRING_LITERAL { $$ = fmt::format("not like '{}'", $3); }
| BETWEEN STRING_LITERAL STRING_LITERAL { $$ = fmt::format("between '{}' '{}'", $2, $3); }
/*| NOT BETWEEN STRING_LITERAL STRING_LITERAL { $$ = fmt::format("not between '{}' '{}'", $3, $4); } - not supported or i entered the wrong syntax? */
| EQUAL STRING_LITERAL  { $$ = fmt::format("= '{}'", $2); }
| NOT_EQUAL STRING_LITERAL { $$ = fmt::format("!= '{}'", $2); }
| LESS_THAN STRING_LITERAL  { $$ = fmt::format("< '{}'", $2); }
| LESS_THAN_OR_EQUAL_TO STRING_LITERAL { $$ = fmt::format("<= '{}'", $2); }
| GREATER_THAN STRING_LITERAL  { $$ = fmt::format("> '{}'", $2); }
| GREATER_THAN_OR_EQUAL_TO STRING_LITERAL { $$ = fmt::format(">= '{}'", $2); }
| IN "(" string_literal_list ")" { $$ = fmt::format("in ({})", $3); }
| IN string_literal_list { $$ = fmt::format("in {}", $2); }
/*| NOT IN "(" string_literal_list ")" { $$ = fmt::format("not in ({})", $4); } - not supported or i entered the wrong syntax? */
/* See https://github.com/irods/irods-legacy/commit/ef62e815bb2df7521acc424d8bc8cf1760cd7e60 for details about the following. */
| BEGIN_OF STRING_LITERAL { $$ = fmt::format("begin_of '{}'", $2); }
| PARENT_OF STRING_LITERAL { $$ = fmt::format("parent_of '{}'", $2); }
;

string_literal_list:
  STRING_LITERAL { $$ = fmt::format("'{}'", $1); }
| string_literal_list "," STRING_LITERAL { $$ = fmt::format("{}, '{}'", $1, $3); }
;

%%

auto gq1::parser::error(const gq1::location& _loc, const std::string& _msg) -> void
{
    THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, fmt::format("{} at position {}", _msg, _loc.begin.column));
} // gq1::parser::error

auto yylex(irods::experimental::genquery1::driver& drv) -> gq1::parser::symbol_type
{
    return drv.lexer.yylex(drv);
} // yylex

auto find_column_id_by_name(std::string_view _column) -> int
{
    for (int i = 0; i < NumOfColumnNames; ++i) {
        if (const auto& entry = columnNames[i]; _column == entry.columnName) {
            return entry.columnId;
        }
    }

    THROW(NO_COLUMN_NAME_FOUND, fmt::format("column [{}] not supported", _column));
} // find_column_id_by_name

auto add_column_to_projection_list(irods::experimental::genquery1::driver& _drv,
                                   std::string_view _column,
                                   std::optional<std::string_view> _fn) -> void
{
    const auto column_id = find_column_id_by_name(_column);

    if (_fn) {
        int aggregate_fn = -1;

        // clang-format off
        if      (boost::iequals(*_fn, "order"))      { aggregate_fn = ORDER_BY; }
        else if (boost::iequals(*_fn, "order_desc")) { aggregate_fn = ORDER_BY_DESC; }
        else if (boost::iequals(*_fn, "min"))        { aggregate_fn = SELECT_MIN; }
        else if (boost::iequals(*_fn, "max"))        { aggregate_fn = SELECT_MAX; }
        else if (boost::iequals(*_fn, "sum"))        { aggregate_fn = SELECT_SUM; }
        else if (boost::iequals(*_fn, "avg"))        { aggregate_fn = SELECT_AVG; }
        else if (boost::iequals(*_fn, "count"))      { aggregate_fn = SELECT_COUNT; }
        // clang-format on
        else {
            // When fillGenQueryInpFromStrCond() entered this case, it would ignore the unknown aggregate
            // function and continue as if the client never passed it. For example, the following iquest
            // queries are equivalent when fillGenQueryInpFromStrCond() is used.
            //
            //     iquest "select ignored(COLL_NAME)"
            //     iquest "select COLL_NAME"
            //
            // This flex / bison parser correctly treats this as an error and throws an exception.
            THROW(INVALID_OPERATION, fmt::format("aggregate function [{}] not supported", *_fn));
        }

        addInxIval(&_drv.gq_input->selectInp, column_id, aggregate_fn);

        return;
    }

    addInxIval(&_drv.gq_input->selectInp, column_id, 1);
} // add_column_to_projection_list

auto add_condition(irods::experimental::genquery1::driver& _drv,
                   std::string_view _column,
                   std::string_view _condition) -> void
{
    const auto column_id = find_column_id_by_name(_column);
    addInxVal(&_drv.gq_input->sqlCondInp, column_id, std::string{_condition}.c_str());
} // add_condition

// Function prototype (declaration) is located in rcMisc.h.
// The definition is placed here to avoid a circular dependency between
// rcMisc.cpp and this flex/bison parser code.
int parse_genquery1_string(const char* _s, genQueryInp_t* _out)
{
    try {
        namespace gq1 = irods::experimental::genquery1;

        gq1::driver driver;
        driver.gq_input = _out;

        if (const auto ec = driver.parse(_s); ec != 0) {
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        return 0;
    }
    catch (const irods::exception& e) {
        return e.code();
    }
    catch (const std::exception& e) {
        return SYS_LIBRARY_ERROR;
    }
} // parse_genquery1_string
