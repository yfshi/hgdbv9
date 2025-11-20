#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "postgres.h"
#include "access/hash.h"

#include "funcapi.h"
#include "libpq/pqformat.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/elog.h"
#include "utils/float.h"
#include "utils/fmgrprotos.h"
#include "utils/numeric.h"
#include "utils/timestamp.h"
#include "utils/guc.h" // Fix Bug#Z4800
#include "../include/ivorysql_my.h"

#define DatumGetYearADT(x)      ((YearADT)DatumGetInt16(x))
#define PG_GETARG_YEARADT(n)    DatumGetYearADT(PG_GETARG_DATUM(n))

#define MAX_YEAR_NUM    2155
#define MIN_YEAR_NUM    1901

#define MAX_YEAR2_NUM   99
#define MIN_YEAR2_NUM   0

#define TYPMODOUT_LEN   64
#define YEARSTRLEN		32

#define YEAR4_IN_RANGE(x)   ((x) >= MIN_YEAR_NUM && (x) <= MAX_YEAR_NUM)
#define YEAR_VALID(x)       (YEAR4_IN_RANGE(x) || YEAR2_IN_RANGE(x))

/* YEAR(2) -> YEAR(4) or YEAR(4) -> YEAR(2) */
#define YearADT_to_Year(x)  ((x) == 0 ? 0 : (int32)(x) + MIN_YEAR_NUM - 1)

/* Datetime limits */
/* lower bound: 1753-01-01 00:00:00.000 */
#define MIN_DATETIME	INT64CONST(-7794489600000000)
/* upper bond: 9999-12-31 23:59:29.999 */
#define END_DATETIME	INT64CONST(252455615999999000)
#define IS_VALID_DATETIME(t)  (MIN_DATETIME <= (t) && (t) < END_DATETIME)

PG_FUNCTION_INFO_V1(year_in);
PG_FUNCTION_INFO_V1(year_out);
PG_FUNCTION_INFO_V1(year_recv);
PG_FUNCTION_INFO_V1(year_send);
PG_FUNCTION_INFO_V1(yeartypmodin);
PG_FUNCTION_INFO_V1(yeartypmodout);

PG_FUNCTION_INFO_V1(year_eq);
PG_FUNCTION_INFO_V1(year_ne);
PG_FUNCTION_INFO_V1(year_le);
PG_FUNCTION_INFO_V1(year_lt);
PG_FUNCTION_INFO_V1(year_ge);
PG_FUNCTION_INFO_V1(year_gt);
PG_FUNCTION_INFO_V1(year_seq); /* <=> operator */
PG_FUNCTION_INFO_V1(year_cmp);
PG_FUNCTION_INFO_V1(year_sortsupport);
PG_FUNCTION_INFO_V1(year_hash);

PG_FUNCTION_INFO_V1(year_scale);
PG_FUNCTION_INFO_V1(int4_year);
PG_FUNCTION_INFO_V1(timestamptz_year);
PG_FUNCTION_INFO_V1(timestamptz_eq_year);
PG_FUNCTION_INFO_V1(year_eq_timestamptz);
PG_FUNCTION_INFO_V1(numeric_year);
PG_FUNCTION_INFO_V1(date_year);
PG_FUNCTION_INFO_V1(time_year);
/* Fix Bug#Z5406 begin */
PG_FUNCTION_INFO_V1(times_year);
/* Fix Bug#Z5406 end */
PG_FUNCTION_INFO_V1(timetz_year);
/* Fix Bug#Z5406 begin */
PG_FUNCTION_INFO_V1(datetime_year);
PG_FUNCTION_INFO_V1(datetimes_year);
PG_FUNCTION_INFO_V1(timestamp_year);
PG_FUNCTION_INFO_V1(timestamps_year);
/* Fix Bug#Z5406 end */
PG_FUNCTION_INFO_V1(char_year);
PG_FUNCTION_INFO_V1(bigint_year);
PG_FUNCTION_INFO_V1(year_lt_bigint);
PG_FUNCTION_INFO_V1(bigint_lt_year);
PG_FUNCTION_INFO_V1(year_gt_bigint);
PG_FUNCTION_INFO_V1(bigint_gt_year);

