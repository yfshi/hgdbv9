/*-------------------------------------------------------------------------------
 *
 * File: numeric_datatype_functions.c
 *
 * Abstract:
 * 		This file contains the implementation of Oracle's
 *		numeric data type related built-in functions.
 *
 * Authored by zhenmingyang@highgo.com, 20220510.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_ora/src/builtin_functions/numeric_datatype_functions.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/fmgrprotos.h"		/* ReqID:SRS-SQL-SYSFUNC */
#include "utils/formatting.h"
#include "utils/guc.h"				/* ReqID:SRS-SQL-SYSFUNC */
#include "utils/numeric.h"

/* Begin - ReqID:SRS-SQL-SYSFUNC */
#include <math.h>
#include <string.h>

#define INVALID_NLS ereport(ERROR, (errmsg("invalid NLS parameter string " \
										   "used in SQL Function")))
#define INVALID_NUMBER ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),(errmsg("invalid number"))))
#define NUMERIC_OVERFLOW ereport(ERROR, (errmsg("numeric overflow")))

typedef struct FormatElems
{
	uint8	n_elements,			/* the number of number format model element */
			n_integer,			/* the number of digits before decimal character */
			n_decimal,			/* the number of digits after decimal character */
			first_digit,		/* the position of the first digit */
			miprs,				/* the position of MI/PR/S */
			dollar_clu,			/* the position of $/C/L/U */
			period_d,			/* the position of ./D */
			comma_g,			/* g:103 G:71 ,:174 */
			eeee;
} FormatElems;

typedef struct NLSParams
{
	char   *numeric_characters,			/* decimal_character group_separator */
		   *currency,					/* L - NLS_CURRENCY */
		   *iso_currency,				/* C - NLS_ISO_CURRENCY
										 * "CHINA" and "AMERICA" supported now
										 * store "CNY" or "USD" here
										 */
		   *dual_currency;				/* U - NLS_DUAL_CURRENCY */
} NLSParams;


static Numeric ora_to_number_implement(text *expr, text *defval,
									   text *fmt, text *nlsparam);
static void ora_to_number_nls_preprocess(NLSParams *nls);
static void ora_to_number_nls_check(text *nlsparam, NLSParams *nls);
static void ora_to_number_nls_process(char **src, char **currency,
									  char **iso_currency, char **dual_currency,
									  char **numeric_characters);
static char * ora_to_number_nls_value(char **src, int nls_name_length);
static bool ora_to_number_fmt_check(char *format, FormatElems *fmts,
									NLSParams *nls);
static bool ora_to_number_hex_check(char *format, char **expression,
									NLSParams *nls);
static Numeric ora_hex_to_dec_calc(char *hexadecimal);
static bool ora_to_number_expr_check(char *format, char **expression,
									 FormatElems *fmts, NLSParams *nls);
static bool ora_to_number_digits_check(char *format, char *digits, char character,
									   FormatElems *fmts, NLSParams *nls);
static bool ora_to_number_comma_match(char *format, char *expression,
									  char character, FormatElems *fmts);
static Numeric string_to_numeric(char *str, bool *has_error);


PG_FUNCTION_INFO_V1(ora_to_number_w);
/* End - ReqID:SRS-SQL-SYSFUNC */
PG_FUNCTION_INFO_V1(number_bitand);
PG_FUNCTION_INFO_V1(ora_to_number);		/* bug0000451 */


/* Begin - bug0000457 */
Datum
number_bitand(PG_FUNCTION_ARGS)
{
	Numeric		arg1 = PG_GETARG_NUMERIC(0);
	Numeric		arg2 = PG_GETARG_NUMERIC(1);

	PG_RETURN_NUMERIC(numeric_bitand(arg1, arg2));
}
/* End - bug0000457 */

/* Begin - ReqID:SRS-SQL-SYSFUNC */
/*
 * Functioncall for TO_NUMBER syntax
 */
