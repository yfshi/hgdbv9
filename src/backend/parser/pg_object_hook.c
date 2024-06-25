/*--------------------------------------------------------------------
 * 
 * File: pg_object_hook.c
 *
 * Abstract: 
 *    This file contains hooks declarations for pg_object.
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *    src/backend/parser/pg_object_hook.c
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *--------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/makefuncs.h"
#include "nodes/parsenodes.h"
#include "parser/pg_object_hook.h"

Pg_object_create_obj_hook_type pg_object_create_obj_hook_type = NULL;
Pg_object_alter_obj_hook_type pg_object_alter_obj_hook_type = NULL;
Pg_object_drop_obj_hook_type pg_object_drop_obj_hook_type = NULL;
Pg_object_bulid_cache_obj_hook_type pg_object_bulid_cache_obj_hook_type = NULL;
Pg_object_get_ori_infor_hook_type pg_object_get_ori_infor_hook_type = NULL;
Pg_object_bulid_cache_obj_hook_type pg_object_bulid_relcache = NULL;

