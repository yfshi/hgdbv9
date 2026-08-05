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

PG_FUNCTION_INFO_V1(mysdate_cmp);
PG_FUNCTION_INFO_V1(mysdate_hash);
PG_FUNCTION_INFO_V1(mys_date_hash_extended);
PG_FUNCTION_INFO_V1(mysdate_bit_count);

PG_FUNCTION_INFO_V1(mysdate_boolean);
PG_FUNCTION_INFO_V1(date_mysdate);
PG_FUNCTION_INFO_V1(timestamp_mysdate);
PG_FUNCTION_INFO_V1(timestamptz_mysdate);

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
		datetime[13] != ':' || datetime[16] != ':')
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

	PG_RETURN_INT64(dateVal1);
}

Datum mysdate_numeric(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

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
	char *str = PG_GETARG_CSTRING(0);
	DateADT date = MYSQL_INVAILD_DATE;
	bool is_insert = CALL_IN_INSERT();	/* bug#z8274 */
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr;
	mysql_dttype ftype;
	int flag = TIME_FLAG_INIT;
	int warn = 0;
	int nanoseconds = 0;
	bool nonsec = false;
	int fsec_len = 0;

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
		{
			/* Begin - bug#z8274 */
			if (is_insert)
				ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Incorrect datetime value: '%s'", str)));
			/* End - bug#z8274 */
			else
				ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));
		}

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (MYSQL_INVAILD_DATE == date)
	{
		date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	}
	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		date = MYSQL_ZERO_DATE;
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
	DateADT date = PG_GETARG_DATEADT(0);
	char *result = NULL;
	struct pg_tm tt,
		*tm = &tt;
	char buf[MAXDATELEN + 1] = {0};

	if (date == MYSQL_ZERO_DATE)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00");
	}
	else
	{
		int64 part1, part2;

		part1 = (date / 10000);
		part2 = (date - part1 * 10000);
		tm->tm_year = part1;

		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;
		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d", tm->tm_year, tm->tm_mon, tm->tm_mday);
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
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);
	DateADT result;

	result = (DateADT)pq_getmsgint(buf, sizeof(DateADT));

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		/* ok */;
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
	DateADT date = PG_GETARG_DATEADT(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, date);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

static DateADT
int_mysdate_internal(int64 arg, bool is_insert, char *msg)
{
	DateADT date = MYSQL_INVAILD_DATE;
	fsec_t fsec = 0;
	int dterr, flag, warn = 0;
	mysql_dttype ftype;
	struct pg_tm tt, *tm = &tt;

	MYS_ZERO_tm(tm);

	if (arg == 0)
	{
		date = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_insert)
		{
			bool is_strict_mode = false;
			flag = 1;

			/* Get the sql_mode parameter */
			if (has_sql_mode(MODE_NO_ZERO_DATE))
				flag |= TIME_NO_ZERO_DATE;
			if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
				flag |= TIME_NO_ZERO_IN_DATE;
			if (has_sql_mode(MODE_INVALID_DATES))
				flag |= TIME_INVALID_DATES;
			if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
				has_sql_mode(MODE_STRICT_ALL_TABLES))
				is_strict_mode = true;

			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				if (is_strict_mode) /* strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_TRUNCATED |
								MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
					{
						elog(ERROR, "Incorrect DATE value: '%lld'", (long long)arg);
					}
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						date = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
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
				if (!non_zero_date(tm))
					date = MYSQL_ZERO_DATE;

				if (date != MYSQL_ZERO_DATE)
					date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
			}
		}
		else
		{
			flag = 9;
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				/* Begin - bug#z5068 */
				if (msg)
					ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", msg)));
				/* End - bug#z5068 */
				else
				{
					ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", (long long)arg)));
				}

				if (dterr < 0)
					return MYSQL_INVAILD_DATE;
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
	int64 arg = PG_GETARG_INT64(0);
	bool is_insert = CALL_IN_INSERT();
	DateADT ret;

	ret = int_mysdate_internal(arg, is_insert, NULL);
	if (ret == MYSQL_INVAILD_DATE)
		PG_RETURN_NULL();
	PG_RETURN_DATEADT(ret);
}

/* numeric_mysdate()
 * Convert numeric to mysql date data type.
 */
