/*-------------------------------------------------------------------------
 *
 * File:parse_pivot.c
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
 *    src/backend/parser/parse_pivot.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "parser/analyze.h"
#include "parser/parse_clause.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_pivot.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "parser/parser.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/ruleutils.h"


typedef struct PivotCollectCtx
{
	List	  **colnames;
} PivotCollectCtx;

typedef enum PivotUnpivotTypeGroup
{
	PIVOT_UNPIVOT_TYPE_OTHER,
	PIVOT_UNPIVOT_TYPE_CHARACTER,
	PIVOT_UNPIVOT_TYPE_NUMERIC
} PivotUnpivotTypeGroup;

static bool pivot_has_name(List *names, const char *name);
static char *pivot_quote_qualified(const char *alias, const char *colname);
static char *pivot_const_sql(Node *n);
static char *pivot_label_text(Node *label_or_value);
static Const *transformPivotInValue(ParseState *pstate, Node *val,
								   char **label, bool is_unpivot);
static bool pivot_in_has_param(Node *node, void *context);
static bool pivot_needs_deparsed_label(Node *node, void *context);
static void pivot_add_output_name(List **names, const char *name);
static char *pivot_simple_colname(Node *n);
static char *pivot_colref_name(ColumnRef *cr);
static bool pivot_collect_colrefs_walker(Node *node, PivotCollectCtx *ctx);
static char *pivot_expr_sql(Node *n, const char *source_alias);
static char *pivot_sortby_sql(SortBy *sortby, const char *source_alias);
static char *pivot_agg_sql(FuncCall *fcall, const char *source_alias, const char *condsql);
static void pivot_collect_colrefs(Node *node, List **colnames);
static Alias *pivot_source_alias(Node *source);
static char *pivot_func_name(FuncCall *fcall);
static char *pivot_build_source_sql(ParseState *pstate, Node *source,
									List **colnames, List **coltypes,
									List **coltypmods);
static char *pivot_build_pivot_sql(ParseState *pstate, PivotCall *pcall,
								   const char *source_sql, List *source_cols);
static char *pivot_build_unpivot_sql(ParseState *pstate, PivotCall *pcall,
									 const char *source_sql,
									 List *source_cols, List *source_types,
									 List *source_typmods);
static Query *pivot_analyze_sql(ParseState *pstate, const char *sql);
static char *pivot_raw_select_sql(ParseState *pstate, Node *stmt,
								 int expected_cols);
static char *pivot_agg_plain_sql(FuncCall *fcall, const char *source_alias, const char *count_col);
static char *pivot_agg_xml_label(PivotClause *pc, ResTarget *rt, int idx,
								 bool *quoted);
static void pivot_append_group_cols(StringInfo buf, const char *alias, List *group_cols);
static void pivot_append_xml_value_cols(StringInfo buf, const char *alias,
									int value_count);
static void pivot_append_xml_value_match(StringInfo buf,
									 const char *left_alias,
									 const char *right_alias,
									 int value_count);
static char *pivot_internal_colname(List *source_cols, const char *base_name);
static char *pivot_build_pivot_xml_sql(ParseState *pstate, PivotCall *pcall,
									   const char *source_sql, List *source_cols,
									   List *source_types, List *source_typmods);
static char *pivot_xml_identifier_name(const char *name, bool quoted);
static char *pivot_xml_output_name(List *for_cols);
static void pivot_source_col_type(List *source_cols, List *source_types,
								  List *source_typmods, const char *name,
								  Oid *type_oid, int32 *typmod);
static char *pivot_unpivot_target_type(PivotClause *pc, int target_index,
									 List *source_cols, List *source_types,
									 List *source_typmods);

/*
 * Return true if names contains a String node whose value equals name.
 * Non-String entries are ignored and comparison is case-sensitive.  Callers
 * use this for column-name membership and collision checks without reordering
 * the source or output name lists.
 */
static bool
pivot_has_name(List *names, const char *name)
{
	ListCell   *lc;

	foreach(lc, names)
	{
		Value	   *v = lfirst(lc);

		if (v == NULL || !IsA(v, String) || strVal(v) == NULL)
			elog(ERROR, "invalid PIVOT|UNPIVOT output name");
		if (strcmp(strVal(v), name) == 0)
			return true;
	}

	return false;
}

/*
 * Quote both components of a generated alias.column reference.
 * The builders supply internal aliases such as pvt_src and unp_src; quoting
 * the column name preserves case and permits names that are SQL keywords.
 */
static char *
pivot_quote_qualified(const char *alias, const char *colname)
{
	if (alias == NULL || colname == NULL)
		elog(ERROR, "null qualified name in PIVOT|UNPIVOT");
	return psprintf("%s.%s", quote_identifier(alias), quote_identifier(colname));
}

/*
 * Serialize a raw literal or a supported raw expression as SQL text.
 * A_Const values are emitted directly; other nodes go through pivot_expr_sql()
 * with no source alias, which rejects column references.  This helper does
 * not analyze or evaluate expressions and is not a general constancy check.
 *
 * Callers use it for literal arguments in the raw expression serializer and
 * legacy PIVOT output headings.  PIVOT IN comparison values and UNPIVOT label
 * expressions instead use transformPivotInValue() and the core deparser.
 */
static char *
pivot_const_sql(Node *n)
{
	if (n != NULL && IsA(n, A_Const))
	{
		A_Const    *c = castNode(A_Const, n);

		if ((IsA(&c->val, Float) || IsA(&c->val, String) ||
			IsA(&c->val, BitString)) && strVal(&c->val) == NULL)
			elog(ERROR, "null literal in PIVOT expression");

		switch (nodeTag(&c->val))
		{
			case T_Integer:
				return psprintf("%d", intVal(&c->val));
			case T_Float:
				return pstrdup(strVal(&c->val));
			case T_String:
				return quote_literal_cstr(strVal(&c->val));
			case T_BitString:
				{
					const char *value = strVal(&c->val);

					if (value == NULL || (value[0] != 'b' && value[0] != 'x'))
						elog(ERROR, "invalid bit string literal in PIVOT expression");

					return psprintf("%c%s", pg_toupper((unsigned char) value[0]),
									quote_literal_cstr(value + 1));
				}
			case T_Null:
				return pstrdup("NULL");
			default:
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("unsupported literal in PIVOT expression")));
		}
	}

	return pivot_expr_sql(n, NULL);
}

/*
 * Return the text of a PIVOT IN alias stored as a raw constant.
 * String aliases lose their SQL literal quotes; numeric constants use their
 * literal text, other nodes fall back to pivot_const_sql(), and NULL input
 * returns "col".  The caller appends any aggregate alias, even when there is
 * only one aggregate.  Unaliased IN expressions obtain their headings from
 * transformPivotInValue() instead.
 */
static char *
pivot_label_text(Node *label_or_value)
{
	if (label_or_value == NULL)
		return pstrdup("col");

	if (IsA(label_or_value, A_Const))
	{
		A_Const    *c = castNode(A_Const, label_or_value);

		if ((IsA(&c->val, Float) || IsA(&c->val, String) ||
			IsA(&c->val, BitString)) && strVal(&c->val) == NULL)
			elog(ERROR, "null literal in PIVOT expression");

		switch (nodeTag(&c->val))
		{
			case T_Integer:
				return psprintf("%d", intVal(&c->val));
			case T_Float:
				return pstrdup(strVal(&c->val));
			case T_String:
				return pstrdup(strVal(&c->val));
			default:
				break;
		}
	}

	return pivot_const_sql(label_or_value);
}

/*
 * Return true when an IN expression heading needs the core deparser.
 * The supported raw-node shapes return false so their existing heading
 * spelling can be preserved with pivot_const_sql().  Descendants are checked
 * recursively.  This only selects a formatting path: transformPivotInValue()
 * validates and evaluates IN expressions independently of this helper.
 */
static bool
pivot_needs_deparsed_label(Node *node, void *context)
{
	check_stack_depth();

	if (node == NULL)
		return false;

	switch (nodeTag(node))
	{
		case T_A_Const:
			switch (nodeTag(&((A_Const *) node)->val))
			{
				case T_Integer:
				case T_Float:
				case T_String:
				case T_BitString:
				case T_Null:
					return false;
				default:
					return true;
			}
		case T_TypeCast:
			return pivot_needs_deparsed_label(((TypeCast *) node)->arg, context);
		case T_A_Expr:
			{
				A_Expr	   *a = (A_Expr *) node;

				if (a->kind != AEXPR_OP || list_length(a->name) != 1)
					return true;
				break;
			}
		case T_FuncCall:
			{
				FuncCall   *f = (FuncCall *) node;

				if (f->agg_star || f->agg_order != NIL || f->agg_filter || f->over)
					return true;
				break;
			}
		case T_List:
		case T_CaseExpr:
		case T_CoalesceExpr:
			break;
		default:
			return true;
	}

	return raw_expression_tree_walker(node, pivot_needs_deparsed_label, context);
}

/*
 * Return true if an analyzed expression contains a Param node.
 * transformPivotInValue() uses this before planning to catch parameters,
 * including those introduced by parser hooks, before constant folding can
 * remove an unused branch.  Raw bind references are rejected separately by
 * the EXPR_KIND_PIVOT_IN transformation rules.
 */
static bool
pivot_in_has_param(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
		return true;
	return expression_tree_walker(node, pivot_in_has_param, context);
}

/*
 * Transform a PIVOT IN value or UNPIVOT IN label expression into a constant,
 * following the partition bound pipeline:
 * transformExpr, collation assignment, expression_planner, evaluate_expr.
 * The expression kinds reject references to input rows, subqueries,
 * aggregates, window functions and SRFs during transformation.
 *
 * Unlike a partition bound, a PIVOT value is an equality-comparison operand,
 * not a value stored in the FOR column.  Preserve its type and typmod so that
 * normal operator resolution is unchanged (e.g., integer 1 must not match 1.4
 * through an assignment cast).  Unknown string/NULL literals are already Const
 * nodes and will acquire the comparison type when the rewrite is analyzed.
 *
 * Analyze a copy of val, leaving the raw tree available for heading generation.
 * If label is non-NULL, return the expression heading before folding; explicit
 * IN aliases bypass that work.  Preserve legacy spelling where possible and
 * use the core deparser for other headings.  An existing Const needs no
 * planning or evaluation; attach val's location to the final Const in either
 * case, and retain its type, typmod, and assigned collation.
 *
 * STABLE functions such as Oracle TO_DATE may be evaluated at analysis time.
 * Reject volatile functions before planning and again afterward for calls
 * exposed by default-argument expansion or inlining, before evaluate_expr().
 * Both clauses use this same policy, without per-function exceptions.
 */
static Const *
transformPivotInValue(ParseState *pstate, Node *val, char **label, bool is_unpivot)
{
	Node	   *value;
	Oid			type;
	int32		typmod;
	Oid			collation;

	if (pstate == NULL || val == NULL)
		elog(ERROR, "invalid PIVOT|UNPIVOT IN expression");

	value = transformExpr(pstate, copyObject(val),
						  is_unpivot ? EXPR_KIND_UNPIVOT_IN : EXPR_KIND_PIVOT_IN);
	if (is_unpivot && exprType(value) == BOOLOID)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("condition is not allowed in UNPIVOT IN expressions"),
				 parser_errposition(pstate, exprLocation(val))));
	if (contain_var_clause(value))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
				 errmsg("non-constant expression is not allowed in a PIVOT or UNPIVOT value list"),
				 parser_errposition(pstate, exprLocation(val))));
	if (pivot_in_has_param(value, NULL))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("bind variables are not allowed in %s IN expressions",
						is_unpivot ? "UNPIVOT" : "PIVOT"),
				 parser_errposition(pstate, exprLocation(val))));
	if (contain_volatile_functions(value))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
				 errmsg("non-constant expression is not allowed in a PIVOT or UNPIVOT value list"),
				 errdetail("%s IN expressions must not contain volatile functions.",
						  is_unpivot ? "UNPIVOT" : "PIVOT"),
				 parser_errposition(pstate, exprLocation(val))));

	assign_expr_collations(pstate, value);

	/*
	 * Derive the heading before folding; aliases bypass expression headings.
	 */
	if (label != NULL)
		*label = !pivot_needs_deparsed_label(val, NULL) ? pivot_const_sql(val) :
			deparse_expression(value, NIL, false, false);

	if (!IsA(value, Const))
	{
		type = exprType(value);
		typmod = exprTypmod(value);
		collation = exprCollation(value);
		value = (Node *) expression_planner((Expr *) value);

		/*
		 * Planning can expose volatile defaults or inlined function bodies.
		 */
		if (contain_volatile_functions(value))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
					 errmsg("non-constant expression is not allowed in a PIVOT or UNPIVOT value list"),
					 errdetail("%s IN expressions must not contain volatile functions.",
						  is_unpivot ? "UNPIVOT" : "PIVOT"),
					 parser_errposition(pstate, exprLocation(val))));
		value = (Node *) evaluate_expr((Expr *) value, type, typmod, collation);
		if (!IsA(value, Const))
			elog(ERROR, "could not evaluate %s IN expression", is_unpivot ? "UNPIVOT" : "PIVOT");
	}

	((Const *) value)->location = exprLocation(val);

	return (Const *) value;
}

