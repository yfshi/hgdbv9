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
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         TECC_STYPE
#define YYLTYPE         TECC_LTYPE
/* Substitute the variable and function names.  */
#define yyparse         tecc_parse
#define yylex           tecc_lex
#define yyerror         tecc_error
#define yydebug         tecc_debug
#define yynerrs         tecc_nerrs

#define yylval          tecc_lval
#define yychar          tecc_char
#define yylloc          tecc_lloc

/* Copy the first part of user declarations.  */

#line 78 "tecc_grammar.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "tecc_grammar.h".  */
#ifndef YY_TECC_TECC_GRAMMAR_H_INCLUDED
# define YY_TECC_TECC_GRAMMAR_H_INCLUDED
/* Debug traces.  */
#ifndef TECC_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define TECC_DEBUG 1
#  else
#   define TECC_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define TECC_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined TECC_DEBUG */
#if TECC_DEBUG
extern int tecc_debug;
#endif

/* Token type.  */
#ifndef TECC_TOKENTYPE
# define TECC_TOKENTYPE
  enum tecc_tokentype
  {
    TECC_TOK_IDENT = 258,
    TECC_TOK_STRING = 259,
    TECC_TOK_INTEGER = 260,
    TECC_TOK_FLOAT = 261,
    TECC_TOK_SCONST = 262,
    TECC_TOK_BCONST = 263,
    TECC_TOK_XCONST = 264,
    TECC_TOK_ECONST = 265,
    TECC_TOK_MAXVALUE = 266,
    TECC_TOK_MINVALUE = 267,
    TECC_TOK_PARAM = 268,
    TECC_TOK_SELECT = 269,
    TECC_TOK_INSERT = 270,
    TECC_TOK_UPDATE = 271,
    TECC_TOK_DELETE = 272,
    TECC_TOK_CREATE = 273,
    TECC_TOK_DROP = 274,
    TECC_TOK_ALTER = 275,
    TECC_TOK_EXPLAIN = 276,
    TECC_TOK_ANALYZE = 277,
    TECC_TOK_VERBOSE = 278,
    TECC_TOK_KEK = 279,
    TECC_TOK_DEK = 280,
    TECC_TOK_TABLE = 281,
    TECC_TOK_VIEW = 282,
    TECC_TOK_INDEX = 283,
    TECC_TOK_SEQUENCE = 284,
    TECC_TOK_FUNCTION = 285,
    TECC_TOK_MATERIALIZED = 286,
    TECC_TOK_REPLACE = 287,
    TECC_TOK_REFRESH = 288,
    TECC_TOK_KEY_ALGO = 289,
    TECC_TOK_KEY_LENGTH = 290,
    TECC_TOK_KEY_PATH = 291,
    TECC_TOK_KEY_STORE = 292,
    TECC_TOK_KEY_TYPE = 293,
    TECC_TOK_ARRAY = 294,
    TECC_TOK_CURRENT_DATE = 295,
    TECC_TOK_CURRENT_TIME = 296,
    TECC_TOK_CURRENT_TIMESTAMP = 297,
    TECC_TOK_WITH = 298,
    TECC_TOK_RECURSIVE = 299,
    TECC_TOK_WHERE = 300,
    TECC_TOK_FROM = 301,
    TECC_TOK_SET = 302,
    TECC_TOK_INTO = 303,
    TECC_TOK_VALUES = 304,
    TECC_TOK_ADD = 305,
    TECC_TOK_COLUMN = 306,
    TECC_TOK_CONSTRAINT = 307,
    TECC_TOK_DATA = 308,
    TECC_TOK_TYPE = 309,
    TECC_TOK_ENCRYPTED = 310,
    TECC_TOK_TEMPORARY = 311,
    TECC_TOK_TEMP = 312,
    TECC_TOK_COLLATE = 313,
    TECC_TOK_CHECK = 314,
    TECC_TOK_UNIQUE = 315,
    TECC_TOK_PRIMARY = 316,
    TECC_TOK_KEY = 317,
    TECC_TOK_REFERENCES = 318,
    TECC_TOK_FOREIGN = 319,
    TECC_TOK_AND = 320,
    TECC_TOK_OR = 321,
    TECC_TOK_NOT = 322,
    TECC_TOK_NULL_P = 323,
    TECC_TOK_IS = 324,
    TECC_TOK_BETWEEN = 325,
    TECC_TOK_LIKE = 326,
    TECC_TOK_ILIKE = 327,
    TECC_TOK_IN = 328,
    TECC_TOK_REGEX_NOT_MATCH = 329,
    TECC_TOK_REGEX_NOT_MATCH_CI = 330,
    TECC_TOK_REGEX_MATCH = 331,
    TECC_TOK_REGEX_MATCH_CI = 332,
    TECC_TOK_TRUE_P = 333,
    TECC_TOK_FALSE_P = 334,
    TECC_TOK_UNKNOWN = 335,
    TECC_TOK_EXISTS = 336,
    TECC_TOK_ALL = 337,
    TECC_TOK_ANY = 338,
    TECC_TOK_SOME = 339,
    TECC_TOK_SYMMETRIC = 340,
    TECC_TOK_SIMILAR = 341,
    TECC_TOK_VARIADIC = 342,
    TECC_TOK_WITHIN = 343,
    TECC_TOK_GROUP = 344,
    TECC_TOK_BY = 345,
    TECC_TOK_HAVING = 346,
    TECC_TOK_ORDER = 347,
    TECC_TOK_ASC = 348,
    TECC_TOK_DESC = 349,
    TECC_TOK_LIMIT = 350,
    TECC_TOK_OFFSET = 351,
    TECC_TOK_DISTINCT = 352,
    TECC_TOK_UNION = 353,
    TECC_TOK_INTERSECT = 354,
    TECC_TOK_EXCEPT = 355,
    TECC_TOK_JOIN = 356,
    TECC_TOK_INNER = 357,
    TECC_TOK_LEFT = 358,
    TECC_TOK_RIGHT = 359,
    TECC_TOK_FULL = 360,
    TECC_TOK_OUTER = 361,
    TECC_TOK_ON = 362,
    TECC_TOK_CASCADE = 363,
    TECC_TOK_RESTRICT = 364,
    TECC_TOK_IF_P = 365,
    TECC_TOK_EXISTS_P = 366,
    TECC_TOK_PREPARE = 367,
    TECC_TOK_EXECUTE = 368,
    TECC_TOK_DEALLOCATE = 369,
    TECC_TOK_DECLARE = 370,
    TECC_TOK_CURSOR = 371,
    TECC_TOK_FOR = 372,
    TECC_TOK_FETCH = 373,
    TECC_TOK_MOVE = 374,
    TECC_TOK_CLOSE = 375,
    TECC_TOK_COPY = 376,
    TECC_TOK_TO = 377,
    TECC_TOK_STDIN = 378,
    TECC_TOK_STDOUT = 379,
    TECC_TOK_BINARY = 380,
    TECC_TOK_CSV = 381,
    TECC_TOK_TEXT = 382,
    TECC_TOK_RENAME = 383,
    TECC_TOK_SCHEMA = 384,
    TECC_TOK_FORMAT = 385,
    TECC_TOK_DELIMITER = 386,
    TECC_TOK_ENCODING = 387,
    TECC_TOK_FREEZE = 388,
    TECC_TOK_HEADER = 389,
    TECC_TOK_QUOTE = 390,
    TECC_TOK_ESCAPE = 391,
    TECC_TOK_FORCE_QUOTE = 392,
    TECC_TOK_FORCE_NOT_NULL = 393,
    TECC_TOK_FORCE_NULL = 394,
    TECC_TOK_WINDOW = 395,
    TECC_TOK_USING = 396,
    TECC_TOK_AS = 397,
    TECC_TOK_RETURNING = 398,
    TECC_TOK_DEFAULT = 399,
    TECC_TOK_MATCH = 400,
    TECC_TOK_PARTIAL = 401,
    TECC_TOK_SIMPLE = 402,
    TECC_TOK_NO = 403,
    TECC_TOK_ACTION = 404,
    TECC_TOK_INHERITS = 405,
    TECC_TOK_INHERIT = 406,
    TECC_TOK_UNLOGGED = 407,
    TECC_TOK_OF = 408,
    TECC_TOK_TABLESPACE = 409,
    TECC_TOK_PARTITION = 410,
    TECC_TOK_RANGE = 411,
    TECC_TOK_LIST = 412,
    TECC_TOK_HASH = 413,
    TECC_TOK_DEFERRABLE = 414,
    TECC_TOK_INITIALLY = 415,
    TECC_TOK_DEFERRED = 416,
    TECC_TOK_IMMEDIATE = 417,
    TECC_TOK_GENERATED = 418,
    TECC_TOK_IDENTITY = 419,
    TECC_TOK_ALWAYS = 420,
    TECC_TOK_STORED = 421,
    TECC_TOK_EXCLUDE = 422,
    TECC_TOK_COMMIT = 423,
    TECC_TOK_PRESERVE = 424,
    TECC_TOK_ROWS = 425,
    TECC_TOK_INCLUDING = 426,
    TECC_TOK_EXCLUDING = 427,
    TECC_TOK_CROSS = 428,
    TECC_TOK_NATURAL = 429,
    TECC_TOK_CASE = 430,
    TECC_TOK_WHEN = 431,
    TECC_TOK_THEN = 432,
    TECC_TOK_ELSE_P = 433,
    TECC_TOK_END_P = 434,
    TECC_TOK_OVER = 435,
    TECC_TOK_CAST = 436,
    TECC_TOK_ATTACH = 437,
    TECC_TOK_DETACH = 438,
    TECC_TOK_ROW = 439,
    TECC_TOK_DO = 440,
    TECC_TOK_NOTHING = 441,
    TECC_TOK_CONFLICT = 442,
    TECC_TOK_FILTER = 443,
    TECC_TOK_NULLS_P = 444,
    TECC_TOK_FIRST_P = 445,
    TECC_TOK_LAST_P = 446,
    TECC_TOK_LATERAL = 447,
    TECC_TOK_SHARE = 448,
    TECC_TOK_NOWAIT = 449,
    TECC_TOK_SKIP = 450,
    TECC_TOK_LOCKED = 451,
    TECC_TOK_OVERRIDING = 452,
    TECC_TOK_USER_P = 453,
    TECC_TOK_SYSTEM_P = 454,
    TECC_TOK_VALUE_P = 455,
    TECC_TOK_AT_P = 456,
    TECC_TOK_TIME_P = 457,
    TECC_TOK_ZONE_P = 458,
    TECC_TOK_LOCAL = 459,
    TECC_TOK_SESSION = 460,
    TECC_TOK_TRANSACTION = 461,
    TECC_TOK_CHARACTERISTICS = 462,
    TECC_TOK_CATALOG_P = 463,
    TECC_TOK_NAMES = 464,
    TECC_TOK_ROLE = 465,
    TECC_TOK_AUTHORIZATION = 466,
    TECC_TOK_XML_P = 467,
    TECC_TOK_OPTION = 468,
    TECC_TOK_DOCUMENT = 469,
    TECC_TOK_CONTENT = 470,
    TECC_TOK_SNAPSHOT = 471,
    TECC_TOK_ISOLATION = 472,
    TECC_TOK_LEVEL = 473,
    TECC_TOK_READ = 474,
    TECC_TOK_UNCOMMITTED = 475,
    TECC_TOK_COMMITTED = 476,
    TECC_TOK_REPEATABLE = 477,
    TECC_TOK_SERIALIZABLE = 478,
    TECC_TOK_ONLY = 479,
    TECC_TOK_WRITE = 480,
    TECC_TOK_CURRENT_P = 481,
    TECC_TOK_PLUS = 482,
    TECC_TOK_MINUS = 483,
    TECC_TOK_STAR = 484,
    TECC_TOK_SLASH = 485,
    TECC_TOK_PERCENT = 486,
    TECC_TOK_EQUAL = 487,
    TECC_TOK_NOT_EQUAL = 488,
    TECC_TOK_LESS = 489,
    TECC_TOK_GREATER = 490,
    TECC_TOK_LESS_EQUAL = 491,
    TECC_TOK_GREATER_EQUAL = 492,
    TECC_TOK_CONCAT = 493,
    TECC_TOK_TYPECAST = 494,
    TECC_TOK_OPERATOR = 495,
    TECC_TOK_OP = 496,
    TECC_TOK_SEMICOLON = 497,
    TECC_TOK_COMMA = 498,
    TECC_TOK_DOT = 499,
    TECC_TOK_LPAREN = 500,
    TECC_TOK_RPAREN = 501,
    TECC_TOK_LBRACKET = 502,
    TECC_TOK_RBRACKET = 503,
    TECC_TOK_COLON = 504,
    TECC_TOK_DOLLAR = 505,
    TECC_TOK_UMINUS = 506,
    TECC_TOK_IS_TRUE_PREC = 507
  };
#endif

/* Value type.  */
#if ! defined TECC_STYPE && ! defined TECC_STYPE_IS_DECLARED

union TECC_STYPE
{
#line 18 "tecc_grammar.y" /* yacc.c:355  */

    long long ival;
    char *str;
    double dval;
    int bval;  /* Use int for bool, will be converted in code */
    void *node;
    void *list;
    void *range;

#line 389 "tecc_grammar.c" /* yacc.c:355  */
};

typedef union TECC_STYPE TECC_STYPE;
# define TECC_STYPE_IS_TRIVIAL 1
# define TECC_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined TECC_LTYPE && ! defined TECC_LTYPE_IS_DECLARED
typedef struct TECC_LTYPE TECC_LTYPE;
struct TECC_LTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define TECC_LTYPE_IS_DECLARED 1
# define TECC_LTYPE_IS_TRIVIAL 1
#endif


extern TECC_STYPE tecc_lval;
extern TECC_LTYPE tecc_lloc;
int tecc_parse (void);

#endif /* !YY_TECC_TECC_GRAMMAR_H_INCLUDED  */

/* Copy the second part of user declarations.  */
#line 37 "tecc_grammar.y" /* yacc.c:358  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

/* Include TECC node definitions */
#include "tecc_nodes.h"
/* Include list functions (must be before tecc_parser.h which uses List type) */
#include "../tecc_common/tecc_frontend_list.h"
#include "tecc_parser.h"
/* Include logging */
#include "../tecc_common/tecc_logging.h"

/* Bison error reporting function - defined by Bison in tecc_grammar.c */
void tecc_yyerror(const char *msg);
void tecc_error(const char *msg);

/* Use our memory allocation functions for Bison */
#define YYMALLOC tecc_palloc
#define YYFREE tecc_pfree

/* Define TECC_LTYPE before including headers */
#if ! defined TECC_LTYPE && ! defined TECC_LTYPE_IS_DECLARED
typedef struct TECC_LTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} TECC_LTYPE;
#define TECC_LTYPE_IS_DECLARED 1
#endif

/* Use extern "C" for C++ compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* Global variable for parse tree */
List *tecc_parsetree = NIL;

/* Declare parser functions */
int tecc_lex(void);
void tecc_yyerror(const char *msg);

/* Helper functions for node creation */
static Node *make_column_ref_from_list(List *fields, int location);
static Node *make_func_call_from_list(List *funcname, List *args, int location);
static Node *make_a_expr(int kind, List *name, Node *lexpr, Node *rexpr, int location);
static Node *make_a_expr_escape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location);
static Node *make_bool_expr(int boolop, List *args, int location);
/* ReqID:TECC4.0 - Expand `expr op ANY/SOME/ALL(ARRAY[...])` into OR/AND of
 * element comparisons so each constant goes through the normal encryption path. */
static Node *make_any_all_array_expr(void *op_name, void *lhs, List *elements, int use_or, int location);
static Node *make_null_test(Node *arg, int nulltesttype, int location);
static List *convert_string_list_to_restargets(List *string_list, int location);

/* Node copy function */
extern Node *tecc_copy_node(const Node *node);

/* Simple helper to create a string value node */
static Value *makeString(const char *str) {
    Value *v = (Value *)tecc_palloc(sizeof(Value));
    v->type = FE_T_String;
    v->val.str = tecc_pstrdup(str);
    v->location = 0;
    return v;
}


/* ReqID:TECC4.0 - lcons: prepend element to front of list */
static List *lcons(void *datum, List *list) {
    List *new_list = list_make1(datum);
    if (list != NIL && list != NULL) {
        new_list->head->next = list->head;
        new_list->tail = list->tail;
        new_list->length += list->length;
        new_list->next = list->next;
    }
    return new_list;
}
/* Helper to free a list containing nodes */
static pg_attribute_unused() void free_node_list(List *list) {
    if (!list) return;
    {
        ListCell *cell = list->head;
        while (cell) {
            ListCell *next = cell->next;
            if (cell->data.ptr_value) {
                freeNode((Node *)cell->data.ptr_value);
            }
            tecc_pfree(cell);
            cell = next;
        }

        tecc_pfree(list);
    }
}

/* Helper to create ColumnDef for ALTER COLUMN TYPE with collation */
static Node *makeColumnDefForAlter(const char *colname, TypeName *typeName, Node *opt_collate) {
    ColumnDef *coldef = makeColumnDef(0, 0); /* location will be set by caller */
    coldef->colname = tecc_pstrdup(colname);
    coldef->typeName = typeName;
    coldef->constraints = NIL;
    coldef->collClause = opt_collate;

    return (Node *)coldef;
}

/* Helper to create ColumnDef for ALTER COLUMN SET COLLATE only */
static Node *makeColumnDefForAlterCollateOnly(const char *colname, Node *collate_clause) {
    ColumnDef *coldef = makeColumnDef(0, 0);
    coldef->colname = tecc_pstrdup(colname);
    coldef->typeName = NULL; /* No type change, only collation */
    coldef->constraints = NIL;
    coldef->collClause = collate_clause;

    return (Node *)coldef;
}

#ifdef __cplusplus
}
#endif

/* 辅助结构：用于在 opt_partition_of 规则中同时传递父表和分区边界 */
typedef struct PartitionOfClause {
    Node   *parent;
    Node   *partbound;
} PartitionOfClause;

/* JOIN type constants */
#define JOIN_INNER 0
#define JOIN_LEFT 1
#define JOIN_RIGHT 2
#define JOIN_FULL 3
#define JOIN_CROSS 4

#line 560 "tecc_grammar.c" /* yacc.c:358  */

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
         || (defined TECC_LTYPE_IS_TRIVIAL && TECC_LTYPE_IS_TRIVIAL \
             && defined TECC_STYPE_IS_TRIVIAL && TECC_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  139
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5514

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  261
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  185
/* YYNRULES -- Number of rules.  */
#define YYNRULES  727
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1468

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   507

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     255,   256,     2,     2,   254,     2,   257,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   260,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   258,     2,   259,   251,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   252,   253
};

#if TECC_DEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   405,   405,   412,   422,   425,   431,   434,   437,   440,
     443,   446,   449,   452,   455,   458,   461,   464,   467,   470,
     473,   476,   479,   482,   485,   488,   507,   512,   517,   522,
     534,   539,   544,   552,   555,   558,   562,   565,   568,   571,
     577,   578,   581,   584,   587,   591,   594,   597,   600,   603,
     606,   612,   613,   617,   618,   622,   623,   624,   625,   626,
     630,   631,   632,   633,   634,   638,   639,   640,   641,   642,
     643,   644,   645,   649,   653,   654,   655,   659,   660,   661,
     665,   666,   667,   671,   672,   673,   674,   675,   679,   680,
     681,   682,   686,   687,   691,   692,   696,   697,   701,   702,
     706,   707,   708,   709,   710,   715,   723,   734,   739,   750,
     758,   769,   774,   791,   808,   821,   834,   847,   864,   874,
     875,   879,   881,   883,   885,   887,   889,   891,   893,   894,
     896,   897,   898,   899,   900,   902,   904,   906,   907,   908,
     909,   910,   912,   917,   918,   919,   920,   924,   925,   929,
     930,   931,   935,   942,   943,   944,   945,   946,   950,   962,
     966,   968,   969,   970,   971,   972,   973,   977,   978,   982,
     983,   986,   987,   991,   992,   996,   997,  1001,  1002,  1003,
    1004,  1005,  1006,  1007,  1008,  1010,  1011,  1012,  1013,  1015,
    1016,  1020,  1021,  1025,  1026,  1027,  1028,  1029,  1031,  1036,
    1037,  1041,  1042,  1043,  1044,  1045,  1046,  1047,  1052,  1057,
    1058,  1062,  1067,  1068,  1072,  1076,  1077,  1081,  1082,  1086,
    1087,  1088,  1089,  1090,  1094,  1095,  1096,  1097,  1098,  1102,
    1103,  1107,  1108,  1109,  1110,  1115,  1116,  1126,  1133,  1139,
    1147,  1148,  1152,  1156,  1161,  1167,  1175,  1183,  1188,  1194,
    1201,  1208,  1214,  1225,  1229,  1235,  1236,  1240,  1241,  1245,
    1246,  1250,  1261,  1262,  1266,  1267,  1268,  1269,  1273,  1274,
    1278,  1279,  1284,  1293,  1302,  1313,  1322,  1331,  1352,  1353,
    1358,  1359,  1363,  1367,  1371,  1375,  1379,  1387,  1404,  1413,
    1428,  1442,  1456,  1457,  1458,  1459,  1460,  1461,  1462,  1463,
    1464,  1465,  1466,  1467,  1468,  1469,  1470,  1471,  1472,  1476,
    1477,  1478,  1482,  1487,  1488,  1489,  1490,  1491,  1492,  1493,
    1494,  1495,  1496,  1497,  1498,  1499,  1500,  1501,  1502,  1503,
    1507,  1512,  1516,  1520,  1524,  1528,  1532,  1537,  1541,  1545,
    1549,  1557,  1561,  1565,  1570,  1578,  1579,  1580,  1581,  1582,
    1592,  1598,  1615,  1617,  1619,  1621,  1623,  1625,  1627,  1629,
    1631,  1633,  1635,  1637,  1639,  1641,  1643,  1645,  1647,  1657,
    1662,  1677,  1681,  1688,  1692,  1693,  1694,  1696,  1700,  1701,
    1704,  1705,  1709,  1714,  1721,  1726,  1730,  1734,  1739,  1745,
    1749,  1753,  1754,  1755,  1765,  1772,  1777,  1789,  1798,  1818,
    1828,  1840,  1845,  1848,  1853,  1856,  1857,  1862,  1872,  1873,
    1878,  1883,  1890,  1891,  1896,  1897,  1902,  1906,  1913,  1914,
    1919,  1920,  1924,  1932,  1946,  1958,  1961,  1964,  1967,  1970,
    1976,  1981,  1985,  1989,  1993,  2000,  2001,  2005,  2016,  2020,
    2027,  2028,  2032,  2033,  2037,  2038,  2042,  2043,  2044,  2048,
    2049,  2053,  2054,  2055,  2056,  2057,  2058,  2059,  2060,  2061,
    2062,  2063,  2065,  2066,  2067,  2068,  2069,  2070,  2071,  2072,
    2073,  2074,  2075,  2076,  2077,  2078,  2079,  2080,  2081,  2082,
    2083,  2084,  2085,  2086,  2087,  2091,  2095,  2099,  2103,  2107,
    2111,  2116,  2120,  2124,  2129,  2133,  2155,  2180,  2184,  2185,
    2190,  2191,  2195,  2196,  2201,  2204,  2207,  2211,  2217,  2218,
    2222,  2223,  2227,  2228,  2232,  2236,  2240,  2244,  2248,  2253,
    2259,  2265,  2271,  2278,  2284,  2290,  2296,  2302,  2311,  2317,
    2321,  2325,  2331,  2335,  2341,  2343,  2347,  2348,  2349,  2350,
    2351,  2355,  2356,  2361,  2365,  2371,  2380,  2381,  2385,  2386,
    2390,  2391,  2395,  2396,  2400,  2401,  2405,  2406,  2410,  2422,
    2436,  2437,  2438,  2442,  2443,  2444,  2448,  2449,  2453,  2454,
    2459,  2460,  2464,  2465,  2469,  2478,  2479,  2480,  2481,  2485,
    2486,  2490,  2491,  2492,  2497,  2498,  2503,  2504,  2505,  2509,
    2510,  2519,  2532,  2548,  2549,  2558,  2559,  2563,  2570,  2574,
    2586,  2587,  2599,  2603,  2623,  2624,  2629,  2630,  2635,  2636,
    2641,  2642,  2643,  2648,  2649,  2653,  2654,  2658,  2663,  2668,
    2673,  2681,  2693,  2705,  2718,  2723,  2733,  2734,  2735,  2736,
    2740,  2741,  2745,  2746,  2750,  2757,  2764,  2768,  2772,  2776,
    2783,  2784,  2785,  2789,  2798,  2805,  2814,  2829,  2834,  2839,
    2844,  2849,  2854,  2864,  2876,  2883,  2889,  2895,  2904,  2905,
    2906,  2907,  2908,  2912,  2913,  2917,  2918,  2922,  2927,  2932,
    2938,  2944,  2950,  2955,  2960,  2965,  2970,  2975,  2980,  2986,
    2992,  2998,  3003,  3008,  3013,  3018,  3025,  3031,  3043,  3051,
    3059,  3067,  3075,  3086,  3087,  3088,  3089,  3090,  3094,  3095,
    3099,  3100,  3101,  3102,  3103,  3104,  3108,  3109,  3112,  3113,
    3114,  3115,  3116,  3117,  3118,  3119,  3120,  3121,  3122,  3123,
    3133,  3137,  3146,  3150,  3157,  3181,  3201,  3221
};
#endif

