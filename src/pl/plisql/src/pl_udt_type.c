/*-------------------------------------------------------------------------
 *
 * File:pl_udt_type.c
 *
 * Abstract:
 * 	 Executor for the PLiSQL user define type
 *
 * ReqID:SRS-PLSQL-CURSOR
 *
 * Authored by dwdai@highgo.com,20230306.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *    src/pl/plisql/src/pl_udt_type.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include "pl_udt_type.h"
#include "pl_subproc_function.h"
#include "pl_package.h"
#include "commands/extension.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include "access/relation.h"
#include "parser/parse_type.h"


static void check_cursorvar_assignable(PLiSQL_execstate *estate,
									PLiSQL_var *dest,
									Datum value,
									bool isNull,
									Oid valtypee,
									bool missok);



/*
 * check cursor var can assignable
 */
static void
check_cursorvar_assignable(PLiSQL_execstate *estate, PLiSQL_var *dest,
							Datum value, bool isNull,
							Oid valtype, bool missok)
{
	PLiSQL_ref_cursor_type *destrowtype = (PLiSQL_ref_cursor_type *) dest->datatype;
	PLiSQL_ref_cursor_type *sourcerowtype;

	/*
	 * source is null is ok
	 */
	if (isNull)
		return;

	/* target is weak type, we allow it to assign */
	if (destrowtype->type.ttype != PLISQL_TTYPE_UDT_REF_CURSOR ||
		destrowtype->rettype == NULL)
		return;

	if (valtype == SYS_REFCURSOROID)
		goto open_check;

	sourcerowtype = (PLiSQL_ref_cursor_type *) orarefcursor_funcs.DatumGetOraRefCursorType(value);

	if (sourcerowtype == NULL)
		goto open_check;

	/* two strong ref cursor type */
	if (sourcerowtype->rettype != NULL &&
		sourcerowtype == destrowtype)
		return;
	else if (sourcerowtype->rettype != NULL &&
		sourcerowtype != destrowtype)
		elog(ERROR, "expression is of wrong type");

open_check:
	/* source is weak, target is strong */
	/* if weak type and not open, can assign */
	if (valtype == ORAREFCURSOROID &&
		orarefcursor_funcs.OraRefcursorIsnull(value))
		return;
	else
	{
		Portal portal;
		char *portal_name;
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);
		if (valtype == ORAREFCURSOROID)
			portal_name = orarefcursor_funcs.DatumGetOraRefCursorPortal(value);
		else
			portal_name = TextDatumGetCString(value);
		MemoryContextSwitchTo(oldcontext);
		portal = SPI_cursor_find(portal_name);
		if (portal == NULL)
		{
			/*
			 * there,portalname is invalid because
			 * exec_stmt_close which not assign cursorvar to
			 * null,so maybe references it as a func param
			 */
			if (missok)
				return;

			elog(ERROR, "not find portal :%s", portal_name);
		}
		if (portal->tupDesc == NULL)
			elog(ERROR, "portal :%s don't return tuple", portal_name);

		if (destrowtype->rettype_tupdesc != NULL)
		{
			ora_test_tupdesc_match(destrowtype->rettype_tupdesc, portal->tupDesc, true);
		}
		pfree(portal_name);
	}

	return;
}



/*
 * build a cursor%rowtype type
 */
PLiSQL_explicit_cursor_type*
plisql_build_explicit_cursor_type(TupleDesc retdesc)
{
	PLiSQL_type *basetype;
	PLiSQL_explicit_cursor_type *cursor_type;
	PLiSQL_datum_udt_type *datum_type;

	basetype = plisql_build_datatype(RECORDOID, -1, InvalidOid, NULL);
	cursor_type = (PLiSQL_explicit_cursor_type *) palloc0(sizeof(PLiSQL_explicit_cursor_type));

	memcpy(&cursor_type->type, basetype, sizeof(PLiSQL_type));
	pfree(basetype);

	cursor_type->retdesc = retdesc;
	cursor_type->type.ttype = PLISQL_TTYPE_EXPLICIT_CURSOR;
	datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));

	datum_type->dtype = PLISQL_DTYPE_TYPE;
	datum_type->udt_type = (PLiSQL_type *) cursor_type;
	datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

	plisql_adddatum((PLiSQL_datum *) datum_type);

	return cursor_type;
}


/*
 * wether it is rowtype record var
 */
bool
is_plisql_rowtype_record(PLiSQL_rec *rec)
{
	/* cursor%rowtype */
	if (rec->datatype != NULL &&
		rec->datatype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR)
		return true;

	/* table%rowtype */
	if (plisql_name_composite_type_var_assign_check &&
		rec->rectypeid != RECORDOID)
		return true;

	return false;
}


/*
 * if build a cursor'var
 * we should set nsitem'oid to cursor
 * datatype'oid, so that,we can search
 * cursor.attribute, see details in
 * plisql_ns_lookup func
 */
