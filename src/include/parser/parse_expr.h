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

#endif							/* PARSE_EXPR_H */
