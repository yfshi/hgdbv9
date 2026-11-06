/*-------------------------------------------------------------------------
 *
 * tecc_tabledef.h
 *    Declarations for tecc_get_tabledef function.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2024, HighGo Software
 *
 * IDENTIFICATION
 *    src/include/commands/tecc_tabledef.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_TABLEDEF_H
#define TECC_TABLEDEF_H

#include "fmgr.h"

extern Datum tecc_get_tabledef(PG_FUNCTION_ARGS);

#endif							/* TECC_TABLEDEF_H */
