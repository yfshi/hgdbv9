/*-------------------------------------------------------------------------
 *
 * parse_clause.h
 *	  handle clauses in parser
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_clause.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_CLAUSE_H
#define PARSE_CLAUSE_H

#include "parser/parse_node.h"

extern void transformFromClause(ParseState *pstate, List *frmList);
extern int	setTargetTable(ParseState *pstate, RangeVar *relation,
						   bool inh, bool alsoSource, AclMode requiredPerms);

extern Node *transformWhereClause(ParseState *pstate, Node *clause,
								  ParseExprKind exprKind, const char *constructName);
extern Node *transformLimitClause(ParseState *pstate, Node *clause,
								  ParseExprKind exprKind, const char *constructName,
								  LimitOption limitOption);
extern List *transformGroupClause(ParseState *pstate, List *grouplist,
								  List **groupingSets,
								  List **targetlist, List *sortClause,
								  ParseExprKind exprKind, bool useSQL99);
extern List *transformSortClause(ParseState *pstate, List *orderlist,
								 List **targetlist, ParseExprKind exprKind,
								 bool useSQL99);

extern List *transformWindowDefinitions(ParseState *pstate,
										List *windowdefs,
										List **targetlist);

extern List *transformDistinctClause(ParseState *pstate,
									 List **targetlist, List *sortClause, bool is_agg);
extern List *transformDistinctOnClause(ParseState *pstate, List *distinctlist,
									   List **targetlist, List *sortClause);
extern void transformOnConflictArbiter(ParseState *pstate,
									   OnConflictClause *onConflictClause,
									   List **arbiterExpr, Node **arbiterWhere,
									   Oid *constraint);

extern List *addTargetToSortList(ParseState *pstate, TargetEntry *tle,
								 List *sortlist, List *targetlist, SortBy *sortby);
extern Index assignSortGroupRef(TargetEntry *tle, List *tlist);
extern bool targetIsInSortList(TargetEntry *tle, Oid sortop, List *sortList);

/* Begin - ReqID:SRS-SQL-ROWID */
extern bool	interpretRowidOption(List *defList, bool allowRowid);
/* End - ReqID:SRS-SQL-ROWID */
extern bool	interpretAntiTamperOption(List *defList, bool *enableatamper); /* ReqID: SRS-SECURITY-ANTITAMPERING */
extern void checkTDEOption(List *defList);    /* BUG#z8555 */

/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
extern int	ora_extractRemainingColumns(ParseNamespaceColumn *src_nscolumns,
									List *src_colnames,
									List **src_colnos,
									List **res_colnames, List **res_colvars,
									ParseNamespaceColumn *res_nscolumns,
									int res_colindex);

extern List *check_Outerjoin_Opexpr(Node *expr);

extern void AddJoinOnToRtable(ParseState *pstate, JoinExpr *joinexpr);

extern JoinExpr *make_inner_join(ParseState *pstate, List *tables);
/* End - ReqID:SRS-SQL-OUTERJOIN(+) */
/* Begin - ReqID:SRS-MYSQL-UPDATE */
extern int setMultiTargetTable(ParseState* pstate, RangeVar* relRv, bool inh,
									bool alsoSource, AclMode requiredPerms,
									bool multiModify);
extern List *transfromRelationposition(ParseState *pstate, List *frmList);
/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - ReqID:SRS-SQL-NLSSORT */
extern void pseudo_collation_replace(ParseState *pstate, Node *node, Node *base, Oid targetcoll);
/* End - ReqID:SRS-SQL-NLSSORT */

/* Begin - ReqID:SRS-SQL-PIVOT */
extern Node *transformFromClauseItemForPivot(ParseState *pstate, Node *n,
											 ParseNamespaceItem **top_nsitem,
											 List **namespace);
/* End - ReqID:SRS-SQL-PIVOT */

#endif							/* PARSE_CLAUSE_H */