PG_FUNCTION_INFO_V1(year_larger);
PG_FUNCTION_INFO_V1(year_smaller);

PG_FUNCTION_INFO_V1(year_eq_int);
PG_FUNCTION_INFO_V1(year_ne_int);
PG_FUNCTION_INFO_V1(year_lt_int);
PG_FUNCTION_INFO_V1(year_le_int);
PG_FUNCTION_INFO_V1(year_gt_int);
PG_FUNCTION_INFO_V1(year_ge_int);
PG_FUNCTION_INFO_V1(year_add_int);
PG_FUNCTION_INFO_V1(year_sub_int);
PG_FUNCTION_INFO_V1(year_multi_int);
PG_FUNCTION_INFO_V1(year_div_int);

PG_FUNCTION_INFO_V1(int_eq_year);
PG_FUNCTION_INFO_V1(int_ne_year);
PG_FUNCTION_INFO_V1(int_lt_year);
PG_FUNCTION_INFO_V1(int_le_year);
PG_FUNCTION_INFO_V1(int_gt_year);
PG_FUNCTION_INFO_V1(int_ge_year);
PG_FUNCTION_INFO_V1(int_add_year);
PG_FUNCTION_INFO_V1(int_sub_year);
PG_FUNCTION_INFO_V1(int_multi_year);
PG_FUNCTION_INFO_V1(int_div_year);

PG_FUNCTION_INFO_V1(year_eq_num);
PG_FUNCTION_INFO_V1(num_eq_year);

/* Fix Bug#Z5263 begin */
PG_FUNCTION_INFO_V1(year_text_like);
PG_FUNCTION_INFO_V1(year_text_notlike);
PG_FUNCTION_INFO_V1(year_int_like);
PG_FUNCTION_INFO_V1(year_int_notlike);
/* Fix Bug#Z5263 end */

static double float8in_internals(char* str, char** endptr_p, bool* hasError);
static YearADT int32_to_YearADT(int32 year);
static YearADT int4_to_YearADT(int32 year);
static int year_cmp_internal(YearADT y1, YearADT y2);
static int year_fastcmp(Datum x, Datum y, SortSupport ssup);

/*
 * year_in()
 * Given year text string, convert to internal year format.
 */
Datum year_in(PG_FUNCTION_ARGS)
{
	char *str_in  = PG_GETARG_CSTRING(0);
	bool hasError = false;
	char *endptr  = NULL;
	double num = 0;
	int32  tmp = 0;
	int    len = 0;
	YearADT result;

	num = float8in_internals(str_in, &endptr, &hasError) + 0.05;
	tmp = (int32) DirectFunctionCall1(dtoi4, Float8GetDatum(num));
	len = strlen(str_in);

	if (hasError)
	{
		ereport(WARNING,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type double precision: \"%s\"", str_in)));
	}

	/*
	 * number equals 0, and string length is 4, the result is 0
	 * number equals 0, string length is NOT 4, the result is 2000
	 * '0000', '+000', '00.1', '-0.2', ' 000' -- 0
	 * '  000 ', '00000', '0.1' --2000
	 */
	if (tmp == 0 && len != YEAR4_LEN && !hasError)
	{
		tmp = 2000;
	}

	result = int4_to_YearADT(tmp);

	/* year(2) */
	if (PG_GETARG_INT32(1) == YEAR2_LEN)
	{
		YEAR_CONVERT(result);
	}

	PG_RETURN_YEARADT(result);
}

/*
 * year_out()
 * Given internal format year4, convert to text string.
 */
Datum year_out(PG_FUNCTION_ARGS)
{
	YearADT year = PG_GETARG_INT16(0);
	int32   val  = 0;
	char *result = NULL;
	char buf[8];    /* buffer's size = 8 is enough for "YYYY\0" */

	if (year >= 0)
	{
		/* year(4) */
		val = YearADT_to_Year(year);
		snprintf(buf, sizeof(buf), "%04d", val);
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-year) % 100;
	snprintf(buf, sizeof(buf), "%04d", val);
	}

	result = pstrdup(buf);

	PG_RETURN_CSTRING(result);
}

/*
 * year_recv - converts external binary format to year4
 */
