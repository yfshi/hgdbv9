/*-------------------------------------------------------------------------------
 *
 * File: datetime_func.c
 *
 * Abstract:
 * 		This file contains the implementation of Mysql's
 *		datetime data type related built-in functions.
 *
 * Authored by linjh@highgo.com, 2025216.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/builtin_functions/datetime_func.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"


#include "access/hash.h"
#include "access/xact.h"
#include "c.h"
#include "catalog/pg_namespace_d.h"
#include "datatype/timestamp.h"
#include "fmgr.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "mb/pg_wchar.h"
#include "parser/scansup.h"
#include "pgtime.h"
#include "sys/types.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/elog.h"
#include "utils/float.h"
#include "utils/fmgrprotos.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(dayofmonth);
PG_FUNCTION_INFO_V1(dayofweek);
PG_FUNCTION_INFO_V1(dayofyear);
PG_FUNCTION_INFO_V1(quarter);

PG_FUNCTION_INFO_V1(hour);
PG_FUNCTION_INFO_V1(minute);
PG_FUNCTION_INFO_V1(second);
PG_FUNCTION_INFO_V1(microsecond);

PG_FUNCTION_INFO_V1(year);
PG_FUNCTION_INFO_V1(weekday);
PG_FUNCTION_INFO_V1(weekofyear);

PG_FUNCTION_INFO_V1(now);
PG_FUNCTION_INFO_V1(curtime);
PG_FUNCTION_INFO_V1(sysdate);

PG_FUNCTION_INFO_V1(makedate);
PG_FUNCTION_INFO_V1(maketime);

PG_FUNCTION_INFO_V1(period_add);
PG_FUNCTION_INFO_V1(period_diff);

PG_FUNCTION_INFO_V1(mystime);
PG_FUNCTION_INFO_V1(timediff);
PG_FUNCTION_INFO_V1(time_format);

PG_FUNCTION_INFO_V1(subtime);
PG_FUNCTION_INFO_V1(sec_to_time);

PG_FUNCTION_INFO_V1(timestampadd);
PG_FUNCTION_INFO_V1(timestamp_arg1);
PG_FUNCTION_INFO_V1(timestamp_arg2);

#define MICRO_PER_NANOSEC 1000
#define FORMAT_TIME_BOUND 838
#define FORMAT_TIME_MAX_VALUE (FORMAT_TIME_BOUND * 10000 + 59 * 100 + 59)
#define FORMAT_TIMESTAMP_MAX_VALUE INT64CONST(252455615999999999)  /* datetime 9999-12-31 23:59:59.999999 */
#define FORMAT_TIMESTAMP_MIN_VALUE INT64CONST(-63113904000000000)  /* datetime 0000-01-01 00:00:00.000000 */
#define FORMAT_TIMESTAMP_FIRST_YEAR INT64CONST(-63082281600000000) /* datetime 0001-01-01 00:00:00.000000 */
#define TIME_MAX_FRAC 999999
#define MAX_YEAR_OF_DATE 9999
#define MIN_YEAR_OF_DATE 0
#define YEAR2_ADJUST_VALUE_20C_21C(x) ((x) + ((x) >= YEAR2_BOUND_BETWEEN_20C_21C ? 1900 : 2000))

#define MAX_VALUE_12_CLOCK (HOURS_PER_DAY / 2)
#define MIN_VALUE_12_CLOCK 1
#define MAX_VALUE_24_CLOCK (HOURS_PER_DAY)
#define FRAC_PART_LEN_IN_NUMERICSEC 100000000
#define ROUNDING_BORDER 5
#define MONTHS_PER_QUARTER 3
#define DAYS_PER_WEEK 7
#define NANOSEC_PER_USEC 1000

#define FORMAT_TIME_MAX_VALUE_TO_DAY 70

static unsigned long long pow_of_10[20] = {
    1, 10, 100, 1000, 10000UL, 100000UL, 1000000UL, 10000000UL,
    100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL,
    1000000000000ULL, 10000000000000ULL, 100000000000000ULL,
    1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL,
    1000000000000000000ULL, 10000000000000000000ULL
};

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

static Datum ivymy_dayof_date(FunctionCallInfo fcinfo, int datekind);
static int64 getPartFromTm(struct pg_tm *tm, fsec_t fsec, int part);
static Datum GetSpecificPartOfTime(FunctionCallInfo fcinfo, int part);
static unsigned long convert_period_to_month(unsigned long period);
static unsigned long convert_month_to_period(unsigned long month);
static bool calc_interval(Interval *span, lldiv_t arg, int dtk);
static TimeADT time_sub_interval(TimeADT time, Interval *span);
static bool datetime_sub_interval(Timestamp datetime, Interval *span, Timestamp *result, bool is_add_func);
static TimeOffset time2t(const int hour, const int min, const int sec, const fsec_t fsec);
static Datum timestamp_add_internal(FunctionCallInfo fcinfo, char *lowunits, int unit, int unit_type, Numeric num);
static bool determine_conversion(int dtk);

static Datum
ivymy_dayof_date(FunctionCallInfo fcinfo, int datekind)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    /* Begin - bug#z5083 */
    if (mysdate == 0)
        PG_RETURN_NULL();

    if (mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_INT32(intresult);
    /* End - bug#z5083 */

    MYS_ZERO_tm(tm); /* bug#5127 */

    part1 = (mysdate / 10000);
    part2 = (mysdate - part1 * 10000);
    tm->tm_year = part1;

    tm->tm_mon = part2 / 100;
    tm->tm_mday = part2 % 100;

    switch (datekind)
    {
        case WEEK:
            intresult = j2day(date2j(tm->tm_year, tm->tm_mon, tm->tm_mday)) + 1;
            break;

        case MONTH:
            intresult = tt.tm_mday;
            break;

        case YEAR:
            intresult = (date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - date2j(tm->tm_year, 1, 1) + 1);
            break;

        case QUARTER:
            intresult = (tm->tm_mon - 1) / 3 + 1;
            break;

        default:
            PG_RETURN_NULL();
    }

    PG_RETURN_INT32(intresult);
}

Datum 
dayofmonth(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, MONTH));
}

Datum 
dayofweek(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, WEEK));
}

Datum 
dayofyear(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, YEAR));
}

Datum 
quarter(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, QUARTER));
}

static int64
getPartFromTm(struct pg_tm *tm, fsec_t fsec, int part)
{
    int64 result = 0;
    if (fsec > TIME_MAX_FRAC)
    {
        tm->tm_sec = 0;
        tm->tm_min++;
    }

    if (tm->tm_min > TIME_MAX_MINUTE)
    {
        tm->tm_min = 0;
        tm->tm_hour++;
    }

    switch (part)
    {
        case HOUR:
            result = tm->tm_hour;
            break;
        case MINUTE:
            result = tm->tm_min;
            break;
        case SECOND:
            result = tm->tm_sec;
            break;
        case MICROSECOND:
            result = fsec;
            break;
        default:
            break;
    }

    return result;
}

static Datum
GetSpecificPartOfTime(FunctionCallInfo fcinfo, int part)
{
    struct pg_tm tt, *tm = &tt;
    Numeric num = PG_GETARG_NUMERIC(0);
    Numeric tempfsec;
    int64 fsec = 0;
    int64 time = 0;

    MYS_ZERO_tm(tm); /* bug#5127 */

    time = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(num)));
    tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(num),
                                                   NumericGetDatum(int64_to_numeric(1000000))));
    tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(tempfsec),
                                                   NumericGetDatum(int64_to_numeric(1000000))));
    fsec = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(tempfsec)));

    if (fsec >= 500000)
        time = time - 1;

    if (time == MYSQL_NULL_TIME)
        PG_RETURN_NULL();

    if (time < 0)
    {
        time = -time;
    }

    tm->tm_hour = time / 10000;
    tm->tm_min = time % 10000 / 100;
    tm->tm_sec = time % 100;

    PG_RETURN_INT64(getPartFromTm(tm, fsec, part));
}