#if TECC_DEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENT", "STRING", "INTEGER", "FLOAT",
  "SCONST", "BCONST", "XCONST", "ECONST", "MAXVALUE", "MINVALUE", "PARAM",
  "SELECT", "INSERT", "UPDATE", "DELETE", "CREATE", "DROP", "ALTER",
  "EXPLAIN", "ANALYZE", "VERBOSE", "KEK", "DEK", "TABLE", "VIEW", "INDEX",
  "SEQUENCE", "FUNCTION", "MATERIALIZED", "REPLACE", "REFRESH", "KEY_ALGO",
  "KEY_LENGTH", "KEY_PATH", "KEY_STORE", "KEY_TYPE", "ARRAY",
  "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "WITH", "RECURSIVE",
  "WHERE", "FROM", "SET", "INTO", "VALUES", "ADD", "COLUMN", "CONSTRAINT",
  "DATA", "TYPE", "ENCRYPTED", "TEMPORARY", "TEMP", "COLLATE", "CHECK",
  "UNIQUE", "PRIMARY", "KEY", "REFERENCES", "FOREIGN", "AND", "OR", "NOT",
  "NULL_P", "IS", "BETWEEN", "LIKE", "ILIKE", "IN", "REGEX_NOT_MATCH",
  "REGEX_NOT_MATCH_CI", "REGEX_MATCH", "REGEX_MATCH_CI", "TRUE_P",
  "FALSE_P", "UNKNOWN", "EXISTS", "ALL", "ANY", "SOME", "SYMMETRIC",
  "SIMILAR", "VARIADIC", "WITHIN", "GROUP", "BY", "HAVING", "ORDER", "ASC",
  "DESC", "LIMIT", "OFFSET", "DISTINCT", "UNION", "INTERSECT", "EXCEPT",
  "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER", "ON", "CASCADE",
  "RESTRICT", "IF_P", "EXISTS_P", "PREPARE", "EXECUTE", "DEALLOCATE",
  "DECLARE", "CURSOR", "FOR", "FETCH", "MOVE", "CLOSE", "COPY", "TO",
  "STDIN", "STDOUT", "BINARY", "CSV", "TEXT", "RENAME", "SCHEMA", "FORMAT",
  "DELIMITER", "ENCODING", "FREEZE", "HEADER", "QUOTE", "ESCAPE",
  "FORCE_QUOTE", "FORCE_NOT_NULL", "FORCE_NULL", "WINDOW", "USING", "AS",
  "RETURNING", "DEFAULT", "MATCH", "PARTIAL", "SIMPLE", "NO", "ACTION",
  "INHERITS", "INHERIT", "UNLOGGED", "OF", "TABLESPACE", "PARTITION",
  "RANGE", "LIST", "HASH", "DEFERRABLE", "INITIALLY", "DEFERRED",
  "IMMEDIATE", "GENERATED", "IDENTITY", "ALWAYS", "STORED", "EXCLUDE",
  "COMMIT", "PRESERVE", "ROWS", "INCLUDING", "EXCLUDING", "CROSS",
  "NATURAL", "CASE", "WHEN", "THEN", "ELSE_P", "END_P", "OVER", "CAST",
  "ATTACH", "DETACH", "ROW", "DO", "NOTHING", "CONFLICT", "FILTER",
  "NULLS_P", "FIRST_P", "LAST_P", "LATERAL", "SHARE", "NOWAIT", "SKIP",
  "LOCKED", "OVERRIDING", "USER_P", "SYSTEM_P", "VALUE_P", "AT_P",
  "TIME_P", "ZONE_P", "LOCAL", "SESSION", "TRANSACTION", "CHARACTERISTICS",
  "CATALOG_P", "NAMES", "ROLE", "AUTHORIZATION", "XML_P", "OPTION",
  "DOCUMENT", "CONTENT", "SNAPSHOT", "ISOLATION", "LEVEL", "READ",
  "UNCOMMITTED", "COMMITTED", "REPEATABLE", "SERIALIZABLE", "ONLY",
  "WRITE", "CURRENT_P", "PLUS", "MINUS", "STAR", "SLASH", "PERCENT",
  "EQUAL", "NOT_EQUAL", "LESS", "GREATER", "LESS_EQUAL", "GREATER_EQUAL",
  "CONCAT", "TYPECAST", "OPERATOR", "OP", "SEMICOLON", "COMMA", "DOT",
  "LPAREN", "RPAREN", "LBRACKET", "RBRACKET", "COLON", "DOLLAR", "'^'",
  "UMINUS", "IS_TRUE_PREC", "','", "'('", "')'", "'.'", "'['", "']'",
  "':'", "$accept", "input", "stmt_list", "stmt", "VariableSetStmt",
  "set_rest", "generic_set", "set_rest_more", "var_name", "var_list",
  "var_value", "opt_boolean_or_string", "zone_value", "opt_interval",
  "opt_encoding", "transaction_mode_list", "transaction_mode_item",
  "iso_level", "document_or_content", "ident_or_sconst", "opt_reloptions",
  "reloption_list", "reloption_elem", "create_kek_stmt", "kek_opt_options",
  "alter_kek_stmt", "create_dek_stmt", "dek_opt_options",
  "create_table_stmt", "alter_table_stmt", "alter_table_cmds",
  "alter_table_cmd", "opt_temp", "table_element_list", "table_element",
  "like_table", "opt_like_options", "column_def", "col_name", "type_name",
  "encrypted_options", "encrypted_option", "opt_collate",
  "opt_column_constraint_list", "column_constraint_list",
  "column_constraint", "table_constraint", "constraint_spec",
  "exclude_elem_list", "exclude_operator", "constraint_def", "column_list",
  "column_elem", "index_list", "index_elem", "opt_reference_options",
  "reference_options", "reference_option", "reference_action",
  "opt_inherit", "opt_partition_by", "opt_partition_of", "for_values",
  "for_val_list", "for_val", "opt_of_table_column_list",
  "of_table_element_list", "of_table_element", "of_column_def",
  "opt_of_column_options", "opt_on_commit", "opt_tablespace",
  "qualified_name_list", "drop_stmt", "select_operand", "select_stmt",
  "simple_select", "insert_stmt", "update_stmt", "delete_stmt", "all_Op",
  "any_operator", "qual_Op", "b_expr", "expr", "a_expr", "simple_expr",
  "column_ref", "func_call", "func_expr_list", "opt_sort_clause",
  "opt_within_group_clause", "opt_filter_clause", "opt_over_clause",
  "window_specification", "opt_partition_clause", "window_definition_list",
  "window_definition", "constant", "param_ref", "case_expr",
  "when_clause_list", "when_clause", "indirection_el", "opt_slice_bound",
  "indirection", "any_name_list", "any_name", "attrs", "name", "col_id",
  "iconst", "sconst", "stmt_name", "target_list", "id_list", "expr_list",
  "type_list", "opt_all_clause", "opt_from_clause", "from_list",
  "table_ref", "basic_table_ref", "opt_outer", "opt_table_alias",
  "qualified_name", "relation_expr_opt_alias", "opt_where", "opt_group",
  "opt_having", "opt_window", "opt_order", "sortby_list", "sortby",
  "opt_asc_desc", "opt_nulls_order", "opt_limit", "opt_offset",
  "opt_locking_clause", "for_locking_items", "for_locking_item",
  "for_locking_strength", "opt_locked_rels", "opt_nowait_or_skip",
  "opt_column_list", "opt_overriding", "insert_rest", "values_clause",
  "opt_on_conflict", "conflict_target", "conflict_action",
  "set_clause_list", "set_clause", "set_target_list", "opt_returning",
  "opt_using", "opt_drop_behavior", "kek_opt_list", "dek_opt_list",
  "kek_param", "dek_param", "PreparableStmt", "prep_type_clause",
  "execute_param_clause", "prepare_stmt", "execute_stmt",
  "deallocate_stmt", "opt_with_data", "create_as_stmt", "view_stmt",
  "matview_stmt", "declare_stmt", "copy_stmt", "copy_options",
  "copy_option_list_nonempty", "copy_option_list", "copy_option",
  "explain_stmt", "explainable_stmt", "explain_option_list", "any_oper",
  "explain_option_elem", "with_clause", "cte_list", "cte", YY_NULLPTR
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,    94,   506,   507,    44,    40,    41,    46,    91,    93,
      58
};
# endif

#define YYPACT_NINF -1187

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-1187)))

