/*
 * psql - the PostgreSQL interactive terminal
 *
 * Authored by lanke@highgo.com, 20220705.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/bin/psql/mys_psqlscanslash.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 */
/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
#ifndef MYS_PSQLSCANSLASH_H
#define MYS_PSQLSCANSLASH_H

#include "mysql_fe_utils/mys_psqlscan.h"
#include "psqlscanslash.h"

/*typedef char *(*psql_scan_slash_option_hook_type) (PsqlScanState state,
					   enum slash_option_type type,
					   char *quote,
					   bool semicolon);
extern psql_scan_slash_option_hook_type psql_scan_slash_option_parser;*/

extern char *mys_psql_scan_slash_command(PsqlScanState state);

extern char *psql_scan_slash_option(PsqlScanState state,
					   enum slash_option_type type,
					   char *quote,
					   bool semicolon);

extern char *mys_psql_scan_slash_option(PsqlScanState state,
					   enum slash_option_type type,
					   char *quote,
					   bool semicolon);

extern void mys_psql_scan_slash_command_end(PsqlScanState state);

extern PsqlScanStateSave *mys_psql_scan_get_lex_state(PsqlScanState state);

extern void mys_psql_scan_set_lex_state(PsqlScanState state,
									const PsqlScanStateSave *lex_state);

extern int	mys_psql_scan_get_paren_depth(PsqlScanState state);

extern void mys_psql_scan_set_paren_depth(PsqlScanState state, int depth);

extern void mys_dequote_downcase_identifier(char *str, bool downcase, int encoding);

#endif   /* PSQLSCANSLASH_H */
/* End - ReqID:SRS-MISC-MYSQL-FRAME */
