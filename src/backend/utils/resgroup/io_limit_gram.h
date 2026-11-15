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

#ifndef YY_IO_LIMIT_YY_IO_LIMIT_GRAM_H_INCLUDED
# define YY_IO_LIMIT_YY_IO_LIMIT_GRAM_H_INCLUDED
/* Debug traces.  */
#ifndef IO_LIMIT_YYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define IO_LIMIT_YYDEBUG 1
#  else
#   define IO_LIMIT_YYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define IO_LIMIT_YYDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined IO_LIMIT_YYDEBUG */
#if IO_LIMIT_YYDEBUG
extern int io_limit_yydebug;
#endif
/* "%code requires" blocks.  */
#line 19 "io_limit_gram.y" /* yacc.c:1909  */

	#include "utils/cgroup_io_limit.h"

#line 56 "io_limit_gram.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef IO_LIMIT_YYTOKENTYPE
# define IO_LIMIT_YYTOKENTYPE
  enum io_limit_yytokentype
  {
    STAR = 258,
    VALUE_MAX = 259,
    ID = 260,
    IO_KEY = 261,
    NUMBER = 262
  };
#endif

/* Value type.  */
#if ! defined IO_LIMIT_YYSTYPE && ! defined IO_LIMIT_YYSTYPE_IS_DECLARED

union IO_LIMIT_YYSTYPE
{
#line 23 "io_limit_gram.y" /* yacc.c:1909  */

	char *str;
	uint64 integer;
	IOconfig *ioconfig;
	TblSpcIOLimit *tblspciolimit;
	List *list;
	IOconfigItem *ioconfigitem;

#line 85 "io_limit_gram.h" /* yacc.c:1909  */
};

typedef union IO_LIMIT_YYSTYPE IO_LIMIT_YYSTYPE;
# define IO_LIMIT_YYSTYPE_IS_TRIVIAL 1
# define IO_LIMIT_YYSTYPE_IS_DECLARED 1
#endif



int io_limit_yyparse (IOLimitParserContext *context, void *scanner);

#endif /* !YY_IO_LIMIT_YY_IO_LIMIT_GRAM_H_INCLUDED  */
