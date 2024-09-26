/*-------------------------------------------------------------------------
 *
 * String-processing utility routines for frontend code (MySQL compatibility)
 *
 * Utility functions that interpret backend output or quote strings for
 * assorted contexts.
 *
 *
 * Portions Copyright (c) 2023, IvorySQL-2.0
 *
 * src/include/mysql_fe_utils/mys_string_utils.h
 *
 * add the file for requirement "MySQL SQL PARSER"
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
extern const char *mys_fmtId(const char *identifier);

#endif							/* MYS_STRING_UTILS_H */