void
set_plisql_cursorvar_nsitem_typoid(PLiSQL_variable *variable, bool set_retype)
{
	PLiSQL_nsitem *nse;
	PLiSQL_var *cursor;

	Assert(variable->dtype == PLISQL_DTYPE_VAR);

	cursor = (PLiSQL_var *) variable;

	nse = plisql_ns_top();
	nse->typoid = cursor->datatype->typoid;

	if (set_retype &&
		cursor->datatype->ttype == PLISQL_TTYPE_UDT_REF_CURSOR)
	{
		PLiSQL_ref_cursor_type *cursor_type = (PLiSQL_ref_cursor_type *) cursor->datatype;

		cursor->cursor_rettype_tupledesc = cursor_type->rettype_tupdesc;
	}
}


/*
 * get Param tupdesc if param is a cursor%rowtype or a Oracle record
 */
TupleDesc
plisql_param_tupdesc_ref(ParseState *pstate, Param *param)
{
	PLiSQL_expr *expr = (PLiSQL_expr *) pstate->p_ref_hook_state;
	PLiSQL_execstate *estate;
	int dno;
	PLiSQL_datum *datum;
	TupleDesc tupdesc = NULL;

	dno = param->paramid - 1;

	/* see comment in resolve_column_ref */
	estate = expr->func->cur_estate;
	Assert(dno >= 0 && dno < estate->ndatums);
	datum = estate->datums[dno];

	switch (datum->dtype)
	{
		case PLISQL_DTYPE_REC:
			{
				PLiSQL_rec *rec = (PLiSQL_rec *) datum;

				if (rec->datatype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR)
				{
					PLiSQL_explicit_cursor_type *cursor_type;

					cursor_type = (PLiSQL_explicit_cursor_type *) rec->datatype;
					tupdesc = cursor_type->retdesc;
				}
				/* Begin - ReqID:SRS-PLSQL-RECORD */
				else if (rec->erh != NULL)
					tupdesc = expanded_record_get_tupdesc(rec->erh);
				/* End - ReqID:SRS-PLSQL-RECORD */
			}
			break;
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		case PLISQL_DTYPE_ORA_RECORD:
			{
				PLiSQL_row *row = (PLiSQL_row*) datum;

				tupdesc = row->rowtupdesc;
			}
			break;
		/* End - ReqID:SRS-PLSQL-RECORD */
		default:
			break;
	}

	return tupdesc;
}



/*
 * get cursor tupdesc
 */
TupleDesc
plisql_get_cursor_tupdesc_incompile(PLiSQL_expr *cursor_expr,
													int cursor_options)
{
	/*
	 * plsql_get_cursor_tupdesc has change memorycontext
	 * we should save currentMemoryContext and when we return
	 * function, we should change memoryContext to old
	 */
	MemoryContext old;
	TupleDesc desc;
	TupleDesc retdesc = NULL;
	PLiSQL_execstate estate;
	bool	reset_cur_estate = false;
	PLiSQL_datum **saved_datums;
	int saved_ndatums;
	int saved_nsubprocfuncs;
	PLiSQL_subproc_function **saved_subprocfuncs;

	saved_datums = plisql_curr_compile->datums;
	saved_ndatums = plisql_curr_compile->ndatums;
	saved_nsubprocfuncs = plisql_curr_compile->nsubprocfuncs;
	saved_subprocfuncs = plisql_curr_compile->subprocfuncs;
	PG_TRY();
	{
		plisql_curr_compile->datums = plisql_Datums;
		plisql_curr_compile->ndatums = plisql_nDatums;
		plisql_curr_compile->nsubprocfuncs = plisql_nsubprocFuncs;
		plisql_curr_compile->subprocfuncs = plisql_subprocFuncs;

		memset(&estate, 0x00, sizeof(PLiSQL_execstate));

		estate.func = plisql_curr_compile;
		/* curvar maybe use datums */
		estate.ndatums = plisql_nDatums;
		estate.datums = plisql_Datums;
		if (estate.func->cur_estate == NULL)
		{
			reset_cur_estate = true;
			estate.func->cur_estate = &estate;
		}
		old = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

		plisql_push_compile_global_proper();

		desc = plisql_get_cursor_tupdesc(&estate,
										cursor_expr,
										cursor_options);
		MemoryContextSwitchTo(old);
		retdesc = CreateTupleDescCopy(desc);
		FreeTupleDesc(desc);
	}
	PG_FINALLY();
	{
		plisql_pop_compile_global_proper();
		plisql_curr_compile->datums = saved_datums;
		plisql_curr_compile->ndatums = saved_ndatums;
		plisql_curr_compile->nsubprocfuncs = saved_nsubprocfuncs;
		plisql_curr_compile->subprocfuncs = saved_subprocfuncs;
	}
	PG_END_TRY();

	/* curvar maybe use datums */
	if (reset_cur_estate)
		estate.func->cur_estate = NULL;
	return retdesc;
}


/*
 * check explicit cursor
 * declare and define has match
 * return:
 * 0 is match
 * -1 the rettype is not match
 * -2 the arglist is not match
 */
