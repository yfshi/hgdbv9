/*-------------------------------------------------------------------------
 *
 * File:parse_pivot.h
 *
 * Abstract:
 *      transform Oracle-compatible PIVOT and UNPIVOT clauses
 *
 *
 * ReqID:SRS-SQL-PIVOT
 *
 * Authored by zhaoshengbin@highgo.com,20260714.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *    src/include/parser/parse_pivot.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PARSE_PIVOT_H
#define PARSE_PIVOT_H

#include "nodes/parsenodes.h"
#include "parser/parse_node.h"

extern Node *transformPivotFromClauseItem(ParseState *pstate, PivotCall *pcall,
										  ParseNamespaceItem **top_nsitem,
										  List **namespace);

#endif							/* PARSE_PIVOT_H */
