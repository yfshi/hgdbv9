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
#include "../include/mys_locale.h"

PG_FUNCTION_INFO_V1(str_to_date);
PG_FUNCTION_INFO_V1(timestampdiff);
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

PG_FUNCTION_INFO_V1(to_days);
PG_FUNCTION_INFO_V1(to_seconds);
PG_FUNCTION_INFO_V1(unix_timestamp);
PG_FUNCTION_INFO_V1(utc_date_func);
PG_FUNCTION_INFO_V1(utc_time_func);
PG_FUNCTION_INFO_V1(utc_timestamp_func);
PG_FUNCTION_INFO_V1(dayname);
PG_FUNCTION_INFO_V1(monthname);
PG_FUNCTION_INFO_V1(time_to_sec);
PG_FUNCTION_INFO_V1(mys_month);
PG_FUNCTION_INFO_V1(mys_date);
PG_FUNCTION_INFO_V1(mys_week);
PG_FUNCTION_INFO_V1(mys_week_mode);
PG_FUNCTION_INFO_V1(yearweek);
PG_FUNCTION_INFO_V1(datediff);
PG_FUNCTION_INFO_V1(from_days);
PG_FUNCTION_INFO_V1(convert_tz);
PG_FUNCTION_INFO_V1(get_format);

#define MICRO_PER_NANOSEC 1000
#define FORMAT_TIME_BOUND 838
#define FORMAT_TIME_MAX_VALUE (FORMAT_TIME_BOUND * 10000 + 59 * 100 + 59)
#define FORMAT_TIMESTAMP_MAX_VALUE INT64CONST(252455615999999999)  /* datetime 9999-12-31 23:59:59.999999 */
#define FORMAT_TIMESTAMP_MIN_VALUE INT64CONST(-63113904000000000)  /* datetime 0000-01-01 00:00:00.000000 */
#define FORMAT_TIMESTAMP_FIRST_YEAR INT64CONST(-63082281600000000) /* datetime 0001-01-01 00:00:00.000000 */
#define TIME_MAX_FRAC 999999
#define MAX_YEAR_OF_DATE 9999
#define MIN_YEAR_OF_DATE 0
#define YEAR2_ADJUST_VALUE_20C_21C(x) ((x) + ((x) >= YY_PART_YEAR ? 1900 : 2000))

#define MAX_VALUE_12_CLOCK (HOURS_PER_DAY / 2)
#define MIN_VALUE_12_CLOCK 1
#define MAX_VALUE_24_CLOCK (HOURS_PER_DAY)
#define FRAC_PART_LEN_IN_NUMERICSEC 100000000
#define ROUNDING_BORDER 5
#define MONTHS_PER_QUARTER 3
#define DAYS_PER_WEEK 7
#define NANOSEC_PER_USEC 1000

#define FORMAT_TIME_MAX_VALUE_TO_DAY 70

#define MONS_PER_YEAR 12
#define MONSPERYEAR 12
#define DAYSPERNYEAR 365
#define DAYSPERLYEAR 366
#define WEEK_MONDAY_FIRST 1
#define WEEK_YEAR 2
#define WEEK_FIRST_WEEKDAY 4
#define MONDAY_IS_FIRST_WEEKDAY 1
#define MONDAY_IS_FIRST_WEEKDAY 1
#define SCOPE_OF_WEEK 2
#define FIRST_FULL_WEEK 4
#define FOUR_DAYS_IN_YEAR 4
#define WEEKS_PER_YEAR 52
#define DAYS_PER_COMMON_YEAR 365
#define DAYS_PER_LEAP_YEAR 366
#define SECS_PER_MIN 60
#define MINS_PER_HOUR 60
#define HOURS_PER_DAY 24
#define DAYS_PER_WEEK 7
#define DAYS_PER_NYEAR 365
#define DAYS_PER_LYEAR 366
#define MONS_PER_YEAR 12
#define FRAC_PRECISION 6
#define JANUARY 1
#define EPOCH_YEAR 1970
#define LEAPS_THRU_END_OF(y)  ((y) / 4 - (y) / 100 + (y) / 400)
#define FIRST_JULIAN_DAY 1721060

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

