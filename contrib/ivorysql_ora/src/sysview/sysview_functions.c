/*------------------------------------------------------------------
 * 
 * File: sysview_functions.c
 *
 * Abstract: 
 * 		Function which converts all-uppercase text to all-lowercase text
 * 		and vice versa.
 *
 * ReqID:SRS-SQL-SYSVIEW
 *
 * Authored by xuzhuang@highgo.com, 20220923.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/sysview/sysview_functions.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include "fmgr.h"
#include "parser/scansup.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h"


PG_FUNCTION_INFO_V1(ora_case_trans);


Datum
ora_case_trans(PG_FUNCTION_ARGS)
{
	char	*string = NULL,
			*retval = NULL;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* if identifier_case_switch is NORMAL / LOWERCASE, return it directly. */
	if (INTERCHANGE != identifier_case_switch)
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));

	/* otherwise, performs the transformation. */
	string = (char *) TextDatumGetCString(PG_GETARG_DATUM(0));
	retval = identifier_case_transform(string, strlen(string));
	PG_RETURN_TEXT_P(CStringGetTextDatum(retval));
}
