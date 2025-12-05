/*------------------------------------------------------------------
 * mystext.c
 *
 * Abstract:
 * 		Compatible with MySQL's TEXT data type.
 *
 * Authored by wangjie@highgo.com, 20240329.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/character/mystext.c
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/float.h"
#include "fmgr.h"
#include "../include/ivorysql_my.h"

#include <float.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

PG_FUNCTION_INFO_V1(mystextfloat8_pl);
PG_FUNCTION_INFO_V1(mystext_pl);
PG_FUNCTION_INFO_V1(mystext_mi);
PG_FUNCTION_INFO_V1(mystext_mul);
PG_FUNCTION_INFO_V1(mystext_div);
PG_FUNCTION_INFO_V1(mystext_mod);
PG_FUNCTION_INFO_V1(mystext_modiv);
PG_FUNCTION_INFO_V1(mystext_xor);
PG_FUNCTION_INFO_V1(mystext_logic_not);
PG_FUNCTION_INFO_V1(mystext_logic_and);
PG_FUNCTION_INFO_V1(text_bool);
PG_FUNCTION_INFO_V1(varlena_int2);
PG_FUNCTION_INFO_V1(varlena_int4);
PG_FUNCTION_INFO_V1(varlena_int8);
PG_FUNCTION_INFO_V1(varlena_uint1);
PG_FUNCTION_INFO_V1(varlena_uint2);
PG_FUNCTION_INFO_V1(varlena_uint4);
PG_FUNCTION_INFO_V1(varlena_uint8);
PG_FUNCTION_INFO_V1(mys_isnull);
PG_FUNCTION_INFO_V1(varlena_float4);
PG_FUNCTION_INFO_V1(varlena_float8);
PG_FUNCTION_INFO_V1(varlena_numeric);

/*
 * extract_leading_number_str:
 *	Parse the leading numeric portion from the input string (allowing one decimal point and scientific notation).
 *	- Skips leading whitespace.
 *	- Accepts an optional sign: negative sign is kept; positive sign is discarded.
 *	- If the number starts with a decimal point, treat it as if it begins with "0.".
 *	- Parsing stops when a character that cannot be part of the number is encountered.
 *	- Supports scientific notation (e.g., "1.23e-4").
 *	- If the parsed value is equivalent to zero, returns "0" (or "-0" if the sign was negative).
 *
 * Parameters:
 *	str     - Input string.
 *	out_buf - Output buffer where the parsed number (null-terminated) will be stored.
 *	size    - Size of the output buffer.
 *
 * Returns:
 *	The length of the extracted result.
 */
