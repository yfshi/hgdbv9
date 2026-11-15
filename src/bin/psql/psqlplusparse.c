/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         psqlplus_yyparse
#define yylex           psqlplus_yylex
#define yyerror         psqlplus_yyerror
#define yydebug         psqlplus_yydebug
#define yynerrs         psqlplus_yynerrs


/* Copy the first part of user declarations.  */
#line 1 "psqlplusparse.y" /* yacc.c:339  */

/*-------------------------------------------------------------------------
 *
 * File: psqlplusparse.y
 *
 * Abstract:
 *		bison grammar for a oracle-compatible client commands.
 *
 * Authored by zhenmingyang@highgo.com, 20221221.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/bin/psql/psqlplusparse.y
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "common/logging.h"
#include "fe_utils/psqlscan_int.h"
#include "oracle_fe_utils/ora_psqlscan.h"
#include "psqlplus.h"
#include "settings.h"

static psqlplus_cmd_var *make_variable_node(void);
static psqlplus_cmd_print *make_print_node(void);
static psqlplus_cmd_execute *make_exec_node(void);
/* Begin - ReqID:SRS-SQL-SYSFUNC */
static psqlplus_cmd_sv *make_set_node(void);
static psqlplus_cmd_sv *make_show_node(void);
static sys_var *make_system_variable(const char *name);
static sys_var_list *sv_list_make1(sys_var *sv);
static sys_var_list *sv_list_append(sys_var_list *list, sys_var *sv);
/* End - ReqID:SRS-SQL-SYSFUNC */
static BindVarType *make_bindvartype(int32 oid, int32 typmod, const char *typname);
static char *get_guc_settings(const char *gucname);
static print_item *make_print_item(char *name, bool valid, bool need_downcase/* BUG#Z351 */);
static print_list *make_print_list1(print_item *item);
static print_list *merge_print_list(print_list *list1, print_list *list2);


#line 116 "psqlplusparse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int psqlplus_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    K_VAR = 258,
    K_VARIABLE = 259,
    K_CHAR = 260,
    K_BYTE = 261,
    K_NCHAR = 262,
    K_VARCHAR2 = 263,
    K_NVARCHAR2 = 264,
    K_NUMBER = 265,
    K_BINARY_FLOAT = 266,
    K_BINARY_DOUBLE = 267,
    K_PRINT = 268,
    K_EXECUTE = 269,
    K_EXEC = 270,
    K_REFCURSOR = 271,
    IDENT = 272,
    FCONST = 273,
    LITERAL = 274,
    DQCONST = 275,
    SQCONST = 276,
    ICONST = 277,
    SINGLE_QUOTE_NO_END = 278,
    DOUBLE_QUOTE_NO_END = 279,
    TYPECAST = 280,
    K_OFF = 281,
    K_ON = 282,
    K_SET = 283,
    K_SHOW = 284,
    K_SERVEROUT = 285,
    K_SERVEROUTP = 286,
    K_SERVEROUTPU = 287,
    K_SERVEROUTPUT = 288,
    K_SIZE = 289,
    K_UNL = 290,
    K_UNLIMITED = 291
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 53 "psqlplusparse.y" /* yacc.c:355  */

	psql_YYSTYPE		psql_yysype;
	int					ival;
	char				*str;
	const char			*keyword;
	psqlplus_cmd		*psqlplus_cmd;
	BindVarType			*bindvartype;
	print_list			*print_item_list;
	/* Begin - ReqID:SRS-SQL-SYSFUNC */
	bool				boolean;
	sys_var				*system_variable;
	sys_var_list		*system_variable_list;
	/* End - ReqID:SRS-SQL-SYSFUNC */

#line 205 "psqlplusparse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int psqlplus_yyparse (yyscan_t yyscanner);



/* Copy the second part of user declarations.  */

