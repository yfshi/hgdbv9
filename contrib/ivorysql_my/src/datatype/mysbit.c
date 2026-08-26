/*------------------------------------------------------------------
 * 
 * File: mysbit.c
 *
 * Abstract: 
 * 		Compatible with MySQL's BIT data type.
 *
 * Authored by liyonghao@highgo.com, 202505020
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysbit.c
 *
 * ReqID:SRS-MYSQL-BITTYPE
 * 
 *------------------------------------------------------------------
 */

#include <math.h>

#include <math.h>
#include "postgres.h"
#include "fmgr.h"
#include "common/hashfn.h"
#include "common/hashfn.h"
#include "common/hex.h"
#include "common/int.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "utils/varbit.h"
#include "../include/ivorysql_my.h"

/* Mask off any bits that should be zero in the last byte of a bitstring */
#define VARBIT_PAD(vb) \
	do { \
		int32	pad_ = VARBITPAD(vb); \
		Assert(pad_ >= 0 && pad_ < BITS_PER_BYTE); \
		if (pad_ > 0) \
			*(VARBITS(vb) + VARBITBYTES(vb) - 1) &= BITMASK << pad_; \
	} while (0)

/* Assert proper padding of a bitstring */
#ifdef USE_ASSERT_CHECKING
#define VARBIT_CORRECTLY_PADDED(vb) \
	do { \
		int32	pad_ = VARBITPAD(vb); \
		Assert(pad_ >= 0 && pad_ < BITS_PER_BYTE); \
		Assert(pad_ == 0 || \
			   (*(VARBITS(vb) + VARBITBYTES(vb) - 1) & ~(BITMASK << pad_)) == 0); \
	} while (0)
#else
#define VARBIT_CORRECTLY_PADDED(vb) ((void) 0)
#endif

#define MYSBIT_MAX_VALUE(typmod) \
		(typmod == 64 ? ULLONG_MAX : (1ULL << typmod) - 1)

#define mi_int4store(T,A) { uint32 def_temp = (uint32) (A);\
		((uchar*) (T))[3]= (uchar) (def_temp);\
		((uchar*) (T))[2]= (uchar) (def_temp >> 8);\
		((uchar*) (T))[1]= (uchar) (def_temp >> 16);\
		((uchar*) (T))[0]= (uchar) (def_temp >> 24); }

#define mi_int8store(T,A) { uint32 def_temp3 = (uint32) (A),\
		def_temp4 = (uint32) ((A) >> 32);\
		mi_int4store((uchar*) (T) + 0, def_temp4);\
		mi_int4store((uchar*) (T) + 4, def_temp3); }

PG_FUNCTION_INFO_V1(mysql_bit_in);
PG_FUNCTION_INFO_V1(mysql_bit_in2);
PG_FUNCTION_INFO_V1(mysql_bit_out);
PG_FUNCTION_INFO_V1(mysbitfrompgbit);
PG_FUNCTION_INFO_V1(bitfrombool);
PG_FUNCTION_INFO_V1(bitfromuint8);
PG_FUNCTION_INFO_V1(bitfromvarbinary);
PG_FUNCTION_INFO_V1(bittobool);
PG_FUNCTION_INFO_V1(bittobinary);
PG_FUNCTION_INFO_V1(bittouint8);
PG_FUNCTION_INFO_V1(bittotext);
PG_FUNCTION_INFO_V1(mysbit_eq);
PG_FUNCTION_INFO_V1(mysbit_ne);
PG_FUNCTION_INFO_V1(mysbit_lt);
PG_FUNCTION_INFO_V1(mysbit_le);
PG_FUNCTION_INFO_V1(mysbit_ge);
PG_FUNCTION_INFO_V1(mysbit_gt);
PG_FUNCTION_INFO_V1(mysbit_safeeq);
PG_FUNCTION_INFO_V1(mysbit_bit_and);
PG_FUNCTION_INFO_V1(mysbit_left_shift_mysbit);
PG_FUNCTION_INFO_V1(mysbit_right_shift_mysbit);
PG_FUNCTION_INFO_V1(mysbit_left_shift_int4);
PG_FUNCTION_INFO_V1(mysbit_right_shift_int4);
PG_FUNCTION_INFO_V1(mysbit_bit_xor);
PG_FUNCTION_INFO_V1(mysbit_bit_or);
PG_FUNCTION_INFO_V1(mysbit_bit_neg);
PG_FUNCTION_INFO_V1(mysbit_positive);
PG_FUNCTION_INFO_V1(mysbit_exact_div);
PG_FUNCTION_INFO_V1(mysbit_or);
PG_FUNCTION_INFO_V1(mysbit_and);
PG_FUNCTION_INFO_V1(mysbit_not);
PG_FUNCTION_INFO_V1(mysbit_like);
PG_FUNCTION_INFO_V1(mysbit_notlike);
PG_FUNCTION_INFO_V1(mysbit_hash);
PG_FUNCTION_INFO_V1(mysbit_hash_extended);
PG_FUNCTION_INFO_V1(bit_uint8);
PG_FUNCTION_INFO_V1(mysbit_length);

