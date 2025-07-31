/*-------------------------------------------------------------------------
 *
 * pg_recyclebin.c
 *		routines to support manipulation of the pg_recyclebin relation.
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * IDENTIFICATION
 *	  src/backend/catalog/pg_recyclebin.c
 *
 * add the file for requirement "SRS-RECOVERY-FLASHBACK"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_recyclebin.h"
#include "access/htup_details.h"
#include "storage/lockdefs.h"
#include "utils/fmgrprotos.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "utils/timestamp.h"
#include "utils/rel.h"
#include "catalog/indexing.h"
#include "access/xact.h"
#include "utils/inval.h"
#include "access/tableam.h"
#include "utils/builtins.h"
#include "access/skey.h"
#include "utils/fmgroids.h"
#include "miscadmin.h"
#include "catalog/namespace.h"
#include "utils/snapmgr.h"
#include "catalog/pg_namespace.h"
#include "executor/spi.h"

/* Record a new recycle bin entry. */
Oid 
RecyclebinWrite(RbObjDesc *desc, Oid baseid)
{
	Relation rel;
	HeapTuple tup;
	bool nulls[Natts_pg_recyclebin] = {0};
	Datum values[Natts_pg_recyclebin];
	Oid rbid;

	rel = table_open(RecyclebinRelationId, RowExclusiveLock);
	
	rbid = GetNewOidWithIndex(rel, RecyclebinOidIndexId, Anum_pg_recyclebin_oid);
	values[Anum_pg_recyclebin_oid - 1] = rbid;
	values[Anum_pg_recyclebin_rcybaseid - 1] = ObjectIdGetDatum(OidIsValid(baseid) ? baseid : rbid);
	values[Anum_pg_recyclebin_rcyrelid - 1] = ObjectIdGetDatum(desc->relid);
	values[Anum_pg_recyclebin_rcyname - 1] = DirectFunctionCall1(namein, CStringGetDatum(desc->name));
	values[Anum_pg_recyclebin_rcyoriginname - 1] = DirectFunctionCall1(namein, CStringGetDatum(desc->originname));
	values[Anum_pg_recyclebin_rcyoperation - 1] = CharGetDatum(desc->operation);
	values[Anum_pg_recyclebin_rcytype - 1] = Int32GetDatum(desc->type);
	values[Anum_pg_recyclebin_rcyrecycletime - 1] = TimestampTzGetDatum(desc->recycletime);
	values[Anum_pg_recyclebin_rcynamespace - 1] = ObjectIdGetDatum(desc->nspace);
	values[Anum_pg_recyclebin_rcytablespace -1] = ObjectIdGetDatum(desc->tspace);
	values[Anum_pg_recyclebin_rcyrelfilenode - 1] = ObjectIdGetDatum(desc->relfilenode);

	tup = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	CatalogTupleInsert(rel, tup);

	heap_freetuple(tup);
	table_close(rel, RowExclusiveLock);

	CommandCounterIncrement();

	return rbid;
}

/* Read a recycle bin entry. */
void 
RecyclebinRead(RbObjDesc *desc, HeapTuple rbtup)
{
	Form_pg_recyclebin rbForm = (Form_pg_recyclebin)GETSTRUCT(rbtup);

	desc->id = rbForm->oid;
	desc->baseid = rbForm->rcybaseid;
	desc->relid = rbForm->rcyrelid;
	(void)namestrcpy((Name)desc->name, NameStr(rbForm->rcyname));
	(void)namestrcpy((Name)desc->originname, NameStr(rbForm->rcyoriginname));
	desc->operation = rbForm->rcyoperation;
	desc->type = rbForm->rcytype;
	desc->recycletime = DatumGetTimestampTz(rbForm->rcyrecycletime);
	desc->nspace = rbForm->rcynamespace;
	desc->tspace = rbForm->rcytablespace;
	desc->relfilenode = rbForm->rcyrelfilenode;
	desc->authid = GetUserId();
}