Datum
hour(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, HOUR);
}

Datum
microsecond(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, MICROSECOND);
}

Datum 
minute(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, MINUTE);
}

Datum 
second(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, SECOND);
}

Datum 
weekday(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

    MYS_ZERO_tm(tm); /* bug#5127 */

    part1 = (mysdate / 10000);
    part2 = (mysdate - part1 * 10000);
    tm->tm_year = part1;

    tm->tm_mon = part2 / 100;
    tm->tm_mday = part2 % 100;

    /*return week*/
    intresult = j2day(date2j(tm->tm_year, tm->tm_mon, tm->tm_mday));
    if (intresult == 0)
        intresult = 7;
    PG_RETURN_INT32(intresult - 1);
}

Datum
weekofyear(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

    MYS_ZERO_tm(tm); /* bug#5127 */

    part1 = (mysdate / 10000);
    part2 = (mysdate - part1 * 10000);
    tm->tm_year = part1;

    tm->tm_mon = part2 / 100;
    tm->tm_mday = part2 % 100;

    intresult = date2isoweek(tm->tm_year, tm->tm_mon, tm->tm_mday);
    PG_RETURN_INT32(intresult);
}

Datum 
year(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

    MYS_ZERO_tm(tm); /* bug#5127 */

    part1 = (mysdate / 10000);
    part2 = (mysdate - part1 * 10000);
    tm->tm_year = part1;

    tm->tm_mon = part2 / 100;
    tm->tm_mday = part2 % 100;

    if (tm->tm_year <= MAX_YEAR_OF_DATE)
        PG_RETURN_INT64(tm->tm_year);

    PG_RETURN_NULL();
}

Datum 
curtime(PG_FUNCTION_ARGS)
{
    int typmod = PG_GETARG_INT32(0);
    TimestampTz state_start_timestamp;
    int64 time = 0;
    struct pg_tm tt, *tm = &tt;
    fsec_t fsec = 0;
    int tz = 0;
    int warn = 0;
    int is_strict_mode = 0;
    char buf[MAXDATELEN + 1] = {0};
    Numeric num;

    if (typmod > DATETIME_MAX_DECIMALS)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Too-big precision %d specified for \'curtime\'. Maximum is 6.", typmod)));
    }

    MYS_ZERO_tm(tm); /* bug#5127 */

    state_start_timestamp = GetCurrentStatementStartTimestamp();

    if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
        (has_sql_mode(MODE_STRICT_ALL_TABLES)))
        is_strict_mode = true;

    if (timestamp2tm(state_start_timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                 errmsg("timestamp out of range")));

    mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[typmod], &warn);

    fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];

    if (!warn)
    {
        time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
    }
    else
    {
        if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
        {
            ereport(ERROR, (errmsg("Incorrect TIME value: '%ld'", state_start_timestamp)));
        }

        if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
        {
            ereport(WARNING, (errmsg("Out of range value.")));
            time = MYSQL_ZERO_TIME;
        }
    }

#ifdef WIN32
    snprintf(buf, MAXDATELEN, "%lld.%06d", time, fsec);
#else
    snprintf(buf, MAXDATELEN, "%ld.%06d", time, fsec);
#endif

    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));
    PG_RETURN_NUMERIC(num);
}

/* now()
 * return current timestamp in b compatibility
 */
Datum 
now(PG_FUNCTION_ARGS)
{
    int typmod = PG_GETARG_INT32(0);
    TimestampTz state_start_timestamp;
    fsec_t fsec;
    int64 datetime = -1;
    int64 temp = 0;
    int64 time = 0;
    struct pg_tm tt, *tm = &tt;
    int is_strict_mode = 0;
    int tz = 0;
    int warn = 0;
    Numeric num;
    char buf[MAXDATELEN + 1] = {0};

    /* lexical analyzer can only recognize positive integers for token ICONST in gram.y */
    if (typmod > DATETIME_MAX_DECIMALS)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Too-big precision %d specified for \'now\'. Maximum is 6.", typmod)));
    }

    MYS_ZERO_tm(tm); /* bug#5127 */
    
    state_start_timestamp = GetCurrentStatementStartTimestamp();

    if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
        (has_sql_mode(MODE_STRICT_ALL_TABLES)))
        is_strict_mode = true;

    if (timestamp2tm(state_start_timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                 errmsg("timestamp out of range")));

    mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[typmod]);
    fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];

    if (!warn)
    {
        if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
            datetime = MYSQL_ZERO_TIMESTAMP;
    }
    else
    {
        if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
        {
            ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", state_start_timestamp)));
        }
        if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
        {
            ereport(WARNING, (errmsg("Out of range value.")));
            datetime = MYSQL_ZERO_TIMESTAMP;
        }
    }

    if (datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
    {
        temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
        temp = temp * 1000000;
        time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
        datetime = temp + time;
    }

#ifdef WIN32
    snprintf(buf, MAXDATELEN, "%lld.%06d", datetime, fsec);
#else
    snprintf(buf, MAXDATELEN, "%ld.%06d", datetime, fsec);
#endif
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));

    return NumericGetDatum(num);
}

Datum 
sysdate(PG_FUNCTION_ARGS)
{
    int typmod = PG_GETARG_INT32(0);
    TimestampTz sys_timestamp;
    fsec_t fsec;
    int64 datetime = -1;
    int64 temp = 0;
    int64 time = 0;
    struct pg_tm tt, *tm = &tt;
    int is_strict_mode = 0;
    int tz = 0;
    int warn = 0;
    Numeric num;
    char buf[MAXDATELEN + 1] = {0};

    /* lexical analyzer can only recognize positive integers for token ICONST in gram.y */
    if (typmod > 6)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Too-big precision %d specified for \'now\'. Maximum is 6.", typmod)));
    }

    MYS_ZERO_tm(tm); /* bug#5127 */

    sys_timestamp = GetCurrentTimestamp();

    if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
        (has_sql_mode(MODE_STRICT_ALL_TABLES)))
        is_strict_mode = true;

    if (timestamp2tm(sys_timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                 errmsg("timestamp out of range")));

    (void)mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warn, msec_round_add[typmod]);
    fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];

    if (!warn)
    {
        if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
            datetime = MYSQL_ZERO_TIMESTAMP;
    }
    else
    {
        if (is_strict_mode && (warn & (MYSQL_TIME_WARN_OUT_OF_RANGE)))
        {
            ereport(ERROR, (errmsg("Incorrect DATETIME value: '%ld'", sys_timestamp)));
        }
        if (warn & (MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
        {
            ereport(WARNING, (errmsg("Out of range value.")));
            datetime = MYSQL_ZERO_TIMESTAMP;
        }
    }

    if (datetime != 0 && datetime != MYSQL_ZERO_TIMESTAMP)
    {
        temp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
        temp = temp * 1000000;
        time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
        datetime = temp + time;
    }

#ifdef WIN32
    snprintf(buf, MAXDATELEN, "%lld.%06d", datetime, fsec);
#else
    snprintf(buf, MAXDATELEN, "%ld.%06d", datetime, fsec);
#endif
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));

    return NumericGetDatum(num);
}

/* makedate()
 * Description: Convert yyyy.ddd into date type
 * return: Convert result, date type.
 */
