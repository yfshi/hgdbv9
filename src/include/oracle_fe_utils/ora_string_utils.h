/*-------------------------------------------------------------------------
 *
 * String-processing utility routines for frontend code (Oracle compatibility)
 *
 * Utility functions that interpret backend output or quote strings for
 * assorted contexts.
 *
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/oracle_fe_utils/ora_string_utils.h
 *
 * add the file for requirement "SQL PARSER"
 *
 *-------------------------------------------------------------------------
 */
#ifndef ORA_STRING_UTILS_H
#define ORA_STRING_UTILS_H

#include "libpq-fe.h"
#include "pqexpbuffer.h"
#include "oracle_parser/ora_keywords.h"
#include "utils/ora_compatible.h"

/* Global variables controlling behavior of ora_fmtId() */
extern DBMode db_mode;
extern bool native_PG;      /*  BUG: M0000332 */
extern void getDbCompatibleMode(PGconn *conn);

/* Functions */
extern const char *ora_fmtId(const char *identifier, int encoding);
extern const char *ora_dblink_fmtId(const char *identifier, bool quote_dblink); /* BUG#Z1317 */

#endif							/* ORA_STRING_UTILS_H */
