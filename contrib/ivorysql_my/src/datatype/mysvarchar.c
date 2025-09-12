/*-------------------------------------------------------------------------
 *
 * mysvarchar.c
 *	  Implenmentation of the MySQL varchar functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/set/set_user_var.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/builtins.h"
#include "utils/float.h"
#include "fmgr.h"
#include "../include/ivorysql_my.h"
#include "utils/numeric.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


PG_FUNCTION_INFO_V1(mysvarchar_to_numeric);

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
	char	   *temp;
	int			temp_max_len = 0;
	/* Enlarged buffer to avoid snprintf truncation warnings */
	char	   *final_buf;
	size_t		temp_idx = 0;
	const char *p = str;
	int			negative = 0;	/* Flag for negative sign */
	/* Check if all digits (ignoring '.', 'e', 'E', and exponent sign) are zero */
	bool		allZero = true;

	temp = (char *)palloc0(size);
	final_buf = (char *)palloc0(size);

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

	pfree(temp);
	pfree(final_buf);

	return strlen(out_buf);
}

/*
 * mysvarchar_to_numeric:
 *   transform varchar to numeric.
 *
 * Parameters
 *   varchar type string.
 *
 * Returns:
 *   numeric datum.
 */
Datum
mysvarchar_to_numeric(PG_FUNCTION_ARGS)
{
	VarChar		   *source = PG_GETARG_VARCHAR_PP(0);
	char		   *s_data;
	char		   *num1;

	s_data = VARDATA_ANY(source);

	num1 = (char *)palloc0(sizeof(s_data) + 1);
	extract_leading_number_str(s_data, num1, sizeof(s_data) + 1);

	PG_RETURN_DATUM(DirectFunctionCall3(numeric_in,
										CStringGetDatum(num1),
										ObjectIdGetDatum(InvalidOid),
										Int32GetDatum(-1)));
}
