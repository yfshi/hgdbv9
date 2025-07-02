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
 *	 mysuint2 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint2_hash);
PG_FUNCTION_INFO_V1(mysuint2_hash_extended);

Datum
mysuint2_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
mysuint2_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 mysuint2 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint2int2_cmp);
PG_FUNCTION_INFO_V1(int2mysuint2_cmp);
PG_FUNCTION_INFO_V1(mysuint2int4_cmp);
PG_FUNCTION_INFO_V1(int4mysuint2_cmp);
PG_FUNCTION_INFO_V1(mysuint2int8_cmp);
PG_FUNCTION_INFO_V1(int8mysuint2_cmp);
PG_FUNCTION_INFO_V1(mysuint21_cmp);
PG_FUNCTION_INFO_V1(mysuint22_cmp);
PG_FUNCTION_INFO_V1(mysuint24_cmp);
PG_FUNCTION_INFO_V1(mysuint28_cmp);


Datum
mysuint2int2_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int2mysuint2_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint2int4_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int4mysuint2_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint2int8_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int8mysuint2_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint21_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint22_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint24_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
mysuint28_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 mysuint2 --> mysuint1 mysuint4 mysuint8
*****************************************************************************/
PG_FUNCTION_INFO_V1(mysuint21_eq);
PG_FUNCTION_INFO_V1(mysuint21_ne);
PG_FUNCTION_INFO_V1(mysuint21_lt);
PG_FUNCTION_INFO_V1(mysuint21_gt);
PG_FUNCTION_INFO_V1(mysuint21_le);
PG_FUNCTION_INFO_V1(mysuint21_ge);


Datum
mysuint21_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint21_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint21_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint21_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint21_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint21_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint24_eq);
PG_FUNCTION_INFO_V1(mysuint24_ne);
PG_FUNCTION_INFO_V1(mysuint24_lt);
PG_FUNCTION_INFO_V1(mysuint24_gt);
PG_FUNCTION_INFO_V1(mysuint24_le);
PG_FUNCTION_INFO_V1(mysuint24_ge);


Datum
mysuint24_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint24_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint24_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint24_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint24_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint24_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(mysuint28_eq);
PG_FUNCTION_INFO_V1(mysuint28_ne);
PG_FUNCTION_INFO_V1(mysuint28_lt);
PG_FUNCTION_INFO_V1(mysuint28_gt);
PG_FUNCTION_INFO_V1(mysuint28_le);
PG_FUNCTION_INFO_V1(mysuint28_ge);


Datum
mysuint28_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint28_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint28_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint28_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint28_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint28_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