int
plisql_check_cursor_define(PLiSQL_var *var,
							PLiSQL_type *rettype,
							List *arglist)
{
	ListCell *lc1;
	ListCell *lc2;

	/* check return type is match */
	if (rettype != NULL &&
		var->cursor_explicit_rettype != NULL)
	{
		/* check typoid */
		if (rettype->typoid != var->cursor_explicit_rettype->typoid)
			return -1;
		if (rettype->ttype != var->cursor_explicit_rettype->ttype)
			return -1;
		/* check user define type */
		if (rettype->ttype == PLISQL_TTYPE_UDT_REF_CURSOR &&
			rettype != var->cursor_explicit_rettype)
			return -1;
	}

	/* check arglist match */
	if (list_length(arglist) != list_length(var->arglist))
		return -2;
	if (arglist == NIL)
		return 0;

	forboth(lc1, var->arglist, lc2, arglist)
	{
		PLiSQL_function_argitem	*argitem1 = (PLiSQL_function_argitem *) lfirst(lc1);
		PLiSQL_function_argitem	*argitem2 = (PLiSQL_function_argitem *) lfirst(lc2);

		if (strcmp(argitem1->argname, argitem2->argname) != 0)
			return -2;
		/* check argtype */
		if (argitem1->type->typoid != argitem2->type->typoid)
			return -2;
		if (argitem1->type->ttype != argitem2->type->ttype)
			return -2;
		/* check user define type */
		if (argitem1->type->ttype == PLISQL_TTYPE_UDT_REF_CURSOR &&
			argitem1->type != argitem2->type)
			return -2;
		/*
		 * check default expr, there, oracle doesn't check, it use the declare
		 * default
		 */
	}
	return 0;
}

/*
 * build cursor arg datum
 */
int
plisql_build_cursor_args(List *arglist, int lineno)
{
	PLiSQL_row *new;
	int i;
	ListCell *l;

	new = palloc0(sizeof(PLiSQL_row));
	new->dtype = PLISQL_DTYPE_ROW;
	new->refname = "(unnamed row)";
	new->lineno = lineno;
	new->rowtupdesc = NULL;
	new->nfields = list_length(arglist);
	new->fieldnames = palloc(new->nfields * sizeof(char *));
	new->varnos = palloc(new->nfields * sizeof(int));

	i = 0;
	foreach (l, arglist)
	{
		PLiSQL_variable *arg;
		PLiSQL_function_argitem *argitem = (PLiSQL_function_argitem *) lfirst(l);

		/*
		 * Check to make sure name isn't already declared
		 * in the current block.
		 */
		if (plisql_ns_lookup(plisql_ns_top(), true,
							  argitem->argname, NULL, NULL,
							  NULL) != NULL)
			elog(ERROR, "duplicate declaration in cursor arguments");

		arg = plisql_build_variable(argitem->argname, -1, argitem->type, true);
		new->fieldnames[i] = arg->refname;
		new->varnos[i] = arg->dno;
		i++;
	}

	plisql_adddatum((PLiSQL_datum *) new);
	return new->dno;
}


/*
 * build rows from datums dnos
 */
int
plisql_buld_rows_from_datums(int *datum_dnos, int ndatums)
{
	PLiSQL_row *new;
	int i;

	new = palloc0(sizeof(PLiSQL_row));
	new->dtype = PLISQL_DTYPE_ROW;
	new->refname = "(unnamed row)";
	new->lineno = -1;
	new->rowtupdesc = NULL;
	new->nfields = ndatums;
	new->fieldnames = palloc(new->nfields * sizeof(char *));
	new->varnos = palloc(new->nfields * sizeof(int));

	for (i = 0; i < ndatums; i++)
	{
		PLiSQL_variable *arg;

		arg = (PLiSQL_variable *) plisql_Datums[datum_dnos[i]];
		new->fieldnames[i] = arg->refname;
		new->varnos[i] = arg->dno;
		i++;
	}

	plisql_adddatum((PLiSQL_datum *) new);
	return new->dno;
}


/*
 * check all args has default
 */
bool
check_all_args_has_default(List *arglist)
{
	ListCell *lc;

	foreach (lc, arglist)
	{
		PLiSQL_function_argitem *argitem = (PLiSQL_function_argitem *) lfirst(lc);

		if (argitem->defexpr == NULL)
			return false;
	}

	return true;
}

/*
 * get the argno default query of arglist
 */
char *
get_args_default_query(List *arglist, int argno)
{
	PLiSQL_function_argitem *argitem = NULL;

	if (arglist == NIL)
		return NULL;

	argitem = (PLiSQL_function_argitem *) list_nth(arglist, argno);

	if (argitem->defexpr == NULL)
		return NULL;
	return pstrdup(argitem->defexpr->query);
}

/*
 * build user define ref cursor type
 */
