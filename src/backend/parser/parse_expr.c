/*-------------------------------------------------------------------------
 *
 * parse_expr.c
 *	  handle expressions in parser
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_expr.c
 *
 *-------------------------------------------------------------------------
 */

#include <assert.h>

#include "postgres.h"

#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "nodes/primnodes.h"
#include "optimizer/optimizer.h"
#include "parser/analyze.h"
#include "parser/parse_agg.h"
#include "parser/parse_clause.h"
#include "parser/parse_coerce.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_oper.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "parser/parse_type.h"
#include "parser/parse_package.h" /* OA#00002227 */
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/guc.h"	/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/lsyscache.h"
#include "utils/timestamp.h"
#include "utils/ora_compatible.h"	/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/xml.h"
/* Begin - BUGID:0000536 */
#include "utils/fmgroids.h"
#include "access/htup_details.h"
/* End - BUGID:0000536 */
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
/* End - ReqID:SRS-MYSQL-CASE */
/* Begin - ReqID:SRS-MYSQL-SET */
#include "mysql_parser/mys_set_hook.h"
#include "parser/parser.h"
/* End - ReqID:SRS-MYSQL-SET */
/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
#include "catalog/pg_class.h"
/* End - ReqID:SRS-SECURITY-FLASHBACK */
/* Begin - ReqID:SRS-MYSQL-SQL-Select */
#include "rewrite/rewriteManip.h"
/* End - ReqID:SRS-MYSQL-SQL-Select */
/* Begin - ReqID:SRS-SQL-CustomObject */
#include "catalog/pg_object_type.h"
#include "utils/syscache.h"
/* End - ReqID:SRS-SQL-CustomObject */


/* GUC parameters */
bool		Transform_null_equals = false;


static Node *transformExprRecurse(ParseState *pstate, Node *expr);
static Node *transformParamRef(ParseState *pstate, ParamRef *pref);
/* Begin - ReqID:SRS-SQL-ANONYOUT */
static Node *transformOraParamRef(ParseState *pstate, OraParamRef *pref);
/* End - ReqID:SRS-SQL-ANONYOUT */
static Node *transformAExprOp(ParseState *pstate, A_Expr *a);
static Node *transformAExprOpAny(ParseState *pstate, A_Expr *a);
static Node *transformAExprOpAll(ParseState *pstate, A_Expr *a);
static Node *transformAExprDistinct(ParseState *pstate, A_Expr *a);
static Node *transformAExprNullIf(ParseState *pstate, A_Expr *a);
static Node *transformAExprIn(ParseState *pstate, A_Expr *a);
static Node *transformAExprBetween(ParseState *pstate, A_Expr *a);
static Node *transformBoolExpr(ParseState *pstate, BoolExpr *a);
static Node *transformFuncCall(ParseState *pstate, FuncCall *fn);
static Node *transformMultiAssignRef(ParseState *pstate, MultiAssignRef *maref);
static Node *transformCaseExpr(ParseState *pstate, CaseExpr *c);
/* Begin - BUGID:0000543 */
/* Begin - ReqID:SRS-SQL-SYSFUNC */
static Node *transformdecodeexpr(ParseState *pstate, CaseExpr *c);
/* End - ReqID:SRS-SQL-SYSFUNC */
/* End - BUGID:0000543 */
static Node *transformSubLink(ParseState *pstate, SubLink *sublink);
static Node *transformArrayExpr(ParseState *pstate, A_ArrayExpr *a,
								Oid array_type, Oid element_type, int32 typmod);
static Node *transformRowExpr(ParseState *pstate, RowExpr *r, bool allowDefault);
static Node *transformCoalesceExpr(ParseState *pstate, CoalesceExpr *c);
static Node *transformMinMaxExpr(ParseState *pstate, MinMaxExpr *m);
static Node *transformSQLValueFunction(ParseState *pstate,
									   SQLValueFunction *svf);
static Node *transformXmlExpr(ParseState *pstate, XmlExpr *x);
static Node *transformXmlSerialize(ParseState *pstate, XmlSerialize *xs);
static Node *transformBooleanTest(ParseState *pstate, BooleanTest *b);
static Node *transformCurrentOfExpr(ParseState *pstate, CurrentOfExpr *cexpr);
static Node *transformColumnRef(ParseState *pstate, ColumnRef *cref);
/* Begin: Req:SRS-MYSQL-SQL-Select */
void get_tablename_colname_from_var(ParseState *pstate, Var *var, char **colname, char **tabname);
static bool can_find_targetList_and_groupList(ParseState *pstate);
static bool current_in_having_clause(ParseState *pstate);
static Node *resolve_having_ref(ParseState *pstate, ColumnRef *cref);
static Node *resolve_having_field(ParseState *pstate, ColumnRef *cref);

/* End: Req:SRS-MYSQL-SQL-Select */
static Node *transformWholeRowRef(ParseState *pstate,
								  ParseNamespaceItem *nsitem,
								  int sublevels_up, int location);
static Node *transformIndirection(ParseState *pstate, A_Indirection *ind);
static Node *transformTypeCast(ParseState *pstate, TypeCast *tc);
static Node *transformCollateClause(ParseState *pstate, CollateClause *c);
/* Begin - ReqID:SRS-SQL-CustomObject */
static bool is_object_type_typid(Oid typid);
static bool object_type_is_subtype_of(Oid subtypeTypid, Oid targetTypid);
/* End - ReqID:SRS-SQL-CustomObject */
static Node *make_row_comparison_op(ParseState *pstate, List *opname,
									List *largs, List *rargs, int location);
static Node *make_row_distinct_op(ParseState *pstate, List *opname,
								  RowExpr *lrow, RowExpr *rrow, int location);
static Expr *make_distinct_op(ParseState *pstate, List *opname,
							  Node *ltree, Node *rtree, int location);
static Node *make_nulltest_from_distinct(ParseState *pstate,
										 A_Expr *distincta, Node *arg);
/* Begin - ReqID:SRS-SQL-PROC */
static Node *transformColumnRefOrFunCall(ParseState *pstate, ColumnRefOrFuncCall *cref_func);
/* End - ReqID:SRS-SQL-PROC */
/* Begin - ReqID:SRS-SQL-ROWNUM */
static Node *transformPseudoColumnRef(ParseState *pstate, PseudoColumnRef *pcr);
/* End - ReqID:SRS-SQL-ROWNUM */
static Node *mysmakeIntConst(int val, int location);	/* BUG#M533 */
/* Begin - ReqID:SRS-MYSQL-SET */
static Node *transformVarRef(ParseState *pstate, VarRef *varref);
/* End - ReqID:SRS-MYSQL-SET */
/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
typedef enum HgVersionsPseudoColumn
{
	HG_VERSIONS_PSEUDO_NONE,
	HG_VERSIONS_PSEUDO_STARTSCN,
	HG_VERSIONS_PSEUDO_ENDSCN,
	HG_VERSIONS_PSEUDO_STARTTIME,
	HG_VERSIONS_PSEUDO_ENDTIME,
	HG_VERSIONS_PSEUDO_XID,
	HG_VERSIONS_PSEUDO_OPERATION
} HgVersionsPseudoColumn;

typedef struct HgVersionsSourceRewriteContext
{
	Oid		funcid;
	int32	source;
} HgVersionsSourceRewriteContext;

static HgVersionsPseudoColumn matchHgVersionsPseudoColumn(const char *colname);
static ParseNamespaceItem *findHgVersionsNamespaceItem(ParseState *pstate,
													   const char *colname,
													   int location,
													   int *sublevels_up);
static Node *buildHgVersionsArg(ParseState *pstate,
								ParseNamespaceItem *nsitem,
								int sublevels_up,
								const char *attname,
								bool has_plus_token,
								int location);
static Node *buildHgVersionsExpr(ParseState *pstate,
								 ParseNamespaceItem *nsitem,
								 int sublevels_up,
								 HgVersionsPseudoColumn pseudo,
								 bool has_plus_token,
								 int location);
/* End - ReqID:SRS-SECURITY-FLASHBACK */
/* Begin - BUG#7712 */
static Node *transformOraRowExpr(ParseState *pstate, OraRowExpr *r, bool allowDefault);
/* End - BUG#7712 */

/* Begin - Bug#Z167 */
static inline void
set_merge_on_attrno(ParseState *pstate, char *colname)
{
	AttrNumber	attno = InvalidAttrNumber;

	if (colname && pstate->merge_on_attrno != NULL && pstate->p_expr_kind == EXPR_KIND_JOIN_ON)
		attno = attnameAttNum(pstate->p_target_relation, colname, true);

	if (attno > InvalidAttrNumber && attno <= pstate->merge_on_attr_size)
	{
		pstate->merge_on_attrno[attno - 1] = 1;
	}
}
/* End - Bug#Z167 */

/* Begin - OA-REQ-2132 */

/* OracleSystemTypeName()  -- refer ora_gram.y
 *  Build a properly-qualified reference to a built-in type with oracle schema.
 */
static TypeName *
OracleSystemTypeName(char *name)
{
	return makeTypeNameFromNameList(list_make2(makeString("sys"),
											   makeString(name)));
}

/* OracleSystemFuncName() -- refer ora_gram.y */
static List *
OracleSystemFuncName(char *name)
{
        return list_make2(makeString("sys"), makeString(name));
}

/*
 * transformRatioToReportExpr()
 * rewrite function ratio_ro_report() over () to
 * a / sum(a) over(?)
 */
static Node *
transformRatioToReportExpr(ParseState *pstate, RatioToReportCall *rtrc)
{
	Node		*arg;
	WindowDef	*wd;
	FuncCall	*sum_fc;
	TypeName	*typname;
	Node	*numerator;
	Node	*denominator;
	Node	*transformed_arg;
	Node	*sum_cast;
	Node	*result;
	Node	*node_0;
	Oid	arg_type;
	int	location = rtrc->location;
	Oid	target_oid = NUMBEROID;

	/* ratio_to_report() support one arg */
	/* oracle error msg 'invalid number of arguments' */
	if (list_length(rtrc->args) != 1)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("invalid number of arguments"),
				 parser_errposition(pstate, location)));
	}

	/* get the arg list */
	arg = (Node *) linitial(rtrc->args);

	/* 1. get column type of xx in ratio_to_report(xx) */
	transformed_arg = transformExprRecurse(pstate, copyObject(arg));
	arg_type = exprType(transformed_arg);

	/* check if expr could cast to number type */
	if (!can_coerce_type(1, &arg_type, &target_oid, COERCION_IMPLICIT))
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("inconsistent datatypes: expected NUMBER got %s", format_type_be(arg_type)),
				 parser_errposition(pstate, location)));
	}

	/* 2. handle over() function using WindowDef */
	if (rtrc->over)
	{
		wd = castNode(WindowDef, copyObject(rtrc->over));
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("missing window specification for this function"),
				 parser_errposition(pstate, location)));
	}
	wd->frameOptions = FRAMEOPTION_DEFAULTS;
	wd->orderClause = NIL;
	wd->location = location;

	/* 3. create a new sys.sum(arg) OVER (?) */
	sum_fc = makeFuncCall(OracleSystemFuncName("sum"),
				list_make1(copyObject(arg)),
				COERCE_EXPLICIT_CALL, location);
	sum_fc->over = wd;

	/* new a '0' */
	node_0 = (Node *)ivy_makeStringConst("0", location);

	/*
	 * 4. for different type, using different expr
	 * if there have mismatch for the precision
	 * there have an implicit transfer
	 * so when expalin the SQL, showing more than we rewritted
	 */
	switch (arg_type)
	{
		/*
		 * for BINARY_FLOAT:
		 * 	as sys.sum(binary_float) = sys.sum((bd)::number)
		 * 	so binary_float / sys.sum(binary_float), result is binary_float
		 * 	keep the same with select (binary_float)/sys.sum(binary_float) over()
		 */
		case BINARY_FLOATOID:
		{
			/*
			 * sum(binary_float) == sum((bd)::number)
			 */
			/* b */
			numerator = copyObject(arg);
			/* NULLIF(...) */
			denominator = (Node *)makeA_Expr(AEXPR_NULLIF,
						list_make1(makeString("=")),
						(Node *)sum_fc,
						node_0,
						location);
			result = (Node *)makeSimpleA_Expr(AEXPR_OP, "/", numerator, denominator, location);

			break;
		}
		/*
		 * for BINARY_DOUBLE:
		 * 	as sys.sum(binary_double) --> sys.sum((binary_double)::number)
		 * 	so binary_double / sum(binary_double), result is binary_double
		 * 	ratio_to_report(binary_double), result is binary_double
		 */
		case BINARY_DOUBLEOID:
		{
			/* sum(binary_double) == sum((bd)::double precision) */
			/* b */
			numerator = copyObject(arg);
			/* NULLIF(...) */
			denominator = (Node *)makeA_Expr(AEXPR_NULLIF,
						list_make1(makeString("=")),
						(Node *)sum_fc,
						node_0,
						location);
			result = (Node *)makeSimpleA_Expr(AEXPR_OP, "/", numerator, denominator, location);

			break;
		}
		default: /* NUMBER */
		{
			/*
			 * other type: Integer, Numeric, Varchar2, timestamp, interval...
			 * (a)::number / NULLIF(sum(a) OVER (?)::number, '0')
			 */

			/* numeric without typmod */
			typname = OracleSystemTypeName("number");
			typname->typmods = NIL;
			typname->typemod = -1;
			typname->location = location;

			/* (a)::number */
			numerator = ivy_makeTypeCast(copyObject(arg), typname, location);

			/* sum(a) OVER (?)::numeric */
			sum_cast = (Node *)ivy_makeTypeCast((Node *)sum_fc, typname, location);
 
			/* NULLIF(sum::numeric, '0') */
			denominator = (Node *)makeA_Expr(AEXPR_NULLIF,
							list_make1(makeString("=")),
							sum_cast,
							node_0,
							location);

			result = (Node *)makeSimpleA_Expr(AEXPR_OP, "/", numerator, denominator, location);

			break;
		}
	}

	/* 5. A_Expr / */
	return result;
}
/* End - OA-REQ-2132 */

/*
 * transformExpr -
 *	  Analyze and transform expressions. Type checking and type casting is
 *	  done here.  This processing converts the raw grammar output into
 *	  expression trees with fully determined semantics.
 */
Node *
transformExpr(ParseState *pstate, Node *expr, ParseExprKind exprKind)
{
	Node	   *result;
	ParseExprKind sv_expr_kind;

	/* Save and restore identity of expression type we're parsing */
	Assert(exprKind != EXPR_KIND_NONE);
	sv_expr_kind = pstate->p_expr_kind;
	pstate->p_expr_kind = exprKind;

	result = transformExprRecurse(pstate, expr);

	pstate->p_expr_kind = sv_expr_kind;

	return result;
}

/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
static HgVersionsPseudoColumn
matchHgVersionsPseudoColumn(const char *colname)
{
	if (pg_strcasecmp(colname, "versions_startscn") == 0)
		return HG_VERSIONS_PSEUDO_STARTSCN;
	if (pg_strcasecmp(colname, "versions_endscn") == 0)
		return HG_VERSIONS_PSEUDO_ENDSCN;
	if (pg_strcasecmp(colname, "versions_starttime") == 0)
		return HG_VERSIONS_PSEUDO_STARTTIME;
	if (pg_strcasecmp(colname, "versions_endtime") == 0)
		return HG_VERSIONS_PSEUDO_ENDTIME;
	if (pg_strcasecmp(colname, "versions_xid") == 0)
		return HG_VERSIONS_PSEUDO_XID;
	if (pg_strcasecmp(colname, "versions_operation") == 0)
		return HG_VERSIONS_PSEUDO_OPERATION;

	return HG_VERSIONS_PSEUDO_NONE;
}

/*
 * Find the single relation namespace item that should supply system-column
 * arguments for an unqualified VERSIONS_* pseudo-column reference.
 */
static ParseNamespaceItem *
findHgVersionsNamespaceItem(ParseState *pstate, const char *colname,
							int location, int *sublevels_up)
{
	ParseNamespaceItem *result = NULL;
	int			result_levels_up = 0;
	int			levels = 0;

	while (pstate != NULL)
	{
		ListCell   *l;

		foreach(l, pstate->p_namespace)
		{
			ParseNamespaceItem *nsitem = (ParseNamespaceItem *) lfirst(l);

			if (!nsitem->p_cols_visible)
				continue;
			if (nsitem->p_lateral_only && !pstate->p_lateral_active)
				continue;
			if (nsitem->p_rte->rtekind != RTE_RELATION ||
				nsitem->p_rte->relkind == RELKIND_COMPOSITE_TYPE)
				continue;

			if (result != NULL)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_COLUMN),
						 errmsg("column reference \"%s\" is ambiguous",
								colname),
						 parser_errposition(pstate, location)));

			result = nsitem;
			result_levels_up = levels;
		}

		pstate = pstate->parentParseState;
		levels++;
	}

	if (result != NULL && sublevels_up != NULL)
		*sublevels_up = result_levels_up;

	return result;
}

/*
 * Pseudo-columns are rewritten into builtin function calls over ordinary
 * system columns, so resolve the requested system-column argument here.
 */
static Node *
buildHgVersionsArg(ParseState *pstate, ParseNamespaceItem *nsitem,
				   int sublevels_up, const char *attname, bool has_plus_token, int location)
{
	Node	   *arg;

	arg = scanNSItemForColumnExt(pstate, nsitem, sublevels_up, attname, has_plus_token, location);
	if (arg == NULL)
		elog(ERROR, "could not resolve flashback pseudo-column argument \"%s\"",
			 attname);

	return arg;
}

/*
 * Expand a flashback pseudo-column into the builtin function call or system
 * column reference that implements it.
 */
static Node *
buildHgVersionsExpr(ParseState *pstate, ParseNamespaceItem *nsitem,
					int sublevels_up, HgVersionsPseudoColumn pseudo,
					bool has_plus_token, int location)
{
	Node *res = NULL;

	switch (pseudo)
	{
		case HG_VERSIONS_PSEUDO_STARTSCN:
			res = ParseFuncOrColumn(pstate,
									list_make1(makeString("pg_versions_startscn")),
									NIL,
									pstate->p_last_srf,
									NULL,
									false,
									location);
			break;
		case HG_VERSIONS_PSEUDO_ENDSCN:
			res = ParseFuncOrColumn(pstate,
									list_make1(makeString("pg_versions_endscn")),
									NIL,
									pstate->p_last_srf,
									NULL,
									false,
									location);
			break;
		case HG_VERSIONS_PSEUDO_STARTTIME:
			res = ParseFuncOrColumn(pstate,
									list_make1(makeString("pg_versions_starttime")),
									list_make1(buildHgVersionsArg(pstate, nsitem,
															   	  sublevels_up,
															   	  "xmin",
															   	  has_plus_token,
															   	  location)),
									pstate->p_last_srf,
									NULL,
									false,
									location);
			break;
		case HG_VERSIONS_PSEUDO_ENDTIME:
			res = ParseFuncOrColumn(pstate,
									list_make1(makeString("pg_versions_endtime")),
									list_make1(buildHgVersionsArg(pstate, nsitem,
															   	  sublevels_up,
															   	  "xmax",
															   	  has_plus_token,
															   	  location)),
									pstate->p_last_srf,
									NULL,
									false,
									location);
			break;
		case HG_VERSIONS_PSEUDO_XID:
			res = buildHgVersionsArg(pstate, nsitem, sublevels_up,
									 "xmin", has_plus_token, location);
			break;
		case HG_VERSIONS_PSEUDO_OPERATION:
			res = ParseFuncOrColumn(pstate,
									list_make1(makeString("pg_versions_operation")),
									list_make2(buildHgVersionsArg(pstate, nsitem,
															   	  sublevels_up,
															   	  "ctid",
															   	  has_plus_token,
															   	  location),
											   buildHgVersionsArg(pstate, nsitem,
															   	  sublevels_up,
															   	  "tableoid",
															   	  has_plus_token,
															   	  location)),
									pstate->p_last_srf,
									NULL,
									false,
									location);
			break;
		case HG_VERSIONS_PSEUDO_NONE:
			res = NULL;
			break;
	}

	return res;
}
/* End - ReqID:SRS-SECURITY-FLASHBACK */