static const int mon_starts[2][MONS_PER_YEAR] =
{
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

static const int year_lengths[2] =
{
	DAYSPERNYEAR, DAYSPERLYEAR
};

typedef struct DateTimeFormat
{
	const char   *format_name;
	const char   *date_format;
	const char   *datetime_format;
	const char   *time_format;
}DateTimeFormat;

typedef struct Date_time_format
{
	unsigned char positions[8];
	char time_separator;
	unsigned int flag;
	char *format_str;
	size_t format_length;
}Date_time_format;

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

static Date_time_format time_24hrs_format = {{0}, '\0', 0, (char *)"%H:%i:%S", 8};
static Date_time_format time_ampm_format= {{0}, '\0', 0, (char *)"%I:%i:%S %p", 11};

static long long strtoll10_n(const char *src, size_t max, char **endptr);

static Datum ivymy_dayof_date(FunctionCallInfo fcinfo, int datekind);
static int64 getPartFromTm(struct pg_tm *tm, fsec_t fsec, int part);
static Datum GetSpecificPartOfTime(FunctionCallInfo fcinfo, int part);
static unsigned long convert_period_to_month(unsigned long period);
static unsigned long convert_month_to_period(unsigned long month);

static long calc_day(int year, int month, int day);
static int calc_weekday(long daynr, bool sunday_first_day_of_week);
static unsigned int calc_days_in_year(int year);
static bool get_datetime_from_numeric(Numeric numvalue, struct pg_tm *tm, fsec_t *fsec, int flag, int *dec);
static bool get_date_from_int(int64 num, struct pg_tm *tm, fsec_t *fsec, int flag);
static bool get_time_from_num(Numeric num, struct pg_tm *tm, fsec_t *fsec, int flag, int *dec);
static int calc_week(struct pg_tm* tm, int64 mode, int *year);
static int week_mode(int mode);
static void make_warning(char *str, mysql_dttype ftype);
static void get_date_from_day(int64 val, struct pg_tm *tm);
static pg_time_t sec_since_epoch(int year, int mon, int mday, int hour, int min ,int sec);
static pg_time_t time_to_gmt_sec(struct pg_tm *tm, int offset);
static void mys_sec_to_time(struct pg_tm *tmp, pg_time_t t, int offset);
static int determine_precision(int micro);
static bool parse_timezone(struct pg_tm *tm, char *str, int *tz);

static mysql_dttype fix_from_format(const char *format, size_t length);

static bool extract_date_time(Date_time_format *format,
							  const char *val, size_t length, struct pg_tm *l_time,
							  mysql_dttype cached_timestamp_type,
							  const char **sub_pattern_end,
							  const char *date_time_type,
							  fsec_t *fsec);

static bool valid_datetime(char *val, char *format, 
						  struct pg_tm *ltime, int fuzzy_date,
						  mysql_dttype cached_timestamp_type, fsec_t *fsec);

static int MyWeekdaySearch(char **input);

static DateTimeFormat date_time_formats[] =
{
	{"usa", "%m.%d.%Y", "%Y-%m-%d %H.%i.%s", "%h:%i:%s %p" },
	{"jis", "%Y-%m-%d", "%Y-%m-%d %H:%i:%s", "%H:%i:%s" },
	{"iso", "%Y-%m-%d", "%Y-%m-%d %H:%i:%s", "%H:%i:%s" },
	{"eur", "%d.%m.%Y", "%Y-%m-%d %H.%i.%s", "%H.%i.%s" },
	{"internal", "%Y%m%d", "%Y%m%d%H%i%s", "%H%i%s" }
};

static int szdate_time_formats = sizeof date_time_formats / sizeof date_time_formats[0];

static long long 
strtoll10_n(const char *src, size_t max, char **endptr)
{
	char buf[8];
	char *tmp;
	long long val = 0;

	if (max >= sizeof(buf))
		max = sizeof(buf) - 1;

	memcpy(buf, src, max);
	buf[max] = '\0';

	errno = 0;
	val = strtoll(buf, &tmp, 10);

	if (endptr)
		*endptr = (char *)src + (tmp - buf); /* map back to original str */

	return val;
}

static mysql_dttype 
fix_from_format(const char *format, size_t length)
{
	const char *time_part_frms = "HISThiklrs";
	const char *date_part_frms = "MVUXYWabcjmvuxyw";
	bool date_part_used = 0, time_part_used = 0, frac_second_used = 0;
	const char *val = format;
	const char *end = format + length;
	mysql_dttype cached_timestamp_type = MYSQL_TIMESTAMP_NONE;

	for (; val != end; val++)
	{
		if (*val == '%' && val + 1 != end)
		{
			val++;

			if (*val == 'f')
				frac_second_used = time_part_used= 1;
			else if (!time_part_used && strchr(time_part_frms, *val))
				time_part_used = 1;
			else if (!date_part_used && strchr(date_part_frms, *val))
				date_part_used = 1;

			if (date_part_used && frac_second_used)
			{
				/*
				 * frac_second_used implies time_part_used, and thus we already
				 * have all types of date-time components and can end our search.
				 */
				cached_timestamp_type = MYSQL_TIMESTAMP_DATETIME;
				return cached_timestamp_type;
			}
		}
	}

	/* We don't have all three types of date-time components */
	if (frac_second_used) /* TIME with microseconds */
	{
		cached_timestamp_type = MYSQL_TIMESTAMP_TIME;
	}
	else if (time_part_used)
	{
		if (date_part_used) /* DATETIME, no microseconds */
		{
			cached_timestamp_type = MYSQL_TIMESTAMP_DATETIME;
		}
		else /* TIME, no microseconds */
		{
			cached_timestamp_type = MYSQL_TIMESTAMP_TIME;
		}
	}
	else /* DATE */
	{
		cached_timestamp_type = MYSQL_TIMESTAMP_DATE;
	}	
	return cached_timestamp_type;
}

static int 
MyWeekdaySearch(char **input)
{
	int i = 0;
	char *const weekday_names[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
	char *const ab_weekday_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sund"};

	for (i = 0; i < 7; i++)
	{
		if (!pg_strncasecmp(*input, weekday_names[i], strlen(weekday_names[i])))
		{
			*input = *input + strlen(weekday_names[i]);
			return i + 1;
		}
		else if (!pg_strncasecmp(*input, ab_weekday_names[i], strlen(ab_weekday_names[i])))
		{
			*input = *input + strlen(ab_weekday_names[i]);
			return i + 1;
		}
	}
	return 0;
}

/*
 * Extract datetime value to pg_tm struct from string value
 * according to format string.
 */
static bool extract_date_time(Date_time_format *format,
								const char *val, size_t length, struct pg_tm *l_time,		
								mysql_dttype cached_timestamp_type,
								const char **sub_pattern_end,
								 const char *date_time_type,
								 fsec_t *fsec)
{
	int weekday = 0, yearday = 0, daypart = 0;
	int week_number = -1;
	int error = 0;
	int  strict_week_number_year = -1;
	int frac_part;
	bool usa_time = 0; //12-hour system 
	bool sunday_first_n_first_week_non_iso = false;
	bool strict_week_number = false;
	bool strict_week_number_year_type = false;
	const char *val_begin = val;
	const char *val_end = val + length;
	const char *ptr = format->format_str;
	const char *end = ptr + format->format_length;

	if (!sub_pattern_end)
		memset(l_time, 0, sizeof(*l_time));

	/* Skip pre-space between each argument */
	for (; val != end && isspace((unsigned char)*val) ; val++)
	;

	for (; ptr != end && val != val_end; ptr++)
	{

		/* Skip pre-space between each argument */
		for (; val != end && isspace((unsigned char)*val) ; val++)
		;

		if (*ptr == '%' && ptr + 1 != end)
		{
			int val_len;
			char *tmp;
			error= 0;

			val_len= (unsigned int) (val_end - val);
			switch (*++ptr) 
			{
				/* Year */
				case 'Y':
					tmp = (char*) val + Min(4, val_len);
					l_time->tm_year = (int) strtoll10_n(val, Min(4, val_len), &tmp);
					if ((int) (tmp-val) <= 2)
						l_time->tm_year = l_time->tm_year + (l_time->tm_year >= YY_PART_YEAR ? 1900 : 2000);
					val = tmp;
					break;

				case 'y':
					tmp= (char*) val + Min(2, val_len);
					l_time->tm_year = (int) strtoll10_n(val, Min(2, val_len),&tmp);
					val = tmp;
					l_time->tm_year = l_time->tm_year + (l_time->tm_year >= YY_PART_YEAR ? 1900 : 2000);
					break;

					/* Month */
					case 'm':
					case 'c':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_mon = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						val = tmp;
						break;
					case 'M':
					case 'b':
						tmp = (char*) val;
						if ((l_time->tm_mon = MyMonthSearch(lc_time_names, &tmp)) <= 0)
							goto err;
						else
							val = tmp;
						break;

					/* Day */
					case 'd':
					case 'e':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_mday = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						//l_time->tm_mday = (int) strtoll(val, &tmp, 10);
						val = tmp;
						break;
					case 'D':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_mday = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						/* Skip 'st, 'nd, 'th .. */
						val= tmp + Min((int) (val_end-tmp), 2);
						break;

					/* Hour */
					case 'h':
					case 'I':
					case 'l':
						usa_time = 1;
					/* fall through */
					case 'k':
					case 'H':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_hour = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						val = tmp;
						break;

					/* Minute */
					case 'i':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_min = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						val = tmp;
						break;

					/* Second */
					case 's':
					case 'S':
						tmp = (char*) val + Min(2, val_len);
						l_time->tm_sec = (int) strtoll10_n(val, Min(2, val_len), &tmp);
						val = tmp;
						break;

					/* Second part */
					case 'f':
						tmp = (char*) val_end;
						if (tmp - val > 6)
							tmp = (char*) val + 6;
						(*fsec) = (int) strtoll(val, &tmp, 10);
						frac_part = 6 - (int) (tmp - val);
						if (frac_part > 0)
							*fsec = (*fsec) * (unsigned long) log_10_int[frac_part];
						val = tmp;
						break;

					/* AM / PM */
					case 'p':
						if (val_len < 2 || !usa_time)
							goto err;
						if (!pg_strncasecmp(val, "PM", 2))
							daypart = 12;
						else if (pg_strncasecmp(val,"AM", 2))
							goto err;
						val += 2;
						break;

				/* Exotic things */
					case 'W':
					case 'a':
						tmp = (char*) val;
						if ((l_time->tm_mon = MyWeekdaySearch(&tmp)) <= 0)
							goto err;
						else
							val = tmp;
						break;
			
					case 'w':
						tmp = (char*) val + 1;
						if ((weekday= (int) strtoll(val, &tmp, 10)) < 0 || weekday >= 7)
							goto err;
						/* We should use the same 1 - 7 scale for %w as for %W */
						if (!weekday)
							weekday= 7;
						val = tmp;
						break;
					case 'j':
						tmp = (char*) val + Min(val_len, 3);
						yearday = (int) strtoll10_n(val, Min(val_len, 3), &tmp);
						val= tmp;
						break;

					/* Week numbers */
					case 'V':
					case 'U':
					case 'v':
					case 'u':
						sunday_first_n_first_week_non_iso = (*ptr=='U' || *ptr== 'V');
						strict_week_number = (*ptr=='V' || *ptr=='v');
						tmp = (char*) val + Min(val_len, 2);
						if ((week_number = (int) strtoll10_n(val, Min(val_len, 2), &tmp)) < 0 ||
							(strict_week_number && !week_number) ||
							week_number > 53)
							goto err;
						val= tmp;
						break;

					/* Year used with 'strict' %V and %v week numbers */
					case 'X':
					case 'x':
						strict_week_number_year_type = (*ptr=='X');
						tmp = (char*) val + Min(4, val_len);
						strict_week_number_year = (int) strtoll10_n(val, Min(4, val_len), &tmp);
						val = tmp;
						break;

					/* Time in AM/PM notation */
					case 'r':
						/* We can't just set error here, as we don't want to generate two warnings in case of errors */
						if (!extract_date_time(&time_ampm_format, val, strlen(val), l_time, 
												cached_timestamp_type, &val, "time", fsec))
							return false;
						else 
							return true;
						break;

					/* Time in 24-hour notation */
					case 'T':
						if (!extract_date_time(&time_24hrs_format, val, strlen(val), l_time, 
												cached_timestamp_type, &val, "time", fsec))
							return false;
						else 
							return true;
					break;

					/* Conversion specifiers that match classes of characters */
					case '.':
						while (val < val_end && ispunct(*val))
							val++;
						break;
					case '@':
						while (val < val_end && isalpha(*val))
							val++;
						break;
					case '#':
						while (val < val_end && isdigit(*val))
							val++;
						break;
					default:
						goto err;
			}	
			if (error)				// Error from my_strtoll10
				goto err;
		}
		else if (!isspace(*ptr))
		{
			if (*val != *ptr)
				goto err;
			val++;
		}
	}
	if (usa_time)
	{
		if (l_time->tm_hour > 12 || l_time->tm_hour < 1)
			goto err;
		l_time->tm_hour = l_time->tm_hour%12+daypart;
	}

	/*
	 * If we are recursively called for parsing string matching compound
	 * specifiers we are already done.
	*/
	if (sub_pattern_end)
	{
		*sub_pattern_end = val;
		return true;
	}

	if (yearday > 0)
	{
		unsigned int days;
		days= date2j(l_time->tm_year,1,1) + yearday - 1 - FIRST_JULIAN_DAY;
		if (days <= 0 || days > 3652424L)
			goto err;
		get_date_from_day(days, l_time);
	}

	if (week_number >= 0 && weekday)
	{
		int days;
		unsigned int weekday_b;

		/*
		 * %V,%v require %X,%x resprectively,
		 * %U,%u should be used with %Y and not %X or %x
		 */
		if ((strict_week_number &&
			(strict_week_number_year < 0 ||
			strict_week_number_year_type != sunday_first_n_first_week_non_iso)) ||
			(!strict_week_number && strict_week_number_year >= 0))
			goto err;

		/* Number of days since year 0 till 1st Jan of this year */
		days = date2j((strict_week_number ? strict_week_number_year : l_time->tm_year), 1, 1) - FIRST_JULIAN_DAY;
		/* Which day of week is 1st Jan of this year */
		weekday_b = calc_weekday(days, sunday_first_n_first_week_non_iso);

		/*
		 * Below we are going to sum:
		 * 1) number of days since year 0 till 1st day of 1st week of this year
		 * 2) number of days between 1st week and our week
		 * 3) and position of our day in the week
		 */
		if (sunday_first_n_first_week_non_iso)
		{
			days += ((weekday_b == 0) ? 0 : 7) - weekday_b +
					(week_number - 1) * 7 +
					weekday % 7;
		}
		else
		{
			days+= ((weekday_b <= 3) ? 0 : 7) - weekday_b +
					(week_number - 1) * 7 +
					(weekday - 1);
		}

		if (days <= 0 || days > 3652424L)
			goto err;
		get_date_from_day(days, l_time);
	}

	if (l_time->tm_mon > 12 || l_time->tm_mday > 31 || l_time->tm_hour > 23 || 
		l_time->tm_min > 59 || l_time->tm_sec > 59)
		goto err;

	if (val != val_end)
	{
		do
		{
			if (!isspace(*val))
			{
				// TS-TODO: extract_date_time is not UCS2 safe
				ereport(WARNING,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("Truncated incorrect time value: '%s'", val_begin)));
				break;
			}
		} while (++val != val_end);
	}
	return true;

err:
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Incorrect datetime value: '%s' for function str_to_date", val_begin)));
	}
	return false;
}