static int
get_bit_length(uint8 *s, int len)
{
	int 	bitlen = 0;
	uint8 	*e = s + len;

	while(*s == 0x00 && s < e) 
	{
		s++;
		len--;
	}
	
	// all bytes is 0x00
	if (s == e)
		return 0;
	
	bitlen = len * BITS_PER_BYTE;

	/* Ignore the leading 0 in the first byte. */
	for (int i = 0; i < BITS_PER_BYTE; i++)
	{
		uint8 probe = HIGHBIT >> i;

		if ((*s & probe) != 0)
			break;
		
		bitlen--;
	}

	return bitlen;
}

static void
fill_mysbit(VarBit *bit, uint8 *s, int len, bool is_insert)
{
	int 	atttypmod = VARBITLEN(bit);
	int 	bytes = VARBITBYTES(bit);
	int 	pad = VARBITPAD(bit);
	int 	bits = get_bit_length(s, len);
	uint8 	*e = s + len - 1, 
			*dst = VARBITS(bit) + bytes - 1;

	if (bits == 0)
		return;

	if (bits > atttypmod)
	{
		if (is_insert)
		{
			if (is_strict_mode())
			{
				ereport(ERROR,
						(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
						 errmsg("Data too long for column")));
			}
			else
			{
				ereport(WARNING,
						(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
					 	 errmsg("Out of range value for column")));
			}
		}
		else
		{
			ereport(WARNING,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type bit(%d)", atttypmod)));
		}

		// full fill with 1
		memset(VARBITS(bit), 0xff, bytes);

		VARBIT_PAD(bit);

		return;
	}

	*(dst--) = *e << pad;

	while (s < e)
	{
		*(dst--) = (*(e - 1) << pad) | (*e >> (BITS_PER_BYTE - pad));
		e--;
	}

	*dst |= *e >> (BITS_PER_BYTE - pad);
}

static VarBit *
string_to_varbit(char *s, int atttypmod, int slen, bool is_insert)
{
	int		size = VARBITTOTALLEN(atttypmod);
	VarBit	*result = (VarBit *) palloc0(size);

	SET_VARSIZE(result, size);
	VARBITLEN(result) = atttypmod;

	fill_mysbit(result, (uint8 *) s, slen, is_insert);

	return result;
}

static uint64
bit_to_uint64(VarBit *arg)
{
	uint64		result;
	bits8	   *r;

	/* Check that the bit string is not too long */
	if (VARBITLEN(arg) > sizeof(result) * BITS_PER_BYTE)
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("bigint out of range")));

	result = 0;
	for (r = VARBITS(arg); r < VARBITEND(arg); r++)
	{
		result <<= BITS_PER_BYTE;
		result |= *r;
	}
	/* Now shift the result to take account of the padding at the end */
	result >>= VARBITPAD(arg);

	return result;
}