Datum year_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
	int32 tmp = pq_getmsgint(buf, sizeof(int32));

	YearADT result = int32_to_YearADT(tmp);
	/* year(2) */
	if (PG_GETARG_INT32(2) == YEAR2_LEN)
	{
		YEAR_CONVERT(result);
	}

	PG_RETURN_YEARADT(result);
}

/*
 *  year_send - converts year4 to binary format
 */
Datum year_send(PG_FUNCTION_ARGS)
{
	YearADT year = PG_GETARG_YEARADT(0);
	int32    val = 0;
	StringInfoData buf;

	if (year >= 0)
	{
		/* year(4) */
		val = YearADT_to_Year(year);
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digitis */
		val = YearADT_to_Year(-year) % 100;
	}

	pq_begintypsend(&buf);
	pq_sendint32(&buf, val);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum yeartypmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32   typmod;
	int32  *t1 = NULL;
	int     n;

	t1 = ArrayGetIntegerTypmods(ta, &n);
	if (n != 1)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid type modifier")));
	}
	typmod = *t1;

	/* neither year(2) nor year(4) */
	if (typmod != YEAR2_LEN && typmod != YEAR4_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid display width and use YEAR instead")));
	}

	PG_RETURN_INT32(typmod);
}

/* determine display width */
Datum yeartypmodout(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	char *ret = (char *) palloc(TYPMODOUT_LEN);

	if (typmod >= 0)
	{
		snprintf(ret, TYPMODOUT_LEN, "(%d)", (int)typmod);
	}
	else
	{
		strncpy(ret, "", strlen(""));
	}

	PG_RETURN_CSTRING(ret);
}

Datum year_eq(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) == 0);
}

Datum year_ne(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) != 0);
}

Datum year_lt(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) < 0);
}

Datum year_le(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) <= 0);
}

Datum year_gt(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) > 0);
}

Datum year_ge(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) >= 0);
}

Datum year_seq(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_BOOL(yearLeft == yearRight);
}

Datum year_cmp(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);

	PG_RETURN_INT32(year_cmp_internal(yearLeft, yearRight));
}

Datum year_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);

	ssup->comparator = year_fastcmp;

	PG_RETURN_VOID();
}

Datum year_hash(PG_FUNCTION_ARGS)
{
	return hashint8(fcinfo);
}

Datum int4_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);

	PG_RETURN_YEARADT(int4_to_YearADT(year));
}

Datum timestamptz_year(PG_FUNCTION_ARGS)
{
	TimestampTz timestamp = PG_GETARG_TIMESTAMPTZ(0);
	int32 tmpResult;
	fsec_t fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamptz_year_eq timestamp out of range")));

	tmpResult = 0;
	if (tm->tm_year > 0)
		tmpResult = tm->tm_year;

	PG_RETURN_YEARADT(int32_to_YearADT(tmpResult));
}

Datum year_eq_timestamptz(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_YEARADT(0);
	YearADT yearRight = 0;
	TimestampTz timestamp = PG_GETARG_TIMESTAMPTZ(1);
	int32 tmpResult;
	fsec_t fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamptz_year_eq timestamp out of range")));

	tmpResult = 0;
	if (tm->tm_year > 0)
		tmpResult = tm->tm_year;

	yearRight = int32_to_YearADT(tmpResult);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) == 0);
}

Datum timestamptz_eq_year(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = 0;
	TimestampTz timestamp = PG_GETARG_TIMESTAMPTZ(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);
	int32 tmpResult;
	fsec_t fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				errmsg("timestamptz_year_eq timestamp out of range")));

	tmpResult = 0;
	if (tm->tm_year > 0)
		tmpResult = tm->tm_year;

	yearLeft = int32_to_YearADT(tmpResult);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) == 0);
}

Datum numeric_year(PG_FUNCTION_ARGS)
{
	Numeric numeric = (Numeric) PG_GETARG_NUMERIC(0);
	Datum retValue;
	YearADT tmp_year;

	retValue = DirectFunctionCall1(numeric_int2,
									NumericGetDatum(numeric));

	tmp_year = int32_to_YearADT(DatumGetInt32(retValue));

	PG_RETURN_YEARADT(tmp_year);
}

