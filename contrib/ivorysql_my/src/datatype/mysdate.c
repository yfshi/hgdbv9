/*------------------------------------------------------------------
 * 
 * File: mydate.c
 *
 * Abstract: 
 * 		Compatible with MySQL's DATE data type.
 *
 * Authored by jinhuajian@highgo.com, 20230807.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysdate.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>

#include "access/hash.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/scansup.h"
#include "pgtime.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "utils/lsyscache.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"

/* Begin - ReqID:SRS-MYSQL-DATETYPE */
PG_FUNCTION_INFO_V1(mysdate_in);
PG_FUNCTION_INFO_V1(mysdate_out);
PG_FUNCTION_INFO_V1(mysdate_recv);
PG_FUNCTION_INFO_V1(mysdate_send);
PG_FUNCTION_INFO_V1(int_mysdate);
PG_FUNCTION_INFO_V1(numeric_mysdate);
PG_FUNCTION_INFO_V1(mysdate_eq);
PG_FUNCTION_INFO_V1(mysdate_ne);
PG_FUNCTION_INFO_V1(mysdate_lt);
PG_FUNCTION_INFO_V1(mysdate_gt);
PG_FUNCTION_INFO_V1(mysdate_le);
PG_FUNCTION_INFO_V1(mysdate_ge);
PG_FUNCTION_INFO_V1(mysdate_seq);
PG_FUNCTION_INFO_V1(mysdate_seqstr);
PG_FUNCTION_INFO_V1(mysdate_adddate);
PG_FUNCTION_INFO_V1(mysdate_subdate);
PG_FUNCTION_INFO_V1(mysdate_cmp);
PG_FUNCTION_INFO_V1(mysdate_hash);
PG_FUNCTION_INFO_V1(mys_date_hash_extended);

PG_FUNCTION_INFO_V1(mysdate_muldate);
PG_FUNCTION_INFO_V1(mysdate_divdate);
PG_FUNCTION_INFO_V1(mysdate_exact_divdate);
PG_FUNCTION_INFO_V1(mysdate_moddate);
PG_FUNCTION_INFO_V1(mysdate_bitanddate);
PG_FUNCTION_INFO_V1(mysdate_bitordate);
PG_FUNCTION_INFO_V1(mysdate_negadate);
PG_FUNCTION_INFO_V1(mysdate_bitxordate);
PG_FUNCTION_INFO_V1(mysdate_rightmovdate);
PG_FUNCTION_INFO_V1(mysdate_leftmovdate);
PG_FUNCTION_INFO_V1(mysdate_bit_count);
PG_FUNCTION_INFO_V1(mysdate_anddate);
PG_FUNCTION_INFO_V1(mysdate_invertmysdate);
PG_FUNCTION_INFO_V1(mysdate_boolean);

PG_FUNCTION_INFO_V1(mysdate2_like);
PG_FUNCTION_INFO_V1(mysdate_text_like);
PG_FUNCTION_INFO_V1(mysdate_int8_like);
PG_FUNCTION_INFO_V1(mysdate2_notlike);
PG_FUNCTION_INFO_V1(mysdate_text_notlike);
PG_FUNCTION_INFO_V1(mysdate_int8_notlike);

static int msec_round_add[7] = {
	500000000,
	50000000,
	5000000,
	500000,
	50000,
	5000,
	0
};

static int64 log_10_int[20] = {
	1, 10, 100, 1000, 10000UL, 100000UL, 1000000UL, 10000000UL,
	100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL,
	1000000000000ULL, 10000000000000ULL, 100000000000000ULL,
	1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL,
	1000000000000000000ULL, 10000000000000000000ULL
};

static int
getCount(uint64 num)
{
	int count = 0;
	while (num)
	{
		count += num & 1;
		num >>= 1;
	}
	return count;
}

static bool
isValidDatetimeFormat(const char *datetime) 
{
	if (strlen(datetime) > 26)
	{
		return false;
	}

	if (datetime[4] != '-' || datetime[7] != '-' ||
		datetime[13] != ':' || datetime[16] != ':' )
	{
		return false;
	}

	return true;
}

PG_FUNCTION_INFO_V1(mysdate_int8);
PG_FUNCTION_INFO_V1(mysdate_numeric);
Datum
mysdate_int8(PG_FUNCTION_ARGS)
{
	TimeADT	dateVal1 = PG_GETARG_TIMEADT(0);

	if (dateVal1 == MYSQL_ZERO_DATE)
	{
		dateVal1 = 0;
	}

	PG_RETURN_INT64(dateVal1);
}

Datum mysdate_numeric(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if (dateVal1 == MYSQL_ZERO_DATE)
	{
		dateVal1 = 0;
	}
	return NumericGetDatum(int64_to_numeric(dateVal1));
}
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/
/* 
 * mysdate_in
 * Given date text string, convert to internal date format.
 */
Datum
mysdate_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	DateADT		date = MYSQL_NULL_DATE;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	int			fsec_len = 0;

	if(has_sql_mode( MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		date = MYSQL_ZERO_DATE;
	}
	else
	{
		date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	}

	PG_RETURN_DATEADT(date);
}

/* 
 * mysdate_out
 * Given internal format date, convert to text string.
 */
Datum
mysdate_out(PG_FUNCTION_ARGS)
{
	DateADT		date = PG_GETARG_DATEADT(0);
	char	   *result = NULL;
	struct pg_tm tt,
			   *tm = &tt;
	char		buf[MAXDATELEN + 1] ={0};

	if(date == MYSQL_NULL_DATE)
		PG_RETURN_NULL();

	if(date == MYSQL_ZERO_DATE)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00");
	}
	else
	{
		int64	part1, part2;

		part1 = (date / 10000);
		part2 = (date - part1 * 10000);
		tm->tm_year = part1;

		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;
		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d",tm->tm_year,tm->tm_mon,tm->tm_mday);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mysdate_recv			- converts external binary format to date
 */
Datum
mysdate_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	DateADT		result;

	result = (DateADT) pq_getmsgint(buf, sizeof(DateADT));

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("date out of range")));

	PG_RETURN_DATEADT(result);
}

/*
 *mysdate_send			- converts date to binary format
 */
Datum
mysdate_send(PG_FUNCTION_ARGS)
{
	DateADT		date = PG_GETARG_DATEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, date);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

static DateADT
int_mysdate_internal(int64 arg, bool is_insert, char *msg)
{
	DateADT		date = MYSQL_NULL_DATE;
	fsec_t		fsec = 0;
	int			dterr, flag, warn = 0;	
	mysql_dttype ftype;
	struct pg_tm tt, *tm = &tt;

	MYS_ZERO_tm(tm);

	if(arg == 0)
	{
		date = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_insert)
		{
			bool	is_strict_mode = false;
			flag = 1;

			/* Get the sql_mode parameter */
			if(has_sql_mode(MODE_NO_ZERO_DATE))
				flag |= TIME_NO_ZERO_DATE;
			if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
				flag |= TIME_NO_ZERO_IN_DATE;
			if(has_sql_mode(MODE_INVALID_DATES))
				flag |= TIME_INVALID_DATES;
			if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
			   has_sql_mode(MODE_STRICT_ALL_TABLES))
				is_strict_mode = true;

			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED | 
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
					   {
						#ifdef WIN32
							elog(ERROR, "Incorrect DATE value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect DATE value: '%ld'", arg);
						#endif
					   }
				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						date = MYSQL_ZERO_DATE;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						date = MYSQL_ZERO_DATE;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					date = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check date is zero. */
				if(!non_zero_date(tm))
					date = MYSQL_ZERO_DATE;

				if(date != MYSQL_ZERO_DATE)
					date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
			}
		}
		else
		{
			flag = 9;
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				/* Begin - bug#z5068 */
				if (msg)
					ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", msg)));
				/* End - bug#z5068 */
				else
				{
					#ifdef WIN32
						ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", arg)));
					#else
						ereport(WARNING, (errmsg("Incorrect datetime value: '%ld'", arg)));
					#endif
				}
				if(dterr < 0)
					return MYSQL_NULL_DATE;
			}
			date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
		}
	}

	return date;
}


/* int_mysdate()
 * Convert integer to mysql date data type.
 */
Datum
int_mysdate(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	bool		is_insert = CALL_IN_INSERT();
	DateADT		ret;

	ret = int_mysdate_internal(arg, is_insert, NULL);

	if (ret == MYSQL_NULL_DATE)
		PG_RETURN_NULL();

	PG_RETURN_DATEADT(ret);
}

/* numeric_mysdate()
 * Convert numeric to mysql date data type.
 */
Datum
numeric_mysdate(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		n = 0;
	int64		len;
	DateADT		ret = 0;
	char	   *str, *tmpstr;;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while(*str && *str != '.')
	{
		len++;
		if(len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		n = n * 10 + *str - '0';
		str++;
	}

	ret = int_mysdate_internal(n, false, tmpstr);
	if (ret == MYSQL_NULL_DATE)
		PG_RETURN_NULL();

	PG_RETURN_DATEADT(ret);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysdate_eq(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mysdate_ne(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mysdate_lt(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;

	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mysdate_le(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mysdate_gt(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
		
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mysdate_ge(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
		
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mysdate_seq(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}


Datum
mysdate_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mysdate_adddate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
		
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;

	PG_RETURN_INT64(dateVal1 + dateVal2);
}

Datum
mysdate_subdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
		
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		dateVal2 = 0;


	PG_RETURN_INT64(dateVal1 - dateVal2);
}

/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mysdate_cmp(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (dateVal1 < dateVal2)
		PG_RETURN_INT32(-1);
	else if (dateVal1 > dateVal2)
		PG_RETURN_INT32(1);
	PG_RETURN_INT32(0);
}

Datum
mysdate_hash(PG_FUNCTION_ARGS)
{
	return hashint4(fcinfo);
}

Datum
mys_date_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint4extended(fcinfo);
}

// *
Datum mysdate_muldate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);
	int64		res;

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(MYSQL_MAX_VALUE_DATETIME/dateVal1 < dateVal2)
		ereport(ERROR, (errmsg("BIGINT value is out of range")));
	
	res = (int64)dateVal2 * dateVal1;
	PG_RETURN_INT64(res);
}

// div
Datum mysdate_divdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);
	float8 doubleval1 = 0;
	float8 doubleval2 = 0;
	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		PG_RETURN_NULL();
	
	if(dateVal2 == 0)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}
	doubleval1 = dateVal1;
	doubleval2 = dateVal2;
	
	PG_RETURN_FLOAT8(doubleval1 / doubleval2);
}

// exact_div
Datum mysdate_exact_divdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATE)
		PG_RETURN_NULL();
	
	PG_RETURN_INT64(dateVal1 / dateVal2);
}

// mod
Datum mysdate_moddate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;

	PG_RETURN_INT64(dateVal1 % dateVal2);
}

// bitand
Datum mysdate_bitanddate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 & dateVal2);
}

// bitor
Datum mysdate_bitordate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 | dateVal2);
}

// nega
Datum mysdate_negadate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	uint64		res;

	if(!dateVal1 )
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;

	res = ~(uint64)dateVal1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum mysdate_bitxordate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 ^ dateVal2);
}
// right move
Datum mysdate_rightmovdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(dateVal1 >> dateVal2);
}

// left move
Datum mysdate_leftmovdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}
	PG_RETURN_INT64(dateVal1 << dateVal2);
}


//BIT_COUNT
Datum mysdate_bit_count(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(getCount(dateVal1));
}

// and &&
Datum mysdate_anddate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 && dateVal2);
}

// invert ! 
Datum mysdate_invertmysdate(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_INT64(!dateVal1);
}

// date to boolean
Datum mysdate_boolean(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATE)
		dateVal1 = 0;
	PG_RETURN_BOOL(dateVal1 != 0);
}

