#include "postgres.h"
#include "c.h"
#include "common/hex.h"
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
PG_FUNCTION_INFO_V1(hex2bpchar);	/* bug#z6026 */
PG_FUNCTION_INFO_V1(hex2year);		/* bug#z6028 */
PG_FUNCTION_INFO_V1(hex_positive); 	/* bug#z6031 */
PG_FUNCTION_INFO_V1(hex_negative); 	/* bug#z6039 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
static unsigned char mys_get_bits(const char *bits, int n);
static void mys_bits_decode(const char *src, int len, char *dst);
/* End - SRS-MYSQL-HEX-LITERAL */

static const char hextbl[] = "0123456789abcdef";

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

/* Begin - SRS-MYSQL-HEX-LITERAL */
Datum
parse_bitstring(const char *src)
{
	int		len = strlen(src);
	size_t	bc = (len + BITS_PER_BYTE - 1) / BITS_PER_BYTE + VARHDRSZ ;

	bytea	*result = palloc0(bc) ;
	char	*dst = VARDATA(result);

	int remain = len % BITS_PER_BYTE;
	if (remain)
	{
		*dst++ = mys_get_bits(src, remain);
		src += remain;
		len -= remain;
	}

	mys_bits_decode(src, len, dst);

	SET_VARSIZE(result, bc);

	PG_RETURN_BYTEA_P(result);
}
/* End - SRS-MYSQL-HEX-LITERAL */

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

/* Begin - SRS-MYSQL-HEX-LITERAL */
unsigned char
mys_get_bits(const char *bits, int n)
{
	unsigned char byte = 0;
	int i;

	for (i = 0; i < n; i++)
	{
		byte = (byte << 1) | ((bits[i] - '0'));
	}

	return byte;
}

void
mys_bits_decode(const char *src, int len, char *dst)
{
	int i;

	for (i = 0; i < len; i += 8)
	{
		*dst++ = mys_get_bits(src + i, 8);
	}
}
/* End - SRS-MYSQL-HEX-LITERAL */
/* Begin - bug#z6031 */
Datum
hex_positive(PG_FUNCTION_ARGS)
{
	Datum hex = PG_GETARG_DATUM(0);

	PG_RETURN_DATUM(hex);
}
/* End - bug#z6031 */

/* Begin - bug#z6039 */
Datum
hex_negative(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));

	PG_RETURN_FLOAT8((double) base * (-1));
}
/* End - bug#z6039 */

/* Begin - bug#z6028 */
Datum
hex2year(PG_FUNCTION_ARGS)
{
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	uint64 		base = (uint64) parse_hex_string_to_int64(VARDATA_ANY(hex), VARSIZE_ANY_EXHDR(hex));
	YearADT 	ret = bigint_to_YearADT(base);

	PG_RETURN_YEARADT(ret);
}
/* End - bug#z6028 */

/* Begin - bug#z6026 */
Datum
hex2bpchar(PG_FUNCTION_ARGS)
{
	BpChar	  	*result;
	bytea   	*hex = PG_GETARG_BYTEA_PP(0);
	char		*src = VARDATA_ANY(hex);
	int 		len = VARSIZE_ANY_EXHDR(hex);
	int			atttypmod = PG_GETARG_INT32(1);
	int 		maxlen = atttypmod - VARHDRSZ;
	bool		is_insert = CALL_IN_INSERT();
	char		*dst;

	if (atttypmod < (int32) VARHDRSZ)
		maxlen = len;

	if (len > maxlen)
	{
		if (is_insert)
		{
			if (is_strict_mode())
				ereport(ERROR, errmsg("Data too long for column"));
			else
				ereport(WARNING, errmsg("Data truncated for column"));
			// error
		}
		else
		{
			ereport(WARNING, errmsg("Truncated incorrect CHAR(%d) value: '%.*s'", maxlen, len, src));
		}

		len = maxlen;
	}

	result = (BpChar *) palloc0(maxlen + VARHDRSZ);
	SET_VARSIZE(result, maxlen + VARHDRSZ);
	dst = VARDATA(result);
	memcpy(dst, src, len);

	/* blank pad the string if necessary */
	if (maxlen > len)
		memset(dst + len, ' ', maxlen - len);
	
	PG_RETURN_BPCHAR_P(result);
}
/* End - bug#z6026 */