Datum date_year(PG_FUNCTION_ARGS)
{
	DateADT dateVal = PG_GETARG_DATEADT(0);
	uint64  dateOffsetYear = 10000;
	int32  tmpResult;

	/* fix Bug#Z5261 begin */
	/*
	 * date type format: 2025 0701
	 * dateOffsetYear: 10000
	 * year = 20250702 / 10000;
	 */
	tmpResult = dateVal / dateOffsetYear;
	/* fix Bug#Z5261 end */

	PG_RETURN_YEARADT(int32_to_YearADT(tmpResult));
}

Datum time_year(PG_FUNCTION_ARGS)
{
	TimeADT		timeVal = PG_GETARG_TIMEADT(0);
	Timestamp	result = 0;
	YearADT     year;
	struct pg_tm tt,
				*tm = &tt;

	GetCurrentDateTime(tm);

	if (timeVal >= USECS_PER_DAY)
		tm->tm_year += 1;

	if (tm->tm_year > 0)
		result = tm->tm_year;

	year = int32_to_YearADT(result);

	PG_RETURN_YEARADT(year);
}

/* Fix Bug#Z5406 begin */
Datum times_year(PG_FUNCTION_ARGS)
{
	Timestamp	result = 0;
	YearADT     year;
	struct pg_tm tt,
				*tm = &tt;

	GetCurrentDateTime(tm);

	if (tm->tm_year > 0)
		result = tm->tm_year;

	year = int32_to_YearADT(result);

	PG_RETURN_YEARADT(year);
}
/* Fix Bug#Z5406 end */

Datum timetz_year(PG_FUNCTION_ARGS)
{
	TimeTzADT  *query = PG_GETARG_TIMETZADT_P(0);
	TimeADT		timeVal = query->time;
	Timestamp	result = 0;
	YearADT     year;
	struct pg_tm tt,
				*tm = &tt;

	GetCurrentDateTime(tm);

	if (timeVal >= USECS_PER_DAY)
		tm->tm_year += 1;

	if (tm->tm_year > 0)
		result = tm->tm_year;

	year = int32_to_YearADT(result);

	PG_RETURN_YEARADT(year);
}

/* Fix Bug#Z5406 begin */
Datum datetime_year(PG_FUNCTION_ARGS)
{
	TimeADT dateTimeVal = PG_GETARG_TIMEADT(0);
	uint64  dateTimeOffsetYear = 10000000000;
	int32   tmpResult;
	YearADT year;

	if (!dateTimeVal)
		PG_RETURN_NULL();

	/*
	 * datetime type format: 2025 0707020134
	 * year offset: 100000 00000
	 * year = 2025 0707020134 / 100000 00000
	 */
	tmpResult = dateTimeVal / dateTimeOffsetYear;

	year = int32_to_YearADT(tmpResult);

	PG_RETURN_YEARADT(year);
}

Datum datetimes_year(PG_FUNCTION_ARGS)
{
	Numeric		dateTimeVal = PG_GETARG_NUMERIC(0);
	uint64  dateTimeOffsetYear = 10000000000;
	int32   tmpResult;
	int64 	retValue;
	YearADT year;

	if (!dateTimeVal)
		PG_RETURN_NULL();

	/*
	 * datetime type format: 2025 0707020134
	 * year offset: 100000 00000
	 * year = 2025 0707020134 / 100000 00000
	 */
	// tmpResult = dateTimeVal / dateTimeOffsetYear;
	retValue = DatumGetInt64(DirectFunctionCall1(numeric_int8,
						NumericGetDatum(dateTimeVal)));

	tmpResult = retValue / dateTimeOffsetYear;

	year = int32_to_YearADT(tmpResult);

	PG_RETURN_YEARADT(year);
}

Datum timestamp_year(PG_FUNCTION_ARGS)
{
	TimeADT 	time_stamp = PG_GETARG_TIMESTAMP(0);
	uint64 		timeStampOffsetYear = 10000000000;
	int32 		tmpResult;
	YearADT		year;
	
	if (!time_stamp)
		PG_RETURN_NULL();

	/*
	 * timestamp type format: 2025 0707020134
	 * year offset: 100000 00000
	 * year = 2025 0707020134 / 100000 00000
	 */
	tmpResult = time_stamp / timeStampOffsetYear;

	year = int32_to_YearADT(tmpResult);

	PG_RETURN_YEARADT(year);
}

