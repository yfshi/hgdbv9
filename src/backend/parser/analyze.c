/*-------------------------------------------------------------------------
 *
 * analyze.c
 *	  transform the raw parse tree into a query tree
 *
 * For optimizable statements, we are careful to obtain a suitable lock on
 * each referenced table, and other modules of the backend preserve or
 * re-obtain these locks before depending on the results.  It is therefore
 * okay to do significant semantic analysis of these statements.  For
 * utility commands, no locks are obtained here (and if they were, we could
 * not be sure we'd still have them at execution).  Hence the general rule
 * for utility commands is to just dump them into a Query node untransformed.
 * DECLARE CURSOR, EXPLAIN, and CREATE TABLE AS are exceptions because they
 * contain optimizable statements, which we should transform.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * Authored by wanghaoyan@highgo.com, 20240327
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved
 *
 *
 *	src/backend/parser/analyze.c
 *
 *-------------------------------------------------------------------------
 */

#include <assert.h>

#include "postgres.h"

#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_enum.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "commands/sequence.h"
#include "commands/typecmds.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "parser/analyze.h"
#include "parser/parse_agg.h"
#include "parser/parse_clause.h"
#include "parser/parse_coerce.h"
#include "parser/parse_collate.h"
#include "parser/parse_cte.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_merge.h"
#include "parser/parse_oper.h"
#include "parser/parse_param.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "parser/parse_type.h"
#include "parser/parse_utilcmd.h" /* BUG#Z7282 */
#include "parser/parsetree.h"
#include "parser/pg_object_hook.h"
#include "rewrite/rewriteManip.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/guc.h"
#include "utils/queryjumble.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/ora_compatible.h"
#include "funcapi.h" /* SRS-PLSQL-SUBPROC */
/* Begin - BUG#Z196 */
#include "commands/proclang.h"
/* End - BUG#Z196 */
#include "utils/lsyscache.h"	/* ReqID:SRS-MYSQL-UPDATE */
/* Begin - BUG#Z1435 */
#include "rewrite/rewriteHandler.h"
#include "catalog/pg_constraint.h"
/* End - BUG#Z1435 */
#include "catalog/partition.h" /* ReqID:SRS-MYSQL-SQL-REPLACE */
/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
#include "parser/parse_connectby.h"
/* End - ReqID:SRS-SQL-HierarchicalQueries */
/* Begin - ReqID:SRS-SECURITY-AUDIT */
#include "pg_contrib_audit.h"
/* End - ReqID:SRS-SECURITY-AUDIT */
/* Begin - ReqID:SRS-MYSQL-SET */
#include "mysql_parser/mys_set_hook.h"
#include "utils/varlena.h"
#include "parser/parser.h"
/* End - ReqID:SRS-MYSQL-SET */
/* Begin - SRS-SQL-MULTITABLEINSERT */
#include "parser/parse_multitable_insert.h"
/* End - SRS-SQL-MULTITABLEINSERT */


/* Hook for plugins to get control at end of parse analysis */
post_parse_analyze_hook_type post_parse_analyze_hook = NULL;
redact_parse_analyze_hook_type redact_parse_analyze_hook = NULL;	/* ReqID:SRS-SECURITY-DATADESENSITIZATION */

static Query *transformOptionalSelectInto(ParseState *pstate, Node *parseTree);
static Query *transformDeleteStmt(ParseState *pstate, DeleteStmt *stmt);
static Query *transformInsertStmt(ParseState *pstate, InsertStmt *stmt);
static OnConflictExpr *transformOnConflictClause(ParseState *pstate,
												 OnConflictClause *onConflictClause);
static int	count_rowexpr_columns(ParseState *pstate, Node *expr);
static Query *transformSelectStmt(ParseState *pstate, SelectStmt *stmt);
static Query *transformValuesClause(ParseState *pstate, SelectStmt *stmt);
static Query *transformSetOperationStmt(ParseState *pstate, SelectStmt *stmt);
static Node *transformSetOperationTree(ParseState *pstate, SelectStmt *stmt,
									   bool isTopLevel, List **targetlist);
static void determineRecursiveColTypes(ParseState *pstate,
									   Node *larg, List *nrtargetlist);
static Query *transformReturnStmt(ParseState *pstate, ReturnStmt *stmt);
static Query *transformUpdateStmt(ParseState *pstate, UpdateStmt *stmt);
/* Begin - ReqID:SRS-MYSQL-UPDATE */
static char *checkUpdateResTargetName(Relation rd, RangeVar* rel, ResTarget* res, bool* matchRelname);
/* End - ReqID:SRS-MYSQL-UPDATE */
static List *transformReturningList(ParseState *pstate, List *returningList);
static Query *transformPLAssignStmt(ParseState *pstate,
									PLAssignStmt *stmt);
static Query *transformDeclareCursorStmt(ParseState *pstate,
										 DeclareCursorStmt *stmt);
static Query *transformExplainStmt(ParseState *pstate,
								   ExplainStmt *stmt);
static Query *transformCreateTableAsStmt(ParseState *pstate,
										 CreateTableAsStmt *stmt);
static Query *transformCallStmt(ParseState *pstate,
								CallStmt *stmt);
static void transformLockingClause(ParseState *pstate, Query *qry,
								   LockingClause *lc, bool pushedDown);
#ifdef RAW_EXPRESSION_COVERAGE_TEST
static bool test_raw_expression_coverage(Node *node, void *context);
#endif

/* Begin - ReqID:SRS-PLSQL-TYPE */
static List *transformUpdateRowTargetList(ParseState *pstate, List *origTlist);
static List *transformIvyUpdateTargetList(ParseState *pstate, List *origTlist);
/* End - ReqID:SRS-PLSQL-TYPE */

/* Begin - ReqID:SRS-MYSQL-UPDATE */
static void transformLimitSortClause(ParseState *pstate, void *stmt, Query *qry, bool isdel); /* ReqID:SRS-MYSQL-SQL-DELETE */
static List* transformUpdateSortClause(ParseState *pstate, UpdateStmt *stmt, Query *qry);
static void CheckUpdateRelations(ParseState *pstate, UpdateStmt *stmt);
static void transformMergeTargetList(RangeVar *rel,List **targetList, bool ismulti);
static void transformMultiMergeTargetList(List *rellist, List **targetList);

/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
static UpsertExpr *transformUpsertClause(ParseState *pstate, UpsertClause *upsertClause, List *resultRelations);
static void transformSchemaInTargetlist(List **targetlist);
static void transformTableAndSchemaInCols(ParseState *pstate, List **cols);
static Node *make_null_const(Oid typeid);
/* End - ReqID:SRS-MYSQL-SQL-INSERT */

/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
static void transformRangeSubselectInListClause(DeleteStmt *stmt);
static void transformCollistInFromcClause(List *rangevarlist, List *collist);
/* End - ReqID:SRS-MYSQL-SQL-DELETE */

/* Begin - BUG#Z1324 */
static void transformAliasInTargetlist(RangeVar *rel, List *targetList);
static void transformRemoveschemaInTargetlist(RangeVar *rel, List **targetList);
/* End - BUG#Z1324 */
/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
static void checkForUpdateInSubqury(ParseState *pstate, LockingClause *lc);
static void applyLockingTable(Query *qry, Oid relid,
				LockingClause *lockclause,
				bool pushedDown);
/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
static void transformGetpartionRelationOid(Query            *qry, RangeVar  *relation);
static void transformGetpartionRelationOids(Query *qry, List *relations);
/* End - ReqID:SRS-MYSQL-SQL-REPLACE */

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
static Expr *set_auto_increment_func_node(ParseState *pstate, Expr* expr, int attno);
static Expr *update_set_auto_increment_func_node(ParseState *pstate, Expr* expr, int attno);
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

/* Begin - ReqID:SRS-MYSQL-SET */
static Query *transformMysDoStmt(ParseState *pstate, MysDoStmt *stmt);
static Query *transformSelectIntoUservarStmt(ParseState *pstate, SelectIntoUserVarStmt *stmt);
/* End - ReqID:SRS-MYSQL-SET */
static List *transformSorbyfromGroupclause(List **grouplist);		/* ReqID:SRS-MYSQL-SELECT */
static Node *makeAndExpr(Node *lexpr, Node *rexpr, int location);	/* ReqID:SRS-MYSQL-SELECT */

/* Begin - OA-REQ-2252 */
/*
 * Keep these values aligned with parse_func.c.
 * They mark folded inner aggregates that originally appeared as
 * outer_aggregate(inner_aggregate) in Oracle compatibility mode.
 */
#define ORA_NESTED_AGG_MARK	(-810001)

typedef struct OraNestedAggExprCtx
{
	ParseState *pstate;
	Var		   *input_var;
	bool		saw_agg;
} OraNestedAggExprCtx;

static bool ora_has_nested_agg_marker(Node *node, void *context);
static bool ora_clear_nested_agg_marker(Node *node, void *context);
static bool ora_find_marked_agg(Node *node, void *context);
static Node *ora_build_nested_agg_outer_expr(Node *node, void *context);
static Query *ora_rewrite_nested_agg_two_stage(ParseState *pstate, Query *inner_qry);
/* End - OA-REQ-2252 */

/*
 * parse_analyze
 *		Analyze a raw parse tree and transform it to Query form.
 *
 * Optionally, information about $n parameter types can be supplied.
 * References to $n indexes not defined by paramTypes[] are disallowed.
 *
 * The result is a Query node.  Optimizable statements require considerable
 * transformation, while utility-type statements are simply hung off
 * a dummy CMD_UTILITY Query node.
 */
Query *
parse_analyze(RawStmt *parseTree, const char *sourceText,
			  Oid *paramTypes, int numParams,
			  QueryEnvironment *queryEnv)
{
	ParseState *pstate = make_parsestate(NULL);
	Query	   *query;
	JumbleState *jstate = NULL;

	Assert(sourceText != NULL); /* required as of 8.4 */

	pstate->p_sourcetext = sourceText;

	if (numParams > 0)
		parse_fixed_parameters(pstate, paramTypes, numParams);

	pstate->p_queryEnv = queryEnv;

	query = transformTopLevelStmt(pstate, parseTree);

	/* Begin - ReqID:SRS-MYSQL-SET */
	query->hasuservar = pstate->p_hasuservar;
	/* End - ReqID:SRS-MYSQL-SET */

	if (IsQueryIdEnabled())
		jstate = JumbleQuery(query, sourceText);

	if (post_parse_analyze_hook)
		(*post_parse_analyze_hook) (pstate, query, jstate);

	free_parsestate(pstate);

	pgstat_report_query_id(query->queryId, false);

	/* SRS-MISC-HSH */
	pgstat_report_query_type(query->commandType, false);


	return query;
}

/*
 * parse_analyze_varparams
 *
 * This variant is used when it's okay to deduce information about $n
 * symbol datatypes from context.  The passed-in paramTypes[] array can
 * be modified or enlarged (via repalloc).
 */
Query *
parse_analyze_varparams(RawStmt *parseTree, const char *sourceText,
						Oid **paramTypes, int *numParams,
						/* Begin - ReqID:SRS-SQL-FUNCOUT */
						char **paramModes)
						/* End - ReqID:SRS-SQL-FUNCOUT */
{
	ParseState *pstate = make_parsestate(NULL);
	Query	   *query;
	JumbleState *jstate = NULL;

	Assert(sourceText != NULL); /* required as of 8.4 */

	pstate->p_sourcetext = sourceText;

	parse_variable_parameters(pstate, paramTypes, numParams,
						/* Begin - ReqID:SRS-SQL-FUNCOUT */
						paramModes);
						/* End - ReqID:SRS-SQL-FUNCOUT */

	query = transformTopLevelStmt(pstate, parseTree);

	/* make sure all is well with parameter types */
	check_variable_parameters(pstate, query);

	if (IsQueryIdEnabled())
		jstate = JumbleQuery(query, sourceText);

	if (post_parse_analyze_hook)
		(*post_parse_analyze_hook) (pstate, query, jstate);

	free_parsestate(pstate);

	pgstat_report_query_id(query->queryId, false);

	/* SRS-MISC-HSH */
        pgstat_report_query_type(query->commandType, false);

	return query;
}

/*
 * parse_sub_analyze
 *		Entry point for recursively analyzing a sub-statement.
 */
Query *
parse_sub_analyze(Node *parseTree, ParseState *parentParseState,
				  CommonTableExpr *parentCTE,
				  bool locked_from_parent,
				  bool resolve_unknowns)
{
	ParseState *pstate = make_parsestate(parentParseState);
	Query	   *query;

	pstate->p_parent_cte = parentCTE;
	pstate->p_locked_from_parent = locked_from_parent;
	pstate->p_resolve_unknowns = resolve_unknowns;

	query = transformStmt(pstate, parseTree);

	parentParseState->p_subselect = false;	/* ReqID:SRS-SECURITY-DATADESENSITIZATION */

	free_parsestate(pstate);

	return query;
}

/*
 * transformTopLevelStmt -
 *	  transform a Parse tree into a Query tree.
 *
 * This function is just responsible for transferring statement location data
 * from the RawStmt into the finished Query.
 */
Query *
transformTopLevelStmt(ParseState *pstate, RawStmt *parseTree)
{
	Query	   *result;

	/* We're at top level, so allow SELECT INTO */
	result = transformOptionalSelectInto(pstate, parseTree->stmt);

	result->stmt_location = parseTree->stmt_location;
	result->stmt_len = parseTree->stmt_len;

	return result;
}

/*
 * transformOptionalSelectInto -
 *	  If SELECT has INTO, convert it to CREATE TABLE AS.
 *
 * The only thing we do here that we don't do in transformStmt() is to
 * convert SELECT ... INTO into CREATE TABLE AS.  Since utility statements
 * aren't allowed within larger statements, this is only allowed at the top
 * of the parse tree, and so we only try it before entering the recursive
 * transformStmt() processing.
 */
static Query *
transformOptionalSelectInto(ParseState *pstate, Node *parseTree)
{
	if (IsA(parseTree, SelectStmt))
	{
		SelectStmt *stmt = (SelectStmt *) parseTree;

		/* If it's a set-operation tree, drill down to leftmost SelectStmt */
		while (stmt && stmt->op != SETOP_NONE)
			stmt = stmt->larg;
		Assert(stmt && IsA(stmt, SelectStmt) && stmt->larg == NULL);

		if (stmt->intoClause)
		{
			CreateTableAsStmt *ctas = makeNode(CreateTableAsStmt);

			/* Begin - ReqID:SRS-MYSQL-SET */
			if (compatible_mode == MYS_MODE && stmt->intoClause->colNames != NIL)
			{
				ListCell *cell;

				if (IsA(linitial(stmt->intoClause->colNames), VarRef))
				{
					bool	usevarflg = true;
					foreach(cell, stmt->intoClause->colNames)
					{
						if (IsA(lfirst(cell), VarRef))
						{
							VarRef *ref = (VarRef *)lfirst(cell);

							if (ref->varkind != USER_VAR)
							{
								usevarflg = false;
								break;
							}
						}
						else
						{
							usevarflg = false;
							break;
						}
					}

					if (usevarflg)
					{
						SelectIntoUserVarStmt *newStmt;
						newStmt = makeNode(SelectIntoUserVarStmt);
						newStmt->target = stmt->intoClause->colNames;
						newStmt->expr = parseTree;
						stmt->intoClause = NULL;

						parseTree = (Node *) newStmt;
						return transformStmt(pstate, parseTree);
					}
				}
			}
			/* End - ReqID:SRS-MYSQL-SET */

			/* Begin - ReqID:SRS-MYSQL-SQL-LOADDATA */
			if (compatible_mode == MYS_MODE && stmt->intoClause->is_outfile)
			{

				CopyStmt *copys = makeNode(CopyStmt);
				copys->relation = NULL;
				copys->attlist = NIL;
				copys->is_from = false;
				copys->options = stmt->intoClause->copy_options;
				copys->filename = stmt->intoClause->filename;

				stmt->intoClause = NULL;
				copys->query = parseTree;
				copys->select_into = true;	/* bug#z7477 */

				parseTree = (Node *) copys;

				return transformStmt(pstate, parseTree);
			}
			/* End - ReqID:SRS-MYSQL-SQL-LOADDATA */

			ctas->query = parseTree;
			ctas->into = stmt->intoClause;
			ctas->objtype = OBJECT_TABLE;
			ctas->is_select_into = true;

			/*
			 * Remove the intoClause from the SelectStmt.  This makes it safe
			 * for transformSelectStmt to complain if it finds intoClause set
			 * (implying that the INTO appeared in a disallowed place).
			 */
			stmt->intoClause = NULL;

			parseTree = (Node *) ctas;
		}
	}

	return transformStmt(pstate, parseTree);
}

/*
 * transformStmt -
 *	  recursively transform a Parse tree into a Query tree.
 */
Query *
transformStmt(ParseState *pstate, Node *parseTree)
{
	Query	   *result;

	/*
	 * We apply RAW_EXPRESSION_COVERAGE_TEST testing to basic DML statements;
	 * we can't just run it on everything because raw_expression_tree_walker()
	 * doesn't claim to handle utility statements.
	 */
#ifdef RAW_EXPRESSION_COVERAGE_TEST
	switch (nodeTag(parseTree))
	{
		case T_SelectStmt:
		case T_InsertStmt:
		case T_UpdateStmt:
		case T_DeleteStmt:
		case T_MergeStmt:
		case T_MultiTableInsertStmt: /* SRS-SQL-MULTITABLEINSERT */
			(void) test_raw_expression_coverage(parseTree, NULL);
			break;
		default:
			break;
	}
#endif							/* RAW_EXPRESSION_COVERAGE_TEST */

	/*
	 * Caution: when changing the set of statement types that have non-default
	 * processing here, see also stmt_requires_parse_analysis() and
	 * analyze_requires_snapshot().
	 */
	switch (nodeTag(parseTree))
	{
			/*
			 * Optimizable statements
			 */
		case T_InsertStmt:
			result = transformInsertStmt(pstate, (InsertStmt *) parseTree);
			break;

		case T_DeleteStmt:
			result = transformDeleteStmt(pstate, (DeleteStmt *) parseTree);
			break;

		case T_UpdateStmt:
			result = transformUpdateStmt(pstate, (UpdateStmt *) parseTree);
			break;

		case T_MergeStmt:
			result = (*pg_transform_merge_stmt_hook)(pstate, (MergeStmt *) parseTree);
			break;

		/* Begin - SRS-SQL-MULTITABLEINSERT */
		case T_MultiTableInsertStmt:
			if (convert_multiinsert_with_cte)
				result = transformMultiTableInsertWithCte(pstate,
						(MultiTableInsertStmt *) parseTree);
			else
				result = transformMultiTableInsertStmt(pstate,
						(MultiTableInsertStmt *) parseTree);
			break;
		/* End - SRS-SQL-MULTITABLEINSERT */

		case T_SelectStmt:
			{
				SelectStmt *n = (SelectStmt *) parseTree;

				if (n->valuesLists)
					result = transformValuesClause(pstate, n);
				else if (n->op == SETOP_NONE)
					result = transformSelectStmt(pstate, n);
				else
					result = transformSetOperationStmt(pstate, n);
			}
			break;

		case T_ReturnStmt:
			result = transformReturnStmt(pstate, (ReturnStmt *) parseTree);
			break;

		case T_PLAssignStmt:
			result = transformPLAssignStmt(pstate,
										   (PLAssignStmt *) parseTree);
			break;

			/*
			 * Special cases
			 */
		case T_DeclareCursorStmt:
			result = transformDeclareCursorStmt(pstate,
												(DeclareCursorStmt *) parseTree);
			break;

		case T_ExplainStmt:
			result = transformExplainStmt(pstate,
										  (ExplainStmt *) parseTree);
			break;

		case T_CreateTableAsStmt:
			result = transformCreateTableAsStmt(pstate,
												(CreateTableAsStmt *) parseTree);
			break;

		case T_CallStmt:
			result = transformCallStmt(pstate,
									   (CallStmt *) parseTree);
			break;

		/* Begin - ReqID:SRS-MYSQL-SET */
		case T_MysDoStmt:
			result = transformMysDoStmt(pstate,
										(MysDoStmt *) parseTree);
			break;

		case T_SelectIntoUserVarStmt:
			result = transformSelectIntoUservarStmt(pstate, (SelectIntoUserVarStmt *) parseTree);
			break;

		case T_VarAssign:
			result = (Query *)transformVarAssign(pstate, (VarAssign *)parseTree, true);
			break;
		/* End - ReqID:SRS-MYSQL-SET */

		default:

			/*
			 * other statements don't require any transformation; just return
			 * the original parsetree with a Query node plastered on top.
			 */
			result = makeNode(Query);
			result->commandType = CMD_UTILITY;
			result->utilityStmt = (Node *) parseTree;
			break;
	}

	/* Mark as original query until we learn differently */
	result->querySource = QSRC_ORIGINAL;
	result->canSetTag = true;

	return result;
}

/*
 * stmt_requires_parse_analysis
 *		Returns true if parse analysis will do anything non-trivial
 *		with the given raw parse tree.
 *
 * Generally, this should return true for any statement type for which
 * transformStmt() does more than wrap a CMD_UTILITY Query around it.
 * When it returns false, the caller can assume that there is no situation
 * in which parse analysis of the raw statement could need to be re-done.
 *
 * Currently, since the rewriter and planner do nothing for CMD_UTILITY
 * Queries, a false result means that the entire parse analysis/rewrite/plan
 * pipeline will never need to be re-done.  If that ever changes, callers
 * will likely need adjustment.
 */
bool
stmt_requires_parse_analysis(RawStmt *parseTree)
{
	bool		result;

	switch (nodeTag(parseTree->stmt))
	{
			/*
			 * Optimizable statements
			 */
		case T_InsertStmt:
		case T_DeleteStmt:
		case T_UpdateStmt:
		case T_MergeStmt:
		/* Begin - SRS-SQL-MULTITABLEINSERTT */
		case T_MultiTableInsertStmt:
		/* End - SRS-SQL-MULTITABLEINSERT */
		case T_SelectStmt:
                case T_ReturnStmt:	
		case T_PLAssignStmt:
			result = true;
			break;

			/*
			 * Special cases
			 */
		case T_DeclareCursorStmt:
		case T_ExplainStmt:
		case T_CreateTableAsStmt:
		case T_CallStmt:
			result = true;
			break;

		default:
			/* all other statements just get wrapped in a CMD_UTILITY Query */
			result = false;
			break;
	}

	return result;
}

/*
 * analyze_requires_snapshot
 *		Returns true if a snapshot must be set before doing parse analysis
 *		on the given raw parse tree.
 */
bool
analyze_requires_snapshot(RawStmt *parseTree)
{
	/*
	 * Currently, this should return true in exactly the same cases that
	 * stmt_requires_parse_analysis() does, so we just invoke that function
	 * rather than duplicating it.  We keep the two entry points separate for
	 * clarity of callers, since from the callers' standpoint these are
	 * different conditions.
	 *
	 * While there may someday be a statement type for which transformStmt()
	 * does something nontrivial and yet no snapshot is needed for that
	 * processing, it seems likely that making such a choice would be fragile.
	 * If you want to install an exception, document the reasoning for it in a
	 * comment.
	 */
	return stmt_requires_parse_analysis(parseTree);
}


/*Begin - ReqID:SRS-MYSQL-SQL-DELETE */
/* 
 * transformRangeSubselectInListClause
 * Check if a subquery exists.
 */
static void
transformRangeSubselectInListClause(DeleteStmt *stmt)
{
	List	   *rellist = NIL;
	ListCell   *lc = NULL;

	if (stmt->usingClause)
		rellist = copyObject(stmt->usingClause);
	else if(stmt->relationList)
		rellist = copyObject(stmt->relationList);
	/* check subquery */
	foreach(lc, rellist)
	{
		Node   *node = (Node *)lfirst(lc);
		if (IsA(node, RangeSubselect))
		{
			RangeSubselect   *rs = (RangeSubselect *)node;
			elog(ERROR,"The target table %s of the DELETE is not updatable", rs->alias->aliasname);
		}
		else
			continue;
	}
	return;
}

/*
 * transformCollistInFromcClause - 
 * Checks whether the specified table exists in the from clause or using clause.
 */
static void
transformCollistInFromcClause(List *rangevarlist, List *collist)
{
	List	   *rangelist = NIL;
	ListCell   *lc = NULL;
	ListCell   *lc1 = NULL;
	bool		is_existinrv = false;
	foreach(lc, collist)
	{
		char		   *relname = NULL;
		char		   *nspname = NULL;
		Node		   *starnode = NULL;
		ColumnRef	   *cref = (ColumnRef *)lfirst(lc);

		starnode = llast(cref->fields);
		/* If exists A_star,delete it in fileds. */
		if (IsA(starnode, A_Star))
			cref->fields = list_delete(cref->fields, starnode);
		switch(list_length(cref->fields))
		{
			case 1:
				{
					Node   *field1 = (Node *)linitial(cref->fields);
					Assert(IsA(field1, String));
					relname = strVal(field1);
				}
				break;
			case 2:
				{
					Node   *field1 = (Node *)linitial(cref->fields);
					Node   *field2 = (Node *)lsecond(cref->fields);
					Assert(IsA(field1, String));
					Assert(IsA(field2, String));
					relname = strVal(field2);
					nspname = strVal(field1);
				}
				break;
			default:
				elog(ERROR, " The format of the table name specified for deletion is incorrect! ");
				break;
		}
		is_existinrv = false;
		rangelist = copyObject(rangevarlist);
		foreach(lc1, rangelist)
		{
			RangeVar   *rv = (RangeVar *)lfirst(lc1);
			if (!strcmp(relname, rv->relname))
			{
				if (nspname && rv->schemaname)
				{
					if (!strcmp(nspname, rv->schemaname))
					{
						is_existinrv = true;
						break;
					}
				}
				else if (!nspname || !rv->schemaname)
				{
					is_existinrv = true;
					break;
				}
			}
			else
			{
				if (rv->alias)
				{
					if (!strcmp(relname, rv->alias->aliasname))
					{
						if (nspname && rv->schemaname)
						{
							if (!strcmp(nspname, rv->schemaname))
							{
								is_existinrv = true;
								break;
							}
						}
						else if (!nspname || !rv->schemaname)
						{
							is_existinrv = true;
							break;
						}
					}
				}
			}
		}
		/* If it does not exist, an error is reported. */
		if (!is_existinrv)
			elog(ERROR, "Unknown table %s in MULTI DELETE", relname);
	}
	return;
}
/* End - ReqID:SRS-MYSQL-SQL-DELETE */


/*
 * transformDeleteStmt -
 *	  transforms a Delete Statement
 */
