/*-------------------------------------------------------------------------
 *
 * File:pl_udt_exception.h
 *
 * Abstract:
 *		Definitions for the PLISQL
 *			  procedural language user define exception type
 *
 * ReqID:SRS-PLSQL-EXCEPTION
 *
 * Authored by dwdai@highgo.com,20240806.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_udt_exception.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include <ctype.h>

#include "plisql.h"
#include "pl_udt_exception.h"

#include "catalog/pg_namespace.h"
#include "access/tupdesc.h"
#include "access/relscan.h"
#include "access/relation.h"
#include "access/genam.h"
#include "access/table.h"
#include "access/skey.h"
#include "utils/lsyscache.h"
#include "utils/fmgroids.h"
#include "utils/builtins.h"
#include "utils/snapmgr.h"
#include "utils/memutils.h"


/*
 * global error stack
 */
typedef struct Errorstack
{
	int sqlcode;
	char *errmsg;
	struct Errorstack *prev;
}  Errorstack;

static Errorstack *cur_error_stack = NULL;
static int current_error_stack_deep = 0;


static List* ora_exceptionmap(int oraerrcode, char *pgqlcode,
										int mapflag, TupleDesc *tupledesc);




/*
 * find map errcode from _ivorysql_ora_exception_map table
 */
static List*
ora_exceptionmap(int oraerrcode, char *pgqlcode,
						int mapflag, TupleDesc *tupdesc)
{
	Snapshot snapshot;
	IndexScanDesc indexScan;
	Relation rel;
	Relation indexrel;
	HeapTuple tuple = NULL;
	Oid	ora_exception_map_tableid;
	TupleDesc desc;
	Oid		indexOid = InvalidOid;
	Oid		sysoid = PG_SYS_NAMESPACE;
	ScanKeyData scankeys[1];
	List	*ret = NIL;
	TupleTableSlot *existing_slot;

	if (tupdesc != NULL)
		*tupdesc = NULL;

	ora_exception_map_tableid = get_relname_relid(ORA_EXCEPTION_MAP_TABLE_NAME, sysoid);
	if (!OidIsValid(ora_exception_map_tableid))
	{
		elog(WARNING, "relation \"%s\" does not exist", ORA_EXCEPTION_MAP_TABLE_NAME);
		return NULL;
	}
	rel = table_open(ora_exception_map_tableid, AccessShareLock);
	desc = RelationGetDescr(rel);

	switch (mapflag)
	{
		case ORA_EXCEPTION_MAP_BY_PGCODE:
			indexOid = get_relname_relid(ORA_EXCEPTION_MAP_PGCODE_INDEX_NAME,
										sysoid);
			ScanKeyInit(&scankeys[0],
						1,
						HTEqualStrategyNumber,
						F_TEXTEQ,
						CStringGetTextDatum(pgqlcode));
			break;
		case ORA_EXCEPTION_MAP_BY_ORACODE:
			indexOid = get_relname_relid(ORA_EXCEPTION_MAP_ORACODE_INDEX_NAME,
										sysoid);
			ScanKeyInit(&scankeys[0],
						1,
						HTEqualStrategyNumber,
						F_INT4EQ,
						Int32GetDatum(oraerrcode));
			break;
		default:
			elog(ERROR, "invalid exception map flag");
			break;
	}

	Assert(OidIsValid(indexOid));
	indexrel = index_open(indexOid, AccessShareLock);
	Assert(indexrel->rd_index->indnatts == 1);
	/*
	 * there,we use GetCataLogSanspShot not GetLatestSnapshot
	 * for Parallel execute plisql function
	 */
	snapshot = RegisterSnapshot(GetCatalogSnapshot(ora_exception_map_tableid));
	/* scan tuple from index */
	indexScan = index_beginscan(rel, indexrel, snapshot, 1, 0);
	index_rescan(indexScan, &scankeys[0], 1, NULL, 0);

	existing_slot = table_slot_create(rel, NULL);
	while (index_getnext_slot(indexScan, ForwardScanDirection, existing_slot))
	{
		tuple = ExecFetchSlotHeapTuple(existing_slot, false, NULL);

		/* recheck tuple is check */
		if (indexScan->xs_recheck)
		{
			Datum datum;
			bool isnull = false;
			bool	match = false;

			if (mapflag == ORA_EXCEPTION_MAP_BY_PGCODE)
			{
				datum = heap_getattr(tuple, Anum_ivorysql_ora_exceptionmap_pg_sqlcode_idx, desc, &isnull);
				Assert(!isnull);
				if (strcmp(pgqlcode, TextDatumGetCString(datum)) == 0)
					match = true;
			}
			else if (mapflag == ORA_EXCEPTION_MAP_BY_ORACODE)
			{
				datum = heap_getattr(tuple, Anum_ivorysql_ora_exceptionmap_ora_errcode_idx, desc, &isnull);
				Assert(!isnull);
				if (oraerrcode == DatumGetInt32(datum))
					match = true;
			}

			if (!match)
				continue;
		}
		tuple = heap_copytuple(tuple);
		ret = lappend(ret, tuple);
		if (*tupdesc == NULL)
			*tupdesc = CreateTupleDescCopy(desc);
	}
	index_endscan(indexScan);
	UnregisterSnapshot(snapshot);
	ExecDropSingleTupleTableSlot(existing_slot);

	index_close(indexrel, AccessShareLock);
	table_close(rel, AccessShareLock);

	return ret;
}