static Node *
transformExprRecurse(ParseState *pstate, Node *expr)
{
	Node	   *result;

	if (expr == NULL)
		return NULL;

	/* Guard against stack overflow due to overly complex expressions */
	check_stack_depth();

	switch (nodeTag(expr))
	{
		case T_ColumnRef:
			result = transformColumnRef(pstate, (ColumnRef *) expr);
			break;
		/* Begin - ReqID:SRS-SQL-ROWNUM */
		case T_PseudoColumnRef:
			result = transformPseudoColumnRef(pstate, (PseudoColumnRef *) expr);
			break;
		/* End - ReqID:SRS-SQL-ROWNUM */

		case T_ParamRef:
			result = transformParamRef(pstate, (ParamRef *) expr);
			break;

		/* Begin - ReqID:SRS-SQL-ANONYOUT */
		case T_OraParamRef:
			result = transformOraParamRef(pstate, (OraParamRef *)expr);
			break;
		/* End - ReqID:SRS-SQL-ANONYOUT */

		case T_A_Const:
			{
				A_Const    *con = (A_Const *) expr;
				Value	   *val = &con->val;

				result = (Node *) make_const(pstate, val, con->location);
				break;
			}

		/* Begin - ReqID:RSR-SQL-Partition-interval */
		/* Do nothing for const */
		case T_Const:
			result = expr;
			break;
		/* End - ReqID:RSR-SQL-Partition-interval */

		case T_A_Indirection:
			result = transformIndirection(pstate, (A_Indirection *) expr);
			break;

		case T_A_ArrayExpr:
			result = transformArrayExpr(pstate, (A_ArrayExpr *) expr,
										InvalidOid, InvalidOid, -1);
			break;

		case T_TypeCast:
			result = transformTypeCast(pstate, (TypeCast *) expr);
			break;

		case T_CollateClause:
			result = transformCollateClause(pstate, (CollateClause *) expr);
			break;

		case T_A_Expr:
			{
				A_Expr	   *a = (A_Expr *) expr;

				switch (a->kind)
				{
					case AEXPR_OP:
						result = transformAExprOp(pstate, a);
						break;
					case AEXPR_OP_ANY:
						result = transformAExprOpAny(pstate, a);
						break;
					case AEXPR_OP_ALL:
						result = transformAExprOpAll(pstate, a);
						break;
					case AEXPR_DISTINCT:
					case AEXPR_NOT_DISTINCT:
						result = transformAExprDistinct(pstate, a);
						break;
					case AEXPR_NULLIF:
						result = transformAExprNullIf(pstate, a);
						break;
					case AEXPR_IN:
						result = transformAExprIn(pstate, a);
						break;
					case AEXPR_LIKE:
					case AEXPR_ILIKE:
					case AEXPR_SIMILAR:
						/* we can transform these just like AEXPR_OP */
						result = transformAExprOp(pstate, a);
						break;
					case AEXPR_BETWEEN:
					case AEXPR_NOT_BETWEEN:
					case AEXPR_BETWEEN_SYM:
					case AEXPR_NOT_BETWEEN_SYM:
						result = transformAExprBetween(pstate, a);
						break;
					default:
						elog(ERROR, "unrecognized A_Expr kind: %d", a->kind);
						result = NULL;	/* keep compiler quiet */
						break;
				}

				/** 
				 * Begin - Req: SRS-SQL-NLSSORT
				 * Replace pseudo collation for "=, !=, >, <, >=, <=, BETWEEN, NOT BETWEEN, IN, NOT IN, REGEXP_LIKE, LIKE" conditions.
				 */
				if (ORA_MODE == compatible_mode && enable_ora_default_collation)
				{
					Oid 	 coll;
					Node 	*copy = copyObject(result);

					assign_expr_collations(pstate, copy);

					coll = exprCollation(copy);

					if (!OidIsValid(coll))
						coll = exprInputCollation(copy);

					if (IsPseudoCollation(coll))
					{
						Oid targetcoll = PseudoCollationReplaceForAExpr(a, coll);

						if (targetcoll != coll)
							pseudo_collation_replace(pstate, result, NULL, targetcoll);
					}

					pfree(copy);
				}
				/* End - Req: SRS-SQL-NLSSORT */

				break;
			}

		case T_BoolExpr:
			result = transformBoolExpr(pstate, (BoolExpr *) expr);
			break;

		case T_FuncCall:
			result = transformFuncCall(pstate, (FuncCall *) expr);
			break;
		/* Begin - OA-REQ-2132 */
		case T_RatioToReportCall:
		{
			Node *rewritten = transformRatioToReportExpr(pstate, (RatioToReportCall *)expr);
			result = transformExprRecurse(pstate, rewritten);
			break;
		}
		/* End - OA-REQ-2132 */
		/* Begin - ReqID:SRS-SQL-PROC */
		case T_ColumnRefOrFuncCall:
			result = transformColumnRefOrFunCall(pstate, ((ColumnRefOrFuncCall *)expr));
			break;
		/* End - ReqID:SRS-SQL-PROC */

		case T_MultiAssignRef:
			result = transformMultiAssignRef(pstate, (MultiAssignRef *) expr);
			break;

		case T_GroupingFunc:
			result = transformGroupingFunc(pstate, (GroupingFunc *) expr);
			break;

		case T_NamedArgExpr:
			{
				NamedArgExpr *na = (NamedArgExpr *) expr;

				na->arg = (Expr *) transformExprRecurse(pstate, (Node *) na->arg);
				result = expr;
				break;
			}

		case T_SubLink:
			{
				List *p_varnamespace = NIL;

				/**
				 * Begin - bug#5797
				 * In InsertStmt, do not replace Var node with column's default value
				 * in SubLink node if the Var occur in value list.
				 */
				p_varnamespace = pstate->p_varnamespace;
				pstate->p_varnamespace = NIL;
				/* End - bug#5797 */

				result = transformSubLink(pstate, (SubLink *) expr);
			
				/* Begin - bug#5797 */
				pstate->p_varnamespace = p_varnamespace;
				/* End - bug#5797 */

				break;
			}

		case T_CaseExpr:
			{
				/* Begin - BUGID:0000543 */
				CaseExpr *n = (CaseExpr *) expr;

				if (!n->is_decode)
					result = transformCaseExpr(pstate, n);
				else
				{
					/* ReqID:SRS-SQL-SYSFUNC, 
					 * change name transformCaseExpr_for_decode to transformdecodeexpr 
					 */
					result = transformdecodeexpr(pstate, n);
				}
				/* End - BUGID:0000543 */
				break;
			}

		case T_RowExpr:
			result = transformRowExpr(pstate, (RowExpr *) expr, false);
			break;

		case T_CoalesceExpr:
			result = transformCoalesceExpr(pstate, (CoalesceExpr *) expr);
			break;

		case T_MinMaxExpr:
			result = transformMinMaxExpr(pstate, (MinMaxExpr *) expr);
			break;

		case T_SQLValueFunction:
			result = transformSQLValueFunction(pstate,
											   (SQLValueFunction *) expr);
			break;

		case T_XmlExpr:
			result = transformXmlExpr(pstate, (XmlExpr *) expr);
			break;

		case T_XmlSerialize:
			result = transformXmlSerialize(pstate, (XmlSerialize *) expr);
			break;

		case T_NullTest:
			{
				NullTest   *n = (NullTest *) expr;

				n->arg = (Expr *) transformExprRecurse(pstate, (Node *) n->arg);
				/* the argument can be any type, so don't coerce it */
				n->argisrow = type_is_rowtype(exprType((Node *) n->arg));
				result = expr;
				break;
			}

		case T_BooleanTest:
			result = transformBooleanTest(pstate, (BooleanTest *) expr);
			break;

		case T_CurrentOfExpr:
			result = transformCurrentOfExpr(pstate, (CurrentOfExpr *) expr);
			break;

			/*
			 * In all places where DEFAULT is legal, the caller should have
			 * processed it rather than passing it to transformExpr().
			 */
		case T_SetToDefault:
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("DEFAULT is not allowed in this context"),
					 parser_errposition(pstate,
										((SetToDefault *) expr)->location)));
			break;

			/*
			 * CaseTestExpr doesn't require any processing; it is only
			 * injected into parse trees in a fully-formed state.
			 *
			 * Ordinarily we should not see a Var here, but it is convenient
			 * for transformJoinUsingClause() to create untransformed operator
			 * trees containing already-transformed Vars.  The best
			 * alternative would be to deconstruct and reconstruct column
			 * references, which seems expensively pointless.  So allow it.
			 */
		case T_CaseTestExpr:
		case T_Var:
		case T_PseudoColVar: /* ReqID:SRS-SQL-ROWNUM */
		case T_TupdescNatts: /* ReqID:SRS-SQL-DBLINK */
			{
				result = (Node *) expr;
				break;
			}

		/* Begin - BUG0000331 */
		case T_ResTarget:
			{
				result = transformExprRecurse(pstate, ((ResTarget *)expr)->val);
				break;
			}
		/* End - BUG0000331 */

		/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
		case T_PriorAexpr:
			/* no connect by,should raise error */
			if (!pstate->p_check_connectby)
				elog(ERROR, "CONNECT BY clause required in this query block");
			result = transformExprRecurse(pstate, ((PriorAexpr *) expr)->arg);
			break;
		case T_ConnectByRoot:
			if (!pstate->p_check_connectby)
				elog(ERROR, "CONNECT BY clause required in this query block");
			/* only check grammer, so simple check is colnode */
			result = transformExprRecurse(pstate, ((ConnectByRoot *) expr)->colnode);
			break;
		case T_ConnectByIsCycle:
			{
				PseudoColVar	*pv = makeNode(PseudoColVar);

				if (!pstate->p_check_connectby)
					elog(ERROR, "CONNECT BY clause required in this query block");

				/* only for grammer check */
				pv->pvartype = NUMBEROID;
				pv->pvarkind = PSEUDOCOL_KIND_ISCYCLE;
				pv->pvartypmod = -1;

				result = (Node *) pv;
				break;
			}
		case T_SysConnectPath:
			elog(ERROR, "SYS_CONNECT_BY_PATH function is not allowed here");
			break;
		/* End - ReqID:SRS-SQL-HierarchicalQueries */
		/* Begin - ReqID:SRS-MYSQL-SET */
		case T_VarAssign:
			result = transformVarAssign(pstate, (VarAssign *)expr, false);
			break;
		case T_VarRef:
			result = transformVarRef(pstate, (VarRef *)expr);
			break;
		/* End - ReqID:SRS-MYSQL-SET */
		/* Begin - BUG#7712 */
		case T_OraRowExpr:
			result = transformOraRowExpr(pstate, (OraRowExpr *) expr, false);
			break;
		/* End - BUG#7712 */
		default:
			/* should not reach here */
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(expr));
			result = NULL;		/* keep compiler quiet */
			break;
	}

	return result;
}

/*
 * helper routine for delivering "column does not exist" error message
 *
 * (Usually we don't have to work this hard, but the general case of field
 * selection from an arbitrary node needs it.)
 */
static void
unknown_attribute(ParseState *pstate, Node *relref, const char *attname,
				  int location)
{
	RangeTblEntry *rte;

	if (IsA(relref, Var) &&
		((Var *) relref)->varattno == InvalidAttrNumber)
	{
		/* Reference the RTE by alias not by actual table name */
		rte = GetRTEByRangeTablePosn(pstate,
									 ((Var *) relref)->varno,
									 ((Var *) relref)->varlevelsup);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
				 errmsg("column %s.%s does not exist",
						rte->eref->aliasname, attname),
				 parser_errposition(pstate, location)));
	}
	else
	{
		/* Have to do it by reference to the type of the expression */
		Oid			relTypeId = exprType(relref);

		if (ISCOMPLEX(relTypeId))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("column \"%s\" not found in data type %s",
							attname, format_type_be(relTypeId)),
					 parser_errposition(pstate, location)));
		else if (relTypeId == RECORDOID)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("could not identify column \"%s\" in record data type",
							attname),
					 parser_errposition(pstate, location)));
		else
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("column notation .%s applied to type %s, "
							"which is not a composite type",
							attname, format_type_be(relTypeId)),
					 parser_errposition(pstate, location)));
	}
}

static Node *
transformIndirection(ParseState *pstate, A_Indirection *ind)
{
	Node	   *last_srf = pstate->p_last_srf;
	Node	   *result = transformExprRecurse(pstate, ind->arg);
	List	   *subscripts = NIL;
	int			location = exprLocation(result);
	ListCell   *i;

	/* Begin - ReqID:SRS-PLSQL-Collection */
	if (pstate->p_indirection_hook != NULL)
	{
		Node	*node;
		node = pstate->p_indirection_hook(pstate, result, NULL, ind->indirection, NULL, location);
		if (node != NULL)
			return node;
	}
	/* End - ReqID:SRS-PLSQL-Collection */

	/*
	 * We have to split any field-selection operations apart from
	 * subscripting.  Adjacent A_Indices nodes have to be treated as a single
	 * multidimensional subscript operation.
	 */
	foreach(i, ind->indirection)
	{
		Node	   *n = lfirst(i);

		if (IsA(n, A_Indices))
			subscripts = lappend(subscripts, n);
		else if (IsA(n, A_Star))
		{
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("row expansion via \"*\" is not supported here"),
					 parser_errposition(pstate, location)));
		}
		else
		{
			Node	   *newresult;

			Assert(IsA(n, String));

			/* Begin - ReqID:SRS-PLSQL-CURSOR */
			/* maybe :x.attribute */
			if (IsA(result, Param) &&
				ISORACURSOROID(((Param *) result)->paramtype) &&
				IsA(ind->arg, OraParamRef) &&
				list_length(ind->indirection) == 1)
			{
				OraParamRef *oraref = (OraParamRef *) ind->arg;
				char		pname[32];
				ColumnRef *colref = makeNode(ColumnRef);

				memset(pname, 0x00, 32);
				snprintf(pname, sizeof(pname), "$%d", oraref->number);
				colref->fields = list_make2(makeString(pname),
											n);
				newresult = transformColumnRef(pstate, colref);
				if (newresult != NULL)
					return newresult;
			}
			/* End - ReqID:SRS-PLSQL-CURSOR */

			/* process subscripts before this field selection */
			if (subscripts)
				result = (Node *) transformContainerSubscripts(pstate,
															   result,
															   exprType(result),
															   exprTypmod(result),
															   subscripts,
															   false);
			subscripts = NIL;

			newresult = ParseFuncOrColumn(pstate,
										  list_make1(n),
										  list_make1(result),
										  last_srf,
										  NULL,
										  false,
										  location);
			if (newresult == NULL)
				unknown_attribute(pstate, result, strVal(n), location);
			result = newresult;
		}
	}
	/* process trailing subscripts, if any */
	if (subscripts)
		result = (Node *) transformContainerSubscripts(pstate,
													   result,
													   exprType(result),
													   exprTypmod(result),
													   subscripts,
													   false);

	return result;
}

/* Begin - ReqID:SRS-SQL-PROC */
/*
 * This is a copy of transformColumnRef, except that we have added
 * a boolean missing_ok parameter to control whether an error is 
 * reported when the column does not exist. The native PG reports
 * an error directly. We hope to return a NULL Node instead of 
 * reporting an error in some cases.
 */
static Node *
transformColumnRefInternal(ParseState *pstate, ColumnRef *cref, bool missing_ok)
{
	Node	   *node = NULL;
	char	   *nspname = NULL;
	char	   *relname = NULL;
	char	   *colname = NULL;
	ParseNamespaceItem *nsitem;
	int			levels_up;
	enum
	{
		CRERR_NO_COLUMN,
		CRERR_NO_RTE,
		CRERR_WRONG_DB,
		CRERR_TOO_MANY
	}			crerr = CRERR_NO_COLUMN;
	const char *err;

	/*
	 * Check to see if the column reference is in an invalid place within the
	 * query.  We allow column references in most places, except in default
	 * expressions and partition bound expressions.
	 */
	err = NULL;
	switch (pstate->p_expr_kind)
	{
		case EXPR_KIND_NONE:
			Assert(false);		/* can't happen */
			break;
		case EXPR_KIND_OTHER:
		case EXPR_KIND_JOIN_ON:
		case EXPR_KIND_JOIN_USING:
		case EXPR_KIND_FROM_SUBSELECT:
		case EXPR_KIND_FROM_FUNCTION:
		case EXPR_KIND_WHERE:
		case EXPR_KIND_POLICY:
		case EXPR_KIND_HAVING:
		case EXPR_KIND_FILTER:
		case EXPR_KIND_WINDOW_PARTITION:
		case EXPR_KIND_WINDOW_ORDER:
		case EXPR_KIND_WINDOW_FRAME_RANGE:
		case EXPR_KIND_WINDOW_FRAME_ROWS:
		case EXPR_KIND_WINDOW_FRAME_GROUPS:
		case EXPR_KIND_SELECT_TARGET:
		case EXPR_KIND_INSERT_TARGET:
		case EXPR_KIND_UPDATE_SOURCE:
		case EXPR_KIND_UPDATE_TARGET:
		case EXPR_KIND_MERGE_WHEN:
		case EXPR_KIND_GROUP_BY:
		case EXPR_KIND_ORDER_BY:
		case EXPR_KIND_DISTINCT_ON:
		case EXPR_KIND_LIMIT:
		case EXPR_KIND_OFFSET:
		case EXPR_KIND_RETURNING:
		case EXPR_KIND_VALUES:
		case EXPR_KIND_VALUES_SINGLE:
		case EXPR_KIND_CHECK_CONSTRAINT:
		case EXPR_KIND_DOMAIN_CHECK:
		case EXPR_KIND_FUNCTION_DEFAULT:
		case EXPR_KIND_INDEX_EXPRESSION:
		case EXPR_KIND_INDEX_PREDICATE:
		case EXPR_KIND_STATS_EXPRESSION:
		case EXPR_KIND_ALTER_COL_TRANSFORM:
		case EXPR_KIND_EXECUTE_PARAMETER:
		case EXPR_KIND_TRIGGER_WHEN:
		case EXPR_KIND_PARTITION_EXPRESSION:
		case EXPR_KIND_CALL_ARGUMENT:
		case EXPR_KIND_COPY_WHERE:
		case EXPR_KIND_GENERATED_COLUMN:
		case EXPR_KIND_CYCLE_MARK:
		/* Begin: Req:SRS-SQL-NLSSORT */
		case EXPR_KIND_GROUP_BY_DISTINCT:
		/* End: Req:SRS-SQL-NLSSORT */
		/* Begin - ReqID:SRS-RECOVERY-FLASHBACK */
		case EXPR_KIND_ASOF:
		case EXPR_KIND_VERSIONS_BETWEEN:
		/* End - ReqID:SRS-RECOVERY-FLASHBACK */
		/* Begin: Req:SRS-MYSQL-SQL-Select */
		case EXPR_KIND_HAVING_FUNCTION:
		/* End: Req:SRS-MYSQL-SQL-Select */
			/* okay */
			break;

		case EXPR_KIND_COLUMN_DEFAULT:
			err = _("cannot use column reference in DEFAULT expression");
			break;
		case EXPR_KIND_PARTITION_BOUND:
			err = _("cannot use column reference in partition bound expression");
			break;

			/*
			 * There is intentionally no default: case here, so that the
			 * compiler will warn if we add a new ParseExprKind without
			 * extending this switch.  If we do see an unrecognized value at
			 * runtime, the behavior will be the same as for EXPR_KIND_OTHER,
			 * which is sane anyway.
			 */
	}
	if (err)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg_internal("%s", err),
				 parser_errposition(pstate, cref->location)));

	/*
	 * Give the PreParseColumnRefHook, if any, first shot.  If it returns
	 * non-null then that's all, folks.
	 */
	if (pstate->p_pre_columnref_hook != NULL)
	{
		node = pstate->p_pre_columnref_hook(pstate, cref);
		if (node != NULL)
			return node;
	}

	/*----------
	 * The allowed syntaxes are:
	 *
	 * A		First try to resolve as unqualified column name;
	 *			if no luck, try to resolve as unqualified table name (A.*).
	 * A.B		A is an unqualified table name; B is either a
	 *			column or function name (trying column name first).
	 * A.B.C	schema A, table B, col or func name C.
	 * A.B.C.D	catalog A, schema B, table C, col or func D.
	 * A.*		A is an unqualified table name; means whole-row value.
	 * A.B.*	whole-row value of table B in schema A.
	 * A.B.C.*	whole-row value of table C in schema B in catalog A.
	 *
	 * We do not need to cope with bare "*"; that will only be accepted by
	 * the grammar at the top level of a SELECT list, and transformTargetList
	 * will take care of it before it ever gets here.  Also, "A.*" etc will
	 * be expanded by transformTargetList if they appear at SELECT top level,
	 * so here we are only going to see them as function or operator inputs.
	 *
	 * Currently, if a catalog name is given then it must equal the current
	 * database name; we check it here and then discard it.
	 *----------
	 */
	switch (list_length(cref->fields))
	{
		case 1:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);

				Assert(IsA(field1, String));
				colname = strVal(field1);
				/* Begin - Bug#Z167 */
				set_merge_on_attrno(pstate, colname);
				/* End - Bug#Z167 */

				/* Begin - BUG#M0000217 */
				if (compatible_mode == ORA_MODE &&
					(pstate->p_expr_kind == EXPR_KIND_ORDER_BY ||
					 pstate->p_expr_kind == EXPR_KIND_GROUP_BY_DISTINCT) &&
					pstate->p_targetlist)
				{
					ListCell	*tl;
					List		**tlist = pstate->p_targetlist;
					TargetEntry	*target_result = NULL;

					foreach(tl, *tlist)
					{
						TargetEntry *tle = (TargetEntry *) lfirst(tl);

						if (!tle->resjunk &&
							strcmp(tle->resname, colname) == 0)
						{
							if (target_result != NULL)
							{
								if (!equal(target_result->expr, tle->expr))
									ereport(ERROR,
											(errcode(ERRCODE_AMBIGUOUS_COLUMN),
											 errmsg("%s \"%s\" is ambiguous",
													ParseExprKindName(EXPR_KIND_ORDER_BY),
													colname),
											 parser_errposition(pstate, cref->location)));
							}
							else
								target_result = tle;
							/* Stay in loop to check for ambiguity */
						}
					}

					if (target_result != NULL)
					{
						/*
						 * target_result->expr become new arg instead of cref,
						 * We need modify the location of node, so must use a copy.
						 */
						node = (Node *) copyObject(target_result->expr);

						/*
						 * Try to propagate location knowledge.  This should
						 * be extended if node can ever take on other node types.
						 */
						if (IsA(node, Var))
							((Var *) node)->location = cref->location;
					}
					else
						node = colNameToVarExt(pstate, colname, false, cref->has_outer_join, cref->location, false);	/* ReqID:SRS-SQL-OUTERJOIN(+) */
				}
				/* Begin: Req:SRS-MYSQL-SQL-Select */
				else if (compatible_mode == MYS_MODE)
				{
					/**
					 * If MySQL constructs Item_field, enter resolve_having_field parsing,
					 * else if Item_ref, enter resolve_having_ref parsing.
					 */
					if (pstate->p_expr_kind == EXPR_KIND_HAVING)
						node = resolve_having_ref(pstate, cref);
					else
						node = resolve_having_field(pstate, cref);
				}
				/* End: Req:SRS-MYSQL-SQL-Select */
				else
				/* End - BUG#M0000217 */
				{
					/* Try to identify as an unqualified column */
					node = colNameToVarExt(pstate, colname, false, cref->has_outer_join, cref->location, false);	/* ReqID:SRS-SQL-OUTERJOIN(+) */
				}

				/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
				/*
				 * The connect by query performs special processing on the level or connect_by_isleaf
				 * column, which is currently being used for syntax detection.
				 */
				if (pstate->p_check_connectby &&
					(pg_strcasecmp(colname, "level") == 0 ||
					pg_strcasecmp(colname, "connect_by_isleaf") == 0))
				{
					/*
					 * only for grammer check, so we make a pseduocolvar
					 */
					PseudoColVar	*pv = makeNode(PseudoColVar);

					pv->pvartype = NUMBEROID;
					if (pg_strcasecmp(colname, "level") == 0)
						pv->pvarkind = PSEUDOCOL_KIND_LEVEL;
					else
						pv->pvarkind = PSEUDOCOL_KIND_ISLEAF;
					pv->pvartypmod = -1;

					node = (Node *) pv;
				}
				/* End - ReqID:SRS-SQL-HierarchicalQueries */

				/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
				if (pstate->p_tryVersionsPseudoColumn && node == NULL)
				{
					HgVersionsPseudoColumn pseudo = matchHgVersionsPseudoColumn(colname);
					if (pseudo != HG_VERSIONS_PSEUDO_NONE)
					{
						nsitem = findHgVersionsNamespaceItem(pstate, colname,
															cref->location,
															&levels_up);
						if (nsitem != NULL)
							node = buildHgVersionsExpr(pstate, nsitem, levels_up,
													pseudo, cref->has_outer_join, cref->location);
					}
				}
				/* End - ReqID:SRS-SECURITY-FLASHBACK */

				if (node == NULL)
				{
					/*
					 * Not known as a column of any range-table entry.
					 *
					 * Try to find the name as a relation.  Note that only
					 * relations already entered into the rangetable will be
					 * recognized.
					 *
					 * This is a hack for backwards compatibility with
					 * PostQUEL-inspired syntax.  The preferred form now is
					 * "rel.*".
					 */
					nsitem = refnameNamespaceItem(pstate, NULL, colname,
												  cref->location,
												  &levels_up);
					if (nsitem)
						node = transformWholeRowRef(pstate, nsitem, levels_up,
													cref->location);
				}
				break;
			}
		case 2:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);
				Node	   *field2 = (Node *) lsecond(cref->fields);

				Assert(IsA(field1, String));
				relname = strVal(field1);

				/* Begin - ReqID:SRS-SRS-MYSQL-CASE */
				if (compatible_mode == MYS_MODE)
				{
					if (mysql_lower_hook())
						relname = mysql_string_case_hook(relname);
					if (!pg_strcasecmp(relname, "excluded"))
						relname = mysql_string_case_hook(relname);
				}
				/* End - ReqID:SRS-SRS-MYSQL-CASE */

				/* Locate the referenced nsitem */
				nsitem = refnameNamespaceItem(pstate, nspname, relname,
											  cref->location,
											  &levels_up);
				if (nsitem == NULL)
				{
					/* Begin - ReqID:SRS-SQL-SEQ */
					if(ORA_MODE == compatible_mode && IsA(field2, String) &&
						(!pg_strcasecmp(strVal(field2), "nextval") || !pg_strcasecmp(strVal(field2), "currval")))	/* Bug#m353 */
					{
						FuncCall *seq_func = NULL;
						A_Const *n = makeNode(A_Const);

						n->val.type = T_String;
						n->val.val.str = strVal(field1);
						n->location = cref->location;

						seq_func = makeFuncCall(list_make2(makeString("pg_catalog"),
							makeString(strVal(field2))), list_make1(n), COERCE_EXPLICIT_CALL, cref->location);
						/* Begin - SRS-SQL-MULTITABLEINSERT */
						/*
						 * insert all/first doesn't allow sequence
						 * ORA-02287, we doesn't place it on ParseFunOrColumn
						 * which can't decide wether it is pg sequnce
						 */
						if (pstate->p_stmt_kind == PARSE_KIND_MULTITABLE_INSERT)
							ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("sequence number not allowed here"),
										  parser_errposition(pstate, cref->location)));
						/* End - SRS-SQL-MULTITABLEINSERT */
						return transformFuncCall(pstate, seq_func);
					}
					/* End - ReqID:SRS-SQL-SEQ */
					crerr = CRERR_NO_RTE;
					break;
				}

				/* Whole-row reference? */
				if (IsA(field2, A_Star))
				{
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					break;
				}

				Assert(IsA(field2, String));
				colname = strVal(field2);

				/* Begin - Bug#Z167 */
				set_merge_on_attrno(pstate, colname);
				/* End - Bug#Z167 */

				/* Try to identify as a column of the nsitem */
				node = scanNSItemForColumnExt(pstate, nsitem, levels_up, colname, cref->has_outer_join, /* ReqID:SRS-SQL-OUTERJOIN(+) */
											  cref->location);

				/* Begin: Req:SRS-MYSQL-SQL-Select */
				/**
				 * If the table.colname constructed by having is
				 * parsed and cannot be found in the upper level table column,
				 * return CRERR_NO_COLUMN
				 */
				if (compatible_mode == MYS_MODE)
				{
					if (levels_up != 0)
					{
						ParseState *field_pstate = pstate;

						for (int i = 0; i < levels_up; i++)
							field_pstate = field_pstate->parentParseState;

						if (field_pstate->p_expr_kind == EXPR_KIND_HAVING)
						{
							/**
							 * Under condition having clause,threr are two situations where an error
							 * needs to be reported.
							 * 1. not aggregation ,not group ,the node still hasn`t been found.
							 * 2. have group and  no agg, not found in targetlist and grouplist
							 */
							if (!field_pstate->p_hasAggs)
							{
								if (!field_pstate->p_group_clauses && !node)
								{
									crerr = CRERR_NO_COLUMN;
									break;
								}
								if (field_pstate->p_group_clauses)
								{
									if (can_find_targetList_and_groupList(field_pstate) &&
										!search_targetList_and_groupList(pstate, field_pstate, cref, cref->location, levels_up, true, NULL))
									{
										node = NULL;
										crerr = CRERR_NO_COLUMN;
										break;
									}
								}
							}
						}
					}
				}
				/* End: Req:SRS-MYSQL-SQL-Select */

				/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
				if (pstate->p_tryVersionsPseudoColumn && node == NULL)
				{
					HgVersionsPseudoColumn pseudo = matchHgVersionsPseudoColumn(colname);

					if (pseudo != HG_VERSIONS_PSEUDO_NONE)
						node = buildHgVersionsExpr(pstate, nsitem, levels_up,
												   pseudo, cref->has_outer_join, cref->location);
				}
				/* End - ReqID:SRS-SECURITY-FLASHBACK */

				if (node == NULL)
				{
					/* Try it as a function call on the whole row */
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					node = ParseFuncOrColumn(pstate,
											 list_make1(makeString(colname)),
											 list_make1(node),
											 pstate->p_last_srf,
											 NULL,
											 false,
											 cref->location);
				}
				break;
			}
		case 3:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);
				Node	   *field2 = (Node *) lsecond(cref->fields);
				Node	   *field3 = (Node *) lthird(cref->fields);

				Assert(IsA(field1, String));
				nspname = strVal(field1);
				Assert(IsA(field2, String));
				relname = strVal(field2);

				/* Begin - ReqID:SRS-SRS-MYSQL-CASE */
				if (compatible_mode == MYS_MODE)
				{
					if (mysql_lower_hook())
					{
						nspname = mysql_string_case_hook(nspname);
						relname = mysql_string_case_hook(relname);
					}
					if (!pg_strcasecmp(relname, "excluded"))
						relname = mysql_string_case_hook(relname);
				}
				/* End - ReqID:SRS-SRS-MYSQL-CASE */

				/* Locate the referenced nsitem */
				nsitem = refnameNamespaceItem(pstate, nspname, relname,
											  cref->location,
											  &levels_up);
				if (nsitem == NULL)
				{
					/* Begin - ReqID:SRS-SQL-SEQ */
					if(ORA_MODE == compatible_mode && IsA(field3, String) &&
						(!pg_strcasecmp(strVal(field3), "nextval") || !pg_strcasecmp(strVal(field3), "currval")))	/* Bug#m353 */
					{
						FuncCall *seq_func = NULL;
						/* Begin - Bug#m351 */
						Oid 	 nspoid;
						char	 nextvalarg[MAXPGPATH];
						/* End - Bug#m351 */
						A_Const *n = makeNode(A_Const);

						n->val.type = T_String;
						/* Begin - Bug#m351 */
						sprintf(nextvalarg, "%s.%s", nspname, strVal(field2));
						n->val.val.str = nextvalarg;
						/* End - Bug#m351 */
						n->location = cref->location;
						/* Begin - Bug#m351 */
						nspoid = LookupExplicitNamespace(nspname, true);
						if (!OidIsValid(nspoid))
							elog(ERROR, "sequence does not exist");
						/* End - Bug#m351 */
						seq_func = makeFuncCall(list_make2(makeString("pg_catalog"),
							makeString(strVal(field3))), list_make1(n), COERCE_EXPLICIT_CALL, cref->location);
						/* Begin - SRS-SQL-MULTITABLEINSERT */
						/*
						 * insert all/first doesn't allow sequence
						 * ORA-02287, we doesn't place it on ParseFunOrColumn
						 * which can't decide wether it is pg sequnce
						 */
						if (pstate->p_stmt_kind == PARSE_KIND_MULTITABLE_INSERT)
							ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("sequence number not allowed here"),
										  parser_errposition(pstate, cref->location)));
						/* End - SRS-SQL-MULTITABLEINSERT */
						return transformFuncCall(pstate, seq_func);
					}
					/* End - ReqID:SRS-SQL-SEQ */
					crerr = CRERR_NO_RTE;
					break;
				}

				/* Whole-row reference? */
				if (IsA(field3, A_Star))
				{
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					break;
				}

				Assert(IsA(field3, String));
				colname = strVal(field3);

				/* Begin - Bug#Z167 */
				set_merge_on_attrno(pstate, colname);
				/* End - Bug#Z167 */

				/* Try to identify as a column of the nsitem */
				node = scanNSItemForColumnExt(pstate, nsitem, levels_up, colname, cref->has_outer_join, /* ReqID:SRS-SQL-OUTERJOIN(+) */
											  cref->location);

				/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
				if (pstate->p_tryVersionsPseudoColumn && node == NULL)
				{
					HgVersionsPseudoColumn pseudo = matchHgVersionsPseudoColumn(colname);

					if (pseudo != HG_VERSIONS_PSEUDO_NONE)
						node = buildHgVersionsExpr(pstate, nsitem, levels_up,
												   pseudo, cref->has_outer_join, cref->location);
				}
				/* End - ReqID:SRS-SECURITY-FLASHBACK */

				if (node == NULL)
				{
					/* Try it as a function call on the whole row */
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					node = ParseFuncOrColumn(pstate,
											 list_make1(makeString(colname)),
											 list_make1(node),
											 pstate->p_last_srf,
											 NULL,
											 false,
											 cref->location);
				}
				break;
			}
		case 4:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);
				Node	   *field2 = (Node *) lsecond(cref->fields);
				Node	   *field3 = (Node *) lthird(cref->fields);
				Node	   *field4 = (Node *) lfourth(cref->fields);
				char	   *catname;

				Assert(IsA(field1, String));
				catname = strVal(field1);
				Assert(IsA(field2, String));
				nspname = strVal(field2);
				Assert(IsA(field3, String));
				relname = strVal(field3);

				/* Begin - ReqID:SRS-SRS-MYSQL-CASE */
				if (compatible_mode == MYS_MODE)
				{
					if (mysql_lower_hook())
					{
						nspname = mysql_string_case_hook(nspname);
						relname = mysql_string_case_hook(relname);
					}
					if (!pg_strcasecmp(relname, "excluded"))
						relname = mysql_string_case_hook(relname);
				}
				/* End - ReqID:SRS-SRS-MYSQL-CASE */
				/*
				 * We check the catalog name and then ignore it.
				 */
				if (strcmp(catname, get_database_name(MyDatabaseId)) != 0)
				{
					/* Begin - Bug#m351 */
					if(ORA_MODE == compatible_mode && IsA(field4, String) &&
						(!pg_strcasecmp(strVal(field4), "nextval") || !pg_strcasecmp(strVal(field4), "currval")))	/* Bug#m353 */
						elog(ERROR, "sequence does not exist");
					/* End - Bug#m351 */
					crerr = CRERR_WRONG_DB;
					break;
				}
				/* Begin - Bug#m351 */
				/* Begin - ReqID:SRS-SQL-SEQ */
				if(ORA_MODE == compatible_mode && IsA(field4, String) &&
					(!pg_strcasecmp(strVal(field4), "nextval") || !pg_strcasecmp(strVal(field4), "currval")))	/* Bug#m353 */
				{
					FuncCall *seq_func = NULL;
					/* Begin - Bug#m351 */
					Oid 	 nspoid;
					char	 nextvalarg[MAXPGPATH];
					/* End - Bug#m351 */
					A_Const *n = makeNode(A_Const);

					n->val.type = T_String;
					sprintf(nextvalarg, "%s.%s.%s", catname, nspname, relname);	/* Bug#m351 */
					n->val.val.str = nextvalarg;
					n->location = cref->location;
					/* Begin - Bug#m351 */
					nspoid = LookupExplicitNamespace(nspname, true);
					if (!OidIsValid(nspoid))
						elog(ERROR, "sequence does not exist");
					/* End - Bug#m351 */
					seq_func = makeFuncCall(list_make2(makeString("pg_catalog"),
						makeString(strVal(field4))), list_make1(n), COERCE_EXPLICIT_CALL, cref->location);
					/* Begin - SRS-SQL-MULTITABLEINSERT */
					/*
					 * insert all/first doesn't allow sequence
					 * ORA-02287, we doesn't place it on ParseFunOrColumn
					 * which can't decide wether it is pg sequnce
					 */
					if (pstate->p_stmt_kind == PARSE_KIND_MULTITABLE_INSERT)
						ereport(ERROR,
									(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
									 errmsg("sequence number not allowed here"),
									  parser_errposition(pstate, cref->location)));
					/* End - SRS-SQL-MULTITABLEINSERT */
					return transformFuncCall(pstate, seq_func);
				}
				/* End - ReqID:SRS-SQL-SEQ */
				/* End - Bug#m351 */

				/* Locate the referenced nsitem */
				nsitem = refnameNamespaceItem(pstate, nspname, relname,
											  cref->location,
											  &levels_up);
				if (nsitem == NULL)
				{
					crerr = CRERR_NO_RTE;
					break;
				}

				/* Whole-row reference? */
				if (IsA(field4, A_Star))
				{
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					break;
				}

				Assert(IsA(field4, String));
				colname = strVal(field4);

				/* Begin - Bug#Z167 */
				set_merge_on_attrno(pstate, colname);
				/* End - Bug#Z167 */

				/* Try to identify as a column of the nsitem */
				node = scanNSItemForColumnExt(pstate, nsitem, levels_up, colname, cref->has_outer_join, /* ReqID:SRS-SQL-OUTERJOIN(+) */
											  cref->location);

				/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
				if (pstate->p_tryVersionsPseudoColumn && node == NULL)
				{
					HgVersionsPseudoColumn pseudo = matchHgVersionsPseudoColumn(colname);

					if (pseudo != HG_VERSIONS_PSEUDO_NONE)
						node = buildHgVersionsExpr(pstate, nsitem, levels_up,
												   pseudo, cref->has_outer_join, cref->location);
				}
				/* End - ReqID:SRS-SECURITY-FLASHBACK */

				if (node == NULL)
				{
					/* Try it as a function call on the whole row */
					node = transformWholeRowRef(pstate, nsitem, levels_up,
												cref->location);
					node = ParseFuncOrColumn(pstate,
											 list_make1(makeString(colname)),
											 list_make1(node),
											 pstate->p_last_srf,
											 NULL,
											 false,
											 cref->location);
				}
				break;
			}
		default:
			crerr = CRERR_TOO_MANY; /* too many dotted names */
			break;
	}

	/*
	 * Now give the PostParseColumnRefHook, if any, a chance.  We pass the
	 * translation-so-far so that it can throw an error if it wishes in the
	 * case that it has a conflicting interpretation of the ColumnRef. (If it
	 * just translates anyway, we'll throw an error, because we can't undo
	 * whatever effects the preceding steps may have had on the pstate.) If it
	 * returns NULL, use the standard translation, or throw a suitable error
	 * if there is none.
	 */
	if (pstate->p_post_columnref_hook != NULL)
	{
		Node	   *hookresult;
		Node	   *tmpnode = NULL;

		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		if (compatible_mode == MYS_MODE && node)
		{
			tmpnode = copyObject(node);
			if (pstate->p_isexistVar)
				node = NULL;
		}
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */

		hookresult = pstate->p_post_columnref_hook(pstate, cref, node);

		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		if (compatible_mode == MYS_MODE && !hookresult && tmpnode)
		{
			if (node == NULL)
				node = tmpnode;
		}
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */
		else
		{
			if (node == NULL)
				node = hookresult;
			else if (hookresult != NULL)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_COLUMN),
						 errmsg("column reference \"%s\" is ambiguous",
								NameListToString(cref->fields)),
						 parser_errposition(pstate, cref->location)));
		}
	}

	/* Begin - OA#00002227 */
	/*
	 * maybe function argitem default is a package'var
	 */
	if (compatible_mode == ORA_MODE &&
		node == NULL &&
		pstate->p_expr_kind == EXPR_KIND_FUNCTION_DEFAULT)
	{
		Node *newnode;

		/* append pkgname to check */
		if (pstate->p_pkgname != NULL)
		{
			List *names = lcons(makeString(pstate->p_pkgname), (List *) copyObject(cref->fields));

			newnode = (Node *)  ParsePkgGlobalVarBynames(names, true);

			list_free_deep(names);

			if (newnode != NULL)
				return newnode;
		}

		newnode = (Node *) ParsePkgGlobalVarBynames(cref->fields, true);

		if (newnode != NULL)
			return newnode;
	}
	/* End - OA#00002227*/

	/*
	 * Throw error if no translation found.
	 */
	if ((compatible_mode == ORA_MODE && !missing_ok) ||
		compatible_mode == PG_MODE || compatible_mode == MYS_MODE)	/* ReqID:SRS-MISC-MYSQL-FRAME */
	{
		if (node == NULL)
		{
			switch (crerr)
			{
				case CRERR_NO_COLUMN:
					errorMissingColumn(pstate, relname, colname, cref->location);
					break;
				case CRERR_NO_RTE:
					errorMissingRTE(pstate, makeRangeVar(nspname, relname,
														 cref->location));
					break;
				case CRERR_WRONG_DB:
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("cross-database references are not implemented: %s",
									NameListToString(cref->fields)),
							 parser_errposition(pstate, cref->location)));
					break;
				case CRERR_TOO_MANY:
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("improper qualified name (too many dotted names): %s",
									NameListToString(cref->fields)),
							 parser_errposition(pstate, cref->location)));
					break;
			}
		}
	}

	return node;
}

