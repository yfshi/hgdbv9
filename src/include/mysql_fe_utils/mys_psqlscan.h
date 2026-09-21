/*-------------------------------------------------------------------------
 *
 * mys_psqlscan.h
 *	  lexical scanner for MySQL compatible SQL commands
 *
 * This lexer used to be part of psql, and that heritage is reflected in
 * the file name as well as function and typedef names, though it can now
 * be used by other frontend programs as well.  It's also possible to extend
 * this lexer with a compatible add-on lexer to handle program-specific
 * backslash commands.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/mysql_fe_utils/mys_psqlscan.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */
#ifndef MYS_PSQLSCAN_H
#define MYS_PSQLSCAN_H

#include "pqexpbuffer.h"


/* Abstract type for lexer's internal state */
typedef struct PsqlScanStateData *PsqlScanState;

/* Termination states for psql_scan() */
typedef enum
{
	MYSPSCAN_SEMICOLON,			/* found command-ending semicolon */
	MYSPSCAN_BACKSLASH,			/* found backslash command */
	MYSPSCAN_INCOMPLETE,			/* end of line, SQL statement incomplete */
	MYSPSCAN_EOL					/* end of line, SQL possibly complete */
} Mys_PsqlScanResult;

/* Prompt type returned by psql_scan() */
typedef enum Mys_promptStatus
{
	MYSPROMPT_READY,
	MYSPROMPT_CONTINUE,
	MYSPROMPT_COMMENT,
	MYSPROMPT_SINGLEQUOTE,
	MYSPROMPT_DOUBLEQUOTE,
	MYSPROMPT_DOLLARQUOTE,
	MYSPROMPT_QQUOTE,		/* ReqID:SRS-SQL-ESCAPECHAR */
	MYSPROMPT_PAREN,
	MYSPROMPT_COPY
} Mys_promptStatus_t;

/* Quoting request types for get_variable() callback */
typedef enum
{
	MYSPQUOTE_PLAIN,				/* just return the actual value */
	MYSPQUOTE_SQL_LITERAL,			/* add quotes to make a valid SQL literal */
	MYSPQUOTE_SQL_IDENT,			/* quote if needed to make a SQL identifier */
	MYSPQUOTE_SHELL_ARG			/* quote if needed to be safe in a shell cmd */
} Mys_psqlScanQuoteType;

/* Callback functions to be used by the lexer */
typedef struct Mys_psqlScanCallbacks
{
	/* Fetch value of a variable, as a free'able string; NULL if unknown */
	/* This pointer can be NULL if no variable substitution is wanted */
	char	   *(*get_variable) (const char *varname, Mys_psqlScanQuoteType quote,
								 void *passthrough);
} Mys_psqlScanCallbacks;

extern PsqlScanState mys_psql_scan_create(const Mys_psqlScanCallbacks *callbacks);
extern void mys_psql_scan_destroy(PsqlScanState state);

extern void mys_psql_scan_set_passthrough(PsqlScanState state, void *passthrough);

extern void mys_psql_scan_setup(PsqlScanState state,
							const char *line, int line_len,
							int encoding, bool std_strings);
extern void mys_psql_scan_finish(PsqlScanState state);

extern Mys_PsqlScanResult mys_psql_scan(PsqlScanState state,
								PQExpBuffer query_buf,
								Mys_promptStatus_t *prompt);

extern void mys_psql_scan_reset(PsqlScanState state);

extern void mys_psql_scan_reselect_sql_lexer(PsqlScanState state);

extern int mys_psql_scan_count_copy_from_stdin(PsqlScanState state);

extern bool mys_psql_scan_in_quote(PsqlScanState state);

/* Begin - ReqID:SRS-SQL-PROC */
extern bool is_mysql_slash(PsqlScanState state, const char *line);
/* End - ReqID:SRS-SQL-PROC */

#endif							/* MYS_PSQLSCAN_H */