Datum timestamps_year(PG_FUNCTION_ARGS)
{
	Numeric		timestampsVal = PG_GETARG_NUMERIC(0);
	uint64  timestampsOffsetYear = 10000000000;
	int32   tmpResult;
	int64 	retValue;
	YearADT year;

	if (!timestampsVal)
		PG_RETURN_NULL();

	/*
	 * datetime type format: 2025 0707020134
	 * year offset: 100000 00000
	 * year = 2025 0707020134 / 100000 00000
	 */
	// tmpResult = dateTimeVal / dateTimeOffsetYear;
	retValue = DatumGetInt64(DirectFunctionCall1(numeric_int8,
						NumericGetDatum(timestampsVal)));

	tmpResult = retValue / timestampsOffsetYear;

	year = int32_to_YearADT(tmpResult);

	PG_RETURN_YEARADT(year);
}
/* Fix Bug#Z5406 end */

Datum char_year(PG_FUNCTION_ARGS)
{
	Datum		txt = PG_GETARG_DATUM(0);
	char	   *str = TextDatumGetCString(txt);
	YearADT     year;
	int32_t     tmpResult;

	if (str == NULL)
		PG_RETURN_NULL();

	tmpResult = atoi(str);

	year = int32_to_YearADT(tmpResult);

	PG_RETURN_YEARADT(year);
}

Datum bigint_year(PG_FUNCTION_ARGS)
{
	int64   num = PG_GETARG_INT64(0);
	YearADT year;

	if (num <= MIN_YEAR_NUM)
		num = MIN_YEAR_NUM;
	else if (num >= MAX_YEAR_NUM)
		num = MAX_YEAR_NUM;

	year = bigint_to_YearADT(num);

	PG_RETURN_YEARADT(year);
}

Datum year_lt_bigint(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_YEARADT(0);
	int64   num = PG_GETARG_INT64(1);
	YearADT yearRight;
	
	if (num <= MIN_YEAR_NUM)
		num = MIN_YEAR_NUM;
	else if (num >= MAX_YEAR_NUM)
		num = MAX_YEAR_NUM;

	yearRight = bigint_to_YearADT(num);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) < 0);
}

Datum bigint_lt_year(PG_FUNCTION_ARGS)
{
	int64   num = PG_GETARG_INT64(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);
	YearADT yearLeft;

	if (num <= MIN_YEAR_NUM)
		num = MIN_YEAR_NUM;
	else if (num >= MAX_YEAR_NUM)
		num = MAX_YEAR_NUM;

	yearLeft = bigint_to_YearADT(num);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) < 0);
}

Datum year_gt_bigint(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_YEARADT(0);
	int64   num = PG_GETARG_INT64(1);
	YearADT yearRight;
	
	if (num <= MIN_YEAR_NUM)
		num = MIN_YEAR_NUM;
	else if (num >= MAX_YEAR_NUM)
		num = MAX_YEAR_NUM;

	yearRight = bigint_to_YearADT(num);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) > 0);
}

Datum bigint_gt_year(PG_FUNCTION_ARGS)
{
	int64   num = PG_GETARG_INT64(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);
	YearADT yearLeft;

	if (num <= MIN_YEAR_NUM)
		num = MIN_YEAR_NUM;
	else if (num >= MAX_YEAR_NUM)
		num = MAX_YEAR_NUM;

	yearLeft = bigint_to_YearADT(num);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) > 0);
}

Datum year_scale(PG_FUNCTION_ARGS)
{
	YearADT result = PG_GETARG_YEARADT(0);
	int32   typmod = PG_GETARG_INT32(1);

	if (typmod != YEAR2_LEN && typmod != YEAR4_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid display width and use YEAR instead")));
	}
	/* Mark the year(2) type as negative */
	if (typmod == YEAR2_LEN)
		YEAR_CONVERT(result);

	PG_RETURN_YEARADT(result);
}