static bool 
valid_datetime(char *val, char *format, struct pg_tm *ltime, int fuzzy_date, 
				mysql_dttype cached_timestamp_type, fsec_t *fsec)
{
	Date_time_format date_time_format;

	if (sql_mode & MODE_NO_ZERO_DATE)
		fuzzy_date |= TIME_NO_ZERO_DATE;

	if (val == NULL || format == NULL)
		goto null_date;

	memset(ltime, 0, sizeof(*ltime));
	date_time_format.format_str = format;
	date_time_format.format_length = strlen(format);

	if (!extract_date_time(&date_time_format, val, strlen(val),
		ltime, cached_timestamp_type, 0, "datetime", fsec) ||
		((fuzzy_date & TIME_NO_ZERO_DATE) && (ltime->tm_year == 0 || ltime->tm_mon == 0 || ltime->tm_mday == 0)))
		goto null_date;

	if (cached_timestamp_type == MYSQL_TIMESTAMP_TIME && ltime->tm_mday)
	{
		/*
		 * Day part for time type can be nonzero value and so 
		 * we should add hours from day part to hour part to
		 * keep valid time value.
		 */
		ltime->tm_hour += ltime->tm_mday * 24;
		ltime->tm_mday = 0;
	}
	return true;

null_date:
	if (val && (fuzzy_date & TIME_NO_ZERO_DATE) /*warnings*/)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Incorrect datetime value: '%s' for function str_to_date", val)));
	}
	return false;
}

Datum
str_to_date(PG_FUNCTION_ARGS)
{
	text	*arg0 = PG_GETARG_TEXT_PP(0);
	text	*arg1 = PG_GETARG_TEXT_PP(1);
	
	char	*str = text_to_cstring(arg0);
	char	*str_format = text_to_cstring(arg1);

	int32	typmod = 0;
	
	TimeADT		datetime = -1;
	fsec_t		fsec = 0;
	struct pg_tm tt,
				*tm = &tt;
	TimeOffset	time = 0;
	int64 		datetemp = 0;
	char		buf[MAXDATELEN + 1] = {0};

	Datum	result;

	mysql_dttype cached_timestamp_type = MYSQL_TIMESTAMP_NONE;
	cached_timestamp_type = fix_from_format(str_format, sizeof(str_format));
	
	if (!valid_datetime(str, str_format, tm, TIME_FUZZY_DATE, cached_timestamp_type, &fsec))
	{
		PG_RETURN_NULL();
	}

	if (fsec)
	{
		typmod = 6;
	}

	snprintf(buf, MAXDATELEN, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
				tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, fsec);

	result = DirectFunctionCall3(mysdatetimes_in,
										CStringGetDatum(buf),
										ObjectIdGetDatum(InvalidOid),
										Int32GetDatum(typmod));
	if (result)
	{
		TimeADT dtcheck = 0;
		int64	part1, part2;
		int		year, mon, day;
		DateADT		date;
		
		dtcheck = DatumGetInt64(DirectFunctionCall1(numeric_int8, result));

		date = dtcheck/1000000;
		part1 = (date / 10000);
		part2 = (date - part1 * 10000);

		year = part1;
		mon = part2 / 100;
		day = part2 % 100;

		/*
		 * if param not 0000-00-00 and result not 1000-01-01,
		 * can return by mysdatetimes.
		 */
		if (tm->tm_year || tm->tm_mon || tm->tm_mday
			|| year != 1000 || mon != 1 || day != 1)
		{
			PG_RETURN_DATUM(result);
		}
	}

	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday);
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	datetime = datetemp + time;

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

	PG_RETURN_DATUM(DirectFunctionCall3(numeric_in,
										CStringGetDatum(buf),
										ObjectIdGetDatum(InvalidOid),
										Int32GetDatum((((14 + typmod) << 16) | typmod) + VARHDRSZ)));
}

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
        time--;

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

    time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[typmod], &warn);

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

    snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) time, fsec);

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

    snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

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

    snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

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

    if (!numeric_to_lldiv_t(PG_GETARG_NUMERIC(2), &seconds))
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

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) time, fsec);

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
    if ((a = period / 100) < YY_PART_YEAR)
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
        year += (year < YY_PART_YEAR) ? 2000 : 1900;
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

Datum 
sec_to_time(PG_FUNCTION_ARGS)
{
    Numeric     num = PG_GETARG_NUMERIC(0);
    char        *raw = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
    fsec_t      fsec = 0;
    int         total_sec = 0;
    int         hour = 0, min = 0, sec = 0;
    lldiv_t     seconds;
    TimeADT     time;
    bool        neg = 0;
    int         dec = 0;
    char        buf[MAXDATELEN + 1];
    char        *dot = strchr(raw, '.');

    /* Begin - bug#5125 */
    if (dot)
    {
        dec = raw + strlen(raw) - dot - 1;
        if (dec > DATETIME_MAX_DECIMALS)
            dec = DATETIME_MAX_DECIMALS;
    }

    if (numeric_is_neg(num))
    {
        neg = true;
        num = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(num),
                                                  NumericGetDatum(int64_to_numeric(-1))));
    }
    /* End - bug#5125 */
    
    if ((!numeric_to_lldiv_t(num, &seconds)) ||
        (seconds.quot > TIME_MAX_VALUE_SECONDS) ||
        (seconds.quot == TIME_MAX_VALUE_SECONDS && seconds.rem != 0))
    {
        hour = TIME_MAX_HOUR;
        min = TIME_MAX_MINUTE;
        sec = TIME_MAX_MINUTE;
        fsec = 0;

        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", raw)));
    }
    else
    {
        total_sec = (int)seconds.quot;
        fsec = (int)(seconds.rem / MICRO_PER_NANOSEC);

        // Will do round off
        if (seconds.rem % MICRO_PER_NANOSEC >= 500)
        {
            fsec += 1;
            if (fsec >= 1000000)
            {
                fsec -= 1000000;
                total_sec++;
            }
        }

        hour = total_sec / SECS_PER_HOUR;
        min = (total_sec % SECS_PER_HOUR) / MINS_PER_HOUR;
        sec = total_sec % MINS_PER_HOUR;
    }

    time = hour * 10000 + min * 100 + sec;
    snprintf(buf, MAXDATELEN, "%s%ld.%06d", neg ? "-" : "", time, fsec);
    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

    pfree(raw);
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

	if (mystr_to_datetime(str, strlen(str), result_tm, flag, result_fsec, &tm_type, &warning, &nanoseconds, &fsec_len) == -1)
    {
        return InvalidOid; /* bug#z5110 */
    }

	mysdatetime_add_nanoseconds_with_round(result_tm, result_fsec, &warning, nanoseconds);

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
	int warn = 0;
	int nanoseconds = 0;
	int typmod = 6;
	int dash_count = 0;

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
    if (str_to_time(start, strlen(start), result_tm, &fsec, &ftype, &warn, &nanoseconds, result_neg) == -1)
    {
        return InvalidOid; /* bug#z5110 */
    }
	
    if (result_tm->tm_min > TIME_MAX_MINUTE || result_tm->tm_sec > TIME_MAX_SECOND)
        return InvalidOid;

    fsec += (nanoseconds + 500) / 1000;
	time_add_nanoseconds_with_round(result_tm, &fsec, msec_round_add[typmod], &warn);
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

