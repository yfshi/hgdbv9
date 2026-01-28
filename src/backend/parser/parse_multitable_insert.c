/*-------------------------------------------------------------------------
 *
 * File:parse_multitable_insert.c
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
 *	  src/backend/parser/parse_multitable_insert.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "parser/parse_node.h"
#include "parser/parse_cte.h"
#include "parser/parsetree.h"
#include "parser/parse_clause.h"
#include "parser/parse_multitable_insert.h"
#include "parser/parse_collate.h"
#include "parser/analyze.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "nodes/parsenodes.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "utils/relcache.h"
#include "utils/rel.h"
#include "utils/palloc.h"
#include "catalog/pg_class.h"
#include "access/sysattr.h"
#include "access/table.h"
#include "access/relation.h"
#include "optimizer/optimizer.h"
#include "commands/defrem.h"


static char *ChooseCteName(List *ctes, int check_len, int *pos);
static Node *makeAndExpr(Node *lexpr, Node *rexpr, int location);


static Node *
makeAndExpr(Node *lexpr, Node *rexpr, int location)
{
	/* Flatten "a AND b AND c ..." to a single BoolExpr on sight */
	if (IsA(lexpr, BoolExpr))
	{
		BoolExpr *blexpr = (BoolExpr *) lexpr;

		if (blexpr->boolop == AND_EXPR)
		{
			blexpr->args = lappend(blexpr->args, rexpr);
			return (Node *) blexpr;
		}
	}
	return (Node *) makeBoolExpr(AND_EXPR, list_make2(lexpr, rexpr), false, location);
}


/*
 * choose a ctname which is not colfict with ctes
 */
static char*
ChooseCteName(List *ctes, int check_len, int *pos)
{
	char	   *cteame = NULL;
	char		modlabel[NAMEDATALEN];
	ListCell	*lc;
	int			current_check;
	bool		name_ok = true;
	char		*label = "_cte";

	for (;;)
	{
		snprintf(modlabel, sizeof(modlabel), "%s%d", label, *pos);
		(*pos)++;

		cteame = makeObjectName("insert", NULL, modlabel);

		current_check = 0;
		foreach (lc, ctes)
		{
			CommonTableExpr *cte = (CommonTableExpr *) lfirst(lc);

			if (current_check == check_len)
				break;

			if (strcmp(cte->ctename, cteame) == 0)
			{
				name_ok = false;
				break;
			}
			current_check++;
		}

		if (name_ok)
			break;
		/* found a conflict, so try a new name component */
		pfree(cteame);
	}

	return cteame;
}


/*
 * transformMultiTableInsertStmt -
 *    transform an Insert all/first Statement
 */
