/*-------------------------------------------------------------------------
 *
 * ora_indexcond.c
 *	  Routines to process clause with ora_case_trans() function
 *
 * BUG#M0000302
 *
 *
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/optimizer/path/ora_indexcond.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_class.h"
#include "catalog/pg_collation_d.h"
#include "catalog/pg_namespace_d.h"
#include "catalog/pg_operator.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/ora_indexcond.h"
#include "parser/scansup.h"
#include "utils/arrayaccess.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"


#define SA_OpExpr_args(__nodeptr__) \
		(IsA((__nodeptr__), OpExpr) \
			? (((OpExpr *) (__nodeptr__))->args) \
			: (((ScalarArrayOpExpr *) (__nodeptr__))->args))

#define SA_OpExpr_opno(__nodeptr__) \
		(IsA((__nodeptr__), OpExpr) \
			? (((OpExpr *) (__nodeptr__))->opno) \
			: (((ScalarArrayOpExpr *) (__nodeptr__))->opno))


Oid		ora_indexcond_oct_oid = InvalidOid;


static bool is_var_inside(Node *node, void *context);
static Oid opr_left_right_internal(const char *oprname,
								   Oid oprleft,
								   Oid oprright);
static Oid opr_left_right(Node *leftop, Node *rightop, Oid origin_opno);
static void consttype_varchar2_to_text(Node *node);
static void pull_var_update_vardata(Node *clause,
									Node **lop,
									Node **rop,
									Node **inner_var);
static Node * update_ScalarArray_OpExpr(Node *node, Node *arg1, Node *arg2);


/*
 * take_oracasetrans_away
 *
 * ora_case_trans(col::varchar2)::varchar2() = 'xxx'
 * -> col = ora_case_trans('xxx')
 */
Node *
take_oracasetrans_away(Node *clause)
{
	Node   *lop = NULL,
		   *rop = NULL,
		   *l_var = NULL,
		   *r_var = NULL;

	/*
	 * 1. condition check
	 */
	if (clause == NULL)		/* should not happend */
		return clause;

	if (!IsA(clause, OpExpr) && !IsA(clause, ScalarArrayOpExpr))
		return clause;

	if (IsA(clause, OpExpr) && (((OpExpr *) clause)->opresulttype != BOOLOID ||
								list_length(((OpExpr *) clause)->args) != 2))
		return clause;

	if (IsA(clause, ScalarArrayOpExpr) &&
		list_length(((ScalarArrayOpExpr *) clause)->args) != 2)
		return clause;

	/* only handle operator "=" while identifier_case_switch is INTERCHANGE */
	if (identifier_case_switch == INTERCHANGE &&
		strcmp(get_opname(SA_OpExpr_opno(clause)), "=") != 0)
		return clause;

	/*
	 * 2. change the clause
	 */
	lop = (Node *) linitial(SA_OpExpr_args(clause));
	rop = (Node *) lsecond(SA_OpExpr_args(clause));

	if (!IsA(lop, RelabelType) && !IsA(rop, RelabelType) &&
		!IsA(lop, FuncExpr) && !IsA(rop, FuncExpr))
		return clause;

	/* whether args of OpExpr / ScalarArrayOpExpr need update or not */
	if (is_oct_inside(lop, &l_var))
		pull_var_update_vardata(clause, &lop, &rop, &l_var);
	else if (is_oct_inside(rop, &r_var))
		pull_var_update_vardata(clause, &rop, &lop, &r_var);

	/* whether OpExpr / ScalarArrayOpExpr needs update or not */
	if (lop != linitial(SA_OpExpr_args(clause)) ||
		rop != lsecond(SA_OpExpr_args(clause)))
		return update_ScalarArray_OpExpr(clause, lop, rop);

	return clause;
}

/*
 * consttype_varchar2_to_text
 *
 * Make Const's consttype from ORAVARCHARBYTEOID / ORAVARCHARCHAROID to TEXTOID,
 * and from ORAVARCHARBYTEARRAYOID / ORAVARCHARCHARARRAYOID to TEXTARRAYOID.
 */
