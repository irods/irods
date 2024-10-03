%{
    #include "irods/private/genquery1_driver.hpp"

    #include <fmt/format.h>

    #include <charconv>
    #include <cstdint>
    #include <string>
%}

%option c++ warn nodefault noyywrap nounput noinput batch
%option prefix="gq1"

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
                                throw gq1::parser::syntax_error{loc, fmt::format("invalid hex value: [{}]", YYText())};
                            }
                            else if (err.ec == std::errc::result_out_of_range) {
                                throw gq1::parser::syntax_error{loc, fmt::format("hex value cannot be represented: [{}]", YYText())};
                            }

                            string_literal.push_back(value);
                       }
    '                  { BEGIN(INITIAL); return gq1::parser::make_STRING_LITERAL(string_literal, loc); }
    .                  { string_literal += YYText(); }
    <<EOF>>            { throw gq1::parser::syntax_error{loc, fmt::format("missing closing single quote: [{}]", string_literal)}; }
}
[ \t\n]                ;
(?i:select)            return gq1::parser::make_SELECT(loc);
(?i:where)             return gq1::parser::make_WHERE(loc);
(?i:like)              return gq1::parser::make_LIKE(loc);
(?i:in)                return gq1::parser::make_IN(loc);
(?i:between)           return gq1::parser::make_BETWEEN(loc);
(?i:and)               return gq1::parser::make_AND(loc);
(?i:not)               return gq1::parser::make_NOT(loc);
(?i:begin_of)          return gq1::parser::make_BEGIN_OF(loc);
(?i:parent_of)         return gq1::parser::make_PARENT_OF(loc);
"="                    return gq1::parser::make_EQUAL(loc);
"!="                   return gq1::parser::make_NOT_EQUAL(loc);
"<>"                   return gq1::parser::make_NOT_EQUAL(loc);
"<"                    return gq1::parser::make_LESS_THAN(loc);
"<="                   return gq1::parser::make_LESS_THAN_OR_EQUAL_TO(loc);
">"                    return gq1::parser::make_GREATER_THAN(loc);
">="                   return gq1::parser::make_GREATER_THAN_OR_EQUAL_TO(loc);
","                    return gq1::parser::make_COMMA(loc);
"("                    return gq1::parser::make_PAREN_OPEN(loc);
")"                    return gq1::parser::make_PAREN_CLOSE(loc);
"||"                   return gq1::parser::make_DOUBLE_VERTICAL_BARS(loc);
"&&"                   return gq1::parser::make_DOUBLE_AMPERSANDS(loc);
[a-zA-Z][a-zA-Z0-9_]*  return gq1::parser::make_IDENTIFIER(YYText(), loc);
.                      throw gq1::parser::syntax_error{loc, fmt::format("invalid character: {}", YYText())};
<<EOF>>                return gq1::parser::make_END_OF_INPUT(loc);

%%