/*
 * Check and record an ordinary PIVOT output column name.
 * Reject names of NAMEDATALEN bytes or more and exact matches in names before
 * reparsing generated SQL.  The caller seeds names with implicit grouping
 * columns, so collisions with those columns are checked as well as duplicate
 * aggregate output names.
 */
static void
pivot_add_output_name(List **names, const char *name)
{
	if (names == NULL || name == NULL)
		elog(ERROR, "invalid PIVOT output column name");

	if (strlen(name) >= NAMEDATALEN)
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_COLUMN),
				 errmsg("generated PIVOT column name is too long"),
				 errdetail("Generated column name \"%s\" exceeds the maximum identifier length of %d bytes.",
						   name, NAMEDATALEN - 1),
				 errhint("Specify shorter aliases for the pivot value or aggregate function.")));

	if (pivot_has_name(*names, name))
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_COLUMN),
				 errmsg("column ambiguously defined"),
				 errdetail("PIVOT generates column name \"%s\" more than once.",
						   name)));

	*names = lappend(*names, makeString(pstrdup(name)));
}

/*
 * Extract an unqualified UNPIVOT input-column name from raw grammar nodes.
 * Accept a String or a one-component ColumnRef, unwrapping singleton lists,
 * ResTarget, TypeCast, ColumnRefOrFuncCall, and empty A_Indirection nodes as
 * implemented below.  Other shapes raise an error.  This extracts a name;
 * pivot_source_col_type() subsequently checks that the source exposes it.
 */
static char *
pivot_simple_colname(Node *n)
{
	check_stack_depth();

	if (n != NULL)
	{
		if (IsA(n, String))
		{
			if (strVal(n) == NULL)
				elog(ERROR, "null UNPIVOT input column name");
			return pstrdup(strVal(n));
		}

		if (IsA(n, List))
		{
			List	   *l = (List *) n;

			if (list_length(l) == 1)
				return pivot_simple_colname(linitial(l));
		}
		else if (IsA(n, ResTarget))
			return pivot_simple_colname(castNode(ResTarget, n)->val);
		else if (IsA(n, TypeCast))
			return pivot_simple_colname(castNode(TypeCast, n)->arg);
		else if (IsA(n, ColumnRefOrFuncCall))
		{
			ColumnRefOrFuncCall *cf = castNode(ColumnRefOrFuncCall, n);

			if (cf->cref != NULL)
				return pivot_simple_colname((Node *) cf->cref);
			if (cf->func != NULL)
				return pivot_simple_colname((Node *) cf->func);
		}
		else if (IsA(n, A_Indirection))
		{
			A_Indirection *ai = castNode(A_Indirection, n);

			/* plain parenthesized/forwarded reference */
			if (ai->indirection == NIL)
				return pivot_simple_colname(ai->arg);
		}
		else if (IsA(n, ColumnRef))
		{
			ColumnRef  *cr = castNode(ColumnRef, n);

			if (cr->fields != NIL &&
				(!IsA(cr->fields, List) || cr->fields->length == 0))
				 elog(ERROR, "invalid name list in PIVOT|UNPIVOT");

			if (list_length(cr->fields) == 1 && IsA(linitial(cr->fields), String))
				return pstrdup(strVal(linitial(cr->fields)));
		}
	}

	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("PIVOT/UNPIVOT currently supports simple column references only")));

	return NULL;
}

/*
 * Return a copy of the last component of a nonempty ColumnRef whose fields
 * are all String nodes; return NULL for other shapes, including stars.
 * Aggregate dependency collection and serialization use this to map qualified
 * references such as t.sport to the visible source column named "sport".
 */
static char *
pivot_colref_name(ColumnRef *cr)
{
	Value	   *last = NULL;
	ListCell   *lc;

	if (cr == NULL || !IsA(cr, ColumnRef))
		elog(ERROR, "invalid PIVOT column reference");

	foreach(lc, cr->fields)
	{
		Node	   *f = lfirst(lc);

		if (IsA(f, String))
		{
			if (strVal(f) == NULL)
				elog(ERROR, "null PIVOT column name");

			last = (Value *) f;
		}
		else
			return NULL;
	}

	if (last == NULL)
		return NULL;

	return pstrdup(strVal(last));
}

/*
 * Collect unique column names from a raw aggregate expression tree.
 * ColumnRefOrFuncCall is inspected through both its column and function forms;
 * ColumnRef contributes its last name component.  Other nodes use the raw
 * expression walker.  Callers inspect aggregate arguments and ORDER BY
 * expressions to exclude their referenced columns from implicit grouping.
 */
static bool
pivot_collect_colrefs_walker(Node *node, PivotCollectCtx *ctx)
{
	check_stack_depth();

	if (node == NULL)
		return false;

	if (ctx == NULL || ctx->colnames == NULL)
		elog(ERROR, "invalid PIVOT column collection context");

	if (IsA(node, FuncCall))
	{
		FuncCall	*fcall = (FuncCall *) node;
		ListCell	*lc;

		if ((fcall->funcname != NIL && !IsA(fcall->funcname, List)) ||
			(fcall->args != NIL && !IsA(fcall->args, List)) ||
			(fcall->agg_order != NIL && !IsA(fcall->agg_order, List)))
			elog(ERROR, "invalid PIVOT function lists");

		foreach(lc, fcall->agg_order)
		{
			SortBy	*sb = lfirst(lc);

			if (sb == NULL || !IsA(sb, SortBy) || sb->node == NULL)
				elog(ERROR, "invalid PIVOT aggregate ORDER BY item");
		}
	}

	if (IsA(node, ColumnRefOrFuncCall))
	{
		ColumnRefOrFuncCall *cf = castNode(ColumnRefOrFuncCall, node);

		if (cf->cref != NULL)
			pivot_collect_colrefs((Node *) cf->cref, ctx->colnames);
		if (cf->func != NULL)
			pivot_collect_colrefs((Node *) cf->func, ctx->colnames);
		return false;
	}

	if (IsA(node, ColumnRef))
	{
		char	   *col = pivot_colref_name(castNode(ColumnRef, node));

		if (col != NULL && !pivot_has_name(*ctx->colnames, col))
			*ctx->colnames = lappend(*ctx->colnames, makeString(col));
		return false;
	}

	return raw_expression_tree_walker(node,
									  pivot_collect_colrefs_walker,
									  (void *) ctx);
}

/*
 * Append previously unseen column names referenced in the raw node to
 * colnames, preserving any names already present.  Qualified references
 * contribute their last component through pivot_collect_colrefs_walker().
 */
static void
pivot_collect_colrefs(Node *node, List **colnames)
{
	PivotCollectCtx ctx;

	if (colnames == NULL)
		elog(ERROR, "missing PIVOT column collection output");

	ctx.colnames = colnames;
	(void) pivot_collect_colrefs_walker(node, &ctx);
}

/*
 * Serialize an aggregate ORDER BY expression, including its ASC/DESC and
 * NULLS FIRST/LAST options.  pivot_expr_sql() rewrites column references using
 * the supplied source alias.  ORDER BY USING raises an unsupported-feature
 * error.
 */
static char *
pivot_sortby_sql(SortBy *sortby, const char *source_alias)
{
	StringInfoData buf;
	char	   *exprsql;

	if (sortby == NULL || !IsA(sortby, SortBy) || sortby->node == NULL)
		elog(ERROR, "invalid PIVOT aggregate ORDER BY item");

	exprsql = pivot_expr_sql(sortby->node, source_alias);
	initStringInfo(&buf);
	appendStringInfoString(&buf, exprsql);

	switch (sortby->sortby_dir)
	{
		case SORTBY_DEFAULT:
			break;
		case SORTBY_ASC:
			appendStringInfoString(&buf, " ASC");
			break;
		case SORTBY_DESC:
			appendStringInfoString(&buf, " DESC");
			break;
		case SORTBY_USING:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PIVOT aggregate ORDER BY USING is not supported")));
			break;
		default:
			elog(ERROR, "invalid PIVOT aggregate sort direction");
			break;
	}

	switch (sortby->sortby_nulls)
	{
		case SORTBY_NULLS_DEFAULT:
			break;
		case SORTBY_NULLS_FIRST:
			appendStringInfoString(&buf, " NULLS FIRST");
			break;
		case SORTBY_NULLS_LAST:
			appendStringInfoString(&buf, " NULLS LAST");
			break;
		default:
			elog(ERROR, "invalid PIVOT aggregate NULLS order");
			break;
	}

	return buf.data;
}

/*
 * Serialize the supported raw expression subset used in aggregate arguments
 * and ORDER BY expressions and legacy PIVOT headings.
 * For example:
 *	sport || '#' || event || '#' || gender is rebuilt recursively.
 *
 * With a source alias, column references become <source_alias>.<column>;
 * without one, column references are rejected.  Supported forms include
 * literals, casts, operators and predicates, CASE, COALESCE, and function
 * calls.  Unsupported node shapes raise an error.  This serializer does not
 * perform general expression analysis or constant evaluation; ordinary PIVOT
 * IN values use transformPivotInValue().
 */
