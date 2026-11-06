/*-------------------------------------------------------------------------
 *
 * createkek.h
 *    Declarations for CREATE KEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/createkek.h
 *
 * ReqID:TECC4.0 - CREATE KEK command header
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef CREATEKEK_H
#define CREATEKEK_H

#include "nodes/parsenodes.h"

/* Begin - ReqID:TECC4.0 */

extern ObjectAddress CreateKek(CreateKekStmt *stmt);

/* End - ReqID:TECC4.0 */

#endif	/* CREATEKEK_H */