/*-------------------------------------------------------------------------
 *
 * binary_gin.c
 *   for support mysql binary type gin index
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/binary_gin.c
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/stratnum.h"
#include "anybinary.h"
#include "fmgr.h"
#include "utils/builtins.h"

typedef struct QueryInfo
{
	StrategyNumber strategy;
	Datum		datum;
	bool		is_varlena;
	Datum		(*typecmp) (FunctionCallInfo);
} QueryInfo;

/*** GIN support functions shared by all datatypes ***/

static Datum
gin_btree_extract_value(FunctionCallInfo fcinfo, bool is_varlena)
{
	Datum		datum = PG_GETARG_DATUM(0);
	int32	   *nentries = (int32 *) PG_GETARG_POINTER(1);
	Datum	   *entries = (Datum *) palloc(sizeof(Datum));

	if (is_varlena)
		datum = PointerGetDatum(PG_DETOAST_DATUM(datum));
	entries[0] = datum;
	*nentries = 1;

	PG_RETURN_POINTER(entries);
}

/*
 * For BTGreaterEqualStrategyNumber, BTGreaterStrategyNumber, and
 * BTEqualStrategyNumber we want to start the index scan at the
 * supplied query datum, and work forward. For BTLessStrategyNumber
 * and BTLessEqualStrategyNumber, we need to start at the leftmost
 * key, and work forward until the supplied query datum (which must be
 * sent along inside the QueryInfo structure).
 */
static Datum
gin_btree_extract_query(FunctionCallInfo fcinfo,
						bool is_varlena,
						Datum (*leftmostvalue) (void),
						Datum (*typecmp) (FunctionCallInfo))
{
	Datum		datum = PG_GETARG_DATUM(0);
	int32	   *nentries = (int32 *) PG_GETARG_POINTER(1);
	StrategyNumber strategy = PG_GETARG_UINT16(2);
	bool	  **partialmatch = (bool **) PG_GETARG_POINTER(3);
	Pointer   **extra_data = (Pointer **) PG_GETARG_POINTER(4);
	Datum	   *entries = (Datum *) palloc(sizeof(Datum));
	QueryInfo  *data = (QueryInfo *) palloc(sizeof(QueryInfo));
	bool	   *ptr_partialmatch;

	*nentries = 1;
	ptr_partialmatch = *partialmatch = (bool *) palloc(sizeof(bool));
	*ptr_partialmatch = false;
	if (is_varlena)
		datum = PointerGetDatum(PG_DETOAST_DATUM(datum));
	data->strategy = strategy;
	data->datum = datum;
	data->is_varlena = is_varlena;
	data->typecmp = typecmp;
	*extra_data = (Pointer *) palloc(sizeof(Pointer));
	**extra_data = (Pointer) data;

	switch (strategy)
	{
		case BTLessStrategyNumber:
		case BTLessEqualStrategyNumber:
			entries[0] = leftmostvalue();
			*ptr_partialmatch = true;
			break;
		case BTGreaterEqualStrategyNumber:
		case BTGreaterStrategyNumber:
			*ptr_partialmatch = true;
			/* FALLTHROUGH */
		case BTEqualStrategyNumber:
			entries[0] = datum;
			break;
		default:
			elog(ERROR, "unrecognized strategy number: %d", strategy);
	}

	PG_RETURN_POINTER(entries);
}

/*
 * Datum a is a value from extract_query method and for BTLess*
 * strategy it is a left-most value.  So, use original datum from QueryInfo
 * to decide to stop scanning or not.  Datum b is always from index.
 */
static Datum
gin_btree_compare_prefix(FunctionCallInfo fcinfo)
{
	Datum		a = PG_GETARG_DATUM(0);
	Datum		b = PG_GETARG_DATUM(1);
	QueryInfo  *data = (QueryInfo *) PG_GETARG_POINTER(3);
	int32		res,
				cmp;

	cmp = DatumGetInt32(CallerFInfoFunctionCall2(data->typecmp,
													fcinfo->flinfo,
													PG_GET_COLLATION(),
													(data->strategy == BTLessStrategyNumber ||
													data->strategy == BTLessEqualStrategyNumber)
													? data->datum : a,
													b));

	switch (data->strategy)
	{
		case BTLessStrategyNumber:
			/* If original datum > indexed one then return match */
			if (cmp > 0)
				res = 0;
			else
				res = 1;
			break;
		case BTLessEqualStrategyNumber:
			/* The same except equality */
			if (cmp >= 0)
				res = 0;
			else
				res = 1;
			break;
		case BTEqualStrategyNumber:
			if (cmp != 0)
				res = 1;
			else
				res = 0;
			break;
		case BTGreaterEqualStrategyNumber:
			/* If original datum <= indexed one then return match */
			if (cmp <= 0)
				res = 0;
			else
				res = 1;
			break;
		case BTGreaterStrategyNumber:
			/* If original datum <= indexed one then return match */
			/* If original datum == indexed one then continue scan */
			if (cmp < 0)
				res = 0;
			else if (cmp == 0)
				res = -1;
			else
				res = 1;
			break;
		default:
			elog(ERROR, "unrecognized strategy number: %d",
					data->strategy);
			res = 0;
	}

	PG_RETURN_INT32(res);
}

PG_FUNCTION_INFO_V1(gin_btree_consistent_binary);
Datum
gin_btree_consistent_binary(PG_FUNCTION_ARGS)
{
	bool	   *recheck = (bool *) PG_GETARG_POINTER(5);

	*recheck = false;
	PG_RETURN_BOOL(true);
}

/*** GIN_SUPPORT macro defines the datatype specific functions ***/

#define GIN_SUPPORT(type, is_varlena, leftmostvalue, typecmp)				\
PG_FUNCTION_INFO_V1(gin_extract_value_##type);								\
Datum																		\
gin_extract_value_##type(PG_FUNCTION_ARGS)									\
{																			\
	return gin_btree_extract_value(fcinfo, is_varlena);						\
}	\
PG_FUNCTION_INFO_V1(gin_extract_query_##type);								\
Datum																		\
gin_extract_query_##type(PG_FUNCTION_ARGS)									\
{																			\
	return gin_btree_extract_query(fcinfo,									\
								   is_varlena, leftmostvalue, typecmp);		\
}	\
PG_FUNCTION_INFO_V1(gin_compare_prefix_##type);								\
Datum																		\
gin_compare_prefix_##type(PG_FUNCTION_ARGS)									\
{																			\
	return gin_btree_compare_prefix(fcinfo);								\
}

static Datum
leftmostvalue_text(void)
{
	return PointerGetDatum(cstring_to_text_with_len("", 0));
}

GIN_SUPPORT(binary, true, leftmostvalue_text, anybinary_cmp)
GIN_SUPPORT(varbinary, true, leftmostvalue_text, anybinary_cmp)
GIN_SUPPORT(myblob, true, leftmostvalue_text, anybinary_cmp)
GIN_SUPPORT(tinyblob, true, leftmostvalue_text, anybinary_cmp)
GIN_SUPPORT(mediumblob, true, leftmostvalue_text, anybinary_cmp)
GIN_SUPPORT(longblob, true, leftmostvalue_text, anybinary_cmp)