static Query *
transformDeleteStmt(ParseState *pstate, DeleteStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	ParseNamespaceItem *nsitem = NULL;
	Node	   *qual;

	qry->commandType = CMD_DELETE;

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		if (stmt->usingClause || stmt->relationList)
		{
			pstate->p_addDeleteTable = true;
			/* check if exist subquery. */
			transformRangeSubselectInListClause(stmt);
		}
		if (stmt->collist)
		{
			if (list_length(stmt->collist) > 1 && !stmt->usingClause &&
				 !stmt->relationList)
				ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("multi-relation delete must specify the table to delete."),
				 errhint("Please note whether it's compatible with MySQL mode.")));
			pstate->p_delrellist = stmt->collist;
		}
	}
	/* set up range table with just the result rel */
	if (stmt->relation)
	{
		if (MYS_MODE == compatible_mode && stmt->relationList && stmt->collist) /* BUG#Z5121 */
		{
			transformCollistInFromcClause(list_make1(stmt->relation), stmt->collist);
			stmt->relationList = NIL;
		}
		if (MYS_MODE == compatible_mode && stmt->relation->ispartition) /* BUG#Z5121 */
			transformGetpartionRelationOid(qry, stmt->relation);
		qry->resultRelation = setTargetTable(pstate, stmt->relation,
											 stmt->relation->inh,
											 true,
											 ACL_DELETE);
		nsitem = pstate->p_target_nsitem;
		
		/* subqueries in USING cannot access the result relation */
		nsitem->p_lateral_only = true;
		nsitem->p_lateral_ok = false;
	}
	/* End - ReqID:SRS-MYSQL-SQL-DELETE */

	/* there's no DISTINCT in DELETE */
	qry->distinctClause = NIL;

	/*
	 * The USING clause is non-standard SQL syntax, and is equivalent in
	 * functionality to the FROM list that can be specified for UPDATE. The
	 * USING keyword is used rather than FROM because FROM is already a
	 * keyword in the DELETE syntax.
	 */
	if (MYS_MODE == compatible_mode &&
		(stmt->usingClause || stmt->relationList)) /* BUG#Z5121 */
	{
		if (qry->cteList)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("Un-support feature"),
					errdetail("mutilple-relations delete doesn't support with clause.")));
		if (stmt->usingClause)
			stmt->usingClause = transfromRelationposition(pstate, stmt->usingClause);
		else
			stmt->relationList = transfromRelationposition(pstate, stmt->relationList);
	}

	transformFromClause(pstate, stmt->usingClause);

	/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		transformFromClause(pstate, stmt->relationList);

		if (pstate->p_indexhintLists)
			qry->indexhintList = pstate->p_indexhintLists;

		/* Checks whether the specified table exists in the source table. */
		if (pstate->p_deleteRangerVars && pstate->p_delrellist && 
			(stmt->relationList || stmt->usingClause))
			transformCollistInFromcClause(pstate->p_deleteRangerVars, pstate->p_delrellist);
		/* Support for other situations, eg:delete from t,t1 using t; */
		else if (pstate->p_delrellist && (stmt->relationList || stmt->usingClause))
			transformCollistInFromcClause(stmt->relationList ? stmt->relationList : stmt->usingClause, pstate->p_delrellist);

		if (pstate->p_addupdatedeleterv)
			transformGetpartionRelationOids(qry, pstate->p_addupdatedeleterv);

		if (pstate->p_updateRelations)
		{
			if (list_length(pstate->p_updateRelations) == 1)
				qry->resultRelation = linitial_int(pstate->p_updateRelations);
			else
				qry->resultRelations = pstate->p_updateRelations;
		}
		transformLimitSortClause(pstate, stmt, qry, true);
	}

	/* remaining clauses can reference the result relation normally */
	if (stmt->relation)
	{
		nsitem->p_lateral_only = false;
		nsitem->p_lateral_ok = true;
	}
	/* End - ReqID:SRS-MYSQL-SQL-DELETE */

	qual = transformWhereClause(pstate, stmt->whereClause,
								EXPR_KIND_WHERE, "WHERE");

	qry->returningList = transformReturningList(pstate, stmt->returningList);

	/* Begin - ReqID:SRS-SQL-ROWNUM */
	if (pstate->p_hasRownum)
	{
		qry->hasRownum = pstate->p_hasRownum;
		addRangeTableEntryForCount(pstate);
	}
	/* End - ReqID:SRS-SQL-ROWNUM */

	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasWindowFuncs = pstate->p_hasWindowFuncs;
	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasAggs = pstate->p_hasAggs;

	assign_query_collations(pstate, qry);

	/* this must be done after collations, for reliable comparison of exprs */
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry);

	return qry;
}

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
/*
 * make_null_const
 * Construct a null node.
 */
static Node *
make_null_const(Oid typeid)
{
	Const   *cnst = makeNode(Const);
	cnst->consttype = typeid;
	cnst->consttypmod = -1;
	cnst->constcollid = 0;
	cnst->constlen = -2;
	cnst->constvalue = 0;
	cnst->constisnull = true;
	cnst->constbyval = false;
	cnst->location = -1;		/* "unknown" */
	return (Node *) cnst;
}

static Node *
build_default_value(ParseState *pstate, Var *var)
{
	Node *node;
	AttrNumber attrno = var->varattno;
	Form_pg_attribute att_tup;

	att_tup = TupleDescAttr(pstate->p_target_relation->rd_att, attrno - 1);

	if (att_tup->atthasdef)
		node = build_column_default(pstate->p_target_relation, attrno);
	else
		node = make_null_const(exprType((Node *) var));

	return node;
}

/*
 * GetTableOidInSubquery
 * Get temporary table oid in subquery.
 */
static Oid
GetTableOidInSubquery(RangeTblEntry* rte)
{
	ListCell   *lc;
	if (rte->rtekind == RTE_SUBQUERY && rte->subquery)
	{
		if (rte->subquery->rtable)
		{
			foreach(lc, rte->subquery->rtable)
			{
				RangeTblEntry   *subrte = lfirst(lc);
				if (subrte->relid != InvalidOid)
				{
					if (get_rel_persistence(subrte->relid) == 't')
						return subrte->relid;
					else
						return InvalidOid;
				}
				else
				{
					return GetTableOidInSubquery(subrte);
				}
			}
		}
	}
	return InvalidOid;
}

/*
 * transformCheckTemptableIsMultiUse
 * Analyze whether the temporary table is used multiple times.
 */
static void
transformCheckTemptableIsMultiUse(List* rtable, List* subrtable)
{
	char		relpersistence = '0';
	char		subrelpersistence = '0';
	ListCell   *lc = NULL;
	ListCell   *sublc = NULL;
	Oid			reloid = InvalidOid;
	Oid			subreloid = InvalidOid;
	foreach(lc, rtable)
	{
		RangeTblEntry   *rte = lfirst(lc);
		if (rte->relid != InvalidOid)
		{
			reloid = rte->relid;
			relpersistence = get_rel_persistence(rte->relid);
			if (relpersistence != 't')
				continue;
		}
		else
		{
			reloid = GetTableOidInSubquery(rte);
			if(reloid != InvalidOid)
			{
				relpersistence = get_rel_persistence(reloid);
				if (relpersistence != 't')
					continue;
			}
		}
		foreach(sublc, subrtable)
		{
			RangeTblEntry   *sub_rte = lfirst(sublc);
			if (sub_rte->relid != InvalidOid)
			{
				subreloid = sub_rte->relid;
				subrelpersistence = get_rel_persistence(sub_rte->relid);
			}
			else
			{
				subreloid = GetTableOidInSubquery(sub_rte);
				if (subreloid != InvalidOid)
					subrelpersistence = get_rel_persistence(subreloid);
			}

			if (reloid == subreloid && subrelpersistence == relpersistence &&
				relpersistence == 't')
			{
				elog(ERROR," Can't reopen table:%s", get_rel_name(reloid));
			}
		}
	}
}
/* End - ReqID:SRS-MYSQL-SQL-INSERT */


/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
/*
 * transformGetpartiontable
 * Get the specified partition table information and get the partition table oid.
 */
static void
transformGetpartionRelationOid(Query            *qry, RangeVar *relation)
{
	ListCell   *lc = NULL;
	List	   *partitionoid = NIL;
	Oid			relid = InvalidOid;
	/* Get partition relation Oid. */
	if (relation->schemaname)
		relid = get_relname_relid(relation->relname, get_namespace_oid(relation->schemaname, false));
	else
		relid = RelnameGetRelid(relation->relname);
	/* Get the partition OID based on the partition name. */
	foreach(lc, relation->partitionnamelist)
	{
		Node   *node = (Node*)lfirst(lc);
		if (IsA(node, String))
		{
			char   *relname = strVal(node);
			Oid		reloid = InvalidOid;
			if (relation->schemaname)
				reloid = get_relname_relid(relname, get_namespace_oid(relation->schemaname, false));
			else
				reloid = RelnameGetRelid(relname);
			/* Record partition Oid. */
			if (OidIsValid(reloid))
			{
				/* lookup pg_inherits, Check whether the partition table and partition correspond. */
				if (get_partition_parent(reloid, true) != relid)
					ereport(ERROR,
								(errcode(ERRCODE_UNDEFINED_TABLE),
								 errmsg("Partition relation \"%s\" does not exist in %s.", relname, relation->relname)));
				if (get_rel_relkind(reloid) == RELKIND_PARTITIONED_TABLE)
					ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("Un-support feature"),
								 errdetail("The specified partition %s cannot be a partition table!", relname)));
				if (!list_member_oid(partitionoid, reloid))
					partitionoid = lappend_oid(partitionoid, reloid);
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_TABLE),
						 errmsg("Partition relation \"%s\" does not exist", relname)));
		}
	}
	qry->partOids = partitionoid;
	return;
}
/* End - ReqID:SRS-MYSQL-SQL-REPLACE */

static void
transformGetpartionRelationOids(Query *qry, List *relations)
{
	ListCell   *lc = NULL;
	foreach(lc, relations)
	{
		Node   *node = lfirst(lc);
		if (IsA(node, RangeVar))
		{
			RangeVar *rel = (RangeVar *)node;

			Oid		reloid = InvalidOid;
			if (rel->schemaname)
				reloid = get_relname_relid(rel->relname, get_namespace_oid(rel->schemaname, false));
			else
				reloid = RelnameGetRelid(rel->relname);
			
			if (rel->ispartition)
			{
				transformGetpartionRelationOid(qry, rel);
				qry->partOids = lcons_oid(reloid, qry->partOids);
				qry->partreloids = lappend(qry->partreloids, qry->partOids);
				qry->partOids = NIL;
			}
		}
	}
	return;
}

/* Begin - bug#5797 */
typedef struct
{
	ParseState 	*pstate;
	List		*exprList;
	List		*attrnos;
	AttrNumber 	 attrno;
} check_insert_values_context;

static Node *
check_insert_values_mutator(Node *node, check_insert_values_context *context)
{
	List		*exprList = context->exprList;
	List		*attrnos = context->attrnos;
	ParseState 	*pstate = context->pstate;
	AttrNumber	 attrno = context->attrno;

	if (node == NULL)
		return node;

	if (IsA(node, SubLink))
		return node;

	/**
	 * Replace the Var with column's default value.
	 * Only OpExpr and FuncExpr node need to be processed.
	 * 
	 * Such as
	 *		CREATE TABLE t (
	 * 			col1 int DEFAULT 1,
	 * 			col2 int DEFAULT 2,
	 * 		);
	 * 
	 * 1. INSERT INTO t(col1) VALUES (col2);
	 * We will replace `col2` with col2's default value, which is 2.
	 * 
	 * 2. INSERT INTO t(col1, col2) VALUES (3, col1);
	 * We will replace `col1` with col1's value, which is 3.
	 * 
	 * 3. INSERT INTO t(col1, col2) VALUES (col2, 3);
	 * We will replace `col2` with col2's defailt value, which is 2, 
	 * although the value for col1 refers to col2, but it's assigned after col1. 
	 */
	if (IsA(node, Var))
	{
		Node	   *ret = NULL;
		Var		   *var = (Var *) node;
		AttrNumber	varattno = var->varattno;
		
		if (attrno <= varattno)
		{
			ret = build_default_value(pstate, var);
		}
		else
		{
			ListCell   *lc, *lc2;
			
			forboth (lc, attrnos, lc2, exprList)
			{
				int	attrno = lfirst_int(lc);

				if (attrno == varattno)
				{
					ret = lfirst(lc2);
					break;
				}
			}

			if (ret == NULL)
				ret = build_default_value(pstate, var);
		}

		if (exprType(ret) != exprType(node))
		{
			ret = coerce_to_target_type(pstate, ret, 
										exprType(ret), 
										exprType(node), 
										exprTypmod(node), 
										COERCION_EXPLICIT, 
										COERCE_EXPLICIT_CAST, 
										-1);
		}

		return ret;
	}

	return expression_tree_mutator(node, check_insert_values_mutator, context);
}

static void
check_insert_values(ParseState *pstate, List *exprList, List *attrnos)
{
	ListCell   *lc, *lc2;
	check_insert_values_context context;

	/**
	 * Do nothing if Insert stmt called by Rule Stmt
	 */
	if (pstate->p_ruleflag)
		return;

	context.pstate = pstate;
	context.exprList = exprList;
	context.attrnos = attrnos;
	context.attrno = 0;

	forboth(lc, exprList, lc2, attrnos)
	{
		Node 	*node = lfirst(lc);

		context.attrno = lfirst_int(lc2);

		lfirst(lc) = check_insert_values_mutator(node, &context);
	}
}
/* End - bug#5797 */

/*
 * transformInsertStmt -
 *	  transform an Insert Statement
 */
static Query *
transformInsertStmt(ParseState *pstate, InsertStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	SelectStmt *selectStmt = (SelectStmt *) stmt->selectStmt;
	List	   *exprList = NIL;
	bool		isGeneralSelect;
	List	   *sub_rtable;
	List	   *sub_namespace;
	List	   *icolumns;
	List	   *attrnos;
	ParseNamespaceItem *nsitem;
	RangeTblEntry *rte;
	ListCell   *icols;
	ListCell   *attnos;
	ListCell   *lc;
	bool		isOnConflictUpdate;
	AclMode		targetPerms;

	/* There can't be any outer WITH to worry about */
	Assert(pstate->p_ctenamespace == NIL);

	qry->commandType = CMD_INSERT;
	pstate->p_is_insert = true;

	/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		qry->isReplace = stmt->isReplace;
		pstate->p_isreplace = stmt->isReplace; /* BUG#Z1626 */
		if (stmt->relation && stmt->relation->ispartition)
		{
			/* Get the specified partition table information. */
			transformGetpartionRelationOid(qry, stmt->relation);
			if (!stmt->cols && stmt->relation->colnamelist)
				stmt->cols = stmt->relation->colnamelist;
		}
	}
	/* End - ReqID:SRS-MYSQL-SQL-REPLACE */

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	qry->override = stmt->override;

	isOnConflictUpdate = (stmt->onConflictClause &&
						  stmt->onConflictClause->action == ONCONFLICT_UPDATE);

	/*
	 * We have three cases to deal with: DEFAULT VALUES (selectStmt == NULL),
	 * VALUES list, or general SELECT input.  We special-case VALUES, both for
	 * efficiency and so we can handle DEFAULT specifications.
	 *
	 * The grammar allows attaching ORDER BY, LIMIT, FOR UPDATE, or WITH to a
	 * VALUES clause.  If we have any of those, treat it as a general SELECT;
	 * so it will work, but you can't use DEFAULT items together with those.
	 */
	isGeneralSelect = (selectStmt && (selectStmt->valuesLists == NIL ||
									  selectStmt->sortClause != NIL ||
									  selectStmt->limitOffset != NULL ||
									  selectStmt->limitCount != NULL ||
									  selectStmt->lockingClause != NIL ||
									  selectStmt->withClause != NULL));

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
	if (isOnConflictUpdate)
		targetPerms |= ACL_UPDATE;
	qry->resultRelation = setTargetTable(pstate, stmt->relation,
										 false, false, targetPerms);

	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
		transformTableAndSchemaInCols(pstate, &(stmt->cols));
	/* Validate stmt->cols list, or build default list if no list given */
	icolumns = checkInsertTargets(pstate, stmt->cols, &attrnos);
	Assert(list_length(icolumns) == list_length(attrnos));

	/*
	 * Determine which variant of INSERT we have.
	 */
	if (selectStmt == NULL)
	{
		/*
		 * We have INSERT ... DEFAULT VALUES.  We can handle this case by
		 * emitting an empty targetlist --- all columns will be defaulted when
		 * the planner expands the targetlist.
		 */
		exprList = NIL;
	}
	else if (isGeneralSelect)
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

		selectQuery = transformStmt(sub_pstate, stmt->selectStmt);

		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		/* In MySQL mode, check if the temporary table is referenced multiple times */
		if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
			transformCheckTemptableIsMultiUse(pstate->p_rtable, sub_pstate->p_rtable);
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */

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
		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
		{
			addNSItemToQuery(pstate, nsitem, true, false, true);
		}
		else
			addNSItemToQuery(pstate, nsitem, true, false, false);
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */
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
		}

		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		/* Check that the number of column values in the query is the same as
		 * the number of columns inserted into the table.
		 */
		if (compatible_mode ==  MYS_MODE && exprList && icolumns) /* BUG#Z5121 */
		{
			if (list_length(exprList) != list_length(icolumns))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("Column count doesn't match value count!")));
		}
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */

		/* Prepare row for assignment to target table */
		exprList = transformInsertRow(pstate, exprList,
									  stmt->cols,
									  icolumns, attrnos,
									  false);
	}
	else if (list_length(selectStmt->valuesLists) > 1)
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

		foreach(lc, selectStmt->valuesLists)
		{
			List	   *sublist = (List *) lfirst(lc);

			/*
			 * Do basic expression transformation (same as a ROW() expr, but
			 * allow SetToDefault at top level)
			 */
			sublist = transformExpressionList(pstate, sublist,
											  EXPR_KIND_VALUES, true);

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

			/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
			/* In mysql mode, handle column references. */
			if (MYS_MODE == compatible_mode && sublist) /* BUG#Z5121 */
			{
				check_insert_values(pstate, sublist, attrnos); /* bug#5797 */
				
				/* Begin - BUG#Z1434 */
				if (enable_ignore_columns_list)
				{
					if (list_length(sublist) != list_length(icolumns) && !stmt->cols)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("Column count doesn't match value count!")));
				}
				/* End - BUG#Z1434 */
			}
			/*End - ReqID:SRS-MYSQL-SQL-INSERT */
			/*
			 * Prepare row for assignment to target table.  We process any
			 * indirection on the target column specs normally but then strip
			 * off the resulting field/array assignment nodes, since we don't
			 * want the parsed statement to contain copies of those in each
			 * VALUES row.  (It's annoying to have to transform the
			 * indirection specs over and over like this, but avoiding it
			 * would take some really messy refactoring of
			 * transformAssignmentIndirection.)
			 */
			sublist = transformInsertRow(pstate, sublist,
										 stmt->cols,
										 icolumns, attrnos,
										 true);

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
		addNSItemToQuery(pstate, nsitem, true, false, false);

		/*
		 * Generate list of Vars referencing the RTE
		 */
		exprList = expandNSItemVars(nsitem, 0, -1, NULL);

		/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		/* the auto_increment check has been completed in previous foreach */
		pstate->p_check_column_auto_increment = false;
		/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		/*
		 * Re-apply any indirection on the target column specs to the Vars
		 */
		exprList = transformInsertRow(pstate, exprList,
									  stmt->cols,
									  icolumns, attrnos,
									  false);
		/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		pstate->p_check_column_auto_increment = true;
		/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	}
	else
	{
		/*
		 * Process INSERT ... VALUES with a single VALUES sublist.  We treat
		 * this case separately for efficiency.  The sublist is just computed
		 * directly as the Query's targetlist, with no VALUES RTE.  So it
		 * works just like a SELECT without any FROM.
		 */
		List	   *valuesLists = selectStmt->valuesLists;
		int			nattrs;
		int			nvalues;

		Assert(list_length(valuesLists) == 1);
		Assert(selectStmt->intoClause == NULL);

		/* Begin - ReqID:SRS-PLSQL-TYPE */
		if (selectStmt->valuesIsrow == true)
		{
			/*
			 * Transform the row expr into its fields expr.
			 * e.g., the insert statment is INSERT INTO tablename VALUES
			 * row_variable; We convert to the equivalent statement: INSERT INTO
			 * tablename VALUES (row_variable.field1,...,row_variable.fieldN);
			 */
			exprList = transformRowExpression(pstate,
											  (List *) linitial(valuesLists),
											  EXPR_KIND_VALUES_SINGLE,
											  true,
											  stmt->cols /* ReqID:SRS-PLSQL-RECORD */);
			nattrs = list_length(icolumns);
			nvalues = list_length(exprList);
			if (nattrs > nvalues)
				elog(ERROR, "not enough values");
			else if (nattrs < nvalues)
				elog(ERROR, "too many values");
		}
		else
		/* End - ReqID:SRS-PLSQL-TYPE */
		{
			/*
			 * Do basic expression transformation (same as a ROW() expr, but allow
			 * SetToDefault at top level)
			 */
			exprList = transformExpressionList(pstate,
											   (List *) linitial(valuesLists),
											   EXPR_KIND_VALUES_SINGLE,
											   true);
		}

		if (MYS_MODE == compatible_mode && exprList) /* BUG#Z5121 */
		{
			check_insert_values(pstate, exprList, attrnos); /* bug#5797 */

			/* Begin - BUG#Z1434 */
			if (enable_ignore_columns_list)
			{
				if (list_length(exprList) != list_length(icolumns) && !stmt->cols)
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("Column count doesn't match value count!")));
			}
			/* End - BUG#Z1434 */
		}

		/* Prepare row for assignment to target table */
		exprList = transformInsertRow(pstate, exprList,
									  stmt->cols,
									  icolumns, attrnos,
									  false);
	}

	/*
	 * Generate query's target list using the computed list of expressions.
	 * Also, mark all the target columns as needing insert permissions.
	 */
	rte = pstate->p_target_nsitem->p_rte;
	qry->targetList = NIL;
	Assert(list_length(exprList) <= list_length(icolumns));
	forthree(lc, exprList, icols, icolumns, attnos, attrnos)
	{
		Expr	   *expr = (Expr *) lfirst(lc);
		ResTarget  *col = lfirst_node(ResTarget, icols);
		AttrNumber	attr_num = (AttrNumber) lfirst_int(attnos);
		TargetEntry *tle;

		tle = makeTargetEntry(expr,
							  attr_num,
							  col->name,
							  false);
		qry->targetList = lappend(qry->targetList, tle);

		rte->insertedCols = bms_add_member(rte->insertedCols,
										   attr_num - FirstLowInvalidHeapAttributeNumber);
	}

	/*
	 * If we have any clauses yet to process, set the query namespace to
	 * contain only the target relation, removing any entries added in a
	 * sub-SELECT or VALUES list.
	 */
	if (stmt->onConflictClause || stmt->returningList)
	{
		pstate->p_namespace = NIL;
		addNSItemToQuery(pstate, pstate->p_target_nsitem,
						 false, true, true);
	}

	/* Process ON CONFLICT, if any. */
	if (stmt->onConflictClause)
		qry->onConflict = transformOnConflictClause(pstate,
													stmt->onConflictClause);

	/* Process DUPLICATE KEY UPDATE, if any. */
	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	if (MYS_MODE == compatible_mode && stmt->upsertClause) /* BUG#Z5121 */
	{
		qry->upsertClause = transformUpsertClause(pstate, stmt->upsertClause, qry->resultRelations);
	}
	/* End - ReqID:SRS-MYSQL-SQL-INSERT */

	/* Process RETURNING, if any. */
	if (stmt->returningList)
		qry->returningList = transformReturningList(pstate,
													stmt->returningList);

	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);

	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasSubLinks = pstate->p_hasSubLinks;

	/* Begin - ReqID:SRS-SQL-ROWNUM */
	if (pstate->p_hasRownum)
		elog(ERROR, "not expected ROWNUM in INSERT statement");
	/* End - ReqID:SRS-SQL-ROWNUM */

	assign_query_collations(pstate, qry);

	return qry;
}

/*
 * Prepare an INSERT row for assignment to the target table.
 *
 * exprlist: transformed expressions for source values; these might come from
 * a VALUES row, or be Vars referencing a sub-SELECT or VALUES RTE output.
 * stmtcols: original target-columns spec for INSERT (we just test for NIL)
 * icolumns: effective target-columns spec (list of ResTarget)
 * attrnos: integer column numbers (must be same length as icolumns)
 * strip_indirection: if true, remove any field/array assignment nodes
 */
List *
transformInsertRow(ParseState *pstate, List *exprlist,
				   List *stmtcols, List *icolumns, List *attrnos,
				   bool strip_indirection)
{
	List	   *result;
	ListCell   *lc;
	ListCell   *icols;
	ListCell   *attnos;

	/*
	 * Check length of expr list.  It must not have more expressions than
	 * there are target columns.  We allow fewer, but only if no explicit
	 * columns list was given (the remaining columns are implicitly
	 * defaulted).  Note we must check this *after* transformation because
	 * that could expand '*' into multiple items.
	 */
	if (list_length(exprlist) > list_length(icolumns))
	/* Begin - BUG#Z1626 */
	{
		if (pstate->p_isreplace && MYS_MODE == compatible_mode) /* BUG#Z5121 */
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("REPLACE has more expressions than target columns"),
					 parser_errposition(pstate,
										exprLocation(list_nth(exprlist,
															  list_length(icolumns))))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("INSERT has more expressions than target columns"),
					 parser_errposition(pstate,
										exprLocation(list_nth(exprlist,
															  list_length(icolumns))))));
	}
	/* End - BUG#Z1626 */
	if (stmtcols != NIL &&
		list_length(exprlist) < list_length(icolumns))
	{
		/*
		 * We can get here for cases like INSERT ... SELECT (a,b,c) FROM ...
		 * where the user accidentally created a RowExpr instead of separate
		 * columns.  Add a suitable hint if that seems to be the problem,
		 * because the main error message is quite misleading for this case.
		 * (If there's no stmtcols, you'll get something about data type
		 * mismatch, which is less misleading so we don't worry about giving a
		 * hint in that case.)
		 */
		/* Begin - BUG#Z1626 */
		if (pstate->p_isreplace && MYS_MODE == compatible_mode) /* BUG#Z5121 */
						ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg(" REPLACE has more target columns than expressions"),
					 ((list_length(exprlist) == 1 &&
					   count_rowexpr_columns(pstate, linitial(exprlist)) ==
					   list_length(icolumns)) ?
					  errhint("The replace source is a row expression containing the same number of columns expected by the REPLACE. Did you accidentally use extra parentheses?") : 0),
					 parser_errposition(pstate,
										exprLocation(list_nth(icolumns,
															  list_length(exprlist))))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("INSERT has more target columns than expressions"),
					 ((list_length(exprlist) == 1 &&
					   count_rowexpr_columns(pstate, linitial(exprlist)) ==
					   list_length(icolumns)) ?
					  errhint("The insertion source is a row expression containing the same number of columns expected by the INSERT. Did you accidentally use extra parentheses?") : 0),
					 parser_errposition(pstate,
										exprLocation(list_nth(icolumns,
															  list_length(exprlist))))));
		/* End - BUG#Z1626 */
	}

	/*
	 * Prepare columns for assignment to target table.
	 */
	result = NIL;
	forthree(lc, exprlist, icols, icolumns, attnos, attrnos)
	{
		Expr	   *expr = (Expr *) lfirst(lc);
		ResTarget  *col = lfirst_node(ResTarget, icols);
		int			attno = lfirst_int(attnos);
		/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		if (MYS_MODE == compatible_mode && pstate->p_check_column_auto_increment)
		{
			expr = set_auto_increment_func_node(pstate, expr, attno);
		}
		/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

		expr = transformAssignedExpr(pstate, expr,
									 EXPR_KIND_INSERT_TARGET,
									 col->name,
									 attno,
									 col->indirection,
									 col->location);

		if (strip_indirection)
		{
			/*
			 * We need to remove top-level FieldStores and SubscriptingRefs,
			 * as well as any CoerceToDomain appearing above one of those ---
			 * but not a CoerceToDomain that isn't above one of those.
			 */
			while (expr)
			{
				Expr	   *subexpr = expr;

				while (IsA(subexpr, CoerceToDomain))
				{
					subexpr = ((CoerceToDomain *) subexpr)->arg;
				}
				if (IsA(subexpr, FieldStore))
				{
					FieldStore *fstore = (FieldStore *) subexpr;

					expr = (Expr *) linitial(fstore->newvals);
				}
				else if (IsA(subexpr, SubscriptingRef))
				{
					SubscriptingRef *sbsref = (SubscriptingRef *) subexpr;

					if (sbsref->refassgnexpr == NULL)
						break;

					expr = sbsref->refassgnexpr;
				}
				else
					break;
			}
		}

		result = lappend(result, expr);
	}

	return result;
}

/*
 * transformOnConflictClause -
 *	  transforms an OnConflictClause in an INSERT
 */