#define YYTABLE_NINF -452

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-452)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     785,    51,   137,   135,   152,  1039,   863,   393,   541,  4251,
     645,  -178,   198,   198,  3959,   198,    31,   182,   212,   -16,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,   832,
   -1187, -1187, -1187, -1187,     2, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187,    85, -1187,   180,  1694,   135,    70,
    4299,   303,   135,  4026,  4199,   135,   366,   369,   374,   400,
   -1187,   438,  4424,  4541,    54,  4598,   135,   426,    74,   239,
     832, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,  5288,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187,   206,   219, -1187, -1187,   487,   294,   784,   661,   418,
     507,   177,    92,   332, -1187, -1187, -1187,   256,  2364, -1187,
     296,   299, -1187,  4650, -1187,   457,   182,   206,   328, -1187,
     785,    16,   125,   276,   327,   832, -1187, -1187, -1187,   331,
   -1187, -1187, -1187, -1187, -1187,   363, -1187, -1187, -1187,  2364,
   -1187, -1187, -1187,   341, -1187,  1803,   346, -1187,  2806,  2806,
   -1187,  1468,  2872,  3658,     8, -1187, -1187, -1187, -1187, -1187,
      11, -1187,    -2,   206,   643,  5288, -1187,  3161,   499, -1187,
     582,   631,   405,   618,   680,   206,   135,   135,   135,   756,
      90,   617,   433,   697,   433,   701,   433,   631,   934,    74,
   -1187, -1187,    75, -1187, -1187,   249, -1187,   219,  5288,   601,
    5288, -1187,   100,   162, -1187,   716,   311, -1187,   664, -1187,
     844,   656,   446,   343, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187,   534,   653,   399,   687,   880,  1990,   270,  5288,
     752,  2364, -1187, -1187, -1187,   799,   416,   238, -1187, -1187,
     182, -1187,   182, -1187,   182, -1187,  2364,  2364,  2364,  2232,
     182,  2364,  1655,   651, -1187,  2364, -1187, -1187,  1468,   422,
     937,   695,   867,   781,  2806,  2806,  2806,  2806,  2806,  2806,
    2806,  2806,  2806,  2806,  2806,  2806,   746,  2806,  2806,   916,
    2364,  2364,  1143,  2472,  2364,  2364,   789,  2364,  2364,  2364,
    2364,   873,  5288, -1187, -1187,  4598,  5288, -1187,  1359,  3434,
    2806, -1187,   798,  3035,  2029,   994,   850, -1187, -1187,  5288,
     861,     1, -1187,  3035,   994,  1007,   846, -1187,  5288,   887,
    1025,  1078, -1187,   995,   206,   206,   206,   135,  1121,  1134,
    1144,   237,  5288, -1187, -1187, -1187,  5288, -1187,   135, -1187,
   -1187,   852,  1115,  1068,   856,    34,  1072,  1086,   965, -1187,
   -1187, -1187, -1187, -1187, -1187,  1130,   506,    74, -1187,   462,
     999, -1187,  1243, -1187, -1187, -1187, -1187, -1187,   830, -1187,
    1044, -1187, -1187, -1187, -1187,   549, -1187, -1187,   428, -1187,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187,   915,  1046, -1187, -1187, -1187,  1046, -1187,  2364,
   -1187, -1187,    37,   474,    85,   478,   182,  1137,  1196,  1199,
   -1187, -1187, -1187,   532,   590,   340,   508,  1583,   686,  2364,
   -1187, -1187,   702,   511, -1187, -1187,  2364,   740, -1187, -1187,
   -1187, -1187,  1329,  1184,  1005,  1005,   -94,   -94,   -94,  2219,
    2219,  2219,  2219,  2219,  2219,  1005,   849,   655,   972,  1133,
    1138,  1139,  2232,  2199,  2698,  2364,  2364,  1140,  1269,  2806,
    1886,  1082,  1332,  1468,  1425,  1425,  1425,  1425,  2364, -1187,
   -1187, -1187,  2364,  2806,  2364, -1187,  1155,   -46, -1187, -1187,
    2872,  1153,  1132, -1187,   425,  3261,  2935,  4771,   540,  1142,
     868, -1187,  4771, -1187,  3685,  2364,  1325,   776,   110, -1187,
     706,  2364,  3161,   994,  1142,  1274,  4598,   142, -1187,  5288,
    4598,  1168,   182,  1280,  1282,  1283,   206,   135,   135,  1349,
      45,  4598,  1278,  3786,   433,   433,   433,  4823,  5288,  5288,
    5288,  4890,  5288,  5288,  5288,   135,   135,   934, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187,  5288, -1187,    85,
    1429, -1187, -1187,   428,   719,  1214, -1187, -1187, -1187, -1187,
   -1187,   663,  1990,  1391,  1430,   887,  5288, -1187,   832, -1187,
   -1187, -1187, -1187, -1187,   832,  1312,   620,   620, -1187, -1187,
   -1187, -1187,  2364,  2364, -1187,  1231, -1187,   533,  1388, -1187,
   -1187, -1187, -1187,  1393,  2806,  2806,  1188, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187,  1202,   345,   348,   350,  2806,
    2060,  1638,  1868,  1468,  2364,  2393,  2806,  2806,  2806,   560,
     762,  2325,  1201,  1563,    -8,  1371,  1370,  2138, -1187,  2806,
     182,  4771,  2935,   637,   925,  5288,  1206, -1187,  5288,  3035,
    3035,  1361,  1358,  1358,  1358,  1364,   964, -1187,  5288, -1187,
    1990,  1376,  1378,  1270,  1275,  1418,   832,  1369,  5288,  1245,
    1990, -1187,  1274,  1694, -1187,   631,  1247,  1248,   765, -1187,
   -1187,   680,   619,   832,   182,   182,   182,  1341,   206,   206,
     135,  5288,   778,  1229,   135,  5288,  1230,  1232,  1426,  1428,
     135,  1350,   801, -1187, -1187, -1187,  5288, -1187, -1187, -1187,
   -1187, -1187, -1187,  1413,   870, -1187,   665, -1187,  1437, -1187,
     891,  1383, -1187,  1389, -1187, -1187, -1187,   640,  1256,  1257,
    1258,  1394, -1187,   343, -1187, -1187, -1187, -1187,  1260, -1187,
     817, -1187,   620,  1286,    84,  1515, -1187, -1187, -1187,   342,
    1515,  1515, -1187,   909,  1515,  1515,   154,  1265,  1266,  1136,
   -1187,  1136, -1187, -1187,  1990,  1377, -1187, -1187,  2806,  3065,
     148,   849, -1187,  1268,   662,  1271,   677,  1273,   681,  2567,
    2806,  2806,  2806,   726,   818,  1973,  2806,  2872, -1187, -1187,
   -1187, -1187,  2806,  1371,  1433,  1435,  1339,  2806,  2806, -1187,
    1279,   741, -1187,   782,  4771,  4771,  1277,  5288,   868,   293,
    3035, -1187,  1427,  1432,  1436,  3035,  3035,  1438,  1439,  1440,
   -1187,  2364,  2364,  1402, -1187, -1187, -1187,  1357,  1274, -1187,
    2364, -1187,  1291, -1187,  1021,  1048,   142, -1187, -1187,  1316,
    1318,  1319,   835, -1187,   778,   832,   832,   182,  1410,  1411,
     555,   299,   184, -1187,  3909,  1511,  1389,   891,  2806,  2806,
    1300,  1303,   888,  5288,  3786,  1441,  1502,  5288, -1187, -1187,
     242,   314,  5288,  1509, -1187, -1187,  5288,  1543, -1187, -1187,
   -1187, -1187, -1187,  1590,  5288,  1430,  1539, -1187,  1515,  1136,
    1136, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187,  5288,  5288,  5288,   838, -1187, -1187,
   -1187,  3065, -1187,  2364, -1187,  2364, -1187,  2364, -1187,  2806,
    2872, -1187, -1187, -1187, -1187,  2806,  2872, -1187,  1339,  2806,
    1342,  1345,  1416,  2970,  1347, -1187,  1733, -1187,  4771,  1122,
   -1187, -1187,  5288,   864,  2364,  1348,   338,  3035,  3035,  3035,
   -1187, -1187,  3035,  3035,  3035,  1201,  1990,  1596,  1510,    53,
   -1187,  1990, -1187, -1187, -1187, -1187, -1187,  1128,  4598,  1167,
     619, -1187, -1187,   832,   182,   182,   199,  4598,  1451,  3786,
    1562, -1187,  1553,  1564, -1187,   865, -1187, -1187,  1353,  1503,
    1454, -1187,  1355,   874, -1187,  2872,  5288,  5288,  5063,  5115,
   -1187,  1372, -1187,  1374,  1458,  4598,  1717,  1074,  1565, -1187,
    1576,  4598,  1566,  2364,  1502,  5288, -1187,   389, -1187,   884,
   -1187,  1399,  1379,  1593, -1187,   889,   911,   921,   924,  1136,
   -1187,   385,   435,   455,  2872, -1187,  1416,  1347,  1545,  1595,
      38, -1187, -1187, -1187,  1053,  1449,  2806,  1371, -1187,   932,
   -1187,  1990,  5288,  2364,  1386,   339,   358,   397, -1187, -1187,
   -1187,  1500,  1392, -1187,  1555,  1552,  1599,  2364,  1476, -1187,
   -1187, -1187, -1187, -1187, -1187,   832,   832,  5288,   778,  1229,
     135,   933, -1187, -1187,  1659,  1717,  3909, -1187,  5288,  1495,
    1513,  1574,  1511,  1517,  2806, -1187, -1187,   936, -1187,   942,
   -1187, -1187, -1187, -1187,  5288,   135,  1548,  1454, -1187,  1447,
   -1187,  1641,   135,  1648, -1187,  2806, -1187,  1070,    39, -1187,
    1717, -1187, -1187, -1187, -1187,  5288, -1187, -1187,  1990, -1187,
   -1187,  1462,  1482, -1187,  5288, -1187,  5163,  1651,  1483, -1187,
   -1187, -1187, -1187, -1187,  1484,  1486,  1487, -1187,  1649,  2364,
   -1187,  1589, -1187, -1187, -1187, -1187, -1187, -1187, -1187,  1267,
    1180, -1187, -1187,  1339, -1187,   970,  1990,  5288,  2364,  1490,
    2364,  1491,  2364,  1492,    38,  1596,  2806,  2364,  1653,  5288,
     974,   119, -1187,   299, -1187,  1511,  1389,  1441, -1187, -1187,
   -1187,   984, -1187,  1518,   167,  5288, -1187,   875,  1503,  1601,
   -1187,  5288, -1187,  1690,   988,  1711,   992, -1187,   135,  1511,
    2806, -1187,  1504, -1187, -1187, -1187, -1187,  1611,  1615, -1187,
    1502,   229,   229, -1187, -1187, -1187,  1715,  5288, -1187, -1187,
   -1187,  2806,  1037,  1673,  1674,  1449, -1187, -1187,  1416, -1187,
     997,  1990,  5288,  1990,  5288,  1990,  5288, -1187, -1187,  1347,
    1990,  2364,  1650, -1187,   994,  1718, -1187,  1562,  1503,  1454,
    1458,  5288, -1187,   962,  1598, -1187,  1600, -1187,  1514,  1516,
    1524,  1513, -1187, -1187,   135,  5288,   994,  3387,   135, -1187,
    1389,  1503,  1525,  5288,  1640,   -90, -1187, -1187, -1187,  1544,
    1547, -1187, -1187,   906,  1001, -1187,  1004,  1532,  1022,  1023,
   -1187,  2364,  1698,  1533, -1187, -1187, -1187,  1027,  1038,  1055,
    1990,   273, -1187,  1650, -1187, -1187,  3161, -1187,  1513,  1511,
    1454, -1187, -1187, -1187, -1187, -1187, -1187, -1187,  5288,  5288,
    5288, -1187,  1549,  1762, -1187, -1187, -1187, -1187, -1187, -1187,
   -1187, -1187, -1187, -1187, -1187,  1513,  1662,  1056,  1647, -1187,
    2806,  5288,  5288, -1187,  1573,  1578,   229,  1693, -1187,  5288,
   -1187, -1187,  1201,  2806, -1187, -1187, -1187, -1187, -1187,  1625,
    1758, -1187,  1669, -1187,   -27, -1187,  1503,  1511,  1060,  1064,
    1095,  5288,  3387, -1187,  1672,   -25, -1187,  1569, -1187, -1187,
    5288,  5288, -1187,  1571,  1096,  1347, -1187,  1813,   135,  1179,
   -1187,  1513,  1503, -1187, -1187, -1187,  1099, -1187, -1187,  1360,
     403, -1187,   -25, -1187,  1664, -1187, -1187,   229, -1187, -1187,
    1579, -1187,  1636, -1187, -1187,  1513,   -25,   304,   304, -1187,
   -1187, -1187, -1187, -1187,  1102, -1187, -1187, -1187,   286, -1187,
   -1187,  1685, -1187, -1187, -1187, -1187, -1187, -1187
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,   504,     0,     0,     0,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
       4,    29,     6,     7,     8,     9,    10,    11,   280,    12,
     278,    13,    14,    15,   288,    16,    17,    18,    21,    22,
      23,    19,    20,    24,     0,   506,   505,   497,     0,   541,
     543,     0,     0,     0,     0,     0,     0,   144,   145,     0,
     146,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     693,   694,   695,   696,   697,   688,   451,   453,   452,     0,
     457,   460,   454,   458,   455,   459,   456,   461,   462,   463,
     464,   465,   466,   467,   468,   469,   470,   471,   472,   473,
     474,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   584,   720,   722,    51,     0,     0,     0,     0,     0,
       0,    79,     0,     0,    30,    40,    35,     0,     0,   488,
     631,   633,   638,     0,   636,     0,     0,   584,     0,     1,
       3,     0,     0,     0,     0,    25,    26,    27,    28,     0,
     425,   423,   424,   426,   430,     0,   385,   386,   387,     0,
     427,   428,   429,     0,   377,     0,     0,   388,     0,     0,
     495,     0,   345,   489,   313,   374,   378,   375,   376,   379,
     395,   444,   508,   584,     0,     0,   544,     0,   608,   448,
       0,     0,   446,     0,     0,   584,     0,     0,     0,     0,
       0,     0,   610,     0,   610,     0,   610,     0,     0,     0,
     689,   691,   706,   718,   719,     0,   698,   721,     0,     0,
       0,    44,     0,     0,    31,     0,     0,    32,     0,    86,
       0,     0,     0,    33,    80,    43,    77,    78,    45,    94,
      95,    46,     0,     0,     0,     0,     0,   500,     0,     0,
       0,     0,   635,   639,   637,     0,     0,     0,   279,     5,
       0,   282,     0,   284,     0,   286,     0,     0,     0,   348,
       0,     0,     0,     0,   435,     0,   391,   392,     0,     0,
     500,     0,     0,     0,     0,     0,     0,     0,     0,   700,
     701,   702,   703,   704,   705,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   485,   491,     0,     0,   389,   402,     0,
     441,   442,   396,     0,     0,   546,   586,   542,   545,     0,
       0,   508,   600,     0,   546,     0,     0,   105,     0,   447,
       0,     0,   109,     0,   584,   584,   584,     0,     0,     0,
       0,     0,     0,   611,   612,   272,     0,   273,     0,   274,
     108,     0,     0,     0,     0,     0,     0,     0,   118,   119,
     690,   707,   709,   710,   708,     0,     0,     0,   498,     0,
       0,   723,    66,    65,    67,    68,    71,    72,     0,    42,
       0,    48,    47,    87,    50,     0,    84,    85,     0,    82,
      92,    93,    49,    41,    63,    64,    56,    57,    60,    61,
      62,    38,     0,    36,    53,    55,    39,    37,    52,     0,
     591,   502,   160,     0,     0,     0,     0,     0,     0,     0,
     281,   283,   285,     0,     0,     0,     0,     0,     0,     0,
     434,   436,     0,     0,   390,   380,     0,     0,   326,   331,
     333,   335,     0,     0,   314,   315,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   327,     0,   316,   344,     0,
       0,     0,   346,   347,     0,     0,     0,     0,     0,     0,
       0,   352,   356,     0,   364,   365,   366,   367,     0,   490,
     394,   393,     0,     0,     0,   401,     0,   405,   397,   445,
     373,   440,     0,   443,   541,     0,     0,   536,     0,   509,
     510,   512,   536,   496,   492,     0,   548,     0,     0,   604,
       0,     0,     0,   546,   609,   606,     0,     0,   449,     0,
       0,     0,     0,     0,     0,     0,   584,     0,     0,     0,
       0,     0,     0,     0,   610,   610,   610,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   711,   715,
     716,   717,   713,   714,   712,   699,   692,     0,   585,     0,
      74,    69,    70,     0,     0,     0,    91,    83,    81,    58,
      59,     0,   501,     0,     0,   161,     0,   630,   626,   627,
     628,   629,   634,   632,   654,     0,   658,   658,   592,   507,
     382,   381,     0,     0,   433,     0,   383,   390,   345,   328,
     332,   334,   336,     0,     0,     0,   309,   307,   308,   305,
     306,   292,   293,   294,   295,   296,   298,   299,   300,   301,
     302,   303,   304,   297,   310,     0,     0,     0,     0,     0,
       0,   354,   358,     0,     0,     0,     0,     0,     0,     0,
       0,   360,   404,     0,   403,   411,     0,     0,   438,   441,
       0,   536,     0,     0,     0,     0,   539,   532,     0,     0,
       0,     0,   534,   534,   534,     0,     0,   528,     0,   494,
     547,     0,   550,     0,     0,     0,   589,   593,     0,     0,
     602,   601,   606,   497,   291,     0,     0,     0,     0,   613,
     450,     0,     0,   647,     0,     0,     0,     0,   584,   584,
       0,     0,   642,   255,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   147,   151,   149,     0,   150,   192,   159,
     275,   276,   277,     0,   130,   137,     0,   140,     0,   128,
       0,     0,   139,     0,   142,   120,   499,     0,     0,     0,
       0,    75,    73,    34,    88,    89,    90,    54,     0,   486,
       0,   503,   658,     0,     0,     0,   676,   675,   677,     0,
       0,     0,   686,   678,     0,     0,     0,     0,     0,     0,
     655,   662,   663,   656,   437,     0,   432,   384,     0,   329,
     337,     0,   312,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   362,     0,   350,   353,   357,
     371,   351,     0,   411,     0,     0,   412,     0,     0,   440,
       0,     0,   533,     0,   536,   536,   537,     0,   511,     0,
       0,   535,     0,     0,     0,     0,     0,     0,     0,     0,
     493,     0,     0,   552,   587,   588,   590,     0,   606,   605,
       0,   290,   607,   106,     0,     0,     0,   107,   110,     0,
       0,     0,     0,   615,   642,   651,   649,     0,     0,     0,
       0,   633,     0,   644,     0,    96,     0,     0,     0,     0,
       0,     0,   153,     0,     0,   229,   171,     0,   132,   133,
       0,     0,     0,     0,   208,   136,     0,     0,   141,   724,
     725,   726,   727,     0,     0,     0,   162,   657,     0,     0,
     661,   487,   673,   670,   669,   671,   668,   672,   674,   679,
     680,   681,   682,   685,     0,     0,     0,     0,   665,   664,
     431,   330,   311,     0,   340,     0,   338,     0,   339,     0,
     369,   355,   359,   372,   370,     0,   349,   361,   412,     0,
       0,     0,   414,   560,   406,   556,     0,   439,   536,   536,
     529,   530,     0,     0,     0,     0,     0,     0,     0,     0,
     518,   519,     0,     0,     0,   549,   551,     0,   554,   595,
     289,   603,   618,   617,   620,   619,   614,     0,     0,     0,
       0,   111,   653,   648,     0,     0,     0,     0,     0,     0,
     642,   640,     0,   263,   260,     0,   257,   259,     0,   264,
     231,   191,     0,     0,   212,   214,     0,     0,     0,     0,
     152,     0,   148,     0,   235,     0,   173,   131,     0,   125,
       0,     0,     0,     0,   171,     0,   138,     0,    76,     0,
     167,     0,     0,     0,   667,     0,     0,     0,     0,     0,
     660,     0,     0,     0,   368,   363,   414,   407,     0,     0,
       0,   398,   561,   562,     0,   563,     0,   411,   531,     0,
     540,   513,     0,     0,     0,     0,     0,     0,   520,   521,
     522,     0,   553,   420,     0,   566,     0,     0,     0,   622,
     621,   623,   625,   624,   616,   652,   650,     0,   642,   255,
       0,     0,   643,   641,     0,   173,     0,   256,     0,     0,
     268,     0,    96,   193,     0,   195,   211,     0,   209,     0,
     156,   154,   157,   155,     0,     0,     0,   231,   172,     0,
     182,     0,     0,     0,   177,     0,   185,     0,     0,   158,
     174,   175,   134,   135,   127,     0,   123,   126,   124,   122,
     129,     0,     0,   239,     0,   164,     0,   163,     0,   659,
     683,   684,   687,   666,     0,     0,     0,   399,     0,     0,
     416,   418,   415,   700,   701,   702,   703,   704,   705,   560,
       0,   558,   557,   412,   538,     0,   514,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   568,     0,
       0,     0,   594,   633,   646,    96,     0,   229,   262,   261,
     258,     0,    98,   100,     0,     0,   114,     0,   264,     0,
     213,     0,   196,     0,     0,     0,     0,   270,     0,    96,
       0,   183,     0,   178,   181,   186,   187,     0,     0,   176,
     171,     0,     0,   168,   170,   169,     0,     0,   343,   341,
     342,     0,     0,     0,   408,   563,   564,   565,   414,   523,
       0,   515,     0,   516,     0,   517,     0,   422,   421,   555,
     567,     0,   570,   597,   546,     0,   598,   642,   264,   231,
     235,     0,    97,     0,     0,   267,     0,   269,     0,     0,
       0,   268,   194,   210,     0,     0,   546,     0,     0,   230,
       0,   264,     0,     0,     0,     0,   121,   251,   242,   243,
     247,   253,   254,     0,     0,   240,     0,     0,     0,     0,
     413,     0,     0,     0,   559,   400,   524,     0,     0,     0,
     569,     0,   287,   571,   572,   596,     0,   645,   268,    96,
     231,    99,   101,   102,   103,   104,   266,   265,     0,     0,
       0,   116,     0,     0,   198,   202,   203,   204,   205,   206,
     207,   199,   201,   271,   236,   268,   179,     0,     0,   189,
       0,     0,     0,   252,   245,   249,     0,     0,   238,     0,
     165,   410,   419,     0,   417,   525,   526,   527,   575,     0,
       0,   577,   579,   573,   546,   115,   264,    96,     0,     0,
       0,     0,     0,   112,     0,   215,   190,     0,   244,   248,
       0,     0,   241,     0,     0,   409,   578,     0,     0,   581,
     599,   268,   264,   232,   233,   234,     0,   200,   180,     0,
       0,   184,   216,   217,     0,   246,   250,     0,   166,   576,
     580,   582,     0,   574,   117,   268,   215,     0,     0,   219,
     220,   221,   218,   188,     0,   583,   113,   197,     0,   226,
     225,     0,   223,   222,   237,   227,   228,   224
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1187, -1187, -1187,  1695, -1187,  1262, -1187, -1187, -1187,  1591,
    1259, -1187, -1187, -1187, -1187,  1264,  -230, -1187, -1187,  1613,
   -1079, -1187,   565, -1187,  -197, -1187, -1187,  1146, -1187, -1187,
   -1187,  1284, -1187,   851,   967, -1187, -1187,  -546,  -489,  -312,
   -1068,   694,  -991,   744, -1187,   713,  -821,  -671, -1187,   450,
   -1187,  -942,  -996, -1187,   742,   408, -1187,   423,   410,   652,
   -1077,   583,  -864, -1186,   486,   767, -1187,   758, -1187, -1187,
    -794,  -909,   449, -1187,   786,   155, -1187,     7,    26,    29,
   -1187,  1077, -1187,  -176,   -28,  -639,  1213, -1187,  -317, -1187,
   -1187,  -789,  -900, -1001,   678, -1187, -1187,   679, -1187, -1187,
   -1187,  1603,  -141,  1551,  1220, -1187,  -247,   -40,  1461,    18,
    -151,   980,   -70,  1344,  1193,  -249,  -126, -1187, -1187,  1556,
    1557,  -331,  -354,   712,  -484,    -3,  1836,  -333, -1187, -1187,
   -1187, -1187,  -932,   823,   714,   636, -1187, -1187, -1187, -1187,
     562, -1187, -1187, -1187,  -116, -1187, -1187, -1187, -1187, -1187,
   -1187,   588,  1403, -1187,  -535, -1187,  -169, -1187, -1187,  1071,
     938, -1187, -1187,  -860, -1187,   183, -1187,  -833, -1187, -1187,
   -1187, -1187, -1187,  -461,  1162,  1020,  -739, -1187,   -29, -1187,
     872,  1561, -1187,  1852,  1712
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    19,    20,    21,   124,   125,   126,   127,   413,
     414,   415,   389,   752,   238,   233,   234,   577,   402,   241,
    1009,  1211,  1212,    22,   337,    23,    24,   342,    25,    26,
     368,   369,    61,   722,   723,   724,  1020,   725,  1116,   421,
    1039,  1040,  1026,  1139,  1140,  1141,   727,   728,  1224,  1361,
     895,  1117,  1118,  1013,  1014,  1431,  1432,  1433,  1462,  1024,
    1112,  1127,   898,  1314,  1315,   875,  1005,  1006,  1007,  1105,
    1110,  1216,  1226,    27,    28,    70,    30,    71,    72,    73,
     634,   635,   298,   172,   247,   501,   174,   175,   176,   496,
    1323,   816,   952,  1061,  1172,  1254,  1082,  1083,   177,   178,
     179,   273,   274,   321,   502,   322,   180,   191,   339,   181,
     378,   760,   912,   130,   182,   379,   281,   423,    47,   325,
     509,   510,   511,   832,   960,   512,    51,   516,   682,   843,
     978,  1085,   954,   955,  1065,  1181,  1198,  1272,  1332,  1333,
    1334,  1392,  1419,  1443,   219,   518,   687,    34,   848,  1088,
    1202,   331,   332,   520,   694,   334,   355,   698,   862,   699,
     863,   592,   250,   252,    35,    74,    37,   873,    38,    39,
      40,    41,    42,   780,   781,   927,   782,    43,    75,   215,
     299,   216,    44,   112,   113
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      50,   525,   248,   399,   491,   739,   507,    31,   808,   809,
     360,  1000,  1010,   137,   194,   317,   507,  1057,   515,   173,
     819,   257,   314,   667,   948,   207,    32,   111,   677,    33,
       1,   992,   134,  1218,    49,   357,   330,   359,   210,   211,
     928,  1170,   929,  1149,   323,   183,   656,   323,  1056,    50,
    1229,   146,   195,  1004,   726,  1167,  1316,    49,   734,     1,
     736,   206,   726,   208,   741,    11,   315,   326,   186,   894,
     147,   192,   192,   148,  1369,  1119,   508,   128,   371,   343,
     202,   204,  1429,   192,   814,   553,   508,   763,     1,     2,
       3,     4,   583,    49,    11,   239,   240,   111,   260,     1,
       2,     3,     4,   382,   383,   384,   385,   283,   454,   455,
     456,   457,   458,   459,   460,   461,   462,   463,   464,   465,
    1430,   467,   468,    11,     1,   425,  1278,   480,  1225,  1237,
    1185,   269,   441,    45,    11,  1275,   783,   272,    49,     1,
     433,   434,   435,   280,   500,   696,   296,    31,    46,   442,
    1301,   254,   765,   372,   373,    29,   554,   851,   711,    11,
    1086,   489,   941,   942,   205,  1370,    32,  1102,   578,    33,
     928,   929,   138,   947,    11,   664,   697,   822,   519,  1318,
     370,   236,   374,    36,  1284,    48,  1285,    13,   661,   507,
     692,   313,   497,   344,   345,   346,     1,   351,    52,   145,
     350,   129,  1339,   328,  1238,   313,  1011,   262,   657,   766,
     767,   768,   139,     1,   769,   770,   771,   772,   773,   774,
     775,   776,   777,   778,   138,  1293,   140,   522,   533,   534,
     535,    11,  1307,  1308,  1309,  1310,   313,  1001,   111,  1012,
    1311,  1312,   212,   437,   386,  1260,   419,   316,    11,  1306,
     280,  1454,   324,     1,   685,   522,   144,  1325,   508,   508,
    1396,   213,   214,  1397,  1269,  1204,   318,   422,   319,   320,
     608,    17,   472,   473,   761,   490,   481,   482,  1183,   484,
     485,   486,   487,  1258,   428,  1004,   136,   149,    11,  1388,
       1,   256,   584,  1171,   338,    29,   514,   441,   640,  1353,
      17,   907,   243,   645,   387,  1276,  1055,   375,  1087,  1028,
    1163,  1414,  1097,   980,   239,   240,   829,   653,  1422,  1319,
    1327,   237,  1328,    36,  1329,    11,   279,   184,   388,    17,
     313,   664,  1002,   192,   422,  1389,  1286,   499,   828,   909,
      17,   961,  1279,  1277,   536,   507,   911,   313,   566,  -452,
     187,  1458,   507,   507,  1465,   546,   528,   650,   264,     1,
     429,  1367,     1,   679,     1,    17,   652,  1030,   654,   225,
     544,   330,  1031,   226,   545,   730,   731,   732,   244,   540,
      17,  1032,  1351,   923,   793,  1003,  1029,   795,   296,   797,
     541,   582,   542,   196,    11,   726,   197,    11,  1027,    11,
     964,   198,   404,   405,   406,   407,  1398,  1399,  1400,   924,
     228,   605,  1459,  1460,   886,   508,   746,    65,   582,    66,
     707,  1390,   508,   508,  1291,   436,   141,   142,   143,  1395,
    1466,   589,   199,   443,   965,  1151,  1364,    17,   789,   790,
       1,     2,     3,     4,  1337,  1073,  1188,   641,   642,   209,
     590,  1415,  1461,   591,    17,   391,  1403,  1313,  1033,  1426,
     651,   218,  1152,   799,   200,  1190,  1391,   913,   914,   915,
     807,   500,   500,   220,  1068,    11,   966,   408,   409,  1074,
    1189,   970,   971,   500,  1338,   228,   695,   680,   245,  1150,
     701,   221,   543,   690,    17,   228,  1234,   222,   853,  1191,
     246,   713,   229,   376,  1192,   377,   410,  1365,  1449,   212,
     726,   235,  1444,   507,   141,   142,   143,   804,   507,   507,
     141,   142,   143,   399,   419,   666,   420,   840,   213,   214,
     666,    17,   313,  1153,   708,   709,  1456,   849,  1193,    13,
     313,   353,   354,   411,   192,   242,   726,   700,   192,  1450,
    1451,   249,   743,   744,   251,     1,     2,     3,     4,   192,
     231,   729,   232,    67,    68,   729,   735,   729,   737,   729,
     740,   729,   742,   255,   784,   785,   748,   229,   963,   588,
    1034,   594,   266,   508,   258,   313,   267,   229,   508,   508,
      11,  1302,   868,   869,   419,   749,   270,   398,   750,   600,
      17,   275,  1421,    17,   422,    17,   141,   142,   143,   141,
     142,   143,   931,  1075,  1076,  1077,   805,  1003,  1078,  1079,
    1080,   268,   859,   763,   940,   500,   500,   412,  1445,   582,
     946,  -279,  -279,  -279,   230,   231,   500,   232,   649,   419,
     333,   953,   956,   860,  1164,   231,   327,   232,   114,   335,
     507,   507,   507,   861,    13,   507,   507,   507,   141,   142,
     143,   663,   338,   764,   114,   173,   404,   405,   406,   407,
     396,   397,   427,   686,   336,  1046,  1047,  1048,   444,   666,
    -451,    17,   184,   826,   890,   340,   499,   703,   765,   419,
     404,   405,   406,   407,  1165,   712,   313,   996,   352,   916,
     917,   918,   500,  1015,   921,   922,   313,   870,   997,   419,
     998,   876,   891,  1069,  1166,   975,   567,   882,   568,   892,
     508,   508,   508,   341,   747,   508,   508,   508,   586,   871,
     587,  1407,   419,   877,   593,   141,   142,   143,   141,   142,
     143,   408,   409,   380,   422,   766,   767,   768,   400,   401,
     769,   770,   771,   772,   773,   774,   775,   776,   777,   778,
     141,   142,   143,  1054,   601,   408,   409,   607,   574,   500,
     410,   575,   576,   953,   115,   141,   142,   143,   356,   141,
     142,   143,   358,   347,   983,   985,   419,   114,   598,  -279,
     115,   794,   796,   798,   410,   318,    69,   668,   803,     1,
       2,     3,     4,     5,     6,     7,     8,  1051,   609,  1052,
     999,  1053,   348,   349,   976,   821,   810,   823,   610,   611,
     612,   872,   981,   393,   141,   142,   143,   271,     9,   439,
     440,   416,    10,  1240,    11,   571,   572,   613,  1044,   141,
     142,   143,   666,   666,   419,   313,   599,   116,   394,   117,
     118,   119,   616,   120,   121,   122,   283,   123,   390,   864,
     865,   866,   271,   116,   603,   604,   223,   119,   225,   120,
     121,   122,   226,   123,   395,   779,   141,   142,   143,   403,
     141,   142,   143,   418,   286,   287,   288,    62,    63,    64,
     953,   412,   729,   824,   424,   296,   899,    12,    13,    14,
      15,  1021,   729,   547,   548,   729,    16,   551,   552,  1373,
     422,  1374,  1375,   115,  1036,   412,   426,  1090,   934,  1093,
     579,   580,  1041,   617,   618,   619,   620,   261,   263,   265,
     141,   142,   143,   936,   447,   448,  1071,   938,  1015,   754,
     755,  1335,   313,   313,   313,   449,   450,   451,  1091,   446,
     716,   717,   718,   361,   362,   719,   419,  1099,   606,   500,
     688,  1200,   689,  1354,   452,  1342,  1343,  1344,  1345,   670,
     671,   672,   673,   674,   683,   684,   666,   666,   888,   889,
     313,   363,   943,   453,   364,  1128,   116,   919,   920,   223,
     119,  1146,   120,   121,   122,   488,   123,   958,   469,   470,
     471,   466,   300,   301,   302,  1148,   192,   303,   304,   305,
     306,   307,   308,   309,   310,   192,   419,   729,   811,   856,
     953,   857,   993,   311,   982,   911,   670,   671,   672,   673,
     674,  1288,  1289,  1290,   729,   729,  1121,  1123,   959,   515,
      17,   675,   676,   192,   483,  1186,   430,   517,   431,   192,
     432,   984,   911,   729,   500,   884,   320,   885,   721,  1018,
    1019,  1420,   365,    53,    54,   836,    55,   837,   838,   839,
      56,   905,   419,   906,   944,   953,   621,   622,   623,   624,
     625,   626,   627,   628,   629,   630,   631,   632,   526,   990,
     729,   991,  1049,   521,  1050,    57,    58,  1206,   675,   676,
     633,   527,   300,   301,   302,    59,   530,   303,   304,   305,
     306,   307,   308,   309,   310,  1203,   366,   367,   567,  1106,
    1070,  1107,  1227,   311,   729,    76,  1213,   531,  1114,  1232,
    1115,  1089,   911,   558,   559,   560,   561,   532,  1154,   763,
    1155,  1252,   729,  1049,   529,  1159,    77,    78,   537,  1095,
    1096,  1098,  -452,  -452,  -452,  -452,  -452,  -452,  -452,  -452,
    1261,   538,  1263,   422,  1265,   567,   549,  1160,  -452,  1270,
    1092,   911,  1041,   283,  1245,   567,    80,  1161,   567,    81,
    1162,   825,  1142,  1143,    82,   330,   567,   884,  1184,  1207,
    1221,    60,  1222,   445,   500,  1382,  1221,   550,  1223,   284,
     285,   286,   287,   288,   765,   729,   283,   953,   562,   563,
     295,   539,   296,   474,   475,   476,   477,  1273,   647,   557,
    -279,  -279,  -279,   297,  1221,  1300,  1259,   555,   419,   478,
    1274,  1235,  1236,  1287,   286,   287,   288,   564,  1281,   729,
    1282,   556,  1295,  1330,  1296,   296,  1298,   570,  1299,    83,
     573,  1221,    84,  1326,   569,  1376,   297,  1377,  1376,   595,
    1378,   766,   767,   768,   665,  1041,   769,   770,   771,   772,
     773,   774,   775,   776,   777,   778,  1154,  1066,  1380,  1381,
     729,  1221,   729,  1385,   729,  1173,  1174,  1175,  1176,  1177,
    1178,  1352,  1221,  1320,  1386,  1363,   300,   301,   302,  1213,
     581,   303,   304,   305,   306,   307,   308,   309,   310,  1221,
    1221,  1387,  1405,   729,  1221,  1362,  1423,   311,  1221,   596,
    1424,   729,    85,   597,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,  1221,
    1154,  1425,  1438,  1221,   313,  1446,  1376,   131,  1464,   135,
    1062,  1063,    76,   150,   151,   152,   729,   729,   729,   153,
    1256,  1257,   154,  1441,  1442,   614,  1447,  1448,  -279,   224,
     227,   276,   277,    77,    78,   833,   834,   615,   636,  1408,
    1409,   644,   659,   637,   638,   643,   669,  1041,   155,   156,
     157,   158,  -452,  -452,  -452,  -452,  -452,  -452,  -452,  -452,
     786,   655,   658,    80,   681,  1227,    81,   693,  -452,   729,
    1362,    82,   704,   702,   705,   706,   159,   160,  1435,  1436,
     710,   714,   751,   756,   758,   759,   762,   161,   162,   788,
     163,   492,   300,   301,   302,   791,   493,   303,   304,   305,
     306,   307,   308,   309,   310,   419,   494,   282,   792,   815,
     817,   827,   830,   311,   831,   835,   841,   846,   648,   842,
     844,    76,   150,   151,   152,   845,   847,   850,   153,   854,
     855,   154,     1,   867,   874,   878,    83,   879,   880,    84,
     881,   883,    77,    78,   887,  -452,  -452,  -452,  -452,  -452,
    -452,  -452,  -452,   164,   893,   896,   897,   155,   156,   157,
     158,  -452,   900,   901,   902,   904,   903,    11,   908,   911,
     925,   926,    80,   949,   950,    81,   933,   951,   967,   935,
      82,   937,   962,   968,   165,   159,   160,   969,   957,   972,
     973,   974,   977,   166,   979,   324,   161,   162,   987,   163,
     988,   989,   994,   995,  1008,  1016,   930,   167,  1017,    85,
    1025,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   168,   169,   495,   283,
    1035,  1023,  1037,  1038,  1043,    83,  1060,  1058,    84,  1081,
    1059,  1066,  1084,  1072,  1100,   872,  1103,  1104,  1108,  1111,
    1109,  1113,   164,  1126,   171,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,  1124,   296,  1125,
    1145,  1156,   282,  1144,  1147,  1157,  1158,  1168,  1180,   297,
    1169,  1187,  1194,   165,   787,  1196,  1195,  1197,   300,   301,
     302,  1199,   166,   303,   304,   305,   306,   307,   308,   309,
     310,  1201,  1208,  1214,  1217,  1219,   167,  1215,    85,   311,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   168,   169,    76,   150,   151,
     152,  1228,  1230,  1231,   153,   302,  1246,   154,   303,   304,
     305,   306,   307,   308,   309,   310,  1233,  1241,    77,    78,
     300,   301,   302,   278,   311,   303,   304,   305,   306,   307,
     308,   309,   310,   155,   156,   157,   158,  1242,  1247,  1251,
    1248,   311,  1249,  1250,  1253,  1262,  1264,  1266,    80,  1271,
    1283,    81,  1292,  1294,  1297,  1304,    82,  1305,  1317,  1303,
     602,   159,   160,  1321,   283,  1336,  1322,  1331,  1346,  1348,
    1347,  1349,   161,   162,   801,   163,  1129,  1130,  1131,  1350,
    1132,  1366,  1368,  1371,  1133,  1134,  1372,  1379,  1383,  1384,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   282,   296,  1401,  1402,    76,   150,   151,   152,
    1404,  1406,  1410,   153,   297,  1413,   154,  1411,  1416,   813,
    1417,    83,  1418,  1428,    84,  1434,  1437,    77,    78,  1439,
    1453,   271,  1455,  1298,  1467,   259,   417,   753,   164,   392,
     757,   745,   155,   156,   157,   158,  1341,   858,  1243,  1209,
    1101,  1022,  1427,  1239,  1457,  1452,  1220,    80,  1463,  1280,
      81,  1135,  1412,  1340,  1210,    82,  1205,  1440,   932,   165,
     159,   160,  1267,   503,  1268,   438,  1136,  1137,   166,   820,
    1138,   161,   162,   585,   163,  1042,   852,   523,   188,  1182,
     524,  1324,   167,  1255,    85,  1393,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   168,   169,   170,  1394,   691,   910,   986,  1094,  1045,
      83,   217,   381,    84,   283,   302,  1179,   565,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   164,     0,   171,
       0,   646,     0,     0,   311,   282,     0,     0,     0,     0,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,     0,   296,     0,     0,     0,     0,   165,   271,
       0,     0,     0,     0,   297,     0,     0,   166,     0,  1067,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   167,     0,    85,   802,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     168,   169,    76,   150,   151,   152,     0,     0,     0,   153,
     302,     0,   154,   303,   304,   305,   306,   307,   308,   309,
     310,     0,     0,    77,    78,   300,   301,   302,   171,   311,
     303,   304,   305,   306,   307,   308,   309,   310,   155,   156,
     157,   158,     0,     0,     0,     0,   311,     0,     0,     0,
       0,     0,     0,    80,     0,     0,    81,   283,     0,     0,
       0,    82,     0,     0,     0,     0,   159,   160,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   161,   162,   945,
     163,     0,     0,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   800,   296,     0,     0,   282,
       0,     0,     0,     0,     0,     0,     0,   297,     0,     0,
       0,    76,   150,   151,   152,     0,     0,     0,   153,     0,
       0,   154,     0,     0,     0,     0,    83,     0,     0,    84,
       0,     0,    77,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   164,     0,     0,     0,   155,   156,   157,
     158,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    80,     0,     0,    81,     0,     0,     0,     0,
      82,     0,     0,     0,   165,   159,   160,     0,     0,     0,
       0,     0,     0,   166,     0,     0,   161,   162,     0,   163,
       0,     0,     0,     0,     0,   818,     0,   167,     0,    85,
       0,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   168,   169,   513,     0,
       0,   283,     0,     0,   300,    83,   302,     0,    84,   303,
     304,   305,   306,   307,   308,   309,   310,     0,     0,     0,
       0,     0,   164,     0,   171,   311,     0,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   302,
     296,     0,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   297,     0,   165,     0,     0,     0,     0,   311,     0,
       0,     0,   166,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   167,     0,    85,     0,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   168,   169,    76,   150,   151,
     152,     0,     0,     0,   153,     0,     0,   154,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    77,    78,
     300,   301,   302,   171,     0,   303,   304,   305,   306,   307,
     308,   309,   310,   155,   156,   157,   158,     0,     0,     0,
       0,   311,     0,     0,     0,     0,     0,     0,    80,     0,
     283,    81,     0,     0,     0,     0,    82,     0,     0,     0,
       0,   159,   160,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   161,   162,     0,   163,   284,   285,   286,   287,
     288,  -452,  -452,  -452,  -452,  -452,  -452,   295,   806,   296,
       0,   812,   282,     0,     0,     0,     0,     0,     0,     0,
     297,     0,     0,     0,     0,    76,   150,   151,   152,     0,
       0,     0,   153,     0,     0,   154,     0,     0,     0,     0,
       0,    83,     0,     0,    84,     0,    77,    78,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   164,     0,
       0,   155,   156,   157,   158,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    80,     0,     0,    81,
       0,     0,     0,     0,    82,     0,     0,     0,     0,   165,
     160,     0,     0,     0,     0,     0,     0,     0,   166,     0,
     161,   162,     0,   163,     0,     0,     0,   479,     0,     0,
       0,     0,   167,     0,    85,     0,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   168,   169,     0,   283,     0,     0,     0,     0,    83,
       0,     0,    84,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   164,     0,     0,   171,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   939,   296,     0,     0,   282,     0,     0,     0,
       0,     0,     0,     0,   297,     0,     0,   165,     0,     0,
       0,     0,     0,     0,     0,     0,   166,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     167,     0,    85,     0,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   168,
     169,    76,   150,   151,   152,     0,     0,     0,   153,     0,
       0,   154,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    77,    78,     0,     0,     0,   171,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   155,   156,   157,
     158,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    80,     0,     0,    81,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,   160,     0,   283,     0,
       0,     0,     0,     0,     0,     0,   161,   162,     0,   163,
       0,     0,     0,   639,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,     0,   296,     0,    76,
     150,   151,   152,     0,     0,     0,   153,     0,   297,   154,
       0,     0,     0,     0,     0,    83,     0,     0,    84,     0,
      77,    78,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   164,     0,     0,   155,   156,   157,   158,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      80,     0,     0,    81,     0,     0,     0,     0,    82,     0,
       0,     0,     0,   165,   160,     0,     0,     0,     0,     0,
       0,     0,   166,     0,   161,   162,     0,   163,     0,     0,
       0,     0,     0,     0,     0,     0,   167,     0,    85,     0,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   168,   169,     0,     0,     0,
       0,     0,     0,    83,     0,     0,    84,     0,   504,     0,
       0,   282,     0,     0,     0,     0,     0,     0,     0,     1,
     164,     0,     0,   171,     0,     0,     0,     0,     0,    77,
      78,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   165,     0,     0,    11,     0,     0,     0,     0,    80,
     166,     0,    81,     0,     0,     0,     0,    82,     0,     0,
       0,     0,     0,     0,   167,     0,    85,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   168,   169,     0,     0,     0,   504,   282,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
      78,   171,    83,  1062,  1063,    84,     0,     0,     0,     0,
       0,     0,     0,   283,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    80,
       0,     0,    81,     0,     0,     0,     0,    82,     0,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,  1064,   296,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   297,     0,     0,     0,   505,     0,     0,
       0,     0,     0,     0,  -452,    85,     0,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,    83,     0,    76,    84,     0,     0,     0,     0,
       0,   283,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    78,     0,     0,     0,
     662,     0,     0,     0,     0,     0,     0,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,     0,
     296,     0,     0,     0,     0,    80,     0,     0,    81,     0,
       0,   297,     0,    82,     0,     0,     0,   505,     0,     0,
       0,     0,     0,     0,     0,    85,     0,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,    76,     0,   283,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    78,     0,    83,     0,
     506,    84,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,     0,   296,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    80,   297,     0,    81,     0,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    85,     0,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    83,     0,
      76,    84,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    77,    78,     0,     0,     0,   329,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,     0,
       0,    80,     0,     0,    81,     0,     0,     0,     0,    82,
       0,     0,     0,     0,     0,     0,     0,     0,    77,    78,
       0,    85,     0,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    80,     0,
       0,    81,     0,     0,     0,     0,    82,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    83,     0,   660,    84,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,     0,    84,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    85,     0,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,     0,     0,     0,     0,     0,  1355,
    1356,  1357,  1358,  1359,  1360,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    85,     0,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,    76,     0,   498,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    77,    78,     0,     0,     0,     0,    76,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
      78,     0,    80,     0,     0,    81,     0,     0,     0,     0,
      82,     0,     0,   300,   301,   302,     0,     0,   303,   304,
     305,   306,   307,   308,   309,   310,     0,     0,     0,    80,
       0,     0,    81,     0,   311,     0,     0,    82,     0,     0,
     300,   301,   302,     0,     0,   303,   304,   305,   306,   307,
     308,   309,   310,     0,     0,     0,     0,     0,     0,     0,
       0,   311,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,     0,    84,    76,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     312,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      77,    78,    83,     0,     0,    84,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   678,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   715,     0,
      80,     0,     0,    81,     0,   716,   717,   718,    82,     0,
     719,     0,     0,     0,     0,     0,     0,   720,    85,     0,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,    85,     0,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,    76,    83,     0,     0,    84,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,    78,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   721,     0,     0,     0,     0,     0,     0,
       0,   715,    76,    80,     0,     0,    81,     0,   716,   717,
     718,    82,     0,   719,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,    78,     0,    85,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,    80,     0,     0,    81,     0,     0,     0,
       0,    82,     0,     0,     0,     0,     0,     0,     0,    76,
     189,     0,     0,     0,     0,     0,    83,     0,     0,    84,
       0,   132,     0,     0,     0,     0,     0,     0,     0,     0,
      77,    78,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   133,     0,     0,     0,     0,   721,     0,     0,     0,
      80,     0,     0,    81,     0,     0,    83,     0,    82,    84,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    85,
       0,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   190,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,     0,    84,     0,     0,    85,
       0,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,   189,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,    78,     0,    85,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,    80,    76,     0,    81,     0,     0,     0,
       0,    82,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    79,     0,     0,     0,     0,
       0,     0,    76,     0,     0,    80,     0,     0,    81,   193,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,    78,     0,    83,     0,     0,    84,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    80,     0,     0,    81,     0,     0,     0,
       0,    82,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    83,     0,
       0,    84,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    85,
       0,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,    83,    76,     0,    84,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   185,     0,     0,     0,     0,     0,     0,    77,    78,
       0,    85,     0,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    80,     0,
       0,    81,     0,     0,     0,     0,    82,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    85,
       0,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     0,     0,     0,     0,
       0,     0,     0,     0,   201,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,     0,     0,     0,     0,
       0,    83,     0,     0,    84,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    80,     0,     0,    81,     0,
       0,    76,   189,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    77,    78,    85,     0,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   203,    80,    76,     0,    81,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,     0,     0,    83,     0,
       0,    84,     0,     0,    77,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    80,     0,     0,    81,     0,     0,
       0,     0,    82,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,     0,    84,     0,
       0,     0,   253,     0,     0,     0,     0,     0,     0,     0,
       0,    85,     0,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     0,     0,
       0,     0,     0,     0,    76,     0,     0,    83,     0,     0,
      84,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    78,     0,    85,     0,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,    80,    76,     0,    81,     0,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    77,    78,     0,
      85,     0,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,    80,     0,     0,
      81,     0,     0,     0,     0,    82,     0,     0,     0,     0,
       0,     0,     0,    76,     0,     0,     0,     0,    83,     0,
       0,    84,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   665,    77,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   733,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    80,     0,     0,    81,     0,     0,
      83,     0,    82,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    85,     0,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     0,     0,
     738,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,     0,
      84,     0,     0,    85,     0,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    77,    78,     0,
      85,     0,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,    80,    76,     0,
      81,     0,     0,     0,     0,    82,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
      78,     0,     0,     0,     0,  1120,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,  1244,     0,    80,
       0,     0,    81,     0,     0,     0,     0,    82,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    77,    78,     0,
      83,     0,     0,    84,     0,     0,     0,  1122,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    80,     0,     0,
      81,     0,     0,     0,     0,    82,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    83,     0,     0,    84,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    85,     0,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
      83,    76,     0,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    77,    78,     0,    85,     0,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,    80,     0,     0,    81,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    85,     0,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,     0,    84,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    85,     0,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110
};

