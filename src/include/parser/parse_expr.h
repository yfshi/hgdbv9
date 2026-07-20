/*-------------------------------------------------------------------------
 *
 * parse_expr.h
 *	  handle expressions in parser
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_expr.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_EXPR_H
#define PARSE_EXPR_H

#include "parser/parse_node.h"

/* Begin BUG#5048 5070 */
#define exprIsHexConstant(n) \
		  ((n) && IsA((n), A_Const) && \
		  (((A_Const *) (n))->val.type == T_HexString || \
		   ((A_Const *) (n))->val.type == T_MysBitString))
/* End BUG#5048 5070 */

/* GUC parameters */
extern bool Transform_null_equals;

extern Node *transformExpr(ParseState *pstate, Node *expr, ParseExprKind exprKind);

/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
extern Node *IvyTransformColumnRef(ParseState *pstate,
											ColumnRef *cref,
											bool missing_ok);
/* End - ReqID:SRS-SQL-HierarchicalQueries */


/* Begin - BUG#M0000376 */
extern void transformFuncArg(ParseState *pstate, Node *func_arg);
/* End - BUG#M0000376 */

extern const char *ParseExprKindName(ParseExprKind exprKind);

extern Node *make_bigint_const_from_hex(Node *n); /* BUG#5048 5070 */

/* Begin - BUG#7845 */
extern void get_tablename_colname_from_var(ParseState *pstate, Var *var, char **colName, char **tabName);
/* End - BUG#7845 */
#endif							/* PARSE_EXPR_H */