void
plisql_build_udt_cursor_type(const char *refname, PLiSQL_type *rettype,
										int lineno,
										bool add2namespace)
{
	PLiSQL_type *basetype;
	PLiSQL_ref_cursor_type *cursor_type;
	PLiSQL_datum_udt_type *datum_type;

	if (check_strong_cursor_assign)
		basetype = plisql_build_datatype(ORAREFCURSOROID, -1, InvalidOid, NULL);
	else
		basetype = plisql_build_datatype(SYS_REFCURSOROID, -1, InvalidOid, NULL);
	cursor_type = (PLiSQL_ref_cursor_type *) palloc0(sizeof(PLiSQL_ref_cursor_type));

	memcpy(&cursor_type->type, basetype, sizeof(PLiSQL_type));
	pfree(basetype);

	cursor_type->rettype = rettype;
	if (cursor_type->rettype != NULL)
	{
		cursor_type->rettype_tupdesc = plisql_get_rowtype_tupledesc(rettype);
		if (cursor_type->rettype_tupdesc == NULL)
			elog(ERROR, "ref cursor return type is not row type");
	}
	cursor_type->type.ttype = PLISQL_TTYPE_UDT_REF_CURSOR;

	pfree(cursor_type->type.typname);
	cursor_type->type.typname = pstrdup(refname);
	cursor_type->lineno = lineno;
	datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));

	datum_type->dtype = PLISQL_DTYPE_TYPE;
	datum_type->udt_type = (PLiSQL_type *) cursor_type;
	datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

	plisql_adddatum((PLiSQL_datum *) datum_type);

	if (add2namespace)
		plisql_ns_additem(PLISQL_NSTYPE_TYPE,
						datum_type->dno, refname);

	return;
}


/*
 * load orarefcursor func
 */
void
plisql_register_orarefcursor_func(void)
{
	Oid extensionOid;
	Oid funcid;
	List *funcname = NIL;

	if (orarefcursor_funcs.loadstate != UNLOADED)
		return;

	extensionOid = get_extension_oid("ivorysql_ora", true);

	if (!OidIsValid(extensionOid))
	{
		orarefcursor_funcs.loadstate = LOAD_FAILED;
		elog(WARNING, "extension \"ivorysql_ora\" does not exist");

		return;
	}
	PG_TRY();
	{
		funcname = list_make2(makeString("sys"), makeString("orarefcursor_init"));
		funcid = LookupFuncName(funcname, 0 , NULL, false);

		/* check if returns VOID, else it's a wrong function */
		if (get_func_rettype(funcid) != VOIDOID)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("\"%s\" function is wrong", "sys.orarefcursor_init")));

		OidFunctionCall0Coll(funcid, InvalidOid);
		orarefcursor_funcs.loadstate = LOADED;
	}
	PG_CATCH();
	{
		orarefcursor_funcs.loadstate = LOAD_FAILED;
		PG_RE_THROW();
	}
	PG_END_TRY();

	return;
}


/*
 * build cursor attribute dno
 * we should make sure the cursor attribute'dno
 * is continuous, because we only record firstattribute dno
 * in its parents
 */
void
plisql_build_cursor_attribute(PLiSQL_var *cursor)
{
	PLiSQL_attribute *attr;
	Oid pkgoid;

	if (plisql_compile_packageitem == NULL)
		pkgoid = InvalidOid;
	else
		pkgoid = plisql_compile_packageitem->source.fn_oid;

	/* build isopen attribute */
	attr = palloc0(sizeof(PLiSQL_attribute));
	attr->datatype = plisql_build_datatype(BOOLOID, -1, InvalidOid, NULL);
	attr->attributeparentno = cursor->dno;
	attr->dtype = PLISQL_DTYPE_ATTRIBUTE;
	attr->fieldname = pstrdup("isopen");
	attr->get_from_portal = true;
	attr->pkgoid = pkgoid;

	plisql_adddatum((PLiSQL_datum *) attr);
	cursor->firstattribute = attr->dno;

	/* build found attribute */
	attr = palloc0(sizeof(PLiSQL_attribute));
	attr->datatype = plisql_build_datatype(BOOLOID, -1, InvalidOid, NULL);
	attr->attributeparentno = cursor->dno;
	attr->dtype = PLISQL_DTYPE_ATTRIBUTE;
	attr->fieldname = pstrdup("found");
	attr->get_from_portal = true;
	attr->pkgoid = pkgoid;
	plisql_adddatum((PLiSQL_datum *) attr);

	/* build notfound attribute */
	attr = palloc0(sizeof(PLiSQL_attribute));
	attr->datatype = plisql_build_datatype(BOOLOID, -1, InvalidOid, NULL);
	attr->attributeparentno = cursor->dno;
	attr->dtype = PLISQL_DTYPE_ATTRIBUTE;
	attr->fieldname = pstrdup("notfound");
	attr->get_from_portal = true;
	attr->pkgoid = pkgoid;
	plisql_adddatum((PLiSQL_datum *) attr);

	/* build rowcount attribute */
	attr = palloc0(sizeof(PLiSQL_attribute));
	attr->datatype = plisql_build_datatype(INT8OID, -1, InvalidOid, NULL);
	attr->attributeparentno = cursor->dno;
	attr->dtype = PLISQL_DTYPE_ATTRIBUTE;
	attr->fieldname = pstrdup("rowcount");
	attr->get_from_portal = true;
	attr->pkgoid = pkgoid;
	plisql_adddatum((PLiSQL_datum *) attr);

	/* Begin - ReqID:SRS-PLSQL-FORALL */
	/* build bulk_rowcount attribute */
	attr = palloc0(sizeof(PLiSQL_attribute));
	attr->datatype = plisql_build_datatype(INT8ARRAYOID, -1, InvalidOid, NULL);
	attr->attributeparentno = cursor->dno;
	attr->dtype = PLISQL_DTYPE_ATTRIBUTE;
	attr->fieldname = pstrdup("bulk_rowcount");
	attr->get_from_portal = true;
	attr->pkgoid = pkgoid;
	plisql_adddatum((PLiSQL_datum *) attr);
	/* End - ReqID:SRS-PLSQL-FORALL */

	/*
	 * TODO for BULK_EXCEPTIONS for
	 * sql cursor
	 */

	/* make sure cursor attribute is continuous */
	Assert(attr->dno == PLISQL_SUPPORT_ATTRIBUTE_NUM + cursor->firstattribute);

	return;
}