//like
Datum mysdate2_like(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum mysdate_text_like(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	DateADT		date = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("date function: date field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect date value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("date field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	date = datetemp + time;

	PG_RETURN_BOOL(date == dateVal1);
}

Datum mysdate_int8_like(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

Datum mysdate2_notlike(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum mysdate_text_notlike(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	text *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	DateADT		date = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("date function: date field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect date value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("date field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	date = datetemp + time;

	PG_RETURN_BOOL(date != dateVal1);
}
Datum mysdate_int8_notlike(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(mysdate_larger);
PG_FUNCTION_INFO_V1(mysdate_smaller);
/* AGGREGATE */
Datum
mysdate_larger(PG_FUNCTION_ARGS)
{
	DateADT		val1 = PG_GETARG_DATEADT(0);
	DateADT		val2 = PG_GETARG_DATEADT(1);

	PG_RETURN_DATEADT(val1 > val2 ? val1 : val2);
}

Datum
mysdate_smaller(PG_FUNCTION_ARGS)
{
	DateADT		val1 = PG_GETARG_DATEADT(0);
	DateADT		val2 = PG_GETARG_DATEADT(1);

	PG_RETURN_DATEADT(val1 < val2 ? val1 : val2);
}

/* End - ReqID:SRS-MYSQL-DATETYPE */

/* Begin - ReqID:SRS-MYSQL-DATETIMETYPE */
PG_FUNCTION_INFO_V1(mysdatetime_in);
PG_FUNCTION_INFO_V1(mysdatetime_out);
PG_FUNCTION_INFO_V1(mysdatetime_recv);
PG_FUNCTION_INFO_V1(mysdatetime_send);
PG_FUNCTION_INFO_V1(int_mysdatetime);
PG_FUNCTION_INFO_V1(numeric_mysdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_eq);
PG_FUNCTION_INFO_V1(mysdatetime_ne);
PG_FUNCTION_INFO_V1(mysdatetime_lt);
PG_FUNCTION_INFO_V1(mysdatetime_gt);
PG_FUNCTION_INFO_V1(mysdatetime_le);
PG_FUNCTION_INFO_V1(mysdatetime_ge);
PG_FUNCTION_INFO_V1(mysdatetime_seq);
PG_FUNCTION_INFO_V1(mysdatetime_seqstr);

PG_FUNCTION_INFO_V1(mysdatetime_adddatetime);
PG_FUNCTION_INFO_V1(mysdatetime_subdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_cmp);
PG_FUNCTION_INFO_V1(mysdatetime_hash);
PG_FUNCTION_INFO_V1(mysdatetime_hash_extended);

PG_FUNCTION_INFO_V1(mysdatetime_muldatetime);

PG_FUNCTION_INFO_V1(mysdatetime_divdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_exact_divdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_moddatetime);

PG_FUNCTION_INFO_V1(mysdatetime_bitanddatetime);

PG_FUNCTION_INFO_V1(mysdatetime_bitordatetime);

PG_FUNCTION_INFO_V1(mysdatetime_negadatetime);

PG_FUNCTION_INFO_V1(mysdatetime_bitxordatetime);

PG_FUNCTION_INFO_V1(mysdatetime_rightmovdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_leftmovdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_bit_count);

PG_FUNCTION_INFO_V1(mysdatetime_anddatetime);

PG_FUNCTION_INFO_V1(mysdatetime_invertmysdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_boolean);

PG_FUNCTION_INFO_V1(mysdatetime2_like);
PG_FUNCTION_INFO_V1(mysdatetime_text_like);
PG_FUNCTION_INFO_V1(mysdatetime_int8_like);
PG_FUNCTION_INFO_V1(mysdatetime2_notlike);
PG_FUNCTION_INFO_V1(mysdatetime_text_notlike);
PG_FUNCTION_INFO_V1(mysdatetime_int8_notlike);

PG_FUNCTION_INFO_V1(mysdatetime_int8);
PG_FUNCTION_INFO_V1(mysdatetime_numeric);
Datum
mysdatetime_int8(PG_FUNCTION_ARGS)
{
	TimeADT	dateVal1 = PG_GETARG_TIMEADT(0);

	if (dateVal1 == MYSQL_ZERO_DATE)
	{
		dateVal1 = 0;
	}

	PG_RETURN_INT64(dateVal1);
}

Datum mysdatetime_numeric(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if (dateVal1 == MYSQL_ZERO_DATE)
	{
		dateVal1 = 0;
	}
	return NumericGetDatum(int64_to_numeric(dateVal1));
}
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/

/* 
 * mysdatetime_in
 * Given date text string, convert to internal date format.
 */
Datum
mysdatetime_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	TimeADT		datetime = MYSQL_NULL_TIME;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr = 0;
	mysql_dttype ftype = 0;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	PG_RETURN_TIMEADT(datetime);
}

/* 
 * mysdatetime_out
 * Given internal format date, convert to text string.
 */
Datum
mysdatetime_out(PG_FUNCTION_ARGS)
{
	TimeADT		datetime = PG_GETARG_TIMEADT(0);
	char	   *result;
	struct pg_tm tt,
			   *tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};

	if(datetime == MYSQL_NULL_DATE)
		PG_RETURN_NULL();
	
	if(datetime == MYSQL_ZERO_DATETIME)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00");
	}
	else
	{
		int64 part1, part2;
		DateADT date = 0;
		date = datetime / 1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = datetime % 1000000 / 10000;
		tm->tm_min = datetime % 1000000 % 10000 / 100;
		tm->tm_sec = datetime % 1000000 % 10000 % 100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year, tm->tm_mon, tm->tm_mday,
				 tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mysdatetime_recv			- converts external binary format to datetime
 */
Datum
mysdatetime_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("datetime out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mysdatetime_send			- converts datetime to binary format
 */
Datum
mysdatetime_send(PG_FUNCTION_ARGS)
{
	TimeADT		datetime = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, datetime);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* numeric_mysdatetime()
 * Convert numeric to mysql datetime data type.
 */
Datum
numeric_mysdatetime(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		datetimetemp = 0;
	int64		len;
	char	   *str, *tmpstr;
	TimeADT		datetime  = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;

	MemSet(tm, 0, sizeof(struct pg_tm));
	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while(*str && *str != '.')
	{
		len++;
		if(len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		datetimetemp = datetimetemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	str++; /* jump '.' */
	for(int i=0; i<6; i++)
	{
		int num = 0;
		if(*str != '\0')
		{
			num = *str - '0';
			str++;
		}
		fsec = fsec * 10 + num;
	}

	if(datetimetemp == 0)
	{
		datetime = MYSQL_ZERO_DATETIME;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
	
		if (is_insert)
		{
			dterr = mysnumber_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect DATETIME value: '%ld'", datetimetemp);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect DATETIME value: '%ld'", datetimetemp);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						datetime = MYSQL_ZERO_DATETIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						datetime = MYSQL_ZERO_DATETIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					datetime = MYSQL_ZERO_DATETIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect DATETIME value:'%s'", tmpstr)));
				}

				/* check datetime is zero. */
				if(!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATETIME;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("DATETIME field overflow.")));
					PG_RETURN_NULL();
				}
				if(datetime != MYSQL_ZERO_DATETIME)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					datetime = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect DATETIME value: '%ld'", datetimetemp)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			//dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if( mysdatetime_rounded_seconds(tm, &fsec,&warn))
			{
				ereport(WARNING, (errmsg("DATETIME field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

	PG_RETURN_TIMEADT(datetime);
}


/* int_mysdatetime()
 * Convert integer to mysql datetime data type.
 */
Datum
int_mysdatetime(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	TimeADT		datetime = MYSQL_NULL_DATE;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;

	if(arg == 0)
	{
		datetime = MYSQL_ZERO_DATETIME;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED |
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						{
						#ifdef WIN32
							elog(ERROR, "Incorrect DATETIME value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect DATETIME value: '%ld'", arg);
						#endif
						}

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATETIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATETIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					datetime = MYSQL_ZERO_DATETIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check datetime is zero. */
				if(!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATETIME;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("DATETIME field overflow.")));
					PG_RETURN_NULL();
				}
				if(datetime != MYSQL_ZERO_DATETIME)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					datetime = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
			#ifdef WIN32
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", arg)));
			#else
				ereport(WARNING, (errmsg("Incorrect datetime value: '%ld'", arg)));
			#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			//dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if(mysdatetime_rounded_seconds(tm, &fsec,&warn))
			{
				ereport(WARNING, (errmsg("DATETIME field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

	PG_RETURN_TIMEADT(datetime);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysdatetime_eq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;



	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mysdatetime_ne(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mysdatetime_lt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mysdatetime_le(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mysdatetime_gt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mysdatetime_ge(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mysdatetime_seq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mysdatetime_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mysdatetime_adddatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_INT64(dateVal1 + dateVal2);
}

Datum
mysdatetime_subdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		dateVal2 = 0;


	PG_RETURN_INT64(dateVal1 - dateVal2);
}

/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mysdatetime_cmp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (dateVal1 < dateVal2)
		PG_RETURN_INT32(-1);
	else if (dateVal1 > dateVal2)
		PG_RETURN_INT32(1);
	PG_RETURN_INT32(0);
}

Datum
mysdatetime_hash(PG_FUNCTION_ARGS)
{
	return hashint8(fcinfo);
}

Datum
mysdatetime_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint8extended(fcinfo);
}


// *
Datum mysdatetime_muldatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(MYSQL_MAX_VALUE_DATETIME/dateVal1 < dateVal2)
		ereport(ERROR, (errmsg("BIGINT value is out of range")));
	PG_RETURN_INT64(dateVal2 * dateVal1);
}


// div
Datum mysdatetime_divdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	float8 doubleval1 = 0;
	float8 doubleval2 = 0;
	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		PG_RETURN_NULL();
	
	if(dateVal2 == 0)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}
	doubleval1 = dateVal1;
	doubleval2 = dateVal2;
	
	PG_RETURN_FLOAT8(doubleval1 / doubleval2);
}

// exact_div
Datum mysdatetime_exact_divdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_DATETIME)
		PG_RETURN_NULL();
	
	PG_RETURN_INT64(dateVal1 / dateVal2);
}

// mod
Datum mysdatetime_moddatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;

	PG_RETURN_INT64(dateVal1 % dateVal2);
}

// bitand
Datum mysdatetime_bitanddatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;

	PG_RETURN_INT64(dateVal1 & dateVal2);
}

// bitor
Datum mysdatetime_bitordatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 | dateVal2);
}

// nega
Datum mysdatetime_negadatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_UINT64(~dateVal1);
}

// bitxor
Datum mysdatetime_bitxordatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 ^ dateVal2);
}

// right move
Datum mysdatetime_rightmovdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
	#ifdef WIN32
		ereport(WARNING, (errmsg("Truncated incorrect DECIMAL value:'%lld'",(int64)dateVal2)));
	#else
		ereport(WARNING, (errmsg("Truncated incorrect DECIMAL value:'%ld'",(int64)dateVal2)));
	#endif
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(dateVal1 >> dateVal2);
}

// left move
Datum mysdatetime_leftmovdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
	#ifdef WIN32
		ereport(WARNING, (errmsg("Truncated incorrect DECIMAL value:'%lld'",(int64)dateVal2)));
	#else
		ereport(WARNING, (errmsg("Truncated incorrect DECIMAL value:'%ld'",(int64)dateVal2)));
	#endif
		PG_RETURN_INT64(0);
	}
	PG_RETURN_INT64(dateVal1 << dateVal2);
}


Datum mysdatetime_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_INT64(getCount(dateVal1));
}

// and &&
Datum mysdatetime_anddatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 && dateVal2);
}

// invert ! 
Datum mysdatetime_invertmysdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_INT64(!dateVal1);
}

// datetime to boolean
Datum mysdatetime_boolean(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_DATETIME)
		dateVal1 = 0;
	PG_RETURN_BOOL(dateVal1 != 0);
}

//like
Datum mysdatetime2_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum mysdatetime_text_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	TimeADT		datetime;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	PG_RETURN_BOOL(datetime == dateVal1);
}

Datum mysdatetime_int8_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

Datum mysdatetime2_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum mysdatetime_text_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	TimeADT		datetime = -1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	PG_RETURN_BOOL(datetime != dateVal1);
}
Datum mysdatetime_int8_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(mysdatetime_larger);
PG_FUNCTION_INFO_V1(mysdatetime_smaller);
/* AGGREGATE */
Datum
mysdatetime_larger(PG_FUNCTION_ARGS)
{
	TimeADT		val1 = PG_GETARG_TIMEADT(0);
	TimeADT		val2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_TIMEADT(val1 > val2 ? val1 : val2);
}

Datum
mysdatetime_smaller(PG_FUNCTION_ARGS)
{
	TimeADT		val1 = PG_GETARG_TIMEADT(0);
	TimeADT		val2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_TIMEADT(val1 < val2 ? val1 : val2);
}


PG_FUNCTION_INFO_V1(mysdatetimes_in);
PG_FUNCTION_INFO_V1(mysdatetimes_out);
PG_FUNCTION_INFO_V1(mysdatetimes_recv);
PG_FUNCTION_INFO_V1(mysdatetimes_send);
PG_FUNCTION_INFO_V1(mysdatetimes_typmodin);
PG_FUNCTION_INFO_V1(mysdatetimes_typmodout);
PG_FUNCTION_INFO_V1(mysdatetimes_scale);

PG_FUNCTION_INFO_V1(int_mysdatetimes);
PG_FUNCTION_INFO_V1(numeric_mysdatetimes);

PG_FUNCTION_INFO_V1(pgdate_mysdatetimes);
PG_FUNCTION_INFO_V1(pgtimestamp_mysdatetimes);
PG_FUNCTION_INFO_V1(pgtimestamptz_mysdatetimes);
PG_FUNCTION_INFO_V1(pgtime_mysdatetimes);

PG_FUNCTION_INFO_V1(mysdatetimes_eq);
PG_FUNCTION_INFO_V1(mysdatetimes_ne);

PG_FUNCTION_INFO_V1(mysdatetimes_lt);

PG_FUNCTION_INFO_V1(mysdatetimes_gt);

PG_FUNCTION_INFO_V1(mysdatetimes_le);

PG_FUNCTION_INFO_V1(mysdatetimes_ge);

