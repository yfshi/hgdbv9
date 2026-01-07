/*-------------------------------------------------------------------------
 *
 * hooks.c
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/hooks.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#include "postgres.h"

#include "catalog/objectaccess.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "catalog/namespace.h"
#include "commands/seclabel.h"
#include "executor/executor.h"
#include "parser/parse_func.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/guc.h"
#include "utils/rls.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"

#include "hg_sysmac.h"
#include "se_features_interface.h"

PG_MODULE_MAGIC;

void		_PG_init(void);

/* Begin - bug#z5940 */
bool global_on = false;
bool global_table_level = false;
bool global_column_level = false;
bool global_row_level = true;
/* End - bug#z5940 */

/*
 * Saved hook entries (if stacked)
 */
static ExecutorCheckPerms_hook_type next_exec_check_perms_hook = NULL;
static ProcessUtility_hook_type next_ProcessUtility_hook = NULL;


static bool
is_sysmac_function(Oid funcoid)
{
	#define		PROCESSLIST_COLUMN_NUMS 7
	char	*funcname = NULL;
	int		i;
	Oid		nspoid;
	char	funclist[PROCESSLIST_COLUMN_NUMS][32] =
							{"get_user_seclabel",   "set_user_seclabel",
							 "get_table_seclabel",  "set_table_seclabel",
							 "get_column_seclabel", "get_column_seclabel",
							 "apply_row_policy"};

	nspoid = get_namespace_oid("hg_security", true);
	if (!OidIsValid(nspoid))
		return false;

	if (get_func_namespace(funcoid) != nspoid)
		return false;

	funcname = get_func_name(funcoid);
	if (!funcname)
		return false;

	for (i = 0; i < PROCESSLIST_COLUMN_NUMS; i++)
	{
		if (pg_strcasecmp(funcname, funclist[i]) == 0)
			return true;
	}

	return false;
}

static bool
sysmac_exec_check_perms(List *rangeTabls, bool abort)
{
	/*
	 * If security provider is stacking and one of them replied 'false' at
	 * least, we don't need to check any more.
	 */
	if (next_exec_check_perms_hook &&
		!(*next_exec_check_perms_hook) (rangeTabls, abort))
		return false;

	if (!sysmac_dml_privileges(rangeTabls, abort))
		return false;

	return true;
}

/*
 * sysmac_utility_command
 *
 * It tries to rough-grained control on utility commands; some of them can
 * break whole of the things if nefarious user would use.
 */
static void
sysmac_utility_command(PlannedStmt *pstmt,
						const char *queryString,
						bool readOnlyTree,
						ProcessUtilityContext context,
						ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest,
						QueryCompletion *qc)
{
	Node	   *parsetree = pstmt->utilityStmt;
	GrantStmt  *action;
	ListCell   *cell;

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, readOnlyTree,
									 context, params, queryEnv,
									 dest, qc);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree,
								context, params, queryEnv,
								dest, qc);

	if (!IsUnderPostmaster || !global_on)
		return;

	switch (nodeTag(parsetree))
	{
		case T_GrantStmt:
			action = (GrantStmt *) parsetree;

			/*
			 * We do not allow granting permissions on hg_sysmac
			 * functions and tables to regular users.
			 */
			foreach(cell, action->grantees)
			{
				RoleSpec   *grantee = (RoleSpec *) lfirst(cell);
				Oid			grantee_uid;

				switch (grantee->roletype)
				{
					case ROLESPEC_PUBLIC:
						grantee_uid = ACL_ID_PUBLIC;
						break;
					default:
						grantee_uid = get_rolespec_oid(grantee, false);
						break;
				}

				if (!IS_SYSSSO(grantee_uid) && !superuser_arg(grantee_uid))
				{
					if (action->objtype == OBJECT_TABLE)
					{
						foreach(cell, action->objects)
						{
							RangeVar   *relvar = (RangeVar *) lfirst(cell);
							Oid			relOid;

							relOid = RangeVarGetRelid(relvar, NoLock, true);
							if (relOid == SharedSecLabelRelationId ||
								relOid == SecLabelRelationId)
							{
								ereport(ERROR,
										(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
										errmsg("permission denied"),
										errdetail("Cannot grant sysmac-related `%s` to regular users.", relvar->relname)));
							}
						}
					}
					else if (action->objtype == OBJECT_FUNCTION)
					{
						foreach(cell, action->objects)
						{
							ObjectWithArgs *func = (ObjectWithArgs *) lfirst(cell);
							Oid			funcid;

							funcid = LookupFuncWithArgs(OBJECT_FUNCTION, func, false);
							if (is_sysmac_function(funcid))
							{
								ereport(ERROR,
										(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
										errmsg("permission denied"),
										errdetail("Cannot grant sysmac-related functions to regular users.")));
							}
						}
					}
				}
			}
			break;

		default:
			break;
	}
}

bool
is_security_roles()
{
	if (se_feature_mode) /* Bug - #5937 #z6015 #z6056 #z6058 */
	{
		if (!IS_SYSSSO(GetUserId()))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("Only member of syssso_role can set and query seclabel.")));
	}
	else /* Begin - Bug #5937 #z6015 #z6056 #z6058 */
	{
		if (!superuser())
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("Only member of superuser can set and query seclabel.")));
	} /* End - Bug #5937 #z6015 #z6056 #z6058 */

	return true;
}

void
_PG_init(void)
{
	DefineCustomBoolVariable("hg_sysmac.global_on",
							 "Turn on/off global_on",
							 NULL,
							 &global_on, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_table_level",
							 "Turn on/off global_table_level",
							 NULL,
							 &global_table_level, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_column_level",
							 "Turn on/off global_column_level",
							 NULL,
							 &global_column_level, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_row_level",
							 "Turn on/off global_row_level",
							 NULL,
							 &global_row_level, /* Bug - #z5940 */
							 true,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	/* Security label provider hook */
	register_label_provider(SYSMAC_LABEL_TAG,
							check_relabel);

	/* DML permission check */
	next_exec_check_perms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = sysmac_exec_check_perms;

	/* ProcessUtility hook */
	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = sysmac_utility_command;
}

/* End - ReqID: SRS-SECURITY-LMAC */