Datum
mysql_bit_in(PG_FUNCTION_ARGS)
{
	char	*input_string = PG_GETARG_CSTRING(0);	
	int		atttypmod = PG_GETARG_INT32(2);
	int		slen = strlen(input_string);
	bool	is_insert = CALL_IN_INSERT();

	/*
	 * Sometimes atttypmod is not supplied. If it is supplied we need to make
	 * sure that the bitstring fits.
	 */
	if (atttypmod <= 0)
		atttypmod = slen * BITS_PER_BYTE;
	
	PG_RETURN_VARBIT_P(string_to_varbit(input_string, atttypmod, slen, is_insert));
}

Datum
mysql_bit_out(PG_FUNCTION_ARGS)
{
	VarBit *input = PG_GETARG_VARBIT_P(0);
	int input_len = VARBITLEN(input);
	bits8 *input_bits = VARBITS(input);
	VarBit *padded;
	char *hex_output;
	bits8 *padded_bits;
	int result_len = 0;
	int i, j;

	int pad_len = 8 - (input_len % 8); // The number of digits to padding zeros

	if (pad_len == 8)
		pad_len = 0;
	
	result_len = input_len + pad_len;

	padded = (VarBit *)palloc(VARHDRSZ + result_len); //create a new varbit struct to store the result of padding zeros
	SET_VARSIZE(padded, VARHDRSZ + result_len);		 
	padded_bits = VARBITS(padded);

	memset(padded_bits, 0, result_len / 8);

	if (pad_len == 0)
	{
		for (i = 0; i < result_len / 8; i++)
			padded_bits[i] = input_bits[i];
	}
	else
	{
		padded_bits[0] = input_bits[0] >> (8 - (input_len % 8));
		for (i = 1; i < result_len / 8; i++)
		{
			padded_bits[i] = (input_bits[i] >> (8 - (input_len % 8))) | (input_bits[i - 1] << (input_len % 8));
		}
	}

	/*
	 * hex_output stores the varbit converted hex string.
	 * 'result_len' is the number of hex characters required.
	 * Every four bit digits are converted to one hex character.
	 * 1 is '\0' at the ending of the hex string.
	 * 2 is '0x' at the beginning of the hex string.
	 * 
	 */
	hex_output = (char *)palloc(result_len / 4 + 1 + 2); 

	hex_output[0] = '0';
	hex_output[1] = 'x';

	// cast to hex string
	for (i = result_len / 8 - 1, j = result_len / 4 - 1; j >= 0; j = j - 2, i--)
	{
		int8 nibble1 = 0;
		int8 nibble2 = 0;

		bits8 tmp = padded_bits[i] << 4;
		nibble2 = tmp >> 4;
		nibble1 = padded_bits[i] >> 4;
		hex_output[j + 2] = (nibble2 < 10) ? ('0' + nibble2) : ('A' + nibble2 - 10);
		hex_output[j + 1] = (nibble1 < 10) ? ('0' + nibble1) : ('A' + nibble1 - 10);
	}
	hex_output[result_len / 4 + 2] = '\0';

	pfree(padded);

	PG_RETURN_CSTRING(hex_output);
}

