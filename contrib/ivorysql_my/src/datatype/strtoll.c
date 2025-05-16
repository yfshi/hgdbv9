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

#include <limits.h>

#define CUTOFF  (ULLONG_MAX / 10)
#define CUTLIM  (ULLONG_MAX % 10)
#define DIGITS_IN_ULONGLONG 20

static uint64 d10[DIGITS_IN_ULONGLONG] =
{
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	10000000000ULL,
	100000000000ULL,
	1000000000000ULL,
	10000000000000ULL,
	100000000000000ULL,
	1000000000000000ULL,
	10000000000000000ULL,
	100000000000000000ULL,
	1000000000000000000ULL,
	10000000000000000000ULL
};

/*

  Convert a string to unsigned long long integer value
  with rounding.
  
  SYNOPSYS
    my_strntoull10_8bit()
      cs              in      pointer to character set
      str             in      pointer to the string to be converted
      length          in      string length
      unsigned_flag   in      whether the number is unsigned
      endptr          out     pointer to the stop character
      error           out     returned error code

  DESCRIPTION
    This function takes the decimal representation of integer number
    from string str and converts it to an signed or unsigned
    long long integer value.
    Space characters and tab are ignored.
    A sign character might precede the digit characters.
    The number may have any number of pre-zero digits.
    The number may have decimal point and exponent.
    Rounding is always done in "away from zero" style:
      0.5  ->   1
     -0.5  ->  -1

    The function stops reading the string str after "length" bytes
    or at the first character that is not a part of correct number syntax:

    <signed numeric literal> ::=
      [ <sign> ] <exact numeric literal> [ E [ <sign> ] <unsigned integer> ]

    <exact numeric literal> ::=
                        <unsigned integer> [ <period> [ <unsigned integer> ] ]
                      | <period> <unsigned integer>
    <unsigned integer>   ::= <digit>...
     
  RETURN VALUES
    Value of string as a signed/unsigned longlong integer

    endptr cannot be NULL. The function will store the end pointer
    to the stop character here.

    The error parameter contains information how things went:
    0	     ok
    ERANGE   If the the value of the converted number is out of range
    In this case the return value is:
    - ULLONG_MAX if unsigned_flag and the number was too big
    - 0 if unsigned_flag and the number was negative
    - LLONG_MAX if no unsigned_flag and the number is too big
    - LLONG_MIN if no unsigned_flag and the number it too big negative
    
    EDOM If the string didn't contain any digits.
    In this case the return value is 0.
*/
uint64 
my_strntoull10rnd_8bit(const char *str, size_t length, int unsigned_flag, char **endptr, int *error)
{
	const char *dot, *end9, *beg, *end = str + length;
	uint64 ull;
	uint64 ul;
	uchar ch;
	int shift = 0, digits = 0, negative, addon;

	/* Skip leading spaces and tabs */
	for (; str < end && (*str == ' ' || *str == '\t'); str++)
		;

	if (str >= end)
		goto ret_edom;

	if ((negative = (*str == '-')) || *str == '+') /* optional sign */
	{
		if (++str == end)
			goto ret_edom;
	}

	beg = str;
	end9 = (str + 9) > end ? end : (str + 9);
	/* Accumulate small number into uint64, for performance purposes */
	for (ul = 0; str < end9 && (ch = (uchar)(*str - '0')) < 10; str++)
	{
		ul = ul * 10 + ch;
	}

	if (str >= end) /* Small number without dots and expanents */
	{
		*endptr = (char *)str;
		if (negative)
		{
			if (unsigned_flag)
			{
				*error = ul ? ERANGE : 0;
				return 0;
			}
			else
			{
				*error = 0;
				return (uint64)(int64) - (long)ul;
			}
		}
		else
		{
			*error = 0;
			return (uint64)ul;
		}
	}

	digits = (int)(str - beg);

	/* Continue to accumulate into uint64 */
	for (dot = NULL, ull = ul; str < end; str++)
	{
		if ((ch = (uchar)(*str - '0')) < 10)
		{
			if (ull < CUTOFF || (ull == CUTOFF && ch <= CUTLIM))
			{
				ull = ull * 10 + ch;
				digits++;
				continue;
			}
			/**
			 * Adding the next digit would overflow.
			 * Remember the next digit in "addon", for rounding.
			 * Scan all digits with an optional single dot.
			 */
			if (ull == CUTOFF)
			{
				ull = ULLONG_MAX;
				addon = 1;
				str++;
			}
			else
				addon = (*str >= '5');
			if (!dot)
			{
				for (; str < end && (ch = (uchar)(*str - '0')) < 10; shift++, str++)
					;
				if (str < end && *str == '.')
				{
					str++;
					for (; str < end && (ch = (uchar)(*str - '0')) < 10; str++)
						;
				}
			}
			else
			{
				shift = (int)(dot - str);
				for (; str < end && (ch = (uchar)(*str - '0')) < 10; str++)
					;
			}
			goto exp;
		}

		if (*str == '.')
		{
			if (dot)
			{
				/* The second dot character */
				addon = 0;
				goto exp;
			}
			else
			{
				dot = str + 1;
			}
			continue;
		}

		/* Unknown character, exit the loop */
		break;
	}
	shift = dot ? (int)(dot - str) : 0; /* Right shift */
	addon = 0;

exp: /* [ E [ <sign> ] <unsigned integer> ] */

	if (!digits)
	{
		str = beg;
		goto ret_edom;
	}

	if (str < end && (*str == 'e' || *str == 'E'))
	{
		str++;
		if (str < end)
		{
			int negative_exp, exponent;
			if ((negative_exp = (*str == '-')) || *str == '+')
			{
				if (++str == end)
					goto ret_sign;
			}
			for (exponent = 0;
				 str < end && (ch = (uchar)(*str - '0')) < 10;
				 str++)
			{
				exponent = exponent * 10 + ch;
			}
			shift += negative_exp ? -exponent : exponent;
		}
	}

	if (shift == 0) /* No shift, check addon digit */
	{
		if (addon)
		{
			if (ull == ULLONG_MAX)
				goto ret_too_big;
			ull++;
		}
		goto ret_sign;
	}

	if (shift < 0) /* Right shift */
	{
		uint64 d, r;

		if (-shift >= DIGITS_IN_ULONGLONG)
			goto ret_zero; /* Exponent is a big negative number, return 0 */

		d = d10[-shift];
		r = (ull % d) * 2;
		ull /= d;
		if (r >= d)
			ull++;
		goto ret_sign;
	}

	if (shift > DIGITS_IN_ULONGLONG) /* Huge left shift */
	{
		if (!ull)
			goto ret_sign;
		goto ret_too_big;
	}

	for (; shift > 0; shift--, ull *= 10) /* Left shift */
	{
		if (ull > CUTOFF)
			goto ret_too_big; /* Overflow, number too big */
	}

ret_sign:
	*endptr = (char *)str;

	if (!unsigned_flag)
	{
		if (negative)
		{
			if (ull > (uint64)LLONG_MIN)
			{
				*error = ERANGE;
				return (uint64)LLONG_MIN;
			}
			*error = 0;
			return (uint64) - (int64)ull;
		}
		else
		{
			if (ull > (uint64)LLONG_MAX)
			{
				*error = ERANGE;
				return (uint64)LLONG_MAX;
			}
			*error = 0;
			return ull;
		}
	}

	/* Unsigned number */
	if (negative && ull)
	{
		*error = ERANGE;
		return 0;
	}
	*error = 0;
	return ull;

ret_zero:
	*endptr = (char *)str;
	*error = 0;
	return 0;

ret_edom:
	*endptr = (char *)str;
	*error = EDOM;
	return 0;

ret_too_big:
	*endptr = (char *)str;
	*error = ERANGE;
	return unsigned_flag ? ULLONG_MAX : negative ? (uint64)LLONG_MIN
												 : (uint64)LLONG_MAX;
}

