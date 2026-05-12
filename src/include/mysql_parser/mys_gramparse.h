/*-------------------------------------------------------------------------
 *
 * mys_gramparse.h
 *		Shared definitions for the "raw" parser (flex and bison phases only)
 *
 * NOTE: this file is only meant to be included in the core parsing files,
 * ie, parser.c, mys_gram.y, mys_scan.l, and src/backend/mysql_parser/mys_keywords.c.
 * Definitions that are needed outside the core parser should be in parser.h.
 *
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/mysql_parser/mys_gramparse.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */

#ifndef MYS_GRAMPARSE_H
#define MYS_GRAMPARSE_H

#include "nodes/parsenodes.h"
#include "mysql_parser/mys_scanner.h"

/*
 * NB: include mys_gram.h only AFTER including mys_scanner.h, because mys_scanner.h
 * is what #defines YYLTYPE.
 */
#include "mys_gram.h"

typedef enum MysBodyStyle
{
	MysBody_UNKOWN,
	MysBody_FUNC,
	MysBody_ANONYMOUS_BLOCK,
	MysBody_MAYBE_ANONYMOUS_BLOCK_BEGIN,
	MysBody_MAYBE_ANONYMOUS_BLOCK_DECLARE,
	MysBody_MAYBE_ANONYMOUS_BLOCK_LESS_LESS
}MysBodyStyle;

/* Auxiliary data about a token (other than the token type) */
typedef struct
{
	YYSTYPE		lval;			/* semantic information */
	YYLTYPE		lloc;			/* offset in scanbuf */
	int			leng;			/* length in bytes */
} TokenAuxData;

/* Token pushback stack */
#define MAX_PUSHBACKS 16

/*
 * The YY_EXTRA data that a flex scanner allows us to pass around.  Private
 * state needed for raw parsing/lexing goes here.
 */
typedef struct mys_base_yy_extra_type
{
	/*
	 * Fields used by the core scanner.
	 */
	mys_core_yy_extra_type core_yy_extra;

	/*
	 * State variables for ora_base_yylex().
	 */
	bool		have_lookahead; /* is lookahead info valid? */
	int			lookahead_token;	/* one-token lookahead */
	mys_core_YYSTYPE lookahead_yylval;	/* yylval for lookahead token */
	YYLTYPE		lookahead_yylloc;	/* yylloc for lookahead token */
	char	   *lookahead_end;	/* end of current token */
	char		lookahead_hold_char;	/* to be put back at *lookahead_end */

	/*
	 * State variables that belong to the grammar.
	 */
	List	   *parsetree;		/* final parse result is delivered here */

	/* Begin - ReqID:SRS-SQL-PROC */
	/*
	 * The native PG only cache one-token info include yylloc, yylval and token
	 * number in yyextra, IvorySQL cache multiple tokens info using two arrays. 
	 */
	int max_pushbacks;		/* the maxsize of cache array */
	int loc_pushback; 		/* # of used tokens */
	int	num_pushbacks;		/* # of cached tokens */
	int	*pushback_token;			/* token number array */
	TokenAuxData *pushback_auxdata; /* auxdata array */

	MysBodyStyle body_style;
	int          body_start;
	int          body_level;
	/* End - ReqID:SRS-SQL-PROC */
} mys_base_yy_extra_type;

/*
 * In principle we should use yyget_extra() to fetch the yyextra field
 * from a yyscanner struct.  However, flex always puts that field first,
 * and this is sufficiently performance-critical to make it seem worth
 * cheating a bit to use an inline macro.
 */
#define pg_yyget_extra(yyscanner) (*((mys_base_yy_extra_type **) (yyscanner)))


/* from libparser_mysql.c */
extern int	mys_base_yylex(YYSTYPE *lvalp, YYLTYPE *llocp,
					   	   mys_core_yyscan_t yyscanner);
/* from mys_gram.y */
extern void mys_parser_init(mys_base_yy_extra_type *yyext);
extern int	mys_base_yyparse(mys_core_yyscan_t yyscanner);

#endif							/* MYS_GRAMPARSE_H */