Datum 
makedate(PG_FUNCTION_ARGS)
{
    int64 year = PG_GETARG_INT64(0);
    int64 dayofyear = PG_GETARG_INT64(1);
    int julian;
    int res_year, res_mon, res_day;
    DateADT date;

    if (year < MIN_YEAR_OF_DATE || year > MAX_YEAR_OF_DATE ||
        dayofyear <= 0 || dayofyear >= (int64)pow_of_10[7])
        PG_RETURN_NULL();

    if (YEAR2_IN_RANGE(year))
        year = YEAR2_ADJUST_VALUE_20C_21C(year);

    /*the date 'year-1-1' add (dayofyear - 1)*/
    julian = date2j(year, 1, 1);

    julian += dayofyear - 1;

    j2date(julian, &res_year, &res_mon, &res_day);

    if (res_year == 0 && res_mon == 0 && res_day == 0)
    {
        date = MYSQL_ZERO_DATE;
    }
    else
    {
        date = res_year * 10000 + res_mon * 100 + res_day;
    }

    if (res_year < MIN_YEAR_OF_DATE || res_year > MAX_YEAR_OF_DATE)
        PG_RETURN_NULL();

    PG_RETURN_DATEADT(date);
}

/* maketime()
 * Description: Convert hh, min, sec into time type
 * return: Convert result, time type.
 */

Datum 
maketime(PG_FUNCTION_ARGS)
{
    int64 hour = PG_GETARG_INT64(0);
    int64 minute = PG_GETARG_INT64(1);
    lldiv_t seconds;
    struct pg_tm tt;
    TimeADT time;
    char buf[MAXDATELEN + 1] = {0};
    Numeric num;
    unsigned int hour_abs;
    fsec_t fsec = 0;
    int dec = 6;
    bool overflow = 0;
    bool neg = 0;
    unsigned int nanoseconds;

    MYS_ZERO_tm(&tt); /* bug#5127 */

    if (!mysnumeric_to_lldiv_t(PG_GETARG_NUMERIC(2), &seconds))
    {
        PG_RETURN_NULL();
    }

    if (minute < 0 || minute > TIME_MAX_MINUTE || seconds.quot < 0 || seconds.quot > TIME_MAX_SECOND || seconds.rem < 0)
        PG_RETURN_NULL();

    if (hour < 0)
        neg = 1;

    hour_abs = (unsigned int)((hour < 0 ? -hour : hour));

    if (-hour > UINT_MAX || hour > UINT_MAX || hour_abs > TIME_MAX_HOUR)
        overflow = 1;

    if (!overflow)
    {
        tt.tm_hour = (int)hour_abs;
        tt.tm_min = (int)minute;
        tt.tm_sec = (int)seconds.quot;
        fsec = (int)(seconds.rem / MICRO_PER_NANOSEC);
        nanoseconds = seconds.rem % MICRO_PER_NANOSEC;
        if (nanoseconds >= (MICRO_PER_NANOSEC / 2))
        {
            fsec += 1;
        }

        time = tt.tm_hour * 10000 + tt.tm_min * 100 + tt.tm_sec;
        if (neg)
        {
            time = -time;
        }

        for (int i = 1; i <= 6; i++)
        {
            if (fsec % pow_of_10[i] == 0)
            {
                dec--;
            }
        }
    }

    if (overflow || time < -FORMAT_TIME_MAX_VALUE || time > FORMAT_TIME_MAX_VALUE)
    {

        time = neg ? -FORMAT_TIME_MAX_VALUE : FORMAT_TIME_MAX_VALUE;
        dec = 0;
    }

    snprintf(buf, MAXDATELEN, "%ld.%06d", time, fsec);

    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));
    PG_RETURN_NUMERIC(num);
}

static unsigned long
convert_period_to_month(unsigned long period)
{
    unsigned long a, b;
    if (period == 0)
        return 0L;
    if ((a = period / 100) < YEAR2_BOUND_BETWEEN_20C_21C)
        a += 2000;
    else if (a < 100)
        a += 1900;
    b = period % 100;
    return a * 12 + b - 1;
}

static unsigned long
convert_month_to_period(unsigned long month)
{
    unsigned long year;
    if (month == 0L)
        return 0L;
    if ((year = month / 12) < 100)
    {
        year += (year < YEAR2_BOUND_BETWEEN_20C_21C) ? 2000 : 1900;
    }
    return year * 100 + month % 12 + 1;
}

/* period_add()
 * Description: Add some months to a period of time.
 * return: result, int64 type.
 */
Datum 
period_add(PG_FUNCTION_ARGS)
{
    unsigned long period = (unsigned long)PG_GETARG_INT64(0);
    int months = (int)PG_GETARG_INT64(1);
    if (period == 0)
        PG_RETURN_INT32(0);
    PG_RETURN_INT64((int64)convert_month_to_period((unsigned int)((int)convert_period_to_month(period) + months)));
}

/* period_diff()
 * Description: Calculate the number of months between two period of times.
 * return: result, int64 type.
 */
Datum 
period_diff(PG_FUNCTION_ARGS)
{
    uint64 input1 = (uint64)PG_GETARG_INT64(0);
    uint64 input2 = (uint64)PG_GETARG_INT64(1);

    int64 ret = (int64)((int64)convert_period_to_month(input1) - (int64)convert_period_to_month(input2));
    PG_RETURN_INT64(ret);
}

/* sec_to_time()
 * Description: Convert seconds into time type
 * return: Convert result, time type.
 */
Datum 
sec_to_time(PG_FUNCTION_ARGS)
{
    lldiv_t seconds;
    TimeADT time;
    bool neg = 0;
    bool overflow = 0;
    int dec = 6;
    char buf[MAXDATELEN + 1] = {0};
    Numeric num;
    fsec_t fsec = 0;
    int total_sec = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    unsigned int nanoseconds;
    
    if (!mysnumeric_to_lldiv_t(PG_GETARG_NUMERIC(0), &seconds))
    {
        overflow = 1;
    }

    if (seconds.quot < 0)
    {
        neg = 1;
        seconds.quot = -seconds.quot;
        seconds.rem = -seconds.rem;
    }

    if (seconds.quot > TIME_MAX_VALUE_SECONDS)
        overflow = 1;

    if (!overflow)
    {
        total_sec = (int)seconds.quot; // keep only the integer part
        fsec = (int)(seconds.rem / MICRO_PER_NANOSEC);
        nanoseconds = seconds.rem % MICRO_PER_NANOSEC;
        if (nanoseconds >= (MICRO_PER_NANOSEC / 2))
        {
            fsec += 1;
        }

        hour = total_sec / 3600;
        min = (total_sec % 3600) / 60;
        sec = total_sec % 60;
        time = hour * 10000 + min * 100 + sec;

        if (neg)
        {
            time = -time;
        }

        for (int i = 1; i <= 6; i++)
        {
            if (fsec % pow_of_10[i] == 0)
            {
                dec--;
            }
        }
    }

    if (overflow || time < -FORMAT_TIME_MAX_VALUE || time > FORMAT_TIME_MAX_VALUE)
    {
        time = FORMAT_TIME_MAX_VALUE;
        dec = 0;
    }
    snprintf(buf, MAXDATELEN, "%ld.%06d", time, fsec);
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

    PG_RETURN_NUMERIC(num);
}

static bool
datetime_sub_time(Timestamp datetime, TimeADT time_sub, Timestamp *result)
{
    Interval span;

    span.month = 0;
    span.day = 0;
    span.time = -time_sub;

    if (datetime < FORMAT_TIMESTAMP_MIN_VALUE || datetime > FORMAT_TIMESTAMP_MAX_VALUE)
        return false;

    *result = DirectFunctionCall2(timestamp_pl_interval, TimestampGetDatum(datetime), PointerGetDatum(&span));

    if (*result < FORMAT_TIMESTAMP_MIN_VALUE || *result > FORMAT_TIMESTAMP_MAX_VALUE)
        return false;
    return true;
}

