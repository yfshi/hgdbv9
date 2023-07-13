/*-------------------------------------------------------------------------
 *
 * NUMBER-processing utility routines for frontend code (Oracle Compatibility)
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/oracle_fe_utils/ora_number_utils.c
 *
 * add the file for compatible requirement "TO_NUMBER()"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include <ctype.h>

#include "common/logging.h"
#include "fe_utils/psqlscan_int.h"
#include "oracle_fe_utils/ora_number_utils.h"

#define NUMBEROID 9020

/* GUC parameter */
static char	decimal_point;
/* format parameter */
static int numwidth;
static int explen;

static char * sci_notation(char *value);
static void get_numeric_symbol(const PGconn *conn);
static void calc_exp_len(int exponent);
static char * out_of_numwidth(bool is_negative, int exponent);
static char * round_sigs(char *sigs, bool is_negative, int *exponent, int dp_offset);
static char * format_number(bool is_negative, char *sigs, int exponent);

/*
 * format each number datatype field in results
 */
bool
ora_numwidth_format(PGconn *conn, PGresult **results)
{
	int		ntuples = PQntuples(*results),
			nfields = PQnfields(*results),
			row, col;

	/* set numwidth -1 to disable the format */
	numwidth = GetSysVariable("NUMWIDTH")->value.ival;
	if (-1 == numwidth)
		return true;

	/* no NUMBER datatype column exists */
	for (col = 0; col < nfields; ++col)
		if (NUMBEROID == PQftype(*results, col))
			break;
	if (col == nfields)
		return true;

	get_numeric_symbol(conn);

	for (; col < nfields; ++col)
	{
		if (NUMBEROID != PQftype(*results, col))
			continue;

		for (row = 0; row < ntuples; ++row)
		{
			char	*value = PQgetvalue(*results, row, col),
					*newval = sci_notation(value);

			if (PQsetvalue(*results, row, col, newval, numwidth))
			{
				if (newval != value)
					pg_free(newval);
			}
			else
			{
				if (newval != value)
					pg_free(newval);
				return false;
			}
		}
	}

	return true;
}

/*
 * get decimal point character set by NLS_NUMERIC_CHARACTERS
 */
static void
get_numeric_symbol(const PGconn *conn)
{
	PGresult	*res;

	res = PQexec((PGconn *) conn, "select setting from pg_settings "
								  "where name = 'nls_numeric_characters';");
	if (PGRES_TUPLES_OK == PQresultStatus(res))
		decimal_point = PQgetvalue(res, 0, 0)[0];
	else
		decimal_point = '.';

	if (res)
		PQclear(res);
}

/*
 * value should be converted to scientific notation
 * when its length more than numwidth
 */
static char *
sci_notation(char *value)
{
	char   *ret,
		   *sigs;
	int		dp_offset,
			first_sig,
			exponent;
	bool	is_negative;

	/*
	 * the position of the decimal point
	 *             and the first significant figure
	 */
	dp_offset = strspn(value, "0123456789+-");
	first_sig = strspn(value, "0+-.");

	/*
	 * dp_offset == first_sig == strlen(value) => "" or "0"
	 * none significant figure found
	 */
	if (dp_offset == first_sig)
		return strlen(value) == 0 ? NULL : pg_strdup(value);

	/* exponent while it looks like \d.\d+E[+-]\d{2,} */
	exponent = dp_offset > first_sig ? dp_offset - first_sig - 1
									 : dp_offset - first_sig;
	calc_exp_len(exponent);
	is_negative = value[0] == '-' ? true : false;

	/* is numwidth enough or not */
	ret = out_of_numwidth(is_negative, exponent);
	if (ret)
		return ret;

	/*
	 * make the string of significant figures
	 */
	sigs = pg_malloc0(strlen(value) + 1);
	if (exponent < 0)
		memcpy(sigs, value + first_sig, strlen(value) - first_sig);
	else
	{
		/* decimal point's left */
		memcpy(sigs, value + first_sig, dp_offset - first_sig);
		/* decimal point's right (if exists) */
		if (dp_offset != strlen(value))
			memcpy(sigs + strlen(sigs), value + dp_offset + 1,
				   strlen(value) - dp_offset - 1);
	}

	/*
	 * round at the specific position if necessary
	 */
	if (strlen(value) > numwidth)
	{
		sigs = round_sigs(sigs, is_negative, &exponent, dp_offset);

		/* is numwidth enough or not after round */
		ret = out_of_numwidth(is_negative, exponent);
		if (ret)
			return ret;
	}

	/*
	 * format digits
	 */
	ret = format_number(is_negative, sigs, exponent);
	pg_free(sigs);

	return ret;
}