/**
 * Unlike function MysTimestampDifference(), the third
 * parameter of TimestampDifference() is of type long *, 
 * which may lead to inconsistent results between the
 * Windows and Linux.
 */
static void
MysTimestampDifference(TimestampTz start_time, TimestampTz stop_time, int64 *secs, int *microsecs)
{
	TimestampTz diff = stop_time - start_time;

	if (diff <= 0)
	{
		*secs = 0;
		*microsecs = 0;
	}
	else
	{
		*secs = (diff / USECS_PER_SEC);
		*microsecs = (int) (diff % USECS_PER_SEC);
	}
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
            int64 secs = 0;
            int usecs = 0;
            int sign = 1;
            tm2timestamp(tm1, fsec1, NULL, &datetime1);
            tm2timestamp(tm2, fsec2, NULL, &datetime2);

            if (datetime1 < datetime2)
            {
                MysTimestampDifference(datetime1, datetime2, &secs, &usecs);
                sign = -1;
                neg = true;
            }
            else
            {
                MysTimestampDifference(datetime2, datetime1, &secs, &usecs);
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

    time_add_nanoseconds_with_round(res_tm, &res_fsec, msec_round_add[dec], &warn);
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

    time_add_nanoseconds_with_round(tm, &fsec, msec_round_add[dec], &warn);

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

    if (convert_cstring_to_pg_tm(val_str, tm, &fsec) == InvalidOid)
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str)));
        PG_RETURN_NULL(); /* bug#z5110 */
    }

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

    if (convert_cstring_to_pg_tm(val_str1, tm1, &fsec1) == InvalidOid)
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str1)));
        PG_RETURN_NULL(); /* bug#z5110 */
    }

    tm2timestamp(tm1, fsec1, NULL, &datetime1);

    if (convert_cstring_to_pg_tm(val_str2, tm2, &fsec2) == InvalidOid)
    {
        ereport(WARNING, (errmsg("Truncated incorrect time value: '%s'", val_str2)));
        PG_RETURN_NULL(); /* bug#z5110 */
    }

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

/* Begin - bug#z5130 */
static int64
mys_tm2time(struct pg_tm *tm)
{
    int64 time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

    return time;
}

static int64
mys_tm2date(struct pg_tm *tm)
{
    int64 date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

    return date;
}

static int64
mys_tm2datetime(struct pg_tm *tm)
{
    int64 date = mys_tm2date(tm);
    int64 time = mys_tm2time(tm);
    int64 datetime = date * 1000000 + time;

    return datetime;
}

static void
mys_time2tm(int64 time, struct pg_tm *tm)
{
    tm->tm_hour = time / 10000;
    tm->tm_min = (time % 10000) / 100;
    tm->tm_sec = (time % 10000) % 100;
}

static void
mys_date2tm(int64 date, struct pg_tm *tm)
{
    tm->tm_year = date / 10000;
    tm->tm_mon = (date % 10000) / 100;
    tm->tm_mday = (date % 10000) % 100;
}

static void
mys_datetime2tm(int64 datetime, struct pg_tm *tm)
{
    int64   date = datetime / 1000000;
    int64   time = datetime % 1000000;

    mys_date2tm(date, tm);
    mys_time2tm(time, tm);
}

static void
mys_datetimes2tm(Numeric datetimes, struct pg_tm *tm, fsec_t *fsec)
{
    int64       datetime;
    Numeric		tempfsec;

    datetime = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(datetimes)));
    tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mul, NumericGetDatum(datetimes),
                                                   NumericGetDatum(int64_to_numeric(1000000))));
    tempfsec = DatumGetNumeric(DirectFunctionCall2(numeric_mod, NumericGetDatum(tempfsec),
                                                   NumericGetDatum(int64_to_numeric(1000000))));

    *fsec = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(tempfsec)));

    if (*fsec > 500000)
        datetime--;

    mys_datetime2tm(datetime, tm);
}
/* End - bug#z5130 */

Datum 
timestampadd(PG_FUNCTION_ARGS)
{
    int             unit_val = PG_GETARG_INT32(0);
    int64           interval = (int64) PG_GETARG_INT32(1); // may be negative
    TimeOffset      timestamp = 0, units_timestamp = 0;
    struct pg_tm    tt, *tm = &tt;
    fsec_t	        fsec = 0;
    int64           datetime = 0;
    char            buf[MAXDATELEN];
    int             dec = 0;
    Numeric         num;
    
    MYS_ZERO_tm(tm);

    mys_datetimes2tm(PG_GETARG_NUMERIC(2), tm, &fsec);

    if (fsec != 0 || unit_val == DTK_MICROSEC)
    {
		dec = 6;
	}

	switch (unit_val)
	{
		case DTK_WEEK:
			interval *= DAYS_PER_WEEK;
			/* fallthrough */
		case DTK_DAY:
			interval *= HOURS_PER_DAY;
			/* fallthrough */
		case DTK_HOUR:
			interval *= MINS_PER_HOUR;
			/* fallthrough */
		case DTK_MINUTE:
			interval *= SECS_PER_MINUTE;
			/* fallthrough */
		case DTK_SECOND:
			interval *= USECS_PER_SEC;
			/* fallthrough */
		case DTK_MICROSEC:
			units_timestamp = interval;
			break;
		case DTK_YEAR:
			interval *= 4;
			/* fallthrough */
		case DTK_QUARTER:
			interval *= 3;
			/* fallthrough */
		case DTK_MONTH:
		{
			int days; /* bug#z5367 */
			
			tm->tm_mon += interval % MONTHS_PER_YEAR;
			tm->tm_year += interval / MONTHS_PER_YEAR;
				
			if (tm->tm_mon > MONTHS_PER_YEAR)
			{
				tm->tm_year++;
				tm->tm_mon -= MONTHS_PER_YEAR;
			}
			
			if (tm->tm_mon <= 0)
			{
				tm->tm_year--;
				tm->tm_mon += MONTHS_PER_YEAR;
			}

			/**
			 * Begin - bug#z5367
			 * If you add MONTH, QUARTER, or YEAR and the resulting date has
			 * a day that is larger than the maximum day for the new month,
			 * the day is adjusted to the maximum days in the new month.
			 * such as timestamp(MONTH, 1, '2019-01-30'), it will get '2019-2-28'.
			 */
			days = day_tab[isleap(tm->tm_year)][tm->tm_mon - 1];

			if (tm->tm_mday > days)
				tm->tm_mday = days;
			/* End - bug#z5367 */

			break;
		}
		default:
			break;
	}
    
    if (tm->tm_year < 0 || tm->tm_year > 9999)
    {
        ereport(WARNING, (errmsg("Datetime function: datetime field overflow")));
        PG_RETURN_NULL();
    }

    tm2timestamp(tm, 0, NULL, &timestamp);
    timestamp += units_timestamp;
    timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL);

    datetime = mys_tm2datetime(tm);
    
    snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) datetime, fsec);

    num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
                                              CStringGetDatum(buf),
                                              ObjectIdGetDatum(InvalidOid),
                                              Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

    PG_RETURN_NUMERIC(num);
}