static void
consttype_varchar2_to_text(Node *node)
{
	if (!IsA(node, Const))
		return;

	switch (((Const *) node)->consttype)
	{
		case ORACHARBYTEOID:
		case ORACHARCHAROID:
			{
				if (false)
					((Const *) node)->constvalue = DirectFunctionCall1(
											rtrim1, ((Const *) node)->constvalue);
				((Const *) node)->consttypmod = -1;
				((Const *) node)->consttype = TEXTOID;
			}
			break;

		/*
		 * XXX "= ANY(..::char(n)[])" is different with "IN (..::char(n), ...)"
		 * transformAExprIn() makes an varchar2 array ArrayExpr.
		 * transformAExprOpAny() makes an char array ArrayCoerceExpr.
		 */
		case ORACHARBYTEARRAYOID:
		case ORACHARCHARARRAYOID:
			{
				if (false)
				{
					Datum		   *elems = NULL;
					AnyArrayType   *arr = NULL;
					array_iter		iter;
					int				nextelem = 0,
									numelems = 0,
									nelems = 0;
					int16			elmlen = 0;
					bool			elmbyval = false;
					char			elmalign = 0x00;

					arr = DatumGetAnyArrayP(((Const *) node)->constvalue);
					array_iter_setup(&iter, arr);
					numelems = ArrayGetNItems(AARR_NDIM(arr), AARR_DIMS(arr));

					if (VARATT_IS_EXPANDED_HEADER(arr))
					{
						elmlen = arr->xpn.typlen;
						elmbyval = arr->xpn.typbyval;
						elmalign = arr->xpn.typalign;
					}
					else
						get_typlenbyvalalign(AARR_ELEMTYPE(arr), &elmlen,
											 &elmbyval, &elmalign);

					elems = (Datum *) palloc0(sizeof(Datum) * numelems);

					while (nextelem < numelems)
					{
						Datum	elem = (Datum) 0;
						bool	isnull = true;

						elem = array_iter_next(&iter, &isnull, ++nextelem,
											   elmlen, elmbyval, elmalign);

						if (!isnull)
							elems[nelems++] = DirectFunctionCall1(rtrim1, elem);
					}

					((Const *) node)->constvalue = (Datum) construct_array(elems,
																		   nelems,
																		   TEXTOID,
																		   -1,
																		   false,
																		   'i');
				}
				((Const *) node)->consttypmod = -1;
				((Const *) node)->consttype = TEXTARRAYOID;
			}
			break;

		case ORAVARCHARBYTEOID:
		case ORAVARCHARCHAROID:
			((Const *) node)->consttype = TEXTOID;
			break;

		case ORAVARCHARBYTEARRAYOID:
		case ORAVARCHARCHARARRAYOID:
			((Const *) node)->consttype = TEXTARRAYOID;
			break;

		default:
			break;
	}

	return;
}

/*
 * opr_left_right_internal
 *
 * search operator with oprname, oprleft(Oid), oprright(Oid),
 * and namespace(pg_catalog)
 */
static Oid
opr_left_right_internal(const char *oprname, Oid oprleft, Oid oprright)
{
	HeapTuple	tuple;

	/*
	 * XXX Search from pg_catalog, there's no operator about varchar2 now.
	 * There's no operator for array type, so get its base type.
	 */
	if (oprleft == ORAVARCHARBYTEOID ||
		oprleft == ORAVARCHARCHAROID ||
		oprleft == ORAVARCHARBYTEARRAYOID ||
		oprleft == ORAVARCHARCHARARRAYOID ||
		oprleft == TEXTARRAYOID)
		oprleft = TEXTOID;

	if (oprright == ORAVARCHARBYTEOID ||
		oprright == ORAVARCHARCHAROID ||
		oprright == ORAVARCHARBYTEARRAYOID ||
		oprright == ORAVARCHARCHARARRAYOID ||
		oprright == TEXTARRAYOID)
		oprright = TEXTOID;

	tuple = SearchSysCache4(OPERNAMENSP,
							PointerGetDatum(oprname),
							ObjectIdGetDatum(oprleft),
							ObjectIdGetDatum(oprright),
							ObjectIdGetDatum(PG_CATALOG_NAMESPACE));

	if (HeapTupleIsValid(tuple))
	{
		Form_pg_operator	opr;
		Oid					opno;

		opr = (Form_pg_operator) GETSTRUCT(tuple);
		opno = opr->oid;
		ReleaseSysCache(tuple);

		return opno;
	}
	else
		return InvalidOid;
}