static const yytype_int16 yycheck[] =
{
       3,   334,   128,   233,   316,   551,   323,     0,   647,   648,
     207,   871,   876,    16,    54,     4,   333,   949,    45,    47,
     659,   137,   173,   507,   813,    65,     0,     9,   512,     0,
      14,   864,    14,  1112,     3,   204,   187,   206,    67,    68,
     779,     3,   781,  1034,    46,    48,    92,    46,   948,    52,
    1127,    44,    55,   874,   543,  1056,  1242,     3,   547,    14,
     549,    64,   551,    66,   553,    49,    58,   183,    50,   740,
      44,    53,    54,    44,   164,  1017,   323,   255,     3,   195,
      62,    63,   107,    65,    92,    51,   333,     3,    14,    15,
      16,    17,    55,     3,    49,     3,     4,    79,    82,    14,
      15,    16,    17,     3,     4,     5,     6,   201,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     145,   297,   298,    49,    14,   251,  1205,   303,  1124,    90,
    1072,   159,   273,    82,    49,    16,   597,   165,     3,    14,
     266,   267,   268,   171,   320,     3,   240,   140,    97,   275,
    1229,   133,    68,    78,    79,     0,   122,   692,   113,    49,
     107,   312,   801,   802,   110,   255,   140,  1000,   398,   140,
     909,   910,    17,   812,    49,   506,    34,   661,   329,  1247,
     209,     4,   107,     0,    17,    48,    19,   113,   505,   506,
     523,   173,   318,   196,   197,   198,    14,   200,    46,    44,
     110,     3,  1279,   185,   165,   187,   877,    82,   254,   125,
     126,   127,     0,    14,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,    69,  1221,   242,   254,   344,   345,
     346,    49,     3,     4,     5,     6,   218,    53,   220,   878,
      11,    12,     3,   271,   144,  1187,   254,   239,    49,  1240,
     278,  1437,   254,    14,   144,   254,   254,  1258,   505,   506,
    1339,    22,    23,  1340,  1196,  1098,   255,   249,   257,   258,
     446,   255,   300,   301,   586,   315,   304,   305,  1067,   307,
     308,   309,   310,  1183,    46,  1106,   255,   107,    49,    16,
      14,   136,   255,   255,   257,   140,   324,   438,   474,  1295,
     255,   762,    46,   479,   204,   186,   945,   232,   255,    67,
    1049,  1379,   113,   848,     3,     4,   670,   493,  1397,  1251,
    1262,   144,  1264,   140,  1266,    49,   171,   257,   228,   255,
     312,   662,   148,   315,   316,    62,   169,   319,   669,   255,
     255,   825,  1206,  1203,   347,   662,     4,   329,   377,   201,
      47,    47,   669,   670,    68,   358,   338,   483,    82,    14,
     122,  1303,    14,   514,    14,   255,   492,    53,   494,   207,
     352,   522,    58,   211,   356,   544,   545,   546,   122,   142,
     255,    67,  1291,   229,    39,   874,   144,    39,   240,    39,
     153,   419,   155,    27,    49,   884,    27,    49,   887,    49,
     107,    27,     3,     4,     5,     6,  1348,  1349,  1350,   255,
      67,   439,   108,   109,   726,   662,   567,    24,   446,    26,
     536,   148,   669,   670,  1218,   270,    98,    99,   100,  1338,
     144,   424,    32,   278,   141,    46,  1300,   255,   614,   615,
      14,    15,    16,    17,  1277,   107,   107,   475,   476,    23,
     424,  1383,   148,   424,   255,   144,  1365,   228,   144,  1401,
     488,   255,    73,   639,    26,   107,   193,   125,   126,   127,
     646,   647,   648,   254,   958,    49,   830,    78,    79,   141,
     141,   835,   836,   659,  1278,    67,   526,   515,   232,  1035,
     530,     4,   255,   521,   255,    67,  1135,   203,   695,   141,
     244,   541,   159,   254,   107,   256,   107,  1301,   105,     3,
     999,     4,  1421,   830,    98,    99,   100,   643,   835,   836,
      98,    99,   100,   753,   254,   507,   256,   678,    22,    23,
     512,   255,   514,   144,   537,   538,  1445,   688,   141,   113,
     522,   108,   109,   144,   526,   213,  1035,   529,   530,   146,
     147,   255,   555,   556,   255,    14,    15,    16,    17,   541,
     217,   543,   219,    22,    23,   547,   548,   549,   550,   551,
     552,   553,   554,   116,   602,   603,   569,   159,   827,   424,
     892,   426,   255,   830,   256,   567,   255,   159,   835,   836,
      49,  1230,   708,   709,   254,   569,   255,   254,   569,   259,
     255,   255,  1396,   255,   586,   255,    98,    99,   100,    98,
      99,   100,   788,   967,   968,   969,   644,  1106,   972,   973,
     974,   258,     3,     3,   800,   801,   802,   228,  1422,   657,
     806,    98,    99,   100,   216,   217,   812,   219,   483,   254,
     141,   817,   818,    24,   259,   217,     3,   219,     3,    67,
     967,   968,   969,    34,   113,   972,   973,   974,    98,    99,
     100,   506,   257,    43,     3,   693,     3,     4,     5,     6,
     224,   225,   256,   518,    43,   924,   925,   926,   256,   661,
     255,   255,   257,   665,    19,    67,   668,   532,    68,   254,
       3,     4,     5,     6,   259,   540,   678,   142,    81,   769,
     770,   771,   878,   879,   774,   775,   688,   710,   153,   254,
     155,   714,    47,   962,   259,   841,   254,   720,   256,    54,
     967,   968,   969,    43,   569,   972,   973,   974,   254,   711,
     256,  1370,   254,   715,   256,    98,    99,   100,    98,    99,
     100,    78,    79,   142,   726,   125,   126,   127,   214,   215,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
      98,    99,   100,   939,   256,    78,    79,   256,   219,   945,
     107,   222,   223,   949,   129,    98,    99,   100,    81,    98,
      99,   100,    81,    27,   854,   855,   254,     3,   256,   256,
     129,   636,   637,   638,   107,   255,   255,   257,   643,    14,
      15,    16,    17,    18,    19,    20,    21,   933,    68,   935,
     255,   937,    56,    57,   842,   660,   256,   662,    78,    79,
      80,    43,   850,   159,    98,    99,   100,   176,    43,   178,
     179,   144,    47,  1145,    49,     5,     6,    97,   908,    98,
      99,   100,   824,   825,   254,   827,   256,   202,     4,   204,
     205,   206,     3,   208,   209,   210,   201,   212,   142,   704,
     705,   706,   176,   202,   178,   179,   205,   206,   207,   208,
     209,   210,   211,   212,   218,   255,    98,    99,   100,   226,
      98,    99,   100,     3,   229,   230,   231,    24,    25,    26,
    1066,   228,   874,   256,   142,   240,   256,   112,   113,   114,
     115,   883,   884,    51,    52,   887,   121,    51,    52,     3,
     892,     5,     6,   129,   896,   228,   117,   987,   256,   989,
       5,     6,   904,    74,    75,    76,    77,   141,   142,   143,
      98,    99,   100,   256,    67,    68,   964,   256,  1114,   220,
     221,  1274,   924,   925,   926,    78,    79,    80,   988,   254,
      59,    60,    61,    19,    20,    64,   254,   997,   256,  1135,
     254,  1087,   256,  1296,    97,     3,     4,     5,     6,   101,
     102,   103,   104,   105,   198,   199,   958,   959,   108,   109,
     962,    47,   256,   202,    50,  1025,   202,    78,    79,   205,
     206,  1031,   208,   209,   210,   122,   212,   256,    82,    83,
      84,   255,    65,    66,    67,  1033,   988,    70,    71,    72,
      73,    74,    75,    76,    77,   997,   254,   999,   256,   254,
    1196,   256,   867,    86,     3,     4,   101,   102,   103,   104,
     105,   156,   157,   158,  1016,  1017,  1018,  1019,   256,    45,
     255,   173,   174,  1025,   255,  1073,   260,   197,   262,  1031,
     264,     3,     4,  1035,  1230,   254,   258,   256,   167,   171,
     172,  1394,   128,    24,    25,   101,    27,   103,   104,   105,
      31,   254,   254,   256,   256,  1251,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,    81,   254,
    1072,   256,   254,   232,   256,    56,    57,  1100,   173,   174,
     251,   255,    65,    66,    67,    66,    81,    70,    71,    72,
      73,    74,    75,    76,    77,  1097,   182,   183,   254,   254,
     256,   256,  1125,    86,  1106,     3,  1108,    49,   254,  1132,
     256,     3,     4,     3,     4,     5,     6,   142,   254,     3,
     256,  1169,  1124,   254,   257,   256,    24,    25,    27,   994,
     995,   996,    70,    71,    72,    73,    74,    75,    76,    77,
    1188,    27,  1190,  1145,  1192,   254,    51,   256,    86,  1197,
       3,     4,  1154,   201,  1156,   254,    54,   256,   254,    57,
     256,   256,   108,   109,    62,  1336,   254,   254,   256,   256,
     254,   152,   256,   256,  1370,  1321,   254,   129,   256,   227,
     228,   229,   230,   231,    68,  1187,   201,  1383,    78,    79,
     238,    67,   240,    70,    71,    72,    73,  1199,   136,   254,
      98,    99,   100,   251,   254,  1228,   256,   155,   254,    86,
     256,   161,   162,  1215,   229,   230,   231,   107,   254,  1221,
     256,   155,   254,  1271,   256,   240,   254,     4,   256,   127,
     206,   254,   130,   256,   255,   254,   251,   256,   254,   122,
     256,   125,   126,   127,   142,  1247,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   254,   254,   256,   256,
    1262,   254,  1264,   256,  1266,   232,   233,   234,   235,   236,
     237,  1294,   254,   256,   256,  1298,    65,    66,    67,  1281,
     254,    70,    71,    72,    73,    74,    75,    76,    77,   254,
     254,   256,   256,  1295,   254,  1297,   256,    86,   254,   123,
     256,  1303,   200,   124,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   254,
     254,   256,   256,   254,  1336,   256,   254,    13,   256,    15,
      93,    94,     3,     4,     5,     6,  1348,  1349,  1350,    10,
     190,   191,    13,   194,   195,    46,    16,    17,   256,   117,
     118,   168,   169,    24,    25,   673,   674,   203,   255,  1371,
    1372,   122,   260,   255,   255,   255,   254,  1379,    39,    40,
      41,    42,    70,    71,    72,    73,    74,    75,    76,    77,
     179,   256,   259,    54,    89,  1418,    57,   143,    86,  1401,
    1402,    62,   142,   255,   142,   142,    67,    68,  1410,  1411,
      81,   153,     3,   219,    43,     5,   124,    78,    79,    46,
      81,    82,    65,    66,    67,   257,    87,    70,    71,    72,
      73,    74,    75,    76,    77,   254,    97,    69,   256,    88,
      90,   255,   101,    86,   106,   101,    90,    49,   136,    91,
     200,     3,     4,     5,     6,   200,   107,   232,    10,   232,
     232,    13,    14,   142,   255,   255,   127,   255,    62,   130,
      62,   141,    24,    25,    81,    70,    71,    72,    73,    74,
      75,    76,    77,   144,    67,   122,   117,    39,    40,    41,
      42,    86,   256,   256,   256,   255,   122,    49,   232,     4,
     255,   255,    54,    90,    89,    57,   258,   188,   101,   258,
      62,   258,   255,   101,   175,    67,    68,   101,   259,   101,
     101,   101,   140,   184,   187,   254,    78,    79,   232,    81,
     232,   232,   142,   142,    43,   255,   179,   198,   255,   200,
      58,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   201,
      81,   150,    49,     3,    55,   127,   180,   255,   130,     3,
     255,   254,    92,   255,   153,    43,    53,    43,   255,   155,
     107,   256,   144,   155,   255,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   255,   240,   255,
      54,   232,    69,    68,    68,   256,    43,    92,   189,   251,
      45,   255,   142,   175,   256,    90,   254,    95,    65,    66,
      67,    52,   184,    70,    71,    72,    73,    74,    75,    76,
      77,   185,     3,   168,    90,   148,   198,   154,   200,    86,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,     3,     4,     5,
       6,   153,   255,    62,    10,    67,    55,    13,    70,    71,
      72,    73,    74,    75,    76,    77,    68,   255,    24,    25,
      65,    66,    67,   255,    86,    70,    71,    72,    73,    74,
      75,    76,    77,    39,    40,    41,    42,   255,   255,    90,
     256,    86,   256,   256,   155,   255,   255,   255,    54,    96,
     232,    57,   151,    63,    43,   144,    62,   142,    43,   255,
     177,    67,    68,    90,   201,    47,    92,   117,   170,   255,
     170,   255,    78,    79,   136,    81,    59,    60,    61,   255,
      63,   256,   142,   239,    67,    68,   239,   255,    90,   256,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    69,   240,   255,    43,     3,     4,     5,     6,
     148,   164,   239,    10,   251,   122,    13,   239,   193,   256,
      62,   127,   153,   151,   130,   256,   255,    24,    25,    16,
     166,   176,   196,   254,   149,   140,   245,   573,   144,   226,
     581,   557,    39,    40,    41,    42,  1281,   701,  1154,  1105,
     999,   884,  1402,  1140,  1446,  1432,  1114,    54,  1448,  1207,
      57,   144,  1376,  1280,  1106,    62,  1099,  1418,   791,   175,
      67,    68,  1194,   322,  1195,   272,   159,   160,   184,   659,
     163,    78,    79,   422,    81,   905,   693,   331,    52,  1066,
     333,  1255,   198,  1179,   200,  1333,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,  1336,   522,   764,   856,   990,   909,
     127,    79,   220,   130,   201,    67,  1064,   376,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,   144,    -1,   255,
      -1,    65,    -1,    -1,    86,    69,    -1,    -1,    -1,    -1,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,    -1,    -1,    -1,    -1,   175,   176,
      -1,    -1,    -1,    -1,   251,    -1,    -1,   184,    -1,   256,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,   200,   136,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,     3,     4,     5,     6,    -1,    -1,    -1,    10,
      67,    -1,    13,    70,    71,    72,    73,    74,    75,    76,
      77,    -1,    -1,    24,    25,    65,    66,    67,   255,    86,
      70,    71,    72,    73,    74,    75,    76,    77,    39,    40,
      41,    42,    -1,    -1,    -1,    -1,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    54,    -1,    -1,    57,   201,    -1,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    79,   136,
      81,    -1,    -1,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,   237,   238,    65,   240,    -1,    -1,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   251,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,
      -1,    -1,    24,    25,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   144,    -1,    -1,    -1,    39,    40,    41,
      42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    -1,   175,    67,    68,    -1,    -1,    -1,
      -1,    -1,    -1,   184,    -1,    -1,    78,    79,    -1,    81,
      -1,    -1,    -1,    -1,    -1,    87,    -1,   198,    -1,   200,
      -1,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
      -1,   201,    -1,    -1,    65,   127,    67,    -1,   130,    70,
      71,    72,    73,    74,    75,    76,    77,    -1,    -1,    -1,
      -1,    -1,   144,    -1,   255,    86,    -1,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,    67,
     240,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,   251,    -1,   175,    -1,    -1,    -1,    -1,    86,    -1,
      -1,    -1,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,   200,    -1,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,     3,     4,     5,
       6,    -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      65,    66,    67,   255,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    39,    40,    41,    42,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    54,    -1,
     201,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    78,    79,    -1,    81,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,    65,   240,
      -1,   136,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     251,    -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    -1,
      -1,   127,    -1,    -1,   130,    -1,    24,    25,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,    -1,
      -1,    39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    54,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,   175,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   184,    -1,
      78,    79,    -1,    81,    -1,    -1,    -1,    85,    -1,    -1,
      -1,    -1,   198,    -1,   200,    -1,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,   201,    -1,    -1,    -1,    -1,   127,
      -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   144,    -1,    -1,   255,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    65,   240,    -1,    -1,    69,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   251,    -1,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   184,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     198,    -1,   200,    -1,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,     3,     4,     5,     6,    -1,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,    -1,    -1,   255,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    41,
      42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    68,    -1,   201,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    -1,    81,
      -1,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,    -1,     3,
       4,     5,     6,    -1,    -1,    -1,    10,    -1,   251,    13,
      -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,    -1,
      24,    25,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   144,    -1,    -1,    39,    40,    41,    42,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    -1,
      -1,    -1,    -1,   175,    68,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   184,    -1,    78,    79,    -1,    81,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,   200,    -1,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,   127,    -1,    -1,   130,    -1,     3,    -1,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,
     144,    -1,    -1,   255,    -1,    -1,    -1,    -1,    -1,    24,
      25,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   175,    -1,    -1,    49,    -1,    -1,    -1,    -1,    54,
     184,    -1,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,   198,    -1,   200,    -1,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,     3,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,   255,   127,    93,    94,   130,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   201,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    -1,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   141,   240,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   251,    -1,    -1,    -1,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    69,   200,    -1,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   127,    -1,     3,   130,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    -1,
     255,    -1,    -1,    -1,    -1,    -1,    -1,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,    -1,
     240,    -1,    -1,    -1,    -1,    54,    -1,    -1,    57,    -1,
      -1,   251,    -1,    62,    -1,    -1,    -1,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,    -1,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,    -1,    -1,     3,    -1,   201,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,   127,    -1,
     255,   130,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,   251,    -1,    57,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,    -1,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   127,    -1,
       3,   130,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    24,    25,    -1,    -1,    -1,   255,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,    -1,
      -1,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      -1,   200,    -1,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,    -1,   255,   130,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   127,    -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,    -1,   202,
     203,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   226,    -1,    -1,    -1,    -1,    -1,   232,
     233,   234,   235,   236,   237,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   200,    -1,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,     3,    -1,   229,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,    -1,    -1,    -1,     3,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,    -1,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    65,    66,    67,    -1,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    -1,    54,
      -1,    -1,    57,    -1,    86,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,     3,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,   127,    -1,    -1,   130,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,    -1,
      54,    -1,    -1,    57,    -1,    59,    60,    61,    62,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    71,   200,    -1,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   200,    -1,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,     3,   127,    -1,    -1,   130,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    52,     3,    54,    -1,    -1,    57,    -1,    59,    60,
      61,    62,    -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    -1,   200,    -1,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,
      -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,   167,    -1,    -1,    -1,
      54,    -1,    -1,    57,    -1,    -1,   127,    -1,    62,   130,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
      -1,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   127,    -1,    -1,   130,    -1,    -1,   200,
      -1,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    -1,   200,    -1,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,    54,     3,    -1,    57,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    -1,     3,    -1,    -1,    54,    -1,    -1,    57,   110,
      -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    -1,   127,    -1,    -1,   130,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,
      -1,   130,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
      -1,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   127,     3,    -1,   130,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      -1,   200,    -1,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
      -1,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,    -1,    -1,    -1,
      -1,   127,    -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    -1,    57,    -1,
      -1,     3,     4,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,   200,    -1,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   110,    54,     3,    -1,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,
      -1,   130,    -1,    -1,    24,    25,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,    -1,
      -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,    -1,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,   127,    -1,    -1,
     130,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,   200,    -1,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,    54,     3,    -1,    57,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,
     200,    -1,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,    -1,    -1,   127,    -1,
      -1,   130,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    24,    25,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   110,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    54,    -1,    -1,    57,    -1,    -1,
     127,    -1,    62,   130,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,    -1,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,    -1,    -1,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,
     130,    -1,    -1,   200,    -1,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,
     200,    -1,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,    54,     3,    -1,
      57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,
     127,    -1,    -1,   130,    -1,    -1,    -1,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   127,    -1,    -1,   130,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,    -1,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     127,     3,    -1,   130,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,   200,    -1,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,    -1,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,   130,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,    -1,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    14,    15,    16,    17,    18,    19,    20,    21,    43,
      47,    49,   112,   113,   114,   115,   121,   255,   262,   263,
     264,   265,   284,   286,   287,   289,   290,   334,   335,   336,
     337,   338,   339,   340,   408,   425,   426,   427,   429,   430,
     431,   432,   433,   438,   443,    82,    97,   379,    48,     3,
     386,   387,    46,    24,    25,    27,    31,    56,    57,    66,
     152,   293,    24,    25,    26,    24,    26,    22,    23,   255,
     336,   338,   339,   340,   426,   439,     3,    24,    25,    44,
      54,    57,    62,   127,   130,   200,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   370,   444,   445,     3,   129,   202,   204,   205,   206,
     208,   209,   210,   212,   266,   267,   268,   269,   255,     3,
     374,   374,    82,   112,   370,   374,   255,   386,   336,     0,
     242,    98,    99,   100,   254,   336,   338,   339,   340,   107,
       4,     5,     6,    10,    13,    39,    40,    41,    42,    67,
      68,    78,    79,    81,   144,   175,   184,   198,   227,   228,
     229,   255,   344,   345,   347,   348,   349,   359,   360,   361,
     367,   370,   375,   386,   257,   142,   370,    47,   387,     4,
     110,   368,   370,   110,   368,   386,    27,    27,    27,    32,
      26,   110,   370,   110,   370,   110,   386,   368,   386,    23,
     439,   439,     3,    22,    23,   440,   442,   444,   255,   405,
     254,     4,   203,   205,   266,   207,   211,   266,    67,   159,
     216,   217,   219,   276,   277,     4,     4,   144,   275,     3,
       4,   280,   213,    46,   122,   232,   244,   345,   377,   255,
     423,   255,   424,    82,   370,   116,   336,   405,   256,   264,
      82,   335,    82,   335,    82,   335,   255,   255,   258,   345,
     255,   176,   345,   362,   363,   255,   347,   347,   255,   336,
     345,   377,    69,   201,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   240,   251,   343,   441,
      65,    66,    67,    70,    71,    72,    73,    74,    75,    76,
      77,    86,   142,   370,   371,    58,   239,     4,   255,   257,
     258,   364,   366,    46,   254,   380,   405,     3,   370,   255,
     371,   412,   413,   141,   416,    67,    43,   285,   257,   369,
      67,    43,   288,   405,   386,   386,   386,    27,    56,    57,
     110,   386,    81,   108,   109,   417,    81,   417,    81,   417,
     285,    19,    20,    47,    50,   128,   182,   183,   291,   292,
     439,     3,    78,    79,   107,   232,   254,   256,   371,   376,
     142,   445,     3,     4,     5,     6,   144,   204,   228,   273,
     142,   144,   280,   159,     4,   218,   224,   225,   254,   277,
     214,   215,   279,   226,     3,     4,     5,     6,    78,    79,
     107,   144,   228,   270,   271,   272,   144,   270,     3,   254,
     256,   300,   370,   378,   142,   377,   117,   256,    46,   122,
     335,   335,   335,   377,   377,   377,   336,   345,   362,   178,
     179,   363,   377,   336,   256,   256,   254,    67,    68,    78,
      79,    80,    97,   202,   344,   344,   344,   344,   344,   344,
     344,   344,   344,   344,   344,   344,   255,   344,   344,    82,
      83,    84,   345,   345,    70,    71,    72,    73,    86,    85,
     344,   345,   345,   255,   345,   345,   345,   345,   122,   371,
     368,   300,    82,    87,    97,   229,   350,   377,   229,   370,
     344,   346,   365,   364,     3,   192,   255,   349,   367,   381,
     382,   383,   386,   229,   345,    45,   388,   197,   406,   371,
     414,   232,   254,   380,   381,   388,    81,   255,   370,   257,
      81,    49,   142,   405,   405,   405,   386,    27,    27,    67,
     142,   153,   155,   255,   370,   370,   386,    51,    52,    51,
     129,    51,    52,    51,   122,   155,   155,   254,     3,     4,
       5,     6,    78,    79,   107,   442,   439,   254,   256,   255,
       4,     5,     6,   206,   219,   222,   223,   278,   277,     5,
       6,   254,   345,    55,   255,   369,   254,   256,   336,   338,
     339,   340,   422,   256,   336,   122,   123,   124,   256,   256,
     259,   256,   177,   178,   179,   345,   256,   256,   344,    68,
      78,    79,    80,    97,    46,   203,     3,    74,    75,    76,
      77,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   251,   341,   342,   255,   255,   255,    85,
     344,   345,   345,   255,   122,   344,    65,   136,   136,   336,
     377,   345,   377,   344,   377,   256,    92,   254,   259,   260,
     255,   349,   255,   336,   382,   142,   370,   385,   257,   254,
     101,   102,   103,   104,   105,   173,   174,   385,   142,   371,
     345,    89,   389,   198,   199,   144,   336,   407,   254,   256,
     345,   413,   388,   143,   415,   368,     3,    34,   418,   420,
     370,   368,   255,   336,   142,   142,   142,   405,   386,   386,
      81,   113,   336,   368,   153,    52,    59,    60,    61,    64,
      71,   167,   294,   295,   296,   298,   299,   307,   308,   370,
     417,   417,   417,   110,   299,   370,   299,   370,   110,   298,
     370,   299,   370,   386,   386,   292,   371,   336,   338,   339,
     340,     3,   274,   276,   220,   221,   219,   271,    43,     5,
     372,   300,   124,     3,    43,    68,   125,   126,   127,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   255,
     434,   435,   437,   434,   345,   345,   179,   256,    46,   344,
     344,   257,   256,    39,   336,    39,   336,    39,   336,   344,
      65,   136,   136,   336,   377,   345,    65,   344,   346,   346,
     256,   256,   136,   256,    92,    88,   352,    90,    87,   346,
     365,   336,   385,   336,   256,   256,   370,   255,   382,   383,
     101,   106,   384,   384,   384,   101,   101,   103,   104,   105,
     371,    90,    91,   390,   200,   200,    49,   107,   409,   371,
     232,   415,   375,   285,   232,   232,   254,   256,   288,     3,
      24,    34,   419,   421,   336,   336,   336,   142,   405,   405,
     386,   370,    43,   428,   255,   326,   386,   370,   255,   255,
      62,    62,   386,   141,   254,   256,   300,    81,   108,   109,
      19,    47,    54,    67,   308,   311,   122,   117,   323,   256,
     256,   256,   256,   122,   255,   254,   256,   434,   232,   255,
     435,     4,   373,   125,   126,   127,   373,   373,   373,    78,
      79,   373,   373,   229,   255,   255,   255,   436,   437,   437,
     179,   344,   342,   258,   256,   258,   256,   258,   256,    65,
     344,   346,   346,   256,   256,   136,   344,   346,   352,    90,
      89,   188,   353,   344,   393,   394,   344,   259,   256,   256,
     385,   385,   255,   376,   107,   141,   383,   101,   101,   101,
     383,   383,   101,   101,   101,   377,   345,   140,   391,   187,
     415,   345,     3,   373,     3,   373,   420,   232,   232,   232,
     254,   256,   428,   336,   142,   142,   142,   153,   155,   255,
     424,    53,   148,   299,   307,   327,   328,   329,    43,   281,
     323,   308,   346,   314,   315,   344,   255,   255,   171,   172,
     297,   370,   295,   150,   320,    58,   303,   299,    67,   144,
      53,    58,    67,   144,   300,    81,   370,    49,     3,   301,
     302,   370,   372,    55,   373,   436,   376,   376,   376,   254,
     256,   377,   377,   377,   344,   346,   353,   393,   255,   255,
     180,   354,    93,    94,   141,   395,   254,   256,   385,   376,
     256,   345,   255,   107,   141,   383,   383,   383,   383,   383,
     383,     3,   357,   358,    92,   392,   107,   255,   410,     3,
     373,   368,     3,   373,   421,   336,   336,   113,   336,   368,
     153,   294,   428,    53,    43,   330,   254,   256,   255,   107,
     331,   155,   321,   256,   254,   256,   299,   312,   313,   312,
      82,   370,    82,   370,   255,   255,   155,   322,   368,    59,
      60,    61,    63,    67,    68,   144,   159,   160,   163,   304,
     305,   306,   108,   109,    68,    54,   368,    68,   345,   303,
     298,    46,    73,   144,   254,   256,   232,   256,    43,   256,
     256,   256,   256,   437,   259,   259,   259,   354,    92,    45,
       3,   255,   355,   232,   233,   234,   235,   236,   237,   441,
     189,   396,   394,   352,   256,   312,   345,   255,   107,   141,
     107,   141,   107,   141,   142,   254,    90,    95,   397,    52,
     377,   185,   411,   370,   428,   326,   386,   256,     3,   304,
     328,   282,   283,   370,   168,   154,   332,    90,   281,   148,
     315,   254,   256,   256,   309,   313,   333,   386,   153,   321,
     255,    62,   386,    68,   346,   161,   162,    90,   165,   306,
     300,   255,   255,   302,     4,   370,    55,   255,   256,   256,
     256,    90,   345,   155,   356,   395,   190,   191,   353,   256,
     312,   345,   255,   345,   255,   345,   255,   355,   358,   393,
     345,    96,   398,   370,   256,    16,   186,   424,   281,   323,
     320,   254,   256,   232,    17,    19,   169,   370,   156,   157,
     158,   331,   151,   313,    63,   254,   256,    43,   254,   256,
     386,   281,   346,   255,   144,   142,   303,     3,     4,     5,
       6,    11,    12,   228,   324,   325,   324,    43,   301,   393,
     256,    90,    92,   351,   396,   354,   256,   312,   312,   312,
     345,   117,   399,   400,   401,   388,    47,   428,   331,   321,
     322,   283,     3,     4,     5,     6,   170,   170,   255,   255,
     255,   332,   386,   313,   388,   232,   233,   234,   235,   236,
     237,   310,   370,   386,   323,   331,   256,   312,   142,   164,
     255,   239,   239,     3,     5,     6,   254,   256,   256,   255,
     256,   256,   377,    90,   256,   256,   256,   256,    16,    62,
     148,   193,   402,   401,   412,   332,   281,   321,   312,   312,
     312,   255,    43,   332,   148,   256,   164,   346,   370,   370,
     239,   239,   325,   122,   301,   393,   193,    62,   153,   403,
     388,   331,   281,   256,   256,   256,   312,   310,   151,   107,
     145,   316,   317,   318,   256,   370,   370,   255,   256,    16,
     333,   194,   195,   404,   332,   331,   256,    16,    17,   105,
     146,   147,   318,   166,   324,   196,   332,   316,    47,   108,
     109,   148,   319,   319,   256,    68,   144,   149
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   261,   262,   262,   263,   263,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     265,   265,   265,   266,   266,   266,   267,   267,   267,   267,
     268,   268,   268,   268,   268,   268,   268,   268,   268,   268,
     268,   269,   269,   270,   270,   271,   271,   271,   271,   271,
     272,   272,   272,   272,   272,   273,   273,   273,   273,   273,
     273,   273,   273,   273,   274,   274,   274,   275,   275,   275,
     276,   276,   276,   277,   277,   277,   277,   277,   278,   278,
     278,   278,   279,   279,   280,   280,   281,   281,   282,   282,
     283,   283,   283,   283,   283,   284,   284,   285,   286,   287,
     287,   288,   289,   289,   289,   289,   289,   289,   290,   291,
     291,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   293,   293,   293,   293,   294,   294,   295,
     295,   295,   296,   297,   297,   297,   297,   297,   298,   299,
     300,   300,   300,   300,   300,   300,   300,   301,   301,   302,
     302,   303,   303,   304,   304,   305,   305,   306,   306,   306,
     306,   306,   306,   306,   306,   306,   306,   306,   306,   306,
     306,   307,   307,   308,   308,   308,   308,   308,   308,   309,
     309,   310,   310,   310,   310,   310,   310,   310,   311,   312,
     312,   313,   314,   314,   315,   316,   316,   317,   317,   318,
     318,   318,   318,   318,   319,   319,   319,   319,   319,   320,
     320,   321,   321,   321,   321,   322,   322,   323,   323,   323,
     324,   324,   325,   325,   325,   325,   325,   325,   325,   325,
     325,   325,   325,   325,   325,   326,   326,   327,   327,   328,
     328,   329,   330,   330,   331,   331,   331,   331,   332,   332,
     333,   333,   334,   334,   334,   334,   334,   334,   335,   335,
     336,   336,   336,   336,   336,   336,   336,   337,   337,   338,
     339,   340,   341,   341,   341,   341,   341,   341,   341,   341,
     341,   341,   341,   341,   341,   341,   341,   341,   341,   342,
     342,   342,   343,   344,   344,   344,   344,   344,   344,   344,
     344,   344,   344,   344,   344,   344,   344,   344,   344,   344,
     344,   344,   344,   344,   344,   344,   344,   344,   344,   344,
     344,   344,   344,   344,   344,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   346,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   348,   348,   348,   349,   349,
     349,   350,   350,   350,   350,   350,   350,   350,   351,   351,
     352,   352,   353,   353,   354,   354,   355,   355,   356,   356,
     357,   357,   358,   359,   359,   359,   359,   359,   359,   359,
     360,   361,   361,   361,   361,   362,   362,   363,   364,   364,
     365,   365,   366,   366,   367,   367,   368,   368,   368,   369,
     369,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   370,   370,   370,   371,   372,   373,   374,   375,
     375,   375,   375,   375,   375,   375,   375,   375,   376,   376,
     377,   377,   378,   378,   379,   379,   379,   379,   380,   380,
     381,   381,   382,   382,   382,   382,   382,   382,   382,   382,
     382,   382,   382,   382,   382,   382,   382,   382,   383,   383,
     383,   383,   383,   383,   384,   384,   385,   385,   385,   385,
     385,   386,   386,   387,   387,   387,   388,   388,   389,   389,
     390,   390,   391,   391,   392,   392,   393,   393,   394,   394,
     395,   395,   395,   396,   396,   396,   397,   397,   398,   398,
     399,   399,   400,   400,   401,   402,   402,   402,   402,   403,
     403,   404,   404,   404,   405,   405,   406,   406,   406,   407,
     407,   408,   408,   409,   409,   410,   410,   410,   411,   411,
     412,   412,   413,   413,   414,   414,   415,   415,   416,   416,
     417,   417,   417,   418,   418,   419,   419,   420,   420,   420,
     420,   421,   421,   421,   421,   421,   422,   422,   422,   422,
     423,   423,   424,   424,   425,   426,   427,   427,   427,   427,
     428,   428,   428,   429,   429,   429,   429,   430,   430,   430,
     430,   430,   430,   431,   432,   433,   433,   433,   434,   434,
     434,   434,   434,   435,   435,   436,   436,   437,   437,   437,
     437,   437,   437,   437,   437,   437,   437,   437,   437,   437,
     437,   437,   437,   437,   437,   437,   437,   437,   438,   438,
     438,   438,   438,   439,   439,   439,   439,   439,   440,   440,
     441,   441,   441,   441,   441,   441,   442,   442,   442,   442,
     442,   442,   442,   442,   442,   442,   442,   442,   442,   442,
     443,   443,   444,   444,   445,   445,   445,   445
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     2,     2,     1,
       2,     3,     3,     2,     5,     1,     3,     3,     3,     3,
       1,     3,     3,     2,     2,     2,     2,     3,     3,     3,
       3,     1,     3,     1,     3,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     1,     1,     3,     0,     1,     3,     1,     1,     0,
       1,     3,     2,     3,     2,     2,     1,     2,     2,     2,
       2,     1,     1,     1,     1,     1,     0,     4,     1,     3,
       1,     3,     3,     3,     3,     4,     7,     4,     4,     4,
       7,     5,    13,    16,    10,    13,    12,    15,     4,     1,
       3,     8,     6,     6,     6,     5,     6,     6,     3,     6,
       3,     5,     4,     4,     6,     6,     4,     3,     5,     3,
       3,     4,     3,     0,     1,     1,     1,     1,     3,     1,
       1,     1,     3,     0,     2,     2,     2,     2,     4,     1,
       1,     2,     4,     6,     6,     9,    11,     1,     3,     3,
       3,     0,     2,     0,     1,     1,     2,     1,     2,     4,
       6,     2,     1,     2,     6,     1,     2,     2,     7,     4,
       5,     3,     1,     4,     6,     4,     5,    11,     7,     3,
       5,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     3,     1,     0,     1,     1,     2,     2,
       2,     2,     3,     3,     2,     1,     1,     2,     2,     0,
       4,     0,     6,     6,     6,     0,     4,    10,     6,     3,
       1,     3,     1,     1,     3,     2,     4,     1,     3,     2,
       4,     1,     2,     1,     1,     0,     3,     1,     3,     1,
       1,     3,     2,     0,     0,     4,     4,     3,     0,     2,
       1,     3,     4,     4,     4,     6,     6,     6,     1,     3,
       1,     4,     3,     4,     3,     4,     3,    12,     1,     8,
       7,     6,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     4,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     5,
       6,     3,     4,     3,     4,     3,     4,     5,     6,     6,
       6,     9,     9,     9,     3,     1,     3,     3,     2,     6,
       5,     5,     3,     5,     4,     6,     3,     5,     4,     6,
       4,     6,     5,     7,     3,     3,     3,     3,     7,     6,
       6,     5,     6,     1,     1,     1,     1,     1,     1,     1,
       3,     4,     4,     4,     5,     1,     1,     1,     1,     2,
       3,     2,     2,     3,     3,     1,     2,     3,     7,     8,
      10,     1,     0,     2,     2,     1,     4,     5,     0,     3,
       7,     0,     0,     5,     0,     2,     1,     4,     0,     3,
       1,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     6,     5,     4,     3,     1,     2,     4,     3,     5,
       1,     0,     1,     2,     1,     3,     1,     2,     1,     2,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     2,     3,     5,     4,     1,     3,     0,     1,     3,
       1,     3,     1,     3,     0,     1,     1,     5,     0,     2,
       1,     3,     1,     5,     6,     7,     7,     7,     4,     4,
       5,     5,     5,     7,     8,     9,     9,     9,     2,     4,
       4,     5,     2,     3,     0,     1,     0,     2,     5,     1,
       4,     1,     3,     1,     2,     3,     0,     2,     0,     3,
       0,     2,     0,     2,     0,     3,     1,     3,     3,     5,
       0,     1,     1,     0,     2,     2,     0,     2,     0,     2,
       0,     1,     1,     2,     4,     1,     3,     1,     2,     0,
       2,     0,     1,     2,     0,     3,     0,     3,     3,     1,
       2,     4,     5,     0,     4,     0,     4,     3,     2,     5,
       1,     3,     3,     5,     1,     3,     0,     2,     0,     2,
       0,     1,     1,     1,     3,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     1,
       3,     0,     3,     0,     5,     3,     2,     3,     2,     3,
       2,     3,     0,     9,     7,    12,    10,     6,     8,     7,
       9,     7,     9,     8,     5,     6,     6,     7,     0,     4,
       3,     2,     1,     1,     2,     1,     3,     3,     2,     2,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     2,
       2,     2,     2,     4,     4,     2,     1,     4,     2,     3,
       4,     3,     5,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       2,     3,     1,     3,     6,     6,     6,     6
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
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if TECC_DEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined TECC_LTYPE_IS_TRIVIAL && TECC_LTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
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
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !TECC_DEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !TECC_DEBUG */


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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined TECC_LTYPE_IS_TRIVIAL && TECC_LTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

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

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
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
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
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
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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
      yychar = yylex ();
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
  *++yylsp = yylloc;
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

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 405 "tecc_grammar.y" /* yacc.c:1646  */
    {
        tecc_parsetree = (yyvsp[0].list);
        if (tecc_parsetree)
            tecc_parsetree->type = FE_T_List;
        /* Clear the semantic value to prevent Bison from freeing it */
        (yyvsp[0].list) = NULL;
    }
#line 3654 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 3:
#line 412 "tecc_grammar.y" /* yacc.c:1646  */
    {
        tecc_parsetree = (yyvsp[-1].list);
        if (tecc_parsetree)
            tecc_parsetree->type = FE_T_List;
        /* Clear the semantic value to prevent Bison from freeing it */
        (yyvsp[-1].list) = NULL;
    }
#line 3666 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 4:
#line 422 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = list_make1((yyvsp[0].node));
    }
