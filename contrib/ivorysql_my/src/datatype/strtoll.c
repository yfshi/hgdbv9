/*------------------------------------------------------------------
 * 
 * File: strtoll.c
 *
 * Abstract: 
 * 		MySQL's ctype simple funciton
 *
 * Authored by liyonghao@highgo.com, 20250409.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/strtoll.c
 *
 * ReqID:SRS-MYSQL-NUMBERICTYPE
 * 
 *------------------------------------------------------------------
 */

#include <float.h>

#include "postgres.h"

#include "fmgr.h"
#include "utils/elog.h"
#include "utils/float.h"
#include "utils/guc.h"
#include "utils/fmgrprotos.h"
#include "utils/ora_compatible.h"
#include "../include/ivorysql_my.h"

static char *
trim_space(const char *src)
{
	char *s = pstrdup(src);
	char *e = s + strlen(s) - 1;
	
	while (isspace(*s))
		s++;

	// src is empty or all character is space
	if (*s == '\0')
		return s;

	while (isspace(*e))
		*(e--) = 0;

	return s;
}

/**
 * Mysql will do round off in insert context if input number is decimal,
 * but not in other context.
 */
static int64
cstring_to_signed(const char *src, int64 min_val, int64 max_val, bool is_insert)
{
	int64		ret;
	char		*endptr = NULL;
	bool		negative = (src[0] == '-');
	
	src = trim_space(src);

	negative = (src[0] == '-');

	errno = 0;
	ret = strtoll(src, &endptr, 10);

	if (ret > max_val)
	{
		ret = max_val;
		errno = ERANGE;
	}

	if (ret < min_val)
	{
		ret = min_val;
		errno = ERANGE;
	}

	if (is_insert)
	{
		if (endptr == src && *endptr != '.')
		{
			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errmsg("Incorrect integer value: '%s' for column.", src)));
		}
		else if (errno == ERANGE)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errmsg("Out of range value for column.")));
			
			if (negative)
				ret = min_val;
			else
				ret = max_val;
		}
		else if (*endptr != '\0')
		{
			// do round off
			if (*endptr == '.')
			{
				uint8 next;

				if (*(endptr + 1) == '\0')
					next = 0;
				else
					next = *(endptr + 1) - '0';

				// Check there are illegal character
				strtoull(endptr + 1, &endptr, 10);
				
				/** 
				 * Next char is not digit or larger than 4, 
				 * or the integral part is MAX,
				 * or input number like '-0.9'.
				 */
				if ((next > 4 && next < 10) &&
					((errno == ERANGE) ||
					 (ret == min_val) ||
					 (ret == max_val)))
				{
					ereport(is_strict_mode() ? ERROR : WARNING, 
							(errmsg("Out of range value for column.")));
				}
				else if (next > 4 && next < 10)
				{
					if (ret > 0)
						ret++;

					if (ret < 0)
						ret--;

					if (ret == 0)
					{
						if (negative)
							ret--;
						else
							ret++;
					}
				}
			}

			if (*endptr != '\0')
				ereport(is_strict_mode() ? ERROR : WARNING, 
						(errmsg("Data truncated for column.")));
		}
	}
	else
	{
		/**
		 * If src-number larger than INT64_MAX, strtoll() will return INT64_MAX.
		 * We use strtoull() to distinguish whether src-number is larger than INT64_MAX.
		 */
		if (ret == INT64_MAX)
		{
			errno = 0;
			ret = (int64) strtoull(src, &endptr, 10);
		}
		
		if (errno == ERANGE || *endptr != '\0' || endptr == src)
		{
			ereport(WARNING, 
					(errmsg("Truncated incorrect INTEGER value: '%s'", src)));
		}
		
		/**
		 * Input is positive, but result is negative.
		 * And input cannnot be out of range for strtoull().
		 */
		if (!negative && ret < 0 && errno != ERANGE)
		{
			ereport(WARNING, 
					(errmsg("Cast to signed converted positive out-of-range integer to it's negative complement")));
		}
	}

	return ret;
}