#line 221 "psqlplusparse.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   172

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  111
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  136

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   291

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      40,    41,     2,    38,    42,    39,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    43,
       2,    37,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   101,   101,   105,   109,   114,   125,   129,   148,   149,
     153,   161,   201,   214,   222,   236,   250,   259,   300,   315,
     325,   342,   360,   369,   372,   375,   376,   380,   381,   382,
     383,   384,   385,   386,   387,   388,   393,   405,   409,   416,
     420,   424,   428,   432,   436,   443,   447,   451,   455,   459,
     463,   468,   473,   480,   481,   482,   489,   493,   502,   506,
     513,   517,   521,   530,   539,   559,   560,   565,   573,   585,
     586,   587,   588,   589,   594,   598,   605,   609,   616,   623,
     630,   637,   644,   652,   668,   669,   670,   671,   685,   686,
     687,   688,   693,   694,   704,   705,   709,   710,   714,   715,
     716,   717,   718,   719,   720,   721,   722,   723,   724,   725,
     726,   727
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "K_VAR", "K_VARIABLE", "K_CHAR",
  "K_BYTE", "K_NCHAR", "K_VARCHAR2", "K_NVARCHAR2", "K_NUMBER",
  "K_BINARY_FLOAT", "K_BINARY_DOUBLE", "K_PRINT", "K_EXECUTE", "K_EXEC",
  "K_REFCURSOR", "IDENT", "FCONST", "LITERAL", "DQCONST", "SQCONST",
  "ICONST", "SINGLE_QUOTE_NO_END", "DOUBLE_QUOTE_NO_END", "TYPECAST",
  "K_OFF", "K_ON", "K_SET", "K_SHOW", "K_SERVEROUT", "K_SERVEROUTP",
  "K_SERVEROUTPU", "K_SERVEROUTPUT", "K_SIZE", "K_UNL", "K_UNLIMITED",
  "'='", "'+'", "'-'", "'('", "')'", "','", "';'", "$accept",
  "psqlplus_toplevel_stmt", "variable_stmt", "variable_keyword",
  "variable_spec", "opt_varname", "bind_varname", "bind_varvalue",
  "bind_varvalue_sconst", "bind_vartype", "truncate_char", "print_stmt",
  "print_items_list", "print_items", "exec_stmt", "exec_keyword",
  "system_variable_stmt", "show_system_variable",
  "show_system_variable_list", "set_system_variable_list",
  "set_system_variable", "serveroutput_keyword", "opt_serveroutput_size",
  "system_variable_on_off", "Sconst", "opt_semi", "unreserved_keyword", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,    61,    43,    45,
      40,    41,    44,    59
};
# endif

#define YYPACT_NINF -19

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-19)))

#define YYTABLE_NINF -23

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      36,   -19,   -19,    74,   -19,   -19,    37,    41,    23,   -18,
       1,   -18,   -18,   -19,   -18,   -19,   -19,   -19,   -19,   -19,
     -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,
     -19,   -19,    74,   -19,   -19,    21,   -19,   -19,   -19,   -19,
      37,   -19,    26,   -19,   -19,   -19,   -19,   -19,   -19,    41,
     -19,   -19,   -19,   -19,   -19,    85,    19,   -19,   -19,   -19,
     -19,   -19,   -19,   -19,   -19,   -19,   -19,    15,    24,   -19,
     -19,   -19,    11,   -19,   -19,   -19,   -19,   -19,    35,    52,
      56,    83,   -19,   -19,   -19,   -19,    96,    18,   -19,   -19,
      -2,   -19,    39,    44,   106,   107,   -19,   -19,   -19,   -19,
     -19,   -19,    14,    75,   -19,   -19,   -19,   -19,   133,   -19,
     -19,   -19,    -3,    22,    16,    89,   -19,   -19,   -19,   -19,
     -19,   -19,   -19,   -19,    90,    91,   -19,   -19,    92,   117,
     -19,   -19,   -19,   -19,   -19,   -19
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     9,     8,    56,    65,    66,     0,     0,     0,    97,
      24,    97,    97,    64,    97,   109,   111,   102,   101,   105,
     110,   107,   106,    99,    98,   108,   104,   103,   100,    60,
      62,    63,    57,    58,    61,     0,    84,    85,    86,    87,
      68,    76,     0,    69,    70,    71,    72,    73,    74,    67,
       1,    96,     2,    25,     6,     0,    23,    26,     3,     4,
       5,    59,    95,    94,    80,    93,    92,     0,     0,    78,
      79,    77,    91,    75,    53,    54,    55,     7,    37,    41,
      43,    47,    49,    50,    52,    51,    13,    16,    81,    82,
       0,    83,     0,     0,     0,     0,    28,    31,    27,    34,
      14,    15,     0,     0,    11,    12,    36,    35,    19,    88,
      89,    90,     0,     0,     0,     0,    29,    32,    30,    33,
      20,    21,    17,    18,     0,     0,    38,    42,     0,     0,
      44,    48,    39,    40,    45,    46
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -19,   -19,   -19,   -19,   -19,   -19,   -19,    51,    53,   -19,
     -19,   -19,   -19,   128,   -19,   -19,   -19,   113,   -19,   -19,
     123,   -19,   -19,   122,   130,   110,   -10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     8,     9,    10,    54,    55,    56,   104,   105,    87,
      77,    11,    32,    33,    12,    13,    14,    48,    49,    40,
      41,    42,    91,    69,   106,    52,    34
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      57,   -22,   124,   125,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    53,   -10,
     109,   128,   129,    50,    78,    51,    79,    80,    81,    82,
      83,    84,   116,   110,   111,    85,   117,    88,   126,     1,
       2,    62,    63,    64,   -22,    90,    89,    65,    66,     3,
       4,     5,    65,    66,    35,   108,    86,   130,    43,    67,
      68,   112,   -10,   127,     6,     7,   113,    36,    37,    38,
      39,    44,    45,    46,    47,    92,   107,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    93,   118,    30,    31,    94,   119,   107,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    96,    97,    98,    62,    63,    99,   100,
     101,    58,    59,    95,    60,    74,    75,    76,   114,   115,
     131,   132,   133,   134,   102,   103,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      96,    97,    98,    62,    63,    99,   120,   121,   135,   122,
      61,   123,    73,    71,    72,    70,     0,     0,     0,     0,
       0,   102,   103
};