#line 3674 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 5:
#line 425 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node));
    }
#line 3682 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 6:
#line 431 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3690 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 7:
#line 434 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3698 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 8:
#line 437 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3706 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 9:
#line 440 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3714 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 10:
#line 443 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3722 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 11:
#line 446 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3730 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 12:
#line 449 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3738 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 13:
#line 452 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3746 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 14:
#line 455 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3754 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 15:
#line 458 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3762 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 16:
#line 461 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3770 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 17:
#line 464 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3778 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 18:
#line 467 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3786 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 19:
#line 470 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3794 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 20:
#line 473 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3802 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 21:
#line 476 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3810 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 22:
#line 479 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3818 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 23:
#line 482 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3826 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 24:
#line 485 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3834 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 25:
#line 488 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - Fix type confusion: select_stmt may return
         * SetOperationStmt (for UNION/INTERSECT/EXCEPT), not SelectStmt.
         * Traversing to the leftmost SelectStmt avoids heap overflow. */
        Node *n = (yyvsp[0].node);
        if (n != NULL && nodeTag(n) == FE_T_SelectStmt) {
            ((SelectStmt *)n)->cteList = (List *)(yyvsp[-1].node);
        } else if (n != NULL && nodeTag(n) == FE_T_SetOp) {
            SetOperationStmt *setop = (SetOperationStmt *)n;
            Node *left = setop->larg;
            while (left != NULL && nodeTag(left) == FE_T_SetOp) {
                left = ((SetOperationStmt *)left)->larg;
            }
            if (left != NULL && nodeTag(left) == FE_T_SelectStmt) {
                ((SelectStmt *)left)->cteList = (List *)(yyvsp[-1].node);
            }
        }
        (yyval.node) = n;
    }
#line 3858 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 26:
#line 507 "tecc_grammar.y" /* yacc.c:1646  */
    {
        InsertStmt *n = (InsertStmt *)(yyvsp[0].node);
        n->withClause = (List *)(yyvsp[-1].node);
        (yyval.node) = (Node *)n;
    }
#line 3868 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 27:
#line 512 "tecc_grammar.y" /* yacc.c:1646  */
    {
        UpdateStmt *n = (UpdateStmt *)(yyvsp[0].node);
        n->withClause = (List *)(yyvsp[-1].node);
        (yyval.node) = (Node *)n;
    }
#line 3878 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 28:
#line 517 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DeleteStmt *n = (DeleteStmt *)(yyvsp[0].node);
        n->withClause = (List *)(yyvsp[-1].node);
        (yyval.node) = (Node *)n;
    }
#line 3888 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 29:
#line 522 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 3896 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 30:
#line 534 "tecc_grammar.y" /* yacc.c:1646  */
    {
        SetStmt *n = (SetStmt *) (yyvsp[0].node);
        n->is_local = false;
        (yyval.node) = (Node *) n;
    }
#line 3906 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 31:
#line 539 "tecc_grammar.y" /* yacc.c:1646  */
    {
        SetStmt *n = (SetStmt *) (yyvsp[0].node);
        n->is_local = true;
        (yyval.node) = (Node *) n;
    }
#line 3916 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 32:
#line 544 "tecc_grammar.y" /* yacc.c:1646  */
    {
        SetStmt *n = (SetStmt *) (yyvsp[0].node);
        n->is_local = false;
        (yyval.node) = (Node *) n;
    }
#line 3926 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 33:
#line 552 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt(NULL, false, (yylsp[-1]).first_line, TECC_SET_MULTI, NIL);
    }
#line 3934 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 34:
#line 555 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt(NULL, false, (yylsp[-4]).first_line, TECC_SET_MULTI, NIL);
    }
#line 3942 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 36:
#line 562 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt((yyvsp[-2].str), false, (yylsp[-2]).first_line, TECC_SET_VALUE, (yyvsp[0].list));
    }
#line 3950 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 37:
#line 565 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt((yyvsp[-2].str), false, (yylsp[-2]).first_line, TECC_SET_VALUE, (yyvsp[0].list));
    }
#line 3958 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 38:
#line 568 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt((yyvsp[-2].str), false, (yylsp[-2]).first_line, TECC_SET_DEFAULT, NIL);
    }
#line 3966 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 39:
#line 571 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt((yyvsp[-2].str), false, (yylsp[-2]).first_line, TECC_SET_DEFAULT, NIL);
    }
#line 3974 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 40:
#line 577 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 3980 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 41:
#line 578 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt((yyvsp[-2].str), false, (yylsp[-2]).first_line, TECC_SET_CURRENT, NIL);
    }
#line 3988 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 42:
#line 581 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("timezone", false, (yylsp[-2]).first_line, TECC_SET_MULTI, NIL);
    }
#line 3996 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 43:
#line 584 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt(NULL, false, (yylsp[-1]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4004 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 44:
#line 587 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("search_path", false, (yylsp[-1]).first_line, TECC_SET_VALUE,
                                  list_make1(makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0)));
    }
#line 4013 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 45:
#line 591 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("client_encoding", false, (yylsp[-1]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4021 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 46:
#line 594 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("role", false, (yylsp[-1]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4029 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 47:
#line 597 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("session_authorization", false, (yylsp[-2]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4037 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 48:
#line 600 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("session_authorization", false, (yylsp[-2]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4045 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 49:
#line 603 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt("xmloption", false, (yylsp[-2]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4053 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 50:
#line 606 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *) makeSetStmt(NULL, false, (yylsp[-2]).first_line, TECC_SET_MULTI, NIL);
    }
#line 4061 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 51:
#line 612 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4067 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 52:
#line 613 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[-2].str); }
#line 4073 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 53:
#line 617 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4079 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 54:
#line 618 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4085 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 55:
#line 622 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4091 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 56:
#line 623 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4097 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 57:
#line 624 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4103 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 58:
#line 625 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0); }
#line 4109 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 59:
#line 626 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0); }
#line 4115 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 60:
#line 630 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "true"; }
#line 4121 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 61:
#line 631 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "false"; }
#line 4127 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 62:
#line 632 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "on"; }
#line 4133 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 63:
#line 633 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4139 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 64:
#line 634 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4145 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 65:
#line 638 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4151 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 66:
#line 639 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4157 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 67:
#line 640 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4163 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 68:
#line 641 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 4169 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 69:
#line 642 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0); }
#line 4175 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 70:
#line 643 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0); }
#line 4181 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 71:
#line 644 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4187 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 72:
#line 645 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4193 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 73:
#line 649 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[-1].str), (yylsp[-2]).first_line, 0); }
#line 4199 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 74:
#line 653 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4205 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 75:
#line 654 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4211 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 76:
#line 655 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4217 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 77:
#line 659 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4223 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 78:
#line 660 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = NULL; }
#line 4229 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 79:
#line 661 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = NULL; }
#line 4235 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 80:
#line 665 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4241 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 81:
#line 666 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4247 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 82:
#line 667 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 4253 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 83:
#line 671 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst((yyvsp[0].str), (yylsp[-2]).first_line, 0); }
#line 4259 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 84:
#line 672 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("read only", (yylsp[-1]).first_line, 0); }
#line 4265 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 85:
#line 673 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("read write", (yylsp[-1]).first_line, 0); }
#line 4271 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 86:
#line 674 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("deferrable", (yylsp[0]).first_line, 0); }
#line 4277 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 87:
#line 675 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("not deferrable", (yylsp[-1]).first_line, 0); }
#line 4283 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 88:
#line 679 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "read uncommitted"; }
#line 4289 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 89:
#line 680 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "read committed"; }
#line 4295 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 90:
#line 681 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "repeatable read"; }
#line 4301 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 91:
#line 682 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "serializable"; }
#line 4307 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 92:
#line 686 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("DOCUMENT", (yylsp[0]).first_line, 0); }
#line 4313 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 93:
#line 687 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *) makeStringConst("CONTENT", (yylsp[0]).first_line, 0); }
#line 4319 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 94:
#line 691 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4325 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 95:
#line 692 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4331 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 96:
#line 696 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 4337 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 97:
#line 697 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 4343 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 98:
#line 701 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4349 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 99:
#line 702 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4355 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 100:
#line 706 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem((yyvsp[0].str), NULL, (yylsp[0]).first_line); }
#line 4361 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 101:
#line 707 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-2]).first_line, 0), (yylsp[-2]).first_line); }
#line 4367 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 102:
#line 708 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-2]).first_line, 0), (yylsp[-2]).first_line); }
#line 4373 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 103:
#line 709 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-2]).first_line, 0), (yylsp[-2]).first_line); }
#line 4379 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 104:
#line 710 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-2]).first_line, 0), (yylsp[-2]).first_line); }
#line 4385 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 105:
#line 716 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateKekStmt *n = makeCreateKekStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->names = (yyvsp[-1].list);
        n->options = (yyvsp[0].list);
        n->if_not_exists = false;
        (yyval.node) = (Node *)n;
    }
#line 4397 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 106:
#line 724 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateKekStmt *n = makeCreateKekStmt((yylsp[-6]).first_line, (yyloc).last_column - (yylsp[-6]).first_column + 1);
        n->names = (yyvsp[-1].list);
        n->options = (yyvsp[0].list);
        n->if_not_exists = true;
        (yyval.node) = (Node *)n;
    }
#line 4409 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 107:
#line 734 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 4415 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 108:
#line 740 "tecc_grammar.y" /* yacc.c:1646  */
    {
        AlterKekStmt *n = makeAlterKekStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->names = (yyvsp[-1].list);
        n->options = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 4426 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 109:
#line 751 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateDekStmt *n = makeCreateDekStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->names = (yyvsp[-1].list);
        n->options = (yyvsp[0].list);
        n->if_not_exists = false;
        (yyval.node) = (Node *)n;
    }
#line 4438 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 110:
#line 759 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateDekStmt *n = makeCreateDekStmt((yylsp[-6]).first_line, (yyloc).last_column - (yylsp[-6]).first_column + 1);
        n->names = (yyvsp[-1].list);
        n->options = (yyvsp[0].list);
        n->if_not_exists = true;
        (yyval.node) = (Node *)n;
    }
#line 4450 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 111:
#line 769 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 4456 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 112:
#line 775 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-12]).first_line, (yyloc).last_column - (yylsp[-12]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-9].node);
        n->tableElts = (yyvsp[-7].list);
        n->options = (yyvsp[-2].list);
        n->inhRelations = (yyvsp[-5].list);
        if ((yyvsp[-4].node) != NULL) {
            PartitionOfClause *clause = (PartitionOfClause *)(yyvsp[-4].node);
            n->inhRelations = list_make1(clause->parent);
            n->partbound = clause->partbound;
        }
        n->partspec = (yyvsp[-3].list);
        n->ofTypename = NULL;
        n->is_temp = (yyvsp[-11].bval);
        (yyval.node) = (Node *)n;
    }
#line 4477 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 113:
#line 792 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-15]).first_line, (yyloc).last_column - (yylsp[-15]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-9].node);
        n->tableElts = (yyvsp[-7].list);
        n->options = (yyvsp[-2].list);
        n->inhRelations = (yyvsp[-5].list);
        if ((yyvsp[-4].node) != NULL) {
            PartitionOfClause *clause = (PartitionOfClause *)(yyvsp[-4].node);
            n->inhRelations = list_make1(clause->parent);
            n->partbound = clause->partbound;
        }
        n->partspec = (yyvsp[-3].list);
        n->ofTypename = NULL;
        n->is_temp = (yyvsp[-14].bval);
        (yyval.node) = (Node *)n;
    }
#line 4498 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 114:
#line 809 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-9]).first_line, (yyloc).last_column - (yylsp[-9]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-6].node);
        n->tableElts = (yyvsp[-3].list);
        n->options = (yyvsp[-2].list);
        n->inhRelations = NIL;
        n->partbound = NULL;
        n->partspec = NULL;
        n->ofTypename = makeTypeNameFromList((yyvsp[-4].list), (yylsp[-4]).first_line, (yylsp[-4]).last_column - (yylsp[-4]).first_column + 1);
        n->is_temp = (yyvsp[-8].bval);
        (yyval.node) = (Node *)n;
    }
#line 4515 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 115:
#line 822 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-12]).first_line, (yyloc).last_column - (yylsp[-12]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-6].node);
        n->tableElts = (yyvsp[-3].list);
        n->options = (yyvsp[-2].list);
        n->inhRelations = NIL;
        n->partbound = NULL;
        n->partspec = NULL;
        n->ofTypename = makeTypeNameFromList((yyvsp[-4].list), (yylsp[-4]).first_line, (yylsp[-4]).last_column - (yylsp[-4]).first_column + 1);
        n->is_temp = (yyvsp[-11].bval);
        (yyval.node) = (Node *)n;
    }
#line 4532 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 116:
#line 835 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-11]).first_line, (yyloc).last_column - (yylsp[-11]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-8].node);
        n->tableElts = NIL;
        n->options = (yyvsp[-2].list);
        n->inhRelations = list_make1((yyvsp[-5].node));
        n->partbound = (yyvsp[-4].node);
        n->partspec = (yyvsp[-3].list);
        n->ofTypename = NULL;
        n->is_temp = (yyvsp[-10].bval);
        (yyval.node) = (Node *)n;
    }
#line 4549 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 117:
#line 848 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateStmt *n = makeCreateStmt((yylsp[-14]).first_line, (yyloc).last_column - (yylsp[-14]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-8].node);
        n->tableElts = NIL;
        n->options = (yyvsp[-2].list);
        n->inhRelations = list_make1((yyvsp[-5].node));
        n->partbound = (yyvsp[-4].node);
        n->partspec = (yyvsp[-3].list);
        n->ofTypename = NULL;
        n->is_temp = (yyvsp[-13].bval);
        (yyval.node) = (Node *)n;
    }
