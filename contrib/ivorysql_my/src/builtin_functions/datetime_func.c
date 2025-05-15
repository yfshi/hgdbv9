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

#include <float.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "access/xact.h"
#include "c.h"
#include "catalog/pg_namespace_d.h"
#include "datatype/timestamp.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "parser/scansup.h"
#include "pgtime.h"
#include "sys/types.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "../include/ivorysql_my.h"
#include "../include/datetime_func.h"

PG_FUNCTION_INFO_V1(ivymy_dayofmonth);
PG_FUNCTION_INFO_V1(ivymy_dayofweek);
PG_FUNCTION_INFO_V1(ivymy_dayofyear);
PG_FUNCTION_INFO_V1(ivymy_dayofmonth_text);
PG_FUNCTION_INFO_V1(ivymy_dayofweek_text);
PG_FUNCTION_INFO_V1(ivymy_dayofyear_text);
PG_FUNCTION_INFO_V1(ivymy_quarter_text);
PG_FUNCTION_INFO_V1(ivymy_dayofmonth_num);
PG_FUNCTION_INFO_V1(ivymy_dayofweek_num);
PG_FUNCTION_INFO_V1(ivymy_dayofyear_num);
PG_FUNCTION_INFO_V1(ivymy_quarter_num);
PG_FUNCTION_INFO_V1(ivymy_quarter);
PG_FUNCTION_INFO_V1(GetHour);
PG_FUNCTION_INFO_V1(GetMicrosecond);
PG_FUNCTION_INFO_V1(GetMinute);
PG_FUNCTION_INFO_V1(GetSecond);
PG_FUNCTION_INFO_V1(GetHour_text);
PG_FUNCTION_INFO_V1(GetMicrosecond_text);
PG_FUNCTION_INFO_V1(GetMinute_text);
PG_FUNCTION_INFO_V1(GetSecond_text);
PG_FUNCTION_INFO_V1(ivymy_weekday);
PG_FUNCTION_INFO_V1(ivymy_weekofyear);
PG_FUNCTION_INFO_V1(ivymy_year);
PG_FUNCTION_INFO_V1(ivymy_year_text);
PG_FUNCTION_INFO_V1(ivymy_curtime_fsp);
PG_FUNCTION_INFO_V1(ivymy_curdate);
PG_FUNCTION_INFO_V1(ivymy_now_fsp);


#define MAX_PRECISION 6
#define MAX_YEAR_OF_DATE 9999
#define TIME_MAX_FRAC 999999

static unsigned long long pow_of_10[20] = {
    1, 10, 100, 1000, 10000UL, 100000UL, 1000000UL, 10000000UL, 
    100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL, 
    1000000000000ULL, 10000000000000ULL, 100000000000000ULL, 
    1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL, 
    1000000000000000000ULL, 10000000000000000000ULL};

static int msec_round_add[7] = {
        500000000, 
        50000000, 
        5000000, 
        500000, 
        50000, 
        5000, 
        0};

static int64 log_10_int[20] = {
        1, 10, 100, 1000, 10000UL, 100000UL, 1000000UL, 10000000UL, 
        100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL, 
        1000000000000ULL, 10000000000000ULL, 100000000000000ULL, 
        1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL, 
        1000000000000000000ULL, 10000000000000000000ULL};

static Datum ivymy_dayof_date(FunctionCallInfo fcinfo, int datekind);
static Datum ivymy_dayof_text(FunctionCallInfo fcinfo, int datekind);
static Datum ivymy_dayof_num(FunctionCallInfo fcinfo, int datekind);

static int64 getPartFromTm(struct pg_tm *tm, fsec_t fsec, int part);
static Datum GetSpecificPartOfTime(FunctionCallInfo fcinfo, int part);
static Datum GetSpecificPartOfTime_text(FunctionCallInfo fcinfo, int part);
static bool mystr_to_time(char *str, struct pg_tm *tm, fsec_t *fsec, int *timeSign, mysql_dttype *tm_type, int *warnings, bool *null_func_result);

static Datum
ivymy_dayof_date(FunctionCallInfo fcinfo, int datekind)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

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

static Datum
ivymy_dayof_text(FunctionCallInfo fcinfo, int datekind)
{
    text *arg_text = PG_GETARG_TEXT_PP(0);
    char *str = text_to_cstring(arg_text);
    int32 intresult = 0;
    mysql_dttype tm_type = MYSQL_TIMESTAMP_NONE;
    struct pg_tm result_tt, *result_tm = &result_tt;
    fsec_t fsec;

    if (!str_datetime_in_with_flag_internal(str, result_tm, &fsec, &tm_type))
    {
        PG_RETURN_NULL();
    }

    switch (datekind)
    {
        case WEEK:
            intresult = j2day(date2j(result_tm->tm_year, result_tm->tm_mon, result_tm->tm_mday)) + 1;
            break;

        case MONTH:
            intresult = result_tt.tm_mday;
            break;

        case YEAR:
            intresult = (date2j(result_tm->tm_year, result_tm->tm_mon, result_tm->tm_mday) - date2j(result_tm->tm_year, 1, 1) + 1);
            break;

        case QUARTER:
            intresult = (result_tm->tm_mon - 1) / 3 + 1;
            break;

        default:
            PG_RETURN_NULL();
    }

    PG_RETURN_INT32(intresult);
}