PG_FUNCTION_INFO_V1(mysdatetimes_seq);

PG_FUNCTION_INFO_V1(mysdatetimes_seqstr);
PG_FUNCTION_INFO_V1(mysdatetimes_adddatetimes);

PG_FUNCTION_INFO_V1(mysdatetimes_subdatetimes);

PG_FUNCTION_INFO_V1(mysdatetimes_cmp);
PG_FUNCTION_INFO_V1(mysdatetimes_hash);
PG_FUNCTION_INFO_V1(mysdatetimes_hash_extended);

PG_FUNCTION_INFO_V1(mysdatetimes_muldatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_divdatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_exact_divdatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_moddatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_bitanddatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_bitordatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_negadatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_bitxordatetime);
PG_FUNCTION_INFO_V1(mysdatetimes_bitxorint2);
PG_FUNCTION_INFO_V1(mysdatetimes_bitxorint4);
PG_FUNCTION_INFO_V1(mysdatetimes_bitxorint8);
PG_FUNCTION_INFO_V1(mysdatetimes_bitxornum);
PG_FUNCTION_INFO_V1(int2_bitxormysdatetimes);
PG_FUNCTION_INFO_V1(int4_bitxormysdatetimes);
PG_FUNCTION_INFO_V1(int8_bitxormysdatetimes);
PG_FUNCTION_INFO_V1(num_bitxormysdatetimes);

PG_FUNCTION_INFO_V1(mysdatetimes_rightmovdatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_leftmovdatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_bit_count);

PG_FUNCTION_INFO_V1(mysdatetimes_anddatetime);

PG_FUNCTION_INFO_V1(mysdatetimes_invertmysdatetimes);

PG_FUNCTION_INFO_V1(mysdatetimes_boolean);

PG_FUNCTION_INFO_V1(mysdatetimes2_like);
PG_FUNCTION_INFO_V1(mysdatetimes_text_like);
PG_FUNCTION_INFO_V1(mysdatetimes_int8_like);
PG_FUNCTION_INFO_V1(mysdatetimes_numeric_like);
PG_FUNCTION_INFO_V1(mysdatetimes2_notlike);
PG_FUNCTION_INFO_V1(mysdatetimes_text_notlike);
PG_FUNCTION_INFO_V1(mysdatetimes_int8_notlike);
PG_FUNCTION_INFO_V1(mysdatetimes_numeric_notlike);

PG_FUNCTION_INFO_V1(mysdatetimes_int8);
PG_FUNCTION_INFO_V1(mysdatetimes_numeric);
Datum
mysdatetimes_int8(PG_FUNCTION_ARGS)
{
	Numeric datetimeVal = PG_GETARG_NUMERIC(0);
	bool res;
	int64 retValue = 0;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(datetimeVal),
										   NumericGetDatum(int64_to_numeric(MYSQL_ZERO_DATE))));
	if (res)
	{
		PG_RETURN_INT64(0);
	}
	retValue = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(datetimeVal)));

	PG_RETURN_INT64(retValue);
}

Datum mysdatetimes_numeric(PG_FUNCTION_ARGS)
{
	Numeric datetimeVal = PG_GETARG_NUMERIC(0);
	bool res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(datetimeVal),
										   NumericGetDatum(int64_to_numeric(MYSQL_ZERO_DATE))));
	if (res)
	{
		PG_RETURN_NUMERIC(int64_to_numeric(0));
	}

	PG_RETURN_NUMERIC(datetimeVal);
}
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/

/* 
 * mysdatetimes_in
 * Given date text string, convert to internal date format.
 */
Datum
mysdatetimes_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	int32		typmod = PG_GETARG_INT32(2);

	TimeADT		datetime = MYSQL_NULL_TIME;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 		datetemp = 0;
	Numeric		num ;
	int			fsec_len;
	char		buf[MAXDATELEN + 1] = {0};

	/**
	 * Begin - SRS-MYSQL-BUILDINFUNCTION
	 * When do cast, typmod may be -1, it's illegal in this logic.
	 */
	if (typmod == -1)
		typmod = DATETIME_MAX_DECIMALS;
	/* End - SRS-MYSQL-BUILDINFUNCTION */

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	fsec += (nanoseconds + 500) / 1000;
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[typmod]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];

	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
		{
			PG_RETURN_NULL();
		}
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	if (fsec == 1000000)
	{
		dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
		if (dterr)
		{
			ereport(WARNING, (errmsg("datetime field overflow.")));
			PG_RETURN_NULL();
		}
		fsec = 0;
	}

	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d", datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d", datetime, fsec);
#endif

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));

	PG_RETURN_NUMERIC(num);
}

/* 
 * mysdatetimes_out
 * Given internal format date, convert to text string.
 */
Datum
mysdatetimes_out(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	Numeric		tempfsec;
	int64 		fsec = 0;
	int64		datetime = MYSQL_NULL_DATE;
	char		*result = NULL;
	struct pg_tm tt,
				*tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};
	char		*str = NULL;
	
	datetime = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(num)));
	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));
	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(tempfsec),
						   NumericGetDatum(int64_to_numeric(1000000))));

	fsec = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(tempfsec)));

	//Process rounding
	if(fsec >= 500000)
		datetime = datetime - 1;

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if(str == NULL)
		str = "";

	if(datetime == MYSQL_NULL_DATE)
		PG_RETURN_NULL();
	
	if(datetime == MYSQL_ZERO_DATETIME && fsec == 0 )
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00%s", str);
	}
	else
	{
		int64	part1, part2;
		DateADT		date = 0;
		date = datetime/1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = datetime%1000000 / 10000;
		tm->tm_min = datetime%1000000%10000 / 100;
		tm->tm_sec = datetime%1000000%10000%100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d%s",tm->tm_year,tm->tm_mon,tm->tm_mday,
				tm->tm_hour,tm->tm_min,tm->tm_sec, str);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mysdatetimes_recv			- converts external binary format to datetime
 */
Datum
mysdatetimes_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result = -1;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("datetime out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mysdatetimes_send			- converts datetime to binary format
 */
Datum
mysdatetimes_send(PG_FUNCTION_ARGS)
{
	TimeADT		datetime = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, datetime);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mysdatetimes_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mysdatetimes_typmodout(PG_FUNCTION_ARGS)
{
	int32		typmod = PG_GETARG_INT32(0);
	char	   *res = (char *) palloc(64);

	if (typmod >= 0)
		snprintf(res, 64, "(%d)", typmod);
	else
		*res = '\0';

	PG_RETURN_CSTRING(res);
}

Datum
mysdatetimes_scale(PG_FUNCTION_ARGS)
{
	Numeric		datetimenum = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(datetimenum);
}


/* numeric_mysdatetimes()
 * Convert numeric to mysql datetime data type.
 */
Datum
numeric_mysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		numvalue = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	int64		datetimetemp = 0;
	int64		len = 0;
	char	   *str, *tmpstr;
	TimeADT		datetime = 0;
	fsec_t		fsec = 0;
	fsec_t		nanoseconds = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(numvalue)));
	tmpstr = pstrdup(str);
	while(*str && *str != '.')
	{
		len++;
		datetimetemp = datetimetemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	if (*str == '.') /* BUGZ#5610 */
	{
		str++; /* jump '.' */
		for(int i=0; i<9; i++)
		{
			int numtemp = 0;
			if(*str != '\0')
			{
				numtemp = *str - '0';
				str++;
			}
			nanoseconds = nanoseconds * 10 + numtemp;
		}
	}

	if(datetimetemp == 0)
	{
		datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = mysnumber_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect datetime value: '%s'", tmpstr);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect datetime value: '%s'", tmpstr);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						datetime = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						datetime = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (ftype == MYSQL_TIMESTAMP_DATE)
				{
					/*
					Generate a warning in case of DATE with fractional part:
					20011231.1234 -> '2001-12-31'
					unless the caller does not want the warning: for example, CAST does.
					*/
					if (nanoseconds && !(flag & TIME_NO_DATE_FRAC_WARN))
						warn|= MYSQL_TIME_WARN_TRUNCATED;
				}
				else if (!(flag & TIME_NO_NSEC_ROUNDING))
				{
					fsec = (nanoseconds+500)/1000;
					(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
					fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
				}

				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					datetime = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect datetime value:'%s'", tmpstr)));
				}

				/* check datetime is zero. */
				if(!non_zero_date(tm))
					datetime = MYSQL_ZERO_TIMESTAMP;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("datetime field overflow.")));
					PG_RETURN_NULL();					
				}
				if(datetime != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					datetime = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}

			if (ftype == MYSQL_TIMESTAMP_DATE)
			{
				/*
				Generate a warning in case of DATE with fractional part:
				20011231.1234 -> '2001-12-31'
				unless the caller does not want the warning: for example, CAST does.
				*/
				if (nanoseconds && !(flag & TIME_NO_DATE_FRAC_WARN))
					warn|= MYSQL_TIME_WARN_TRUNCATED;
			}
			else if (!(flag & TIME_NO_NSEC_ROUNDING))
			{
				fsec = (nanoseconds+500)/1000;
				(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
				fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
			}

			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}


/* int_mysdatetimes()
 * Convert integer to mysql datetime data type.
 */
Datum
int_mysdatetimes(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	int			dec = PG_GETARG_INT32(1);
	TimeADT		datetime = -1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

	if(arg == 0)
	{
		datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED |
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
						MYSQL_TIME_WARN_ZERO_IN_DATE))
						{
						#ifdef WIN32
							elog(ERROR, "Incorrect datetime value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect datetime value: '%ld'", arg);
						#endif
						}
				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					datetime = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check datetime is zero. */
				if(!non_zero_date(tm))
					datetime = MYSQL_ZERO_TIMESTAMP;

				if(datetime != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					datetime = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
			#ifdef WIN32
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", arg)));
			#else
				ereport(WARNING, (errmsg("Incorrect datetime value: '%ld'", arg)));
			#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}

			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}


Datum pgdate_mysdatetimes(PG_FUNCTION_ARGS)
{
	DateADT pgdate = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			warn = 0;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		&(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%d'", pgdate)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}

	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		datetime = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		datetime = datetime * 1000000;
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);	
}

Datum pgtimestamp_mysdatetimes(PG_FUNCTION_ARGS)
{
	Timestamp pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	int64 temp = 0;
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			warn = 0;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}

	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

Datum pgtimestamptz_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimestampTz pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	int64 temp=0;
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			tz = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;
	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}
	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

Datum pgtime_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimeADT pgtime = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	time2tm(pgtime, tm, &fsec);

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			time = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtime)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIMESTAMP;
		}
	}
	if(time != 0 && time != MYSQL_ZERO_TIMESTAMP)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysdatetimes_eq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_ne(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ne, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_lt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_lt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_le(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_le, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}


Datum
mysdatetimes_gt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_ge(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ge, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_seq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mysdatetimes_adddatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;
	bool		nullval1 = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
										NumericGetDatum(int64_to_numeric(MYSQL_NULL_TIME))));
	bool		nullval2 = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal2),
										NumericGetDatum(int64_to_numeric(MYSQL_NULL_TIME))));

	if (nullval1 || nullval2)
	{
		PG_RETURN_NULL();
	}

	res = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}

Datum
mysdatetimes_subdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_sub, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}

/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mysdatetimes_cmp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		isgt ; 
	bool		iseq ; 
	isgt = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	iseq = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	if (isgt)
		PG_RETURN_INT32(1);
	else if (iseq)
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(-1);
}

Datum
mysdatetimes_hash(PG_FUNCTION_ARGS)
{
	return hash_numeric(fcinfo);
}

Datum
mysdatetimes_hash_extended(PG_FUNCTION_ARGS)
{
	return hash_numeric_extended(fcinfo);
}

// *
Datum mysdatetimes_muldatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_NUMERIC(res);
}

// div
Datum mysdatetimes_divdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;
	bool		divzero;
	char		*str;
	int			prelen;
	int			scalelen=0;

	divzero = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal2),
						NumericGetDatum(int64_to_numeric(0))));	
	if(divzero)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(dateVal1)));
	prelen = strlen(str) - 1 + 4;
	scalelen = prelen - 14;
	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											CStringGetDatum(str),
											ObjectIdGetDatum(InvalidOid),
											Int32GetDatum(((prelen << 16)| scalelen) + VARHDRSZ)));
	
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(res)));
	prelen = strlen(str) - 1;
	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											CStringGetDatum(str),
											ObjectIdGetDatum(InvalidOid),
											Int32GetDatum(((prelen << 16)| scalelen) + VARHDRSZ)));	
	PG_RETURN_NUMERIC(res);
}

// exact_div
Datum mysdatetimes_exact_divdatetime(PG_FUNCTION_ARGS)
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
	PG_RETURN_INT64(DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(res))));
}

// mod
Datum mysdatetimes_moddatetime(PG_FUNCTION_ARGS)
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

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_NUMERIC(res);
}

// bitand
Datum mysdatetimes_bitanddatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 & val2);
}


// bitor
Datum mysdatetimes_bitordatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 | val2);
}