/**
 * Mysql will do round off in insert context if input number is decimal,
 * but not in other context, which is same as cstring_to_signed().
 */
static uint64
cstring_to_unsigned(const char *src, uint64 max_val, bool is_insert)
{
	uint64		ret;
	char		*endptr = NULL;
	bool 		negative;

	src = trim_space(src);

	negative = (src[0] == '-');
	
	errno = 0;
	
	if (negative)
	{
		ret = (uint64) strtoll(src, &endptr, 10);
	}
	else
	{
		ret = strtoull(src, &endptr, 10);
	}

	// If src is negative, such as '-1', strtoull() will not not errno to ERANGE.
	if (ret > max_val)
	{
		ret = max_val;
		errno = ERANGE;
	}

	/**
	 * When in insert stmt, '-0' is the same as '0', which is not negative.
	 * But in other stmt, '-0' is negative number.
	 */
	if (is_insert)
	{
		if (endptr == src && *endptr != '.')
		{
			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errmsg("Incorrect integer value: '%s' for column.", src)));
		}
		/**
		 * String with prefix minus sign, such as '-0', '-0a', '-a', 
		 * they is not negative.
		 */
		else if (negative && ret != 0)
		{
			uint64 dcheck;
			
			errno = 0;
			dcheck = strtoull(src, &endptr, 10);

			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errmsg("Out of range value for column.")));
			
			/**
			 * If src number is small than -18446744073709551615, 
			 * set ret to MAX, else set ret to 0.
			 * 
			 * If src number is negative and is small than -18446744073709551615, 
			 * strtoull() will return UINT64_MAX, which can be used to distinguish
			 * wether src number is small than -18446744073709551615.
			 */
			if (errno != ERANGE || dcheck != UINT64_MAX)
				ret = 0;
			else
				ret = max_val;
		}
		else if (errno == ERANGE)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, 
					(errmsg("Out of range value for column.")));
		}
		else if (*endptr != '\0')
		{
			// do round off
			if (*endptr == '.')
			{
				uint8 next;

				if (*(endptr + 1) == '\0')
					next = 0;
				else
					next = *(endptr + 1) - '0';
				
				// Check there are illegal character
				strtoull(endptr + 1, &endptr, 10);

				/** 
				 * Next char is not digit or larger than 4, 
				 * or the integral part is MAX,
				 * or input number like '-0.9'.
				 */
				if ((next > 4 && next < 10) &&
					((errno == ERANGE) ||
					 (ret == max_val) ||
					 (ret == 0 && negative)))
				{
					ereport(is_strict_mode() ? ERROR : WARNING, 
							(errmsg("Out of range value for column.")));
				}
				else if (next > 4 && next < 10)
				{
					if (ret >= 0)
						ret++;

					if (ret < 0)
						ret--;
				}
			}

			if (*endptr != '\0')
				ereport(is_strict_mode() ? ERROR : WARNING, 
						(errmsg("Data truncated for column.")));
		}
	}
	else
	{
		/**
		 * Such as '-0', '-0a' is negative,
		 * but '-a' is not negative.
		 */
		negative = negative && (ret != 0 || endptr != src);

		if (errno == ERANGE || *endptr != '\0' || endptr == src)
		{
			ereport(WARNING, 
					(errmsg("Truncated incorrect INTEGER value: '%s'", src)));
		}

		// There is a leading minus and the number is not out of range
		if (negative && ret >= 0 && errno != ERANGE)
		{
			ereport(WARNING, 
					(errmsg("Cast to unsigned converted negative integer to it's positive complement")));
		}
	}

	return ret;
}

int8
my_atoi_int8(const char *s, bool is_insert)
{
	return (int8) cstring_to_signed(s, INT8_MIN, INT8_MAX, is_insert);
}