static size_t
extract_leading_number_str(const char *str, char *out_buf, size_t size)
{
	/* Temporary buffer for the numeric part (without sign) */
	char temp[128] = {0};
	int temp_max_len = 0;
	/* Enlarged buffer to avoid snprintf truncation warnings */
	char final_buf[256] = {0};
	size_t temp_idx = 0;
	const char *p = str;
	int negative = 0;	/* Flag for negative sign */
	/* Check if all digits (ignoring '.', 'e', 'E', and exponent sign) are zero */
	bool allZero = true;

	memset(out_buf, 0, size);

	/* Skip leading whitespace */
	while (isspace(*p))
		p++;

	/* If the first character is not a digit, '+', '-', or '.', return "0" */
	if (!isdigit(*p) && *p != '+' && *p != '-' && *p != '.')
	{
		strncpy(out_buf, "0", 2);
		return 1;
	}

	/* Check for sign */
	if (*p == '+' || *p == '-')
	{
		if (*p == '-')
			negative = 1;
		p++;	/* Skip the sign */
	}

	/* Parse integer and fractional part */
	temp_max_len = sizeof(temp) - 1;
	if (*p == '.')
	{
		/* No integer part: prepend "0." */
		temp[temp_idx++] = '0';
		temp[temp_idx++] = '.';
		p++;	/* Skip the decimal point */
		/* Parse fractional digits */
		while (isdigit(*p) && temp_idx < temp_max_len)
			temp[temp_idx++] = *p++;
	}
	else if (isdigit(*p))
	{
		/* Parse integer part */
		while (isdigit(*p) && temp_idx < temp_max_len)
			temp[temp_idx++] = *p++;
		/* If there is a decimal point, parse it and the following fraction */
		if (*p == '.')
		{
			if (temp_idx < temp_max_len)
				temp[temp_idx++] = '.';
			p++;	/* Skip decimal point */
			while (isdigit(*p) && temp_idx < temp_max_len)
				temp[temp_idx++] = *p++;
		}
	}
	else
	{
		strncpy(out_buf, "0", 2);
		return 1;
	}

	/* Check for scientific notation (exponent part) */
	if ((*p == 'e' || *p == 'E') && temp_idx < temp_max_len)
	{
		temp[temp_idx++] = *p++;	/* Append 'e' or 'E' */
		/* Optional sign for exponent */
		if ((*p == '+' || *p == '-') && temp_idx < temp_max_len)
			temp[temp_idx++] = *p++;
		/* Parse exponent digits */
		while (isdigit(*p) && temp_idx < temp_max_len)
			temp[temp_idx++] = *p++;
	}
	temp[temp_idx] = '\0';

	for (size_t i = 0; i < temp_idx; i++)
	{
		if (temp[i] != '0' && temp[i] != '.' && temp[i] != 'e' && temp[i] != 'E' &&
		    temp[i] != '+' && temp[i] != '-')
		{
			allZero = false;
			break;
		}
	}

	if (allZero)
	{
		if (negative)
			snprintf(final_buf, sizeof(final_buf), "-0");
		else
			snprintf(final_buf, sizeof(final_buf), "0");
	}
	else
	{
		if (negative)
			snprintf(final_buf, sizeof(final_buf), "-%s", temp);
		else
			snprintf(final_buf, sizeof(final_buf), "%s", temp);
	}

	/* Remove a leading '+' if present */
	if (final_buf[0] == '+')
	{
		memmove(final_buf, final_buf + 1, strlen(final_buf) + 1);	/* +1 for null terminator */
	}

	/* Copy final result to output buffer */
	strncpy(out_buf, final_buf, size);
	out_buf[size - 1] = '\0';
	return strlen(out_buf);
}


/*
 * extract_leading_integer_str:
 *   Parse the leading integer portion from the input string (ignoring any fractional part).
 *   - Skips leading whitespace.
 *   - Accepts an optional sign: keeps the negative sign; discards the positive sign.
 *   - If the number starts with a decimal point, treat the integer part as "0".
 *   - Parsing stops when a non-digit is encountered.
 *   - If the parsed value is equivalent to zero, returns "0" (or "-0" if the sign was negative).
 *
 * Parameters:
 *   str     - Input string.
 *   out_buf - Output buffer to store the parsed result (null-terminated).
 *   size    - Size of the output buffer.
 *
 * Returns:
 *   The length of the extracted result.
 */