/* Begin: Req:SRS-MYSQL-SQL-Select */
/*
 * get_tablename_colname_from_var
 *		Find the namespace alias name that exposed a Var.
 *
 * This is the inverse lookup for ordinary column Vars produced by
 * colNameToVar(): given a Var and the ParseState where it was seen, locate
 * the visible ParseNamespaceItem at varlevelsup and return
 * table name and column name.  The returned string is owned by the ParseState.
 */
void
get_tablename_colname_from_var(ParseState *pstate, Var *var, char **colName, char **tabName)
{
	ParseState *level_pstate = pstate;
	ListCell   *lc;

	if (pstate == NULL || var == NULL)
		return;

	foreach(lc, level_pstate->p_namespace)
	{
		ParseNamespaceItem *nsitem = (ParseNamespaceItem *) lfirst(lc);
		int			i;
		int			ncols;

		if (!nsitem->p_cols_visible)
			continue;
		if (nsitem->p_lateral_only && !level_pstate->p_lateral_active)
			continue;

		ncols = list_length(nsitem->p_names->colnames);
		for (i = 0; i < ncols; i++)
		{
			ParseNamespaceColumn *nscol = &nsitem->p_nscolumns[i];
			/*deleted col*/
			if (nscol->p_varno == 0)
				continue;

			if (nscol->p_varnosyn == var->varnosyn &&
				nscol->p_varattnosyn == var->varattnosyn)
			{
				if (colName)
					*colName = strVal(list_nth(nsitem->p_names->colnames, i));
				if(tabName)
					*tabName = nsitem->p_names->aliasname;
				return ;
			}
		}
	}

}

Node *
search_targetList_and_groupList(ParseState *orig_pstate,
								ParseState *level_pstate,
								ColumnRef *cref,
								int location,
								int sublevels_up,
								bool second_parsing,
								TargetEntry **target_tle)
{
	/* Per-level rule: prefer targetList alias, then try GROUP BY found_name. */
	Node *target_result = NULL;
	Node *group_result = NULL;
	TargetEntry *found = NULL;
	TargetEntry *found_unaliased = NULL;
	bool found_unaliased_non_uniq = false;
	ListCell *lc;
	const char *found_name = strVal(llast(cref->fields));

	if (level_pstate->p_targetlist)
	{
		foreach (lc, *(level_pstate->p_targetlist))
		{
			TargetEntry *tle = (TargetEntry *)lfirst(lc);
			/* Begin - BUG#M7840 M7839 */
			char *orig_name = NULL;
			char *alias_name = tle->resname;
			bool alias_name_match = false;
			bool orig_name_match = false;
			/**
			 * invisible  not to users, continue.
			 * If found != NULL, Directly use high priority candidates(alias).
			 * If found == NULL, then look at found_unaliased(table col).
			 * If the found_unaliased is not unique and there are no high priority candidates(alias), it will be reported as ambiguous.
			 *
			 * if is second parsing. only comparing the original colname is the same,and not alias.
			 */
			if (tle->resjunk)
				continue;

			alias_name_match = (alias_name != NULL && pg_strcasecmp(alias_name, found_name) == 0);

			/* Olny contain one col that targetList item, eg: a as b */
			if (IsA(tle->expr, Var))
			{
				get_tablename_colname_from_var(level_pstate, (Var *)tle->expr, &orig_name, NULL);
				orig_name_match = (orig_name != NULL && pg_strcasecmp(orig_name, found_name) == 0);

				/* scan the targetList once and record the tle with the same alias expr */
				if (!second_parsing && alias_name_match)
				{
					if (found != NULL)
					{
						if (equal((Node *)found->expr, (Node *)tle->expr))
							continue;

						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_COLUMN),
								 errmsg("Column \"%s\" in %s clause is ambiguous",
										found_name,
										ParseExprKindName(orig_pstate->p_expr_kind)),
								 parser_errposition(orig_pstate, location)));
					}
					found = tle;
				}
				/* scan the targetList once and record the tle with the same table col expr */
				else if (orig_name_match)
				{
					if (found_unaliased != NULL)
					{
						if (equal((Node *)found_unaliased->expr,
								  (Node *)tle->expr))
							continue;

						found_unaliased_non_uniq = true;
					}
					found_unaliased = tle;
				}
			}
			/* The target item is not a regular field, such as aggregation, func, expression, strong conversion */
			else
			{
				if (found && alias_name_match)
				{
					if (equal((Node *)found->expr, (Node *)tle->expr))
						continue;

					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_COLUMN),
							 errmsg("Column \"%s\" in %s clause is ambiguous",
									found_name,
									ParseExprKindName(orig_pstate->p_expr_kind)),
							 parser_errposition(orig_pstate, location)));
				}
				/* strong conversion : cast(a as int) as  a , cast(a as int)
				 * both tle->resname=a, deistinction should be tle->resname_is_alias.
				 * cast(a as int) as  a -> tle->resname_is_alias=true.
				 * cast(a as int)		-> tle->resname_is_alias=false.
				 * 
				 * cast(a as int) the resname from the expression in FigUreColname();
				 * cast(a as int) as  a  the resname from As a is an alias explicitly written by the user,
				 * at the TargetEntry level.
				 * 
				 * Mysql does not consider cast(a as int) to be an alias match.
				 */
				if (tle->resname_is_alias && alias_name_match)
					found = tle;
			}
		}

		/* If the found is empty, it means that no alias was found that is the same tle, so check low priority table col */
		if (second_parsing || found == NULL)
		{
			/* just no alias to choose low priority table col to return judge ambiguous */
			if (found_unaliased_non_uniq)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_COLUMN),
						 errmsg("Column \"%s\" in %s clause is ambiguous",
								found_name,
								ParseExprKindName(orig_pstate->p_expr_kind)),
						 parser_errposition(orig_pstate, location)));

			found = found_unaliased;
		}
		/**
		 * copy the high priority and first same alias expr
		 */
		if (found != NULL)
		{
			target_result = copyObject((Node *)found->expr);
			IncrementVarSublevelsUp(target_result, sublevels_up, 0);
		}
	}
	/* End - BUG#M7840 M7839 */

	if (orig_pstate->p_expr_kind != EXPR_KIND_GROUP_BY)
	{
		foreach (lc, level_pstate->p_group_clauses)
		{
			Node *gexpr = (Node *)lfirst(lc);
			Node *group_expr;

			if (!IsA(gexpr, ColumnRef) ||
				list_length(((ColumnRef *)gexpr)->fields) != 1)
				continue;

			if (pg_strcasecmp(strVal(linitial(((ColumnRef *)gexpr)->fields)), found_name) != 0)
				continue;

			group_expr = colNameToVarExt(level_pstate, found_name, false, cref->has_outer_join, location, true);
			if (group_expr == NULL)
				continue;
			IncrementVarSublevelsUp(group_expr, sublevels_up, 0);

			group_result = group_expr;
			break;
		}
	}

	/*
	 * if the same name matches both a SELECT alias and a GROUP BY item,
	 * first check whether they are the same expression.
	 * If they differ, treat it as ambiguous; on the HAVING-fix path, prefer GROUP BY.
	 */
	if (target_result != NULL && group_result != NULL)
	{
		if (!equal(target_result, group_result))
		{
			ereport(WARNING,
					(errcode(ERRCODE_AMBIGUOUS_COLUMN),
					 errmsg("column reference \"%s\" in HAVING is ambiguous", found_name),
					 parser_errposition(orig_pstate, location)));

			if (orig_pstate->p_expr_kind == EXPR_KIND_HAVING)
				return group_result;
		}
	}

	if (target_result != NULL)
	{
		if (target_tle != NULL)
			*target_tle = found;
		return target_result;
	}

	return group_result;
}

static bool
can_find_targetList_and_groupList(ParseState *pstate)
{
	return 	pstate->p_expr_kind == EXPR_KIND_GROUP_BY ||
			pstate->p_expr_kind == EXPR_KIND_HAVING ||
			pstate->p_expr_kind == EXPR_KIND_ORDER_BY ||
			pstate->p_expr_kind == EXPR_KIND_HAVING_FUNCTION;
}

static bool
current_in_having_clause(ParseState *pstate)
{
	ParseState *parent = pstate;

	while (parent != NULL)
	{
		if (parent->p_expr_kind == EXPR_KIND_HAVING)
		{
			return true;
		}
		parent = parent->parentParseState;
	}

	return false;
}
static Node *
resolve_having_ref(ParseState *pstate, ColumnRef *cref)
{
	char *name;
	Node *result;
	ParseState *level_pstate;
	int sublevels_up;

	name = strVal(llast(cref->fields));

	result = search_targetList_and_groupList(pstate, pstate, cref,
											 cref->location, 0, false, NULL);
	if (result != NULL)
		return result;

	/*
	 * walk outward level by level, first trying targetList/GROUP BY,
	 * then deciding by context whether base-table column lookup is allowed.
	 */
	level_pstate = pstate->parentParseState;
	sublevels_up = 1;
	while (level_pstate != NULL)
	{
		/*
		 *  these contexts allow lookup
		 * in outer SELECT targetList / GROUP BY names first.
		 */
		if (can_find_targetList_and_groupList(level_pstate))
		{
			result = search_targetList_and_groupList(pstate, level_pstate,
													 cref, cref->location,
													 sublevels_up, false, NULL);
			if (result != NULL)
				return result;
		}

		/*
		 * In HAVING, base-table lookup is allowed only when this level has
		 * neither aggregate nor GROUP BY; other expression contexts stay allowed.
		 */
		if (level_pstate->p_expr_kind != EXPR_KIND_HAVING ||
			(!level_pstate->p_hasAggs &&
			 level_pstate->p_group_clauses == NIL))
		{
			result = colNameToVarExt(level_pstate, name, true, cref->has_outer_join, cref->location, false);
			IncrementVarSublevelsUp(result, sublevels_up, 0);
			if (result != NULL)
				return result;
		}

		level_pstate = level_pstate->parentParseState;
		sublevels_up++;
	}

	return NULL;
}

static Node *
resolve_having_field(ParseState *pstate, ColumnRef *cref)
{
	char *name;
	Node *result;
	ParseState *level_pstate;
	int sublevels_up;

	name = strVal(llast(cref->fields));

	/*
	 * walk outward level by level, first trying FROM table,
	 * then deciding by context whether targetList lookup is allowed.
	 */
	level_pstate = pstate;
	sublevels_up = 0;
	while (level_pstate != NULL)
	{

		/**
		 * fix_outer_field() wraps an outer field found while having clause
		 * as a qualified Item_ref, and then fixes that new ref again.
		 * Re-enter with table.colname so the second pass cannot match a same-named SELECT alias.
		 */

		char *matched_refname = NULL;
		result = colNameToVarExt(level_pstate, name, true, cref->has_outer_join, cref->location, false);
		IncrementVarSublevelsUp(result, sublevels_up, 0);

		if (result != NULL)
		{
			if (sublevels_up > 0)
			{
				get_tablename_colname_from_var(level_pstate, (Var *)result, NULL, &matched_refname);
				if (level_pstate->p_expr_kind == EXPR_KIND_HAVING && matched_refname != NULL)
				{
					ColumnRef *qualified_cref = makeNode(ColumnRef);
					qualified_cref->fields = list_make2(makeString(pstrdup(matched_refname)),
														makeString(pstrdup(name)));
					qualified_cref->location = cref->location;
					return transformColumnRefInternal(pstate, qualified_cref, false);
				}
			}
			return result;
		}
		/*
		 *  these contexts allow lookup
		 * in outer SELECT targetList / GROUP BY names first.
		 */
		if (can_find_targetList_and_groupList(level_pstate))
		{
			result = search_targetList_and_groupList(pstate, level_pstate,
													 cref, cref->location,
													 sublevels_up, false, NULL);
			if (result != NULL)
				return result;
		}

		level_pstate = level_pstate->parentParseState;
		sublevels_up++;
	}

	return NULL;
}
/* End: Req:SRS-MYSQL-SQL-Select */

/*
 * Transform a ColumnRef.
 *
 * If you find yourself changing this code, see also ExpandColumnRefStar.
 */
static Node *
transformColumnRef(ParseState *pstate, ColumnRef *cref)
{
	return transformColumnRefInternal(pstate, cref, false);
}
/* End - ReqID:SRS-SQL-PROC */

/* Begin - ReqID:SRS-SQL-CustomObject */
Node *
TryTransformColumnRef(ParseState *pstate, List *fields)
{
	ColumnRef  *cref = makeNode(ColumnRef);
	ParseExprKind save_expr_kind = pstate->p_expr_kind;
	Node	   *result;

	if (pstate->p_expr_kind == EXPR_KIND_COLUMN_DEFAULT ||
		pstate->p_expr_kind == EXPR_KIND_PARTITION_BOUND)
		return NULL;

	cref->fields = copyObject(fields);
	cref->location = -1;
	cref->has_outer_join = false;

	if (pstate->p_expr_kind == EXPR_KIND_NONE)
		pstate->p_expr_kind = EXPR_KIND_OTHER;

	result = transformColumnRefInternal(pstate, cref, true);
	pstate->p_expr_kind = save_expr_kind;

	return result;
}
/* End - ReqID:SRS-SQL-CustomObject */

/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
/*
 * ivorysql transform columnref for missing_ok is true
 */
