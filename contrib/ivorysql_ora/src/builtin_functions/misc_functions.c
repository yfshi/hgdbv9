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

PG_FUNCTION_INFO_V1(uid);	/* bug0000459 */


/* Begin - bug0000459 */
Datum
uid(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(GetUserId());
}
/* End - bug0000459 */
