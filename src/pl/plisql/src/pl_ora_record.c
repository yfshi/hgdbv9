/*-------------------------------------------------------------------------
 *
 * pl_ora_record.c		- Handler for the PL/iSQL record
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_ora_record.c
 *
 *
 *-------------------------------------------------------------------------
 */
/* Begin - ReqID:SRS-PLSQL-RECORD */
#include "postgres.h"

#include "plisql.h"
#include "pl_udt_type.h"
#include "pl_package.h"
#include "catalog/pg_package.h"
#include "utils/memutils.h"
#include "utils/builtins.h"
#include "utils/jsonb.h"
#include "utils/syscache.h"
#include "parser/parse_type.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "utils/lsyscache.h"

#define get_eval_mcontext(estate) \
	((estate)->eval_econtext->ecxt_per_tuple_memory)
#define eval_mcontext_alloc(estate, sz) \
	MemoryContextAlloc(get_eval_mcontext(estate), sz)
#define eval_mcontext_alloc0(estate, sz) \
	MemoryContextAllocZero(get_eval_mcontext(estate), sz)

/*
 * build user defined record.
 */
PLiSQL_type *
plisql_build_ora_rec_type(char *refname, int lineno, List *args)
{
	PLiSQL_datum_udt_type *datum_type;
	PLiSQL_ora_record_type *ora_rec;
	Oid		pkgoid;

	ora_rec = (PLiSQL_ora_record_type *) palloc0(sizeof(PLiSQL_ora_record_type));

	pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
	if (pkgoid != InvalidOid)
	{
		char		typname[(NAMEDATALEN * 2) + 100];
		HeapTuple 	pkgTup;

		/*
		 * Lookup the pg_package tuple by Oid
		 */
		pkgTup = SearchSysCache1(PKGOID, ObjectIdGetDatum(pkgoid));
		if (!HeapTupleIsValid(pkgTup))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("package with OID %u does not exist", pkgoid)));

		snprintf(typname, sizeof(typname), "%s.%s", NameStr(((Form_pg_package) GETSTRUCT(pkgTup))->pkgname),
				refname);
		ReleaseSysCache(pkgTup);

		ora_rec->type.typname = pstrdup(typname);
	}
	else
		ora_rec->type.typname = pstrdup(refname);

	/* The maximum number of record fields is 100 */
	if (list_length(args) > max_record_fields /* OA-REQ-2466 */)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
				 errmsg("record cannot have more than %d fields",
							   max_record_fields /* OA-REQ-2466 */)));

	ora_rec->type.typoid = ORARECORDOID;
	ora_rec->type.ttype = PLISQL_TTYPE_ORARECORD_TYPE;
	ora_rec->type.typlen = -1;
	ora_rec->type.typbyval = false;
	ora_rec->type.typtype = TYPTYPE_ORA_RECORD;
	ora_rec->type.collation = 0;
	ora_rec->type.typisarray = false;
	ora_rec->type.atttypmod = -1;
	ora_rec->type.notnull = false;
	ora_rec->type.pctrowtypname = NULL;
	ora_rec->type.origtypname = NULL;
	ora_rec->type.tcache = NULL;
	ora_rec->type.tupdesc_id = 0;
	ora_rec->type.sqlcollectionoid = InvalidOid; /* ReqID:SRS-SQL-Collection */

	ora_rec->args = args;

	/* build a PLiSQL_datum_udt_type, and add it to plisql_Datums */
	datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));

	datum_type->dtype = PLISQL_DTYPE_TYPE;
	datum_type->udt_type = (PLiSQL_type *) ora_rec;
	datum_type->pkgoid = pkgoid;

	/* add udt datum */
	plisql_adddatum((PLiSQL_datum *) datum_type);
	plisql_ns_additem(PLISQL_NSTYPE_TYPE,
					datum_type->dno, refname);

	return (PLiSQL_type *) datum_type;
}

/*
 * Build oracle record variable, and optionally add it to namespace
 */
