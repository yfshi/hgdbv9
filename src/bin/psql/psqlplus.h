/*------------------------------------------------------------------
 * 
 * File: psqlplus.h
 *
 * Abstract: 
 * 		lexical scanner for client commands.
 *
 * Authored by zhenmingyang@highgo.com, 20221221.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		src/bin/psql/psqlplus.h
 *
 *------------------------------------------------------------------
 */
#ifndef PSQLPLUS_H
#define PSQLPLUS_H

#include "fe_utils/psqlscan_int.h"

#define PSQL_KEYWORD(a,b,c) {a,b,c},

/*
 * Keyword categories --- should match lists in psqlplusparse.y,
 * the keyword categories may not be useful at the moment,  but
 * the name of the "object" created by the some Oracle client
 * command is allowed to have the same name as the keyword ,
 * which can use this to distinguish.
 */
#define UNRESERVED_PSQL_KEYWORD		0
#define RESERVED_PSQL_KEYWORD		1

typedef struct PsqlScanKeyword
{
	const char *name;			/* in lower case */
	int16		value;			/* grammar's token code */
	int16		category;		/* see codes above */
} PsqlScanKeyword;

#define psql_yyget_extra(yyscanner) (*((PsqlScanStateData **) (yyscanner)))

/*
 * You can compile the psql client separately from the source code.
 * In order not to depend on the backend pg_type_d.h file, declare
 * the type OID macro here.  If you modify the OID values of these
 * types, pay attention to keeping in sync with pg_type_d.h.
 */
#define ORACHARCHAROID		9000
#define ORACHARBYTEOID		9001
#define ORAVARCHARCHAROID	9002
#define ORAVARCHARBYTEOID	9003
#define NUMBEROID			9020
#define BINARY_FLOATOID		9022
#define BINARY_DOUBLEOID	9024
/* Begin - ReqID:SRS-PLSQL-CURSOR */
#define SYS_REFCURSOROID	9043

#define NAMEDATALEN 64
#define MAX_PORTALNAME_LEN		NAMEDATALEN
/* End - ReqID:SRS-PLSQL-CURSOR */


/* Maxsize of char and varchar2 within Oracle */
#define MaxOraCharLen 2000
#define MaxOraVarcharLen 32767

#define BINDVARREF 3000

/*
 * The scanner returns extra data about scanned tokens in this union type.
 * Note that this is a subset of the fields used in YYSTYPE of the bison
 * parsers built atop the scanner.
 */
typedef union psql_YYSTYPE
{
	int			ival;
	char	   *str;
	const char *keyword;
} psql_YYSTYPE;

/* callback functions for our flex lexer */
static const Ora_psqlScanCallbacks psqlplus_callbacks = {
	NULL,
};

union YYSTYPE;
extern int	psqlplus_yyparse(yyscan_t yyscanner);
extern int	psqlplus_yylex(union YYSTYPE *lvalp, yyscan_t yyscanner);
extern int orapsql_yylex(psql_YYSTYPE *lvalp, yyscan_t yyscanner);
extern void psqlplus_yyerror(yyscan_t yyscanner, const char *message);
extern void orapsql_yyerror(yyscan_t yyscanner, const char *message);
extern yyscan_t psqlplus_scanner_init(PsqlScanState state);
extern void psqlplus_scanner_finish(yyscan_t yyscanner);
extern char *psqlplus_downcase_identifier(const char *str, int len); /* BUG#Z351 */
#endif   /* PSQLPLUS_H */