/*
 * init error statkc
 */
void
init_plisql_error_stack(void)
{
	int i;

	for (i = 0; i < current_error_stack_deep; i++)
	{
		Errorstack *saved_stack;

		if (cur_error_stack->errmsg != NULL)
			pfree(cur_error_stack->errmsg);
		saved_stack = cur_error_stack;
		cur_error_stack = cur_error_stack->prev;
		pfree(saved_stack);
	}

	current_error_stack_deep = 0;
	Assert(cur_error_stack == NULL);

	return;
}


/*
 * get current global sqlcode
 */
int
get_current_sqlcode_sqlerrm(char **psqlerrm)
{
	if (psqlerrm != NULL)
		*psqlerrm = NULL;

	if (current_error_stack_deep == 0)
		return 0;

	Assert(cur_error_stack != NULL);
	if (psqlerrm != NULL)
		*psqlerrm = cur_error_stack->errmsg;

	return cur_error_stack->sqlcode;
}


/*
 * set current global sqlcode
 */
void
push_error_stack(int newcode, char *newerrm)
{
	Errorstack *current_stack = cur_error_stack;
	Errorstack *new_stack;
	MemoryContext oldcxt;

	oldcxt = MemoryContextSwitchTo(TopMemoryContext);

	new_stack = palloc0(sizeof(Errorstack));

	/* assign sqlerrm */
	if (newerrm != NULL)
	{
		new_stack->errmsg = pstrdup(newerrm);
	}

	new_stack->sqlcode = newcode;

	new_stack->prev = current_stack;

	cur_error_stack = new_stack;
	current_error_stack_deep++;
	MemoryContextSwitchTo(oldcxt);
}


/*
 * pop error stack
 */
void
pop_error_stack(void)
{
	Errorstack *current_stack;

	Assert(cur_error_stack != NULL);

	current_stack = cur_error_stack->prev;

	if (cur_error_stack->errmsg != NULL)
		pfree(cur_error_stack->errmsg);
	pfree(cur_error_stack);
	cur_error_stack = current_stack;
	current_error_stack_deep--;
}


/*
 * replace current error_stack sqlcode
 * and sqlerrm
 */
bool
set_current_error_stack(int newcode, char *newerrm,
									int *oldcode, char **olderrm,
									bool shoud_pop,
									bool should_push)
{
	bool	ret = false;
	MemoryContext oldcxt;

	oldcxt = MemoryContextSwitchTo(TopMemoryContext);

	if (cur_error_stack == NULL)
	{
		if (!should_push)
			return ret;

		ret = true;
		cur_error_stack = palloc0(sizeof(Errorstack));
		cur_error_stack->sqlcode = newcode;
		if (newerrm != NULL)
			cur_error_stack->errmsg = pstrdup(newerrm);
		current_error_stack_deep++;
	}
	else
	{
		if (shoud_pop)
		{
			Errorstack *newentry = cur_error_stack->prev;

			if (cur_error_stack->errmsg != NULL)
				pfree(cur_error_stack->errmsg);
			pfree(cur_error_stack);
			cur_error_stack = newentry;
			current_error_stack_deep--;
		}
		if (cur_error_stack != NULL)
		{
			if (oldcode != NULL)
				*oldcode = cur_error_stack->sqlcode;
			if (olderrm != NULL &&
				cur_error_stack->errmsg != NULL)
				*olderrm = MemoryContextStrdup(oldcxt,
								cur_error_stack->errmsg);
			cur_error_stack->sqlcode = newcode;
			if (cur_error_stack->errmsg != NULL)
				pfree(cur_error_stack->errmsg);
			cur_error_stack->errmsg = NULL;
			if (newerrm != NULL)
				cur_error_stack->errmsg = pstrdup(newerrm);
		}
	}
	MemoryContextSwitchTo(oldcxt);

	return ret;
}