// nega
Datum mysdatetimes_negadatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_UINT64(~val1);
}

// bitxor
Datum mysdatetimes_bitxordatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mysdatetimes_bitxorint2(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int16		dateVal2 = PG_GETARG_INT16(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mysdatetimes_bitxorint4(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int32		dateVal2 = PG_GETARG_INT32(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mysdatetimes_bitxorint8(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		dateVal2 = PG_GETARG_INT64(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mysdatetimes_bitxornum(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int2_bitxormysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int16		dateVal2 = PG_GETARG_INT16(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int4_bitxormysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int32		dateVal2 = PG_GETARG_INT32(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int8_bitxormysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int64		dateVal2 = PG_GETARG_INT64(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum num_bitxormysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}

// right move
Datum mysdatetimes_rightmovdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

// left move
Datum mysdatetimes_leftmovdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

Datum mysdatetimes_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
}

// and &&
Datum mysdatetimes_anddatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));

	PG_RETURN_INT64(val1 && val2);
}

// invert ! 
Datum mysdatetimes_invertmysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(!val1);
}

// datetime to boolean
Datum mysdatetimes_boolean(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum mysdatetimes2_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(res);
}

Datum mysdatetimes_text_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	int64		datetime;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(datetime)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(res);
}

Datum mysdatetimes_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mysdatetimes_numeric_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mysdatetimes2_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(!res);
}

Datum mysdatetimes_text_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	int64		datetime = -1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(datetime)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(!res);
}
Datum mysdatetimes_int8_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mysdatetimes_numeric_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(mysdatetimes_larger);
PG_FUNCTION_INFO_V1(mysdatetimes_smaller);
/* AGGREGATE */
Datum
mysdatetimes_larger(PG_FUNCTION_ARGS)
{
	Numeric		val1 = PG_GETARG_NUMERIC(0);
	Numeric		val2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(val1),
						NumericGetDatum(val2)));
	PG_RETURN_NUMERIC(res ? val1 : val2);
}

Datum
mysdatetimes_smaller(PG_FUNCTION_ARGS)
{
	Numeric		val1 = PG_GETARG_NUMERIC(0);
	Numeric		val2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_lt, NumericGetDatum(val1),
						NumericGetDatum(val2)));
	PG_RETURN_NUMERIC(res ? val1 : val2);
}
/* End - ReqID:SRS-MYSQL-DATETIMETYPE */

/* Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE */
PG_FUNCTION_INFO_V1(mystimestamp_in);
PG_FUNCTION_INFO_V1(mystimestamp_out);
PG_FUNCTION_INFO_V1(mystimestamp_recv);
PG_FUNCTION_INFO_V1(mystimestamp_send);
PG_FUNCTION_INFO_V1(int_mystimestamp);
PG_FUNCTION_INFO_V1(numeric_mystimestamp);
PG_FUNCTION_INFO_V1(timestamptz_mystimestamp);
PG_FUNCTION_INFO_V1(mystimestamp_eq);

PG_FUNCTION_INFO_V1(mystimestamp_ne);

PG_FUNCTION_INFO_V1(mystimestamp_lt);

PG_FUNCTION_INFO_V1(mystimestamp_gt);

PG_FUNCTION_INFO_V1(mystimestamp_le);

PG_FUNCTION_INFO_V1(mystimestamp_ge);

PG_FUNCTION_INFO_V1(mystimestamp_seq);

PG_FUNCTION_INFO_V1(mystimestamp_seqstr);
PG_FUNCTION_INFO_V1(mystimestamp_addtimestamp);

PG_FUNCTION_INFO_V1(mystimestamp_subtimestamp);

PG_FUNCTION_INFO_V1(mystimestamp_cmp);
PG_FUNCTION_INFO_V1(mystimestamp_hash);
PG_FUNCTION_INFO_V1(mystimestamp_hash_extended);

PG_FUNCTION_INFO_V1(mystimestamp_multimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_divtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_exact_divtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_modtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_bitandtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_bitortimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_negatimestamp);

PG_FUNCTION_INFO_V1(mystimestamp_bitxortimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_rightmovtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_leftmovtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_bit_count);

PG_FUNCTION_INFO_V1(mystimestamp_andtimestamp);


PG_FUNCTION_INFO_V1(mystimestamp_invertmystimestamp);

PG_FUNCTION_INFO_V1(mystimestamp_boolean);

PG_FUNCTION_INFO_V1(mystimestamp2_like);
PG_FUNCTION_INFO_V1(mystimestamp_text_like);
PG_FUNCTION_INFO_V1(mystimestamp_int8_like);
PG_FUNCTION_INFO_V1(mystimestamp2_notlike);
PG_FUNCTION_INFO_V1(mystimestamp_text_notlike);
PG_FUNCTION_INFO_V1(mystimestamp_int8_notlike);
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/

/* 
 * mystimestamp_in
 * Given date text string, convert to internal date format.
 */
Datum
mystimestamp_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	TimeADT		timestamp = MYSQL_NULL_TIME;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}

	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	PG_RETURN_TIMEADT(timestamp);
}

/* 
 * mystimestamp_out
 * Given internal format date, convert to text string.
 */
Datum
mystimestamp_out(PG_FUNCTION_ARGS)
{
	TimeADT		timestamp = PG_GETARG_TIMEADT(0);
	char	   *result = NULL;
	struct pg_tm tt,
			   *tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};

	if(timestamp == MYSQL_NULL_DATE)
		PG_RETURN_NULL();
	
	if(timestamp == MYSQL_ZERO_DATETIME)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00");
	}
	else
	{
		int64	part1, part2;
		DateADT		date = 0;
		date = timestamp/1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = timestamp%1000000 / 10000;
		tm->tm_min = timestamp%1000000%10000 / 100;
		tm->tm_sec = timestamp%1000000%10000%100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d",tm->tm_year,tm->tm_mon,tm->tm_mday,
				tm->tm_hour,tm->tm_min,tm->tm_sec);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mystimestamp_recv			- converts external binary format to timestamp
 */
Datum
mystimestamp_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mystimestamp_send			- converts timestamp to binary format
 */
Datum
mystimestamp_send(PG_FUNCTION_ARGS)
{
	TimeADT		timestamp = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, timestamp);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* numeric_mystimestamp()
 * Convert numeric to mysql timestamp data type.
 */
Datum
numeric_mystimestamp(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		timestamptemp = 0;
	int64		len;
	char	   *str, *tmpstr;
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while(*str && *str != '.')
	{
		len++;
		if(len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		timestamptemp = timestamptemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	str++; /* jump '.' */
	for(int i=0; i<6; i++)
	{
		int num = 0;
		if(*str != '\0')
		{
			num = *str - '0';
			str++;
		}
		fsec = fsec * 10 + num;
	}

	if(timestamptemp == 0)
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
			
		if (is_insert)
		{
			dterr = mysnumber_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					timestamp = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIMESTAMP value:'%s'", tmpstr)));
				}

				/* check timestamp is zero. */
				if(!non_zero_date(tm))
					timestamp = MYSQL_ZERO_TIMESTAMP;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();					
				}
				if(timestamp != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					timestamp = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIMESTAMP value: '%ld'", timestamptemp)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if(dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();					
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			timestamp = temp + time;
		}
	}

	PG_RETURN_TIMEADT(timestamp);
}

/* int_mystimestamp()
 * Convert integer to mysql timestamp data type.
 */
Datum
int_mystimestamp(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;

	if(arg == 0)
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
			
		if (is_insert)
		{
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED |
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
					   	{
						#ifdef WIN32
							elog(ERROR, "Incorrect TIMESTAMP value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", arg);
						#endif
						}
				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					timestamp = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check timestamp is zero. */
				if(!non_zero_date(tm))
					timestamp = MYSQL_ZERO_TIMESTAMP;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();					
				}
				if(timestamp != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					timestamp = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
			#ifdef WIN32
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", arg)));
			#else
				ereport(WARNING, (errmsg("Incorrect datetime value: '%ld'", arg)));
			#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if(dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();					
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			timestamp = temp + time;
		}
	}

	PG_RETURN_TIMEADT(timestamp);
}

Datum timestamptz_mystimestamp(PG_FUNCTION_ARGS)
{
	TimestampTz pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int64 res = 0;
	int64 temp = 0;
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			tz = 0;
	int			dterr = 0;
	int			warn = 0;

	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();					
	}
	temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	temp = temp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	res = temp + time;

	PG_RETURN_TIMEADT(res);
}
/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mystimestamp_eq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}


Datum
mystimestamp_ne(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mystimestamp_lt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mystimestamp_le(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mystimestamp_gt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mystimestamp_ge(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mystimestamp_seq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mystimestamp_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mystimestamp_addtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_INT64(dateVal1 + dateVal2);
}


Datum
mystimestamp_subtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		dateVal2 = 0;


	PG_RETURN_INT64(dateVal1 - dateVal2);
}


/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mystimestamp_cmp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (dateVal1 < dateVal2)
		PG_RETURN_INT32(-1);
	else if (dateVal1 > dateVal2)
		PG_RETURN_INT32(1);
	PG_RETURN_INT32(0);
}

Datum
mystimestamp_hash(PG_FUNCTION_ARGS)
{
	return hashint8(fcinfo);
}

Datum
mystimestamp_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint8extended(fcinfo);
}


// *
Datum mystimestamp_multimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(MYSQL_MAX_VALUE_DATETIME/dateVal1 < dateVal2)
		ereport(ERROR, (errmsg("BIGINT value is out of range")));
	PG_RETURN_INT64(dateVal2 * dateVal1);
}

// div
Datum mystimestamp_divtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	float8 doubleval1 = 0;
	float8 doubleval2 = 0;
	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		PG_RETURN_NULL();
	
	if(dateVal2 == 0)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}
	doubleval1 = dateVal1;
	doubleval2 = dateVal2;
	
	PG_RETURN_FLOAT8(doubleval1 / doubleval2);
}

// exact_div
Datum mystimestamp_exact_divtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 == MYSQL_ZERO_TIMESTAMP)
		PG_RETURN_NULL();
	
	PG_RETURN_INT64(dateVal1 / dateVal2);
}


// mod
Datum mystimestamp_modtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 % dateVal2);
}


// bitand
Datum mystimestamp_bitandtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 & dateVal2);
}

// bitor
Datum mystimestamp_bitortimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 | dateVal2);
}

// nega
Datum mystimestamp_negatimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_UINT64(~dateVal1);
}

// bitxor
Datum mystimestamp_bitxortimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 ^ dateVal2);
}

// right move
Datum mystimestamp_rightmovtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(dateVal1 >> dateVal2);
}

// left move
Datum mystimestamp_leftmovtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}
	PG_RETURN_INT64(dateVal1 << dateVal2);
}

Datum mystimestamp_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(getCount(dateVal1));
}

// and &&
Datum mystimestamp_andtimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(dateVal1 && dateVal2);
}

// invert ! 
Datum mystimestamp_invertmystimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_NULL();
	if(dateVal1 == MYSQL_ZERO_TIMESTAMP)
		dateVal1 = 0;
	PG_RETURN_INT64(!dateVal1);
}

// timestamp to boolean
Datum mystimestamp_boolean(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != 0);
}

//like
Datum mystimestamp2_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum mystimestamp_text_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	PG_RETURN_BOOL(timestamp == dateVal1);
}

Datum mystimestamp_int8_like(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

Datum mystimestamp2_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if(MYSQL_NULL_DATE == dateVal1 || MYSQL_NULL_DATE == dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum mystimestamp_text_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	PG_RETURN_BOOL(timestamp != dateVal1);
}
Datum mystimestamp_int8_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if (MYSQL_NULL_DATE == dateVal1)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(false);
}

/* Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE */
PG_FUNCTION_INFO_V1(mystimestamps_in);
PG_FUNCTION_INFO_V1(mystimestamps_out);
PG_FUNCTION_INFO_V1(mystimestamps_recv);
PG_FUNCTION_INFO_V1(mystimestamps_send);
PG_FUNCTION_INFO_V1(mystimestamps_typmodin);
PG_FUNCTION_INFO_V1(mystimestamps_typmodout);
PG_FUNCTION_INFO_V1(mystimestamps_scale);

PG_FUNCTION_INFO_V1(int_mystimestamps);
PG_FUNCTION_INFO_V1(numeric_mystimestamps);

PG_FUNCTION_INFO_V1(pgdate_mystimestamps);
PG_FUNCTION_INFO_V1(pgtimestamp_mystimestamps);
PG_FUNCTION_INFO_V1(pgtimestamptz_mystimestamps);
PG_FUNCTION_INFO_V1(pgtime_mystimestamps);

PG_FUNCTION_INFO_V1(mystimestamps_eq);
PG_FUNCTION_INFO_V1(mystimestamps_ne);

PG_FUNCTION_INFO_V1(mystimestamps_lt);

PG_FUNCTION_INFO_V1(mystimestamps_gt);

PG_FUNCTION_INFO_V1(mystimestamps_le);

PG_FUNCTION_INFO_V1(mystimestamps_ge);