#line 4566 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 118:
#line 865 "tecc_grammar.y" /* yacc.c:1646  */
    {
        AlterTableStmt *n = makeAlterTableStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->relation = (RangeVar *)(yyvsp[-1].node);
        n->cmds = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 4577 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 119:
#line 874 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4583 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 120:
#line 875 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4589 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 121:
#line 880 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlter((yyvsp[-5].str), (yyvsp[-1].node), (yyvsp[0].node)), (yyvsp[-5].str), DROP_RESTRICT, false, (yylsp[-7]).first_line); }
#line 4595 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 122:
#line 882 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlter((yyvsp[-3].str), (yyvsp[-1].node), (yyvsp[0].node)), (yyvsp[-3].str), DROP_RESTRICT, false, (yylsp[-5]).first_line); }
#line 4601 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 123:
#line 884 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlterCollateOnly((yyvsp[-3].str), (yyvsp[0].list)), (yyvsp[-3].str), DROP_RESTRICT, false, (yylsp[-5]).first_line); }
#line 4607 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 124:
#line 886 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_GenericOptions, (yyvsp[0].node), (yyvsp[-3].str), DROP_RESTRICT, false, (yylsp[-5]).first_line); }
#line 4613 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 125:
#line 888 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_ResetOptions, NULL, (yyvsp[-2].str), DROP_RESTRICT, false, (yylsp[-4]).first_line); }
#line 4619 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 126:
#line 890 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_SetNotNull, NULL, (yyvsp[-3].str), DROP_RESTRICT, false, (yylsp[-5]).first_line); }
#line 4625 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 127:
#line 892 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropNotNull, NULL, (yyvsp[-3].str), DROP_RESTRICT, false, (yylsp[-5]).first_line); }
#line 4631 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 128:
#line 893 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AddColumn, (yyvsp[0].node), NULL, DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4637 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 129:
#line 895 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AddColumn, (yyvsp[0].node), NULL, DROP_RESTRICT, true, (yylsp[-5]).first_line); }
#line 4643 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 130:
#line 896 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[0].str), DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4649 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 131:
#line 897 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[0].str), DROP_RESTRICT, true, (yylsp[-4]).first_line); }
#line 4655 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 132:
#line 898 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[-1].str), DROP_CASCADE, false, (yylsp[-3]).first_line); }
#line 4661 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 133:
#line 899 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[-1].str), DROP_RESTRICT, false, (yylsp[-3]).first_line); }
#line 4667 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 134:
#line 901 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[-1].str), DROP_CASCADE, true, (yylsp[-5]).first_line); }
#line 4673 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 135:
#line 903 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, (yyvsp[-1].str), DROP_RESTRICT, true, (yylsp[-5]).first_line); }
#line 4679 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 136:
#line 905 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AddConstraint, (yyvsp[0].node), (yyvsp[-1].str), DROP_RESTRICT, false, (yylsp[-3]).first_line); }
#line 4685 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 137:
#line 906 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DropConstraint, NULL, (yyvsp[0].str), DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4691 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 138:
#line 907 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString((yyvsp[0].str)), (yyvsp[-2].str), DROP_RESTRICT, false, (yylsp[-4]).first_line); }
#line 4697 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 139:
#line 908 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString((yyvsp[0].str)), NULL, DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4703 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 140:
#line 909 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString((yyvsp[0].str)), NULL, DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4709 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 141:
#line 911 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_AttachPartition, (yyvsp[0].node), NULL, DROP_RESTRICT, false, (yylsp[-3]).first_line); }
#line 4715 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 142:
#line 912 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlterTableCmd(AT_DetachPartition, NULL, (yyvsp[0].node), DROP_RESTRICT, false, (yylsp[-2]).first_line); }
#line 4721 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 143:
#line 917 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = false; }
#line 4727 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 144:
#line 918 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = true; }
#line 4733 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 145:
#line 919 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = true; }
#line 4739 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 146:
#line 920 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = false; }
#line 4745 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 147:
#line 924 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4751 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 148:
#line 925 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4757 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 149:
#line 929 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4763 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 150:
#line 930 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4769 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 151:
#line 931 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4775 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 152:
#line 936 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDefElem("like", (yyvsp[-1].node), (yylsp[-2]).first_line);
    }
#line 4783 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 153:
#line 942 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = 0; }
#line 4789 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 154:
#line 943 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = 0; }
#line 4795 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 155:
#line 944 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = 0; }
#line 4801 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 156:
#line 945 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = 0; }
#line 4807 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 157:
#line 946 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = 0; }
#line 4813 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 158:
#line 951 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ColumnDef *n = makeColumnDef((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->colname = tecc_pstrdup((yyvsp[-3].str));
        n->typeName = (yyvsp[-2].node);
        n->collClause = (yyvsp[-1].node);
        n->constraints = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 4826 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 159:
#line 962 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 4832 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 160:
#line 966 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeTypeName((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column - (yylsp[0]).first_column + 1); }
#line 4838 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 161:
#line 968 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeTypeNameFromList(lcons(makeString((yyvsp[-1].str)), (yyvsp[0].list)), (yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1); }
#line 4844 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 162:
#line 969 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeTypeNameWithTypmod((yyvsp[-3].str), (yyvsp[-1].ival), (yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1); }
#line 4850 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 163:
#line 970 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeTypeNameWithTypmod2((yyvsp[-5].str), (yyvsp[-3].ival), (yyvsp[-1].ival), (yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1); }
#line 4856 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 164:
#line 971 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeEncryptedTypeName((yyvsp[-5].str), (yyvsp[-1].list), (yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1); }
#line 4862 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 165:
#line 972 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeEncryptedTypeNameWithTypmod((yyvsp[-8].str), (yyvsp[-6].ival), (yyvsp[-1].list), (yylsp[-8]).first_line, (yyloc).last_column - (yylsp[-8]).first_column + 1); }
#line 4868 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 166:
#line 973 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeEncryptedTypeNameWithTypmod2((yyvsp[-10].str), (yyvsp[-8].ival), (yyvsp[-6].ival), (yyvsp[-1].list), (yylsp[-10]).first_line, (yyloc).last_column - (yylsp[-10]).first_column + 1); }
#line 4874 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 167:
#line 977 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4880 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 168:
#line 978 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 4886 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 169:
#line 982 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeDefElem((yyvsp[-2].str), (Node *)makeString((yyvsp[0].str)), (yylsp[-1]).first_line); }
#line 4892 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 170:
#line 983 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeDefElem((yyvsp[-2].str), (Node *)makeString((yyvsp[0].str)), (yylsp[-1]).first_line); }
#line 4898 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 171:
#line 986 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 4904 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 172:
#line 987 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeCollateClause(NULL, (yyvsp[0].list), (yylsp[-1]).first_line); }
#line 4910 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 173:
#line 991 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 4916 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 174:
#line 992 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 4922 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 175:
#line 996 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 4928 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 176:
#line 997 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 4934 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 177:
#line 1001 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_NULL, (yylsp[0]).first_line, (yylsp[0]).last_column - (yylsp[0]).first_column + 1); }
#line 4940 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 178:
#line 1002 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_NOTNULL, (yylsp[-1]).first_line, (yylsp[0]).last_column - (yylsp[-1]).first_column + 1); }
#line 4946 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 179:
#line 1003 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_CHECK, (yyvsp[-1].node), (yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1); }
#line 4952 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 180:
#line 1004 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_CHECK, (yyvsp[-3].node), (yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1); }
#line 4958 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 181:
#line 1005 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_DEFAULT, (yyvsp[0].node), (yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1); }
#line 4964 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 182:
#line 1006 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_UNIQUE, (yylsp[0]).first_line, (yylsp[0]).last_column - (yylsp[0]).first_column + 1); }
#line 4970 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 183:
#line 1007 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_PRIMARY, (yylsp[-1]).first_line, (yylsp[0]).last_column - (yylsp[-1]).first_column + 1); }
#line 4976 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 184:
#line 1009 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeForeignKeyConstraint((yyvsp[-4].node), (yyvsp[-2].list), (yyvsp[0].list), (yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1); }
#line 4982 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 185:
#line 1010 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_ATTR_DEFERRABLE, (yylsp[0]).first_line, (yylsp[0]).last_column - (yylsp[0]).first_column + 1); }
#line 4988 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 186:
#line 1011 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_ATTR_DEFERRED, (yylsp[-1]).first_line, (yylsp[0]).last_column - (yylsp[-1]).first_column + 1); }
#line 4994 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 187:
#line 1012 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_ATTR_IMMEDIATE, (yylsp[-1]).first_line, (yylsp[0]).last_column - (yylsp[-1]).first_column + 1); }
#line 5000 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 188:
#line 1014 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_GENERATED, (yyvsp[-2].node), (yylsp[-6]).first_line, (yyloc).last_column - (yylsp[-6]).first_column + 1); }
#line 5006 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 189:
#line 1015 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_IDENTITY, (yylsp[-3]).first_line, (yylsp[-1]).last_column - (yylsp[-3]).first_column + 1); }
#line 5012 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 190:
#line 1016 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraint(CONSTR_IDENTITY, (yylsp[-4]).first_line, (yylsp[-1]).last_column - (yylsp[-4]).first_column + 1); }
#line 5018 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 191:
#line 1020 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = setConstraintName((yyvsp[0].node), (yyvsp[-1].str), (yylsp[-1]).first_line, (yylsp[-1]).last_column - (yylsp[-1]).first_column + 1); }
#line 5024 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 192:
#line 1021 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5030 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 193:
#line 1025 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_CHECK, (yyvsp[-1].node), (yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1); }
#line 5036 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 194:
#line 1026 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithExpr(CONSTR_CHECK, (yyvsp[-3].node), (yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1); }
#line 5042 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 195:
#line 1027 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithColumns(CONSTR_UNIQUE, (yyvsp[-1].list), (yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1); }
#line 5048 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 196:
#line 1028 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithColumns(CONSTR_PRIMARY, (yyvsp[-1].list), (yylsp[-4]).first_line, (yyloc).last_column - (yylsp[-4]).first_column + 1); }
#line 5054 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 197:
#line 1030 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeForeignKeyConstraint((yyvsp[-4].node), (yyvsp[-7].list), (yyvsp[-2].list), (yylsp[-10]).first_line, (yyloc).last_column - (yylsp[-10]).first_column + 1); }
#line 5060 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 198:
#line 1032 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = makeConstraintWithColumns(CONSTR_EXCLUSION, (yyvsp[-2].list), (yylsp[-6]).first_line, (yyloc).last_column - (yylsp[-6]).first_column + 1); }
#line 5066 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 199:
#line 1036 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[-2].node)); }
#line 5072 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 200:
#line 1037 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-4].list), (yyvsp[-2].node)); }
#line 5078 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 201:
#line 1041 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString((yyvsp[0].str)); }
#line 5084 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 202:
#line 1042 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString("="); }
#line 5090 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 203:
#line 1043 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString("<>"); }
#line 5096 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 204:
#line 1044 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString("<"); }
#line 5102 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 205:
#line 1045 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString(">"); }
#line 5108 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 206:
#line 1046 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString("<="); }
#line 5114 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 207:
#line 1047 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeString(">="); }
#line 5120 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 208:
#line 1052 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5126 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 209:
#line 1057 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5132 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 210:
#line 1058 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 5138 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 211:
#line 1062 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0); }
#line 5144 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 212:
#line 1067 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5150 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 213:
#line 1068 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 5156 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 214:
#line 1072 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5162 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 215:
#line 1076 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5168 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 216:
#line 1077 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 5174 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 217:
#line 1081 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5180 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 218:
#line 1082 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 5186 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 219:
#line 1086 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem("match", (Node *)makeString("full"), (yylsp[-1]).first_line); }
#line 5192 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 220:
#line 1087 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem("match", (Node *)makeString("partial"), (yylsp[-1]).first_line); }
#line 5198 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 221:
#line 1088 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem("match", (Node *)makeString("simple"), (yylsp[-1]).first_line); }
#line 5204 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 222:
#line 1089 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem("on_delete", (Node *)makeString((yyvsp[0].str)), (yylsp[-2]).first_line); }
#line 5210 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1090 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeDefElem("on_update", (Node *)makeString((yyvsp[0].str)), (yylsp[-2]).first_line); }
#line 5216 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 224:
#line 1094 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "no action"; }
#line 5222 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1095 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "restrict"; }
#line 5228 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1096 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "cascade"; }
#line 5234 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1097 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "set null"; }
#line 5240 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1098 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "set default"; }
#line 5246 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1102 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5252 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1103 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5258 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 231:
#line 1107 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5264 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 232:
#line 1108 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5270 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 233:
#line 1109 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5276 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 234:
#line 1110 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5282 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 235:
#line 1115 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 5288 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 236:
#line 1117 "tecc_grammar.y" /* yacc.c:1646  */
    {
            PartitionOfClause *clause = (PartitionOfClause *)tecc_palloc(sizeof(PartitionOfClause));
            clause->parent = (yyvsp[-1].node);
            clause->partbound = (yyvsp[0].node);
            (yyval.node) = (Node *)clause;
        }
#line 5299 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 237:
#line 1127 "tecc_grammar.y" /* yacc.c:1646  */
    {
            PartitionBoundSpec *spec = makePartitionBoundSpec('r', false, (yylsp[-9]).first_line);
            spec->lowerdatums = (yyvsp[-5].list);
            spec->upperdatums = (yyvsp[-1].list);
            (yyval.node) = (Node *)spec;
        }
#line 5310 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 238:
#line 1134 "tecc_grammar.y" /* yacc.c:1646  */
    {
            PartitionBoundSpec *spec = makePartitionBoundSpec('l', false, (yylsp[-5]).first_line);
            spec->listdatums = (yyvsp[-1].list);
            (yyval.node) = (Node *)spec;
        }
#line 5320 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 239:
#line 1140 "tecc_grammar.y" /* yacc.c:1646  */
    {
            PartitionBoundSpec *spec = makePartitionBoundSpec(0, true, (yylsp[-2]).first_line);
            (yyval.node) = (Node *)spec;
        }
#line 5329 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 240:
#line 1147 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5335 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 241:
#line 1148 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 5341 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 242:
#line 1153 "tecc_grammar.y" /* yacc.c:1646  */
    {
            (yyval.node) = (Node *)makePartitionRangeDatum(makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column), (yylsp[0]).first_line);
        }
#line 5349 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 243:
#line 1157 "tecc_grammar.y" /* yacc.c:1646  */
    {
            int int_value = atoi((yyvsp[0].str));
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, (yylsp[0]).first_line, (yylsp[0]).last_column), (yylsp[0]).first_line);
        }
#line 5358 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 244:
#line 1162 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "10000::int2" — ignore the type cast, keep INTEGER value */
            int int_value = atoi((yyvsp[-2].str));
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, (yylsp[-2]).first_line, (yylsp[-2]).last_column), (yylsp[-2]).first_line);
        }
#line 5368 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 245:
#line 1168 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "-32767" — combine minus + integer into a single negative Const */
            int int_value = -atoi((yyvsp[0].str));
            int full_location = (yylsp[-1]).first_line;
            int full_length = (yylsp[-1]).last_column + (yylsp[0]).last_column;
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, full_location, full_length), (yylsp[-1]).first_line);
        }
#line 5380 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 246:
#line 1176 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "-32767::int2" — combine minus + integer, ignore the type cast */
            int int_value = -atoi((yyvsp[-2].str));
            int full_location = (yylsp[-3]).first_line;
            int full_length = (yylsp[-3]).last_column + (yylsp[-2]).last_column;
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, full_location, full_length), (yylsp[-3]).first_line);
        }
#line 5392 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 247:
#line 1184 "tecc_grammar.y" /* yacc.c:1646  */
    {
            double float_value = strtod((yyvsp[0].str), NULL);
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, (yylsp[0]).first_line, (yylsp[0]).last_column), (yylsp[0]).first_line);
        }
#line 5401 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 248:
#line 1189 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "3.14::float4" — ignore the type cast, keep FLOAT value */
            double float_value = strtod((yyvsp[-2].str), NULL);
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, (yylsp[-2]).first_line, (yylsp[-2]).last_column), (yylsp[-2]).first_line);
        }
#line 5411 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 249:
#line 1195 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "-20.0" — combine minus + float */
            double float_value = -strtod((yyvsp[0].str), NULL);
            int full_length = (yylsp[-1]).last_column + (yylsp[0]).last_column;
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, (yylsp[-1]).first_line, full_length), (yylsp[-1]).first_line);
        }
#line 5422 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 250:
#line 1202 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "-99999999.99::numeric" — combine minus + float, ignore type cast */
            double float_value = -strtod((yyvsp[-2].str), NULL);
            int full_length = (yylsp[-3]).last_column + (yylsp[-2]).last_column;
            (yyval.node) = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, (yylsp[-3]).first_line, full_length), (yylsp[-3]).first_line);
        }
#line 5433 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 251:
#line 1209 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle 'Infinity' as a special float value in partition bounds */
            (yyval.node) = (Node *)makePartitionRangeDatum(
                (Node *)makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column), (yylsp[0]).first_line);
        }
#line 5443 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 252:
#line 1215 "tecc_grammar.y" /* yacc.c:1646  */
    {
            /* Handle "-Infinity" — combine minus + ident for special float values */
            size_t slen = strlen((yyvsp[0].str));
            char *neg = (char *)pg_malloc(slen + 2);
            int full_length = (yylsp[-1]).last_column + (yylsp[0]).last_column;
            neg[0] = '-'; memcpy(neg + 1, (yyvsp[0].str), slen + 1);
            (yyval.node) = (Node *)makePartitionRangeDatum(
                (Node *)makeStringConst(neg, (yylsp[-1]).first_line, full_length), (yylsp[-1]).first_line);
            pg_free(neg);
        }
#line 5458 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 253:
#line 1226 "tecc_grammar.y" /* yacc.c:1646  */
    {
            (yyval.node) = (Node *)makePartitionRangeDatum(NULL, (yylsp[0]).first_line);
        }
#line 5466 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 254:
#line 1230 "tecc_grammar.y" /* yacc.c:1646  */
    {
            (yyval.node) = (Node *)makePartitionRangeDatum(NULL, (yylsp[0]).first_line);
        }
#line 5474 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 255:
#line 1235 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5480 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 256:
#line 1236 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5486 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 257:
#line 1240 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5492 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 258:
#line 1241 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 5498 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 259:
#line 1245 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5504 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 260:
#line 1246 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5510 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 261:
#line 1251 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ColumnDef *n = makeColumnDef((yylsp[-2]).first_line, (yyloc).last_column - (yylsp[-2]).first_column + 1);
        n->colname = tecc_pstrdup((yyvsp[-2].str));
        n->typeName = NULL;
        n->constraints = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 5522 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 262:
#line 1261 "tecc_grammar.y" /* yacc.c:1646  */
    { /* WITH OPTIONS - processed by backend */ }
#line 5528 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 264:
#line 1266 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5534 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1267 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5540 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 266:
#line 1268 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5546 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1269 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 5552 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 268:
#line 1273 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 5558 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 269:
#line 1274 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0); }
#line 5564 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 270:
#line 1278 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 5570 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 271:
#line 1279 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 5576 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 272:
#line 1285 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->objects = list_make1(makeString((yyvsp[-1].str)));
        n->removeType = OBJECT_KEK;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = false;
        (yyval.node) = (Node *)n;
    }
#line 5589 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 273:
#line 1294 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        n->objects = list_make1(makeString((yyvsp[-1].str)));
        n->removeType = OBJECT_DEK;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = false;
        (yyval.node) = (Node *)n;
    }
#line 5602 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 274:
#line 1303 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);
        /* $3 is a RangeVar*, need to extract the table name */
        RangeVar *rv = (RangeVar *)(yyvsp[-1].node);
        n->objects = list_make1(makeStringConst(rv->relname, (yylsp[-1]).first_line, 0));
        n->removeType = OBJECT_TABLE;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = false;
        (yyval.node) = (Node *)n;
    }
#line 5617 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1314 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1);
        n->objects = list_make1(makeString((yyvsp[-1].str)));
        n->removeType = OBJECT_KEK;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = true;
        (yyval.node) = (Node *)n;
    }
#line 5630 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1323 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1);
        n->objects = list_make1(makeString((yyvsp[-1].str)));
        n->removeType = OBJECT_DEK;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = true;
        (yyval.node) = (Node *)n;
    }
#line 5643 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1332 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DropStmt *n = makeDropStmt((yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1);
        /* $5 is a RangeVar*, need to extract the table name */
        RangeVar *rv = (RangeVar *)(yyvsp[-1].node);
        n->objects = list_make1(makeString(rv->relname));
        n->removeType = OBJECT_TABLE;
        n->behavior = (yyvsp[0].ival);
        n->missing_ok = true;
        (yyval.node) = (Node *)n;
    }
#line 5658 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1352 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5664 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1353 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[-1].node); }
#line 5670 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 280:
#line 1358 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5676 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1360 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_UNION, true, (yyvsp[-3].node), (yyvsp[0].node), (yylsp[-2]).first_line, (yyloc).last_column - (yylsp[-2]).first_column + 1);
    }
#line 5684 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1364 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_UNION, false, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1);
    }
#line 5692 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1368 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_INTERSECT, true, (yyvsp[-3].node), (yyvsp[0].node), (yylsp[-2]).first_line, (yyloc).last_column - (yylsp[-2]).first_column + 1);
    }
#line 5700 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1372 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_INTERSECT, false, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1);
    }
#line 5708 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1376 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_EXCEPT, true, (yyvsp[-3].node), (yyvsp[0].node), (yylsp[-2]).first_line, (yyloc).last_column - (yylsp[-2]).first_column + 1);
    }
#line 5716 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1380 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetOperationStmt(SETOP_EXCEPT, false, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1);
    }
#line 5724 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1389 "tecc_grammar.y" /* yacc.c:1646  */
    {
        SelectStmt *n = makeSelectStmt((yylsp[-11]).first_line, (yyloc).last_column - (yylsp[-11]).first_column + 1);
        n->distinctClause = (yyvsp[-10].list);
        n->targetList = (yyvsp[-9].list);
        n->fromClause = (yyvsp[-8].list);
        n->whereClause = (yyvsp[-7].node);
        n->groupClause = (yyvsp[-6].list);
        n->havingClause = (yyvsp[-5].node);
        n->windowClause = (yyvsp[-4].list);
        n->sortClause = (yyvsp[-3].list);
        n->limitCount = (yyvsp[-2].node);
        n->limitOffset = (yyvsp[-1].node);
        n->lockingClause = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 5744 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1405 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - VALUES as simple_select variant, aligned with PG14 backend */
        (yyval.node) = (yyvsp[0].node);
    }
#line 5753 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1414 "tecc_grammar.y" /* yacc.c:1646  */
    {
        InsertStmt *n = makeInsertStmt((yylsp[-7]).first_line, (yyloc).last_column - (yylsp[-7]).first_column + 1);
        n->relation = (struct RangeVar *)(yyvsp[-5].node);
        n->cols = (yyvsp[-4].list);
        n->override = (yyvsp[-3].ival);
        n->selectStmt = (yyvsp[-2].node);
        n->onConflictClause = (yyvsp[-1].list);
        n->returningList = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 5768 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1429 "tecc_grammar.y" /* yacc.c:1646  */
    {
        UpdateStmt *n = makeUpdateStmt((yylsp[-6]).first_line, (yyloc).last_column - (yylsp[-6]).first_column + 1);
        n->relation = (struct RangeVar *)(yyvsp[-5].node);
        n->targetList = (yyvsp[-3].list);
        n->fromClause = (yyvsp[-2].list);
        n->whereClause = (yyvsp[-1].node);
        n->returningList = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 5782 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1443 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DeleteStmt *n = makeDeleteStmt((yylsp[-5]).first_line, (yyloc).last_column - (yylsp[-5]).first_column + 1);
        n->relation = (struct RangeVar *)(yyvsp[-3].node);
        n->usingClause = (yyvsp[-2].list);
        n->whereClause = (yyvsp[-1].node);
        n->returningList = (yyvsp[0].list);
        (yyval.node) = (Node *)n;
    }
#line 5795 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 292:
#line 1456 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "+"; }
#line 5801 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 293:
#line 1457 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "-"; }
#line 5807 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 294:
#line 1458 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "*"; }
#line 5813 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 295:
#line 1459 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "/"; }
#line 5819 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 296:
#line 1460 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "%"; }
#line 5825 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 297:
#line 1461 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "^"; }
#line 5831 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1462 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "="; }
#line 5837 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 299:
#line 1463 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "<>"; }
#line 5843 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 300:
#line 1464 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "<"; }
#line 5849 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1465 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = ">"; }
#line 5855 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 302:
#line 1466 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "<="; }
#line 5861 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 303:
#line 1467 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = ">="; }
#line 5867 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1468 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "||"; }
#line 5873 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1469 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "~"; }
#line 5879 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 306:
#line 1470 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "~*"; }
#line 5885 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 307:
#line 1471 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "!~"; }
#line 5891 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 308:
#line 1472 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "!~*"; }
#line 5897 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 309:
#line 1476 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeString((yyvsp[0].str))); }
#line 5903 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 310:
#line 1477 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeString((yyvsp[0].str))); }
#line 5909 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 311:
#line 1478 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lcons(makeString((yyvsp[-2].str)), (yyvsp[0].list)); }
#line 5915 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 312:
#line 1482 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 5921 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 313:
#line 1487 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5927 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 314:
#line 1488 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("+")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5933 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 315:
#line 1489 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("-")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5939 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 316:
#line 1490 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("^")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5945 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 317:
#line 1491 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("*")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5951 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 318:
#line 1492 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("/")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5957 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 319:
#line 1493 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("%")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5963 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 320:
#line 1494 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("=")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5969 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 321:
#line 1495 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("<>")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5975 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 322:
#line 1496 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("<")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5981 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 323:
#line 1497 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString(">")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5987 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 324:
#line 1498 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5993 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 325:
#line 1499 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 5999 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 326:
#line 1500 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_null_test((yyvsp[-2].node), IS_NULL, (yylsp[-1]).first_line); }
#line 6005 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 327:
#line 1501 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("||")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6011 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 328:
#line 1502 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_null_test((yyvsp[-3].node), IS_NOT_NULL, (yylsp[-2]).first_line); }
#line 6017 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 329:
#line 1504 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("IS DISTINCT FROM")), (yyvsp[-4].node), (yyvsp[0].node), (yylsp[-3]).first_line);
    }
#line 6025 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 330:
#line 1508 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("IS NOT DISTINCT FROM")), (yyvsp[-5].node), (yyvsp[0].node), (yylsp[-4]).first_line);
    }
#line 6033 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 331:
#line 1513 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-2].node), IS_TRUE, (yylsp[-1]).first_line);
    }
#line 6041 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 332:
#line 1517 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-3].node), IS_NOT_TRUE, (yylsp[-2]).first_line);
    }
#line 6049 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 333:
#line 1521 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-2].node), IS_FALSE, (yylsp[-1]).first_line);
    }
#line 6057 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 334:
#line 1525 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-3].node), IS_NOT_FALSE, (yylsp[-2]).first_line);
    }
#line 6065 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 335:
#line 1529 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-2].node), IS_UNKNOWN, (yylsp[-1]).first_line);
    }
#line 6073 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 336:
#line 1533 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeBooleanTest((yyvsp[-3].node), IS_NOT_UNKNOWN, (yylsp[-2]).first_line);
    }
#line 6081 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 337:
#line 1538 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeTimeZoneExpr((yyvsp[-4].node), (yyvsp[0].node), (yylsp[-3]).first_line);
    }
#line 6089 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 338:
#line 1542 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(ANY_SUBLINK, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-1].node), (yylsp[-4]).first_line);
    }
#line 6097 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 339:
#line 1546 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(ANY_SUBLINK, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-1].node), (yylsp[-4]).first_line);
    }
#line 6105 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 340:
#line 1550 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(ALL_SUBLINK, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-1].node), (yylsp[-4]).first_line);
    }
#line 6113 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 341:
#line 1558 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_any_all_array_expr((yyvsp[-7].node), (yyvsp[-8].node), (List *) (yyvsp[-2].list), 1, (yylsp[-7]).first_line);
    }
#line 6121 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 342:
#line 1562 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_any_all_array_expr((yyvsp[-7].node), (yyvsp[-8].node), (List *) (yyvsp[-2].list), 1, (yylsp[-7]).first_line);
    }
#line 6129 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 343:
#line 1566 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_any_all_array_expr((yyvsp[-7].node), (yyvsp[-8].node), (List *) (yyvsp[-2].list), 0, (yylsp[-7]).first_line);
    }
#line 6137 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 344:
#line 1571 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_a_expr(AEXPR_OP, (yyvsp[-1].list), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line);
    }
#line 6145 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 345:
#line 1578 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6151 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 346:
#line 1579 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_bool_expr(AND_EXPR, list_make2((yyvsp[-2].node), (yyvsp[0].node)), (yylsp[-1]).first_line); }
#line 6157 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 347:
#line 1580 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_bool_expr(OR_EXPR, list_make2((yyvsp[-2].node), (yyvsp[0].node)), (yylsp[-1]).first_line); }
#line 6163 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 348:
#line 1581 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_bool_expr(NOT_EXPR, list_make1((yyvsp[0].node)), (yylsp[-1]).first_line); }
#line 6169 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 349:
#line 1582 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* BETWEEN SYMMETRIC: order-independent, i.e. (a>=x AND a<=y) OR (a>=y AND a<=x) */
        Node *lower1 = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), (yyvsp[-5].node), (yyvsp[-2].node), (yylsp[-4]).first_line);
        Node *upper1 = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), (yyvsp[-5].node), (yyvsp[0].node), (yylsp[-2]).first_line);
        Node *range1 = make_bool_expr(AND_EXPR, list_make2(lower1, upper1), (yylsp[-4]).first_line);
        Node *lower2 = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), (yyvsp[-5].node), (yyvsp[0].node), (yylsp[-4]).first_line);
        Node *upper2 = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), (yyvsp[-5].node), (yyvsp[-2].node), (yylsp[-2]).first_line);
        Node *range2 = make_bool_expr(AND_EXPR, list_make2(lower2, upper2), (yylsp[-4]).first_line);
        (yyval.node) = make_bool_expr(OR_EXPR, list_make2(range1, range2), (yylsp[-4]).first_line);
      }