static OnConflictExpr *
transformOnConflictClause(ParseState *pstate,
						  OnConflictClause *onConflictClause)
{
	ParseNamespaceItem *exclNSItem = NULL;
	List	   *arbiterElems;
	Node	   *arbiterWhere;
	Oid			arbiterConstraint;
	List	   *onConflictSet = NIL;
	Node	   *onConflictWhere = NULL;
	int			exclRelIndex = 0;
	List	   *exclRelTlist = NIL;
	OnConflictExpr *result;

	/*
	 * If this is ON CONFLICT ... UPDATE, first create the range table entry
	 * for the EXCLUDED pseudo relation, so that that will be present while
	 * processing arbiter expressions.  (You can't actually reference it from
	 * there, but this provides a useful error message if you try.)
	 */
	if (onConflictClause->action == ONCONFLICT_UPDATE)
	{
		Relation	targetrel = pstate->p_target_relation;
		RangeTblEntry *exclRte;

		exclNSItem = addRangeTableEntryForRelation(pstate,
												   targetrel,
												   RowExclusiveLock,
												   makeAlias("excluded", NIL),
												   false, false);
		exclRte = exclNSItem->p_rte;
		exclRelIndex = exclNSItem->p_rtindex;

		/*
		 * relkind is set to composite to signal that we're not dealing with
		 * an actual relation, and no permission checks are required on it.
		 * (We'll check the actual target relation, instead.)
		 */
		exclRte->relkind = RELKIND_COMPOSITE_TYPE;
		exclRte->requiredPerms = 0;
		/* other permissions fields in exclRte are already empty */

		/* Create EXCLUDED rel's targetlist for use by EXPLAIN */
		exclRelTlist = BuildOnConflictExcludedTargetlist(targetrel,
														 exclRelIndex);
	}

	/* Process the arbiter clause, ON CONFLICT ON (...) */
	transformOnConflictArbiter(pstate, onConflictClause, &arbiterElems,
							   &arbiterWhere, &arbiterConstraint);

	/* Process DO UPDATE */
	if (onConflictClause->action == ONCONFLICT_UPDATE)
	{
		/*
		 * Expressions in the UPDATE targetlist need to be handled like UPDATE
		 * not INSERT.  We don't need to save/restore this because all INSERT
		 * expressions have been parsed already.
		 */
		pstate->p_is_insert = false;

		/*
		 * Add the EXCLUDED pseudo relation to the query namespace, making it
		 * available in the UPDATE subexpressions.
		 */
		addNSItemToQuery(pstate, exclNSItem, false, true, true);

		/*
		 * Now transform the UPDATE subexpressions.
		 */
		onConflictSet =
			transformUpdateTargetList(pstate, onConflictClause->targetList);

		onConflictWhere = transformWhereClause(pstate,
											   onConflictClause->whereClause,
											   EXPR_KIND_WHERE, "WHERE");

		/*
		 * Remove the EXCLUDED pseudo relation from the query namespace, since
		 * it's not supposed to be available in RETURNING.  (Maybe someday we
		 * could allow that, and drop this step.)
		 */
		Assert((ParseNamespaceItem *) llast(pstate->p_namespace) == exclNSItem);
		pstate->p_namespace = list_delete_last(pstate->p_namespace);
	}

	/* Finally, build ON CONFLICT DO [NOTHING | UPDATE] expression */
	result = makeNode(OnConflictExpr);

	result->action = onConflictClause->action;
	result->arbiterElems = arbiterElems;
	result->arbiterWhere = arbiterWhere;
	result->constraint = arbiterConstraint;
	result->onConflictSet = onConflictSet;
	result->onConflictWhere = onConflictWhere;
	result->exclRelIndex = exclRelIndex;
	result->exclRelTlist = exclRelTlist;

	return result;
}

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
/*
 * transformTableAndSchemaInCols
 * Handle table columns with schema and table name.
 * eg:schema.tbl_name.column or tbl_name.column
 */
static void
transformTableAndSchemaInCols(ParseState *pstate, List **cols)
{
	ListCell	   *tl;
	char		   *relname;
	foreach(tl, *cols)
	{
		ResTarget  *res = (ResTarget *) lfirst(tl);
		relname = NULL;
		if (list_length(res->indirection) == 2)
		{
			Oid		namespaceId = InvalidOid;
			Oid		relid = InvalidOid;

			Node   *node = linitial(res->indirection);
			if (IsA(node, String))
			{
				relname = strVal(node);
				namespaceId = LookupNamespaceNoError(res->name);
				if (OidIsValid(namespaceId))
				{
					relid = get_relname_relid(relname, namespaceId);
					if (OidIsValid(relid))
					{
						/* replace res->name ,delete res->indirection. */
						res->name = strVal(llast(res->indirection));
						res->indirection = NIL;
					}
				}
			}
		}
		else if(list_length(res->indirection) == 1)
		{
			Node   *node = linitial(res->indirection);
			if (IsA(node, String))
			{
				int		attrno;
				/* Lookup column name. */
				attrno = attnameAttNum(pstate->p_target_relation, strVal(node), false);
				if (attrno != InvalidAttrNumber)
				{
					/* replace res->name ,delete res->indirection. */
					res->name = strVal(node);
					res->indirection = NIL;
				}
			}
		}
	}
	return;
}

/*
 * transformSchemaInTargetlist
 * Handle target columns with schema.
 */
static void
transformSchemaInTargetlist(List **targetlist)
{
	ListCell   *tl;
	char	   *relname;
	foreach(tl, *targetlist)
	{
		ResTarget  *res = (ResTarget *) lfirst(tl);
		relname = NULL;
		if (list_length(res->indirection) == 2)
		{
			Oid		namespaceId = InvalidOid;
			Oid		relid = InvalidOid;

			Node   *node = linitial(res->indirection);
			if (IsA(node, String))
			{
				relname = strVal(node);
				namespaceId = LookupNamespaceNoError(res->name);
				if (OidIsValid(namespaceId))
				{
					relid = get_relname_relid(relname, namespaceId);
					if (OidIsValid(relid))
					{
						/* replace res->name ,delete first cell in res->indirection. */
						res->name = relname;
						res->indirection = list_delete_first(res->indirection);
					}
				}
			}
		}
	}
	return;
}

/*
 * transformUpsertClause
 * transforms upsert statement.
 */
static UpsertExpr *
transformUpsertClause(ParseState *pstate, UpsertClause *upsertClause, List *resultRelations)
{
	ParseNamespaceItem *exclNSItem = NULL;
	UpsertExpr		   *result = NULL;
	List			   *updateTlist = NIL;
	RangeTblEntry	   *exclRte = NULL;
	int					exclRelIndex = 0;
	List			   *exclRelTlist = NIL;
	UpsertAction		action = UPSERT_NOTHING;
	Relation			targetrel = pstate->p_target_relation;

	/* Foreign table is not support duplicate key update statement */
	if (get_rel_relkind(targetrel->rd_id) == 'f')
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Foreign table is not yet supported whithin INSERT ON DUPLICATE KEY UPDATE statement.")));

	if (upsertClause->targetList != NIL)
	{
		pstate->p_is_insert = false;
		action = UPSERT_UPDATE;
		exclNSItem = addRangeTableEntryForRelation(pstate,
												   targetrel,
												   RowExclusiveLock,
												   makeAlias("excluded", NIL),
												   false, false);
		exclRte = exclNSItem->p_rte;
		exclRelIndex = exclNSItem->p_rtindex;

		/*
		 * Build a targetlist for the EXCLUDED pseudo relation. Out of
		 * simplicity we do that here, because expandRelAttrs() happens to
		 * nearly do the right thing; specifically it also works with views.
		 * It'd be more proper to instead scan some pseudo scan node, but it
		 * doesn't seem worth the amount of code required.
		 *
		 * The only caveat of this hack is that the permissions expandRelAttrs
		 * adds have to be reset. markVarForSelectPriv() will add the exact
		 * required permissions back.
		 */

		exclRelTlist = BuildOnConflictExcludedTargetlist(targetrel, exclRelIndex);
		exclRte->requiredPerms = 0;
		exclRte->selectedCols = NULL;

		/*
		 * Add EXCLUDED and the target RTE to the namespace, so that they can
		 * be used in the UPDATE statement.
		 */
		addNSItemToQuery(pstate, exclNSItem, false, true, true);
//		addNSItemToQuery(pstate, (RangeTblEntry*)linitial(pstate->p_target_rangetblentry), false, true, true);

		transformSchemaInTargetlist(&(upsertClause->targetList));
//		updateTlist = transformTargetList(pstate, upsertClause->targetList, EXPR_KIND_UPDATE_TARGET);
		/* Done with select-like processing, move on transforming to match update set target column */
		updateTlist = transformUpdateTargetList(pstate, upsertClause->targetList);
	}

	/* Finally, build DUPLICATE KEY UPDATE expression */
	result = makeNode(UpsertExpr);
	result->updateTlist = updateTlist;
	result->exclRelIndex = exclRelIndex;
	result->exclRelTlist = exclRelTlist;
	result->upsertAction = action;
	return result;
}
/* End - ReqID:SRS-MYSQL-SQL-INSERT */


/*
 * BuildOnConflictExcludedTargetlist
 *		Create target list for the EXCLUDED pseudo-relation of ON CONFLICT,
 *		representing the columns of targetrel with varno exclRelIndex.
 *
 * Note: Exported for use in the rewriter.
 */
List *
BuildOnConflictExcludedTargetlist(Relation targetrel,
								  Index exclRelIndex)
{
	List	   *result = NIL;
	int			attno;
	Var		   *var;
	TargetEntry *te;

	/*
	 * Note that resnos of the tlist must correspond to attnos of the
	 * underlying relation, hence we need entries for dropped columns too.
	 */
	for (attno = 0; attno < RelationGetNumberOfAttributes(targetrel); attno++)
	{
		Form_pg_attribute attr = TupleDescAttr(targetrel->rd_att, attno);
		char	   *name;

		if (attr->attisdropped)
		{
			/*
			 * can't use atttypid here, but it doesn't really matter what type
			 * the Const claims to be.
			 */
			var = (Var *) makeNullConst(INT4OID, -1, InvalidOid);
			name = NULL;
		}
		else
		{
			var = makeVar(exclRelIndex, attno + 1,
						  attr->atttypid, attr->atttypmod,
						  attr->attcollation,
						  0);
			name = pstrdup(NameStr(attr->attname));
		}

		te = makeTargetEntry((Expr *) var,
							 attno + 1,
							 name,
							 false);

		result = lappend(result, te);
	}

	/*
	 * Add a whole-row-Var entry to support references to "EXCLUDED.*".  Like
	 * the other entries in the EXCLUDED tlist, its resno must match the Var's
	 * varattno, else the wrong things happen while resolving references in
	 * setrefs.c.  This is against normal conventions for targetlists, but
	 * it's okay since we don't use this as a real tlist.
	 */
	var = makeVar(exclRelIndex, InvalidAttrNumber,
				  targetrel->rd_rel->reltype,
				  -1, InvalidOid, 0);
	te = makeTargetEntry((Expr *) var, InvalidAttrNumber, NULL, true);
	result = lappend(result, te);

	return result;
}


/*
 * count_rowexpr_columns -
 *	  get number of columns contained in a ROW() expression;
 *	  return -1 if expression isn't a RowExpr or a Var referencing one.
 *
 * This is currently used only for hint purposes, so we aren't terribly
 * tense about recognizing all possible cases.  The Var case is interesting
 * because that's what we'll get in the INSERT ... SELECT (...) case.
 */
static int
count_rowexpr_columns(ParseState *pstate, Node *expr)
{
	if (expr == NULL)
		return -1;
	if (IsA(expr, RowExpr))
		return list_length(((RowExpr *) expr)->args);
	if (IsA(expr, Var))
	{
		Var		   *var = (Var *) expr;
		AttrNumber	attnum = var->varattno;

		if (attnum > 0 && var->vartype == RECORDOID)
		{
			RangeTblEntry *rte;

			rte = GetRTEByRangeTablePosn(pstate, var->varno, var->varlevelsup);
			if (rte->rtekind == RTE_SUBQUERY)
			{
				/* Subselect-in-FROM: examine sub-select's output expr */
				TargetEntry *ste = get_tle_by_resno(rte->subquery->targetList,
													attnum);

				if (ste == NULL || ste->resjunk)
					return -1;
				expr = (Node *) ste->expr;
				if (IsA(expr, RowExpr))
					return list_length(((RowExpr *) expr)->args);
			}
		}
	}
	return -1;
}

/** 
 * Begin - BUG#M4610
 * When call aggregate function without group by clause, 
 * order by clause may be meanless.
 */
static void
aggregates_without_group_by_check(ParseState *pstate, Query *qry)
{
	assert(pstate->p_hasAggs && qry->groupClause == NULL && qry->sortClause != NULL);

	if (aggregates_without_group_by)
	{
		ListCell   *l;
		List	   *sortClause_copy = copyObject(qry->sortClause);

		foreach(l, qry->sortClause)
		{
			SortGroupClause *sortcl = (SortGroupClause *) lfirst(l);
			TargetEntry *tle = get_sortgroupclause_tle(sortcl, qry->targetList);

			/**
			 * If the TargetEntry add by order by clause, the resjunk is true.
			 */
			if (tle->resjunk)
			{
				qry->targetList = list_delete(qry->targetList, tle);
				sortClause_copy = list_delete(sortClause_copy, sortcl);
			}
		}

		qry->sortClause = sortClause_copy;
	}
}
/* End - BUG#M4610 */

/*
 * transformSelectStmt -
 *	  transforms a Select Statement
 *
 * Note: this covers only cases with no set operations and no VALUES lists;
 * see below for the other cases.
 */
static Query *
transformSelectStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;
	ListCell   *l;

	qry->commandType = CMD_SELECT;

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
	/*
	 * transform connect by to with
	 * clause
	 */
	if(stmt->connect_by)
	{
		IgnoreStr  *ignoreStr = NULL;

		/* Begin - BUG#Z00004710 */
		/*
		 * if has group by, then order SIBLINGS by is not allowed here
		 */
		if (stmt->groupClause != NIL && stmt->sortClause != NIL)
		{
			SortBy *srby = (SortBy *) linitial(stmt->sortClause);

			if (srby->siblings)
				elog(ERROR, "ORDER SIBLINGS BY clause not allowed here");
		}
		/* End - BUG#Z00004710 */

		stmt = transformHierarQueryClause(pstate, stmt,
					&ignoreStr, pstate->p_sourcetext);
		pstate->p_connectby_names = ignoreStr;
	}
	else if (stmt->sortClause != NIL)
	{
		SortBy *srby = (SortBy *) linitial(stmt->sortClause);

		if (srby->siblings)
			elog(ERROR, "ORDER SIBLINGS BY clause not allowed here");
	}
	/* End - ReqID:SRS-SQL-HierarchicalQueries */

	/* Complain if we get called from someplace where INTO is not allowed */
	if (stmt->intoClause)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("SELECT ... INTO is not allowed here"),
				 parser_errposition(pstate,
									exprLocation((Node *) stmt->intoClause))));

	/* make FOR UPDATE/FOR SHARE info available to addRangeTableEntry */
	pstate->p_locking_clause = stmt->lockingClause;

	/* make WINDOW info available for window functions, too */
	pstate->p_windowdefs = stmt->windowClause;

	/* process the FROM clause */
	transformFromClause(pstate, stmt->fromClause);

	/* Begin - SRS-SQL-MULTITABLEINSERT */
	/*
	 * Here, when using the `with` clause to rewrite
	 * `insert all/first` statements, we require that sequences
	 * are allowed in the target list, but not elsewhere.
	 * This restriction is based on Oracle's behavior,
	 * but it might be possible to remove this limitation.
	 */
	if (stmt->multiInsert)
	{
		Assert(pstate->p_stmt_kind == PARSE_KIND_MULTITABLE_INSERT);

		pstate->p_stmt_kind = PARSE_KIND_NONE;
	}
	/* End - SRS-SQL-MULTITABLEINSERT */

	/* transform targetlist */
	qry->targetList = transformTargetList(pstate, stmt->targetList,
										  EXPR_KIND_SELECT_TARGET);

	/* Begin - SRS-SQL-MULTITABLEINSERT */
	if (stmt->multiInsert)
	{
		pstate->p_stmt_kind = PARSE_KIND_MULTITABLE_INSERT;
	}
	/* End - SRS-SQL-MULTITABLEINSERT */

	/* Begin - BUG#Z6106
	 * Keep the original target before data masking,
	 * as it will be used for later judgment.
	 */
	if (hg_redaction_data)
		qry->oriredactiontargetList = list_copy_deep(qry->targetList);
	/* End - BUG#Z6106 */
	/* Begin - ReqID:SRS-SECURITY-DATADESENSITIZATION
	 * To emsure that the sorting behavior of group by
	 * and order by remains the same and after data
	 * masking.
	 */
	if (redact_parse_analyze_hook)
		redact_parse_analyze_hook(pstate, qry);
	/* End - ReqID:SRS-SECURITY-DATADESENSITIZATION */

	/* mark column origins */
	markTargetListOrigins(pstate, qry->targetList);
	/* Begin - ReqID:SRS-SECURITY-AUDIT */
	if (audit_privi_mode_hook)
		stmt->whereClause = (*audit_privi_mode_hook)(pstate->p_sourcetext, stmt->whereClause, true);
	/* End - ReqID:SRS-SECURITY-AUDIT */

	/* Begin: Req:SRS-MYSQL-SQL-Select */
	if (MYS_MODE == compatible_mode)
	{
		pstate->p_targetlist = &qry->targetList;
		pstate->p_group_clauses = stmt->groupClause;
	}
	/* End: Req:SRS-MYSQL-SQL-Select */

	/* transform WHERE */
	qual = transformWhereClause(pstate, stmt->whereClause,
								EXPR_KIND_WHERE, "WHERE");

	/* initial processing of HAVING clause is much like WHERE clause */
	qry->havingQual = transformWhereClause(pstate, stmt->havingClause,
										   EXPR_KIND_HAVING, "HAVING");

	/* Begin - ReqID:SRS-MYSQL-SELECT */
	if (MYS_MODE == compatible_mode)
	{
		/*
		 * Compatible with the HAVING clause in MYSQL without the GROUP BY clause and
		 * the use of aggregate funcitons
		 */
		if (!pstate->p_hasAggs && !stmt->groupClause && stmt->havingClause)
		{
			if (qual == NULL)
				qual = qry->havingQual;
			else
				qual = (Node *)makeAndExpr(qual, qry->havingQual, -1);

			qry->havingQual = NULL;
		}

		/*
		 * Compatible with the sorting function in the GROUP BY clause in MYSQL
		 */
		if (stmt->groupClause)
		{
			if (!stmt->sortClause)
				stmt->sortClause = transformSorbyfromGroupclause(&stmt->groupClause);
			else
				transformSorbyfromGroupclause(&stmt->groupClause);
		}
	}
	/* End - ReqID:SRS-MYSQL-SELECT */

	/*
	 * Transform sorting/grouping stuff.  Do ORDER BY first because both
	 * transformGroupClause and transformDistinctClause need the results. Note
	 * that these functions can also change the targetList, so it's passed to
	 * them by reference.
	 */
	qry->sortClause = transformSortClause(pstate,
										  stmt->sortClause,
										  &qry->targetList,
										  EXPR_KIND_ORDER_BY,
										  false /* allow SQL92 rules */ );

	qry->groupClause = transformGroupClause(pstate,
											stmt->groupClause,
											&qry->groupingSets,
											&qry->targetList,
											qry->sortClause,
											stmt->groupDistinct ? EXPR_KIND_GROUP_BY_DISTINCT : EXPR_KIND_GROUP_BY, /* SRS-SQL-NLSSORT */
											false /* allow SQL92 rules */ );
	qry->groupDistinct = stmt->groupDistinct;

	/* Begin - BUG#M4610 */
	if (pstate->p_hasAggs && qry->groupClause == NULL && qry->sortClause != NULL)
	{
		aggregates_without_group_by_check(pstate, qry);
	}
	/* End - BUG#M4610 */

	if (stmt->distinctClause == NIL)
	{
		qry->distinctClause = NIL;
		qry->hasDistinctOn = false;
	}
	else if (linitial(stmt->distinctClause) == NULL)
	{
		/* We had SELECT DISTINCT */
		qry->distinctClause = transformDistinctClause(pstate,
													  &qry->targetList,
													  qry->sortClause,
													  false);
		qry->hasDistinctOn = false;
	}
	else
	{
		/* We had SELECT DISTINCT ON */
		qry->distinctClause = transformDistinctOnClause(pstate,
														stmt->distinctClause,
														&qry->targetList,
														qry->sortClause);
		qry->hasDistinctOn = true;
	}

	/**
	 * Begin - Req: SRS-SQL-NLSSORT
	 * Replace pseudo-collation in subquery with DISTINCT clause.
	 */
	foreach(l, qry->distinctClause)
	{
		SortGroupClause *sgc = (SortGroupClause *) lfirst(l);
		TargetEntry 	*tle = get_sortgroupclause_tle(sgc, qry->targetList);
		Node	   		*expr = (Node *) tle->expr;
		Oid			 	 coll = exprCollation(expr);

		if (IsPseudoCollation(coll))
		{
			tle->expr = (Expr *) makePseudoCollationReplaceNode(expr, 
																PseudoCollationReplace_BNB(coll));
		}
	}
	/* End - Req: SRS-SQL-NLSSORT */

	/* transform LIMIT */
	qry->limitOffset = transformLimitClause(pstate, stmt->limitOffset,
											EXPR_KIND_OFFSET, "OFFSET",
											stmt->limitOption);
	qry->limitCount = transformLimitClause(pstate, stmt->limitCount,
										   EXPR_KIND_LIMIT, "LIMIT",
										   stmt->limitOption);
	qry->limitOption = stmt->limitOption;

	/* transform window clauses after we have seen all window functions */
	qry->windowClause = transformWindowDefinitions(pstate,
												   pstate->p_windowdefs,
												   &qry->targetList);

	/* resolve any still-unresolved output columns as being type text */
	if (pstate->p_resolve_unknowns)
		resolveTargetListUnknowns(pstate, qry->targetList);

	/* Begin - ReqID:SRS-SQL-ROWNUM */
	if (pstate->p_hasRownum)
	{
		qry->hasRownum = pstate->p_hasRownum;
		addRangeTableEntryForCount(pstate);
	}
	/* End - ReqID:SRS-SQL-ROWNUM */

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasWindowFuncs = pstate->p_hasWindowFuncs;
	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasAggs = pstate->p_hasAggs;

	foreach(l, stmt->lockingClause)
	{
		transformLockingClause(pstate, qry,
							   (LockingClause *) lfirst(l), false);
	}

	assign_query_collations(pstate, qry);

	/* this must be done after collations, for reliable comparison of exprs */
	if (pstate->p_hasAggs || qry->groupClause || qry->groupingSets || qry->havingQual)
		parseCheckAggregates(pstate, qry);

	/* Begin - OA-REQ-2252 */
	if (ORA_MODE == compatible_mode)
		qry = ora_rewrite_nested_agg_two_stage(pstate, qry);
	/* End - OA-REQ-2252 */

	return qry;
}

/* Begin - OA-REQ-2252 */
/*
 * ora_has_nested_agg_marker
 *
 *	Check whether an expression tree contains a marked nested Aggref.
 */
static bool
ora_has_nested_agg_marker(Node *node, void *context)
{
	bool	   *found = (bool *) context;

	if (node == NULL)
		return false;

	if (IsA(node, Aggref))
	{
		Aggref	   *agg = (Aggref *) node;

		if (agg->aggtransno == ORA_NESTED_AGG_MARK)
		{
			*found = true;
			return true;
		}
	}

	return expression_tree_walker(node, ora_has_nested_agg_marker, context);
}

/*
 * ora_clear_nested_agg_marker
 *
 *	Clear temporary Oracle nested-aggregate markers from Aggres nodes.
 */
static bool
ora_clear_nested_agg_marker(Node *node, void *context)
{
	(void) context;

	if (node == NULL)
		return false;

	if (IsA(node, Aggref))
	{
		Aggref	   *agg = (Aggref *) node;

		if (agg->aggtransno == ORA_NESTED_AGG_MARK)
		{
			agg->aggtransno = -1;
			agg->aggno = -1;
			agg->aggtranstype = InvalidOid;
		}
	}

	return expression_tree_walker(node, ora_clear_nested_agg_marker, context);
}

/*
 * ora_find_marked_agg
 *
 *	Locate the first Aggref carrying an Oracle nested-aggregate marker.
 */
static bool
ora_find_marked_agg(Node *node, void *context)
{
	Node	  **found = (Node **) context;

	if (node == NULL || *found != NULL)
		return false;

	if (IsA(node, Aggref))
	{
		Aggref	   *agg = (Aggref *) node;

		if (agg->aggtransno == ORA_NESTED_AGG_MARK)
		{
			*found = node;
			return true;
		}
	}

	return expression_tree_walker(node, ora_find_marked_agg, context);
}

/*
 * ora_build_nested_agg_outer_expr
 *
 *	Build the outer-stage aggregate expression for a two-stage rewrite.
 *
 *	Every Aggref in the expression is rebuilt to aggregate over input_var
 *	from the derived-table output. For marker-carrying Aggrefs, restore
 *	the outer aggregate identity saved during parse_func.c processing.
 */
static Node *
ora_build_nested_agg_outer_expr(Node *node, void *context)
{
	OraNestedAggExprCtx *ctx = (OraNestedAggExprCtx *) context;

	if (node == NULL)
		return NULL;

	if (IsA(node, Aggref))
	{
		Aggref	   *agg = (Aggref *) node;
		Aggref	   *newagg = makeNode(Aggref);
		TargetEntry *tle;
		Oid			outer_fnoid = agg->aggfnoid;
		Oid			input_type = exprType((Node *) ctx->input_var);
		Oid			outer_rettype = agg->aggtype;
		Oid			outer_arg_type = input_type;
		Node	   *outer_arg;

		ctx->saw_agg = true;
		if (agg->aggtransno == ORA_NESTED_AGG_MARK)
		{
			outer_fnoid = (Oid) agg->aggno;
			outer_rettype = agg->aggtranstype;
			outer_arg_type = input_type;
		}

		newagg->aggfnoid = outer_fnoid;
		newagg->aggtype = outer_rettype;
		newagg->aggcollid = agg->aggcollid;
		newagg->inputcollid = agg->inputcollid;
		newagg->aggtranstype = InvalidOid;
		newagg->aggstar = false;
		newagg->aggvariadic = false;
		newagg->aggkind = agg->aggkind;
		newagg->agglevelsup = 0;
		newagg->aggsplit = AGGSPLIT_SIMPLE;
		newagg->aggno = -1;
		newagg->aggtransno = -1;
		newagg->aggdirectargs = NIL;
		newagg->aggorder = NIL;
		newagg->aggdistinct = NIL;
		newagg->aggfilter = NULL;
		newagg->has_outer_join = false;
		newagg->location = agg->location;

		outer_arg = (Node *) copyObject(ctx->input_var);

		tle = makeTargetEntry((Expr *) outer_arg, 1, NULL, false);
		newagg->args = list_make1(tle);
		newagg->aggargtypes = list_make1_oid(outer_arg_type);

		return (Node *) newagg;
	}

	return expression_tree_mutator(node,
								   ora_build_nested_agg_outer_expr,
								   (void *) ctx);
}

/*
 * ora_rewrite_nested_agg_two_stage
 *
 *	Rewrite Oracle-style nested aggregates into a two-stage query shape.
 *
 * The inner query computes grouped aggregate values, and the outer
 * query re-aggregates those values, emulating expressions such as
 * max(count(...)).
 */
