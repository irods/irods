%{
    #include "irods/private/genquery2_driver.hpp"

    #include <fmt/format.h>

    #include <charconv>
    #include <cstdint>
    #include <string>
%}

%option c++ warn nodefault noyywrap nounput noinput batch

/*
 Informs flex that we've derived a class from yyFlexLexer and its
 name is "scanner". flex will then place the lexer implementation
 into <derived_class>::yylex.
*/
%option yyclass="scanner"

%{
    // Code run each time a pattern is matched.
    #define YY_USER_ACTION loc.columns(YYLeng());
%}

%x ss_string_literal

%%

%{
    // A handy shortcut to the location held by the driver.
    auto& loc = drv.location;

    // Code run each time yylex() is called.
    loc.step();

    // A handy shortcut to the object used for capturing string literals.
    auto& string_literal = drv.string_literal;
%}

'                      { BEGIN(ss_string_literal); string_literal.clear(); }
<ss_string_literal>{
    "''"               { string_literal += '\''; }
    \\x[0-9A-Fa-f]{2}  {
                            std::uint8_t value;
                            const auto err = std::from_chars(YYText() + 2, YYText() + 4, value, 16);

                            if (err.ec == std::errc::invalid_argument) {
                                throw yy::parser::syntax_error{loc, fmt::format("invalid hex value: [{}]", YYText())};
                            }
                            else if (err.ec == std::errc::result_out_of_range) {
                                throw yy::parser::syntax_error{loc, fmt::format("hex value cannot be represented: [{}]", YYText())};
                            }

                            string_literal.push_back(value);
                       }
    '                  { BEGIN(INITIAL); return yy::parser::make_STRING_LITERAL(string_literal, loc); }
    .                  { string_literal += YYText(); }
    <<EOF>>            { throw yy::parser::syntax_error{loc, fmt::format("missing closing single quote: [{}]", string_literal)}; }
}
[ \t\n]                ;
(?i:select)            return yy::parser::make_SELECT(loc);
(?i:where)             return yy::parser::make_WHERE(loc);
(?i:like)              return yy::parser::make_LIKE(loc);
(?i:in)                return yy::parser::make_IN(loc);
(?i:between)           return yy::parser::make_BETWEEN(loc);
(?i:no)                return yy::parser::make_NO(loc);
(?i:distinct)          return yy::parser::make_DISTINCT(loc);
(?i:order)             return yy::parser::make_ORDER(loc);
(?i:by)                return yy::parser::make_BY(loc);
(?i:asc)               return yy::parser::make_ASC(loc);
(?i:desc)              return yy::parser::make_DESC(loc);
(?i:offset)            return yy::parser::make_OFFSET(loc);
(?i:limit)             return yy::parser::make_LIMIT(loc);
(?i:fetch)             return yy::parser::make_FETCH(loc);
(?i:first)             return yy::parser::make_FIRST(loc);
(?i:rows)              return yy::parser::make_ROWS(loc);
(?i:only)              return yy::parser::make_ONLY(loc);
(?i:case)              return yy::parser::make_CASE(loc);
(?i:when)              return yy::parser::make_WHEN(loc);
(?i:else)              return yy::parser::make_ELSE(loc);
(?i:end)               return yy::parser::make_END(loc);
(?i:group)             return yy::parser::make_GROUP(loc);
(?i:having)            return yy::parser::make_HAVING(loc);
(?i:exists)            return yy::parser::make_EXISTS(loc);
(?i:is)                return yy::parser::make_IS(loc);
(?i:null)              return yy::parser::make_NULL(loc);
(?i:not)               return yy::parser::make_NOT(loc);
(?i:and)               return yy::parser::make_AND(loc);
(?i:or)                return yy::parser::make_OR(loc);
(?i:cast)              return yy::parser::make_CAST(loc);
(?i:as)                return yy::parser::make_AS(loc);
"="                    return yy::parser::make_EQUAL(loc);
"!="                   return yy::parser::make_NOT_EQUAL(loc);
"<>"                   return yy::parser::make_NOT_EQUAL(loc);
"<"                    return yy::parser::make_LESS_THAN(loc);
"<="                   return yy::parser::make_LESS_THAN_OR_EQUAL_TO(loc);
">"                    return yy::parser::make_GREATER_THAN(loc);
">="                   return yy::parser::make_GREATER_THAN_OR_EQUAL_TO(loc);
","                    return yy::parser::make_COMMA(loc);
"("                    return yy::parser::make_PAREN_OPEN(loc);
")"                    return yy::parser::make_PAREN_CLOSE(loc);
[a-zA-Z][a-zA-Z0-9_]*  return yy::parser::make_IDENTIFIER(YYText(), loc);
[0-9]+                 return yy::parser::make_POSITIVE_INTEGER(YYText(), loc);
-?[0-9]+               return yy::parser::make_INTEGER(YYText(), loc);
.                      throw yy::parser::syntax_error{loc, fmt::format("invalid character: {}", YYText())};
<<EOF>>                return yy::parser::make_END_OF_INPUT(loc);

%%
