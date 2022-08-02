/*--------------------------------------------------------------------
 * 
 * File: ivorysql_ora.h
 *
 * Abstract: 
 * 		This file contains extern declarations for ivorysql_ora itself.
 *
 * Authored by zhenmingyang@highgo.com, 20220510.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/include/ivorysql_ora.h
 *
 *--------------------------------------------------------------------
 */
#ifndef IVORYSQL_ORA_H_
#define IVORYSQL_ORA_H_

#include "postgres.h"
#include "parser/parse_node.h"
#include "executor/nodeModifyTable.h"


/* Hooks */
extern bool pg_compatible_oracle_precedence(Oid arg1, Oid arg2, char *opname_p, Oid *result_arg1, Oid *result_arg2);

/* Begin - ReqID:SRS-SQL-INSERT */
extern int IvyOraGetAttrno(ParseState *pstate, ResTarget  *col);
/* End - ReqID:SRS-SQL-INSERT */

/* Begin - ReqID:SRS-SQL-MERGE */
extern bool IvyExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
									ItemPointer tupleid, bool canSetTag);
extern Query* IvytransformMergeStmt(ParseState *pstate, MergeStmt *stmt);
/* End - ReqID:SRS-SQL-MERGE */

#endif	/* IVORYSQL_ORA_H_ */