PG_FUNCTION_INFO_V1(mystimestamps_seq);

PG_FUNCTION_INFO_V1(mystimestamps_seqstr);
PG_FUNCTION_INFO_V1(mystimestamps_addtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_subtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_cmp);
PG_FUNCTION_INFO_V1(mystimestamps_hash);
PG_FUNCTION_INFO_V1(mystimestamps_hash_extended);

PG_FUNCTION_INFO_V1(mystimestamps_multimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_divtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_exact_divtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_modtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_bitandtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_bitortimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_negatimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_bitxortimestamp);
PG_FUNCTION_INFO_V1(mystimestamps_bitxorint2);
PG_FUNCTION_INFO_V1(mystimestamps_bitxorint4);
PG_FUNCTION_INFO_V1(mystimestamps_bitxorint8);
PG_FUNCTION_INFO_V1(mystimestamps_bitxornum);
PG_FUNCTION_INFO_V1(int2_bitxormystimestamps);
PG_FUNCTION_INFO_V1(int4_bitxormystimestamps);
PG_FUNCTION_INFO_V1(int8_bitxormystimestamps);
PG_FUNCTION_INFO_V1(num_bitxormystimestamps);

PG_FUNCTION_INFO_V1(mystimestamps_rightmovtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_leftmovtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_bit_count);

PG_FUNCTION_INFO_V1(mystimestamps_andtimestamp);

PG_FUNCTION_INFO_V1(mystimestamps_invertmystimestamps);

PG_FUNCTION_INFO_V1(mystimestamps_boolean);

PG_FUNCTION_INFO_V1(mystimestamps2_like);
PG_FUNCTION_INFO_V1(mystimestamps_text_like);
PG_FUNCTION_INFO_V1(mystimestamps_int8_like);
PG_FUNCTION_INFO_V1(mystimestamps_numeric_like);
PG_FUNCTION_INFO_V1(mystimestamps2_notlike);
PG_FUNCTION_INFO_V1(mystimestamps_text_notlike);
PG_FUNCTION_INFO_V1(mystimestamps_int8_notlike);
PG_FUNCTION_INFO_V1(mystimestamps_numeric_notlike);
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/

/* 
 * mystimestamps_in
 * Given date text string, convert to internal date format.
 */
Datum
mystimestamps_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	int32		typmod = PG_GETARG_INT32(2);
	TimeADT		timestamp = MYSQL_NULL_TIME;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64 datetemp = 0;
	Numeric		num ;
	int			fsec_len = 0;
	char		buf[MAXDATELEN + 1] = {0};

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}

	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d", timestamp, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d", timestamp, fsec);
#endif

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));

	PG_RETURN_NUMERIC(num);
}

/* 
 * mystimestamps_out
 * Given internal format date, convert to text string.
 */
Datum
mystimestamps_out(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	Numeric		tempfsec;
	int64 		fsec = 0;
	int64		timestamp = 0;
	char		*result = NULL;
	struct pg_tm tt,
				*tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};
	char		*str = NULL;
	
	timestamp = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(num)));
	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));
	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(tempfsec),
						   NumericGetDatum(int64_to_numeric(1000000))));

	fsec = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(tempfsec)));

	//Process rounding
	if(fsec >= 500000)
		timestamp = timestamp - 1;

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if(str == NULL)
		str = "";

	if(timestamp == 0)
		PG_RETURN_NULL();
	
	if(timestamp == MYSQL_ZERO_DATETIME && fsec == 0)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00.000000");
	}
	else
	{
		int64	part1, part2;
		DateADT		date = 0;
		date = timestamp/1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = timestamp%1000000 / 10000;
		tm->tm_min = timestamp%1000000%10000 / 100;
		tm->tm_sec = timestamp%1000000%10000%100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d%s",tm->tm_year,tm->tm_mon,tm->tm_mday,
				tm->tm_hour,tm->tm_min,tm->tm_sec, str);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mystimestamps_recv			- converts external binary format to timestamp
 */
Datum
mystimestamps_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mystimestamps_send			- converts timestamp to binary format
 */
Datum
mystimestamps_send(PG_FUNCTION_ARGS)
{
	TimeADT		timestamp = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, timestamp);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mystimestamps_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mystimestamps_typmodout(PG_FUNCTION_ARGS)
{
	int32		typmod = PG_GETARG_INT32(0);
	char	   *res = (char *) palloc(64);

	if (typmod >= 0)
		snprintf(res, 64, "(%d)", typmod);
	else
		*res = '\0';

	PG_RETURN_CSTRING(res);
}

Datum
mystimestamps_scale(PG_FUNCTION_ARGS)
{
	Numeric		datetimenum = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(datetimenum);
}


/* numeric_mystimestamps()
 * Convert numeric to mysql timestamp data type.
 */
Datum
numeric_mystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		timestamptemp = 0;
	int64		len;
	char	   *str, *tmpstr;
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	Numeric		res;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while(*str && *str != '.')
	{
		len++;
		if(len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		timestamptemp = timestamptemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	if (*str == '.') /* BUGZ#5610 */
	{
		str++; /* jump '.' */
		for(int i=0; i<6; i++)
		{
			int num = 0;
			if(*str != '\0')
			{
				num = *str - '0';
				str++;
			}
			fsec = fsec * 10 + num;
		}
	}

	if(timestamptemp == 0)
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
			
		if (is_insert)
		{
			dterr = mysnumber_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					timestamp = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIMESTAMP value:'%s'", tmpstr)));
				}

				/* check timestamp is zero. */
				if(!non_zero_date(tm))
					timestamp = MYSQL_ZERO_TIMESTAMP;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();					
				}
				if(timestamp != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					timestamp = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIMESTAMP value: '%ld'", timestamptemp)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if(dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();					
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			timestamp = temp + time;
		}
	}

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(timestamp)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	res = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(res),
						   NumericGetDatum(int64_to_numeric(fsec))));
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(res),
						   NumericGetDatum(int64_to_numeric(1000000))));

	PG_RETURN_NUMERIC(res);
}


/* int_mystimestamps()
 * Convert integer to mysql timestamp data type.
 */
Datum
int_mystimestamps(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	int32		typmod = PG_GETARG_INT32(1);
	TimeADT		timestamp = 0;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	Numeric		res;
	char		buf[MAXDATELEN + 1] = {0};

	if(arg == 0)
	{
		timestamp = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
			
		if (is_insert)
		{
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED |
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						{
						#ifdef WIN32
							elog(ERROR, "Incorrect TIMESTAMP value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", arg);
						#endif
						}
				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_TIMESTAMP;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					timestamp = MYSQL_ZERO_TIMESTAMP;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check timestamp is zero. */
				if(!non_zero_date(tm))
					timestamp = MYSQL_ZERO_TIMESTAMP;

				dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
				if(dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();					
				}
				if(timestamp != MYSQL_ZERO_TIMESTAMP)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
					temp = temp * 1000000;
					time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
					timestamp = temp + time;
				}
			}
		}
		else
		{
			int64 temp = 0;
			flag = 9;
			dterr = mysnumber_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
			#ifdef WIN32
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", arg)));
			#else
				ereport(WARNING, (errmsg("Incorrect datetime value: '%ld'", arg)));
			#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if(dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();					
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			timestamp = temp + time;
		}
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d", timestamp, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d", timestamp, fsec);
#endif

	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));

	PG_RETURN_NUMERIC(res);
}

Datum pgdate_mystimestamps(PG_FUNCTION_ARGS)
{
	DateADT pgdate = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			warn = 0;
	int			is_strict_mode = 0;

	MemSet(tm, 0, sizeof(struct pg_tm));

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		&(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_TIMESTAMP;
	}

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%d'", pgdate)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}

	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		datetime = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		datetime = datetime * 1000000;
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);	
}

Datum pgtimestamp_mystimestamps(PG_FUNCTION_ARGS)
{
	Timestamp pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	int64 temp=0;
	int64 time=0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec=0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			warn = 0;
	int			is_strict_mode=0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_TIMESTAMP;
	}

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}

	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}


#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

Datum pgtimestamptz_mystimestamps(PG_FUNCTION_ARGS)
{
	TimestampTz pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 datetime = -1;
	int64 temp = 0;
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			tz;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;
	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_TIMESTAMP;
	}

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_TIMESTAMP;
		}
	}
	if(datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

Datum pgtime_mystimestamps(PG_FUNCTION_ARGS)
{
	TimeADT pgtime = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	time2tm(pgtime, tm, &fsec);

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];

	if(datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		time = MYSQL_ZERO_TIMESTAMP;
	}

	if(!warn)
	{
		if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			time = MYSQL_ZERO_TIMESTAMP;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtime)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIMESTAMP;
		}
	}
	if(time != 0 && time != MYSQL_ZERO_TIMESTAMP)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));

	return NumericGetDatum(num);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mystimestamps_eq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_BOOL(res);
}


Datum
mystimestamps_ne(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ne, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}



Datum
mystimestamps_lt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_lt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimestamps_le(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_le, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimestamps_gt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimestamps_ge(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ge, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}


Datum
mystimestamps_seq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}


Datum
mystimestamps_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mystimestamps_addtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}

Datum
mystimestamps_subtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_sub, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}


/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mystimestamps_cmp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		isgt ; 
	bool		iseq ; 
	isgt = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	iseq = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	if (isgt)
		PG_RETURN_INT32(1);
	else if (iseq)
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(-1);
}

Datum
mystimestamps_hash(PG_FUNCTION_ARGS)
{
	return hash_numeric(fcinfo);
}

Datum
mystimestamps_hash_extended(PG_FUNCTION_ARGS)
{
	return hash_numeric_extended(fcinfo);
}

// *
Datum mystimestamps_multimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_NUMERIC(res);
}

// div
Datum mystimestamps_divtimestamp(PG_FUNCTION_ARGS)
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
	PG_RETURN_NUMERIC(res);
}

// exact_div
Datum mystimestamps_exact_divtimestamp(PG_FUNCTION_ARGS)
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
	PG_RETURN_INT64(DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(res))));
}

// mod
Datum mystimestamps_modtimestamp(PG_FUNCTION_ARGS)
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

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_NUMERIC(res);
}

// bitand
Datum mystimestamps_bitandtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 & val2);
}

// bitor
Datum mystimestamps_bitortimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 | val2);
}

// nega
Datum mystimestamps_negatimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_UINT64(~val1);
}

// bitxor
Datum mystimestamps_bitxortimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimestamps_bitxorint2(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int16		dateVal2 = PG_GETARG_INT16(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimestamps_bitxorint4(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int32		dateVal2 = PG_GETARG_INT32(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimestamps_bitxorint8(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		dateVal2 = PG_GETARG_INT64(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimestamps_bitxornum(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int2_bitxormystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int16		dateVal2 = PG_GETARG_INT16(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int4_bitxormystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int32		dateVal2 = PG_GETARG_INT32(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int8_bitxormystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int64		dateVal2 = PG_GETARG_INT64(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum num_bitxormystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}

// right move
Datum mystimestamps_rightmovtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

// left move
Datum mystimestamps_leftmovtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

Datum mystimestamps_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
}

// and &&
Datum mystimestamps_andtimestamp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));

	PG_RETURN_INT64(val1 && val2);
}

// invert ! 
Datum mystimestamps_invertmystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(!val1);
}

// timestamp to boolean
Datum mystimestamps_boolean(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum mystimestamps2_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(res);
}

Datum mystimestamps_text_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	int64		timestamp;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len=0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(timestamp)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(res);
}

Datum mystimestamps_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimestamps_numeric_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimestamps2_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(!res);
}

Datum mystimestamps_text_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	int64		timestamp;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len = 0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(timestamp)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(!res);
}

Datum mystimestamps_int8_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimestamps_numeric_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

/* End - ReqID:SRS-MYSQL-TIMESTAMPTYPE */



/* Begin - ReqID:SRS-MYSQL-TIMETYPE */
PG_FUNCTION_INFO_V1(mystime_in);
PG_FUNCTION_INFO_V1(mystime_out);
PG_FUNCTION_INFO_V1(mystime_recv);
PG_FUNCTION_INFO_V1(mystime_send);
PG_FUNCTION_INFO_V1(int_mystime);
PG_FUNCTION_INFO_V1(numeric_mystime);
PG_FUNCTION_INFO_V1(mystime_numeric);

PG_FUNCTION_INFO_V1(pgdate_mystime);
PG_FUNCTION_INFO_V1(pgtimestamp_mystime);
PG_FUNCTION_INFO_V1(pgtimestamptz_mystime);

PG_FUNCTION_INFO_V1(mystime_eq);
PG_FUNCTION_INFO_V1(mystime_ne);
PG_FUNCTION_INFO_V1(mystime_lt);
PG_FUNCTION_INFO_V1(mystime_gt);
PG_FUNCTION_INFO_V1(mystime_le);
PG_FUNCTION_INFO_V1(mystime_ge);
PG_FUNCTION_INFO_V1(mystime_seq);
PG_FUNCTION_INFO_V1(mystime_add);
PG_FUNCTION_INFO_V1(mystime_sub);
PG_FUNCTION_INFO_V1(mystime_cmp);
PG_FUNCTION_INFO_V1(mystime_hash);
PG_FUNCTION_INFO_V1(mystime_mul);
PG_FUNCTION_INFO_V1(mystime_hash_extended);
PG_FUNCTION_INFO_V1(mystime_div);
PG_FUNCTION_INFO_V1(mystime_exact_divtime);
PG_FUNCTION_INFO_V1(mystime_mod);
PG_FUNCTION_INFO_V1(mystime_neg);
PG_FUNCTION_INFO_V1(mystime_and);
PG_FUNCTION_INFO_V1(mystime_bitand);
PG_FUNCTION_INFO_V1(mystime_bitor);
PG_FUNCTION_INFO_V1(mystime_bitxor);
PG_FUNCTION_INFO_V1(mystime_rightmov);
PG_FUNCTION_INFO_V1(mystime_leftmov);
PG_FUNCTION_INFO_V1(mystime_bit_count);
PG_FUNCTION_INFO_V1(mystime_invert);
PG_FUNCTION_INFO_V1(mystime_boolean);
PG_FUNCTION_INFO_V1(mystime_int8);
/* 
 * mystime_in
 * Given date text string, convert to internal date format.
 */
Datum
mystime_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nanosec = false;
	TimeOffset	time = MYSQL_NULL_TIME;
	int neg = 0;

#ifdef NOT_USED
	Oid             typelem = PG_GETARG_OID(1);
#endif

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mysstr_to_time(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &neg);
	nanosec = mystime_add_nanoseconds_with_round(tm, &fsec, nanoseconds, &warn);
	dterr |= mystime_rounded_seconds(tm, &fsec,&warn, 0);
	if(dterr || warn || nanosec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nanosec)
		{
			ereport(WARNING, (errmsg("time function: time field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect time value: '%s'", str)));

		if(dterr < 0 || nanosec)
			PG_RETURN_TIMEADT(MYSQL_NULL_TIME);
	}

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	if(neg)
	{
		time = -time;
	}

	PG_RETURN_TIMEADT(time);
}