static Query *
ora_rewrite_nested_agg_two_stage(ParseState *pstate, Query *inner_qry)
{
	ParseState *outer_pstate;
	ParseNamespaceItem *nsitem;
	Query	   *outer_qry;
	List	   *new_tlist = NIL;
	List	   *orig_tlexprs = NIL;
	ListCell   *lc;
	ListCell   *lc_orig;
	int			attno = 1;
	bool		has_marker_tlist = false;
	bool		has_marker_having = false;
	bool		has_grouping = false;

	if (inner_qry == NULL)
		return inner_qry;

	(void) ora_has_nested_agg_marker((Node *) inner_qry->targetList, &has_marker_tlist);
	(void) ora_has_nested_agg_marker((Node *) inner_qry->havingQual, &has_marker_having);

	if (has_marker_having)
	{
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->havingQual, NULL);
		ereport(ERROR,
				(errcode(ERRCODE_GROUPING_ERROR),
				 errmsg("aggregate function is nested too deeply")));
	}

	if (!has_marker_tlist)
	{
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->havingQual, NULL);

		return inner_qry;
	}

	has_grouping = (inner_qry->groupClause != NIL || inner_qry->groupingSets != NIL);
	if (!has_grouping)
	{
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);
		ereport(ERROR,
				(errcode(ERRCODE_GROUPING_ERROR),
				 errmsg("nested aggregate functions require GROUP BY clause")));
	}

	if (inner_qry->hasDistinctOn)
	{
		(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("nested aggregate rewrite does not support DISTINCT ON")));
	}

	outer_pstate = make_parsestate(pstate);
	outer_pstate->p_sourcetext = pstate->p_sourcetext;
	outer_pstate->p_queryEnv = pstate->p_queryEnv;

	foreach(lc, inner_qry->targetList)
	{
		TargetEntry *te = (TargetEntry *) lfirst(lc);
		Node	   *marked_agg = NULL;

		if (te->resjunk)
			continue;

		orig_tlexprs = lappend(orig_tlexprs, copyObject(te->expr));
		(void) ora_find_marked_agg((Node *) te->expr, &marked_agg);
		if (marked_agg != NULL)
			te->expr = (Expr *) copyObject(marked_agg);
	}

	nsitem = addRangeTableEntryForSubquery(outer_pstate,
										   inner_qry,
										   makeAlias("ora_nested_agg_s", NIL),
										   false,
										   true);
	addNSItemToQuery(outer_pstate, nsitem, true, true, true);

	forboth(lc, inner_qry->targetList, lc_orig, orig_tlexprs)
	{
		TargetEntry *te = (TargetEntry *) lfirst(lc);
		Node	   *orig_expr = (Node *) lfirst(lc_orig);
		OraNestedAggExprCtx exprctx;
		Var		   *input_var;
		Node	   *new_expr;
		TargetEntry *new_tle;

		if (te->resjunk)
			continue;

		input_var = makeVar(nsitem->p_rtindex,
							te->resno,
							exprType((Node *) te->expr),
							exprTypmod((Node *) te->expr),
							exprCollation((Node *) te->expr),
							0);
		exprctx.pstate = outer_pstate;
		exprctx.input_var = input_var;
		exprctx.saw_agg = false;

		new_expr = ora_build_nested_agg_outer_expr(orig_expr, &exprctx);
		if (!exprctx.saw_agg)
		{
			if (contain_vars_of_level(orig_expr, 0))
			{
				free_parsestate(outer_pstate);
				(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);
					ereport(ERROR,
							(errcode(ERRCODE_GROUPING_ERROR),
							 errmsg("columns are not allowed with nested aggregate rewrite")));

			}
			new_expr = copyObject(orig_expr);
		}

		new_tle = makeTargetEntry((Expr *) new_expr,
								  attno++,
								  pstrdup(te->resname),
								  false);
		new_tlist = lappend(new_tlist, new_tle);
	}

	(void) ora_clear_nested_agg_marker((Node *) inner_qry->targetList, NULL);

	outer_qry = makeNode(Query);
	outer_qry->commandType = CMD_SELECT;
	outer_qry->targetList = new_tlist;
	outer_qry->rtable = outer_pstate->p_rtable;
	outer_qry->jointree = makeFromExpr(outer_pstate->p_joinlist, NULL);
	outer_qry->hasAggs = true;
	outer_qry->hasSubLinks = inner_qry->hasSubLinks;
	free_parsestate(outer_pstate);

	return outer_qry;
}
/* End - OA-REQ-2252 */

/*
 * transformValuesClause -
 *	  transforms a VALUES clause that's being used as a standalone SELECT
 *
 * We build a Query containing a VALUES RTE, rather as if one had written
 *			SELECT * FROM (VALUES ...) AS "*VALUES*"
 */
static Query *
transformValuesClause(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	List	   *exprsLists = NIL;
	List	   *coltypes = NIL;
	List	   *coltypmods = NIL;
	List	   *colcollations = NIL;
	List	  **colexprs = NULL;
	int			sublist_length = -1;
	bool		lateral = false;
	ParseNamespaceItem *nsitem;
	ListCell   *lc;
	ListCell   *lc2;
	int			i;

	qry->commandType = CMD_SELECT;

	/* Most SELECT stuff doesn't apply in a VALUES clause */
	Assert(stmt->distinctClause == NIL);
	Assert(stmt->intoClause == NULL);
	Assert(stmt->targetList == NIL);
	Assert(stmt->fromClause == NIL);
	Assert(stmt->whereClause == NULL);
	Assert(stmt->groupClause == NIL);
	Assert(stmt->havingClause == NULL);
	Assert(stmt->windowClause == NIL);
	Assert(stmt->op == SETOP_NONE);

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	/*
	 * For each row of VALUES, transform the raw expressions.
	 *
	 * Note that the intermediate representation we build is column-organized
	 * not row-organized.  That simplifies the type and collation processing
	 * below.
	 */
	foreach(lc, stmt->valuesLists)
	{
		List	   *sublist = (List *) lfirst(lc);

		/*
		 * Do basic expression transformation (same as a ROW() expr, but here
		 * we disallow SetToDefault)
		 */
		sublist = transformExpressionList(pstate, sublist,
										  EXPR_KIND_VALUES, false);

		/*
		 * All the sublists must be the same length, *after* transformation
		 * (which might expand '*' into multiple items).  The VALUES RTE can't
		 * handle anything different.
		 */
		if (sublist_length < 0)
		{
			/* Remember post-transformation length of first sublist */
			sublist_length = list_length(sublist);
			/* and allocate array for per-column lists */
			colexprs = (List **) palloc0(sublist_length * sizeof(List *));
		}
		else if (sublist_length != list_length(sublist))
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("VALUES lists must all be the same length"),
					 parser_errposition(pstate,
										exprLocation((Node *) sublist))));
		}

		/* Build per-column expression lists */
		i = 0;
		foreach(lc2, sublist)
		{
			Node	   *col = (Node *) lfirst(lc2);

			colexprs[i] = lappend(colexprs[i], col);
			i++;
		}

		/* Release sub-list's cells to save memory */
		list_free(sublist);

		/* Prepare an exprsLists element for this row */
		exprsLists = lappend(exprsLists, NIL);
	}

	/*
	 * Now resolve the common types of the columns, and coerce everything to
	 * those types.  Then identify the common typmod and common collation, if
	 * any, of each column.
	 *
	 * We must do collation processing now because (1) assign_query_collations
	 * doesn't process rangetable entries, and (2) we need to label the VALUES
	 * RTE with column collations for use in the outer query.  We don't
	 * consider conflict of implicit collations to be an error here; instead
	 * the column will just show InvalidOid as its collation, and you'll get a
	 * failure later if that results in failure to resolve a collation.
	 *
	 * Note we modify the per-column expression lists in-place.
	 */
	for (i = 0; i < sublist_length; i++)
	{
		Oid			coltype;
		int32		coltypmod;
		Oid			colcoll;

		coltype = select_common_type(pstate, colexprs[i], "VALUES", NULL);

		foreach(lc, colexprs[i])
		{
			Node	   *col = (Node *) lfirst(lc);

			col = coerce_to_common_type(pstate, col, coltype, "VALUES");
			lfirst(lc) = (void *) col;
		}

		coltypmod = select_common_typmod(pstate, colexprs[i], coltype);
		colcoll = select_common_collation(pstate, colexprs[i], true);

		coltypes = lappend_oid(coltypes, coltype);
		coltypmods = lappend_int(coltypmods, coltypmod);
		colcollations = lappend_oid(colcollations, colcoll);
	}

	/*
	 * Finally, rearrange the coerced expressions into row-organized lists.
	 */
	for (i = 0; i < sublist_length; i++)
	{
		forboth(lc, colexprs[i], lc2, exprsLists)
		{
			Node	   *col = (Node *) lfirst(lc);
			List	   *sublist = lfirst(lc2);

			sublist = lappend(sublist, col);
			lfirst(lc2) = sublist;
		}
		list_free(colexprs[i]);
	}

	/*
	 * Ordinarily there can't be any current-level Vars in the expression
	 * lists, because the namespace was empty ... but if we're inside CREATE
	 * RULE, then NEW/OLD references might appear.  In that case we have to
	 * mark the VALUES RTE as LATERAL.
	 */
	if (pstate->p_rtable != NIL &&
		contain_vars_of_level((Node *) exprsLists, 0))
		lateral = true;

	/*
	 * Generate the VALUES RTE
	 */
	nsitem = addRangeTableEntryForValues(pstate, exprsLists,
										 coltypes, coltypmods, colcollations,
										 NULL, lateral, true);
	addNSItemToQuery(pstate, nsitem, true, true, true);

	/*
	 * Generate a targetlist as though expanding "*"
	 */
	Assert(pstate->p_next_resno == 1);
	qry->targetList = expandNSItemAttrs(pstate, nsitem, 0, true, -1);

	/*
	 * The grammar allows attaching ORDER BY, LIMIT, and FOR UPDATE to a
	 * VALUES, so cope.
	 */
	qry->sortClause = transformSortClause(pstate,
										  stmt->sortClause,
										  &qry->targetList,
										  EXPR_KIND_ORDER_BY,
										  false /* allow SQL92 rules */ );

	qry->limitOffset = transformLimitClause(pstate, stmt->limitOffset,
											EXPR_KIND_OFFSET, "OFFSET",
											stmt->limitOption);
	qry->limitCount = transformLimitClause(pstate, stmt->limitCount,
										   EXPR_KIND_LIMIT, "LIMIT",
										   stmt->limitOption);
	qry->limitOption = stmt->limitOption;

	if (stmt->lockingClause)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s cannot be applied to VALUES",
						LCS_asString(((LockingClause *)
									  linitial(stmt->lockingClause))->strength))));

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;

	assign_query_collations(pstate, qry);

	return qry;
}

/*
 * transformSetOperationStmt -
 *	  transforms a set-operations tree
 *
 * A set-operation tree is just a SELECT, but with UNION/INTERSECT/EXCEPT
 * structure to it.  We must transform each leaf SELECT and build up a top-
 * level Query that contains the leaf SELECTs as subqueries in its rangetable.
 * The tree of set operations is converted into the setOperations field of
 * the top-level Query.
 */
static Query *
transformSetOperationStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	SelectStmt *leftmostSelect;
	int			leftmostRTI;
	Query	   *leftmostQuery;
	SetOperationStmt *sostmt;
	List	   *sortClause;
	Node	   *limitOffset;
	Node	   *limitCount;
	List	   *lockingClause;
	WithClause *withClause;
	Node	   *node;
	ListCell   *left_tlist,
			   *lct,
			   *lcm,
			   *lcc,
			   *l;
	List	   *targetvars,
			   *targetnames,
			   *sv_namespace;
	int			sv_rtable_length;
	ParseNamespaceItem *jnsitem;
	ParseNamespaceColumn *sortnscolumns;
	int			sortcolindex;
	int			tllen;

	qry->commandType = CMD_SELECT;

	/*
	 * Find leftmost leaf SelectStmt.  We currently only need to do this in
	 * order to deliver a suitable error message if there's an INTO clause
	 * there, implying the set-op tree is in a context that doesn't allow
	 * INTO.  (transformSetOperationTree would throw error anyway, but it
	 * seems worth the trouble to throw a different error for non-leftmost
	 * INTO, so we produce that error in transformSetOperationTree.)
	 */
	leftmostSelect = stmt->larg;
	while (leftmostSelect && leftmostSelect->op != SETOP_NONE)
		leftmostSelect = leftmostSelect->larg;
	Assert(leftmostSelect && IsA(leftmostSelect, SelectStmt) &&
		   leftmostSelect->larg == NULL);
	if (leftmostSelect->intoClause)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("SELECT ... INTO is not allowed here"),
				 parser_errposition(pstate,
									exprLocation((Node *) leftmostSelect->intoClause))));

	/*
	 * We need to extract ORDER BY and other top-level clauses here and not
	 * let transformSetOperationTree() see them --- else it'll just recurse
	 * right back here!
	 */
	sortClause = stmt->sortClause;
	limitOffset = stmt->limitOffset;
	limitCount = stmt->limitCount;
	lockingClause = stmt->lockingClause;
	withClause = stmt->withClause;

	stmt->sortClause = NIL;
	stmt->limitOffset = NULL;
	stmt->limitCount = NULL;
	stmt->lockingClause = NIL;
	stmt->withClause = NULL;

	/* We don't support FOR UPDATE/SHARE with set ops at the moment. */
	if (lockingClause)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with UNION/INTERSECT/EXCEPT",
						LCS_asString(((LockingClause *)
									  linitial(lockingClause))->strength))));

	/* Process the WITH clause independently of all else */
	if (withClause)
	{
		qry->hasRecursive = withClause->recursive;
		qry->cteList = transformWithClause(pstate, withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	/*
	 * Recursively transform the components of the tree.
	 */
	sostmt = castNode(SetOperationStmt,
					  transformSetOperationTree(pstate, stmt, true, NULL));
	Assert(sostmt);
	qry->setOperations = (Node *) sostmt;

	/*
	 * Re-find leftmost SELECT (now it's a sub-query in rangetable)
	 */
	node = sostmt->larg;
	while (node && IsA(node, SetOperationStmt))
		node = ((SetOperationStmt *) node)->larg;
	Assert(node && IsA(node, RangeTblRef));
	leftmostRTI = ((RangeTblRef *) node)->rtindex;
	leftmostQuery = rt_fetch(leftmostRTI, pstate->p_rtable)->subquery;
	Assert(leftmostQuery != NULL);

	/*
	 * Generate dummy targetlist for outer query using column names of
	 * leftmost select and common datatypes/collations of topmost set
	 * operation.  Also make lists of the dummy vars and their names for use
	 * in parsing ORDER BY.
	 *
	 * Note: we use leftmostRTI as the varno of the dummy variables. It
	 * shouldn't matter too much which RT index they have, as long as they
	 * have one that corresponds to a real RT entry; else funny things may
	 * happen when the tree is mashed by rule rewriting.
	 */
	qry->targetList = NIL;
	targetvars = NIL;
	targetnames = NIL;
	sortnscolumns = (ParseNamespaceColumn *)
		palloc0(list_length(sostmt->colTypes) * sizeof(ParseNamespaceColumn));
	sortcolindex = 0;

	forfour(lct, sostmt->colTypes,
			lcm, sostmt->colTypmods,
			lcc, sostmt->colCollations,
			left_tlist, leftmostQuery->targetList)
	{
		Oid			colType = lfirst_oid(lct);
		int32		colTypmod = lfirst_int(lcm);
		Oid			colCollation = lfirst_oid(lcc);
		TargetEntry *lefttle = (TargetEntry *) lfirst(left_tlist);
		char	   *colName;
		TargetEntry *tle;
		Var		   *var;

		Assert(!lefttle->resjunk);
		colName = pstrdup(lefttle->resname);
		var = makeVar(leftmostRTI,
					  lefttle->resno,
					  colType,
					  colTypmod,
					  colCollation,
					  0);
		var->location = exprLocation((Node *) lefttle->expr);
		tle = makeTargetEntry((Expr *) var,
							  (AttrNumber) pstate->p_next_resno++,
							  colName,
							  false);
		qry->targetList = lappend(qry->targetList, tle);
		targetvars = lappend(targetvars, var);
		targetnames = lappend(targetnames, makeString(colName));
		sortnscolumns[sortcolindex].p_varno = leftmostRTI;
		sortnscolumns[sortcolindex].p_varattno = lefttle->resno;
		sortnscolumns[sortcolindex].p_vartype = colType;
		sortnscolumns[sortcolindex].p_vartypmod = colTypmod;
		sortnscolumns[sortcolindex].p_varcollid = colCollation;
		sortnscolumns[sortcolindex].p_varnosyn = leftmostRTI;
		sortnscolumns[sortcolindex].p_varattnosyn = lefttle->resno;
		sortcolindex++;
	}

	/*
	 * As a first step towards supporting sort clauses that are expressions
	 * using the output columns, generate a namespace entry that makes the
	 * output columns visible.  A Join RTE node is handy for this, since we
	 * can easily control the Vars generated upon matches.
	 *
	 * Note: we don't yet do anything useful with such cases, but at least
	 * "ORDER BY upper(foo)" will draw the right error message rather than
	 * "foo not found".
	 */
	sv_rtable_length = list_length(pstate->p_rtable);

	jnsitem = addRangeTableEntryForJoin(pstate,
										targetnames,
										sortnscolumns,
										JOIN_INNER,
										0,
										targetvars,
										NIL,
										NIL,
										NULL,
										NULL,
										false);

	sv_namespace = pstate->p_namespace;
	pstate->p_namespace = NIL;

	/* add jnsitem to column namespace only */
	addNSItemToQuery(pstate, jnsitem, false, false, true);

	/*
	 * For now, we don't support resjunk sort clauses on the output of a
	 * setOperation tree --- you can only use the SQL92-spec options of
	 * selecting an output column by name or number.  Enforce by checking that
	 * transformSortClause doesn't add any items to tlist.
	 */
	tllen = list_length(qry->targetList);

	qry->sortClause = transformSortClause(pstate,
										  sortClause,
										  &qry->targetList,
										  EXPR_KIND_ORDER_BY, /* Req: SRS-SQL-NLSSORT */
										  false /* allow SQL92 rules */ );

	/* restore namespace, remove join RTE from rtable */
	pstate->p_namespace = sv_namespace;
	pstate->p_rtable = list_truncate(pstate->p_rtable, sv_rtable_length);

	if (tllen != list_length(qry->targetList))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("invalid UNION/INTERSECT/EXCEPT ORDER BY clause"),
				 errdetail("Only result column names can be used, not expressions or functions."),
				 errhint("Add the expression/function to every SELECT, or move the UNION into a FROM clause."),
				 parser_errposition(pstate,
									exprLocation(list_nth(qry->targetList, tllen)))));

	qry->limitOffset = transformLimitClause(pstate, limitOffset,
											EXPR_KIND_OFFSET, "OFFSET",
											stmt->limitOption);
	qry->limitCount = transformLimitClause(pstate, limitCount,
										   EXPR_KIND_LIMIT, "LIMIT",
										   stmt->limitOption);
	qry->limitOption = stmt->limitOption;

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasWindowFuncs = pstate->p_hasWindowFuncs;
	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasAggs = pstate->p_hasAggs;

	foreach(l, lockingClause)
	{
		transformLockingClause(pstate, qry,
							   (LockingClause *) lfirst(l), false);
	}

	assign_query_collations(pstate, qry);

	/* Begin - BUG#M4610 */
	if (pstate->p_hasAggs && qry->groupClause == NULL && qry->sortClause != NULL)
	{
		aggregates_without_group_by_check(pstate, qry);
	}
	/* End - BUG#M4610 */

	/* this must be done after collations, for reliable comparison of exprs */
	if (pstate->p_hasAggs || qry->groupClause || qry->groupingSets || qry->havingQual)
		parseCheckAggregates(pstate, qry);

	/**
	 * Begin - BUG#Z1461 
	 * For example,
	 * 	CREATE TABLE tbl AS SELECT val FROM tbl1 UNION SELECT val FROM tbl2.
	 * The collation of column `val` in `tbl1` & `tbl2` is USING_NLS_COMP.
	 * 
	 * According to the pseudo-collation replace rule, we will trans the statement to this:
	 * 	CREATE TABLE tbl AS SELECT val COLLATE BINARY FROM tbl1 UNION SELECT val COLLATE BINARY FROM tbl2.
	 * 
	 * However, this statement will create tbl.val with BINARY collation. In oracle, 
	 * The collation of tbl.val is USING_NLS_COMP.
	 * 
	 * So, we add following logic to handle this case.
	 * With this logic, we will trans raw statement to this:
	 *  CREATE TABLE tbl AS SELECT val COLLATE USING_NLS_COMP FROM (SELECT val COLLATE BINARY FROM tbl1 UNION SELECT val COLLATE BINARY FROM tbl2).
	 */
	if (ORA_MODE == compatible_mode && enable_ora_default_collation && 
		subquery_with_raw_collation)
	{
		ListCell *cc, *rcc;
		bool donePseudoCollationReplace = false;
		
		forboth(cc,  sostmt->colCollations,
				rcc, sostmt->rawcolCollations)
		{
			Oid	colCollation = lfirst_oid(cc);
			Oid	rawcolCollation = lfirst_oid(rcc);

			if (colCollation != rawcolCollation)
			{
				donePseudoCollationReplace = true;
				break;
			}
		}

		if (donePseudoCollationReplace)
		{
			char 			selectName[64];
			int				varattno = 0;
			Alias	   		*eref;
			List			*targetList = NIL;
			ListCell   		*tlistitem;
			Query 			*newquery = makeNode(Query);
			RangeTblEntry 	*rte = makeNode(RangeTblEntry);
			RangeTblRef 	*rtr = makeNode(RangeTblRef);

			rtr->rtindex = 1;

			forboth(tlistitem, qry->targetList,
					rcc, sostmt->rawcolCollations)
			{
				TargetEntry *newtle;
				TargetEntry *tle = (TargetEntry *) lfirst(tlistitem);
				
				Oid	rawcolCollation = lfirst_oid(rcc);

				if (OidIsValid(exprCollation((Node *) tle->expr)))
				{
					Node *expr = (Node *) tle->expr;

					if (IsA(expr, CollateExpr))
						exprSetCollation(expr, rawcolCollation);
					else
						expr = makePseudoCollationReplaceNode(expr, rawcolCollation);

					newtle = makeTargetEntry((Expr *) expr,
											 tle->resno,
											 tle->resname,
											 false);

					targetList = lappend(targetList, newtle);
				}
				else
					targetList = lappend(targetList, tle);
			}

			snprintf(selectName, sizeof(selectName), "*SELECT* PseudoCollationReplace %d", list_length(pstate->p_rtable) + 1);
			
			newquery->commandType = CMD_SELECT;
			newquery->querySource = qry->querySource;
			newquery->rtable = list_make1(rte);
			newquery->jointree = makeFromExpr(list_make1(rtr), NULL);
			
			rte->rtekind = RTE_SUBQUERY;
			rte->subquery = qry;
			rte->alias = makeAlias(selectName, NIL);

			eref = copyObject(rte->alias);

			foreach(tlistitem, qry->targetList)
			{
				char	   	*attrname;
				TargetEntry *te = (TargetEntry *) lfirst(tlistitem);
				
				if (te->resjunk)
					continue;
				
				varattno++;

				attrname = pstrdup(te->resname);
				eref->colnames = lappend(eref->colnames, makeString(attrname));
			}

			rte->eref = eref;

			newquery->targetList = targetList;
			newquery->rtable = list_make1(rte);
			newquery->jointree = makeFromExpr(list_make1(rtr), NULL);

			qry = newquery;
		}
	}
	/* End - BUG#Z1461 */

	return qry;
}

/*
 * Make a SortGroupClause node for a SetOperationStmt's groupClauses
 *
 * If require_hash is true, the caller is indicating that they need hash
 * support or they will fail.  So look extra hard for hash support.
 */
SortGroupClause *
makeSortGroupClauseForSetOp(Oid rescoltype, bool require_hash)
{
	SortGroupClause *grpcl = makeNode(SortGroupClause);
	Oid			sortop;
	Oid			eqop;
	bool		hashable;

	/* determine the eqop and optional sortop */
	get_sort_group_operators(rescoltype,
							 false, true, false,
							 &sortop, &eqop, NULL,
							 &hashable);

	/*
	 * The type cache doesn't believe that record is hashable (see
	 * cache_record_field_properties()), but if the caller really needs hash
	 * support, we can assume it does.  Worst case, if any components of the
	 * record don't support hashing, we will fail at execution.
	 */
	if (require_hash && (rescoltype == RECORDOID || rescoltype == RECORDARRAYOID))
		hashable = true;

	/* we don't have a tlist yet, so can't assign sortgrouprefs */
	grpcl->tleSortGroupRef = 0;
	grpcl->eqop = eqop;
	grpcl->sortop = sortop;
	grpcl->nulls_first = false; /* OK with or without sortop */
	grpcl->hashable = hashable;

	return grpcl;
}

/*
 * transformSetOperationTree
 *		Recursively transform leaves and internal nodes of a set-op tree
 *
 * In addition to returning the transformed node, if targetlist isn't NULL
 * then we return a list of its non-resjunk TargetEntry nodes.  For a leaf
 * set-op node these are the actual targetlist entries; otherwise they are
 * dummy entries created to carry the type, typmod, collation, and location
 * (for error messages) of each output column of the set-op node.  This info
 * is needed only during the internal recursion of this function, so outside
 * callers pass NULL for targetlist.  Note: the reason for passing the
 * actual targetlist entries of a leaf node is so that upper levels can
 * replace UNKNOWN Consts with properly-coerced constants.
 */