static Datum
ivymy_dayof_num(FunctionCallInfo fcinfo, int datekind)
{
    Numeric num = PG_GETARG_NUMERIC(0);
    int32 intresult = 0;
    mysql_dttype tm_type = MYSQL_TIMESTAMP_NONE;
    struct pg_tm result_tt, *result_tm = &result_tt;
    fsec_t fsec;

    if (!num_datetime_in_with_flag_internal(num, result_tm, &fsec, &tm_type))
    {
        PG_RETURN_NULL();
    }

    switch (datekind)
    {
        case WEEK:
            intresult = j2day(date2j(result_tm->tm_year, result_tm->tm_mon, result_tm->tm_mday)) + 1;
            break;

        case MONTH:
            intresult = result_tt.tm_mday;
            break;

        case YEAR:
            intresult = (date2j(result_tm->tm_year, result_tm->tm_mon, result_tm->tm_mday) - date2j(result_tm->tm_year, 1, 1) + 1);
            break;

        case QUARTER:
            intresult = (result_tm->tm_mon - 1) / 3 + 1;
            break;

        default:
            PG_RETURN_NULL();
    }

    PG_RETURN_INT32(intresult);
}

Datum 
ivymy_dayofmonth(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, MONTH));
}

Datum 
ivymy_dayofweek(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, WEEK));
}

Datum 
ivymy_dayofyear(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, YEAR));
}

Datum 
ivymy_quarter(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_date(fcinfo, QUARTER));
}

Datum
ivymy_dayofmonth_text(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_text(fcinfo, MONTH));
}

Datum 
ivymy_dayofweek_text(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_text(fcinfo, WEEK));
}

Datum 
ivymy_dayofyear_text(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_text(fcinfo, YEAR));
}

Datum 
ivymy_quarter_text(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_text(fcinfo, QUARTER));
}

Datum 
ivymy_dayofmonth_num(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_num(fcinfo, MONTH));
}

Datum 
ivymy_dayofweek_num(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_num(fcinfo, WEEK));
}

Datum 
ivymy_dayofyear_num(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_num(fcinfo, YEAR));
}

Datum 
ivymy_quarter_num(PG_FUNCTION_ARGS)
{
    return (ivymy_dayof_num(fcinfo, QUARTER));
}