PLiSQL_row *
plisql_build_row_for_ora_record(const char *refname, int lineno,
					 PLiSQL_ora_record_type *dtype,
					 bool add2namespace)
{
	PLiSQL_row 		*row;
	int 			numcols;
	int 			i;
	ListCell 		*lc;
	PLiSQL_variable *var1;
	PLiSQL_type 	*newtype;

	numcols = list_length(dtype->args) + 1;

	row = palloc0(sizeof(PLiSQL_row));

	row->dtype = PLISQL_DTYPE_ORA_RECORD;
	row->refname = pstrdup(refname);
	row->lineno = lineno;
	row->nfields = numcols;
	row->fieldnames = palloc(numcols * sizeof(char*));
	row->varnos = palloc(numcols * sizeof(int));
	row->notnull = false;
	row->isconst = 0;
	row->default_val = NULL;
	row->datatype = (PLiSQL_type *) dtype;
	row->rowtupdesc = CreateTemplateTupleDesc(numcols);
	row->rowtupdesc->tdtypeid = ORARECORDOID;

	row->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

	newtype = plisql_build_datatype(UDTPOINTEROID, -1, InvalidOid, false);
	newtype->ttype = PLISQL_TTYPE_SCALAR;
	var1 = plisql_build_variable("__###__", 0, newtype, false);

	TupleDescInitEntry(row->rowtupdesc, 1,
					var1->refname,
					UDTPOINTEROID, -1,
					0);	
	row->fieldnames[0] = var1->refname;
	row->varnos[0] = var1->dno;

	i = 1;
	foreach(lc, dtype->args)
	{
		PLiSQL_variable 		*var;
		PLiSQL_ora_record_argitem	*argitem;
		char		refname[(NAMEDATALEN * 2) + 100];
		Oid			typoid = RECORDOID;
		int32		typmod = -1;
		Oid 		typcoll = InvalidOid;

		argitem = (PLiSQL_ora_record_argitem*)lfirst(lc);

		snprintf(refname, sizeof(refname), "%s.%s", dtype->type.typname, argitem->argname);

		var = plisql_build_variable(refname, lineno, argitem->type, false);

		if (argitem->defexpr != NULL)
		{
			((PLiSQL_var *) var)->default_val = argitem->defexpr;
		}
		else if (argitem->notnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("variable \"%s\" must have a default value, since it's declared NOT NULL",
							var->refname)));

		switch (var->dtype)
		{
			case PLISQL_DTYPE_VAR:
				typoid = ((PLiSQL_var *) var)->datatype->typoid;
				typmod = ((PLiSQL_var *) var)->datatype->atttypmod;
				typcoll = ((PLiSQL_var *) var)->datatype->collation;
				((PLiSQL_var *) var)->default_val = argitem->defexpr;
				((PLiSQL_var *) var)->notnull = argitem->notnull;
				break;
			case PLISQL_DTYPE_REC:
				/* Begin - BUG#Z8399 */
				typoid = ((PLiSQL_rec *) var)->datatype->typoid;
				typmod = ((PLiSQL_rec *) var)->datatype->atttypmod;
				/* End - BUG#Z8399 */
				break;
			case PLISQL_DTYPE_ROW:
				{
					typoid = ROWPOINTEROID;
					typmod = -1;
					((PLiSQL_row *) var)->default_val = argitem->defexpr;
				}
				break;
			case PLISQL_DTYPE_ORA_RECORD:
				typoid = ROWPOINTEROID;
				typmod = -1;				
				break;
			default:
				elog(ERROR, "unsupport dtype: %d", var->dtype);
		}

		((PLiSQL_var *) var)->notnull = argitem->notnull;

		row->fieldnames[i] = argitem->argname;
		row->varnos[i] = var->dno;
		TupleDescInitEntry(row->rowtupdesc, i + 1,
						   var->refname,
						   typoid,
						   typmod,
						   0);
		TupleDescInitEntryCollation(row->rowtupdesc, i + 1, typcoll);
		i++;
	}

	row->rowtupdesc = BlessTupleDesc(row->rowtupdesc);
	plisql_adddatum((PLiSQL_datum *) row);
	if (add2namespace)
		plisql_ns_additem(PLISQL_NSTYPE_ORA_RECORD, row->dno, row->refname);

	return row;
}