/* 
 * mystime_out
 * Given internal format date, convert to text string.
 */
Datum
mystime_out(PG_FUNCTION_ARGS)
{
	TimeADT		time = PG_GETARG_TIMEADT(0);
	char	   *result;
	struct pg_tm tt,
			   *tm = &tt;
	char		buf[MAXDATELEN + 1];
	int			neg = 0;

	if(time == MYSQL_NULL_TIME)
		PG_RETURN_NULL();

	if(time < 0)
	{
		neg = 1;
		time = -time;
	}

	if(time == MYSQL_ZERO_TIME)
	{
		snprintf(buf, MAXDATELEN, "00:00:00");
	}
	else
	{
		tm->tm_hour = (time / 10000) % 1000;
		tm->tm_min = (time / 100) % 100;
		tm->tm_sec = time % 100;

		snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d", neg==1?"-":"", tm->tm_hour,tm->tm_min,tm->tm_sec);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mystime_recv			- converts external binary format to time
 */
Datum
mystime_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("time out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mystime_send			- converts time to binary format
 */
Datum
mystime_send(PG_FUNCTION_ARGS)
{
	TimeADT		time = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, time);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* int_mystime()
 * Convert integer to mysql time data type.
 */
Datum
int_mystime(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	int 		neg = 0;

	if(arg == 0)
	{
		time = MYSQL_ZERO_TIME;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = mysnumber_to_time(arg, tm, flag, &fsec, &ftype, &warn, &neg);

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIME value: '%ld'", arg);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIME value: '%ld'", arg);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						time = MYSQL_ZERO_TIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						time = MYSQL_ZERO_TIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					time = MYSQL_ZERO_TIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			}
		}
		else
		{
			flag = 9;
			dterr = mysnumber_to_time(arg, tm, flag, &fsec, &ftype, &warn, &neg);
			if (dterr < 0 || warn)
			{
				#ifdef WIN32
					ereport(WARNING, (errmsg("Incorrect time value: '%lld'", arg)));
				#else
					ereport(WARNING, (errmsg("Incorrect time value: '%ld'", arg)));
				#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}

			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		}
	}
	
	PG_RETURN_TIMEADT(neg!=1?time:-time);
}

/* numeric_mystime()
 * Convert numeric to mysql time data type.
 */
Datum
numeric_mystime(PG_FUNCTION_ARGS)
{
	Numeric		numtemp = PG_GETARG_NUMERIC(0);
	int64		timetemp = 0;
	char		*str;
	char		*strtemp;
	fsec_t		fsec = 0;
	fsec_t		nanoseconds = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeADT		time = -2;
	int			neg = 0;
	int			len=0;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	strtemp = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(numtemp)));
	str = pstrdup(strtemp);
	while(*strtemp && *strtemp != '.')
	{
		if(*strtemp == '-')
		{
			neg = 1;
			strtemp++;
			continue;
		}
		len++;
		timetemp = timetemp * 10 + *strtemp - '0';
		strtemp++;
	}

	/* Calculate the time */
	strtemp++; /* jump '.' */
	for(int i=0; i<9; i++)
	{
		int numtemp = 0;
		if(*strtemp != '\0')
		{
			numtemp = *strtemp - '0';
			strtemp++;
		}
		nanoseconds = nanoseconds * 10 + numtemp;
	}

	timetemp = neg==1?-timetemp:timetemp;

	if(timetemp == 0)
	{
		time = MYSQL_ZERO_TIME;
		PG_RETURN_TIMEADT(time);
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
			
		if (is_insert)
		{
			dterr = mysnumber_to_time(timetemp, tm, flag, &fsec, &ftype, &warn, &neg);

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIME value: '%s'", str);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIME value: '%s'", str);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						time = MYSQL_ZERO_TIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						time = MYSQL_ZERO_TIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					time = MYSQL_ZERO_TIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIME value:'%s'", str)));
				}

			}
		}
		else
		{
			flag = 9;
			dterr = mysnumber_to_time(timetemp, tm, flag, &fsec, &ftype, &warn, &neg);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIME value: '%s'", str)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}
		}

		fsec = nanoseconds / 1000;
		nanoseconds = nanoseconds%1000;
		warn = 0;
		(void)mystime_add_nanoseconds_with_round(tm, &fsec,nanoseconds, &warn);
		if(warn)
		{
			ereport(WARNING, (errmsg("Incorrect TIME value: '%s'", str)));
		}
		mystime_rounded_seconds(tm, &fsec,&warn, 0);

		if(time != MYSQL_ZERO_TIME)
		{
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		}
	}

	PG_RETURN_TIMEADT(time);
}

Datum mystime_numeric(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	return NumericGetDatum(int64_to_numeric(dateVal1));
}

Datum pgtimestamp_mystime(PG_FUNCTION_ARGS)
{
	Timestamp pgtimestamp = PG_GETARG_TIMESTAMP(0);
	int64 time =0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			dterr = 0;
	int			warn = 0;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();					
	}

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	PG_RETURN_TIMEADT(time);
}

Datum pgdate_mystime(PG_FUNCTION_ARGS)
{
	PG_RETURN_TIMEADT(0);
}

Datum pgtimestamptz_mystime(PG_FUNCTION_ARGS)
{
	TimestampTz pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int64 time=0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			tz = 0;
	int			dterr = 0;
	int			warn = 0;

	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();					
	}

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	PG_RETURN_TIMEADT(time);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mystime_eq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mystime_ne(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mystime_lt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mystime_le(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mystime_gt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mystime_ge(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mystime_seq(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mystime_add(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 + dateVal2);
}

Datum
mystime_sub(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 - dateVal2);
}

/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mystime_cmp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (dateVal1 < dateVal2)
		PG_RETURN_INT32(-1);
	else if (dateVal1 > dateVal2)
		PG_RETURN_INT32(1);
	PG_RETURN_INT32(0);
}

Datum
mystime_hash(PG_FUNCTION_ARGS)
{
	return hashint8(fcinfo);
}

Datum
mystime_hash_extended(PG_FUNCTION_ARGS)
{
	return hashint8extended(fcinfo);
}


// *
Datum mystime_mul(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	if(MYSQL_MAX_VALUE_DATETIME/dateVal1 < dateVal2)
		ereport(ERROR, (errmsg("BIGINT value is out of range")));
	PG_RETURN_INT64(dateVal2 * dateVal1);
}

// div
Datum mystime_div(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	Numeric		res;
	char		*str;
	int			prelen;
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	if(dateVal2 == 0)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}

	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(dateVal1)),
						   NumericGetDatum(int64_to_numeric(dateVal2))));
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(res)));
	prelen = strlen(str);

	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											CStringGetDatum(str),
											ObjectIdGetDatum(InvalidOid),
											Int32GetDatum(((prelen << 16)| 4) + VARHDRSZ)));
	PG_RETURN_NUMERIC(res);

}

// exact_div
Datum mystime_exact_divtime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 / dateVal2);
}

// mod
Datum mystime_mod(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 % dateVal2);
}

// bitand
Datum mystime_bitand(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 & dateVal2);
}

// bitor
Datum mystime_bitor(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 | dateVal2);
}

// nega
Datum mystime_neg(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if(dateVal1 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(~dateVal1);
}

// bitxor
Datum mystime_bitxor(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 ^ dateVal2);
}

// right move
Datum mystime_rightmov(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(dateVal1 >> dateVal2);
}

// left move
Datum mystime_leftmov(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	if(dateVal2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}
	PG_RETURN_INT64(dateVal1 << dateVal2);
}

Datum mystime_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if(dateVal1 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(getCount(dateVal1));
}

// and &&
Datum mystime_and(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);
	if(dateVal1 == MYSQL_NULL_TIME || dateVal2 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(dateVal1 && dateVal2);
}

// invert ! 
Datum mystime_invert(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if(dateVal1 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(!dateVal1);
}

// time to boolean
Datum
mystime_boolean(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	if (dateVal1 == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_BOOL(dateVal1 != 0);
}

Datum
mystime_int8(PG_FUNCTION_ARGS)
{
	TimeADT	val = PG_GETARG_TIMEADT(0);

	if (val == MYSQL_NULL_TIME)
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_INT64(val);
}

PG_FUNCTION_INFO_V1(mystimes_in);
PG_FUNCTION_INFO_V1(mystimes_out);
PG_FUNCTION_INFO_V1(mystimes_recv);
PG_FUNCTION_INFO_V1(mystimes_send);
PG_FUNCTION_INFO_V1(mystimes_typmodin);
PG_FUNCTION_INFO_V1(mystimes_typmodout);
PG_FUNCTION_INFO_V1(mystimes_scale);

PG_FUNCTION_INFO_V1(int_mystimes);
PG_FUNCTION_INFO_V1(numeric_mystimes);

PG_FUNCTION_INFO_V1(pgdate_mystimes);
PG_FUNCTION_INFO_V1(pgtimestamp_mystimes);
PG_FUNCTION_INFO_V1(pgtimestamptz_mystimes);
PG_FUNCTION_INFO_V1(pgtime_mystimes);

PG_FUNCTION_INFO_V1(mystimes_eq);

PG_FUNCTION_INFO_V1(mystimes_ne);

PG_FUNCTION_INFO_V1(mystimes_lt);

PG_FUNCTION_INFO_V1(mystimes_gt);

PG_FUNCTION_INFO_V1(mystimes_le);

PG_FUNCTION_INFO_V1(mystimes_ge);

PG_FUNCTION_INFO_V1(mystimes_seq);

PG_FUNCTION_INFO_V1(mystimes_seqstr);
PG_FUNCTION_INFO_V1(mystimes_addtimes);

PG_FUNCTION_INFO_V1(mystimes_subtimes);

PG_FUNCTION_INFO_V1(mystimes_cmp);
PG_FUNCTION_INFO_V1(mystimes_hash);
PG_FUNCTION_INFO_V1(mystimes_hash_extended);

PG_FUNCTION_INFO_V1(mystimes_muldatetime);


PG_FUNCTION_INFO_V1(mystimes_divdatetime);


PG_FUNCTION_INFO_V1(mystimes_exact_divdatetime);

PG_FUNCTION_INFO_V1(mystimes_moddatetime);

PG_FUNCTION_INFO_V1(mystimes_bitanddatetime);

PG_FUNCTION_INFO_V1(mystimes_bitordatetime);

PG_FUNCTION_INFO_V1(mystimes_negatimes);

PG_FUNCTION_INFO_V1(mystimes_bitxordatetime);
PG_FUNCTION_INFO_V1(mystimes_bitxorint2);
PG_FUNCTION_INFO_V1(mystimes_bitxorint4);
PG_FUNCTION_INFO_V1(mystimes_bitxorint8);
PG_FUNCTION_INFO_V1(mystimes_bitxornum);
PG_FUNCTION_INFO_V1(int2_bitxormystimes);
PG_FUNCTION_INFO_V1(int4_bitxormystimes);
PG_FUNCTION_INFO_V1(int8_bitxormystimes);
PG_FUNCTION_INFO_V1(num_bitxormystimes);

PG_FUNCTION_INFO_V1(mystimes_rightmovdatetime);

PG_FUNCTION_INFO_V1(mystimes_leftmovdatetime);

PG_FUNCTION_INFO_V1(mystimes_bit_count);

PG_FUNCTION_INFO_V1(mystimes_anddatetime);

PG_FUNCTION_INFO_V1(mystimes_invertmystimes);

PG_FUNCTION_INFO_V1(mystimes_boolean);

PG_FUNCTION_INFO_V1(mystimes2_like);
PG_FUNCTION_INFO_V1(mystimes_text_like);
PG_FUNCTION_INFO_V1(mystimes_int4_like);
PG_FUNCTION_INFO_V1(mystimes_int8_like);
PG_FUNCTION_INFO_V1(mystimes_numeric_like);
PG_FUNCTION_INFO_V1(mystimes2_notlike);
PG_FUNCTION_INFO_V1(mystimes_text_notlike);
PG_FUNCTION_INFO_V1(mystimes_int4_notlike);
PG_FUNCTION_INFO_V1(mystimes_int8_notlike);
PG_FUNCTION_INFO_V1(mystimes_numeric_notlike);

PG_FUNCTION_INFO_V1(mystimes_int8);
PG_FUNCTION_INFO_V1(mystimes_numeric);
Datum
mystimes_int8(PG_FUNCTION_ARGS)
{
	Numeric datetimeVal = PG_GETARG_NUMERIC(0);
	int64 retValue = 0;

	retValue = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(datetimeVal)));

	PG_RETURN_INT64(retValue);
}