bool
extract_leading_integer_str(const char *str, char *out_buf, size_t size)
{
	char temp[64] = {0};   /* Temporary buffer to hold the numeric part (without the sign) */
	int temp_max_len = 0;
	size_t temp_idx = 0;
	const char *p = str;
	int negative = 0;      /* Flag for negative sign */
	bool allZero = true;
	char final_buf[128] = {0};  /* Larger buffer to avoid snprintf truncation warnings */
	const char *raw;

	memset(out_buf, 0, size);

	/* Skip leading whitespace */
	while (isspace(*p)) p++;

	raw = p;

	/* If the first character is not a digit, '+', '-', or '.', return "0" */
	if (!isdigit(*p) && *p != '+' && *p != '-' && *p != '.')
	{
		strncpy(out_buf, "0", 2);
		return false;
	}

	/* Check for the sign */
	if (*p == '+' || *p == '-')
	{
		if (*p == '-')
			negative = 1;
		p++;  /* Skip the sign */
	}

	/* If the next character is not a digit, treat the integer part as "0" */
	if (!isdigit(*p))
	{
		strncpy(temp, "0", 2);
		temp_idx = strlen(temp);
	}
	else
	{
		/* Parse the integer part, only processing consecutive digits */
		temp_max_len = sizeof(temp) - 1;
		while (isdigit(*p) && temp_idx < temp_max_len)
		{
			temp[temp_idx++] = *p++;
		}
	}
	temp[temp_idx] = '\0';

	/* Check if all digits (ignoring the sign) are zero */
	for (size_t i = 0; i < temp_idx; i++)
	{
		if (temp[i] != '0')
		{
			allZero = false;
			break;
		}
	}

	if (allZero)
	{
		if (negative)
			snprintf(final_buf, sizeof(final_buf), "-0");
		else
			snprintf(final_buf, sizeof(final_buf), "0");
	}
	else
	{
		if (negative)
			snprintf(final_buf, sizeof(final_buf), "-%s", temp);
		else
			snprintf(final_buf, sizeof(final_buf), "%s", temp);
	}

	/* Remove a leading '+' if present (theoretically, this shouldn't occur) */
	if (final_buf[0] == '+')
	{
		memmove(final_buf, final_buf + 1, strlen(final_buf) + 1);  /* +1 to include the null terminator */
	}

	/* Copy the final result into the output buffer */
	strncpy(out_buf, final_buf, size);
	out_buf[size - 1] = '\0';

	if (strcmp(raw, out_buf))
		return false;

	return true;
}

Datum
mystextfloat8_pl(PG_FUNCTION_ARGS)
{
	char	   num1[64];
	float8	   f_arg1 = 0;
	text	  *arg1 = PG_GETARG_TEXT_PP(0);
	float8	   f_arg2 = PG_GETARG_FLOAT8(1);

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl,
							   Float8GetDatum(f_arg1),
							   Float8GetDatum(f_arg2)));
}

Datum
mystext_pl(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl,
							   Float8GetDatum(f_arg1),
							   Float8GetDatum(f_arg2)));
}

Datum
mystext_mi(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi,
							   Float8GetDatum(f_arg1),
							   Float8GetDatum(f_arg2)));
}

Datum
mystext_mul(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul,
							   Float8GetDatum(f_arg1),
							   Float8GetDatum(f_arg2)));

}

Datum
mystext_div(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	if (f_arg2 == 0 || f_arg2 == -0)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(DirectFunctionCall2(float8div,
							   Float8GetDatum(f_arg1),
							   Float8GetDatum(f_arg2)));
}

Datum
mystext_mod(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	if (f_arg2 == 0 || f_arg2 == -0)
		PG_RETURN_NULL();

	PG_RETURN_FLOAT8(fmod(f_arg1, f_arg2));
}

Datum
mystext_modiv(PG_FUNCTION_ARGS)
{
	text	   *arg1 = PG_GETARG_TEXT_PP(0);
	text	   *arg2 = PG_GETARG_TEXT_PP(1);
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	if (f_arg2 == 0 || f_arg2 == -0)
		PG_RETURN_NULL();

	PG_RETURN_INT64(f_arg1 / f_arg2);
}

Datum
mystext_xor(PG_FUNCTION_ARGS)
{
	text	   *arg1;
	text	   *arg2;
	char	   num1[64];
	char	   num2[64];
	int64	   i_arg1 = 0;
	int64	   i_arg2 = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_TEXT_PP(0);
	arg2 = PG_GETARG_TEXT_PP(1);

	extract_leading_integer_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_integer_str(text_to_cstring(arg2), num2, sizeof(num2));

	i_arg1 = DatumGetInt64(DirectFunctionCall1(int8in,
												CStringGetDatum(num1)));
	i_arg2 = DatumGetInt64(DirectFunctionCall1(int8in,
												CStringGetDatum(num2)));

	PG_RETURN_INT64(i_arg1 ^ i_arg2);
}