Node*
IvyTransformColumnRef(ParseState *pstate, ColumnRef *cref, bool missing_ok)
{
	ParseExprKind expr_kind = pstate->p_expr_kind;
	Node *retNode;

	if (pstate->p_expr_kind == EXPR_KIND_NONE)
		pstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;

	retNode = transformColumnRefInternal(pstate, cref, missing_ok);
	pstate->p_expr_kind = expr_kind;

	return retNode;
}
/* End - ReqID:SRS-SQL-HierarchicalQueries */


static Node *
transformParamRef(ParseState *pstate, ParamRef *pref)
{
	Node	   *result;

	/*
	 * The core parser knows nothing about Params.  If a hook is supplied,
	 * call it.  If not, or if the hook returns NULL, throw a generic error.
	 */
	if (pstate->p_paramref_hook != NULL)
		result = pstate->p_paramref_hook(pstate, pref);
	else
		result = NULL;

	if (result == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_PARAMETER),
				 errmsg("there is no parameter $%d", pref->number),
				 parser_errposition(pstate, pref->location)));

	return result;
}

/* Begin - ReqID:SRS-SQL-ANONYOUT */
static Node *
transformOraParamRef(ParseState *pstate, OraParamRef *pref)
{
	Node	   *result;

	/*
	* The core parser knows nothing about Params.  If a hook is supplied,
	* call it.  If not, or if the hook returns NULL, throw a generic error.
	*/
	if (pstate->p_paramref_hook != NULL)
		result = (*pstate->p_paramref_hook) (pstate, (ParamRef *)pref);
	else
		result = NULL;

	if (result == NULL)
		ereport(ERROR,
		(errcode(ERRCODE_UNDEFINED_PARAMETER),
		errmsg("there is no parameter %s", pref->name),
		parser_errposition(pstate, pref->location)));

	return result;
}
/* End - ReqID:SRS-SQL-ANONYOUT */


/* Test whether an a_expr is a plain NULL constant or not */
static bool
exprIsNullConstant(Node *arg)
{
	if (arg && IsA(arg, A_Const))
	{
		A_Const    *con = (A_Const *) arg;

		if (con->val.type == T_Null)
			return true;
	}
	return false;
}

static Node *
transformAExprOp(ParseState *pstate, A_Expr *a)
{
	Node	   *lexpr = a->lexpr;
	Node	   *rexpr = a->rexpr;
	Node	   *result;

	/*
	 * Special-case "foo = NULL" and "NULL = foo" for compatibility with
	 * standards-broken products (like Microsoft's).  Turn these into IS NULL
	 * exprs. (If either side is a CaseTestExpr, then the expression was
	 * generated internally from a CASE-WHEN expression, and
	 * transform_null_equals does not apply.)
	 */
	if (Transform_null_equals &&
		list_length(a->name) == 1 &&
		strcmp(strVal(linitial(a->name)), "=") == 0 &&
		(exprIsNullConstant(lexpr) || exprIsNullConstant(rexpr)) &&
		(!IsA(lexpr, CaseTestExpr) && !IsA(rexpr, CaseTestExpr)))
	{
		NullTest   *n = makeNode(NullTest);

		n->nulltesttype = IS_NULL;
		n->location = a->location;

		if (exprIsNullConstant(lexpr))
			n->arg = (Expr *) rexpr;
		else
			n->arg = (Expr *) lexpr;

		result = transformExprRecurse(pstate, (Node *) n);
	}
	else if (lexpr && IsA(lexpr, RowExpr) &&
			 rexpr && IsA(rexpr, SubLink) &&
			 ((SubLink *) rexpr)->subLinkType == EXPR_SUBLINK)
	{
		/*
		 * Convert "row op subselect" into a ROWCOMPARE sublink. Formerly the
		 * grammar did this, but now that a row construct is allowed anywhere
		 * in expressions, it's easier to do it here.
		 */
		SubLink    *s = (SubLink *) rexpr;

		s->subLinkType = ROWCOMPARE_SUBLINK;
		s->testexpr = lexpr;
		s->operName = a->name;
		s->location = a->location;
		result = transformExprRecurse(pstate, (Node *) s);
	}
	else if (lexpr && IsA(lexpr, RowExpr) &&
			 rexpr && IsA(rexpr, RowExpr))
	{
		/* ROW() op ROW() is handled specially */
		lexpr = transformExprRecurse(pstate, lexpr);
		rexpr = transformExprRecurse(pstate, rexpr);

		result = make_row_comparison_op(pstate,
										a->name,
										castNode(RowExpr, lexpr)->args,
										castNode(RowExpr, rexpr)->args,
										a->location);
	}
	else
	{
		/* Ordinary scalar operator */
		Node	   *last_srf = pstate->p_last_srf;
		Oid 		ltypeId = InvalidOid;
		bool		has_outer_join = false;	/* ReqID:SRS-SQL-OUTERJOIN(+) */

		/* Begin - ReqID:SRS-SQL-ROWID */
		lexpr = transformExprRecurse(pstate, lexpr);
		ltypeId = exprType(lexpr);

		if (compatible_mode == ORA_MODE && ltypeId == ROWIDOID && nodeTag(rexpr) != T_TypeCast)
		{
			TypeCast   *castnode = makeNode(TypeCast);
			castnode->typeName = makeTypeNameFromNameList(list_make2(makeString("sys"), makeString("rowid")));
			castnode->arg = rexpr;
			castnode->location = -1;

			rexpr = transformExprRecurse(pstate, (Node *)castnode);
		}
		else
			rexpr = transformExprRecurse(pstate, rexpr);
		/* End - ReqID:SRS-SQL-ROWID */

		/* Begin BUG#Z959 */
		/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
		/* Can not outer join subquery */
		if (lexpr != NULL  && rexpr != NULL)
		{
			if (hasPlusFlag(lexpr) && nodeTag(rexpr) == T_SubLink)
				elog(ERROR, "a column may not be outer-joined to a subquery");

			/* Check if table columns with (+) in subqueries are referenced by external table */
			if (hasPlusFlag(lexpr) && pstate->parentParseState && nodeTag(lexpr) == T_Var)
			{
				int l_level = 0;
				int r_level = 0;
				l_level = ((Var *)lexpr)->varlevelsup;
				if (nodeTag(rexpr) == T_Var)
				{
					r_level = ((Var *)rexpr)->varlevelsup;
					if (l_level > r_level)
						elog(ERROR, "an outer join cannot be specified on a correlation column");
				}
			}

			if (hasPlusFlag(rexpr) && pstate->parentParseState && nodeTag(rexpr) == T_Var)
			{
				int l_level = 0;
				int r_level = 0;
				r_level = ((Var *)rexpr)->varlevelsup;
				if (nodeTag(lexpr) == T_Var)
				{
					l_level = ((Var *)lexpr)->varlevelsup;
					if (l_level < r_level)
						elog(ERROR, "an outer join cannot be specified on a correlation column");
				}
			}
		}
		/* End BUG#Z959 */

		if ((lexpr && hasPlusFlag(lexpr)) || hasPlusFlag(rexpr))
			has_outer_join = true;
		/* End - ReqID:SRS-SQL-OUTERJOIN(+) */

		result = (Node *) make_op_ext(pstate,
									 a->name,
									 lexpr,
									 rexpr,
									 last_srf,
									 has_outer_join,	/* ReqID:SRS-SQL-OUTERJOIN(+) */
									 a->location, AEXPR_OP);	/* SRS-MISC-COMPATDM-SQL */

	}

	return result;
}

static Node *
transformAExprOpAny(ParseState *pstate, A_Expr *a)
{
	Node	   *lexpr = transformExprRecurse(pstate, a->lexpr);
	Node	   *rexpr = transformExprRecurse(pstate, a->rexpr);

	return (Node *) make_scalar_array_op(pstate,
										 a->name,
										 true,
										 lexpr,
										 rexpr,
										 a->location);
}

static Node *
transformAExprOpAll(ParseState *pstate, A_Expr *a)
{
	Node	   *lexpr = transformExprRecurse(pstate, a->lexpr);
	Node	   *rexpr = transformExprRecurse(pstate, a->rexpr);

	return (Node *) make_scalar_array_op(pstate,
										 a->name,
										 false,
										 lexpr,
										 rexpr,
										 a->location);
}

static Node *
transformAExprDistinct(ParseState *pstate, A_Expr *a)
{
	Node	   *lexpr = a->lexpr;
	Node	   *rexpr = a->rexpr;
	Node	   *result;

	/*
	 * If either input is an undecorated NULL literal, transform to a NullTest
	 * on the other input. That's simpler to process than a full DistinctExpr,
	 * and it avoids needing to require that the datatype have an = operator.
	 */
	if (exprIsNullConstant(rexpr))
		return make_nulltest_from_distinct(pstate, a, lexpr);
	if (exprIsNullConstant(lexpr))
		return make_nulltest_from_distinct(pstate, a, rexpr);

	lexpr = transformExprRecurse(pstate, lexpr);
	rexpr = transformExprRecurse(pstate, rexpr);

	if (lexpr && IsA(lexpr, RowExpr) &&
		rexpr && IsA(rexpr, RowExpr))
	{
		/* ROW() op ROW() is handled specially */
		result = make_row_distinct_op(pstate, a->name,
									  (RowExpr *) lexpr,
									  (RowExpr *) rexpr,
									  a->location);
	}
	else
	{
		/* Ordinary scalar operator */
		result = (Node *) make_distinct_op(pstate,
										   a->name,
										   lexpr,
										   rexpr,
										   a->location);
	}

	/*
	 * If it's NOT DISTINCT, we first build a DistinctExpr and then stick a
	 * NOT on top.
	 */
	if (a->kind == AEXPR_NOT_DISTINCT)
		result = (Node *) makeBoolExpr(NOT_EXPR,
									   list_make1(result),
									   a->location);

	return result;
}

static Node *
transformAExprNullIf(ParseState *pstate, A_Expr *a)
{
	Node	   *lexpr = transformExprRecurse(pstate, a->lexpr);
	Node	   *rexpr = transformExprRecurse(pstate, a->rexpr);
	OpExpr	   *result;
	/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
	bool		has_outer_join = false;

	if (hasPlusFlag(lexpr) || hasPlusFlag(rexpr))
		has_outer_join = true;
	/* End - ReqID:SRS-SQL-OUTERJOIN(+) */

	result = (OpExpr *) make_op_ext(pstate,
									a->name,
									lexpr,
									rexpr,
									pstate->p_last_srf,
									has_outer_join,	/* ReqID:SRS-SQL-OUTERJOIN(+) */
									a->location, AEXPR_NULLIF);	/* SRS-MISC-COMPATDM-SQL */

	/*
	 * The comparison operator itself should yield boolean ...
	 */
	if (result->opresulttype != BOOLOID)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("NULLIF requires = operator to yield boolean"),
				 parser_errposition(pstate, a->location)));
	if (result->opretset)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
		/* translator: %s is name of a SQL construct, eg NULLIF */
				 errmsg("%s must not return a set", "NULLIF"),
				 parser_errposition(pstate, a->location)));

	/*
	 * ... but the NullIfExpr will yield the first operand's type.
	 */
	result->opresulttype = exprType((Node *) linitial(result->args));

	/*
	 * We rely on NullIfExpr and OpExpr being the same struct
	 */
	NodeSetTag(result, T_NullIfExpr);

	return (Node *) result;
}

static Node *
transformAExprIn(ParseState *pstate, A_Expr *a)
{
	Node	   *result = NULL;
	Node	   *lexpr;
	List	   *rexprs;
	List	   *rvars;
	List	   *rnonvars;
	bool		useOr;
	ListCell   *l;
	bool		has_outer_join = false;	/* ReqID:SRS-SQL-OUTERJOIN(+) */

	/*
	 * If the operator is <>, combine with AND not OR.
	 */
	if (strcmp(strVal(linitial(a->name)), "<>") == 0)
		useOr = false;
	else
		useOr = true;

	/*
	 * We try to generate a ScalarArrayOpExpr from IN/NOT IN, but this is only
	 * possible if there is a suitable array type available.  If not, we fall
	 * back to a boolean condition tree with multiple copies of the lefthand
	 * expression.  Also, any IN-list items that contain Vars are handled as
	 * separate boolean conditions, because that gives the planner more scope
	 * for optimization on such clauses.
	 *
	 * First step: transform all the inputs, and detect whether any contain
	 * Vars.
	 */
	lexpr = transformExprRecurse(pstate, a->lexpr);
	rexprs = rvars = rnonvars = NIL;
	foreach(l, (List *) a->rexpr)
	{
		Node	   *rexpr = transformExprRecurse(pstate, lfirst(l));

		/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
		if (hasPlusFlag(lexpr) || hasPlusFlag(rexpr))
			has_outer_join = true;
		/* End - ReqID:SRS-SQL-OUTERJOIN(+) */

		rexprs = lappend(rexprs, rexpr);
		if (contain_vars_of_level(rexpr, 0))
			rvars = lappend(rvars, rexpr);
		else
			rnonvars = lappend(rnonvars, rexpr);
	}

	/*
	 * ScalarArrayOpExpr is only going to be useful if there's more than one
	 * non-Var righthand item.
	 */
	if (list_length(rnonvars) > 1 && !type_is_enum(exprType(lexpr)) /* ReqID:SRS-MYSQL-ENUMTYPE */)
	{
		List	   *allexprs;
		Oid			scalar_type;
		Oid			array_type;

		/*
		 * Try to select a common type for the array elements.  Note that
		 * since the LHS' type is first in the list, it will be preferred when
		 * there is doubt (eg, when all the RHS items are unknown literals).
		 *
		 * Note: use list_concat here not lcons, to avoid damaging rnonvars.
		 */
		allexprs = list_concat(list_make1(lexpr), rnonvars);
		scalar_type = select_common_type(pstate, allexprs, NULL, NULL);

		/* We have to verify that the selected type actually works */
		if (OidIsValid(scalar_type) &&
			!verify_common_type(scalar_type, allexprs))
			scalar_type = InvalidOid;

		/*
		 * Do we have an array type to use?  Aside from the case where there
		 * isn't one, we don't risk using ScalarArrayOpExpr when the common
		 * type is RECORD, because the RowExpr comparison logic below can cope
		 * with some cases of non-identical row types.
		 */
		if (OidIsValid(scalar_type) && scalar_type != RECORDOID)
			array_type = get_array_type(scalar_type);
		else
			array_type = InvalidOid;
		if (array_type != InvalidOid)
		{
			/*
			 * OK: coerce all the right-hand non-Var inputs to the common type
			 * and build an ArrayExpr for them.
			 */
			List	   *aexprs;
			ArrayExpr  *newa;

			aexprs = NIL;
			foreach(l, rnonvars)
			{
				Node	   *rexpr = (Node *) lfirst(l);

				rexpr = coerce_to_common_type(pstate, rexpr,
											  scalar_type,
											  "IN");
				aexprs = lappend(aexprs, rexpr);
			}
			newa = makeNode(ArrayExpr);
			newa->array_typeid = array_type;
			/* array_collid will be set by parse_collate.c */
			newa->element_typeid = scalar_type;
			newa->elements = aexprs;
			newa->multidims = false;
			newa->location = -1;

			result = (Node *) make_scalar_array_op(pstate,
												   a->name,
												   useOr,
												   lexpr,
												   (Node *) newa,
												   a->location);

			/* Consider only the Vars (if any) in the loop below */
			rexprs = rvars;
		}
	}

	/*
	 * Must do it the hard way, ie, with a boolean expression tree.
	 */
	foreach(l, rexprs)
	{
		Node	   *rexpr = (Node *) lfirst(l);
		Node	   *cmp;

		if (IsA(lexpr, RowExpr) &&
			IsA(rexpr, RowExpr))
		{
			/* ROW() op ROW() is handled specially */
			cmp = make_row_comparison_op(pstate,
										 a->name,
										 copyObject(((RowExpr *) lexpr)->args),
										 ((RowExpr *) rexpr)->args,
										 a->location);
		}
		else
		{
			/* Ordinary scalar operator */
			cmp = (Node *) make_op_ext(pstate,
									  a->name,
									  copyObject(lexpr),
									   rexpr,
									   pstate->p_last_srf,
									   has_outer_join,	/* ReqID:SRS-SQL-OUTERJOIN(+) */
									   a->location, AEXPR_IN);	/* SRS-MISC-COMPATDM-SQL */
		}

		cmp = coerce_to_boolean(pstate, cmp, "IN");
		if (result == NULL)
			result = cmp;
		else
			result = (Node *) makeBoolExprExt(useOr ? OR_EXPR : AND_EXPR,
											  list_make2(result, cmp),
											  has_outer_join, /* ReqID:SRS-SQL-OUTERJOIN(+) */
											  a->location);
	}

	return result;
}

/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
static Node *
makeTypeCast(Node *arg, TypeName *typename, int location)
{
	TypeCast *n = makeNode(TypeCast);

	n->arg = arg;
	n->typeName = typename;
	n->location = location;

	return (Node *)n;
}

/* ReplaceEnumExpr
 * when comparing an enum type with a numeric type, the comparison is performed using
 * the enum index value. the enum node is wrapped in a function call node that coverts
 * the enum to its float8 index representation.
 */
static Node *
ReplaceEnumExpr(Oid typeoid, Node *expr)
{
	if (type_is_enum(typeoid))
		return makeTypeCast(expr, makeTypeName("float8"), exprLocation(expr));

	if (typeoid == UNKNOWNOID)
		return makeTypeCast(expr, makeTypeName("text"), exprLocation(expr));

	return expr;
}

/**
 * modify between and expression
 *
 * Note: when the input comparison type has enum and numeric types, covert enum to 
 * 		 double tupe and unknown type cast to text(subsequently rely on text-<int to 
 * 		 convert text to int type).
 *       subsequent comparison of double and int types.
 */
static void
CheckAndReplaceEnumExpr(ParseState *pstate, Node **paexpr, Node **pbexpr, Node **pcexpr)
{
	Oid 	oidaexpr,
			oidbexpr,
			oidcexpr;
	Node 	*transaexpr = NULL,
			*transbexpr = NULL,
			*transcexpr = NULL;
	Node 	*aexpr = *paexpr,
			*bexpr = *pbexpr,
			*cexpr = *pcexpr;

	transaexpr = transformExprRecurse(pstate, copyObject(aexpr));
	transbexpr = transformExprRecurse(pstate, copyObject(bexpr));
	transcexpr = transformExprRecurse(pstate, copyObject(cexpr));

	/* get expr oid */
	oidaexpr = exprType(transaexpr);
	oidbexpr = exprType(transbexpr);
	oidcexpr = exprType(transcexpr);

	/**
	 * In BETWEEN expression, if one of arguments is enum and one of arguments
	 * is number, we treat all argument as DOUBLE.
	 */
	if ((type_is_enum(oidaexpr) || type_is_enum(oidbexpr) || type_is_enum(oidcexpr)) &&
		(type_is_numeric(oidaexpr) || type_is_numeric(oidbexpr) || type_is_numeric(oidcexpr)))
	{
		*paexpr = ReplaceEnumExpr(oidaexpr, aexpr);
		*pbexpr = ReplaceEnumExpr(oidbexpr, bexpr);
		*pcexpr = ReplaceEnumExpr(oidcexpr, cexpr);
	}

	return;
}
/* End - ReqID:SRS-MYSQL-ENUMTYPE */

static Node *
transformAExprBetween(ParseState *pstate, A_Expr *a)
{
	Node	   *aexpr;
	Node	   *bexpr;
	Node	   *cexpr;
	Node	   *result;
	Node	   *sub1;
	Node	   *sub2;
	List	   *args;

	/* Deconstruct A_Expr into three subexprs */
	aexpr = a->lexpr;
	args = castNode(List, a->rexpr);
	Assert(list_length(args) == 2);
	bexpr = (Node *) linitial(args);
	cexpr = (Node *) lsecond(args);

	/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
	if (MYS_MODE == compatible_mode)
	{
		CheckAndReplaceEnumExpr(pstate, &aexpr, &bexpr, &cexpr);
	}
	/* End - ReqID:SRS-MYSQL-ENUMTYPE */

	/*
	 * Build the equivalent comparison expression.  Make copies of
	 * multiply-referenced subexpressions for safety.  (XXX this is really
	 * wrong since it results in multiple runtime evaluations of what may be
	 * volatile expressions ...)
	 *
	 * Ideally we would not use hard-wired operators here but instead use
	 * opclasses.  However, mixed data types and other issues make this
	 * difficult:
	 * http://archives.postgresql.org/pgsql-hackers/2008-08/msg01142.php
	 */
	switch (a->kind)
	{
		case AEXPR_BETWEEN:
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, ">=",
											   aexpr, bexpr,
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, "<=",
											   copyObject(aexpr), cexpr,
											   a->location));
			result = (Node *) makeBoolExpr(AND_EXPR, args, a->location);
			break;
		case AEXPR_NOT_BETWEEN:
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, "<",
											   aexpr, bexpr,
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, ">",
											   copyObject(aexpr), cexpr,
											   a->location));
			result = (Node *) makeBoolExpr(OR_EXPR, args, a->location);
			break;
		case AEXPR_BETWEEN_SYM:
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, ">=",
											   aexpr, bexpr,
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, "<=",
											   copyObject(aexpr), cexpr,
											   a->location));
			sub1 = (Node *) makeBoolExpr(AND_EXPR, args, a->location);
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, ">=",
											   copyObject(aexpr), copyObject(cexpr),
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, "<=",
											   copyObject(aexpr), copyObject(bexpr),
											   a->location));
			sub2 = (Node *) makeBoolExpr(AND_EXPR, args, a->location);
			args = list_make2(sub1, sub2);
			result = (Node *) makeBoolExpr(OR_EXPR, args, a->location);
			break;
		case AEXPR_NOT_BETWEEN_SYM:
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, "<",
											   aexpr, bexpr,
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, ">",
											   copyObject(aexpr), cexpr,
											   a->location));
			sub1 = (Node *) makeBoolExpr(OR_EXPR, args, a->location);
			args = list_make2(makeSimpleA_Expr(AEXPR_OP, "<",
											   copyObject(aexpr), copyObject(cexpr),
											   a->location),
							  makeSimpleA_Expr(AEXPR_OP, ">",
											   copyObject(aexpr), copyObject(bexpr),
											   a->location));
			sub2 = (Node *) makeBoolExpr(OR_EXPR, args, a->location);
			args = list_make2(sub1, sub2);
			result = (Node *) makeBoolExpr(AND_EXPR, args, a->location);
			break;
		default:
			elog(ERROR, "unrecognized A_Expr kind: %d", a->kind);
			result = NULL;		/* keep compiler quiet */
			break;
	}

	return transformExprRecurse(pstate, result);
}

static Node *
transformBoolExpr(ParseState *pstate, BoolExpr *a)
{
	List	   *args = NIL;
	const char *opname;
	ListCell   *lc;
	bool		has_outer_join_lable = false;	/* ReqID:SRS-SQL-OUTERJOIN(+) */

	switch (a->boolop)
	{
		case AND_EXPR:
			opname = "AND";
			break;
		case OR_EXPR:
			opname = "OR";
			break;
		case NOT_EXPR:
			opname = "NOT";
			break;
		default:
			elog(ERROR, "unrecognized boolop: %d", (int) a->boolop);
			opname = NULL;		/* keep compiler quiet */
			break;
	}

	foreach(lc, a->args)
	{
		Node	   *arg = (Node *) lfirst(lc);

		arg = transformExprRecurse(pstate, arg);
		/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
		if (hasPlusFlag(arg))
			has_outer_join_lable = true;
		/* End - ReqID:SRS-SQL-OUTERJOIN(+) */
		arg = coerce_to_boolean(pstate, arg, opname);
		args = lappend(args, arg);
	}

	return (Node *) makeBoolExprExt(a->boolop, args, has_outer_join_lable, a->location);	/* ReqID:SRS-SQL-OUTERJOIN(+) */
}

