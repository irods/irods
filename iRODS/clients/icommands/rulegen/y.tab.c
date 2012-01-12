/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "rulegen.y"

#include "rulegen.h"

#define	YYTRACE
#define	YYMAX_READ 0
#define	INTSIZE	long
extern char *yytext;
extern void *stitch(int typ, void *arg1, void *arg2, void *arg3, void *arg4); 
extern int stack_top;
extern FILE *outf;
extern char cutstr[10];
extern char nopstr[10];
char *stack[100];
char *pop_stack(char *stack[]);
int push_stack(char *stack[], char *item);
char *get_stack(char *stack[]);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 20 "rulegen.y"
{
        int i;
        long l;
        struct symbol *s;
        struct node *n;
}
/* Line 193 of yacc.c.  */
#line 238 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 251 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   326

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  82
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  30
/* YYNRULES -- Number of rules.  */
#define YYNRULES  81
/* YYNRULES -- Number of states.  */
#define YYNSTATES  172

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   313

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    31,     2,     2,     2,    20,    25,     2,
      29,    81,    18,    21,    17,    22,    28,    19,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    35,    37,
      23,    33,    24,    34,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    30,     2,     2,    26,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    36,    27,    80,    32,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    12,    15,    17,    20,    25,
      31,    33,    38,    40,    45,    47,    49,    53,    55,    57,
      59,    61,    62,    65,    69,    71,    74,    76,    78,    80,
      83,    86,    88,    94,   101,   107,   117,   124,   133,   139,
     142,   149,   153,   157,   159,   165,   173,   179,   182,   188,
     194,   198,   200,   204,   206,   208,   212,   216,   218,   222,
     224,   228,   232,   236,   240,   244,   246,   248,   250,   254,
     258,   262,   266,   270,   274,   278,   283,   285,   287,   289,
     291,   293
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      83,     0,    -1,    -1,    83,    86,    84,    85,    -1,    74,
     103,    -1,    75,   105,    -1,    87,    -1,    87,    86,    -1,
      88,    36,    93,    80,    -1,    88,    36,    93,    95,    80,
      -1,    90,    -1,    90,    29,    91,    81,    -1,    90,    -1,
      90,    29,    91,    81,    -1,   111,    -1,    92,    -1,    92,
      17,    91,    -1,     5,    -1,     7,    -1,     6,    -1,    97,
      -1,    -1,    36,    80,    -1,    36,    95,    80,    -1,    96,
      -1,    95,    96,    -1,    97,    -1,    98,    -1,    94,    -1,
     100,    37,    -1,   106,    37,    -1,   101,    -1,    66,    29,
     108,    81,    96,    -1,    66,    29,   108,    81,    96,    99,
      -1,    44,    29,   108,    81,    96,    -1,    45,    29,   107,
      37,   108,    37,   107,    81,    96,    -1,    41,    29,   108,
      81,    43,    96,    -1,    41,    29,   108,    81,    43,    96,
      42,    96,    -1,    68,    29,   108,    81,    96,    -1,    69,
      96,    -1,    68,    29,   108,    81,    96,    99,    -1,    69,
      96,    99,    -1,    89,    14,    89,    -1,    89,    -1,    52,
      29,   108,    81,    96,    -1,    53,    29,   111,    17,   108,
      81,    96,    -1,    54,    29,   108,    81,    96,    -1,    55,
      96,    -1,    56,    29,   111,    81,    96,    -1,    57,    29,
     111,    81,    96,    -1,   111,    33,   108,    -1,   102,    -1,
     102,    17,   103,    -1,    92,    -1,   104,    -1,   104,    17,
     105,    -1,   111,    33,   108,    -1,   106,    -1,   106,    17,
     107,    -1,   109,    -1,    29,   109,    81,    -1,   108,    10,
     108,    -1,   108,    11,   108,    -1,   108,    21,   108,    -1,
     108,    22,   108,    -1,    48,    -1,    49,    -1,   110,    -1,
     109,     8,   109,    -1,   109,     9,   109,    -1,   109,    23,
     109,    -1,   109,    24,   109,    -1,   109,    12,   109,    -1,
     109,    13,   109,    -1,   109,    15,   109,    -1,   109,    16,
      15,   109,    -1,     5,    -1,     6,    -1,     7,    -1,     5,
      -1,     7,    -1,     6,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    51,    51,    52,    56,    59,    63,    64,    67,    68,
      72,    73,    77,    78,    82,    86,    87,    90,    91,    92,
      96,    97,   100,   101,   105,   107,   112,   113,   114,   115,
     116,   117,   121,   122,   127,   129,   131,   133,   137,   138,
     139,   141,   146,   147,   151,   153,   155,   157,   159,   161,
     166,   169,   170,   175,   177,   178,   183,   186,   187,   192,
     193,   194,   195,   196,   197,   201,   202,   203,   204,   206,
     208,   210,   212,   214,   216,   218,   222,   223,   224,   227,
     228,   229
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LIT", "CHAR_LIT", "STR_LIT", "NUM_LIT",
  "Q_STR_LIT", "EQ_OP", "NE_OP", "AND_OP", "OR_OP", "LE_OP", "GE_OP",
  "ACRAC_SEP", "LIKE", "NOT", "','", "'*'", "'/'", "'%'", "'+'", "'-'",
  "'<'", "'>'", "'&'", "'^'", "'|'", "'.'", "'('", "'['", "'!'", "'~'",
  "'='", "'?'", "':'", "'{'", "';'", "PAREXP", "BRAC", "STLIST", "IF",
  "ELSE", "THEN", "WHILE", "FOR", "ASSIGN", "ASLIST", "TRUE", "FALSE",
  "ELSEIFELSEIF", "IFELSEIF", "DELAY", "REMOTE", "PARALLEL", "ONEOF",
  "SOMEOF", "FOREACH", "RLLIST", "RULE", "ACDEF", "ARGVAL", "AC_REAC",
  "REL_EXP", "EMPTYSTMT", "MICSER", "ON", "ONORLIST", "ORON", "OR",
  "ORONORLIST", "ORORLIST", "IFTHEN", "IFTHENELSE", "INPUT", "OUTPUT",
  "INPASS", "INPASSLIST", "OUTPASS", "OUTPASSLIST", "'}'", "')'",
  "$accept", "program", "inputs", "outputs", "rule_list", "rule",
  "action_def", "microserve", "action_name", "arg_list", "arg_val",
  "first_statement", "compound_statement", "statement_list", "statement",
  "selection_statement", "iteration_statement", "or_list_statement_list",
  "action_statement", "execution_statement", "inp_expr", "inp_expr_list",
  "out_expr", "out_expr_list", "ass_expr", "ass_expr_list", "cond_expr",
  "logical_expr", "relational_expr", "identifier", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,    44,    42,    47,
      37,    43,    45,    60,    62,    38,    94,   124,    46,    40,
      91,    33,   126,    61,    63,    58,   123,    59,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     125,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    82,    83,    83,    84,    85,    86,    86,    87,    87,
      88,    88,    89,    89,    90,    91,    91,    92,    92,    92,
      93,    93,    94,    94,    95,    95,    96,    96,    96,    96,
      96,    96,    97,    97,    98,    98,    98,    98,    99,    99,
      99,    99,   100,   100,   101,   101,   101,   101,   101,   101,
     102,   103,   103,   104,   105,   105,   106,   107,   107,   108,
     108,   108,   108,   108,   108,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   110,   110,   110,   111,
     111,   111
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     2,     2,     1,     2,     4,     5,
       1,     4,     1,     4,     1,     1,     3,     1,     1,     1,
       1,     0,     2,     3,     1,     2,     1,     1,     1,     2,
       2,     1,     5,     6,     5,     9,     6,     8,     5,     2,
       6,     3,     3,     1,     5,     7,     5,     2,     5,     5,
       3,     1,     3,     1,     1,     3,     3,     1,     3,     1,
       3,     3,     3,     3,     3,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     4,     1,     1,     1,     1,
       1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    79,    81,    80,     0,     6,     0,    10,
      14,     0,     0,     7,    21,     0,    51,     4,     0,     0,
       3,     0,     0,    20,    17,    19,    18,     0,    15,     0,
       0,    53,    54,     5,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,    43,    12,    28,     0,
      24,    26,    27,     0,    31,     0,    14,    11,     0,    52,
      76,    77,    78,     0,    65,    66,    50,    59,    67,     0,
       0,    22,     0,     0,     0,     0,     0,     0,     0,    47,
       0,     0,     0,     0,     9,    25,    29,    30,     0,    16,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    55,     0,    23,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,    42,     0,    56,    60,
      61,    62,    63,    64,    68,    69,    72,    73,    74,     0,
      70,    71,    32,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,    75,     0,     0,    33,     0,    34,    58,
       0,    44,     0,    46,    48,    49,     0,    39,    36,     0,
       0,     0,    41,     0,     0,    45,     0,    37,     0,    38,
      35,    40
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    12,    20,     6,     7,     8,    46,    47,    27,
      28,    22,    48,    49,    50,    51,    52,   146,    53,    54,
      16,    17,    32,    33,    55,   109,    66,    67,    68,    56
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -153
static const yytype_int16 yypact[] =
{
    -153,   202,  -153,  -153,  -153,  -153,   -60,   181,   -16,     1,
    -153,   181,   -39,  -153,   -26,   242,    28,  -153,    15,   242,
    -153,    21,    96,  -153,  -153,  -153,  -153,   -25,    55,   181,
     150,  -153,    56,  -153,   150,   137,    53,    54,    58,    61,
      62,    66,   260,    76,    78,  -153,    71,    79,  -153,   178,
    -153,  -153,  -153,    60,  -153,    73,    80,  -153,   242,  -153,
    -153,  -153,  -153,   111,  -153,  -153,    67,   230,  -153,   242,
      11,  -153,   219,   150,   150,   181,   150,   181,   150,  -153,
     181,   181,   181,   242,  -153,  -153,  -153,  -153,   150,  -153,
       3,   150,   150,   150,   150,   111,   111,   111,   111,   111,
     100,   111,   111,  -153,   260,  -153,    13,    31,   103,    84,
      80,    33,   106,    47,    43,    45,  -153,    52,    67,  -153,
      67,    67,    67,    67,   230,   230,   230,   230,   230,   111,
     230,   230,   -30,    95,   260,   181,   150,   260,   150,   260,
     260,   260,  -153,   230,   110,   260,  -153,   260,  -153,  -153,
     190,  -153,    49,  -153,  -153,  -153,   150,   -30,   112,   181,
     260,   125,  -153,   260,    64,  -153,   260,  -153,   260,   -30,
    -153,  -153
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -153,  -153,  -153,  -153,   154,  -153,  -153,    86,     2,   -54,
       6,  -153,  -153,   136,   -41,   158,  -153,  -152,  -153,  -153,
    -153,   145,  -153,   108,   -73,  -122,   -27,    68,  -153,    -1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      10,    79,   108,     9,    89,   162,    10,    70,    85,     9,
      18,    95,    96,   149,    11,    97,    98,   171,    99,   100,
      14,    91,    92,    91,    92,    31,   101,   102,    18,   117,
      15,    85,    93,    94,    93,    94,    19,   164,   144,   145,
      21,    91,    92,    91,    92,    29,   106,   107,    30,   111,
      34,   113,    93,    94,    93,    94,    57,    91,    92,    91,
      92,   118,   108,   132,   120,   121,   122,   123,    93,    94,
      93,    94,    58,    69,   110,    31,   112,    91,    92,   114,
     115,    10,    73,    74,   119,    82,   108,    75,    93,    94,
      76,    77,   104,   148,   133,    78,   151,    86,   153,   154,
     155,     3,     4,     5,   157,    80,   158,    81,    83,   150,
      87,   152,   134,    88,   137,   129,    60,    61,    62,   165,
     135,   136,   167,   138,   140,   169,   141,   170,   139,   161,
     160,    90,    35,   142,   110,    91,    92,    36,   147,   156,
      37,    38,     3,     4,     5,   168,    93,    94,    39,    40,
      41,    42,    43,    44,   163,    60,    61,    62,   110,    64,
      65,    13,    21,   124,   125,   126,   127,   128,   116,   130,
     131,    72,    23,    35,    59,     0,    45,   103,    36,    63,
       0,    37,    38,     3,     4,     5,     3,     4,     5,    39,
      40,    41,    42,    43,    44,     0,     0,   143,    64,    65,
      91,    92,     2,    21,     0,     0,   166,     3,     4,     5,
       0,    93,    94,     0,    35,     0,     0,    71,     0,    36,
       0,     0,    37,    38,     3,     4,     5,   159,     0,     0,
      39,    40,    41,    42,    43,    44,     0,     0,    95,    96,
       0,     0,    97,    98,    21,    99,   100,    24,    25,    26,
       0,     0,     0,   101,   102,    35,     0,     0,    84,     0,
      36,     0,     0,    37,    38,     3,     4,     5,     0,     0,
       0,    39,    40,    41,    42,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    35,     0,     0,   105,
       0,    36,     0,     0,    37,    38,     0,     0,     0,     0,
       0,     0,    39,    40,    41,    42,    43,    44,     0,     0,
       0,     0,     0,     0,     0,     0,    21
};