Query *
transformMultiTableInsertStmt(ParseState *pstate,
		MultiTableInsertStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	SelectStmt *selectStmt = (SelectStmt *) stmt->subquery;
	List	   *exprList = NIL;
	bool		isGeneralSelect;
	List	   *sub_rtable;
	List	   *sub_namespace;
	ParseNamespaceItem *nsitem;
	ListCell   *lc;
	AclMode		targetPerms;
	ListCell	*iinserts;
	int targetindex;
	List *multiInsertList;
	List *targetnsitemList = NIL;
	ParseStmtKind save_kind = pstate->p_stmt_kind;
	AttrNumber	attr_num;
	List		*resnames = NIL;

	pstate->p_stmt_kind = PARSE_KIND_MULTITABLE_INSERT;

	/* There can't be any outer WITH to worry about */
	Assert(pstate->p_ctenamespace == NIL);

	qry->commandType = CMD_MULTI_INSERT;
	pstate->p_is_insert = true;
	qry->multiInsertFirst = stmt->is_first;

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	Assert(selectStmt != NULL);

	/*
	 * We have three cases to deal with: DEFAULT VALUES (selectStmt == NULL),
	 * VALUES list, or general SELECT input.  We special-case VALUES, both for
	 * efficiency and so we can handle DEFAULT specifications.
	 *
	 * The grammar allows attaching ORDER BY, LIMIT, FOR UPDATE, or WITH to a
	 * VALUES clause.  If we have any of those, treat it as a general SELECT;
	 * so it will work, but you can't use DEFAULT items together with those.
	 */
	isGeneralSelect = (selectStmt->valuesLists == NIL ||
						selectStmt->sortClause != NIL ||
						selectStmt->limitOffset != NULL ||
						selectStmt->limitCount != NULL ||
						selectStmt->lockingClause != NIL ||
						selectStmt->withClause != NULL);

	/*
	 * If a non-nil rangetable/namespace was passed in, and we are doing
	 * INSERT/SELECT, arrange to pass the rangetable/namespace down to the
	 * SELECT.  This can only happen if we are inside a CREATE RULE, and in
	 * that case we want the rule's OLD and NEW rtable entries to appear as
	 * part of the SELECT's rtable, not as outer references for it.  (Kluge!)
	 * The SELECT's joinlist is not affected however.  We must do this before
	 * adding the target table to the INSERT's rtable.
	 */
	if (isGeneralSelect)
	{
		sub_rtable = pstate->p_rtable;
		pstate->p_rtable = NIL;
		sub_namespace = pstate->p_namespace;
		pstate->p_namespace = NIL;
	}
	else
	{
		sub_rtable = NIL;		/* not used, but keep compiler quiet */
		sub_namespace = NIL;
	}

	/*
	 * Must get write lock on INSERT target table before scanning SELECT, else
	 * we will grab the wrong kind of initial lock if the target table is also
	 * mentioned in the SELECT part.  Note that the target table is not added
	 * to the joinlist or namespace.
	 */
	targetPerms = ACL_INSERT;
	foreach (iinserts, stmt->condition_list)
	{
		ConditionInsertClause *cinsert = (ConditionInsertClause *) lfirst(iinserts);
		ListCell *lc_into;

		foreach (lc_into, cinsert->intos)
		{
			InsertIntoClause *intoclause = (InsertIntoClause *) lfirst(lc_into);

			/*
			 * check dblink rel before setTargetTable for dblink rel will be
			 * transformed to local temp table
			 */
			if (intoclause->relation->srvname != NULL)
			{
				if (intoclause->relation->schemaname != NULL)
					ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						errmsg("cannot execute insert all/first on a remote "
						"database relation \"%s.""%s\"",
						intoclause->relation->schemaname,
						intoclause->relation->relname)));
				else
					ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						errmsg("cannot execute insert all/first on a remote "
						"database relation \"%s\"",
						intoclause->relation->relname)));
			}

			qry->resultRelation = setTargetTable(pstate, intoclause->relation,
											false, false, targetPerms);
			/* only support common relation */
			if (pstate->p_target_relation->rd_rel->relkind != RELKIND_RELATION &&
				pstate->p_target_relation->rd_rel->relkind != RELKIND_PARTITIONED_TABLE)
				ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot execute insert all/first on relation \"%s\"",
						RelationGetRelationName(pstate->p_target_relation)),
				 errdetail_relkind_not_supported(pstate->p_target_relation->rd_rel->relkind)));
			qry->resultRelations = lappend_int(qry->resultRelations,
								qry->resultRelation);
			targetnsitemList = lappend(targetnsitemList, pstate->p_target_nsitem);
		}
	}

	/*
	 * Determine which variant of INSERT we have.
	 */
	if (isGeneralSelect)
	{
		/*
		 * We make the sub-pstate a child of the outer pstate so that it can
		 * see any Param definitions supplied from above.  Since the outer
		 * pstate's rtable and namespace are presently empty, there are no
		 * side-effects of exposing names the sub-SELECT shouldn't be able to
		 * see.
		 */
		ParseState *sub_pstate = make_parsestate(pstate);
		Query	   *selectQuery;

		/*
		 * Process the source SELECT.
		 *
		 * It is important that this be handled just like a standalone SELECT;
		 * otherwise the behavior of SELECT within INSERT might be different
		 * from a stand-alone SELECT. (Indeed, Postgres up through 6.5 had
		 * bugs of just that nature...)
		 *
		 * The sole exception is that we prevent resolving unknown-type
		 * outputs as TEXT.  This does not change the semantics since if the
		 * column type matters semantically, it would have been resolved to
		 * something else anyway.  Doing this lets us resolve such outputs as
		 * the target column's type, which we handle below.
		 */
		sub_pstate->p_rtable = sub_rtable;
		sub_pstate->p_joinexprs = NIL;	/* sub_rtable has no joins */
		sub_pstate->p_namespace = sub_namespace;
		sub_pstate->p_resolve_unknowns = false;

		selectQuery = transformStmt(sub_pstate, stmt->subquery);

		free_parsestate(sub_pstate);

		/* The grammar should have produced a SELECT */
		if (!IsA(selectQuery, Query) ||
			selectQuery->commandType != CMD_SELECT)
			elog(ERROR, "unexpected non-SELECT command in INSERT ... SELECT");

		/*
		 * Make the source be a subquery in the INSERT's rangetable, and add
		 * it to the INSERT's joinlist (but not the namespace).
		 */
		nsitem = addRangeTableEntryForSubquery(pstate,
											   selectQuery,
											   makeAlias("*SELECT*", NIL),
											   false,
											   false);

		/*
		 * Transform the condition_list, which when expr and value_clause maybe references to
		 * the subquery side, so add that to the namespace.
		 */
		addNSItemToQuery(pstate, nsitem, true, true, true);

		/*----------
		 * Generate an expression list for the INSERT that selects all the
		 * non-resjunk columns from the subquery.  (INSERT's tlist must be
		 * separate from the subquery's tlist because we may add columns,
		 * insert datatype coercions, etc.)
		 *
		 * HACK: unknown-type constants and params in the SELECT's targetlist
		 * are copied up as-is rather than being referenced as subquery
		 * outputs.  This is to ensure that when we try to coerce them to
		 * the target column's datatype, the right things happen (see
		 * special cases in coerce_type).  Otherwise, this fails:
		 *		INSERT INTO foo SELECT 'bar', ... FROM baz
		 *----------
		 */
		exprList = NIL;
		foreach(lc, selectQuery->targetList)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(lc);
			Expr	   *expr;

			if (tle->resjunk)
				continue;
			if (tle->expr &&
				(IsA(tle->expr, Const) || IsA(tle->expr, Param)) &&
				exprType((Node *) tle->expr) == UNKNOWNOID)
				expr = tle->expr;
			else
			{
				Var		   *var = makeVarFromTargetEntry(nsitem->p_rtindex, tle);

				var->location = exprLocation((Node *) tle->expr);
				expr = (Expr *) var;
			}
			exprList = lappend(exprList, expr);
			resnames = lappend(resnames, tle->resname);
		}
	}
	else
	{
		/*
		 * Process INSERT ... VALUES with multiple VALUES sublists. We
		 * generate a VALUES RTE holding the transformed expression lists, and
		 * build up a targetlist containing Vars that reference the VALUES
		 * RTE.
		 */
		List	   *exprsLists = NIL;
		List	   *coltypes = NIL;
		List	   *coltypmods = NIL;
		List	   *colcollations = NIL;
		int			sublist_length = -1;
		bool		lateral = false;

		Assert(selectStmt->intoClause == NULL);
		Assert(selectStmt->valuesLists != NIL);

		foreach(lc, selectStmt->valuesLists)
		{
			List	   *sublist = (List *) lfirst(lc);

			/*
			 * Do basic expression transformation (same as a ROW() expr, not
			 * allow SetToDefault at top level)
			 */
			sublist = transformExpressionList(pstate, sublist,
											  EXPR_KIND_VALUES, false);

			/*
			 * All the sublists must be the same length, *after*
			 * transformation (which might expand '*' into multiple items).
			 * The VALUES RTE can't handle anything different.
			 */
			if (sublist_length < 0)
			{
				/* Remember post-transformation length of first sublist */
				sublist_length = list_length(sublist);
			}
			else if (sublist_length != list_length(sublist))
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("VALUES lists must all be the same length"),
						 parser_errposition(pstate,
											exprLocation((Node *) sublist))));
			}

			/*
			 * We must assign collations now because assign_query_collations
			 * doesn't process rangetable entries.  We just assign all the
			 * collations independently in each row, and don't worry about
			 * whether they are consistent vertically.  The outer INSERT query
			 * isn't going to care about the collations of the VALUES columns,
			 * so it's not worth the effort to identify a common collation for
			 * each one here.  (But note this does have one user-visible
			 * consequence: INSERT ... VALUES won't complain about conflicting
			 * explicit COLLATEs in a column, whereas the same VALUES
			 * construct in another context would complain.)
			 */
			assign_list_collations(pstate, sublist);

			exprsLists = lappend(exprsLists, sublist);
		}

		/*
		 * Construct column type/typmod/collation lists for the VALUES RTE.
		 * Every expression in each column has been coerced to the type/typmod
		 * of the corresponding target column or subfield, so it's sufficient
		 * to look at the exprType/exprTypmod of the first row.  We don't care
		 * about the collation labeling, so just fill in InvalidOid for that.
		 */
		foreach(lc, (List *) linitial(exprsLists))
		{
			Node	   *val = (Node *) lfirst(lc);

			coltypes = lappend_oid(coltypes, exprType(val));
			coltypmods = lappend_int(coltypmods, exprTypmod(val));
			colcollations = lappend_oid(colcollations, InvalidOid);
		}

		/*
		 * Ordinarily there can't be any current-level Vars in the expression
		 * lists, because the namespace was empty ... but if we're inside
		 * CREATE RULE, then NEW/OLD references might appear.  In that case we
		 * have to mark the VALUES RTE as LATERAL.
		 */
		if (list_length(pstate->p_rtable) != 1 &&
			contain_vars_of_level((Node *) exprsLists, 0))
			lateral = true;

		/*
		 * Generate the VALUES RTE
		 */
		nsitem = addRangeTableEntryForValues(pstate, exprsLists,
											 coltypes, coltypmods, colcollations,
											 NULL, lateral, true);
		/*
		 * Transform the condition_list, which when expr and value_clause maybe references to
		 * the subquery side, so add that to the namespace.
		 */
		addNSItemToQuery(pstate, nsitem, true, true, true);

		/*
		 * Generate list of Vars referencing the RTE
		 */
		exprList = expandNSItemVars(nsitem, 0, -1, NULL);

	}

	pstate->p_stmt_kind = save_kind;

	/*
	 * handle into clause like insert into
	 */
	targetindex = 0;
	multiInsertList = NIL;
	foreach (iinserts, stmt->condition_list)
	{
		ConditionInsertClause *cinsert = (ConditionInsertClause *) lfirst(iinserts);
		ListCell *lc_into;
		ConditionInsertAction *caction = makeNode(ConditionInsertAction);
		List *intos = NIL;

		save_kind = pstate->p_stmt_kind;
		/* where clause doesn't allow sequence */
		pstate->p_stmt_kind = PARSE_KIND_MULTITABLE_INSERT;
		caction->qual = transformWhereClause(pstate, cinsert->whenClause,
											EXPR_KIND_WHERE, "WHERE");
		caction->is_else = cinsert->is_else;
		pstate->p_stmt_kind = save_kind;

		foreach (lc_into, cinsert->intos)
		{
			InsertIntoClause *intoclause = (InsertIntoClause *) lfirst(lc_into);
			List	   *icolumns;
			List	   *attrnos;
			ListCell   *icols;
			ListCell   *attnos;
			RangeTblEntry *rte;
			int			resultRelation = list_nth_int(qry->resultRelations, targetindex);
			List		*value_exprlist;
			InsertIntoAction *intoaction = (InsertIntoAction *) makeNode(InsertIntoAction);

			rte = rt_fetch(resultRelation, pstate->p_rtable);
			Assert(rte->rtekind == RTE_RELATION);
			intoaction->target_rte = resultRelation;

			if (pstate->p_target_relation != NULL)
				table_close(pstate->p_target_relation, NoLock);
			pstate->p_target_relation = relation_open(rte->relid,
											RowExclusiveLock);
			pstate->p_target_nsitem = list_nth(targetnsitemList, targetindex++);

			/* Validate stmt->cols list, or build default list if no list given */
			icolumns = checkInsertTargets(pstate, intoclause->cols, &attrnos);
			Assert(list_length(icolumns) == list_length(attrnos));

			/*
			 * like Handle transformMergeIntoStmt, value_exprlsit is null
			 * we use exprList
			 */
			if (intoclause->values  == NIL)
				value_exprlist =  (List *) copyObject(exprList);
			else
				value_exprlist = transformExpressionList(pstate, intoclause->values,
												EXPR_KIND_VALUES_SINGLE,
												true);
			value_exprlist = transformInsertRow(pstate, value_exprlist,
													intoclause->cols, icolumns,
													attrnos, false);

			/*
			 * Generate query's target list using the computed list of expressions.
			 * Also, mark all the target columns as needing insert permissions.
			 */
			Assert(list_length(value_exprlist) <= list_length(icolumns));
			forthree(lc, value_exprlist, icols, icolumns, attnos, attrnos)
			{
				Expr	   *expr = (Expr *) lfirst(lc);
				ResTarget  *col = lfirst_node(ResTarget, icols);
				AttrNumber	attr_num = (AttrNumber) lfirst_int(attnos);
				TargetEntry *tle;

				tle = makeTargetEntry(expr,
									  attr_num,
									  col->name,
									  false);
				intoaction->targetList = lappend(intoaction->targetList, tle);

				rte->insertedCols = bms_add_member(rte->insertedCols,
												   attr_num - FirstLowInvalidHeapAttributeNumber);
			}
			intos = lappend(intos, intoaction);
			relation_close(pstate->p_target_relation, NoLock);
			pstate->p_target_relation = NULL;
		}
		caction->intos = intos;
		multiInsertList = lappend(multiInsertList, caction);
	}

	pstate->p_stmt_kind = save_kind;

	qry->targetList = NIL;
	attr_num = 1;
	foreach (lc, exprList)
	{
		Expr	   *expr = (Expr *) lfirst(lc);
		TargetEntry *tle;
		char *resname;

		if (resnames != NIL)
			resname = pstrdup((char *) lfirst(list_nth_cell(resnames, attr_num - 1)));
		else
			resname = psprintf("column%d", attr_num);

		tle = makeTargetEntry(expr,
							  attr_num++,
							  resname,
							  false);
		qry->targetList = lappend(qry->targetList, tle);
	}
	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);
	qry->multiInsertActionList = multiInsertList;

	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->returningList = NULL;

	list_free(resnames);

	/* Begin - ReqID:SRS-SQL-ROWNUM */
	if (pstate->p_hasRownum)
		elog(ERROR, "not expected ROWNUM in INSERT statement");
	/* End - ReqID:SRS-SQL-ROWNUM */

	assign_query_collations(pstate, qry);

	return qry;
}



