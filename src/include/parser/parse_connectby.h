/*-------------------------------------------------------------------------
 *
 * File:parse_connectby.h
 *
 * Abstract:
 *		Handle connect by
 *
 *
 * ReqID:SRS-SQL-HierarchicalQueries
 *
 * Authored by dwdai@highgo.com,20241230.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/parser/parse_connectby.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PARSE_CONNECTBY_H
#define PARSE_CONNECTBY_H
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/primnodes.h"
#include "parser/parse_node.h"


extern SelectStmt *transformHierarQueryClause(ParseState *pstate,
						SelectStmt *stmt,
						IgnoreStr **ignoreStr,
						const char *source_sql);
extern bool checkLevelQueryName(IgnoreStr *name, char *label);
#endif /* PARSE_CONNECTBY_H */