static Oid 
convert_cstring_to_pg_tm(char *str, struct pg_tm *result_tm, fsec_t *result_fsec)
{
	int flag = TIME_FLAG_INIT;
	int nanoseconds = 0;
	int fsec_len = 0;
	mysql_dttype tm_type = MYSQL_TIMESTAMP_DATE;
	int warning = 0;

	/* Check whether the string is a full timestamp */
	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	mystr_to_datetime(str, strlen(str), result_tm, flag, result_fsec, &tm_type, &warning, &nanoseconds, &fsec_len);
	mysdatetime_add_nanoseconds_with_round(result_tm, result_fsec, &warning, nanoseconds);

	if (warning)
	{
		ereport(WARNING, (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						  errmsg("date/time field value out of range: \"%s\"", str)));
	}
	/*Return the timestamp type of str according to tm_type*/
	switch (tm_type)
	{
        case MYSQL_TIMESTAMP_NONE:
            break; /* Not a timestamp */
        case MYSQL_TIMESTAMP_ERROR:
            ereport(ERROR, (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                            errmsg("date/time field value out of range: \"%s\"", str)));
            break;
        case MYSQL_TIMESTAMP_DATE:
            return MYSDATEOID;
        case MYSQL_TIMESTAMP_DATETIME:
            if (result_fsec)
                return MYSDATETIMESOID;
            else
                return MYSDATETIMEOID;
        case MYSQL_TIMESTAMP_TIME:
            if (result_fsec)
                return MYSTIMESOID;
            else
                return MYSTIMEOID;
        default:
            ereport(ERROR, (errcode(ERRCODE_INVALID_DATETIME_FORMAT),
                            errmsg("invalid input syntax for type time: \"%s\"", str)));
            return InvalidOid;
	}
	return InvalidOid;
}

static Oid
convert_cstring_to_datetime_time(char *str, struct pg_tm *result_tm, fsec_t *result_fsec, int *result_neg)
{
    char *raw = str;
	size_t len = strlen(str);
	const char *start;
	const char *end = str + len;
	Oid typid;
	mysql_dttype ftype;
	fsec_t fsec = 0;
	int flag = TIME_FLAG_INIT;
	int warn = 0;
	int nanoseconds = 0;
	int typmod = 6;
	int dash_count = 0;

	if (has_sql_mode(MODE_NO_ZERO_DATE))
		flag |= TIME_NO_ZERO_DATE;
	if (has_sql_mode(MODE_INVALID_DATES))
		flag |= TIME_INVALID_DATES;

	start = str;

	/* Determine positive or negative time */
	if (str != end && *str == '-')
	{
		str++;
	}

	for (int i = 0; str + i != end; i++)
	{
		if (str[i] == '-')
			dash_count++;
	}

	/* Check whether the string is a full timestamp */
	if (dash_count == 2)
	{
		typid = convert_cstring_to_pg_tm(str, result_tm, result_fsec);
		if (typid != InvalidOid)
			return typid;
	}

	/* Not a timestamp. Try to convert str to time*/
	mysstr_to_time(start, strlen(start), result_tm, flag, &fsec, &ftype, &warn, &nanoseconds, result_neg);
	
    if (result_tm->tm_min > TIME_MAX_MINUTE || result_tm->tm_sec > TIME_MAX_SECOND)
        return InvalidOid;

    fsec += (nanoseconds + 500) / 1000;
	mystime_add_nanoseconds_with_round(result_tm, &fsec, msec_round_add[typmod], &warn);
	fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - typmod];
	*result_fsec = fsec;

    if (warn & MYSQL_TIME_WARN_TRUNCATED)
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", raw)));
    }
	
    if (fsec)
		return MYSTIMESOID;
	
    return MYSTIMEOID;
}

Datum 
subtime(PG_FUNCTION_ARGS)
{
    Oid val_type1, val_type2;
    Oid typeOutput1, typeOutput2;
    bool typIsVarlena1, typIsVarlena2;
    char buf[MAXDATELEN + 1] = {0};
    mysql_dttype tm_type2 = MYSQL_TIMESTAMP_NONE;
    text *res;
    struct pg_tm tt1, tt2, res_tt, *tm1 = &tt1, *tm2 = &tt2, *res_tm = &res_tt;
    fsec_t fsec1, fsec2, res_fsec = 0;
    int neg = 0, neg1 = 0, neg2 = 0;
    int flag2 = TIME_FLAG_INIT;
    int warning2 = 0;
    int fsec_len2 = 0;
    int nanoseconds2 = 0;
    TimeADT time1, time2, res_time;
    Timestamp datetime1, res_datetime;
    char *val_str1, *val_str2;

    /* Begin - bug#5127 */
    MYS_ZERO_tm(tm1);
    MYS_ZERO_tm(tm2);
    MYS_ZERO_tm(res_tm);
    /* End - bug#5127 */

    val_type1 = get_fn_expr_argtype(fcinfo->flinfo, 0);
    val_type2 = get_fn_expr_argtype(fcinfo->flinfo, 1);

    if (!OidIsValid(val_type1))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type1, &typeOutput1, &typIsVarlena1);
    val_str1 = OidOutputFunctionCall(typeOutput1, PG_GETARG_DATUM(0));

    if (!OidIsValid(val_type2))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type2, &typeOutput2, &typIsVarlena2);
    val_str2 = OidOutputFunctionCall(typeOutput2, PG_GETARG_DATUM(1));

    switch (val_type2)
    {
        case TIMESTAMPOID:
        case TIMESTAMPTZOID:
        case DATEOID:
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
        case MYSDATEOID:
            mystr_to_datetime(val_str2, strlen(val_str2), tm2, flag2, &fsec2, &tm_type2, &warning2, &nanoseconds2, &fsec_len2);
            mysdatetime_add_nanoseconds_with_round(tm2, &fsec2, &warning2, nanoseconds2);
            if (tm2time(tm2, fsec2, &time2) != 0)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("datetime string and format model does not match")));
            }
            val_type2 = MYSTIMEOID;
            break;
        default:
        {
            val_type2 = convert_cstring_to_datetime_time(val_str2, tm2, &fsec2, &neg2);

            if (!OidIsValid(val_type2))
            {
                ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str2)));
                PG_RETURN_NULL();
            }

            if (tm2time(tm2, fsec2, &time2) != 0)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("datetime string and format model does not match")));
            }

            if (neg2)
            {
                time2 = -time2;
            }

            if (val_type2 == MYSDATETIMEOID || val_type2 == MYSDATETIMESOID)
            {
                PG_RETURN_NULL();
            }
        }
    }

    val_type1 = convert_cstring_to_datetime_time(val_str1, tm1, &fsec1, &neg1);

    if (!OidIsValid(val_type1))
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str1)));
        PG_RETURN_NULL();
    }
    
    if (tm2time(tm1, fsec1, &time1) != 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                 errmsg("datetime string and format model does not match")));
    }

    if (neg1)
    {
        time1 = -time1;
    }

    switch (val_type1)
    {
        case MYSDATEOID:
            time1 = 0;
            /* fallthrough */
        case MYSTIMEOID:
        case MYSTIMESOID:
            {
                res_time = time1 - time2;
                time2tm(res_time, res_tm, &res_fsec);
                /*## time_text/*/
                if (res_fsec)
                {
                    snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d.%06d", neg == 1 ? "-" : "", res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec, res_fsec);
                }
                else
                {
                    snprintf(buf, MAXDATELEN, "%s%02d:%02d:%02d", neg == 1 ? "-" : "", res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec);
                }
                res = cstring_to_text(buf);
                PG_RETURN_TEXT_P(res);
            }
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
            {
                tm2timestamp(tm1, fsec1, NULL, &datetime1);
                if (datetime_sub_time(datetime1, time2, &res_datetime))
                {
                    /* The variable datetime or result does not exceed the specified range */
                    if (res_datetime >= FORMAT_TIMESTAMP_FIRST_YEAR)
                    {
                        /* The variable datetime or result does not exceed the specified range ## datetime_text/*/
                        timestamp2tm(res_datetime, NULL, res_tm, &res_fsec, NULL, NULL);
                        if (res_fsec)
                        {
                            snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d.%06d", res_tm->tm_year, res_tm->tm_mon, res_tm->tm_mday,
                                    res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec, res_fsec);
                        }
                        else
                        {
                            snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d", res_tm->tm_year, res_tm->tm_mon, res_tm->tm_mday,
                                    res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec);
                        }
                        res = cstring_to_text(buf);
                        PG_RETURN_TEXT_P(res);
                    }
                    PG_RETURN_NULL();
                }
                ereport(ERROR, (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                                errmsg("date/time field overflow")));
                break;
            }
        default:
                ereport(ERROR, (errcode(ERRCODE_DATATYPE_MISMATCH),
                                errmsg("unsupported input data type: %s", format_type_be(val_type1))));
    }
    PG_RETURN_NULL();
}

