/*------------------------------------------------------------------
 * 
 * File: guc.c
 *
 * Abstract: 
 * 		Ivorysql_ora GUC variables define.
 *
 * Authored by zhenmingyang@highgo.com, 20220510.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/guc/guc.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/guc.h"

#include "../include/guc.h"

#if 0
static const struct config_enum_entry nls_length_semantics_options[] =
{
	{"char", NLS_LENGTH_SEMANTICS_CHAR, false},
	{"byte", NLS_LENGTH_SEMANTICS_BYTE, false},
	{NULL, 0, false}
};

/* GUC variables */
int	nls_length_semantics = NLS_LENGTH_SEMANTICS_BYTE;
#endif

/*
 * Check function for GUC variable.
 */



/*
 * Define various GUC.
 */
void
IvorysqlOraDefineGucs(void)
{
#if 0
	DefineCustomEnumVariable(
		"ivorysql_ora.nls_length_semantics",
		gettext_noop("Compatible Oracle NLS parameter for charater data type"),
		NULL,
		&nls_length_semantics,
		NLS_LENGTH_SEMANTICS_BYTE, nls_length_semantics_options,
		PGC_USERSET,
		GUC_NOT_IN_SAMPLE,
		NULL,
		NULL,
		NULL);
#endif
}