static const yytype_int8 yycheck[] =
{
      10,     0,     5,     6,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     0,
      22,     5,     6,     0,     5,    43,     7,     8,     9,    10,
      11,    12,    18,    35,    36,    16,    22,    22,    41,     3,
       4,    20,    21,    22,    43,    34,    22,    26,    27,    13,
      14,    15,    26,    27,    17,    37,    37,    41,    17,    38,
      39,    22,    43,    41,    28,    29,    22,    30,    31,    32,
      33,    30,    31,    32,    33,    40,    86,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    40,    18,    20,    21,    40,    22,   108,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    11,    12,    40,    14,    40,    41,    42,    22,    22,
      41,    41,    41,    41,    38,    39,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    41,   108,
      32,   108,    49,    40,    42,    35,    -1,    -1,    -1,    -1,
      -1,    38,    39
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,    13,    14,    15,    28,    29,    45,    46,
      47,    55,    58,    59,    60,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      20,    21,    56,    57,    70,    17,    30,    31,    32,    33,
      63,    64,    65,    17,    30,    31,    32,    33,    61,    62,
       0,    43,    69,    17,    48,    49,    50,    70,    69,    69,
      69,    57,    20,    21,    22,    26,    27,    38,    39,    67,
      68,    64,    67,    61,    40,    41,    42,    54,     5,     7,
       8,     9,    10,    11,    12,    16,    37,    53,    22,    22,
      34,    66,    40,    40,    40,    40,    17,    18,    19,    22,
      23,    24,    38,    39,    51,    52,    68,    70,    37,    22,
      35,    36,    22,    22,    22,    22,    18,    22,    18,    22,
      23,    24,    51,    52,     5,     6,    41,    41,     5,     6,
      41,    41,    41,    41,    41,    41
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    44,    45,    45,    45,    45,    46,    46,    47,    47,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    49,    49,    50,    50,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    52,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    54,    54,    54,    55,    55,    56,    56,
      57,    57,    57,    57,    58,    59,    59,    60,    60,    61,
      61,    61,    61,    61,    62,    62,    63,    63,    64,    64,
      64,    64,    64,    64,    65,    65,    65,    65,    66,    66,
      66,    66,    67,    67,    68,    68,    69,    69,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     3,     1,     1,
       1,     3,     3,     2,     3,     3,     2,     4,     4,     3,
       4,     4,     0,     1,     0,     1,     1,     1,     1,     2,
       2,     1,     2,     2,     1,     1,     1,     1,     4,     5,
       5,     1,     4,     1,     4,     5,     5,     1,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     1,     2,     1,     2,     2,     2,
       2,     3,     3,     3,     1,     1,     1,     1,     2,     2,
       2,     0,     1,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (yyscanner, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, yyscanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, yyscan_t yyscanner)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yyscanner);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, yyscan_t yyscanner)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yyscanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, yyscan_t yyscanner)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , yyscanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, yyscanner); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, yyscan_t yyscanner)
{
  YYUSE (yyvaluep);
  YYUSE (yyscanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (yyscan_t yyscanner)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, yyscanner);
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 102 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psql_yyget_extra(yyscanner)->psqlpluscmd = (yyvsp[-1].psqlplus_cmd);
			}
