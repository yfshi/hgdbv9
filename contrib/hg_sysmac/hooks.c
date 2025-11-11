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
#include "commands/seclabel.h"
#include "executor/executor.h"
#include "storage/ipc.h"
#include "miscadmin.h"
#include "storage/shmem.h"
#include "utils/guc.h"
#include "utils/rls.h"

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

bool
is_security_roles()
{
	if (se_feature.mode) /* Bug - #5937 #z6015 #z6056 #z6058 */
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
}

/* End - ReqID: SRS-SECURITY-LMAC */
