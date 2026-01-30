/*------------------------------------------------------------------
 * 
 * File: mysuint1_index.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint1_index.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/*****************************************************************************
 *	 uint1 hash
 *****************************************************************************/
PG_FUNCTION_INFO_V1(uint1_hash);
PG_FUNCTION_INFO_V1(uint1_hash_extended);

Datum
uint1_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
uint1_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

/*****************************************************************************
 *	 uint1 btree 
 *   Comparison functions for mysql btree func.
 *   cmp function 
 *****************************************************************************/
PG_FUNCTION_INFO_V1(uint1int1_cmp);
PG_FUNCTION_INFO_V1(uint1int2_cmp);
PG_FUNCTION_INFO_V1(int2uint1_cmp);
PG_FUNCTION_INFO_V1(uint1int4_cmp);
PG_FUNCTION_INFO_V1(int4uint1_cmp);
PG_FUNCTION_INFO_V1(uint1int8_cmp);
PG_FUNCTION_INFO_V1(int8uint1_cmp);
PG_FUNCTION_INFO_V1(uint11_cmp);
PG_FUNCTION_INFO_V1(uint12_cmp);
PG_FUNCTION_INFO_V1(uint14_cmp);
PG_FUNCTION_INFO_V1(uint18_cmp);

Datum
uint1int1_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint1int2_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int2uint1_cmp(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint1int4_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int4uint1_cmp(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint1int8_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int8uint1_cmp(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint11_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint8		val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint12_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint14_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
uint18_cmp(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	 uint1 --> uint2 uint4 uint8 int1
*****************************************************************************/
PG_FUNCTION_INFO_V1(uint12_eq);
PG_FUNCTION_INFO_V1(uint12_ne);
PG_FUNCTION_INFO_V1(uint12_lt);
PG_FUNCTION_INFO_V1(uint12_gt);
PG_FUNCTION_INFO_V1(uint12_le);
PG_FUNCTION_INFO_V1(uint12_ge);

Datum
uint12_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint12_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint12_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint12_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint12_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint12_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint16		val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint14_eq);
PG_FUNCTION_INFO_V1(uint14_ne);
PG_FUNCTION_INFO_V1(uint14_lt);
PG_FUNCTION_INFO_V1(uint14_gt);
PG_FUNCTION_INFO_V1(uint14_le);
PG_FUNCTION_INFO_V1(uint14_ge);

Datum
uint14_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint14_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint14_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint14_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint14_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint14_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint18_eq);
PG_FUNCTION_INFO_V1(uint18_ne);
PG_FUNCTION_INFO_V1(uint18_lt);
PG_FUNCTION_INFO_V1(uint18_gt);
PG_FUNCTION_INFO_V1(uint18_le);
PG_FUNCTION_INFO_V1(uint18_ge);

Datum
uint18_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint18_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint18_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint18_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint18_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint18_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	uint64		val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(uint1int1_eq);
PG_FUNCTION_INFO_V1(uint1int1_ne);
PG_FUNCTION_INFO_V1(uint1int1_lt);
PG_FUNCTION_INFO_V1(uint1int1_gt);
PG_FUNCTION_INFO_V1(uint1int1_le);
PG_FUNCTION_INFO_V1(uint1int1_ge);

Datum
uint1int1_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1int1_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint1int1_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint1int1_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint1int1_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint1int1_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT8(0);
	int8		val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}