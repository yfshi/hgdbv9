/*-------------------------------------------------------------------------
 *
 * set_user_var.c
 *	  Implenmentation of the MySQL SET functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/set/set_user_var.c
 *
 *-------------------------------------------------------------------------
 */
#include "c.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "access/detoast.h"
#include "../include/mysset.h"
#include "catalog/pg_type.h"
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "executor/tstoreReceiver.h"
#include "mb/pg_wchar.h"
#include "nodes/plannodes.h"
#include "nodes/nodes.h"
#include "nodes/nodeFuncs.h"
#include "parser/analyze.h"
#include "parser/parser.h"
#include "parser/parse_type.h"
#include "parser/scansup.h"
#include "rewrite/rewriteHandler.h"
#include "tcop/dest.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/float.h"
#include "utils/hsearch.h"
#include "utils/int8.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"
#include "utils/varbit.h"
#include "nodes/makefuncs.h"
#include "utils/syscache.h"
#include "utils/datum.h"
#include "optimizer/clauses.h"
#include "mb/pg_wchar.h"

#define USER_VARIABLE_MAX_NUM 1024

/* This strict represents a user-defined variable,
 * stored in the hash table mysql_user_variables.
 */
typedef struct MysqlUserVariableItem
{
	/* dynahash.c requires key to be first field */
	char varName[NAMEDATALEN];
	Const *varValue;
	bool isnull;
} MysqlUserVariableItem;

static HTAB *mysql_user_variables = NULL;

PG_FUNCTION_INFO_V1(mysSetUserVar);
PG_FUNCTION_INFO_V1(return_all_user_variable);

static void initMysqlUserVariableHashTable(void);


/* mys_get_call_expr_typemod
 *
 * Get the typemod and collate of the specified parameter.
 */
static void
mys_get_call_expr_typemod_and_colloid(FmgrInfo *flinfo,
									  int argnum,
									  int *argtypemod,
									  Oid *argcolloid)
{
	List	   *args;
	Node	   *expr;

	if (!flinfo || !flinfo->fn_expr)
	{
		*argtypemod = -1;
		*argcolloid = -1;
		return;
	}

	expr = flinfo->fn_expr;
	if (expr == NULL)
	{
		*argtypemod = -1;
		*argcolloid = -1;
		return;
	}

	if (IsA(expr, FuncExpr))
		args = ((FuncExpr *) expr)->args;
	else if (IsA(expr, OpExpr))
		args = ((OpExpr *) expr)->args;
	else if (IsA(expr, DistinctExpr))
		args = ((DistinctExpr *) expr)->args;
	else if (IsA(expr, ScalarArrayOpExpr))
		args = ((ScalarArrayOpExpr *) expr)->args;
	else if (IsA(expr, NullIfExpr))
		args = ((NullIfExpr *) expr)->args;
	else if (IsA(expr, WindowFunc))
		args = ((WindowFunc *) expr)->args;
	else
	{
		*argtypemod = -1;
		*argcolloid = -1;
		return;
	}

	if (argnum < 0 || argnum >= list_length(args))
	{
		*argtypemod = -1;
		*argcolloid = -1;
		return;
	}

	*argtypemod = exprTypmod((Node *) list_nth(args, argnum));
	*argcolloid = exprCollation((Node *) list_nth(args, argnum));
}

/* mysSetUserVarInternal
 *
 * The storage of mysql custom variables is stored in
 * the hash table mysql_user_variables,If it exists,the
 * value is updated.if it does not exist, it is directly
 * stored.
 *
 * Parameter
 * userVarName:user defined variable name.
 * consttype:variable type.
 * consttypmod:variable typemode
 * constcollid:variable collate
 * constlen:typelen
 * constvalue:variable value,there is a datum.
 * constisnull:value is null or not，true is null.
 * constbyval:passing by value.
 */
void
mysSetUserVarInternal(char *userVarName,
					  Oid consttype,
					  int32 consttypmod,
					  Oid constcollid,
					  int constlen,
					  Datum constvalue,
					  bool constisnull,
					  bool constbyval)
{
	MysqlUserVariableItem *entry;
	bool			found;
	MemoryContext	oldContext;
	Const		   *con;
	Datum			savedatum = (Datum)0;

	/* Check user variable exceed the allowed number */
	if (mysql_user_variables != NULL &&
		hash_get_num_entries(mysql_user_variables) >= USER_VARIABLE_MAX_NUM)
	{
		entry = (MysqlUserVariableItem *) hash_search(mysql_user_variables,
							userVarName,
							HASH_FIND,
							&found);
		if (found)
		{
			found = false;
			entry = NULL;
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_LIMIT_VALUE),
					 errmsg("user variable exceed the allowed number")));
		}
	}

	if (userVarName != NULL)
	{
		int		charlen;

		charlen = pg_mbstrlen_with_len(userVarName, strlen(userVarName));
		if (charlen > NAMEDATALEN)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_NAME),
					 errmsg("user variable name \'%s\' is illegal", userVarName)));
		}

		userVarName = downcase_truncate_identifier(userVarName, strlen(userVarName), false);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_NAME),
				 errmsg("user variable name '' is illegal")));
	}

	/* Initialize the hash table, if necessary */
	if (!mysql_user_variables)
		initMysqlUserVariableHashTable();

	oldContext = MemoryContextSwitchTo(TopMemoryContext);

	if (!constisnull)
	{
		savedatum = datumCopy(constvalue, constbyval, constlen);
	}

	con = makeConst(consttype, consttypmod, constcollid, constlen, savedatum, constisnull, constbyval);
	entry = (MysqlUserVariableItem *) hash_search(mysql_user_variables,
							userVarName,
							HASH_ENTER,
							&found);
	if (found)
	{
		if (!entry->isnull)
		{
			Const *sconst = entry->varValue;
			if (!sconst->constbyval)
			{
				pfree(DatumGetPointer(sconst->constvalue));
			}
			pfree(entry->varValue);
			entry->varValue = NULL;
		}
	}

	entry->isnull = constisnull;
	entry->varValue = (Const *)copyObject(con);

	pfree(con);

	MemoryContextSwitchTo(oldContext);
}

