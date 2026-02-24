/*------------------------------------------------------------------
 * 
 * File: mysuint4_index.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint4_index.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/*****************************************************************************
 *	 uint4 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint4_hash);
PG_FUNCTION_INFO_V1(uint4_hash_extended);

Datum
uint4_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
uint4_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 uint4 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(uint4int1_cmp);
PG_FUNCTION_INFO_V1(uint4int2_cmp);
PG_FUNCTION_INFO_V1(int2uint4_cmp);
PG_FUNCTION_INFO_V1(uint4int4_cmp);
PG_FUNCTION_INFO_V1(int4uint4_cmp);
PG_FUNCTION_INFO_V1(uint4int8_cmp);
PG_FUNCTION_INFO_V1(int8uint4_cmp);
PG_FUNCTION_INFO_V1(uint41_cmp);
PG_FUNCTION_INFO_V1(uint42_cmp);
PG_FUNCTION_INFO_V1(uint44_cmp);
PG_FUNCTION_INFO_V1(uint48_cmp);

Datum
uint4int1_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint4int2_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int2uint4_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint4int4_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int4uint4_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint4int8_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int8uint4_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint41_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint42_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint44_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}
Datum
uint48_cmp(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 uint4 --> uint1 uint2 uint8 int1
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint41_eq);
PG_FUNCTION_INFO_V1(uint41_ne);
PG_FUNCTION_INFO_V1(uint41_lt);
PG_FUNCTION_INFO_V1(uint41_gt);
PG_FUNCTION_INFO_V1(uint41_le);
PG_FUNCTION_INFO_V1(uint41_ge);

Datum
uint41_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint41_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint41_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint41_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint41_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint41_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint42_eq);
PG_FUNCTION_INFO_V1(uint42_ne);
PG_FUNCTION_INFO_V1(uint42_lt);
PG_FUNCTION_INFO_V1(uint42_gt);
PG_FUNCTION_INFO_V1(uint42_le);
PG_FUNCTION_INFO_V1(uint42_ge);

Datum
uint42_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint42_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint42_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint42_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint42_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint42_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint48_eq);
PG_FUNCTION_INFO_V1(uint48_ne);
PG_FUNCTION_INFO_V1(uint48_lt);
PG_FUNCTION_INFO_V1(uint48_gt);
PG_FUNCTION_INFO_V1(uint48_le);
PG_FUNCTION_INFO_V1(uint48_ge);

Datum
uint48_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint48_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint48_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint48_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint48_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint48_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint4int1_eq);
PG_FUNCTION_INFO_V1(uint4int1_ne);
PG_FUNCTION_INFO_V1(uint4int1_lt);
PG_FUNCTION_INFO_V1(uint4int1_gt);
PG_FUNCTION_INFO_V1(uint4int1_le);
PG_FUNCTION_INFO_V1(uint4int1_ge);

Datum
uint4int1_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint4int1_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint4int1_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint4int1_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint4int1_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint4int1_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}