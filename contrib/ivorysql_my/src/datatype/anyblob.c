/*-------------------------------------------------------------------------
 *
 * anyblob.c
 *   for mysql 'blob/tinyblob/mediumblob/longblob' type
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/anyblob.c
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/detoast.h"
#include "anybinary.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "common/int.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/fmgrprotos.h"
#include "utils/sortsupport.h"
#include "utils/varlena.h"

#define TINYBLOB_MAX_LENGTH     255
#define BLOB_MAX_LENGTH         65535
#define MEDIUMBLOB_MAX_LENGTH   16777215
#define LONGBLOB_MAX_LENGTH     1073741771 /* bug#z5655 */

/**
 * 'pg text' 'blob' 'tinyblob' 'mediumblob' 'longblob' all call this to check length limit.
 * finally, return bytea type.
 */
static bytea *
cast_to_anyblob(bytea *source, int64 max_length,
				const char *typname, bool isExplicit, bool isPg2My)
{
	bytea	*result;
	char 	*s;
	char 	*r;
	int64 	len;

	len = VARSIZE_ANY_EXHDR(source);

	if (len <= max_length)
	{
		return source;
	}

	if (is_strict_mode())
	{
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("value too long for type %s(%ld)",
						typname, max_length)));
		PG_RETURN_VOID();
	}

	ereport(WARNING,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				errmsg("value too long for type %s(%ld), data truncated",
					typname, max_length)));

	s = VARDATA_ANY(source);
	result = palloc(max_length + VARHDRSZ);
	SET_VARSIZE(result, max_length + VARHDRSZ);
	r = VARDATA(result);

	memcpy(r, s, max_length);

	return result;
}

static bytea  *
anyblob_in(char *inputText, int64 max_length, const char *typname)
{
	size_t      len = strlen(inputText);
	size_t		maxmblen = pg_mbcharcliplen(inputText, len, max_length); // actual save length
	bytea	   *result;

	if (len <= max_length)
	{
		result = DatumGetByteaP(DirectFunctionCall1(byteain, CStringGetDatum(inputText)));
		return result;
	}

	if (is_strict_mode())
	{
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("value too long for type %s(%ld)",
							typname, max_length)));
	}

	ereport(WARNING,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				errmsg("value too long for type %s(%ld), data truncated",
						typname, max_length)));

	result = cstring_to_text_with_len(inputText, maxmblen);
	return result;
}

PG_FUNCTION_INFO_V1(myblob);
PG_FUNCTION_INFO_V1(myblob2);
PG_FUNCTION_INFO_V1(myblob_in);
PG_FUNCTION_INFO_V1(myblob_typmodin);
PG_FUNCTION_INFO_V1(myblob_sortsupport);
PG_FUNCTION_INFO_V1(tinyblob);
PG_FUNCTION_INFO_V1(tinyblob2);
PG_FUNCTION_INFO_V1(tinyblob_in);
PG_FUNCTION_INFO_V1(tinyblob_typmodin);
PG_FUNCTION_INFO_V1(tinyblob_sortsupport);
PG_FUNCTION_INFO_V1(mediumblob);
PG_FUNCTION_INFO_V1(mediumblob2);
PG_FUNCTION_INFO_V1(mediumblob_in);
PG_FUNCTION_INFO_V1(mediumblob_typmodin);
PG_FUNCTION_INFO_V1(mediumblob_sortsupport);
PG_FUNCTION_INFO_V1(longblob);
PG_FUNCTION_INFO_V1(longblob2);
PG_FUNCTION_INFO_V1(longblob_in);
PG_FUNCTION_INFO_V1(longblob_typmodin);
PG_FUNCTION_INFO_V1(longblob_sortsupport);
PG_FUNCTION_INFO_V1(mys_repeat);
PG_FUNCTION_INFO_V1(mys_bit_count_binary);
PG_FUNCTION_INFO_V1(mys_bit_count_blob);
PG_FUNCTION_INFO_V1(pgtext2myblob);