static Node *
transformFuncCall(ParseState *pstate, FuncCall *fn)
{
	Node	   *last_srf = pstate->p_last_srf;
	List	   *targs;
	ListCell   *args;
	/* Begin - BUG#M0000376 */
	Node	   *result;
	bool		need_reset_p_oraparammode = false;

	/*
	 * If enable_get_oraparam_mode is on, copy fn->args to fn->rawargs.
	 * fn->rawargs are the list of arguments without doing transform.
	 * because only after find the function and get the arguments mode,
	 * we can get parameters by doing transform fn->rawargs.
	 */
	if (enable_get_oraparam_mode && fn->args != NIL)
	{
		/*
		 * To get the placeholder mode, we do transform the FuncCall expr.
		 * IF in a nested function call, a function argument is another function call,
		 * e.g., id = func1(func2(:x), :y); the :x and :y are placeholder,
		 * when transform func2, we already set argument modes for the function func2,
		 * then transform func1, we don't need to transform func2.
		 *
		 * If there aren't nested function call, just do tranform.
		 */
		if (pstate->p_not_set_oraparammode &&
			pstate->p_oraparammode != ' ')
		{
			pstate->p_not_transform_funcCall = true;
		}
		else
		{
			fn->rawargs = list_copy_deep(fn->args);
			if (!pstate->p_not_set_oraparammode)
			{
				pstate->p_not_set_oraparammode = true;
				pstate->p_oraparammode = ' ';
				need_reset_p_oraparammode = true;
			}
		}
	}
	/* End - BUG#M0000376 */

	/* Begin - ReqID:SRS-MYSQL-CASE */
	if (fn->funcname != NIL && mysql_string_case_hook &&
		mysql_lower_hook && mysql_lower_hook())
	{
		if (list_length(fn->funcname) == 1 ||
			(list_length(fn->funcname) == 2 && IsA(linitial(fn->funcname), String) &&
			 !pg_strcasecmp(strVal(linitial(fn->funcname)), "pg_catalog")))
		{
			if (IsA(llast(fn->funcname), String) &&
				(!pg_strcasecmp(strVal(llast(fn->funcname)), "nextval") ||
				 !pg_strcasecmp(strVal(llast(fn->funcname)), "currval")))
			{
				if (fn->args != NIL && list_length(fn->args) == 1 &&
					IsA(llast(fn->args), A_Const))
				{
					A_Const *aconst = (A_Const *)llast(fn->args);

					if (aconst->val.type == T_String)
					aconst->val.val.str = mysql_string_case_hook(aconst->val.val.str);
				}
			}
			else if (IsA(llast(fn->funcname), String) &&
					 (!pg_strcasecmp(strVal(llast(fn->funcname)), "setval")))
			{
				if (fn->args != NIL && (list_length(fn->args) == 2 || list_length(fn->args) == 3) &&
					IsA(linitial(fn->args), A_Const))
				{
					A_Const *aconst = (A_Const *)linitial(fn->args);
					if (aconst->val.type == T_String)
						aconst->val.val.str = mysql_string_case_hook(aconst->val.val.str);
				}
			}
		}
	}
	/* End - ReqID:SRS-MYSQL-CASE */

	/* Transform the list of arguments ... */
	targs = NIL;
	foreach(args, fn->args)
	{
		/* Begin: Req:SRS-MYSQL-SQL-Select */
		if (current_in_having_clause(pstate))
			targs = lappend(targs, transformExpr(pstate,
												 (Node *)lfirst(args), EXPR_KIND_HAVING_FUNCTION));
		else
			/* End: Req:SRS-MYSQL-SQL-Select */
			targs = lappend(targs, transformExprRecurse(pstate,
														(Node *) lfirst(args)));
	}

	/* Begin - BUG #Z5517 #Z5518 */
	if (MYS_MODE == compatible_mode && fn->funcname != NIL && targs != NIL)
	{
		if (list_length(fn->funcname) == 1 ||
			(list_length(fn->funcname) == 2 && IsA(linitial(fn->funcname), String) &&
			 !pg_strcasecmp(strVal(linitial(fn->funcname)), "pg_catalog")))
		{
			Node *funcname = llast(fn->funcname);

			if (IsA(funcname, String) &&
				(!pg_strcasecmp(strVal(funcname), "replace")))
			{
				List * new_targs_list = NIL;
				foreach(args, targs)
				{
					/*
					 * For the bool type,it must first be cast to `int` and then to `text`;
					 * other data types can be directly cast to `text`.
					 * -- bool  -> int -> text
					 * -- true  ->  1  -> '1'
					 * -- false ->  0  -> '0'
					 */
					Node *new_arg;

					if (exprType(lfirst(args)) == BOOLOID)
					{
						new_arg = coerce_to_target_type(pstate, lfirst(args), exprType(lfirst(args)),
														INT4OID, -1,
														COERCION_EXPLICIT,
														COERCE_IMPLICIT_CAST,
														-1);
						new_arg = coerce_to_target_type(pstate, new_arg, exprType(new_arg),
														TEXTOID, -1,
														COERCION_EXPLICIT,
														COERCE_IMPLICIT_CAST,
														-1);
						new_targs_list = lappend(new_targs_list, new_arg);
					}
					else if (exprType(lfirst(args)) == TEXTOID)
					{
						new_targs_list = lappend(new_targs_list, lfirst(args));
					}
					else
					{
						new_arg = coerce_to_target_type(pstate, lfirst(args), exprType(lfirst(args)),
														TEXTOID, -1,
														COERCION_EXPLICIT,
														COERCE_IMPLICIT_CAST,
														-1);
						new_targs_list = lappend(new_targs_list, new_arg);
					}
				}
				list_free(targs);
				targs = new_targs_list;
			}
		}
	}
	/* End - BUG #Z5517 #Z5518 */

	/*
	 * When WITHIN GROUP is used, we treat its ORDER BY expressions as
	 * additional arguments to the function, for purposes of function lookup
	 * and argument type coercion.  So, transform each such expression and add
	 * them to the targs list.  We don't explicitly mark where each argument
	 * came from, but ParseFuncOrColumn can tell what's what by reference to
	 * list_length(fn->agg_order).
	 */
	if (fn->agg_within_group)
	{
		Assert(fn->agg_order != NIL);
		foreach(args, fn->agg_order)
		{
			SortBy	   *arg = (SortBy *) lfirst(args);

			targs = lappend(targs, transformExpr(pstate, arg->node,
												 EXPR_KIND_ORDER_BY));
		}
	}

	/* Begin - ReqID:SRS-PLSQL-RECORD */
	if (DB_ORACLE == database_mode &&
		fn && last_srf == NULL &&
		fn->agg_filter == NULL &&
		fn->over == NULL &&
		!fn->agg_within_group &&
		!fn->agg_star &&
		!fn->agg_distinct &&
		!fn->func_variadic &&
		pstate->p_orarecordconstruct_hook != NULL)
	{
		/* check whether it is Oracle record's constructor */
		Node *node = pstate->p_orarecordconstruct_hook(pstate,
											 fn->funcname,
											 targs,
											 fn,
											 fn->location);

		if (node != NULL)
			return node;
	}
	/* End - ReqID:SRS-PLSQL-RECORD */

	PG_TRY();
	{
		/* ... and hand off to ParseFuncOrColumn */
		result = ParseFuncOrColumn(pstate,
								 fn->funcname,
								 targs,
								 last_srf,
								 fn,
								 false,
								 fn->location);
	}
	/* Begin - BUG#M0000376 */
	PG_CATCH();
	{
		if (enable_get_oraparam_mode && fn->args != NIL)
		{
			pstate->p_not_set_oraparammode = false;
			pstate->p_oraparammode = ' ';
			list_free_deep(fn->rawargs);
			fn->rawargs = NIL;
			pstate->p_not_transform_funcCall = false;
		}
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (enable_get_oraparam_mode
		&& fn->args != NIL)
	{
		list_free_deep(fn->rawargs);
		fn->rawargs = NIL;
		pstate->p_oraparammode = ' ';
		pstate->p_not_transform_funcCall = false;

		if (need_reset_p_oraparammode)
			pstate->p_not_set_oraparammode = false;
	}
	/* End - BUG#M0000376 */

	return result;
}

/* Begin - ReqID:SRS-SQL-PROC */
/*
 * First we assume it's a ColumnRef, if it's not a ColumnRef
 * we assume it's a FuncCall, still not, report an error.
 */
Node *
transformColumnRefOrFunCall(ParseState *pstate, ColumnRefOrFuncCall *cref_func)
{
	Node *node = transformColumnRefInternal(pstate, cref_func->cref, true);

	if (node == NULL)
	{
		PG_TRY();
		{
			node = transformFuncCall(pstate, cref_func->func);
		}
		PG_CATCH();
		{
			char str[264] = {'\0'};
			ListCell	*lc;
			int			i = 1;

			strcpy(str, "\"");
			foreach(lc, cref_func->cref->fields)
			{
				if (i > 1)
					strcat(str, "\".\"");
				strcat(str, ((Value *)lfirst(lc))->val.str);
				i++;
			}
			strcat(str, "\"");

			ereport(ERROR,
					(errcode(ERRCODE_INVALID_NAME),
					 errmsg("%s: invalid identifier", str),
					 parser_errposition(pstate, cref_func->cref->location)));
		}
		PG_END_TRY();
	}

	return node;
}
/* End - ReqID:SRS-SQL-PROC */

/* Begin - ReqID:SRS-SQL-ROWNUM */
/*
 * transformPseudoColumnRef:
 *	 transform ROWNUM
 */
static Node *
transformPseudoColumnRef(ParseState *pstate, PseudoColumnRef *pcr)
{
	Node	   *node = NULL;
	PseudoColVar	*pv = makeNode(PseudoColVar);

	pv->pvarkind = pcr->pckind;

	/* Begin - BUG#M0000451 */
	if (pstate->p_post_columnref_hook != NULL)
	{
		if (pv->pvarkind == PSEUDOCOL_KIND_ROWNUM)
		{
			ColumnRef	*cf = makeNode(ColumnRef);

			cf->type = T_ColumnRef;
			cf->fields = list_make1(makeString("rownum"));
			cf->location = pcr->location;
			cf->has_outer_join = false;

			node = pstate->p_post_columnref_hook(pstate, cf, NULL);
			if (node != NULL)
				return node;
		}
	}
	/* End - BUG#M0000451 */

	switch (pv->pvarkind)
	{
		case PSEUDOCOL_KIND_ROWNUM:
			/* Count RTE is add on demand. */
			pstate->p_hasRownum = true;

			pv->pvartype = INT8OID;
			pv->pvartypmod = -1;
			break;

		default:
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("Invalid pseudocolumn kind:%d", pv->pvarkind),
					parser_errposition(pstate, pcr->location)));
			break;
	}
	return (Node*) pv;
}
/* End - ReqID:SRS-SQL-ROWNUM */

static Node *
transformMultiAssignRef(ParseState *pstate, MultiAssignRef *maref)
{
	SubLink    *sublink;
	RowExpr    *rexpr;
	Query	   *qtree;
	TargetEntry *tle;

	/* We should only see this in first-stage processing of UPDATE tlists */
	Assert(pstate->p_expr_kind == EXPR_KIND_UPDATE_SOURCE);

	/* We only need to transform the source if this is the first column */
	if (maref->colno == 1)
	{
		/*
		 * For now, we only allow EXPR SubLinks and RowExprs as the source of
		 * an UPDATE multiassignment.  This is sufficient to cover interesting
		 * cases; at worst, someone would have to write (SELECT * FROM expr)
		 * to expand a composite-returning expression of another form.
		 */
		if (IsA(maref->source, SubLink) &&
			((SubLink *) maref->source)->subLinkType == EXPR_SUBLINK)
		{
			/* Relabel it as a MULTIEXPR_SUBLINK */
			sublink = (SubLink *) maref->source;
			sublink->subLinkType = MULTIEXPR_SUBLINK;
			/* And transform it */
			sublink = (SubLink *) transformExprRecurse(pstate,
													   (Node *) sublink);

			qtree = castNode(Query, sublink->subselect);

			/* Check subquery returns required number of columns */
			if (count_nonjunk_tlist_entries(qtree->targetList) != maref->ncolumns)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("number of columns does not match number of values"),
						 parser_errposition(pstate, sublink->location)));

			/*
			 * Build a resjunk tlist item containing the MULTIEXPR SubLink,
			 * and add it to pstate->p_multiassign_exprs, whence it will later
			 * get appended to the completed targetlist.  We needn't worry
			 * about selecting a resno for it; transformUpdateStmt will do
			 * that.
			 */
			tle = makeTargetEntry((Expr *) sublink, 0, NULL, true);
			pstate->p_multiassign_exprs = lappend(pstate->p_multiassign_exprs,
												  tle);

			/*
			 * Assign a unique-within-this-targetlist ID to the MULTIEXPR
			 * SubLink.  We can just use its position in the
			 * p_multiassign_exprs list.
			 */
			sublink->subLinkId = list_length(pstate->p_multiassign_exprs);
		}
		else if (IsA(maref->source, RowExpr))
		{
			/* Transform the RowExpr, allowing SetToDefault items */
			rexpr = (RowExpr *) transformRowExpr(pstate,
												 (RowExpr *) maref->source,
												 true);

			/* Check it returns required number of columns */
			if (list_length(rexpr->args) != maref->ncolumns)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("number of columns does not match number of values"),
						 parser_errposition(pstate, rexpr->location)));

			/*
			 * Temporarily append it to p_multiassign_exprs, so we can get it
			 * back when we come back here for additional columns.
			 */
			tle = makeTargetEntry((Expr *) rexpr, 0, NULL, true);
			pstate->p_multiassign_exprs = lappend(pstate->p_multiassign_exprs,
												  tle);
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("source for a multiple-column UPDATE item must be a sub-SELECT or ROW() expression"),
					 parser_errposition(pstate, exprLocation(maref->source))));
	}
	else
	{
		/*
		 * Second or later column in a multiassignment.  Re-fetch the
		 * transformed SubLink or RowExpr, which we assume is still the last
		 * entry in p_multiassign_exprs.
		 */
		Assert(pstate->p_multiassign_exprs != NIL);
		tle = (TargetEntry *) llast(pstate->p_multiassign_exprs);
	}

	/*
	 * Emit the appropriate output expression for the current column
	 */
	if (IsA(tle->expr, SubLink))
	{
		Param	   *param;

		sublink = (SubLink *) tle->expr;
		Assert(sublink->subLinkType == MULTIEXPR_SUBLINK);
		qtree = castNode(Query, sublink->subselect);

		/* Build a Param representing the current subquery output column */
		tle = (TargetEntry *) list_nth(qtree->targetList, maref->colno - 1);
		Assert(!tle->resjunk);

		param = makeNode(Param);
		param->paramkind = PARAM_MULTIEXPR;
		param->paramid = (sublink->subLinkId << 16) | maref->colno;
		param->paramtype = exprType((Node *) tle->expr);
		param->paramtypmod = exprTypmod((Node *) tle->expr);
		param->paramcollid = exprCollation((Node *) tle->expr);
		param->location = exprLocation((Node *) tle->expr);
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		param->type = NULL;
		/* End - ReqID:SRS-PLSQL-RECORD */

		return (Node *) param;
	}

	if (IsA(tle->expr, RowExpr))
	{
		Node	   *result;

		rexpr = (RowExpr *) tle->expr;

		/* Just extract and return the next element of the RowExpr */
		result = (Node *) list_nth(rexpr->args, maref->colno - 1);

		/*
		 * If we're at the last column, delete the RowExpr from
		 * p_multiassign_exprs; we don't need it anymore, and don't want it in
		 * the finished UPDATE tlist.  We assume this is still the last entry
		 * in p_multiassign_exprs.
		 */
		if (maref->colno == maref->ncolumns)
			pstate->p_multiassign_exprs =
				list_delete_last(pstate->p_multiassign_exprs);

		return result;
	}

	elog(ERROR, "unexpected expr type in multiassign list");
	return NULL;				/* keep compiler quiet */
}

static Node *
transformCaseExpr(ParseState *pstate, CaseExpr *c)
{
	CaseExpr   *newc = makeNode(CaseExpr);
	Node	   *last_srf = pstate->p_last_srf;
	Node	   *arg;
	CaseTestExpr *placeholder;
	List	   *newargs;
	List	   *resultexprs;
	ListCell   *l;
	Node	   *defresult;
	Oid			ptype;

	/* transform the test expression, if any */
	arg = transformExprRecurse(pstate, (Node *) c->arg);

	/* generate placeholder for test expression */
	if (arg)
	{
		/*
		 * If test expression is an untyped literal, force it to text. We have
		 * to do something now because we won't be able to do this coercion on
		 * the placeholder.  This is not as flexible as what was done in 7.4
		 * and before, but it's good enough to handle the sort of silly coding
		 * commonly seen.
		 */
		if (exprType(arg) == UNKNOWNOID)
		{
			/* Begin - BUG#Z1506 */
			Oid	unknown_type;

			if (compatible_mode == ORA_MODE)
			{
				/* Begin - BUG#8431 */
				if (nls_length_semantics == NLS_LENGTH_BYTE && !pstate->p_parent_cte)
					unknown_type = ORAVARCHARBYTEOID;
				else
					unknown_type = ORAVARCHARCHAROID;
				/* End - BUG#8431 */
			}
			else
				unknown_type = TEXTOID;
			/* End - BUG#Z1506 */

			arg = coerce_to_common_type(pstate, arg, unknown_type /* BUG#Z1506 */, "CASE");
		}

		/*
		 * Run collation assignment on the test expression so that we know
		 * what collation to mark the placeholder with.  In principle we could
		 * leave it to parse_collate.c to do that later, but propagating the
		 * result to the CaseTestExpr would be unnecessarily complicated.
		 */
		assign_expr_collations(pstate, arg);

		placeholder = makeNode(CaseTestExpr);
		placeholder->typeId = exprType(arg);
		placeholder->typeMod = exprTypmod(arg);
		placeholder->collation = exprCollation(arg);

		/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
		if (MYS_MODE == compatible_mode)
		{
			int encoding = -1;
			introducer_detect_encoding_walker((Node *)arg, &encoding);
			if(encoding >= 0)
				placeholder->typeId = VARCHAROID;

			/* Preprocessing that is neither an operator nor a function */
			introducer_convert_encoding_walker(arg, NULL);
		}
		/* End - SRS-MYSQL-CHARSET-INTRODUCER */
	}
	else
		placeholder = NULL;

	newc->arg = (Expr *) arg;

	/* transform the list of arguments */
	newargs = NIL;
	resultexprs = NIL;
	foreach(l, c->args)
	{
		CaseWhen   *w = lfirst_node(CaseWhen, l);
		CaseWhen   *neww = makeNode(CaseWhen);
		Node	   *warg;

		warg = (Node *) w->expr;
		if (placeholder)
		{
			/* shorthand form was specified, so expand... */
			warg = (Node *) makeSimpleA_Expr(AEXPR_OP, "=",
											 (Node *) placeholder,
											 warg,
											 w->location);
		}
		neww->expr = (Expr *) transformExprRecurse(pstate, warg);

		neww->expr = (Expr *) coerce_to_boolean(pstate,
												(Node *) neww->expr,
												"CASE/WHEN");

		warg = (Node *) w->result;
		neww->result = (Expr *) transformExprRecurse(pstate, warg);
		neww->location = w->location;

		/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
		if (MYS_MODE == compatible_mode)
			introducer_convert_encoding_walker((Node *)neww->result, NULL);
		/* End - SRS-MYSQL-CHARSET-INTRODUCER */

		newargs = lappend(newargs, neww);
		resultexprs = lappend(resultexprs, neww->result);
	}

	newc->args = newargs;

	/* transform the default clause */
	defresult = (Node *) c->defresult;
	if (defresult == NULL)
	{
		A_Const    *n = makeNode(A_Const);

		n->val.type = T_Null;
		n->location = -1;
		defresult = (Node *) n;
	}
	newc->defresult = (Expr *) transformExprRecurse(pstate, defresult);

	/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
	if (MYS_MODE == compatible_mode)
		introducer_convert_encoding_walker((Node *)newc->defresult, NULL);
	/* End - SRS-MYSQL-CHARSET-INTRODUCER */

	/*
	 * Note: default result is considered the most significant type in
	 * determining preferred type. This is how the code worked before, but it
	 * seems a little bogus to me --- tgl
	 */
	resultexprs = lcons(newc->defresult, resultexprs);

	/* Begin - bug#z6172 */
	if (compatible_mode == MYS_MODE)
	{
		if (mys_agg_field_type_hook)
		{
			ptype = mys_agg_field_type_hook(resultexprs);
			if (!OidIsValid(ptype))
			{
				ptype = select_common_type(pstate, resultexprs, "CASE", NULL);
			}
		}
		else
			ptype = select_common_type(pstate, resultexprs, "CASE", NULL);
	}
	else /* End - bug#z6172 */
		ptype = select_common_type(pstate, resultexprs, "CASE", NULL);
	Assert(OidIsValid(ptype));
	newc->casetype = ptype;
	/* casecollid will be set by parse_collate.c */

	/* Convert default result clause, if necessary */
	/* Begin - bug#z6172 */
	if (compatible_mode == MYS_MODE)
		newc->defresult = (Expr *)
			mys_coerce_to_common_type(pstate,
								  (Node *) newc->defresult,
								  ptype,
								  "CASE/ELSE");
	else /* End - bug#z6172 */
		newc->defresult = (Expr *)
			coerce_to_common_type(pstate,
								  (Node *) newc->defresult,
								  ptype,
								  "CASE/ELSE");
	/* Convert when-clause results, if necessary */
	foreach(l, newc->args)
	{
		CaseWhen   *w = (CaseWhen *) lfirst(l);

		/* Begin - bug#z6172 */
		if (compatible_mode == MYS_MODE)
			w->result = (Expr *)
				mys_coerce_to_common_type(pstate,
									  (Node *) w->result,
									  ptype,
									  "CASE/WHEN");
		else /* End - bug#z6172 */
			w->result = (Expr *)
				coerce_to_common_type(pstate,
									  (Node *) w->result,
									  ptype,
									  "CASE/WHEN");
	}

	/* if any subexpression contained a SRF, complain */
	if (pstate->p_last_srf != last_srf)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/* translator: %s is name of a SQL construct, eg GROUP BY */
				 errmsg("set-returning functions are not allowed in %s",
						"CASE"),
				 errhint("You might be able to move the set-returning function into a LATERAL FROM item."),
				 parser_errposition(pstate,
									exprLocation(pstate->p_last_srf))));

	newc->location = c->location;
	/* Begin - BUGID:0000543 */
	newc->is_decode = c->is_decode;
	/* End - BUGID:0000543 */

	return (Node *) newc;
}

/* Begin - BUGID:0000543 */
/* ReqID:SRS-SQL-SYSFUNC: change function name, 
 * transformCaseExpr_for_decode to transformdecodeexpr
 *
 * OA#1682
 * Refactor the function to support sublink and optimize performance
 * by using placeholder. eg.
 *   decode(expr, s1, v1, s2, v2)
 *   == ora_gram.y ==>
 *     case expr
 *       when s1 then v1
 *       when s2 then v2
 *       else null
 *     end
 *   == transformdecodeexpr ==>
 *     case expr
 *       when ($1 = s1) or ($1 is null and s1 is null) then v1
 *       when ($1 = s2) or ($1 is null and s2 is null) then v2
 *       else null
 *     end
 * $1 is the placeholder of expr.
 *
 * Record the historical bugs fixed or feature before refactoring:
 *   BUGID:0000543, BUG#Z4593, ReqID:SRS-SQL-OUTERJOIN(+),
 *   ReqID:SRS-SQL-SYSFUNC, ReqID:SRS-SQL-HierarchicalQueries
 */