static const yytype_int16 yycheck[] =
{
       1,    42,    75,     1,    58,   157,     7,    34,    49,     7,
      11,     8,     9,   135,    74,    12,    13,   169,    15,    16,
      36,    10,    11,    10,    11,    19,    23,    24,    29,    83,
      29,    72,    21,    22,    21,    22,    75,   159,    68,    69,
      66,    10,    11,    10,    11,    17,    73,    74,    33,    76,
      29,    78,    21,    22,    21,    22,    81,    10,    11,    10,
      11,    88,   135,   104,    91,    92,    93,    94,    21,    22,
      21,    22,    17,    17,    75,    69,    77,    10,    11,    80,
      81,    82,    29,    29,    81,    14,   159,    29,    21,    22,
      29,    29,    81,   134,    81,    29,   137,    37,   139,   140,
     141,     5,     6,     7,   145,    29,   147,    29,    29,   136,
      37,   138,    81,    33,    81,    15,     5,     6,     7,   160,
      17,    37,   163,    17,    81,   166,    81,   168,    81,   156,
      81,    63,    36,    81,   135,    10,    11,    41,    43,    29,
      44,    45,     5,     6,     7,    81,    21,    22,    52,    53,
      54,    55,    56,    57,    42,     5,     6,     7,   159,    48,
      49,     7,    66,    95,    96,    97,    98,    99,    82,   101,
     102,    35,    14,    36,    29,    -1,    80,    69,    41,    29,
      -1,    44,    45,     5,     6,     7,     5,     6,     7,    52,
      53,    54,    55,    56,    57,    -1,    -1,   129,    48,    49,
      10,    11,     0,    66,    -1,    -1,    81,     5,     6,     7,
      -1,    21,    22,    -1,    36,    -1,    -1,    80,    -1,    41,
      -1,    -1,    44,    45,     5,     6,     7,    37,    -1,    -1,
      52,    53,    54,    55,    56,    57,    -1,    -1,     8,     9,
      -1,    -1,    12,    13,    66,    15,    16,     5,     6,     7,
      -1,    -1,    -1,    23,    24,    36,    -1,    -1,    80,    -1,
      41,    -1,    -1,    44,    45,     5,     6,     7,    -1,    -1,
      -1,    52,    53,    54,    55,    56,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    80,
      -1,    41,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    66
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    83,     0,     5,     6,     7,    86,    87,    88,    90,
     111,    74,    84,    86,    36,    29,   102,   103,   111,    75,
      85,    66,    93,    97,     5,     6,     7,    91,    92,    17,
      33,    92,   104,   105,    29,    36,    41,    44,    45,    52,
      53,    54,    55,    56,    57,    80,    89,    90,    94,    95,
      96,    97,    98,   100,   101,   106,   111,    81,    17,   103,
       5,     6,     7,    29,    48,    49,   108,   109,   110,    17,
     108,    80,    95,    29,    29,    29,    29,    29,    29,    96,
      29,    29,    14,    29,    80,    96,    37,    37,    33,    91,
     109,    10,    11,    21,    22,     8,     9,    12,    13,    15,
      16,    23,    24,   105,    81,    80,   108,   108,   106,   107,
     111,   108,   111,   108,   111,   111,    89,    91,   108,    81,
     108,   108,   108,   108,   109,   109,   109,   109,   109,    15,
     109,   109,    96,    81,    81,    17,    37,    81,    17,    81,
      81,    81,    81,   109,    68,    69,    99,    43,    96,   107,
     108,    96,   108,    96,    96,    96,    29,    96,    96,    37,
      81,   108,    99,    42,   107,    96,    81,    96,    81,    96,
      96,    99
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 51 "rulegen.y"
    { }
    break;

  case 3:
#line 52 "rulegen.y"
    { print_final((yyvsp[(2) - (4)].s),(yyvsp[(3) - (4)].s),(yyvsp[(4) - (4)].s)); }
    break;

  case 4:
#line 56 "rulegen.y"
    { (yyval.s)=(yyvsp[(2) - (2)].s); }
    break;

  case 5:
#line 59 "rulegen.y"
    { (yyval.s)=(yyvsp[(2) - (2)].s); }
    break;

  case 6:
#line 63 "rulegen.y"
    { (yyval.s)=stitch(RLLIST,(yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 7:
#line 64 "rulegen.y"
    { (yyval.s)=stitch(RLLIST,(yyvsp[(1) - (2)].s),(yyvsp[(2) - (2)].s),NULL,NULL); }
    break;

  case 8:
#line 67 "rulegen.y"
    { (yyval.s)=stitch(RULE,(yyvsp[(1) - (4)].s),(yyvsp[(3) - (4)].s),NULL,NULL); }
    break;

  case 9:
#line 68 "rulegen.y"
    { (yyval.s)=stitch(RULE,(yyvsp[(1) - (5)].s),(yyvsp[(3) - (5)].s),(yyvsp[(4) - (5)].s),NULL); }
    break;

  case 10:
#line 72 "rulegen.y"
    {(yyval.s)=stitch(ACDEF, (yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 11:
#line 73 "rulegen.y"
    {(yyval.s)=stitch(ACDEF, (yyvsp[(1) - (4)].s),(yyvsp[(3) - (4)].s),NULL,NULL); }
    break;

  case 12:
#line 77 "rulegen.y"
    {(yyval.s)=stitch(MICSER, (yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 13:
#line 78 "rulegen.y"
    {(yyval.s)=stitch(MICSER, (yyvsp[(1) - (4)].s),(yyvsp[(3) - (4)].s),NULL,NULL); }
    break;

  case 15:
#line 86 "rulegen.y"
    { (yyval.s)=stitch (ARGVAL, (yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 16:
#line 87 "rulegen.y"
    { (yyval.s)=stitch (ARGVAL, (yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 17:
#line 90 "rulegen.y"
    { (yyval.s)=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 18:
#line 91 "rulegen.y"
    { (yyval.s)=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 19:
#line 92 "rulegen.y"
    { (yyval.s)=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 20:
#line 96 "rulegen.y"
    { (yyval.s)=stitch(STLIST,(yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 21:
#line 97 "rulegen.y"
    { (yyval.s)=stitch (EMPTYSTMT,NULL,NULL,NULL,NULL); }
    break;

  case 22:
#line 100 "rulegen.y"
    { (yyval.s)=stitch(BRAC,NULL,NULL,NULL,NULL); }
    break;

  case 23:
#line 101 "rulegen.y"
    { (yyval.s)=stitch(BRAC,(yyvsp[(2) - (3)].s),NULL,NULL,NULL); }
    break;

  case 24:
#line 106 "rulegen.y"
    { (yyval.s)=stitch(STLIST,(yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 25:
#line 108 "rulegen.y"
    { (yyval.s)=stitch(STLIST,(yyvsp[(1) - (2)].s),(yyvsp[(2) - (2)].s),NULL,NULL); }
    break;

  case 29:
#line 115 "rulegen.y"
    {}
    break;

  case 30:
#line 116 "rulegen.y"
    {}
    break;

  case 32:
#line 121 "rulegen.y"
    { (yyval.s)=stitch(ON,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 33:
#line 123 "rulegen.y"
    { (yyval.s)=stitch(ONORLIST,(yyvsp[(3) - (6)].s),(yyvsp[(5) - (6)].s),(yyvsp[(6) - (6)].s),NULL); }
    break;

  case 34:
#line 128 "rulegen.y"
    { (yyval.s)=stitch(WHILE,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 35:
#line 130 "rulegen.y"
    { (yyval.s)=stitch(FOR,(yyvsp[(3) - (9)].s),(yyvsp[(5) - (9)].s),(yyvsp[(7) - (9)].s),(yyvsp[(9) - (9)].s)); }
    break;

  case 36:
#line 132 "rulegen.y"
    { (yyval.s)=stitch(IFTHEN,(yyvsp[(3) - (6)].s),(yyvsp[(6) - (6)].s),NULL,NULL); }
    break;

  case 37:
#line 134 "rulegen.y"
    { (yyval.s)=stitch(IFTHENELSE,(yyvsp[(3) - (8)].s),(yyvsp[(6) - (8)].s),(yyvsp[(8) - (8)].s),NULL); }
    break;

  case 38:
#line 137 "rulegen.y"
    { (yyval.s)=stitch(ORON,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 39:
#line 138 "rulegen.y"
    { (yyval.s)=stitch(OR,(yyvsp[(2) - (2)].s),NULL,NULL,NULL); }
    break;

  case 40:
#line 140 "rulegen.y"
    { (yyval.s)=stitch(ORONORLIST,(yyvsp[(3) - (6)].s),(yyvsp[(5) - (6)].s),(yyvsp[(6) - (6)].s),NULL); }
    break;

  case 41:
#line 142 "rulegen.y"
    { (yyval.s)=stitch(ORORLIST,(yyvsp[(2) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 42:
#line 146 "rulegen.y"
    { (yyval.s)=stitch(AC_REAC,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 43:
#line 147 "rulegen.y"
    { (yyval.s)=stitch(AC_REAC,(yyvsp[(1) - (1)].s),NULL,NULL,NULL); }
    break;

  case 44:
#line 152 "rulegen.y"
    { (yyval.s)=stitch(DELAY,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 45:
#line 154 "rulegen.y"
    { (yyval.s)=stitch(REMOTE,(yyvsp[(3) - (7)].s),(yyvsp[(5) - (7)].s),(yyvsp[(7) - (7)].s),NULL); }
    break;

  case 46:
#line 156 "rulegen.y"
    { (yyval.s)=stitch(PARALLEL,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 47:
#line 158 "rulegen.y"
    { (yyval.s)=stitch(ONEOF,(yyvsp[(2) - (2)].s),NULL,NULL,NULL); }
    break;

  case 48:
#line 160 "rulegen.y"
    { (yyval.s)=stitch(SOMEOF,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 49:
#line 162 "rulegen.y"
    { (yyval.s)=stitch(FOREACH,(yyvsp[(3) - (5)].s),(yyvsp[(5) - (5)].s),NULL,NULL); }
    break;

  case 50:
#line 166 "rulegen.y"
    { (yyval.s)=stitch(INPASS,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 52:
#line 171 "rulegen.y"
    { (yyval.s)=stitch(INPASSLIST,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 55:
#line 179 "rulegen.y"
    { (yyval.s)=stitch(OUTPASSLIST,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 56:
#line 183 "rulegen.y"
    { (yyval.s)=stitch(ASSIGN,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 58:
#line 188 "rulegen.y"
    { (yyval.s)=stitch(ASLIST,(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 60:
#line 193 "rulegen.y"
    { (yyval.s)=stitch(PAREXP, (yyvsp[(2) - (3)].s),NULL,NULL,NULL); }
    break;

  case 61:
#line 194 "rulegen.y"
    { (yyval.s)=stitch((yyvsp[(2) - (3)].i),(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 62:
#line 195 "rulegen.y"
    { (yyval.s)=stitch((yyvsp[(2) - (3)].i),(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 63:
#line 196 "rulegen.y"
    { (yyval.s)=stitch((yyvsp[(2) - (3)].i),(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 64:
#line 197 "rulegen.y"
    { (yyval.s)=stitch((yyvsp[(2) - (3)].i),(yyvsp[(1) - (3)].s),(yyvsp[(3) - (3)].s),NULL,NULL); }
    break;

  case 65:
#line 201 "rulegen.y"
    {(yyval.s)=stitch (TRUE,NULL,NULL,NULL,NULL); }
    break;

  case 66:
#line 202 "rulegen.y"
    {(yyval.s)=stitch (FALSE,NULL,NULL,NULL,NULL); }
    break;

  case 68:
#line 205 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),"==",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 69:
#line 207 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),"!=",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 70:
#line 209 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),"<",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 71:
#line 211 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),">",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 72:
#line 213 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),"<=",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 73:
#line 215 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),">=",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 74:
#line 217 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (3)].s),"like",(yyvsp[(3) - (3)].s),NULL); }
    break;

  case 75:
#line 219 "rulegen.y"
    { (yyval.s)=stitch (REL_EXP,(yyvsp[(1) - (4)].s),"not like",(yyvsp[(4) - (4)].s),NULL); }
    break;

  case 76:
#line 222 "rulegen.y"
    { (yyval.s)=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 77:
#line 223 "rulegen.y"
    { (yyval.s)=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 78:
#line 224 "rulegen.y"
    { (yyval.s)=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 79:
#line 227 "rulegen.y"
    { (yyval.s)=stitch (STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 80:
#line 228 "rulegen.y"
    { (yyval.s)=stitch (Q_STR_LIT,yytext,NULL,NULL,NULL); }
    break;

  case 81:
#line 229 "rulegen.y"
    { (yyval.s)=stitch (NUM_LIT,yytext,NULL,NULL,NULL); }
    break;


/* Line 1267 of yacc.c.  */
#line 1985 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 231 "rulegen.y"



void *stitch(int typ, void *inarg1, void  *inarg2, void  *inarg3, void  *inarg4)
{

  int i,j,k;
  char tmpStr[10000];
  char *s, *t;
  char *u, *v;
  char *arg1, *arg2, *arg3, *arg4;

  arg1 = (char *) inarg1;
  arg2 = (char *) inarg2;
  arg3 = (char *) inarg3;
  arg4 = (char *) inarg4;

  switch (typ) {
  case BRAC:
    return(arg1);
    break;
  case STLIST:
    if (arg2 == NULL)
      return(arg1);
    else {
      if ((t = strstr(arg1,":::")) != NULL) {
	*t = '\0';
	if ((s = strstr(arg2,":::")) != NULL) {
	  *s = '\0';
	  sprintf(tmpStr,"%s##%s%s%s##%s", 
		  arg1, arg2, ":::", (char *) t + strlen(":::"), 
		  (char *) s + strlen(":::"));
	}
	else {
	  sprintf(tmpStr,"%s##%s%s%s", 
		  arg1, arg2, ":::", (char *) t + strlen(":::"));
	}
      }
      else {
	sprintf(tmpStr,"%s##%s", arg1, arg2);
      }
    }
    break;
  case ASSIGN:
    sprintf(tmpStr,"assign(%s,%s):::nop", arg1, arg2);
    break;
  case INPASS:
    stripEndQuotes(arg2);
    sprintf(tmpStr,"%s=%s", arg1, arg2);
    break;
  case INPASSLIST:
    sprintf(tmpStr,"%s%%%s", arg1, arg2);
    break;
  case OUTPASSLIST:
    sprintf(tmpStr,"%s%%%s", arg1, arg2);
    break;
  case PAREXP:
    sprintf(tmpStr,"(%s)", arg1);
    break;
  case ORON:
    sprintf(tmpStr,"%s|%s%s",arg1,cutstr,arg2);
    break;
  case OR:
    sprintf(tmpStr,"|%s%s",cutstr,arg1);
    break;
  case ORONORLIST:
    sprintf(tmpStr,"%s|%s%s;;;%s",arg1,cutstr,arg2,arg3);
    break;
  case ORORLIST:
    sprintf(tmpStr,"|%s%s;;;%s",cutstr,arg1,arg2);
    break;
  case ON:
    sprintf(tmpStr,"%s|%s%s",  arg1, cutstr,arg2);
    break;
  case ONORLIST:
    sprintf(tmpStr,"%s|%s%s;;;%s",arg1,cutstr,arg2,arg3);
    break;
  case WHILE:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"whileExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"whileExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case IFTHEN:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"ifExec(%s,%s,%s,nop,nop):::nop", arg1,arg2, (char *) t + strlen(":::")); 
      *t = ':';
    }
    else {
      sprintf(tmpStr,"ifExec(%s,%s,nop,nop,nop):::nop", arg1,arg2);
    }
    break;
  case IFTHENELSE:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      if ((s = strstr(arg3,":::")) != NULL) {
	*s = '\0';
	sprintf(tmpStr,"ifExec(%s,%s,%s,%s,%s):::nop", arg1,arg2,(char *) t + strlen(":::"),
		arg3, (char *) s + strlen(":::"));
	*s = ':';
      }
      else {
	sprintf(tmpStr,"ifExec(%s,%s,%s,%s,nop):::nop", arg1,arg2, (char *) t + strlen(":::"), arg3); 
      }
      *t = ':';
    }
    else {
      if ((s = strstr(arg3,":::")) != NULL) {
	*s = '\0';
	printf(tmpStr,"ifExec(%s,%s,nop,%s,%s):::nop", arg1,arg2, arg3, (char *) s + strlen(":::"));
	*s = ':';
      }
      else {
	sprintf(tmpStr,"ifExec(%s,%s,nop,%s,nop):::nop", arg1,arg2,arg3);
      }
    }
    break;
  case DELAY:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"delayExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"delayExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case REMOTE:
    if ((t = strstr(arg3,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"remoteExec(%s,%s,%s,%s):::nop", arg1, arg2, arg3,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"remoteExec(%s,%s,%s,%s):::nop", arg1, arg2, arg3,"''");
    }
    break;
  case PARALLEL:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"parallelExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"parallelExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case SOMEOF:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"someOfExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"someOfExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case ONEOF:
    if ((t = strstr(arg1,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"oneOfExec(%s,%s):::nop", arg1,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"oneOfExec(%s,%s):::nop", arg1, "''");
    }
    break;
  case FOREACH:
    if ((t = strstr(arg2,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"forEachExec(%s,%s,%s):::nop", arg1, arg2,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"forEachExec(%s,%s,%s):::nop", arg1, arg2,"''");
    }
    break;
  case FOR:
    if ((u =  strstr(arg1,":::"))!= NULL) *u = '\0';
    if ((v =  strstr(arg3,":::"))!= NULL) *v = '\0';
    if ((t = strstr(arg4,":::")) != NULL) {
      *t = '\0';
      sprintf(tmpStr,"forExec(%s,%s,%s,%s,%s):::nop", 
	      arg1, arg2,arg3,arg4,(char *) t + strlen(":::"));
      *t = ':';
    }
    else {
      sprintf(tmpStr,"forExec(%s,%s,%s,%s,%s):::nop", 
	      arg1, arg2,arg3,arg4,"''");
    }
    if (u != NULL) *u = ':';
    if (v != NULL) *v = ':';
    break;
  case ASLIST:
    break;
  case RLLIST:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s\n%s", arg1, arg2);
    break;
  case RULE:
    if (yydebug == 1)
      if (arg3 == NULL)
	printf("BBB:%s:%s:NOARG3\n",arg1, arg2);
      else
	printf("BBB:%s:%s:%s\n",arg1, arg2, arg3);
    u = arg2;
    strcpy(tmpStr,"");
    while (u != NULL) {
      v = strstr(u,";;;");
      if (v != NULL)
	*v = '\0';
      if (arg3 == NULL) {
	if (!strcmp(u," ")) 
	  return(arg1);
	else {
	  if ((t = strstr(u,":::")) != NULL) {
	    *t = '\0';
	    sprintf(tmpStr,"%s%s|%s|%s%s",tmpStr, arg1, u,nopstr, (char *) t + strlen(":::"));
	  }
	  else {
	    sprintf(tmpStr,"%s%s|%s",tmpStr, arg1,u); 
	  }
	}
      }
      else { 
	if (!strcmp(u," ")) {
	  if ((t = strstr(arg3,":::")) != NULL) {
	    *t = '\0';
	    sprintf(tmpStr,"%s%s||%s|%s%s",tmpStr, arg1, arg3,nopstr, (char *) t + strlen(":::"));
	  }
	  else 
	    sprintf(tmpStr,"%s%s||%s",tmpStr, arg1, arg3);
	}
	else {
	  if ((t = strstr(u,":::")) != NULL) {
	    *t = '\0';
	    if ((s = strstr(arg3,":::")) != NULL) {
	      *s = '\0';
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s##%s ",tmpStr, 
		      arg1, u, arg3,  nopstr,  (char *) t + strlen(":::"), 
		      (char *) s + strlen(":::"));
	    }
	    else {
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s",tmpStr, 
		      arg1, u, arg3,   nopstr, (char *) t + strlen(":::"));
	    } 
	  }
	  else {
	    if ((s = strstr(arg3,":::")) != NULL) {
	      *s = '\0';
	      sprintf(tmpStr,"%s%s|%s##%s|%s%s",tmpStr,
		      arg1, u, arg3,  nopstr,(char *) s + strlen(":::"));
	    }
	    else
	      sprintf(tmpStr,"%s%s|%s##%s",tmpStr, arg1, u, arg3);
	  }
	}
      }
      if (v == NULL)
	break;
      *v = ';';
      u = v + 3;
      strcat(tmpStr,";;;");
    }
    pop_stack(stack);
    break;
  case ACDEF:
    if (arg2 == NULL)
      sprintf(tmpStr,"%s", arg1);
    else 
      sprintf(tmpStr,"%s(%s)", arg1, arg2);
    if (push_stack(stack, tmpStr) < 0) {
          printf("Stack OverFlow for Rule Depth");
	  exit(1);
    }
    break;
  case MICSER:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s(%s)", arg1, arg2);
    break;
  case ARGVAL:
    if (arg2 == NULL)
      return(arg1);
    else 
      sprintf(tmpStr,"%s,%s", arg1, arg2);
    break;
  case AC_REAC:
    if (arg2 == NULL)
      sprintf(tmpStr,"%s:::nop", arg1);
    else 
      sprintf(tmpStr,"%s:::%s", arg1, arg2);
    break;
  case REL_EXP:
    sprintf(tmpStr,"%s %s %s",  arg1, arg2, arg3);
    break;
  case TRUE:
    sprintf(tmpStr,"%d",1);
    break;
  case FALSE:
    sprintf(tmpStr,"%d",0);
    break;
  case STR_LIT:
    sprintf(tmpStr,"%s", arg1);
    break;
  case Q_STR_LIT:
    sprintf(tmpStr,"\"%s\"", arg1);
    stripEscFromQuotedStr(tmpStr);
    break;
  case NUM_LIT:
    sprintf(tmpStr,"%s", arg1);
    break;
  case EMPTYSTMT:
       strcpy(tmpStr," "); 
    /* strcpy(tmpStr,"cut");*/
    /* strcpy(tmpStr,"|");*/
    break;
  case AND_OP:
    sprintf(tmpStr,"%s %s %s", arg1, "&&", arg2);
    break;
  case OR_OP:
    sprintf(tmpStr,"%s %s %s", arg1, "!!", arg2);
    break;
  case '+':
    sprintf(tmpStr,"%s %s %s", arg1, "+", arg2);
    break;
  case '-':
    sprintf(tmpStr,"%s %s %s", arg1, "-", arg2);
    break;
  default:
    printf("Error in stictch typ: %i", typ);
    exit(1);
    break;
  }
  s = strdup(tmpStr);
  if (yydebug == 1)
    printf("AAA:%i: %s\n",typ,tmpStr);
  return(s);

}

int
print_final ( char *out, char* input, char *output)
{
  
  char *s, *t;

  s = out;
  while ((t = strstr(s, ";;;")) != NULL) {
    *t = '\0';
    fprintf(outf,"%s\n",s);
    *t = ';';
    s = t+3;
  }
 fprintf(outf,"%s\n",s);
 fprintf(outf,"%s\n%s\n", input, output);
 return(0);
}

int 
stripEndQuotes(char *s)
{
  char *t;

  if (*s == '\'' ||*s == '"' ) {
    memmove(s,s+1,strlen(s+1)+1);
    t = s+strlen(s)-1;
    if (*t == '\'' ||*t == '"' )
      *t = '\0';
  }
  /* made it so that end quotes are removed only if quoted initially */
  return (0);
}

int
stripEscFromQuotedStr(char *str)
{
 char *s, *t, *u;
 s = strdup(str + 1);
 t = s;
 u = str;
 while (*t != '\0') {
   if (*t == '\\' && (*(t+1) == '"' || *(t+1) == '\\')) {
     t++;
   }
   *u = *t;
   u++;
   t++;
 }
 u--;
 while (*u != '"')
   u--;
 *u = '\0';
}