Datum
mysbitfrompgbit(PG_FUNCTION_ARGS)
{
	VarBit	   *arg = PG_GETARG_VARBIT_P(0);
	int32		typmod = PG_GETARG_INT32(1);
	VarBit	   *result;
	int			rlen;
	int			padlen;
	int			i, s;
	int			delta, pre;
	bits8		*result_string, *bin_string;		/* pointer into the result */
	bits8		x = 0,
				y = 0;

	/* No work if typmod is invalid or supplied data matches it already */
	if (typmod <= 0 || typmod > VARBITMAXLEN || typmod == VARBITLEN(arg))
		PG_RETURN_VARBIT_P(arg);
	
	if (typmod <= VARBITLEN(arg))
	{
		if (is_strict_mode())
		{

			ereport(ERROR,
			(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
				errmsg("bit string length %d does not match type bit(%d)",
					VARBITLEN(arg), typmod)));
		}

		ereport(WARNING,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				errmsg("value too long for type bit(%d)",
					typmod)));

		PG_RETURN_VARBIT_P(DatumGetVarBitP(DirectFunctionCall2(bitfromuint8, 
															   UInt64GetDatum(MYSBIT_MAX_VALUE(typmod)),
															   Int64GetDatum(typmod))));
	}

	rlen = VARBITTOTALLEN(typmod);
	/* set to 0 so that string is zero-padded */
	result = (VarBit *) palloc0(rlen);
	SET_VARSIZE(result, rlen);
	VARBITLEN(result) = typmod;

	padlen = typmod - VARBITLEN(arg);
	delta = padlen / 8; //skip bytes numbers
	pre = padlen % 8; // each byte offset bits
	result_string = VARBITS(result);
	bin_string = VARBITS(arg);

	x = *bin_string;
	y = 0;	
	
	result_string += delta;	//skip zero, delta * 8

	if (typmod > 8)
	{
		i = 0;
		s = pre + VARBITLEN(arg);
		while (1)
		{
			/* Combine the low pre bits of the previous one with the high 8-pre bits of the next one */
			*result_string = (y << (8 - pre)) | (x >> pre); 

			y = *bin_string++;
			x = *bin_string;	
			result_string++;

			i += 8;
			s -= 8;
			if (i >= VARBITLEN(arg))
				break;
		}

		/*Append the low pre bits of the previous digit to the end of result_string*/
		if (s > 0)
			*result_string = (y << (8 - pre)) | 0;
	}
	else
	{
		/*
		 * If typmod < 8, then only handle single bytes, 
		 * directly shift the input string to the right by pre positions
		 */
		*result_string = (x >> pre) | 0;
	}

	/*
	 * Make sure last byte is zero-padded if needed.  This is useless but safe
	 * if source data was shorter than target length (we assume the last byte
	 * of the source data was itself correctly zero-padded).
	 */
	VARBIT_PAD(result);

	PG_RETURN_VARBIT_P(result);
}

Datum
bitfrombool(PG_FUNCTION_ARGS)
{
	bool		b = PG_GETARG_BOOL(0);
	int32		typmod = PG_GETARG_INT32(1);
	VarBit	   *result;	

	if (b)
	{	
		result = DatumGetVarBitP(DirectFunctionCall2(bitfromint4, 
								Int64GetDatum(1),
								Int64GetDatum(typmod)));
	}
	else
	{
		result = DatumGetVarBitP(DirectFunctionCall2(bitfromint4, 
							Int64GetDatum(0),
							Int64GetDatum(typmod)));
	}

	PG_RETURN_VARBIT_P(result);
}

Datum
bittobool(PG_FUNCTION_ARGS)
{
	VarBit	*arg0 = PG_GETARG_VARBIT_P(0);
	int64	tmp0 = 0;

	tmp0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));

	PG_RETURN_BOOL(tmp0 ? true : false);
}

