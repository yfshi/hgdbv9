#include "postgres.h"
#include "c.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "../include/ivorysql_my.h"

#include <assert.h>
#include <float.h>

PG_FUNCTION_INFO_V1(hex_in);
PG_FUNCTION_INFO_V1(hex_out);

// cast
PG_FUNCTION_INFO_V1(hex2int2);
PG_FUNCTION_INFO_V1(hex2int4);
PG_FUNCTION_INFO_V1(hex2int8);
PG_FUNCTION_INFO_V1(hex2uint1);
PG_FUNCTION_INFO_V1(hex2uint2);
PG_FUNCTION_INFO_V1(hex2uint4);
PG_FUNCTION_INFO_V1(hex2uint8);
PG_FUNCTION_INFO_V1(hex2float4);
PG_FUNCTION_INFO_V1(hex2float8);
PG_FUNCTION_INFO_V1(hex2numeric);
PG_FUNCTION_INFO_V1(hex2bool);
PG_FUNCTION_INFO_V1(hex2bit);

static const int8 hexlookup[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static const char hextbl[] = "0123456789abcdef";

char
mys_get_hex(const char *cp)
{
	unsigned char c = (unsigned char) *cp;
	int			res = -1;

	if (c < 127)
		res = hexlookup[c];

	if (res < 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid hexadecimal digit: \"%.*s\"",
						pg_mblen(cp), cp)));

	return (char) res;
}

/**
 * There are some difference from hex_decode(),
 * mys_hex_decode() will not filter special char, like '\n', '\t'.
 */
uint64
mys_hex_decode(const char *src, size_t len, char *dst)
{
	const char *s,
			   *srcend;
	char		v1,
				v2,
			   *p;

	assert(len % 2 == 0);

	srcend = src + len;
	s = src;
	p = dst;
	while (s < srcend)
	{
		v1 = mys_get_hex(s) << 4;
		s++;
		v2 = mys_get_hex(s);
		s++;
		*p++ = v1 | v2;
	}

	return p - dst;
}

/**
 * Hex value without '0x' prefix
 */
Datum
parse_hexstring(const char *src)
{
	int 	len = strlen(src);
	int 	bc = (len + 1) / 2  + VARHDRSZ;

	bytea	*result = palloc0(bc);
	char	*dst = VARDATA(result);

	if (len % 2)
	{
		*dst++ = mys_get_hex(src++);
		len--;
	}

	mys_hex_decode(src, len, dst);

	SET_VARSIZE(result, bc);

	PG_RETURN_BYTEA_P(result);
}

Datum
hex_in(PG_FUNCTION_ARGS)
{
	return parse_hexstring(PG_GETARG_CSTRING(0));
}

Datum
hex_out(PG_FUNCTION_ARGS)
{
    bytea       *hex = PG_GETARG_BYTEA_PP(0);
	char        *result = palloc0(VARSIZE_ANY_EXHDR(hex) + 1);

    memcpy(result, VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));

    PG_RETURN_CSTRING(result);
}

static int64
parse_hex_string_to_int64(char *hex, int len)
{
	int64	value = 0;
	char 	*end = hex + len;

	if (len > sizeof(int64))
	{
		const char *lost_end = end - sizeof(int64);
		for (char *p = hex; p < lost_end; p++)
		{
			if (*p != '\0')
			{				
				char errbuff[512], *errptr= errbuff;

				*errptr++ = 'x';
				*errptr++ = '\'';

				for (p = hex; p < end; p++)
				{
					if (errptr > errbuff + sizeof(errbuff) - 4)
						break;
					*errptr++ = hextbl[((uint8) *p) >> 4];
					*errptr++ = hextbl[((uint8) *p) & 0x0F];
				}

				*errptr++ = '\'';
				*errptr++ = '\0';

				ereport(WARNING,
						errmsg("Truncated incorrect BINARY value: %s", errbuff));

				return -1;
			}
		}
	}

	for (int i = 0; i < len; i++)
	{
		value = (value << 8) + (uint8) hex[i];
	}

	return value;
}

Datum
hex2int2(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	int16 		ret = (int16) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > PG_INT16_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = PG_INT16_MAX;
		}
	}

	PG_RETURN_INT16(ret);
}

Datum
hex2int4(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	int32 		ret = (int32) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > INT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = INT32_MAX;
		}
	}

	PG_RETURN_INT32(ret);
}

Datum
hex2int8(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	int64 		ret = base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > INT64_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = INT64_MAX;
		}
	}

	PG_RETURN_INT64(ret);
}

Datum
hex2uint1(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	uint8 		ret = (uint8) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > PG_UINT8_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = PG_UINT8_MAX;
		}
	}

	PG_RETURN_UINT8(ret);
}

Datum
hex2uint2(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	uint16 		ret = (uint16) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > PG_UINT16_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = PG_UINT16_MAX;
		}
	}

	PG_RETURN_UINT16(ret);
}

Datum
hex2uint4(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	uint32 		ret = (uint32) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > PG_UINT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = PG_UINT32_MAX;
		}
	}

	PG_RETURN_UINT32(ret);
}

Datum
hex2uint8(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		value = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));

	PG_RETURN_UINT64(value);
}

Datum
hex2float4(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	float 		ret = (float) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > FLT_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = FLT_MAX;
		}
	}
	
	PG_RETURN_FLOAT4(ret);
}

Datum
hex2float8(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	double 		ret = (double) base;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (base > DBL_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			ret = DBL_MAX;
		}
	}
	
	PG_RETURN_FLOAT8(ret);
}

Datum
hex2numeric(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	int64 		value = parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));

	PG_RETURN_NUMERIC(NumericGetDatum(uint64_to_numeric(value)));
}

Datum
hex2bool(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		value = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));

	PG_RETURN_BOOL(!!value);
}

Datum
hex2bit(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	int32		atttypmod = PG_GETARG_INT32(1);
	char 		*src = VARDATA_ANY(hex);
	int32		len = VARSIZE_ANY_EXHDR(hex);
	int32		bitlen = len * BITS_PER_BYTE;
	VarBit		*result;

	if (atttypmod <= 0)
		atttypmod = bitlen;
	else if (bitlen != atttypmod)
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
				 errmsg("bit string length %d does not match type bit(%d)",
						bitlen, atttypmod)));
	
	len = VARBITTOTALLEN(atttypmod);
	result = (VarBit *) palloc0(len);
	SET_VARSIZE(result, len);
	VARBITLEN(result) = atttypmod;

	memcpy(VARBITS(result), src, len);

	PG_RETURN_VARBIT_P(result);
}