/*
 * when reference a package cursor'var
 * we should build local PLiSQL_pkg_datum for
 * its cursor attribute datum
 */
void
plisql_build_package_local_cursor_attribute(PLiSQL_var *cursor,
															bool add2namespace,
															const char *refname,
															int lineno)
{
	int i;
	PackageCacheItem *item;
	StringInfoData name_data;
	int first_attno;
	PLiSQL_package *psource;

	Assert(OidIsValid(cursor->pkgoid));

	item = PackageCacheLookup(&cursor->pkgoid);
	Assert(item != NULL);
	psource = (PLiSQL_package *) item->source;

	initStringInfo(&name_data);

	first_attno = cursor->firstattribute;

	for (i = 0; i <= PLISQL_SUPPORT_ATTRIBUTE_NUM; i++)
	{
		PLiSQL_pkg_datum *pkg_datum;
		char *attribute_name;

		resetStringInfo(&name_data);

		switch (i)
		{
			case PLISQL_ATTRIBUTE_ISOPEN: /* bug#M364 */
				attribute_name = "isopen";
				break;
			case PLISQL_ATTRIBUTE_FOUND:
				attribute_name = "found";
				break;
			case PLISQL_ATTRIBUTE_NOTFOUND:
				attribute_name = "notfound";
				break;
			case PLISQL_ATTRIBUTE_ROWCOUNT:
				attribute_name = "rowcount";
				break;
			/* Begin - ReqID:SRS-PLSQL-FORALL */
			case PLISQL_ATTRIBUTE_BULK_ROWCOUNT:
				attribute_name = "bulk_rowcount";
				break;
			/* End - ReqID:SRS-PLSQL-FORALL */
			default:
				elog(ERROR, "invalid attribute");
				break;
		}
		appendStringInfo(&name_data, "%s.%s", refname, attribute_name);

		pkg_datum = (PLiSQL_pkg_datum *) palloc(sizeof(PLiSQL_pkg_datum));

		pkg_datum->dtype = PLISQL_DTYPE_PACKAGE_DATUM;
		pkg_datum->lineno = lineno;
		pkg_datum->refname = pstrdup(name_data.data);
		pkg_datum->item = item;
		pkg_datum->pkgvar = psource->source.datums[first_attno + i];

		plisql_adddatum((PLiSQL_datum *) pkg_datum);
		if (add2namespace)
			plisql_ns_additem(PLISQL_NSTYPE_VAR,
								pkg_datum->dno,
								name_data.data);
	}
	pfree(name_data.data);

	return;
}


/*
 * wether this var is a sql cursor ?
 */
bool
is_sql_cursor(PLiSQL_execstate *estate, PLiSQL_var *var)
{
	if (OidIsValid(var->pkgoid))
	{
		PLiSQL_package *psource;
		PackageCacheItem *item = NULL;

		item = PackageCacheLookup(&var->pkgoid);
		Assert(item != NULL);
		psource = (PLiSQL_package *) item->source;

		if (var->dno == psource->source.sql_varno)
			return true;
		else
			return false;
	}
	else
	{
		if (var->dno == estate->sql_varno)
			return true;
		else
			return false;
	}
}

/*
 * is a oracle cursor datum ?
 */
bool
is_oracursor_datum(PLiSQL_datum *datum)
{
	if (datum->dtype == PLISQL_DTYPE_PACKAGE_DATUM)
	{
		PLiSQL_pkg_datum *pkg_datum = (PLiSQL_pkg_datum *) datum;

		return is_oracursor_datum(pkg_datum->pkgvar);
	}
	else if (datum->dtype == PLISQL_DTYPE_VAR)
	{
		PLiSQL_var *var = (PLiSQL_var *) datum;

		if (ISORACURSOROID(var->datatype->typoid))
			return true;
		else
			return false;
	}

	return false;
}