Datum
bittobinary(PG_FUNCTION_ARGS)
{
    VarBit* s = PG_GETARG_VARBIT_P(0);
	uint64  num = bit_to_uint64(s);
	StringInfoData buf;

	pq_begintypsend(&buf);

	/* Begin - bug#z5768 */
	for (int i = 56; i >= 0; i -= 8)
	{
		if (VARBITLEN(s) > i)
			appendStringInfoChar(&buf, (char)((num >> i) & 0xFF));	
	}
	/* End - bug#z5768 */
	
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
bittouint8(PG_FUNCTION_ARGS)
{
	VarBit	   *arg = PG_GETARG_VARBIT_P(0);
	uint64		result;

	result = bit_to_uint64(arg);

	PG_RETURN_UINT64(result);
}

Datum
bittotext(PG_FUNCTION_ARGS)
{
	VarBit	   *arg = PG_GETARG_VARBIT_P(0);
	int 		bit_len, char_len;
	uint64		tmp = bit_to_uint64(arg);

	char buff[sizeof(int64)];
	char *valbuffer;

	if (tmp == 0)
	{
		char_len = 1;
	}
	else 
	{
		bit_len = (int) (floor(log2(tmp)) + 1);
		char_len = (bit_len + 7) / 8;
	}

	valbuffer = (char *)palloc(char_len); 

	mi_int8store(buff, tmp);
	memcpy(valbuffer, buff + sizeof(int64) - char_len, char_len);
	
	PG_RETURN_TEXT_P(cstring_to_text_with_len(valbuffer, char_len));
}

Datum
bitfromuint8(PG_FUNCTION_ARGS)
{
	uint64		arg = PG_GETARG_UINT64(0);
	int32		typmod = PG_GETARG_INT32(1);
	VarBit	   *result;
	bits8	   *r;
	uint64		value_max; 
	int			rlen;
	int			destbitsleft,
				srcbitsleft;

	if (typmod <= 0 || typmod > VARBITMAXLEN)
		typmod = 1;				/* default bit length */

	if (typmod > 64)
	{
		ereport(ERROR,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				errmsg("value too long for type bit(%d)",
						typmod)));
	}

	if (typmod == 64)
	{
		value_max = ULLONG_MAX;
	}
	else 
	{
		value_max = (1ULL << (typmod)) - 1;
	}

	if (arg > value_max)
	{
		if (is_strict_mode())
		{
			ereport(ERROR,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
						errmsg("value too long for type bit(%d)",
							typmod)));
		}

		ereport(WARNING,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("value too long for type bit(%d)",
						typmod)));
		arg = value_max;
	}

	rlen = VARBITTOTALLEN(typmod);
	result = (VarBit *) palloc(rlen);
	SET_VARSIZE(result, rlen);
	VARBITLEN(result) = typmod;

	r = VARBITS(result);
	destbitsleft = typmod;
	srcbitsleft = 64;	// 64 is sizeof(uint64) * 8
	/* drop any input bits that don't fit */
	srcbitsleft = Min(srcbitsleft, destbitsleft);
	/* sign-fill any excess bytes in output */
	while (destbitsleft >= srcbitsleft + 8)
	{
		*r++ = 0;
		destbitsleft -= 8;
	}
	/* store first fractional byte */
	if (destbitsleft > srcbitsleft)
	{
		unsigned int val = (unsigned int) (arg >> (destbitsleft - 8));

		/* Force sign-fill in case the compiler implements >> as zero-fill */
		*r++ = (bits8) (val & BITMASK);
		destbitsleft -= 8;
	}
	/* Now srcbitsleft and destbitsleft are the same, need not track both */
	/* store whole bytes */
	while (destbitsleft >= 8)
	{
		*r++ = (bits8) ((arg >> (destbitsleft - 8)) & BITMASK);
		destbitsleft -= 8;
	}
	/* store last fractional byte */
	if (destbitsleft > 0)
		*r = (bits8) ((arg << (8 - destbitsleft)) & BITMASK);

	PG_RETURN_VARBIT_P(result);
}

Datum
bitfromvarbinary(PG_FUNCTION_ARGS)
{
	int32	atttypmod = PG_GETARG_INT32(1);
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	char	*sp = VARDATA_ANY(arg);	
	int 	slen = VARSIZE_ANY_EXHDR(arg);
	bool	is_insert = CALL_IN_INSERT();
	
	PG_RETURN_VARBIT_P(string_to_varbit(sp, atttypmod, slen, is_insert));
}