static Node *
transformdecodeexpr(ParseState *pstate, CaseExpr *c)
{
	CaseExpr   *newc;
	Node	   *arg;
	CaseTestExpr *placeholder;
	List	   *newargs;
	List	   *resultexprs;
	ListCell   *l;
	Node	   *defresult;
	Oid			ptype = InvalidOid;
	Oid			restype = InvalidOid;
	CaseWhen   *first_when; /* BUG#Z4593 */
	bool		first_is_numeric = false; /* BUG#Z4593 */

	/* Begin - BUG#Z4593 */
	/* gram.y ensures this is true, future syntax changes will be captured here */
	Assert(list_length(c->args) >= 1);

	/*
	 * Oracle doc say:
	 * Oracle automatically converts expr and each search value
	 * to data type of the first search value before comparing.
	 * Oracle automatically converts the return value to the same
	 * data type as the first result.
	 */
	first_when = (CaseWhen *) linitial(c->args);
	arg = transformExprRecurse(pstate, (Node *) first_when->expr);
	ptype = exprType(arg);
	if (ptype == UNKNOWNOID)
	{
		/* Begin - BUG#8431 */
		if (nls_length_semantics == NLS_LENGTH_BYTE && !pstate->p_parent_cte)
			ptype = ORAVARCHARBYTEOID;
		else
			ptype = ORAVARCHARCHAROID;
		/* End - BUG#8431 */
	}
	/* Oracle noexist bigint data type, but I think add int8 would be better? */
	else if (ptype == INT4OID || ptype == INT2OID || ptype == INT8OID)
		ptype = NUMBEROID;

	arg = transformExprRecurse(pstate, (Node *) first_when->result);
	restype = exprType(arg);
	if (restype == UNKNOWNOID)
	{
		/* Begin - BUG#8431 */
		if (nls_length_semantics == NLS_LENGTH_BYTE && !pstate->p_parent_cte)
			restype = ORAVARCHARBYTEOID;
		else
			restype = ORAVARCHARCHAROID;
		/* End - BUG#8431 */
	}
	else if (restype == INT4OID || restype == INT2OID || restype == INT8OID)
		restype = NUMBEROID;

	/*
	 * Oracle doc say:
	 * If the first search-result pair are numeric, then Oracle compares
	 * all search-result expressions and the first expr to determine the
	 * argument with the highest numeric precedence, implicitly converts
	 * the remaining arguments to that data type, and returns that datatype.
	 */
#define ISNUMERICTYPE(t) ((t) == NUMBEROID || (t) == BINARY_FLOATOID || (t) == BINARY_DOUBLEOID)
	if (ISNUMERICTYPE(ptype) || ISNUMERICTYPE(restype))
		first_is_numeric = true;
	/* End - BUG#Z4593 */

	newc = makeNode(CaseExpr);

	/* transform the test expression, if any */
	/*
	 * ReqID:SRS-SQL-HierarchicalQueries:
	 * there, maybe c->args has sublink, if we don't
	 * copyit, maybe it has change its struct for SelectStmt
	 * to Query
	 */
	arg = transformExprRecurse(pstate, (Node *) copyObject(c->arg));

	/* Begin - BUG#Z4593 */
	if (arg && ISNUMERICTYPE(ptype) && ISNUMERICTYPE(exprType(arg)) && exprType(arg) > ptype)
		ptype = exprType(arg);
	/* End - BUG#Z4593 */

	if (database_mode == DB_ORACLE)
	{
		int			args_cnt = 0; /* BUG#Z4593 */
		List	   *whenresults = NIL; /* BUG#Z4593 */
		ListCell   *result_cell;
		List	   *whenexprs = NIL;

		foreach(l, c->args)
		{
			CaseWhen   *w = (CaseWhen *) lfirst(l);
			Node	   *warg;
			Node	   *wresult; /* BUG#Z4593 */
			Expr	   *arg_expr; /* BUG#Z4593 */
			Expr	   *result_expr; /* BUG#Z4593 */

			Assert(IsA(w, CaseWhen));

			warg = (Node *) w->expr;
			wresult = (Node *) w->result; /* BUG#Z4593 */

			arg_expr = (Expr *) transformExprRecurse(pstate, warg);
			whenexprs = lappend(whenexprs, arg_expr);

			/* Begin - BUG#Z4593 */
			result_expr = (Expr *) transformExprRecurse(pstate, wresult);
			whenresults = lappend(whenresults, result_expr);

			args_cnt++;

			/*
			 * For the special case of decode, if the first search-result pair
			 * are numeric, ptype and restype must be adjusted if necessary.
			 */
			if(first_is_numeric && args_cnt > 1)
			{
				Oid		new_ptype = exprType((Node *)arg_expr);
				Oid		new_restype = exprType((Node *)result_expr);

				/*
				 * There is no need to worry about the legacy processing of the
				 * int type, because int is mapped to NUMBER(38) in Oracle and
				 * NUMBER has the lowest precedence.
				 *
				 * Oracle precedence order is:
				 * BINARY_DOUBLEOID > BINARY_FLOATOID > NUMBEROID
				 * we using the OID check which argument with the highest numeric
				 * precedence, as a coincidence, the OIDs and precedence of numeric
				 * types are always increasing in order.
				 * This is a bit ugly, but it works.
				 */
				if (ISNUMERICTYPE(ptype) &&
					ISNUMERICTYPE(new_ptype) &&
					new_ptype > ptype)
					ptype = new_ptype;

				if (ISNUMERICTYPE(restype) &&
					ISNUMERICTYPE(new_restype) &&
					new_restype > restype)
					restype = new_restype;
			}
			/* End - BUG#Z4593 */
		}

		l = lnext(whenexprs, list_head(whenexprs));
		result_cell = lnext(whenresults, list_head(whenresults)); /* BUG#Z4593 */
		while (l && result_cell /* BUG#Z4593 */)
		{
			Expr   *w = (Expr *) lfirst(l);
			Oid			ntype = exprType((Node *) w);
			/* Begin - BUG#Z4593 */
			Expr   *wresult = (Expr *) lfirst(result_cell);
			Oid		wresult_type = exprType((Node *) wresult);
			/* End - BUG#Z4593 */

			if (!can_coerce_type(1, &ntype, &ptype, COERCION_IMPLICIT))
				ereport(ERROR,
						(errcode(ERRCODE_CANNOT_COERCE),
				/* translator: first %s is name of a SQL construct, eg CASE */
						 errmsg("%s could not convert type %s to %s",
								"DECODE",
								format_type_be(ntype),
								format_type_be(ptype)),
						 parser_errposition(pstate, exprLocation((Node *) w))));

			/* Begin - BUG#Z4593 */
			if (!can_coerce_type(1, &wresult_type, &restype, COERCION_IMPLICIT))
				ereport(ERROR,
						(errcode(ERRCODE_CANNOT_COERCE),
				/* translator: first %s is name of a SQL construct, eg CASE */
						 errmsg("%s could not convert type %s to %s",
								"DECODE",
								format_type_be(wresult_type),
								format_type_be(restype)),
						 parser_errposition(pstate, exprLocation((Node *) w))));
			/* End - BUG#Z4593 */

			l = lnext(whenexprs, l);
			result_cell = lnext(whenresults, result_cell); /* BUG#Z4593 */
		}
	}

	/* generate placeholder for test expression */
	if (arg)
	{
		if (OidIsValid(ptype))
			arg = coerce_to_common_type(pstate,
									  (Node *) arg,
									  ptype,
									  "DECODE");
		/*
		 * If test expression is an untyped literal, force it to text. We have
		 * to do something now because we won't be able to do this coercion on
		 * the placeholder.  This is not as flexible as what was done in 7.4
		 * and before, but it's good enough to handle the sort of silly coding
		 * commonly seen.
		 */
		else if (exprType(arg) == UNKNOWNOID)
		{
			if (compatible_mode == PG_MODE)
				arg = coerce_to_common_type(pstate, arg, TEXTOID, "DECODE");
			/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
			else if (compatible_mode == MYS_MODE)
				arg = coerce_to_common_type(pstate, arg, TEXTOID, "DECODE");
			/* End - ReqID:SRS-MISC-MYSQL-FRAME */
			else if (compatible_mode == ORA_MODE)
			{
				/* Begin - BUG#8431 */
				if (nls_length_semantics == NLS_LENGTH_BYTE && !pstate->p_parent_cte) /* BUG#Z091 */
					arg = coerce_to_common_type(pstate, arg, ORAVARCHARBYTEOID, "DECODE");
				else
					arg = coerce_to_common_type(pstate, arg, ORAVARCHARCHAROID, "DECODE");
				/* End - BUG#8431 */
			}
		}

		/*
		 * Run collation assignment on the test expression so that we know
		 * what collation to mark the placeholder with.  In principle we could
		 * leave it to parse_collate.c to do that later, but propagating the
		 * result to the CaseTestExpr would be unnecessarily complicated.
		 */
		assign_expr_collations(pstate, arg);

		placeholder = makeNode(CaseTestExpr);
		placeholder->typeId = exprType(arg);
		placeholder->typeMod = exprTypmod(arg);
		placeholder->collation = exprCollation(arg);
	}
	else
		placeholder = NULL;

	newc->arg = (Expr *) arg;

	/* transform the list of arguments */
	newargs = NIL;
	resultexprs = NIL;

	foreach(l, c->args)
	{
		CaseWhen   *w = (CaseWhen *) lfirst(l);
		CaseWhen   *neww = makeNode(CaseWhen);
		Node	   *warg;

		Assert(IsA(w, CaseWhen));

		warg = (Node *) w->expr;
		if (!c->is_decode)
		{
			if (placeholder)
			{
				/* shorthand form was specified, so expand... */
				warg = (Node *) makeSimpleA_Expr(AEXPR_OP, "=",
												 (Node *) placeholder,
												 warg,
												 w->location);
			}
			neww->expr = (Expr *) transformExprRecurse(pstate, warg);

			neww->expr = (Expr *) coerce_to_boolean(pstate,
													(Node *) neww->expr,
													"DECODE");
		}
		else
		{
			/* Begin - ReqID:SRS-SQL-SYSFUNC: Here you need to consider dealing 
			 * with decode null values, so the original method was changed.
			 */
			TypeCast	*expr_cast;
			TypeCast	*search_cast;
			NullTest	*nulltest;
			List		*nulltest_list = NIL;
			A_Expr 		*aexpr = NULL;
			Expr		*bexpr = NULL;
			List		*blist = NIL;

			if (placeholder)
			{
				/*
				 * decode(expr, s1, v1)
				 * == parser ==>
				 *   case expr
				 *     when s1 then v1
				 *     then null
				 *   end
				 * == tansfrom ==>
				 *   case expr
				 *     when ($1 = b) or (a is null and $1 is null) then c
				 *   end;
				 * $1 is the placeholder of expr.
				 */

				/*
				 * when ($1 = b) or (a is null and $1 is null) then c
				 *   step: ($1 = b)
				 */
				expr_cast = makeNode(TypeCast);
				search_cast = makeNode(TypeCast);

				search_cast->arg = (Node*) warg;
				search_cast->typeName = makeTypeNameFromOid(ptype, -1);
				search_cast->location = -1;

				expr_cast->arg = (Node*) placeholder;
				expr_cast->typeName = makeTypeNameFromOid(ptype, -1);
				expr_cast->location = -1;
				aexpr = makeSimpleA_Expr(AEXPR_OP, "=",
										 (Node *) expr_cast,
										 (Node *) search_cast,
										 w->location);

				/*
				 * when ($1 = b) or (a is null and $1 is null) then c
				 *   step: (a is null and $1 is null)
				 */
				nulltest = makeNode(NullTest);
				nulltest->arg = (Expr *) placeholder;
				nulltest->nulltesttype = IS_NULL;
				nulltest->location = -1;
				nulltest_list = lappend(nulltest_list, nulltest);

				nulltest = makeNode(NullTest);
				nulltest->arg = copyObject(w->expr);
				nulltest->nulltesttype = IS_NULL;
				nulltest->location = -1;
				nulltest_list = lappend(nulltest_list, nulltest);

				bexpr = makeBoolExpr(AND_EXPR, nulltest_list, -1);

				/*
				 * when ($1 = b) or (a is null and $1 is null) then c
				 *   step: ($1 = b) or (a is null and $1 is null)
				 */
				blist = lappend(blist, aexpr);
				blist = lappend(blist, bexpr);
				warg = (Node *) makeBoolExpr(OR_EXPR, blist, -1);
			}

			neww->expr = (Expr *) transformExprRecurse(pstate, warg);

			neww->expr = (Expr *) coerce_to_boolean(pstate,
													(Node *) neww->expr,
													"DECODE");

			warg = (Node *) w->orig_expr;
			neww->orig_expr = (Expr *) transformExprRecurse(pstate, warg);
			if (OidIsValid(ptype))
				neww->orig_expr = (Expr *) coerce_to_common_type(pstate,
													(Node *) neww->orig_expr,
													ptype,
													"DECODE");
		}

		warg = (Node *) w->result;
		neww->result = (Expr *) transformExprRecurse(pstate, warg);
		neww->location = w->location;

		newargs = lappend(newargs, neww);
		resultexprs = lappend(resultexprs, neww->result);
	}

	newc->args = newargs;

	/* transform the default clause */
	defresult = (Node *) c->defresult;
	if (defresult == NULL)
	{
		A_Const    *n = makeNode(A_Const);

		n->val.type = T_Null;
		n->location = -1;
		defresult = (Node *) n;
	}
	newc->defresult = (Expr *) transformExprRecurse(pstate, defresult);

	/* Begin - BUG#Z4593 */
	#if 0
	/*
	 * Note: default result is considered the most significant type in
	 * determining preferred type. This is how the code worked before, but it
	 * seems a little bogus to me --- tgl
	 */
	if (database_mode == DB_PG)
		resultexprs = lcons(newc->defresult, resultexprs);
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (database_mode == DB_MYSQL)
		resultexprs = lcons(newc->defresult, resultexprs);
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (database_mode == DB_ORACLE)
		resultexprs = lappend(resultexprs, newc->defresult);

	if (database_mode == DB_PG)
		ptype = select_common_type(pstate, resultexprs, "DECODE", NULL);
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (database_mode == DB_MYSQL)
		ptype = select_common_type(pstate, resultexprs, "DECODE", NULL);
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (database_mode == DB_ORACLE)
		ptype = select_common_type_for_nvl(pstate, resultexprs, "DECODE", NULL);
	Assert(OidIsValid(ptype));
	#endif
	/* End - BUG#Z4593 */

	newc->casetype = restype; /* BUG#Z4593 */
	/* casecollid will be set by parse_collate.c */

	/* Convert default result clause, if necessary */
	newc->defresult = (Expr *)
		coerce_to_common_type(pstate,
							  (Node *) newc->defresult,
							  restype, /* BUG#Z4593 */
							  "DECODE");

	/* Convert when-clause results, if necessary */
	foreach(l, newc->args)
	{
		CaseWhen   *w = (CaseWhen *) lfirst(l);

		w->result = (Expr *)
			coerce_to_common_type(pstate,
								  (Node *) w->result,
								  restype, /* BUG#Z4593 */
								  "DECODE");
	}

	newc->location = c->location;
	newc->is_decode = c->is_decode;

	return (Node *) newc;
}
/* End - BUGID:0000543 */

static Node *
transformSubLink(ParseState *pstate, SubLink *sublink)
{
	Node	   *result = (Node *) sublink;
	Query	   *qtree;
	const char *err;

	/*
	 * Check to see if the sublink is in an invalid place within the query. We
	 * allow sublinks everywhere in SELECT/INSERT/UPDATE/DELETE, but generally
	 * not in utility statements.
	 */
	err = NULL;
	switch (pstate->p_expr_kind)
	{
		case EXPR_KIND_NONE:
			Assert(false);		/* can't happen */
			break;
		case EXPR_KIND_OTHER:
			/* Accept sublink here; caller must throw error if wanted */
			break;
		case EXPR_KIND_JOIN_ON:
		case EXPR_KIND_JOIN_USING:
		case EXPR_KIND_FROM_SUBSELECT:
		case EXPR_KIND_FROM_FUNCTION:
		case EXPR_KIND_WHERE:
		case EXPR_KIND_POLICY:
		case EXPR_KIND_HAVING:
		case EXPR_KIND_FILTER:
		case EXPR_KIND_WINDOW_PARTITION:
		case EXPR_KIND_WINDOW_ORDER:
		case EXPR_KIND_WINDOW_FRAME_RANGE:
		case EXPR_KIND_WINDOW_FRAME_ROWS:
		case EXPR_KIND_WINDOW_FRAME_GROUPS:
		case EXPR_KIND_SELECT_TARGET:
		case EXPR_KIND_INSERT_TARGET:
		case EXPR_KIND_UPDATE_SOURCE:
		case EXPR_KIND_UPDATE_TARGET:
		case EXPR_KIND_MERGE_WHEN:
		case EXPR_KIND_GROUP_BY:
		case EXPR_KIND_ORDER_BY:
		case EXPR_KIND_DISTINCT_ON:
		case EXPR_KIND_LIMIT:
		case EXPR_KIND_OFFSET:
		case EXPR_KIND_RETURNING:
		case EXPR_KIND_VALUES:
		case EXPR_KIND_VALUES_SINGLE:
		case EXPR_KIND_CYCLE_MARK:
		/* Begin: Req:SRS-SQL-NLSSORT */
		case EXPR_KIND_GROUP_BY_DISTINCT:
		/* End: Req:SRS-SQL-NLSSORT */
		/* Begin - ReqID:SRS-RECOVERY-FLASHBACK */
		case EXPR_KIND_ASOF:
		case EXPR_KIND_VERSIONS_BETWEEN:
		/* End - ReqID:SRS-RECOVERY-FLASHBACK */
		/* Begin - ReqID:SRS-MYSQL-SQL-Select */
		case EXPR_KIND_HAVING_FUNCTION:
		/* End - ReqID:SRS-MYSQL-SQL-Select */
			/* okay */
			break;
		case EXPR_KIND_CHECK_CONSTRAINT:
		case EXPR_KIND_DOMAIN_CHECK:
			err = _("cannot use subquery in check constraint");
			break;
		case EXPR_KIND_COLUMN_DEFAULT:
		case EXPR_KIND_FUNCTION_DEFAULT:
			err = _("cannot use subquery in DEFAULT expression");
			break;
		case EXPR_KIND_INDEX_EXPRESSION:
			err = _("cannot use subquery in index expression");
			break;
		case EXPR_KIND_INDEX_PREDICATE:
			err = _("cannot use subquery in index predicate");
			break;
		case EXPR_KIND_STATS_EXPRESSION:
			err = _("cannot use subquery in statistics expression");
			break;
		case EXPR_KIND_ALTER_COL_TRANSFORM:
			err = _("cannot use subquery in transform expression");
			break;
		case EXPR_KIND_EXECUTE_PARAMETER:
			err = _("cannot use subquery in EXECUTE parameter");
			break;
		case EXPR_KIND_TRIGGER_WHEN:
			err = _("cannot use subquery in trigger WHEN condition");
			break;
		case EXPR_KIND_PARTITION_BOUND:
			err = _("cannot use subquery in partition bound");
			break;
		case EXPR_KIND_PARTITION_EXPRESSION:
			err = _("cannot use subquery in partition key expression");
			break;
		case EXPR_KIND_CALL_ARGUMENT:
			err = _("cannot use subquery in CALL argument");
			break;
		case EXPR_KIND_COPY_WHERE:
			err = _("cannot use subquery in COPY FROM WHERE condition");
			break;
		case EXPR_KIND_GENERATED_COLUMN:
			err = _("cannot use subquery in column generation expression");
			break;

			/*
			 * There is intentionally no default: case here, so that the
			 * compiler will warn if we add a new ParseExprKind without
			 * extending this switch.  If we do see an unrecognized value at
			 * runtime, the behavior will be the same as for EXPR_KIND_OTHER,
			 * which is sane anyway.
			 */
	}
	if (err)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg_internal("%s", err),
				 parser_errposition(pstate, sublink->location)));

	pstate->p_hasSubLinks = true;

	pstate->p_subselect = true; /* ReqID:SRS-SECURITY-DATADESENSITIZATION */
	/*
	 * OK, let's transform the sub-SELECT.
	 */
	qtree = parse_sub_analyze(sublink->subselect, pstate, NULL, false, true);

	/*
	 * Check that we got a SELECT.  Anything else should be impossible given
	 * restrictions of the grammar, but check anyway.
	 */
	if (!IsA(qtree, Query) ||
		qtree->commandType != CMD_SELECT)
		elog(ERROR, "unexpected non-SELECT command in SubLink");

	sublink->subselect = (Node *) qtree;

	if (sublink->subLinkType == EXISTS_SUBLINK)
	{
		/*
		 * EXISTS needs no test expression or combining operator. These fields
		 * should be null already, but make sure.
		 */
		sublink->testexpr = NULL;
		sublink->operName = NIL;
	}
	else if (sublink->subLinkType == EXPR_SUBLINK ||
			 sublink->subLinkType == ARRAY_SUBLINK)
	{
		/*
		 * Make sure the subselect delivers a single column (ignoring resjunk
		 * targets).
		 */
		if (count_nonjunk_tlist_entries(qtree->targetList) != 1)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("subquery must return only one column"),
					 parser_errposition(pstate, sublink->location)));

		/*
		 * EXPR and ARRAY need no test expression or combining operator. These
		 * fields should be null already, but make sure.
		 */
		sublink->testexpr = NULL;
		sublink->operName = NIL;
	}
	else if (sublink->subLinkType == MULTIEXPR_SUBLINK)
	{
		/* Same as EXPR case, except no restriction on number of columns */
		sublink->testexpr = NULL;
		sublink->operName = NIL;
	}
	else
	{
		/* ALL, ANY, or ROWCOMPARE: generate row-comparing expression */
		Node	   *lefthand;
		List	   *left_list;
		List	   *right_list;
		ListCell   *l;

		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		/* In mysql mode, can't specify target table for update in FROM clause. */
		if (MYS_MODE == compatible_mode && qtree &&
			pstate->p_isupdate && qtree->rtable) /* BUG#Z5121 */
		{
			ListCell   *lc;
			ListCell   *lc1;
			foreach (lc, pstate->p_updateRelations)
			{
				int		updateRelation = lfirst_int(lc);
				if (updateRelation <= list_length(pstate->p_rtable))
				{
					RangeTblEntry *rte1 = list_nth(pstate->p_rtable, updateRelation - 1);
					foreach(lc1, qtree->rtable)
					{
						RangeTblEntry *rte2 = (RangeTblEntry*)lfirst(lc1);
						if (rte1->relid == rte2->relid)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("You can't specify target table %s for update in FROM clause.", get_rel_name(rte1->relid))));
					}
				}
			}
		}
		/* End - ReqID:SRS-MYSQL-UPDATE */

		/*
		 * If the source was "x IN (select)", convert to "x = ANY (select)".
		 */
		if (sublink->operName == NIL)
			sublink->operName = list_make1(makeString("="));

		/*
		 * Transform lefthand expression, and convert to a list
		 */
		lefthand = transformExprRecurse(pstate, sublink->testexpr);
		if (lefthand && IsA(lefthand, RowExpr))
			left_list = ((RowExpr *) lefthand)->args;
		else
			left_list = list_make1(lefthand);

		/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
		if (hasPlusFlag(lefthand))
			elog(ERROR, "a column may not be outer-joined to a subquery");
		/* End - ReqID:SRS-SQL-OUTERJOIN(+) */

		/*
		 * Build a list of PARAM_SUBLINK nodes representing the output columns
		 * of the subquery.
		 */
		right_list = NIL;
		foreach(l, qtree->targetList)
		{
			TargetEntry *tent = (TargetEntry *) lfirst(l);
			Param	   *param;

			if (tent->resjunk)
				continue;

			param = makeNode(Param);
			param->paramkind = PARAM_SUBLINK;
			param->paramid = tent->resno;
			param->paramtype = exprType((Node *) tent->expr);
			param->paramtypmod = exprTypmod((Node *) tent->expr);
			param->paramcollid = exprCollation((Node *) tent->expr);
			param->location = -1;
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			param->type = NULL;
			/* End - ReqID:SRS-PLSQL-RECORD */

			right_list = lappend(right_list, param);
		}

		/*
		 * We could rely on make_row_comparison_op to complain if the list
		 * lengths differ, but we prefer to generate a more specific error
		 * message.
		 */
		if (list_length(left_list) < list_length(right_list))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("subquery has too many columns"),
					 parser_errposition(pstate, sublink->location)));
		if (list_length(left_list) > list_length(right_list))
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("subquery has too few columns"),
					 parser_errposition(pstate, sublink->location)));

		/*
		 * Identify the combining operator(s) and generate a suitable
		 * row-comparison expression.
		 */
		sublink->testexpr = make_row_comparison_op(pstate,
												   sublink->operName,
												   left_list,
												   right_list,
												   sublink->location);
	}

	return result;
}

/*
 * transformArrayExpr
 *
 * If the caller specifies the target type, the resulting array will
 * be of exactly that type.  Otherwise we try to infer a common type
 * for the elements using select_common_type().
 */
static Node *
transformArrayExpr(ParseState *pstate, A_ArrayExpr *a,
				   Oid array_type, Oid element_type, int32 typmod)
{
	ArrayExpr  *newa = makeNode(ArrayExpr);
	List	   *newelems = NIL;
	List	   *newcoercedelems = NIL;
	ListCell   *element;
	Oid			coerce_type;
	bool		coerce_hard;

	/*
	 * Transform the element expressions
	 *
	 * Assume that the array is one-dimensional unless we find an array-type
	 * element expression.
	 */
	newa->multidims = false;
	foreach(element, a->elements)
	{
		Node	   *e = (Node *) lfirst(element);
		Node	   *newe;

		/*
		 * If an element is itself an A_ArrayExpr, recurse directly so that we
		 * can pass down any target type we were given.
		 */
		if (IsA(e, A_ArrayExpr))
		{
			newe = transformArrayExpr(pstate,
									  (A_ArrayExpr *) e,
									  array_type,
									  element_type,
									  typmod);
			/* we certainly have an array here */
			Assert(array_type == InvalidOid || array_type == exprType(newe));
			newa->multidims = true;
		}
		else
		{
			newe = transformExprRecurse(pstate, e);

			/*
			 * Check for sub-array expressions, if we haven't already found
			 * one.  Note we don't accept domain-over-array as a sub-array,
			 * nor int2vector nor oidvector; those have constraints that don't
			 * map well to being treated as a sub-array.
			 */
			if (!newa->multidims)
			{
				Oid			newetype = exprType(newe);

				if (newetype != INT2VECTOROID && newetype != OIDVECTOROID &&
					type_is_array(newetype))
					newa->multidims = true;
			}
		}

		newelems = lappend(newelems, newe);
	}

	/*
	 * Select a target type for the elements.
	 *
	 * If we haven't been given a target array type, we must try to deduce a
	 * common type based on the types of the individual elements present.
	 */
	if (OidIsValid(array_type))
	{
		/* Caller must ensure array_type matches element_type */
		Assert(OidIsValid(element_type));
		coerce_type = (newa->multidims ? array_type : element_type);
		coerce_hard = true;
	}
	else
	{
		/* Can't handle an empty array without a target type */
		if (newelems == NIL)
			ereport(ERROR,
					(errcode(ERRCODE_INDETERMINATE_DATATYPE),
					 errmsg("cannot determine type of empty array"),
					 errhint("Explicitly cast to the desired type, "
							 "for example ARRAY[]::integer[]."),
					 parser_errposition(pstate, a->location)));

		/* Select a common type for the elements */
		coerce_type = select_common_type(pstate, newelems, "ARRAY", NULL);

		if (newa->multidims)
		{
			array_type = coerce_type;
			element_type = get_element_type(array_type);
			if (!OidIsValid(element_type))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("could not find element type for data type %s",
								format_type_be(array_type)),
						 parser_errposition(pstate, a->location)));
		}
		else
		{
			element_type = coerce_type;
			array_type = get_array_type(element_type);
			if (!OidIsValid(array_type))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("could not find array type for data type %s",
								format_type_be(element_type)),
						 parser_errposition(pstate, a->location)));
		}
		coerce_hard = false;
	}

	/*
	 * Coerce elements to target type
	 *
	 * If the array has been explicitly cast, then the elements are in turn
	 * explicitly coerced.
	 *
	 * If the array's type was merely derived from the common type of its
	 * elements, then the elements are implicitly coerced to the common type.
	 * This is consistent with other uses of select_common_type().
	 */
	foreach(element, newelems)
	{
		Node	   *e = (Node *) lfirst(element);
		Node	   *newe;

		if (coerce_hard)
		{
			newe = coerce_to_target_type(pstate, e,
										 exprType(e),
										 coerce_type,
										 typmod,
										 COERCION_EXPLICIT,
										 COERCE_EXPLICIT_CAST,
										 -1);
			if (newe == NULL)
				ereport(ERROR,
						(errcode(ERRCODE_CANNOT_COERCE),
						 errmsg("cannot cast type %s to %s",
								format_type_be(exprType(e)),
								format_type_be(coerce_type)),
						 parser_errposition(pstate, exprLocation(e))));
		}
		else
			newe = coerce_to_common_type(pstate, e,
										 coerce_type,
										 "ARRAY");
		newcoercedelems = lappend(newcoercedelems, newe);
	}

	newa->array_typeid = array_type;
	/* array_collid will be set by parse_collate.c */
	newa->element_typeid = element_type;
	newa->elements = newcoercedelems;
	newa->location = a->location;

	return (Node *) newa;
}

