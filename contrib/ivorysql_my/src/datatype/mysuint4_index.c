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
 *		contrib/ivorysql_my/src/datatype/mysuint1.c
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
 *	 mysuint4 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint4_hash);
PG_FUNCTION_INFO_V1(mysuint4_hash_extended);

Datum
mysuint4_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
mysuint4_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 mysuint4 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint4int2_cmp);
PG_FUNCTION_INFO_V1(int2mysuint4_cmp);
PG_FUNCTION_INFO_V1(mysuint4int4_cmp);
PG_FUNCTION_INFO_V1(int4mysuint4_cmp);
PG_FUNCTION_INFO_V1(mysuint4int8_cmp);
PG_FUNCTION_INFO_V1(int8mysuint4_cmp);
PG_FUNCTION_INFO_V1(mysuint41_cmp);
PG_FUNCTION_INFO_V1(mysuint42_cmp);
PG_FUNCTION_INFO_V1(mysuint44_cmp);
PG_FUNCTION_INFO_V1(mysuint48_cmp);


Datum
mysuint4int2_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int2mysuint4_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint4int4_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int4mysuint4_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint4int8_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int8mysuint4_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint41_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint42_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint44_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint48_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 mysuint4 --> mysuint1 mysuint2 mysuint8
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint41_eq);
PG_FUNCTION_INFO_V1(mysuint41_ne);
PG_FUNCTION_INFO_V1(mysuint41_lt);
PG_FUNCTION_INFO_V1(mysuint41_gt);
PG_FUNCTION_INFO_V1(mysuint41_le);
PG_FUNCTION_INFO_V1(mysuint41_ge);


Datum
mysuint41_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint41_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint41_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint41_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint41_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint41_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint42_eq);
PG_FUNCTION_INFO_V1(mysuint42_ne);
PG_FUNCTION_INFO_V1(mysuint42_lt);
PG_FUNCTION_INFO_V1(mysuint42_gt);
PG_FUNCTION_INFO_V1(mysuint42_le);
PG_FUNCTION_INFO_V1(mysuint42_ge);


Datum
mysuint42_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint42_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint42_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint42_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint42_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint42_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint48_eq);
PG_FUNCTION_INFO_V1(mysuint48_ne);
PG_FUNCTION_INFO_V1(mysuint48_lt);
PG_FUNCTION_INFO_V1(mysuint48_gt);
PG_FUNCTION_INFO_V1(mysuint48_le);
PG_FUNCTION_INFO_V1(mysuint48_ge);


Datum
mysuint48_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint48_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint48_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint48_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint48_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint48_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}
