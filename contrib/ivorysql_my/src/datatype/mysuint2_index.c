/*------------------------------------------------------------------
 * 
 * File: mysuint2_index.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint2_index.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/*****************************************************************************
 *	 uint2 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint2_hash);
PG_FUNCTION_INFO_V1(uint2_hash_extended);

Datum
uint2_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
uint2_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 uint2 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(uint2int1_cmp);
PG_FUNCTION_INFO_V1(uint2int2_cmp);
PG_FUNCTION_INFO_V1(int2uint2_cmp);
PG_FUNCTION_INFO_V1(uint2int4_cmp);
PG_FUNCTION_INFO_V1(int4uint2_cmp);
PG_FUNCTION_INFO_V1(uint2int8_cmp);
PG_FUNCTION_INFO_V1(int8uint2_cmp);
PG_FUNCTION_INFO_V1(uint21_cmp);
PG_FUNCTION_INFO_V1(uint22_cmp);
PG_FUNCTION_INFO_V1(uint24_cmp);
PG_FUNCTION_INFO_V1(uint28_cmp);

Datum
uint2int1_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint2int2_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int2uint2_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint2int4_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int4uint2_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint2int8_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
int8uint2_cmp(PG_FUNCTION_ARGS)
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
uint21_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint22_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint24_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint28_cmp(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 uint2 --> uint1 uint4 uint8 int1
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint21_eq);
PG_FUNCTION_INFO_V1(uint21_ne);
PG_FUNCTION_INFO_V1(uint21_lt);
PG_FUNCTION_INFO_V1(uint21_gt);
PG_FUNCTION_INFO_V1(uint21_le);
PG_FUNCTION_INFO_V1(uint21_ge);

Datum
uint21_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint21_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint21_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint21_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint21_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint21_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint24_eq);
PG_FUNCTION_INFO_V1(uint24_ne);
PG_FUNCTION_INFO_V1(uint24_lt);
PG_FUNCTION_INFO_V1(uint24_gt);
PG_FUNCTION_INFO_V1(uint24_le);
PG_FUNCTION_INFO_V1(uint24_ge);


Datum
uint24_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint24_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint24_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint24_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint24_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint24_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint28_eq);
PG_FUNCTION_INFO_V1(uint28_ne);
PG_FUNCTION_INFO_V1(uint28_lt);
PG_FUNCTION_INFO_V1(uint28_gt);
PG_FUNCTION_INFO_V1(uint28_le);
PG_FUNCTION_INFO_V1(uint28_ge);

Datum
uint28_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint28_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint28_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint28_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint28_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint28_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint2int1_eq);
PG_FUNCTION_INFO_V1(uint2int1_ne);
PG_FUNCTION_INFO_V1(uint2int1_lt);
PG_FUNCTION_INFO_V1(uint2int1_gt);
PG_FUNCTION_INFO_V1(uint2int1_le);
PG_FUNCTION_INFO_V1(uint2int1_ge);

Datum
uint2int1_eq(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint2int1_ne(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint2int1_lt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint2int1_gt(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint2int1_le(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint2int1_ge(PG_FUNCTION_ARGS)
{
	uint16		val1 = PG_GETARG_UINT16(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}
