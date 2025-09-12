/*-------------------------------------------------------------------------
 *
 * mys_set_hook.c
 *	  Define MySQL SET hook
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/mysql_parser/mys_set_hook.c
 *
 *-------------------------------------------------------------------------
 */
#include "mysql_parser/mys_set_hook.h"


uservariable_hash_search_hook_type uservariable_hash_search_hook = NULL;
SetUserVarInternal_hook_type setuservarinternal_hook = NULL;