#line 6184 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 350:
#line 1592 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* BETWEEN a AND b is equivalent to a >= x AND a <= y */
        Node *lower = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), (yyvsp[-4].node), (yyvsp[-2].node), (yylsp[-3]).first_line);
        Node *upper = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), (yyvsp[-4].node), (yyvsp[0].node), (yylsp[-1]).first_line);
        (yyval.node) = make_bool_expr(AND_EXPR, list_make2(lower, upper), (yylsp[-3]).first_line);
      }
#line 6195 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 351:
#line 1598 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* IN (x, y, z) is equivalent to expr = x OR expr = y OR expr = z */
        List *args = NIL;
        List *expr_list = (List *)(yyvsp[-1].list);
        ListCell *lc;
        /* 使用标准C循环替换foreach宏 */
        for (lc = list_head(expr_list); lc != NULL; lc = lnext(lc)) {
            Node *arg = (Node *)lfirst(lc);
            Node *eq = make_a_expr(AEXPR_OP, list_make1(makeString("=")), (yyvsp[-4].node), arg, (yylsp[-3]).first_line);
            args = lappend(args, eq);
        }
        if (list_length(args) == 1) {
            (yyval.node) = (Node *)lfirst(list_head(args));
        } else {
            (yyval.node) = make_bool_expr(OR_EXPR, args, (yylsp[-3]).first_line);
        }
      }
#line 6217 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 352:
#line 1616 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("LIKE")), (yyvsp[-2].node), (yyvsp[0].node), NULL, (yylsp[-1]).first_line); }
#line 6223 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 353:
#line 1618 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("LIKE")), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]).first_line); }
#line 6229 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 354:
#line 1620 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT LIKE")), (yyvsp[-3].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line); }
#line 6235 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 355:
#line 1622 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT LIKE")), (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]).first_line); }
#line 6241 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 356:
#line 1624 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("ILIKE")), (yyvsp[-2].node), (yyvsp[0].node), NULL, (yylsp[-1]).first_line); }
#line 6247 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 357:
#line 1626 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("ILIKE")), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]).first_line); }
#line 6253 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 358:
#line 1628 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT ILIKE")), (yyvsp[-3].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line); }
#line 6259 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 359:
#line 1630 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT ILIKE")), (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]).first_line); }
#line 6265 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 360:
#line 1632 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("SIMILAR TO")), (yyvsp[-3].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line); }
#line 6271 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 361:
#line 1634 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("SIMILAR TO")), (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]).first_line); }
#line 6277 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 362:
#line 1636 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("NOT SIMILAR TO")), (yyvsp[-4].node), (yyvsp[0].node), NULL, (yylsp[-3]).first_line); }
#line 6283 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 363:
#line 1638 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("NOT SIMILAR TO")), (yyvsp[-6].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]).first_line); }
#line 6289 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 364:
#line 1640 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("!~")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6295 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 365:
#line 1642 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("!~*")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6301 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 366:
#line 1644 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("~")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6307 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 367:
#line 1646 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("~*")), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6313 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 368:
#line 1647 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* NOT BETWEEN SYMMETRIC: order-independent, i.e. (a<x OR a>y) AND (a<y OR a>x) */
        Node *lower1 = make_a_expr(AEXPR_OP, list_make1(makeString("<")), (yyvsp[-6].node), (yyvsp[-2].node), (yylsp[-5]).first_line);
        Node *upper1 = make_a_expr(AEXPR_OP, list_make1(makeString(">")), (yyvsp[-6].node), (yyvsp[0].node), (yylsp[-3]).first_line);
        Node *range1 = make_bool_expr(OR_EXPR, list_make2(lower1, upper1), (yylsp[-5]).first_line);
        Node *lower2 = make_a_expr(AEXPR_OP, list_make1(makeString("<")), (yyvsp[-6].node), (yyvsp[0].node), (yylsp[-5]).first_line);
        Node *upper2 = make_a_expr(AEXPR_OP, list_make1(makeString(">")), (yyvsp[-6].node), (yyvsp[-2].node), (yylsp[-3]).first_line);
        Node *range2 = make_bool_expr(OR_EXPR, list_make2(lower2, upper2), (yylsp[-5]).first_line);
        (yyval.node) = make_bool_expr(AND_EXPR, list_make2(range1, range2), (yylsp[-5]).first_line);
      }
#line 6328 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 369:
#line 1657 "tecc_grammar.y" /* yacc.c:1646  */
    {
        Node *lower = make_a_expr(AEXPR_OP, list_make1(makeString("<")), (yyvsp[-5].node), (yyvsp[-2].node), (yylsp[-4]).first_line);
        Node *upper = make_a_expr(AEXPR_OP, list_make1(makeString(">")), (yyvsp[-5].node), (yyvsp[0].node), (yylsp[-2]).first_line);
        (yyval.node) = make_bool_expr(OR_EXPR, list_make2(lower, upper), (yylsp[-4]).first_line);
      }
#line 6338 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 370:
#line 1662 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *args = NIL;
        List *elist = (List *)(yyvsp[-1].list);
        ListCell *lc;
        for (lc = list_head(elist); lc != NULL; lc = lnext(lc)) {
            Node *arg = (Node *)lfirst(lc);
            Node *neq = make_a_expr(AEXPR_OP, list_make1(makeString("<>")), (yyvsp[-5].node), arg, (yylsp[-4]).first_line);
            args = lappend(args, neq);
        }
        if (list_length(args) == 1) {
            (yyval.node) = (Node *)lfirst(list_head(args));
        } else {
            (yyval.node) = make_bool_expr(AND_EXPR, args, (yylsp[-4]).first_line);
        }
      }
#line 6358 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 371:
#line 1678 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(ANY_SUBLINK, (yyvsp[-4].node), list_make1(makeString("=")), (yyvsp[-1].node), (yylsp[-3]).first_line);
    }
#line 6366 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 372:
#line 1682 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(ALL_SUBLINK, (yyvsp[-5].node), list_make1(makeString("<>")), (yyvsp[-1].node), (yylsp[-4]).first_line);
    }
#line 6374 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 373:
#line 1688 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6380 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 374:
#line 1692 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6386 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 375:
#line 1693 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6392 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 376:
#line 1694 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6398 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 377:
#line 1697 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSetToDefault((yylsp[0]).first_line);
    }
#line 6406 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 378:
#line 1700 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6412 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 379:
#line 1701 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6418 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 380:
#line 1704 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[-1].node); }
#line 6424 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 381:
#line 1706 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(EXISTS_SUBLINK, NULL, NIL, (yyvsp[-1].node), (yylsp[-3]).first_line);
    }
#line 6432 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 382:
#line 1710 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall(list_make1(makeStringConst("array", (yylsp[-3]).first_line, 0)), (yyvsp[-1].list), (yylsp[-3]).first_line);
    }
#line 6440 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 383:
#line 1715 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeRowExpr((yyvsp[-1].list), (yylsp[-3]).first_line);
    }
#line 6448 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 384:
#line 1722 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *args = lappend((yyvsp[-3].list), (yyvsp[-1].node));
        (yyval.node) = (Node *)makeRowExpr(args, (yylsp[-4]).first_line);
    }
#line 6457 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 385:
#line 1727 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall(list_make1(makeStringConst("current_date", (yylsp[0]).first_line, 0)), NIL, (yylsp[0]).first_line);
    }
#line 6465 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 386:
#line 1731 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall(list_make1(makeStringConst("current_time", (yylsp[0]).first_line, 0)), NIL, (yylsp[0]).first_line);
    }
#line 6473 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 387:
#line 1735 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall(list_make1(makeStringConst("current_timestamp", (yylsp[0]).first_line, 0)), NIL, (yylsp[0]).first_line);
    }
#line 6481 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 388:
#line 1740 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall(list_make1(makeStringConst("user", (yylsp[0]).first_line, 0)), NIL, (yylsp[0]).first_line);
    }
#line 6489 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 389:
#line 1746 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeFuncCall((yyvsp[-1].list), list_make1(makeString((yyvsp[0].str))), (yylsp[-1]).first_line);
    }
#line 6497 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 390:
#line 1750 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeSubLink(EXPR_SUBLINK, NULL, NIL, (yyvsp[-1].node), (yylsp[-2]).first_line);
    }
#line 6505 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 391:
#line 1753 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("+")), NULL, (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6511 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 392:
#line 1754 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = make_a_expr(AEXPR_OP, list_make1(makeString("-")), NULL, (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 6517 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 393:
#line 1756 "tecc_grammar.y" /* yacc.c:1646  */
    {
        TypeCast *tc = (TypeCast *)tecc_palloc(sizeof(TypeCast));
        tc->type = FE_T_TypeCast;
        tc->arg = (yyvsp[-2].node);
        tc->typeName = (TypeName *)(yyvsp[0].node);
        tc->location = (yylsp[-2]).first_line;
        (yyval.node) = (Node *)tc;
    }
#line 6530 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 394:
#line 1766 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCollateClause((yyvsp[-2].node), (yyvsp[0].list), (yylsp[-1]).first_line);
    }
#line 6538 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 395:
#line 1773 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = make_column_ref_from_list((yyvsp[0].list), (yylsp[0]).first_line);
    }
#line 6546 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 396:
#line 1778 "tecc_grammar.y" /* yacc.c:1646  */
    {
        Node *cr = make_column_ref_from_list((yyvsp[-1].list), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)makeA_Indirection(cr, (yyvsp[0].list));
    }
#line 6555 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 397:
#line 1790 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeColumnRef(lappend((yyvsp[-2].list), (Node *)makeA_Star()),
                                   (yylsp[-2]).first_line);
    }
#line 6564 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 398:
#line 1799 "tecc_grammar.y" /* yacc.c:1646  */
    {
		if ((yyvsp[-4].node) != NULL && ((Node*)(yyvsp[-4].node))->type == FE_T_FuncCall) {
			/* Aggregation star or distinct case (may include ORDER BY) */
			FuncCall *fc = (FuncCall*)(yyvsp[-4].node);
			fc->funcname = (yyvsp[-6].list);
			if ((yyvsp[-2].list) != NIL) { fc->agg_order = (yyvsp[-2].list); fc->agg_within_group = true; }
			if ((yyvsp[-1].node) != NULL) fc->agg_filter = (yyvsp[-1].node);
			if ((yyvsp[0].node) != NULL) fc->over = (yyvsp[0].node);
			(yyval.node) = (Node*)fc;
		} else {
			FuncCall *fc = makeFuncCall((yyvsp[-6].list), (List*)(yyvsp[-4].node), (yylsp[-6]).first_line);
            fc->end_location = (yylsp[-3]).first_line + 1; /* ReqID:TECC4.0 */
			if ((yyvsp[-2].list) != NIL) { fc->agg_order = (yyvsp[-2].list); fc->agg_within_group = true; }
			if ((yyvsp[-1].node) != NULL) fc->agg_filter = (yyvsp[-1].node);
			if ((yyvsp[0].node) != NULL) fc->over = (yyvsp[0].node);
			(yyval.node) = (Node*)fc;
		}
	}
#line 6587 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 399:
#line 1819 "tecc_grammar.y" /* yacc.c:1646  */
    {
		FuncCall *fc = makeFuncCall((yyvsp[-7].list), list_make1((yyvsp[-4].node)), (yylsp[-7]).first_line);
		fc->func_variadic = true;
		if ((yyvsp[-2].list) != NIL) { fc->agg_order = (yyvsp[-2].list); fc->agg_within_group = true; }
		if ((yyvsp[-1].node) != NULL) fc->agg_filter = (yyvsp[-1].node);
		if ((yyvsp[0].node) != NULL) fc->over = (yyvsp[0].node);
		(yyval.node) = (Node*)fc;
	}
#line 6600 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 400:
#line 1829 "tecc_grammar.y" /* yacc.c:1646  */
    {
		FuncCall *fc = makeFuncCall((yyvsp[-9].list), lappend((yyvsp[-7].list), (yyvsp[-4].node)), (yylsp[-9]).first_line);
		fc->func_variadic = true;
		if ((yyvsp[-2].list) != NIL) { fc->agg_order = (yyvsp[-2].list); fc->agg_within_group = true; }
		if ((yyvsp[-1].node) != NULL) fc->agg_filter = (yyvsp[-1].node);
		if ((yyvsp[0].node) != NULL) fc->over = (yyvsp[0].node);
		(yyval.node) = (Node*)fc;
	}
#line 6613 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 401:
#line 1840 "tecc_grammar.y" /* yacc.c:1646  */
    {
        FuncCall *fc = makeFuncCall(NIL, NIL, (yylsp[0]).first_line);
        fc->agg_star = true;
        (yyval.node) = (Node*)fc;
    }
#line 6623 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 402:
#line 1845 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = NULL;
    }
#line 6631 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 403:
#line 1848 "tecc_grammar.y" /* yacc.c:1646  */
    {
        FuncCall *fc = makeFuncCall(NIL, (List*)(yyvsp[0].list), (yylsp[-1]).first_line);
        fc->agg_distinct = true;
        (yyval.node) = (Node*)fc;
    }
#line 6641 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 404:
#line 1853 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node*)(yyvsp[0].list);
    }
#line 6649 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 405:
#line 1856 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node*)(yyvsp[0].list); }
#line 6655 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 406:
#line 1857 "tecc_grammar.y" /* yacc.c:1646  */
    {
        FuncCall *fc = makeFuncCall(NIL, (List*)(yyvsp[-3].list), (yylsp[-3]).first_line);
        fc->agg_order = (yyvsp[0].list);
        (yyval.node) = (Node*)fc;
    }
#line 6665 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 407:
#line 1862 "tecc_grammar.y" /* yacc.c:1646  */
    {
        FuncCall *fc = makeFuncCall(NIL, (List*)(yyvsp[-3].list), (yylsp[-4]).first_line);
        fc->agg_distinct = true;
        fc->agg_order = (yyvsp[0].list);
        (yyval.node) = (Node*)fc;
    }
#line 6676 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 408:
#line 1872 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 6682 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 409:
#line 1873 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 6688 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 410:
#line 1879 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = (yyvsp[-1].list);
    }
#line 6696 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 411:
#line 1883 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = NIL;
    }
#line 6704 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 412:
#line 1890 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 6710 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 413:
#line 1891 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[-1].node); }
#line 6716 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 414:
#line 1896 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 6722 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 415:
#line 1897 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6728 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 416:
#line 1903 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeWindowDef(NULL, (yyvsp[0].str), NIL, NIL, (yylsp[0]).first_line);
    }
#line 6736 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 417:
#line 1907 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeWindowDef(NULL, NULL, (yyvsp[-2].list), (yyvsp[-1].list), (yylsp[-3]).first_line);
    }
#line 6744 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 418:
#line 1913 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 6750 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 419:
#line 1914 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 6756 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 420:
#line 1919 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 6762 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 421:
#line 1920 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 6768 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 422:
#line 1925 "tecc_grammar.y" /* yacc.c:1646  */
    {
        WindowDef *wd = (WindowDef *)(yyvsp[0].node);
        wd->name = (yyvsp[-2].str);
        (yyval.node) = (Node *)wd;
    }
#line 6778 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 423:
#line 1932 "tecc_grammar.y" /* yacc.c:1646  */
    {
        char *endptr;
        errno = 0;
        (void)strtoll((yyvsp[0].str), &endptr, 10);
        if (errno == ERANGE)
        {
            /* Overflow: preserve exact text as string constant,
             * avoiding precision loss through int64 or float8.
             * A_Expr unary minus handler will prepend '-' for negation. */
            (yyval.node) = makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column);
        }
        else
            (yyval.node) = makeIntConst(atoll((yyvsp[0].str)), (yylsp[0]).first_line, (yylsp[0]).last_column);
    }
#line 6797 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 424:
#line 1946 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - Preserve original text as string to avoid precision
         * loss from double conversion (strtod limits to ~15 significant digits).
         * The formatter handles string→numeric conversion with full precision
         * for all types (float4/8 via Ryu, numeric via digit-by-digit parsing).
         * Pre-requisite for Bug8178: DET equality match for high-precision
         * numeric values (>15 digits) between INSERT and WHERE paths.
         * 注意：用 makeNumericTextConst 而非 makeStringConst —— 同样保留文本，
         * 但为该常量打上数值形态标记，使 GREATEST/LEAST 等参数组合校验按数值族
         * 归类（否则 numeric/float 列 + 小数常量会被误判为"常量类型不匹配"）。 */
        (yyval.node) = makeNumericTextConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column);
    }
#line 6814 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 425:
#line 1958 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column);
    }
#line 6822 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 426:
#line 1961 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, (yylsp[0]).last_column);
    }
#line 6830 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 427:
#line 1964 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = makeNullConst((yylsp[0]).first_line, (yylsp[0]).last_column);
    }
#line 6838 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 428:
#line 1967 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = makeStringConst("true", (yylsp[0]).first_line, 4);
    }
#line 6846 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 429:
#line 1970 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = makeStringConst("false", (yylsp[0]).first_line, 5);
    }
#line 6854 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 430:
#line 1976 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeParam(PARAM_EXTERN, (yyvsp[0].ival), 0, (yylsp[0]).first_line); }
#line 6860 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 431:
#line 1982 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCaseExpr((yyvsp[-4].node), (yyvsp[-1].node), (yyvsp[-3].list), (yylsp[-5]).first_line);
    }
#line 6868 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 432:
#line 1986 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCaseExpr(NULL, (yyvsp[-1].node), (yyvsp[-3].list), (yylsp[-4]).first_line);
    }
#line 6876 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 433:
#line 1990 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCaseExpr((yyvsp[-2].node), NULL, (yyvsp[-1].list), (yylsp[-3]).first_line);
    }
#line 6884 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 434:
#line 1994 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCaseExpr(NULL, NULL, (yyvsp[-1].list), (yylsp[-2]).first_line);
    }
#line 6892 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 435:
#line 2000 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 6898 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 436:
#line 2001 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 6904 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 437:
#line 2006 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeCaseWhen((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]).first_line);
    }
#line 6912 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 438:
#line 2017 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeA_Indices(false, NULL, (yyvsp[-1].node));
    }
#line 6920 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 439:
#line 2021 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeA_Indices(true, (yyvsp[-3].node), (yyvsp[-1].node));
    }
#line 6928 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 440:
#line 2027 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6934 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 441:
#line 2028 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 6940 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 442:
#line 2032 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 6946 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 443:
#line 2033 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 6952 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 444:
#line 2037 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0)); }
#line 6958 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 445:
#line 2038 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), makeStringConst((yyvsp[0].str), (yylsp[0]).first_line, 0)); }
#line 6964 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 446:
#line 2042 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeString((yyvsp[0].str))); }
#line 6970 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 447:
#line 2043 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lcons(makeString((yyvsp[-1].str)), (yyvsp[0].list)); }
#line 6976 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 448:
#line 2044 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeString((yyvsp[0].str))); }
#line 6982 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 449:
#line 2048 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeString((yyvsp[0].str))); }
#line 6988 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 450:
#line 2049 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), makeString((yyvsp[0].str))); }
#line 6994 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 451:
#line 2053 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 7000 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 452:
#line 2054 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "dek"; }
#line 7006 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 453:
#line 2055 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "kek"; }
#line 7012 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 454:
#line 2056 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "key"; }
#line 7018 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 455:
#line 2057 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "format"; }
#line 7024 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 456:
#line 2058 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "time"; }
#line 7030 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 457:
#line 2059 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "type"; }
#line 7036 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 458:
#line 2060 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "text"; }
#line 7042 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 459:
#line 2061 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "value"; }
#line 7048 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 460:
#line 2062 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "temp"; }
#line 7054 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 461:
#line 2063 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "zone"; }
#line 7060 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 462:
#line 2065 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "local"; }
#line 7066 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 463:
#line 2066 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "session"; }
#line 7072 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 464:
#line 2067 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "transaction"; }
#line 7078 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 465:
#line 2068 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "characteristics"; }
#line 7084 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 466:
#line 2069 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "catalog"; }
#line 7090 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 467:
#line 2070 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "names"; }
#line 7096 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 468:
#line 2071 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "role"; }
#line 7102 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 469:
#line 2072 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "authorization"; }
#line 7108 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 470:
#line 2073 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "xml"; }
#line 7114 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 471:
#line 2074 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "option"; }
#line 7120 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 472:
#line 2075 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "document"; }
#line 7126 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 473:
#line 2076 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "content"; }
#line 7132 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 474:
#line 2077 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "snapshot"; }
#line 7138 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 475:
#line 2078 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "isolation"; }
#line 7144 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 476:
#line 2079 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "level"; }
#line 7150 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 477:
#line 2080 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "read"; }
#line 7156 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 478:
#line 2081 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "uncommitted"; }
#line 7162 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 479:
#line 2082 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "committed"; }
#line 7168 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 480:
#line 2083 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "repeatable"; }
#line 7174 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 481:
#line 2084 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "serializable"; }
#line 7180 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 482:
#line 2085 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "only"; }
#line 7186 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 483:
#line 2086 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "write"; }
#line 7192 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 484:
#line 2087 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "current"; }
#line 7198 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 485:
#line 2091 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 7204 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 486:
#line 2095 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = atoll((yyvsp[0].str)); }
#line 7210 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 487:
#line 2099 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 7216 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 488:
#line 2103 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 7222 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 489:
#line 2107 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget(NULL, (yyvsp[0].node), (yylsp[0]).first_line);
        (yyval.list) = list_make1((Node *)res);
    }
#line 7231 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 490:
#line 2111 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget((yyvsp[0].str), (yyvsp[-2].node), (yylsp[-1]).first_line);
        (yyval.list) = list_make1((Node *)res);
    }
#line 7240 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 491:
#line 2116 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget((yyvsp[0].str), (yyvsp[-1].node), (yylsp[-1]).first_line);
        (yyval.list) = list_make1((Node *)res);
    }
#line 7249 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 492:
#line 2120 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget(NULL, (yyvsp[0].node), (yylsp[0]).first_line);
        (yyval.list) = lappend((yyvsp[-2].list), (Node *)res);
    }
#line 7258 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 493:
#line 2124 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget((yyvsp[0].str), (yyvsp[-2].node), (yylsp[-1]).first_line);
        (yyval.list) = lappend((yyvsp[-4].list), (Node *)res);
    }
#line 7267 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 494:
#line 2129 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *res = makeResTarget((yyvsp[0].str), (yyvsp[-1].node), (yylsp[-1]).first_line);
        (yyval.list) = lappend((yyvsp[-3].list), (Node *)res);
    }
#line 7276 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 495:
#line 2133 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* SELECT * case - create a special representation */
        /* Instead of A_Star node, create a ResTarget with a ColumnRef containing "*" */
        {
            ColumnRef *cref = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
            ResTarget *res = (ResTarget *)tecc_palloc(sizeof(ResTarget));

            memset(cref, 0, sizeof(ColumnRef));
            cref->type = FE_T_ColumnRef;
            cref->fields = list_make1(makeStringConst("*", (yylsp[0]).first_line, 0));
            cref->location = (yylsp[0]).first_line;

            memset(res, 0, sizeof(ResTarget));
            res->type = FE_T_ResTarget;
            res->name = NULL;  /* No alias */
            res->indirection = NIL;
            res->val = (Node *)cref;
            res->location = (yylsp[0]).first_line;

            (yyval.list) = list_make1((Node *)res);
        }
    }
#line 7303 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 496:
#line 2155 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* SELECT 'literal', * FROM ... */
        {
            ColumnRef *cref = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
            ResTarget *res = (ResTarget *)tecc_palloc(sizeof(ResTarget));

            memset(cref, 0, sizeof(ColumnRef));
            cref->type = FE_T_ColumnRef;
            cref->fields = list_make1(makeStringConst("*", (yylsp[0]).first_line, 0));
            cref->location = (yylsp[0]).first_line;

            memset(res, 0, sizeof(ResTarget));
            res->type = FE_T_ResTarget;
            res->name = NULL;
            res->indirection = NIL;
            res->val = (Node *)cref;
            res->location = (yylsp[0]).first_line;

            (yyval.list) = lappend((yyvsp[-2].list), (Node *)res);
        }
    }
#line 7329 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 497:
#line 2180 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7335 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 498:
#line 2184 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].str)); }
#line 7341 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 499:
#line 2185 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].str)); }
#line 7347 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 500:
#line 2190 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 7353 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 501:
#line 2191 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 7359 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 502:
#line 2195 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 7365 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 503:
#line 2196 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 7371 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 504:
#line 2201 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = NIL;
    }
#line 7379 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 505:
#line 2204 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = list_make1(makeIntConst(1, (yylsp[0]).first_line, 0));
    }
#line 7387 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 506:
#line 2207 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = NIL;
    }
#line 7395 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 507:
#line 2211 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = (yyvsp[-1].list);
    }
#line 7403 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 508:
#line 2217 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7409 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 509:
#line 2218 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7415 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 510:
#line 2222 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 7421 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 511:
#line 2223 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 7427 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 512:
#line 2227 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7433 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 513:
#line 2229 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_INNER, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]).first_line);
    }
#line 7441 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 514:
#line 2233 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_INNER, (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]).first_line);
    }
#line 7449 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 515:
#line 2237 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_LEFT, (yyvsp[-6].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]).first_line);
    }
#line 7457 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 516:
#line 2241 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_RIGHT, (yyvsp[-6].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]).first_line);
    }
#line 7465 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 517:
#line 2245 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_FULL, (yyvsp[-6].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]).first_line);
    }
#line 7473 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 518:
#line 2249 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeJoinExpr(JOIN_CROSS, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line);
    }
#line 7481 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 519:
#line 2254 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line);
        j->isNatural = true;
        (yyval.node) = (Node *)j;
    }
#line 7491 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 520:
#line 2260 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_LEFT, (yyvsp[-4].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line);
        j->isNatural = true;
        (yyval.node) = (Node *)j;
    }
#line 7501 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 521:
#line 2266 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_RIGHT, (yyvsp[-4].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line);
        j->isNatural = true;
        (yyval.node) = (Node *)j;
    }
#line 7511 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 522:
#line 2272 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_FULL, (yyvsp[-4].node), (yyvsp[0].node), NULL, (yylsp[-2]).first_line);
        j->isNatural = true;
        (yyval.node) = (Node *)j;
    }
#line 7521 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 523:
#line 2279 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, (yyvsp[-6].node), (yyvsp[-4].node), NULL, (yylsp[-5]).first_line);
        j->usingClause = (yyvsp[-1].list);
        (yyval.node) = (Node *)j;
    }
#line 7531 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 524:
#line 2285 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, (yyvsp[-7].node), (yyvsp[-4].node), NULL, (yylsp[-6]).first_line);
        j->usingClause = (yyvsp[-1].list);
        (yyval.node) = (Node *)j;
    }
#line 7541 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 525:
#line 2291 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_LEFT, (yyvsp[-8].node), (yyvsp[-4].node), NULL, (yylsp[-7]).first_line);
        j->usingClause = (yyvsp[-1].list);
        (yyval.node) = (Node *)j;
    }
#line 7551 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 526:
#line 2297 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_RIGHT, (yyvsp[-8].node), (yyvsp[-4].node), NULL, (yylsp[-7]).first_line);
        j->usingClause = (yyvsp[-1].list);
        (yyval.node) = (Node *)j;
    }
#line 7561 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 527:
#line 2303 "tecc_grammar.y" /* yacc.c:1646  */
    {
        JoinExpr *j = makeJoinExpr(JOIN_FULL, (yyvsp[-8].node), (yyvsp[-4].node), NULL, (yylsp[-7]).first_line);
        j->usingClause = (yyvsp[-1].list);
        (yyval.node) = (Node *)j;
    }
#line 7571 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 528:
#line 2312 "tecc_grammar.y" /* yacc.c:1646  */
    {
        RangeVar *rv = (RangeVar *)(yyvsp[-1].node);
        rv->alias = (Alias *)(yyvsp[0].node);
        (yyval.node) = (Node *)rv;
    }
#line 7581 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 529:
#line 2318 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeRangeSubselect((yyvsp[-2].node), (Alias *)(yyvsp[0].node), (yylsp[-3]).first_line);
    }
