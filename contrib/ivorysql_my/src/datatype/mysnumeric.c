#include "postgres.h"

#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "../include/mys_locale.h"

#define MAX_PRECISION 30
#define ASCII_MINUS 45
#define THOUS_INTERVAL 3

PG_FUNCTION_INFO_V1(numeric_eq_int4);
PG_FUNCTION_INFO_V1(int4_eq_numeric);
PG_FUNCTION_INFO_V1(numeric_eq_int8);
PG_FUNCTION_INFO_V1(numeric_add_int4);
PG_FUNCTION_INFO_V1(int4_add_numeric);
PG_FUNCTION_INFO_V1(numeric_add_int8);
PG_FUNCTION_INFO_V1(int8_add_numeric);
/* Begin - bug#z6039 */
PG_FUNCTION_INFO_V1(uint8_mul_numeric);
PG_FUNCTION_INFO_V1(numeric_mul_uint8);
/* End - bug#z6039 */
PG_FUNCTION_INFO_V1(numeric_exact_divnumeric);
PG_FUNCTION_INFO_V1(numeric_format);

Datum
numeric_eq_int4(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int32 val2 = PG_GETARG_INT32(1);

    return DirectFunctionCall2(numeric_eq, val1, NumericGetDatum(int64_to_numeric((int64) val2)));
}

Datum
int4_eq_numeric(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(1);
    int32 val2 = PG_GETARG_INT32(0);

    return DirectFunctionCall2(numeric_eq, val1, NumericGetDatum(int64_to_numeric((int64) val2)));
}

Datum
numeric_eq_int8(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int64 val2 = PG_GETARG_INT64(1);

    return DirectFunctionCall2(numeric_eq, val1, NumericGetDatum(int64_to_numeric(val2)));
}

Datum
numeric_add_int4(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int32 val2 = PG_GETARG_INT32(1);

    return DirectFunctionCall2(numeric_add, val1, NumericGetDatum(int64_to_numeric((int64) val2)));
}

Datum
numeric_add_int8(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int64 val2 = PG_GETARG_INT64(1);

    return DirectFunctionCall2(numeric_add, val1, NumericGetDatum(int64_to_numeric(val2)));
}

Datum
int4_add_numeric(PG_FUNCTION_ARGS)
{
    int64 val1 = PG_GETARG_INT32(0);
    Datum val2 = PG_GETARG_DATUM(1);

    return DirectFunctionCall2(numeric_add, NumericGetDatum(int64_to_numeric(val1)), val2);
}

Datum
int8_add_numeric(PG_FUNCTION_ARGS)
{
    int64 val1 = PG_GETARG_INT64(0);
    Datum val2 = PG_GETARG_DATUM(1);

    return DirectFunctionCall2(numeric_add, NumericGetDatum(int64_to_numeric(val1)), val2);
}

Datum
numeric_exact_divnumeric(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;
	bool		divzero;

	divzero = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal2),
						NumericGetDatum(int64_to_numeric(0))));	
	if(divzero)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}

	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	res = DatumGetNumeric(DirectFunctionCall2(numeric_sub, NumericGetDatum(res),
					DirectFunctionCall1(float8_numeric, Float8GetDatum(0.5))));	
	PG_RETURN_INT64(DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(res))));
}

/* Begin - bug#z6039 */
Datum
uint8_mul_numeric(PG_FUNCTION_ARGS)
{
    Datum arg1 = PG_GETARG_DATUM(0);    // uint64
    Datum arg2 = PG_GETARG_DATUM(1);    // numeric

    Datum tmp = NumericGetDatum(uint64_to_numeric(DatumGetUInt64(arg1)));
    Datum ret = DirectFunctionCall2(numeric_mul, tmp, arg2);

    PG_RETURN_DATUM(ret);
}

Datum
numeric_mul_uint8(PG_FUNCTION_ARGS)
{
    Datum arg1 = PG_GETARG_DATUM(0);    // numeric
    Datum arg2 = PG_GETARG_DATUM(1);    // uint64

    Datum tmp = NumericGetDatum(uint64_to_numeric(DatumGetUInt64(arg2)));
    Datum ret = DirectFunctionCall2(numeric_mul, tmp, arg1);

    PG_RETURN_DATUM(ret);
}
/* End - bug#z6039 */