Datum
ora_to_number_w(PG_FUNCTION_ARGS)
{
	Numeric	result = NULL;
	bool	has_def,
			def_isnull;
	text   *expr,
		   *defval,
		   *fmt,
		   *nlsparam;

	has_def = PG_GETARG_BOOL(0);
	def_isnull = PG_GETARG_BOOL(1);
	expr = PG_GETARG_TEXT_P(2);
	if (has_def)
	{
		if (def_isnull)
		{
			defval = (text *) palloc0(1 + VARHDRSZ);
			SET_VARSIZE(defval, 1 + VARHDRSZ);
			memset(VARDATA_ANY(defval), 0, 1);
		}
		else
			defval = PG_NARGS() > 3 ? PG_GETARG_TEXT_P(3) : NULL;
		fmt = PG_NARGS() > 4 ? PG_GETARG_TEXT_P(4) : NULL;
		nlsparam = PG_NARGS() > 5 ? PG_GETARG_TEXT_P(5) : NULL;
	}
	else
	{
		defval = NULL;
		fmt = PG_NARGS() > 3 ? PG_GETARG_TEXT_P(3) : NULL;
		nlsparam = PG_NARGS() > 4 ? PG_GETARG_TEXT_P(4) : NULL;
	}

	result = ora_to_number_implement(expr, defval, fmt, nlsparam);

	if (result == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_NUMERIC(result);
}

/*
 * ora_to_number_implement
 *     implementation of Oracle compatible function TO_NUMBER()
 *
 *     TO_NUMBER( expr [ default return_value on conversion error ]
 *               [ , fmt [ , 'nlsparam' ] ] )
 */
static Numeric
ora_to_number_implement(text *expr, text *defval, text *fmt, text *nlsparam)
{
	Numeric		result;
	int			length;
	char	   *expression,				/* String of "expr" / "return_value" */
			   *format;					/* String of "fmt" */
	bool		has_error = false;
	FormatElems	fmts = {0, 0, 0, 255, 255, 255, 255, 255, 255};
	NLSParams	nls;

	/*
	 * nls parameters' value initilization
	 */
	ora_to_number_nls_preprocess(&nls);

	/*
	 * 'nlsparam' validation check,
	 * and update the default value if necessary.
	 */
	if (nlsparam)
		ora_to_number_nls_check(nlsparam, &nls);

	/*
	 * Number Format Model Elements check
	 */
	if (fmt)
	{
		length = VARSIZE_ANY_EXHDR(fmt);
		format = (char *) palloc0(length + 1);
		memcpy(format, VARDATA_ANY(fmt), length);
		length = VARSIZE_ANY_EXHDR(expr);

		/* Report error when it is not a valid Format Model. */
		if (!ora_to_number_fmt_check(format, &fmts, &nls))
		{
			expression = (char *) palloc0(length + 6);
			memcpy(expression, VARDATA_ANY(expr), length);

HEX_CHECK_AGAIN:	/* there may be "DEFAULT return_value ON CONVERSION ERROR" */
			if (!ora_to_number_hex_check(format, &expression, &nls))
			{
				if (defval)
				{
					pfree(expression);
					length = VARSIZE_ANY_EXHDR(defval);
					expression = (char *) palloc0(length + 6);
					memcpy(expression, VARDATA_ANY(defval), length);
					defval = NULL;
					goto HEX_CHECK_AGAIN;
				}
				else
				{
					INVALID_NUMBER;
					return NULL;
				}
			}
			else
				return ora_hex_to_dec_calc(expression);
		}
		else
		{
			expression = (char *) palloc0(length + 1);
			memcpy(expression, VARDATA_ANY(expr), length);

EXPR_CHECK_AGAIN:							/* same as "HEX_CHECK_AGAIN" */
			if (!ora_to_number_expr_check(format, &expression, &fmts, &nls))
			{
				if (defval)
				{
					pfree(expression);
					length = VARSIZE_ANY_EXHDR(defval);
					expression = (char *) palloc0(length + 1);
					memcpy(expression, VARDATA_ANY(defval), length);
					defval = NULL;
					goto EXPR_CHECK_AGAIN;
				}
				else
				{
					INVALID_NUMBER;
					return NULL;
				}
			}
		}

		pfree(format);
	}
	else
	{
		length = VARSIZE_ANY_EXHDR(expr);
		expression = (char *) palloc0(length + 1);
		memcpy(expression, VARDATA_ANY(expr), length);
	}

	/*
	 * try to convert "expr"
	 */
CONVERT_AGAIN:								/* same as "HEX_CHECK_AGAIN" */
	result = string_to_numeric(expression, &has_error);
	if (has_error)
	{
		if (defval)
		{
			has_error = false;
			pfree(expression);
			length = VARSIZE_ANY_EXHDR(defval);
			expression = palloc0(length + 1);
			memcpy(expression, VARDATA_ANY(defval), length);
			defval = NULL;
			goto CONVERT_AGAIN;
		}
		else if (strncmp(expression, "Inf", 3) == 0)
			NUMERIC_OVERFLOW;
		else
			INVALID_NUMBER;
	}

	pfree(expression);
	pfree(nls.numeric_characters);
	pfree(nls.currency);
	pfree(nls.iso_currency);
	pfree(nls.dual_currency);

	return result;
}

/*
 * ora_to_number_nls_preprocess
 *     initilization of "nls" using corresponding guc parameters
 */
static void
ora_to_number_nls_preprocess(NLSParams *nls)
{
	int		length;

	nls->currency = (char *) palloc0(11);
	nls->iso_currency = (char *) palloc0(4);
	nls->dual_currency = (char *) palloc0(8);
	nls->numeric_characters = (char *) palloc0(3);

	length = strlen(nls_currency) > 10 ? 10 : strlen(nls_currency);
	memcpy(nls->currency, nls_currency, length);
	if (pg_strcasecmp(nls_iso_currency, "CHINA") == 0)
		memcpy(nls->iso_currency, "CNY", 3);
	else if (pg_strcasecmp(nls_iso_currency, "AMERICA") == 0)
		memcpy(nls->iso_currency, "USD", 3);
	length = strlen(nls_dual_currency) > 7 ? 7 : strlen(nls_dual_currency);
	memcpy(nls->dual_currency, nls_dual_currency, length);
	memcpy(nls->numeric_characters, nls_numeric_characters, 2);
}

/*
 * which_nls
 *     which NLS_xxx leading
 */
static int
which_nls(const char *met)
{
	if (met == NULL)
		return -1;
	else if (pg_strncasecmp(met, "NLS_NUMERIC_CHARACTERS", 22) == 0)
		return 1;
	else if (pg_strncasecmp(met, "NLS_CURRENCY", 12) == 0)
		return 2;
	else if (pg_strncasecmp(met, "NLS_ISO_CURRENCY", 16) == 0)
		return 3;
	else if (pg_strncasecmp(met, "NLS_DUAL_CURRENCY", 17) == 0)
		return 4;
	else
		return 0;
}

/*
 * ora_to_number_nls_check
 *     check each nlsparam
 */
static void
ora_to_number_nls_check(text *nlsparam, NLSParams *nls)
{
	char   *str,
		   *ptr,
		   *currency = NULL,
		   *iso_currency = NULL,
		   *dual_currency = NULL,
		   *numeric_characters = NULL;
	int		length;

	length = VARSIZE_ANY_EXHDR(nlsparam);
	str = (char *) palloc0(length + 1);
	memcpy(str, VARDATA_ANY(nlsparam), length);
	/* toupper(NLS_) */
	for (ptr = str; *ptr != '\0'; ++ptr)
	{
		switch (which_nls(ptr))
		{
			case 1:
				memcpy(ptr, "NLS_NUMERIC_CHARACTERS", 22);
				ptr += 21;
				break;
			case 2:
				memcpy(ptr, "NLS_CURRENCY", 12);
				ptr += 11;
				break;
			case 3:
				memcpy(ptr, "NLS_ISO_CURRENCY", 16);
				ptr += 15;
				break;
			case 4:
				memcpy(ptr, "NLS_DUAL_CURRENCY", 17);
				ptr += 16;
				break;
		}
	}
	/* Skip leading space. */
	for (ptr = str; *ptr == ' '; ++ptr);

	ora_to_number_nls_process(&ptr, &currency, &iso_currency, &dual_currency,
							  &numeric_characters);

	if (currency)
	{
		if (strcmp(currency, nls->currency) != 0)
		{
			length = strlen(currency) > 10 ? 10 : strlen(currency);
			memset(nls->currency, 0, 11);
			memcpy(nls->currency, currency, length);
		}
		pfree(currency);
	}
	if (iso_currency)
	{
		if (strcmp(iso_currency, nls->currency) != 0)
			memcpy(nls->iso_currency, iso_currency, 3);
		pfree(iso_currency);
	}
	if (dual_currency)
	{
		if (strcmp(dual_currency, nls->dual_currency) != 0)
		{
			length = strlen(dual_currency) > 7 ? 7 : strlen(dual_currency);
			memset(nls->dual_currency, 0, 8);
			memcpy(nls->dual_currency, dual_currency, length);
		}
		pfree(dual_currency);
	}
	if (numeric_characters)
	{
		if (strncmp(numeric_characters, nls->numeric_characters, 2) != 0)
			memcpy(nls->numeric_characters, numeric_characters, 2);
		pfree(numeric_characters);
	}

	/* clean */
	pfree(str);
	return;
}

/*
 * ora_to_number_nls_process
 *     get value from 'nlsparam'
 */
static void
ora_to_number_nls_process(char **src, char **currency, char **iso_currency,
						  char **dual_currency, char **numeric_characters)
{
	do
	{
		switch (which_nls(*src))
		{
			case 0:
				INVALID_NLS;
				/* FALLTHROUGH */
			case 1:
				if (*numeric_characters)
					INVALID_NLS;

				*numeric_characters = ora_to_number_nls_value(&*src, 22);

				if (!*numeric_characters)
					break;
				/*
				 * 'nls_numeric_characters = abcde' => decimal character : a
				 *                                     group separator : b
				 * 'nls_numeric_characters = aa' => ORA-12702: invalid nls ...
				 */
				else if (strlen(*numeric_characters) < 2
						|| (strlen(*numeric_characters) >= 2
							&& (**numeric_characters == *(*numeric_characters + 1))))
					INVALID_NLS;
				break;
			case 2:
				if (*currency)
					INVALID_NLS;

				*currency = ora_to_number_nls_value(&*src, 12);
				break;
			case 3:
				if (*iso_currency)
					INVALID_NLS;

				*iso_currency = ora_to_number_nls_value(&*src, 16);

				if (!iso_currency)
					break;
				/* Only CHINA and AMERICA supported now. */
				else if (!pg_strcasecmp(*iso_currency, "CHINA"))
					memcpy(*iso_currency, "CNY", 3);
				else if (!pg_strcasecmp(*iso_currency, "AMERICA"))
					memcpy(*iso_currency, "USD", 3);
				else
					INVALID_NLS;
				break;
			case 4:
				if (*dual_currency)
					INVALID_NLS;

				*dual_currency = ora_to_number_nls_value(&*src, 17);
				break;
			default:
				break;
		}
	} while (*src);
}

/*
 * ora_to_number_nls_value
 *     get the value of specific nls param
 */
static char *
ora_to_number_nls_value(char **src, int nls_name_length)
{
	char   *dest,
		   *start,
		   *end = NULL,
		   *step;
	int		n_quote = 0;

	/*
	 * *src is a string with terminal symbol, so that
	 * step here must not be out of the range of *src.
	 */
	start = *src + nls_name_length;
	for (step = start; *step == ' '; ++step);
	if (*step != '=')
		INVALID_NLS;
	else
		for (start = step + 1; *start == ' '; ++start);

	/*
	 * If there's a '\'' at the beginning, we need a '\''
	 * which makes the number of quotes even before the next NLS_PARAMETER.
	 *
	 * The next NLS_PARAMETER may not be a valid one, re-check it outside.
	 */
	end = strstr(start, "NLS_");
	end = end ? end : start + strlen(start);
	dest = (char *) palloc0(257);
	if (*start == '\'')
	{
		char   *last_quote;

		/* locate the last quote */
		for (last_quote = end; *last_quote != '\''; --last_quote);
		for (step = start; step <= end; ++step)
		{
			if (*step == '\'')
				++n_quote;

			/* If the last quote could be the end. */
			if (step == last_quote && n_quote % 2 == 0)
				break;

			/* It's not an even number of quotes until the end. */
			if (step == end && n_quote % 2 == 1)
			{
				end = strstr(end + 1, "NLS_");
				end = end ? end : start + strlen(start);
			}

			if (step > start && step < end)
			{
				if (*step == '\'')
				{
					if (*(step + 1) == '\'')
					{
						++step;
						++n_quote;
					}
					else
						INVALID_NLS;
				}

				dest[strlen(dest)] = *step;
				if (strlen(dest) > 256)
					INVALID_NLS;
			}
		}
	}
	else
	{
		/*
		 * The value of nls_currency is from 'a' to 'b',
		 * leading space and trailing space should be ignored.
		 */
		for (step = start; *step != ' ' && step < end; ++step)
		{
			/* extra space is wrong */
			if (*step == '\'')
				INVALID_NLS;
			else
				dest[strlen(dest)] = *step;

			if (strlen(dest) > 256)
				INVALID_NLS;
		}

		/*
		 * There must be all space between step and end.
		 */
		for (; step < end; ++step)
			if (*step != ' ')
				INVALID_NLS;
	}

	*src = end == *src + strlen(*src) ? NULL : end;

	return dest;
}

/*
 * ora_to_number_fmt_check
 *     check which element is in "fmt"
 */
static bool
ora_to_number_fmt_check(char *format, FormatElems *fmts, NLSParams *nls)
{
	char   *copy,
		   *step = NULL,
		   *bystep = NULL;
	int		n_comma = 0;
	bool	has_dp = false;

	/*
	 * "fm..." or "sfm..." is valid.
	 */
	if (pg_strncasecmp(format, "fm", 2) == 0)
	{
		copy = (char *) palloc0(strlen(format) - 1);
		memcpy(copy, format + 2, strlen(format) - 2);
		memset(format, 0, strlen(format));
		memcpy(format, copy, strlen(copy));
	}
	else if (pg_strncasecmp(format + 1, "fm", 2) == 0)
	{
		if (*format != 's' && *format != 'S')
			goto INVALID_FORMAT;
		copy = (char *) palloc0(strlen(format) - 1);
		*copy = *format;
		memcpy(copy + 1, format + 3, strlen(format) - 3);
		memset(format, 0, strlen(format));
		memcpy(format, copy, strlen(copy));
	}

	/*
	 * Collect Number Format Model Elements in "fmt"
	 */
	for (step = format; *step != '\0'; ++step)
	{
		if (fmts->n_elements == 63)
			goto INVALID_FORMAT;

		/*
		 * Number Format Model Element
		 */
		switch (*step)
		{
			/*
			 * Number
			 */
			case '0':
				/* FALLTHROUGH */
			case '9':
				/* Position of the first digit(0 or 9). */
				if (fmts->first_digit == 255)
					fmts->first_digit = step - format;
				if (!has_dp)
					++fmts->n_integer;
				else
					++fmts->n_decimal;
				++fmts->n_elements;
				break;

			/*
			 * not supported now
			 */
			/* TM9 , TME */
			case 't':
				/* FALLTHROUGH */
			case 'T':
				if (step == format
						&& (pg_strcasecmp(step, "tm9") == 0
							|| pg_strcasecmp(step, "tme") == 0))
					ereport(ERROR, (errmsg("This Format Model Element(TM) "
										   "is not supported now.")));
				else
					goto INVALID_FORMAT;
				break;
			/* rn , RN */
			case 'r':
				/* FALLTHROUGH */
			case 'R':
				if (step == format && pg_strncasecmp(step, "rn", 2) == 0)
					ereport(ERROR, (errmsg("This Format Model Element(RN) "
										   "is not supported now.")));
				else
					goto INVALID_FORMAT;
				break;
			case 'b':
				/* FALLTHROUGH */
			case 'B':
				ereport(ERROR, (errmsg("This Format Model Element(B) "
									   "is not supported now.")));
				break;
			case 'v':
				/* FALLTHROUGH */
			case 'V':
				ereport(ERROR, (errmsg("This Format Model Element(V) "
									   "is not supported now.")));
				break;

			/*
			 * Decimal character
			 */
			case '.':
				/* . and g/G are mutually exclusive. */
				if (fmts->comma_g == 71 || fmts->comma_g == 103)
					goto INVALID_FORMAT;
				/* FALLTHROUGH */
			case 'd':
				/* FALLTHROUGH */
			case 'D':
				/* d/D and , are mutually exclusive. */
				if (fmts->period_d != 255
						|| (*step != '.' && fmts->comma_g == 174))
					goto INVALID_FORMAT;
				fmts->period_d = step - format;
				++fmts->n_elements;
				has_dp = true;
				break;

			/*
			 * There must be only one Currency Element at most.
			 */
			case '$':
				/* FALLTHROUGH */
			case 'c':
				/* FALLTHROUGH */
			case 'C':
				/* FALLTHROUGH */
			case 'l':
				/* FALLTHROUGH */
			case 'L':
				/* FALLTHROUGH */
			case 'u':
				/* FALLTHROUGH */
			case 'U':
				if (fmts->dollar_clu != 255)
					goto INVALID_FORMAT;
				fmts->dollar_clu = step - format;
				if (*step == '$')
					break;
				/* C/L/U could be regarded as D but $ could not */
				if (fmts->dollar_clu > fmts->first_digit
						&& (strchr(step, '0') || strchr(step, '9')
							|| strchr(step, 'e') || strchr(step, 'E')))
				{
					if (fmts->period_d != 255 || fmts->comma_g == 174)
						goto INVALID_FORMAT;
					fmts->period_d = fmts->dollar_clu;
					has_dp = true;
				}
				++fmts->n_elements;
				break;

			/*
			 * Scientific notation
			 */
			case 'e':
				/* FALLTHROUGH */
			case 'E':
				/*
				 * eeee/EEEE + PR/MI/S is valid
				 * g/G is valid but , is invalid.
				 */
				if (fmts->comma_g == 174
						|| (strncmp(step, "eeee", 4) != 0
							&& strncmp(step, "EEEE", 4) != 0))
					goto INVALID_FORMAT;
				if (pg_strcasecmp(step, "eeee") != 0
						&& pg_strcasecmp(step, "eeeepr") != 0
						&& pg_strcasecmp(step, "eeeemi") != 0
						&& pg_strcasecmp(step, "eeees") != 0)
					goto INVALID_FORMAT;
				fmts->eeee = step - format;
				++fmts->n_elements;
				step += 3;
				break;

			/*
			 * Group Separator
			 */
			case ',':
				if (has_dp || step == format || n_comma == 39
						|| (fmts->miprs == 0 && step - format == 1)
						|| fmts->comma_g == 71 || fmts->comma_g == 103)
					goto INVALID_FORMAT;
				if (fmts->comma_g == 255)
					fmts->comma_g = 174;
				++n_comma;
				++fmts->n_elements;
				break;
			case 'g':
				/* FALLTHROUGH */
			case 'G':
				if (has_dp || step == format || n_comma == 39
						|| (fmts->miprs == 0 && step - format == 1)
						|| fmts->comma_g == 174)
					goto INVALID_FORMAT;
				if (fmts->comma_g == 255)
					fmts->comma_g = *step == 'g' ? 103 : 71;
				++n_comma;
				++fmts->n_elements;
				break;

			/*
			 * positive or negative
			 */
			case 'm':
				/* FALLTHROUGH */
			case 'M':
				if (fmts->miprs != 255 || pg_strcasecmp(step, "mi") != 0)
					goto INVALID_FORMAT;
				fmts->miprs = step++ - format;
				++fmts->n_elements;
				break;
			case 'p':
				/* FALLTHROUGH */
			case 'P':
				if (fmts->miprs != 255 || pg_strcasecmp(step, "pr") != 0)
					goto INVALID_FORMAT;
				fmts->miprs = step++ - format;
				++fmts->n_elements;
				break;
			case 's':
				/* FALLTHROUGH */
			case 'S':
				if (fmts->miprs != 255 || (step != format && *(step + 1) != '\0'))
					goto INVALID_FORMAT;
				fmts->miprs = step - format;
				++fmts->n_elements;
				break;

			/*
			 * hexadecimal
			 */
			case 'x':
				/* FALLTHROUGH */
			case 'X':
				/* only '0' before 'x'/'X' is valid */
				if (step > format && *(step - 1) != '0')
					goto INVALID_FORMAT;
				/*
				 * sanity check
				 * [0]*[xX]+ | [sS]*[09]*[0]+[xX]+
				 */
				bystep = *format == 's' || *format == 'S' ? format + 1 : format;
				for (; *bystep == '0' || *bystep == '9'; ++bystep);
				for (; *bystep == 'x' || *bystep == 'X'; ++bystep);
				if (*bystep != '\0')
					goto INVALID_FORMAT;
				else
					return false;
				++fmts->n_elements;
				break;

			/*
			 * invalid number Format Model Element
			 */
			default:
INVALID_FORMAT:
				ereport(ERROR, (errmsg("invalid number format model")));
				break;
		}
	}

	return true;
}

/*
 * ora_to_number_hex_check
 *     check the hexadecimal string's validation
 */
static bool
ora_to_number_hex_check(char *format, char **expression, NLSParams *nls)
{
	char   *copy,
		   *ptr;
	int		n_digits = 0;

	/*
	 * Make a copy of "expr" without leading space,
	 * and record the number of hexadecimal digits in it.
	 * There could be group separator, which will be ignored.
	 */
	copy = (char *) palloc0(strlen(*expression) + 1);
	for (ptr = *expression; *ptr == ' '; ++ptr);
	if (*ptr == '+' || *ptr == '-')
		*(copy + n_digits++) = *(ptr++);
	for (; *ptr != '\0'; ++ptr)
	{
		if (*ptr == nls->numeric_characters[1]
				&& !(('a' <= nls->numeric_characters[1]
						&& nls->numeric_characters[1] <= 'f')
					|| ('A' <= nls->numeric_characters[1]
						&& nls->numeric_characters[1] <= 'F')))
			continue;
		else if (('0' <= *ptr && *ptr <= '9')
				|| ('a' <= *ptr && *ptr <= 'f')
				|| ('A' <= *ptr && *ptr <= 'F'))
			*(copy + n_digits++) = *ptr;
		else
			return false;
	}
	if (*copy == '+' || *copy == '-')
		--n_digits;

	/*
	 * There must be a leading 'S' when there's a leading '+' in "expr".
	 * There must be a leading '+'/'-' when there's a leading 'S' in "fmt".
	 */
	if ((*copy == '+' && *format != 's' && *format != 'S')
			|| ((*format == 's' || *format == 'S')
				&& *copy != '+' && *copy != '-'))
	{
		pfree(copy);
		return false;
	}

	/*
	 * There could be more leading 'X' in "fmt", but not more leading '0'.
	 * "ptr" may be the first 0 or the end.
	 */
	format = *format == 's' || *format == 'S' ? format + 1 : format;
	for (ptr = format; *ptr != '0' && *ptr != '\0'; ++ptr);
	if ((*ptr == '0' && strlen(ptr) != n_digits)
			|| (*ptr == '\0' && strlen(format) < n_digits))
	{
		pfree(copy);
		return false;
	}

	/*
	 * leading '-' needs to be processed specially
	 */
	memset(*expression, 0, strlen(*expression));
	if (*copy == '+' || *copy == '-')
	{
		if (n_digits < 3)
		{
			pfree(copy);
			return true;
		}
		else if (n_digits % 3 == 0)
		{
			if (*copy == '+')
				memcpy(*expression, "FFFFB", 5);
			else
				memcpy(*expression, "FFFFD", 5);
			memcpy(*expression + 5, copy + 1, n_digits);
		}
		else if (n_digits % 3 == 1)
		{
			if (*copy == '+')
				memcpy(*expression, "FFFB", 4);
			else
				memcpy(*expression, "FFFD", 4);
			memcpy(*expression + 4, copy + 1, n_digits);
		}
		else if (n_digits % 3 == 2)
		{
			if (*copy == '+')
				memcpy(*expression, "FFB", 3);
			else
				memcpy(*expression, "FFD", 3);
			memcpy(*expression + 3, copy + 1, n_digits);
		}
	}
	else
		memcpy(*expression, copy, n_digits);

	pfree(copy);
	return true;
}

/*
 * ora_hex_to_dec_calc
 *     convert hexadecimal to decimal
 */
static Numeric
ora_hex_to_dec_calc(char *hexadecimal)
{
	Datum	result = 0,
			power = 0,
			multiple = 0;
	char   *decstr,
		   *expstr,
		   *ptr;
	char	hexbase[3] = "16\0";
	int		length,
			exponent;

	if (*hexadecimal == '\0')
		return NULL;

	for (ptr = hexadecimal; *ptr == ' '; ++ptr);
	length = strlen(ptr);
	/* Convert "hexadecimal" to decimal from hexadecimal. */
	for (exponent = length - 1; *ptr != '\0'; ++ptr, --exponent)
	{
		/* Convert radix of each character. */
		decstr = (char *) palloc0(3);
		if (*ptr >= 'A' && *ptr <= 'F')
			sprintf(decstr, "%d", *ptr - 'A' + 10);
		else if (*ptr >= 'a' && *ptr <= 'f')
			sprintf(decstr, "%d", *ptr - 'a' + 10);
		else
			sprintf(decstr, "%c", *ptr);

		/* String of exponent. */
		expstr = (char *) palloc0(63);
		sprintf(expstr, "%d", exponent);

		/* Expression : "power" = 16 ^ exponent */
		power = DirectFunctionCall2(numeric_power,
									DirectFunctionCall3(
												numeric_in,
												CStringGetDatum(hexbase),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum(131076)),
									DirectFunctionCall3(
												numeric_in,
												CStringGetDatum(expstr),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum(-1)));

		/* Conversion which could clear decimal part of "power". */
		power = DirectFunctionCall1(numeric_float8, power);
		power = DirectFunctionCall1(float8_numeric, power);

		/* Expression : "multiple" = "power" * "dec" */
		multiple = DirectFunctionCall2(
						numeric_mul,
						power,
						DirectFunctionCall3(
										numeric_in,
										CStringGetDatum(decstr),
										ObjectIdGetDatum(InvalidOid),
										Int32GetDatum((2 << 16) + VARHDRSZ)));

		result = ptr == hexadecimal ? multiple
									: DirectFunctionCall2(numeric_add,
													NumericGetDatum(result),
													NumericGetDatum(multiple));

		pfree(expstr);
		pfree(decstr);
	}

	return DatumGetNumeric(result);
}

/*
 * ora_to_number_expr_check
 *     check the format of "expr"
 */
static bool
ora_to_number_expr_check(char *format, char **expression,
						 FormatElems *fmts, NLSParams *nls)
{
	char	sign = 0,					/* Positive / Negative Sign */
			group_separator,
		   *str_currency = NULL,
		   *step = NULL,
		   *bystep = NULL;
	int		length;

	/* Skip leading space. */
	length = strlen(*expression);
	for (step = *expression; *step == ' '; ++step);
	if (step > *expression && *step == '\0')
		return false;

	/* MI, PR, S */
	if (fmts->miprs != 255)
	{
		switch (format[fmts->miprs])
		{
			/* MI */
			case 'm':
				/* FALLTHROUGH */
			case 'M':
				/*
				 * select to_number('1$23-', '9L99MI') from dual;
				 * ORA-10722: invalid number
				 */
				if (fmts->dollar_clu != 0 && fmts->dollar_clu != 255
						&& format[fmts->dollar_clu] != '$'
						&& fmts->dollar_clu == fmts->period_d)
					return false;

				/*
				 * select to_number('123 ', '999') from dual; => 123
				 * select to_number('123+', '999') from dual; => 123
				 * select to_number('123-', '999') from dual; => -123
				 *
				 * Otherwise, ORA-01722: invalid number
				 */
				if (isdigit((unsigned char) (*expression)[length - 1]))
					return false;
				else
				{
					if ((*expression)[length - 1] == '+'
							|| (*expression)[length - 1] == '-')
						sign = (*expression)[length - 1];
					else
						sign = '+';
					(*expression)[--length] = '\0';
				}
				break;
			/* PR */
			case 'p':
				/* FALLTHROUGH */
			case 'P':
				/*
				 * select to_number('1$23', '9L99PR'); => 1.23
				 * select to_number('<1$23', '9L99PR'); => -1.23
				 *
				 * select to_number('<1$23>', '9L99PR');
				 * => ORA-01722: invalid number
				 */
				if ((*expression)[length - 1] == '>'
						&& fmts->dollar_clu != 255
						&& format[fmts->dollar_clu] != '$'
						&& fmts->dollar_clu == fmts->period_d)
					return false;

				/*
				 * select to_number('<123', '999PR') from dual; => -123
				 * select to_number('<123>', '999PR') from dual; => -123
				 *
				 * '<' is necessary for a negative value.
				 * It's wrong when there's only '>'.
				 */
				if (*step == '<')
				{
					sign = '-';
					*(step++) = ' ';
					if ((*expression)[length - 1] == '>')
						(*expression)[--length] = '\0';
				}
				else if (*step != '<' && (*expression)[length - 1] == '>')
					return false;
				break;
			/* S */
			case 's':
				/* FALLTHROUGH */
			case 'S':
				/* leading S */
				if (fmts->miprs == 0)
				{
					if (*step != '+' && *step != '-')
						return false;
					else
					{
						sign = *step;
						*(step++) = ' ';
					}
				}
				/* trailing S */
				else if (format[fmts->miprs + 1] == '\0')
				{
					/*
					 * select to_number('1$23+', '9L99S') from dual;
					 * select to_number('1$23-', '9L99S') from dual;
					 *
					 * ORA-01722: invalid number
					 */
					if (fmts->dollar_clu != 0 && fmts->dollar_clu != 255
							&& format[fmts->dollar_clu] != '$'
							&& fmts->dollar_clu == fmts->period_d)
						return false;

					/* Only '+'/'-' could match with 'S'. */
					if ((*expression)[length - 1] != '+'
							&& (*expression)[length - 1] != '-')
						return false;
					else
					{
						sign = (*expression)[--length];
						(*expression)[length] = '\0';
					}
				}
				break;
		}
	}
	else if (*step == '-')
	{
		sign = *step;
		*(step++) = ' ';
	}

	if (fmts->dollar_clu != 255)
	{
		switch (format[fmts->dollar_clu])
		{
			case '$':
				/* leading $ */
				if (*step != '$')
					return false;
				else
					*(step++) = ' ';
				break;
			/* ISO 4217 - Currency Codes */
			case 'c':
				/* FALLTHROUGH */
			case 'C':
				str_currency = !str_currency ? nls->iso_currency : str_currency;
				/* FALLTHROUGH */
			/* Any valid character string, with a maximum of 10 bytes */
			case 'l':
				/* FALLTHROUGH */
			case 'L':
				str_currency = !str_currency ? nls->currency : str_currency;
				/* FALLTHROUGH */
			/* Any valid format name up to 10 characters */
			case 'u':
				/* FALLTHROUGH */
			case 'U':
				str_currency = !str_currency ? nls->dual_currency : str_currency;

				if (strlen(step) < strlen(str_currency))
					return false;

				/*
				 * Leading C/L/U
				 *
				 * e.g.
				 *     alter session set nls_currency = ABC;
				 *     select to_number('abc123', 'L999') from dual; => 123
				 *     select to_number('ABC123', 'L999') from dual; => 123
				 * It's case INSENSITIVE.
				 */
				if (fmts->dollar_clu == 0
						|| (fmts->miprs == 0 && fmts->dollar_clu == 1))
				{
					if (pg_strncasecmp(step, str_currency, strlen(str_currency)))
						return false;
					memset(step, 32, strlen(str_currency));
					step += strlen(str_currency);
				}
				/*
				 * non-leading C/L/U ( [09][CLU] )
				 *
				 * e.g.
				 *     alter session set nls_currency = ABC;
				 *     select to_number('1abc23', '9L99') from dual;
				 *                                    => ERROR: invalid number
				 *     select to_number('1ABC23', '9L99') from dual; => 1.23
				 *     select to_number('123abc', '999L') from dual;
				 *                                    => ERROR: invalid number
				 *     select to_number('123ABC', '999L') from dual; => 123
				 * It's case SENSITIVE.
				 */
				else if (fmts->dollar_clu == fmts->n_elements - 1
						|| (fmts->dollar_clu == fmts->n_elements - 2
							&& fmts->miprs == fmts->n_elements - 1))
				{
					if (isdigit((unsigned char) *str_currency)
							|| strncmp(step + strlen(step) - strlen(str_currency),
									   str_currency, strlen(str_currency)) != 0)
						return false;
					memset(step + strlen(step) - strlen(str_currency), 0,
						   strlen(str_currency));
				}
				else
				{
					char   *copy;

					/* Find the string of NLS[_/_ISO_/_DUAL_]CURRENCY. */
					bystep = strstr(step, str_currency);
					if (bystep == NULL || isdigit((unsigned char) *str_currency))
						return false;

					/* Make it a Decimal Character. */
					copy = (char *) palloc0(strlen(bystep) + 2
													- strlen(str_currency));
					/* XXX DO NOT SUPPORT REPLACE '.' TO OTHER CHARACTERS NOW. */
					copy[0] = '.';
					memcpy(copy + 1, bystep + strlen(str_currency),
						   strlen(bystep) - strlen(str_currency));
					memset(bystep, 0, strlen(bystep));
					memcpy(bystep, copy, strlen(copy));
					pfree(copy);
					copy = NULL;
				}
				break;
		}
	}

	group_separator = fmts->comma_g == 174 ? ',' : nls->numeric_characters[1];
	/* Digits check */
	if (!ora_to_number_digits_check(format, step, group_separator, fmts, nls))
		return false;

	/* Group Separator check */
	if (!ora_to_number_comma_match(format, step, group_separator, fmts))
		return false;
	else
	{
		char   *copy = (char *) palloc0(length + 1);

		for (bystep = step; *bystep != '\0'; ++bystep)
		{
			if (*bystep != group_separator)
				copy[strlen(copy)] = *bystep;
			else if (*bystep == 'e' || *bystep == 'E')
			{
				memcpy(copy + strlen(copy), bystep, strlen(bystep));
				bystep += strlen(bystep) - 1;
			}
		}
		memset(step, 0, strlen(step));
		memcpy(step, copy, strlen(copy) + 1);
		pfree(copy);
		copy = NULL;
	}

	/* Add "sign" to "*expression" saved before. */
	if (sign != 0)
	{
		char   *newval = palloc0(length + 2);

		newval[0] = sign;
		memcpy(newval + 1, step, strlen(step));
		memset(*expression, 0, length);
		memcpy(*expression, newval, strlen(newval));
		pfree(newval);
	}

	return true;
}

/*
 * ora_to_number_digits_check
 *     the number of digits and characters are valid or not
 */
static bool
ora_to_number_digits_check(char *format, char *digits, char character,
						   FormatElems *fmts, NLSParams *nls)
{
	char   *ptr = NULL;
	int		n_integer = 0,
			n_decimal = 0;
	bool	has_dp = false;

	/*
	 * Number of digits
	 */
	for (ptr = digits; *ptr != '\0'; ++ptr)
	{
		if (*ptr == 'e' || *ptr == 'E')
		{
			if (fmts->eeee != 255)
				break;
			else
				return false;
		}
		/*
		 * '.'/'d'/'D' in "fmt"
		 */
		else if (!has_dp)
		{
			/*
			 * +/- is removed, and so '$' or other string of currnecy is.
			 * So that there're digits, decimal character and group separator.
			 *
			 * decimal character could be '.' or other character defined by
			 * nls->numeric_characters[0].
			 */
			if (fmts->period_d != 255
					&& ((*ptr == '.' && format[fmts->period_d] == '.')
						|| (*ptr == nls->numeric_characters[0]
							&& format[fmts->period_d] != '.')))
			{
				has_dp = true;
				*ptr = '.';
			}
			else if (isdigit((unsigned char) *ptr))
				++n_integer;
			else if (*ptr != character)
				return false;
		}
		else if (isdigit((unsigned char) *ptr))
			++n_decimal;
		else
			return false;
	}

	if (n_integer > fmts->n_integer || n_decimal > fmts->n_decimal
			|| (fmts->eeee != 255 && n_integer > 1))
		return false;

	/* The extra '0' in front of "fmt" is invalid. */
	if (n_integer < fmts->n_integer)
	{
		/* The number of extra digits in front of "fmt". */
		int	extra = fmts->n_integer - n_integer;

		for (ptr = format; extra > 0 && *ptr != '\0'; ++ptr)
		{
			if (isdigit((unsigned char) *ptr) && *ptr == '9')
				--extra;
			else if (isdigit((unsigned char) *ptr) && *ptr == '0')
				return false;
		}
	}

	return true;
}

/*
 * ora_to_number_comma_match
 *     group separators are at the correct position or not
 */
static bool
ora_to_number_comma_match(char *format, char *expression, char character,
						  FormatElems *fmts)
{
	char   *ptr,
		   *step = NULL,
		   *bystep = NULL;
	int		fmtgroup[40] = {0},
			exprgroup[512] = {0},
			n_fmtgroups = 0,
			n_exprgroups = 0,
			i = 0,
			j = 0;

	/*
	 * Do nothing when there's no group separator in the "fmt".
	 *
	 * And if the position of the decimal point is 0, then there's also none
	 * group separator in the "fmt".
	 */
	if (fmts->comma_g == 255)
		return true;

	/* The first digit in integer part of format. */
	bystep = (fmts->period_d != 255) ? format + fmts->period_d - 1
									 : format + strlen(format) - 1;
	/* The first digit in integer part of expression. */
	step = strchr(expression, '.');
	step = step ? step - 1 : expression + strlen(expression) - 1;

	/*
	 * record the number of digits in each group from right to left
	 */
	for (ptr = bystep; ptr >= format; --ptr)
	{
		if (*ptr == '0' || *ptr == '9')
			++fmtgroup[n_fmtgroups];
		else if (*ptr == 'g' || *ptr == 'G' || *ptr == ',')
			++n_fmtgroups;
		else if (ptr == format)
			break;
	}
	for (ptr = step; ptr >= expression; --ptr)
	{
		if (isdigit((unsigned char) *ptr))
			++exprgroup[n_exprgroups];
		else if (*ptr == character)
			++n_exprgroups;
		else if (ptr == expression)
			break;
	}

	/*
	 * fmtgroup[0] could be equal to exprgroup[0] + exprgroup[x] (x >= 0),
	 * and then fmtgroup[1...] must be equal to exprgroup[x+1...] respectively.
	 */
	for (j = 0; j <= n_exprgroups; ++j)
	{
		if (fmtgroup[0] < exprgroup[j])
			return false;
		else if (fmtgroup[0] > exprgroup[j])
			fmtgroup[0] -= exprgroup[j];
		else
		{
			int		n_skip = 0;

			for (i = i + 1; i <= n_fmtgroups && fmtgroup[i] == 0; ++i, ++n_skip);
			for (j = j + 1; j <= n_exprgroups && exprgroup[j] == 0; ++j, --n_skip);

			if (n_skip > 0)
				return false;
			else
				break;
		}
	}

	if (n_exprgroups - j > n_fmtgroups - i)
		return false;
	else
	{
		for (; i <= n_fmtgroups && j <= n_exprgroups; ++i, ++j)
		{
			if (i == n_fmtgroups && exprgroup[j] > fmtgroup[i])
				return false;
			if (i != n_fmtgroups && exprgroup[j] != fmtgroup[i])
				return false;
		}
	}

	return true;
}

/*
 * string_to_numeric
 *     convert to numeric datatype
 */
static Numeric
string_to_numeric(char *str, bool *has_error)
{
	Numeric	result;
	int		precision = 0,
			scale = 0;
	long	exponent = 0;
	bool	has_dp = false;
	char   *ptr = NULL;

	if (*str == '\0')
		return NULL;

	/* Skip leading space. */
	for (ptr = str; *ptr == ' '; ++ptr);
	ptr = (*ptr == '+' || *ptr == '-') ? ptr + 1 : ptr;

	if (*ptr == '.')
	{
		has_dp = true;
		++ptr;
	}

	if (!isdigit((unsigned char) *ptr))
	{
		*has_error = true;
		return NULL;
	}

	while (*ptr)
	{
		if (isdigit((unsigned char) *ptr))
		{
			if (!has_dp)
				++precision;
			else
				++scale;
		}
		else if (*ptr == '.')
		{
			if (has_dp)
			{
				*has_error = true;
				return NULL;
			}
			has_dp = true;
		}
		else
			break;
		++ptr;
	}

	/* If there's exponent. */
	if (*ptr == 'e' || *ptr == 'E')
	{
		char   *endptr;

		++ptr;
		exponent = strtol(ptr, &endptr, 10);
		if (endptr == ptr)
		{
			*has_error = true;
			return NULL;
		}
		ptr = endptr;
	}

	/* Maybe there's trailing space */
	for (; *ptr == ' '; ++ptr);
	if (*ptr != '\0')
	{
		*has_error = true;
		return NULL;
	}

	precision = exponent < 0 ? precision + scale - exponent
							 : precision + (exponent > scale ? exponent : scale);
	scale = exponent > 0 && exponent > scale ? 0 : scale - exponent;

	result = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												 CStringGetDatum(str),
												 ObjectIdGetDatum(InvalidOid),
						Int32GetDatum(((precision << 16) | scale) + VARHDRSZ)));

	return result;
}
/* End - ReqID:SRS-SQL-SYSFUNC */
