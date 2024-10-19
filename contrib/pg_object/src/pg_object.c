/*------------------------------------------------------------------
 * 
 * File: ivorysql_my.c
 *
 * Abstract: 
 *    Ivorysql_my main entrypoint.
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *    contrib/ivorysql_my/src/ivorysql_my.c
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"
#include "parser/pg_object_hook.h"
#include "pg_object_interface.h"
#include "utils/guc.h"

bool enable_pg_object = false;

static pg_object_create_obj_hook_type 		prepg_object_create_obj_hook = NULL;
static pg_object_alter_obj_hook_type 		prepg_object_alter_obj_hook = NULL;
static pg_object_drop_obj_hook_type 		prepg_object_drop_obj_hook = NULL;
static pg_object_bulid_cache_obj_hook_type 	prepg_object_bulid_cache_obj_hook = NULL;
static pg_object_get_ori_infor_hook_type 	prepg_object_get_ori_infor_hook = NULL;

/* Only include it once in any C file */
PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

/*
 * Module initialization function.
 */
void
_PG_init(void)
{
	/* Add a guc parameter in the extension */
	DefineCustomBoolVariable("pg_object.enable",
						 "pg_object system table interface switch.",
						 NULL,
						 &enable_pg_object,
						 true,
						 PGC_USERSET,
						 0,
						 NULL, NULL, NULL);

	/* initialization hook */
	prepg_object_create_obj_hook = pg_object_create_obj_hook;
	pg_object_create_obj_hook = pg_object_createobj;
	prepg_object_alter_obj_hook = pg_object_alter_obj_hook;
	pg_object_alter_obj_hook = pg_object_alterobj;
	prepg_object_drop_obj_hook = pg_object_drop_obj_hook;
	pg_object_drop_obj_hook = pg_object_drop;
	prepg_object_bulid_cache_obj_hook = pg_object_bulid_cache_obj_hook;
	pg_object_bulid_cache_obj_hook = RelationBuildColumnAttr;
	prepg_object_get_ori_infor_hook = pg_object_get_ori_infor_hook;
	pg_object_get_ori_infor_hook = get_object_origin_info;
}

/*
 * Module finalization function.
 *
 * Note that _PG_fini will only be called during an unload of the module, 
 * not during process termination. Presently, unloads are disabled and
 * will never occur, but this may change in the future.
 */
void
_PG_fini(void)
{
	pg_object_create_obj_hook = prepg_object_create_obj_hook;
	pg_object_alter_obj_hook = prepg_object_alter_obj_hook;
	pg_object_drop_obj_hook = prepg_object_drop_obj_hook;
	pg_object_bulid_cache_obj_hook = prepg_object_bulid_cache_obj_hook;
	pg_object_get_ori_infor_hook = prepg_object_get_ori_infor_hook;
}