/*
 * opr_left_right
 *
 * get an operator for leftop and rightop with same opname of opno
 */
static Oid
opr_left_right(Node *leftop, Node *rightop, Oid origin_opno)
{
	Oid			opno = InvalidOid;
	HeapTuple	tuple = NULL;

	/* information of operator with oid origin_opno */
	tuple = SearchSysCache1(OPEROID, ObjectIdGetDatum(origin_opno));
	if (HeapTupleIsValid(tuple))
	{
		Form_pg_operator	opr;
		char			   *oprname;
		Oid					oprleft,
							oprright;

		opr = (Form_pg_operator) GETSTRUCT(tuple);
		oprname = (char *) NameStr(opr->oprname);
		oprleft = opr->oprleft;
		oprright = opr->oprright;

		/* search using the operator itself and its args' types */
		opno = opr_left_right_internal((const char *) oprname,
									   exprType(leftop),
									   exprType(rightop));
		if (opno == InvalidOid)
			opno = opr_left_right_internal((const char *) oprname,
										   exprType(leftop),
										   oprright);
		if (opno == InvalidOid)
			opno = opr_left_right_internal((const char *) oprname,
										   oprleft,
										   exprType(rightop));

		ReleaseSysCache(tuple);
	}

	return opno == InvalidOid ? origin_opno : opno;
}

/*
 * is_oct_inside
 *
 * Check whether node is a column of Oracle compatiable views.
 *
 * Only ora_case_trans(col::varchar2)::varchar2(128) is true, because not
 * every situation could be handled here, and it is unnecessary.
 */
bool
is_oct_inside(Node *node, Node **inner_var)
{
	Node   *n = NULL;
	char	flags = 0x00;

	if (IsA(node, RelabelType))
		n = (Node *) ((RelabelType *) node)->arg;
	else
		n = node;

	/* check recursively */
	while (n && IsA(n, FuncExpr) && list_length(((FuncExpr *) n)->args) > 0)
	{
		FuncExpr   *func = ((FuncExpr *) n);
		char	   *proname = get_func_name(func->funcid);

		if (proname == NULL)
			return false;

		/* ::varchar2(128) - the third functioncall, oravarcharbyte */
		if (flags == 0x00 && (strcmp(proname, "oravarcharbyte") == 0 ||
							  strcmp(proname, "oravarcharchar") == 0))
			flags = 0x01;
		/* ora_case_trans() - the second functioncall, ora_case_trans */
		else if (flags == 0x01 && strcmp(proname, "ora_case_trans") == 0)
		{
			if (ora_indexcond_oct_oid == InvalidOid)
				ora_indexcond_oct_oid = func->funcid;

			flags = 0x10;
		}
		/* (::varchar2) - the first functioncall, name_oravarchar */
		else if (flags == 0x10 && strcmp(proname, "name_oravarchar") == 0)
			flags = 0x11;
		else
		{
			pfree(proname);
			return false;
		}

		n = (Node *) linitial(func->args);
		pfree(proname);
	}

	if (flags == 0x11 && IsA(n, Var))
	{
		*inner_var = n;

		return true;
	}

	return false;
}

/*
 * is_var_inside
 *
 * There's a Var in this node or not.
 */
static bool
is_var_inside(Node *node, void *context)
{
	return IsA(node, Var) ? true : expression_tree_walker(node,
														  is_var_inside,
														  NULL);
}

