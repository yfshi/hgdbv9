/*-------------------------------------------------------------------------
 *
 * Utility routines for SQL dumping
 *
 * Basically this is stuff that is useful in both pg_dump and pg_dumpall.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/bin/pg_dump/dumputils.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef DUMPUTILS_H
#define DUMPUTILS_H

#include "libpq-fe.h"
#include "pqexpbuffer.h"
#include "utils/ora_compatible.h"
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "pg_backup.h"
/* End - ReqID:SRS-MYSQL-CASE */

/*
 * Preferred strftime(3) format specifier for printing timestamps in pg_dump
 * and friends.
 *
 * We don't print the timezone on Windows, because the names are long and
 * localized, which means they may contain characters in various random
 * encodings; this has been seen to cause encoding errors when reading the
 * dump script.  Think not to get around that by using %z, because
 * (1) %z is not portable to pre-C99 systems, and
 * (2) %z doesn't actually act differently from %Z on Windows anyway.
 */
#ifndef WIN32
#define PGDUMP_STRFTIME_FMT  "%Y-%m-%d %H:%M:%S %Z"
#else
#define PGDUMP_STRFTIME_FMT  "%Y-%m-%d %H:%M:%S"
#endif

/* Begin - ReqID:SRS-MYSQL-CASE */
#define MYS_LOWRE_CASE_TABLE_NAMES_ZERO 0
#define MYS_LOWRE_CASE_TABLE_NAMES_ONE 1
#define MYS_LOWRE_CASE_TABLE_NAMES_TWO 2
/* End - ReqID:SRS-MYSQL-CASE */

extern bool restoremode; 			/*  bug#M0000238 */
extern DBMode database_mode;

extern CompatibleMode compatible_mode;	/* ReqID:SRS-CMD-PGDUMP */
extern const char *DumpM;	/* ReqID:SRS-CMD-PGDUMP */
extern int caselowernum;	/* ReqID:SRS-MYSQL-CASE */

extern char *sanitize_line(const char *str, bool want_hyphen);
extern bool buildACLCommands(const char *name, const char *subname, const char *nspname,
							 const char *type, const char *acls, const char *racls,
							 const char *owner, const char *prefix, int remoteVersion,
							 PQExpBuffer sql);
extern bool buildDefaultACLCommands(const char *type, const char *nspname,
									const char *acls, const char *racls,
									const char *initacls, const char *initracls,
									const char *owner,
									int remoteVersion,
									PQExpBuffer sql);
extern void buildShSecLabelQuery(const char *catalog_name,
								 Oid objectId, PQExpBuffer sql);
extern void emitShSecLabels(PGconn *conn, PGresult *res,
							PQExpBuffer buffer, const char *objtype, const char *objname);

extern void buildACLQueries(PQExpBuffer acl_subquery, PQExpBuffer racl_subquery,
							PQExpBuffer init_acl_subquery, PQExpBuffer init_racl_subquery,
							const char *acl_column, const char *acl_owner,
							const char *obj_kind, bool binary_upgrade);

extern bool variable_is_guc_list_quote(const char *name);

extern bool SplitGUCList(char *rawstring, char separator,
						 char ***namelist);

extern void makeAlterConfigCommand(PGconn *conn, const char *configitem,
								   const char *type, const char *name,
								   const char *type2, const char *name2,
								   PQExpBuffer buf);

extern void setIdentifierCaseSwitch(PGconn *conn);		/* BUG#M229 */
extern void setNLSdateFormat(PGconn *conn); 				/* BUG:z774 */
extern void SetDumptypeMode(PGconn *conn);
extern void ReSetDumptypeMode(PGconn *conn);

extern void getDatabaseMode(PGconn *conn);
extern AssignDumpMode  getDumpMode(const char *dumpmode);		/*  bug#M0000261 */

/* Begin - ReqID:SRS-MYSQL-CASE */
extern void GetMySQLCompPara(Archive *AHX);
/* End - ReqID:SRS-MYSQL-CASE */

extern char *generate_restrict_key(void);
extern bool valid_restrict_key(const char *restrict_key);

/* Begin - BUG#6400 */
bool highgo_is_dumpACL(const char *name);
/* End - BUG#6400 */

#endif							/* DUMPUTILS_H */