static bool 
RbFetchOrinameImpl(Oid nspId, const char *oriname, RbObjType type, 
				   RbObjDesc *desc, RbOperMode operMode)
{
	StringInfoData	buf;
	int				ret;
	bool			found = false;

	initStringInfo(&buf);
	if (operMode == RB_OPER_PURGE)
		appendStringInfo(&buf, "SELECT * FROM pg_catalog.pg_recyclebin WHERE "
							   "rcynamespace = %u AND rcyoriginname = '%s' "
							   "AND rcytype = %d "
							   "ORDER BY rcyrecycletime ASC LIMIT 1",
							   nspId, oriname, RB_OBJ_TABLE);
	else
		appendStringInfo(&buf, "SELECT * FROM pg_catalog.pg_recyclebin WHERE "
							   "rcynamespace = %u AND rcyoriginname = '%s' "
							   "AND rcytype = %d AND rcyoperation = '%c' "
							   "ORDER BY rcyrecycletime DESC LIMIT 1",
							   nspId, oriname, RB_OBJ_TABLE,
							   operMode == RB_OPER_RESTORE_DROP ?
							   RCYOPERATION_DROP :
							   RCYOPERATION_TRUNCATE);

	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
		elog(ERROR, "search recyclebin failed: SPI_connect returned %d", ret);

	PushActiveSnapshot(GetTransactionSnapshot());
	
	ret = SPI_execute(buf.data, true, 0);

	/* Check for qualifying tuples */
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "search recyclebin failed: SPI_execute returned %d", ret);
	
	if (SPI_processed != 1)
		goto cleanup;

	/* Fetch the tuple succeed. */
	found = true;
	RecyclebinRead(desc, SPI_tuptable->vals[0]);

cleanup:
	pfree(buf.data);
	SPI_finish();
	PopActiveSnapshot();

	return found;
}

static bool 
RbFetchOriname(const char *schemaname, const char *relname,
	RbObjType type, RbObjDesc *desc, RbOperMode operMode)
{
	bool found = false;
	Oid nspId;

	if (schemaname)
	{
		nspId = get_namespace_oid(schemaname, true);
		found = RbFetchOrinameImpl(nspId, relname, type, desc, operMode);
	} 
	else 
	{
		List *activeSearchPath = NIL;
		ListCell *l = NULL;

		activeSearchPath = fetch_search_path(true);
		foreach (l, activeSearchPath) 
		{
			nspId = lfirst_oid(l);
			if (RbFetchOrinameImpl(nspId, relname, type, desc, operMode)) 
			{
				found = true;
				break;
			}
		}
		list_free(activeSearchPath);
	}

	return found;
}

/*
 * Fetch object from recycle bin for rb operations - purge, restore :
 *     Prefer to fetch as original name, then recycle name.
 */
void 
RbOperFetch(const char* schemaname, const char* relname, 
		RbObjType type, RbObjDesc *desc, RbOperMode operMode)
{
	bool found = false;

	AcceptInvalidationMessages();

	/* Prefer to fetch as original name */
	found = RbFetchOriname(schemaname, relname, type, desc, operMode);

	/* not found, throw error */
	if (!found) 
	{
		char *operStr = "unknown";

		switch (operMode)
		{
			case RB_OPER_PURGE:
				operStr = "DROP/TRUNCATE";
				break;
			case RB_OPER_RESTORE_DROP:
				operStr = "DROP";
				break;
			case RB_OPER_RESTORE_TRUNCATE:
				operStr = "TRUNCATE";
				break;
			default:
				break;
		}
		ereport(ERROR,
			(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			 errmsg("recycle object \"%s\" does not exist", relname),
			 errhint("Make sure the configuration parameter \"%s\" is set before \"%s\".",
					 "ivorysql.enable_recycle_bin", operStr)));
	}

	return;
}

/*
 * Fetch an object from recycle bin for purge recyclebin.
 */
bool 
RbFetchAny(RbObjDesc *desc)
{
	Relation rbRel;
	TableScanDesc scan;
	ScanKeyData skey[1];
	bool found = false;
	TupleTableSlot *slot;

	rbRel = table_open(RecyclebinRelationId, AccessShareLock);
	slot = table_slot_create(rbRel, NULL);
	ScanKeyInit(&skey[0], Anum_pg_recyclebin_rcytype, BTEqualStrategyNumber,
		F_INT2EQ, Int16GetDatum(0));
	scan = table_beginscan(rbRel, SnapshotSelf, 1, skey);

	while (table_scan_getnextslot(scan, ForwardScanDirection, slot)) 
	{
		HeapTuple tup = ExecFetchSlotHeapTuple(slot, false, NULL);

		found = true;
		RecyclebinRead(desc, tup);
		break;
	}

	ExecDropSingleTupleTableSlot(slot);
	table_endscan(scan);
	table_close(rbRel, AccessShareLock);

	return found;
}
