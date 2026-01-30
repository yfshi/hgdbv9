/*-------------------------------------------------------------------------
 *
 * hg_rolstatus.c
 *	  
 *
 * Authored by xuji@highgo.com, 20251223.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_rolstatus/hg_rolstatus.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-EAL4-ROLSTATUS */

#include "postgres.h"

#include "access/htup.h"
#include "access/tupdesc.h"
#include "access/heapam.h"
#include "access/table.h"
#include "catalog/pg_namespace.h"
#include "catalog/indexing.h"
#include "commands/user.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "miscadmin.h"
#include "se_features_interface.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

#define HG_SYS_ROLE_STATUS "roles_status"
#define Natts_roles_status (2)
#define Anum_roles_oid_col (1)
#define Anum_roles_status_col (2)

static bool set_role_switch = false;

/* role_get_status:
 *    get role status from roles_status relation 
 */
static bool
role_get_status(uint32 roleoid)
{
	HeapTuple tup;
	Relation rel;
	ScanKeyData scanKey[1];
	TableScanDesc scandesc;
	bool isRoleStatus = true;
	Datum isRoleStatusDatum;
	bool isNull;
	Oid rolesStatusOid = InvalidOid;

	if (!set_role_switch)
	{
		return true;
	}

	rolesStatusOid = get_relname_relid(HG_SYS_ROLE_STATUS, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(rolesStatusOid) || !OidIsValid(roleoid))
	{
		return isRoleStatus;
	}
	/* open table roles_status */
	rel = table_open(rolesStatusOid, AccessShareLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(roleoid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		isRoleStatusDatum = heap_getattr(tup, Anum_roles_status_col, RelationGetDescr(rel), &isNull);
		isRoleStatus = DatumGetBool(isRoleStatusDatum);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);

	return isRoleStatus;
}

/* drop_role_status:
 *    delete roles_status reltation data when drop role. 
 */
static bool 
drop_role_status(uint32 roleoid)
{
	HeapTuple tup;
	Relation rel;
	ScanKeyData scanKey[1];
	TableScanDesc scandesc;
	bool result = false;
	Oid rolesStatusOid = InvalidOid;

	rolesStatusOid = get_relname_relid(HG_SYS_ROLE_STATUS, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(rolesStatusOid) || !OidIsValid(roleoid))
	{
		return result;
	}
	/* open table roles_status */
	rel = table_open(rolesStatusOid, RowExclusiveLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(roleoid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		CatalogTupleDelete(rel, &tup->t_self);
	}

	table_endscan(scandesc);
	table_close(rel, RowExclusiveLock);

	return result;
}

static bool
set_rolestatus_check(bool *newval, void **extra, GucSource source)
{
	if (source < PGC_S_TEST)
		return true;

	if (IS_SYSSSO(GetUserId()))
	{
		return true;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("Only syssso users can set this value")));
		return false;
	}
}

void
_PG_init(void)
{
	/* role_status switch */
	DefineCustomBoolVariable("hg_rolstatus.enable",
							 "turn on or off set role status.",
							 NULL,
							 &set_role_switch,
							 false,
							 PGC_USERSET,
							 0,
							 set_rolestatus_check,
							 NULL,
							 NULL);

	RoleGetStatus = role_get_status;
	RoleDropRoleStatus = drop_role_status;
}

void
_PG_fini(void)
{

}
/* End - ReqID: SRS-SECURITY-EAL4-ROLSTATUS */