Datum 
timediff(PG_FUNCTION_ARGS)
{
    TimeADT time1, time2, res_time;
    Timestamp datetime1, datetime2;
    Oid typeOutput1, typeOutput2;
    char *val_str1, *val_str2;
    bool typIsVarlena1, typIsVarlena2;
    Oid val_type1, val_type2;
    int64 result = 0;
    struct pg_tm tt1, tt2, res_tt, *tm1 = &tt1, *tm2 = &tt2, *res_tm = &res_tt;
    fsec_t fsec1, fsec2, res_fsec = 0;
    int neg1 = 0, neg2 = 0;
    int warn = 0;
    char buf[MAXDATELEN + 1] = {0};
    Numeric num;
    int is_strict_mode = 0;
    int dec = 0, dec1 = 0, dec2 = 0;
    char *dot;
    bool neg = false;

    /* Begin - bug#5127 */
    MYS_ZERO_tm(tm1);
    MYS_ZERO_tm(tm2);
    MYS_ZERO_tm(res_tm);
    /* End - bug#5127 */

    val_type1 = get_fn_expr_argtype(fcinfo->flinfo, 0);
    val_type2 = get_fn_expr_argtype(fcinfo->flinfo, 1);

    if (!OidIsValid(val_type1))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type1, &typeOutput1, &typIsVarlena1);
    val_str1 = OidOutputFunctionCall(typeOutput1, PG_GETARG_DATUM(0));

    if (!OidIsValid(val_type2))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type2, &typeOutput2, &typIsVarlena2);
    val_str2 = OidOutputFunctionCall(typeOutput2, PG_GETARG_DATUM(1));

    /* Begin - bug#z5127 */
    dot = strchr(val_str1, '.');
    if (dot)
    {
        dec1 = strlen(val_str1) - (dot - val_str1) - 1;

        if (dec1 > DATETIME_MAX_DECIMALS)
            dec1 = DATETIME_MAX_DECIMALS;
    }

    dot = strchr(val_str2, '.');
    if (dot)
    {
        dec2 = strlen(val_str2) - (dot - val_str2) - 1;

        if (dec2 > DATETIME_MAX_DECIMALS)
            dec2 = DATETIME_MAX_DECIMALS;
    }

    dec = Max(dec1, dec2);
    /* End - bug#z5127 */

    val_type1 = convert_cstring_to_datetime_time(val_str1, tm1, &fsec1, &neg1);
    val_type2 = convert_cstring_to_datetime_time(val_str2, tm2, &fsec2, &neg2);

    if (!OidIsValid(val_type1))
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str1)));
        PG_RETURN_NULL();
    }

    if (!OidIsValid(val_type2))
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str1)));
        PG_RETURN_NULL();
    }

    if (val_type1 != val_type2)
    {
        // If the datetime/time types of the two parameters are different
        PG_RETURN_NULL();
    }

    switch (val_type1)
    {
        case MYSTIMEOID:
        case MYSTIMESOID:
        {
            if (tm2time(tm1, fsec1, &time1) != 0)
                ereport(ERROR,
                        (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("datetime string and format model does not match")));
            if (neg1)
            {
                time1 = -time1;
            }

            if (tm2time(tm2, fsec2, &time2) != 0)
                ereport(ERROR,
                        (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("datetime string and format model does not match")));
            if (neg2)
            {
                time2 = -time2;
            }

            res_time = time1 - time2;
            break;
        }
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
        case MYSDATEOID:
        {
            long secs = 0;
            int usecs = 0;
            int sign = 1;
            tm2timestamp(tm1, fsec1, NULL, &datetime1);
            tm2timestamp(tm2, fsec2, NULL, &datetime2);

            if (datetime1 < datetime2)
            {
                TimestampDifference(datetime1, datetime2, &secs, &usecs);
                sign = -1;
                neg = true;
            }
            else
            {
                TimestampDifference(datetime2, datetime1, &secs, &usecs);
            }
            res_time = (secs * USECS_PER_SEC + usecs) * sign;
            break;
        }
        default:
            ereport(ERROR, (errcode(ERRCODE_DATATYPE_MISMATCH),
                            errmsg("unsupported input data type: %s", format_type_be(val_type1))));
    }

    if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
        (has_sql_mode(MODE_STRICT_ALL_TABLES)))
        is_strict_mode = true;

    time2tm(res_time, res_tm, &res_fsec);

    mystime_add_nanoseconds_with_round(res_tm, &res_fsec, msec_round_add[dec], &warn);
    res_fsec = res_fsec - res_fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

    result = res_tm->tm_hour * 10000 + res_tm->tm_min * 100 + res_tm->tm_sec;

    if (result < -FORMAT_TIME_MAX_VALUE || result > FORMAT_TIME_MAX_VALUE)
    {
        if (is_strict_mode)
            ereport(ERROR, (errmsg("Truncated incorrect time value")));
        else
            ereport(WARNING, (errmsg("Truncated incorrect time value")));
        
        result = result < -FORMAT_TIME_MAX_VALUE ? -FORMAT_TIME_MAX_VALUE : FORMAT_TIME_MAX_VALUE;
        res_fsec = 0;
    }

    if (res_fsec < 0)
    {
        res_fsec = -res_fsec;
        if (result == 0)
            neg = true;
    }

    /* Keep only one negative sign */
    if (result < 0)
        neg = false;

    snprintf(buf, MAXDATELEN, "%s%ld.%06d", neg ? "-" : "", result, res_fsec);
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

    PG_RETURN_NUMERIC(num);
}

Datum 
mystime(PG_FUNCTION_ARGS)
{
    int         neg = 0,
                warn = 0,
                dec = 0,
                is_strict_mode = 0;
    char        *dot, /* bug#z5128 */
                *val_str,
                buf[MAXDATELEN + 1] = {0};
    int64       res = 0;
    bool        typIsVarlena;
    TimeADT     time, result = 0;
    Timestamp   datetime;
    Oid         val_type, typeOutput;
    fsec_t      fsec = 0;
    Numeric     num;
    struct pg_tm tt, *tm = &tt;

    MYS_ZERO_tm(tm); /* bug#5127 */

    val_type = get_fn_expr_argtype(fcinfo->flinfo, 0);
    if (!OidIsValid(val_type))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type, &typeOutput, &typIsVarlena);

    val_str = OidOutputFunctionCall(typeOutput, PG_GETARG_DATUM(0));
    
    /* Begin - bug#z5128 */
    dot = strchr(val_str, '.');
    if (dot)
    {
        dec = strlen(val_str) - (dot - val_str) - 1;

        if (dec > DATETIME_MAX_DECIMALS)
            dec = DATETIME_MAX_DECIMALS;
    }
    /* End - bug#z5128 */

    val_type = convert_cstring_to_datetime_time(val_str, tm, &fsec, &neg);

    switch (val_type)
    {
        case MYSTIMEOID:
        case MYSTIMESOID:
            if (tm2time(tm, fsec, &time) != 0)
                ereport(ERROR,
                        (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("datetime string and format model does not match")));

            result = time;
            break;
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
        case MYSDATEOID:
            tm2timestamp(tm, fsec, NULL, &datetime);

            result = DatumGetTimeADT(DirectFunctionCall1(timestamp_time, TimeADTGetDatum(datetime)));
            break;
        default:
            if (!OidIsValid(val_type))
            {
                ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str)));
                PG_RETURN_NULL();
            }

            ereport(ERROR, (errcode(ERRCODE_DATATYPE_MISMATCH),
                            errmsg("unsupported input data type: %s", format_type_be(val_type))));
    }

    if ((has_sql_mode(MODE_STRICT_TRANS_TABLES)) ||
        (has_sql_mode(MODE_STRICT_ALL_TABLES)))
        is_strict_mode = true;

    time2tm(result, tm, &fsec);

    mystime_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);

    fsec = fsec - fsec % (long)log_10_int[DATETIME_MAX_DECIMALS - dec];

    res = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;    
    
    if (res < -FORMAT_TIME_MAX_VALUE || res > FORMAT_TIME_MAX_VALUE)
    {
        ereport(is_strict_mode, (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                                 errmsg("time field value out of range")));
        res = res < -FORMAT_TIME_MAX_VALUE ? -FORMAT_TIME_MAX_VALUE : FORMAT_TIME_MAX_VALUE;
        fsec = 0;
    }
    
    if (fsec < 0 && res < 0)
    {
        fsec = -fsec;
        if (res > 0)
        {
            res = -res;
        }
    }

    snprintf(buf, MAXDATELEN, "%s%ld.%06d", neg != 0 ? "-" : "", res, fsec);
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

    PG_RETURN_NUMERIC(num);
}

