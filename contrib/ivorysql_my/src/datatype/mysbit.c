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
#include "postgres.h"
#include "fmgr.h"
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

static Datum
string_to_varbit(char *sp, int atttypmod, int slen)
{
	int		len = 0, /* Length of the whole data structure */
			delta = 0,
			pre = 0,
			i = 0;
	bits8	x = 0,
			y = 0;
	VarBit	*result;	/* The resulting bit string */
	bits8	*r;		/* pointer into the result */

	/*
	* attypmod is target bit length, slen is source string byte length.
	*
	* First, pad the insufficient bytes with zeros (delta bytes).
	* 
	* Then, for each byte in the source string, shift and combine them according to bits,
	* retain the lower pre bits of the previous byte, and combine them with the higher 8-pre
	* bits of the current byte fom a new byte.
	* 
	* Finally, clear the remaining pre bits of the last byte and append it to the end.
	*/
	
	len = VARBITTOTALLEN(atttypmod);
	/* set to 0 so that *r is always initialised and string is zero-padded */
	result = (VarBit *) palloc0(len); 
	SET_VARSIZE(result, len);
	VARBITLEN(result) = atttypmod;

	r = VARBITS(result);

	delta = atttypmod / BITS_PER_BYTE - slen;
	pre = atttypmod % BITS_PER_BYTE;
	
	for (i = 0; i < delta; i++)
	{
		*r++ = 0;
	}
	
	x = (bits8) *sp;
	*r = (x >> pre);
	r++;
	sp++;

	for (i = 1; i < slen; sp++, i++)
	{
		x = (bits8) *sp;
		y = (bits8) *(sp - 1);
		*r = (y << (BITS_PER_BYTE - pre)) | (x >> pre);
		r++;
	}

	if (pre != 0)
	{
		y = (bits8) *(sp - 1);
		*r = (y << (BITS_PER_BYTE - pre)) | 0;
	}

	PG_RETURN_VARBIT_P(result);
}

Datum
mysql_bit_in(PG_FUNCTION_ARGS)
{
	char	   *input_string = PG_GETARG_CSTRING(0);

#ifdef NOT_USED
	Oid			typelem = PG_GETARG_OID(1);
#endif
	
	int32		atttypmod = PG_GETARG_INT32(2);
	
	char	   *sp;				/* pointer into the character string  */
	int			bitlen,			/* Number of bits in the bit string   */
				slen;			/* Length of the input string		  */		

	sp = input_string;
	slen = strlen(sp);
	bitlen = slen * 8;	//8 bits for ascii code of each character	
	
	/*
	 * Sometimes atttypmod is not supplied. If it is supplied we need to make
	 * sure that the bitstring fits.
	 */
	if (atttypmod <= 0)
		atttypmod = bitlen;
	else if (bitlen > atttypmod)
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
				 errmsg("bit string length %d does not match type bit(%d)",
						bitlen, atttypmod)));

	PG_RETURN_VARBIT_P(string_to_varbit(sp, atttypmod, slen));
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
		ereport(ERROR,
		(errcode(ERRCODE_STRING_DATA_LENGTH_MISMATCH),
			errmsg("bit string length %d does not match type bit(%d)",
				VARBITLEN(arg), typmod)));

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

	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendbytes(&buf, (char *) VARBITS(s), VARBITBYTES(s));
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
bittouint8(PG_FUNCTION_ARGS)
{
	VarBit	   *arg = PG_GETARG_VARBIT_P(0);
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

	PG_RETURN_UINT64(result);
}

Datum
bitfromuint8(PG_FUNCTION_ARGS)
{
	uint64		arg = PG_GETARG_UINT64(0);
	int32		typmod = PG_GETARG_INT32(1);
	VarBit	   *result;
	bits8	   *r;
	int			rlen;
	int			destbitsleft,
				srcbitsleft;

	if (typmod <= 0 || typmod > VARBITMAXLEN)
		typmod = 1;				/* default bit length */

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
	int32		atttypmod = PG_NARGS() == 2 ? PG_GETARG_INT32(1) : PG_GETARG_INT32(2);
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	char	*sp = VARDATA_ANY(arg);	
	int 	slen = VARSIZE_ANY_EXHDR(arg);
	
	PG_RETURN_VARBIT_P(string_to_varbit(sp, atttypmod, slen));
}

Datum
mysql_bit_in2(PG_FUNCTION_ARGS)
{
	char	   *arg = PG_GETARG_CSTRING(0);
	char		*input_string = arg + 2;
	VarBit		*result;
	int32		atttypmod = PG_NARGS() == 2 ? PG_GETARG_INT32(1) : PG_GETARG_INT32(2);
	

	int 	len = strlen(input_string);
	int 	bc = (len + 1) / 2  + VARHDRSZ;

	bytea	*tmp = palloc0(bc);
	char	*dst = VARDATA(tmp);

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