int16
my_atoi_int16(const char *s, bool is_insert)
{
	if (compatible_mode != MYS_MODE)
		return DirectFunctionCall1(int2in, CStringGetDatum(s));

	return (int16) cstring_to_signed(s, INT16_MIN, INT16_MAX, is_insert);
}

int32
my_atoi_int32(const char *s, bool is_insert)
{
	return (int32) cstring_to_signed(s, INT32_MIN, INT32_MAX, is_insert);
}

int64
my_atoi_int64(const char *s, bool is_insert)
{
	return (int64) cstring_to_signed(s, INT64_MIN, INT64_MAX, is_insert);
}

uint8
my_atoi_uint8(const char *s, bool is_insert)
{
	return (uint8) cstring_to_unsigned(s, UINT8_MAX, is_insert);
}

uint16
my_atoi_uint16(const char *s, bool is_insert)
{
	return (uint16) cstring_to_unsigned(s, UINT16_MAX, is_insert);
}

uint32
my_atoi_uint24(const char *s, bool is_insert)
{
	return (uint32) cstring_to_unsigned(s, PG_UINT24_MAX, is_insert);
}

uint32
my_atoi_uint32(const char *s, bool is_insert)
{
	return (uint32) cstring_to_unsigned(s, UINT32_MAX, is_insert);
}

uint64
my_atoi_uint64(const char *s, bool is_insert)
{
	return (uint64) cstring_to_unsigned(s, UINT64_MAX, is_insert);
}

/* Begin - bug#z5590 */
static char *
prepare_numeric_string(char *str)
{
	bool		have_dp = false;
	char 		*cp = str;

	/*
	 * We first parse the string to extract decimal digits and determine the
	 * correct decimal weight.  Then convert to NBASE representation.
	 */
	if (*cp == '+' || *cp == '-')
	{
		cp++;
	}

	if (*cp == '.')
	{
		have_dp = true;
		cp++;
	}

	if (!isdigit((unsigned char) *cp))
	{
		// Truncated
		return cp;
	}

	while (*cp)
	{
		if (isdigit((unsigned char) *cp))
		{
			cp++;
		}
		else if (*cp == '.')
		{
			if (have_dp)
			{
				// Truncated
				return cp;
			}

			have_dp = true;
			cp++;
		}
		else
			break;
	}

	/* Handle exponent, if any */
	if (*cp == 'e' || *cp == 'E')
	{
		long		exponent;
		char	   *endptr;

		cp++;
		exponent = strtol(cp, &endptr, 10);
		if (*endptr)
		{
			// Truncated
			return cp;
		}

		cp = endptr;

		/*
		 * At this point, dweight and dscale can't be more than about
		 * INT_MAX/2 due to the MaxAllocSize limit on string length, so
		 * constraining the exponent similarly should be enough to prevent
		 * integer overflow in this function.  If the value is too large to
		 * fit in storage format, make_result() will complain about it later;
		 * for consistency use the same ereport errcode/text as make_result().
		 */
		if (exponent >= INT_MAX / 2 || exponent <= -(INT_MAX / 2))
		{
			return NULL;
		}
	}

	/* Return end+1 position for caller */
	return cp;
}