#line 1426 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 3:
#line 106 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psql_yyget_extra(yyscanner)->psqlpluscmd = (yyvsp[-1].psqlplus_cmd);
			}
#line 1434 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 4:
#line 110 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psql_yyget_extra(yyscanner)->psqlpluscmd = (yyvsp[-1].psqlplus_cmd);
			}
#line 1442 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 5:
#line 115 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psql_yyget_extra(yyscanner)->psqlpluscmd = (yyvsp[-1].psqlplus_cmd);
			}
#line 1450 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 6:
#line 126 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.psqlplus_cmd) = (yyvsp[0].psqlplus_cmd);
		}
#line 1458 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 7:
#line 130 "psqlplusparse.y" /* yacc.c:1646  */
    {
			/*
			 * The weird behavior of oracle, when encountering these characters 
			 * represented by $3, ignore these characters and the following text.
			 */
			PsqlScanState		state = psql_yyget_extra(yyscanner);
			psqlplus_cmd_var	*var = make_variable_node();

			if ((yyvsp[-1].str))
				var->var_name = (yyvsp[-1].str);

			var->list_bind_var = true;
			state->psqlpluscmd = (psqlplus_cmd *) var;
			return 0;
		}
#line 1478 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 154 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* list the specified bind variables */
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = (yyvsp[0].str);
				var->list_bind_var = true;
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
			}
#line 1490 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 162 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* bind variable assignment */
				PsqlScanState		state = psql_yyget_extra(yyscanner); /* BUG#Z352 */
				psqlplus_cmd_var	*var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = (yyvsp[-2].str);
				/* Begin - BUG#Z352 */
				if (state->safe_encoding &&
					state->scanline[strlen(state->scanbuf)] != '\0' &&
					state->scanline[strlen(state->scanbuf)] != ' ' &&
					!(state->scanline[strlen(state->scanbuf)] == ';' &&
					 state->scanline[strlen(state->scanbuf) + 1] == '\0'))
				{
					int		i = 0;
					char	*val = pg_malloc0(strlen(state->scanline) - strlen(state->scanbuf) + 1);
					const char *ptr = state->scanline + strlen(state->scanbuf);

					while(ptr[i] != '\0' && ptr[i] != ' ')
					{
						if (ptr[i] == ';' && ptr[i + 1] == '\0')
							break;

						val[i] = ptr[i];
						i++;
					}
					val[i] = '\0';
					
					var->init_value = psprintf("%s%s", (yyvsp[0].str), val);
					pg_free(val);
				}
				else
				/* End - BUG#Z352 */
					var->init_value = (yyvsp[0].str);
				/* Begin - BUG#Z352 */
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
				/* End - BUG#Z352 */
			}
#line 1533 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 202 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* bind variable assignment */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var *var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = (yyvsp[-2].str);
				var->init_value = (yyvsp[0].str);
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1549 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 215 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* An error scenario, bind variable assignment without a value */
				psqlplus_cmd_var *var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = (yyvsp[-1].str);
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
			}
#line 1561 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 223 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/*
				 * An error scenario
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-2].str);
				var->init_value = (yyvsp[0].str);
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1579 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 237 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/*
				 * An error scenario
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-2].str);
				var->init_value = (yyvsp[0].str);
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1597 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 251 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* declare a bind variable */
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = (yyvsp[-1].str);
				var->vartype = (yyvsp[0].bindvartype);
				var->list_bind_var = false;
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
			}