static char *
pivot_expr_sql(Node *n, const char *source_alias)
{
	check_stack_depth();

	if (n == NULL)
		return pstrdup("NULL");

	if (n != NULL && IsA(n, A_Const))
		return pivot_const_sql(n);

	if (IsA(n, ColumnRef))
	{
		char	   *col = pivot_colref_name(castNode(ColumnRef, n));

		if (source_alias == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
					 errmsg("non-constant expression is not allowed in a PIVOT or UNPIVOT value list")));

		if (col == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PIVOT aggregate expression currently supports simple column references only")));
		return pivot_quote_qualified(source_alias, col);
	}

	if (IsA(n, TypeCast))
	{
		TypeCast   *tc = castNode(TypeCast, n);
		char	   *arg;
		char	   *typ;

		if (tc->arg == NULL || tc->typeName == NULL || !IsA(tc->typeName, TypeName))
			elog(ERROR, "invalid type cast in PIVOT expression");

		arg = pivot_expr_sql(tc->arg, source_alias);
		typ = TypeNameToString(tc->typeName);

		return psprintf("(%s)::%s", arg, typ);
	}

	if (IsA(n, ColumnRefOrFuncCall))
	{
		ColumnRefOrFuncCall *cf = castNode(ColumnRefOrFuncCall, n);

		if (cf->cref != NULL)
			return pivot_expr_sql((Node *) cf->cref, source_alias);
		if (cf->func != NULL)
			return pivot_expr_sql((Node *) cf->func, source_alias);
	}

	if (IsA(n, A_Indirection))
	{
		A_Indirection *ai = castNode(A_Indirection, n);

		if (ai->indirection == NIL)
			return pivot_expr_sql(ai->arg, source_alias);
	}

	if (IsA(n, A_Expr))
	{
		A_Expr	   *a = castNode(A_Expr, n);
		char	   *opname;
		char	   *l;
		char	   *r;

		if (a->kind == AEXPR_BETWEEN ||
			a->kind == AEXPR_NOT_BETWEEN ||
			a->kind == AEXPR_BETWEEN_SYM ||
			a->kind == AEXPR_NOT_BETWEEN_SYM)
		{
			List	   *args;
			char	   *neg = (a->kind == AEXPR_NOT_BETWEEN ||
							   a->kind == AEXPR_NOT_BETWEEN_SYM) ? " NOT" : "";
			char	   *sym = (a->kind == AEXPR_BETWEEN_SYM ||
							   a->kind == AEXPR_NOT_BETWEEN_SYM) ? " SYMMETRIC" : "";

			if ((a->rexpr == NULL || !IsA(a->rexpr, List)) || list_length((List *) a->rexpr) != 2)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("unsupported PIVOT aggregate expression operator")));

			args = (List *) a->rexpr;
			return psprintf("(%s%s BETWEEN%s %s AND %s)",
							pivot_expr_sql(a->lexpr, source_alias),
							neg,
							sym,
							pivot_expr_sql(linitial(args), source_alias),
							pivot_expr_sql(lsecond(args), source_alias));
		}

		if ((a->kind != AEXPR_OP && a->kind != AEXPR_IN &&
			 a->kind != AEXPR_DISTINCT && a->kind != AEXPR_NOT_DISTINCT &&
			 a->kind != AEXPR_NULLIF) ||
			list_length(a->name) != 1 ||
			!IsA(linitial(a->name), String))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("unsupported PIVOT aggregate expression operator")));

		if ((a->lexpr == NULL && a->rexpr == NULL) ||
			(a->kind != AEXPR_OP && (a->lexpr == NULL || a->rexpr == NULL)))
			elog(ERROR, "missing PIVOT expression operand");
		opname = strVal(linitial(a->name));
		l = (a->lexpr != NULL) ? pivot_expr_sql(a->lexpr, source_alias) : NULL;

		if (a->kind == AEXPR_IN)
		{
			StringInfoData inbuf;
			ListCell   *lc;
			bool		first = true;
			const char *notstr = (strcmp(opname, "<>") == 0) ? " NOT" : "";

			if ((a->rexpr == NULL || !IsA(a->rexpr, List)))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("unsupported PIVOT aggregate expression operator")));

			initStringInfo(&inbuf);
			foreach(lc, (List *) a->rexpr)
			{
				if (!first)
					appendStringInfoString(&inbuf, ", ");
				appendStringInfoString(&inbuf,
									   pivot_expr_sql((Node *) lfirst(lc), source_alias));
				first = false;
			}
			return psprintf("(%s%s IN (%s))", l, notstr, inbuf.data);
		}

		r = (a->rexpr != NULL) ? pivot_expr_sql(a->rexpr, source_alias) : NULL;

		if (a->kind == AEXPR_DISTINCT || a->kind == AEXPR_NOT_DISTINCT)
			return psprintf("(%s IS%s DISTINCT FROM %s)",
							l,
							a->kind == AEXPR_NOT_DISTINCT ? " NOT" : "",
							r);

		if (a->kind == AEXPR_NULLIF)
			return psprintf("NULLIF(%s, %s)", l, r);
		if (l != NULL && r != NULL)
			return psprintf("(%s %s %s)", l, opname, r);
		if (r != NULL)
			return psprintf("(%s %s)", opname, r);
		return psprintf("(%s %s)", opname, l);
	}

	if (IsA(n, BoolExpr))
	{
		BoolExpr   *b = castNode(BoolExpr, n);
		StringInfoData buf;
		ListCell   *lc;
		bool		first = true;
		const char *sep;

		if (b->args == NIL ||
			(b->boolop == NOT_EXPR && list_length(b->args) != 1) ||
			(b->boolop != NOT_EXPR && b->boolop != AND_EXPR && b->boolop != OR_EXPR))
			elog(ERROR, "invalid Boolean expression in PIVOT");

		if (b->boolop == NOT_EXPR)
			return psprintf("(NOT %s)",
							pivot_expr_sql((Node *) linitial(b->args), source_alias));

		sep = (b->boolop == AND_EXPR) ? " AND " : " OR ";
		initStringInfo(&buf);
		foreach(lc, b->args)
		{
			if (!first)
				appendStringInfoString(&buf, sep);
			appendStringInfoString(&buf,
								   pivot_expr_sql((Node *) lfirst(lc), source_alias));
			first = false;
		}

		return psprintf("(%s)", buf.data);
	}

	if (IsA(n, NullTest))
	{
		NullTest   *nt = castNode(NullTest, n);

		if (nt->arg == NULL ||
			(nt->nulltesttype != IS_NULL && nt->nulltesttype != IS_NOT_NULL))
			elog(ERROR, "invalid NULL in PIVOT expression");
		return psprintf("(%s IS%s NULL)",
						pivot_expr_sql((Node *) nt->arg, source_alias),
						nt->nulltesttype == IS_NOT_NULL ? " NOT" : "");
	}

	if (IsA(n, CaseExpr))
	{
		CaseExpr   *ce = castNode(CaseExpr, n);
		StringInfoData buf;
		ListCell   *lc;

		initStringInfo(&buf);
		appendStringInfoString(&buf, "CASE");
		if (ce->arg != NULL)
			appendStringInfo(&buf, " %s",
							 pivot_expr_sql((Node *) ce->arg, source_alias));
		if (ce->args != NIL &&
			(!IsA(ce->args, List) || ce->args->length == 0))
			elog(ERROR, "invalid CASE expr list in PIVOT|UNPIVOT");

		foreach(lc, ce->args)
		{
			CaseWhen   *cw = lfirst(lc);

			if (cw == NULL || !IsA(cw, CaseWhen) ||
				cw->expr == NULL || cw->result == NULL)
				elog(ERROR, "invalid CASE expr in PIVOT expression");

			appendStringInfo(&buf, " WHEN %s THEN %s",
							 pivot_expr_sql((Node *) cw->expr, source_alias),
							 pivot_expr_sql((Node *) cw->result, source_alias));
		}
		if (ce->defresult != NULL)
			appendStringInfo(&buf, " ELSE %s",
							 pivot_expr_sql((Node *) ce->defresult, source_alias));
		appendStringInfoString(&buf, " END");
		return psprintf("(%s)", buf.data);
	}

	if (IsA(n, CoalesceExpr))
	{
		CoalesceExpr *ce = castNode(CoalesceExpr, n);
		StringInfoData buf;
		ListCell   *lc;
		bool		first = true;

		if (ce->args == NIL)
			elog(ERROR, "missing COALESCE arguments in PIVOT expression");

		initStringInfo(&buf);
		appendStringInfoString(&buf, "COALESCE(");
		foreach(lc, ce->args)
		{
			if (!first)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf,
								   pivot_expr_sql((Node *) lfirst(lc), source_alias));
			first = false;
		}
		appendStringInfoChar(&buf, ')');
		return buf.data;
	}

	if (IsA(n, FuncCall))
	{
		FuncCall   *fcall = castNode(FuncCall, n);
		StringInfoData buf;
		ListCell   *lc;
		bool		first = true;
		char	   *fname = pivot_func_name(fcall);

		if (fcall->agg_star)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("unsupported PIVOT aggregate expression")));

		initStringInfo(&buf);
		appendStringInfo(&buf, "%s(", fname);

		if (fcall->agg_distinct)
			appendStringInfoString(&buf, "DISTINCT ");

		foreach(lc, fcall->args)
		{
			if (!first)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf,
								   pivot_expr_sql((Node *) lfirst(lc), source_alias));
			first = false;
		}
		appendStringInfoChar(&buf, ')');

		if (fcall->agg_order != NIL)
		{
			ListCell   *lc2;
			bool		first_order = true;

			if (fcall->agg_within_group)
				appendStringInfoString(&buf, " WITHIN GROUP (ORDER BY ");
			else
				appendStringInfoString(&buf, " ORDER BY ");

			foreach(lc2, fcall->agg_order)
			{
				SortBy	   *sb = lfirst_node(SortBy, lc2);

				if (!first_order)
					appendStringInfoString(&buf, ", ");
				appendStringInfoString(&buf, pivot_sortby_sql(sb, source_alias));
				first_order = false;
			}
			if (fcall->agg_within_group)
				appendStringInfoChar(&buf, ')');
		}

		return buf.data;
	}

	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("unsupported PIVOT aggregate expression")));

	return NULL;
}

/*
 * Build one ordinary PIVOT aggregate using the supplied IN-match predicate.
 * Wrap only the first argument in CASE WHEN <condition> THEN <argument> END;
 * subsequent arguments, such as a LISTAGG delimiter, are serialized unchanged.
 * The star form becomes CASE WHEN <condition> THEN 1 END, giving COUNT(*) its
 * conditional-count behavior.
 *
 * Preserve DISTINCT and the supplied ORDER BY expressions.  LISTAGG with an
 * ORDER BY is emitted with WITHIN GROUP even if agg_within_group was not set.
 * FILTER, OVER, and explicit VARIADIC options are rejected.  The generated SQL
 * is analyzed later to resolve the function and its arguments.
 */
static char *
pivot_agg_sql(FuncCall *fcall, const char *source_alias, const char *condsql)
{
	StringInfoData buf;
	ListCell   *lc;
	bool		first = true;
	char	   *fname;
	bool		use_within_group;
	Value	   *last_name;

	if (source_alias == NULL || condsql == NULL)
		elog(ERROR, "invalid PIVOT aggregate agruments");

	fname = pivot_func_name(fcall);
	use_within_group = fcall->agg_within_group;
	last_name = llast(fcall->funcname);

	if (last_name != NULL &&
		IsA(last_name, String) &&
		pg_strcasecmp(strVal(last_name), "listagg") == 0)
		fname = pstrdup("listagg");

	if (!use_within_group &&
		last_name != NULL &&
		IsA(last_name, String) &&
		pg_strcasecmp(strVal(last_name), "listagg") == 0 &&
		fcall->agg_order != NIL)
		use_within_group = true;

	if (fcall->agg_filter != NULL || fcall->over != NULL || fcall->func_variadic)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("unsupported PIVOT aggregate function option")));

	initStringInfo(&buf);
	appendStringInfo(&buf, "%s(", fname);

	if (fcall->agg_distinct)
		appendStringInfoString(&buf, "DISTINCT ");

	if (fcall->agg_star)
		appendStringInfo(&buf, "CASE WHEN %s THEN 1 END", condsql);
	else
	{
		foreach(lc, fcall->args)
		{
			char	   *arg = pivot_expr_sql((Node *) lfirst(lc), source_alias);

			if (!first)
				appendStringInfoString(&buf, ", ");

			if (first)
				appendStringInfo(&buf, "CASE WHEN %s THEN %s END", condsql, arg);
			else
				appendStringInfoString(&buf, arg);

			first = false;
		}
	}

	appendStringInfoChar(&buf, ')');

	if (fcall->agg_order != NIL)
	{
		ListCell   *lc2;
		bool		first_order = true;

		if (use_within_group)
			appendStringInfoString(&buf, " WITHIN GROUP (ORDER BY ");
		else
			appendStringInfoString(&buf, " ORDER BY ");

		foreach(lc2, fcall->agg_order)
		{
			SortBy	   *sb = lfirst(lc2);

			if (!first_order)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf, pivot_sortby_sql(sb, source_alias));
			first_order = false;
		}
		if (use_within_group)
			appendStringInfoChar(&buf, ')');
	}

	return buf.data;
}

/*
 * Parse a single generated PostgreSQL statement and analyze it as a subquery
 * of pstate.  raw_parser() produces the raw statement, and parse_sub_analyze()
 * resolves it in the caller's context.  The returned Query is used either for
 * result-type inspection or as the final rewritten subquery.
 */
static Query *
pivot_analyze_sql(ParseState *pstate, const char *sql)
{
	List	   *rawlist;
	RawStmt    *rawstmt;

	if (pstate == NULL || sql == NULL)
		elog(ERROR, "invalid PIVOT|UNPIVOT rewrite arguments");

	rawlist = raw_parser(sql, RAW_PARSE_DEFAULT);
	if (list_length(rawlist) != 1)
		elog(ERROR, "internal error: unexpected rewritten statement count");
	rawstmt = linitial_node(RawStmt, rawlist);

	return parse_sub_analyze(rawstmt->stmt, pstate, NULL, false, true);
}

/*
 * Analyze a copy of the source FROM item in a child ParseState, expand its
 * visible columns, and deparse a SELECT over that item with ivy_get_query_def().
 * The FROM-item transformer also handles nested PivotCall nodes, allowing
 * PIVOT/UNPIVOT chains to be rewritten recursively.
 *
 * Return lists of non-junk column names, type OIDs, and typmods in
 * source order.  The builders use these lists for implicit grouping,
 * UNPIVOT pass-through columns and result types, and PIVOT XML key coercions.
 */
