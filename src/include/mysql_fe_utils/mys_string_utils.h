/*-------------------------------------------------------------------------
 *
 * String-processing utility routines for frontend code (MySQL compatibility)
 *
 * Utility functions that interpret backend output or quote strings for
 * assorted contexts.
 *
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/mysql_fe_utils/mys_string_utils.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */
#ifndef MYS_STRING_UTILS_H
#define MYS_STRING_UTILS_H

#include "libpq-fe.h"
#include "pqexpbuffer.h"
#include "mysql_parser/mys_keywords.h"
#include "utils/ora_compatible.h"

/* Global variables controlling behavior of ora_fmtId() */
extern DBMode db_mode;
extern bool native_PG;      /*  BUG: M0000332 */
extern void getDbCompatibleMode(PGconn *conn);

/* Functions */
extern const char *mys_fmtId(const char *identifier, int encoding);
extern void get_mysql_objname(PGconn *conn, Oid oid, char **objname, char **nspname);

#endif							/* MYS_STRING_UTILS_H */
