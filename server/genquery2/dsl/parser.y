%skeleton "lalr1.cc" /* Enables C++ */
%require "3.0" /* position.hh and stack.hh are deprecated in 3.2 and later. */
%language "C++" /* Redundant? */
%defines /* Doesn't work in version 3.0 - %header */

/* The name of the parser class. Defaults to "parser" in C++. */
/* %define api.parser.class { parser } */

/*
Request that symbols be handled as a whole (type, value, and possibly location)
in the scanner. In C++, only works when variant-based semantic values are enabled.
This option causes make_* functions to be generated for each token kind.
*/
%define api.token.constructor

/* The type used for semantic values. */
%define api.value.type variant

/* Enables runtime assertions to catch invalid uses. In C++, detects improper use of variants. */
%define parse.assert

%define parse.trace
%define parse.error verbose /* Can produce incorrect information if LAC is not enabled. */

/* Defines the namespace for the parser class. */
/*%define api.namespace { irods::experimental::genquery2 }*/

/* An argument passed to both the lexer and the parser. */
%param {irods::experimental::genquery2::driver& drv}

%locations

/* Code to be included in the parser's header and implementation file. */
%code provides {
    #undef YY_DECL

    #define YY_DECL \
        yy::parser::symbol_type irods::experimental::genquery2::scanner::yylex(irods::experimental::genquery2::driver& drv)
}

/* Code to be included in the parser's implementation file. */
%code requires {
    #include "irods/private/genquery2_ast_types.hpp"

    #include <string>
    #include <variant>
    #include <vector>

    #include <fmt/format.h>

    namespace irods::experimental::genquery2
    {
        class driver;
    } // namespace irods::experimental::genquery2

    // According to the Bison docs, code in a %code requires/provides block will be
    // included in the parser header file if Bison is instructed to generate a header file.
    namespace gq2_detail = irods::experimental::genquery2;
}

%code {
    #include "irods/private/genquery2_driver.hpp"

    auto yylex(irods::experimental::genquery2::driver&) -> yy::parser::symbol_type;
}

%define api.token.prefix {IRODS_GENQUERY2_TOKEN_}
%token
    AND
    AS
    ASC
    BETWEEN
    BY
    CASE
    CAST
    COMMA
    DESC
    ELSE
    END
    EQUAL
    EXISTS
    FETCH
    FIRST
    GREATER_THAN
    GREATER_THAN_OR_EQUAL_TO
    GROUP
    HAVING
    IN
    IS
    LESS_THAN
    LESS_THAN_OR_EQUAL_TO
    LIKE
    LIMIT
    DISTINCT
    NOT
    NOT_EQUAL
    NULL
    OFFSET
    ONLY
    OR
    ORDER
    PAREN_CLOSE
    PAREN_OPEN
    ROWS
    SELECT
    WHEN
    WHERE
;
%token <std::string>
    IDENTIFIER
    STRING_LITERAL
    POSITIVE_INTEGER
    NEGATIVE_INTEGER
;
%token END_OF_INPUT 0

/*
%left is the same as %token, but defines associativity.
%token does not define associativity or precedence.
%precedence does not define associativity.

Operator precedence is determined by the line ordering of the declarations.
The further down the page the line is, the higher the precedence. For example,
NOT has higher precedence than AND.

While these directives support specifying a semantic type, Bison recommends not
doing that and using these directives to specify precedence and associativity
rules only.
*/
%left OR
%left AND
%precedence NOT

%nterm <gq2_detail::projections>                      projection_list;
%nterm <gq2_detail::conditions>                       condition_list;
%nterm <gq2_detail::group_by>                         group_by;
%nterm <gq2_detail::order_by>                         order_by;
%nterm <std::vector<gq2_detail::sort_expression>>     sort_expression;
%nterm <std::vector<std::variant<gq2_detail::column, gq2_detail::function>>>  group_expression;
%nterm <gq2_detail::range>                            range;
%nterm <gq2_detail::projection>                       projection;
%nterm <gq2_detail::column>                           column;
%nterm <std::vector<std::variant<std::string, gq2_detail::column, gq2_detail::function>>> argument_list;
%nterm <gq2_detail::function>                         function;
%nterm <gq2_detail::condition>                        condition;
%nterm <gq2_detail::condition_expression>             condition_expression;
%nterm <std::vector<std::string>>                     string_literal_list;
%nterm <std::vector<std::string>>                     identifier_list;
%nterm <std::string>                                  integer;

%start genquery2 /* Defines where grammar starts */

%%

genquery2:
  select group_by { std::swap(drv.select.group_by, $2); }