Datum mystimes_numeric(PG_FUNCTION_ARGS)
{
	Numeric datetimeVal = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(datetimeVal);
}
/*****************************************************************************
 *	 Date ADT
 *****************************************************************************/

/* 
 * mystimes_in
 * Given date text string, convert to internal date format.
 */
Datum
mystimes_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int 		dterr;
	mysql_dttype ftype;
	int 		flag = TIME_FLAG_INIT;
	int 		warn = 0;
	int 		nanoseconds = 0;
	bool		nanosec = false;
	TimeOffset	time = 0;
	int 		neg = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int32		typmod = PG_GETARG_INT32(2);

	/**
	 * Begin - SRS-MYSQL-BUILDINFUNCTION
	 * When do cast, typmod may be -1, it's illegal in this logic.
	 */
	if (typmod == -1)
		typmod = DATETIME_MAX_DECIMALS;
	/* End - SRS-MYSQL-BUILDINFUNCTION */

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mysstr_to_time(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &neg);
	fsec += (nanoseconds+500) / 1000;
	nanosec = mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[typmod], &warn);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - typmod];

	if(dterr || warn || nanosec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nanosec)
		{
			ereport(WARNING, (errmsg("time function: time field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect time value: '%s'", str)));

		if(dterr < 0 || nanosec)
		{
			PG_RETURN_NULL();PG_RETURN_NUMERIC(Int32GetDatum(num));
		}
	}

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	if(neg)
	{
		time = -time;
	}

	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											   CStringGetDatum(buf),
											   ObjectIdGetDatum(InvalidOid),
											   Int32GetDatum((((14+typmod) << 16)|typmod) + VARHDRSZ)));

	PG_RETURN_NUMERIC(num);
}

/* 
 * mystimes_out
 * Given internal format date, convert to text string.
 */
Datum
mystimes_out(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	Numeric		tempfsec;
	int64 		fsec = 0;
	int64		time = 0;
	char		*result;
	struct pg_tm tt,
				*tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};
	char		*str = NULL;
	bool		neg = false;

	time = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(num)));

	/* Begin - bug#z5127 */
	if (numeric_is_neg(num))
	{
		if (time < 0)
			time = -time;
		neg = true;
	}
	/* End - bug#z5127 */

	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(num),
												   NumericGetDatum(int64_to_numeric(1000000))));
	tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(tempfsec),
												   NumericGetDatum(int64_to_numeric(1000000))));

	fsec = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(tempfsec)));

	// Process rounding
	if (fsec >= 500000)
		time = time - 1;

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if (str == NULL)
		str = "";

	if (time == MYSQL_NULL_TIME)
		PG_RETURN_NULL();

	if (time == MYSQL_ZERO_TIME && fsec == 0)
	{
		snprintf(buf, MAXDATELEN, "00:00:00%s", str);
	}
	else
	{
		tm->tm_hour = time / 10000;
		tm->tm_min = time % 10000 / 100;
		tm->tm_sec = time % 100;

		snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d%s", neg ? "-" : "", tm->tm_hour, tm->tm_min, tm->tm_sec, str);
	}

	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *mystimes_recv			- converts external binary format to datetime
 */
Datum
mystimes_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	TimeADT		result;

	result = (TimeADT) pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		 /* ok */ ;
	else if (!IS_VALID_DATE(result))
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("datetime out of range")));

	PG_RETURN_TIMEADT(result);
}

/*
 *mystimes_send			- converts datetime to binary format
 */
Datum
mystimes_send(PG_FUNCTION_ARGS)
{
	TimeADT		datetime = PG_GETARG_TIMEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, datetime);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mystimes_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mystimes_typmodout(PG_FUNCTION_ARGS)
{
	int32		typmod = PG_GETARG_INT32(0);
	char	   *res = (char *) palloc(64);

	if (typmod >= 0)
		snprintf(res, 64, "(%d)",typmod);
	else
		*res = '\0';

	PG_RETURN_CSTRING(res);
}

Datum
mystimes_scale(PG_FUNCTION_ARGS)
{
	Numeric		datetimenum = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(datetimenum);
}


/* numeric_mystimes()
 * Convert numeric to mysql datetime data type.
 */
Datum
numeric_mystimes(PG_FUNCTION_ARGS)
{
	Numeric		numtemp = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	int64		timetemp = 0;
	char		*str;
	char		*strtemp;
	fsec_t		fsec = 0;
	fsec_t		nanoseconds = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeADT		time = -2;
	int			neg = 0;
	bool		nanosec = false;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int64		len = 0;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	strtemp = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(numtemp)));
	str = pstrdup(strtemp);
	while(*strtemp && *strtemp != '.')
	{
		if(*strtemp == '-')
		{
			neg = 1;
			strtemp++;
			continue;
		}
		len++;
		timetemp = timetemp * 10 + *strtemp - '0';
		strtemp++;
	}

	/* Calculate the time */
	strtemp++; /* jump '.' */
	for(int i=0; i<9; i++)
	{
		int numtemp = 0;
		if(*strtemp != '\0')
		{
			numtemp = *strtemp - '0';
			strtemp++;
		}
		nanoseconds = nanoseconds * 10 + numtemp;
	}

	timetemp = neg==1?-timetemp:timetemp;

	if(timetemp == 0)
	{
		time = MYSQL_ZERO_TIME;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
	
		if (is_insert)
		{
			dterr = mysnumber_to_time(timetemp, tm, flag, &fsec, &ftype, &warn, &neg);
			/*if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;*/

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIME value: '%s'", str);
					if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIME value: '%s'", str);

				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						time = MYSQL_ZERO_TIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						time = MYSQL_ZERO_TIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					time = MYSQL_ZERO_TIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIME value:'%s'", str)));
				}
			}
		}
		else
		{
			flag = 9;
			dterr = mysnumber_to_time(timetemp, tm, flag, &fsec, &ftype, &warn, &neg);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIME value: '%s'", str)));
				if(dterr < 0)
					PG_RETURN_NULL();
			}
		}

		fsec = (nanoseconds+500) / 1000;
		nanoseconds = nanoseconds%1000;
		warn = 0;
		nanosec = mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
		fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
		if(nanoseconds && (nanosec || warn))
		{
			ereport(WARNING, (errmsg("Incorrect TIME value: '%s'", str)));
		}

		if(time != MYSQL_ZERO_TIME)
		{
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		}
	}

	snprintf(buf, MAXDATELEN, "%ld.%06d",neg!=1?time:-time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}


/* int_mystimes()
 * Convert integer to mysql datetime data type.
 */
Datum
int_mystimes(PG_FUNCTION_ARGS)
{
	int64		arg = PG_GETARG_INT64(0);
	int			dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;
	int neg = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

	if(arg == 0)
	{
		time = MYSQL_ZERO_TIME;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool		is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */
	
		if (is_insert)
		{
			dterr = mysnumber_to_time(arg, tm, flag, &fsec, &ftype, &warn, &neg);

			if(dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool	is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if(has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if(has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if(has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if(has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if(is_strict_mode)/* strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_TRUNCATED | 
						MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE))
					   {
						#ifdef WIN32
							elog(ERROR, "Incorrect TIME value: '%lld'", arg);
						#else
							elog(ERROR, "Incorrect TIME value: '%ld'", arg);
						#endif
					   }
				}
				else /* non-strict mode */
				{
					if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						time = MYSQL_ZERO_TIME;
					}
					if(warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						time = MYSQL_ZERO_TIME;
					}
				}
			}
			else /* Returns the correct result with a value of 0 */
			{
				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
					   MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
					time = MYSQL_ZERO_TIME;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
				   !(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			}
		}
		else
		{
			flag = 9;
			dterr = mysnumber_to_time(arg, tm, flag, &fsec, &ftype, &warn, &neg);
			if (dterr < 0 || warn)
			{
			#ifdef WIN32
				ereport(WARNING, (errmsg("Incorrect time value: '%lld'", arg)));
			#else
				ereport(WARNING, (errmsg("Incorrect time value: '%ld'", arg)));
			#endif
				if(dterr < 0)
					PG_RETURN_NULL();
			}

			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		}
	}

	snprintf(buf, MAXDATELEN, "%ld.%06d",neg!=1?time:-time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}

Datum pgdate_mystimes(PG_FUNCTION_ARGS)
{
	DateADT pgdate = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time =0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		&(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
	
	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	(void)mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
	if( !warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%d'", pgdate)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%ld.%06d", time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}

Datum pgtimestamp_mystimes(PG_FUNCTION_ARGS)
{
	Timestamp pgtimestamp = PG_GETARG_TIMESTAMP(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamp out of range")));

	(void)mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
	if( !warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}


Datum pgtimestamptz_mystimes(PG_FUNCTION_ARGS)
{
	TimestampTz pgtimestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			tz = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	(void)mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
	if( !warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtimestamp)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}
		
	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}

Datum pgtime_mystimes(PG_FUNCTION_ARGS)
{
	TimeADT pgtime = PG_GETARG_TIMESTAMPTZ(0);
	int32		dec = PG_GETARG_INT32(1);
	int64 time = 0;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec = 0;
	int			warn = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			is_strict_mode = 0;

	if((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
	   (has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	time2tm(pgtime, tm, &fsec);

	(void)mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - dec];
	if( !warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if(is_strict_mode &&(warn & ( MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtime)));
		}
		if(warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}
		
	snprintf(buf, MAXDATELEN, "%ld.%06d",time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mystimes_eq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_BOOL(res);
}

Datum
mystimes_ne(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ne, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_lt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_lt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_le(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_le, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_gt(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_ge(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_ge, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_seq(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_BOOL(res);
}

Datum
mystimes_seqstr(PG_FUNCTION_ARGS)
{
	bool		res;
	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(), 
			PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}

Datum
mystimes_addtimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}

Datum
mystimes_subtimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_sub, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	
	PG_RETURN_NUMERIC(res);
}

/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mystimes_cmp(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		isgt ; 
	bool		iseq ; 
	isgt = DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	iseq = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	if (isgt)
		PG_RETURN_INT32(1);
	else if (iseq)
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(-1);
}

Datum
mystimes_hash(PG_FUNCTION_ARGS)
{
	return hash_numeric(fcinfo);
}

Datum
mystimes_hash_extended(PG_FUNCTION_ARGS)
{
	return hash_numeric_extended(fcinfo);
}

// *
Datum mystimes_muldatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_NUMERIC(res);
}

// div
Datum mystimes_divdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	Numeric		res;
	bool		divzero;
	char		*str;
	int			prelen;
	int			scalelen=0;

	divzero = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal2),
						NumericGetDatum(int64_to_numeric(0))));	
	if(divzero)
	{
		ereport(WARNING, (errmsg("Division by 0.")));
		PG_RETURN_NULL();
	}

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(dateVal1)));
	prelen = strlen(str) - 1 + 4;
	scalelen = prelen - (strchr(str, '.') - str);
	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											CStringGetDatum(str),
											ObjectIdGetDatum(InvalidOid),
											Int32GetDatum(((prelen << 16)| scalelen) + VARHDRSZ)));
	
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(res)));
	prelen = strlen(str) - 1;
	res = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											CStringGetDatum(str),
											ObjectIdGetDatum(InvalidOid),
											Int32GetDatum(((prelen << 16)| scalelen) + VARHDRSZ)));	
	PG_RETURN_NUMERIC(res);
}

// exact_div
Datum mystimes_exact_divdatetime(PG_FUNCTION_ARGS)
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
	PG_RETURN_INT64(DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(res))));
}