static Node *
transformSetOperationTree(ParseState *pstate, SelectStmt *stmt,
						  bool isTopLevel, List **targetlist)
{
	bool		isLeaf;

	Assert(stmt && IsA(stmt, SelectStmt));

	/* Guard against stack overflow due to overly complex set-expressions */
	check_stack_depth();

	/*
	 * Validity-check both leaf and internal SELECTs for disallowed ops.
	 */
	if (stmt->intoClause)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("INTO is only allowed on first SELECT of UNION/INTERSECT/EXCEPT"),
				 parser_errposition(pstate,
									exprLocation((Node *) stmt->intoClause))));

	/* We don't support FOR UPDATE/SHARE with set ops at the moment. */
	if (stmt->lockingClause)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with UNION/INTERSECT/EXCEPT",
						LCS_asString(((LockingClause *)
									  linitial(stmt->lockingClause))->strength))));

	/*
	 * If an internal node of a set-op tree has ORDER BY, LIMIT, FOR UPDATE,
	 * or WITH clauses attached, we need to treat it like a leaf node to
	 * generate an independent sub-Query tree.  Otherwise, it can be
	 * represented by a SetOperationStmt node underneath the parent Query.
	 */
	if (stmt->op == SETOP_NONE)
	{
		Assert(stmt->larg == NULL && stmt->rarg == NULL);
		isLeaf = true;
	}
	else
	{
		Assert(stmt->larg != NULL && stmt->rarg != NULL);
		if (stmt->sortClause || stmt->limitOffset || stmt->limitCount ||
			stmt->lockingClause || stmt->withClause)
			isLeaf = true;
		else
			isLeaf = false;
	}

	if (isLeaf)
	{
		/* Process leaf SELECT */
		Query	   *selectQuery;
		char		selectName[32];
		ParseNamespaceItem *nsitem;
		RangeTblRef *rtr;
		ListCell   *tl;

		/*
		 * Transform SelectStmt into a Query.
		 *
		 * This works the same as SELECT transformation normally would, except
		 * that we prevent resolving unknown-type outputs as TEXT.  This does
		 * not change the subquery's semantics since if the column type
		 * matters semantically, it would have been resolved to something else
		 * anyway.  Doing this lets us resolve such outputs using
		 * select_common_type(), below.
		 *
		 * Note: previously transformed sub-queries don't affect the parsing
		 * of this sub-query, because they are not in the toplevel pstate's
		 * namespace list.
		 */
		selectQuery = parse_sub_analyze((Node *) stmt, pstate,
										NULL, false, false);

		/*
		 * Check for bogus references to Vars on the current query level (but
		 * upper-level references are okay). Normally this can't happen
		 * because the namespace will be empty, but it could happen if we are
		 * inside a rule.
		 */
		if (pstate->p_namespace)
		{
			if (contain_vars_of_level((Node *) selectQuery, 1))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
						 errmsg("UNION/INTERSECT/EXCEPT member statement cannot refer to other relations of same query level"),
						 parser_errposition(pstate,
											locate_var_of_level((Node *) selectQuery, 1))));
		}

		/*
		 * Extract a list of the non-junk TLEs for upper-level processing.
		 */
		if (targetlist)
		{
			*targetlist = NIL;
			foreach(tl, selectQuery->targetList)
			{
				TargetEntry *tle = (TargetEntry *) lfirst(tl);

				if (!tle->resjunk)
					*targetlist = lappend(*targetlist, tle);
			}
		}

		/*
		 * Make the leaf query be a subquery in the top-level rangetable.
		 */
		snprintf(selectName, sizeof(selectName), "*SELECT* %d",
				 list_length(pstate->p_rtable) + 1);
		nsitem = addRangeTableEntryForSubquery(pstate,
											   selectQuery,
											   makeAlias(selectName, NIL),
											   false,
											   false);

		/*
		 * Return a RangeTblRef to replace the SelectStmt in the set-op tree.
		 */
		rtr = makeNode(RangeTblRef);
		rtr->rtindex = nsitem->p_rtindex;
		return (Node *) rtr;
	}
	else
	{
		/* Process an internal node (set operation node) */
		SetOperationStmt *op = makeNode(SetOperationStmt);
		List	   *ltargetlist;
		List	   *rtargetlist;
		ListCell   *ltl;
		ListCell   *rtl;
		const char *context;
		bool recursive = (pstate->p_parent_cte &&
						  pstate->p_parent_cte->cterecursive);

		context = (stmt->op == SETOP_UNION ? "UNION" :
				   (stmt->op == SETOP_INTERSECT ? "INTERSECT" :
					"EXCEPT"));

		op->op = stmt->op;
		op->all = stmt->all;

		/*
		 * Recursively transform the left child node.
		 */
		op->larg = transformSetOperationTree(pstate, stmt->larg,
											 false,
											 &ltargetlist);

		/*
		 * If we are processing a recursive union query, now is the time to
		 * examine the non-recursive term's output columns and mark the
		 * containing CTE as having those result columns.  We should do this
		 * only at the topmost setop of the CTE, of course.
		 */
		if (isTopLevel && recursive)
			determineRecursiveColTypes(pstate, op->larg, ltargetlist);

		/*
		 * Recursively transform the right child node.
		 */
		op->rarg = transformSetOperationTree(pstate, stmt->rarg,
											 false,
											 &rtargetlist);

		/*
		 * Verify that the two children have the same number of non-junk
		 * columns, and determine the types of the merged output columns.
		 */
		if (list_length(ltargetlist) != list_length(rtargetlist))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("each %s query must have the same number of columns",
							context),
					 parser_errposition(pstate,
										exprLocation((Node *) rtargetlist))));

		if (targetlist)
			*targetlist = NIL;
		op->colTypes = NIL;
		op->colTypmods = NIL;
		op->colCollations = NIL;
		op->groupClauses = NIL;
		forboth(ltl, ltargetlist, rtl, rtargetlist)
		{
			TargetEntry *ltle = (TargetEntry *) lfirst(ltl);
			TargetEntry *rtle = (TargetEntry *) lfirst(rtl);
			Node	   *lcolnode = (Node *) ltle->expr;
			Node	   *rcolnode = (Node *) rtle->expr;
			Oid			lcoltype = exprType(lcolnode);
			Oid			rcoltype = exprType(rcolnode);
			Node	   *bestexpr;
			int			bestlocation;
			Oid			rescoltype;
			int32		rescoltypmod;
			Oid			rescolcoll;
			Oid			rawcolcoll = InvalidOid; /* BUG#Z1461 */

			/* select common type, same as CASE et al */
			rescoltype = select_common_type(pstate,
											list_make2(lcolnode, rcolnode),
											context,
											&bestexpr);
			bestlocation = exprLocation(bestexpr);

			/*
			 * Verify the coercions are actually possible.  If not, we'd fail
			 * later anyway, but we want to fail now while we have sufficient
			 * context to produce an error cursor position.
			 *
			 * For all non-UNKNOWN-type cases, we verify coercibility but we
			 * don't modify the child's expression, for fear of changing the
			 * child query's semantics.
			 *
			 * If a child expression is an UNKNOWN-type Const or Param, we
			 * want to replace it with the coerced expression.  This can only
			 * happen when the child is a leaf set-op node.  It's safe to
			 * replace the expression because if the child query's semantics
			 * depended on the type of this output column, it'd have already
			 * coerced the UNKNOWN to something else.  We want to do this
			 * because (a) we want to verify that a Const is valid for the
			 * target type, or resolve the actual type of an UNKNOWN Param,
			 * and (b) we want to avoid unnecessary discrepancies between the
			 * output type of the child query and the resolved target type.
			 * Such a discrepancy would disable optimization in the planner.
			 *
			 * If it's some other UNKNOWN-type node, eg a Var, we do nothing
			 * (knowing that coerce_to_common_type would fail).  The planner
			 * is sometimes able to fold an UNKNOWN Var to a constant before
			 * it has to coerce the type, so failing now would just break
			 * cases that might work.
			 */
			if (lcoltype != UNKNOWNOID)
				lcolnode = coerce_to_common_type(pstate, lcolnode,
												 rescoltype, context);
			else if (IsA(lcolnode, Const) ||
					 IsA(lcolnode, Param))
			{
				lcolnode = coerce_to_common_type(pstate, lcolnode,
												 rescoltype, context);
				ltle->expr = (Expr *) lcolnode;
			}

			if (rcoltype != UNKNOWNOID)
				rcolnode = coerce_to_common_type(pstate, rcolnode,
												 rescoltype, context);
			else if (IsA(rcolnode, Const) ||
					 IsA(rcolnode, Param))
			{
				rcolnode = coerce_to_common_type(pstate, rcolnode,
												 rescoltype, context);
				rtle->expr = (Expr *) rcolnode;
			}

			/* Begin - BUG#Z1461 */
			if (ORA_MODE == compatible_mode && enable_ora_default_collation)
			{
				Node *lcopy = copyObject(lcolnode);
				Node *rcopy = copyObject(rcolnode);

				// will do assign_expr_collations
				rawcolcoll = select_common_collation(pstate,
												 	 list_make2(lcopy, rcopy),
													 (op->op == SETOP_UNION && op->all));

				if (IsPseudoCollation(exprCollation(lcopy)))
				{
					lcolnode = makePseudoCollationReplaceNode(lcolnode, PseudoCollationReplace_BNB(exprCollation(lcopy)));
					ltle->expr = (Expr *) lcolnode;
				}

				if (IsPseudoCollation(exprCollation(rcopy)))
				{
					rcolnode = makePseudoCollationReplaceNode(rcolnode, PseudoCollationReplace_BNB(exprCollation(rcopy)));
					rtle->expr = (Expr *) rcolnode;
				}
			}
			/* End - BUG#Z1461 */

			rescoltypmod = select_common_typmod(pstate,
												list_make2(lcolnode, rcolnode),
												rescoltype);

			/*
			 * Select common collation.  A common collation is required for
			 * all set operators except UNION ALL; see SQL:2008 7.13 <query
			 * expression> Syntax Rule 15c.  (If we fail to identify a common
			 * collation for a UNION ALL column, the colCollations element
			 * will be set to InvalidOid, which may result in a runtime error
			 * if something at a higher query level wants to use the column's
			 * collation.)
			 */
			/* Begin - ReqID:SRS-SQL-UNION */
			if (enable_setoperation_null &&	ORA_MODE == compatible_mode &&
				lcoltype == UNKNOWNOID && rescoltype == UNKNOWNOID) {
				if (IsA(lcolnode, Const))
				{
					((Const *) lcolnode)->constcollid = DEFAULT_COLLATION_OID;

				}
				if (IsA(rcolnode, Const))
				{
					((Const *) rcolnode)->constcollid = DEFAULT_COLLATION_OID;

				}
				rescolcoll = DEFAULT_COLLATION_OID;
			}
			else
			{
				/* Begin - BUG#8279 */
				if (enable_setoperation_null && ORA_MODE == compatible_mode &&
					lcoltype == UNKNOWNOID && !OidIsValid(get_typcollation(rescoltype)))
					exprSetCollation(lcolnode, InvalidOid);
				/* End - BUG#8279 */

				rescolcoll = select_common_collation(pstate,
								list_make2(lcolnode, rcolnode),
								(op->op == SETOP_UNION && op->all));
			}
			/* End - ReqID:SRS-SQL-UNION */

			/* emit results */
			op->colTypes = lappend_oid(op->colTypes, rescoltype);
			op->colTypmods = lappend_int(op->colTypmods, rescoltypmod);
			op->colCollations = lappend_oid(op->colCollations, rescolcoll);

			/*
			 * Begin - BUG#Z1461
			 * Record targetlist's collation before do pseudo-collation replacement.
			 */
			op->rawcolCollations = lappend_oid(op->rawcolCollations, OidIsValid(rawcolcoll) ? rawcolcoll : rescolcoll);
			/* End - BUG#Z1461 */

			/*
			 * For all cases except UNION ALL, identify the grouping operators
			 * (and, if available, sorting operators) that will be used to
			 * eliminate duplicates.
			 */
			if (op->op != SETOP_UNION || !op->all)
			{
				ParseCallbackState pcbstate;

				setup_parser_errposition_callback(&pcbstate, pstate,
												  bestlocation);

				/* Begin - ReqID:SRS-SQL-UNION */
				/*
				 * If it's a recursive union, we need to require hashing support.
				 * for unkonw type, use text type for comparison
				 */
				if (enable_setoperation_null &&	ORA_MODE == compatible_mode &&
					lcoltype == UNKNOWNOID && rescoltype == UNKNOWNOID &&
					(op->op == SETOP_UNION || op->op == SETOP_INTERSECT || op->op == SETOP_EXCEPT))
				{
					op->groupClauses = lappend(op->groupClauses,
								makeSortGroupClauseForSetOp(TEXTOID, recursive));
				}
				else
				{
					op->groupClauses = lappend(op->groupClauses,
								makeSortGroupClauseForSetOp(rescoltype, recursive));
				}
				/* End - ReqID:SRS-SQL-UNION */

				cancel_parser_errposition_callback(&pcbstate);
			}

			/*
			 * Construct a dummy tlist entry to return.  We use a SetToDefault
			 * node for the expression, since it carries exactly the fields
			 * needed, but any other expression node type would do as well.
			 */
			if (targetlist)
			{
				SetToDefault *rescolnode = makeNode(SetToDefault);
				TargetEntry *restle;

				rescolnode->typeId = rescoltype;
				rescolnode->typeMod = rescoltypmod;
				rescolnode->collation = rescolcoll;
				rescolnode->location = bestlocation;
				restle = makeTargetEntry((Expr *) rescolnode,
										 0, /* no need to set resno */
										 NULL,
										 false);
				*targetlist = lappend(*targetlist, restle);
			}
		}

		return (Node *) op;
	}
}

/*
 * Process the outputs of the non-recursive term of a recursive union
 * to set up the parent CTE's columns
 */
static void
determineRecursiveColTypes(ParseState *pstate, Node *larg, List *nrtargetlist)
{
	Node	   *node;
	int			leftmostRTI;
	Query	   *leftmostQuery;
	List	   *targetList;
	ListCell   *left_tlist;
	ListCell   *nrtl;
	int			next_resno;

	/*
	 * Find leftmost leaf SELECT
	 */
	node = larg;
	while (node && IsA(node, SetOperationStmt))
		node = ((SetOperationStmt *) node)->larg;
	Assert(node && IsA(node, RangeTblRef));
	leftmostRTI = ((RangeTblRef *) node)->rtindex;
	leftmostQuery = rt_fetch(leftmostRTI, pstate->p_rtable)->subquery;
	Assert(leftmostQuery != NULL);

	/*
	 * Generate dummy targetlist using column names of leftmost select and
	 * dummy result expressions of the non-recursive term.
	 */
	targetList = NIL;
	next_resno = 1;

	forboth(nrtl, nrtargetlist, left_tlist, leftmostQuery->targetList)
	{
		TargetEntry *nrtle = (TargetEntry *) lfirst(nrtl);
		TargetEntry *lefttle = (TargetEntry *) lfirst(left_tlist);
		char	   *colName;
		TargetEntry *tle;

		Assert(!lefttle->resjunk);
		colName = pstrdup(lefttle->resname);

		/* Begin - BUG#Z1461 */
		if (ORA_MODE == compatible_mode && enable_ora_default_collation)
		{
			Node 	*expr = (Node *) nrtle->expr;
			Oid		 colcoll = exprCollation(expr);
			if (IsPseudoCollation(colcoll))
				nrtle->expr = (Expr *) makePseudoCollationReplaceNode(expr, PseudoCollationReplace_BNB(colcoll));
		}
		/* End - BUG#Z1461 */

		tle = makeTargetEntry(nrtle->expr,
							  next_resno++,
							  colName,
							  false);
		targetList = lappend(targetList, tle);
	}

	/* Now build CTE's output column info using dummy targetlist */
	analyzeCTETargetList(pstate, pstate->p_parent_cte, targetList);
}


/*
 * transformReturnStmt -
 *	  transforms a return statement
 */
static Query *
transformReturnStmt(ParseState *pstate, ReturnStmt *stmt)
{
	Query	   *qry = makeNode(Query);

	qry->commandType = CMD_SELECT;
	qry->isReturn = true;

	qry->targetList = list_make1(makeTargetEntry((Expr *) transformExpr(pstate, stmt->returnval, EXPR_KIND_SELECT_TARGET),
												 1, NULL, false));

	if (pstate->p_resolve_unknowns)
		resolveTargetListUnknowns(pstate, qry->targetList);
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);
	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasWindowFuncs = pstate->p_hasWindowFuncs;
	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasAggs = pstate->p_hasAggs;

	assign_query_collations(pstate, qry);

	return qry;
}

/* Begin - ReqID:SRS-MYSQL-UPDATE */
/*
 * transformUpdateSortClause -
 * transforms an update statement sort clause
 */
static List*
transformUpdateSortClause(ParseState* pstate, UpdateStmt* stmt, Query* qry)
{
	ListCell	   *lc = NULL;
	TargetEntry	   *tle = NULL;
	int				i = 0;
	List		   *rlst = NIL;
	int				len = 0;

	if (stmt->sortClause == NULL)
	{
		return NULL;
	}

	len = list_length(qry->targetList);
	foreach(lc, qry->targetList)
	{
		tle = (TargetEntry*)lfirst(lc);
		tle->resjunk = true;
	}

	rlst = transformSortClause(pstate, stmt->sortClause, &qry->targetList, EXPR_KIND_ORDER_BY, false);
	
	foreach(lc, qry->targetList)
	{
		tle = (TargetEntry*)lfirst(lc);
		tle->resjunk = false;
		if (++i >= len)
		{
			break;
		}
	}
	return rlst;
}


/*
 * transformLimitSortClause -
 * transforms update statement limit and sort clause
 */
static void
transformLimitSortClause(ParseState *pstate, void *stmt, Query *qry, bool isdel)
{
	List	   *rlist = NIL;

	/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
	qry->limitCount = isdel ? transformLimitClause(pstate, ((DeleteStmt*)stmt)->limitClause, EXPR_KIND_LIMIT, "LIMIT", LIMIT_OPTION_DEFAULT) :
					  transformLimitClause(pstate, ((UpdateStmt*)stmt)->limitClause, EXPR_KIND_LIMIT, "LIMIT", LIMIT_OPTION_DEFAULT);
	if (isdel)
		rlist = transformSortClause(pstate, ((DeleteStmt*)stmt)->sortClause, &qry->targetList, EXPR_KIND_ORDER_BY, false);
	else
		rlist = transformUpdateSortClause(pstate, (UpdateStmt*)stmt, qry);
	/* End - ReqID:SRS-MYSQL-SQL-DELETE */
	/*
	 * For update statement, the effective range of sort clause is not optimized. For delete statement,
	 * sorting is performed only when limit or returning clause takes effect.
	 */
	if (qry->limitCount != NULL)
	{
		/* flag for discriminating rownum */
		Const *flag = makeNode(Const);
		flag->location = -1;
		flag->consttype = INT8OID;
		flag->consttypmod = -1;
		qry->limitOffset = (Node*)flag;
		qry->sortClause = rlist;
	}
	else if (!isdel || qry->returningList)
		qry->sortClause = rlist;
}


/*
 * If a relation has no column updated in the resultRelations, it is redundant
 * and remove it from the resultRelations.
 */
static List
*remove_update_redundant_relation(List *resultRelations, List *target_rangetblentry)
{
	RangeTblEntry	   *target_rte;
	ListCell		   *rte = list_head(target_rangetblentry);
	ListCell		   *res = list_head(resultRelations);
	List			   *reslist = NIL;

	if (list_length(resultRelations) == 1)
		return resultRelations;

	forboth(rte, target_rangetblentry, res, resultRelations)
	{
		target_rte = (RangeTblEntry*)lfirst(rte);
		if(!bms_is_empty(target_rte->updatedCols))
			reslist = lappend_int(reslist, lfirst_int(res));
	}

	return reslist;
}


/*
 * CheckUpdateRelations
 * For mutilple-relations modifying, the limit clause, order by clause,
 * and returning clause are not supported.
 */
static void
CheckUpdateRelations(ParseState* pstate,        UpdateStmt *stmt)
{
	if (list_length(pstate->p_target_rels) == 1)
		return;

	if (stmt->sortClause != NIL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("mutilple-relations modifying doesn't support %s ORDER BY", "update")));
	}
	if (stmt->limitClause != NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("mutilple-relations modifying doesn't support %s LIMIT", "update")));
	}
	if (stmt->returningList != NIL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("mutilple-relations modifying doesn't support %s returning", "update")));
	}
	if(stmt->fromClause)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("mutilple-relations modifying doesn't support %s fromclause", "update")));
	}
}


/*
 * transformMergeTargetList - Multiple sets of the same column in a single table.
 * If the same column is set multiple times in a single table, the last set value is retained.
 */
static void
transformMergeTargetList(RangeVar *rel, List **targetList, bool ismulti)
{
	List	   *tmptle = NIL;
	List	   *tmplist= NIL;
	ListCell   *lc = NULL;
	ListCell   *lc1 = NULL;
	int			i = 0, j = 0;
	int			num = list_length(*targetList);

	/* To determine whether an alias exists in a table,
	 * you need to compare whether the alias or table name appears in the target column.
	 */
	if (rel->alias && !ismulti)
	{
		foreach (lc1, *targetList)
		{
			ResTarget   *ret = (ResTarget *)lfirst(lc1);
			if (ret->indirection)
			{
				if (strcmp(ret->name, rel->alias->aliasname) != 0)
					return;
			}
		}
	}

	/* Merge same columns, retaining the last set value. */
	tmptle = copyObject(*targetList);
	foreach (lc, *targetList)
	{
		ResTarget   *tmpret;
		ResTarget   *ret1 = (ResTarget *)lfirst(lc);
		bool		flag = false;

		j = i + 1;
		while (j < num)
		{
			ResTarget   *ret2 = (ResTarget *)list_nth(tmptle, j);

			if (strcmp(ret1->name, ret2->name) == 0)
			{
				if (ret1->indirection && ret2->indirection)
				{
					if (equal(ret1->indirection, ret2->indirection))
					{
						tmpret = ret2;
						flag = true;
					}
				}
				else if (!ret1->indirection && !ret2->indirection)
				{
					tmpret = ret2;
					flag = true;
				}
			}
			else
			{
				/* Column with or without alias example:id = 3,a.id = 4. */
				if (ret1->indirection && !ret2->indirection)
				{
					if (strcmp(ret2->name, strVal((Value*)llast(ret1->indirection))) == 0)
					{
						tmpret = ret2;
						flag = true;
					}
				}
				else if (!ret1->indirection && ret2->indirection)
				{
					if (strcmp(ret1->name, strVal((Value*)llast(ret2->indirection))) == 0)
					{
						tmpret = ret2;
						flag = true;
					}
				}
			}
			j++;
		}
		if (flag)
		{
			if (!list_member(tmplist, tmpret))
				tmplist = lappend(tmplist, tmpret);
		}
		else
		{
			if (!list_member(tmplist, ret1))
				tmplist = lappend(tmplist, ret1);
		}
		i++;
	}

	*targetList = NIL;
	*targetList = tmplist;
}


/*
 * transformAliasInTargetlist
 * Whether to use alias for target column.
 */
static void
transformAliasInTargetlist(RangeVar *rel, List *targetList)
{
	ListCell	   *lc = NULL;
	char		   *resname = NULL;
	char		   *colname = NULL;
	foreach (lc, targetList)
	{
		ResTarget   *ret = (ResTarget *)lfirst(lc);

		/* Target column with table name. eg:tbl_name.colname. */
		if (ret->name && list_length(ret->indirection) == 1)
		{
			Node	   *node = linitial(ret->indirection);
			if (IsA(node, String))
			{
				colname = pstrdup(strVal(node));
				/* If the target column uses the table name but not the table alias,
				 * an error is reported.
				 */
				if (!strcmp(ret->name, rel->relname))
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("Unknown column %s.%s in field list ", ret->name, colname)));
			}
		}
		/* Target column with schema and table name. eg:schema.tbl_name.colname. */
		else if (rel->schemaname && ret->name && list_length(ret->indirection) == 2)
		{
			Node	   *node1 = linitial(ret->indirection);
			Node	   *node2 = lsecond(ret->indirection);
			if (IsA(node1, String) && IsA(node2, String))
			{
				resname = pstrdup(strVal(node1));
				colname = pstrdup(strVal(node2));
				/* If the target column uses the schema and table name 
				 * but not the table alias, an error is reported.
				 */
				if (!strcmp(resname, rel->relname) && 
					!strcmp(ret->name, rel->schemaname))
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("Unknown column %s.%s.%s in field list ", ret->name, resname, colname)));
			}
		}
	}
	return;
}

/*
 * transformRemoveschemaInTargetlist
 * Remove the schema from the target column.
 */
static void
transformRemoveschemaInTargetlist(RangeVar *rel, List **targetList)
{
	ListCell	   *lc = NULL;
	char		   *resname = NULL;
	foreach (lc, *targetList)
	{
		ResTarget	*ret = (ResTarget *)lfirst(lc);
		if (list_length(ret->indirection) == 2)
		{
			Node	   *node = linitial(ret->indirection);
			if (IsA(node, String))
			{
				resname = pstrdup(strVal(node));
				if (rel->schemaname)
				{
					if (!strcmp(rel->relname, resname) && 
						!strcmp(rel->schemaname, ret->name))
					{
						ret->name = resname;
						ret->indirection = list_delete_first(ret->indirection);
					}
				}
			}
		}
	}
	return;
}


/*
 * transformMultiMergeTargetList
 * Parse columns of multiple relationships and merge same target columns.
 */
static void
transformMultiMergeTargetList(List *rellist, List **targetList)
{
	ListCell   *lc = NULL;
	foreach (lc, rellist)
	{
		RangeVar   *ret = (RangeVar *)lfirst(lc);
		/* Begin - BUG#Z1324 */
		if (ret->alias)
			transformAliasInTargetlist(ret, *targetList);
		else
			transformRemoveschemaInTargetlist(ret, targetList);
		/* End - BUG#Z1324 */
		transformMergeTargetList(ret, targetList, true);
	}
	return;
}


/*
 * transformUpdateStmt -
 *	  transforms an update statement
 */
static Query *
transformUpdateStmt(ParseState *pstate, UpdateStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	ParseNamespaceItem *nsitem = NULL;
	Node	   *qual = NULL;

	qry->commandType = CMD_UPDATE;
	pstate->p_is_insert = false;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		pstate->p_isupdate = true;
		if (stmt->relation)
		{
			transformMergeTargetList(stmt->relation, &stmt->targetList, false);
			/* Begin - BUG#Z1324 */
			transformRemoveschemaInTargetlist(stmt->relation, &stmt->targetList);
			if (stmt->relation->alias)
				transformAliasInTargetlist(stmt->relation, stmt->targetList);
			/* End - BUG#Z1324 */
			if (stmt->relation->ispartition)
				transformGetpartionRelationOid(qry, stmt->relation);
		}
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */
	/* Begin - ReqID:SRS-SQL-MULTICOLUMNUPDATE */
	else if (ORA_MODE == compatible_mode)
	{
		if (stmt->relation && enable_update_multicol)
			transformMergeTargetList(stmt->relation, &stmt->targetList, false);
	}
	/* End - ReqID:SRS-SQL-MULTICOLUMNUPDATE */

	/* process the WITH clause independently of all else */
	if (stmt->withClause)
	{
		qry->hasRecursive = stmt->withClause->recursive;
		qry->cteList = transformWithClause(pstate, stmt->withClause);
		qry->hasModifyingCTE = pstate->p_hasModifyingCTE;
	}

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && stmt->relationClause) /* BUG#Z5121 */
	{
		if (qry->cteList)
			ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("mutilple-relations modifying doesn't support %s with claues.", "update")));
		pstate->p_addUpdateTable = true;
		/* Adjust the position of the relationship and put the base table in front */
		stmt->relationClause = transfromRelationposition(pstate, stmt->relationClause);
		/* Process the FROM clause and add items to the query's range table,
		 * joinlist, and namespace. get rtable index of target relation for update stmt.
		 */
		transformFromClause(pstate, stmt->relationClause);
		qry->resultRelations = copyObject(pstate->p_updateRelations);
		CheckUpdateRelations(pstate, stmt);
		transformMultiMergeTargetList(pstate->p_updateRangeVars, &stmt->targetList);
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */
	else
	{
		qry->resultRelation = setTargetTable(pstate, stmt->relation,
											 stmt->relation->inh,
											 true,
											 ACL_UPDATE);
		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
			pstate->p_updateRelations = lappend_int(pstate->p_updateRelations, qry->resultRelation);
		/* End - ReqID:SRS-MYSQL-UPDATE */
	}
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (pstate->p_target_nsitem)
	{
		nsitem = pstate->p_target_nsitem;

		/* subqueries in FROM cannot access the result relation */
		nsitem->p_lateral_only = true;
		nsitem->p_lateral_ok = false;
	}

	if (MYS_MODE == compatible_mode && pstate->p_indexhintLists) /* BUG#Z5121 */
		qry->indexhintList = pstate->p_indexhintLists;
	/* End - ReqID:SRS-MYSQL-UPDATE */

	/*
	 * the FROM clause is non-standard SQL syntax. We used to be able to do
	 * this with REPLACE in POSTQUEL so we keep the feature.
	 */
	transformFromClause(pstate, stmt->fromClause);

	/* remaining clauses can reference the result relation normally */
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (nsitem)
	{
		nsitem->p_lateral_only = false;
		nsitem->p_lateral_ok = true;
	}

	if (compatible_mode != MYS_MODE) /* BUG#Z5121 */
		qual = transformWhereClause(pstate, stmt->whereClause,
									EXPR_KIND_WHERE, "WHERE");
	/* End - ReqID:SRS-MYSQL-UPDATE */

	qry->returningList = transformReturningList(pstate, stmt->returningList);

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	if (ORA_MODE == compatible_mode)
	{
		qry->targetList = transformIvyUpdateTargetList(pstate, stmt->targetList);

		/* Begin - ReqID:SRS-SQL-ROWNUM */
		if (pstate->p_hasRownum)
		{
			qry->hasRownum = pstate->p_hasRownum;
			addRangeTableEntryForCount(pstate);
		}
		/* End - ReqID:SRS-SQL-ROWNUM */
	}
	else
	/* End - ReqID:SRS-PLSQL-TYPE */
	{
		/*
		 * Now we are done with SELECT-like processing, and can get on with
		 * transforming the target list to match the UPDATE target columns.
		 */
		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode && stmt->relationClause) /* BUG#Z5121 */
		{
			if (pstate->p_addupdatedeleterv)
				transformGetpartionRelationOids(qry, pstate->p_addupdatedeleterv);
			qry->targetList = transformMultiUpdateTargetList(pstate, stmt->targetList, qry->resultRelations);
			qry->resultRelations = remove_update_redundant_relation(qry->resultRelations, pstate->p_target_rangetblentry);
			pstate->p_updateRelations = copyObject(qry->resultRelations);
		}
		/* End - ReqID:SRS-MYSQL-UPDATE */
		else
			qry->targetList = transformUpdateTargetList(pstate, stmt->targetList);

		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode && qry->resultRelation) /* BUG#Z5121 */
			transformLimitSortClause(pstate, stmt, qry, false);
		/* End - ReqID:SRS-MYSQL-UPDATE */
	}

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
		qual = transformWhereClause(pstate, stmt->whereClause,
										EXPR_KIND_WHERE, "WHERE");
	/* End - ReqID:SRS-MYSQL-UPDATE */

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasSubLinks = pstate->p_hasSubLinks;

	assign_query_collations(pstate, qry);

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && list_length(qry->resultRelations) == 1) /* BUG#Z5121 */
	{
		qry->resultRelation = linitial_int(qry->resultRelations);
		qry->resultRelations = NIL;
	}

	if (pstate->p_updateRelations && MYS_MODE == compatible_mode) /* BUG#Z5121 */
		pstate->p_updateRelations = NIL;
	/* End - ReqID:SRS-MYSQL-UPDATE */

	return qry;
}