| select group_by order_by { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $3); }
| select group_by range { std::swap(drv.select.group_by, $2); std::swap(drv.select.range, $3); }
| select group_by order_by range { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $3); std::swap(drv.select.range, $4); }
| select group_by range order_by { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $4); std::swap(drv.select.range, $3); }
;

select:
  SELECT projection_list { std::swap(drv.select.projections, $2); }
| SELECT projection_list WHERE condition_list { std::swap(drv.select.projections, $2); std::swap(drv.select.conditions, $4); }
| SELECT DISTINCT projection_list { drv.select.distinct = true; std::swap(drv.select.projections, $3); }
| SELECT DISTINCT projection_list WHERE condition_list { drv.select.distinct = true; std::swap(drv.select.projections, $3); std::swap(drv.select.conditions, $5); }
;

group_by:
  %empty { /* Generates a default initialized group_by structure. */ }
| GROUP BY group_expression { std::swap($$.expressions, $3); }
;

order_by:
  ORDER BY sort_expression { std::swap($$.sort_expressions, $3); }
;

/*
The "projection" rule would simplify this, but attempting to use that results in a compiler
error due to boost::variant not being compatible with std::variant. For now, we just add more rules
to achieve the desired outcome.

TODO(#7679): This will be simplified once the parser is converted to use std::variant.

The following would be possible if everything used std::variant.

    sort_expression:
      projection { $$.push_back(gq2_detail::sort_expression{$1, true}); }
    | projection ASC { $$.push_back(gq2_detail::sort_expression{$1, true}); }
    | projection DESC { $$.push_back(gq2_detail::sort_expression{$1, false}); }
    | sort_expression COMMA projection { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
    | sort_expression COMMA projection ASC { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
    | sort_expression COMMA projection DESC { $1.push_back(gq2_detail::sort_expression{$3, false}); std::swap($$, $1); }
    ;
*/
sort_expression:
  column { $$.push_back(gq2_detail::sort_expression{$1, true}); }