#line 7589 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 530:
#line 2322 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7597 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 531:
#line 2326 "tecc_grammar.y" /* yacc.c:1646  */
    {
        RangeSubselect *rss = makeRangeSubselect((yyvsp[-2].node), (Alias *)(yyvsp[0].node), (yylsp[-4]).first_line);
        rss->lateral = true;
        (yyval.node) = (Node *)rss;
    }
#line 7607 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 532:
#line 2332 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeRangeFunction((yyvsp[-1].node), (Alias *)(yyvsp[0].node), false, (yylsp[-1]).first_line);
    }
#line 7615 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 533:
#line 2336 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeRangeFunction((yyvsp[-1].node), (Alias *)(yyvsp[0].node), true, (yylsp[-1]).first_line);
    }
#line 7623 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 536:
#line 2347 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 7629 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 537:
#line 2348 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlias((yyvsp[0].str), NIL); }
#line 7635 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 538:
#line 2349 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlias((yyvsp[-3].str), (yyvsp[-1].list)); }
#line 7641 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 539:
#line 2350 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlias((yyvsp[0].str), NIL); }
#line 7647 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 540:
#line 2351 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeAlias((yyvsp[-3].str), (yyvsp[-1].list)); }
#line 7653 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 541:
#line 2355 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeRangeVar(NULL, (yyvsp[0].str), (yylsp[0]).first_line); }
#line 7659 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 542:
#line 2356 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (Node *)makeRangeVar((yyvsp[-2].str), (yyvsp[0].str), (yylsp[-2]).first_line); }
#line 7665 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 543:
#line 2362 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7673 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 544:
#line 2366 "tecc_grammar.y" /* yacc.c:1646  */
    {
        RangeVar *rv = (RangeVar *)(yyvsp[-1].node);
        rv->alias = makeAlias((yyvsp[0].str), NIL);
        (yyval.node) = (Node *)rv;
    }
#line 7683 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 545:
#line 2372 "tecc_grammar.y" /* yacc.c:1646  */
    {
        RangeVar *rv = (RangeVar *)(yyvsp[-2].node);
        rv->alias = makeAlias((yyvsp[0].str), NIL);
        (yyval.node) = (Node *)rv;
    }
#line 7693 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 546:
#line 2380 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 7699 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 547:
#line 2381 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7705 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 548:
#line 2385 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7711 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 549:
#line 2386 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7717 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 550:
#line 2390 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 7723 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 551:
#line 2391 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7729 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 552:
#line 2395 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7735 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 553:
#line 2396 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7741 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 554:
#line 2400 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7747 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 555:
#line 2401 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7753 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 556:
#line 2405 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 7759 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 557:
#line 2406 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 7765 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 558:
#line 2411 "tecc_grammar.y" /* yacc.c:1646  */
    {
            SortBy *sb = (SortBy *)tecc_palloc(sizeof(SortBy));
            sb->type = FE_T_SortBy;
            sb->node = (yyvsp[-2].node);
            sb->sortby_dir = (yyvsp[-1].ival);
            sb->sortby_nulls = (yyvsp[0].ival);
            sb->useOp = NIL;
            sb->location = (yylsp[-2]).first_line;
            (yyval.node) = (Node *)sb;
        }
#line 7780 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 559:
#line 2423 "tecc_grammar.y" /* yacc.c:1646  */
    {
            SortBy *sb = (SortBy *)tecc_palloc(sizeof(SortBy));
            sb->type = FE_T_SortBy;
            sb->node = (yyvsp[-4].node);
            sb->sortby_dir = (yyvsp[-1].ival);
            sb->sortby_nulls = (yyvsp[0].ival);
            sb->useOp = (List *)(yyvsp[-2].node);
            sb->location = (yylsp[-4]).first_line;
            (yyval.node) = (Node *)sb;
        }
#line 7795 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 560:
#line 2436 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_DEFAULT; }
#line 7801 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 561:
#line 2437 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_ASC; }
#line 7807 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 562:
#line 2438 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_DESC; }
#line 7813 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 563:
#line 2442 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_NULLS_DEFAULT; }
#line 7819 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 564:
#line 2443 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_NULLS_FIRST; }
#line 7825 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 565:
#line 2444 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = SORTBY_NULLS_LAST; }
#line 7831 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 566:
#line 2448 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 7837 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 567:
#line 2449 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7843 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 568:
#line 2453 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 7849 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 569:
#line 2454 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7855 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 570:
#line 2459 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7861 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 571:
#line 2460 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7867 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 572:
#line 2464 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 7873 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 573:
#line 2465 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 7879 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 574:
#line 2470 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Store locking info as a DefElem: defname = lock strength str */
        DefElem *de = makeDefElem((yyvsp[-2].str), (Node *)(yyvsp[-1].list), (yylsp[-3]).first_line);
        (yyval.node) = (Node *)de;
    }
#line 7889 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 575:
#line 2478 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "for update"; }
#line 7895 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 576:
#line 2479 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "for no key update"; }
#line 7901 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 577:
#line 2480 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "for share"; }
#line 7907 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 578:
#line 2481 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.str) = "for key share"; }
#line 7913 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 579:
#line 2485 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7919 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 580:
#line 2486 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 7925 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 581:
#line 2490 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7931 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 582:
#line 2491 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeStringConst("nowait", (yylsp[0]).first_line, 0)); }
#line 7937 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 583:
#line 2492 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1(makeStringConst("skip locked", (yylsp[-1]).first_line, 0)); }
#line 7943 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 584:
#line 2497 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 7949 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 585:
#line 2498 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = convert_string_list_to_restargets((yyvsp[-1].list), (yylsp[-2]).first_line); }
#line 7955 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 586:
#line 2503 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = OVERRIDING_NOT_SET; }
#line 7961 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 587:
#line 2504 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = OVERRIDING_USER_VALUE; }
#line 7967 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 588:
#line 2505 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = OVERRIDING_SYSTEM_VALUE; }
#line 7973 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 589:
#line 2509 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7979 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 590:
#line 2511 "tecc_grammar.y" /* yacc.c:1646  */
    {
        SelectStmt *select = makeSelectStmt((yylsp[-1]).first_line, (yyloc).last_column - (yylsp[-1]).first_column + 1);
        select->defaultValues = true;
        (yyval.node) = (Node *)select;
    }
#line 7989 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 591:
#line 2519 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Create a simple SELECT statement for VALUES */
        SelectStmt *select = makeSelectStmt((yylsp[-3]).first_line, (yyloc).last_column - (yylsp[-3]).first_column + 1);

        /* For INSERT...VALUES, store the expression list in valuesLists */
        /* valuesLists is a list of lists: outer list = rows, inner list = column values */
        List *single_row_values = (yyvsp[-1].list);  /* expr_list is already a List of expressions */
        List *values_lists = list_make1(single_row_values);

        select->valuesLists = values_lists;
        select->targetList = NIL;  /* targetList should be empty for VALUES clause */
        (yyval.node) = (Node *)select;
    }
#line 8007 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 592:
#line 2532 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Append more values to existing SELECT statement */
        SelectStmt *select = (SelectStmt *)(yyvsp[-4].node);

        /* Add another row to valuesLists */
        if (select->valuesLists == NIL) {
            select->valuesLists = list_make1((yyvsp[-1].list));
        } else {
            select->valuesLists = lappend(select->valuesLists, (yyvsp[-1].list));
        }

        (yyval.node) = (Node *)select;
    }
#line 8025 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 593:
#line 2548 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8031 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 594:
#line 2550 "tecc_grammar.y" /* yacc.c:1646  */
    {
        OnConflictClause *occ = (OnConflictClause *)(yyvsp[0].node);
        occ->infer = (Node *)(yyvsp[-1].node);
        (yyval.list) = list_make1((Node *)occ);
    }
#line 8041 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 595:
#line 2558 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 8047 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 596:
#line 2560 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeInferClause((List *)(yyvsp[-2].list), (yyvsp[0].node), NULL, (yylsp[-3]).first_line);
    }
#line 8055 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 597:
#line 2564 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeInferClause(NIL, NULL, (yyvsp[0].str), (yylsp[0]).first_line);
    }
#line 8063 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 598:
#line 2571 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeOnConflictClause(ONCONFLICT_NOTHING, (yylsp[-1]).first_line);
    }
#line 8071 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 599:
#line 2575 "tecc_grammar.y" /* yacc.c:1646  */
    {
        OnConflictClause *occ = makeOnConflictClause(ONCONFLICT_UPDATE, (yylsp[-4]).first_line);
        occ->targetList = (yyvsp[-1].list);
        occ->whereClause = (yyvsp[0].node);
        (yyval.node) = (Node *)occ;
    }
#line 8082 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 600:
#line 2586 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 8088 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 601:
#line 2588 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ListCell *scl_lc;
        List *sc = (List *)(yyvsp[0].list);  /* cast from bison void* union */
        foreach(scl_lc, sc) { (yyvsp[-2].list) = lappend((yyvsp[-2].list), lfirst(scl_lc)); }
        (yyval.list) = (yyvsp[-2].list);
    }
#line 8099 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 602:
#line 2599 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ResTarget *target = makeResTarget((yyvsp[-2].str), (yyvsp[0].node), (yylsp[-1]).first_line);
        (yyval.list) = list_make1((Node *)target);
    }
#line 8108 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 603:
#line 2604 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Multi-column SET: (col1, col2, ...) = expr.
         * Create a ResTarget for each target column with the shared expr.
         * Processor handles positional extraction from RowExpr/SubLink. */
        ListCell *lc;
        List *result = NIL;
        List *cols = (List *)(yyvsp[-3].list);  /* cast from bison void* union */
        foreach(lc, cols)
        {
            char *colname = (char *) lfirst(lc);
            ResTarget *target = makeResTarget(colname, (yyvsp[0].node), (yylsp[-2]).first_line);
            result = lappend(result, (Node *)target);
        }
        (yyval.list) = result;
    }
#line 8128 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 604:
#line 2623 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].str)); }
#line 8134 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 605:
#line 2624 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].str)); }
#line 8140 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 606:
#line 2629 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8146 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 607:
#line 2630 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 8152 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 608:
#line 2635 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8158 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 609:
#line 2636 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 8164 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 610:
#line 2641 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = DROP_RESTRICT; }
#line 8170 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 611:
#line 2642 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = DROP_CASCADE; }
#line 8176 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 612:
#line 2643 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.ival) = DROP_RESTRICT; }
#line 8182 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 613:
#line 2648 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 8188 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 614:
#line 2649 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 8194 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 615:
#line 2653 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 8200 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 616:
#line 2654 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 8206 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 617:
#line 2659 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8215 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 618:
#line 2664 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8224 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 619:
#line 2669 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8233 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 620:
#line 2674 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8242 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 621:
#line 2682 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - ENCRYPTED_VALUE is auto-generated, reject at parse time */
        DefElem *elem;
        if (strcasecmp((yyvsp[-2].str), "encrypted_value") == 0)
        {
            tecc_yyerror("ENCRYPTED_VALUE is auto-generated and cannot be specified in CREATE DEK");
            YYABORT;
        }
        elem = makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8258 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 622:
#line 2694 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - ENCRYPTED_VALUE is auto-generated, reject at parse time */
        DefElem *elem;
        if (strcasecmp((yyvsp[-2].str), "encrypted_value") == 0)
        {
            tecc_yyerror("ENCRYPTED_VALUE is auto-generated and cannot be specified in CREATE DEK");
            YYABORT;
        }
        elem = makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8274 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 623:
#line 2706 "tecc_grammar.y" /* yacc.c:1646  */
    {
        char buf[NAMEDATALEN * 4];
        ListCell *lc;
        char *p = buf;
        foreach(lc, (List *)(yyvsp[0].list))
        {
            if (p != buf)
                *p++ = '.';
            p += sprintf(p, "%s", strVal((const Value *)lfirst(lc)));
        }
        (yyval.node) = (Node *)makeDefElem("kek", (Node *)makeStringConst(tecc_pstrdup(buf), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
    }
#line 8291 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 624:
#line 2719 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8300 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 625:
#line 2724 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8309 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 626:
#line 2733 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8315 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 627:
#line 2734 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8321 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 628:
#line 2735 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8327 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 629:
#line 2736 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8333 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 630:
#line 2740 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 8339 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 631:
#line 2741 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8345 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 632:
#line 2745 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 8351 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 633:
#line 2746 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8357 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 634:
#line 2751 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makePrepareStmt((yyvsp[-3].str), (yyvsp[-2].list), (yyvsp[0].node), (yylsp[-4]).first_line);
    }
#line 8365 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 635:
#line 2758 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeExecuteStmt((yyvsp[-1].str), (yyvsp[0].list), 0, NULL, NULL, NULL, NULL, (yylsp[-2]).first_line);
    }
#line 8373 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 636:
#line 2765 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDeallocateStmt((yyvsp[0].str), (yylsp[-1]).first_line);
    }
#line 8381 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 637:
#line 2769 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDeallocateStmt((yyvsp[0].str), (yylsp[-2]).first_line);
    }
#line 8389 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 638:
#line 2773 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDeallocateStmt(NULL, (yylsp[-1]).first_line);
    }
#line 8397 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 639:
#line 2777 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDeallocateStmt(NULL, (yylsp[-2]).first_line);
    }
#line 8405 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 640:
#line 2783 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = true; }
#line 8411 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 641:
#line 2784 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = false; }
#line 8417 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 642:
#line 2785 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.bval) = true; }
#line 8423 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 643:
#line 2790 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ExecuteStmt *execStmt;
        CreateTableAsStmt *ctas;

        execStmt = makeExecuteStmt((yyvsp[-2].str), (yyvsp[-1].list), 0, NULL, NULL, NULL, NULL, (yylsp[-8]).first_line);
        ctas = makeCreateTableAsStmt((RangeVar *)(yyvsp[-5].node), (Node *)execStmt, false, (yylsp[-8]).first_line);
        (yyval.node) = (Node *)ctas;
    }
#line 8436 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 644:
#line 2799 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[-1].node), false, (yylsp[-6]).first_line);
        (yyval.node) = (Node *)ctas;
    }
#line 8447 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 645:
#line 2806 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ExecuteStmt *execStmt;
        CreateTableAsStmt *ctas;

        execStmt = makeExecuteStmt((yyvsp[-2].str), (yyvsp[-1].list), 0, NULL, NULL, NULL, NULL, (yylsp[-11]).first_line);
        ctas = makeCreateTableAsStmt((RangeVar *)(yyvsp[-5].node), (Node *)execStmt, true, (yylsp[-11]).first_line);
        (yyval.node) = (Node *)ctas;
    }
#line 8460 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 646:
#line 2815 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[-1].node), true, (yylsp[-9]).first_line);
        (yyval.node) = (Node *)ctas;
    }
#line 8471 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 647:
#line 2830 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), false, false, (yyvsp[-2].list), (yylsp[-5]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8480 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 648:
#line 2835 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), true, false, (yyvsp[-2].list), (yylsp[-7]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8489 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 649:
#line 2840 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), false, true, (yyvsp[-2].list), (yylsp[-6]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8498 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 650:
#line 2845 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), true, true, (yyvsp[-2].list), (yylsp[-8]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8507 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 651:
#line 2850 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), false, true, (yyvsp[-2].list), (yylsp[-6]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8516 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 652:
#line 2855 "tecc_grammar.y" /* yacc.c:1646  */
    {
        ViewStmt *n = makeViewStmt((RangeVar *)(yyvsp[-3].node), (Node *)(yyvsp[0].node), true, true, (yyvsp[-2].list), (yylsp[-8]).first_line);
        (yyval.node) = (Node *)n;
    }
#line 8525 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 653:
#line 2865 "tecc_grammar.y" /* yacc.c:1646  */
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)(yyvsp[-4].node), (Node *)(yyvsp[-1].node), false, (yylsp[-7]).first_line);
        ctas->objtype = 1;      /* OBJECT_MATVIEW */
        ctas->skipData = !(yyvsp[0].bval);   /* opt_with_data: true=WITH DATA, false=WITH NO DATA */
        (yyval.node) = (Node *)ctas;
    }
#line 8538 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 654:
#line 2877 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)makeDeclareCursorStmt((yyvsp[-3].str), (yyvsp[0].node), (yylsp[-4]).first_line);
    }
#line 8546 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 655:
#line 2884 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Full implementation for COPY FROM STDIN */
        (yyval.node) = (Node *)makeCopyStmt((struct RangeVar *)(yyvsp[-4].node), NULL, (yyvsp[-3].list), true, NULL, (yyvsp[0].list),
                                  (yylsp[-5]).first_line, (yyloc).last_line - (yylsp[-5]).first_line + 1);
    }
#line 8556 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 656:
#line 2890 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Full implementation for COPY TO STDOUT */
        (yyval.node) = (Node *)makeCopyStmt((struct RangeVar *)(yyvsp[-4].node), NULL, (yyvsp[-3].list), false, NULL, (yyvsp[0].list),
                                  (yylsp[-5]).first_line, (yyloc).last_line - (yylsp[-5]).first_line + 1);
    }
#line 8566 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 657:
#line 2896 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* Implementation for COPY (SELECT ...) TO STDOUT */
        (yyval.node) = (Node *)makeCopyStmt(NULL, (yyvsp[-4].node), NULL, false, NULL, (yyvsp[0].list),
                                  (yylsp[-6]).first_line, (yyloc).last_line - (yylsp[-6]).first_line + 1);
    }
#line 8576 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 658:
#line 2904 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = NIL; }
#line 8582 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 659:
#line 2905 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 8588 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 660:
#line 2906 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[-1].list); }
#line 8594 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 661:
#line 2907 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 8600 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 662:
#line 2908 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = (yyvsp[0].list); }
#line 8606 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 663:
#line 2912 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 8612 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 664:
#line 2913 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-1].list), (yyvsp[0].node)); }
#line 8618 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 665:
#line 2917 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = list_make1((yyvsp[0].node)); }
#line 8624 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 666:
#line 2918 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node)); }
#line 8630 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 667:
#line 2923 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem((yyvsp[-2].str), (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-2]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8639 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 668:
#line 2928 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8648 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 669:
#line 2933 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* FORMAT CSV (无引号关键字形式) — \copy 元命令构建的 SQL 使用此语法 */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("csv", (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8658 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 670:
#line 2939 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* FORMAT BINARY (无引号关键字形式) */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("binary", (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8668 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 671:
#line 2945 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* FORMAT TEXT (无引号关键字形式) — 默认格式 */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("text", (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8678 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 672:
#line 2951 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("delimiter", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8687 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 673:
#line 2956 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("null", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8696 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 674:
#line 2961 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("encoding", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8705 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 675:
#line 2966 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("csv", (yylsp[0]).first_line, 0), (yylsp[0]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8714 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 676:
#line 2971 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("binary", (yylsp[0]).first_line, 0), (yylsp[0]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8723 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 677:
#line 2976 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("text", (yylsp[0]).first_line, 0), (yylsp[0]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8732 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 678:
#line 2981 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* HEADER 裸关键字形式（不带参数，等效于 HEADER true） */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("true", (yylsp[0]).first_line, 0), (yylsp[0]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8742 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 679:
#line 2987 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* HEADER true（显式 boolean 参数） */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("true", (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8752 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 680:
#line 2993 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* HEADER false */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("false", (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8762 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 681:
#line 2999 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("quote", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8771 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 682:
#line 3004 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("escape", (Node *)makeStringConst((yyvsp[0].str), (yylsp[-1]).first_line, 0), (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8780 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 683:
#line 3009 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("force_quote", (Node *)(yyvsp[-1].list), (yylsp[-3]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8789 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 684:
#line 3014 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("force_not_null", (Node *)(yyvsp[-1].list), (yylsp[-3]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8798 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 685:
#line 3019 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* FORCE_QUOTE * - 将所有列强制加引号，存储 "*" 字符串标记 */
        List *star_list = list_make1((Node *)makeStringConst("*", (yylsp[-1]).first_line, 0));
        DefElem *elem = makeDefElem("force_quote", (Node *)star_list, (yylsp[-1]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8809 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 686:
#line 3026 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* FREEZE 选项，相当于 FREEZE true */
        DefElem *elem = makeDefElem("freeze", (Node *)makeStringConst("true", (yylsp[0]).first_line, 0), (yylsp[0]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8819 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 687:
#line 3032 "tecc_grammar.y" /* yacc.c:1646  */
    {
        DefElem *elem = makeDefElem("force_null", (Node *)(yyvsp[-1].list), (yylsp[-3]).first_line);
        (yyval.node) = (Node *)elem;
    }
#line 8828 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 688:
#line 3044 "tecc_grammar.y" /* yacc.c:1646  */
    {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = (yyvsp[0].node);
            n->options = NIL;
            (yyval.node) = (Node *)n;
        }
#line 8840 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 689:
#line 3052 "tecc_grammar.y" /* yacc.c:1646  */
    {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = (yyvsp[0].node);
            n->options = NIL;
            (yyval.node) = (Node *)n;
        }
#line 8852 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 690:
#line 3060 "tecc_grammar.y" /* yacc.c:1646  */
    {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = (yyvsp[0].node);
            n->options = NIL;
            (yyval.node) = (Node *)n;
        }
#line 8864 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 691:
#line 3068 "tecc_grammar.y" /* yacc.c:1646  */
    {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = (yyvsp[0].node);
            n->options = NIL;
            (yyval.node) = (Node *)n;
        }
#line 8876 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 692:
#line 3076 "tecc_grammar.y" /* yacc.c:1646  */
    {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = (yyvsp[0].node);
            n->options = NIL;
            (yyval.node) = (Node *)n;
        }
#line 8888 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 693:
#line 3086 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8894 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 694:
#line 3087 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8900 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 695:
#line 3088 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8906 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 696:
#line 3089 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8912 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 697:
#line 3090 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8918 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 700:
#line 3099 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString("=")); }
#line 8924 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 701:
#line 3100 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString("<>")); }
#line 8930 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 702:
#line 3101 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString("<")); }
#line 8936 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 703:
#line 3102 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString(">")); }
#line 8942 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 704:
#line 3103 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString("<=")); }
#line 8948 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 705:
#line 3104 "tecc_grammar.y" /* yacc.c:1646  */
    { (yyval.node) = list_make1(makeString(">=")); }
#line 8954 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 720:
#line 3134 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.node) = (Node *)(yyvsp[0].list);
    }
#line 8962 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 721:
#line 3138 "tecc_grammar.y" /* yacc.c:1646  */
    {
        /* ReqID:TECC4.0 - Set recursive flag on all CTEs */
        set_cte_list_recursive((List *)(yyvsp[0].list));
        (yyval.node) = (Node *)(yyvsp[0].list);
    }
#line 8972 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 722:
#line 3147 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = list_make1((yyvsp[0].node));
    }
#line 8980 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 723:
#line 3151 "tecc_grammar.y" /* yacc.c:1646  */
    {
        (yyval.list) = lappend((yyvsp[-2].list), (yyvsp[0].node));
    }
#line 8988 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 724:
#line 3158 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;

        col_list = (yyvsp[-4].list);
        alias_names = NIL;

        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }

        cte_node = makeCommonTableExpr((yyvsp[-5].str), alias_names, (yyvsp[-1].node), (yylsp[-5]).first_line);
        (yyval.node) = (Node *)cte_node;
    }
#line 9015 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 725:
#line 3182 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = (yyvsp[-4].list);
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr((yyvsp[-5].str), alias_names, (yyvsp[-1].node), (yylsp[-5]).first_line);
        (yyval.node) = (Node *)cte_node;
    }
#line 9039 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 726:
#line 3202 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = (yyvsp[-4].list);
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr((yyvsp[-5].str), alias_names, (yyvsp[-1].node), (yylsp[-5]).first_line);
        (yyval.node) = (Node *)cte_node;
    }
#line 9063 "tecc_grammar.c" /* yacc.c:1646  */
    break;

  case 727:
#line 3222 "tecc_grammar.y" /* yacc.c:1646  */
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = (yyvsp[-4].list);
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr((yyvsp[-5].str), alias_names, (yyvsp[-1].node), (yylsp[-5]).first_line);
        (yyval.node) = (Node *)cte_node;
    }
#line 9087 "tecc_grammar.c" /* yacc.c:1646  */
    break;


#line 9091 "tecc_grammar.c" /* yacc.c:1646  */
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
  *++yylsp = yyloc;

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
      yyerror (YY_("syntax error"));
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
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

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
                      yytoken, &yylval, &yylloc);
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

  yyerror_range[1] = yylsp[1-yylen];
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

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

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
  yyerror (YY_("memory exhausted"));
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
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp);
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
#line 3243 "tecc_grammar.y" /* yacc.c:1906  */


/* Helper function implementations */

/* Helper function implementations */
static Node *make_column_ref_from_list(List *fields, int location) {
    return (Node *)makeColumnRef(fields, location);
}


static pg_attribute_unused() Node *make_func_call_from_list(List *funcname, List *args, int location) {
    return (Node *)makeFuncCall(funcname, args, location);
}

static Node *make_a_expr(int kind, List *name, Node *lexpr, Node *rexpr, int location) {
    return (Node *)makeAExpr(kind, name, lexpr, rexpr, location);
}

static Node *make_a_expr_escape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location) {
    return (Node *)makeAExprWithEscape(kind, name, lexpr, rexpr, escape, location);
}

static Node *make_bool_expr(int boolop, List *args, int location) {
    return (Node *)makeBoolExpr(boolop, args, location);
}

/* ReqID:TECC4.0 - Expand `expr op ANY/SOME/ALL(ARRAY[...])` into OR/AND of
 * element comparisons.  `use_or` non-zero => ANY/SOME (OR), zero => ALL (AND).
 * A single element degrades to a bare comparison. */
static Node *make_any_all_array_expr(void *op_name, void *lhs, List *elements, int use_or, int location) {
    List *operName = (List *) op_name;
    Node *lexpr = (Node *) lhs;
    const char *opstr = (operName != NIL) ?
        strVal((Value *) linitial(operName)) : NULL;
    List *args = NIL;
    ListCell *lc;

    for (lc = list_head(elements); lc != NULL; lc = lnext(lc))
    {
        Node *elem = (Node *) lfirst(lc);
        /* Fresh operator-name list per comparison (no aliasing between nodes). */
        List *elem_name = (opstr != NULL) ? list_make1(makeString(opstr)) : operName;
        Node *cmp = make_a_expr(AEXPR_OP, elem_name, lexpr, elem, location);
        args = lappend(args, cmp);
    }
    if (list_length(args) == 0)
        return NULL;
    if (list_length(args) == 1)
        return (Node *) lfirst(list_head(args));
    return make_bool_expr(use_or ? OR_EXPR : AND_EXPR, args, location);
}

static Node *make_null_test(Node *arg, int nulltesttype, int location) {
    return (Node *)makeNullTest(arg, nulltesttype, location);
}

/* Convert a list of column name strings to a list of ResTarget nodes */
static List *convert_string_list_to_restargets(List *string_list, int location) {
    List *result = NIL;
    ListCell *lc;

    if (!string_list) return NIL;

    foreach(lc, string_list) {
        char *col_name = (char *)lfirst(lc);
        ResTarget *target = makeResTarget(col_name, NULL, location);
        result = lappend(result, target);
    }

    return result;
}

/* Parser entry point */
static pg_attribute_unused() List *tecc_parse_string(const char *str) {
    /* This would be implemented with Bison/Flex integration */
    /* For now, return empty list */
    return NIL;
}

/* Bison parser entry point */
int tecc_parse(void);

/* Bison error reporting function - defined by Bison in tecc_grammar.c */
/* void tecc_yyerror(const char *msg) is generated by Bison */

/* Bison error reporting function */
void tecc_yyerror(const char *msg) {
    extern int tecc_lineno;
    TECC_LOG(TECC_LOG_DEBUG1, "Parser error at line %d: %s", tecc_lineno, msg);
}

/* Alias for bison's expected error function */
void tecc_error(const char *msg) {
    tecc_yyerror(msg);
}

/* Public parser interface */
static pg_attribute_unused() Node *tecc_parse_query(const char *query) {
    /* TODO: Implement proper parsing with Bison */
    /* For now, use the existing simple parser */
    return tecc_parser_parse_node(query);
}
