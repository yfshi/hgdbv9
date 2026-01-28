/*-------------------------------------------------------------------------
 *
 * File:parse_multitable_insert.h
 *
 * Abstract:
 *		Definitions for the parse insert all/first stmt
 *
 *
 * ReqID:SRS-SQL-MULTITABLEINSERT
 *
 * Authored by dwdai@highgo.com,20250902.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/parser/parse_multitable_insert.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_MULTITABLE_INSERT_H
#define PARSE_MULTITABLE_INSERT_H

extern Query* transformMultiTableInsertStmt(ParseState *pstate,
					MultiTableInsertStmt *multi);
extern Query* transformMultiTableInsertWithCte(ParseState *pstate,
					MultiTableInsertStmt *multi);
extern bool is_result_relation(List *results, int rte_index, int dest_relation);
#endif /* define PARSE_MULTITABLE_INSERT_H */