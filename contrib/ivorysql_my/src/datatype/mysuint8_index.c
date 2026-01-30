/*------------------------------------------------------------------
 * 
 * File: mysuint8_index.c
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
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/*****************************************************************************
 *	 uint8 hash
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint8_hash);
PG_FUNCTION_INFO_V1(uint8_hash_extended);

Datum
uint8_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
uint8_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}
/*****************************************************************************
 *	 uint8 btree
 *****************************************************************************/
PG_FUNCTION_INFO_V1(uint8int1_cmp);
PG_FUNCTION_INFO_V1(uint8int2_cmp);
PG_FUNCTION_INFO_V1(int2uint8_cmp);
PG_FUNCTION_INFO_V1(uint8int4_cmp);
PG_FUNCTION_INFO_V1(int4uint8_cmp);
PG_FUNCTION_INFO_V1(uint8int8_cmp);
PG_FUNCTION_INFO_V1(int8uint8_cmp);
PG_FUNCTION_INFO_V1(uint81_cmp);
PG_FUNCTION_INFO_V1(uint82_cmp);
PG_FUNCTION_INFO_V1(uint84_cmp);
PG_FUNCTION_INFO_V1(uint88_cmp);

Datum
uint8int1_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint8int2_cmp(PG_FUNCTION_ARGS)
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
int2uint8_cmp(PG_FUNCTION_ARGS)
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
uint8int4_cmp(PG_FUNCTION_ARGS)
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
int4uint8_cmp(PG_FUNCTION_ARGS)
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
uint8int8_cmp(PG_FUNCTION_ARGS)
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
int8uint8_cmp(PG_FUNCTION_ARGS)
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
uint81_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint82_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint84_cmp(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint88_cmp(PG_FUNCTION_ARGS)
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
 *	 uint8 --> uint1 uint2 uint4 int1
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint81_eq);
PG_FUNCTION_INFO_V1(uint81_ne);
PG_FUNCTION_INFO_V1(uint81_lt);
PG_FUNCTION_INFO_V1(uint81_gt);
PG_FUNCTION_INFO_V1(uint81_le);
PG_FUNCTION_INFO_V1(uint81_ge);

Datum
uint81_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint81_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint81_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint81_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint81_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint81_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint82_eq);
PG_FUNCTION_INFO_V1(uint82_ne);
PG_FUNCTION_INFO_V1(uint82_lt);
PG_FUNCTION_INFO_V1(uint82_gt);
PG_FUNCTION_INFO_V1(uint82_le);
PG_FUNCTION_INFO_V1(uint82_ge);

Datum
uint82_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint82_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint82_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint82_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint82_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint82_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint84_eq);
PG_FUNCTION_INFO_V1(uint84_ne);
PG_FUNCTION_INFO_V1(uint84_lt);
PG_FUNCTION_INFO_V1(uint84_gt);
PG_FUNCTION_INFO_V1(uint84_le);
PG_FUNCTION_INFO_V1(uint84_ge);

Datum
uint84_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint84_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint84_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint84_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint84_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint84_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint8int1_eq);
PG_FUNCTION_INFO_V1(uint8int1_ne);
PG_FUNCTION_INFO_V1(uint8int1_lt);
PG_FUNCTION_INFO_V1(uint8int1_gt);
PG_FUNCTION_INFO_V1(uint8int1_le);
PG_FUNCTION_INFO_V1(uint8int1_ge);

Datum
uint8int1_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8int1_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint8int1_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint8int1_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint8int1_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint8int1_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}