float
my_atof(const char *src, bool is_insert)
{
	float 	ret = 0;
	bool	truncated = false,
			out_of_range = false;
	char 	*s, 
			*endptr;
	int 	log_level = WARNING;
	MemoryContext ccxt = CurrentMemoryContext;

	if (is_insert && is_strict_mode())
		log_level = ERROR;
	
	s = trim_space(src);

	if (pg_strcasecmp(s, "NaN") == 0 ||
		pg_strcasecmp(s, "Infinity") == 0 ||
		pg_strcasecmp(s, "+Infinity") == 0 ||
		pg_strcasecmp(s, "-Infinity") == 0 ||
		pg_strcasecmp(s, "inf") == 0 ||
		pg_strcasecmp(s, "+inf") == 0 ||
		pg_strcasecmp(s, "-inf") == 0)
	{
		return DatumGetFloat4(DirectFunctionCall1(float4in, CStringGetDatum(s)));
	}
	
	endptr = prepare_numeric_string(s);

	if (*endptr)
	{
		truncated = true;

		// remove the illegal character
		*endptr = '\0';
	}

	PG_TRY();
	{
		// Only one parameter needs to be passed
		ret = DatumGetFloat4(DirectFunctionCall1(float4in, CStringGetDatum(s)));
	}
	PG_CATCH();
	{
		ErrorData  *errdata;

		MemoryContextSwitchTo(ccxt);

		errdata = CopyErrorData();
		
		switch (errdata->sqlerrcode)
		{
			// input is zero-length string
			case ERRCODE_INVALID_TEXT_REPRESENTATION:
				ret = 0;
				truncated = true;
				FlushErrorState();
				break;
			case ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE:
				out_of_range = true;
				FlushErrorState();
				break;
			default:
				PG_RE_THROW();
				break;
		}

		FreeErrorData(errdata);
	}
	PG_END_TRY();

	if (truncated)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Data truncated for column.")));
		else
			ereport(log_level, (errmsg("Truncated incorrect FLOAT value: \'%s\'", src)));
	}

	if (out_of_range)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Out of range value for column.")));
		else
			ereport(log_level, (errmsg("Truncated incorrect FLOAT value: \'%s\'", src)));

		if (s[0] == '-')
			ret = FLT_MIN;
		else
			ret = FLT_MAX;
	}

	return ret;
}

double
my_atod(const char *src, bool is_insert)
{
	double 	ret = 0;
	bool	truncated = false,
			out_of_range = false;
	char 	*s, 
			*endptr;
	int 	log_level = WARNING;
	MemoryContext ccxt = CurrentMemoryContext;

	if (is_insert && is_strict_mode())
		log_level = ERROR;
	
	s = trim_space(src);

	if (pg_strcasecmp(s, "NaN") == 0 ||
		pg_strcasecmp(s, "Infinity") == 0 ||
		pg_strcasecmp(s, "+Infinity") == 0 ||
		pg_strcasecmp(s, "-Infinity") == 0 ||
		pg_strcasecmp(s, "inf") == 0 ||
		pg_strcasecmp(s, "+inf") == 0 ||
		pg_strcasecmp(s, "-inf") == 0)
	{
		return float8in_internal(s, NULL, "double precision", s);
	}
	
	endptr = prepare_numeric_string(s);

	if (*endptr)
	{
		truncated = true;

		// remove the illegal character
		*endptr = '\0';
	}

	PG_TRY();
	{
		ret = float8in_internal(s, NULL, "double precision", s);
	}
	PG_CATCH();
	{
		ErrorData  *errdata;

		MemoryContextSwitchTo(ccxt);
		
		errdata = CopyErrorData();
		
		switch (errdata->sqlerrcode)
		{
			// input is zero-length string
			case ERRCODE_INVALID_TEXT_REPRESENTATION:
				ret = 0;
				truncated = true;
				FlushErrorState();
				break;
			case ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE:
				out_of_range = true;
				FlushErrorState();
				break;
			default:
				PG_RE_THROW();
				break;
		}

		FreeErrorData(errdata);
	}
	PG_END_TRY();

	if (truncated)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Data truncated for column.")));
		else
			ereport(log_level, (errmsg("Truncated incorrect DOUBLE value: \'%s\'", src)));
	}

	if (out_of_range)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Out of range value for column.")));
		else
			ereport(log_level, (errmsg("Truncated incorrect DOUBLE value: \'%s\'", src)));

		if (s[0] == '-')
			ret = DBL_MIN;
		else
			ret = DBL_MAX;
	}

	return ret;
}