/* Begin - ReqID:SRS-MYSQL-UPDATE */
/*
 * checkUpdateResTargetName- Get the name of the relationship to be updated.
 */
char
*checkUpdateResTargetName(Relation rd, RangeVar *rel, ResTarget *res, bool *matchRelname)
{
	char	   *resname = NULL;
	AttrNumber	attrnoIndName = InvalidAttrNumber;
	AttrNumber	attrnoResName = InvalidAttrNumber;
	Oid			attrtypeid = InvalidOid;
	Oid			typrelid = InvalidOid;

	/* the length of indireciton can't less than 1 and the first item type must not be A_Indices */
	if ((list_length(res->indirection) < 1) || IsA(linitial(res->indirection), A_Indices))
	{
		return res->name;
	}

	/* name must relname or aliasname */
	if ((strcmp(rel->relname, res->name) != 0) &&
		((rel->alias == NULL) || (strcmp((rel->alias)->aliasname, res->name) != 0)))
	{
		return res->name;
	}

	/*
	 * if meet set_clause like a.b.c = 1 then the indirection will exceed one,
	 * so treat first indirection as tablename/alisename like a in this ex.
	 */
	resname = pstrdup(strVal(linitial(res->indirection)));
	if (list_length(res->indirection) > 1)
	{
		*matchRelname = true;
		return resname;
	}

	/*
	 * Here only meet set_clause which has two elements like a.b or test.b.
	 * So first keep forward-compatibility behavior of old version, that results
	 * if a is a composite type and also relname/alisename then we first handle it as
	 * composite type.
	 * Otherwise we handle it like a.b like relname/alisename, then a is a relname just
	 * update set_clause over.
	 */
	attrnoResName = attnameAttNum(rd, res->name, true);
	attrnoIndName = attnameAttNum(rd, resname, true);

	if (attrnoResName > 0)
	{
		attrtypeid = attnumTypeId(rd, attrnoResName);
		if (attrtypeid)
		{
			typrelid = typeidTypeRelid(attrtypeid);
			if (typrelid && ((get_attnum(typrelid, resname) != InvalidAttrNumber) || (attrnoIndName <= 0)))
			{
				ereport(NOTICE, (errmsg("update field '%s' of column '%s', though it's ambiguous.",
						resname, res->name)));
				return res->name;
			}
		}
	}

	*matchRelname = true;
	return resname;
}

/*
 * checkSRFInMultiUpdate
 * Verify that a set-returning function is called in multiple-update and throw a error.
 */
static inline void
checkSRFInMultiUpdate(Expr *expr, int targetRelationNum)
{
	if (targetRelationNum == 1)
		return;

	if ((expr->type == T_FuncExpr) && ((FuncExpr *)expr)->funcretset == true)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("Un-support feature"),
				errdetail("set-returning functions are not allowed in multiple UPDATE")));
	}
}



/*
 * fixUpdateResTargetName
 * Find the attrno corresponding to ResTarget from target tables.
 */
static int
fixUpdateResTargetName(ParseState *pstate, List *resultRelations, ResTarget *res, int *rti,
							   Relation *rd, RangeTblEntry **rte)
{
	ListCell	   *l1;
	ListCell	   *l2;
	ListCell	   *l3;
	ListCell	   *l4;
	bool			removeRelname = false,
					matchRelname = false;
	char		   *resname = NULL;
	char		   *resultResName = NULL;
	int				attrno,
					resultAttrno = InvalidAttrNumber;
	bool			isMatched = false;

	forfour (l1, pstate->p_target_rangetblentry, l2, pstate->p_target_rels,
			 l3, pstate->p_updateRangeVars, l4, resultRelations)
	{
		RangeTblEntry	   *target_rte = (RangeTblEntry*)lfirst(l1);
		Relation			targetrel = (Relation)lfirst(l2);
		RangeVar		   *rangeVar = (RangeVar*)lfirst(l3);
		int					rtindex = lfirst_int(l4);

		resname = checkUpdateResTargetName(targetrel, rangeVar, res, &matchRelname);
		attrno = attnameAttNum(targetrel, resname, true);
		if (attrno != InvalidAttrNumber)
		{
			/*
			 * If the targetlist matches more than once and when target contains a relname(matchRelname)
			 * is the same as before(removeRelname), report error.
			 */
			if (isMatched == true && (matchRelname == removeRelname))
			{
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_COLUMN),
						errmsg("column reference \"%s\" is ambiguous", res->name),
						parser_errposition(pstate, res->location)));
			}
			resultAttrno = attrno;
			resultResName = resname;
			*rti = rtindex;
			*rd = targetrel;
			*rte = target_rte;

			isMatched = true;
			/* Mark the target column as requiring update permissions */
			target_rte->updatedCols = bms_add_member(target_rte->updatedCols,
													 attrno - FirstLowInvalidHeapAttributeNumber);
		}
		if (matchRelname == true)
				removeRelname = true;
	}

	if (!isMatched)
	{
		res->name = resname;
		if (matchRelname)
		{
			res->indirection = list_delete_first_n(res->indirection, 1);
		}
		return InvalidAttrNumber;
	}

	if (removeRelname)
	{
		res->name = resultResName;
		res->indirection = list_delete_first_n(res->indirection, 1);
	}

	return resultAttrno;
}

/*
 * MergeTargetList
 * Merge the targetlists of multiple identical result relations into the one and change their rtindex.
 */
static void
MergeTargetList(List **targetLists, RangeTblEntry *rte1, int rtindex1,
						   RangeTblEntry *rte2, int rtindex2)
{
	ListCell* l = NULL;

	foreach (l, targetLists[rtindex2 - 1])
	{
		TargetEntry   *tle = (TargetEntry*)lfirst(l);
		tle->rtindex = (Index)rtindex1;
		rte1->updatedCols = bms_add_member(rte1->updatedCols, tle->resno - FirstLowInvalidHeapAttributeNumber);
		rte2->updatedCols = bms_del_member(rte2->updatedCols, tle->resno - FirstLowInvalidHeapAttributeNumber);
	}
	targetLists[rtindex1 - 1] = list_concat(targetLists[rtindex1 - 1], targetLists[rtindex2 - 1]);
	targetLists[rtindex2 - 1] = NULL;
}

/*
 * transformMergeMultiTargetList
 * If there are actually the same result relations by different alias in multiple update, merge their targetLists.
 */
static void
transformMergeMultiTargetList(List *target_rangetblentry, List **targetLists)
{
	int rtindex1 = 1, rtindex2 = 1;
	ListCell* l1;
	ListCell* l2;

	if (list_length(target_rangetblentry) <= 1)
		return;
	foreach (l1, target_rangetblentry)
	{
		RangeTblEntry* rte1 = (RangeTblEntry*)lfirst(l1);
		rtindex2 = 0;

		l2 = lnext(target_rangetblentry, l1);
		rtindex2 = rtindex1 + 1;
		while (l2 != NULL)
		{
			RangeTblEntry* rte2 = (RangeTblEntry*)lfirst(l2);
			if (rte2->relid == rte1->relid)
				MergeTargetList(targetLists, rte1, rtindex1, rte2, rtindex2);
			rtindex2++;
			l2 = lnext(target_rangetblentry, l2);
		}
		rtindex1++;
	}
}

/*
 * UndefinedColumnError
 * Unrecognized column information error.
 */
static void
UndefinedColumnError(ParseState *pstate, ResTarget *origTarget, int targetRelationNum)
{
	if (!origTarget->indirection)
	{
		if (targetRelationNum == 1)
		{
			ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
					errmsg("column \"%s\" of relation \"%s\" does not exist",
					origTarget->name,
					RelationGetRelationName((Relation)linitial(pstate->p_target_rels))),
					parser_errposition(pstate, origTarget->location)));
		}
		ereport(ERROR,
		(errcode(ERRCODE_UNDEFINED_COLUMN),
				errmsg("column \"%s\" does not exist",
				origTarget->name),
				parser_errposition(pstate, origTarget->location)));
	}
	else
	{
		char *resname = pstrdup((char*)(((Value*)lfirst(list_head(origTarget->indirection)))->val.str));
		if (targetRelationNum == 1)
		{
			ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
					errmsg("column \"%s.%s\" of relation \"%s\" does not exist",
					origTarget->name, resname,
					RelationGetRelationName((Relation)linitial(pstate->p_target_rels))),
					parser_errposition(pstate, origTarget->location)));
		}
		ereport(ERROR,
			(errcode(ERRCODE_UNDEFINED_COLUMN),
				errmsg("column \"%s.%s\" does not exist",
				origTarget->name, resname),
				parser_errposition(pstate, origTarget->location)));
	}
}


/*
 * transformMultiUpdateTargetList
 * handle SET clause in UPDATE/MERGE/INSERT ... ON CONFLICT UPDATE
 */
List *
transformMultiUpdateTargetList(ParseState *pstate, List *origTlist, List* resultRelations)
{
	List	   *tlist = NIL;
	List	   *rtlist = NIL;
	RangeTblEntry *target_rte;
	ListCell   *orig_tl;
	ListCell   *tl;
	ListCell   *cell;
	Relation	targetrel = NULL;
	int			rtindex = 0;
	int			targetRelationNum = list_length(pstate->p_target_rels);
	int			rangeTableNum = list_length(pstate->p_rtable);
	List	  **new_tle = (List**)palloc0(rangeTableNum * sizeof(List*));

	tlist = transformTargetList(pstate, origTlist,
								EXPR_KIND_UPDATE_SOURCE);

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	pstate->p_next_resno = 1;
	foreach(cell, pstate->p_target_rels)
	{
		targetrel = (Relation)lfirst(cell);
		pstate->p_next_resno += targetrel->rd_rel->relnatts;
	}

	/* Prepare non-junk columns for assignment to target table */
	forboth(tl, tlist, orig_tl, origTlist)
	{
		TargetEntry	   *tle = (TargetEntry *) lfirst(tl);
		ResTarget	   *origTarget = (ResTarget*) lfirst(orig_tl);
		int				attrno;
		Relation		tmprelation;
		char		   *colname_temp = NULL;

		if (tle->resjunk)
		{
			/*
			 * Resjunk nodes need no additional processing, but be sure they
			 * have resnos that do not match any target columns; else rewriter
			 * or planner might get confused.  They don't need a resname
			 * either.
			 */
			tle->resno = (AttrNumber) pstate->p_next_resno++;
			tle->resname = NULL;
			continue;
		}
		attrno = fixUpdateResTargetName(pstate, resultRelations, origTarget, &rtindex, &targetrel, &target_rte);
		if (attrno == InvalidAttrNumber)
		{
			UndefinedColumnError(pstate, origTarget, targetRelationNum);
		}
		tmprelation = pstate->p_target_relation;
		pstate->p_target_relation = targetrel;
		if (colname_temp)
			updateTargetListEntry(pstate, tle, origTarget->name, attrno, NULL, origTarget->location);
		else
			updateTargetListEntry(pstate, tle, origTarget->name, attrno, origTarget->indirection, origTarget->location);
		pstate->p_target_relation = tmprelation;

		checkSRFInMultiUpdate(tle->expr, targetRelationNum);
		tle->rtindex = rtindex;
		new_tle[rtindex - 1] = lappend(new_tle[rtindex - 1], tle);
	}

	/*
	 * If there are actually the same result relations by different alias
	 * or synonym in multiple update, merge their targetLists.
	 */
	transformMergeMultiTargetList(pstate->p_target_rangetblentry, new_tle);

	if (targetRelationNum == 1)
	{
		int i = linitial_int(resultRelations);
		rtlist = new_tle[i - 1];
	}
	else
	{
		for (int i = 0; i < rangeTableNum; i++)
		{
			if (new_tle[i])
			{
				rtlist = list_concat(rtlist, new_tle[i]);
			}
		}
	}
	pfree(new_tle);

	return rtlist;
}
/* End -  ReqID:SRS-MYSQL-UPDATE */

/*
 * transformUpdateTargetList -
 *	handle SET clause in UPDATE/MERGE/INSERT ... ON CONFLICT UPDATE
 */
List *
transformUpdateTargetList(ParseState *pstate, List *origTlist)
{
	List	   *tlist = NIL;
	RangeTblEntry *target_rte;
	ListCell   *orig_tl;
	ListCell   *tl;

	tlist = transformTargetList(pstate, origTlist,
								EXPR_KIND_UPDATE_SOURCE);

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_next_resno <= RelationGetNumberOfAttributes(pstate->p_target_relation))
		pstate->p_next_resno = RelationGetNumberOfAttributes(pstate->p_target_relation) + 1;

	/* Prepare non-junk columns for assignment to target table */
	target_rte = pstate->p_target_nsitem->p_rte;
	orig_tl = list_head(origTlist);

	foreach(tl, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		ResTarget  *origTarget;
		int			attrno;
		char  	   *colname_temp = NULL;
		RangeTblEntry *alias_temp = NULL;

		if (tle->resjunk)
		{
			/*
			 * Resjunk nodes need no additional processing, but be sure they
			 * have resnos that do not match any target columns; else rewriter
			 * or planner might get confused.  They don't need a resname
			 * either.
			 */
			tle->resno = (AttrNumber) pstate->p_next_resno++;
			tle->resname = NULL;
			continue;
		}
		if (orig_tl == NULL)
			elog(ERROR, "UPDATE target count mismatch --- internal error");
		origTarget = lfirst_node(ResTarget, orig_tl);

		attrno = attnameAttNum(pstate->p_target_relation,
							   origTarget->name, true);
		if (attrno == InvalidAttrNumber)
		{
			if (origTarget->indirection)
			{
				colname_temp = strVal(lfirst(list_head(origTarget->indirection)));
				alias_temp = lfirst(list_head(pstate->p_rtable));

				if (alias_temp->alias)
				{
					if (alias_temp->alias->aliasname)
					{
						if (strcmp(origTarget->name, alias_temp->alias->aliasname))
						{
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_COLUMN),
									 errmsg("Perhaps you meant to reference the table alias \"%s\"",
											alias_temp->alias->aliasname)));
						}
						else
						{
							if (colname_temp)
							{
								attrno = attnameAttNum(pstate->p_target_relation, colname_temp, true);
							}
						}
					}
				}
				else
				{
					if (origTarget->name && strcmp(origTarget->name, RelationGetRelationName(pstate->p_target_relation)))
					{
						ereport(ERROR,
								(errcode(ERRCODE_UNDEFINED_COLUMN),
								 errmsg("Perhaps you meant to assign the table alias \"%s\"",
										origTarget->name)));
					}
					else
					{
						if (colname_temp)
						{
							attrno = attnameAttNum(pstate->p_target_relation, colname_temp, true);
						}
					}
				}
			}
			else
			{
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
							errmsg("column \"%s\" of relation \"%s\" does not exist",
							origTarget->name,
							RelationGetRelationName(pstate->p_target_relation)),
							parser_errposition(pstate, origTarget->location)));
			}
		}

		/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		if (compatible_mode == MYS_MODE)
		{
			tle->expr = update_set_auto_increment_func_node(pstate, tle->expr, attrno);
		}
		/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

		if (colname_temp)
		{
			updateTargetListEntry(pstate, tle, colname_temp,
								  attrno,
								  NULL,
								  origTarget->location);
		}
		else
		{
			updateTargetListEntry(pstate, tle, origTarget->name,
								  attrno,
								  origTarget->indirection,
								  origTarget->location);

		}

		/* Mark the target column as requiring update permissions */
		target_rte->updatedCols = bms_add_member(target_rte->updatedCols,
												 attrno - FirstLowInvalidHeapAttributeNumber);

		orig_tl = lnext(origTlist, orig_tl);
	}
	if (orig_tl != NULL)
		elog(ERROR, "UPDATE target count mismatch --- internal error");

	return tlist;
}

/*
 * transformReturningList -
 *	handle a RETURNING clause in INSERT/UPDATE/DELETE
 */
static List *
transformReturningList(ParseState *pstate, List *returningList)
{
	List	   *rlist;
	int			save_next_resno;

	if (returningList == NIL)
		return NIL;				/* nothing to do */

	/*
	 * We need to assign resnos starting at one in the RETURNING list. Save
	 * and restore the main tlist's value of p_next_resno, just in case
	 * someone looks at it later (probably won't happen).
	 */
	save_next_resno = pstate->p_next_resno;
	pstate->p_next_resno = 1;

	/* transform RETURNING identically to a SELECT targetlist */
	rlist = transformTargetList(pstate, returningList, EXPR_KIND_RETURNING);

	/*
	 * Complain if the nonempty tlist expanded to nothing (which is possible
	 * if it contains only a star-expansion of a zero-column table).  If we
	 * allow this, the parsed Query will look like it didn't have RETURNING,
	 * with results that would probably surprise the user.
	 */
	if (rlist == NIL)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("RETURNING must have at least one column"),
				 parser_errposition(pstate,
									exprLocation(linitial(returningList)))));

	/* mark column origins */
	markTargetListOrigins(pstate, rlist);

	/* resolve any still-unresolved output columns as being type text */
	if (pstate->p_resolve_unknowns)
		resolveTargetListUnknowns(pstate, rlist);

	/* restore state */
	pstate->p_next_resno = save_next_resno;

	return rlist;
}


/*
 * transformPLAssignStmt -
 *	  transform a PL/pgSQL assignment statement
 *
 * If there is no opt_indirection, the transformed statement looks like
 * "SELECT a_expr ...", except the expression has been cast to the type of
 * the target.  With indirection, it's still a SELECT, but the expression will
 * incorporate FieldStore and/or assignment SubscriptingRef nodes to compute a
 * new value for a container-type variable represented by the target.  The
 * expression references the target as the container source.
 */
static Query *
transformPLAssignStmt(ParseState *pstate, PLAssignStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	ColumnRef  *cref = makeNode(ColumnRef);
	List	   *indirection = stmt->indirection;
	int			nnames = stmt->nnames;
	SelectStmt *sstmt = stmt->val;
	Node	   *target;
	Oid			targettype;
	int32		targettypmod;
	Oid			targetcollation;
	List	   *tlist;
	TargetEntry *tle;
	Oid			type_id;
	Node	   *qual;
	ListCell   *l;

	/*
	 * First, construct a ColumnRef for the target variable.  If the target
	 * has more than one dotted name, we have to pull the extra names out of
	 * the indirection list.
	 */
	cref->fields = list_make1(makeString(stmt->name));
	cref->location = stmt->location;
	if (nnames > 1)
	{
		/* avoid munging the raw parsetree */
		indirection = list_copy(indirection);
		while (--nnames > 0 && indirection != NIL)
		{
			Node	   *ind = (Node *) linitial(indirection);

			if (!IsA(ind, String))
				elog(ERROR, "invalid name count in PLAssignStmt");
			cref->fields = lappend(cref->fields, ind);
			indirection = list_delete_first(indirection);
		}
	}

	/*
	 * Transform the target reference.  Typically we will get back a Param
	 * node, but there's no reason to be too picky about its type.
	 */
	target = transformExpr(pstate, (Node *) cref,
						   EXPR_KIND_UPDATE_TARGET);
	targettype = exprType(target);
	targettypmod = exprTypmod(target);
	targetcollation = exprCollation(target);

	/*
	 * The rest mostly matches transformSelectStmt, except that we needn't
	 * consider WITH or INTO, and we build a targetlist our own way.
	 */
	qry->commandType = CMD_SELECT;
	pstate->p_is_insert = false;

	/* make FOR UPDATE/FOR SHARE info available to addRangeTableEntry */
	pstate->p_locking_clause = sstmt->lockingClause;

	/* make WINDOW info available for window functions, too */
	pstate->p_windowdefs = sstmt->windowClause;

	/* process the FROM clause */
	transformFromClause(pstate, sstmt->fromClause);

	/* initially transform the targetlist as if in SELECT */
	tlist = transformTargetList(pstate, sstmt->targetList,
								EXPR_KIND_SELECT_TARGET);

	/* we should have exactly one targetlist item */
	if (list_length(tlist) != 1)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg_plural("assignment source returned %d column",
							   "assignment source returned %d columns",
							   list_length(tlist),
							   list_length(tlist))));

	tle = linitial_node(TargetEntry, tlist);

	/*
	 * This next bit is similar to transformAssignedExpr; the key difference
	 * is we use COERCION_PLPGSQL not COERCION_ASSIGNMENT.
	 */
	type_id = exprType((Node *) tle->expr);

	pstate->p_expr_kind = EXPR_KIND_UPDATE_TARGET;

	if (indirection)
	{
		tle->expr = (Expr *)
			transformAssignmentIndirection(pstate,
										   target,
										   stmt->name,
										   false,
										   targettype,
										   targettypmod,
										   targetcollation,
										   indirection,
										   list_head(indirection),
										   (Node *) tle->expr,
										   COERCION_PLPGSQL,
										   exprLocation(target));
	}
	else if (targettype != type_id &&
			 (targettype == RECORDOID || ISCOMPLEX(targettype)) &&
			 (type_id == RECORDOID || ISCOMPLEX(type_id)))
	{
		/*
		 * Hack: do not let coerce_to_target_type() deal with inconsistent
		 * composite types.  Just pass the expression result through as-is,
		 * and let the PL/pgSQL executor do the conversion its way.  This is
		 * rather bogus, but it's needed for backwards compatibility.
		 */
	}
	/* Begin - ReqID:SRS-PLSQL-Collection */
	else if (IS_COLLECTOID(targettype) || IS_COLLECTOID(type_id))
	{
		if (targettype != type_id)
			ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("expression is of wrong type"),
				 parser_errposition(pstate, exprLocation((Node *)tle->expr))));
		/*
		 * Hack: do not let coerce_to_target_type() deal with collection
		 * types.  Just pass the expression result through as-is,
		 * and let the PL/pgSQL executor do the conversion its way.
		 */
	}
	/* End - ReqID:SRS-PLSQL-Collection */
	else
	{
		/*
		 * For normal non-qualified target column, do type checking and
		 * coercion.
		 */
		Node	   *orig_expr = (Node *) tle->expr;

		/* Begin - ReqID:SRS-SQL-CALL */
		if (pstate->do_from_call &&
			(((type_id == ORACHARCHAROID || type_id == ORACHARBYTEOID) &&
				(targettype == ORACHARCHAROID || targettype == ORACHARBYTEOID)) ||
			((targettype == ORAVARCHARCHAROID || targettype == ORAVARCHARBYTEOID) &&
				(targettype == ORAVARCHARCHAROID || targettype == ORAVARCHARBYTEOID))))
			tle->expr = (Expr *)
				coerce_to_target_type(pstate,
									  orig_expr, type_id,
									  targettype, targettypmod,
									  COERCION_EXPLICIT,
									  COERCE_IMPLICIT_CAST,
									  -1);
		else
		/* End - ReqID:SRS-SQL-CALL */
			tle->expr = (Expr *)
				coerce_to_target_type(pstate,
									  orig_expr, type_id,
									  targettype, targettypmod,
									  COERCION_PLPGSQL,
									  COERCE_IMPLICIT_CAST,
									  -1);
		/* With COERCION_PLPGSQL, this error is probably unreachable */
		if (tle->expr == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("variable \"%s\" is of type %s"
							" but expression is of type %s",
							stmt->name,
							format_type_be(targettype),
							format_type_be(type_id)),
					 errhint("You will need to rewrite or cast the expression."),
					 parser_errposition(pstate, exprLocation(orig_expr))));
	}

	pstate->p_expr_kind = EXPR_KIND_NONE;

	qry->targetList = list_make1(tle);

	/* transform WHERE */
	qual = transformWhereClause(pstate, sstmt->whereClause,
								EXPR_KIND_WHERE, "WHERE");

	/* initial processing of HAVING clause is much like WHERE clause */
	qry->havingQual = transformWhereClause(pstate, sstmt->havingClause,
										   EXPR_KIND_HAVING, "HAVING");

	/*
	 * Transform sorting/grouping stuff.  Do ORDER BY first because both
	 * transformGroupClause and transformDistinctClause need the results. Note
	 * that these functions can also change the targetList, so it's passed to
	 * them by reference.
	 */
	qry->sortClause = transformSortClause(pstate,
										  sstmt->sortClause,
										  &qry->targetList,
										  EXPR_KIND_ORDER_BY,
										  false /* allow SQL92 rules */ );

	qry->groupClause = transformGroupClause(pstate,
											sstmt->groupClause,
											&qry->groupingSets,
											&qry->targetList,
											qry->sortClause,
											EXPR_KIND_GROUP_BY,
											false /* allow SQL92 rules */ );

	qry->groupDistinct = sstmt->groupDistinct;

	/* Begin - BUG#M4610 */
	if (pstate->p_hasAggs && qry->groupClause == NULL && qry->sortClause != NULL)
	{
		aggregates_without_group_by_check(pstate, qry);
	}
	/* End - BUG#M4610 */

	if (sstmt->distinctClause == NIL)
	{
		qry->distinctClause = NIL;
		qry->hasDistinctOn = false;
	}
	else if (linitial(sstmt->distinctClause) == NULL)
	{
		/* We had SELECT DISTINCT */
		qry->distinctClause = transformDistinctClause(pstate,
													  &qry->targetList,
													  qry->sortClause,
													  false);
		qry->hasDistinctOn = false;
	}
	else
	{
		/* We had SELECT DISTINCT ON */
		qry->distinctClause = transformDistinctOnClause(pstate,
														sstmt->distinctClause,
														&qry->targetList,
														qry->sortClause);
		qry->hasDistinctOn = true;
	}

	/* transform LIMIT */
	qry->limitOffset = transformLimitClause(pstate, sstmt->limitOffset,
											EXPR_KIND_OFFSET, "OFFSET",
											sstmt->limitOption);
	qry->limitCount = transformLimitClause(pstate, sstmt->limitCount,
										   EXPR_KIND_LIMIT, "LIMIT",
										   sstmt->limitOption);
	qry->limitOption = sstmt->limitOption;

	/* transform window clauses after we have seen all window functions */
	qry->windowClause = transformWindowDefinitions(pstate,
												   pstate->p_windowdefs,
												   &qry->targetList);

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasWindowFuncs = pstate->p_hasWindowFuncs;
	qry->hasTargetSRFs = pstate->p_hasTargetSRFs;
	qry->hasAggs = pstate->p_hasAggs;

	foreach(l, sstmt->lockingClause)
	{
		transformLockingClause(pstate, qry,
							   (LockingClause *) lfirst(l), false);
	}

	assign_query_collations(pstate, qry);

	/* this must be done after collations, for reliable comparison of exprs */
	if (pstate->p_hasAggs || qry->groupClause || qry->groupingSets || qry->havingQual)
		parseCheckAggregates(pstate, qry);

	return qry;
}


/*
 * transformDeclareCursorStmt -
 *	transform a DECLARE CURSOR Statement
 *
 * DECLARE CURSOR is like other utility statements in that we emit it as a
 * CMD_UTILITY Query node; however, we must first transform the contained
 * query.  We used to postpone that until execution, but it's really necessary
 * to do it during the normal parse analysis phase to ensure that side effects
 * of parser hooks happen at the expected time.
 */
