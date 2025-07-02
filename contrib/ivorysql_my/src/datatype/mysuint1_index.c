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
 *	 mysuint1 hash
 *****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint1_hash);
PG_FUNCTION_INFO_V1(mysuint1_hash_extended);

Datum
mysuint1_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
mysuint1_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 mysuint1 btree 
 *   Comparison functions for mysql btree func.
 *   cmp function 
 *****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint1int2_cmp);
PG_FUNCTION_INFO_V1(int2mysuint1_cmp);
PG_FUNCTION_INFO_V1(mysuint1int4_cmp);
PG_FUNCTION_INFO_V1(int4mysuint1_cmp);
PG_FUNCTION_INFO_V1(mysuint1int8_cmp);
PG_FUNCTION_INFO_V1(int8mysuint1_cmp);
PG_FUNCTION_INFO_V1(mysuint11_cmp);
PG_FUNCTION_INFO_V1(mysuint12_cmp);
PG_FUNCTION_INFO_V1(mysuint14_cmp);
PG_FUNCTION_INFO_V1(mysuint18_cmp);

Datum
mysuint1int2_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int2mysuint1_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint1int4_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int4mysuint1_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint1int8_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int8mysuint1_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint11_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint12_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint14_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint18_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}


/*****************************************************************************
 *	 mysuint1 --> mysuint2 mysuint4 mysuint8
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint12_eq);
PG_FUNCTION_INFO_V1(mysuint12_ne);
PG_FUNCTION_INFO_V1(mysuint12_lt);
PG_FUNCTION_INFO_V1(mysuint12_gt);
PG_FUNCTION_INFO_V1(mysuint12_le);
PG_FUNCTION_INFO_V1(mysuint12_ge);

Datum
mysuint12_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint12_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint12_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint12_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint12_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint12_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint14_eq);
PG_FUNCTION_INFO_V1(mysuint14_ne);
PG_FUNCTION_INFO_V1(mysuint14_lt);
PG_FUNCTION_INFO_V1(mysuint14_gt);
PG_FUNCTION_INFO_V1(mysuint14_le);
PG_FUNCTION_INFO_V1(mysuint14_ge);

Datum
mysuint14_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint14_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint14_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint14_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint14_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint14_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint18_eq);
PG_FUNCTION_INFO_V1(mysuint18_ne);
PG_FUNCTION_INFO_V1(mysuint18_lt);
PG_FUNCTION_INFO_V1(mysuint18_gt);
PG_FUNCTION_INFO_V1(mysuint18_le);
PG_FUNCTION_INFO_V1(mysuint18_ge);

Datum
mysuint18_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint18_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint18_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint18_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint18_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint18_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}