Datum
timestampdiff(PG_FUNCTION_ARGS)
{
	int64		res = 0;
	Interval	*span;
	int			unit_val = PG_GETARG_INT32(0);
	TimestampTz		timestamp1 = 0, timestamp2 = 0;
	struct pg_tm	tt1, *tm1 = &tt1;
	struct pg_tm	tt2, *tm2 = &tt2;
	struct pg_tm	tt3, *tm3 = &tt3;
	fsec_t fsec1 = 0;
	fsec_t fsec2 = 0;
	fsec_t fsec3 = 0;

	MYS_ZERO_tm(tm1);
	MYS_ZERO_tm(tm2);

	mys_datetimes2tm(PG_GETARG_NUMERIC(1), tm1, &fsec1);
	mys_datetimes2tm(PG_GETARG_NUMERIC(2), tm2, &fsec2);

	if ((tm1->tm_year == 0 || tm1->tm_mon == 0 || tm1->tm_mday == 0)
		|| (tm2->tm_year == 0 || tm2->tm_mon == 0 || tm2->tm_mday == 0))
	{
		ereport(WARNING, (errmsg("Incorrect datetime value")));
		PG_RETURN_NULL();
	}

	if ((tm1->tm_year == 0 || tm1->tm_mon == 0 || tm1->tm_mday == 0) || 
		(tm2->tm_year == 0 || tm2->tm_mon == 0 || tm2->tm_mday == 0))
	{
		ereport(WARNING, (errmsg("Incorrect datetime value")));
		PG_RETURN_NULL();
	}

	if ((tm2timestamp(tm1, fsec1, NULL, &timestamp1) != 0) || (tm2timestamp(tm2, fsec2, NULL, &timestamp2) != 0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	}

	if (unit_val == DTK_MONTH || unit_val == DTK_QUARTER || unit_val == DTK_YEAR)
	{
		span = DatumGetIntervalP(DirectFunctionCall2(timestamptz_age,
													 TimestampTzGetDatum(timestamp2), TimestampTzGetDatum(timestamp1)));
	}
	else
	{
		span = DatumGetIntervalP(DirectFunctionCall2(timestamp_mi,
													 TimestampTzGetDatum(timestamp2), TimestampTzGetDatum(timestamp1)));
	}

	switch (unit_val)
	{
		case DTK_YEAR:
			res = span->month / 12;
			break;
		case DTK_QUARTER:
			res = span->month / 3;
			break;
		case DTK_MONTH:
			res = span->month;
			break;
		case DTK_WEEK:
			res = span->day / 7;
			break;
		case DTK_DAY:
			res = span->day;
			break;
		case DTK_HOUR:
			if (interval2tm(*span, tm3, &fsec3) != 0)
				elog(ERROR, "could not convert interval to tm");
			res = span->day * 24 + tm3->tm_hour;
			break;
		case DTK_MINUTE:
			if (interval2tm(*span, tm3, &fsec3) != 0)
				elog(ERROR, "could not convert interval to tm");
			res = span->day * 24 * 60 + tm3->tm_hour * 60 + tm3->tm_min;
			break;
		case DTK_SECOND:
			if (interval2tm(*span, tm3, &fsec3) != 0)
				elog(ERROR, "could not convert interval to tm");
			res = span->day * 24 * 60 * 60 +
				tm3->tm_hour * 60 * 60 + tm3->tm_min * 60 + tm3->tm_sec;
			break;
		case DTK_MICROSEC:
				if (interval2tm(*span, tm3, &fsec3) != 0)
					elog(ERROR, "could not convert interval to tm");
				res = span->day * 24 * 60 * 60 * 1000000 +
					  tm3->tm_hour * 60 * 60 * 1000000 +
					  tm3->tm_min * 60 * 1000000 +
					  tm3->tm_sec * 1000000 + fsec3;
				break;
		default:
			/* impossible */
			break;
	}

	PG_RETURN_INT64(res);
}

/*
 * calc_day - calculate the number of days.
 */
static long
calc_day(int year, int month, int day)
{
	long	delsum;
	int		temp;
	int		y = year; /* may be < 0 temporarily */

	if (y == 0 && month == 0)
		return 0; /* Skip errors */

	/* Cast to int to be able to handle month == 0 */
	delsum = (long) (365 * y + 31 *( month - 1) + day);
	if (month <= 2)
		y--;
	else
		delsum -= (long) ( month * 4 + 23) / 10;
	temp = (int) ((y / 100 + 1) * 3) / 4;
	Assert(delsum + y / 4 - temp >= 0);
	return (delsum + y / 4 - temp);
}

/*
 * make_warning - set warning message.
 */
static void
make_warning(char *str, mysql_dttype ftype)
{
	char   *type_str;
	switch (ftype)
	{
		case MYSQL_TIMESTAMP_DATE:
			type_str= "date";
			break;
		case MYSQL_TIMESTAMP_TIME:
			type_str= "time";
			break;
		case MYSQL_TIMESTAMP_DATETIME:
		default:
			type_str= "datetime";
			break;
	}
	if (str)
	{
		if(ftype > MYSQL_TIMESTAMP_ERROR)
			ereport(WARNING, (errmsg("Truncated incorrect %s value: '%s'", type_str, str)));
		else
			ereport(WARNING, (errmsg("Incorrect %s value: '%s'", type_str, str)));
	}
	return;
}

/*
 * get_date_from_int - parsing a date by integer value.
 */
static bool
get_date_from_int(int64 num, struct pg_tm *tm, fsec_t *fsec, int flag)
{
	int			dterr;
	mysql_dttype ftype;
	int			warn = 0;

	dterr = number_to_datetime(num, tm, flag, fsec, &ftype, &warn);
	if (warn || dterr < 0) /* Returns an error result, the value is not 0. */
	{
		char   *strtemp = DatumGetCString(DirectFunctionCall1(int8out, Int64GetDatum(num)));
		make_warning(strtemp, MYSQL_TIMESTAMP_NONE);
		if (dterr)
			return false;
	}

	return true;
}


/*
 * calc_weekday - Returns 0 for monday, 1 for tuesday ....
 */
static int
calc_weekday(long daynr, bool sunday_first_day_of_week)
{
	return ((int) ((daynr + 5 + (sunday_first_day_of_week ? 1 : 0)) % 7));
}

/*
 * week_mode - deal with mode.
 */
static int
week_mode(int mode)
{
	int		week_format = (mode & 7);
	if (!(week_format & WEEK_MONDAY_FIRST))
		week_format ^= WEEK_FIRST_WEEKDAY;
	return week_format;
}

/*
 * calc_week -
 * tm specified date info structure
 * mode specified week mode in [0~7]
 * year this parameter may be used in yearweek()
 */
static int
calc_week(struct pg_tm* tm, int64 mode, int *year)
{
	struct pg_tm tmp_tt, *tmp_tm = &tmp_tt;
	long days;
	long weekday;
	long tm_year = tm->tm_year;
	long tm_mon = tm->tm_mon;
	long tm_mday = tm->tm_mday;

	long sum_day = calc_day(tm_year, tm_mon, tm_mday);
	long sum_start_day = calc_day(tm_year, 1, 1);

	bool monday_is_first_day = mode & MONDAY_IS_FIRST_WEEKDAY;
	bool week_scope = mode & SCOPE_OF_WEEK;
	bool first_week = mode & FIRST_FULL_WEEK;

	tmp_tm->tm_year = tm->tm_year;
	tmp_tm->tm_mon = 1;
	tmp_tm->tm_mday = 1;
	weekday = calc_weekday(sum_start_day, !monday_is_first_day);
	*year = tm_year;

	if (tm_mon == JANUARY && tm_mday <= DAYS_PER_WEEK - weekday)
	{
		if (!week_scope && ((first_week && weekday != 0) || (!first_week && weekday >= FOUR_DAYS_IN_YEAR)))
			return 0;

		week_scope = true;
		(*year)--;
		days = isleap(*year) ? DAYS_PER_LEAP_YEAR : DAYS_PER_COMMON_YEAR;
		sum_start_day -= days;
		weekday = (weekday + (WEEKS_PER_YEAR + 1) * DAYS_PER_WEEK - days) % DAYS_PER_WEEK;
	}

	if ((first_week && weekday != 0) || (!first_week && weekday >= FOUR_DAYS_IN_YEAR))
		days = sum_day - (sum_start_day + (DAYS_PER_WEEK - weekday));
	else
		days = sum_day - (sum_start_day - weekday);

	if (week_scope && days >= WEEKS_PER_YEAR * DAYS_PER_WEEK)
	{
		weekday = (weekday + (isleap(*year) ? DAYS_PER_LEAP_YEAR : DAYS_PER_COMMON_YEAR)) % DAYS_PER_WEEK;
		if ((first_week && weekday == 0) || (!first_week && weekday < FOUR_DAYS_IN_YEAR))
		{
			(*year)++;
			return 1;
		}
	}

	return days / DAYS_PER_WEEK + 1;
}

/*
 * get_date_from_day - converting days to date.
 */
static void
get_date_from_day(int64 val, struct pg_tm *tm)
{
	int		year, temp, leap_day, day_of_year, days_in_year;
	int		month_value = 0;

	if (val <= 365 || val >= 3652500)
	{
		/* Fix if wrong day value */
		tm->tm_year = tm->tm_mon = tm->tm_mday =0;
	}
	else
	{
		year = (val * 100 / 36525);
		temp = (((year - 1) / 100 + 1) * 3) / 4;
		day_of_year = (val - year * 365) - (year - 1) / 4 + temp;
		while (day_of_year > (days_in_year = calc_days_in_year(year)))
		{
			day_of_year -= days_in_year;
			year++;
		}

		leap_day = 0;
		if (days_in_year == 366)
		{
			if (day_of_year > 31 + 28)
			{
				day_of_year--;
				if (day_of_year == 31 + 28)
					leap_day = 1;	/* Handle leapyears leapday */
			}
		}
		tm->tm_mon = 1;
		for (int i = 0; i < 12; i++)
		{
			month_value = day_tab[0][i];
			if (day_of_year > month_value)
			{
				day_of_year -= month_value;
				(tm->tm_mon)++;
			}
			else
				break;
		}
		tm->tm_year = year;
		tm->tm_mday = day_of_year + leap_day;
	}
}

/*
 * calc_days_in_year - Calc days in one year. works with 0 <= year <= 99.
 */
static unsigned int
calc_days_in_year(int year)
{
	return ((year & 3) == 0 && (year % 100 || (year % 400 == 0 && year)) ? DAYS_PER_LEAP_YEAR : DAYS_PER_COMMON_YEAR);
}

/*
 * get_time_from_num - parsing time values through numeric values.
 */
static bool
get_time_from_num(Numeric num, struct pg_tm *tm, fsec_t *fsec, int flag, int *dec)
{
	bool		dterr;
	mysql_dttype ftype;
	int			warn = 0;
	int64		timetemp = 0;
	int			neg = 0;
	int64		nanofsec =0;
	char	   *str;
	int64		len = 0;

	char   *strtemp = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));
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
	for(int i = 0; i < 9; i++)
	{
		int numtemp = 0;
		if(*strtemp != '\0')
		{
			numtemp = *strtemp - '0';
			strtemp++;
			(*dec)++;
		}
		nanofsec = nanofsec * 10 + numtemp;
	}
	timetemp = (neg != 1 ? timetemp : -timetemp);

	/* Begin - BUG#Z5566 */
	if (timetemp == MYSQL_NULL_TIME)
		return false;
	/* End - BUG#Z5566 */

	/* First obtain nanosecond accuracy, then obtain dec accuracy */
	dterr = number_to_time(timetemp, tm, fsec, &ftype, &neg, &warn);
	*fsec = (nanofsec + 500) / 1000;
	(void)time_add_nanoseconds_with_round(tm, fsec, msec_round_add[(*dec)], &warn);
	*fsec = *fsec - *fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - (*dec)];
	if (dterr || warn)
	{
		make_warning(str, ftype);
		if (dterr)
			return false;
	}

	if (neg)
	{
		tm->tm_hour = -tm->tm_hour;
		tm->tm_min = -tm->tm_min;
		tm->tm_sec = -tm->tm_sec;
	}

	return true;
}

