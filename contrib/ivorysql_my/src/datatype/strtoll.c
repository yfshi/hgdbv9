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

#include "postgres.h"
 
#include "../include/ivorysql_my.h"
#include "utils/guc.h"
#include "utils/fmgrprotos.h"
#include "utils/ora_compatible.h"

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
	if (compatible_db != MYS_PARSER)
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
my_atoi_uint32(const char *s, bool is_insert)
{
	return (uint32) cstring_to_unsigned(s, UINT32_MAX, is_insert);
}

uint64
my_atoi_uint64(const char *s, bool is_insert)
{
	return (uint64) cstring_to_unsigned(s, UINT64_MAX, is_insert);
}