/* initMysqlUserVariableHashTable
 *
 * Initalize the hash table of user-defined variable.
 */
static void
initMysqlUserVariableHashTable(void)
{
	HASHCTL		hash_ctl;

	hash_ctl.keysize = NAMEDATALEN;
	hash_ctl.entrysize = sizeof(MysqlUserVariableItem);
	hash_ctl.hcxt = TopMemoryContext;

	mysql_user_variables = hash_create("MySQL User Variables",
									   USER_VARIABLE_MAX_NUM,
									   &hash_ctl,
									   HASH_ELEM | HASH_STRINGS | HASH_CONTEXT);
}

/* mysSetUserVar
 *
 * set user-defined variable to hash table mysql_user_variables.
 */
Datum
mysSetUserVar(PG_FUNCTION_ARGS)
{
	char *varName;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_NAME),
				 errmsg("user variable name '' is illegal")));
	}

	varName = TextDatumGetCString(PG_GETARG_DATUM(0));

	if (!PG_ARGISNULL(1))
	{
		Datum	datumValue = PG_GETARG_DATUM(1);
		Oid		varType = get_fn_expr_argtype(fcinfo->flinfo, 1);
		int32	typmod = -1;
		Oid		colloid = InvalidOid;
		bool	typebyval = true;
		Type	argType;
		int16	typlen;

		/*
		 * By value type, calculate type length, whether to
		 * pass by value.
		 */
		argType = typeidType(varType);
		typebyval = typeByVal(argType);
		typlen = typeLen(argType);
		/* Get the typmod and collate of the value */
		mys_get_call_expr_typemod_and_colloid(fcinfo->flinfo, 1, &typmod, &colloid);
		if (colloid == InvalidOid)
			colloid = typeTypeCollation(argType);

		mysSetUserVarInternal(varName, varType, typmod, colloid, typlen, datumValue, false, typebyval);
		ReleaseSysCache(argType);
	}
	else
	{
		mysSetUserVarInternal(varName, UNKNOWNOID, -1, InvalidOid, -1, (Datum)0, true, false);
		PG_RETURN_NULL();
	}

	PG_RETURN_DATUM(PG_GETARG_DATUM(1));
}

/* userVariable_Hash_Search
 *
 * Returns the node of the user-defined varible by the
 * name of the user-defined variable.
 */
Node *
userVariable_Hash_Search(ParseState *pstate, VarRef *varRef)
{
	bool found = false;
	Const *con;
	MysqlUserVariableItem *entry;
	MemoryContext oldcontext;

	if (mysql_user_variables)
	{
		oldcontext = MemoryContextSwitchTo(TopMemoryContext);
		if (varRef->varname != NULL)
			varRef->varname = downcase_truncate_identifier(varRef->varname,
														   strlen(varRef->varname), true);

		entry = (MysqlUserVariableItem *)hash_search(mysql_user_variables,
													 varRef->varname,
													 HASH_FIND,
													 &found);
		if (found)
			con = (Const *)copyObject((Const *)entry->varValue);
		MemoryContextSwitchTo(oldcontext);
	}

	if (!found)
		con = makeConst(UNKNOWNOID, -1, InvalidOid, -1, (Datum)0, true, false);
	else
	{
		Const *tmpcon;

		tmpcon = (Const *)copyObject((Const *)con);
		/* Free the node cipied in TopMemoryContext */
		pfree(con);

		con = tmpcon;
	}

	return (Node *)con;
}

/* return_all_user_variable
 *
 * Returns all user defined varible by user
 * defined.
 */
Datum
return_all_user_variable(PG_FUNCTION_ARGS)
{
	TupleDesc tupdesc;
	MysqlUserVariableItem *entry;
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	MemoryContext	oldcontext;
	Tuplestorestate *tupstore;

	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
	tupdesc = CreateTemplateTupleDesc(3, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "THREAD_ID", INT4OID, 0, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "VARIABLE_NAME", VARCHAROID, 64, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "VARIABLE_VALUE", VARCHAROID, 1024, 0);
	tupstore =
		tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random,
							  false, work_mem);
	MemoryContextSwitchTo(oldcontext);

	if (mysql_user_variables)
	{
		HASH_SEQ_STATUS scan_status;

		hash_seq_init(&scan_status, mysql_user_variables);
		while ((entry = (MysqlUserVariableItem *) hash_seq_search(&scan_status)) != NULL)
		{
			Datum		values[3];
			bool		nulls[3] = {false, false, false};
			char	   *value = "";

			if (entry->varValue->constisnull)
				nulls[2] = true;
			else
			{
				Oid		typOutput;
				bool	typIsVarlena;

				getTypeOutputInfo(entry->varValue->consttype, &typOutput, &typIsVarlena);
				value = OidOutputFunctionCall(typOutput, entry->varValue->constvalue);
			}
			values[0] = Int32GetDatum(MyProcPid);
			values[1] = CStringGetTextDatum(entry->varName);
			values[2] = CStringGetTextDatum(value);

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}

	}

	tuplestore_donestoring(tupstore);

	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	return (Datum) 0;
}