/*
 * parse a colname of a PLiSQL_ora_record var
 * When called from the compilation, the datums parameter should
 * be a global variable of plisql_Datums datums parameter,
 * when called from execution, it should be estate->datums
 */
PLiSQL_datum *
plisql_parse_ora_rec_var(PLiSQL_datum **datums,
					PLiSQL_row *ora_rec, List *names,
					int match_start_loc, bool noError)
{
	int 	list_size;
	int 	i;
	char 	*parse_name;

	list_size = list_length(names);
	Assert(match_start_loc < list_size);

	parse_name = strVal(list_nth(names, match_start_loc));

	for (i = 0; i < ora_rec->nfields; i++)
	{
		if (ora_rec->varnos[i] < 0)
			continue;
		if (strcmp(ora_rec->fieldnames[i], parse_name) == 0)
		{
			PLiSQL_datum 	*datum = datums[ora_rec->varnos[i]];

			if (match_start_loc == list_size - 1)
				return datum;
			else if (datum->dtype == PLISQL_DTYPE_ORA_RECORD)
				return plisql_parse_ora_rec_var(datums, (PLiSQL_row *) datum,
										names, match_start_loc + 1, noError);
			else if (datum->dtype == PLISQL_DTYPE_REC)
			{
				PLiSQL_rec *rec = (PLiSQL_rec *)datum;
				int 		i;

				match_start_loc += 1;
				parse_name = strVal(list_nth(names, match_start_loc));
				/* search for a datum referencing this field */
				i = rec->firstfield;
				while (i >= 0)
				{
					PLiSQL_recfield *fld = (PLiSQL_recfield *) datums[i];

					Assert(fld->dtype == PLISQL_DTYPE_RECFIELD);

					if (strcmp(fld->fieldname, parse_name) == 0 &&
						match_start_loc == list_size - 1)
						return (PLiSQL_datum *) fld;

					i = fld->nextfield;
				}
			}
			else if (datum->dtype == PLISQL_DTYPE_VAR && match_start_loc == list_size - 2)
			{
				PLiSQL_var 	*var = (PLiSQL_var *) datum;
				char 		*lastname = strVal(list_nth(names, list_size - 1));

				if (var->datatype->typoid == REFCURSOROID &&
					(strcmp(lastname, "isopen") == 0 ||
					strcmp(lastname, "found") == 0 ||
					strcmp(lastname, "notfound") == 0 ||
					strcmp(lastname, "rowcount") == 0))
					return datum;

				/* Begin - ReqID:SRS-PLSQL-Collection */
				if (IS_COLLECTOID(var->datatype->typoid) &&
					is_collect_methodname(lastname))
					return NULL;		
				/* End - ReqID:SRS-PLSQL-Collection */

				if (noError)
					elog(ERROR, "\"%s\" is a scalar variable", parse_name);
				return NULL;
			}
			else
			{
				if (noError)
					elog(ERROR, "\"%s\" is not a record var", parse_name);
				return NULL;
			}
		}
	}

	if (noError)
		elog(ERROR, "\"%s\" doesn't in record variable\"%s\"", parse_name,
								ora_rec->refname);
	return NULL;
}

