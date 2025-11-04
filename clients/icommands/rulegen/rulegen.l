%{
#include "y.tab.h"
void count ();
%}

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
FS			(f|F|l|L)
IS			(u|U|l|L)*
EP			(e|E)(\+|-)?
%%

"/**"(.|\n)*"**/"          { count();  }
"delayExec"			{ count(); return(DELAY); }
"delay"			{ count(); return(DELAY); }
"remoteExec"			{ count(); return(REMOTE); }
"remote"			{ count(); return(REMOTE); }
"parallel"			{ count(); return(PARALLEL); }
"oneof"			{ count(); return(ONEOF); }
"someof"		{ count(); return(SOMEOF); }
"forEachExec"		{ count(); return(FOREACH); }
"forEach"		{ count(); return(FOREACH); }
"foreach"		{ count(); return(FOREACH); }
"like"			{ count(); return(LIKE); }
"not"			{ count(); return(NOT); }
"forExec"			{ count(); return(FOR); }
"for"			{ count(); return(FOR); }
"ifExec"			{ count(); return(IF); }
"if"			{ count(); return(IF); }
"then"			{ count(); return(THEN); }
"else"			{ count(); return(ELSE); }
"whileExec"			{ count(); return(WHILE); }
"while"			{ count(); return(WHILE); }
"on"			{ count(); return(ON); }
"or"			{ count(); return(OR); }
"oron"			{ count(); return(ORON); }
"DELAYEXEC"			{ count(); return(DELAY); }
"DELAY"			{ count(); return(DELAY); }
"REMOTEEXEC"			{ count(); return(REMOTE); }
"REMOTE"			{ count(); return(REMOTE); }
"PARALLEL"			{ count(); return(PARALLEL); }
"ONEOF"			{ count(); return(ONEOF); }
"SOMEOF"		{ count(); return(SOMEOF); }
"FOREACHEXEC"		{ count(); return(FOREACH); }
"FOREACH"		{ count(); return(FOREACH); }
"LIKE"			{ count(); return(LIKE); }
"NOT"			{ count(); return(NOT); }
"FOREXEC"			{ count(); return(FOR); }
"FOR"			{ count(); return(FOR); }
"IFEXEC"			{ count(); return(IF); }
"IF"			{ count(); return(IF); }
"THEN"			{ count(); return(THEN); }
"ELSE"			{ count(); return(ELSE); }
"WHILEEXEC"			{ count(); return(WHILE); }
"WHILE"			{ count(); return(WHILE); }
"ON"			{ count(); return(ON); }
"OR"			{ count(); return(OR); }
"ORON"			{ count(); return(ORON); }
"INPUT"			{ count(); return(INPUT); }
"OUTPUT"		{ count(); return(OUTPUT); }


\"([^"\n]|\\["\n])*\"	        { count(); return(Q_STR_LIT); }
{L}({L}|{D})*		{ count(); return(STR_LIT); }
[*$#/<>]{L}({L}|{D}|[-<>+\*_./])*	{ count(); return(STR_LIT); }
{D}*"."{D}+{FS}?	|
0[xX]{H}+{IS}?		|
0{D}+{IS}?		|
{D}+{IS}?		{ count();
			  return(NUM_LIT);}
"&&"			{ count(); return(yylval.i = AND_OP); }
"||"			{ count(); return(yylval.i = OR_OP); }
"<="			{ count(); return(yylval.i = LE_OP); }
">="			{ count(); return(yylval.i = GE_OP); }
"=="			{ count(); return(yylval.i = EQ_OP); }
"!="			{ count(); return(yylval.i = NE_OP); }
":::"			{ count(); return(yylval.i = ACRAC_SEP); }
";"			|
","			|
":"			|
"="			|
"("			|
")"			|
"["			|
"]"			|
"."			|
"&"			|
"!"			|
"~"			|
"-"			|
"+"			|
"*"			|
"/"			|
"%"			|
"<"			|
">"			|
"^"			|
"|"			|
"?"			{ count(); return yylval.i = *yytext; }

"{"			{ count();  return yylval.i = *yytext; }
"}"			{ count();  return yylval.i= *yytext; }

[ \t\n]			|
^#.*			{ count(); /* Ignore lines that start with '#' */ }
.			{ count();
			  yyerror("Unknown character (%c)", *yytext); }

%%
int column = 0;
int line_num = 1;
char error_line_buffer[256];


void count()
{
}
