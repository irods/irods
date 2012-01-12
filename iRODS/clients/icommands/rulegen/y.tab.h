/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LIT = 258,
     CHAR_LIT = 259,
     STR_LIT = 260,
     NUM_LIT = 261,
     Q_STR_LIT = 262,
     EQ_OP = 263,
     NE_OP = 264,
     AND_OP = 265,
     OR_OP = 266,
     LE_OP = 267,
     GE_OP = 268,
     ACRAC_SEP = 269,
     LIKE = 270,
     NOT = 271,
     PAREXP = 272,
     BRAC = 273,
     STLIST = 274,
     IF = 275,
     ELSE = 276,
     THEN = 277,
     WHILE = 278,
     FOR = 279,
     ASSIGN = 280,
     ASLIST = 281,
     TRUE = 282,
     FALSE = 283,
     ELSEIFELSEIF = 284,
     IFELSEIF = 285,
     DELAY = 286,
     REMOTE = 287,
     PARALLEL = 288,
     ONEOF = 289,
     SOMEOF = 290,
     FOREACH = 291,
     RLLIST = 292,
     RULE = 293,
     ACDEF = 294,
     ARGVAL = 295,
     AC_REAC = 296,
     REL_EXP = 297,
     EMPTYSTMT = 298,
     MICSER = 299,
     ON = 300,
     ONORLIST = 301,
     ORON = 302,
     OR = 303,
     ORONORLIST = 304,
     ORORLIST = 305,
     IFTHEN = 306,
     IFTHENELSE = 307,
     INPUT = 308,
     OUTPUT = 309,
     INPASS = 310,
     INPASSLIST = 311,
     OUTPASS = 312,
     OUTPASSLIST = 313
   };
#endif
/* Tokens.  */
#define LIT 258
#define CHAR_LIT 259
#define STR_LIT 260
#define NUM_LIT 261
#define Q_STR_LIT 262
#define EQ_OP 263
#define NE_OP 264
#define AND_OP 265
#define OR_OP 266
#define LE_OP 267
#define GE_OP 268
#define ACRAC_SEP 269
#define LIKE 270
#define NOT 271
#define PAREXP 272
#define BRAC 273
#define STLIST 274
#define IF 275
#define ELSE 276
#define THEN 277
#define WHILE 278
#define FOR 279
#define ASSIGN 280
#define ASLIST 281
#define TRUE 282
#define FALSE 283
#define ELSEIFELSEIF 284
#define IFELSEIF 285
#define DELAY 286
#define REMOTE 287
#define PARALLEL 288
#define ONEOF 289
#define SOMEOF 290
#define FOREACH 291
#define RLLIST 292
#define RULE 293
#define ACDEF 294
#define ARGVAL 295
#define AC_REAC 296
#define REL_EXP 297
#define EMPTYSTMT 298
#define MICSER 299
#define ON 300
#define ONORLIST 301
#define ORON 302
#define OR 303
#define ORONORLIST 304
#define ORORLIST 305
#define IFTHEN 306
#define IFTHENELSE 307
#define INPUT 308
#define OUTPUT 309
#define INPASS 310
#define INPASSLIST 311
#define OUTPASS 312
#define OUTPASSLIST 313




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 20 "rulegen.y"
{
        int i;
        long l;
        struct symbol *s;
        struct node *n;
}
/* Line 1529 of yacc.c.  */
#line 172 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

