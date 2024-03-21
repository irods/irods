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
    #include <vector>

    #include <fmt/format.h>

    namespace irods::experimental::genquery2
    {
        class driver;
    } // namespace irods::experimental::genquery2

    // According to the Bison docs, code in a %code requires/provides block will be
    // included in the parser header file if Bison is instructed to generate a header file.
    namespace gq2_detail = irods::experimental::api::genquery2;
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
    NO DISTINCT
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
    INTEGER
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

%type <gq2_detail::selections>                   selections;
%type <gq2_detail::conditions>                   conditions;
%type <gq2_detail::group_by>                     group_by;
%type <gq2_detail::order_by>                     order_by;
%type <std::vector<gq2_detail::sort_expression>> sort_expr;
%type <gq2_detail::range>                        range;
%type <gq2_detail::selection>                    selection;
%type <gq2_detail::column>                       column;
%type <gq2_detail::select_function>              select_function;
%type <gq2_detail::condition>                    condition;
%type <gq2_detail::condition_expression>         condition_expression;
%type <std::vector<std::string>>                 list_of_string_literals;
%type <std::vector<std::string>>                 list_of_identifiers;

%start genquery /* Defines where grammar starts */

%%

genquery:
    select group_by  { std::swap(drv.select.group_by, $2); }
  | select group_by order_by  { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $3); }
  | select group_by range  { std::swap(drv.select.group_by, $2); std::swap(drv.select.range, $3); }
  | select group_by order_by range  { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $3); std::swap(drv.select.range, $4); }
  | select group_by range order_by  { std::swap(drv.select.group_by, $2); std::swap(drv.select.order_by, $4); std::swap(drv.select.range, $3); }

select:
    SELECT selections  { std::swap(drv.select.selections, $2); }
  | SELECT selections WHERE conditions  { std::swap(drv.select.selections, $2); std::swap(drv.select.conditions, $4); }
  | SELECT NO DISTINCT selections  { drv.select.distinct = false; std::swap(drv.select.selections, $4); }
  | SELECT NO DISTINCT selections WHERE conditions  { drv.select.distinct = false; std::swap(drv.select.selections, $4); std::swap(drv.select.conditions, $6); }

group_by:
    %empty
  | GROUP BY list_of_identifiers { std::swap($$.columns, $3); }

order_by:
    ORDER BY sort_expr  { std::swap($$.sort_expressions, $3); }

sort_expr:
    IDENTIFIER  { $$.push_back(gq2_detail::sort_expression{$1, true}); }
  | IDENTIFIER ASC  { $$.push_back(gq2_detail::sort_expression{$1, true}); }
  | IDENTIFIER DESC  { $$.push_back(gq2_detail::sort_expression{$1, false}); }
  | sort_expr COMMA IDENTIFIER  { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
  | sort_expr COMMA IDENTIFIER ASC  { $1.push_back(gq2_detail::sort_expression{$3, true}); std::swap($$, $1); }
  | sort_expr COMMA IDENTIFIER DESC  { $1.push_back(gq2_detail::sort_expression{$3, false}); std::swap($$, $1); }

range:
    OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $2); }
  | OFFSET POSITIVE_INTEGER FETCH FIRST POSITIVE_INTEGER ROWS ONLY  { std::swap($$.offset, $2); std::swap($$.number_of_rows, $5); }
  | OFFSET POSITIVE_INTEGER LIMIT POSITIVE_INTEGER  { std::swap($$.offset, $2); std::swap($$.number_of_rows, $4); }
  | FETCH FIRST POSITIVE_INTEGER ROWS ONLY  { std::swap($$.number_of_rows, $3); }
  | FETCH FIRST POSITIVE_INTEGER ROWS ONLY OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $7); std::swap($$.number_of_rows, $3); }
  | LIMIT POSITIVE_INTEGER  { std::swap($$.number_of_rows, $2); }
  | LIMIT POSITIVE_INTEGER OFFSET POSITIVE_INTEGER  { std::swap($$.offset, $4); std::swap($$.number_of_rows, $2); }

selections:
    selection  { $$ = gq2_detail::selections{std::move($1)}; }
  | selections COMMA selection  { $1.push_back(std::move($3)); std::swap($$, $1); }

selection:
    column  { $$ = std::move($1); }
  | select_function  { $$ = std::move($1); }

