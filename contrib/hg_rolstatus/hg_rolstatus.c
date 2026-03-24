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
#include "catalog/pg_role_status.h"
#include "utils/syscache.h"
#include "utils/builtins.h"
PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

static bool set_role_switch = false;
static bool role_get_status(uint32 roleoid);

Datum set_role_status(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(set_role_status);
Datum get_role_status(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(get_role_status);

/* set_role_status:
 *    set role status to pg_role_status table 
 */
Datum
set_role_status(PG_FUNCTION_ARGS)
{
	Relation rel;
	HeapTuple tuple;
	Oid roleoid;
	char *rolename;
	text *rolename_text;
	bool rolestatus;

	if (se_feature_mode)
	{
		if (!IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied")));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied")));
		}
	}

	if (!set_role_switch)
	{
		ereport(ERROR,
				errmsg("hg_rolstatus.enable is OFF"),
				errdetail_internal("Please enable hg_rolstatus.enable."));
	}

	rolename_text = PG_GETARG_TEXT_PP(0);
	rolename = text_to_cstring(rolename_text);
	rolestatus = PG_GETARG_BOOL(1);

	roleoid = get_role_oid(rolename, false);
	if (InvalidOid == roleoid)
	{
		ereport(ERROR,
				errmsg("role %s does not exist.", rolename));
	}

	if (get_role_canlogin_byoid(roleoid))
	{
		ereport(ERROR,
				errmsg("Only roles can be set, users cannot!"));
	}

	if (IS_SYSSAO(roleoid) || IS_SYSSSO(roleoid) || IS_SYSDBA(roleoid))
	{
		ereport(ERROR,
				errmsg("Can not enable/disable sysdba_role syssso_role and syssao_role!"));
	}

	rel = table_open(RoleStatusId, RowExclusiveLock);
	tuple = SearchSysCache1(ROLESTATUSID, ObjectIdGetDatum(roleoid));
	if (HeapTupleIsValid(tuple))
	{
		Form_pg_role_status role_status_form;

		role_status_form = (Form_pg_role_status)GETSTRUCT(tuple);
		if (role_status_form->rolstatus == rolestatus)
		{
			ereport(WARNING, (errmsg("role %s already %s", rolename, rolestatus ? "true" : "false")));
		}
		role_status_form->rolstatus = rolestatus;
		CatalogTupleUpdate(rel, &tuple->t_self, tuple);
		ReleaseSysCache(tuple);
	}
	else
	{
		Datum values[Natts_pg_role_status];
		bool nulls[Natts_pg_role_status];

		MemSet(nulls, false, sizeof(nulls));
		values[Anum_pg_role_status_roleid - 1] = ObjectIdGetDatum(roleoid);
		values[Anum_pg_role_status_rolname - 1] = CStringGetDatum(rolename);
		values[Anum_pg_role_status_rolstatus - 1] = BoolGetDatum(rolestatus);

		tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
		CatalogTupleInsert(rel, tuple);
		heap_freetuple(tuple);
	}
	table_close(rel, RowExclusiveLock);

	PG_RETURN_BOOL(true);
}

/* get_role_status:
 *    get role status by role_get_status function 
 */
Datum
get_role_status(PG_FUNCTION_ARGS)
{
	Oid roleoid;
	char *rolename;
	text *rolename_text;

	if (se_feature_mode)
	{
		if (!IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied")));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied")));
		}
	}

	if (!set_role_switch)
	{
		ereport(ERROR,
				errmsg("hg_rolstatus.enable is OFF"),
				errdetail_internal("Please enable hg_rolstatus.enable."));
	}

	rolename_text = PG_GETARG_TEXT_PP(0);
	rolename = text_to_cstring(rolename_text);

	roleoid = get_role_oid(rolename, false);
	if (InvalidOid == roleoid)
	{
		ereport(ERROR,
				errmsg("role %s does not exist.", rolename));
	}

	if (get_role_canlogin_byoid(roleoid))
	{
		ereport(ERROR,
				errmsg("Only roles can be get, users cannot!"));
	}

	PG_RETURN_BOOL(role_get_status(roleoid));
}

/* role_get_status:
 *    get role status from pg_role_status relation 
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

	if (!set_role_switch)
	{
		return true;
	}

	/* open table pg_role_status */
	rel = table_open(RoleStatusId, AccessShareLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(roleoid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		isRoleStatusDatum = heap_getattr(tup, Anum_pg_role_status_rolstatus, RelationGetDescr(rel), &isNull);
		isRoleStatus = DatumGetBool(isRoleStatusDatum);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);

	return isRoleStatus;
}

/* drop_role_status:
 *    delete pg_role_status reltation data when drop role. 
 */
static bool 
drop_role_status(uint32 roleoid)
{
	HeapTuple tup;
	Relation rel;
	ScanKeyData scanKey[1];
	TableScanDesc scandesc;
	bool result = false;

	/* open table pg_role_status */
	rel = table_open(RoleStatusId, RowExclusiveLock);

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
	/* rolstatus switch */
	DefineCustomBoolVariable("hg_rolstatus.enable",
							 "turn on or off set role status.",
							 NULL,
							 &set_role_switch,
							 false,
							 PGC_SIGHUP,
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