/*
 * get_datetime_from_numeric - parsing date and time values through numeric value.
 */
static bool
get_datetime_from_numeric(Numeric numvalue, struct pg_tm *tm, fsec_t *fsec, int flag, int *dec)
{
	int64		datetimetemp = 0;
	int64		len = 0;
	char	   *str, *tmpstr;
	fsec_t		nanoseconds = 0;
	int			dterr;
	mysql_dttype ftype;
	int			warn = 0;

	/* Calculate the length of the input data and return NULL if the integer part exceeds 14 digits. */
	str = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(numvalue)));
	tmpstr = pstrdup(str);
	/* Get the value before the decimal point. */
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
		for(int i = 0; i < 9; i++)
		{
			int numtemp = 0;
			if(*str != '\0')
			{
				numtemp = *str - '0';
				str++;
				(*dec)++;
			}
			nanoseconds = nanoseconds * 10 + numtemp;
		}
	}

	if(datetimetemp == 0)
	{
		MemSet(tm, 0, sizeof( struct pg_tm));
	}
	else
	{
		dterr = number_to_datetime(datetimetemp, tm, flag, fsec, &ftype, &warn);
		if (nanoseconds < 0 || dterr == -1)
		{
			MemSet(tm, 0, sizeof( struct pg_tm));
			ftype = MYSQL_TIMESTAMP_ERROR;
			if (!warn)
				warn|= MYSQL_TIME_WARN_TRUNCATED;
			if (warn)
				make_warning(tmpstr, ftype);
			return false;
		}
		else if (ftype == MYSQL_TIMESTAMP_DATE)
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
			*fsec = (nanoseconds + 500)/1000;
			(void)mysdatetime_add_nanoseconds_with_round(tm, fsec, &warn, msec_round_add[(*dec)]);
			*fsec = *fsec - *fsec % (long) log_10_int[DATETIME_MAX_DECIMALS - (*dec)];
		}

		if (warn)
			make_warning(tmpstr, ftype);
	}

	return true;
}

/*
 * sec_since_epoch - calculation date and time to seconds.
 */
static pg_time_t
sec_since_epoch(int year, int mon, int mday, int hour, int min ,int sec)
{
	int64	days;
	/* Guard against my_time_t overflow(on system with 32 bit my_time_t) */
	Assert(!(year == TIMESTAMP_MAX_YEAR && mon == 1 && mday > 17));
	/*
	 * It turns out that only whenever month is normalized or unnormalized
	 * plays role.
	 */
	Assert(mon > 0 && mon < 13);
	days= year * DAYS_PER_NYEAR - EPOCH_YEAR * DAYS_PER_NYEAR +
			   LEAPS_THRU_END_OF(year - 1) -
			   LEAPS_THRU_END_OF(EPOCH_YEAR - 1);
	days += mon_starts[isleap(year)][mon - 1];
	days += mday - 1;

	return ((days * HOURS_PER_DAY + hour) * MINS_PER_HOUR + min) * SECS_PER_MIN + sec;
}

/*
 * time_to_gmt_sec - calculation of date from from_tz
 */
static pg_time_t
time_to_gmt_sec(struct pg_tm *tm, int offset)
{
	pg_time_t	local_t;
	int			shift = 0;

	/*
	 * Check timestamp range.we have to do this as calling function relies on
	 * us to make all validation checks here.
	 */
	if (!validate_timestamp_range(tm))
		return 0;

	/*
	 * Do a temporary shift of the boundary dates to avoid
	 * overflow of my_time_t if the time value is near it's
	 * maximum range
	 */
	if ((tm->tm_year == TIMESTAMP_MAX_YEAR) && (tm->tm_mon == 1) && tm->tm_mday > 4)
		shift = 2;

	local_t= sec_since_epoch(tm->tm_year, tm->tm_mon, (tm->tm_mday - shift),
							 tm->tm_hour, tm->tm_min, tm->tm_sec) - offset;

	if (shift)
	{
		/* Add back the shifted time */
		local_t += shift * SECS_PER_DAY;
	}

	if (local_t >= TIMESTAMP_MIN_VALUE && local_t <= TIMESTAMP_MAX_VALUE)
		return local_t;

	/* range error*/
	return 0;
}

/*
 * calculation date to_tz
 */
static void
mys_sec_to_time(struct pg_tm *tmp, pg_time_t t, int offset)
{
	int64	days;
	int64	rem;
	int		y;
	int		yleap;

	days = t / SECS_PER_DAY;
	rem = t % SECS_PER_DAY;

	/*
	 * We do this as separate step after dividing t, because this
	 * allows us handle times near my_time_t bounds without overflows.
	 */
	rem += offset;
	while (rem < 0)
	{
		rem += SECS_PER_DAY;
		days--;
	}
	while (rem >= SECS_PER_DAY)
	{
		rem -= SECS_PER_DAY;
		days++;
	}
	tmp->tm_hour = rem / SECS_PER_HOUR;
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = rem / SECS_PER_MIN;
	/*
	 * A positive leap second requires a special
	 * representation.  This uses "... ??:59:60" et seq.
	 */
	tmp->tm_sec= rem % SECS_PER_MIN;

	y = EPOCH_YEAR;
	while (days < 0 || days >= year_lengths[yleap = isleap(y)])
	{
		int		newy;

		newy = y + days / DAYS_PER_NYEAR;
		if (days < 0)
			newy--;
		days -= (newy - y) * DAYS_PER_NYEAR +
				 LEAPS_THRU_END_OF(newy - 1) -
				 LEAPS_THRU_END_OF(y - 1);
		y = newy;
	}
	tmp->tm_year = y;

	tmp->tm_mon= 0;
	for (int i = 0; i < 12 && days >= day_tab[yleap][i]; i++)
	{
		days = days - day_tab[yleap][i];
		tmp->tm_mon++;
	}
	tmp->tm_mon++;
	tmp->tm_mday= days + 1;
}

/*
 * Determine how many digits to be saved behind the decimal point.
 */