static Query *
transformDeclareCursorStmt(ParseState *pstate, DeclareCursorStmt *stmt)
{
	Query	   *result;
	Query	   *query;

	if ((stmt->options & CURSOR_OPT_SCROLL) &&
		(stmt->options & CURSOR_OPT_NO_SCROLL))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_CURSOR_DEFINITION),
		/* translator: %s is a SQL keyword */
				 errmsg("cannot specify both %s and %s",
						"SCROLL", "NO SCROLL")));

	if ((stmt->options & CURSOR_OPT_ASENSITIVE) &&
		(stmt->options & CURSOR_OPT_INSENSITIVE))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_CURSOR_DEFINITION),
		/* translator: %s is a SQL keyword */
				 errmsg("cannot specify both %s and %s",
						"ASENSITIVE", "INSENSITIVE")));

	/* Transform contained query, not allowing SELECT INTO */
	query = transformStmt(pstate, stmt->query);
	stmt->query = (Node *) query;

	/* Grammar should not have allowed anything but SELECT */
	if (!IsA(query, Query) ||
		query->commandType != CMD_SELECT)
		elog(ERROR, "unexpected non-SELECT command in DECLARE CURSOR");

	/*
	 * We also disallow data-modifying WITH in a cursor.  (This could be
	 * allowed, but the semantics of when the updates occur might be
	 * surprising.)
	 */
	if (query->hasModifyingCTE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("DECLARE CURSOR must not contain data-modifying statements in WITH")));

	/* FOR UPDATE and WITH HOLD are not compatible */
	if (query->rowMarks != NIL && (stmt->options & CURSOR_OPT_HOLD))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("DECLARE CURSOR WITH HOLD ... %s is not supported",
						LCS_asString(((RowMarkClause *)
									  linitial(query->rowMarks))->strength)),
				 errdetail("Holdable cursors must be READ ONLY.")));

	/* FOR UPDATE and SCROLL are not compatible */
	if (query->rowMarks != NIL && (stmt->options & CURSOR_OPT_SCROLL))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("DECLARE SCROLL CURSOR ... %s is not supported",
						LCS_asString(((RowMarkClause *)
									  linitial(query->rowMarks))->strength)),
				 errdetail("Scrollable cursors must be READ ONLY.")));

	/* FOR UPDATE and INSENSITIVE are not compatible */
	if (query->rowMarks != NIL && (stmt->options & CURSOR_OPT_INSENSITIVE))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_CURSOR_DEFINITION),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("DECLARE INSENSITIVE CURSOR ... %s is not valid",
						LCS_asString(((RowMarkClause *)
									  linitial(query->rowMarks))->strength)),
				 errdetail("Insensitive cursors must be READ ONLY.")));

	/* represent the command as a utility Query */
	result = makeNode(Query);
	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	return result;
}


/*
 * transformExplainStmt -
 *	transform an EXPLAIN Statement
 *
 * EXPLAIN is like other utility statements in that we emit it as a
 * CMD_UTILITY Query node; however, we must first transform the contained
 * query.  We used to postpone that until execution, but it's really necessary
 * to do it during the normal parse analysis phase to ensure that side effects
 * of parser hooks happen at the expected time.
 */
static Query *
transformExplainStmt(ParseState *pstate, ExplainStmt *stmt)
{
	Query	   *result;

	/* transform contained query, allowing SELECT INTO */
	stmt->query = (Node *) transformOptionalSelectInto(pstate, stmt->query);

	/* represent the command as a utility Query */
	result = makeNode(Query);
	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	return result;
}

/*
 * transformCreateTableAsStmt -
 *	transform a CREATE TABLE AS, SELECT ... INTO, or CREATE MATERIALIZED VIEW
 *	Statement
 *
 * As with DECLARE CURSOR and EXPLAIN, transform the contained statement now.
 */
static Query *
transformCreateTableAsStmt(ParseState *pstate, CreateTableAsStmt *stmt)
{
	Query	   *result;
	Query	   *query;

	if (compatible_mode == MYS_MODE)
		pstate->p_createastmt = true;

	/* transform contained query, not allowing SELECT INTO */
	query = transformStmt(pstate, stmt->query);
	stmt->query = (Node *) query;


	/* additional work needed for CREATE MATERIALIZED VIEW */
	if (stmt->objtype == OBJECT_MATVIEW)
	{
		/*
		 * Prohibit a data-modifying CTE in the query used to create a
		 * materialized view. It's not sufficiently clear what the user would
		 * want to happen if the MV is refreshed or incrementally maintained.
		 */
		if (query->hasModifyingCTE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialized views must not use data-modifying statements in WITH")));

		/*
		 * Check whether any temporary database objects are used in the
		 * creation query. It would be hard to refresh data or incrementally
		 * maintain it if a source disappeared.
		 */
		if (isQueryUsingTempRelation(query))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialized views must not use temporary tables or views")));

		if (is_query_using_gtt(query))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialized views must not use global temporary tables or views")));

		/*
		 * A materialized view would either need to save parameters for use in
		 * maintaining/loading the data or prohibit them entirely.  The latter
		 * seems safer and more sane.
		 */
		if (query_contains_extern_params(query))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialized views may not be defined using bound parameters")));

		/*
		 * For now, we disallow unlogged materialized views, because it seems
		 * like a bad idea for them to just go to empty after a crash. (If we
		 * could mark them as unpopulated, that would be better, but that
		 * requires catalog changes which crash recovery can't presently
		 * handle.)
		 */
		if (stmt->into->rel->relpersistence == RELPERSISTENCE_UNLOGGED)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialized views cannot be unlogged")));

		/*
		 * At runtime, we'll need a copy of the parsed-but-not-rewritten Query
		 * for purposes of creating the view's ON SELECT rule.  We stash that
		 * in the IntoClause because that's where intorel_startup() can
		 * conveniently get it from.
		 */
		stmt->into->viewQuery = (Node *) copyObject(query);

		/* Begin - ReqID:SRS-SECURITY-TABLESPACE BUG#Z7282 */
		/* Objects stored in an encrypted tablespace must use encryption. */
		stmt->into->options = InheritEncryptFromTablespace(stmt->into->options, stmt->into->tableSpaceName);
		/* End - ReqID:SRS-SECURITY-TABLESPACE BUG#Z7282 */
	}

	/* represent the command as a utility Query */
	result = makeNode(Query);
	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	return result;
}

/*
 * transform a CallStmt
 */
static Query *
transformCallStmt(ParseState *pstate, CallStmt *stmt)
{
	List	   *targs;
	ListCell   *lc;
	Node	   *node;
	FuncExpr   *fexpr;
	HeapTuple	proctup;
	Datum		proargmodes;
	bool		isNull;
	List	   *outargs = NIL;
	Query	   *result;
	/* Begin - BUG#M0000376 */
	bool		need_reset_p_oraparammode = false;

	if (enable_get_oraparam_mode && stmt->funccall->args != NIL)
	{
		/*
		 * To get the placeholder mode, we do transform the FuncCall expr.
		 *
		 * If enable_get_oraparam_mode is on, copy stmt->funccall->args to stmt->funccall->rawargs.
		 * stmt->funccall->rawargs are the list of arguments without doing transform.
		 * because only after find the procedure and get the arguments mode,
		 * we can get parameters by doing transform stmt->funccall->rawargs.
		 *
		 * IF in a nested procedure call, a procedure argument is another function call,
		 * e.g., proc1(func1(:x), :y); the :x and :y are placeholder,
		 * when transform func1, we already set argument modes for the function func1,
		 * then transform proc1, we don't need to transform func1.
		 *
		 * If there aren't nested procedure call, just do tranform.
		 */
		if (pstate->p_not_set_oraparammode &&
			pstate->p_oraparammode != ' ')
		{
			pstate->p_not_transform_funcCall = true;
		}
		else
		{
			stmt->funccall->rawargs = list_copy_deep(stmt->funccall->args);
			if (!pstate->p_not_set_oraparammode)
			{
				pstate->p_not_set_oraparammode = true;
				pstate->p_oraparammode = ' ';
				need_reset_p_oraparammode = true;
			}
		}
	}

	/* End - BUG#M0000376 */

	/*
	 * First, do standard parse analysis on the procedure call and its
	 * arguments, allowing us to identify the called procedure.
	 */
	targs = NIL;
	foreach(lc, stmt->funccall->args)
	{
		targs = lappend(targs, transformExpr(pstate,
											 (Node *) lfirst(lc),
											 EXPR_KIND_CALL_ARGUMENT));
	}

	PG_TRY();
	{
		node = ParseFuncOrColumn(pstate,
								 stmt->funccall->funcname,
								 targs,
								 pstate->p_last_srf,
								 stmt->funccall,
								 true,
								 stmt->funccall->location);
	}
	/* Begin - BUG#M0000376 */
	PG_CATCH();
	{
		if (enable_get_oraparam_mode && stmt->funccall->args != NIL)
		{
			pstate->p_not_set_oraparammode = false;
			pstate->p_oraparammode = ' ';
			list_free_deep(stmt->funccall->rawargs);
			stmt->funccall->rawargs = NIL;
			pstate->p_not_transform_funcCall = false;
		}
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (enable_get_oraparam_mode
		&& stmt->funccall->args != NIL)
	{
		list_free_deep(stmt->funccall->rawargs);
		stmt->funccall->rawargs = NIL;
		pstate->p_oraparammode = ' ';
		pstate->p_not_transform_funcCall = false;

		if (need_reset_p_oraparammode)
			pstate->p_not_set_oraparammode = false;
	}
	/* End - BUG#M0000376 */

	assign_expr_collations(pstate, node);

	fexpr = castNode(FuncExpr, node);

	/* Begin - SRS-PLSQL-SUBPROC */
	if (!FUNC_EXPR_FROM_PG_PROC(fexpr->function_from))
	{
		stmt->funcexpr = fexpr;
		stmt->outargs = get_internal_function_outargs(fexpr);
		goto SKIP_PG_PROC_CHECK;
	}
	/* End - SRS-PLSQL-SUBPROC */

	proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(fexpr->funcid));
	if (!HeapTupleIsValid(proctup))
		elog(ERROR, "cache lookup failed for function %u", fexpr->funcid);

	/*
	 * Expand the argument list to deal with named-argument notation and
	 * default arguments.  For ordinary FuncExprs this'd be done during
	 * planning, but a CallStmt doesn't go through planning, and there seems
	 * no good reason not to do it here.
	 */
	fexpr->args = expand_function_arguments(fexpr->args,
											true,
											fexpr->funcresulttype,
											proctup);

	/* Fetch proargmodes; if it's null, there are no output args */
	proargmodes = SysCacheGetAttr(PROCOID, proctup,
								  Anum_pg_proc_proargmodes,
								  &isNull);
	if (!isNull)
	{
		/*
		 * Split the list into input arguments in fexpr->args and output
		 * arguments in stmt->outargs.  INOUT arguments appear in both lists.
		 */
		ArrayType  *arr;
		int			numargs;
		char	   *argmodes;
		List	   *inargs;
		int			i;

		arr = DatumGetArrayTypeP(proargmodes);	/* ensure not toasted */
		numargs = list_length(fexpr->args);
		if (ARR_NDIM(arr) != 1 ||
			ARR_DIMS(arr)[0] != numargs ||
			ARR_HASNULL(arr) ||
			ARR_ELEMTYPE(arr) != CHAROID)
			elog(ERROR, "proargmodes is not a 1-D char array of length %d or it contains nulls",
				 numargs);
		argmodes = (char *) ARR_DATA_PTR(arr);

		inargs = NIL;
		i = 0;
		foreach(lc, fexpr->args)
		{
			Node	   *n = lfirst(lc);

			switch (argmodes[i])
			{
				case PROARGMODE_IN:
				case PROARGMODE_VARIADIC:
					inargs = lappend(inargs, n);
					break;
				case PROARGMODE_OUT:
					outargs = lappend(outargs, n);
					break;
				case PROARGMODE_INOUT:
					inargs = lappend(inargs, n);
					outargs = lappend(outargs, copyObject(n));
					break;
				default:
					/* note we don't support PROARGMODE_TABLE */
					elog(ERROR, "invalid argmode %c for procedure",
						 argmodes[i]);
					break;
			}
			i++;
		}

		/* Begin - Req: SRS-MYSQL-PROC */
		if (compatible_mode == MYS_MODE)
		{
			i = 0;

			foreach(lc, stmt->funccall->args)
			{
				Node	   *n = lfirst(lc);

				if (IsA(n, VarRef))
				{
					VarRef *ref = (VarRef *) n;

					if (ref->varkind == SYSTEM_VAR && argmodes[i] != PROARGMODE_IN)
					{
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("OUT or INOUT argument %d for routine %s is "
										"not a variable or NEW pseudo-variable in "
										"BEFORE trigger", i, NameListToString(stmt->funccall->funcname))));
					}
				}

				i++;
			}
		}
		/* End - Req: SRS-MYSQL-PROC */

		/* Begin - BUG#Z196: procedure with out arguments supports overload */
		/*
		 * If compatible_mode is ORA_MODE and the procedure language is PLISQL,
		 * the funcexpr->args should be all the args including out argment
		 */
		if (!(ORA_MODE == compatible_mode &&
			((Form_pg_proc) GETSTRUCT(proctup))->prolang == LANG_PLISQL_OID))
		/* End - BUG#Z196: procedure with out arguments supports overload */
			fexpr->args = inargs;
	}

	stmt->funcexpr = fexpr;
	stmt->outargs = outargs;

	ReleaseSysCache(proctup);
	/* End - SRS-PLSQL-SUBPROC */

/* Begin - SRS-PLSQL-SUBPROC */
SKIP_PG_PROC_CHECK:
/* Begin - SRS-PLSQL-SUBPROC */

	/* represent the command as a utility Query */
	result = makeNode(Query);
	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	return result;
}

/*
 * Produce a string representation of a LockClauseStrength value.
 * This should only be applied to valid values (not LCS_NONE).
 */
const char *
LCS_asString(LockClauseStrength strength)
{
	switch (strength)
	{
		case LCS_NONE:
			Assert(false);
			break;
		case LCS_FORKEYSHARE:
			return "FOR KEY SHARE";
		case LCS_FORSHARE:
			return "FOR SHARE";
		case LCS_FORNOKEYUPDATE:
			return "FOR NO KEY UPDATE";
		case LCS_FORUPDATE:
			return "FOR UPDATE";
	}
	return "FOR some";			/* shouldn't happen */
}

/*
 * Check for features that are not supported with FOR [KEY] UPDATE/SHARE.
 *
 * exported so planner can check again after rewriting, query pullup, etc
 */
void
CheckSelectLocking(Query *qry, LockClauseStrength strength)
{
	Assert(strength != LCS_NONE);	/* else caller error */

	if (qry->setOperations)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with UNION/INTERSECT/EXCEPT",
						LCS_asString(strength))));
	if (qry->distinctClause != NIL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with DISTINCT clause",
						LCS_asString(strength))));
	if (qry->groupClause != NIL || qry->groupingSets != NIL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with GROUP BY clause",
						LCS_asString(strength))));
	if (qry->havingQual != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with HAVING clause",
						LCS_asString(strength))));
	if (qry->hasAggs)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with aggregate functions",
						LCS_asString(strength))));
	if (qry->hasWindowFuncs)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with window functions",
						LCS_asString(strength))));
	if (qry->hasTargetSRFs)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/*------
		  translator: %s is a SQL row locking clause such as FOR UPDATE */
				 errmsg("%s is not allowed with set-returning functions in the target list",
						LCS_asString(strength))));
}

/*
 * Transform a FOR [KEY] UPDATE/SHARE clause
 *
 * This basically involves replacing names by integer relids.
 *
 * NB: if you need to change this, see also markQueryForLocking()
 * in rewriteHandler.c, and isLockedRefname() in parse_relation.c.
 */
static void
transformLockingClause(ParseState *pstate, Query *qry, LockingClause *lc,
					   bool pushedDown)
{
	List	   *lockedRels = lc->lockedRels;
	ListCell   *l;
	ListCell   *rt;
	Index		i;
	LockingClause *allrels;

	/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
	checkForUpdateInSubqury(pstate, lc);
	/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

	CheckSelectLocking(qry, lc->strength);

	/* make a clause we can pass down to subqueries to select all rels */
	allrels = makeNode(LockingClause);
	allrels->lockedRels = NIL;	/* indicates all rels */
	allrels->strength = lc->strength;
	allrels->waitPolicy = lc->waitPolicy;
	allrels->waitInteger = lc->waitInteger;	/* BUG#M0000470:FOR UPDATE WAIT integer */

	if (lockedRels == NIL)
	{
		/*
		 * Lock all regular tables used in query and its subqueries.  We
		 * examine inFromCl to exclude auto-added RTEs, particularly NEW/OLD
		 * in rules.  This is a bit of an abuse of a mostly-obsolete flag, but
		 * it's convenient.  We can't rely on the namespace mechanism that has
		 * largely replaced inFromCl, since for example we need to lock
		 * base-relation RTEs even if they are masked by upper joins.
		 */
		i = 0;
		foreach(rt, qry->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

			++i;
			if (!rte->inFromCl)
				continue;
			switch (rte->rtekind)
			{
				case RTE_RELATION:
					applyLockingClause(qry, i, lc->strength, lc->waitPolicy,
									   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
									   NIL,/* BUG#M0000470:FOR UPDATE WAIT integer */
									   pushedDown);
					rte->requiredPerms |= ACL_SELECT_FOR_UPDATE;
					break;
				case RTE_SUBQUERY:
					applyLockingClause(qry, i, lc->strength, lc->waitPolicy,
									   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
									   NIL,/* BUG#M0000470:FOR UPDATE WAIT integer */
									   pushedDown);

					/*
					 * FOR UPDATE/SHARE of subquery is propagated to all of
					 * subquery's rels, too.  We could do this later (based on
					 * the marking of the subquery RTE) but it is convenient
					 * to have local knowledge in each query level about which
					 * rels need to be opened with RowShareLock.
					 */
					transformLockingClause(pstate, rte->subquery,
										   allrels, true);
					break;
				default:
					/* ignore JOIN, SPECIAL, FUNCTION, VALUES, CTE RTEs */
					break;
			}
		}
	}
	else
	{
		/*
		 * Lock just the named tables.  As above, we allow locking any base
		 * relation regardless of alias-visibility rules, so we need to
		 * examine inFromCl to exclude OLD/NEW.
		 */
		/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
		List       *updaterteidlist = NIL;
		/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

		foreach(l, lockedRels)
		{
			RangeVar   *thisrel = (RangeVar *) lfirst(l);
			/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
			char 		*colOrRelname = NULL;
			bool		matched = false;
			List       *lockedColumns = NIL;
			/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

			if (ORA_MODE != compatible_mode)
			{
				/* For simplicity we insist on unqualified alias names here */
				if (thisrel->catalogname || thisrel->schemaname)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
					/*------
					  translator: %s is a SQL row locking clause such as FOR UPDATE */
							 errmsg("%s must specify unqualified relation names",
									LCS_asString(lc->strength)),
							 parser_errposition(pstate, thisrel->location)));
			}

			/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
			/* unqualified names may be table name or column name */
			if (ORA_MODE == compatible_mode)
			{
				colOrRelname = thisrel->relname;

				/*
				 * The lockedColumns is locked column name in the FOR UPDATE OF clause.
				 * It is used when pg_dump dumps a view definiation.
				 */
				lockedColumns = lcons(makeString(thisrel->relname), lockedColumns);
				if (thisrel->schemaname)
					lockedColumns = lcons(makeString(thisrel->schemaname), lockedColumns);
				if (thisrel->catalogname)
					lockedColumns = lcons(makeString(thisrel->catalogname), lockedColumns);
			}
			/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

			i = 0;
			foreach(rt, qry->rtable)
			{
				RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);
				char	   *rtename;
				/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
				bool		isColumn = false;
				Oid			lockrelid = InvalidOid;
				/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

				++i;
				if (!rte->inFromCl)
					continue;

				/*
				 * A join RTE without an alias is not visible as a relation
				 * name and needs to be skipped (otherwise it might hide a
				 * base relation with the same name), except if it has a USING
				 * alias, which *is* visible.
				 */
				if (rte->rtekind == RTE_JOIN && rte->alias == NULL)
				{
					if (rte->join_using_alias == NULL)
						continue;
					rtename = rte->join_using_alias->aliasname;
				}
				else
					rtename = rte->eref->aliasname;

				/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
				if (ORA_MODE == compatible_mode)
				{
					if (!thisrel->catalogname && !thisrel->schemaname)
					{
						/*
						 * Supports "FOR UPDATE OF table.column" and
						 * "FOR UPDATE OF schema.table.column".
						 * Check if it is a column name or a table name.
						 * The columns in the OF clause only indicate which
						 * table or view rows are locked.
						 */
						if (pg_strcasecmp(rtename, colOrRelname) == 0)
							matched = true;
						else if (rte->rtekind == RTE_RELATION &&
							get_attnum(rte->relid, colOrRelname) != InvalidAttrNumber)
						{
							ListCell	*l;
							Oid			columnrelid;

							/*
							 * If there are multiple tables, they cannot have the same column names
							 * that appear in the FOR UPDATE OF column statement.
							 */
							columnrelid = rte->relid;
							foreach(l, qry->rtable)
							{
								RangeTblEntry *tmprte = (RangeTblEntry *) lfirst(l);
								if (tmprte->relid == columnrelid)
									continue;

								if (tmprte->rtekind == RTE_RELATION &&
									get_attnum(tmprte->relid, colOrRelname) != InvalidAttrNumber)
									ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("column ambiguously defined"),
										 parser_errposition(pstate, thisrel->location)));
							}
							matched = true;
							isColumn = true;
						}
						else if (rte->rtekind == RTE_SUBQUERY)
						{
							Query 		*subqry = rte->subquery;
							ListCell	*lc;

							foreach(lc, subqry->targetList)
							{
								TargetEntry *te = (TargetEntry *)lfirst(lc);

								if (pg_strcasecmp(te->resname, colOrRelname) == 0)
								{
									if (te->resorigtbl == InvalidOid)
										ereport(ERROR,
												(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
												 errmsg("virtual column not allowed here"),
												 parser_errposition(pstate, thisrel->location)));

									lockrelid = te->resorigtbl;
									matched = true;
									isColumn = true;
									break;
								}
							}
						}
					}
					else if (rte->rtekind == RTE_RELATION &&
						get_attnum(rte->relid, colOrRelname) != InvalidAttrNumber)
					{
						/*
						 * It may be table.column or schema.table.column.
						 * The columns in the OF clause only indicate which
						 * table or view rows are locked.
						 */
						if (thisrel->schemaname != NULL &&
							pg_strcasecmp(thisrel->schemaname, rtename) == 0)
						{
							if (thisrel->catalogname == NULL)
							{
								matched = true;
								isColumn = true;
							}
							else if (pg_strcasecmp(thisrel->catalogname, get_namespace_name(get_rel_namespace(rte->relid))) == 0)
							{
								matched = true;
								isColumn = true;
							}
						}
					}
				}
				/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

				if ((ORA_MODE != compatible_mode && strcmp(rtename, thisrel->relname) == 0) ||
					(ORA_MODE == compatible_mode && matched))
				{
					switch (rte->rtekind)
					{
						case RTE_RELATION:
							/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
							if (ORA_MODE != compatible_mode)
							{
							/* End - BUG#M0000470: SELECT FOR UPDATE OF column */
								applyLockingClause(qry, i, lc->strength,
												   lc->waitPolicy,
												   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   NIL,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   pushedDown);
								rte->requiredPerms |= ACL_SELECT_FOR_UPDATE;
							/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
							}
							else
							{
								if (!isColumn)
								{
									if (updaterteidlist != NIL)
										ereport(ERROR,
												(errcode(ERRCODE_SYNTAX_ERROR),
										/*------
										  translator: %s is a SQL row locking clause such as FOR UPDATE */
												 errmsg("%s can't specify unqualified relation names and column name together",
														LCS_asString(lc->strength)),
												 parser_errposition(pstate, thisrel->location)));

									applyLockingClause(qry, i, lc->strength,
													   lc->waitPolicy,
													   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
													   NIL,/* BUG#M0000470:FOR UPDATE WAIT integer */
													   pushedDown);
									rte->requiredPerms |= ACL_SELECT_FOR_UPDATE;
								}
								else if (updaterteidlist == NIL || !list_member_oid(updaterteidlist, rte->relid))
								{
									updaterteidlist = lappend_oid(updaterteidlist, rte->relid);
									applyLockingClause(qry, i, lc->strength,
													   lc->waitPolicy,
													   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
													   lockedColumns,/* BUG#M0000470:FOR UPDATE WAIT integer */
													   pushedDown);
									rte->requiredPerms |= ACL_SELECT_FOR_UPDATE;
									list_free(lockedColumns);
								}
							}
							/* End - BUG#M0000470: SELECT FOR UPDATE OF column */
							break;
						case RTE_SUBQUERY:
							/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
							if (ORA_MODE != compatible_mode ||
								!isColumn)
							{
							/* End - BUG#M0000470: SELECT FOR UPDATE OF column */
								applyLockingClause(qry, i, lc->strength,
												   lc->waitPolicy,
												   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   NIL,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   pushedDown);
								/* see comment above */
								transformLockingClause(pstate, rte->subquery,
													   allrels, true);
							/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
							}
							else if (updaterteidlist == NIL || !list_member_oid(updaterteidlist, rte->relid))
							{
								updaterteidlist = lappend_oid(updaterteidlist, lockrelid);
								applyLockingClause(qry, i, lc->strength,
												   lc->waitPolicy,
												   lc->waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   lockedColumns,/* BUG#M0000470:FOR UPDATE WAIT integer */
												   pushedDown);
								/* find the rte in subquery, and then set required access permissions.*/
								applyLockingTable(rte->subquery, lockrelid, lc, pushedDown);
								list_free(lockedColumns);
							}
							/* End - BUG#M0000470: SELECT FOR UPDATE OF column */
							break;
						case RTE_JOIN:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to a join",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;
						case RTE_FUNCTION:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to a function",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;
						case RTE_TABLEFUNC:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to a table function",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;
						case RTE_VALUES:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to VALUES",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;
						case RTE_CTE:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to a WITH query",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;
						case RTE_NAMEDTUPLESTORE:
							ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							/*------
							  translator: %s is a SQL row locking clause such as FOR UPDATE */
									 errmsg("%s cannot be applied to a named tuplestore",
											LCS_asString(lc->strength)),
									 parser_errposition(pstate, thisrel->location)));
							break;

							/* Shouldn't be possible to see RTE_RESULT here */

						default:
							elog(ERROR, "unrecognized RTE type: %d",
								 (int) rte->rtekind);
							break;
					}
					break;		/* out of foreach loop */
				}
			}
			/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
			if (ORA_MODE == compatible_mode && !matched)
			{
				if (!thisrel->catalogname && !thisrel->schemaname)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
					/*------
					  translator: %s is a SQL row locking clause such as FOR UPDATE */
							 errmsg("\"%s\" invalid identifier",
									colOrRelname),
							 parser_errposition(pstate, thisrel->location)));
				if (thisrel->catalogname)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
					/*------
					  translator: %s is a SQL row locking clause such as FOR UPDATE */
							 errmsg("\"%s\".\"%s\".\"%s\" invalid identifier",
									thisrel->catalogname, thisrel->schemaname, colOrRelname),
							 parser_errposition(pstate, thisrel->location)));
				if (thisrel->schemaname)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
					/*------
					  translator: %s is a SQL row locking clause such as FOR UPDATE */
							 errmsg("\"%s\".\"%s\" invalid identifier",
									thisrel->schemaname, colOrRelname),
							 parser_errposition(pstate, thisrel->location)));
			}
			/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

			if (rt == NULL &&
				ORA_MODE != compatible_mode /* BUG#M0000470: SELECT FOR UPDATE OF column */)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_TABLE),
				/*------
				  translator: %s is a SQL row locking clause such as FOR UPDATE */
						 errmsg("relation \"%s\" in %s clause not found in FROM clause",
								thisrel->relname,
								LCS_asString(lc->strength)),
						 parser_errposition(pstate, thisrel->location)));
		}
		/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
		if (updaterteidlist != NIL)
			list_free(updaterteidlist);
		/* End - BUG#M0000470: SELECT FOR UPDATE OF column */
	}
}

/*
 * Record locking info for a single rangetable item
 */
