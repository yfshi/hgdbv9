/*------------------------------------------------------------------
 *
 * File: mysint1.c
 *
 * Abstract:
 * 		Compatible with MySQL's int1 type.
 *
 * Authored by tanyang@highgo.com, 20250415.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysint1.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "common/int.h"
#include "utils/builtins.h"
#include "utils/guc.h"

PG_FUNCTION_INFO_V1(myint1_positive);

#define PG_RETURN_INT8(x)	 return Int8GetDatum(x)

Datum
myint1_positive(PG_FUNCTION_ARGS)
{
	int16 num = PG_GETARG_INT16(0);

	PG_RETURN_INT8(num);
}

