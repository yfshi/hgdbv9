/*-------------------------------------------------------------------------
 *
 * dropdek.h
 *    Declarations for DROP DEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/dropdek.h
 *
 * ReqID:TECC4.0 - DROP DEK command header
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef DROPDEK_H
#define DROPDEK_H

#include "nodes/parsenodes.h"

/* Begin - ReqID:TECC4.0 */

extern void DropDek(DropStmt *stmt);

/* End - ReqID:TECC4.0 */

#endif	/* DROPDEK_H */