/*
 * change insert all/first to
 * insert add with clause
 */
Query*
transformMultiTableInsertWithCte(ParseState *pstate,
					MultiTableInsertStmt *multi)
{
	WithClause *withclause = multi->withClause;
	char		*subquery_ctname;
	int			origin_cte_len = 0;
	ListCell *iinserts;
	Node		*else_qual = NULL;
	int			pos_pass = 1;
	CommonTableExpr *ctexpr;
	InsertStmt	*maininsertstmt = NULL;
	ParseStmtKind save_kind = pstate->p_stmt_kind;
	Query			*query;
	bool	first_into = true;

	pstate->p_stmt_kind = PARSE_KIND_MULTITABLE_INSERT;

	/* construct a new with_clause */
	if (withclause == NULL)
	{
		withclause = makeNode(WithClause);
		withclause->location = -1;
		withclause->recursive = false;
	}
	else
		origin_cte_len = list_length(withclause->ctes);

	/* append subquery to ctes */
	subquery_ctname = ChooseCteName(withclause->ctes, origin_cte_len, &pos_pass);
	ctexpr = makeNode(CommonTableExpr);
	ctexpr->location = -1;
	ctexpr->ctename = subquery_ctname;
	ctexpr->ctequery = multi->subquery;
	withclause->ctes = lappend(withclause->ctes, ctexpr);

	foreach (iinserts, multi->condition_list)
	{
		ConditionInsertClause *cinsert = (ConditionInsertClause *) lfirst(iinserts);
		ListCell *lc_into;

		foreach (lc_into, cinsert->intos)
		{
			InsertIntoClause *intoclause = (InsertIntoClause *) lfirst(lc_into);
			Relation rel;
			InsertStmt *new_insert;
			SelectStmt *new_select;

			/*
			 * check dblink rel before setTargetTable for dblink rel will be
			 * transformed to local temp table
			 */
			if (intoclause->relation->srvname != NULL)
			{
				if (intoclause->relation->schemaname != NULL)
					ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("cannot execute insert all/first on a remote "
					"database relation \"%s.""%s\"",
					intoclause->relation->schemaname,
					intoclause->relation->relname)));
				else
					ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("cannot execute insert all/first on a remote "
					"database relation \"%s\"",
					intoclause->relation->relname)));
			}

			rel = relation_openrv_extended(intoclause->relation,
											RowExclusiveLock, false);
			/* only support common relation */
			if (rel->rd_rel->relkind != RELKIND_RELATION &&
				rel->rd_rel->relkind != RELKIND_PARTITIONED_TABLE)
				ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot execute insert all/first on relation \"%s\"",
						RelationGetRelationName(rel)),
				 errdetail_relkind_not_supported(rel->rd_rel->relkind)));
			relation_close(rel, NoLock);

			/* construct insert as select xxx from  subquery_ctname */
			new_insert = makeNode(InsertStmt);
			new_insert->relation = intoclause->relation;
			new_insert->cols = intoclause->cols;

			new_select = makeNode(SelectStmt);
			new_select->multiInsert = true;
			if (intoclause->values != NIL)
			{
				ListCell *vcell;

				foreach(vcell, intoclause->values)
				{
					Node *vnode = (Node *) lfirst(vcell);
					ResTarget *res;

					if(nodeTag(vnode) == T_ResTarget)
						res = (ResTarget *) vnode;
					else
					{
						res = makeNode(ResTarget);
						res->name = NULL;
						res->indirection = NIL;
						res->val = (Node *) vnode;
						res->location = -1;
					}
					new_select->targetList = lappend(new_select->targetList, res);
				}
			}
			else
			{
				/* append a_star */
				ResTarget *res;
				ColumnRef *n = makeNode(ColumnRef);

				n->fields = list_make1(makeNode(A_Star));
				n->location = -1;

				res = makeNode(ResTarget);
				res->name = NULL;
				res->indirection = NIL;
				res->val = (Node *) n;
				res->location = -1;
				new_select->targetList = list_make1(res);
			}
			new_select->fromClause = list_make1(makeRangeVar(NULL,
												subquery_ctname,
												-1));
			/* append where clause */
			if (else_qual != NULL)
			{
				/*
				 * is first or else clause
				 * we should and else_qual
				 */
				if (multi->is_first || cinsert->is_else)
				{
					if (cinsert->whenClause != NULL)
					{
						new_select->whereClause = (Node *) makeAndExpr((Node *) copyObject(cinsert->whenClause),
													(Node *) copyObject(else_qual), -1);
					}
					else
						new_select->whereClause = (Node *) copyObject(else_qual);
				}
				else
					new_select->whereClause = copyObject(cinsert->whenClause);
			}
			else
				new_select->whereClause = copyObject(cinsert->whenClause);

			new_insert->selectStmt = (Node *) new_select;

			/* first into table as maininsert */
			if (first_into)
			{
				maininsertstmt = new_insert;
			}
			else
			{
				/* construct cte */
				char *new_cte_name;

				new_cte_name = ChooseCteName(withclause->ctes, origin_cte_len, &pos_pass);
				ctexpr = makeNode(CommonTableExpr);
				ctexpr->location = -1;
				ctexpr->ctename = new_cte_name;
				ctexpr->ctequery = (Node *) new_insert;
				withclause->ctes = lappend(withclause->ctes, ctexpr);
			}
			first_into = false;
		}

		/* handle else_qual */
		if (cinsert->whenClause != NULL)
		{
			Node *notnode = (Node *) makeBoolExpr(NOT_EXPR,
										list_make1(cinsert->whenClause), false, -1);
			if (else_qual == NULL)
				else_qual = notnode;
			else
				else_qual = makeAndExpr(copyObject(else_qual), notnode, -1);
		}
	}

	/* consturct main query */
	Assert(maininsertstmt != NULL);

	maininsertstmt->withClause = withclause;
	query = transformStmt(pstate, (Node *) maininsertstmt);
	query->multiInsertUseCte = true;

	pstate->p_stmt_kind = save_kind;

	return query;
}

/*
 * check rte_index is result relation
 */
bool
is_result_relation(List *results, int rte_index, int dest_relation)
{
	if (rte_index == dest_relation)
		return true;
	else if (list_member_int(results, rte_index))
		return true;
	else
		return false;
}