/*
 * pull_var_update_vardata
 *
 * Eliminate the function call ora_case_trans() of Var's outside,
 * and update the other argument of this OpExpr or ScalarArrayOpExpr
 */
static void
pull_var_update_vardata(Node *clause, Node **lop, Node **rop, Node **inner_var)
{
	if (*inner_var == NULL)		/* should not happen */
		return;

	consttype_varchar2_to_text(*rop);

	if (identifier_case_switch == NORMAL)
	{
		*lop = *inner_var;

		if (IsA(*rop, Param) || IsA(*rop, Var))
		{
			if (exprType(*rop) == NAMEOID)
				*rop = (Node *) makeFuncExpr(F_TEXT_NAME,
											 TEXTOID,
											 list_make1(*rop),
											 InvalidOid,
											 C_COLLATION_OID,
											 COERCE_EXPLICIT_CAST);
			else
				*rop = (Node *) makeRelabelType((Expr *) *rop,
												TEXTOID,
												-1,
												DEFAULT_COLLATION_OID,
												COERCE_IMPLICIT_CAST);
		}

		return;
	}

	if (IsA(clause, OpExpr))
	{
		/*
		 * Const
		 * ora_case_trans(col::varchar2)::varchar2(128) = ''
		 */
		if (IsA(*rop, Const) && !((Const *) *rop)->constisnull)
		{
			int		valen = 0;
			char   *dataptr = NULL,
				   *val = NULL,
				   *newval = NULL;

			if (((Const *) *rop)->constlen == -1)
			{
				text    *string = (Datum) 0;

				string = DatumGetTextP(((Const *) *rop)->constvalue);
				dataptr = VARDATA_ANY(string);
				valen = VARSIZE_ANY_EXHDR(string);
			}
			else
			{
				dataptr = (char *) ((Const *) *rop)->constvalue;
				valen = strlen(dataptr);
			}

			val = (char *) palloc0(valen + 1);
			memcpy(val, dataptr, valen);
			newval = identifier_case_transform(val, valen);

			if (val && newval && strcmp(val, newval))
			{
				memcpy(dataptr, newval, valen);

				*lop = *inner_var;
			}

			pfree(val);
			pfree(newval);
		}
		/*
		 * Subquery or Column (*where* or *join on*)
		 * 1. ora_case_trans(col::varchar2)::varchar2(128) in (select ... )
		 * 2. ora_case_trans(col::varchar2)::varchar2(128) = col
		 */
		else if ((IsA(*rop, Param) || is_var_inside(*rop, NULL)) &&
				 OidIsValid(ora_indexcond_oct_oid))
		{
			FuncExpr   *func = NULL;

			/*
			 * convert datatype from name to text, otherwise there may be
			 * *ERROR: compression method lz4 not supported*
			 */
			if (exprType(*rop) == NAMEOID)
				func = makeFuncExpr(ora_indexcond_oct_oid,
									ORAVARCHARBYTEOID,
									list_make1(
										makeFuncExpr(F_TEXT_NAME,
													 TEXTOID,
													 list_make1(*rop),
													 InvalidOid,
													 C_COLLATION_OID,
													 COERCE_EXPLICIT_CAST)),
									InvalidOid,
									DEFAULT_COLLATION_OID,
									COERCE_EXPLICIT_CALL);
			else
				func = makeFuncExpr(ora_indexcond_oct_oid,
									ORAVARCHARBYTEOID,
									list_make1(*rop),
									InvalidOid,
									DEFAULT_COLLATION_OID,
									COERCE_EXPLICIT_CALL);

			*lop = *inner_var;
			*rop = (Node *) makeRelabelType((Expr *) func,
											TEXTOID,
											-1,
											DEFAULT_COLLATION_OID,
											COERCE_IMPLICIT_CAST);
		}
	}
	else if (IsA(clause, ScalarArrayOpExpr))
	{
		/*
		 * Const Array
		 * ora_case_trans(col::varchar2)::varchar2(128) = any ('{}')
		 */
		if (IsA(*rop, Const) && !((Const *) *rop)->constisnull &&
			(((Const *) *rop)->consttype == ORAVARCHARBYTEARRAYOID ||
			 ((Const *) *rop)->consttype == ORAVARCHARCHARARRAYOID ||
			 ((Const *) *rop)->consttype == TEXTARRAYOID ||
			 ((Const *) *rop)->consttype == NAMEARRAYOID))
		{
			AnyArrayType   *arr = NULL;
			array_iter		iter;
			int				nextelem = 0,
							numelems = 0;
			int16			elmlen = 0;
			bool			elmbyval = false;
			char			elmalign = 0x00;

			arr = DatumGetAnyArrayP(((Const *) *rop)->constvalue);
			/* iterating this array */
			array_iter_setup(&iter, arr);
			numelems = ArrayGetNItems(AARR_NDIM(arr), AARR_DIMS(arr));

			if (VARATT_IS_EXPANDED_HEADER(arr))
			{
				elmlen = arr->xpn.typlen;
				elmbyval = arr->xpn.typbyval;
				elmalign = arr->xpn.typalign;
			}
			else
				get_typlenbyvalalign(AARR_ELEMTYPE(arr),
									 &elmlen,
									 &elmbyval,
									 &elmalign);

			/* each one in this array */
			while (nextelem < numelems)
			{
				Datum	elem = (Datum) 0;
				bool	isnull = true;
				int		valen = 0;
				text   *tunpacked = NULL;
				char   *dataptr = NULL,
					   *val = NULL,
					   *newval = NULL;

				elem = array_iter_next(&iter, &isnull, ++nextelem,
									   elmlen, elmbyval, elmalign);

				if (!isnull && elmlen == -1)
				{
					tunpacked = DatumGetTextPP(elem);

					/*
					 * the length of VARDATA_ANY(tunpacked)
					 * may be more than VARSIZE_ANY_EXHDR(tunpacked)
					 *
					 * e.g. VARDATA_ANY(tunpacked) is "PG_CLASS,",
					 *      but valen is 8
					 */
					dataptr = VARDATA_ANY(tunpacked);
					valen = VARSIZE_ANY_EXHDR(tunpacked);
				}
				else
				{
					dataptr = (char *) elem;
					valen = strlen(dataptr);
				}

				val = (char *) palloc0(valen + 1);
				memcpy(val, dataptr, valen);
				newval = identifier_case_transform(val, valen);

				if (val && newval && strcmp(val, newval))
				{
					memcpy(dataptr, newval, valen);

					if (*lop != *inner_var)
						*lop = *inner_var;
				}

				if (tunpacked && (Datum) tunpacked != elem)
					pfree(tunpacked);
				pfree(val);
				pfree(newval);
			}
		}
	}

	return;
}