static bool
MatchOraRecordNamedCall(ParseState *pstate,char *recname,
			   List *argnames,
			   List *orarecargs,
			   int	numposargs,
			   int **argnumbers,
			   bool othersargnames,
			   int location)
{
	ListCell   *lc;
	int			nrecfields;
	int			ap;				/* call args position */
	int			pp;				/* proargs position */
	bool		arggiven[FUNC_MAX_ARGS];
	int			numnameargs;

	Assert(numposargs >= 0);

	nrecfields = list_length(orarecargs);
	numnameargs = list_length(argnames);

	if (numposargs > nrecfields ||
		list_length(argnames) > nrecfields ||
		(numposargs + numnameargs) > nrecfields)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number or types of arguments in call to \"%s\"", recname),
				 parser_errposition(pstate, location)));

	/* initialize state for matching */
	*argnumbers = (int *) palloc0(nrecfields * sizeof(int));
	memset(arggiven, false, nrecfields * sizeof(bool));

	/* there are numposargs positional args before the named args */
	for (ap = 0; ap < numposargs; ap++)
	{
		(*argnumbers)[ap] = ap;
		arggiven[ap] = true;
	}

	/* now examine the named args */
	foreach(lc, argnames)
	{
		char	   *argname = (char *) lfirst(lc);
		bool		found;
		ListCell   *l;

		pp = 0;
		found = false;
		foreach(l, orarecargs)
		{
			char	   *fieldname = ((PLiSQL_ora_record_argitem *) lfirst(l))->argname;
			if (pg_strcasecmp(fieldname, argname) == 0)
			{
				/* fail if argname matches a positional argument */
				if (arggiven[pp])
					return false;
				arggiven[pp] = true;
				(*argnumbers)[ap] = pp;
				found = true;
				break;
			}
			/* increase pp only for considered parameters */
			pp++;
		}
		/* if name isn't in proargnames, fail */
		if (!found)
			return false;
		ap++;
	}

	for (; ap < nrecfields; ap++)
		(*argnumbers)[ap] = ap;


	return true;
}

/*
 * like plisql_param_ref
 * we handle Oracle record type construct
 */