static char *
pivot_build_source_sql(ParseState *pstate, Node *source, List **colnames,
					   List **coltypes, List **coltypmods)
{
	ParseState *spstate;
	ParseNamespaceItem *top_nsitem = NULL;
	List	   *namespace = NIL;
	Node	   *jointree_item;
	Query	   *query = makeNode(Query);
	List	   *tlist;
	ListCell   *lc;
	char	   *source_sql;

	if (pstate == NULL || source == NULL)
		elog(ERROR, "invalid PIVOT|UNPIVOT source sql");

	spstate = make_parsestate(pstate);
	jointree_item = transformFromClauseItemForPivot(spstate, copyObject(source),
													&top_nsitem, &namespace);
	if (jointree_item == NULL || top_nsitem == NULL)
		elog(ERROR, "invalid PIVOT/UNPIVOT source namespace");
	tlist = expandNSItemAttrs(spstate, top_nsitem, 0, -1);

	*colnames = NIL;
	*coltypes = NIL;
	*coltypmods = NIL;
	foreach(lc, tlist)
	{
		TargetEntry *tle = lfirst_node(TargetEntry, lc);

		if (tle->resjunk)
			continue;

		if (tle->resname == NULL || tle->expr == NULL)
			elog(ERROR, "invalid PIVOT|UNPIVOT source column");

		*colnames = lappend(*colnames, makeString(pstrdup(tle->resname)));
		*coltypes = lappend_oid(*coltypes, exprType((Node *) tle->expr));
		*coltypmods = lappend_int(*coltypmods, exprTypmod((Node *) tle->expr));
	}

	query->commandType = CMD_SELECT;
	query->querySource = QSRC_ORIGINAL;
	query->canSetTag = true;
	query->targetList = tlist;
	query->rtable = spstate->p_rtable;
	query->jointree = makeFromExpr(list_make1(jointree_item), NULL);
	query->hasSubLinks = false;
	query->hasWindowFuncs = false;
	query->hasTargetSRFs = false;
	query->hasAggs = false;
	query->hasDistinctOn = false;
	query->hasRecursive = false;
	query->hasModifyingCTE = false;
	query->hasForUpdate = false;
	query->hasRowSecurity = false;

	source_sql = ivy_get_query_def(query);
	free_parsestate(spstate);

	return source_sql;
}

/*
 * Serialize a function name by quoting each String component and joining
 * components with dots, preserving schema qualification.  Reject non-String
 * name components.
 */
static char *
pivot_func_name(FuncCall *fcall)
{
	StringInfoData buf;
	ListCell   *lc;
	bool		first = true;

	if (fcall == NULL || !IsA(fcall, FuncCall) || fcall->funcname == NIL ||
		!IsA(fcall->funcname, List) ||
		(fcall->agg_order != NIL && !IsA(fcall->agg_order, List)))
		elog(ERROR, "invalid function call in PIVOT|UNPIVOT");

	foreach(lc, fcall->agg_order)
	{
		SortBy	*sb = lfirst(lc);

		if (sb == NULL || !IsA(sb, SortBy) || sb->node == NULL)
			elog(ERROR, "invalid PIVOT aggregate ORDER BY item");
	}

	initStringInfo(&buf);
	foreach(lc, fcall->funcname)
	{
		Value	   *v = lfirst(lc);

		if (v == NULL || !IsA(v, String) || strVal(v) == NULL)
			elog(ERROR, "invalid function name in PIVOT|UNPIVOT");
		if (!first)
			appendStringInfoChar(&buf, '.');
		appendStringInfoString(&buf, quote_identifier(strVal(v)));
		first = false;
	}
	return buf.data;
}

/*
 * Build ordinary PIVOT as a SELECT of grouping columns and conditional
 * aggregates over the normalized source subquery.
 *
 * Implicit grouping starts with the visible source columns, excluding FOR
 * columns, columns referenced by aggregate arguments or aggregate ORDER BY,
 * and the anonymous name "?column?".  Preserve the remaining source order.
 * For example, with source columns noc, medal, sport, event, and gender:
 *
 *     PIVOT (COUNT(DISTINCT sport) AS sports
 *            FOR medal IN ('Gold' AS gold))
 *
 * produces a query of the following shape:
 *
 *     SELECT pvt_src.noc, pvt_src.event, pvt_src.gender,
 *            count(DISTINCT CASE WHEN pvt_src.medal = 'Gold'
 *                                THEN pvt_src.sport END) AS gold_sports
 *     FROM (<source SQL>) pvt_src
 *     GROUP BY pvt_src.noc, pvt_src.event, pvt_src.gender
 *     ORDER BY pvt_src.noc, pvt_src.event, pvt_src.gender
 *
 * Each IN item must provide one value per FOR column.  Analyze and evaluate
 * each value with transformPivotInValue(), then join the component equality
 * predicates with AND.  Reuse that predicate for every aggregate of the item.
 *
 * Use an explicit IN alias as the output-name prefix; otherwise join the
 * unfolded expression headings with underscores.  Append an aggregate alias
 * whenever present, including the single-aggregate case.  With multiple
 * aggregates, at most one may omit its alias.  Check generated names for
 * length and collisions with grouping or earlier aggregate output columns.
 * Emit GROUP BY and ORDER BY only when implicit grouping columns exist.
 */
static char *
pivot_build_pivot_sql(ParseState *pstate, PivotCall *pcall,
					  const char *source_sql, List *source_cols)
{
	PivotClause *pc;
	List	   *exclude_cols = NIL;
	List	   *group_cols = NIL;
	List	   *output_names = NIL;
	StringInfoData sql;
	StringInfoData selectbuf;
	StringInfoData groupbuf;
	ListCell   *lc;
	int			agg_count;
	int			unnamed_agg_count = 0;
	bool		has_group = false;
	bool		first_col = true;

	if (pstate == NULL || pcall == NULL || !IsA(pcall, PivotCall))
		elog(ERROR, "invalid pivot clause");
	pc = pcall->pivot;
	agg_count = list_length(pc->aggregate_funcs);

	/*
	 * FOR columns are pivoted into output columns, so they are not grouped
	 */
	foreach(lc, pc->for_cols)
	{
		Value	   *v = lfirst(lc);

		if ((v == NULL || !IsA(v, String)))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid FOR column in PIVOT")));
		exclude_cols = lappend(exclude_cols, makeString(pstrdup(strVal(v))));
	}

	foreach(lc, pc->aggregate_funcs)
	{
		ResTarget  *rt = lfirst_node(ResTarget, lc);
		FuncCall   *fcall;

		if (rt->val == NULL || !IsA(rt, ResTarget) ||
			rt->val == NULL || !IsA(rt->val, FuncCall))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PIVOT aggregate must be a function call")));
		fcall = castNode(FuncCall, rt->val);

		/*
		 * When multiple aggregate functions are used, aliases are required
		 * for all but at most one aggregate item.
		 */
		if (rt->name == NULL)
			unnamed_agg_count++;

		if (agg_count > 1 && unnamed_agg_count > 1)
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_COLUMN),
					 errmsg("column ambiguously defined"),
					 errhint("When multiple aggregate functions are used in PIVOT, "
							 "at most one aggregate expression may omit an alias.")));

		/*
		 * Aggregate arguments are measures, not grouping keys.
		 */
		if (!fcall->agg_star && fcall->args != NIL)
		{
			ListCell   *lc_arg;

			foreach(lc_arg, fcall->args)
				pivot_collect_colrefs((Node *) lfirst(lc_arg), &exclude_cols);
		}

		if (fcall->agg_order != NIL)
		{
			ListCell   *lc_order;

			foreach(lc_order, fcall->agg_order)
			{
				SortBy	   *sb = lfirst_node(SortBy, lc_order);

				pivot_collect_colrefs(sb->node, &exclude_cols);
			}
		}
	}

	/*
	 * Preserve source order for the remaining implicit grouping columns,
	 * skipping anonymous ?column? entries in the source metadata.
	 */
	foreach(lc, source_cols)
	{
		Value	   *v = lfirst(lc);

		if ((v == NULL || !IsA(v, String)))
			continue;
		if (strcmp(strVal(v), "?column?") == 0)
			continue;
		if (!pivot_has_name(exclude_cols, strVal(v)))
			group_cols = lappend(group_cols, makeString(pstrdup(strVal(v))));
	}

	initStringInfo(&selectbuf);
	initStringInfo(&groupbuf);

	/* Emit pass-through grouping columns into both SELECT and GROUP BY. */
	foreach(lc, group_cols)
	{
		Value	   *v = lfirst(lc);
		char	   *q = pivot_quote_qualified("pvt_src", strVal(v));

		if (!first_col)
		{
			appendStringInfoString(&selectbuf, ", ");
			appendStringInfoString(&groupbuf, ", ");
		}
		appendStringInfoString(&selectbuf, q);
		appendStringInfoString(&groupbuf, q);
		output_names = lappend(output_names,
						   makeString(pstrdup(strVal(v))));
		first_col = false;
		has_group = true;
	}

	/*
	 * Evaluate each IN component once and combine its FOR-column equality
	 * predicate with the others.  Reuse the predicate and base heading for
	 * each aggregate output generated from this IN item.
	 */
	foreach(lc, pc->in_list)
	{
		PivotInItem *initem = lfirst(lc);
		StringInfoData condbuf;
		StringInfoData labelbuf;
		char	   *base_label;
		ListCell   *lc2;
		int			pos = 0;
		ListCell   *lc3;

		if (initem == NULL || !IsA(initem, PivotInItem) ||
			initem->values == NIL || !IsA(initem->values, List))
			elog(ERROR, "invalid PIVOT|UNPIVOT in item");

		if (list_length(initem->values) != list_length(pc->for_cols))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("PIVOT value count does not match FOR column count")));

		initStringInfo(&condbuf);
		initStringInfo(&labelbuf);
		forboth(lc2, pc->for_cols, lc3, initem->values)
		{
			Value	   *forcol = lfirst(lc2);
			Node	   *valnode = lfirst(lc3);
			char	   *value_label = NULL;
			Const	   *value;
			char	   *valsql;

			value = transformPivotInValue(pstate, valnode,
										  initem->labels == NIL ? &value_label : NULL, false);
			valsql = deparse_expression((Node *) value, NIL, false, false);

			if ((forcol == NULL || !IsA(forcol, String)))
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("invalid FOR column in PIVOT")));
			if (pos++ > 0)
			{
				appendStringInfoString(&condbuf, " AND ");
				appendStringInfoChar(&labelbuf, '_');
			}
			if (value_label != NULL)
				appendStringInfoString(&labelbuf, value_label);
			appendStringInfo(&condbuf, "%s = %s",
							 pivot_quote_qualified("pvt_src", strVal(forcol)),
							 valsql);
		}

		base_label = initem->labels == NIL ? labelbuf.data :
			pivot_label_text(linitial(initem->labels));

		/* Generate one output column per aggregate for this IN value */
		foreach(lc2, pc->aggregate_funcs)
		{
			ResTarget  *rt = lfirst_node(ResTarget, lc2);
			FuncCall   *fcall = castNode(FuncCall, rt->val);
			char	   *aggsql;
			char	   *out_label;
			const char *qout_label;

			aggsql = pivot_agg_sql(fcall, "pvt_src", condbuf.data);

			/* Append an aggregate alias whenever the syntax supplied one. */
			if (rt->name != NULL)
				out_label = psprintf("%s_%s", base_label, rt->name);
			else
				out_label = base_label;

			pivot_add_output_name(&output_names, out_label);
			qout_label = quote_identifier(out_label);
			if (!first_col)
				appendStringInfoString(&selectbuf, ", ");
			appendStringInfo(&selectbuf, "%s AS %s",
							 aggsql, qout_label);
			first_col = false;
		}
	}

	initStringInfo(&sql);
	appendStringInfo(&sql, "SELECT %s FROM (%s) pvt_src",
					 selectbuf.data, source_sql);
	if (has_group)
	{
		appendStringInfo(&sql, " GROUP BY %s", groupbuf.data);
		appendStringInfo(&sql, " ORDER BY %s", groupbuf.data);
	}

	return sql.data;
}

/*
 * Find the first exact name match in the source metadata lists and
 * return its analyzed type OID and typmod.  Report an undefined-column error
 * when no visible source column matches.
 */