static Node *
transformRowExpr(ParseState *pstate, RowExpr *r, bool allowDefault)
{
	RowExpr    *newr;
	char		fname[16];
	int			fnum;

	newr = makeNode(RowExpr);

	/* Transform the field expressions */
	newr->args = transformExpressionList(pstate, r->args,
										 pstate->p_expr_kind, allowDefault);

	/* Disallow more columns than will fit in a tuple */
	if (list_length(newr->args) > MaxTupleAttributeNumber)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_COLUMNS),
				 errmsg("ROW expressions can have at most %d entries",
						MaxTupleAttributeNumber),
				 parser_errposition(pstate, r->location)));

	/* Barring later casting, we consider the type RECORD */
	newr->row_typeid = RECORDOID;
	newr->row_format = COERCE_IMPLICIT_CAST;

	/* ROW() has anonymous columns, so invent some field names */
	newr->colnames = NIL;
	for (fnum = 1; fnum <= list_length(newr->args); fnum++)
	{
		snprintf(fname, sizeof(fname), "f%d", fnum);
		newr->colnames = lappend(newr->colnames, makeString(pstrdup(fname)));
	}

	newr->location = r->location;

	return (Node *) newr;
}

static Node *
transformCoalesceExpr(ParseState *pstate, CoalesceExpr *c)
{
	CoalesceExpr *newc = makeNode(CoalesceExpr);
	Node	   *last_srf = pstate->p_last_srf;
	List	   *newargs = NIL;
	List	   *newcoercedargs = NIL;
	ListCell   *args;

	foreach(args, c->args)
	{
		Node	   *e = (Node *) lfirst(args);
		Node	   *newe;

		newe = transformExprRecurse(pstate, e);

		/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
		/* Preprocessing that is neither an operator nor a function */
		if (MYS_MODE == compatible_mode)
			introducer_convert_encoding_walker((Node *)newe, NULL);
		/* End - SRS-MYSQL-CHARSET-INTRODUCER */

		newargs = lappend(newargs, newe);
	}

	/* Begin - BUGID:0000536 */
	if (compatible_mode == PG_MODE)
		newc->coalescetype = select_common_type(pstate, newargs, "COALESCE", NULL);
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (compatible_mode == MYS_MODE)
		newc->coalescetype = select_common_type(pstate, newargs, "COALESCE", NULL);
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (compatible_mode == ORA_MODE)
	/* Begin - BUG#Z4592 */
	{
		/*
		 * Even in Oracle compatibility mode, the behavior of native Coalesce expression
		 * should not be broken. Currently, the side effects here break the behavior of
		 * native Coalesce, this error is fixed now.
		 *
		 * The logic of the return type of NVL and NVL2 expressions is different. return
		 * type of NVL is based on expr1, and return type of NVL2 is based on expr2.
		 * However, NVL2 does not check whether expr1 can be implicitly converted to expr2,
		 * but only checks the implicit conversion between expr2 and expr3.
		 */
		if (c->coalescetag == TAG_COALESCE)
			newc->coalescetype = select_common_type(pstate, newargs, "COALESCE", NULL);
		else if (c->coalescetag == TAG_NVL)
			newc->coalescetype = select_common_type_for_nvl(pstate, newargs, "NVL", NULL);
		else if (c->coalescetag == TAG_NVL2)
			newc->coalescetype = select_common_type_for_nvl2(pstate, newargs, "NVL2", NULL);
		else
			elog(ERROR, "invalid coalesce tag");
	}
	/* End - BUG#Z4592 */
	/* End - BUGID:0000536 */
	/* coalescecollid will be set by parse_collate.c */

	/* Convert arguments if necessary */
	foreach(args, newargs)
	{
		Node	   *e = (Node *) lfirst(args);
		Node	   *newe;
		/* Begin - BUG#Z4592 */
		char	   *who;	/* make error messages more user-friendly */

		who = c->coalescetag == TAG_COALESCE ? "COALESCE" : c->coalescetag == TAG_NVL ? "NVL" : "NVL2";

		/* expr1 of NVL2 no need convert */
		if (c->coalescetag == TAG_NVL2 && list_head(newargs) == args)
		{
			newcoercedargs = lappend(newcoercedargs, e);
			continue;
		}
		else
		/* End - BUG#Z4592 */
		{
			newe = coerce_to_common_type(pstate, e,
										 newc->coalescetype,
										 who /* BUG#Z4592 */);

			newcoercedargs = lappend(newcoercedargs, newe);
		}
	}

	/* if any subexpression contained a SRF, complain */
	if (pstate->p_last_srf != last_srf)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		/* translator: %s is name of a SQL construct, eg GROUP BY */
				 errmsg("set-returning functions are not allowed in %s",
						"COALESCE"),
				 errhint("You might be able to move the set-returning function into a LATERAL FROM item."),
				 parser_errposition(pstate,
									exprLocation(pstate->p_last_srf))));

	newc->args = newcoercedargs;
	newc->coalescetag = c->coalescetag;	/* BUG#Z4591 */
	newc->location = c->location;
	return (Node *) newc;
}

static Node *
transformMinMaxExpr(ParseState *pstate, MinMaxExpr *m)
{
	MinMaxExpr *newm = makeNode(MinMaxExpr);
	List	   *newargs = NIL;
	List	   *newcoercedargs = NIL;
	const char *funcname = (m->op == IS_GREATEST) ? "GREATEST" : "LEAST";
	ListCell   *args;

	newm->op = m->op;
	foreach(args, m->args)
	{
		Node	   *e = (Node *) lfirst(args);
		Node	   *newe;

		newe = transformExprRecurse(pstate, e);

		/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
		/* Preprocessing that is neither an operator nor a function */
		if (MYS_MODE == compatible_mode)
			introducer_convert_encoding_walker((Node *)newe, NULL);
		/* End - SRS-MYSQL-CHARSET-INTRODUCER */

		newargs = lappend(newargs, newe);
	}

	newm->minmaxtype = select_common_type(pstate, newargs, funcname, NULL);
	/* minmaxcollid and inputcollid will be set by parse_collate.c */

	/* Convert arguments if necessary */
	foreach(args, newargs)
	{
		Node	   *e = (Node *) lfirst(args);
		Node	   *newe;

		newe = coerce_to_common_type(pstate, e,
									 newm->minmaxtype,
									 funcname);
		newcoercedargs = lappend(newcoercedargs, newe);
	}

	newm->args = newcoercedargs;
	newm->location = m->location;
	return (Node *) newm;
}

static Node *
transformSQLValueFunction(ParseState *pstate, SQLValueFunction *svf)
{
	/*
	 * All we need to do is insert the correct result type and (where needed)
	 * validate the typmod, so we just modify the node in-place.
	 */
	switch (svf->op)
	{
		case SVFOP_CURRENT_DATE:
			svf->type = DATEOID;
			break;
		case SVFOP_CURRENT_TIME:
			svf->type = TIMETZOID;
			break;
		case SVFOP_CURRENT_TIME_N:
			svf->type = TIMETZOID;
			svf->typmod = anytime_typmod_check(true, svf->typmod);
			break;
		case SVFOP_CURRENT_TIMESTAMP:
			svf->type = TIMESTAMPTZOID;
			break;
		case SVFOP_CURRENT_TIMESTAMP_N:
			svf->type = TIMESTAMPTZOID;
			svf->typmod = anytimestamp_typmod_check(true, svf->typmod);
			break;
		case SVFOP_LOCALTIME:
			svf->type = TIMEOID;
			break;
		case SVFOP_LOCALTIME_N:
			svf->type = TIMEOID;
			svf->typmod = anytime_typmod_check(false, svf->typmod);
			break;
		case SVFOP_LOCALTIMESTAMP:
			svf->type = TIMESTAMPOID;
			break;
		case SVFOP_LOCALTIMESTAMP_N:
			svf->type = TIMESTAMPOID;
			svf->typmod = anytimestamp_typmod_check(false, svf->typmod);
			break;
		case SVFOP_CURRENT_ROLE:
		case SVFOP_CURRENT_USER:
		case SVFOP_USER:
		case SVFOP_SESSION_USER:
		case SVFOP_CURRENT_CATALOG:
		case SVFOP_CURRENT_SCHEMA:
			svf->type = NAMEOID;
			break;
	}

	return (Node *) svf;
}

static Node *
transformXmlExpr(ParseState *pstate, XmlExpr *x)
{
	XmlExpr    *newx;
	ListCell   *lc;
	int			i;

	newx = makeNode(XmlExpr);
	newx->op = x->op;
	if (x->name)
		newx->name = map_sql_identifier_to_xml_name(x->name, false, false);
	else
		newx->name = NULL;
	newx->xmloption = x->xmloption;
	newx->type = XMLOID;		/* this just marks the node as transformed */
	newx->typmod = -1;
	newx->location = x->location;

	/*
	 * gram.y built the named args as a list of ResTarget.  Transform each,
	 * and break the names out as a separate list.
	 */
	newx->named_args = NIL;
	newx->arg_names = NIL;

	foreach(lc, x->named_args)
	{
		ResTarget  *r = lfirst_node(ResTarget, lc);
		Node	   *expr;
		char	   *argname;

		expr = transformExprRecurse(pstate, r->val);

		if (r->name)
			argname = map_sql_identifier_to_xml_name(r->name, false, false);
		else if (IsA(r->val, ColumnRef) || IsA(r->val, ColumnRefOrFuncCall))	/* ReqID:SRS-SQL-PROC */
			argname = map_sql_identifier_to_xml_name(FigureColname(r->val),
													 true, false);
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 x->op == IS_XMLELEMENT
					 ? errmsg("unnamed XML attribute value must be a column reference")
					 : errmsg("unnamed XML element value must be a column reference"),
					 parser_errposition(pstate, r->location)));
			argname = NULL;		/* keep compiler quiet */
		}

		/* reject duplicate argnames in XMLELEMENT only */
		if (x->op == IS_XMLELEMENT)
		{
			ListCell   *lc2;

			foreach(lc2, newx->arg_names)
			{
				if (strcmp(argname, strVal(lfirst(lc2))) == 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("XML attribute name \"%s\" appears more than once",
									argname),
							 parser_errposition(pstate, r->location)));
			}
		}

		newx->named_args = lappend(newx->named_args, expr);
		newx->arg_names = lappend(newx->arg_names, makeString(argname));
	}

	/* The other arguments are of varying types depending on the function */
	newx->args = NIL;
	i = 0;
	foreach(lc, x->args)
	{
		Node	   *e = (Node *) lfirst(lc);
		Node	   *newe;

		newe = transformExprRecurse(pstate, e);
		switch (x->op)
		{
			case IS_XMLCONCAT:
				newe = coerce_to_specific_type(pstate, newe, XMLOID,
											   "XMLCONCAT");
				break;
			case IS_XMLELEMENT:
				/* no coercion necessary */
				break;
			case IS_XMLFOREST:
				newe = coerce_to_specific_type(pstate, newe, XMLOID,
											   "XMLFOREST");
				break;
			case IS_XMLPARSE:
				if (i == 0)
					newe = coerce_to_specific_type(pstate, newe, TEXTOID,
												   "XMLPARSE");
				else
					newe = coerce_to_boolean(pstate, newe, "XMLPARSE");
				break;
			case IS_XMLPI:
				newe = coerce_to_specific_type(pstate, newe, TEXTOID,
											   "XMLPI");
				break;
			case IS_XMLROOT:
				if (i == 0)
					newe = coerce_to_specific_type(pstate, newe, XMLOID,
												   "XMLROOT");
				else if (i == 1)
					newe = coerce_to_specific_type(pstate, newe, TEXTOID,
												   "XMLROOT");
				else
					newe = coerce_to_specific_type(pstate, newe, INT4OID,
												   "XMLROOT");
				break;
			case IS_XMLSERIALIZE:
				/* not handled here */
				Assert(false);
				break;
			case IS_DOCUMENT:
				newe = coerce_to_specific_type(pstate, newe, XMLOID,
											   "IS DOCUMENT");
				break;
			/* Begin - ReqID:SRS-SQL-XML */
			case IS_UPDATEXML:
				newe = coerce_to_specific_type(pstate, newe, TEXTOID,
											   "UPDATEXML");
				break;
			/* End - ReqID:SRS-SQL-XML */
		}
		newx->args = lappend(newx->args, newe);
		i++;
	}

	return (Node *) newx;
}

static Node *
transformXmlSerialize(ParseState *pstate, XmlSerialize *xs)
{
	Node	   *result;
	XmlExpr    *xexpr;
	Oid			targetType;
	int32		targetTypmod;

	xexpr = makeNode(XmlExpr);
	xexpr->op = IS_XMLSERIALIZE;
	xexpr->args = list_make1(coerce_to_specific_type(pstate,
													 transformExprRecurse(pstate, xs->expr),
													 XMLOID,
													 "XMLSERIALIZE"));

	typenameTypeIdAndMod(pstate, xs->typeName, &targetType, &targetTypmod);

	xexpr->xmloption = xs->xmloption;
	xexpr->location = xs->location;
	/* We actually only need these to be able to parse back the expression. */
	xexpr->type = targetType;
	xexpr->typmod = targetTypmod;

	/*
	 * The actual target type is determined this way.  SQL allows char and
	 * varchar as target types.  We allow anything that can be cast implicitly
	 * from text.  This way, user-defined text-like data types automatically
	 * fit in.
	 */
	result = coerce_to_target_type(pstate, (Node *) xexpr,
								   TEXTOID, targetType, targetTypmod,
								   COERCION_IMPLICIT,
								   COERCE_IMPLICIT_CAST,
								   -1);
	if (result == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg("cannot cast XMLSERIALIZE result to %s",
						format_type_be(targetType)),
				 parser_errposition(pstate, xexpr->location)));
	return result;
}

static Node *
transformBooleanTest(ParseState *pstate, BooleanTest *b)
{
	const char *clausename;

	switch (b->booltesttype)
	{
		case IS_TRUE:
			clausename = "IS TRUE";
			break;
		case IS_NOT_TRUE:
			clausename = "IS NOT TRUE";
			break;
		case IS_FALSE:
			clausename = "IS FALSE";
			break;
		case IS_NOT_FALSE:
			clausename = "IS NOT FALSE";
			break;
		case IS_UNKNOWN:
			clausename = "IS UNKNOWN";
			break;
		case IS_NOT_UNKNOWN:
			clausename = "IS NOT UNKNOWN";
			break;
		default:
			elog(ERROR, "unrecognized booltesttype: %d",
				 (int) b->booltesttype);
			clausename = NULL;	/* keep compiler quiet */
	}

	b->arg = (Expr *) transformExprRecurse(pstate, (Node *) b->arg);

	b->arg = (Expr *) coerce_to_boolean(pstate,
										(Node *) b->arg,
										clausename);

	return (Node *) b;
}

static Node *
transformCurrentOfExpr(ParseState *pstate, CurrentOfExpr *cexpr)
{
	/* CURRENT OF can only appear at top level of UPDATE/DELETE */
	Assert(pstate->p_target_nsitem != NULL);
	cexpr->cvarno = pstate->p_target_nsitem->p_rtindex;

	/*
	 * Check to see if the cursor name matches a parameter of type REFCURSOR.
	 * If so, replace the raw name reference with a parameter reference. (This
	 * is a hack for the convenience of plpgsql.)
	 */
	if (cexpr->cursor_name != NULL) /* in case already transformed */
	{
		ColumnRef  *cref = makeNode(ColumnRef);
		Node	   *node = NULL;

		/* Build an unqualified ColumnRef with the given name */
		cref->fields = list_make1(makeString(cexpr->cursor_name));
		cref->location = -1;

		/* See if there is a translation available from a parser hook */
		if (pstate->p_pre_columnref_hook != NULL)
			node = pstate->p_pre_columnref_hook(pstate, cref);
		if (node == NULL && pstate->p_post_columnref_hook != NULL)
			node = pstate->p_post_columnref_hook(pstate, cref, NULL);

		/*
		 * XXX Should we throw an error if we get a translation that isn't a
		 * refcursor Param?  For now it seems best to silently ignore false
		 * matches.
		 */
		if (node != NULL && IsA(node, Param))
		{
			Param	   *p = (Param *) node;

			if (p->paramkind == PARAM_EXTERN &&
				ISCURSOROID(p->paramtype)) /* ReqID:SRS-PLSQL-CURSOR */
			{
				/* Matches, so convert CURRENT OF to a param reference */
				cexpr->cursor_name = NULL;
				cexpr->cursor_param = p->paramid;
			}
		}
	}

	return (Node *) cexpr;
}

/*
 * Construct a whole-row reference to represent the notation "relation.*".
 */
static Node *
transformWholeRowRef(ParseState *pstate, ParseNamespaceItem *nsitem,
					 int sublevels_up, int location)
{
	/*
	 * Build the appropriate referencing node.  Normally this can be a
	 * whole-row Var, but if the nsitem is a JOIN USING alias then it contains
	 * only a subset of the columns of the underlying join RTE, so that will
	 * not work.  Instead we immediately expand the reference into a RowExpr.
	 * Since the JOIN USING's common columns are fully determined at this
	 * point, there seems no harm in expanding it now rather than during
	 * planning.
	 *
	 * Note that if the RTE is a function returning scalar, we create just a
	 * plain reference to the function value, not a composite containing a
	 * single column.  This is pretty inconsistent at first sight, but it's
	 * what we've done historically.  One argument for it is that "rel" and
	 * "rel.*" mean the same thing for composite relations, so why not for
	 * scalar functions...
	 */
	if (nsitem->p_names == nsitem->p_rte->eref)
	{
		Var		   *result;

		result = makeWholeRowVar(nsitem->p_rte, nsitem->p_rtindex,
								 sublevels_up, true);

		/* location is not filled in by makeWholeRowVar */
		result->location = location;

		/* mark relation as requiring whole-row SELECT access */
		markVarForSelectPriv(pstate, result);

		return (Node *) result;
	}
	else
	{
		RowExpr    *rowexpr;
		List	   *fields;

		/*
		 * We want only as many columns as are listed in p_names->colnames,
		 * and we should use those names not whatever possibly-aliased names
		 * are in the RTE.  We needn't worry about marking the RTE for SELECT
		 * access, as the common columns are surely so marked already.
		 */
		expandRTE(nsitem->p_rte, nsitem->p_rtindex,
				  sublevels_up, location, false,
				  NULL, &fields);
		rowexpr = makeNode(RowExpr);
		rowexpr->args = list_truncate(fields,
									  list_length(nsitem->p_names->colnames));
		rowexpr->row_typeid = RECORDOID;
		rowexpr->row_format = COERCE_IMPLICIT_CAST;
		rowexpr->colnames = copyObject(nsitem->p_names->colnames);
		rowexpr->location = location;

		return (Node *) rowexpr;
	}
}

/*
 * Handle an explicit CAST construct.
 *
 * Transform the argument, look up the type name, and apply any necessary
 * coercion function(s).
 */
static Node *
transformTypeCast(ParseState *pstate, TypeCast *tc)
{
	Node	   *result;
	Node	   *arg = tc->arg;
	Node	   *expr;
	Oid			inputType;
	Oid			targetType;
	int32		targetTypmod;
	int			location;

	/* Look up the type name first */
	typenameTypeIdAndMod(pstate, tc->typeName, &targetType, &targetTypmod);

	/** 
	 * Begin - BUG#M533 
	 * when cast hex string to other data, we parse it to uint64 or
	 * varbinary according to target type.
	 */
	if (MYS_MODE == compatible_mode)
	{
		if (cast_to_char && targetType == BPCHAROID)
		{
			int32		tmptargetTypmod = -1;

			if (targetTypmod != VARHDRSZ)
			{
				/* when the typemod is -2, represent the type is char or nchar,
				 * not type mode. when the typemod is not -2, represent the type
				 * is char(n) or nchar(n). when the type is char or nchar, if the
				 * arg is string, the type mode is the string length, when then arg is expr,
				 * the type mode is -1;
				 */
				arg = tc->arg;
				if (IsA(arg, A_Const))
				{
					A_Const *aconst = (A_Const *)arg;

					if (aconst->val.type == T_String && aconst->val.val.str != NULL)
						tmptargetTypmod = DatumGetInt32(DirectFunctionCall1(textlen,
																			CStringGetTextDatum(aconst->val.val.str)));
				}

				/* when the typemod is -2, the type is char or nchar,
				 * type mode is charactor lenth;
				 * when the typemod is not -2, the type is char(n) or nchar(n),
				 * type mode is type mode - header;
				 */
				if (tc->typeName->typemod != -2)
					tmptargetTypmod = targetTypmod - VARHDRSZ;

				tc->typeName->names = list_make2(makeString("pg_catalog"), makeString("varchar"));
				tc->typeName->typmods = list_make1(mysmakeIntConst(tmptargetTypmod, -1));
				targetTypmod = 0;
				targetType = InvalidOid;
				typenameTypeIdAndMod(pstate, tc->typeName, &targetType, &targetTypmod);
			}
			/* Begin - BUG#Z00005108 */
			else
			{
				A_Const *n = makeNode(A_Const);

				n->val.type = T_String;
				n->val.val.str = "";
				n->location = -1;
				tc->arg = (Node *)n;
				arg = tc->arg;
				ereport(WARNING,
						(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
						 errmsg("value too long for type char(0), Data truncated.")));
			}
			/* End - BUG#Z00005108 */
		}
	}
	/* End - BUG#M533 */

	/*
	 * If the subject of the typecast is an ARRAY[] construct and the target
	 * type is an array type, we invoke transformArrayExpr() directly so that
	 * we can pass down the type information.  This avoids some cases where
	 * transformArrayExpr() might not infer the correct type.  Otherwise, just
	 * transform the argument normally.
	 */
	if (IsA(arg, A_ArrayExpr))
	{
		Oid			targetBaseType;
		int32		targetBaseTypmod;
		Oid			elementType;

		/*
		 * If target is a domain over array, work with the base array type
		 * here.  Below, we'll cast the array type to the domain.  In the
		 * usual case that the target is not a domain, the remaining steps
		 * will be a no-op.
		 */
		targetBaseTypmod = targetTypmod;
		targetBaseType = getBaseTypeAndTypmod(targetType, &targetBaseTypmod);
		elementType = get_element_type(targetBaseType);
		if (OidIsValid(elementType))
		{
			expr = transformArrayExpr(pstate,
									  (A_ArrayExpr *) arg,
									  targetBaseType,
									  elementType,
									  targetBaseTypmod);
		}
		else
			expr = transformExprRecurse(pstate, arg);
	}
	else
		expr = transformExprRecurse(pstate, arg);

	inputType = exprType(expr);
	if (inputType == InvalidOid)
		return expr;			/* do nothing if NULL input */

	/*
	 * Location of the coercion is preferentially the location of the :: or
	 * CAST symbol, but if there is none then use the location of the type
	 * name (this can happen in TypeName 'string' syntax, for instance).
	 */
	location = tc->location;
	if (location < 0)
		location = tc->typeName->location;

	result = coerce_to_target_type(pstate, expr, inputType,
								   targetType, targetTypmod,
								   COERCION_EXPLICIT,
								   COERCE_EXPLICIT_CAST,
								   location);
	/* Begin - ReqID:SRS-SQL-CustomObject */
	if (result == NULL &&
		ORA_MODE == compatible_mode &&
		object_type_is_subtype_of(inputType, targetType))
	{
		ConvertRowtypeExpr *r = makeNode(ConvertRowtypeExpr);

		r->arg = (Expr *) expr;
		r->resulttype = targetType;
		r->convertformat = COERCE_EXPLICIT_CAST;
		r->location = location;
		result = (Node *) r;
	}
	if (result == NULL)
	{
		if (ORA_MODE == compatible_mode &&
			is_object_type_typid(inputType) &&
			is_object_type_typid(targetType))
			ereport(ERROR,
					(errcode(ERRCODE_CANNOT_COERCE),
					 errmsg("object type %s is not in the parent type chain of %s",
							format_type_be(targetType),
							format_type_be(inputType)),
					 parser_coercion_errposition(pstate, location, expr)));

		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg("cannot cast type %s to %s",
						format_type_be(inputType),
						format_type_be(targetType)),
				 parser_coercion_errposition(pstate, location, expr)));
	}
	/* End - ReqID:SRS-SQL-CustomObject */

	return result;
}

