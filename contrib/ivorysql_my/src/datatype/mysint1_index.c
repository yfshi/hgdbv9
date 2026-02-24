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
 *	int1 hash
 *****************************************************************************/
PG_FUNCTION_INFO_V1(int1_hash);
PG_FUNCTION_INFO_V1(int1_hash_extended);

Datum
int1_hash(PG_FUNCTION_ARGS)
{
	return hash_uint32((int32) PG_GETARG_INT8(0));
}

Datum
int1_hash_extended(PG_FUNCTION_ARGS)
{
	return hash_uint32_extended((int32) PG_GETARG_INT8(0), PG_GETARG_INT64(1));
}

/*****************************************************************************
 *	int1 btree
 *  Comparison functions for mysql btree func.
 *  cmp function
 *****************************************************************************/
PG_FUNCTION_INFO_V1(int1int1_cmp);
PG_FUNCTION_INFO_V1(int1int2_cmp);
PG_FUNCTION_INFO_V1(int2int1_cmp);
PG_FUNCTION_INFO_V1(int1int4_cmp);
PG_FUNCTION_INFO_V1(int4int1_cmp);
PG_FUNCTION_INFO_V1(int1int8_cmp);
PG_FUNCTION_INFO_V1(int8int1_cmp);
PG_FUNCTION_INFO_V1(int1uint1_cmp);
PG_FUNCTION_INFO_V1(int1uint2_cmp);
PG_FUNCTION_INFO_V1(int1uint4_cmp);
PG_FUNCTION_INFO_V1(int1uint8_cmp);
PG_FUNCTION_INFO_V1(btint1sortsupport);

Datum
int1int1_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1int2_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int2int1_cmp(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1int4_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int4int1_cmp(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1int8_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int8int1_cmp(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1uint1_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1uint2_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1uint4_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

Datum
int1uint8_cmp(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	if (val1 < val2)
		PG_RETURN_INT32(A_LESS_THAN_B);
	else if (val1 > val2)
		PG_RETURN_INT32(A_GREATER_THAN_B);
	PG_RETURN_INT32(A_EQUAL_B);
}

/*****************************************************************************
 *	int1 --> int2 int4 int8 uint1 uint2 uint4 uint8
*****************************************************************************/
PG_FUNCTION_INFO_V1(int1int1_eq);
PG_FUNCTION_INFO_V1(int1int1_ne);
PG_FUNCTION_INFO_V1(int1int1_lt);
PG_FUNCTION_INFO_V1(int1int1_gt);
PG_FUNCTION_INFO_V1(int1int1_le);
PG_FUNCTION_INFO_V1(int1int1_ge);

Datum
int1int1_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1int1_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1int1_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1int1_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1int1_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1int1_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1int2_eq);
PG_FUNCTION_INFO_V1(int1int2_ne);
PG_FUNCTION_INFO_V1(int1int2_lt);
PG_FUNCTION_INFO_V1(int1int2_gt);
PG_FUNCTION_INFO_V1(int1int2_le);
PG_FUNCTION_INFO_V1(int1int2_ge);

Datum
int1int2_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1int2_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1int2_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1int2_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1int2_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1int2_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int16	val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1int4_eq);
PG_FUNCTION_INFO_V1(int1int4_ne);
PG_FUNCTION_INFO_V1(int1int4_lt);
PG_FUNCTION_INFO_V1(int1int4_gt);
PG_FUNCTION_INFO_V1(int1int4_le);
PG_FUNCTION_INFO_V1(int1int4_ge);

Datum
int1int4_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1int4_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1int4_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1int4_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1int4_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1int4_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int32	val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1int8_eq);
PG_FUNCTION_INFO_V1(int1int8_ne);
PG_FUNCTION_INFO_V1(int1int8_lt);
PG_FUNCTION_INFO_V1(int1int8_gt);
PG_FUNCTION_INFO_V1(int1int8_le);
PG_FUNCTION_INFO_V1(int1int8_ge);