/*
 * pop error stack by curr_dep
 */
void
pop_error_stack_bydeep(int curr_dep)
{
	Errorstack *current_stack;

	while (current_error_stack_deep > curr_dep)
	{
		current_stack = cur_error_stack->prev;

		if (cur_error_stack->errmsg != NULL)
			pfree(cur_error_stack->errmsg);
		pfree(cur_error_stack);
		cur_error_stack = current_stack;
		current_error_stack_deep--;
	}
}



/*
 * get current_error_stack
 */
int
get_current_error_stack_deep(void)
{
	return current_error_stack_deep;
}


/*
 * format error_stack_msg
 */
char *
format_error_stack_msg(void)
{
	StringInfoData errmsg;
	Errorstack *curr_stack = cur_error_stack;

	if (current_error_stack_deep == 0)
		return NULL;

	initStringInfo(&errmsg);
	while (curr_stack != NULL)
	{
		/* udt exception errmsg is not the same as errmsg */
		if (curr_stack->sqlcode == PLISQL_UDT_EXCEPTION_DEFAULT_SQLCODE)
			appendStringInfo(&errmsg, "%s\n",
					"IVY-06510: PL/SQL: unhandled user-defined exception");
		else if (curr_stack->errmsg != NULL)
			appendStringInfo(&errmsg, "%s\n", curr_stack->errmsg);
		curr_stack = curr_stack->prev;
	}

	return errmsg.data;
}




/*
 * get match oraerrcode for match
 * pgerrcode
 */
int
plisql_get_exception_oraerrcode(MemoryContext stmt_txt,
											int pgerrcode,
											char *origin_msg,
											char **errmsg)
{
	HeapTuple maptuple = NULL;
	TupleDesc desc = NULL;
	int result = pgerrcode;
	List	*tuplist = NIL;
	ListCell *lc;
	MemoryContext oldctx;

	if (errmsg != NULL)
		*errmsg = origin_msg;

	oldctx = MemoryContextSwitchTo(stmt_txt);
	tuplist = ora_exceptionmap(0,
							unpack_sql_state(pgerrcode),
							ORA_EXCEPTION_MAP_BY_PGCODE, &desc);

	/* maybe handle list in further */
	if (tuplist != NIL)
		maptuple = (HeapTuple) linitial(tuplist);

	if (maptuple != NULL)
	{
		bool isnull;
		Datum datum;

		datum = heap_getattr(maptuple,
							Anum_ivorysql_ora_exceptionmap_ora_errcode_idx,
							desc,
							&isnull);
		Assert(!isnull);
		result = Int32GetDatum(datum);
		if (errmsg != NULL)
		{
			datum = heap_getattr(maptuple,
							Anum_ivorysql_ora_exceptionmap_message_idx,
							desc,
							&isnull);
			if (!isnull)
				*errmsg = TextDatumGetCString(datum);
		}
		FreeTupleDesc(desc);
		foreach (lc, tuplist)
			heap_freetuple((HeapTuple)lfirst(lc));
		list_free(tuplist);
	}

	MemoryContextSwitchTo(oldctx);

	return result;
}


/*
 * get pgerrcode for map oraerrcode
 */