Datum
numeric_mysdate(PG_FUNCTION_ARGS)
{
	Numeric num = PG_GETARG_NUMERIC(0);
	int64 n = 0;
	int64 len;
	DateADT ret = 0;
	char *str, *tmpstr;
	;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while (*str && *str != '.')
	{
		len++;
		if (len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		n = n * 10 + *str - '0';
		str++;
	}

	ret = int_mysdate_internal(n, false, tmpstr);
	if (ret == MYSQL_INVAILD_DATE)
		PG_RETURN_NULL();

	PG_RETURN_DATEADT(ret);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysdate_eq(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mysdate_ne(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mysdate_lt(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mysdate_le(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mysdate_gt(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mysdate_ge(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mysdate_seq(PG_FUNCTION_ARGS)
{
	bool res;

	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		DateADT dateVal1 = PG_GETARG_DATEADT(0);
		DateADT dateVal2 = PG_GETARG_DATEADT(1);
		res = (dateVal1 == dateVal2);
	}

	PG_RETURN_BOOL(res);
}


Datum
mysdate_seqstr(PG_FUNCTION_ARGS)
{
	bool res;
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(),
												   PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
}


/*****************************************************************************
 *	 B-tree index support procedure
 *****************************************************************************/
Datum
mysdate_cmp(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

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

//BIT_COUNT
Datum
mysdate_bit_count(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);

	PG_RETURN_INT64(getCount(dateVal1));
}

Datum
mysdate_boolean(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);

	if (!dateVal1)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != 0);
}

Datum
date_mysdate(PG_FUNCTION_ARGS)
{
	DateADT dateVal = PG_GETARG_DATEADT(0);
	DateADT date;
	struct pg_tm tt, *tm = &tt;

	j2date(dateVal + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

	PG_RETURN_DATEADT(date);
}

Datum
timestamp_mysdate(PG_FUNCTION_ARGS)
{
	Timestamp	timestamp = PG_GETARG_TIMESTAMP(0);
	DateADT		date;
	struct pg_tm tt, *tm = &tt;
	fsec_t		fsec;

	if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	}

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	
	PG_RETURN_DATEADT(date);
}

Datum
timestamptz_mysdate(PG_FUNCTION_ARGS)
{
	TimestampTz	dt = PG_GETARG_TIMESTAMPTZ(0);
	DateADT		date;
	struct pg_tm tt, *tm = &tt;
	int			tz;
	fsec_t		fsec;
	int			dterr;
	int			warn;

	if (timestamp2tm(dt, &tz, tm, &fsec, NULL, NULL) != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	
	PG_RETURN_DATEADT(date);
}

//like
Datum mysdate2_like(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum mysdate_text_like(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	text *file = PG_GETARG_TEXT_PP(1);

	char *str = text_to_cstring(file);
	DateADT date = 0;
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr;
	mysql_dttype ftype;
	int flag = TIME_FLAG_INIT;
	int warn = 0;
	int nanoseconds = 0;
	bool nonsec = false;
	TimeOffset time = 0;
	int64 datetemp = 0;
	int fsec_len = 0;

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("date function: date field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect date value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("date field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	date = datetemp + time;

	PG_RETURN_BOOL(date == dateVal1);
}

Datum mysdate_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mysdate2_notlike(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	DateADT dateVal2 = PG_GETARG_DATEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum mysdate_text_notlike(PG_FUNCTION_ARGS)
{
	DateADT dateVal1 = PG_GETARG_DATEADT(0);
	text *file = PG_GETARG_TEXT_PP(1);

	char *str = text_to_cstring(file);
	DateADT date = 0;
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr;
	mysql_dttype ftype;
	int flag = TIME_FLAG_INIT;
	int warn = 0;
	int nanoseconds = 0;
	bool nonsec = false;
	TimeOffset time = 0;
	int64 datetemp = 0;
	int fsec_len = 0;

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("date function: date field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect date value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect date value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("date field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	date = datetemp + time;

	PG_RETURN_BOOL(date != dateVal1);
}
Datum mysdate_int8_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(mysdate_larger);
PG_FUNCTION_INFO_V1(mysdate_smaller);
/* AGGREGATE */
Datum
mysdate_larger(PG_FUNCTION_ARGS)
{
	DateADT val1 = PG_GETARG_DATEADT(0);
	DateADT val2 = PG_GETARG_DATEADT(1);

	PG_RETURN_DATEADT(val1 > val2 ? val1 : val2);
}

Datum
mysdate_smaller(PG_FUNCTION_ARGS)
{
	DateADT val1 = PG_GETARG_DATEADT(0);
	DateADT val2 = PG_GETARG_DATEADT(1);

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
PG_FUNCTION_INFO_V1(date_mysdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_eq);
PG_FUNCTION_INFO_V1(mysdatetime_ne);
PG_FUNCTION_INFO_V1(mysdatetime_lt);
PG_FUNCTION_INFO_V1(mysdatetime_gt);
PG_FUNCTION_INFO_V1(mysdatetime_le);
PG_FUNCTION_INFO_V1(mysdatetime_ge);
PG_FUNCTION_INFO_V1(mysdatetime_seq);
PG_FUNCTION_INFO_V1(mysdatetime_seqstr);

PG_FUNCTION_INFO_V1(mysdatetime_cmp);
PG_FUNCTION_INFO_V1(mysdatetime_hash);
PG_FUNCTION_INFO_V1(mysdatetime_hash_extended);

PG_FUNCTION_INFO_V1(mysdatetime_negadatetime);
PG_FUNCTION_INFO_V1(mysdatetime_bit_count);
PG_FUNCTION_INFO_V1(mysdatetime_invertmysdatetime);

PG_FUNCTION_INFO_V1(mysdatetime_boolean);

PG_FUNCTION_INFO_V1(mysdatetime2_like);
PG_FUNCTION_INFO_V1(mysdatetime_text_like);
PG_FUNCTION_INFO_V1(mysdatetime_int8_like);
PG_FUNCTION_INFO_V1(mysdatetime2_notlike);
PG_FUNCTION_INFO_V1(mysdatetime_text_notlike);
PG_FUNCTION_INFO_V1(mysdatetime_int8_notlike);

PG_FUNCTION_INFO_V1(mysdatetime_numeric);
Datum mysdatetime_numeric(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);

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
	char *str = PG_GETARG_CSTRING(0);
	TimeADT datetime = MYSQL_INVAILD_DATE;
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr = 0;
	mysql_dttype ftype = 0;
	int flag = TIME_FLAG_INIT;
	int warn = 0;
	int nanoseconds = 0;
	bool nonsec = false;
	TimeOffset time = 0;
	int64 datetemp = 0;
	int fsec_len = 0;

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
	TimeADT datetime = PG_GETARG_TIMEADT(0);
	char *result;
	struct pg_tm tt,
		*tm = &tt;
	char buf[MAXDATELEN + 1] = {0};

	if (datetime == MYSQL_ZERO_DATE)
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
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);
	TimeADT result;

	result = (TimeADT)pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		/* ok */;
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
	TimeADT datetime = PG_GETARG_TIMEADT(0);
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
	Numeric num = PG_GETARG_NUMERIC(0);
	int64 datetimetemp = 0;
	int64 len;
	char *str, *tmpstr;
	TimeADT datetime = MYSQL_INVAILD_DATE;
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr;
	mysql_dttype ftype;
	int flag = 0;
	int warn = 0;
	TimeOffset time = 0;

	MemSet(tm, 0, sizeof(struct pg_tm));
	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	len = 0;
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	tmpstr = pstrdup(str);
	while (*str && *str != '.')
	{
		len++;
		if (len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		datetimetemp = datetimetemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	str++; /* jump '.' */
	for (int i = 0; i < 6; i++)
	{
		int num = 0;
		if (*str != '\0')
		{
			num = *str - '0';
			str++;
		}
		fsec = fsec * 10 + num;
	}

	if (datetimetemp == 0)
	{
		datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect DATETIME value: '%ld'", datetimetemp);
					if (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect DATETIME value: '%ld'", datetimetemp);
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						datetime = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						datetime = MYSQL_ZERO_DATE;
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
					datetime = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect DATETIME value:'%s'", tmpstr)));
				}

				/* check datetime is zero. */
				if (!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("DATETIME field overflow.")));
					PG_RETURN_NULL();
				}
				if (datetime != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect DATETIME value: '%ld'", datetimetemp)));
				if (dterr < 0)
					PG_RETURN_NULL();
			}
			// dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if (mysdatetime_rounded_seconds(tm, &fsec, &warn))
			{
				ereport(WARNING, (errmsg("DATETIME field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
	int64 arg = PG_GETARG_INT64(0);
	TimeADT datetime = MYSQL_INVAILD_DATE;
	fsec_t fsec = 0;
	struct pg_tm tt,
		*tm = &tt;
	int dterr;
	mysql_dttype ftype;
	int flag = 0;
	int warn = 0;
	TimeOffset time = 0;

	if (arg == 0)
	{
		datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_TRUNCATED |
								MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
					{
						elog(ERROR, "Incorrect DATETIME value: '%lld'", (long long)arg);
					}
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATE;
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
					datetime = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check datetime is zero. */
				if (!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("DATETIME field overflow.")));
					PG_RETURN_NULL();
				}
				if (datetime != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", (long long)arg)));

				if (dterr < 0)
					PG_RETURN_NULL();
			}
			// dterr = mysdatetime_rounded_seconds(tm, &fsec,&warn);
			if (mysdatetime_rounded_seconds(tm, &fsec, &warn))
			{
				ereport(WARNING, (errmsg("DATETIME field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

	PG_RETURN_TIMEADT(datetime);
}

Datum
date_mysdatetime(PG_FUNCTION_ARGS)
{
	DateADT dateVal = PG_GETARG_DATEADT(0);
	struct pg_tm tt, *tm = &tt;
	TimeADT datetime, date = 0;

	j2date(dateVal + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	datetime = date * 1000000;

	PG_RETURN_TIMEADT(datetime);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysdatetime_eq(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mysdatetime_ne(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mysdatetime_lt(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mysdatetime_le(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mysdatetime_gt(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mysdatetime_ge(PG_FUNCTION_ARGS)
{
	TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mysdatetime_seq(PG_FUNCTION_ARGS)
{
	bool res;
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		TimeADT dateVal1 = PG_GETARG_TIMEADT(0);
		TimeADT dateVal2 = PG_GETARG_TIMEADT(1);
		res = (dateVal1 == dateVal2);
	}

	PG_RETURN_BOOL(res);
}

Datum
mysdatetime_seqstr(PG_FUNCTION_ARGS)
{
	bool res;
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(),
												   PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
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

// nega
Datum mysdatetime_negadatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();

	PG_RETURN_UINT64(~dateVal1);
}

Datum mysdatetime_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_INT64(getCount(dateVal1));
}

// invert ! 
Datum mysdatetime_invertmysdatetime(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_INT64(!dateVal1);
}

// datetime to boolean
Datum mysdatetime_boolean(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();

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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	PG_RETURN_BOOL(datetime == dateVal1);
}

Datum mysdatetime_int8_like(PG_FUNCTION_ARGS)
{
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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("DATETIME field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	PG_RETURN_BOOL(datetime != dateVal1);
}
Datum mysdatetime_int8_notlike(PG_FUNCTION_ARGS)
{
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

PG_FUNCTION_INFO_V1(mysdatetimes_cmp);
PG_FUNCTION_INFO_V1(mysdatetimes_hash);
PG_FUNCTION_INFO_V1(mysdatetimes_hash_extended);

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

PG_FUNCTION_INFO_V1(mysdatetimes_bit_count);


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

	TimeADT		datetime = MYSQL_INVAILD_DATE;
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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

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
	int64		datetime = MYSQL_INVAILD_DATE;
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

	// Process rounding
	if (fsec >= 500000)
		datetime = datetime - 1;

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if (str == NULL)
		str = "";

	if (datetime == MYSQL_ZERO_DATE && fsec == 0)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00%s", str);
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

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d%s", tm->tm_year, tm->tm_mon, tm->tm_mday,
				 tm->tm_hour, tm->tm_min, tm->tm_sec, str);
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

	return DirectFunctionCall3(numeric_recv, 
							   NumericGetDatum(buf),
							   ObjectIdGetDatum(InvalidOid),
							   Int32GetDatum(-1));
}

/*
 *mysdatetimes_send			- converts datetime to binary format
 */
Datum
mysdatetimes_send(PG_FUNCTION_ARGS)
{
	Numeric		datetime = PG_GETARG_NUMERIC(0);

	return DirectFunctionCall1(numeric_send, NumericGetDatum(datetime));
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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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
	while (*str && *str != '.')
	{
		len++;
		datetimetemp = datetimetemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	if (*str == '.') /* BUGZ#5610 */
	{
		str++; /* jump '.' */
		for (int i = 0; i < 9; i++)
		{
			int numtemp = 0;
			if (*str != '\0')
			{
				numtemp = *str - '0';
				str++;
			}
			nanoseconds = nanoseconds * 10 + numtemp;
		}
	}

	if (datetimetemp == 0)
	{
		datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect datetime value: '%s'", tmpstr);
					if (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect datetime value: '%s'", tmpstr);
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						datetime = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						datetime = MYSQL_ZERO_DATE;
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
						warn |= MYSQL_TIME_WARN_TRUNCATED;
				}
				else if (!(flag & TIME_NO_NSEC_ROUNDING))
				{
					fsec = (nanoseconds + 500) / 1000;
					(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
					fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
				}

				if (warn & MYSQL_TIME_WARN_TRUNCATED)
					ereport(WARNING, (errmsg("Data truncated for column.")));
				if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
							MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
				{
					ereport(WARNING, (errmsg("Out of range value.")));
					datetime = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect datetime value:'%s'", tmpstr)));
				}

				/* check datetime is zero. */
				if (!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("datetime field overflow.")));
					PG_RETURN_NULL();
				}
				if (datetime != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(datetimetemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
				if (dterr < 0)
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
					warn |= MYSQL_TIME_WARN_TRUNCATED;
			}
			else if (!(flag & TIME_NO_NSEC_ROUNDING))
			{
				fsec = (nanoseconds + 500) / 1000;
				(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
				fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
			}

			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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

	if (arg == 0)
	{
		datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_TRUNCATED |
								MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
					{
						elog(ERROR, "Incorrect datetime value: '%lld'", (long long)arg);
					}
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						datetime = MYSQL_ZERO_DATE;
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
					datetime = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check datetime is zero. */
				if (!non_zero_date(tm))
					datetime = MYSQL_ZERO_DATE;

				if (datetime != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", (long long)arg)));

				if (dterr < 0)
					PG_RETURN_NULL();
			}

			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			datetime = temp + time;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%d'", pgdate)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}

	if (datetime != MYSQL_ZERO_DATE)
	{
		datetime = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		datetime = datetime * 1000000;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}

	if (datetime != MYSQL_ZERO_DATE)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;
	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}
	if (datetime != MYSQL_ZERO_DATE)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	time2tm(pgtime, tm, &fsec);

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			time = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtime)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_DATE;
		}
	}
	if (time != MYSQL_ZERO_DATE)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

Datum mysdatetimes_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
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
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
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
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
PG_FUNCTION_INFO_V1(date_mystimestamp);
PG_FUNCTION_INFO_V1(mystimestamp_eq);
PG_FUNCTION_INFO_V1(mystimestamp_ne);
PG_FUNCTION_INFO_V1(mystimestamp_lt);
PG_FUNCTION_INFO_V1(mystimestamp_gt);
PG_FUNCTION_INFO_V1(mystimestamp_le);
PG_FUNCTION_INFO_V1(mystimestamp_ge);
PG_FUNCTION_INFO_V1(mystimestamp_seq);
PG_FUNCTION_INFO_V1(mystimestamp_seqstr);

PG_FUNCTION_INFO_V1(mystimestamp_cmp);
PG_FUNCTION_INFO_V1(mystimestamp_hash);
PG_FUNCTION_INFO_V1(mystimestamp_hash_extended);

PG_FUNCTION_INFO_V1(mystimestamp_negatimestamp);
PG_FUNCTION_INFO_V1(mystimestamp_bit_count);

PG_FUNCTION_INFO_V1(mystimestamp_invertmystimestamp);

PG_FUNCTION_INFO_V1(mystimestamp_boolean);

PG_FUNCTION_INFO_V1(mystimestamp2_like);
PG_FUNCTION_INFO_V1(mystimestamp_text_like);
PG_FUNCTION_INFO_V1(mystimestamp_int8_like);
PG_FUNCTION_INFO_V1(mystimestamp2_notlike);
PG_FUNCTION_INFO_V1(mystimestamp_text_notlike);
PG_FUNCTION_INFO_V1(mystimestamp_int8_notlike);

PG_FUNCTION_INFO_V1(timestamp_mystimestamp); /* SRS-MYSQL-PROC */

Datum
mystimestamp_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		timestamp = MYSQL_ZERO_DATE;
	}

	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

	if (timestamp == MYSQL_ZERO_DATE)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00");
	}
	else
	{
		int64 part1, part2;
		DateADT date = 0;
		date = timestamp / 1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = timestamp % 1000000 / 10000;
		tm->tm_min = timestamp % 1000000 % 10000 / 100;
		tm->tm_sec = timestamp % 1000000 % 10000 % 100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year, tm->tm_mon, tm->tm_mday,
				 tm->tm_hour, tm->tm_min, tm->tm_sec);
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

	result = (TimeADT)pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		/* ok */;
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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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
	while (*str && *str != '.')
	{
		len++;
		if (len > 14)
		{
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", tmpstr)));
			PG_RETURN_NULL();
		}
		timestamptemp = timestamptemp * 10 + *str - '0';
		str++;
	}

	/* Calculate the time */
	str++; /* jump '.' */
	for (int i = 0; i < 6; i++)
	{
		int num = 0;
		if (*str != '\0')
		{
			num = *str - '0';
			str++;
		}
		fsec = fsec * 10 + num;
	}

	if (timestamptemp == 0)
	{
		timestamp = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
					if (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						timestamp = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						timestamp = MYSQL_ZERO_DATE;
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
					timestamp = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIMESTAMP value:'%s'", tmpstr)));
				}

				/* check timestamp is zero. */
				if (!non_zero_date(tm))
					timestamp = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();
				}
				if (timestamp != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIMESTAMP value: '%ld'", timestamptemp)));
				if (dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
			if (dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
	fsec_t		fsec = 0;
	struct pg_tm tt,
			   *tm = &tt;
	int			dterr;
	mysql_dttype ftype;
	int 		flag = 0;
	int			warn = 0;
	TimeOffset	time = 0;

	if (arg == 0)
	{
		timestamp = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_TRUNCATED |
								MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
					{
						elog(ERROR, "Incorrect TIMESTAMP value: '%lld'", (long long)arg);
					}
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_DATE;
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
					timestamp = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check timestamp is zero. */
				if (!non_zero_date(tm))
					timestamp = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();
				}
				if (timestamp != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", (long long)arg)));

				if (dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
			if (dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}
	temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	temp = temp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	res = temp + time;

	PG_RETURN_TIMEADT(res);
}

Datum
date_mystimestamp(PG_FUNCTION_ARGS)
{
	DateADT dateVal = PG_GETARG_DATEADT(0);
	struct pg_tm tt, *tm = &tt;
	TimeADT timestamp, datetemp = 0;

	j2date(dateVal + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	datetemp = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
	timestamp = datetemp * 1000000;

	PG_RETURN_TIMEADT(timestamp);
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

	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mystimestamp_le(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mystimestamp_gt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mystimestamp_ge(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	if (!dateVal1 || !dateVal2)
		PG_RETURN_NULL();

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
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		res = DatumGetBool(DirectFunctionCall2Coll(texteq, PG_GET_COLLATION(),
												   PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)));
	}
	PG_RETURN_BOOL(res);
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

// nega
Datum mystimestamp_negatimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	if(!dateVal1 )
		PG_RETURN_NULL();

	PG_RETURN_UINT64(~dateVal1);
}

Datum mystimestamp_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_INT64(getCount(dateVal1));
}

// invert ! 
Datum mystimestamp_invertmystimestamp(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	PG_RETURN_BOOL(timestamp == dateVal1);
}

Datum mystimestamp_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

Datum mystimestamp2_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum mystimestamp_text_notlike(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(1);
	text	   *file = PG_GETARG_TEXT_PP(0);

	char	   *str = text_to_cstring(file);
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	PG_RETURN_BOOL(timestamp != dateVal1);
}
Datum mystimestamp_int8_notlike(PG_FUNCTION_ARGS)
{
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

PG_FUNCTION_INFO_V1(mystimestamps_cmp);
PG_FUNCTION_INFO_V1(mystimestamps_hash);
PG_FUNCTION_INFO_V1(mystimestamps_hash_extended);

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

PG_FUNCTION_INFO_V1(mystimestamps_bit_count);

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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	MemSet(tm, 0, sizeof(struct pg_tm));
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		timestamp = MYSQL_ZERO_DATE;
	}

	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
		PG_RETURN_NULL();
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	timestamp = datetemp + time;

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)timestamp, fsec);

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

	// Process rounding
	if (fsec >= 500000)
		timestamp = timestamp - 1;

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if (str == NULL)
		str = "";

	if (timestamp == MYSQL_ZERO_DATE && fsec == 0)
	{
		snprintf(buf, MAXDATELEN, "0000-00-00 00:00:00.000000");
	}
	else
	{
		int64 part1, part2;
		DateADT date = 0;
		date = timestamp / 1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		tm->tm_year = part1;
		tm->tm_mon = part2 / 100;
		tm->tm_mday = part2 % 100;

		tm->tm_hour = timestamp % 1000000 / 10000;
		tm->tm_min = timestamp % 1000000 % 10000 / 100;
		tm->tm_sec = timestamp % 1000000 % 10000 % 100;

		snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d%s", tm->tm_year, tm->tm_mon, tm->tm_mday,
				 tm->tm_hour, tm->tm_min, tm->tm_sec, str);
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

	return DirectFunctionCall3(numeric_recv, 
							   NumericGetDatum(buf),
							   ObjectIdGetDatum(InvalidOid),
							   Int32GetDatum(-1));
}

/*
 *mystimestamps_send			- converts timestamp to binary format
 */
Datum
mystimestamps_send(PG_FUNCTION_ARGS)
{
	Numeric         datetime = PG_GETARG_NUMERIC(0);

	return DirectFunctionCall1(numeric_send, NumericGetDatum(datetime));
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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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
	while (*str && *str != '.')
	{
		len++;
		if (len > 14)
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
		for (int i = 0; i < 6; i++)
		{
			int num = 0;
			if (*str != '\0')
			{
				num = *str - '0';
				str++;
			}
			fsec = fsec * 10 + num;
		}
	}

	if (timestamptemp == 0)
	{
		timestamp = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
					if (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
						elog(ERROR, "Incorrect TIMESTAMP value: '%ld'", timestamptemp);
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("Out of range value.")));
						timestamp = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("Data truncated for column.")));
						timestamp = MYSQL_ZERO_DATE;
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
					timestamp = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("Incorrect TIMESTAMP value:'%s'", tmpstr)));
				}

				/* check timestamp is zero. */
				if (!non_zero_date(tm))
					timestamp = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();
				}
				if (timestamp != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(timestamptemp, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect TIMESTAMP value: '%ld'", timestamptemp)));
				if (dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
			if (dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
	TimeADT		timestamp = MYSQL_INVAILD_DATE;
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

	if (arg == 0)
	{
		timestamp = MYSQL_ZERO_DATE;
	}
	else
	{
		/* Begin - SRS-MYSQL-HEX-LITERAL */
		bool is_insert = CALL_IN_INSERT();
		/* End - SRS-MYSQL-HEX-LITERAL */

		if (is_insert)
		{
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (non_zero_time(tm, fsec))
				warn |= MYSQL_TIME_NOTE_TRUNCATED;

			if (dterr < 0) /* Returns an error result, the value is not 0. */
			{
				bool is_strict_mode = false;
				flag = 1;

				/* Get the sql_mode parameter */
				if (has_sql_mode(MODE_NO_ZERO_DATE))
					flag |= TIME_NO_ZERO_DATE;
				if (has_sql_mode(MODE_NO_ZERO_IN_DATE))
					flag |= TIME_NO_ZERO_IN_DATE;
				if (has_sql_mode(MODE_INVALID_DATES))
					flag |= TIME_INVALID_DATES;
				if (has_sql_mode(MODE_STRICT_TRANS_TABLES) ||
					has_sql_mode(MODE_STRICT_ALL_TABLES))
					is_strict_mode = true;

				if (is_strict_mode) /* strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_TRUNCATED |
								MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE))
					{
						elog(ERROR, "Incorrect TIMESTAMP value: '%lld'", (long long)arg);
					}
				}
				else /* non-strict mode */
				{
					if (warn & (MYSQL_TIME_WARN_ZERO_DATE |
								MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_DATE;
					}
					if (warn & MYSQL_TIME_WARN_TRUNCATED)
					{
						ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
						timestamp = MYSQL_ZERO_DATE;
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
					timestamp = MYSQL_ZERO_DATE;
				}
				if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
					!(warn & MYSQL_TIME_WARN_TRUNCATED))
				{
					ereport(WARNING, (errmsg("ERROR,You have an error in your SQL syntax.")));
				}

				/* check timestamp is zero. */
				if (!non_zero_date(tm))
					timestamp = MYSQL_ZERO_DATE;

				dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
				if (dterr)
				{
					ereport(WARNING, (errmsg("timestamp field overflow.")));
					PG_RETURN_NULL();
				}
				if (timestamp != MYSQL_ZERO_DATE)
				{
					int64 temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
			dterr = number_to_datetime(arg, tm, flag, &fsec, &ftype, &warn);
			if (dterr < 0 || warn)
			{
				ereport(WARNING, (errmsg("Incorrect datetime value: '%lld'", (long long)arg)));

				if (dterr < 0)
					PG_RETURN_NULL();
			}
			dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
			if (dterr)
			{
				ereport(WARNING, (errmsg("timestamp field overflow.")));
				PG_RETURN_NULL();
			}
			temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
			temp = temp * 1000000;
			time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
			timestamp = temp + time;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)timestamp, fsec);

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_DATE;
	}

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%d'", pgdate)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}

	if (datetime != MYSQL_ZERO_DATE)
	{
		datetime = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		datetime = datetime * 1000000;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_DATE;
	}

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}

	if (datetime != MYSQL_ZERO_DATE)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;
	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		datetime = MYSQL_ZERO_DATE;
	}

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			datetime = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			datetime = MYSQL_ZERO_DATE;
		}
	}
	if (datetime != MYSQL_ZERO_DATE)
	{
		temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
		temp = temp * 1000000;
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
		datetime = temp + time;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)datetime, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	time2tm(pgtime, tm, &fsec);

	(void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[dec]);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

	if (datetime_with_no_zero_in_date_to_timeval(tm, fsec, &warn))
	{
		time = MYSQL_ZERO_DATE;
	}

	if (!warn)
	{
		if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
			time = MYSQL_ZERO_DATE;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", pgtime)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_DATE;
		}
	}
	if (time != MYSQL_ZERO_DATE)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

Datum mystimestamps_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
	if (dterr || warn || nonsec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nonsec)
		{
			ereport(WARNING, (errmsg("timestamp function: timestamp field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect timestamp value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

		// ERROR 1064 (42000): You have an error in your SQL syntax
		if (dterr < 0 || nonsec)
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("timestamp field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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
PG_FUNCTION_INFO_V1(mystime_cmp);
PG_FUNCTION_INFO_V1(mystime_hash);
PG_FUNCTION_INFO_V1(mystime_hash_extended);
PG_FUNCTION_INFO_V1(mystime_neg);

PG_FUNCTION_INFO_V1(mystime_bit_count);
PG_FUNCTION_INFO_V1(mystime_invert);
PG_FUNCTION_INFO_V1(mystime_boolean);

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
	int			warn = 0;
	int			nanoseconds = 0;
	bool		nanosec = false;
	TimeOffset	time = MYSQL_INVAILD_DATE;
	int neg = 0;

#ifdef NOT_USED
	Oid typelem = PG_GETARG_OID(1);
#endif

	dterr = str_to_time(str, strlen(str), tm, &fsec, &ftype, &warn, &nanoseconds, &neg);
	nanosec = time_add_nanoseconds_with_round(tm, &fsec, nanoseconds, &warn);
	dterr |= mystime_rounded_seconds(tm, &fsec, &warn, 0);
	if (dterr || warn || nanosec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nanosec)
		{
			ereport(WARNING, (errmsg("time function: time field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect time value: '%s'", str)));

		if (dterr < 0 || nanosec)
		{
			PG_RETURN_NULL();
		}
	}

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	if (neg)
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

	if (time < 0)
	{
		neg = 1;
		time = -time;
	}

	if (time == MYSQL_ZERO_TIME)
	{
		snprintf(buf, MAXDATELEN, "00:00:00");
	}
	else
	{
		tm->tm_hour = (time / 10000) % 1000;
		tm->tm_min = (time / 100) % 100;
		tm->tm_sec = time % 100;

		snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d", neg == 1 ? "-" : "", tm->tm_hour, tm->tm_min, tm->tm_sec);
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

	result = (TimeADT)pq_getmsgint64(buf);

	/* Limit to the same range that date_in() accepts. */
	if (DATE_NOT_FINITE(result))
		/* ok */;
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

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
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

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
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

	PG_RETURN_BOOL(dateVal1 == dateVal2);
}

Datum
mystime_ne(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 != dateVal2);
}

Datum
mystime_lt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 < dateVal2);
}

Datum
mystime_le(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
mystime_gt(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 > dateVal2);
}

Datum
mystime_ge(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);
	TimeADT		dateVal2 = PG_GETARG_TIMEADT(1);

	PG_RETURN_BOOL(dateVal1 >= dateVal2);
}

Datum
mystime_seq(PG_FUNCTION_ARGS)
{
	bool		res;

	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		TimeADT 	dateVal1 = PG_GETARG_TIMEADT(0);
		TimeADT 	dateVal2 = PG_GETARG_TIMEADT(1);

		res = (dateVal1 == dateVal2);
	}

	PG_RETURN_BOOL(res);
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


// nega
Datum mystime_neg(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_UINT64(~dateVal1);
}
Datum mystime_bit_count(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_INT64(getCount(dateVal1));
}

// invert ! 
Datum mystime_invert(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_INT64(!dateVal1);
}

// time to boolean
Datum
mystime_boolean(PG_FUNCTION_ARGS)
{
	TimeADT		dateVal1 = PG_GETARG_TIMEADT(0);

	PG_RETURN_BOOL(dateVal1 != 0);
}

PG_FUNCTION_INFO_V1(mystimes_in);
PG_FUNCTION_INFO_V1(mystimes_out);
PG_FUNCTION_INFO_V1(mystimes_recv);
PG_FUNCTION_INFO_V1(mystimes_send);
PG_FUNCTION_INFO_V1(mystimes_typmodin);
PG_FUNCTION_INFO_V1(mystimes_typmodout);
PG_FUNCTION_INFO_V1(mystimes_scale);

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

PG_FUNCTION_INFO_V1(mystimes_cmp);
PG_FUNCTION_INFO_V1(mystimes_hash);
PG_FUNCTION_INFO_V1(mystimes_hash_extended);

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

PG_FUNCTION_INFO_V1(mystimes_bit_count);

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
Datum
mystimes_int8(PG_FUNCTION_ARGS)
{
	Numeric datetimeVal = PG_GETARG_NUMERIC(0);
	int64 retValue = 0;

	retValue = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(datetimeVal)));

	PG_RETURN_INT64(retValue);
}

static Datum
pg_tm_to_mys_time(struct pg_tm *tm, fsec_t fsec, bool neg, int dec)
{
	Datum 	ret;
	int64 	time;
	char	buf[MAXDATELEN + 1];

	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

	time = neg ? -time : time;

	if (dec == 0)
	{
		return TimeADTGetDatum(time);
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) time, fsec);

	ret = DirectFunctionCall3(numeric_in,
							  CStringGetDatum(buf),
							  ObjectIdGetDatum(InvalidOid),
							  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ));

	return ret;
}

Datum
mystimes_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	int32		typmod = PG_GETARG_INT32(2);
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	int 		dterr;
	mysql_dttype ftype;
	int 		warn = 0;
	int 		nanoseconds = 0;
	bool		nanosec = false;
	int 		neg = 0;

	/**
	 * Begin - SRS-MYSQL-BUILDINFUNCTION
	 * When do cast, typmod may be -1, it's illegal in this logic.
	 */
	if (typmod == -1)
		typmod = DATETIME_MAX_DECIMALS;
	/* End - SRS-MYSQL-BUILDINFUNCTION */

	memset(tm, 0, sizeof(struct pg_tm));

	dterr = str_to_time(str, strlen(str), tm, &fsec, &ftype, &warn, &nanoseconds, &neg);
	fsec += (nanoseconds + 500) / 1000;
	nanosec = time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[typmod], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];

	if (dterr || warn || nanosec)
	{
		if ((MYSQL_TIME_WARN_TRUNCATED & warn) && !dterr)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warn) && nanosec)
		{
			ereport(WARNING, (errmsg("time function: time field overflow")));
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		}
		else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warn)
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", str)));
		else
			ereport(WARNING, (errmsg("Incorrect time value: '%s'", str)));

		if (dterr < 0 || nanosec)
		{
			PG_RETURN_NULL();
		}
	}

	PG_RETURN_DATUM(pg_tm_to_mys_time(tm, fsec, neg, typmod));
}

/* 
 * mystimes_out
 * Given internal format date, convert to text string.
 */
Datum
mystimes_out(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		time = 0;
	char		*result;
	struct pg_tm tt,
				*tm = &tt;
	char		buf[MAXDATELEN + 1] = {0};
	char		*str = NULL;
	bool		neg = false;

	time = DatumGetInt64(DirectFunctionCall1(numeric_int8, 
											 DirectFunctionCall2(numeric_trunc,
																 NumericGetDatum(num),
																 Int32GetDatum(0))));

	/* Begin - bug#z5127 */
	if (numeric_is_neg(num))
	{
		if (time < 0)
			time = -time;
		neg = true;
	}
	/* End - bug#z5127 */

	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
	str = strchr(str, '.');
	if (str == NULL)
		str = "";

	tm->tm_hour = time / 10000;
	tm->tm_min = time % 10000 / 100;
	tm->tm_sec = time % 100;

	snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d%s", neg ? "-" : "", tm->tm_hour, tm->tm_min, tm->tm_sec, str);

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

	return DirectFunctionCall3(numeric_recv, 
							   NumericGetDatum(buf),
							   ObjectIdGetDatum(InvalidOid),
							   Int32GetDatum(-1));
}

/*
 *mystimes_send			- converts datetime to binary format
 */
Datum
mystimes_send(PG_FUNCTION_ARGS)
{
	Numeric         datetime = PG_GETARG_NUMERIC(0);

	return DirectFunctionCall1(numeric_send, NumericGetDatum(datetime));
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

static inline long
my_time_fraction_remainder(long nr, int32 decimals)
{
	Assert(decimals <= DATETIME_MAX_DECIMALS);
	return nr % (long)log_10_int[DATETIME_MAX_DECIMALS - decimals];
}

static inline void
my_time_trunc(fsec_t *fsec, int32 decimals)
{
	*fsec -= my_time_fraction_remainder(*fsec, decimals);
}

static bool
my_time_round(struct pg_tm *tm, fsec_t *fsec, int dec)
{
	int warnings;
	bool rc;

	rc = time_add_nanoseconds_with_round(tm, fsec, msec_round_add[dec], &warnings);

	my_time_trunc(fsec, dec);

	return rc;
}

/**
 * Return TRUE on error, FALSE on success
 */
static bool
lldiv_t_to_time(lldiv_t *lld, struct pg_tm *tm, fsec_t *fsec, int *neg, int *warnings)
{
	mysql_dttype ftype;
	int64 	time = lld->quot;
	int32 	nanosec = lld->rem > 0 ? lld->rem : -lld->rem;

	if (number_to_time(time, tm, fsec, &ftype, neg, warnings))
		return true;

	*fsec = nanosec / 1000;
	return time_add_nanoseconds_with_round(tm, fsec, nanosec % 1000, warnings);
}

static bool
convert_number_to_TIME(lldiv_t *lld, struct pg_tm *tm, fsec_t *fsec, int *neg, int *warnings)
{
	mysql_dttype ftype;
	int64 	time = lld->quot;
	int32 	nanosec = lld->rem > 0 ? lld->rem : -lld->rem;

	if (number_to_time(time, tm, fsec, &ftype, neg, warnings))
		return true;

	*fsec = 0;

	return !time_add_nanoseconds_with_round(tm, fsec, nanosec, warnings);
}

Datum
numeric_mystime(PG_FUNCTION_ARGS)
{
	char	*msg;
	Datum	num = PG_GETARG_DATUM(0);
	int32	dec = PG_GETARG_INT32(1);
	bool	explicit = PG_GETARG_BOOL(2);
	lldiv_t lld;
	int		neg = 0;
	int		warn = 0;
	fsec_t	fsec = 0;
	struct pg_tm tt, *tm = &tt;

	// Cast numeric to time or time(0)
	if (dec == -1)
		dec = 0;

	memset(tm, 0, sizeof(struct pg_tm));

	msg = DatumGetCString(DirectFunctionCall1(numeric_out, num));

	// round to nanosecond (10^9)
	num = DirectFunctionCall2(numeric, num, Int32GetDatum((((55 + 9) << 16) | 9) + VARHDRSZ));

	if (explicit)
	{
		/**
		 * if num is huge or some error occur when
		 * do lldiv_t_to_time
		 */
		if (!numeric_to_lldiv_t(DatumGetNumeric(num), &lld) ||
			lldiv_t_to_time(&lld, tm, &fsec, &neg, &warn))
		{
			ereport(WARNING, 
					(errmsg("Truncated incorrect time value: '%s'", msg)));

			PG_RETURN_NULL();
		}

		my_time_round(tm, &fsec, dec);
	}
	else
	{
		// not to produce warnings
		numeric_to_lldiv_t(DatumGetNumeric(num), &lld);

		if (convert_number_to_TIME(&lld, tm, &fsec, &neg, &warn))
		{
			my_time_round(tm, &fsec, dec);
		}

		if (non_zero_date(tm))
		{
			warn |= MYSQL_TIME_NOTE_TRUNCATED;
		}
	}

	if (warn)
	{
		if (explicit)
		{
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", msg)));
		}
		else if (is_strict_mode())
		{
			int level = ERROR;

			if (warn & MYSQL_TIME_NOTE_TRUNCATED)
				level = NOTICE;

			ereport(level, (errmsg("Incorrect time value: '%s'", msg)));
		}
		else if (warn & MYSQL_TIME_NOTE_TRUNCATED)
		{
			ereport(NOTICE, (errmsg("Data truncated for column")));
		}
		else
		{
			ereport(WARNING, (errmsg("Out of range value for column")));
		}
	}		

	PG_RETURN_DATUM(pg_tm_to_mys_time(tm, fsec, neg, dec));
}

Datum
int_mystime(PG_FUNCTION_ARGS)
{
	int64	num = PG_GETARG_INT64(0);
	int32	dec = PG_GETARG_INT32(1);
	bool	explicit = PG_GETARG_BOOL(2);

	int		neg = 0;
	int		warn = 0;
	fsec_t 	fsec = 0;
	mysql_dttype ftype;
	struct pg_tm tt, *tm = &tt;
	bool	rc;

	rc = number_to_time(num, tm, &fsec, &ftype, &neg, &warn);

	// Cast integer to time or time(0)
	if (dec == -1)
		dec = 0;

	if (explicit && rc)
	{
		ereport(WARNING, 
				(errmsg("Truncated incorrect time value: '%lld'", (long long) num)));

		PG_RETURN_NULL();
	}

	if (!explicit && non_zero_date(tm))
		warn |= MYSQL_TIME_NOTE_TRUNCATED;

	if (warn)
	{
		if (explicit)
		{
			ereport(WARNING, (errmsg("Truncated incorrect time value: '%lld'", (long long) num)));
		}
		else if (is_strict_mode())
		{
			int level = ERROR;

			if (warn & MYSQL_TIME_NOTE_TRUNCATED)
				level = NOTICE;

			ereport(level, (errmsg("Incorrect time value: '%lld'", (long long) num)));
		}
		else if (warn & MYSQL_TIME_NOTE_TRUNCATED)
		{
			ereport(NOTICE, (errmsg("Data truncated for column")));
		}
		else
		{
			ereport(WARNING, (errmsg("Out of range value for column")));
		}
	}		
	
	PG_RETURN_DATUM(pg_tm_to_mys_time(tm, fsec, neg, dec));
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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	j2date(pgdate + POSTGRES_EPOCH_JDATE,
		   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;

	(void)time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
	if (!warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%d'", pgdate)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
	if (!warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	if (timestamp2tm(pgtimestamp, &tz, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	(void)time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
	if (!warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtimestamp)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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

	if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
		(has_sql_mode(MODE_STRICT_ALL_TABLES)))
		is_strict_mode = true;

	MYS_ZERO_tm(tm); /* bug#7734 */

	time2tm(pgtime, tm, &fsec);

	(void)time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];
	if (!warn)
	{
		time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	}
	else
	{
		if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
		{
			ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", pgtime)));
		}
		if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
		{
			ereport(WARNING, (errmsg("Out of range value.")));
			time = MYSQL_ZERO_TIME;
		}
	}

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long)time, fsec);

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(buf),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

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
	bool		res;

	if(PG_ARGISNULL(0) && PG_ARGISNULL(1))
		res = true;
	else if(PG_ARGISNULL(0) || PG_ARGISNULL(1))
		res = false;
	else
	{
		Numeric 	dateVal1 = PG_GETARG_NUMERIC(0);
		Numeric 	dateVal2 = PG_GETARG_NUMERIC(1);
		res = DatumGetBool(DirectFunctionCall2(numeric_eq, NumericGetDatum(dateVal1),
							NumericGetDatum(dateVal2)));
	}

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

Datum mystimes_bit_count(PG_FUNCTION_ARGS)
{
	Numeric		dateVal1 = PG_GETARG_NUMERIC(0);
	int64		val1;
	
	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(dateVal1)));

	PG_RETURN_INT64(getCount(val1));
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}
	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
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
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	if (!isValidDatetimeFormat(str))
	{
		PG_RETURN_BOOL(false);
	}

	dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &ftype, &warn, &nanoseconds, &fsec_len);
	nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, nanoseconds);
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
			PG_RETURN_NULL();
	}

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
	{
		tm->tm_year = 1000;
		tm->tm_mon = 01;
		tm->tm_mday = 01;
	}

	dterr = mysdatetime_rounded_seconds(tm, &fsec, &warn);
	if (dterr)
	{
		ereport(WARNING, (errmsg("datetime field overflow.")));
	}
	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
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

/* Begin - bug#8693 */
PG_FUNCTION_INFO_V1(mysdatetime_mysdate);
PG_FUNCTION_INFO_V1(mysdatetime_mystime);
/* End - bug#8693 */


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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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
	TimeADT		datetime = MYSQL_INVAILD_DATE;
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
	TimeADT		datetime = MYSQL_INVAILD_DATE;

	datetime = mysdate*1000000;
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

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
	TimeADT		datetime = MYSQL_INVAILD_DATE;

	datetime = mysdate*1000000;
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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
	
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mysdatetime, fsec);

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
	
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mystime, fsec);

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
	
	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mystime, fsec);

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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) mystime, fsec);
	
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

/* Begin - ReqID: SRS-MYSQL-PROC */
Datum
timestamp_mystimestamp(PG_FUNCTION_ARGS)
{
	Timestamp	timestamp = PG_GETARG_TIMESTAMP(0);
	fsec_t		fsec;
	int64 		date;
	TimeOffset	time;
	TimeADT		result;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	}

	date = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) * 1000000L;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	result = date + time;

	PG_RETURN_TIMEADT(result);
}
/* End - ReqID: SRS-MYSQL-PROC */

/* Begin - bug#8693*/
Datum
mysdatetime_mysdate(PG_FUNCTION_ARGS)
{
	TimeADT datetime = PG_GETARG_TIMEADT(0);
	DateADT date = datetime / 1000000;

	PG_RETURN_DATEADT(date);
}

Datum
mysdatetime_mystime(PG_FUNCTION_ARGS)
{
	TimeADT datetime = PG_GETARG_TIMEADT(0);
	TimeOffset	time = datetime % 1000000;

	PG_RETURN_TIMEADT(time);
}
/* End - bug#8693*/