static void
pivot_source_col_type(List *source_cols, List *source_types,
					  List *source_typmods, const char *name,
					  Oid *type_oid, int32 *typmod)
{
	ListCell   *lc_name;
	ListCell   *lc_type;
	ListCell   *lc_typmod;

	if (list_length(source_cols) != list_length(source_types) ||
		list_length(source_cols) != list_length(source_typmods) ||
		(source_types != NIL && !IsA(source_types, OidList)) ||
		(source_typmods != NIL && !IsA(source_typmods, IntList)))
		elog(ERROR, "inconsistent PIVOT|UNPIVOT source column metadata");

	for (lc_name = list_head(source_cols),
		 lc_type = list_head(source_types),
		 lc_typmod = list_head(source_typmods);
		 lc_name != NULL;
		 lc_name = lnext(source_cols, lc_name),
		 lc_type = lnext(source_types, lc_type),
		 lc_typmod = lnext(source_typmods, lc_typmod))
	{
		Value	   *colname = lfirst(lc_name);

		if (IsA(colname, String) && strcmp(strVal(colname), name) == 0)
		{
			*type_oid = lfirst_oid(lc_type);
			*typmod = lfirst_int(lc_typmod);
			return;
		}
	}

	ereport(ERROR,
			(errcode(ERRCODE_UNDEFINED_COLUMN),
			 errmsg("column \"%s\" does not exist", name)));
}

/*
 * Classify the listed Oracle and PostgreSQL type OIDs as character, numeric,
 * or other for UNPIVOT result-type selection.
 */
static PivotUnpivotTypeGroup
pivot_unpivot_type_group(Oid type_oid)
{
	switch (type_oid)
	{
		case ORACHARCHAROID:
		case ORACHARBYTEOID:
		case ORAVARCHARCHAROID:
		case ORAVARCHARBYTEOID:
		case ORACHARCHAR_2OID:
		case ORACHARBYTE_2OID:
		case ORAVARCHARCHAR_2OID:
		case ORAVARCHARBYTE_2OID:
		case BPCHAROID:
		case VARCHAROID:
		case TEXTOID:
			return PIVOT_UNPIVOT_TYPE_CHARACTER;

		case NUMBEROID:
		case BINARY_FLOATOID:
		case BINARY_DOUBLEOID:
		case INT2OID:
		case INT4OID:
		case INT8OID:
		case NUMERICOID:
		case FLOAT4OID:
		case FLOAT8OID:
			return PIVOT_UNPIVOT_TYPE_NUMERIC;

		default:
			return PIVOT_UNPIVOT_TYPE_OTHER;
	}
}

/*
 * Return true for the varying-width character types recognized by UNPIVOT,
 * including PostgreSQL text.  Fixed-width CHAR types return false.
 */
static bool
pivot_unpivot_is_varchar(Oid type_oid)
{
	return type_oid == ORAVARCHARCHAROID ||
		type_oid == ORAVARCHARBYTEOID ||
		type_oid == ORAVARCHARCHAR_2OID ||
		type_oid == ORAVARCHARBYTE_2OID ||
		type_oid == VARCHAROID || type_oid == TEXTOID;
}

/*
 * Rank a numeric-group input: double precision is 3, single precision is 2,
 * and the remaining numeric types are 1.  Call only for numeric-group types.
 */
static int
pivot_unpivot_numeric_rank(Oid type_oid)
{
	if (pivot_unpivot_type_group(type_oid) != PIVOT_UNPIVOT_TYPE_NUMERIC)
		elog(ERROR, "non-numeric type in UNPIVOT numeric promotion");

	if (type_oid == BINARY_DOUBLEOID || type_oid == FLOAT8OID)
		return 3;
	if (type_oid == BINARY_FLOATOID || type_oid == FLOAT4OID)
		return 2;
	return 1;
}

/*
 * Choose the SQL type spelling for one zero-based UNPIVOT measure position.
 * Inspect the corresponding input column in every IN item and require a
 * consistent character, numeric, or other type group.
 *
 * Character inputs retain the first type unless a varying-width type is
 * encountered after a fixed-width type.  Use the largest typmod, or no typmod
 * if any input is unconstrained.  Numeric inputs map to Oracle BINARY_DOUBLE,
 * BINARY_FLOAT, or NUMBER in that precedence order, without a typmod.
 * Other inputs must share a base type; retain the first input type and keep
 * its typmod only if all input typmods agree.
 *
 * The builder casts each measure to this type in every lateral VALUES tuple,
 * rather than relying on VALUES common-type selection alone.
 */
static char *
pivot_unpivot_target_type(PivotClause *pc, int target_index,
						  List *source_cols, List *source_types,
						  List *source_typmods)
{
	PivotUnpivotTypeGroup group = PIVOT_UNPIVOT_TYPE_OTHER;
	ListCell   *lc;
	Oid			result_type = InvalidOid;
	Oid			other_base_type = InvalidOid;
	int32		result_typmod = -1;
	bool		first = true;
	bool		result_is_varchar = false;
	int			numeric_rank = 0;

	if (pc == NULL || !IsA(pc, PivotClause))
		elog(ERROR, "invalid UNPIVOT clause node");

	foreach(lc, pc->in_list)
	{
		PivotInItem *initem = lfirst(lc);
		Node	   *value;
		char	   *colname;
		Oid			type_oid;
		Oid			base_type;
		int32		typmod;
		int32		base_typmod;
		PivotUnpivotTypeGroup this_group;

		if (initem == NULL || !IsA(initem, PivotInItem) || initem->values == NIL)
			elog(ERROR, "invalid PIVOT|UNPIVOT IN item");

		if (target_index >= list_length(initem->values))
			elog(ERROR, "invalid UNPIVOT input column index");

		value = list_nth(initem->values, target_index);
		colname = pivot_simple_colname(value);
		pivot_source_col_type(source_cols, source_types, source_typmods,
							  colname, &type_oid, &typmod);
		this_group = pivot_unpivot_type_group(type_oid);
		base_typmod = typmod;
		base_type = getBaseTypeAndTypmod(type_oid, &base_typmod);

		if (first)
		{
			group = this_group;
			result_type = type_oid;
			result_typmod = typmod;
			result_is_varchar = pivot_unpivot_is_varchar(type_oid);
			other_base_type = base_type;
			first = false;
		}
		else if (this_group != group)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("UNPIVOT value columns must be in the same datatype group"),
					 errdetail("Column \"%s\" has type %s, which is incompatible with the other columns for output position %d.",
							   colname, format_type_be(type_oid), target_index + 1)));

		if (group == PIVOT_UNPIVOT_TYPE_CHARACTER)
		{
			bool		this_is_varchar = pivot_unpivot_is_varchar(type_oid);

			if (this_is_varchar && !result_is_varchar)
			{
				result_type = type_oid;
				result_is_varchar = true;
			}
			if (typmod < 0 || result_typmod < 0)
				result_typmod = -1;
			else if (typmod > result_typmod)
				result_typmod = typmod;
		}
		else if (group == PIVOT_UNPIVOT_TYPE_NUMERIC)
		{
			int			this_rank = pivot_unpivot_numeric_rank(type_oid);

			if (this_rank > numeric_rank)
				numeric_rank = this_rank;
			result_typmod = -1;
		}
		else
		{
			if (base_type != other_base_type)
				ereport(ERROR,
						(errcode(ERRCODE_DATATYPE_MISMATCH),
						 errmsg("UNPIVOT value columns must be in the same datatype group"),
						 errdetail("Column \"%s\" has type %s, which is incompatible with the other columns for output position %d.",
								   colname, format_type_be(type_oid), target_index + 1)));
			if (typmod != result_typmod)
				result_typmod = -1;
		}
	}

	if (group == PIVOT_UNPIVOT_TYPE_NUMERIC)
	{
		if (numeric_rank == 0)
			numeric_rank = pivot_unpivot_numeric_rank(result_type);
		if (numeric_rank == 3)
			result_type = BINARY_DOUBLEOID;
		else if (numeric_rank == 2)
			result_type = BINARY_FLOATOID;
		else
			result_type = NUMBEROID;
	}

	return format_type_with_typemod(result_type, result_typmod);
}

/*
 * Build UNPIVOT as a SELECT over a source subquery crossed with a lateral
 * VALUES relation.  Each source row supplies the input values for one VALUES
 * tuple per IN item.  For two measures, the generated SQL has this shape:
 *
 *     SELECT unp_src.noc, unp.medal, unp.medal_count, unp.sport_count
 *     FROM (<source SQL>) unp_src
 *     CROSS JOIN LATERAL (VALUES
 *         ('GOLD', CAST(unp_src.gold_medals AS <measure-1 type>),
 *                  CAST(unp_src.gold_sports AS <measure-2 type>)),
 *         ('SILVER', CAST(unp_src.silver_medals AS <measure-1 type>),
 *                    CAST(unp_src.silver_sports AS <measure-2 type>))
 *     ) unp(medal, medal_count, sport_count)
 *     WHERE unp.medal_count IS NOT NULL OR unp.sport_count IS NOT NULL
 *
 * The source appears once in the generated FROM clause.  Values in each tuple
 * are ordered as FOR labels followed by measures; the lateral column alias
 * list uses the same order.  The outer SELECT emits pass-through source
 * columns, FOR columns, and measures, in that order, without aggregation.
 *
 * Every IN item must have as many input columns as the UNPIVOT target list.
 * Choose a common result type for each measure position with
 * pivot_unpivot_target_type(), and explicitly cast every corresponding input
 * column in VALUES.  Input columns consumed by any IN item, and anonymous
 * "?column?" columns, are omitted from the pass-through projection.
 *
 * Explicit labels use the common expression grammar and must match the
 * FOR-column count.  Analyze constant expressions before deparsing so their type
 * modifiers survive the rewrite.  Multiple FOR columns require labels; with
 * a single FOR column and no label, use the first input column's name as a
 * string literal, including for a multi-measure IN item.
 *
 * EXCLUDE NULLS is the default: retain a generated row when at least one
 * measure is non-NULL, using OR across the measure columns.  INCLUDE NULLS
 * omits this WHERE clause.  The builder does not add an ORDER BY.
 */