Node *
plisql_ora_record_construct_ref(ParseState *pstate, List *recname, List *fargs,
				  FuncCall *fn, int location)
{
	PLiSQL_expr *expr = (PLiSQL_expr *) pstate->p_ref_hook_state;
	PLiSQL_nsitem *nse = NULL;
	char *ora_rec_name = NULL;
	char *name1 = NULL;
	char *name2 = NULL;
	char *name3 = NULL;
	int list_len = list_length(recname);
	int	len_recargs;
	PLiSQL_ora_record_type *ora_rec_type;
	List		*orarecargs;
	ListCell 	*l = NULL;
	ListCell 	*lc = NULL;
	List	   *argnames;
	bool		othersargnames;
	int			numposargs;
	int		   *argnumbers = NULL;
	ParseCallbackState pcbstate;
	OraRowExpr *rowexpr;	/* BUG#7712 */
	Node	   *retval;

	switch(list_len)
	{
		case 1:
			name1 = strVal(linitial(recname));
			ora_rec_name = name1;
			break;
		case 2:
			name1 = strVal(linitial(recname));
			name2 = strVal(lsecond(recname));
			ora_rec_name = name2;
			break;
		case 3:
			name1 = strVal(linitial(recname));
			name2 = strVal(lsecond(recname));
			name3 = strVal(list_nth(recname, 2));
			ora_rec_name = name3;
			break;
		default:
			name1 = strVal(linitial(recname));
			name2 = strVal(lsecond(recname));
			name3 = strVal(list_nth(recname, 2));
			ora_rec_name = strVal(list_nth(recname, list_len - 1));
			break;
	}

	nse = plisql_ns_lookup(expr->ns, false,
							name1, name2, name3,
							NULL);

	/* Begin - OA-REQ-2466 */
	if (nse == NULL)
	{
		PkgType *pkgtype;

		pkgtype = LookupPkgTypeByTypename(recname, true);
		if (pkgtype != NULL)
		{
			ora_rec_type = (PLiSQL_ora_record_type *) pkgtype->value;
			pfree(pkgtype);
			if (ora_rec_type == NULL)
				return NULL;
		}
		else
			return NULL;
	}
	/* End - OA-REQ-2466 */
	else if (nse->itemtype == PLISQL_NSTYPE_TYPE)
	{
		PLiSQL_datum_udt_type *datum_type = NULL;

		datum_type = (PLiSQL_datum_udt_type *) expr->func->datums[nse->itemno];
		/* Begin - ReqID:SRS-SQL-Collection */
		if (datum_type->udt_type->ttype != PLISQL_TTYPE_ORARECORD_TYPE)
			return NULL;
		/* End - ReqID:SRS-SQL-Collection */

		ora_rec_type = (PLiSQL_ora_record_type *)(datum_type->udt_type);
	}
	else
		return NULL;

	orarecargs = ora_rec_type->args;
	len_recargs = list_length(orarecargs);

	/*
	 * Oracle records do not have
	 * more than max_record_fields parameters.  We have to test here to protect
	 * against array overruns, etc.
	 */
	if (list_length(fargs) > max_record_fields /* OA-REQ-2466 */)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
				 errmsg_plural("cannot pass more than %d argument to a record",
							   "cannot pass more than %d arguments to a record",
							   max_record_fields /* OA-REQ-2466 */,
							   max_record_fields /* OA-REQ-2466 */),
				 parser_errposition(pstate, location)));

	/*
	 * Check for named arguments; if there are any, build a list of names.
	 *
	 * We allow mixed notation (some named and some not), but only with all
	 * the named parameters after all the unnamed ones.  So the name list
	 * corresponds to the last N actual parameters and we don't need any extra
	 * bookkeeping to match things up.
	 */
	argnames = NIL;
	othersargnames = false;
	numposargs = 0;

	foreach(l, fargs)
	{
		Node	   *arg = lfirst(l);

		if (IsA(arg, NamedArgExpr))
		{
			NamedArgExpr *na = (NamedArgExpr *) arg;
			ListCell   *lc;

			if (pg_strcasecmp(na->name, "OTHERS") == 0)
			{
				if (othersargnames)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("\"others\" choice is not the last named argument in this type constructor"),
							 parser_errposition(pstate, na->location)));
				othersargnames = true;
			}
			else
			{

				/* Reject duplicate arg names */
				foreach(lc, argnames)
				{
					if (strcmp(na->name, (char *) lfirst(lc)) == 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("multiple instances of named argument in list"),
								 parser_errposition(pstate, na->location)));
				}
				argnames = lappend(argnames, na->name);
			}
		}
		else
		{
			if (othersargnames)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("\"others\" choice is not the last named argument in this type constructor"),
						 parser_errposition(pstate, exprLocation(arg))));

			if (argnames != NIL)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("a positional parameter association may not follow a named association"),
						 parser_errposition(pstate, exprLocation(arg))));
			numposargs++;
		}
	}

	/* Check for argument name match, generate positional mapping */
	if (argnames != NIL || othersargnames)
	{
		if (!MatchOraRecordNamedCall(pstate, ora_rec_name, 
									argnames,
									orarecargs,
									numposargs,
									&argnumbers,
									othersargnames,
									location))
			return NULL;
	}

	setup_parser_errposition_callback(&pcbstate, pstate, location);

	/*
	 * If processing named args, return actual argument positions into
	 * NamedArgExpr nodes in the fargs list.  This is a bit ugly but not
	 * worth the extra notation needed to do it differently.
	 */
	if (argnumbers != NULL)
	{
		int 		i;
		ListCell   *lc;

		i = numposargs;
		foreach(lc, fargs)
		{
			NamedArgExpr *na = (NamedArgExpr *) lfirst(lc);
	
			if (IsA(na, NamedArgExpr))
			{
				na->argnumber = argnumbers[i];
				i++;
			}
		}
	}

	/* Begin - BUG#7712 */
	rowexpr = makeNode(OraRowExpr);
	rowexpr->row_typeid = ORARECORDOID;
	rowexpr->type = PointerGetDatum(ora_rec_type);
	/* End - BUG#7712 */
	rowexpr->row_format = COERCE_IMPLICIT_CAST;
	rowexpr->location = -1;

	foreach(lc, orarecargs)
	{
		PLiSQL_ora_record_argitem *recfield = (PLiSQL_ora_record_argitem *) lfirst(lc);
	
		rowexpr->colnames = lappend(rowexpr->colnames, makeString(recfield->argname));
	}

	if (argnames == NIL && !othersargnames)
	{
		/* only positional notaion */
		rowexpr->args = list_copy(fargs);
	}
	else if (argnames == NIL && othersargnames)
	{
		List 		*newargs = NIL;
		int			i;

		newargs = list_copy(fargs);

		/* the last arg must be OTHERS notaion */
		newargs = list_delete_last(newargs);
		for (i = numposargs; i < len_recargs; i++)
			newargs = lappend(newargs, ((NamedArgExpr *)llast(fargs))->arg);
		rowexpr->args = newargs;
	}
	else if (argnames != NIL)
	{
		/* there are name notations */
		List 		*newargs = NIL;
		int			i;
		ListCell 	*l;
		int			j;
		bool		found = false;

		for(i = 0; i < numposargs; i++)
			newargs = lappend(newargs, list_nth(fargs, i));

		j = numposargs;
		while(1)
		{
			if (j == len_recargs)
				break;

			found = false;
			foreach(l, fargs)
			{
				Node	   *arg = lfirst(l);

				if (IsA(arg, NamedArgExpr))
				{
					NamedArgExpr *na = (NamedArgExpr *) arg;
					if (na->argnumber == j)
					{
						newargs = lappend(newargs, na->arg);
						j++;
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				if (othersargnames)
				{
					newargs = lappend(newargs, ((NamedArgExpr *)llast(fargs))->arg);
					j++;
				}
				else
				{
					/*
					 * There is no OTHERS notaion, execpt positional and name notations,
					 * there are field not given values, set their default value is 0
					 */
					Const	*con;

					con = makeConst(UNKNOWNOID,
								-1,
								InvalidOid,
								-2,
								(Datum) 0,
								true,
								false);
					con->location = location;
					newargs = lappend(newargs, (Node *) con);
					j++;
				}
			}
		}

		rowexpr->args = newargs;	
	}

	cancel_parser_errposition_callback(&pcbstate);

	if (argnumbers != NULL)
		pfree(argnumbers);
	if (argnames != NIL)
		list_free(argnames);

	retval = (Node *) rowexpr;

	return retval;
}

/*
 * build a tupledesc from Oracle record type
 */
TupleDesc
plisql_build_tupdesc_from_recordtype(PLiSQL_ora_record_type *ora_rec_type)
{
	TupleDesc	destDesc;
	int 		numcols;
	PLiSQL_ora_record_argitem *argitem;
	ListCell	*cell;
	int			i = 0;

	Assert(ora_rec_type->args != NULL);

	numcols = list_length(ora_rec_type->args) + 1;
	destDesc = CreateTemplateTupleDesc(numcols);
	destDesc->tdtypeid = ORARECORDOID;

	TupleDescInitEntry(destDesc, 1,
					"__###__",
					UDTPOINTEROID, -1,
					0);
	i = 1;

	foreach(cell, ora_rec_type->args)
	{
		argitem = (PLiSQL_ora_record_argitem *) lfirst(cell);

		TupleDescInitEntry(destDesc, i + 1,
						argitem->argname,
						argitem->type->typoid,
						argitem->type->atttypmod,
						0);
		i++;
	}
	return destDesc;
}
/* End - ReqID:SRS-PLSQL-RECORD */

/* Begin - BUG#Z1322 */
/*
 * plisql_check_maxnum_record_nest
 * check if the maximum number of layers that record nest
 * is greater than PLISQL_MAXNUM_NESTED_RECORD
 */
void
plisql_check_maxnum_record_nest(int num_nested, int record_begin_pos)
{
if (num_nested - record_begin_pos > PLISQL_MAXNUM_NESTED_RECORD)
	ereport(ERROR,
		(errcode(ERRCODE_SYNTAX_ERROR),
		errmsg("The maximum number of layers that record can nest is %d", PLISQL_MAXNUM_NESTED_RECORD)));
}
/* End - BUG#Z1322 */

/* Begin - ReqID:SRS-PLSQL-Collection */
/*
 * get the datatype of a record filed name
 */
PLiSQL_type *
get_record_field_type(PLiSQL_type *data_type, char *name)
{
	PLiSQL_ora_record_type *ora_rec_type = (PLiSQL_ora_record_type *) data_type;
	ListCell	*lc;

	foreach(lc, ora_rec_type->args)
	{
		PLiSQL_ora_record_argitem	*argitem = (PLiSQL_ora_record_argitem*)lfirst(lc);

		 if (strcmp(name, argitem->argname) == 0)
			return argitem->type;
	}

	return NULL;
}

/*
 * get the datum according the idents
 */
PLwdatum *
plisql_get_ora_record_field(PLiSQL_datum *datum,
					List *idents,
					int nlen,
					int nth,
					int loc)
{
	PLiSQL_row	*ora_rec;
	int 		i;
	char		*word;
	PLwdatum 	*wdatum;

	ora_rec = (PLiSQL_row *) datum;
	word = strVal(list_nth(idents, nth));
	wdatum = palloc0(sizeof(PLwdatum));

	for (i = 0; i < ora_rec->nfields; i++)
	{
		if (ora_rec->fieldnames[i] &&
			pg_strcasecmp(ora_rec->fieldnames[i], word) == 0)
		{
			PLiSQL_datum	*datum;

			datum = plisql_Datums[ora_rec->varnos[i]];
			if (datum->dtype == PLISQL_DTYPE_ORA_RECORD)
			{
				if (nth == nlen - 1)
				{
					wdatum->datum = datum;
					wdatum->ident = NULL;
					wdatum->quoted = false; /* not used */
					wdatum->idents = idents;
					return wdatum;
				}
				else
				{
					nth++;
					pfree(wdatum);
					return plisql_get_ora_record_field(datum, idents, nlen, nth, loc);
				}
			}
			else if (datum->dtype == PLISQL_DTYPE_REC)
			{
				if (nth == nlen - 1)
				{
					wdatum->datum = datum;
					wdatum->ident = NULL;
					wdatum->quoted = false; /* not used */
					wdatum->idents = idents;
					return wdatum;
				}
				else
				{

					PLiSQL_rec *rec = (PLiSQL_rec *)datum;
					int 		i;

					nth++;
					word = strVal(list_nth(idents, nth));

					/* search for a datum referencing this field */
					i = rec->firstfield;
					while (i >= 0)
					{
						PLiSQL_recfield *fld = (PLiSQL_recfield *) plisql_Datums[i];

						Assert(fld->dtype == PLISQL_DTYPE_RECFIELD);

						if (strcmp(fld->fieldname, word) == 0 &&
							nth == nlen - 1)
						{
							wdatum->datum = (PLiSQL_datum *)fld;
							wdatum->ident = NULL;
							wdatum->quoted = false; /* not used */
							wdatum->idents = idents;
							return wdatum;
						}

						i = fld->nextfield;
					}

				}
			}
			else if (datum->dtype == PLISQL_DTYPE_VAR &&
				((PLiSQL_var *)datum)->datatype->ttype == PLISQL_TTYPE_COLLECT_TYPE &&
				nth == nlen - 2 &&
				is_collect_methodname(strVal(list_nth(idents, nth + 1))))
			{
				pfree(wdatum);
				return NULL;
			}
			else if (nth == nlen - 1)
			{
				wdatum->datum = datum;
				wdatum->ident = NULL;
				wdatum->quoted = false; /* not used */
				wdatum->idents = idents;
				return wdatum;
			}
			else
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					plisql_scanner_errposition(loc)));
		}
	}
	pfree(wdatum);

	return NULL;
}
/* End - ReqID:SRS-PLSQL-Collection */
