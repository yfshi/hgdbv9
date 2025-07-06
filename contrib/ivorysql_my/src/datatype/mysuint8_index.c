/*------------------------------------------------------------------
 * 
 * File: myuint1.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint8_index.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "fmgr.h"
#include <limits.h>
#include <sys/time.h>
#include "access/hash.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "pgtime.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/scansup.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"
#include "utils/numeric.h"
#include "../include/ivorysql_my.h"

/*****************************************************************************
 *	 uint8 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint8_hash);
PG_FUNCTION_INFO_V1(mysuint8_hash_extended);

Datum
mysuint8_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
mysuint8_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}
/*****************************************************************************
 *	 uint8 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint8int2_cmp);
PG_FUNCTION_INFO_V1(int2mysuint8_cmp);
PG_FUNCTION_INFO_V1(mysuint8int4_cmp);
PG_FUNCTION_INFO_V1(int4mysuint8_cmp);
PG_FUNCTION_INFO_V1(mysuint8int8_cmp);
PG_FUNCTION_INFO_V1(int8mysuint8_cmp);
PG_FUNCTION_INFO_V1(mysuint81_cmp);
PG_FUNCTION_INFO_V1(mysuint82_cmp);
PG_FUNCTION_INFO_V1(mysuint84_cmp);
PG_FUNCTION_INFO_V1(mysuint88_cmp);

Datum
mysuint8int2_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int2mysuint8_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint8int4_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int4mysuint8_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint8int8_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int8mysuint8_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint81_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint82_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint84_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint88_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 uint8 --> uint1 uint2 uint4 
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint81_eq);
PG_FUNCTION_INFO_V1(mysuint81_ne);
PG_FUNCTION_INFO_V1(mysuint81_lt);
PG_FUNCTION_INFO_V1(mysuint81_gt);
PG_FUNCTION_INFO_V1(mysuint81_le);
PG_FUNCTION_INFO_V1(mysuint81_ge);

Datum
mysuint81_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint81_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint81_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint81_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint81_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint81_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint82_eq);
PG_FUNCTION_INFO_V1(mysuint82_ne);
PG_FUNCTION_INFO_V1(mysuint82_lt);
PG_FUNCTION_INFO_V1(mysuint82_gt);
PG_FUNCTION_INFO_V1(mysuint82_le);
PG_FUNCTION_INFO_V1(mysuint82_ge);

Datum
mysuint82_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint82_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint82_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint82_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint82_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint82_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint84_eq);
PG_FUNCTION_INFO_V1(mysuint84_ne);
PG_FUNCTION_INFO_V1(mysuint84_lt);
PG_FUNCTION_INFO_V1(mysuint84_gt);
PG_FUNCTION_INFO_V1(mysuint84_le);
PG_FUNCTION_INFO_V1(mysuint84_ge);


Datum
mysuint84_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint84_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint84_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint84_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint84_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint84_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}