Datum
int1int8_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1int8_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1int8_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1int8_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1int8_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1int8_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	int64	val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int2int1_eq);
PG_FUNCTION_INFO_V1(int2int1_ne);
PG_FUNCTION_INFO_V1(int2int1_lt);
PG_FUNCTION_INFO_V1(int2int1_gt);
PG_FUNCTION_INFO_V1(int2int1_le);
PG_FUNCTION_INFO_V1(int2int1_ge);

Datum
int2int1_eq(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2int1_ne(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int2int1_lt(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int2int1_gt(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2int1_le(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2int1_ge(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int4int1_eq);
PG_FUNCTION_INFO_V1(int4int1_ne);
PG_FUNCTION_INFO_V1(int4int1_lt);
PG_FUNCTION_INFO_V1(int4int1_gt);
PG_FUNCTION_INFO_V1(int4int1_le);
PG_FUNCTION_INFO_V1(int4int1_ge);

Datum
int4int1_eq(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int4int1_ne(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int4int1_lt(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int4int1_gt(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int4int1_le(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int4int1_ge(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int8int1_eq);
PG_FUNCTION_INFO_V1(int8int1_ne);
PG_FUNCTION_INFO_V1(int8int1_lt);
PG_FUNCTION_INFO_V1(int8int1_gt);
PG_FUNCTION_INFO_V1(int8int1_le);
PG_FUNCTION_INFO_V1(int8int1_ge);

Datum
int8int1_eq(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int8int1_ne(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int8int1_lt(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int8int1_gt(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int8int1_le(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int8int1_ge(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = PG_GETARG_INT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1uint1_eq);
PG_FUNCTION_INFO_V1(int1uint1_ne);
PG_FUNCTION_INFO_V1(int1uint1_lt);
PG_FUNCTION_INFO_V1(int1uint1_gt);
PG_FUNCTION_INFO_V1(int1uint1_le);
PG_FUNCTION_INFO_V1(int1uint1_ge);

Datum
int1uint1_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1uint1_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1uint1_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1uint1_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1uint1_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1uint1_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint8	val2 = PG_GETARG_UINT8(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1uint2_eq);
PG_FUNCTION_INFO_V1(int1uint2_ne);
PG_FUNCTION_INFO_V1(int1uint2_lt);
PG_FUNCTION_INFO_V1(int1uint2_gt);
PG_FUNCTION_INFO_V1(int1uint2_le);
PG_FUNCTION_INFO_V1(int1uint2_ge);

Datum
int1uint2_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1uint2_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1uint2_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1uint2_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1uint2_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1uint2_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint16	val2 = PG_GETARG_UINT16(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1uint4_eq);
PG_FUNCTION_INFO_V1(int1uint4_ne);
PG_FUNCTION_INFO_V1(int1uint4_lt);
PG_FUNCTION_INFO_V1(int1uint4_gt);
PG_FUNCTION_INFO_V1(int1uint4_le);
PG_FUNCTION_INFO_V1(int1uint4_ge);

Datum
int1uint4_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1uint4_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1uint4_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1uint4_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1uint4_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1uint4_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint32	val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

PG_FUNCTION_INFO_V1(int1uint8_eq);
PG_FUNCTION_INFO_V1(int1uint8_ne);
PG_FUNCTION_INFO_V1(int1uint8_lt);
PG_FUNCTION_INFO_V1(int1uint8_gt);
PG_FUNCTION_INFO_V1(int1uint8_le);
PG_FUNCTION_INFO_V1(int1uint8_ge);

Datum
int1uint8_eq(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int1uint8_ne(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int1uint8_lt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int1uint8_gt(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int1uint8_le(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int1uint8_ge(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
	uint64	val2 = PG_GETARG_UINT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

/*****************************************************************************
 *	btree sortsupport for int1
 *****************************************************************************/
static int
btint1fastcmp(Datum x, Datum y, SortSupport ssup)
{
	int8	a = DatumGetInt8(x);
	int8	b = DatumGetInt8(y);

	return (int) a - (int) b;
}

Datum
btint1sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);

	ssup->comparator = btint1fastcmp;
	PG_RETURN_VOID();
}