/*------------------------------------------------------------------
 * 
 * File: sysview_functions.c
 *
 * Abstract: 
 * 		Function which converts all-uppercase text to all-lowercase text 
 * 		and vice versa.
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


/* Begin - ReqID:SRS-SQL-SYSVIEW */
PG_FUNCTION_INFO_V1(ora_case_trans);
Datum
ora_case_trans(PG_FUNCTION_ARGS)
{
	char	*string;
	char	*retval;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	string = (char *) TextDatumGetCString(PG_GETARG_DATUM(0));

	retval = identifier_case_transform(string, strlen(string));

	PG_RETURN_TEXT_P(CStringGetTextDatum(retval));
}
/* End - ReqID:SRS-SQL-SYSVIEW */