Datum year_larger(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_TIMESTAMP(0);
	YearADT yearRight = PG_GETARG_TIMESTAMP(1);
	YearADT result;

	if (year_cmp_internal(yearLeft, yearRight) > 0)
		result = yearLeft;
	else
	result = yearRight;

	PG_RETURN_TIMESTAMP(result);
}

Datum year_smaller(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_TIMESTAMP(0);
	YearADT yearRight = PG_GETARG_TIMESTAMP(1);
	YearADT result;

	if (year_cmp_internal(yearLeft, yearRight) < 0)
		result = yearLeft;
	else
	result = yearRight;

	PG_RETURN_TIMESTAMP(result);
}

Datum year_eq_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) == 0);
}

Datum year_ne_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) != 0);
}

Datum year_lt_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) < 0);
}

Datum year_le_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) <= 0);
}

Datum year_gt_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) > 0);
}

Datum year_ge_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft  = PG_GETARG_YEARADT(0);
	int32_t year = PG_GETARG_INT32(1);
	YearADT yearRight;

	yearRight = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) >= 0);
}

Datum year_add_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_INT16(0);
	int32_t num  = PG_GETARG_INT32(1);
	int32   val  = 0;

	if (yearLeft >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearLeft);
		val += num;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearLeft) % 100;
		val += num;
	}

	PG_RETURN_INT32(val);
}

Datum year_sub_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_INT16(0);
	int32_t num  = PG_GETARG_INT32(1);
	int32   val  = 0;

	if (yearLeft >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearLeft);
		val -= num;
		if (val < 0)
			val = 0;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearLeft) % 100;
		val -= num;
		if (val < 0)
			val = 0;
	}

	PG_RETURN_INT32(val);
}

Datum year_multi_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_INT16(0);
	int32_t num  = PG_GETARG_INT32(1);
	int32   val  = 0;

	if (yearLeft >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearLeft);
		val *= num;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearLeft) % 100;
		val *= num;
	}

	PG_RETURN_INT32(val);
}

Datum year_div_int(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_INT16(0);
	int32_t num  = PG_GETARG_INT32(1);
	int32   val  = 0;

	if (yearLeft >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearLeft);
		val /= num;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearLeft) % 100;
		val /= num;
	}

	PG_RETURN_INT32(val);
}

Datum int_eq_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) == 0);
}

Datum int_ne_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) != 0);
}

Datum int_lt_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) < 0);
}

Datum int_le_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) <= 0);
}

Datum int_gt_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) > 0);
}

Datum int_ge_year(PG_FUNCTION_ARGS)
{
	int32_t year = PG_GETARG_INT32(0);
	YearADT yearLeft;
	YearADT yearRight  = PG_GETARG_YEARADT(1);

	yearLeft = int32_to_YearADT(year);

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, yearRight) >= 0);
}

Datum int_add_year(PG_FUNCTION_ARGS)
{
	int32_t num  = PG_GETARG_INT32(0);
	YearADT yearRight = PG_GETARG_INT16(1);
	int32   val  = 0;

	if (yearRight >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearRight);
		val  = num + val;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearRight) % 100;
		val = num + val;
	}

	PG_RETURN_INT32(val);
}

Datum int_sub_year(PG_FUNCTION_ARGS)
{
	int32_t num  = PG_GETARG_INT32(0);
	YearADT yearRight = PG_GETARG_INT16(1);
	int32   val  = 0;

	if (yearRight >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearRight);
		val = num - val;
		if (val < 0)
			val = 0;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearRight) % 100;
		val = num - val;
		if (val < 0)
			val = 0;
	}

	PG_RETURN_INT32(val);
}

Datum int_multi_year(PG_FUNCTION_ARGS)
{
	int32_t num  = PG_GETARG_INT32(0);
	YearADT yearRight = PG_GETARG_INT16(1);
	int32   val  = 0;

	if (yearRight >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearRight);
		val = num * val;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearRight) % 100;
		val *= num * val;
	}

	PG_RETURN_INT32(val);
}