static bool
mystr_to_time(char *str, struct pg_tm *tm, fsec_t *fsec, int *timeSign, mysql_dttype *tm_type, int *warnings, bool *null_func_result)
{
    size_t length = strlen(str);
    unsigned long date[4]; /* 0~3 correspond to: days, hours, minutes, seconds in turn*/
    unsigned int pos;
    int nano = 0;
    const char *end = str + length, *end_of_days;
    bool found_days, found_hours;
    uint64 value;
    int fsec_len = 0;
    bool is_internal_format = false; /* If true, str is a number format(HHMMSS[.fsec]) */

    /* init */
    *fsec = 0;
    *warnings = false;

    /* Skip space at start */
    for (; str != end && isspace((unsigned char)*str); str++)
        length--;

    /* Determine positive or negative time */
    if (str != end && *str == '-')
    {
        *timeSign = -1;
        str++;
        length--;
    }

    if (str == end)
    {
        *null_func_result = true;
        *warnings = true;
        return false; // error format
    }

    /* Check first if str is a full TIMESTAMP */
    if (length >= 12)
    {
        mystr_to_datetime(str, strlen(str), tm, (TIME_DATETIME_ONLY | TIME_DATETIME_ONLY), fsec, tm_type, warnings, &nano, &fsec_len);
        if (nano >= 500)
        {
            *fsec += 1; /* round */
        }

        if (*tm_type >= MYSQL_TIMESTAMP_ERROR)
        {
            *tm_type = MYSQL_TIMESTAMP_ERROR;
            return *tm_type;
        }

        *fsec = 0;
        nano = 0;
        *warnings = false;
        *null_func_result = false;
    }

    /* Not a timestamp. Try to get this as a DAYS_TO_SECOND string */
    for (value = 0; str != end && isdigit((unsigned char)*str); str++)
    {
        value = value * 10L + (long)(*str - '0');
    }

    if (value > UINT_MAX)
    {
        *null_func_result = true;
        return false;
    }

    /* Skip all space after 'days' */
    end_of_days = str;
    for (; str != end && isspace((unsigned char)*str); str++)
        ;

    pos = found_days = found_hours = 0;
    if ((end - str) > 1 && str != end_of_days && isdigit((unsigned char)*str))
    {
        /* Found days part */
        date[0] = (unsigned long)value;
        pos = 1; /* Assume next is hours */
        found_days = 1;
    }
    else if ((end - str) > 1 && *str == ':' && isdigit((unsigned char)*(str + 1)))
    {
        date[0] = 0; /* not found days part. day set to 0 */
        date[1] = (unsigned long)value;
        pos = 2; /* Assume next is minutes */
        found_hours = 1;
        str++;
    }
    else
    {
        /* str is a number format */
        date[0] = 0;
        date[1] = (unsigned long)(value / 10000);
        date[2] = (unsigned long)(value / 100 % 100);
        date[3] = (unsigned long)(value % 100);
        pos = 4;
        is_internal_format = true;
    }

    if (!is_internal_format)
    {
        for (;;)
        {
            /* Parse the remaining time parts (excluding the fractional second part) */
            for (value = 0; str != end && isdigit((unsigned char)*str); str++)
            {
                value = value * 10L + (long)(*str - '0');
            }
            date[pos++] = (unsigned long)value;
            if (pos == 4 || (end - str) < 2 || *str != ':' || !isdigit((unsigned char)*(str + 1)))
            {
                break;
            }
            str++;
        }
    }

    /* Parse fractional second part */
    pos = 4;
    if (str != end && *str == '.')
    {
        int field_length = MAX_TIME_PRECISION;
        str++;
        for (value = 0; str != end && isdigit((unsigned char)*str); str++)
        {
            if (field_length-- > 0)
            {
                value = value * 10L + (long)(*str - '0');
            }
        }
        if (field_length >= 0)
        {
            value *= (long)pow_of_10[field_length];
        }
        else
        {
            /* Compatible with MySQL's special nanosecond carry mode */
            nano = (int)(*(str - 1) - '0') * pow_of_10[2];
        }
        *fsec = (int32)value;
    }

    /* field overflow checks */
    if (date[0] > INT_MAX || date[1] > INT_MAX || date[2] > INT_MAX || date[3] > INT_MAX)
    {
        *null_func_result = true;
        return false; // overflow error
    }

    /* set pg_tm and type*/
    tm->tm_year = 0;
    tm->tm_mon = 0;
    tm->tm_mday = date[0];
    tm->tm_hour = date[1];
    tm->tm_min = date[2];
    tm->tm_sec = date[3];
    *tm_type = DTK_TIME;

    if (!(tm->tm_hour >= 0 && tm->tm_min >= 0 && tm->tm_min <= TIME_MAX_MINUTE && tm->tm_sec >= 0 &&
          tm->tm_sec <= TIME_MAX_SECOND && *fsec >= 0 && *fsec <= TIME_MAX_FRAC))
    {
        *null_func_result = true;
        *warnings = true; // warning: out of range
        return false;     // error: out of range
    }

    if (nano >= 500)
    {
        *fsec += 1; /* round */
    }

    /* adjust time into supported time range*/
    adjust_time_range(tm, fsec, warnings);
    tm->tm_hour += HOURS_PER_DAY * tm->tm_mday;

    while (str != end)
    {
        if (!isspace((unsigned char)*str))
        {
            *warnings = true;
            break;
        }
        str++;
    }

    return true;
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

static Datum
GetSpecificPartOfTime_text(FunctionCallInfo fcinfo, int part)
{
    int errlevel = ((is_strict_mode()) ? ERROR : WARNING);
    struct pg_tm tt;
    struct pg_tm *tm = &tt;
    fsec_t fsec = 0;
    int timeSign = 1;
    int warnings = false;
    mysql_dttype tm_type = MYSQL_TIMESTAMP_NONE;
    bool null_func_result = false;
    text *arg_text = PG_GETARG_TEXT_PP(0);
    char *str = text_to_cstring(arg_text);

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    if (!mystr_to_time(str, tm, &fsec, &timeSign, &tm_type, &warnings, &null_func_result) || null_func_result)
    {
        PG_RETURN_NULL();
    }
    
    if (warnings)
    {
        errlevel = (((is_strict_mode()) || null_func_result) ? ERROR : WARNING);
        ereport(errlevel,
                (errcode(DTERR_BAD_FORMAT), errmsg("Truncated incorrect time value: \"%s\"", str)));
    }
    PG_RETURN_INT64(getPartFromTm(tm, fsec, part));
}

Datum
GetHour(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, HOUR);
}

Datum
GetMicrosecond(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, MICROSECOND);
}

Datum 
GetMinute(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, MINUTE);
}

Datum 
GetSecond(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime(fcinfo, SECOND);
}

