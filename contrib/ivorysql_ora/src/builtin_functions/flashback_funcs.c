/*-------------------------------------------------------------------------
 *
 * flashback_funcs.c
 *	  builtin helpers used by flashback-style pseudo-columns
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * contrib/ivorysql_ora/src/builtin_functions/flashback_funcs.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/commit_ts.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "executor/tuptable.h"
#include "utils/rel.h"
#include "utils/builtins.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"

PG_FUNCTION_INFO_V1(pg_versions_startscn);
PG_FUNCTION_INFO_V1(pg_versions_endscn);
PG_FUNCTION_INFO_V1(pg_versions_starttime);
PG_FUNCTION_INFO_V1(pg_versions_endtime);
PG_FUNCTION_INFO_V1(pg_versions_operation);

/*
 * Shared implementation for pseudo-columns that expose the commit timestamp
 * associated with an xid.  Invalid, special, or pruned-away xids map to
 * NULL so callers can use the helper directly on xmin/xmax system columns.
 */
static Datum
versions_xid_timestamp(PG_FUNCTION_ARGS)
{
	TransactionId xid = PG_GETARG_TRANSACTIONID(0);
	TimestampTz ts;
	bool		found;

	if (!TransactionIdIsValid(xid) || !TransactionIdIsNormal(xid))
		PG_RETURN_NULL();

	found = TransactionIdGetCommitTsData(xid, &ts, NULL);
	if (!found)
		PG_RETURN_NULL();

	return TimestampTzGetDatum(ts);
}

Datum
pg_versions_startscn(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

Datum
pg_versions_endscn(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

Datum
pg_versions_starttime(PG_FUNCTION_ARGS)
{
	return versions_xid_timestamp(fcinfo);
}

Datum
pg_versions_endtime(PG_FUNCTION_ARGS)
{
	return versions_xid_timestamp(fcinfo);
}

Datum
pg_versions_operation(PG_FUNCTION_ARGS)
{
	ItemPointer	tidptr = (ItemPointer) DatumGetPointer(PG_GETARG_DATUM(0));
	ItemPointerData tid = *tidptr;
	Oid			tableoid = PG_GETARG_OID(1);
	Relation	rel;
	TupleTableSlot *slot;
	HeapTuple	tuple;
	HeapTupleHeader	tuphdr;
	bool		should_free = false;
	char		op;
	text	   *result;

	rel = table_open(tableoid, AccessShareLock);
	slot = table_slot_create(rel, NULL);

	/*
	 * Resolve the current tuple header from the visible row identity.  We need
	 * the on-page tuple header to distinguish delete from update using t_ctid
	 * and HEAP_UPDATED state; the sysattrs alone are insufficient.
	 */
	if (!table_tuple_fetch_row_version(rel, &tid, SnapshotAny, slot))
	{
		ExecDropSingleTupleTableSlot(slot);
		table_close(rel, AccessShareLock);
		PG_RETURN_NULL();
	}

	tuple = ExecFetchSlotHeapTuple(slot, false, &should_free);
	tuphdr = tuple->t_data;

	/*
	 * A valid non-lock xmax means this version has been superseded or deleted,
	 * even if it was originally produced by UPDATE and still carries
	 * HEAP_UPDATED.  Only surviving versions created by UPDATE report U.
	 */
	if (!(tuphdr->t_infomask & HEAP_XMAX_INVALID) &&
			!HeapTupleHeaderIsOnlyLocked(tuphdr))
		op = 'D';
	else if (tuphdr->t_infomask & HEAP_UPDATED)
		op = 'U';
	else
		op = 'I';

	if (should_free)
		heap_freetuple(tuple);
	ExecDropSingleTupleTableSlot(slot);
	table_close(rel, AccessShareLock);

	if (op == '?')
		PG_RETURN_NULL();

	result = cstring_to_text_with_len(&op, 1);
	PG_RETURN_TEXT_P(result);
}