/* Begin - ReqID:SRS-SQL-CustomObject */
static bool
is_object_type_typid(Oid typid)
{
	return OidIsValid(GetSysCacheOid1(OBJTYPETYPOID,
									  Anum_pg_object_type_oid,
									  ObjectIdGetDatum(typid)));
}

static bool
object_type_is_subtype_of(Oid subtypeTypid, Oid targetTypid)
{
	Oid			currentObjoid;
	Oid			targetObjoid;

	if (!OidIsValid(subtypeTypid) || !OidIsValid(targetTypid) ||
		subtypeTypid == targetTypid)
		return false;

	currentObjoid = GetSysCacheOid1(OBJTYPETYPOID,
									Anum_pg_object_type_oid,
									ObjectIdGetDatum(subtypeTypid));
	targetObjoid = GetSysCacheOid1(OBJTYPETYPOID,
								   Anum_pg_object_type_oid,
								   ObjectIdGetDatum(targetTypid));
	if (!OidIsValid(currentObjoid) || !OidIsValid(targetObjoid))
		return false;

	while (OidIsValid(currentObjoid))
	{
		HeapTuple	tup;
		Oid			parenttypid;

		tup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(currentObjoid));
		if (!HeapTupleIsValid(tup))
			break;

		parenttypid = ((Form_pg_object_type) GETSTRUCT(tup))->parenttypid;
		ReleaseSysCache(tup);

		if (!OidIsValid(parenttypid))
			break;
		if (parenttypid == targetTypid)
			return true;

		currentObjoid = GetSysCacheOid1(OBJTYPETYPOID,
										Anum_pg_object_type_oid,
										ObjectIdGetDatum(parenttypid));
	}

	return false;
}
/* End - ReqID:SRS-SQL-CustomObject */

/*
 * Handle an explicit COLLATE clause.
 *
 * Transform the argument, and look up the collation name.
 */
static Node *
transformCollateClause(ParseState *pstate, CollateClause *c)
{
	CollateExpr *newc;
	Oid			argtype;

	newc = makeNode(CollateExpr);
	newc->arg = (Expr *) transformExprRecurse(pstate, c->arg);

	argtype = exprType((Node *) newc->arg);

	/* Begin - BUG#8428 8441 */
	if (compatible_mode == MYS_MODE)
	{
		/*
		 * MySQL supports adding collate to types that don't have typecollate,
		 * but it operates in the form of strings.
		 */
		if (!type_is_collatable(argtype))
		{
			/*
			 * If it's not a string type and needs binary collation,
			 * then convert it to varbinary type
			 */
			if (pg_strcasecmp(strVal(llast(c->collname)), "binary") == 0)
			{
				return (Node *)coerce_type(pstate,
										   (Node *)newc->arg,
										   argtype,
										   VARBINARYOID,
										   -1,
										   COERCION_EXPLICIT,
										   COERCE_EXPLICIT_CAST,
										   c->location);
			}
			/*
			 * If it's not a string type and needs a non-binary collation,
			 * convert it to varchar type
			 */
			else
			{
				newc->arg = (Expr *)coerce_type(pstate,
												(Node *)newc->arg,
												exprType((Node *)newc->arg),
												VARCHAROID,
												-1,
												COERCION_EXPLICIT,
												COERCE_EXPLICIT_CAST,
												c->location);
			}

			argtype = exprType((Node *)newc->arg);
		}
	}
	/* End - BUG#8428 8441 */

	/*
	 * The unknown type is not collatable, but coerce_type() takes care of it
	 * separately, so we'll let it go here.
	 */
	if (!type_is_collatable(argtype) && argtype != UNKNOWNOID)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("collations are not supported by type %s",
						format_type_be(argtype)),
				 parser_errposition(pstate, c->location)));

	newc->collOid = LookupCollation(pstate, c->collname, c->location);
	newc->location = c->location;

	return (Node *) newc;
}

/* Begin - BUG#M0000376 */
void
transformFuncArg(ParseState *pstate, Node *func_arg)
{
	/* Begin - OA-REQ-2454 */
	ParseExprKind	sv_expr_kind;

	/* Save and restore identity of expression type we're parsing */
	sv_expr_kind = pstate->p_expr_kind;
	if (pstate->p_expr_kind == EXPR_KIND_NONE)
		pstate->p_expr_kind = EXPR_KIND_CALL_ARGUMENT;

	transformExprRecurse(pstate, func_arg);

	pstate->p_expr_kind = sv_expr_kind;
	/* End - OA-REQ-2454 */
}
/* End - BUG#M0000376 */

/*
 * Transform a "row compare-op row" construct
 *
 * The inputs are lists of already-transformed expressions.
 * As with coerce_type, pstate may be NULL if no special unknown-Param
 * processing is wanted.
 *
 * The output may be a single OpExpr, an AND or OR combination of OpExprs,
 * or a RowCompareExpr.  In all cases it is guaranteed to return boolean.
 * The AND, OR, and RowCompareExpr cases further imply things about the
 * behavior of the operators (ie, they behave as =, <>, or < <= > >=).
 */
static Node *
make_row_comparison_op(ParseState *pstate, List *opname,
					   List *largs, List *rargs, int location)
{
	RowCompareExpr *rcexpr;
	RowCompareType rctype;
	List	   *opexprs;
	List	   *opnos;
	List	   *opfamilies;
	ListCell   *l,
			   *r;
	List	  **opinfo_lists;
	Bitmapset  *strats;
	int			nopers;
	int			i;

	nopers = list_length(largs);
	if (nopers != list_length(rargs))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unequal number of entries in row expressions"),
				 parser_errposition(pstate, location)));

	/*
	 * We can't compare zero-length rows because there is no principled basis
	 * for figuring out what the operator is.
	 */
	if (nopers == 0)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot compare rows of zero length"),
				 parser_errposition(pstate, location)));

	/*
	 * Identify all the pairwise operators, using make_op so that behavior is
	 * the same as in the simple scalar case.
	 */
	opexprs = NIL;
	forboth(l, largs, r, rargs)
	{
		Node	   *larg = (Node *) lfirst(l);
		Node	   *rarg = (Node *) lfirst(r);
		OpExpr	   *cmp;

		cmp = castNode(OpExpr, make_op_ext(pstate, opname, larg, rarg,
										   pstate->p_last_srf, false, location, ROWCOMPARISONOP));	/* ReqID:SRS-SQL-OUTERJOIN(+) */ /* SRS-MISC-COMPATDM-SQL */

		/*
		 * We don't use coerce_to_boolean here because we insist on the
		 * operator yielding boolean directly, not via coercion.  If it
		 * doesn't yield bool it won't be in any index opfamilies...
		 */
		if (cmp->opresulttype != BOOLOID)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("row comparison operator must yield type boolean, "
							"not type %s",
							format_type_be(cmp->opresulttype)),
					 parser_errposition(pstate, location)));
		if (expression_returns_set((Node *) cmp))
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("row comparison operator must not return a set"),
					 parser_errposition(pstate, location)));
		opexprs = lappend(opexprs, cmp);
	}

	/*
	 * If rows are length 1, just return the single operator.  In this case we
	 * don't insist on identifying btree semantics for the operator (but we
	 * still require it to return boolean).
	 */
	if (nopers == 1)
		return (Node *) linitial(opexprs);

	/*
	 * Now we must determine which row comparison semantics (= <> < <= > >=)
	 * apply to this set of operators.  We look for btree opfamilies
	 * containing the operators, and see which interpretations (strategy
	 * numbers) exist for each operator.
	 */
	opinfo_lists = (List **) palloc(nopers * sizeof(List *));
	strats = NULL;
	i = 0;
	foreach(l, opexprs)
	{
		Oid			opno = ((OpExpr *) lfirst(l))->opno;
		Bitmapset  *this_strats;
		ListCell   *j;

		opinfo_lists[i] = get_op_btree_interpretation(opno);

		/*
		 * convert strategy numbers into a Bitmapset to make the intersection
		 * calculation easy.
		 */
		this_strats = NULL;
		foreach(j, opinfo_lists[i])
		{
			OpBtreeInterpretation *opinfo = lfirst(j);

			this_strats = bms_add_member(this_strats, opinfo->strategy);
		}
		if (i == 0)
			strats = this_strats;
		else
			strats = bms_int_members(strats, this_strats);
		i++;
	}

	/*
	 * If there are multiple common interpretations, we may use any one of
	 * them ... this coding arbitrarily picks the lowest btree strategy
	 * number.
	 */
	i = bms_first_member(strats);
	if (i < 0)
	{
		/* No common interpretation, so fail */
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not determine interpretation of row comparison operator %s",
						strVal(llast(opname))),
				 errhint("Row comparison operators must be associated with btree operator families."),
				 parser_errposition(pstate, location)));
	}
	rctype = (RowCompareType) i;

	/*
	 * For = and <> cases, we just combine the pairwise operators with AND or
	 * OR respectively.
	 */
	if (rctype == ROWCOMPARE_EQ)
		return (Node *) makeBoolExpr(AND_EXPR, opexprs, location);
	if (rctype == ROWCOMPARE_NE)
		return (Node *) makeBoolExpr(OR_EXPR, opexprs, location);

	/*
	 * Otherwise we need to choose exactly which opfamily to associate with
	 * each operator.
	 */
	opfamilies = NIL;
	for (i = 0; i < nopers; i++)
	{
		Oid			opfamily = InvalidOid;
		ListCell   *j;

		foreach(j, opinfo_lists[i])
		{
			OpBtreeInterpretation *opinfo = lfirst(j);

			if (opinfo->strategy == rctype)
			{
				opfamily = opinfo->opfamily_id;
				break;
			}
		}
		if (OidIsValid(opfamily))
			opfamilies = lappend_oid(opfamilies, opfamily);
		else					/* should not happen */
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("could not determine interpretation of row comparison operator %s",
							strVal(llast(opname))),
					 errdetail("There are multiple equally-plausible candidates."),
					 parser_errposition(pstate, location)));
	}

	/*
	 * Now deconstruct the OpExprs and create a RowCompareExpr.
	 *
	 * Note: can't just reuse the passed largs/rargs lists, because of
	 * possibility that make_op inserted coercion operations.
	 */
	opnos = NIL;
	largs = NIL;
	rargs = NIL;
	foreach(l, opexprs)
	{
		OpExpr	   *cmp = (OpExpr *) lfirst(l);

		opnos = lappend_oid(opnos, cmp->opno);
		largs = lappend(largs, linitial(cmp->args));
		rargs = lappend(rargs, lsecond(cmp->args));
	}

	rcexpr = makeNode(RowCompareExpr);
	rcexpr->rctype = rctype;
	rcexpr->opnos = opnos;
	rcexpr->opfamilies = opfamilies;
	rcexpr->inputcollids = NIL; /* assign_expr_collations will fix this */
	rcexpr->largs = largs;
	rcexpr->rargs = rargs;

	return (Node *) rcexpr;
}

/*
 * Transform a "row IS DISTINCT FROM row" construct
 *
 * The input RowExprs are already transformed
 */
static Node *
make_row_distinct_op(ParseState *pstate, List *opname,
					 RowExpr *lrow, RowExpr *rrow,
					 int location)
{
	Node	   *result = NULL;
	List	   *largs = lrow->args;
	List	   *rargs = rrow->args;
	ListCell   *l,
			   *r;

	if (list_length(largs) != list_length(rargs))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unequal number of entries in row expressions"),
				 parser_errposition(pstate, location)));

	forboth(l, largs, r, rargs)
	{
		Node	   *larg = (Node *) lfirst(l);
		Node	   *rarg = (Node *) lfirst(r);
		Node	   *cmp;

		cmp = (Node *) make_distinct_op(pstate, opname, larg, rarg, location);
		if (result == NULL)
			result = cmp;
		else
			result = (Node *) makeBoolExpr(OR_EXPR,
										   list_make2(result, cmp),
										   location);
	}

	if (result == NULL)
	{
		/* zero-length rows?  Generate constant FALSE */
		result = makeBoolConst(false, false);
	}

	return result;
}

/*
 * make the node for an IS DISTINCT FROM operator
 */
static Expr *
make_distinct_op(ParseState *pstate, List *opname, Node *ltree, Node *rtree,
				 int location)
{
	Expr	   *result;

	result = make_op_ext(pstate, opname, ltree, rtree,
						 pstate->p_last_srf, false, location, DISTINCTOP);	/* ReqID:SRS-SQL-OUTERJOIN(+) */	/* SRS-MISC-COMPATDM-SQL */
	if (((OpExpr *) result)->opresulttype != BOOLOID)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("IS DISTINCT FROM requires = operator to yield boolean"),
				 parser_errposition(pstate, location)));
	if (((OpExpr *) result)->opretset)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
		/* translator: %s is name of a SQL construct, eg NULLIF */
				 errmsg("%s must not return a set", "IS DISTINCT FROM"),
				 parser_errposition(pstate, location)));

	/*
	 * We rely on DistinctExpr and OpExpr being same struct
	 */
	NodeSetTag(result, T_DistinctExpr);

	return result;
}

/*
 * Produce a NullTest node from an IS [NOT] DISTINCT FROM NULL construct
 *
 * "arg" is the untransformed other argument
 */
static Node *
make_nulltest_from_distinct(ParseState *pstate, A_Expr *distincta, Node *arg)
{
	NullTest   *nt = makeNode(NullTest);

	nt->arg = (Expr *) transformExprRecurse(pstate, arg);
	/* the argument can be any type, so don't coerce it */
	if (distincta->kind == AEXPR_NOT_DISTINCT)
		nt->nulltesttype = IS_NULL;
	else
		nt->nulltesttype = IS_NOT_NULL;
	/* argisrow = false is correct whether or not arg is composite */
	nt->argisrow = false;
	nt->location = distincta->location;
	return (Node *) nt;
}

/*
 * Produce a string identifying an expression by kind.
 *
 * Note: when practical, use a simple SQL keyword for the result.  If that
 * doesn't work well, check call sites to see whether custom error message
 * strings are required.
 */
const char *
ParseExprKindName(ParseExprKind exprKind)
{
	switch (exprKind)
	{
		case EXPR_KIND_NONE:
			return "invalid expression context";
		case EXPR_KIND_OTHER:
			return "extension expression";
		case EXPR_KIND_JOIN_ON:
			return "JOIN/ON";
		case EXPR_KIND_JOIN_USING:
			return "JOIN/USING";
		case EXPR_KIND_FROM_SUBSELECT:
			return "sub-SELECT in FROM";
		case EXPR_KIND_FROM_FUNCTION:
			return "function in FROM";
		case EXPR_KIND_WHERE:
			return "WHERE";
		case EXPR_KIND_POLICY:
			return "POLICY";
		case EXPR_KIND_HAVING:
			return "HAVING";
		case EXPR_KIND_FILTER:
			return "FILTER";
		case EXPR_KIND_WINDOW_PARTITION:
			return "window PARTITION BY";
		case EXPR_KIND_WINDOW_ORDER:
			return "window ORDER BY";
		case EXPR_KIND_WINDOW_FRAME_RANGE:
			return "window RANGE";
		case EXPR_KIND_WINDOW_FRAME_ROWS:
			return "window ROWS";
		case EXPR_KIND_WINDOW_FRAME_GROUPS:
			return "window GROUPS";
		case EXPR_KIND_SELECT_TARGET:
			return "SELECT";
		case EXPR_KIND_INSERT_TARGET:
			return "INSERT";
		case EXPR_KIND_UPDATE_SOURCE:
		case EXPR_KIND_UPDATE_TARGET:
			return "UPDATE";
		case EXPR_KIND_MERGE_WHEN:
			return "MERGE WHEN";
		case EXPR_KIND_GROUP_BY:
			return "GROUP BY";
		case EXPR_KIND_ORDER_BY:
			return "ORDER BY";
		case EXPR_KIND_DISTINCT_ON:
			return "DISTINCT ON";
		case EXPR_KIND_LIMIT:
			return "LIMIT";
		case EXPR_KIND_OFFSET:
			return "OFFSET";
		case EXPR_KIND_RETURNING:
			return "RETURNING";
		case EXPR_KIND_VALUES:
		case EXPR_KIND_VALUES_SINGLE:
			return "VALUES";
		case EXPR_KIND_CHECK_CONSTRAINT:
		case EXPR_KIND_DOMAIN_CHECK:
			return "CHECK";
		case EXPR_KIND_COLUMN_DEFAULT:
		case EXPR_KIND_FUNCTION_DEFAULT:
			return "DEFAULT";
		case EXPR_KIND_INDEX_EXPRESSION:
			return "index expression";
		case EXPR_KIND_INDEX_PREDICATE:
			return "index predicate";
		case EXPR_KIND_STATS_EXPRESSION:
			return "statistics expression";
		case EXPR_KIND_ALTER_COL_TRANSFORM:
			return "USING";
		case EXPR_KIND_EXECUTE_PARAMETER:
			return "EXECUTE";
		case EXPR_KIND_TRIGGER_WHEN:
			return "WHEN";
		case EXPR_KIND_PARTITION_BOUND:
			return "partition bound";
		case EXPR_KIND_PARTITION_EXPRESSION:
			return "PARTITION BY";
		case EXPR_KIND_CALL_ARGUMENT:
			return "CALL";
		case EXPR_KIND_COPY_WHERE:
			return "WHERE";
		case EXPR_KIND_GENERATED_COLUMN:
			return "GENERATED AS";
		case EXPR_KIND_CYCLE_MARK:
			return "CYCLE";
		/* Begin: Req:SRS-SQL-NLSSORT */
		case EXPR_KIND_GROUP_BY_DISTINCT:
			return "distinct ORDER BY";
		/* End: Req:SRS-SQL-NLSSORT */
		/* Begin - ReqID:SRS-RECOVERY-FLASHBACK */
		case EXPR_KIND_ASOF:
			return "AS OF";
		/* End - ReqID:SRS-RECOVERY-FLASHBACK */
		/* Begin - ReqID:SRS-RECOVERY-FLASHBACK */
		case EXPR_KIND_VERSIONS_BETWEEN:
			return "VERSIONS BETWEEN";
		/* End - ReqID:SRS-RECOVERY-FLASHBACK */
		/* Begin - ReqID:SRS-MYSQL-SQL-Select */
		case EXPR_KIND_HAVING_FUNCTION:
			return "HAVING FUNCTION()";
		/* End - ReqID:SRS-MYSQL-SQL-Select */
			/*
			 * There is intentionally no default: case here, so that the
			 * compiler will warn if we add a new ParseExprKind without
			 * extending this switch.  If we do see an unrecognized value at
			 * runtime, we'll fall through to the "unrecognized" return.
			 */
	}
	return "unrecognized expression kind";
}

/* Begin - BUG#M533 */
static Node *
mysmakeIntConst(int val, int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Integer;
	n->val.val.ival = val;
	n->location = location;

	return (Node *)n;
}
/* End - BUG#M533 */

/* Begin - ReqID:SRS-MYSQL-SET */
/* transformVarAssign
 *
 * transform VarAssign node to function node.
 */
Node *
transformVarAssign(ParseState *pstate, VarAssign *var, bool returnquery)
{
	List		   *funcName;
	List		   *argList;
	Node		   *funcCall;
	char		   *funcname = NULL;
	Node		   *node = var->value;
	Node		   *retnode;
	Query		   *result;

	/*
	 * If the value is a SELECT node,
	 * it needs to be transformed.
	 */
	if (nodeTag(node) == T_SelectStmt)
	{
		node = (Node *)transformStmt(pstate, node);
	}

	if (var->setkind == SET_GLOBALVAR)
		funcname = "mys_set_global_variable";
	else if (var->setkind == SET_SESSIONVAR)
		funcname = "mys_set_session_variable";
	else if (var->setkind == SET_LOCALVAR)
		funcname = "mys_set_local_variable";
	else if (var->setkind == SET_USERVAR)
	{
		if (var->ret == RET_VALUE)
		{
			funcname = "mys_set_user_var_in_expr";

			/*
			 * if set user variable in expr, such as:
			 * SELECT @var := value; if the value is a string,
			 * need to change the type to varchar(n), n is
			 * string length.
			 */
			if (nodeTag(var->value) == T_A_Const && ((A_Const *)var->value)->val.type == T_String)
			{
				int stringlen;
				TypeName *typname;
				A_Const *aconst = (A_Const *)var->value;

				stringlen = strlen(aconst->val.val.str);
				typname = SystemTypeName("varchar");
				typname->typmods = list_make1(ivy_makeIntConst(stringlen, var->location));
				typname->location = var->location;
				typname->typemod = stringlen;
				node = ivy_makeTypeCast(node, typname, var->location);
			}
		}
		else
		{
			funcname = "mys_set_user_var";

			/*
			 * if set user variable in SET, such as:
			 * SET @var = value; if the value is a string,
			 * need to change the type to longtext.
			 */
			if (nodeTag(var->value) == T_A_Const && ((A_Const *)var->value)->val.type == T_String)
			{
				TypeName *typname;

				typname = makeTypeNameFromNameList(list_make2(makeString(pstrdup("ivymy")),
													   makeString(pstrdup("longtext"))));
				typname->typmods = NIL;
				typname->location = var->location;
				node = ivy_makeTypeCast(node, typname, var->location);

			}
		}
	}

	/* Construct the function and it arguments list */
	funcName = list_make2(makeString(pstrdup("ivymy")),
						  makeString(pstrdup(funcname)));
	argList = NIL;
	if (IsA(var->value, SetToDefault))
		argList = list_make2(ivy_makeStringConst(var->varname,var->location), ivy_makeNullAConst(var->location));
	else
		argList = list_make2(ivy_makeStringConst(var->varname, var->location), node);

	if (var->setkind != SET_USERVAR)
	{
		if (IsA(var->value, SetToDefault))
			argList = lappend(argList, ivy_makeBoolAConst(true));
		else
			argList = lappend(argList, ivy_makeBoolAConst(false));
	}

	funcCall = (Node *) makeFuncCall(funcName, argList, COERCE_EXPLICIT_CALL, var->location);

	/* transform function node */
	retnode = transformExprRecurse(pstate, funcCall);

	/*
	 * If this is a SET statement, it needs
	 * to be built as a query node
	 */
	if (returnquery)
	{
		result = makeNode(Query);
		result->commandType = CMD_UTILITY;
		result->utilityStmt = retnode;

		return (Node *)result;
	}

	return retnode;
}

/* transformVarRef
 *
 * get the value of GUC parameter, and user defined variable;
 * if varref node is get user define variable, and the node in
 * SET command, get the current value, else return VarRef node.
 */
static Node *
transformVarRef(ParseState *pstate, VarRef *varref)
{
	Node		   *funcCall = NULL;
	A_Const		   *n = (A_Const *)ivy_makeStringConst(varref->varname, varref->location);

	/* prevent compilation loops */
	if (varref->compiled)
		return (Node *)varref;

	/* set a flag, it means VarRef node has been parsed. */
	pstate->p_hasuservar = true;

	if (varref->varkind == SYSTEM_VAR)
	{
		funcCall = (Node *)makeFuncCall(list_make2(makeString("pg_catalog"), makeString("current_setting")),
										 list_make1(n),
										 COERCE_EXPLICIT_CALL,
										 -1);
	}
	else if (varref->varkind == USER_VAR)
	{
		/* set a flag prevent infinite recursion when parsing VarRef node */
		varref->compiled = true;

		/* Get current of value of user define variable */
		varref->value = (Const *)uservariable_hash_search_hook(pstate, varref);

		varref->rettype = varref->value->consttype;

		/*
		 * if this Node appears in a SET statement, return it's value,
		 * otherwise, return the VarRef node itself. BUG#Z6860
		 */
		if (pstate->p_myssetvar)
			return (Node *)varref->value;

		return (Node *)varref;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("Undefined VarRef node")));
	}

	return transformExprRecurse(pstate, funcCall);
}
/* End - ReqID:SRS-MYSQL-SET */

/* Begin - BUG#7712 */
static Node *
transformOraRowExpr(ParseState *pstate, OraRowExpr *r, bool allowDefault)
{
	OraRowExpr  *newr;

	newr = makeNode(OraRowExpr);

	/* Transform the field expressions */
	newr->args = transformExpressionList(pstate, r->args,
										 pstate->p_expr_kind, allowDefault);

	/* Disallow more columns than will fit in a tuple */
	if (list_length(newr->args) > MaxTupleAttributeNumber)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_COLUMNS),
				 errmsg("ROW expressions can have at most %d entries",
						MaxTupleAttributeNumber),
				 parser_errposition(pstate, r->location)));

	/* Barring later casting, we consider the type ORARECORDOID */
	newr->row_typeid = ORARECORDOID;
	newr->row_format = COERCE_IMPLICIT_CAST;
	newr->type = r->type;

	newr->colnames = list_copy(r->colnames);
	newr->location = r->location;

	return (Node *) newr;
}
/* End - BUG#7712 */
