/*-------------------------------------------------------------------------------
 * 
 * File: misc_functions.c
 *
 * Abstract: 
 * 		This file contains the implementation of Oracle's
 *		datatype-independent built-in functions.
 *
 * Authored by zhenmingyang@highgo.com, 20220526.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/builtin_functions/misc_functions.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/formatting.h"
#include "utils/numeric.h"
/* Begin - Bug#Z1064 */
#include "utils/builtins.h"
#include "access/transam.h"
/* End - Bug#Z1064 */

PG_FUNCTION_INFO_V1(uid);	/* bug0000459 */
PG_FUNCTION_INFO_V1(get_macro_value); /* Bug#Z1064 */


/* Begin - bug0000459 */
Datum
uid(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(GetUserId());
}
/* End - bug0000459 */

/* Begin - Bug#Z1064 */
Datum
get_macro_value(PG_FUNCTION_ARGS)
{
	text	*macro_txt = PG_GETARG_TEXT_PP(0);
	char	*macro_str;
	char	*macro_value;

	macro_str = text_to_cstring(macro_txt);

	if (!strcmp(macro_str, "FirstGenbkiObjectId"))
	{
		macro_value = psprintf("%u", FirstGenbkiObjectId);
	}
	else if (!strcmp(macro_str, "FirstBootstrapObjectId"))
	{
		macro_value = psprintf("%u", FirstBootstrapObjectId);
	}
	else if (!strcmp(macro_str, "FirstNormalObjectId"))
	{
		macro_value = psprintf("%u", FirstNormalObjectId);
	}
	else if (!strcmp(macro_str, "MinTypeObjectId"))
	{
		macro_value = psprintf("%u", MinTypeObjectId);
	}
	else if (!strcmp(macro_str, "MaxTypeObjectId"))
	{
		macro_value = psprintf("%u", MaxTypeObjectId);
	}
	else if (!strcmp(macro_str, "InvalidTransactionId"))
	{
		macro_value = psprintf("%u", InvalidTransactionId);
	}
	else if (!strcmp(macro_str, "BootstrapTransactionId"))
	{
		macro_value = psprintf("%u", BootstrapTransactionId);
	}
	else if (!strcmp(macro_str, "FrozenTransactionId"))
	{
		macro_value = psprintf("%u", FrozenTransactionId);
	}
	else if (!strcmp(macro_str, "FirstNormalTransactionId"))
	{
		macro_value = psprintf("%u", FirstNormalTransactionId);
	}
	else if (!strcmp(macro_str, "MaxTransactionId"))
	{
		macro_value = psprintf("%u", MaxTransactionId);
	}
	else
	{
		macro_value = psprintf("%s", "UNKNOW_MACRO");
	}

	PG_RETURN_TEXT_P(CStringGetTextDatum(macro_value));
}
/* End - Bug#Z1064 */
