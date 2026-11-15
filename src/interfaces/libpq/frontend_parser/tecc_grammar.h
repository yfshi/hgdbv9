/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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
#line 18 "tecc_grammar.y" /* yacc.c:1909  */

    long long ival;
    char *str;
    double dval;
    int bval;  /* Use int for bool, will be converted in code */
    void *node;
    void *list;
    void *range;

#line 325 "tecc_grammar.h" /* yacc.c:1909  */
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