/*
 * get cursor attribute from portal
 */
Datum
plisql_get_cursor_attribute(PLiSQL_execstate *estate,
										PLiSQL_var *cursor,
										PLiSQL_attribute_type attr_type,
										bool *isnull)
{
	char *portal_name;
	Portal portal;
	Datum retval = (Datum) 0;

	portal_name = plisql_get_portal_name(estate, cursor);

	if (portal_name == NULL)
	{
		elog(ERROR, "invalid cursor \"%s\"", cursor->refname);
		return (Datum) 0;
	}

	portal = SPI_cursor_find(portal_name);
	pfree(portal_name);
	if (portal == NULL)
	{
		elog(ERROR, "invalid cursor \"%s\"", cursor->refname);
		return (Datum) 0;
	}
	switch (attr_type)
	{
		case PLISQL_ATTRIBUTE_ISOPEN: /* bug#M364 */
			*isnull = false;
			retval = BoolGetDatum(true);
			break;
		case PLISQL_ATTRIBUTE_FOUND:
			if (portal->atStart && !portal->atEnd)
				*isnull = true;
			else
			{
				*isnull = false;
				retval = BoolGetDatum(!portal->atEnd);
			}
			break;
		case PLISQL_ATTRIBUTE_NOTFOUND:
			if (portal->atStart && !portal->atEnd)
				*isnull = true;
			else
			{
				*isnull = false;
				retval = BoolGetDatum(portal->atEnd);
			}
			break;
		case PLISQL_ATTRIBUTE_ROWCOUNT:
			*isnull = false;
			retval = Int64GetDatum(portal->portalPos);
			break;
		/* Begin - ReqID:SRS-PLSQL-FORALL */
		case PLISQL_ATTRIBUTE_BULK_ROWCOUNT:
			elog(ERROR, "wrong to get bulk_rowcount value");
			break;
		/* End - ReqID:SRS-PLSQL-FORALL */
		default:
			elog(ERROR, "invalid cursor attribute dtype %u", attr_type);
			break;
	}

	return retval;
}


/*
 * dno is implicit cursor or its attribute
 */
bool
is_implicut_cursor_or_attribute(PLiSQL_execstate *estate,
										int dno)
{
	PLiSQL_var *cursor;

	if (dno == estate->sql_varno)
		return true;

	cursor = (PLiSQL_var *) estate->datums[estate->sql_varno];

	/* is sql attribute */
	if (dno >= cursor->firstattribute &&
		dno <= cursor->firstattribute + PLISQL_SUPPORT_ATTRIBUTE_NUM)
		return true;
	return false;
}

/*
 * construct a tupldesc from rowtype
 */
TupleDesc
plisql_get_rowtype_tupledesc(PLiSQL_type *rowtype)
{
	TupleDesc destDesc = NULL;

	switch(rowtype->ttype)
	{
		case PLISQL_TTYPE_UDT_REF_CURSOR:
			{
				PLiSQL_ref_cursor_type *ref_type = (PLiSQL_ref_cursor_type *) rowtype;

				elog(ERROR, "invalid cursor return type; '%s' must be a record type",
						ref_type->type.typname);
			}
			break;
		case PLISQL_TTYPE_EXPLICIT_CURSOR:
			{
				PLiSQL_explicit_cursor_type  *cursor_type = (PLiSQL_explicit_cursor_type *) rowtype;

				destDesc = cursor_type->retdesc;
			}
			break;
		/* Begin - BUG#Z0001350 */
		case PLISQL_TTYPE_ORARECORD_TYPE:
			{
				PLiSQL_ora_record_type *ora_rec_type = (PLiSQL_ora_record_type *) rowtype;

				destDesc = plisql_build_tupdesc_from_recordtype(ora_rec_type);
			}
			break;
		/* End - BUG#Z00001350 */
		default:
			/* table or view */
			{
				TypeCacheEntry *typentry;

				/*
				 * Consult the typcache to see if it's a domain over composite, and in
				 * any case to get the tupdesc and tupdesc identifier.
				 */
				typentry = lookup_type_cache(rowtype->typoid,
												 TYPECACHE_TUPDESC |
												 TYPECACHE_DOMAIN_BASE_INFO);
				if (typentry->typtype == TYPTYPE_DOMAIN)
				{
					typentry = lookup_type_cache(typentry->domainBaseType,
												 TYPECACHE_TUPDESC);
				}
				if (typentry->tupDesc == NULL)
					ereport(ERROR,
							(errcode(ERRCODE_WRONG_OBJECT_TYPE),
							 errmsg("type %s is not composite",
									format_type_be(rowtype->typoid))));
				destDesc =  CreateTupleDescCopy(typentry->tupDesc);
			}
			break;
	}

	if (destDesc != NULL &&
		destDesc->tdtypeid == RECORDOID)
		assign_record_type_typmod(destDesc);

	return destDesc;
}



