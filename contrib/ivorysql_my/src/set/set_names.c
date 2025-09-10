/*-------------------------------------------------------------------------
 *
 * set_names.c
 *	  Implenmentation of the MySQL SET NAMES functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/set/set_names.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "nodes/parsenodes.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "../include/mysset.h"
#include "mb/pg_wchar.h"


PG_FUNCTION_INFO_V1(setnames);

/* setnames
 *
 * mysql set names.
 */
Datum
setnames(PG_FUNCTION_ARGS)
{
	char	   *varName;
	int			i;
	bool		findflg = false;
	bool		isdefult = false;

	if (PG_ARGISNULL(0))
		PG_RETURN_VOID();

	varName = TextDatumGetCString(PG_GETARG_DATUM(0));
	isdefult = DatumGetBool(PG_GETARG_DATUM(1));
	if (ERROR_CHARSET(varName))
	{
		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg("Variable 'character_set_client' can't be set to the value of '%s' ",
						varName)));
	}

	for (i = 0; i < pg_encname_tbl_size; i++)
	{
		const pg_encname *charset = &pg_encname_tbl[i];

		if (pg_strcasecmp(varName, charset->name) == 0)
			findflg = true;
	}

	if (!findflg && !isdefult)
		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg(" '%s' is an unavailable character set ",
						varName)));

	PG_RETURN_VOID();
}
