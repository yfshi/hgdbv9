/*-------------------------------------------------------------------------
 *
 * dropkek.h
 *    Declarations for DROP KEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/dropkek.h
 *
 * ReqID:TECC4.0 - DROP KEK command header
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef DROPKEK_H
#define DROPKEK_H

#include "nodes/parsenodes.h"

/* Begin - ReqID:TECC4.0 */

extern void DropKek(DropStmt *stmt);

/* End - ReqID:TECC4.0 */

#endif	/* DROPKEK_H */