static char *
time_format_text(text *time_text, text *format_text)
{
    char buf[MAXDATELEN];          /* string for temporary storage */
    char format[MAXDATELEN];       /* format string */
    char str[MAXDATELEN];          /* return string */
    char *ptr = NULL, *end = NULL; /* head and tail of format */
    char temp_ptr[2];
    int remain = MAXDATELEN; /* remaining buffer size of variable str */
    int insert_len = 0;      /* number of characters inserted into str */
    struct pg_tm tt, *tm = &tt;
    fsec_t fsec;
    int hours_i; /* hour in range of 0..12 */
    Oid val_type;
    int neg = 0;

    MYS_ZERO_tm(tm); /* bug#5127 */

    /* convert time_text and format_text into string from text */
    text_to_cstring_buffer(time_text, buf, MAXDATELEN);   
    text_to_cstring_buffer(format_text, format, MAXDATELEN);

    /* is time_text in datetime or time format? */
    val_type = convert_cstring_to_datetime_time(buf, tm, &fsec, &neg);

    if (!OidIsValid(val_type))
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", buf)));
        return NULL;
    }

    /* carry for fsec */
    if (fsec / USECS_PER_SEC)
    {
        tt.tm_sec += fsec / USECS_PER_SEC;
        tt.tm_min += tt.tm_sec / SECS_PER_MINUTE;
        tt.tm_sec %= SECS_PER_MINUTE;
        tt.tm_hour += tt.tm_min / MINS_PER_HOUR;
        tt.tm_min %= MINS_PER_HOUR;
        fsec = 0;
    }

    /* is time out of range? */
    if (tm->tm_hour == TIME_MAX_HOUR && tm->tm_min == TIME_MAX_MINUTE &&
        tm->tm_sec == TIME_MAX_SECOND && fsec > 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                 errmsg("date/time field value out of range: \"%s\"", str)));
    }

    if (neg)
    {
        str[0] = '-';
        str[1] = '\0';
        remain--;
    }
    else
    {
        str[0] = '\0';
    }

    ptr = format;
    end = ptr + strlen(format);
    for (; ptr != end; ptr++)
    {
        if (*ptr != '%' || ptr + 1 == end)
        {
            temp_ptr[0] = *ptr;
            temp_ptr[1] = '\0';
            insert_len = 1;
            strncat(str, temp_ptr, remain);
        }
        else
        {
            switch (*++ptr)
            {
                case 'a':
                case 'b':
                case 'D':
                case 'j':
                case 'M':
                case 'U':
                case 'u':
                case 'V':
                case 'v':
                case 'W':
                case 'w':
                case 'X':
                case 'x': /* return null */
                    return NULL;
                case 'c':
                case 'e':
                    insert_len = 1;
                    strncat(str, "0", remain);
                    break;
                case 'd':
                case 'm':
                case 'y':
                    insert_len = YEAR2_LEN;
                    strncat(str, "00", remain);
                    break;
                case 'Y':
                    insert_len = YEAR4_LEN;
                    strncat(str, "0000", remain);
                    break;
                case 'f':
                    insert_len = snprintf(buf, MAXDATELEN, "%06d", fsec);
                    strncat(str, buf, remain);
                    break;
                case 'H': /* hours in range of 0..24 */
                    insert_len = snprintf(buf, MAXDATELEN, "%02d", tm->tm_hour);
                    strncat(str, buf, remain);
                    break;
                case 'k': /* hours in range of 0..24 */
                    insert_len = snprintf(buf, MAXDATELEN, "%d", tm->tm_hour);
                    strncat(str, buf, remain);
                    break;
                case 'h':
                case 'I': /* hours in range of 0..12 */
                    hours_i = (tm->tm_hour % MAX_VALUE_24_CLOCK + MAX_VALUE_12_CLOCK - 1) % MAX_VALUE_12_CLOCK + MIN_VALUE_12_CLOCK;
                    insert_len = snprintf(buf, MAXDATELEN, "%02d", hours_i);
                    strncat(str, buf, remain);
                    break;
                case 'l':
                    hours_i = (tm->tm_hour % MAX_VALUE_24_CLOCK + MAX_VALUE_12_CLOCK - 1) % MAX_VALUE_12_CLOCK + MIN_VALUE_12_CLOCK;
                    insert_len = snprintf(buf, MAXDATELEN, "%d", hours_i);
                    strncat(str, buf, remain);
                    break;
                case 'i': /* minutes */
                    insert_len = snprintf(buf, MAXDATELEN, "%02d", tm->tm_min);
                    strncat(str, buf, remain);
                    break;
                case 'p':
                    hours_i = tm->tm_hour % MAX_VALUE_24_CLOCK;
                    insert_len = 2;
                    strncat(str, (hours_i < MAX_VALUE_12_CLOCK ? "AM" : "PM"), remain);
                    break;
                case 'r':
                    insert_len =
                        snprintf(buf, MAXDATELEN,
                                (((tm->tm_hour % MAX_VALUE_24_CLOCK) < MAX_VALUE_12_CLOCK) ? "%02d:%02d:%02d AM" : "%02d:%02d:%02d PM"),
                                (tm->tm_hour + MAX_VALUE_12_CLOCK - 1) % MAX_VALUE_12_CLOCK + MIN_VALUE_12_CLOCK,
                                tm->tm_min, tm->tm_sec);
                    strncat(str, buf, remain);
                    break;
                case 'S':
                case 's': /* seconds */
                    insert_len = snprintf(buf, MAXDATELEN, "%02d", tm->tm_sec);
                    strncat(str, buf, remain);
                    break;
                case 'T':
                    insert_len = snprintf(buf, MAXDATELEN, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
                    strncat(str, buf, remain);
                    break;
                default:
                    temp_ptr[0] = *ptr;
                    temp_ptr[1] = '\0';
                    insert_len = 1;
                    strncat(str, temp_ptr, remain);
                    break;
            }
        }

        if (insert_len >= remain)
        {
            ereport(ERROR,
                (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                 errmsg("format_result field value out of range!")));
        }
        
        remain -= insert_len;
    }

    return strdup(str);
}

Datum 
time_format(PG_FUNCTION_ARGS)
{
    char *res;
    text *val_text1 = PG_GETARG_TEXT_PP(0);
    text *val_text2 = PG_GETARG_TEXT_PP(1);

    res = time_format_text(val_text1, val_text2);

    if (res)
    {
        PG_RETURN_TEXT_P(cstring_to_text(res));
    }
    
    PG_RETURN_NULL();
}