column:
    IDENTIFIER  { $$ = gq2_detail::column{std::move($1)}; }
  | CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_CLOSE  { $$ = gq2_detail::column{$3, $5}; }
  | CAST PAREN_OPEN IDENTIFIER AS IDENTIFIER PAREN_OPEN POSITIVE_INTEGER PAREN_CLOSE PAREN_CLOSE  { $$ = gq2_detail::column{$3, fmt::format("{}({})", $5, $7)}; }

select_function:
    IDENTIFIER PAREN_OPEN column PAREN_CLOSE  { $$ = gq2_detail::select_function{std::move($1), gq2_detail::column{std::move($3)}}; }
    /*IDENTIFIER PAREN_OPEN IDENTIFIER PAREN_CLOSE  { $$ = gq2_detail::select_function{std::move($1), gq2_detail::column{std::move($3)}}; }*/

conditions:
    condition  { $$ = gq2_detail::conditions{std::move($1)}; }
  | conditions AND conditions  { $1.push_back(gq2_detail::logical_and{std::move($3)}); std::swap($$, $1); }
  | conditions OR conditions  { $1.push_back(gq2_detail::logical_or{std::move($3)}); std::swap($$, $1); }
  | PAREN_OPEN conditions PAREN_CLOSE  { $$ = gq2_detail::conditions{gq2_detail::logical_grouping{std::move($2)}}; }
  | NOT conditions  { $$ = gq2_detail::conditions{gq2_detail::logical_not{std::move($2)}}; }

condition:
    column condition_expression  { $$ = gq2_detail::condition(std::move($1), std::move($2)); }

condition_expression:
    LIKE STRING_LITERAL  { $$ = gq2_detail::condition_like(std::move($2)); }
  | NOT LIKE STRING_LITERAL  { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_like(std::move($3))}; }
  | IN PAREN_OPEN list_of_string_literals PAREN_CLOSE  { $$ = gq2_detail::condition_in(std::move($3)); }
  | NOT IN PAREN_OPEN list_of_string_literals PAREN_CLOSE  { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_in(std::move($4))}; }
  | BETWEEN STRING_LITERAL AND STRING_LITERAL  { $$ = gq2_detail::condition_between(std::move($2), std::move($4)); }
  | NOT BETWEEN STRING_LITERAL AND STRING_LITERAL  { $$ = gq2_detail::condition_operator_not{gq2_detail::condition_between(std::move($3), std::move($5))}; }
  | EQUAL STRING_LITERAL  { $$ = gq2_detail::condition_equal(std::move($2)); }
  | NOT_EQUAL STRING_LITERAL  { $$ = gq2_detail::condition_not_equal(std::move($2)); }
  | LESS_THAN STRING_LITERAL  { $$ = gq2_detail::condition_less_than(std::move($2)); }
  | LESS_THAN_OR_EQUAL_TO STRING_LITERAL  { $$ = gq2_detail::condition_less_than_or_equal_to(std::move($2)); }
  | GREATER_THAN STRING_LITERAL  { $$ = gq2_detail::condition_greater_than(std::move($2)); }
  | GREATER_THAN_OR_EQUAL_TO STRING_LITERAL  { $$ = gq2_detail::condition_greater_than_or_equal_to(std::move($2)); }
  | IS NULL  { $$ = gq2_detail::condition_is_null{}; }
  | IS NOT NULL  { $$ = gq2_detail::condition_is_not_null{}; }

list_of_string_literals:
    STRING_LITERAL  { $$ = std::vector<std::string>{std::move($1)}; }
  | list_of_string_literals COMMA STRING_LITERAL  { $1.push_back(std::move($3)); std::swap($$, $1); }

list_of_identifiers:
    IDENTIFIER  { $$ = std::vector<std::string>{std::move($1)}; }
  | list_of_identifiers COMMA IDENTIFIER  { $1.push_back(std::move($3)); std::swap($$, $1); }

%%

auto yy::parser::error(const yy::location& _loc, const std::string& _msg) -> void
{
    throw std::invalid_argument{fmt::format("{} at position {}", _msg, _loc.begin.column)};
} // yy::parser::error

auto yylex(irods::experimental::genquery2::driver& drv) -> yy::parser::symbol_type
{
    return drv.lexer.yylex(drv);
} // yylex