static char *
pivot_build_unpivot_sql(ParseState *pstate, PivotCall *pcall,
						const char *source_sql,
						List *source_cols, List *source_types,
						List *source_typmods)
{
	PivotClause *pc = pcall->pivot;
	List	   *exclude_cols = NIL;
	List	   *group_cols = NIL;
	List	   *target_types = NIL;
	StringInfoData sql;
	StringInfoData valuesbuf;
	StringInfoData aliasbuf;
	StringInfoData wherebuf;
	ListCell   *lc;
	bool		first_value = true;
	bool		first_alias = true;
	bool		first_out = true;
	int			for_col_count;
	int			target_index;

	if (pstate == NULL || pcall == NULL)
		elog(ERROR, "invalid UNPIVOT clause");
	pc = pcall->pivot;

	if (pc->for_cols == NIL || pc->in_list == NIL || pc->unpivot_targets == NIL)
		elog(ERROR, "invalid UNPIVOT clause");
	for_col_count = list_length(pc->for_cols);

	/*
	 * UNPIVOT input columns are consumed and must not be projected unchanged.
	 */
	foreach(lc, pc->in_list)
	{
		PivotInItem *initem = lfirst_node(PivotInItem, lc);
		ListCell   *lc2;

		if (list_length(initem->values) != list_length(pc->unpivot_targets))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("UNPIVOT target column count does not match IN list item")));

		foreach(lc2, initem->values)
		{
			char	   *col = pivot_simple_colname(lfirst(lc2));

			if (!pivot_has_name(exclude_cols, col))
				exclude_cols = lappend(exclude_cols, makeString(col));
		}
	}

	/*
	 * Preserve named, non-consumed source columns in source order.
	 */
	foreach(lc, source_cols)
	{
		Value	   *v = lfirst(lc);

		if ((v == NULL || !IsA(v, String)))
			continue;
		if (strcmp(strVal(v), "?column?") == 0)
			continue;
		if (!pivot_has_name(exclude_cols, strVal(v)))
			group_cols = lappend(group_cols, makeString(pstrdup(strVal(v))));
	}

	initStringInfo(&valuesbuf);
	for (target_index = 0;
		 target_index < list_length(pc->unpivot_targets);
		 target_index++)
		target_types = lappend(target_types,
							   makeString(pivot_unpivot_target_type(pc,
														 target_index,
														 source_cols,
														 source_types,
														 source_typmods)));

	/*
	 * Each IN item contributes one lateral VALUES tuple: first its FOR
	 * labels, then its input measures cast to the chosen per-position types.
	 */
	foreach(lc, pc->in_list)
	{
		PivotInItem *initem = lfirst_node(PivotInItem, lc);
		ListCell   *lc2;
		ListCell   *lc_target;
		int			label_count = list_length(initem->labels);
		int			value_index = 0;

		if (list_length(initem->values) != list_length(pc->unpivot_targets))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("UNPIVOT target column count does not match IN list item")));

		if (initem->labels != NIL && label_count != for_col_count)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("UNPIVOT label count does not match FOR column count")));

		if (initem->labels == NIL && for_col_count != 1)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("UNPIVOT label is required for multiple FOR columns")));

		if (!first_value)
			appendStringInfoString(&valuesbuf, ", ");
		first_value = false;
		appendStringInfoChar(&valuesbuf, '(');

		/*
		 * Use explicit labels such as AS 'GOLD' or AS (2024, 'Q1');
		 * otherwise, use the first input column name as the single FOR value.
		 */
		if (initem->labels != NIL)
		{
			bool		first_label = true;
			ListCell   *lc_label;

			foreach(lc_label, initem->labels)
			{
				Const	   *label;

				/*
				 * Evaluate the constant label, preserving its type and typmod.
				 */
				label = transformPivotInValue(pstate, lfirst(lc_label), NULL, true);
				if (!first_label)
					appendStringInfoString(&valuesbuf, ", ");
				appendStringInfoString(&valuesbuf,
									   deparse_expression((Node *) label, NIL, false, false));
				first_label = false;
			}
		}
		else
			appendStringInfoString(&valuesbuf,
								   quote_literal_cstr(pivot_simple_colname(linitial(initem->values))));

		/*
		 * Cast each input measure to its position's selected result type.
		 */
		forboth(lc2, initem->values, lc_target, pc->unpivot_targets)
		{
			Node	   *valnode = lfirst(lc2);
			Value	   *target = lfirst(lc_target);
			char	   *col = pivot_simple_colname(valnode);

			if (!IsA(target, String))
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("invalid target column in UNPIVOT")));
			appendStringInfo(&valuesbuf, ", CAST(%s AS %s)",
							 pivot_quote_qualified("unp_src", col),
							 strVal(list_nth(target_types, value_index++)));
		}
		appendStringInfoChar(&valuesbuf, ')');
	}

	/*
	 * Name the lateral VALUES columns: FOR labels first, then measures.
	 */
	initStringInfo(&aliasbuf);

	foreach(lc, pc->for_cols)
	{
		Value	   *forcol = lfirst(lc);

		if ((forcol == NULL || !IsA(forcol, String)))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid FOR column in UNPIVOT")));
		if (!first_alias)
			appendStringInfoString(&aliasbuf, ", ");
		appendStringInfoString(&aliasbuf, quote_identifier(strVal(forcol)));
		first_alias = false;
	}

	foreach(lc, pc->unpivot_targets)
	{
		Value	   *target = lfirst(lc);

		if (!IsA(target, String))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid target column in UNPIVOT")));
		if (!first_alias)
			appendStringInfoString(&aliasbuf, ", ");
		appendStringInfoString(&aliasbuf, quote_identifier(strVal(target)));
		first_alias = false;
	}

	/*
	 * Build the outer SELECT list: pass-through columns, FOR columns, targets.
	 */
	initStringInfo(&sql);
	appendStringInfoString(&sql, "SELECT ");
	foreach(lc, group_cols)
	{
		Value	   *v = lfirst(lc);

		if (!first_out)
			appendStringInfoString(&sql, ", ");
		appendStringInfoString(&sql, pivot_quote_qualified("unp_src", strVal(v)));
		first_out = false;
	}

	foreach(lc, pc->for_cols)
	{
		Value	   *forcol = lfirst(lc);

		if ((forcol == NULL || !IsA(forcol, String)))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid FOR column in UNPIVOT")));
		if (!first_out)
			appendStringInfoString(&sql, ", ");
		appendStringInfo(&sql, "unp.%s", quote_identifier(strVal(forcol)));
		first_out = false;
	}

	foreach(lc, pc->unpivot_targets)
	{
		Value	   *target = lfirst(lc);

		if (!first_out)
			appendStringInfoString(&sql, ", ");
		appendStringInfo(&sql, "unp.%s", quote_identifier(strVal(target)));
		first_out = false;
	}

	appendStringInfo(&sql,
					 " FROM (%s) unp_src CROSS JOIN LATERAL (VALUES %s) unp(%s)",
					 source_sql, valuesbuf.data, aliasbuf.data);

	/*
	 * Oracle defaults to EXCLUDE NULLS.  With multiple target columns, Oracle
	 * keeps a row if any target column in that generated row is non-NULL.
	 */
	if (!pc->include_nulls)
	{
		bool		first_cond = true;

		initStringInfo(&wherebuf);
		foreach(lc, pc->unpivot_targets)
		{
			Value	   *target = lfirst(lc);

			if (!first_cond)
				appendStringInfoString(&wherebuf, " OR ");
			appendStringInfo(&wherebuf, "unp.%s IS NOT NULL",
							 quote_identifier(strVal(target)));
			first_cond = false;
		}
		if (wherebuf.len > 0)
			appendStringInfo(&sql, " WHERE %s", wherebuf.data);
	}

	return sql.data;
}

/*
 * Find the alias to inherit from the immediate source FROM item.
 * Return aliases from supported relation, subquery, function, and table-function
 * nodes, recurse through TABLESAMPLE, and prefer a PivotCall's own alias before
 * looking deeper into its source chain.  Return NULL when none is available.
 * The returned pointer belongs to the source tree; the caller copies it.
 */
static Alias *
pivot_source_alias(Node *source)
{
	check_stack_depth();

	if (source == NULL)
		return NULL;

	switch (nodeTag(source))
	{
		case T_RangeVar:
			return ((RangeVar *) source)->alias;
		case T_RangeSubselect:
			return ((RangeSubselect *) source)->alias;
		case T_RangeFunction:
			return ((RangeFunction *) source)->alias;
		case T_RangeTableFunc:
			return ((RangeTableFunc *) source)->alias;
		case T_RangeTableSample:
			return pivot_source_alias(((RangeTableSample *) source)->relation);
		case T_PivotCall:
			{
				PivotCall  *pcall = (PivotCall *) source;

				/*
				 * A parenthesized operation can attach an alias before the
				 * next operation in a chain.  That alias names the immediate
				 * source table reference and must take precedence over aliases
				 * belonging to relations deeper in the chain.
				 */
				if (pcall->alias != NULL)
					return pcall->alias;
				return pivot_source_alias(pcall->source);
			}
		default:
			return NULL;
	}
}

/*
 * Serialize an aggregate for PIVOT XML without a CASE predicate.
 * The XML builder obtains group/value pairs from the source for ANY, or from
 * all groups crossed with the subquery values.  It then left-joins matching
 * source rows and groups those pairs before constructing XML.
 *
 * count_col is SQL text for the non-NULL source-row marker: replacing the star
 * argument with it prevents COUNT(*) from counting a NULL-extended unmatched
 * row.  Other arguments are
 * serialized unchanged.  DISTINCT and ORDER BY follow pivot_agg_sql()'s
 * rules, including LISTAGG WITHIN GROUP; FILTER, OVER, and VARIADIC are rejected.
 */
static char *
pivot_agg_plain_sql(FuncCall *fcall, const char *source_alias, const char *count_col)
{
	StringInfoData buf;
	ListCell   *lc;
	bool		first = true;
	char	   *fname;
	bool		use_within_group;
	Value	   *last_name;

	if (source_alias == NULL || count_col == NULL)
		elog(ERROR, "invalid PIVOT XML aggregate arguments");

	fname = pivot_func_name(fcall);
	use_within_group = fcall->agg_within_group;
	last_name = llast(fcall->funcname);

	if (last_name != NULL &&
		IsA(last_name, String) &&
		pg_strcasecmp(strVal(last_name), "listagg") == 0)
		fname = pstrdup("listagg");
	else
		fname = pivot_func_name(fcall);

	if (!use_within_group &&
		last_name != NULL &&
		IsA(last_name, String) &&
		pg_strcasecmp(strVal(last_name), "listagg") == 0 &&
		fcall->agg_order != NIL)
		use_within_group = true;

	if (fcall->agg_filter != NULL || fcall->over != NULL || fcall->func_variadic)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("unsupported PIVOT aggregate function option")));

	initStringInfo(&buf);
	appendStringInfo(&buf, "%s(", fname);

	if (fcall->agg_distinct)
		appendStringInfoString(&buf, "DISTINCT ");

	if (fcall->agg_star)
		appendStringInfoString(&buf, count_col);
	else
	{
		foreach(lc, fcall->args)
		{
			if (!first)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf,
								   pivot_expr_sql((Node *) lfirst(lc), source_alias));
			first = false;
		}
	}

	appendStringInfoChar(&buf, ')');

	if (fcall->agg_order != NIL)
	{
		ListCell   *lc2;
		bool		first_order = true;

		if (use_within_group)
			appendStringInfoString(&buf, " WITHIN GROUP (ORDER BY ");
		else
			appendStringInfoString(&buf, " ORDER BY ");

		foreach(lc2, fcall->agg_order)
		{
			SortBy	   *sb = lfirst_node(SortBy, lc2);

			if (!first_order)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf, pivot_sortby_sql(sb, source_alias));
			first_order = false;
		}
		if (use_within_group)
			appendStringInfoChar(&buf, ')');
	}

	return buf.data;
}

/*
 * Return the text and quote state for an aggregate's XML column name attribute.
 * Prefer the explicit aggregate alias and its recorded quote flag; otherwise
 * use the last function-name component or the fallback AGG<idx>, unquoted.
 * idx is one-based.  The caller applies pivot_xml_identifier_name() before
 * embedding the text as an XML attribute value.
 */
static char *
pivot_agg_xml_label(PivotClause *pc, ResTarget *rt, int idx, bool *quoted)
{
	FuncCall   *fcall;
	Value	   *last_name;

	if (pc == NULL || rt == NULL ||
		!IsA(rt, ResTarget) || rt->val == NULL || !IsA(rt->val, FuncCall))
		elog(ERROR, "invalid PIVOT XML aggregate label arguments");

	fcall = (FuncCall *) rt->val;
	if (fcall->funcname == NIL || !IsA(fcall->funcname, List))
		elog(ERROR, "invalid PIVOT XML aggregate function name");

	if (rt->name != NULL)
	{
		if (idx < 1 || idx > list_length(pc->aggregate_alias_quoted))
			elog(ERROR, "invalid PIVOT XML aggregate index");
		*quoted = list_nth_int(pc->aggregate_alias_quoted, idx - 1);
		return pstrdup(rt->name);
	}

	*quoted = false;
	last_name = list_length(fcall->funcname) > 0 ? llast(fcall->funcname) : NULL;
	if (last_name != NULL && IsA(last_name, String))
	{
		if (strVal(last_name) == NULL)
			elog(ERROR, "null PIVOT XML aggregate function name");
		return pstrdup(strVal(last_name));
	}

	return psprintf("AGG%d", idx);
}

/*
 * Append qualified grouping-column references in list order, separated by
 * commas.  No leading or trailing comma is added.  PIVOT XML reuses this helper
 * for group/value pairs, aggregate grouping, and the final SELECT/GROUP BY.
 */
static void
pivot_append_group_cols(StringInfo buf, const char *alias, List *group_cols)
{
	ListCell   *lc;
	bool		first = true;

	if (buf == NULL || alias == NULL)
		elog(ERROR, "invalid PIVOT grouping output arguments");

	foreach(lc, group_cols)
	{
		Value	   *v = lfirst(lc);

		if (!first)
			appendStringInfoString(buf, ", ");
		appendStringInfoString(buf, pivot_quote_qualified(alias, strVal(v)));
		first = false;
	}
}

/*
 * Append alias.__pvt_xml_value_1 through alias.__pvt_xml_value_N, separated
 * by commas without a leading or trailing comma.  alias must be an internal
 * SQL alias supplied by the XML builder.
 */
static void
pivot_append_xml_value_cols(StringInfo buf, const char *alias, int value_count)
{
	int			idx;

	for (idx = 1; idx <= value_count; idx++)
	{
		if (idx > 1)
			appendStringInfoString(buf, ", ");
		appendStringInfo(buf, "%s.__pvt_xml_value_%d", alias, idx);
	}
}

/*
 * Append component-wise IS NOT DISTINCT FROM comparisons for two internal
 * XML value tuples, joined with AND.  The XML subquery path uses this NULL-safe
 * predicate to detect duplicate tuples after coercion to the FOR-column types.
 */
static void
pivot_append_xml_value_match(StringInfo buf, const char *left_alias,
							 const char *right_alias, int value_count)
{
	int			idx;

	for (idx = 1; idx <= value_count; idx++)
	{
		if (idx > 1)
			appendStringInfoString(buf, " AND ");
		appendStringInfo(buf,
						 "%s.__pvt_xml_value_%d IS NOT DISTINCT FROM "
						 "%s.__pvt_xml_value_%d",
						 left_alias, idx, right_alias, idx);
	}
}

