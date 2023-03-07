/*-------------------------------------------------------------------------
 *
 * File:orarefcursor.h
 *
 * Abstract:
 *		Definitions for the PLISQL cursor
 *
 * ReqID:SRS-PLSQL-CURSOR
 *
 * Authored by dwdai@highgo.com,20230315.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/include/orarefcursor.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef __UDT_ORAREFCURSOR_H__
#define __UDT_ORAREFCURSOR_H__
#include "fmgr.h"

/* io function */
extern Datum orarefcursorin(PG_FUNCTION_ARGS);
extern Datum orarefcursorout(PG_FUNCTION_ARGS);
extern Datum orarefcursorrecv(PG_FUNCTION_ARGS);
extern Datum orarefcursorsend(PG_FUNCTION_ARGS);
extern Datum sysrefcursorin(PG_FUNCTION_ARGS);
extern Datum sysrefcursorout(PG_FUNCTION_ARGS);
extern Datum sysrefcursorrecv(PG_FUNCTION_ARGS);
extern Datum sysrefcursorsend(PG_FUNCTION_ARGS);

extern Datum orarefcursor_init(PG_FUNCTION_ARGS);
extern Datum oracursor_syscursor(PG_FUNCTION_ARGS);
extern Datum syscursor_oracursor(PG_FUNCTION_ARGS);

extern char* DatumGetOraRefCursorPortal(Datum datum);
extern bool OraRefcursorIsnull(Datum datum);
extern void *DatumGetOraRefCursorType(Datum datum);
extern Datum init_orarefcursor_value(void *type, const char *name);
extern void OraRefcursorSetType(void *type, Datum datum);
extern void orarefcursor_init_internal(void);

#endif