// mod
Datum mystimes_moddatetime(PG_FUNCTION_ARGS)
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

	res = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));
	PG_RETURN_NUMERIC(res);
}

// bitand
Datum mystimes_bitanddatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 & val2);
}

// bitor
Datum mystimes_bitordatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 | val2);
}

// nega
Datum mystimes_negatimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_UINT64(~val1);
}

// bitxor
Datum mystimes_bitxordatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 || !val2)
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimes_bitxorint2(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int16		dateVal2 = PG_GETARG_INT16(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimes_bitxorint4(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int32		dateVal2 = PG_GETARG_INT32(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimes_bitxorint8(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		dateVal2 = PG_GETARG_INT64(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum mystimes_bitxornum(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int2_bitxormystimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int16		dateVal2 = PG_GETARG_INT16(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int4_bitxormystimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int32		dateVal2 = PG_GETARG_INT32(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum int8_bitxormystimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	int64		dateVal2 = PG_GETARG_INT64(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = dateVal2;
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}
Datum num_bitxormystimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(0);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1 )
		PG_RETURN_NULL();

	PG_RETURN_INT64(val1 ^ val2);
}

// right move
Datum mystimes_rightmovdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

// left move
Datum mystimes_leftmovdatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));
	
	if(!val1)
		PG_RETURN_NULL();

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_INT64(0);
	}

	PG_RETURN_INT64(val1 >> val2);
}

Datum mystimes_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
}

// and &&
Datum mystimes_anddatetime(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	int64		val1;
	int64		val2;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	val2 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal2)));

	PG_RETURN_INT64(val1 && val2);
}

// invert ! 
Datum mystimes_invertmystimes(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(!val1);
}

// datetime to boolean
Datum mystimes_boolean(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));
	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum mystimes2_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(res);
}

Datum mystimes_text_like(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	text	   *file = PG_GETARG_TEXT_PP(1);

	char	   *str = text_to_cstring(file);
	int64		datetime=-1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len=0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(datetime)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(res);
}

Datum mystimes_int4_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}
Datum mystimes_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimes_numeric_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimes2_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	Numeric		dateVal2 = PG_GETARG_NUMERIC(1);
	bool		res;

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(dateVal2)));

	PG_RETURN_BOOL(!res);
}

Datum mystimes_text_notlike(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	int64		datetime=-1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int			flag = TIME_FLAG_INIT;
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nonsec = false;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	bool		res;
	Numeric		num ;
	int			fsec_len=0;

	if(has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if(has_sql_mode( MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if(!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec,&warn, nanoseconds);
	if(dterr || warn || nonsec)
	{
		if((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr) 
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if(MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		//ERROR 1064 (42000): You have an error in your SQL syntax
		if(dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
	if(dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ; 
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(datetime)),
						   NumericGetDatum(int64_to_numeric(1000000))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(fsec))));
	num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
						   NumericGetDatum(int64_to_numeric(1000000))));

	res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
						NumericGetDatum(num)));

	PG_RETURN_BOOL(!res);
}
Datum mystimes_int4_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}
Datum mystimes_int8_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimes_numeric_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}
/* End - ReqID:SRS-MYSQL-TIMETYPE */

/* mysdate to time */
PG_FUNCTION_INFO_V1(mysdate_mystime);
/* mystime mystimes to date datetime timestamp */
PG_FUNCTION_INFO_V1(mystime_mysdate);
PG_FUNCTION_INFO_V1(mystime_mystimestamp);
PG_FUNCTION_INFO_V1(mystimes_mysdate);
PG_FUNCTION_INFO_V1(mystimes_mysdatetime);
PG_FUNCTION_INFO_V1(mystimes_mystimestamp);
/* mysdate to datetimes timestamps times */
PG_FUNCTION_INFO_V1(mysdate_mysdatetimes);
PG_FUNCTION_INFO_V1(mysdate_mystimestamps);
PG_FUNCTION_INFO_V1(mysdate_mystimes);
/* mysdatetime to datetimes timestamps times */
PG_FUNCTION_INFO_V1(mysdatetime_mysdatetimes);
PG_FUNCTION_INFO_V1(mysdatetime_mystimestamps);
PG_FUNCTION_INFO_V1(mysdatetime_mystimes);
/*mysdatetimes to timestamps times*/
PG_FUNCTION_INFO_V1(mysdatetimes_mystimestamps);
PG_FUNCTION_INFO_V1(mysdatetimes_mystimes);
/*mystimestamp to datetimes timestamps times*/
PG_FUNCTION_INFO_V1(mystimestamp_mysdatetimes);
PG_FUNCTION_INFO_V1(mystimestamp_mystimestamps);
PG_FUNCTION_INFO_V1(mystimestamp_mystimes);
/*mystimestamps to datetimes times*/
PG_FUNCTION_INFO_V1(mystimestamps_mysdatetimes);
PG_FUNCTION_INFO_V1(mystimestamps_mystimes);
/*mystime to datetimes timestamps times*/
PG_FUNCTION_INFO_V1(mystime_mysdatetimes);
PG_FUNCTION_INFO_V1(mystime_mystimestamps);
PG_FUNCTION_INFO_V1(mystime_mystimes);
/*mystimes to datetimes timestamps*/
PG_FUNCTION_INFO_V1(mystimes_mysdatetimes);
PG_FUNCTION_INFO_V1(mystimes_mystimestamps);


Datum mysdate_mystime(PG_FUNCTION_ARGS)
{
	TimeADT		time = 0;
	PG_RETURN_TIMEADT(time);
}
Datum mystime_mysdate(PG_FUNCTION_ARGS)
{
	DateADT			date = 0;
	struct pg_tm 	tt, *tm = &tt;
	TimeADT			time = PG_GETARG_TIMEADT(0);
	int 			hour = time / 10000,
					min = time % 10000 / 100,
					sec = time % 10000 % 100;
	TimeOffset 		timestamp1 = hour * USECS_PER_HOUR + min * USECS_PER_MINUTE + sec * USECS_PER_SEC,
					timestamp2 = 0,
					timestamp = 0;
	fsec_t			fsec = 0;

	MYS_ZERO_tm(tm);
	GetCurrentDateTime(tm);

	// Ignore time, just keep date
	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	tm2timestamp(tm, 0, NULL, &timestamp2);

	timestamp = timestamp1 + timestamp2;

	MYS_ZERO_tm(tm);
	timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL);

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	PG_RETURN_DATEADT(date);
}

Datum mystime_mystimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		datetime;
	TimeADT		date = 0;
	TimeADT		time = PG_GETARG_TIMEADT(0);
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	datetime = date * 1000000 + time;
	PG_RETURN_TIMEADT(datetime);
}

Datum mystimes_mysdate(PG_FUNCTION_ARGS)
{
	DateADT			date = 0;
	struct pg_tm 	tt, *tm = &tt;
	Numeric 		times = PG_GETARG_NUMERIC(0);
	TimeADT			time = DatumGetInt64(DirectFunctionCall1(numeric_int8,
															 NumericGetDatum(times)));
	int 			hour = time / 10000,
					min = time % 10000 / 100,
					sec = time % 10000 % 100;
	TimeOffset 		timestamp1 = hour * USECS_PER_HOUR + min * USECS_PER_MINUTE + sec * USECS_PER_SEC,
					timestamp2 = 0,
					timestamp = 0;
	fsec_t			fsec = 0;

	MYS_ZERO_tm(tm);
	GetCurrentDateTime(tm);

	// Ignore time, just keep date
	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	tm2timestamp(tm, 0, NULL, &timestamp2);

	timestamp = timestamp1 + timestamp2;

	MYS_ZERO_tm(tm);
	timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL);

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	PG_RETURN_DATEADT(date);
}

Datum mystimes_mysdatetime(PG_FUNCTION_ARGS)
{
	Numeric 		times = PG_GETARG_NUMERIC(0);
	TimeADT			time_in = DatumGetInt64(DirectFunctionCall1(numeric_int8,
																NumericGetDatum(times)));
	int 			hour = time_in / 10000,
					min = time_in % 10000 / 100,
					sec = time_in % 10000 % 100;
	TimeOffset 		timestamp1 = hour * USECS_PER_HOUR + min * USECS_PER_MINUTE + sec * USECS_PER_SEC,
					timestamp2 = 0,
					timestamp = 0;
	fsec_t			fsec = 0;
	struct pg_tm 	tt, *tm = &tt;
	TimeADT			datetime, date = 0, time = 0;

	MYS_ZERO_tm(tm);
	GetCurrentDateTime(tm);

	// Ignore time, just keep date
	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	tm2timestamp(tm, 0, NULL, &timestamp2);

	timestamp = timestamp1 + timestamp2;

	MYS_ZERO_tm(tm);
	timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL);
	
	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	datetime = date * 1000000 + time;
	PG_RETURN_TIMEADT(datetime);	
}

Datum mystimes_mystimestamp(PG_FUNCTION_ARGS)
{
	Numeric		times = PG_GETARG_NUMERIC(0);
	TimeADT		datetime;
	TimeADT		date = 0;
	TimeADT		time = 0;
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	time = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(times)));
	datetime = date*1000000 + time;
	PG_RETURN_TIMEADT(datetime);
}

Datum mysdate_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimeADT		mysdate = PG_GETARG_DATEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	TimeADT		datetime;

	datetime = mysdate*1000000;
#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdate_mystimestamps(PG_FUNCTION_ARGS)
{
	TimeADT		mysdate = PG_GETARG_DATEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	TimeADT		datetime;

	datetime = mysdate*1000000;
#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",datetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",datetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdate_mystimes(PG_FUNCTION_ARGS)
{
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	int			time = 0;

	snprintf(buf, MAXDATELEN, "%d.%06d",time, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdatetime_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdatetime_mystimestamps(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdatetime_mystimes(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	fsec_t		fsec = 0;
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;

	mysdatetime = mysdatetime%1000000;
#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdatetimes_mystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		mysdatetimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;

	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(mysdatetimes)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mysdatetimes_mystimes(PG_FUNCTION_ARGS)
{
	Numeric		mysdatetimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;
	Numeric		timeval;

	timeval = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(mysdatetimes),
						NumericGetDatum(int64_to_numeric(1000000))));
	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(timeval)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimestamp_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimestamp_mystimestamps(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;

#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimestamp_mystimes(PG_FUNCTION_ARGS)
{
	TimeADT		mysdatetime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;

	mysdatetime = mysdatetime%1000000;
#ifdef WIN32
	snprintf(buf, MAXDATELEN, "%lld.%06d",mysdatetime, fsec);
#else
	snprintf(buf, MAXDATELEN, "%ld.%06d",mysdatetime, fsec);
#endif
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimestamps_mysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		mysdatetimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;

	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(mysdatetimes)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimestamps_mystimes(PG_FUNCTION_ARGS)
{
	Numeric		mysdatetimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;
	Numeric		timeval;

	timeval = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(mysdatetimes),
						NumericGetDatum(int64_to_numeric(1000000))));
	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(timeval)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystime_mysdatetimes(PG_FUNCTION_ARGS)
{
	TimeADT		mystime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;
	TimeADT		date = 0;
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	mystime = date*1000000+mystime;
	snprintf(buf, MAXDATELEN, "%ld.%06d",mystime, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystime_mystimestamps(PG_FUNCTION_ARGS)
{
	TimeADT		mystime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;
	TimeADT		date = 0;
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	mystime = date*1000000+mystime;
	snprintf(buf, MAXDATELEN, "%ld.%06d",mystime, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystime_mystimes(PG_FUNCTION_ARGS)
{
	TimeADT		mystime = PG_GETARG_TIMEADT(0);
	int32		dec = PG_GETARG_INT32(1);
	char		buf[MAXDATELEN + 1] = {0};
	Numeric		num;
	fsec_t		fsec = 0;

	snprintf(buf, MAXDATELEN, "%ld.%06d",mystime, fsec);
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimes_mysdatetimes(PG_FUNCTION_ARGS)
{
	Numeric		mystimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;
	TimeADT		date = 0;
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	date = date*1000000;
	mystimes = DatumGetNumeric(DirectFunctionCall2(numeric_add,  NumericGetDatum(int64_to_numeric(date)),
																NumericGetDatum(mystimes)));
	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(mystimes)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}
Datum mystimes_mystimestamps(PG_FUNCTION_ARGS)
{
	Numeric		mystimes = PG_GETARG_NUMERIC(0);
	int32		dec = PG_GETARG_INT32(1);
	Numeric		num;
	char		*buf = NULL;
	TimeADT		date = 0;
	struct pg_tm tm;

	GetCurrentDateTime(&tm);
	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	date = date*1000000;
	mystimes = DatumGetNumeric(DirectFunctionCall2(numeric_add,  NumericGetDatum(int64_to_numeric(date)),
																NumericGetDatum(mystimes)));
	buf = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(mystimes)));
	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
												CStringGetDatum(buf),
												ObjectIdGetDatum(InvalidOid),
												Int32GetDatum((((14+dec) << 16)|dec) + VARHDRSZ)));


	PG_RETURN_NUMERIC(num);
}