int
plisql_get_exception_pgerrcode(MemoryContext stmt_txt,
											int oraerrcode,
											char **errmsg)
{
	HeapTuple maptuple = NULL;
	TupleDesc desc = NULL;
	int result = oraerrcode;
	List	*tuplist = NIL;
	ListCell *lc;
	MemoryContext oldctx;

	if (errmsg != NULL)
		*errmsg = NULL;

	oldctx = MemoryContextSwitchTo(stmt_txt);
	tuplist = ora_exceptionmap(oraerrcode,
							NULL,
							ORA_EXCEPTION_MAP_BY_ORACODE, &desc);

	/* maybe handle list in further */
	if (tuplist != NIL)
		maptuple = (HeapTuple) linitial(tuplist);

	if (maptuple != NULL)
	{
		bool isnull;
		Datum datum;
		char *sqlcode;

		datum = heap_getattr(maptuple,
							Anum_ivorysql_ora_exceptionmap_pg_sqlcode_idx,
							desc,
							&isnull);
		Assert(!isnull);
		sqlcode = TextDatumGetCString(datum);
		if (strlen(sqlcode) != 5 ||
			strspn(sqlcode, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 5)
			elog(ERROR, "invalid sqlcode :%s\n", sqlcode);

		result = MAKE_SQLSTATE(sqlcode[0],
								sqlcode[1],
								sqlcode[2],
								sqlcode[3],
								sqlcode[4]);
		pfree(sqlcode);
		if (errmsg != NULL)
		{
			datum = heap_getattr(maptuple,
							Anum_ivorysql_ora_exceptionmap_message_idx,
							desc,
							&isnull);
			if (!isnull)
				*errmsg = TextDatumGetCString(datum);
		}
		FreeTupleDesc(desc);
		foreach (lc, tuplist)
			heap_freetuple((HeapTuple)lfirst(lc));
		list_free(tuplist);
	}

	MemoryContextSwitchTo(oldctx);

	return result;
}


/*
 * set pgerrcode by oraerrcode
 */
void
plisql_set_udt_exception_errcode(PLiSQL_var *exception_var,
											int oraerrcode)
{
	List	*ret = NIL;
	ListCell *lc;
	TupleDesc desc = NULL;

	/* init exception informations */
	exception_var->oraerrcode = oraerrcode;
	exception_var->self_define = false;
	exception_var->value = Int32GetDatum(oraerrcode);

	ret = ora_exceptionmap(oraerrcode, NULL, ORA_EXCEPTION_MAP_BY_ORACODE, &desc);

	if (ret == NIL)
	{
		StringInfoData errmsg;

		initStringInfo(&errmsg);

		if (desc != NULL)
			FreeTupleDesc(desc);
		if (exception_var->errormsg)
			pfree(exception_var->errormsg);
		appendStringInfo(&errmsg, "IVY%d:", oraerrcode);
		exception_var->errormsg =  errmsg.data;
		return;
	}
	if (exception_var->errormsg)
		pfree(exception_var->errormsg);
	exception_var->errormsg = NULL;
	list_free(exception_var->pgerrcodes);
	exception_var->pgerrcodes = NIL;

	foreach (lc, ret)
	{
		bool isnull;
		Datum datum;
		HeapTuple tuple;
		char *sqlcode;
		int pgerrcode;

		tuple = (HeapTuple) lfirst(lc);

		/* handle sqlcode */
		datum = heap_getattr(tuple,
						Anum_ivorysql_ora_exceptionmap_pg_sqlcode_idx,
						desc,
						&isnull);
		Assert(!isnull);

		sqlcode = TextDatumGetCString(datum);

		if (strlen(sqlcode) != 5 ||
			strspn(sqlcode, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 5)
			elog(ERROR, "invalid sqlcode :%s\n", sqlcode);

		pgerrcode = MAKE_SQLSTATE(sqlcode[0],
								sqlcode[1],
								sqlcode[2],
								sqlcode[3],
								sqlcode[4]);
		pfree(sqlcode);

		/* first, we assign it to errmsg */
		if (exception_var->errormsg == NULL)
		{
			char *errmsg = NULL;

			/* handle message */
			datum = heap_getattr(tuple,
								Anum_ivorysql_ora_exceptionmap_message_idx,
								desc,
								&isnull);
			Assert(!isnull);
			errmsg = TextDatumGetCString(datum);
			exception_var->errormsg = errmsg;
			exception_var->value = Int32GetDatum(pgerrcode);
		}
		else
			exception_var->pgerrcodes = lappend_int(exception_var->pgerrcodes,
										pgerrcode);
	}

	if (desc)
		FreeTupleDesc(desc);
	foreach(lc, ret)
		heap_freetuple((HeapTuple) lfirst(lc));
	list_free(ret);

	return;
}