Datum
mysql_bit_in2(PG_FUNCTION_ARGS)
{
	char		*arg = PG_GETARG_CSTRING(0);
	char		*input_string = arg + 2;
	VarBit		*result;
	int32		atttypmod = PG_GETARG_INT32(2);
	int 		len = strlen(input_string);
	int 		bc = (len + 1) / 2  + VARHDRSZ;
	bytea		*tmp = palloc0(bc);
	char		*dst = VARDATA(tmp);

	if (len % 2)
	{
		*dst++ = mys_get_hex(input_string++);
		len--;
	}

	mys_hex_decode(input_string, len, dst);

	SET_VARSIZE(tmp, bc);

	result = DatumGetVarBitP(DirectFunctionCall2(bitfromvarbinary, PointerGetDatum(tmp), atttypmod));

	PG_RETURN_VARBIT_P(result);
}

/* operator mysbit = mysbit */
Datum 
mysbit_eq(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 == num1);
}

/* operator mysbit <> mysbit */
Datum 
mysbit_ne(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 != num1);
}

/* operator mysbit < mysbit */
Datum 
mysbit_lt(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 < num1);
}

/* operator mysbit <= mysbit */
Datum 
mysbit_le(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 <= num1);
}

/* operator mysbit >= mysbit */
Datum 
mysbit_ge(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 >= num1);
}

/* operator mysbit > mysbit */
Datum 
mysbit_gt(PG_FUNCTION_ARGS)
{
	VarBit		*arg0 = PG_GETARG_VARBIT_P(0);
	VarBit		*arg1 = PG_GETARG_VARBIT_P(1);
	uint64		num0, num1;

	num0 = bit_to_uint64(arg0);
	num1 = bit_to_uint64(arg1);

	PG_RETURN_BOOL(num0 > num1);
}

/* operator mysbit <=> int2 
 * bit_cmp
 *
 * Compares two bitstrings and returns <0, 0, >0 depending on whether the first
 * string is smaller, equal, or bigger than the second. All bits are considered
 * and additional zero bits may make one string smaller/larger than the other,
 * even if their zero-padded values would be the same.
 */
static int32
bit_cmp(VarBit *arg1, VarBit *arg2)
{
	int			bitlen1,
				bytelen1,
				bitlen2,
				bytelen2;
	int32		cmp;

	bytelen1 = VARBITBYTES(arg1);
	bytelen2 = VARBITBYTES(arg2);

	cmp = memcmp(VARBITS(arg1), VARBITS(arg2), Min(bytelen1, bytelen2));
	if (cmp == 0)
	{
		bitlen1 = VARBITLEN(arg1);
		bitlen2 = VARBITLEN(arg2);
		if (bitlen1 != bitlen2)
			cmp = (bitlen1 < bitlen2) ? -1 : 1;
	}
	return cmp;
}

Datum 
mysbit_safeeq(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);

	bool is_null_a = PG_ARGISNULL(0);
	bool is_null_b = PG_ARGISNULL(1);

	if (is_null_a && is_null_b)
		PG_RETURN_INT32(1);
	if (is_null_a || is_null_b)
		PG_RETURN_INT32(0);

	PG_RETURN_INT32((bit_cmp(arg0, arg1) == 0) ? 1 : 0);
}

/* operator bit & bit*/
Datum 
mysbit_bit_and(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);
	
	int64 tmp0 = 0;
	int64 tmp1 = 0;

	tmp0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));
	tmp1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg1)));

	PG_RETURN_UINT64(tmp0 & tmp1);
}

/* operator bit << bit */
Datum 
mysbit_left_shift_mysbit(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);

	uint64 tmp = 0;
	uint32 shift = 0;
	uint64 res = 0;

	tmp = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));
	shift = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg1)));

	res = tmp << shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

Datum 
mysbit_left_shift_int4(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	int32		arg1 = PG_GETARG_INT32(1);

	uint64 tmp = 0;
	uint32 shift = 0;
	uint64 res = 0;

	tmp = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));

	res = tmp << arg1;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

/* operator bit << bit */
Datum 
mysbit_right_shift_mysbit(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);

	uint64 tmp = 0;
	uint32 shift = 0;
	uint64 res = 0;

	tmp = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));
	shift = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg1)));

	res = tmp >> shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

