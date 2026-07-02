/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright (c) 2000-2021, PostgreSQL Global Development Group
 *
 * src/bin/psql/common.h
 */
#ifndef COMMON_H
#define COMMON_H

#include <setjmp.h>

#include "fe_utils/print.h"
#include "fe_utils/psqlscan.h"
#include "oracle_fe_utils/ora_psqlscan.h"  /* ReqID:SRS-PSQL-PARSER */
#include "mysql_fe_utils/mys_psqlscan.h"  /* ReqID:SRS-MISC-MYSQL-FRAME */
#include "libpq-fe.h"
#include "libpq-ivy.h"	/* ReqID:SRS-CMD-PSQL */

extern bool openQueryOutputFile(const char *fname, FILE **fout, bool *is_pipe);
extern bool setQFout(const char *fname);

extern char *psql_get_variable(const char *varname, PsqlScanQuoteType quote,
							   void *passthrough);

extern char *ora_psql_get_variable(const char *varname, Ora_psqlScanQuoteType quote,
								void *passthrough); /* ReqID:SRS-PSQL-PARSER */
/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
extern char *mys_psql_get_variable(const char *varname, Mys_psqlScanQuoteType quote,
								void *passthrough); /* ReqID:SRS-PSQL-PARSER */
/* End - ReqID:SRS-MISC-MYSQL-FRAME */
extern void NoticeProcessor(void *arg, const char *message);

extern volatile bool sigint_interrupt_enabled;

extern sigjmp_buf sigint_interrupt_jmp;

extern void psql_setup_cancel_handler(void);

extern PGresult *PSQLexec(const char *query);
extern int	PSQLexecWatch(const char *query, const printQueryOpt *opt);

extern bool SendQuery(const char *query);

extern bool is_superuser(void);
extern bool standard_strings(void);
extern const char *session_username(void);

extern void expand_tilde(char **filename);

extern bool recognized_connection_string(const char *connstr);
/* Begin - ReqID:SRS-CMD-PSQL */
extern char *psqlplus_skip_space(char *query);
extern HostVariable *get_hostvariables(const char *sql, bool *error);
extern bool SendQuery_PBE(const char *query, HostVariable *hv);
/* End - ReqID:SRS-CMD-PSQL */

/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
extern void getLoginMessage(PGconn *conn);
extern void getExpirationAlarm(PGconn *conn, const char *user_name);
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

/* Begin - ReqID:SRS-SECURITY-GRACETIME */
extern void CheckPasswordGraceNotice(void);
/* End - ReqID:SRS-SECURITY-GRACETIME */

#endif							/* COMMON_H */
