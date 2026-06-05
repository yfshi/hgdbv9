/*-------------------------------------------------------------------------
 *
 * se_features.c
 *	  all secure enhancement features kernel implimentation.
 *    
 * This could be included by database kernels or other modules, so for 
 * those features that needed by kernel should be implimented here, other
 * implimentation may loaded via shared-load library.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1995, Regents of the University of California
 *
 * src/include/se_features_interface.h
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "funcapi.h"
#include "miscadmin.h"
#include "access/table.h"
#include "catalog/pg_authid.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "utils/guc.h"
#include "utils/syscache.h"
#include "utils/rel.h"
#include "utils/acl.h"
#include "utils/guc.h"

#include "se_features_interface.h"


/* use it with caution, it may make a normal user a superuser,maybe changed latter */
Oid tmp_superuser_oid = InvalidOid;

/* se_feature switch */
bool se_feature_mode = false;
bool restricted_dba = false;

/*
 * Hooks for function calls
 */
grant_sysdba_priv_hook_type grant_sysdba_priv_hook = NULL;
role_is_superuser_hook_type role_is_superuser_hook = NULL;
role_has_createrole_priv_hook_type role_has_createrole_priv_hook = NULL;
role_has_createdb_priv_hook_type role_has_createdb_priv_hook = NULL;
use_sequence_check_hook_type use_sequence_check_hook = NULL;

bool is_member_of_serole(Oid member, char* role_name)
{
	Oid role_id;
	bool result;

	role_id = get_role_oid(role_name, false);

	if (se_feature_mode)
		result = is_member_of_role_nosuper(member, role_id);
	else
		result = is_member_of_role(member, role_id);

	return result;
}

/*
 * Just like check_rolespec_name(), we should prevent user 
 * alter 3 security roles which created at initalization ,
 * also disallow user change its role attributes, 
 * just like how postgres treat those predefined roles
 */
void
check_se_rolespec_name(const RoleSpec *role, const char *detail_msg)
{
	if (!role)
		return;

	if (role->roletype != ROLESPEC_CSTRING)
		return;

	if (strcmp(role->rolename, SYSDBA_ROLE) == 0 ||
		strcmp(role->rolename, SYSSSO_ROLE) == 0 ||
		strcmp(role->rolename, SYSSAO_ROLE) == 0)
	{
		if (detail_msg)
			ereport(ERROR,
					(errcode(ERRCODE_RESERVED_NAME),
					 errmsg("role name \"%s\" is reserved",
							role->rolename),
					 errdetail_internal("%s", detail_msg)));
		else
			ereport(ERROR,
					(errcode(ERRCODE_RESERVED_NAME),
					 errmsg("role name \"%s\" is reserved",
							role->rolename)));
	}
}

PG_FUNCTION_INFO_V1(show_secure_param);
/*
 * show_secure_param - display security-related parameters
 */
Datum
show_secure_param(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	int call_cntr;
	int max_calls;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;
	MemoryContext oldcontext;
	Oid role_oid = GetUserId();

	/* Begin bug#z8069 */
	if (se_feature_mode)
	{
		if (!IS_SYSSSO(role_oid))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied, only role=syssso_role can run this function")));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied, only superuser can run this function")));
		}
	}
	/* End bug#z8069 */

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL())
	{
		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* build tupdesc for result tuples */
		tupdesc = CreateTemplateTupleDesc(2, false);
		TupleDescInitEntry(tupdesc, (AttrNumber)1, "parameter",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber)2, "value",
						   TEXTOID, -1, 0);

		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		/* Pre-calculate how many parameters we'll return */
		max_calls = sizeof(security_parameters)/sizeof(security_parameters[0]) - 1;

		funcctx->max_calls = max_calls;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;

	if (call_cntr < max_calls)
	{
		char *values[2];
		HeapTuple tuple;
		Datum result;
		char *param_value;

		/* get the parameter name */
		values[0] = (char *)security_parameters[call_cntr];

		/* get the parameter value */
		param_value = GetConfigOptionByName(values[0], NULL, true);
		values[1] = param_value ? pstrdup(param_value) : "NULL";

		/* build the tuple */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* free memory if needed */
		if (param_value)
			pfree(param_value);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		SRF_RETURN_DONE(funcctx);
	}
}