/*-------------------------------------------------------------------------
 *
 * clauses.h
 *	  prototypes for clauses.c.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/optimizer/clauses.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CLAUSES_H
#define CLAUSES_H

#include "nodes/pathnodes.h"

/* Begin - ReqID:SRS-SQL-ROWNUM */
/* Bits defined in OraCompatExprKind */
#define OCE_ROWNUM_BIT		(1 << 0)

/*
 * Oracle Compatible Expression.
 */
typedef enum OraExprKind
{
	OCE_INVALID = 0,
	/* single class */
	OCE_ROWNUM = OCE_ROWNUM_BIT,
	OCE_COUNT = OCE_ROWNUM_BIT
} OraExprKind;
/* End - ReqID:SRS-SQL-ROWNUM */

typedef struct
{
	int			numWindowFuncs; /* total number of WindowFuncs found */
	Index		maxWinRef;		/* windowFuncs[] is indexed 0 .. maxWinRef */
	List	  **windowFuncs;	/* lists of WindowFuncs for each winref */
} WindowFuncLists;

extern bool contain_agg_clause(Node *clause);

extern bool contain_window_function(Node *clause);
extern WindowFuncLists *find_window_functions(Node *clause, Index maxWinRef);

extern double expression_returns_set_rows(PlannerInfo *root, Node *clause);

extern bool contain_subplans(Node *clause);

extern char max_parallel_hazard(Query *parse);
extern bool is_parallel_safe(PlannerInfo *root, Node *node);
extern bool contain_nonstrict_functions(Node *clause);
extern bool contain_exec_param(Node *clause, List *param_ids);
extern bool contain_leaked_vars(Node *clause);

extern Relids find_nonnullable_rels(Node *clause);
extern List *find_nonnullable_vars(Node *clause);
extern List *find_forced_null_vars(Node *clause);
extern Var *find_forced_null_var(Node *clause);

extern bool is_pseudo_constant_clause(Node *clause);
extern bool is_pseudo_constant_clause_relids(Node *clause, Relids relids);

extern int	NumRelids(PlannerInfo *root, Node *clause);

extern void CommuteOpExpr(OpExpr *clause);

extern Query *inline_set_returning_function(PlannerInfo *root,
											RangeTblEntry *rte);

extern Bitmapset *pull_paramids(Expr *expr);

/* Begin - ReqID:SRS-SQL-ROWNUM */
extern bool contain_oracompat_expr(Node *clause, OraExprKind oraExprkind);
/* End - ReqID:SRS-SQL-ROWNUM */

/* Begin - ReqID:SRS-MISC-PERFORMANCE-INSERTPARALLEL */
extern bool is_parallel_allowed_for_modify(Query *parse);
/* End - ReqID:SRS-MISC-PERFORMANCE-INSERTPARALLEL */

#endif							/* CLAUSES_H */