Datum int_div_year(PG_FUNCTION_ARGS)
{
	int32_t num  = PG_GETARG_INT32(0);
	YearADT yearRight = PG_GETARG_INT16(1);
	int32   val  = 0;

	if (yearRight >= 0)
	{
		/* year(4) */
		val  = YearADT_to_Year(yearRight);
		val = num / val;
	}
	else
	{
		/* year(2): displays only the last (least significant) 2 digits */
		val = YearADT_to_Year(-yearRight) % 100;
		val = num / val;
	}

	PG_RETURN_INT32(val);
}

Datum year_eq_num(PG_FUNCTION_ARGS)
{
	YearADT yearLeft = PG_GETARG_YEARADT(0);
	Numeric numeric = (Numeric) PG_GETARG_NUMERIC(1);
	YearADT tmp_year;
	Datum retValue;

	retValue = DirectFunctionCall1(numeric_int2,
								NumericGetDatum(numeric));

	tmp_year = int32_to_YearADT(DatumGetInt32(retValue));

	PG_RETURN_BOOL(year_cmp_internal(yearLeft, tmp_year) == 0);
}

Datum num_eq_year(PG_FUNCTION_ARGS)
{
	Numeric numeric = (Numeric) PG_GETARG_NUMERIC(0);
	YearADT yearRight = PG_GETARG_YEARADT(1);	
	YearADT tmp_year;
	Datum retValue;

	retValue = DirectFunctionCall1(numeric_int2,
								NumericGetDatum(numeric));

	tmp_year = int32_to_YearADT(DatumGetInt32(retValue));

	PG_RETURN_BOOL(year_cmp_internal(tmp_year, yearRight) == 0);
}

static double
float8in_internals(char *str, char **endptr_p, bool *hasError)
{
	double val;
	char  *endptr = str + strlen(str);
	/* Marking down the initial value of str. */
	const char *orig_num = str;

	Assert(str != NULL);

	while (*str != '\0' && isspace((unsigned char)*str))
		str++;

	if (*str == '\0')
	{
		*hasError = true;
		return 0.0;
	}

	errno = 0;
	val = strtod(str, &endptr);

	if (endptr == str || errno != 0)
	{
		int save_errno = errno;

		if (pg_strncasecmp(str, "NaN", strlen("NaN")) == 0)
		{
			val = get_float8_nan();
			endptr = str + 3;
		}
		else if (pg_strncasecmp(str, "Infinity", strlen("Infinity")) == 0)
		{
			val = get_float8_infinity();
			endptr = str + 8;
		}
		else if (pg_strncasecmp(str, "-Infinity", strlen("-Infinity")) == 0)
		{
			val = -get_float8_infinity();
			endptr = str + 9;
		}
		else if (save_errno == ERANGE)
		{
			if (val == 0.0 || val >= HUGE_VAL || val <= -HUGE_VAL)
			{
				char *errnumber = pstrdup(str);
				errnumber[endptr - str] = '\0';
				ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
						errmsg("\"%s\" is out of range for type double precision", errnumber),
						errdetail("N/A"),
						errhint("Input number exceeding limit of float8.")));
			}
		}
		else
		{
			*hasError = true;
			return 0.0;
		}
	}

	while (*endptr != '\0' && isspace((unsigned char)*endptr))
		endptr++;

	/* report stopping point, else report error if not end of string */
	if (endptr_p)
	{
		*endptr_p = endptr;
	}
	else if (*endptr != '\0')
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type double precision: \"%s\"", orig_num),
				errdetail("N/A"),
				errhint("Wrong input syntax.")));
	}

	return val;
}

/* Fix Bug#Z4800 begin */
static YearADT
int4_to_YearADT(int32 year)
{
	if (year)
	{
		if (YEAR_VALID(year))
		{
			if (YEAR2_IN_RANGE(year))
				year = year + (year >= YEAR2_BOUND_BETWEEN_20C_21C ? 1900 : 2000);
			year = year - MIN_YEAR_NUM + 1;
		}
		else
		{
			if (is_strict_mode())
				ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						errmsg("%d is out of range value for year data type!", year)));
			else
			{
				ereport(WARNING,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						errmsg("%d is out of range value for year data type!", year)));
				year = 0;
			}
		}
	}

	return year;
}
/* Fix Bug#Z4800 end */