Datum 
mysbit_right_shift_int4(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	int32		arg1 = PG_GETARG_INT32(1);

	uint64 tmp = 0;
	uint32 shift = 0;
	uint64 res = 0;

	tmp = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));

	res = tmp >> arg1;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

/* operator bit ^ bit */
Datum 
mysbit_bit_xor(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);

	int64 tmp0 = 0;
	int64 tmp1 = 0;

	tmp0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));
	tmp1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg1)));

	PG_RETURN_UINT64(tmp0 ^ tmp1);
}

/* operator bit | int4 */
Datum 
mysbit_bit_or(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *arg1 = PG_GETARG_VARBIT_P(1);

	int64 tmp0 = 0;
	int64 tmp1 = 0;

	tmp0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));
	tmp1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg1)));

	PG_RETURN_UINT64(tmp0 | tmp1);
}

/* operator ~bit */
Datum 
mysbit_bit_neg(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);

	int64 tmp0 = 0;

	tmp0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(arg0)));

	PG_RETURN_UINT64(~tmp0);
}

/* operator +mysbit */
Datum 
mysbit_positive(PG_FUNCTION_ARGS)
{
	VarBit	   *arg0 = PG_GETARG_VARBIT_P(0);

	PG_RETURN_VARBIT_P(arg0);
}

/* operator mysbit div mysbit */
Datum 
mysbit_exact_div(PG_FUNCTION_ARGS)
{
	VarBit	   *bitarg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *bitarg1 = PG_GETARG_VARBIT_P(0);
	int64 arg0 = 0;
	int64 arg1 = 0;
	bool arg0_negative, arg1_negative, res_negative;
	uint64 uarg0 = 0;
	uint64 uarg1 = 0;
	uint64 res = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg0)));
	arg1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg1)));

	if(arg1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	arg0_negative = (arg0 < 0);
	arg1_negative = (arg1 < 0);
	res_negative = (arg0_negative != arg1_negative);
	uarg0 = (uint64)(arg0_negative ? -arg0 : arg0);
	uarg1 = (uint64)(arg1_negative ? -arg1 : arg1);
	res = uarg0 / uarg1;
	if (res_negative)
	{
		res = (uint64)(-(int64)res);
	}
	PG_RETURN_INT64(res);
}

/* operator mysbit || mysbit */
Datum 
mysbit_or(PG_FUNCTION_ARGS)
{
	VarBit	   *bitarg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *bitarg1 = PG_GETARG_VARBIT_P(0);
	int64 arg0 = 0;
	int64 arg1 = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	arg0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg0)));
	arg1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg1)));

	if ((arg0 != 0) || (arg1 != 0))
	{
		PG_RETURN_INT32(1);
	}
	PG_RETURN_INT32(0);
}

/* operator mysbit && mysbit */
Datum 
mysbit_and(PG_FUNCTION_ARGS)
{
	VarBit	   *bitarg0 = PG_GETARG_VARBIT_P(0);
	VarBit	   *bitarg1 = PG_GETARG_VARBIT_P(0);
	int64 arg0 = 0;
	int64 arg1 = 0;

	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}
	
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_INT32(0);
	}

	arg0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg0)));
	arg1 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg1)));

	if ((arg0 != 0) && (arg1 != 0))
	{
		PG_RETURN_INT32(1);
	}
	PG_RETURN_INT32(0);
}

/* operator !int2 */
Datum 
mysbit_not(PG_FUNCTION_ARGS)
{
	VarBit	*bitarg0 = PG_GETARG_VARBIT_P(0);
	int64	arg0 = 0;

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	arg0 = DatumGetInt64(DirectFunctionCall1(bittoint8, VarBitPGetDatum(bitarg0)));

	PG_RETURN_INT32(arg0 == 0 ? 1 : 0);
}