Datum 
timestamp_arg1(PG_FUNCTION_ARGS)
{
    text    *date_text = PG_GETARG_TEXT_PP(0);
    char    *dot,
            *val_str = text_to_cstring(date_text);
    int     dec = 0;
    fsec_t  fsec = 0;
    TimeOffset time = 0;
    int64   datetemp = 0;
    Numeric num, precision_base;
    Datum   cstr;
    TimeADT timestamp = -1;
    struct pg_tm tt, *tm = &tt;

    MYS_ZERO_tm(tm); /* bug#5127 */

    /* Begin - bug#z5129 */
    dot = strchr(val_str, '.');
    if (dot)
    {
        dec = strlen(val_str) - (dot - val_str) - 1;

        if (dec > DATETIME_MAX_DECIMALS)
            dec = DATETIME_MAX_DECIMALS;
    }
    /* End - bug#z5129 */

    convert_cstring_to_pg_tm(val_str, tm, &fsec);

    datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
    datetemp = datetemp * 1000000;
    time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
    timestamp = datetemp + time;

    /**
     * Begin - bug#z5129 
     * Set precision to 7, which is bigger than DATETIME_MAX_DECIMALS.
     */
    precision_base = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                                         CStringGetDatum("1000000.0000000"),
                                                         ObjectIdGetDatum(InvalidOid),
                                                         Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));
    /* End - bug#z5129 */

    num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(timestamp)), NumericGetDatum(precision_base)));
    num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num), NumericGetDatum(int64_to_numeric(fsec))));
    num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num), NumericGetDatum(int64_to_numeric(1000000))));

    /** 
     * Begin - bug#z5129 
     * Recompute result's precision
     */
    cstr = DirectFunctionCall1(numeric_out, PointerGetDatum(num));

    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              cstr,
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));
    /* End - bug#z5129 */

    PG_RETURN_NUMERIC(num);
}

Datum 
timestamp_arg2(PG_FUNCTION_ARGS)
{
    TimeADT time2;
    Timestamp datetime1, res_datetime;
    struct pg_tm tt1, *tm1 = &tt1, tt2, *tm2 = &tt2, res_tt, *res_tm = &res_tt;
    fsec_t fsec1, fsec2, res_fsec;
    text *date_text1 = PG_GETARG_TEXT_PP(0);
    char *val_str1 = text_to_cstring(date_text1);
    text *date_text2 = PG_GETARG_TEXT_PP(0);
    char *val_str2 = text_to_cstring(date_text2);
    TimeOffset time = 0;
    int64 datetemp = 0;
    Numeric num;
    int warn = 0;
    int dterr;
    TimeADT timestamp = -1;

    /* Begin - bug#5127 */
    MYS_ZERO_tm(tm1);
    MYS_ZERO_tm(tm2);
    MYS_ZERO_tm(res_tm);
    /* End - bug#5127 */

    convert_cstring_to_pg_tm(val_str1, tm1, &fsec1);
    tm2timestamp(tm1, fsec1, NULL, &datetime1);

    convert_cstring_to_pg_tm(val_str2, tm2, &fsec2);
    tm2time(tm2, fsec2, &time2);

    if (datetime_sub_time(datetime1, -time2, &res_datetime))
    {
        /* The variable datetime or result does not exceed the specified range */
        if (res_datetime >= FORMAT_TIMESTAMP_FIRST_YEAR)
        {
            /* The variable datetime or result does not exceed the specified range ## datetime_text/*/

            timestamp2tm(res_datetime, NULL, res_tm, &res_fsec, NULL, NULL);

            if (res_tm->tm_year == 0 && res_tm->tm_mon == 0 && res_tm->tm_mday == 0)
            {
                res_tm->tm_year = 1000;
                res_tm->tm_mon = 01;
                res_tm->tm_mday = 01;
            }

            dterr = mysdatetime_rounded_seconds(res_tm, &res_fsec, &warn);
            if (dterr)
            {
                ereport(WARNING, (errmsg("timestamp field overflow.")));
                PG_RETURN_NULL();
            }
            datetemp = (res_tm->tm_year * 10000 + res_tm->tm_mon * 100 + res_tm->tm_mday);
            datetemp = datetemp * 1000000;
            time = res_tm->tm_hour * 10000 + res_tm->tm_min * 100 + res_tm->tm_sec;
            timestamp = datetemp + time;

            num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(int64_to_numeric(timestamp)),
                                                      NumericGetDatum(int64_to_numeric(1000000))));
            num = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(num),
                                                      NumericGetDatum(int64_to_numeric(res_fsec))));
            num = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(num),
                                                      NumericGetDatum(int64_to_numeric(1000000))));

            PG_RETURN_NUMERIC(num);
        }
        PG_RETURN_NULL();
    }

    ereport(ERROR, (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                    errmsg("date/time field overflow")));

    PG_RETURN_NULL();
}

/**
 * return -1 if the result is a normal negtive value
 * reutrn 0 if the result >= 0
 */
static long long 
str2ll_with_endptr(char *start, int tmp_len, int *true_len, int *error)
{
    char *cp = (char *)palloc(sizeof(char) * (tmp_len + 1));
    char *endptr = NULL;
    long long value;
    errno = 0;

    for (int i = 0; i < tmp_len; ++i)
    {
        cp[i] = *(start + i);
    }

    cp[tmp_len] = '\0';
    value = strtoll(cp, &endptr, 10);

    if (endptr == cp)
        *error = EDOM;
    else if (errno == ERANGE)
        *error = ERANGE;
    else if (value < 0)
        *error = -1;
    else if (value >= 0)
        *error = 0;

    *true_len = (int)(endptr - cp);

    return value;
}

static TimeOffset
time2t(const int hour, const int min, const int sec, const fsec_t fsec)
{
    return (((((hour * MINS_PER_HOUR) + min) * SECS_PER_MINUTE) + sec) * USECS_PER_SEC) + fsec;
}

static bool 
calc_interval(Interval *span, lldiv_t arg, int dtk)
{
    int64 integer_part = arg.quot;
    int64 frac_part = arg.rem;
    int64 carry = ((abs(frac_part) / FRAC_PART_LEN_IN_NUMERICSEC >= ROUNDING_BORDER) ? 1 : 0);
    int64 signal = ((integer_part >= 0 && frac_part >= 0) ? 1 : -1);

    switch (dtk)
    {
        case DTK_YEAR:
            span->month = (integer_part + carry * signal) * MONTHS_PER_YEAR;
            break;
        case DTK_MONTH:
            span->month = (integer_part + carry * signal);
            break;
        case DTK_QUARTER:
            span->month = (integer_part + carry * signal) * MONTHS_PER_QUARTER;
            break;
        case DTK_DAY:
            span->day = (integer_part + carry * signal);
            break;
        case DTK_WEEK:
            span->day = (integer_part + carry * signal) * DAYS_PER_WEEK;
            break;
        case DTK_HOUR:
            span->time = time2t((integer_part + carry * signal), 0, 0, 0);
            break;
        case DTK_MINUTE:
            span->time = time2t(0, (integer_part + carry * signal), 0, 0);
            break;
        case DTK_SECOND:
            frac_part /= NANOSEC_PER_USEC;
            span->time = time2t(0, 0, integer_part, frac_part);
            break;
        case DTK_MICROSEC:
            span->time = time2t(0, 0, 0, (integer_part + carry * signal));
            break;
        default:
            return false;
    }

    return true;
}

static TimeADT 
time_sub_interval(TimeADT time, Interval *span)
{
    TimeADT result;

    if (span->month || span->day < -FORMAT_TIME_MAX_VALUE_TO_DAY ||
        span->day > FORMAT_TIME_MAX_VALUE_TO_DAY)
    {
        ereport(ERROR, (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("time field value out of range")));
        return FORMAT_TIME_MAX_VALUE;
    }
    
    result = time - (span->day * SECS_PER_DAY * USECS_PER_SEC + span->time);
    if (result < -FORMAT_TIME_MAX_VALUE || result > FORMAT_TIME_MAX_VALUE)
    {
        ereport(ERROR, (errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
                        errmsg("time field value out of range")));
        return FORMAT_TIME_MAX_VALUE;
    }

    return result;
}

