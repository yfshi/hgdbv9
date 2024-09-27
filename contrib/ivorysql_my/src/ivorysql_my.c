/*------------------------------------------------------------------
 * 
 * File: ivorysql_my.c
 *
 * Abstract: 
 * 	 Ivorysql_my main entrypoint.
 *
 * Authored by qinshiyu@highgo.com, 20230511.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 * 	 contrib/ivorysql_my/src/ivorysql_my.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"


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
#if 0
	/* Must be loaded with shared_preload_libaries */
	if (!process_shared_preload_libraries_in_progress)
		ereport(ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("ivorysql_my must be loaded via shared_preload_libraries")));
#endif

	/* Define custom GUC variables */
	/*IvorysqlMyDefineGucs();*/

	
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
	
}