/*
 * assign a oracle cursor variable value
 * like orarefcursoroid or sys_refcursoroid
 */
void
exec_check_assign_cursor_value(PLiSQL_execstate *estate,
										PLiSQL_var *target,
										Datum value, bool isNull,
										Oid valtype, int32 valtypmod,
										bool missok)
{

	if (!ISORACURSOROID(valtype))
		elog(ERROR, "expression is of wrong type");

	Assert(ISORACURSOROID(target->datatype->typoid));

	/* first, check assignable */
	check_cursorvar_assignable(estate, target,
								value, isNull, valtype, missok);

	return;
}


/*
 * like plisql_parse_wordtype, but we find udt type
 * ident: typename or varname
 * byval: if true we find cursor%rowtype, else we
 * find ref cursor type or record type
 */
PLiSQL_type *
plisql_parse_udt_rowtype(const char *ident, bool byval)
{
	PLiSQL_nsitem *nse;
	TypeName *typeName;
	char *name1 = NULL;
	char *name2 = NULL;
	char *name3 = NULL;

	/*
	 * ident maybe include some extral char like \r \t \n
	 * we had better use grammer to get a really name
	 */
	typeName = typeStringToTypeName(ident);
	Assert(typeName->names != NIL);

	switch(list_length(typeName->names))
	{
		case 1:
			name1 = strVal(linitial(typeName->names));
			break;
		case 2:
			name1 = strVal(linitial(typeName->names));
			name2 = strVal(lsecond(typeName->names));
			break;
		case 3:
			name1 = strVal(linitial(typeName->names));
			name2 = strVal(lsecond(typeName->names));
			name3 = strVal(lthird(typeName->names));
			break;
		default:
			/* does not support */
			return NULL;
			break;
	}

	/*
	 * Do a lookup in the current namespace stack
	 */
	nse = plisql_ns_lookup(plisql_ns_top(), false,
							name1, name2, name3,
							NULL);

	/*
	 * Do a lookup in the current namespace stack
	 */
	if (byval)
	{
		if (nse != NULL && nse->itemtype == PLISQL_NSTYPE_VAR)
		{
			PLiSQL_var *var = (PLiSQL_var *) plisql_Datums[nse->itemno];

			if (ISCURSOROID(var->datatype->typoid))
			{
				if (!var->is_explicit_cursor)
				{
					return plisql_return_ref_cursor_rowtype(var->datatype);
				}
				else
				{
					if (var->cursor_resulttupdesc == NULL)
					{
						if (var->cursor_explicit_expr == NULL)
							elog(ERROR, "cursor \"%s\" has not been assigned query",
								var->refname);
						var->cursor_resulttupdesc = plisql_get_cursor_tupdesc_incompile(var->cursor_explicit_expr,
																		var->cursor_options);
					}
					return (PLiSQL_type *) plisql_build_explicit_cursor_type(var->cursor_resulttupdesc);
				}
			}
		}
	}
	else
	{
		if (nse != NULL &&
			nse->itemtype == PLISQL_NSTYPE_TYPE &&
			typeName->arrayBounds == NIL /* BUG#Z1672 */)
		{
			/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
			PLiSQL_datum *datum_type = plisql_Datums[nse->itemno];
			PLiSQL_type *basetype = NULL;

			/* 
			 * The subtype now suppots record as a base type, so cases where the record type
			 * appears here need to be handled. 
			 */
			if (datum_type->dtype == PLISQL_DTYPE_REC) {
				basetype = ((PLiSQL_rec*)datum_type)->datatype;
			} else {
				basetype = ((PLiSQL_datum_udt_type*)datum_type)->udt_type;
			}

			/* 
			 * Set the typmod for each variable.
			 * 
			 * NB: These code was added for feature "subtype", But the setting of typmod 
			 * should not only take effect on the "subtype" type.
			 */
			if (basetype && typeName->typmods != NULL) {
				PLiSQL_type *new_basetype;
				Oid type_id;
				int32 typmod;

				/*
				* Each scalar variable may specify a different typmod, so it is necessary
				* to palloc a new PLiSQL_type for each variable.
				*/
				new_basetype = palloc0(sizeof(PLiSQL_type));
				memcpy(new_basetype, basetype, sizeof(PLiSQL_type));

				/* 
				* We set "names" to be NULL and specified the "typeOid". This allows
				* the corresponding type to be correctly identified in the
				* "LookupTypeNameExtended" function.
				*/
				typeName->names = NULL;
				typeName->typeOid = new_basetype->typoid;
				typenameTypeIdAndMod(NULL, typeName, &type_id, &typmod);
				new_basetype->atttypmod = typmod;
				basetype = new_basetype;
			}

			return basetype;
			/* End - ReqID:SRS-PLSQL-SUBTYPE */
		}
	}

	return NULL;
}



/*
 * handle ref cursor rowtype
 */