Datum 
GetHour_text(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime_text(fcinfo, HOUR);
}

Datum 
GetMicrosecond_text(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime_text(fcinfo, MICROSECOND);
}

Datum 
GetMinute_text(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime_text(fcinfo, MINUTE);
}

Datum 
GetSecond_text(PG_FUNCTION_ARGS)
{
    return GetSpecificPartOfTime_text(fcinfo, SECOND);
}

Datum 
ivymy_weekday(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

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
ivymy_weekofyear(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 intresult = 0;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

    part1 = (mysdate / 10000);
    part2 = (mysdate - part1 * 10000);
    tm->tm_year = part1;

    tm->tm_mon = part2 / 100;
    tm->tm_mday = part2 % 100;

    intresult = date2isoweek(tm->tm_year, tm->tm_mon, tm->tm_mday);
    PG_RETURN_INT32(intresult);
}

Datum 
ivymy_year(PG_FUNCTION_ARGS)
{
    DateADT mysdate = PG_GETARG_DATEADT(0);
    struct pg_tm tt, *tm = &tt;
    int64 part1, part2;

    if (mysdate == 0 || mysdate == MYSQL_ZERO_DATE)
        PG_RETURN_NULL();

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
ivymy_year_text(PG_FUNCTION_ARGS)
{
    struct pg_tm tt, *tm = &tt;
    text *arg_text = PG_GETARG_TEXT_PP(0);
    char *str = text_to_cstring(arg_text);
    mysql_dttype tm_type = MYSQL_TIMESTAMP_NONE;
    fsec_t fsec;
    int fsec_len = 0;
    int warning = 0;
    int flag = TIME_FLAG_INIT;
    int nanoseconds = 0;
    bool nonsec = false;
    int dterr;

    if (has_sql_mode(MODE_NO_ZERO_DATE))
        flag |= TIME_NO_ZERO_DATE;

    if (has_sql_mode(MODE_INVALID_DATES))
        flag |= TIME_INVALID_DATES;

    dterr = mystr_to_datetime(str, strlen(str), tm, flag, &fsec, &tm_type, &warning, &nanoseconds, &fsec_len);
    nonsec = mysdatetime_add_nanoseconds_with_round(tm, &fsec, &warning, nanoseconds);

    if (dterr || warning || nonsec)
    {
        if ((MYSQL_TIME_WARN_TRUNCATED & warning) && !dterr)
            ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
        else if ((MYSQL_TIME_WARN_OUT_OF_RANGE & warning) && nonsec)
        {
            ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
            ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
        }
        else if (MYSQL_TIME_WARN_ZERO_IN_DATE & warning)
            ereport(WARNING, (errmsg("Truncated incorrect datetime value: '%s'", str)));
        else
            ereport(WARNING, (errmsg("Incorrect datetime value: '%s'", str)));

        // ERROR 1064 (42000): You have an error in your SQL syntax
        if (dterr < 0 || nonsec)
            PG_RETURN_NULL();
    }

    if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
    {
        PG_RETURN_NULL();
    }

    if (tm->tm_year <= MAX_YEAR_OF_DATE)
        PG_RETURN_INT64(tm->tm_year);

    PG_RETURN_NULL();
}

/* ivymy_curdate()
 * reruen  current date in b compatibility,    date
 */
Datum 
ivymy_curdate(PG_FUNCTION_ARGS)
{
    TimestampTz state_start_timestamp;
    struct pg_tm tt, *tm = &tt;
    DateADT date;
    fsec_t fsec;
    int tz;

    state_start_timestamp = GetCurrentStatementStartTimestamp();

    if (timestamp2tm(state_start_timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
    {
        ereport(ERROR, (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                        errmsg("date out of range")));
    }

    if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
    {
        date = MYSQL_ZERO_DATE;
    }
    else
    {
        date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;
    }

    PG_RETURN_DATEADT(date);
}

Datum 
ivymy_curtime_fsp(PG_FUNCTION_ARGS)
{
    int32 typmod = PG_GETARG_INT32(0);
    TimestampTz state_start_timestamp;
    int64 time = 0;
    struct pg_tm tt, *tm = &tt;
    fsec_t fsec = 0;
    int tz = 0;
    int warn = 0;
    int is_strict_mode = 0;
    char buf[MAXDATELEN + 1] = {0};
    Numeric num;

    if (typmod > MAX_PRECISION)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Too-big precision %d specified for \'curtime\'. Maximum is 6.", typmod)));
    }

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

/* ivymy_now_fsp()
 * return current timestamp in b compatibility
 */
Datum 
ivymy_now_fsp(PG_FUNCTION_ARGS)
{
    int32 typmod = PG_GETARG_INT32(0);
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
    if (typmod > MAX_PRECISION)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Too-big precision %d specified for \'now\'. Maximum is 6.", typmod)));
    }

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