Datum
numeric_format(PG_FUNCTION_ARGS)
{
    Numeric value;
    int64 precision;
	char *char_locale, *char_value, *rounded_val;
	int has_minus, int_part_len, thousand_sep_num, buf_len;
	int len, dec_point_index, acc;
	char *buf, *buf_ptr, *val_ptr, *point;
	MyLocale *locale = NULL;

	/* Begin Bug#8445 8447 */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

    value = PG_GETARG_NUMERIC(0);
    precision = PG_GETARG_INT64(1);

	/* Begin - ReqID: SRS-MYSQL-BUILDFUNCTION */
	if (numeric_is_inf(value))
		PG_RETURN_TEXT_P(cstring_to_text("0"));
	/* End - ReqID: SRS-MYSQL-BUILDFUNCTION */
	
	precision = Min(MAX_PRECISION, Max(0, precision));

	// if invalid locale received, set locale to 'en_US'
	if (PG_ARGISNULL(2))
		char_locale = "en_US";
	else
		char_locale = text_to_cstring(PG_GETARG_TEXT_PP(2));
	/* End Bug#8445 8447 */

	locale = MyLocaleSearch(char_locale);
	if (locale == NULL)
		locale = MyLocaleSearch("en_US");

	value = DatumGetNumeric(DirectFunctionCall2(numeric_round,
												NumericGetDatum(value),
												Int32GetDatum(precision)));
	char_value = DatumGetCString(DirectFunctionCall1(numeric_out,
													 NumericGetDatum(value)));

	has_minus = (char_value[0] == ASCII_MINUS) ? 1 : 0;

	len = strlen(char_value);
	rounded_val = (char *) palloc(len + 1);
	strcpy(rounded_val, char_value);

	// find decimal point index
	dec_point_index = -1;
	point = strchr(char_value, '.');
	if (point != NULL)
	{
		dec_point_index = point - char_value;
	}

	int_part_len = (dec_point_index == -1 ? len : dec_point_index) - has_minus;
	thousand_sep_num = locale->thousand_sep == '\0' ? 0
						: (int_part_len % THOUS_INTERVAL == 0 ? int_part_len / THOUS_INTERVAL - 1 : int_part_len / THOUS_INTERVAL);

	if (precision <= 0)
	{
		buf_len = int_part_len + thousand_sep_num + 1 + has_minus;
	}
	else
	{
		buf_len = int_part_len + thousand_sep_num + precision + 2 + has_minus;
	}

	buf = (char *)palloc(buf_len);
	buf[buf_len - 1] = '\0';

	// pointer start from position of decimal point
	buf_ptr = buf + int_part_len + thousand_sep_num + has_minus;

	// add decimal point when requiring precision
	if (precision > 0)
	{
		*buf_ptr = locale->decimal_point;
	}

	// handle integer part
	val_ptr = rounded_val + int_part_len + has_minus;
	acc = 0;
	do
	{
		buf_ptr--;
		val_ptr--;
		*buf_ptr = *val_ptr;
		acc++;
		if (acc % THOUS_INTERVAL == 0 && acc != int_part_len && locale->thousand_sep != '\0')
		{
			*--buf_ptr = locale->thousand_sep;
		}
	} while (buf_ptr - 1 > buf);
	*--buf_ptr = *--val_ptr;

	if (precision <= 0)
	{
		PG_RETURN_TEXT_P(cstring_to_text(buf));
	}

	// handle decimal part
	buf_ptr = buf + int_part_len + thousand_sep_num + has_minus;
	val_ptr = rounded_val + int_part_len + has_minus;
	for (int i = 0; i < precision; i++)
	{
		if (*val_ptr == '\0' || *(val_ptr + 1) == '\0')
		{
			*(++buf_ptr) = '0';
		}
		else
		{
			*(++buf_ptr) = *(++val_ptr);
		}
	}
	pfree(rounded_val);

	PG_RETURN_TEXT_P(cstring_to_text(buf));
}