YearADT
bigint_to_YearADT(int64 year)
{
	if (year)
	{
		if (YEAR_VALID(year))
		{
			if (YEAR2_IN_RANGE(year))
				year = year + (year >= YEAR2_BOUND_BETWEEN_20C_21C ? 1900 : 2000);
			year = year - MIN_YEAR_NUM + 1;
		}
		else if (year <= MIN_YEAR2_NUM || (year >= MAX_YEAR2_NUM && year <= MIN_YEAR_NUM))
		{
			year = MIN_YEAR2_NUM;
		}
		else if (year >= MAX_YEAR_NUM)
		{
			year = MAX_YEAR_NUM - MIN_YEAR_NUM + 1;
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					errmsg("%ld is out of range value for year data type!", year)));
		}
	}

	return year;
}

static YearADT
int32_to_YearADT(int32 year)
{
	if (year)
	{
		if (YEAR_VALID(year))
		{
			if (YEAR2_IN_RANGE(year))
				year = year + (year >= YEAR2_BOUND_BETWEEN_20C_21C ? 1900 : 2000);
			year = year - MIN_YEAR_NUM + 1;
		}
		else if (year <= MIN_YEAR2_NUM || (year >= MAX_YEAR2_NUM && year <= MIN_YEAR_NUM))
		{
			year = MIN_YEAR2_NUM;
		}
		else if (year >= MAX_YEAR_NUM)
		{
			year = MAX_YEAR_NUM - MIN_YEAR_NUM + 1;
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					errmsg("%d is out of range value for year data type!", year)));
		}
	}

	return year;
}

static int
year_cmp_internal(YearADT y1, YearADT y2)
{
	if (IS_YEAR2(y1))
		YEAR_CONVERT(y1);
	if (IS_YEAR2(y2))
		YEAR_CONVERT(y2);

	return (y1 < y2) ? -1 : ((y1 > y2) ? 1 : 0);
}

static int
year_fastcmp(Datum x, Datum y, SortSupport ssup)
{
	YearADT yearLeft  = DatumGetYearADT(x);
	YearADT yearRight = DatumGetYearADT(y);

	return year_cmp_internal(yearLeft, yearRight);
}

/* Fix Bug#Z5263 begin */
Datum year_text_like(PG_FUNCTION_ARGS)
{
	YearADT		yearLeft = PG_GETARG_YEARADT(0);
	Datum		val = PG_GETARG_DATUM(1);
	int32		yearVal;
	char		yearStr[YEARSTRLEN] = {0};
	text	   *yearText;
	bool		status;

	yearVal = YearADT_to_Year(yearLeft);
	snprintf(yearStr, sizeof(yearStr), "%d", yearVal);

	yearText = cstring_to_text(yearStr);

	status = DatumGetBool(DirectFunctionCall2(textlike, PointerGetDatum(yearText), val));

	PG_RETURN_BOOL(status);
}

Datum year_text_notlike(PG_FUNCTION_ARGS)
{
	YearADT		yearLeft = PG_GETARG_YEARADT(0);
	Datum		val = PG_GETARG_DATUM(1);
	int32		yearVal;
	char		yearStr[YEARSTRLEN] = {0};
	text	   *yearText;
	bool		status;

	yearVal = YearADT_to_Year(yearLeft);
	snprintf(yearStr, sizeof(yearStr), "%d", yearVal);

	yearText = cstring_to_text(yearStr);

	status = DatumGetBool(DirectFunctionCall2(textnlike, PointerGetDatum(yearText), val));

	PG_RETURN_BOOL(status);
}

Datum year_int_like(PG_FUNCTION_ARGS)
{
	YearADT		yearLeft = PG_GETARG_YEARADT(0);
	int32_t     val  = PG_GETARG_INT32(1);
	YearADT		yearRight;

	yearRight = int32_to_YearADT(val);

	PG_RETURN_BOOL(yearLeft == yearRight);
}

Datum year_int_notlike(PG_FUNCTION_ARGS)
{
	YearADT		yearLeft = PG_GETARG_YEARADT(0);
	int32_t     val  = PG_GETARG_INT32(1);
	YearADT		yearRight;

	yearRight = int32_to_YearADT(val);

	PG_RETURN_BOOL(yearLeft != yearRight);
}
/* Fix Bug#Z5263 end */