void
applyLockingClause(Query *qry, Index rtindex,
				   LockClauseStrength strength, LockWaitPolicy waitPolicy,
				   int32 waitInteger,/* BUG#M0000470:FOR UPDATE WAIT integer */
				   List *lockedColumns,/* BUG#M0000470:FOR UPDATE WAIT integer */
				   bool pushedDown)
{
	RowMarkClause *rc;

	Assert(strength != LCS_NONE);	/* else caller error */

	/* If it's an explicit clause, make sure hasForUpdate gets set */
	if (!pushedDown)
		qry->hasForUpdate = true;

	/* Check for pre-existing entry for same rtindex */
	if ((rc = get_parse_rowmark(qry, rtindex)) != NULL)
	{
		/*
		 * If the same RTE is specified with more than one locking strength,
		 * use the strongest.  (Reasonable, since you can't take both a shared
		 * and exclusive lock at the same time; it'll end up being exclusive
		 * anyway.)
		 *
		 * Similarly, if the same RTE is specified with more than one lock
		 * wait policy, consider that NOWAIT wins over SKIP LOCKED, which in
		 * turn wins over waiting for the lock (the default).  This is a bit
		 * more debatable but raising an error doesn't seem helpful. (Consider
		 * for instance SELECT FOR UPDATE NOWAIT from a view that internally
		 * contains a plain FOR UPDATE spec.)  Having NOWAIT win over SKIP
		 * LOCKED is reasonable since the former throws an error in case of
		 * coming across a locked tuple, which may be undesirable in some
		 * cases but it seems better than silently returning inconsistent
		 * results.
		 *
		 * And of course pushedDown becomes false if any clause is explicit.
		 */
		rc->strength = Max(rc->strength, strength);
		rc->waitPolicy = Max(rc->waitPolicy, waitPolicy);
		rc->waitInteger = waitInteger; /* BUG#M0000470:FOR UPDATE WAIT integer */
		rc->lockedColumns = list_copy(lockedColumns); /* BUG#M0000470:FOR UPDATE WAIT integer */
		rc->pushedDown &= pushedDown;
		return;
	}

	/* Make a new RowMarkClause */
	rc = makeNode(RowMarkClause);
	rc->rti = rtindex;
	rc->strength = strength;
	rc->waitPolicy = waitPolicy;
	rc->waitInteger = waitInteger; /* BUG#M0000470:FOR UPDATE WAIT integer */
	rc->lockedColumns = list_copy(lockedColumns); /* BUG#M0000470:FOR UPDATE WAIT integer */
	rc->pushedDown = pushedDown;
	qry->rowMarks = lappend(qry->rowMarks, rc);
}

/*
 * Coverage testing for raw_expression_tree_walker().
 *
 * When enabled, we run raw_expression_tree_walker() over every DML statement
 * submitted to parse analysis.  Without this provision, that function is only
 * applied in limited cases involving CTEs, and we don't really want to have
 * to test everything inside as well as outside a CTE.
 */
#ifdef RAW_EXPRESSION_COVERAGE_TEST

static bool
test_raw_expression_coverage(Node *node, void *context)
{
	if (node == NULL)
		return false;
	return raw_expression_tree_walker(node,
									  test_raw_expression_coverage,
									  context);
}

#endif							/* RAW_EXPRESSION_COVERAGE_TEST */

/* Begin - ReqID:SRS-PLSQL-TYPE */
/*
 * transformUpdateRowTargetList
 * It is UPDATE tablename SET ROW = value,.. colN = value..[WHERE ..] STATEMENT
 */
static List *
transformUpdateRowTargetList(ParseState *pstate, List *origTlist)
{
	List	   *tlist = NIL;
	RangeTblEntry *target_rte;
	ListCell   *tl;
	int			nattrs;
	int			nvalues;
	int			attrno;
	int			location;
	ResTarget  *origTarget;

	nvalues = list_length(origTlist);
	origTarget = (ResTarget *) linitial(origTlist);
	location = ((ResTarget *)lfirst_node(ResTarget, list_head(origTlist)))->location;

	/*
	 * It is UPDATE tablename SET ROW = record_var[WHERE ..] STATEMENT.
	 * do some checkings, only one SET clause is allowed if ROW is used.
	 * not support functions that return a RECORD type,
	 * not support using the EXECUTE IMMEDIATE statement
	 */
	 if (nvalues != 1)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("too many values"),
				 parser_errposition(pstate,
									location)));

	if (nodeTag(origTarget->val) == T_FuncCall ||
		nodeTag(origTarget->val) == T_OraParamRef ||
		nodeTag(origTarget->val) == T_ParamRef)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("not support"),
				 parser_errposition(pstate,
									location)));

	/* Transform the target list */
	tlist = transformRowTargetList(pstate, origTlist,
								EXPR_KIND_UPDATE_SOURCE);

	location = ((ResTarget *)lfirst_node(ResTarget, list_head(origTlist)))->location;

	nattrs = RelationGetNumberOfAttributes(pstate->p_target_relation);
	nvalues = list_length(tlist);

	if (nattrs > nvalues)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("not enough values"),
				 parser_errposition(pstate,
									location)));

	else if (nattrs < nvalues)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("too many values"),
				 parser_errposition(pstate,
									location)));

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_next_resno <= RelationGetNumberOfAttributes(pstate->p_target_relation))
		pstate->p_next_resno = RelationGetNumberOfAttributes(pstate->p_target_relation) + 1;

	/* Prepare non-junk columns for assignment to target table */
	target_rte = pstate->p_target_nsitem->p_rte;

	attrno = 0;
	foreach(tl, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		Form_pg_attribute att;

		att = TupleDescAttr(pstate->p_target_relation->rd_att, attrno);
		updateTargetListEntry(pstate, tle, NameStr(att->attname),
							  attrno + 1,
							  NIL,
							  location);

		/* Mark the target column as requiring update permissions */
		target_rte->updatedCols = bms_add_member(target_rte->updatedCols,
												 attrno - FirstLowInvalidHeapAttributeNumber);
		attrno++;

	}

	return tlist;
}

/*
 *  transformIvyUpdateTargetList
 *  handle SET clause in UPDATE STATEENT
 *  the difference between transformUpdateTargetList and transformIvyUpdateTargetList is:
 *  transformIvyUpdateTargetList supports UPDATE tablename SET ROW = record_var[WHERE ..] statement,
 *  record_var is a record variable that represents a row.
 *  so far, ROW keyword is col_name_keyword, ROW can be column, table, etc names.
 *  UPDATE tablename SET ROW = value, colN = value..[WHERE ..] STATEMENT and
 *  UPDATE tablename SET ROW = record_var[WHERE ..] STATEMENT are both supported.
 */
static List *
transformIvyUpdateTargetList(ParseState *pstate, List *origTlist)
{
	List	   *tlist = NIL;
	ListCell   *o_target;
	int			target_has_row = false;
	bool		cols_has_row = false;
	int			col_is_complex = false;
	int			row_no;
	Oid		    attrrowtype;

	target_has_row = 0;
	row_no = 0;

	/* Check if the target name is a "row" */
	foreach(o_target, origTlist)
	{
		ResTarget  *origTarget = (ResTarget *) lfirst(o_target);
		int 		attrno;

		/* Begin - ReqID:SRS-SQL-ROWID */
		if (strcmp(origTarget->name, "rowid") == 0)
			elog(ERROR, "cannot specify system column name \"%s\"", origTarget->name);
		/* End - ReqID:SRS-SQL-ROWID */

		/* Check if there is a column named "row" */
		if (pg_strcasecmp(origTarget->name, "row") == 0)
		{
			target_has_row = true;
			attrno = attnameAttNum(pstate->p_target_relation,
								   origTarget->name, true);
			if (attrno != InvalidAttrNumber)
			{
				cols_has_row = true;
				attrrowtype = attnumTypeId(pstate->p_target_relation, attrno);

				if (ISCOMPLEX(attrrowtype))
					col_is_complex = true;
			}
			break;
		}
		row_no++;
	}

	/*
	 * target_has_row is false,
	 * there is not SET ROW = xx clause, call transformUpdateTargetList
	 * to transform the targetlist.
	 */
	if (!target_has_row)
		return transformUpdateTargetList(pstate, origTlist);

	/*
	 * cols_has_row is false, there is not column named "row".
	 * the UPDATE statment is UPDATE tablename SET ROW = record_var[WHERE ..] statement.
	 */
	if (!cols_has_row)
		tlist = transformUpdateRowTargetList(pstate, origTlist);
	else
	{
		TargetEntry *row_tle;
		Oid			value_type;

		tlist = transformTargetList(pstate, origTlist,
									EXPR_KIND_UPDATE_SOURCE);

		/* get the transformed value of the target row */
		row_tle = (TargetEntry *) list_nth(tlist, row_no);
		value_type = exprType((Node *) (row_tle->expr));

		if (ISCOMPLEX(value_type))
		{
			if (!col_is_complex)
				tlist = transformUpdateRowTargetList(pstate, origTlist);
			else
			{
				if (attrrowtype == value_type)
					tlist = transformUpdateTargetList(pstate, origTlist);
				else
					tlist = transformUpdateRowTargetList(pstate, origTlist);
			}
		}
		else
		{
			/* It is UPDATE tablename SET ROW = value,.. colN = value..[WHERE ..] STATEMENT  */
			tlist = transformUpdateTargetList(pstate, origTlist);
		}
	}

	return tlist;
}
/* End - ReqID:SRS-PLSQL-TYPE */

/* Begin - BUG#M0000470: SELECT FOR UPDATE OF column */
/*
 * Check if it is allowed FOR UPDATE clause in a subqury.
 * It is allowed if set the allow_forupdate_in_subquery to true,
 * it is not allowed if if set the allow_forupdate_in_subquery to false.
 */
static void
checkForUpdateInSubqury(ParseState *pstate, LockingClause *lc)
{
	if (!allow_forupdate_in_subquery &&
		pstate->parentParseState != NULL &&
		lc->strength == LCS_FORUPDATE)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("SELECT FOR UPATE clause only appears in a top-level SELECT statement, not in subqueries"),
				 parser_errposition(pstate,
									exprLocation((Node *) lc))));
}

/*
 * In the SELECT xx FROM subquery FOR UPATE OF column statement,
 * find the rte in subquery, and then set required access permission
 * ACL_SELECT_FOR_UPDATE.
 */
static void
applyLockingTable(Query *qry, Oid relid,
				LockingClause *lockclause,
				bool pushedDown)
{
	int			i;
	ListCell	*l;
	bool		foundrel = false;

	i = 0;
	foreach(l, qry->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(l);

		++i;
		if (!rte->inFromCl)
			continue;

		if (rte->rtekind == RTE_RELATION)
		{
			if (rte->relid == relid)
			{
				applyLockingClause(qry, i, lockclause->strength,
								   lockclause->waitPolicy, lockclause->waitInteger,
								   lockclause->lockedRels,
								   pushedDown);
				rte->requiredPerms |= ACL_SELECT_FOR_UPDATE;

				foundrel = true;
				break;
			}
		}
		else if (rte->rtekind == RTE_SUBQUERY)
		{
			applyLockingTable(rte->subquery, relid, lockclause, pushedDown);
			foundrel = true;
			break;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("%s cannot be applied",
							LCS_asString(lockclause->strength))));
	}

	if (!foundrel)
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("cannot find the relation")));

	return;
}
/* End - BUG#M0000470: SELECT FOR UPDATE OF column */

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/*
 * set_auto_increment_func_node :make insert auto_increment col and make params:
 * reloid : table oid
 * attnum : table auto_increment col num
 * colnotnull : auto_increment is not null flag
 * valuenullordefault : 
 * 1 : insert into .. (null)    :AUTO_INCREMENT_INSERT_NULL       (1)
 * 2 : insert into .. (default) :AUTO_INCREMENT_INSERT_DEFAULT    (2)
 * 3 : insert into .. (0)       :AUTO_INCREMENT_INSERT_ZERO       (3)
 * value : insert auto_increment value
 */
static Expr *
set_auto_increment_func_node(ParseState *pstate, Expr* expr, int attno)
{
	FuncExpr *fexpr;
	Form_pg_attribute attr;

	attr = TupleDescAttr(pstate->p_target_relation->rd_att, attno - 1);
	if (attr->attidentity == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
	{
		Const *reloidNode = NULL;
		Const *attnumNode = NULL;
		Const *colnotnullNode = NULL;
		Const *NullordefaultNode = NULL;
		Const *valueNode = NULL;

		/* insert value: (NULL) . (default) . (0) 
		*  null col:
		* ---------------------------------------------------------------------------
		* | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
		* | (NULL)                   NULL                           NULL
		* | (default)                NULL                           NULL
		* | (0)                       0                            lastval ++
		* ---------------------------------------------------------------------------
		* ===========================================================================
		*  not null col:
		* ---------------------------------------------------------------------------
		* | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
		* | (NULL)                   NULL                          lastval ++
		* | (default)                 0                            lastval ++
		* | (0)                       0                            lastval ++
		* ---------------------------------------------------------------------------
		*/
		if (IsA((expr), Const) && ((Const *)expr)->constisnull)
		{
			NullordefaultNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_INSERT_NULL),
										  false, FLOAT8PASSBYVAL);

			valueNode = makeConst(INT8OID, -1, InvalidOid,
								  sizeof(int64),
								  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
								  false, FLOAT8PASSBYVAL);
		}
		else if (IsA((expr), SetToDefault))
		{
			NullordefaultNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_INSERT_DEFAULT), 
										  false, FLOAT8PASSBYVAL);

			valueNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
								  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
								  false, FLOAT8PASSBYVAL);
		}
		else
		{
			if (exprType((Node *)expr) == FLOAT4OID || exprType((Node *)expr) == FLOAT8OID ||
				 exprType((Node *)expr) == NUMERICOID)
			{
				return expr;
			}

			NullordefaultNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_INSERT_ZERO),
										  false, FLOAT8PASSBYVAL);
		}

		reloidNode     = makeConst(OIDOID, -1, InvalidOid, sizeof(Oid),
								   ObjectIdGetDatum(pstate->p_target_relation->rd_id),
								   false, true);
		attnumNode     = makeConst(INT2OID, -1, InvalidOid, sizeof(Oid),
								   Int16GetDatum(attr->attnum),
								   false, true);
		colnotnullNode = makeConst(BOOLOID, -1, InvalidOid, 1,
								   BoolGetDatum(attr->attnotnull),
								   false, true);

		/* convert the original expression node to a cast numeric node*/
		valueNode = (Const *) coerce_to_target_type(pstate,
										  valueNode ? (Node *) valueNode : (Node *) expr,
										  valueNode ? exprType((Node *) valueNode) : exprType((Node *) expr),
										  NUMERICOID, -1,
										  COERCION_EXPLICIT,
										  COERCE_EXPLICIT_CAST,
										  -1);

		/* create funcexpr node : mys_set_insertval function  */
		fexpr = makeFuncExpr(AUTO_INCREMENT_SET_VALUE_FUNC_OID,
							 INT8OID,
							 list_make5(reloidNode, attnumNode, colnotnullNode, NullordefaultNode, valueNode),
							 InvalidOid,
							 InvalidOid,
							 COERCE_EXPLICIT_CALL);

		expr = (Expr*)fexpr;
	}
	return expr;
}

/*
 * update_set_auto_increment_func_node :make update auto_increment col and make params:
 * modify SetToDefault node and null node
 *
 * update value: (NULL) . (default) . (0)
 *  null col:
 * ---------------------------------------------------------------------------
 * | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
 * | (NULL)                   NULL                           NULL
 * | (default)                NULL                           NULL
 * | (0)                       0                             0
 * ---------------------------------------------------------------------------
 * ===========================================================================
 *  not null col:
 * ---------------------------------------------------------------------------
 * | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
 * | (NULL)                    0                            ERR
 * | (default)                 0                            0
 * | (0)                       0                            0
 * ---------------------------------------------------------------------------
 */
static Expr *
update_set_auto_increment_func_node(ParseState *pstate, Expr* expr, int attno)
{
	Form_pg_attribute attr;

	attr = TupleDescAttr(pstate->p_target_relation->rd_att, attno - 1);
	if (attr->attidentity == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
	{
		Const *valueNode = NULL;

		if (has_sql_mode(MODE_NO_AUTO_VALUE_ON_ZERO))
		{
			if (!attr->attnotnull)
			{
				if (IsA((expr), SetToDefault))
				{
					valueNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
										  true, FLOAT8PASSBYVAL);
				}
			}
			else
			{
				if ((IsA((expr), Const) && ((Const *)expr)->constisnull) ||
					IsA((expr), SetToDefault))
				{
					valueNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
										  false, FLOAT8PASSBYVAL);
				}
			}
		}
		else
		{
			if (!attr->attnotnull)
			{
				if (IsA((expr), SetToDefault))
				{
					valueNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
										  true, FLOAT8PASSBYVAL);
				}
			}
			else
			{
				if (IsA((expr), SetToDefault))
				{
					valueNode = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
										  Int64GetDatum(AUTO_INCREMENT_VALUE_ZERO),
										  false, FLOAT8PASSBYVAL);
				}
			}
		}
		if (valueNode)
			expr = (Expr*)valueNode;
	}
	return expr;
}
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

/* Begin - ReqID:SRS-MYSQL-SET */
/* makeStringConst
 *
 * make a String Const node.
 */
Node *
ivy_makeStringConst(char *str, int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = str;
	n->location = location;

	return (Node *)n;
}

/* ivy_makeIntConst
 *
 * make IntConst.
 */
Node *
ivy_makeIntConst(int val, int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Integer;
	n->val.val.ival = val;
	n->location = location;

	return (Node *)n;
}

/* ivy_makeFloatConst
 *
 * make FloatConst.
 */
Node *
ivy_makeFloatConst(char *str, int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Float;
	n->val.val.str = str;
	n->location = location;

	return (Node *)n;
}

/* ivy_makeAConst
 *
 * make a Const.
 */
Node *
ivy_makeAConst(Value *v, int location)
{
	Node *n;

	switch (v->type)
	{
		case T_Float:
			n = ivy_makeFloatConst(v->val.str, location);
			break;

		case T_Integer:
			n = ivy_makeIntConst(v->val.ival, location);
			break;

		case T_String:
		default:
			n = ivy_makeStringConst(v->val.str, location);
			break;
	}

	return n;
}

Node *
ivy_makeNullAConst(int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Null;
	n->location = location;

	return (Node *)n;
}

/* ivy_makeTypeCast
 *
 * make a type cast.
 */
Node *
ivy_makeTypeCast(Node *arg, TypeName *typename, int location)
{
	TypeCast *n = makeNode(TypeCast);

	n->arg = arg;
	n->typeName = typename;
	n->location = location;

	return (Node *)n;
}

/* ivy_makeBoolAConst
 *
 * make a Const to bool cast.
 */
Node *
ivy_makeBoolAConst(bool state)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = (state ? "t" : "f");
	
	return ivy_makeTypeCast((Node *)n, SystemTypeName("bool"), -1);
}

/* trans_MysVariable_to_VariableSet
 *
 * transform Mysql Set clause,If there is only one set, it is
 * a VariableSetStmt node, Otherwise, it is a query node.
 */
VariableSetStmt *
trans_MysVariable_to_VariableSet(ParseState *pstate, MysDoStmt *stmt)
{
	VariableSetStmt *variableSetStmt = NULL;

	/* If this is a Select node, determine the number of
	 * target list on the select, it there is only one node,
	 * determin whether it is a function,if it's a funtion
	 * and the function name is also a parameter setting
	 * function, then determine whether the second parameter
	 * is a simple value, if so, it is a VariableSetStmt node,
	 * otherwise return a MysDoStmt node.
	 */
	if (nodeTag(stmt->stmt) == T_SelectStmt)
	{
		SelectStmt *selstmt = (SelectStmt *)stmt->stmt;

		if (list_length(selstmt->targetList) == 1 &&
			IsA(linitial(selstmt->targetList), VarAssign))
		{
			VarAssign *var = (VarAssign *)linitial(selstmt->targetList);

			if (var->setkind == SET_LOCALVAR || var->setkind == SET_GLOBALVAR ||
				var->setkind == SET_SESSIONVAR)
			{
				char	   *varValue = NULL;
				List	   *vallist = NIL;

				variableSetStmt = makeNode(VariableSetStmt);
				if (IsA(var->value, SetToDefault))
					variableSetStmt->kind = VAR_SET_DEFAULT;
				else
					variableSetStmt->kind = VAR_SET_VALUE;

				variableSetStmt->name = var->varname;
				if (var->setkind == SET_LOCALVAR)
					variableSetStmt->is_local = true;
				else
					variableSetStmt->is_local = false;

				if (IsA(var->value, A_Const))
				{
					A_Const *aconst = (A_Const *)var->value;

					if (aconst->val.type == T_String)
						varValue = aconst->val.val.str;
					else
						vallist = list_make1(ivy_makeAConst(&aconst->val, var->location));
				}
				else if (IsA(var->value, ColumnRef))
				{
					ColumnRef *cref = (ColumnRef *)var->value;
					ListCell *cell;

					foreach(cell, cref->fields)
					{
						if (IsA(lfirst(cell), String))
						{
							vallist = lappend(vallist, ivy_makeAConst((Value *)lfirst(cell), var->location));
						}
					}
				}
				else if (IsA(var->value, TypeCast))
				{
					TypeCast *typcast = (TypeCast *)var->value;

					if (typcast->typeName != NULL)
					{
						if (list_length(typcast->typeName->names) == 2)
						{
							if (IsA(linitial(typcast->typeName->names), String) &&
								IsA(llast(typcast->typeName->names), String))
							{
								if (strcmp(strVal(linitial(typcast->typeName->names)), "pg_catalog") == 0 &&
									strcmp(strVal(llast(typcast->typeName->names)), "bool") == 0)
								{
									if (IsA(typcast->arg, A_Const))
									{
										A_Const *aconst = (A_Const *)typcast->arg;

										if (aconst->val.type == T_String)
										{
											if (strcmp((aconst->val.val.str), "f") == 0)
												vallist = lappend(vallist, ivy_makeStringConst("false", var->location));
											else if (strcmp((aconst->val.val.str), "t") == 0)
												vallist = lappend(vallist, ivy_makeStringConst("true", var->location));
										}
									}
								}
							}
						}
					}
				}
				/* Begin - BUG#Z5838 */
				else if (IsA(var->value, SetToDefault))
				{
					vallist = lappend(vallist, ivy_makeStringConst("default", var->location));
				}
				/* End - BUG#Z5838 */
				else
				{
					return NULL;
				}

				if (variableSetStmt->kind != VAR_SET_DEFAULT && varValue != NULL &&
					guc_list_input_check(var->varname))
				{
					List	   *namelist;
					ListCell   *cell;

					SplitIdentifierString(varValue, ',', &namelist);
					foreach(cell, namelist)
					{
						vallist = lappend(vallist, ivy_makeStringConst((char *)lfirst(cell), var->location));
					}
				}
				else
				{
					if (varValue != NULL && vallist == NIL)
						vallist = list_make1(ivy_makeStringConst(varValue, var->location));
				}
				variableSetStmt->args = vallist;

				/* Begin - BUG#Z5979
				 * If the parameter list is NIL, the node
				 * variableSetStmt should node be constructed.
				 */
				if (vallist == NULL)
				{
					return NULL;
				}
				/* End - BUG#Z5979 */
			}
		}
	}

	return variableSetStmt;
}

/* transformMysDoStmt
 *
 * transform Mysql Set clause,If there is only one set, it is
 * a VariableSetStmt node, Otherwise, it is a query node.
 */
static Query *
transformMysDoStmt(ParseState *pstate, MysDoStmt *stmt)
{
	Query *qry;
	VariableSetStmt *setvar = NULL;

	/* If this is a Select node, determine the number of
	 * target list on the select, it there is only one node,
	 * determin whether it is a function,if it's a funtion
	 * and the function name is also a parameter setting
	 * function, then determine whether the second parameter
	 * is a simple value, if so, it is a VariableSetStmt node,
	 * otherwise return a MysDoStmt node.
	 */
	/* Begin - BUG#Z6201 */
	if (stmt->purpose == MYS_DO_FOR_SET)
	{
		setvar = trans_MysVariable_to_VariableSet(pstate, stmt);
		if (setvar != NULL)
		{
			qry = makeNode(Query);
			qry->commandType = CMD_UTILITY;
			qry->utilityStmt = (Node *) setvar;
			return qry;
		}

		/* this is a SET statement, set p_myssetvar to true, BUG#Z6860 */
		pstate->p_myssetvar = true;
	}
	/* End - BUG#Z6201 */

	if (nodeTag(stmt->stmt) == T_SelectStmt)
	{
		SelectStmt	   *selstmt = (SelectStmt *)stmt->stmt;
		ListCell	   *cell;
		List		   *newtargetlist = NIL;

		/* Transform VarAssign node to a ResTarget node */
		foreach(cell, selstmt->targetList)
		{
			if (IsA(lfirst(cell), VarAssign))
			{
				ResTarget	   *result;
				VarAssign	   *var;

				var = (VarAssign *)lfirst(cell);
				result = makeNode(ResTarget);
				result->name = NULL;
				result->indirection = NIL;
				result->val = (Node *)var;
				result->location = var->location;

				newtargetlist = lappend(newtargetlist, result);
			}
		}

		if (newtargetlist != NIL)
			selstmt->targetList = newtargetlist;
	}

	/* transform contained query(SelectStmt) */
	stmt->stmt = (Node *) transformOptionalSelectInto(pstate, stmt->stmt);

	/* represent the command as a utility Query */
	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;
	qry->utilityStmt = (Node *) stmt;

	return qry;
}

/* transformSelectIntoUservarStmt
 *
 * For select into,
 * stored procedures have their own process, can't reach here
 */
static Query *
transformSelectIntoUservarStmt(ParseState *pstate, SelectIntoUserVarStmt *stmt)
{
	Query *result;
	ListCell *lc;
	int targetNum = list_length(stmt->target);

	foreach(lc, stmt->target)
	{
		if (!IsA(lfirst(lc), VarRef))
		{
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("Undeclared variable")));
		}
	}

	/* transform contained query(SelectStmt) */
	stmt->expr = (Node *) transformStmt(pstate, stmt->expr);

	foreach(lc, ((Query *)(stmt->expr))->targetList)
	{
		TargetEntry *n = (TargetEntry *)lfirst(lc);

		if (n->resjunk == false)
		{
			targetNum--;
		}
	}

	if (targetNum != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("The used SELECT statements have a different number of columns")));
	}

	/* represent the command as a utility Query */
	result = makeNode(Query);
	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	return result;
}
/* End - ReqID:SRS-MYSQL-SET */

/* Begin - ReqID:SRS-MYSQL-SELECT */
static List *
transformSorbyfromGroupclause(List **grouplist)
{
	List	   *sorbylist = NIL;
	ListCell   *l;
	foreach(l, *grouplist)
	{
		Node *node = (Node *) lfirst(l);
		if (IsA(node, SortBy))
		{
			sorbylist = lappend(sorbylist, node);
			*grouplist = list_delete(*grouplist, node);
		}
		else if (IsA(node, GroupingSet) && ((GroupingSet *)node)->kind == GROUPING_SET_ROLLUP)
		{
			ListCell   *m;

			foreach(m, ((GroupingSet *)node)->content)
			{
				Node *groupset = (Node *) lfirst(m);
				if (IsA(groupset, SortBy))
					((GroupingSet *)node)->content = list_delete(((GroupingSet *)node)->content, groupset);
			}
		}
	}

	return sorbylist;
}

static Node *
makeAndExpr(Node *lexpr, Node *rexpr, int location)
{
	Node	   *lexp = lexpr;

	/* Flatten "a AND b AND c ..." to a single BoolExpr on sight */
	if (IsA(lexp, BoolExpr))
	{
		BoolExpr *blexpr = (BoolExpr *)lexp;

		if (blexpr->boolop == AND_EXPR)
		{
			blexpr->args = lappend(blexpr->args, rexpr);
			return (Node *)blexpr;
		}
	}
	return (Node *) makeBoolExpr(AND_EXPR, list_make2(lexpr, rexpr),
			false, location);
}
/* End - ReqID:SRS-MYSQL-SELECT */
