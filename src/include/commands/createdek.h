/*-------------------------------------------------------------------------
 *
 * createdek.h
 *    Declarations for CREATE DEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/createdek.h
 *
 * ReqID:TECC4.0 - CREATE DEK command header
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef CREATEDEK_H
#define CREATEDEK_H

#include "nodes/parsenodes.h"

/* Begin - ReqID:TECC4.0 */

extern ObjectAddress CreateDek(CreateDekStmt *stmt);

/* End - ReqID:TECC4.0 */

#endif	/* CREATEDEK_H */