Datum
mystext_logic_not(PG_FUNCTION_ARGS)
{
	char	   num[64];
	float8	   f_arg = 0;
	text	   *arg;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_TEXT_PP(0);

	extract_leading_number_str(text_to_cstring(arg), num, sizeof(num));

	f_arg = float8in_internal(num, NULL, "double precision", num);

	if (f_arg == 0 || f_arg == -0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
mystext_logic_and(PG_FUNCTION_ARGS)
{
	text	   *arg1;
	text	   *arg2;
	char	   num1[64];
	char	   num2[64];
	float8	   f_arg1 = 0;
	float8	   f_arg2 = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_TEXT_PP(0);
	arg2 = PG_GETARG_TEXT_PP(1);

	extract_leading_number_str(text_to_cstring(arg1), num1, sizeof(num1));
	extract_leading_number_str(text_to_cstring(arg2), num2, sizeof(num2));

	f_arg1 = float8in_internal(num1, NULL, "double precision", num1);
	f_arg2 = float8in_internal(num2, NULL, "double precision", num2);

	if (f_arg1 != 0 && f_arg1 != -0 &&
		f_arg2 != 0 && f_arg2 != -0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
text_bool(PG_FUNCTION_ARGS)
{
	char	    num[64];
	const char *in_str;
	float8		f_arg = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	in_str = text_to_cstring(PG_GETARG_TEXT_PP(0));

	extract_leading_number_str(in_str, num, sizeof(num));

	f_arg = float8in_internal(num, NULL, "double precision", num);

	if (f_arg == 0 || f_arg == -0)
		PG_RETURN_BOOL(false);
	else
		PG_RETURN_BOOL(true);
}

Datum
mys_isnull(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

Datum
varlena_int2(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	int16		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_int16(src, is_insert);
	
	PG_RETURN_INT16(ret);
}

Datum
varlena_int4(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	int32		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_int32(src, is_insert);
	
	PG_RETURN_INT32(ret);
}

Datum
varlena_int8(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	int64		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_int64(src, is_insert);
	
	PG_RETURN_INT64(ret);
}

Datum
varlena_uint1(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	uint8		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_uint8(src, is_insert);
	
	PG_RETURN_UINT8(ret);
}

Datum
varlena_uint2(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	uint16		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_uint16(src, is_insert);
	
	PG_RETURN_UINT16(ret);
}

Datum
varlena_uint4(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	uint32		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_uint32(src, is_insert);
	
	PG_RETURN_UINT32(ret);
}

Datum
varlena_uint8(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	uint64		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atoi_uint64(src, is_insert);
	
	PG_RETURN_UINT64(ret);
}

/* Begin - bug#z5590 */
Datum
varlena_float4(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	float		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atof(src, is_insert);
	
	PG_RETURN_FLOAT4(ret);
}

Datum
varlena_float8(PG_FUNCTION_ARGS)
{
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	double		ret;
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_atod(src, is_insert);
	
	PG_RETURN_FLOAT8(ret);
}

Datum
varlena_numeric(PG_FUNCTION_ARGS)
{
	Numeric		ret;
	text		*arg = PG_GETARG_TEXT_PP(0);
	char		*src = (char *) text_to_cstring(arg);
	bool		is_insert = CALL_IN_INSERT();

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ret = my_aton(src, PG_GETARG_INT32(1), is_insert);
	
	PG_RETURN_NUMERIC(ret);
}
/* End - bug#z5590 */

PG_FUNCTION_INFO_V1(mystextsafeeq);
Datum
mystextsafeeq(PG_FUNCTION_ARGS)
{
	bool res;

	bool is_null_a = PG_ARGISNULL(0);
	bool is_null_b = PG_ARGISNULL(1);

	if (is_null_a && is_null_b)
		PG_RETURN_BOOL(true);

	if (is_null_a || is_null_b)
		PG_RETURN_BOOL(false);

	res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(),
											   PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	PG_RETURN_BOOL(res);
}