#line 1610 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 260 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* declare a bind variable with an initial value */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-3].str);
				var->vartype = (yyvsp[-2].bindvartype);
				/* Begin - BUG#Z352 */
				if (state->safe_encoding &&
					state->scanline[strlen(state->scanbuf)] != '\0' &&
					state->scanline[strlen(state->scanbuf)] != ' ' &&
					!(state->scanline[strlen(state->scanbuf)] == ';' &&
					 state->scanline[strlen(state->scanbuf) + 1] == '\0'))
				{
					int		i = 0;
					char	*val = pg_malloc0(strlen(state->scanline) - strlen(state->scanbuf) + 1);
					const char *ptr = state->scanline + strlen(state->scanbuf);

					while(ptr[i] != '\0' && ptr[i] != ' ')
					{
						if (ptr[i] == ';' && ptr[i + 1] == '\0')
							break;

						val[i] = ptr[i];
						i++;
					}
					val[i] = '\0';
					
					var->init_value = psprintf("%s%s", (yyvsp[0].str), val);
					pg_free(val);
				}
				else
				/* End - BUG#Z352 */
					var->init_value = (yyvsp[0].str);

				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1654 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 301 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* declare a bind variable with an initial value */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-3].str);
				var->vartype = (yyvsp[-2].bindvartype);
				var->init_value = (yyvsp[0].str);
				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1671 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 316 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-2].str);
				var->vartype = (yyvsp[-1].bindvartype);
				var->init_value = NULL;
				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
			}
#line 1685 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 326 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/*
				 * An error scenario, implemented separately in 
				 * order to reduce interaction with the server.
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = (yyvsp[-3].str);
				var->vartype = (yyvsp[-2].bindvartype);
				var->init_value = (yyvsp[0].str);
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1706 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 343 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/*
				 * An error scenario, implemented separately in 
				 * order to reduce interaction with the server.
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = (yyvsp[-3].str);
				var->vartype = (yyvsp[-2].bindvartype);
				var->init_value = (yyvsp[0].str);
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
#line 1727 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 360 "psqlplusparse.y" /* yacc.c:1646  */
    {
				/* list all bind variables */
				psqlplus_cmd_var *var = make_variable_node();
				var->list_bind_var = true;
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) var;
			}
#line 1738 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 370 "psqlplusparse.y" /* yacc.c:1646  */
    {  (yyval.str) = (yyvsp[0].str); }
#line 1744 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 372 "psqlplusparse.y" /* yacc.c:1646  */
    {  (yyval.str) = NULL; }
#line 1750 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 375 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1756 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 376 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 1762 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 380 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1768 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 381 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1774 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 382 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1780 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 383 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = psprintf("-%s", (yyvsp[0].str)); }
#line 1786 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 384 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1792 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 385 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = psprintf("+%d", (yyvsp[0].ival)); }
#line 1798 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 386 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = psprintf("-%d", (yyvsp[0].ival)); }
#line 1804 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 387 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = psprintf("%d", (yyvsp[0].ival)); }
#line 1810 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 388 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 1816 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 393 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 1822 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 406 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORACHARCHAROID, 1, (yyvsp[0].keyword));
			}
#line 1830 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 410 "psqlplusparse.y" /* yacc.c:1646  */
    {
				if (pg_strcasecmp(get_guc_settings("nls_length_semantics"), "byte") == 0)
					(yyval.bindvartype) = make_bindvartype(ORACHARBYTEOID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
				else
					(yyval.bindvartype) = make_bindvartype(ORACHARCHAROID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
			}
#line 1841 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 417 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORACHARCHAROID, (yyvsp[-2].ival), (yyvsp[-4].keyword));
			}
#line 1849 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 421 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORACHARBYTEOID, (yyvsp[-2].ival), (yyvsp[-4].keyword));
			}
#line 1857 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 425 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORACHARCHAROID, 1, (yyvsp[0].keyword));
			}
#line 1865 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 429 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORACHARCHAROID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
			}
#line 1873 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 433 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORAVARCHARCHAROID, 1, (yyvsp[0].keyword));
			}
#line 1881 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 437 "psqlplusparse.y" /* yacc.c:1646  */
    {
				if (pg_strcasecmp(get_guc_settings("nls_length_semantics"), "byte") == 0)
					(yyval.bindvartype) = make_bindvartype(ORAVARCHARBYTEOID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
				else
					(yyval.bindvartype) = make_bindvartype(ORAVARCHARCHAROID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
			}
#line 1892 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 444 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORAVARCHARCHAROID, (yyvsp[-2].ival), (yyvsp[-4].keyword));
			}
#line 1900 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 448 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORAVARCHARBYTEOID, (yyvsp[-2].ival), (yyvsp[-4].keyword));
			}