static void
calc_exp_len(int exponent)
{
	char   *expstr = pg_malloc0(9);

	if (exponent > 0)
		sprintf(expstr, "E+%02d", exponent);
	else
		sprintf(expstr, "E%03d", exponent);

	explen = strlen(expstr);
	pg_free(expstr);
}

static char *
out_of_numwidth(bool is_negative, int exponent)
{
	/* non scientific notation */
	if ((exponent < 0 && (is_negative ? 1 : 0) + 1 - exponent <= numwidth) ||
			(exponent >= 0 && (is_negative ? 1 : 0) + 1 + exponent <= numwidth))
		return NULL;

	/* scientific notation needs at least (is_negative ? 1 : 0) + 3 + explen */
	if (numwidth < (is_negative ? 1 : 0) + 3 + explen)
	{
		char	*ret = pg_malloc0(numwidth + 1);

		if ((-130 <= exponent && exponent <= -100) || exponent > 0)
			memset(ret, '#', numwidth);
		else
			memset(ret, '0', 1);

		return ret;
	}

	return NULL;
}

static char *
round_sigs(char *sigs, bool is_negative, int *exponent, int dp_offset)
{
	bool	round;
	int		pos,
			i;

RE_ROUND:
	/* the position should be rounded */
	pos = numwidth - (is_negative ? 1 : 0);

	if (*exponent >= pos)
		pos -= (1 + explen);
	else if (*exponent < 0)
		pos += *exponent;
	else
		pos -= (dp_offset < numwidth ? 1 : 0);

	/* at least numwidth - (is_negative ? 1 : 0) - 1(decimal point) - explen */
	if (pos < numwidth - (is_negative ? 1 : 0) - 1 - explen)
		pos = numwidth - (is_negative ? 1 : 0) - 1 - explen;

	/* appending 0 */
	if (strlen(sigs) <= pos)
	{
		sigs = pg_realloc(sigs, pos + 1);
		memset(sigs + strlen(sigs), '0', pos - strlen(sigs));
	}

	/* round or not */
	round = '5' <= sigs[pos] && sigs[pos] <= '9' ? true : false;
	for (i = pos - 1; round && i >= 0; --i)
	{
		round = sigs[i] == '9' ? true : false;
		sigs[i] = sigs[i] == '9' ? '0' : sigs[i] + 1;
	}

	/* 9{pos,}+ */
	if (round)
	{
		memset(sigs + pos, (*exponent < 0 ? 0 : '0'), strlen(sigs) - pos);
		sigs[0] = '1';
		++(*exponent);
		++dp_offset;
		if ((is_negative ? 1 : 0) + strlen(sigs) + 1 + explen > numwidth)
			goto RE_ROUND;
	}
	else
		memset(sigs + pos, 0, strlen(sigs) - pos);

	return sigs;
}

static char *
format_number(bool is_negative, char *sigs, int exponent)
{
	char	*newval;

	newval = pg_malloc0(numwidth + 2);
	newval[0] = is_negative ? '-' : 0;

	/* scientific notation */
	if (exponent >= numwidth - (is_negative ? 1 : 0))
		sprintf(newval, "%s%c%c%sE+%02d",
						newval, sigs[0], decimal_point, sigs+1, exponent);
	/* because of the length of ".0000" is equal to ".E-05" */
	else if (exponent < -5
			&& strlen(sigs) + 1 - exponent + (is_negative ? 1 : 0) > numwidth)
		sprintf(newval, "%s%c%c%sE%03d",
						newval, sigs[0], decimal_point, sigs+1, exponent);
	else
	{
		if (exponent >= 0)
		{
			int	i;

			for (i = 0; i <= exponent; ++i)
				newval[strlen(newval)] = sigs[i];
			if (i != strlen(sigs))
				newval[strlen(newval)] = decimal_point;
			for (; i < strlen(sigs); ++i)
				newval[strlen(newval)] = sigs[i];
		}
		else
		{
			int	i;

			newval[strlen(newval)] = decimal_point;
			for (i = exponent; i < -1; ++i)
				newval[strlen(newval)] = '0';
			memcpy(newval + strlen(newval), sigs, strlen(sigs));
		}
	}

	return newval;
}