PLiSQL_type *
plisql_return_ref_cursor_rowtype(PLiSQL_type *rowtype)
{
	PLiSQL_ref_cursor_type *ref_type = (PLiSQL_ref_cursor_type *) rowtype;

	Assert(rowtype != NULL);
	Assert(rowtype->ttype == PLISQL_TTYPE_UDT_REF_CURSOR);

	if (ref_type->rettype == NULL)
		elog(ERROR, "ref cursor \"%s\" don't have a return type", ref_type->type.typname);

	return ref_type->rettype;
}



/*
 * Build empty named record variable, and optionally add it to namespace
 */
PLiSQL_rec *
plisql_build_record_for_cursorrowtype(const char *refname,
						int lineno,
						PLiSQL_type *dtype,
						bool add2namespace)
{
	PLiSQL_rec *rec;
	PLiSQL_explicit_cursor_type *explicit_type;
	int i;

	Assert(dtype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR);

	rec = palloc0(sizeof(PLiSQL_rec));
	rec->dtype = PLISQL_DTYPE_REC;
	rec->refname = pstrdup(refname);
	rec->lineno = lineno;
	/* other fields are left as 0, might be changed by caller */
	rec->datatype = dtype;
	rec->rectypeid = RECORDOID;
	rec->firstfield = -1;
	rec->erh = NULL;
	plisql_adddatum((PLiSQL_datum *) rec);
	if (add2namespace)
		plisql_ns_additem(PLISQL_NSTYPE_REC, rec->dno, rec->refname);

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	rec->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

	explicit_type = (PLiSQL_explicit_cursor_type *) dtype;

	Assert(explicit_type->retdesc != NULL);

	for (i = 0; i < explicit_type->retdesc->natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(explicit_type->retdesc, i);

		if (!attr->attisdropped)
			plisql_build_recfield(rec, NameStr(explicit_type->retdesc->attrs[i].attname));
	}

	return rec;
}

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
/* 
 * Build a subtype and set its constraints. 
 */
void
plisql_build_subtype(char *subtypnme, int lineno, PLiSQL_type* btype_p, List *range_list, bool isNotNull)
{
	PLiSQL_datum *datum_type;

	/* It is only necessary to create a *real* subtype when setting the RANGE constraint. */
	if (range_list != NULL)
	{
		Oid basetypeid;
		PLiSQL_type *basetype;
		PLiSQL_subtype_type *range_subtype;
		PLiSQL_datum_udt_type *datum_udt_type;

		/* syntax: RANGE xx..yy */
		Assert(list_length(range_list) == 2);
		/* 
		 * Only support int4 because in Oracle it only support 
		 * PLS_INTEGER, BINARY_INTEGER and SIMPLE_INTEGER.
		 */
		basetypeid = getBaseType(btype_p->typoid);
		if (basetypeid != INT4OID)
			elog(ERROR, "improper constraint form used");

		basetype = plisql_build_datatype(basetypeid, -1, InvalidOid, NULL);
		range_subtype = (PLiSQL_subtype_type *) palloc0(sizeof(PLiSQL_subtype_type));
		memcpy(&range_subtype->type, basetype, sizeof(PLiSQL_type));
		pfree(basetype);
		range_subtype->type.ttype = PLISQL_TTYPE_SUBTYPE;

		/* Set notnull and typmod constraint. */
		if (isNotNull)
			range_subtype->type.notnull = true;
		range_subtype->type.atttypmod = btype_p->atttypmod;

		/* Set RANGE constraint. */
		range_subtype->setRange = true;
		range_subtype->lower = linitial_int(range_list);
		range_subtype->upper = lsecond_int(range_list);

		datum_udt_type = palloc0(sizeof(PLiSQL_datum_udt_type));
		datum_udt_type->dtype = PLISQL_DTYPE_TYPE;
		datum_udt_type->udt_type = (PLiSQL_type *) range_subtype;
		datum_udt_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
		datum_type = (PLiSQL_datum *)datum_udt_type;
	}
	else
	{
		PLiSQL_type *basetype;

		/* Duplicate a base datatype. */
		datum_type = plisql_copy_base_datatype(btype_p, subtypnme, lineno);

		if (btype_p->ttype == PLISQL_TTYPE_REC)
		{
			basetype = ((PLiSQL_rec *)datum_type)->datatype;
		}
		else
		{
			basetype = ((PLiSQL_datum_udt_type *)datum_type)->udt_type;
		}

		/* Set notnull and typmod constraint. */
		if (isNotNull)
			basetype->notnull = true;
		basetype->atttypmod = btype_p->atttypmod;
	}

	/* add it to namespace. */
	plisql_adddatum(datum_type);
	/* 
	 * To enable the subtype to support record type as a base type, the record type has
	 * also been added to PLISQL_NSTYPE_TYPE.
	 */
	plisql_ns_additem(PLISQL_NSTYPE_TYPE, datum_type->dno, subtypnme);
}
/* End - ReqID:SRS-PLSQL-SUBTYPE */