static Node *
update_ScalarArray_OpExpr(Node *clause, Node *arg1, Node *arg2)
{
	if (IsA(clause, OpExpr))
	{
		OpExpr   *op = (OpExpr *) clause;

		list_free(op->args);
		op->args = list_make2(arg1, arg2);
		op->opno = opr_left_right(arg1, arg2, op->opno);
		op->opfuncid = get_opcode(op->opno);
		/*
		 * N.B. The type of ora_case_trans(col::varchar2)::varchar2(128)'s
		 * col is *name*, so that the typcollation is C_COLLATION_OID (950).
		 * The inputcollid of OpExpr / ScalarArrayOpExpr must be the same,
		 * because match_clause_to_indexcol will check it with idxcollation.
		 */
		op->inputcollid = C_COLLATION_OID;
	}
	else if (IsA(clause, ScalarArrayOpExpr))
	{
		ScalarArrayOpExpr   *sa_op = (ScalarArrayOpExpr *) clause;

		list_free(sa_op->args);
		sa_op->args = list_make2(arg1, arg2);
		sa_op->opno = opr_left_right(arg1, arg2, sa_op->opno);
		sa_op->opfuncid = get_opcode(sa_op->opno);
		sa_op->inputcollid = C_COLLATION_OID;	/* typcollation of *name* */
	}

	return clause;
}