Datum
pgtext2myblob(PG_FUNCTION_ARGS)
{
	/* ARG(1) is always '-1' */
	text *source = PG_GETARG_TEXT_PP(0);
	bool   isExplicit = PG_GETARG_BOOL(2);

	/* 'bytea' and 'text' are both 'struct varlena' */
	PG_RETURN_BYTEA_P(cast_to_anyblob((bytea *)source, BLOB_MAX_LENGTH, "blob", isExplicit, true));
}

// reference form orabytea_to_raw_with_typmod
static Datum
myblob_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	bytea    *source = PG_GETARG_BYTEA_PP(0);
	bool		isExplicit = PG_GETARG_BOOL(2);

	PG_RETURN_BYTEA_P(cast_to_anyblob(source, BLOB_MAX_LENGTH, "blob", isExplicit, isPg2My));
}

Datum
myblob(PG_FUNCTION_ARGS)
{
	return myblob_internel(fcinfo, false);
}

Datum
myblob2(PG_FUNCTION_ARGS)
{
	return myblob_internel(fcinfo, true);
}

// reference from varlena.c function byteain()
Datum
myblob_in(PG_FUNCTION_ARGS)
{
	char   *inputText = PG_GETARG_CSTRING(0);

	PG_RETURN_BYTEA_P(anyblob_in(inputText, BLOB_MAX_LENGTH, "blob"));
}

Datum
myblob_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, BLOB_MAX_LENGTH, "blob"));
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum
myblob_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, MYBLOBOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

// reference form orabytea_to_raw_with_typmod
static Datum
tinyblob_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	bytea    *source = PG_GETARG_BYTEA_PP(0);
	bool		isExplicit = PG_GETARG_BOOL(2);

	PG_RETURN_BYTEA_P(cast_to_anyblob(source, TINYBLOB_MAX_LENGTH, "tinyblob", isExplicit, isPg2My));
}

Datum
tinyblob(PG_FUNCTION_ARGS)
{
	return tinyblob_internel(fcinfo, false);
}

Datum
tinyblob2(PG_FUNCTION_ARGS)
{
	return tinyblob_internel(fcinfo, true);
}

Datum
tinyblob_in(PG_FUNCTION_ARGS)
{
	char   *inputText = PG_GETARG_CSTRING(0);

	PG_RETURN_BYTEA_P(anyblob_in(inputText, TINYBLOB_MAX_LENGTH, "tinyblob"));
}

Datum
tinyblob_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, TINYBLOB_MAX_LENGTH, "tinyblob"));
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum
tinyblob_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, TINYBLOBOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

static Datum
mediumblob_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	bytea   *source = PG_GETARG_BYTEA_PP(0);
	bool	isExplicit = PG_GETARG_BOOL(2);

	PG_RETURN_BYTEA_P(cast_to_anyblob(source, MEDIUMBLOB_MAX_LENGTH, "mediumblob", isExplicit, isPg2My));
}

Datum
mediumblob(PG_FUNCTION_ARGS)
{
	return mediumblob_internel(fcinfo, false);
}

Datum
mediumblob2(PG_FUNCTION_ARGS)
{
	return mediumblob_internel(fcinfo, true);
}

Datum
mediumblob_in(PG_FUNCTION_ARGS)
{
	char   *inputText = PG_GETARG_CSTRING(0);

	PG_RETURN_BYTEA_P(anyblob_in(inputText, MEDIUMBLOB_MAX_LENGTH, "mediumblob"));
}

Datum
mediumblob_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, MEDIUMBLOB_MAX_LENGTH, "mediumblob"));
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum
mediumblob_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, MEDIUMBLOBOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

static Datum
longblob_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	bytea   *source = PG_GETARG_BYTEA_PP(0);
	bool	isExplicit = PG_GETARG_BOOL(2);

	PG_RETURN_BYTEA_P(cast_to_anyblob(source, LONGBLOB_MAX_LENGTH, "longblob", isExplicit, isPg2My));
}

Datum
longblob(PG_FUNCTION_ARGS)
{
	return longblob_internel(fcinfo, false);
}

Datum
longblob2(PG_FUNCTION_ARGS)
{
	return longblob_internel(fcinfo, true);
}

Datum
longblob_in(PG_FUNCTION_ARGS)
{
	char   *inputText = PG_GETARG_CSTRING(0);

	PG_RETURN_BYTEA_P(anyblob_in(inputText, LONGBLOB_MAX_LENGTH, "longblob"));
}