#line 1908 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 452 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORAVARCHARCHAROID, 1, (yyvsp[0].keyword));
			}
#line 1916 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 456 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(ORAVARCHARCHAROID, (yyvsp[-1].ival), (yyvsp[-3].keyword));
			}
#line 1924 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 49:
#line 460 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(NUMBEROID, -1, (yyvsp[0].keyword));
			}
#line 1932 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 464 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(BINARY_FLOATOID, -1, (yyvsp[0].keyword));
			}
#line 1940 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 469 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(SYS_REFCURSOROID, -1, (yyvsp[0].keyword));
			}
#line 1948 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 474 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.bindvartype) = make_bindvartype(BINARY_DOUBLEOID, -1, (yyvsp[0].keyword));
			}
#line 1956 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 490 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) make_print_node();
			}
#line 1964 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 494 "psqlplusparse.y" /* yacc.c:1646  */
    {
				psqlplus_cmd_print *n = make_print_node();
				n->print_items = (yyvsp[0].print_item_list);
				(yyval.psqlplus_cmd) = (psqlplus_cmd *) n;
			}
#line 1974 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 503 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.print_item_list) = (yyvsp[0].print_item_list);
		}
#line 1982 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 507 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.print_item_list) = merge_print_list((yyvsp[-1].print_item_list), (yyvsp[0].print_item_list));
		}
#line 1990 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 514 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.print_item_list) = make_print_list1(make_print_item((yyvsp[0].str), true, false/* BUG#Z351 */));
		}
#line 1998 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 518 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.print_item_list) = make_print_list1(make_print_item(pg_strdup((yyvsp[0].keyword)), true, false/* BUG#Z351 */));
		}
#line 2006 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 522 "psqlplusparse.y" /* yacc.c:1646  */
    {
			/*
			 * When PRINT specifies a bind variable name,
			 * it can be surrounded by double quotes but
			 * not single quotes.
			 */
			(yyval.print_item_list) = make_print_list1(make_print_item((yyvsp[0].str), true, true/* BUG#Z351 */));
		}
#line 2019 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 531 "psqlplusparse.y" /* yacc.c:1646  */
    {
			(yyval.print_item_list) = make_print_list1(make_print_item((yyvsp[0].str), false, true/* BUG#Z351 */));
		}
#line 2027 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 540 "psqlplusparse.y" /* yacc.c:1646  */
    {
			PsqlScanState state = psql_yyget_extra(yyscanner);

			if (state->is_sqlplus_cmd)
			{
				psqlplus_cmd_execute *exec = make_exec_node();
				int	offset = strlen(state->scanbuf);
				exec->plisqlstmts = pg_strdup(state->scanline + offset);
				state->psqlpluscmd = (psqlplus_cmd *) exec;
				return 0;
			}
			else
			{
				return 1;
			}
		}
#line 2048 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 566 "psqlplusparse.y" /* yacc.c:1646  */
    {
				PsqlScanState state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_sv *show = make_show_node();
				show->sv_list = (yyvsp[0].system_variable_list);
				state->psqlpluscmd = (psqlplus_cmd *) show;
				return 0;
			}
#line 2060 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 574 "psqlplusparse.y" /* yacc.c:1646  */
    {
				PsqlScanState state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_sv *set = make_set_node();
				set->sv_list = (yyvsp[0].system_variable_list);
				state->psqlpluscmd = (psqlplus_cmd *) set;
				return 0;
			}
#line 2072 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 585 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 2078 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 586 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 2084 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 587 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 2090 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 588 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 2096 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 589 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = pg_strdup((yyvsp[0].keyword)); }
#line 2102 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 595 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.system_variable_list) = sv_list_make1(make_system_variable((yyvsp[0].str)));
			}
#line 2110 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 599 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.system_variable_list) = sv_list_append((yyvsp[-1].system_variable_list), make_system_variable((yyvsp[0].str)));
			}
#line 2118 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 606 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.system_variable_list) = sv_list_make1((yyvsp[0].system_variable));
			}
#line 2126 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 610 "psqlplusparse.y" /* yacc.c:1646  */
    {
				(yyval.system_variable_list) = sv_list_append((yyvsp[-1].system_variable_list), (yyvsp[0].system_variable));
			}