static int
determine_precision(int micro)
{
	int trailing_zero;
	if (!micro)
	{
		return 0;
	}

	trailing_zero = 0;
	while (micro % 10 == 0)
	{
		micro /= 10;
		trailing_zero++;
	}
	return FRAC_PRECISION - trailing_zero;
}

/*
 * parse timezone and check validity, range '-12:59' to '+13:00'
 */
static bool
parse_timezone(struct pg_tm *tm, char *str, int *tz)
{
	char   *tmpstr = str + 1; /* BUG#Z5582 */
	/* Begin - BUG#Z5583 */
	if (!(str[0] == '-' || str[0] == '+'))
		return false;
	/* End - BUG#Z5583 */
	/* Begin - BUG#Z5582 */
	/* Parse timezone to see if there are any non-numeric vlaues other than : */
	while(*tmpstr)
	{
		if (!isdigit((unsigned char) *tmpstr) && *tmpstr != ':')
			return false;
		tmpstr++;
	}
	/* End - BUG#Z5582 */
	*tz = mys_parse_sane_timezone(tm, cstring_to_text(str));
	if (str[0] == '-' || str[0] == '+')
	{
		if (str[0] == '-')
		{
			if (*tz > 12 * MINS_PER_HOUR * SECS_PER_MIN + 59 * SECS_PER_MIN)
				return false;
		}
		else
		{
			if (-(*tz) > 13 * MINS_PER_HOUR * SECS_PER_MIN)
				return false;
		}
		if (!strstr(str, ":"))
			return false;
	}
	return true;
}


Datum
to_days(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1;
	fsec_t	fsec = 0;
	int64	result = 0;
	int		flag = TIME_NO_ZERO_DATE;

	DateADT	date = PG_GETARG_DATEADT(0);
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();

	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm);
	/* End - BUG#Z5578 */
	result = calc_day(tm->tm_year, tm->tm_mon, tm->tm_mday);
	PG_RETURN_INT64(result);
}

Datum
to_seconds(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	fsec_t	fsec = 0;
	int64	result = 0;
	int		flag = TIME_NO_ZERO_DATE;
	int		dec = 0;
	int64	part1 = 0, part2 = 0;

	Numeric	date = PG_GETARG_NUMERIC(0);

	if (!get_datetime_from_numeric(date, tm, &fsec, flag, &dec))
		PG_RETURN_NULL();

	part1 = calc_day(tm->tm_year, tm->tm_mon, tm->tm_mday);
	part2 = ((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * SECS_PER_MINUTE) + tm->tm_sec;
	result = part2 + part1 * 24L * MINS_PER_HOUR * SECS_PER_MINUTE;
	PG_RETURN_INT64(result);
}

Datum
unix_timestamp(PG_FUNCTION_ARGS)
{
	struct pg_tm ttx, *tmx = &ttx;
	Timestamp input;
	Timestamp epoch;
	Timestamp result;

	GetCurrentDateTime(tmx);

	if(PG_NARGS())
	{
		TimeADT	datetime = PG_GETARG_TIMEADT(0);
		struct pg_tm tt, *tm = &tt;
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

		if (tm->tm_year == 0 || tm->tm_mon == 0 || tm->tm_mday == 0)
		{
			PG_RETURN_NUMERIC(int64_to_numeric(0));
		}

		tm2timestamp(tm, 0, NULL, &input);
	}
	else
	{
		tm2timestamp(tmx, 0, NULL, &input);
	}

	epoch = SetEpochTimestamp();
	result = input - epoch
			- tmx->tm_gmtoff * USECS_PER_SEC /* Subtract timezone offset */
			+ tmx->tm_isdst * SECS_PER_HOUR * USECS_PER_SEC; /* Add back DST hour if active */
	PG_RETURN_NUMERIC(DirectFunctionCall2(numeric_trunc,
										DirectFunctionCall2(numeric_div,
															DirectFunctionCall1(int8_numeric, result),
															DirectFunctionCall1(int8_numeric, USECS_PER_SEC)),
										determine_precision(result % USECS_PER_SEC)));
}


Datum
utc_date_func(PG_FUNCTION_ARGS)
{
	DateADT result = 0;
	struct pg_tm tt, *tm = &tt;

	TimestampTz now = GetCurrentTimestamp();
	Timestamp curtm = (Timestamp)now;
	DateADT date = DatumGetDateADT(DirectFunctionCall1(timestamptz_date, curtm));
	j2date(date + POSTGRES_EPOCH_JDATE,
			   &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
		result = MYSQL_ZERO_DATE;
	else
		result = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

	PG_RETURN_DATEADT(result);
}


Datum
utc_time_func(PG_FUNCTION_ARGS)
{
	Numeric		num;
	TimestampTz	now;
	TimeADT		result;

	int32		typmod = PG_GETARG_INT32(0);
	if (typmod < 0 || typmod > MAX_TIME_PRECISION)
	{
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("Too-big precision %d specified for 'utc_time'. Maximum is %d.", typmod, MAX_TIMESTAMP_PRECISION)));
	}

	now = GetCurrentTimestamp();
	result = DatumGetTimeADT(DirectFunctionCall1(timestamp_time, (Timestamp)now));
	AdjustTimeForTypmod(&result, typmod);

	num = DatumGetNumeric(DirectFunctionCall2(pgtime_mystimes, result, PG_GETARG_DATUM(0)));
	PG_RETURN_NUMERIC(num);
}


Datum
utc_timestamp_func(PG_FUNCTION_ARGS)
{
	Numeric		num;
	TimestampTz	now;
	TimeADT		result;
	int32 typmod = PG_GETARG_INT32(0);

	if (typmod < 0 || typmod > MAX_TIME_PRECISION)
	{
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("Too-big precision %d specified for 'utc_timestamp'. Maximum is %d.", typmod, MAX_TIMESTAMP_PRECISION)));
	}
	now = GetCurrentTimestamp();
	result = (Timestamp)now;
	AdjustTimestampForTypmod(&result, typmod);
	num = DatumGetNumeric(DirectFunctionCall2(pgtimestamp_mystimestamps, result, PG_GETARG_DATUM(0)));
	PG_RETURN_NUMERIC(num);
}

Datum
dayname(PG_FUNCTION_ARGS)
{
	VarChar    *result; /* BUGZ5533 */
	long		weekday;
	char	   *weekname = NULL;
	MyLocale   *locale = NULL;
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1; /* BUG#Z5578 */
	fsec_t		fsec = 0;
	int			flag = TIME_NO_ZERO_DATE;

	DateADT		date = PG_GETARG_DATEADT(0);
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();

	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm);
	/* End - BUG#Z5578 */
	weekday = calc_weekday(calc_day(tm->tm_year, tm->tm_mon, tm->tm_mday), false);

	if (locale == NULL)
		locale = MyLocaleSearch(lc_time_names);
	weekname = locale->day_names[weekday];

	/* Begin - BUGZ5533 */
	result = DatumGetVarCharP(DirectFunctionCall3(varcharin, CStringGetDatum(pstrdup(weekname)),
															  ObjectIdGetDatum(InvalidOid),
															  Int32GetDatum(-1)));
	PG_RETURN_VARCHAR_P(result);
	/* End - BUGZ5533 */
}

Datum
monthname(PG_FUNCTION_ARGS)
{
	VarChar    *result; /* BUGZ5533 */
	char	   *monthname = NULL;
	MyLocale   *locale = NULL;
	struct	pg_tm tt, *tm = &tt;
	fsec_t		fsec = 0;
	int			flag = TIME_FUZZY_DATE;

	DateADT		date = PG_GETARG_DATEADT(0);
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();
	/* End - BUG#Z5578 */
	if (!get_date_from_int(date, tm, &fsec, flag))
		PG_RETURN_NULL();

	if (!tm->tm_mon)
		PG_RETURN_NULL();

	if (locale == NULL)
		locale = MyLocaleSearch(lc_time_names);
	monthname = locale->month_names[tm->tm_mon - 1];

	/* Begin - BUGZ5533 */
	result = DatumGetVarCharP(DirectFunctionCall3(varcharin, CStringGetDatum(pstrdup(monthname)),
															  ObjectIdGetDatum(InvalidOid),
															  Int32GetDatum(-1)));
	PG_RETURN_VARCHAR_P(result);
	/* End - BUGZ5533 */
}