/*
 * Choose an internal name absent from source_cols: try base_name first, then
 * append increasing numeric suffixes.  The XML builder uses this for its
 * non-NULL source-row marker.
 */
static char *
pivot_internal_colname(List *source_cols, const char *base_name)
{
	char	   *candidate;
	int			suffix = 0;

	if (base_name == NULL)
		elog(ERROR, "null PIVOT internal column name");
	candidate = pstrdup(base_name);

	while (pivot_has_name(source_cols, candidate))
		candidate = psprintf("%s_%d", base_name, ++suffix);

	return candidate;
}

/*
 * Build PIVOT XML as a CTE query producing implicit grouping columns and one
 * XML column per group.  Accept only IN (ANY) and IN (subquery); explicit IN
 * values are rejected, and the ANY count must match the FOR-column count.
 *
 * The generated CTEs are:
 *
 *   pvt_src
 *     The normalized source plus a non-NULL marker whose name does not collide
 *     with source columns.  COUNT(*) uses this marker after the left join.
 *
 *   pvt_groups
 *     Distinct implicit groups, excluding FOR columns, aggregate argument and
 *     ORDER BY dependencies, and "?column?".  Without grouping columns this
 *     is SELECT 1 AS __pvt_dummy.  The subquery path uses it to form pairs.
 *
 *   pvt_vals_raw / pvt_vals  (subquery path only)
 *     Analyze the IN subquery and require one output column per FOR column.
 *     pvt_vals_raw casts each component to its FOR column's type and typmod.
 *     pvt_vals uses a correlated scalar subquery with NULL-safe tuple equality
 *     to enforce uniqueness at execution: duplicate matches cause a scalar
 *     subquery cardinality error, rather than being silently deduplicated.
 *
 *   pvt_pair
 *     SELECT DISTINCT group/value pairs.  ANY takes pairs directly from
 *     pvt_src, so each group contains only its own observed values.  The
 *     subquery path crosses pvt_groups with pvt_vals, including values for
 *     which a group has no matching source row.
 *
 *   pvt_agg
 *     Left-join pvt_pair to pvt_src with IS NOT DISTINCT FROM on every group
 *     and FOR component, then aggregate by the group/value tuple.  The source
 *     marker makes COUNT(*) return zero for an unmatched pair.
 *
 * The final SELECT builds XML with xmlelement, xmlattributes, xmlconcat, and
 * xmlagg, ordering <item> elements by the composite pivot value.  Each item
 * contains FOR-value columns followed by aggregate-result columns:
 *
 *     <PivotSet>
 *       <item>
 *         <column name="FOR_COLUMN_1">...</column>
 *         <column name="FOR_COLUMN_2">...</column>
 *         <column name="MEASURE_ALIAS">...</column>
 *       </item>
 *     </PivotSet>
 *
 * The XML name attribute preserves quoted identifiers and uppercases unquoted
 * ones; SQL/XML handles escaping and serializes key and aggregate values
 * using their original types.  The output identifier joins
 * FOR names with underscores, lowercases the name, and appends _xml, with
 * length and grouping-column collision checks.  Final rows are grouped and
 * ordered by the implicit keys,
 * or grouped by the dummy key when there are no implicit keys.
 */
static char *
pivot_build_pivot_xml_sql(ParseState *pstate, PivotCall *pcall,
						  const char *source_sql, List *source_cols,
						  List *source_types, List *source_typmods)
{
	PivotClause *pc;
	List	   *exclude_cols = NIL;
	List	   *group_cols = NIL;
	StringInfoData sql;
	StringInfoData groupsel;
	StringInfoData groupby;
	StringInfoData pairsel;
	StringInfoData pairfrom;
	StringInfoData joincond;
	StringInfoData aggsel;
	StringInfoData finalsel;
	StringInfoData finalgroup;
	StringInfoData xmlcolumns;
	StringInfoData xmlorder;
	ListCell   *lc;
	ListCell   *lc2;
	int			for_col_count;
	int			for_idx;
	int			agg_idx = 1;
	bool		first = true;
	bool		has_group = false;
	char	   *count_col_ref;
	char	   *present_col;
	char	   *subquery_sql = NULL;
	char	   *xml_output_name;

	if (pstate == NULL || pcall == NULL || source_sql == NULL)
		elog(ERROR, "invalid PIVOT XML SQL");
	pc = pcall->pivot;

	if (pc->for_cols == NIL || pc->in_list == NIL || pc->aggregate_funcs == NIL)
		elog(ERROR, "invalid PIVOT clause node");
	for_col_count = list_length(pc->for_cols);

	if (!pc->is_xml)
		elog(ERROR, "internal error: PIVOT XML builder called for non-XML PIVOT");

	if (pc->in_kind == PIVOT_IN_VALUES)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("explicit pivot values are not allowed with PIVOT XML")));

	if (pc->in_kind != PIVOT_IN_SUBQUERY && pc->in_kind != PIVOT_IN_ANY)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("unsupported PIVOT XML IN clause")));

	if (for_col_count == 0)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("PIVOT XML requires at least one FOR column")));

	/*
	 * Quote metadata was checked at the FROM-item entry point.
	 */
	present_col = pivot_internal_colname(source_cols, "__pvt_present");
	count_col_ref = pivot_quote_qualified("pvt_src", present_col);

	if (pc->in_kind == PIVOT_IN_ANY)
	{
		PivotInItem *initem;

		if (list_length(pc->in_list) != 1)
			elog(ERROR, "internal error: invalid PIVOT XML ANY list");
		initem = linitial_node(PivotInItem, pc->in_list);
		if (list_length(initem->values) != for_col_count)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("PIVOT XML ANY count does not match FOR column count")));
	}

	/*
	 * Every FOR column is part of the composite pivot key, not a group key.
	 */
	foreach(lc, pc->for_cols)
	{
		Value	   *forcol = lfirst(lc);

		if ((forcol == NULL || !IsA(forcol, String)))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid FOR column in PIVOT")));

		exclude_cols = lappend(exclude_cols,
							   makeString(pstrdup(strVal(forcol))));
	}

	foreach(lc, pc->aggregate_funcs)
	{
		ResTarget  *rt = lfirst(lc);
		FuncCall   *fcall;

		if ((rt->val == NULL || !IsA(rt->val, FuncCall)))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PIVOT aggregate must be a function call")));

		fcall = castNode(FuncCall, rt->val);

		if (!fcall->agg_star && fcall->args != NIL)
		{
			foreach(lc2, fcall->args)
				pivot_collect_colrefs((Node *) lfirst(lc2), &exclude_cols);
		}

		if (fcall->agg_order != NIL)
		{
			foreach(lc2, fcall->agg_order)
			{
				SortBy	   *sb = lfirst_node(SortBy, lc2);

				pivot_collect_colrefs(sb->node, &exclude_cols);
			}
		}
	}

	foreach(lc, source_cols)
	{
		Value	   *v = lfirst(lc);

		if ((v == NULL || !IsA(v, String)))
			continue;
		if (strcmp(strVal(v), "?column?") == 0)
			continue;
		if (!pivot_has_name(exclude_cols, strVal(v)))
			group_cols = lappend(group_cols, makeString(pstrdup(strVal(v))));
	}

	has_group = (group_cols != NIL);

	if (pc->in_kind == PIVOT_IN_SUBQUERY)
	{
		PivotInItem *initem = linitial_node(PivotInItem, pc->in_list);

		/*
		 * Analyze and deparse the raw SELECT stored in the sole PivotInItem,
		 * checking its visible column count before adding casts and duplicate
		 * checks to the generated SQL.
		 */
		subquery_sql = pivot_raw_select_sql(pstate, linitial(initem->values),
										for_col_count);
	}

	initStringInfo(&groupsel);
	initStringInfo(&groupby);
	if (has_group)
	{
		foreach(lc, group_cols)
		{
			Value	   *v = lfirst(lc);
			char	   *q = pivot_quote_qualified("pvt_src", strVal(v));

			if (!first)
			{
				appendStringInfoString(&groupsel, ", ");
				appendStringInfoString(&groupby, ", ");
			}
			appendStringInfo(&groupsel, "%s AS %s", q, quote_identifier(strVal(v)));
			appendStringInfoString(&groupby, q);
			first = false;
		}
	}
	else
		appendStringInfoString(&groupsel, "1 AS __pvt_dummy");

	initStringInfo(&pairsel);
	initStringInfo(&pairfrom);
	if (pc->in_kind == PIVOT_IN_ANY)
	{
		/*
		 * Oracle ANY includes only pivot values present in the current
		 * implicit group.  Pulling pairs straight from pvt_src naturally
		 * gives that per-group value set, and SELECT DISTINCT below removes
		 * duplicates.
		 */
		if (has_group)
		{
			pivot_append_group_cols(&pairsel, "pvt_src", group_cols);
			appendStringInfoString(&pairsel, ", ");
		}
		else
			appendStringInfoString(&pairsel, "1 AS __pvt_dummy, ");

		for_idx = 1;
		foreach(lc, pc->for_cols)
		{
			Value	   *forcol = lfirst(lc);
			char	   *for_col_ref = pivot_quote_qualified("pvt_src",
														 strVal(forcol));

			if (for_idx > 1)
				appendStringInfoString(&pairsel, ", ");
			appendStringInfo(&pairsel, "%s AS __pvt_xml_value_%d",
							 for_col_ref, for_idx++);
		}
		appendStringInfoString(&pairfrom, "pvt_src");
	}
	else
	{
		/*
		 * Oracle subquery mode includes every subquery value in every group,
		 * even when no source row matches.  A CROSS JOIN between all groups
		 * and all subquery values creates those required empty slots; pvt_agg
		 * then left-joins source rows to fill in the measures where possible.
		 */
		if (has_group)
		{
			pivot_append_group_cols(&pairsel, "pvt_groups", group_cols);
			appendStringInfoString(&pairsel, ", ");
		}
		else
			appendStringInfoString(&pairsel, "pvt_groups.__pvt_dummy, ");
		pivot_append_xml_value_cols(&pairsel, "pvt_vals", for_col_count);
		appendStringInfoString(&pairfrom, "pvt_groups CROSS JOIN pvt_vals");
	}

	initStringInfo(&joincond);
	first = true;
	if (has_group)
	{
		foreach(lc, group_cols)
		{
			Value	   *v = lfirst(lc);

			if (!first)
				appendStringInfoString(&joincond, " AND ");
			appendStringInfo(&joincond, "%s IS NOT DISTINCT FROM %s",
							 pivot_quote_qualified("pvt_src", strVal(v)),
							 pivot_quote_qualified("pvt_pair", strVal(v)));
			first = false;
		}
	}
	else
	{
		appendStringInfoString(&joincond, "pvt_pair.__pvt_dummy = 1");
		first = false;
	}

	for_idx = 1;
	foreach(lc, pc->for_cols)
	{
		Value	   *forcol = lfirst(lc);
		char	   *for_col_ref = pivot_quote_qualified("pvt_src",
													 strVal(forcol));

		if (!first)
			appendStringInfoString(&joincond, " AND ");
		appendStringInfo(&joincond,
						 "%s IS NOT DISTINCT FROM "
						 "pvt_pair.__pvt_xml_value_%d",
						 for_col_ref, for_idx++);
		first = false;
	}

	initStringInfo(&aggsel);
	if (has_group)
	{
		pivot_append_group_cols(&aggsel, "pvt_pair", group_cols);
		appendStringInfoString(&aggsel, ", ");
	}
	else
		appendStringInfoString(&aggsel, "pvt_pair.__pvt_dummy, ");
	pivot_append_xml_value_cols(&aggsel, "pvt_pair", for_col_count);

	agg_idx = 1;
	foreach(lc, pc->aggregate_funcs)
	{
		ResTarget  *rt = lfirst_node(ResTarget, lc);
		FuncCall   *fcall = castNode(FuncCall, rt->val);
		char	   *aggsql = pivot_agg_plain_sql(fcall, "pvt_src",
												 count_col_ref);

		appendStringInfo(&aggsel, ", %s AS __pvt_agg_%d", aggsql, agg_idx++);
	}

	initStringInfo(&xmlcolumns);
	first = true;
	for_idx = 1;
	foreach(lc, pc->for_cols)
	{
		Value	   *forcol = lfirst(lc);
		bool		quoted = list_nth_int(pc->for_col_quoted, for_idx - 1);
		char	   *xml_name;
		char	   *value_ref;

		xml_name = pivot_xml_identifier_name(strVal(forcol), quoted);
		value_ref = psprintf("pvt_agg.__pvt_xml_value_%d", for_idx);

		if (!first)
			appendStringInfoString(&xmlcolumns, ", ");
		appendStringInfo(&xmlcolumns,
						 "xmlelement(name \"column\", "
						 "xmlattributes(%s AS \"name\"), "
						 "%s)",
						 quote_literal_cstr(xml_name), value_ref);
		for_idx++;
		first = false;
	}
	agg_idx = 1;

	foreach(lc, pc->aggregate_funcs)
	{
		ResTarget  *rt = lfirst_node(ResTarget, lc);
		bool		quoted;
		char	   *label = pivot_agg_xml_label(pc, rt, agg_idx, &quoted);
		char	   *xml_name = pivot_xml_identifier_name(label, quoted);
		char	   *value_ref = psprintf("pvt_agg.__pvt_agg_%d", agg_idx);

		if (!first)
			appendStringInfoString(&xmlcolumns, ", ");
		appendStringInfo(&xmlcolumns,
						 "xmlelement(name \"column\", "
						 "xmlattributes(%s AS \"name\"), "
						 "%s)",
						 quote_literal_cstr(xml_name), value_ref);
		agg_idx++;
		first = false;
	}

	initStringInfo(&xmlorder);
	pivot_append_xml_value_cols(&xmlorder, "pvt_agg", for_col_count);
	xml_output_name = pivot_xml_output_name(pc->for_cols);
	if (pivot_has_name(group_cols, xml_output_name))
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_COLUMN),
				 errmsg("column ambiguously defined"),
				 errdetail("PIVOT XML output column \"%s\" conflicts with an implicit grouping column.",
						   xml_output_name)));

	initStringInfo(&finalsel);
	initStringInfo(&finalgroup);
	if (has_group)
	{
		pivot_append_group_cols(&finalsel, "pvt_agg", group_cols);
		appendStringInfoString(&finalsel, ", ");
		pivot_append_group_cols(&finalgroup, "pvt_agg", group_cols);
	}

	appendStringInfo(&finalsel,
					 "xmlelement(name \"PivotSet\", "
					 "xmlagg(xmlelement(name \"item\", xmlconcat(%s)) "
					 "ORDER BY %s)) AS %s",
					 xmlcolumns.data, xmlorder.data,
					 quote_identifier(xml_output_name));

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "WITH pvt_src AS (SELECT pvt_input.*, 1 AS %s "
					 "FROM (%s) pvt_input), ",
					 quote_identifier(present_col), source_sql);
	if (has_group)
	{
		appendStringInfo(&sql,
						 "pvt_groups AS (SELECT %s FROM pvt_src",
						 groupsel.data);
		appendStringInfo(&sql, " GROUP BY %s", groupby.data);
		appendStringInfoChar(&sql, ')');
	}
	else
		appendStringInfoString(&sql,
						   "pvt_groups AS (SELECT 1 AS __pvt_dummy)");

	if (pc->in_kind == PIVOT_IN_SUBQUERY)
	{
		StringInfoData valsel;
		StringInfoData valaliases;
		StringInfoData uniquecond;

		initStringInfo(&valsel);
		initStringInfo(&valaliases);
		initStringInfo(&uniquecond);
		for (for_idx = 1; for_idx <= for_col_count; for_idx++)
		{
			Value	   *forcol = list_nth(pc->for_cols, for_idx - 1);
			Oid			for_type;
			int32		for_typmod;
			char	   *for_type_sql;

			pivot_source_col_type(source_cols, source_types, source_typmods,
							  strVal(forcol), &for_type, &for_typmod);
			for_type_sql = format_type_with_typemod(for_type, for_typmod);
			if (for_idx > 1)
			{
				appendStringInfoString(&valsel, ", ");
				appendStringInfoString(&valaliases, ", ");
			}
			appendStringInfo(&valsel,
							 "CAST(pvt_in.__pvt_xml_value_%d AS %s) "
							 "AS __pvt_xml_value_%d",
							 for_idx, for_type_sql, for_idx);
			appendStringInfo(&valaliases, "__pvt_xml_value_%d", for_idx);
		}
		pivot_append_xml_value_match(&uniquecond, "pvt_dup", "pvt_in",
									 for_col_count);
		appendStringInfo(&sql,
						 ", pvt_vals_raw AS (SELECT %s "
						 "FROM (%s) pvt_in(%s)), "
						 "pvt_vals AS (SELECT ",
						 valsel.data, subquery_sql, valaliases.data);
		pivot_append_xml_value_cols(&sql, "pvt_in", for_col_count);
		appendStringInfo(&sql,
						 " FROM pvt_vals_raw pvt_in WHERE "
						 "(SELECT true FROM pvt_vals_raw pvt_dup WHERE %s) "
						 "IS TRUE)",
						 uniquecond.data);
	}

	appendStringInfo(&sql,
					 ", pvt_pair AS (SELECT DISTINCT %s FROM %s), "
					 "pvt_agg AS (SELECT %s FROM pvt_pair "
					 "LEFT JOIN pvt_src ON %s GROUP BY ",
					 pairsel.data, pairfrom.data, aggsel.data, joincond.data);
	if (has_group)
	{
		pivot_append_group_cols(&sql, "pvt_pair", group_cols);
		appendStringInfoString(&sql, ", ");
	}
	else
		appendStringInfoString(&sql, "pvt_pair.__pvt_dummy, ");
	pivot_append_xml_value_cols(&sql, "pvt_pair", for_col_count);
	appendStringInfoString(&sql, ") ");

	appendStringInfo(&sql, "SELECT %s FROM pvt_agg", finalsel.data);
	if (has_group)
		appendStringInfo(&sql, " GROUP BY %s ORDER BY %s", finalgroup.data, finalgroup.data);
	else
		appendStringInfoString(&sql, " GROUP BY pvt_agg.__pvt_dummy");

	return sql.data;
}