Datum
longblob_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, LONGBLOB_MAX_LENGTH, "longblob"));
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum
longblob_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, LONGBLOBOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}

Datum
mys_repeat(PG_FUNCTION_ARGS)
{
	text	   *string = PG_GETARG_TEXT_PP(0);
	int32		count = PG_GETARG_INT32(1);
	text	   *result;
	int			slen,
				tlen;
	int			i;
	char	   *cp,
					*sp;

	if (count < 0)
		count = 0;

	slen = VARSIZE_ANY_EXHDR(string);

	if (unlikely(pg_mul_s32_overflow(count, slen, &tlen)) ||
		unlikely(pg_add_s32_overflow(tlen, VARHDRSZ, &tlen)))
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					errmsg("requested length too large")));

	result = (text *) palloc(tlen);

	SET_VARSIZE(result, tlen);
	cp = VARDATA(result);
	sp = VARDATA_ANY(string);
	for (i = 0; i < count; i++)
	{
		memcpy(cp, sp, slen);
		cp += slen;
		CHECK_FOR_INTERRUPTS();
	}

	PG_RETURN_BYTEA_P(result);
}

const char _my_bits_nbits[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

static inline uint32 
my_count_bits(uint64 v)
{
#if SIZEOF_LONG_LONG > 4
	/* The following code is a bit faster on 16 bit machines than if we would
			only shift v */
	uint32 v2 = (uint32) (v >> 32);
	return (uint32) (uint8) (_my_bits_nbits[(uint8)  v] +
							 _my_bits_nbits[(uint8) (v >> 8)] +
							 _my_bits_nbits[(uint8) (v >> 16)] +
							 _my_bits_nbits[(uint8) (v >> 24)] +
							 _my_bits_nbits[(uint8) (v2)] +
							 _my_bits_nbits[(uint8) (v2 >> 8)] +
							 _my_bits_nbits[(uint8) (v2 >> 16)] +
							 _my_bits_nbits[(uint8) (v2 >> 24)]);
#else
	return (uint32) (uint8) (_my_bits_nbits[(uint8)  v] +
							 _my_bits_nbits[(uint8) (v >> 8)] +
							 _my_bits_nbits[(uint8) (v >> 16)] +
							 _my_bits_nbits[(uint8) (v >> 24)]);
#endif
}

static Datum
mys_bit_count(FunctionCallInfo fcinfo, bool is_binary)
{
	bytea *src = PG_GETARG_BYTEA_P(0);
	char *src_ptr = VARDATA_ANY(src);
	int32 src_len = VARSIZE_ANY_EXHDR(src);
	uint32 cnt;

	uint64 value = atoll(src_ptr);

	ereport(WARNING,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
			 errmsg("Bitwise operations on BINARY will change behavior in a future version, "
					"check the 'Bit functions' section in the manual.")));

	if (is_binary)
	{
		bool show_warning = false;
		for (int32 offset = 0; offset < src_len; ++offset)
		{
			char *tmp_ptr = src_ptr + offset;
			if (!isdigit((unsigned char)*tmp_ptr))
			{
				show_warning = true;
				break;
			}
		}
		if (show_warning)
		{
			for (int32 offset = 0; offset < src_len; ++offset)
			{
				char *tmp_ptr = src_ptr + offset;
				if (tmp_ptr[0] == 0x7F || tmp_ptr[0] < 0x20)
				{
					if (offset > src_len)
						offset = src_len;

					ereport(WARNING,
							(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
							 errmsg("Truncated incorrect INTEGER value: %.*s",
									offset, src_ptr)));
					goto gt;
				}
			}

			ereport(WARNING,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("Truncated incorrect INTEGER value: %.*s", src_len, src_ptr)));
		}
	}

gt:

	cnt = my_count_bits(value);
	PG_RETURN_INT64(cnt);
}

Datum
mys_bit_count_binary(PG_FUNCTION_ARGS)
{
	return mys_bit_count(fcinfo, true);
}

Datum
mys_bit_count_blob(PG_FUNCTION_ARGS)
{
	return mys_bit_count(fcinfo, false);
}