#line 2134 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 617 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable((yyvsp[-1].str));
				sv->sv_type = SVT_BOOL;
				sv->sv_value.on = (yyvsp[0].boolean);
				(yyval.system_variable) = sv;
			}
#line 2145 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 624 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable((yyvsp[-1].str));
				sv->sv_type = SVT_STRING;
				sv->sv_value.string = pg_strdup((yyvsp[0].str));
				(yyval.system_variable) = sv;
			}
#line 2156 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 631 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable((yyvsp[-1].str));
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = (yyvsp[0].ival);
				(yyval.system_variable) = sv;
			}
#line 2167 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 638 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable((yyvsp[-2].str));
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = (yyvsp[0].ival);
				(yyval.system_variable) = sv;
			}
#line 2178 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 645 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable((yyvsp[-2].str));
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = -(yyvsp[0].ival);
				(yyval.system_variable) = sv;
			}
#line 2189 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 653 "psqlplusparse.y" /* yacc.c:1646  */
    {
				sys_var *sv = make_system_variable("serveroutput");
				csv_serveroutput_node	*data = (csv_serveroutput_node *) pg_malloc0(sizeof(csv_serveroutput_node));
				data->csv_tag = CSV_SERVEROUTPUT;
				data->enable = (yyvsp[-1].boolean);
				data->size = (yyvsp[0].ival);
				sv->sv_type = SVT_COMPOSITE;
				sv->sv_value.string = (char *)data;
				(yyval.system_variable) = sv;
			}
#line 2204 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 685 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2210 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 686 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.ival) = SV_SERVEROUT_BUFSIZE_UNLIMITED; }
#line 2216 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 687 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.ival) = SV_SERVEROUT_BUFSIZE_UNLIMITED; }
#line 2222 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 688 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.ival) = SV_SERVEROUT_NO_SIZE_OPTION; }
#line 2228 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 693 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.boolean) = true; }
#line 2234 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 694 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 2240 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 94:
#line 704 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 2246 "psqlplusparse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 705 "psqlplusparse.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 2252 "psqlplusparse.c" /* yacc.c:1646  */
    break;


#line 2256 "psqlplusparse.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (yyscanner, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yyscanner, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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
                      yytoken, &yylval, yyscanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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
                  yystos[yystate], yyvsp, yyscanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (yyscanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, yyscanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yyscanner);
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
  return yyresult;
}
#line 730 "psqlplusparse.y" /* yacc.c:1906  */


static psqlplus_cmd_var *
make_variable_node(void)
{
	psqlplus_cmd_var *var = pg_malloc0(sizeof(psqlplus_cmd_var));
	var->cmd_type = PSQLPLUS_CMD_VARIABLE;
	var->var_name = NULL;
	var->vartype = NULL;
	var->init_value = NULL;
	var->miss_termination_quote = false;
	var->list_bind_var = false;
	var->assign_bind_var = false;
	var->initial_nonnull_value = false;
	return var;
}

static psqlplus_cmd_print *
make_print_node(void)
{
	psqlplus_cmd_print *print = pg_malloc0(sizeof(psqlplus_cmd_print));
	print->cmd_type = PSQLPLUS_CMD_PRINT;
	print->print_items = NULL;
	return print;
}

static psqlplus_cmd_execute *
make_exec_node(void)
{
	psqlplus_cmd_execute *exec = pg_malloc0(sizeof(psqlplus_cmd_execute));
	exec->cmd_type = PSQLPLUS_CMD_EXECUTE;
	exec->plisqlstmts = NULL;
	return exec;
}

/* Begin - ReqID:SRS-SQL-SYSFUNC */
static psqlplus_cmd_sv *
make_set_node(void)
{
	psqlplus_cmd_sv	*set = pg_malloc0(sizeof(psqlplus_cmd_sv));
	set->cmd_type = PSQLPLUS_CMD_SET;
	set->sv_list = NULL;
	return set;
}

static psqlplus_cmd_sv *
make_show_node(void)
{
	psqlplus_cmd_sv	*show = pg_malloc0(sizeof(psqlplus_cmd_sv));
	show->cmd_type = PSQLPLUS_CMD_SHOW;
	show->sv_list = NULL;
	return show;
}