/*
 * The in function does not need to determine the sql_mode
 * return warning.  
 */
uint64 pg_atoi_uint(const char *s,int c, int sizenum, bool seterr)
{
	unsigned long long l;
	char *badp;
	int save_errno = 0;
	uint64	maxvalue = 0;
	int flag = UNSIGNED_FLAG;
	int EREPORT_LEVEL = 0;

	if (seterr)
	{
		EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
	}
	else
	{
		EREPORT_LEVEL =  WARNING;
	}
	switch (sizenum)
	{
		case  1:
			maxvalue = PG_UINT8_MAX;
		break;
		case  2:
			maxvalue = PG_UINT16_MAX;
		break;
		case  3:
			maxvalue = PG_UINT24_MAX;
		break;
		case  4:
			maxvalue = PG_UINT32_MAX;
		break;
		case  8:
			maxvalue = PG_UINT64_MAX;
		break;
		default:
			maxvalue = PG_UINT64_MAX;
		break;
	}
	/*
	 * Some versions of strtol treat the empty string as an error, but some
	 * seem not to.  Make an explicit test to be sure we catch it.
	 */
	if (s == NULL)
		elog(ERROR, "NULL pointer");
	if (*s == 0)
	{
		ereport(EREPORT_LEVEL,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("invalid input syntax for type %s: \"%s\"",
					"integer", s)));
		l = 0;
		return l;
	}

	if (seterr)
	{
		errno = 0;
		l = my_strntoull10rnd_8bit(s, strlen(s), flag, &badp, &save_errno);
	}
	else
	{
		errno = 0;
		l = strtoull(s, &badp, 10);
		save_errno = errno;
	}

	/* We made no progress parsing the string, so bail out */
	if (s == badp)
	{
		ereport(EREPORT_LEVEL,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("invalid input syntax for type %s: \"%s\"",
					"integer", s)));
		l = 0;
		return l;
	}

	if (save_errno == ERANGE || l < 0 || l > maxvalue)
	{
		ereport(EREPORT_LEVEL,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("value \"%s\" is out of range for unsigned %d-bit integer", s, sizenum*8)));
		l = l < 0 ? 0 : maxvalue;
		return l;
	}

	/*
	 * Skip any trailing whitespace; if anything but whitespace remains before
	 * the terminating character, bail out
	 */
	while (*badp && *badp != c && isspace((unsigned char)*badp))
		badp++;

	if (*badp && *badp != c)
	{
		ereport(EREPORT_LEVEL,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			 errmsg("invalid input syntax for type %s: \"%s\"",
					"integer", s)));
	}

	return (uint64)l;
}