/*
 * Analyze a copy of a PIVOT XML IN subquery in the caller's parse context,
 * require exactly expected_cols non-junk output columns, and deparse the
 * Query with ivy_get_query_def().
 */
static char *
pivot_raw_select_sql(ParseState *pstate, Node *stmt, int expected_cols)
{
	Query	   *query;
	ListCell   *lc;
	int			ncols = 0;

	if (pstate == NULL || stmt == NULL || !IsA(stmt, SelectStmt))
		elog(ERROR, "invalid PIVOT XML subquery arguments");

	query = parse_sub_analyze(copyObject(stmt), pstate, NULL, false, true);
	foreach(lc, query->targetList)
	{
		TargetEntry *tle = lfirst_node(TargetEntry, lc);

		if (!tle->resjunk)
			ncols++;
	}

	if (ncols != expected_cols)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("PIVOT XML subquery column count does not match FOR column count")));

	return ivy_get_query_def(query);
}

/*
 * Return a copy of an XML name-attribute value.
 * Preserve quoted identifiers exactly; apply pg_toupper() to each byte of
 * unquoted names.  SQL quoting and XML escaping are handled by the callers.
 */
static char *
pivot_xml_identifier_name(const char *name, bool quoted)
{
	char	   *result;
	char	   *p;

	if (name == NULL)
		elog(ERROR, "null PIVOT XML identifier");
	result = pstrdup(name);
	if (quoted)
		return result;

	for (p = result; *p != '\0'; p++)
		*p = pg_toupper((unsigned char) *p);

	return result;
}

/*
 * Build a PIVOT XML output column name from the FOR columns.
 * Join the FOR column names with underscores, lowercase the entire generated
 * name (including quoted components), and append "_xml".  XML name attributes
 * inside the payload retain their separate identifier-casing rules.
 *
 * Reject names of NAMEDATALEN bytes or more.  The caller checks for collisions
 * with grouping columns and quotes the resulting identifier in generated SQL.
 */
static char *
pivot_xml_output_name(List *for_cols)
{
	StringInfoData result;
	ListCell   *lc;
	char	   *p;
	bool		first = true;

	if (for_cols == NIL)
		elog(ERROR, "missing PIVOT XML FOR columns");

	initStringInfo(&result);
	foreach(lc, for_cols)
	{
		Value	   *forcol = lfirst(lc);

		if ((forcol == NULL || !IsA(forcol, String)) || strVal(forcol) == NULL)
			elog(ERROR, "invalid PIVOT XML FOR column");
		if (!first)
			appendStringInfoChar(&result, '_');
		appendStringInfoString(&result, strVal(forcol));
		first = false;
	}
	appendStringInfoString(&result, "_xml");
	for (p = result.data; *p != '\0'; p++)
		*p = pg_tolower((unsigned char) *p);
	if (result.len >= NAMEDATALEN)
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_COLUMN),
				 errmsg("generated PIVOT XML column name is too long"),
				 errdetail("Generated column name \"%s\" exceeds the maximum identifier length of %d bytes.",
						   result.data, NAMEDATALEN - 1),
				 errhint("Use shorter FOR column names in the PIVOT XML source query.")));

	return result.data;
}


/*
 * Transform a raw PivotCall into a RangeTblRef for an analyzed subquery RTE.
 * This is the FROM-item entry point for ordinary PIVOT, PIVOT XML, and UNPIVOT.
 *
 * 1. Normalize the source FROM item to SELECT text, recursively transforming
 *    any nested PivotCall, and collect visible names, type OIDs, and typmods.
 * 2. Dispatch to the appropriate SQL builder.  Ordinary PIVOT requires an
 *    explicit value list; XML-specific IN validation belongs to its builder.
 * 3. Parse and analyze the generated SELECT in the caller's parse context.
 * 4. Add a subquery RTE using the current PivotCall alias if supplied, otherwise
 *    the alias inherited from its source, otherwise a generated pivot_subq_N.
 * 5. Return the RangeTblRef and set top_nsitem and namespace to expose the
 *    resulting subquery through normal FROM-clause name resolution.
 *
 * Subsequent query processing receives the analyzed SELECT rather than the
 * Oracle-specific PivotCall syntax.
 */
Node *
transformPivotFromClauseItem(ParseState *pstate, PivotCall *pcall,
							 ParseNamespaceItem **top_nsitem,
							 List **namespace)
{
	PivotClause *pc;
	List	   *source_cols = NIL;
	List	   *source_types = NIL;
	List	   *source_typmods = NIL;
	char	   *source_sql;
	char	   *rewrite_sql;
	Query	   *rewrite_query;
	RangeTblRef *rtr;
	ParseNamespaceItem *nsitem;
	Alias	   *alias;

	check_stack_depth();
	if (pstate == NULL || pcall == NULL || !IsA(pcall, PivotCall) ||
		pcall->source == NULL || top_nsitem == NULL || namespace == NULL)
		elog(ERROR, "invalid PIVOT|UNPIVOT transformation arguments");
	pc = pcall->pivot;

	source_sql = pivot_build_source_sql(pstate, pcall->source, &source_cols,
									&source_types, &source_typmods);

	if (pc->piv_type == PIV_PIVOT)
	{
		if (pc->is_xml)
			rewrite_sql = pivot_build_pivot_xml_sql(pstate, pcall, source_sql,
										  source_cols, source_types,
										  source_typmods);
		else
		{
			if (pc->in_kind != PIVOT_IN_VALUES)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("subquery or ANY is allowed only with PIVOT XML")));
			rewrite_sql = pivot_build_pivot_sql(pstate, pcall, source_sql, source_cols);
		}
	}
	else if (pc->piv_type == PIV_UNPIVOT)
		rewrite_sql = pivot_build_unpivot_sql(pstate, pcall, source_sql, source_cols,
										  source_types, source_typmods);
	else
		elog(ERROR, "unrecognized PIVOT clause type: %d", (int) pc->piv_type);

	rewrite_query = pivot_analyze_sql(pstate, rewrite_sql);
	if (pcall->alias != NULL)
		alias = copyObject(pcall->alias);
	else
		alias = copyObject(pivot_source_alias(pcall->source));
	if (alias == NULL)
		alias = makeAlias(psprintf("pivot_subq_%d",
								   list_length(pstate->p_rtable) + 1), NIL);
	nsitem = addRangeTableEntryForSubquery(pstate, rewrite_query, alias, false, true);

	*top_nsitem = nsitem;
	*namespace = list_make1(nsitem);
	rtr = makeNode(RangeTblRef);
	rtr->rtindex = nsitem->p_rtindex;

	return (Node *) rtr;
}