| column ASC { $$.push_back(gq2_detail::sort_expression{$1, true}); }
| column DESC { $$.push_back(gq2_detail::sort_expression{$1, false}); }
| sort_expression COMMA column { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
| sort_expression COMMA column ASC { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
| sort_expression COMMA column DESC { $1.push_back(gq2_detail::sort_expression{$3, false}); std::swap($$, $1); }
| function { $$.push_back(gq2_detail::sort_expression{$1, true}); }
| function ASC { $$.push_back(gq2_detail::sort_expression{$1, true}); }
| function DESC { $$.push_back(gq2_detail::sort_expression{$1, false}); }
| sort_expression COMMA function { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
| sort_expression COMMA function ASC { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
| sort_expression COMMA function DESC { $1.push_back(gq2_detail::sort_expression{$3, false}); std::swap($$, $1); }
;

group_expression:
  column { $$.push_back($1); }
| group_expression COMMA column { $1.push_back($3); std::swap($$, $1); }
| function { $$.push_back($1); }
| group_expression COMMA function { $1.push_back($3); std::swap($$, $1); }
;

range:
  OFFSET POSITIVE_INTEGER { std::swap($$.offset, $2); }
| OFFSET POSITIVE_INTEGER FETCH FIRST POSITIVE_INTEGER ROWS ONLY { std::swap($$.offset, $2); std::swap($$.number_of_rows, $5); }
| OFFSET POSITIVE_INTEGER LIMIT POSITIVE_INTEGER { std::swap($$.offset, $2); std::swap($$.number_of_rows, $4); }
| FETCH FIRST POSITIVE_INTEGER ROWS ONLY { std::swap($$.number_of_rows, $3); }
| FETCH FIRST POSITIVE_INTEGER ROWS ONLY OFFSET POSITIVE_INTEGER { std::swap($$.offset, $7); std::swap($$.number_of_rows, $3); }
| LIMIT POSITIVE_INTEGER { std::swap($$.number_of_rows, $2); }
| LIMIT POSITIVE_INTEGER OFFSET POSITIVE_INTEGER { std::swap($$.offset, $4); std::swap($$.number_of_rows, $2); }
;

projection_list:
  projection { $$ = gq2_detail::projections{std::move($1)}; }
| projection_list COMMA projection { $1.push_back(std::move($3)); std::swap($$, $1); }
;

projection:
  column { $$ = std::move($1); }
| function { $$ = std::move($1); }
;

column:
  IDENTIFIER { $$ = gq2_detail::column{std::move($1)}; }
| CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_CLOSE { $$ = gq2_detail::column{$3, $5}; }
| CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_OPEN POSITIVE_INTEGER PAREN_CLOSE PAREN_CLOSE { $$ = gq2_detail::column{$3, fmt::format("{}({})", $5, $7)}; }
;

argument_list:
  %empty { /* Generate an empty argument list. */ }
| STRING_LITERAL { $$.emplace_back($1); }
| integer { $$.emplace_back($1); }
| column { $$.emplace_back($1); }
| function { $$.emplace_back($1); }
| argument_list COMMA STRING_LITERAL { $1.emplace_back($3); std::swap($$, $1); }
| argument_list COMMA integer { $1.emplace_back($3); std::swap($$, $1); }
| argument_list COMMA column { $1.emplace_back($3); std::swap($$, $1); }
| argument_list COMMA function { $1.emplace_back($3); std::swap($$, $1); }
;

function:
  IDENTIFIER PAREN_OPEN argument_list PAREN_CLOSE { $$ = gq2_detail::function{std::move($1), std::move($3)}; }
| IDENTIFIER PAREN_OPEN DISTINCT argument_list PAREN_CLOSE { $$ = gq2_detail::function{std::move($1), std::move($4), true}; }
;

condition_list:
  condition { $$ = gq2_detail::conditions{std::move($1)}; }
| condition_list AND condition_list { $1.push_back(gq2_detail::logical_and{std::move($3)}); std::swap($$, $1); }
| condition_list OR condition_list { $1.push_back(gq2_detail::logical_or{std::move($3)}); std::swap($$, $1); }
| PAREN_OPEN condition_list PAREN_CLOSE { $$ = gq2_detail::conditions{gq2_detail::logical_grouping{std::move($2)}}; }
| NOT condition_list { $$ = gq2_detail::conditions{gq2_detail::logical_not{std::move($2)}}; }
;

condition:
  column condition_expression { $$ = gq2_detail::condition(std::move($1), std::move($2)); }
| function condition_expression { $$ = gq2_detail::condition(std::move($1), std::move($2)); }
;

condition_expression:
  LIKE STRING_LITERAL { $$ = gq2_detail::condition_like(std::move($2)); }
| NOT LIKE STRING_LITERAL { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_like(std::move($3))}; }
| IN PAREN_OPEN string_literal_list PAREN_CLOSE { $$ = gq2_detail::condition_in(std::move($3)); }
| NOT IN PAREN_OPEN string_literal_list PAREN_CLOSE { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_in(std::move($4))}; }
| BETWEEN STRING_LITERAL AND STRING_LITERAL { $$ = gq2_detail::condition_between(std::move($2), std::move($4)); }
| NOT BETWEEN STRING_LITERAL AND STRING_LITERAL { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_between(std::move($3), std::move($5))}; }
| EQUAL STRING_LITERAL { $$ = gq2_detail::condition_equal(std::move($2)); }
| NOT_EQUAL STRING_LITERAL { $$ = gq2_detail::condition_not_equal(std::move($2)); }
| LESS_THAN STRING_LITERAL { $$ = gq2_detail::condition_less_than(std::move($2)); }
| LESS_THAN_OR_EQUAL_TO STRING_LITERAL { $$ = gq2_detail::condition_less_than_or_equal_to(std::move($2)); }
| GREATER_THAN STRING_LITERAL { $$ = gq2_detail::condition_greater_than(std::move($2)); }
| GREATER_THAN_OR_EQUAL_TO STRING_LITERAL { $$ = gq2_detail::condition_greater_than_or_equal_to(std::move($2)); }
| IS NULL { $$ = gq2_detail::condition_is_null{}; }
| IS NOT NULL { $$ = gq2_detail::condition_is_not_null{}; }
;

string_literal_list:
  STRING_LITERAL { $$ = std::vector<std::string>{std::move($1)}; }
| string_literal_list COMMA STRING_LITERAL { $1.push_back(std::move($3)); std::swap($$, $1); }
;

identifier_list:
  IDENTIFIER { $$ = std::vector<std::string>{std::move($1)}; }
| identifier_list COMMA IDENTIFIER { $1.push_back(std::move($3)); std::swap($$, $1); }
;

integer:
  POSITIVE_INTEGER
| NEGATIVE_INTEGER { $$ = std::move($1); }
;

%%

auto yy::parser::error(const yy::location& _loc, const std::string& _msg) -> void
{
    throw std::invalid_argument{fmt::format("{} at position {}", _msg, _loc.begin.column)};
} // yy::parser::error

auto yylex(irods::experimental::genquery2::driver& drv) -> yy::parser::symbol_type
{
    return drv.lexer.yylex(drv);
} // yylex