static bool 
determine_conversion(int dtk)
{
    switch (dtk)
    {
        case DTK_HOUR:
        case DTK_MINUTE:
        case DTK_SECOND:
        case DTK_MICROSEC:
            return true;
        default:
            return false;
    }
}

static bool 
datetime_sub_interval(Timestamp datetime, Interval *span, Timestamp *result, bool is_add_func)
{
    if (datetime < FORMAT_TIMESTAMP_MIN_VALUE || datetime > FORMAT_TIMESTAMP_MAX_VALUE)
        return false;

    *result = DirectFunctionCall2(timestamp_mi_interval, TimestampGetDatum(datetime), PointerGetDatum(span));

    if (*result < FORMAT_TIMESTAMP_MIN_VALUE || *result > FORMAT_TIMESTAMP_MAX_VALUE)
        return false;

    if (span->time == 0 && span->day == 0)
        return true;

    if (*result < FORMAT_TIMESTAMP_FIRST_YEAR)
    {
        if (is_add_func)
        {
            struct pg_tm tt, *tm = &tt;
            fsec_t fsec;

            MYS_ZERO_tm(tm); /* bug#5127 */

            timestamp2tm(*result, NULL, tm, &fsec, NULL, NULL);
            tm->tm_year = 0;
            tm->tm_mon = 0;
            tm->tm_mday = 0;
            tm2timestamp(tm, fsec, NULL, result);
            return true;
        }
        
        return false;
    }

    return true;
}

static Datum
timestamp_add_internal(FunctionCallInfo fcinfo, char *lowunits, int unit, int unit_type, Numeric num)
{
    Oid expr_type = get_fn_expr_argtype(fcinfo->flinfo, 2);
    char *expr;
    Timestamp datetime, res_datetime;
    TimeADT time, res_time;
    lldiv_t delta;
    bool typIsVarlena;
    Oid typeOutput;
    struct pg_tm tt, *tm = &tt, res_tt, *res_tm = &res_tt;
    fsec_t fsec = 0, res_fsec = 0;
    char buf[MAXDATELEN + 1] = {0};
    text *res;
    Interval sp, *span = &sp;
    bool conversion;

    /* Begin - bug#5127 */
    MYS_ZERO_tm(tm);
    MYS_ZERO_tm(res_tm);
    /* End - bug#5127 */

    if (!OidIsValid(expr_type))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(expr_type, &typeOutput, &typIsVarlena);
    expr = OidOutputFunctionCall(typeOutput, PG_GETARG_DATUM(2));
    expr_type = convert_cstring_to_pg_tm(expr, tm, &fsec);

    if (!mysnumeric_to_lldiv_t(num, &delta))
    {
        ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE), errmsg("interval out of range")));
    }

    span->time = span->day = span->month = 0;

    if (unit_type == UNITS)
    {
        if (!calc_interval(span, delta, unit))
        {
            ereport(ERROR, (errcode(ERRCODE_ERROR_IN_ASSIGNMENT), errmsg("failed to calculate interval")));
        }
    }
    else
    {
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_PARAMETER), errmsg("unknown unit: \"%s\"", lowunits)));
    }

    /*Call function timestamp_mi_interval to sub span*/
    span->time = -span->time;
    span->day = -span->day;
    span->month = -span->month;

    conversion = determine_conversion(unit);

    switch (expr_type)
    {
        // when the input type is time or timetz
        case MYSTIMEOID:
        case MYSTIMESOID:
            {
                tm2time(tm, fsec, &time);
                res_time = time_sub_interval(time, span);
                if (res_time == 0)
                {
                    snprintf(buf, MAXDATELEN, "00:00:00");
                }
                else if (res_time > 0)
                {
                    time2tm(res_time, res_tm, &res_fsec);
                    snprintf(buf, MAXDATELEN, "%02d:%02d:%02d%06d", res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec, res_fsec);
                }
                else
                {
                    time2tm(res_time, res_tm, &res_fsec);
                    snprintf(buf, MAXDATELEN, "-%02d:%02d:%02d%06d", res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec, res_fsec);
                }
                res = cstring_to_text(buf);
                PG_RETURN_TEXT_P(res);
            }
        case MYSDATEOID:
            if (!conversion)
            {
                tm2timestamp(tm, fsec, NULL, &datetime);
                if (datetime_sub_interval(datetime, span, &res_datetime, false))
                {
                    timestamp2tm(res_datetime, NULL, res_tm, &res_fsec, NULL, NULL);
                    snprintf(buf, MAXDATELEN, "%04d-%02d-%02d", res_tm->tm_year, res_tm->tm_mon, res_tm->tm_mday);

                    res = cstring_to_text(buf);
                    PG_RETURN_TEXT_P(res);
                }
                break;
            }
            /* fallthrough */
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
        case MYSTIMESTAMPOID:
        case MYSTIMESTAMPSOID:
            tm2timestamp(tm, fsec, NULL, &datetime);
            if (datetime_sub_interval(datetime, span, &res_datetime, false))
            {
                timestamp2tm(res_datetime, NULL, res_tm, &res_fsec, NULL, NULL);
                snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d%06d", res_tm->tm_year, res_tm->tm_mon, res_tm->tm_mday,
                        res_tm->tm_hour, res_tm->tm_min, res_tm->tm_sec, res_fsec);

                res = cstring_to_text(buf);
                PG_RETURN_TEXT_P(res);
            }
            break;
        default:
            ereport(ERROR, (errcode(ERRCODE_DATATYPE_MISMATCH),
                            errmsg("unsupported input data type: %s", format_type_be(expr_type))));
    }

    ereport(ERROR, (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE), errmsg("date/time field overflow")));

    PG_RETURN_NULL();
}

Datum 
timestampadd(PG_FUNCTION_ARGS)
{
    Numeric num;
    int true_len, errorno;
    long long ret;
    text *date_text1 = PG_GETARG_TEXT_PP(0);
    char *val_str1 = text_to_cstring(date_text1);
    char *num_str;
    text *units = cstring_to_text(val_str1);
    char *lowunits = NULL;
    int unit = 0;
    int unit_type;
    Interval sp, *span = &sp;
    Oid val_type2, typeOutput2;
    bool typIsVarlena2;

    val_type2 = get_fn_expr_argtype(fcinfo->flinfo, 1);

    if (!OidIsValid(val_type2))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("INVALID TYPE!")));

    getTypeOutputInfo(val_type2, &typeOutput2, &typIsVarlena2);
    num_str = OidOutputFunctionCall(typeOutput2, PG_GETARG_DATUM(1));

    lowunits = downcase_truncate_identifier(VARDATA_ANY(units), VARSIZE_ANY_EXHDR(units), false);

    span->time = span->day = span->month = 0;

    /* parse unit */
    unit_type = DecodeUnits(0, lowunits, &unit);

    if (unit_type == UNKNOWN_FIELD)
    {
        unit_type = DecodeSpecial(0, lowunits, &unit);
    }

    /* check the format of num_str */
    ret = str2ll_with_endptr(num_str, strlen(num_str), &true_len, &errorno);
    if (errorno == EDOM)
    {
        ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
                        errmsg("Truncated incorrect INTEGER value: %s", num_str)));
    }

    /* str to Numeric*/
    if (unit == DTK_SECOND)
    {
        num = DatumGetNumeric(DirectFunctionCall3(numeric_in, CStringGetDatum(num_str), ObjectIdGetDatum(0), Int32GetDatum(-1)));
    }
    else
    {
        num = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(ret)));
    }

    return timestamp_add_internal(fcinfo, lowunits, unit, unit_type, num);
}