static sys_var *
make_system_variable(const char *name)
{
	sys_var	*sv = (sys_var *) pg_malloc0(sizeof(sys_var));
	sv->sv_name = pg_strdup(name);
	sv->sv_type = SVT_NONE;
	sv->sv_value.string = NULL;
	return sv;
}

static sys_var_list *
sv_list_make1(sys_var *sv)
{
	sys_var_list	*list = pg_malloc0(sizeof(sys_var_list));
	list->sysvars = (sys_var **) pg_malloc0(sizeof(sys_var *));
	list->sysvars[0] = sv;
	list->length = 1;
	return list;
}

static sys_var_list *
sv_list_append(sys_var_list *list, sys_var *sv)
{
	if (list == NULL)
		return sv_list_make1(sv);

	list->sysvars = (sys_var **) pg_realloc(list->sysvars,
											sizeof(sys_var *) * (list->length+1));
	list->sysvars[list->length++] = sv;
	return list;
}
/* End - ReqID:SRS-SQL-SYSFUNC */

static BindVarType *
make_bindvartype(int32 oid, int32 typmod, const char *typname)
{
	BindVarType *vartype;

	/*
	 * We emulate what anychar_typmodin does here, avoiding the
	 * need to send queries to the server, typmod value need to
	 * add a 4-byte header length for character datatype.
	 */
	if (oid == ORACHARCHAROID ||
		oid == ORACHARBYTEOID ||
		oid == ORAVARCHARCHAROID ||
		oid == ORAVARCHARBYTEOID)
		typmod = typmod + 4;

	vartype = pg_malloc0(sizeof(BindVarType));
	vartype->name = pg_strdup(typname);
	vartype->oid = oid;
	vartype->typmod = typmod;

	return vartype;
}


/*
 * Get the value of the specified GUC.
 */
static char *
get_guc_settings(const char *gucname)
{
	PQExpBuffer query = createPQExpBuffer();
	PGresult	*res;
	char		*val = NULL;

	appendPQExpBuffer(query, "SELECT setting from pg_settings where name = '%s';", gucname);
	res = PQexec(pset.db, query->data);

	if (PQresultStatus(res) == PGRES_TUPLES_OK &&
		PQntuples(res) == 1 &&
		!PQgetisnull(res, 0, 0))
	{		
		val = pg_strdup(PQgetvalue(res, 0, 0));
	}

	PQclear(res);
	destroyPQExpBuffer(query);

	return val;
}

static print_item *
make_print_item(char *name, bool valid, bool need_downcase/* BUG#Z351 */)
{
	print_item *item = pg_malloc0(sizeof(print_item));

	/* Begin - BUG#Z351 */
	/*
	 * When using PRINT to print bind variables in oracle, the semantics
	 * of double quotes is to deal with the problem that the bind variable
	 * has the same name as the keyword.  The name of the bind variable is
	 * case-insensitive and we save it in lowercase,  so downcase needs to
	 * be performed.
	 */
	if (need_downcase)
		item->bv_name = psqlplus_downcase_identifier(name, strlen(name));
	else
	/* End - BUG#Z351 */
		item->bv_name = name;

	item->valid = valid;

	return item;
}

static print_list *
make_print_list1(print_item *item)
{
	print_list	*pl = pg_malloc0(sizeof(print_list));
	pl->items = (print_item **) pg_malloc0(sizeof(print_item *) * 1);
	pl->items[0] = item;
	pl->length = 1;

	return pl;
}

static print_list *
merge_print_list(print_list *list1, print_list *list2)
{
	int	newlen;
	int i;

	if (list1 == NULL)
		return list2;
	if (list2 == NULL)
		return list1;

	newlen = list1->length + list2->length;
	list1->items = (print_item **) pg_realloc(list1->items, sizeof(print_item *) * newlen);

	for (i = 0; i < list2->length; i++)
	{
		list1->items[list1->length] = list2->items[i];
		list1->length++;
	}

	return list1;
}

int
psqlplus_yylex(YYSTYPE *lvalp, yyscan_t yyscanner)
{
	return orapsql_yylex(&(lvalp->psql_yysype), yyscanner);
}

void
psqlplus_yyerror(yyscan_t yyscanner, const char *message)
{
	/* do nothing */
}

/* yylex and the yylval macro, which flex will have its own definition */
#undef yylval
#undef yylex	/* BUG:M0000080 */

#include "psqlplusscan.c"