Datum
time_to_sec(PG_FUNCTION_ARGS)
{
	int64		result = 0;
	struct	pg_tm tt, *tm = &tt;
	fsec_t		fsec = 0;
	int			flag = 0;
	int			dec = 0;

	Numeric		num = PG_GETARG_NUMERIC(0);
	if (!get_time_from_num(num, tm, &fsec, flag, &dec))
		PG_RETURN_NULL();

	result = ((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * SECS_PER_MINUTE) + tm->tm_sec;
	PG_RETURN_INT64(result);
}

Datum
mys_month(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	fsec_t		fsec = 0;
	int			flag = TIME_FUZZY_DATE;

	DateADT		date = PG_GETARG_DATEADT(0);

	/* Begin - BUG#Z5571 */
	if (date == 0)
		PG_RETURN_NULL();
	else if (date == MYSQL_ZERO_DATE)
		PG_RETURN_INT32(0);
	/* End - BUG#Z5571 */
	if (!get_date_from_int(date, tm, &fsec, flag))
		PG_RETURN_NULL();

	PG_RETURN_INT32(tm->tm_mon);
}

Datum
mys_date(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1; /* BUG#Z5578 */
	fsec_t	fsec = 0;
	DateADT	result = 0;
	int		flag = TIME_FUZZY_DATE | TIME_NO_DATE_FRAC_WARN;

	DateADT	date = PG_GETARG_DATEADT(0);

	/* Begin - BUG#Z5571 */
	if (date == 0)
		PG_RETURN_NULL();
	else if (date == MYSQL_ZERO_DATE)
		PG_RETURN_INT32(MYSQL_ZERO_DATE);
	/* End - BUG#Z5571 */
	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm); /* BUG#Z5578 */

	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
		result = MYSQL_ZERO_DATE;
	else
		result = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

	PG_RETURN_DATEADT(result);
}

Datum
mys_week(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1; /* BUG#Z5578 */
	fsec_t	fsec = 0;
	int		result = 0;
	int		mode = default_week_format;
	int		flag = TIME_NO_ZERO_DATE;

	DateADT		date = PG_GETARG_DATEADT(0);
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();

	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm);
	/* End - BUG#Z5578 */
	result = calc_week(tm, week_mode(mode), &(tm->tm_year));
	PG_RETURN_INT32(result);
}

Datum
mys_week_mode(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1; /* BUG#Z5578 */
	fsec_t	fsec = 0;
	int		result = 0;
	int		flag = TIME_NO_ZERO_DATE;

	/* Begin - BUG#Z5528 */
	DateADT		date;
	int			mode;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
		mode = default_week_format;
	else
		mode = PG_GETARG_INT32(1);
	date = PG_GETARG_DATEADT(0);
	/* End - BUG#Z5528 */
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();
	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm);
	/* End - BUG#Z5578 */
	result = calc_week(tm, week_mode(mode), &(tm->tm_year));
	PG_RETURN_INT32(result);
}

Datum
yearweek(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	struct	pg_tm tt1, *tm1 = &tt1;
	fsec_t	fsec = 0;
	int		result = 0;
	int		flag = TIME_NO_ZERO_DATE;

	/* Begin - BUG#Z5530 */
	DateADT		date;
	int			mode;
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
		mode = 0;
	else
		mode = PG_GETARG_INT32(1);
	date = PG_GETARG_DATEADT(0);
	/* End - BUG#Z5530 */
	/* Begin - BUG#Z5578 */
	/* When the parameter is the value 0 or the character '0000-00-00',and 
	 * sql_mode is default or empty, the conversion calls the int_mysdate function
	 * or the mysdate_in function to obtain MYSQL_ZERO_DATE. A warning message
	 * appears because the original input parameter cannot be obtained.
	 * Therefore,ensur the result is correct and omit the warning message.
	 * eg: value 0 or 00 or 0000 or 00000000 -> 0
	 */
	if (date == 0 || date == MYSQL_ZERO_DATE) /* BUG#Z5604 */
		PG_RETURN_NULL();

	if (!get_date_from_int(date, tm1, &fsec, flag))
		PG_RETURN_NULL();
	mys_date2tm(date, tm);
	/* End - BUG#Z5578 */
	/* Begin - BUG#Z5575 */
	if (mode > 7)
		mode = mode %8;
	else if (mode < 0)
		mode = -(mode % 8);
	/* End - BUG#Z5575 */

	result = calc_week(tm, week_mode(mode) | WEEK_YEAR, &(tm->tm_year));
	result += tm->tm_year * 100;
	PG_RETURN_INT32(result); /* BUG#Z5576 */
}

Datum
datediff(PG_FUNCTION_ARGS)
{
	int64	day1 = -1;
	int64	day2 = -1;

	day1 = DatumGetInt64(DirectFunctionCall1(to_days, PG_GETARG_DATUM(0)));
	day2 = DatumGetInt64(DirectFunctionCall1(to_days, PG_GETARG_DATUM(1)));

	if (!day1 || !day2)
		PG_RETURN_NULL();
	PG_RETURN_INT64(day1 - day2);
}

Datum
from_days(PG_FUNCTION_ARGS)
{
	struct	pg_tm tt, *tm = &tt;
	DateADT	result = 0;
	int64	num = PG_GETARG_INT64(0);

	get_date_from_day(num, tm);
	if (tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0)
		result = MYSQL_ZERO_DATE;
	else
		result = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

	PG_RETURN_DATEADT(result);
}

Datum
convert_tz(PG_FUNCTION_ARGS)
{
	char   *str1, *str2;
	struct	pg_tm tt, *tm = &tt;
	fsec_t	fsec = 0;
	int64	result = 0;
	Numeric nic;
	int		flag = TIME_NO_ZERO_DATE;
	int		tz1 = 0;
	int		tz2 = 0;
	pg_time_t	tm_tmp = 0;
	TimeOffset	time = 0;
	int64		datetemp = 0;
	int			dec = 0;
	text	   *arg1;
	text	   *arg2;
	Numeric		datetime; /* BUG#Z5532 */
	char		buf[MAXDATELEN + 1] = {0};

	/* Begin - BUG#Z5532 */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	/* End - BUG#Z5532 */

	datetime = PG_GETARG_NUMERIC(0);
	if (!get_datetime_from_numeric(datetime, tm, &fsec, flag, &dec))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_TEXT_PP(1);
	arg2 = PG_GETARG_TEXT_PP(2);
	str1 = text_to_cstring(arg1);
	str2 = text_to_cstring(arg2);
	/* Begin - BUG#Z5532 */
	if (!strcasecmp(str1, "Invalid") ||
		!strcasecmp(str2, "Invalid"))
		PG_RETURN_NULL();
	/* End - BUG#Z5532 */
	/* Begin - BUG#Z5581 */
	if (!strlen(str1) || !strlen(str2))
		PG_RETURN_NULL();
	/* End - BUG#Z5581 */
	/* if the digital time zone does not have a +/- sign. */
	if (!parse_timezone(tm, str1, &tz1))
		PG_RETURN_NULL();
	if (!parse_timezone(tm, str2, &tz2))
		PG_RETURN_NULL();

	/* calculation of date from from_tz to to_tz. */
	tm_tmp = time_to_gmt_sec(tm, -tz1);
	if (tm_tmp)
		mys_sec_to_time(tm, tm_tmp, -tz2);

	datetemp = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) ;
	datetemp = datetemp * 1000000;
	time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
	result = datetemp + time;

	snprintf(buf, MAXDATELEN, "%lld.%06d", (long long) result, fsec);

	nic = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											   CStringGetDatum(buf),
											   ObjectIdGetDatum(InvalidOid),
											   Int32GetDatum((((14 + dec) << 16) | dec) + VARHDRSZ)));

	PG_RETURN_NUMERIC(nic);
}

/*
 * get_format(date/datetime/time, 'EUR'|'USA'|'JIS'|'ISO'|'INTERNAL')
 */
Datum
get_format(PG_FUNCTION_ARGS)
{
	VarChar    *result; /* BUGZ5533 */
	int32		time_type = PG_GETARG_INT32(0);
	text	   *units = PG_GETARG_TEXT_PP(1);
	const char *result_str = NULL;

	char   *lowunits = downcase_truncate_identifier(VARDATA_ANY(units), VARSIZE_ANY_EXHDR(units), false);
	bool	find_format = false;
	int		i;
	for (i = 0; i < szdate_time_formats; i++)
	{
		if (strcmp(lowunits, date_time_formats[i].format_name) == 0)
		{
			find_format = true;
			break;
		}
	}

	if (!find_format)
		PG_RETURN_NULL();

	switch (time_type)
	{
		case DTK_DATE:
			result_str = date_time_formats[i].date_format;
			break;
		case DTK_DATE_TIME:
			result_str = date_time_formats[i].datetime_format;
			break;
		case DTK_TIME:
			result_str = date_time_formats[i].time_format;
			break;
		default:
			Assert(0);  /* impossible */
			break;
	}

	/* Begin - BUGZ5533 */
	result = DatumGetVarCharP(DirectFunctionCall3(varcharin, CStringGetDatum(pstrdup(result_str)),
															  ObjectIdGetDatum(InvalidOid),
															  Int32GetDatum(-1)));
	PG_RETURN_VARCHAR_P(result);
	/* End - BUGZ5533 */
}