static Numeric
make_max_numeric(int typmod, bool negative)
{
	int			precision;
	int			scale;
	int			maxdigits;
	char		*buf = NULL,
				*pos = NULL;

	if (typmod < (int32) (VARHDRSZ))
		typmod = (10 << 16) + VARHDRSZ;

	typmod -= VARHDRSZ;
	precision = (typmod >> 16) & 0xffff;
	scale = typmod & 0xffff;
	maxdigits = precision - scale;

	// Apply additional space for: dot(.), negative sign(-) and NULL-terminal(\0)
	pos = buf = palloc0(precision + 3);

	if (negative)
		*pos++ = '-';

	for (int i = 0; i < maxdigits; i++)
		*pos++ = '9';
	
	if (scale)
		*pos++ = '.';

	for (int i = 0; i < scale; i++)
		*pos++ = '9';

	return DatumGetNumeric(DirectFunctionCall3(numeric_in,
											   CStringGetDatum(buf),
											   ObjectIdGetDatum(InvalidOid),
											   Int32GetDatum(typmod + VARHDRSZ)));
}

Numeric
my_aton(const char *src, int typmod, bool is_insert)
{
	Numeric ret = 0;
	bool	truncated = false,
			out_of_range = false;
	char 	*s, 
			*endptr;
	int 	log_level = WARNING;
	MemoryContext ccxt = CurrentMemoryContext;

	if (is_insert && is_strict_mode())
		log_level = ERROR;
	
	s = trim_space(src);

	if (pg_strcasecmp(s, "NaN") == 0 ||
		pg_strcasecmp(s, "Infinity") == 0 ||
		pg_strcasecmp(s, "+Infinity") == 0 ||
		pg_strcasecmp(s, "-Infinity") == 0 ||
		pg_strcasecmp(s, "inf") == 0 ||
		pg_strcasecmp(s, "+inf") == 0 ||
		pg_strcasecmp(s, "-inf") == 0)
	{
		return DatumGetNumeric(DirectFunctionCall3(numeric_in,
												   CStringGetDatum(s),
												   ObjectIdGetDatum(InvalidOid),
												   Int32GetDatum(typmod)));
	}
	
	endptr = prepare_numeric_string(s);

	if (*endptr)
	{
		truncated = true;

		// remove the illegal character
		*endptr = '\0';
	}

	PG_TRY();
	{
		ret = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												  CStringGetDatum(s),
												  ObjectIdGetDatum(InvalidOid),
												  Int32GetDatum(typmod)));
	}
	PG_CATCH();
	{
		ErrorData  *errdata;

		MemoryContextSwitchTo(ccxt);
		
		errdata = CopyErrorData();
		
		switch (errdata->sqlerrcode)
		{
			// input is zero-length string
			case ERRCODE_INVALID_TEXT_REPRESENTATION:
				ret = DatumGetNumeric(DirectFunctionCall3(numeric_in,
														  CStringGetDatum("0"),
														  ObjectIdGetDatum(InvalidOid),
														  Int32GetDatum(typmod)));
				truncated = true;
				FlushErrorState();
				break;
			case ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE:
				out_of_range = true;
				FlushErrorState();
				break;
			default:
				PG_RE_THROW();
				break;
		}

		FreeErrorData(errdata);
	}
	PG_END_TRY();

	if (truncated)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Incorrect decimal value: '%s' for column.", src)));
		else if (strlen(s) == 0)
			ereport(log_level, (errmsg("Truncated incorrect DECIMAL value: '%s'", src)));
	}

	if (out_of_range)
	{
		if (is_insert)
			ereport(log_level, (errmsg("Out of range value for column.")));
		else
			ereport(log_level, (errmsg("Truncated incorrect DECIMAL value: '%s'", src)));

		ret = make_max_numeric(typmod, s[0] == '-');
	}

	return ret;
}
/* End - bug#z5590 */
