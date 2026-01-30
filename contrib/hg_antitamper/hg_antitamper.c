/*-------------------------------------------------------------------------
 *
 * hg_antitamper.c
 *	  
 *
 * Authored by xuji@highgo.com, 20251223.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_antitamper/hg_antitamper.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-ANTITAMPERING */

#include "postgres.h"

#include "access/htup.h"
#include "access/tupdesc.h"
#include "access/heapam.h"
#include "access/table.h"
#include "access/visibilitymap.h"
#include "catalog/objectaccess.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "catalog/namespace.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_namespace.h"
#include "commands/seclabel.h"
#include "commands/schemacmds.h"
#include "executor/executor.h"
#include "executor/tuptable.h"
#include "funcapi.h"
#include "parser/parse_func.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/date.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/rls.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/uuid.h"
#include "utils/rel.h"
#include "nodes/bitmapset.h"
#include "nodes/parsenodes.h"
#include "nodes/execnodes.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

bool hg_antitamper_enabled = false;

#define HG_SYS_SCHEMA_ANTITAMPER "schema_antitamper"
#define Natts_schema_antitamper (2)
#define Anum_schema_antitamper_schemaoid_col (1)
#define Anum_schema_antitamper_antitamper_col (2)

/* schema_set_antitamper_rel:
 *    set schema_antitamper relation 
 *    when the mode is set the namespace to antitampering mode
 *    it is not saved in the system relation pg_namespace.
 *    so it is saved in the schema_antitamper relation.
 */
static bool 
schema_set_antitamper_rel(uint32 schemaoid, bool isantitamper)
{
	HeapTuple tup;
	Relation rel;
	Datum values[Natts_schema_antitamper];
	bool nulls[Natts_schema_antitamper];
	Oid schemaantitamperoid = InvalidOid;

	schemaantitamperoid = get_relname_relid(HG_SYS_SCHEMA_ANTITAMPER, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(schemaantitamperoid))
	{
		return false;
	}
	/* open table schema_antitamper */
	rel = table_open(schemaantitamperoid, RowExclusiveLock);

	/* zero-clear */
	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	values[Anum_schema_antitamper_schemaoid_col - 1] = UInt32GetDatum(schemaoid);
	values[Anum_schema_antitamper_antitamper_col - 1] = BoolGetDatum(isantitamper);

	/* insert the rule attribute into the table. */
	tup = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	simple_heap_insert(rel, tup);
	table_close(rel, RowExclusiveLock);

	return true;
}

/* schema_get_antitamper_flag:
 *    get antitampering mode from schema_antitamper relation 
 * 
 *    when the mode is set the namespace to antitampering mode
 *    it is not saved in the system relation pg_namespace.
 *    so it is saved in the hg_security.schema_antitamper relation.
 */
static bool
schema_get_antitamper_flag(uint32 schemaoid)
{
	HeapTuple tup;
	Relation rel;
	ScanKeyData scanKey[1];
	TableScanDesc scandesc;
	bool is_antitamper = false;
	Datum isantitamper;
	bool isNull;
	Oid schemaantitamperoid = InvalidOid;

	schemaantitamperoid = get_relname_relid(HG_SYS_SCHEMA_ANTITAMPER, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(schemaantitamperoid))
	{
		return false;
	}
	/* open table schema_antitamper */
	rel = table_open(schemaantitamperoid, AccessShareLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(schemaoid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		isantitamper = heap_getattr(tup, Anum_schema_antitamper_antitamper_col, RelationGetDescr(rel), &isNull);
		is_antitamper = DatumGetBool(isantitamper);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);

	return is_antitamper;
}

void
_PG_init(void)
{
	SchemaSetAntiTamperRel = schema_set_antitamper_rel;
	SchemaGetAntiTamperFlag = schema_get_antitamper_flag;
}

void
_PG_fini(void)
{

}
/* End - ReqID: SRS-SECURITY-ANTITAMPERING */