/* operator ~~ */
Datum
mysbit_like(PG_FUNCTION_ARGS)
{
	bytea 	*tmp0 = DatumGetByteaPP(DirectFunctionCall1(bittobinary, PointerGetDatum(PG_GETARG_VARBIT_P(0))));
	bytea 	*tmp1 = DatumGetByteaPP(DirectFunctionCall1(bittobinary, PointerGetDatum(PG_GETARG_VARBIT_P(1))));
	bool	result;
	
	char 	*data0 = VARDATA_ANY(tmp0);
	char 	*data1 = VARDATA_ANY(tmp1);
	int 	len0 = VARSIZE_ANY_EXHDR(tmp0);
	int 	len1 = VARSIZE_ANY_EXHDR(tmp0);

	text	*ret0 = cstring_to_text_with_len(data0, len0);
	text	*ret1 = cstring_to_text_with_len(data1, len1);

	result = DatumGetBool(DirectFunctionCall2(textlike, PointerGetDatum(ret0), PointerGetDatum(ret1)));

	PG_RETURN_BOOL(result);
}

/* operator !~~ */
Datum
mysbit_notlike(PG_FUNCTION_ARGS)
{
	bytea 	*tmp0 = DatumGetByteaPP(DirectFunctionCall1(bittobinary, PointerGetDatum(PG_GETARG_VARBIT_P(0))));
	bytea 	*tmp1 = DatumGetByteaPP(DirectFunctionCall1(bittobinary, PointerGetDatum(PG_GETARG_VARBIT_P(1))));
	bool	result;
	
	char 	*data0 = VARDATA_ANY(tmp0);
	char 	*data1 = VARDATA_ANY(tmp1);
	int 	len0 = VARSIZE_ANY_EXHDR(tmp0);
	int 	len1 = VARSIZE_ANY_EXHDR(tmp0);

	text	*ret0 = cstring_to_text_with_len(data0, len0);
	text	*ret1 = cstring_to_text_with_len(data1, len1);

	result = DatumGetBool(DirectFunctionCall2(textlike, PointerGetDatum(ret0), PointerGetDatum(ret1)));

	PG_RETURN_BOOL(!result);
}

Datum
mysbit_hash(PG_FUNCTION_ARGS)
{
	VarBit	   *key = PG_GETARG_VARBIT_P(0);

	return hash_any((unsigned char *) VARBITS(key), VARBITLEN(key));
}

Datum
mysbit_hash_extended(PG_FUNCTION_ARGS)
{
	VarBit	   *key = PG_GETARG_VARBIT_P(0);

	return hash_any_extended((unsigned char *) VARBITS(key), VARBITLEN(key),
							 PG_GETARG_INT64(1));
}

/* Begin - bug#z5805 */
Datum
bit_uint8(PG_FUNCTION_ARGS)
{
	VarBit	   *arg = PG_GETARG_VARBIT_P(0);
	uint64		result;
	bits8	   *r;
	bool		is_insert = CALL_IN_INSERT();

	/* Check that the bit string is not too long */
	if (VARBITLEN(arg) > sizeof(result) * BITS_PER_BYTE)
	{
		if (is_insert)
		{
			ereport(is_strict_mode() ? ERROR : WARNING,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 	 errmsg("Out of range value for column")));
			
			PG_RETURN_UINT64(UINT64_MAX);
		}
		else
		{
			ereport(WARNING,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 	 errmsg("Truncated incorrect BINARY value")));
			
			PG_RETURN_UINT64(UINT64_MAX);
		}
	}

	result = 0;
	for (r = VARBITS(arg); r < VARBITEND(arg); r++)
	{
		result <<= BITS_PER_BYTE;
		result |= *r;
	}

	/* Now shift the result to take account of the padding at the end */
	result >>= VARBITPAD(arg);

	PG_RETURN_UINT64(result);
}
/* End - bug#z5805 */

/* Begin - BUG#8554 */
Datum
mysbit_length(PG_FUNCTION_ARGS)
{
	VarBit	   *input = PG_GETARG_VARBIT_P(0);
	int			bit_len = VARBITLEN(input);

	PG_RETURN_INT32((bit_len + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
}
/* End - BUG#8554 */