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

/*local copy*/
bool local_on = false;
bool local_table_level = false;
bool local_column_level = false;
bool local_row_level = true;
/* the shared paramerter*/
hg_sysmac_param *hg_sysmac_g = NULL;

/*
 * Saved hook entries (if stacked)
 */
static ExecutorCheckPerms_hook_type next_exec_check_perms_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

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
	if (!IS_SYSSSO(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("Only member of syssso_role can set and query seclabel.")));

	return true;
}

static bool
check_sysmac_param(bool *newval, void **extra, GucSource source)
{
	GucSource *mysrouce;
	/*only check on set command*/
	if (source == PGC_S_SESSION)
	{
		is_security_roles();
		mysrouce = (GucSource *) malloc(sizeof(GucSource));
		if (!mysrouce)
			return false;
		*mysrouce = source;
		*extra = (void *) mysrouce;
	}

	return true;
}

static void
assign_sysmac_param_on(bool newval, void *extra)
{
	GucSource *mysrouce = (GucSource *) extra;

	if (extra != NULL)
	{
		/*once we set a variable, we shared it*/
		if (*mysrouce == PGC_S_SESSION)
		{
			if (hg_sysmac_g == NULL)
				elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
			else
				hg_sysmac_g->global_on = newval;
		}
	}
}
static void
assign_sysmac_param_table(bool newval, void *extra)
{
	GucSource *mysrouce = (GucSource *) extra;

	if (extra != NULL)
	{
		/*once we set a variable, we shared it*/
		if (*mysrouce == PGC_S_SESSION)
		{
			if (hg_sysmac_g == NULL)
				elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
			else
				hg_sysmac_g->global_table_level = newval;
		}
	}
}

static void
assign_sysmac_param_column(bool newval, void *extra)
{
	GucSource *mysrouce = (GucSource *) extra;

	if (extra != NULL)
	{
		/*once we set a variable, we shared it*/
		if (*mysrouce == PGC_S_SESSION)
		{
			if (hg_sysmac_g == NULL)
				elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
			else
				hg_sysmac_g->global_column_level = newval;
		}
	}
}

static void
assign_sysmac_param_row(bool newval, void *extra)
{
	GucSource *mysrouce = (GucSource *) extra;

	if (extra != NULL)
	{
		/*once we set a variable, we shared it*/
		if (*mysrouce == PGC_S_SESSION)
		{
			if (hg_sysmac_g == NULL)
				elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
			else
				hg_sysmac_g->global_row_level = newval;
		}
	}
}

static const char *
show_sysmac_param_on()
{
	if (hg_sysmac_g == NULL)
		return "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.";
	else if (hg_sysmac_g->global_on)
		return "on";
	else
		return "off";
}

static const char *
show_sysmac_param_table()
{
	if (hg_sysmac_g == NULL)
		return "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.";
	else if (hg_sysmac_g->global_table_level)
		return "on";
	else
		return "off";
}

static const char *
show_sysmac_param_column()
{
	if (hg_sysmac_g == NULL)
		return "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.";
	else if (hg_sysmac_g->global_column_level)
		return "on";
	else
		return "off";
}

static const char *
show_sysmac_param_row()
{
	if (hg_sysmac_g == NULL)
		return "The 'hg_sysmac' extension was not loaded correctly. Please check the configuration.";
	else if (hg_sysmac_g->global_row_level)
		return "on";
	else
		return "off";
}

static void
sysmac_shmem_init()
{
	bool found;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	hg_sysmac_g = ShmemInitStruct("hg_sysmac",
								  sizeof(hg_sysmac_param),
								  &found);

	if (!found)
	{
		hg_sysmac_g->global_on = local_on;
		hg_sysmac_g->global_table_level = local_table_level;
		hg_sysmac_g->global_column_level = local_column_level;
		hg_sysmac_g->global_row_level = local_row_level;
	}
}

void
_PG_init(void)
{
	DefineCustomBoolVariable("hg_sysmac.global_on",
							 "Turn on/off global_on",
							 NULL,
							 &local_on,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 check_sysmac_param,
							 assign_sysmac_param_on,
							 show_sysmac_param_on);

	DefineCustomBoolVariable("hg_sysmac.global_table_level",
							 "Turn on/off global_table_level",
							 NULL,
							 &local_table_level,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 check_sysmac_param,
							 assign_sysmac_param_table,
							 show_sysmac_param_table);

	DefineCustomBoolVariable("hg_sysmac.global_column_level",
							 "Turn on/off global_column_level",
							 NULL,
							 &local_column_level,
							 false,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 check_sysmac_param,
							 assign_sysmac_param_column,
							 show_sysmac_param_column);

	DefineCustomBoolVariable("hg_sysmac.global_row_level",
							 "Turn on/off global_row_level",
							 NULL,
							 &local_row_level,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 check_sysmac_param,
							 assign_sysmac_param_row,
							 show_sysmac_param_row);

	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = sysmac_shmem_init;

	/* Security label provider hook */
	register_label_provider(SYSMAC_LABEL_TAG,
							check_relabel);

	/* DML permission check */
	next_exec_check_perms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = sysmac_exec_check_perms;
}

/* End - ReqID: SRS-SECURITY-LMAC */
