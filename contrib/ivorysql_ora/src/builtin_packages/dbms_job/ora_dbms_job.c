/*---------------------------------------------------------------------
 * 
 * File: ora_dbms_job.c 
 *
 * Abstract: 
 * 		Compatible with Oracle DBMS_JOB Package.
 *
 * Authored by dongyongbo@highgo.com, 20241024.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/builtin_packages/dbms_job/ora_dbms_job.c
 *
 *---------------------------------------------------------------------
 */
#include "postgres.h"
#include <float.h>
#include "fmgr.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"
#include "../../include/ora_dbms_jobdata.h"

PG_FUNCTION_INFO_V1(evaluate_calendar_string); /* BUG#M0000349 */

#define schedule_isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

const int	schedule_day_tab[2][13] =
{
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0}
};

/* Begin - BUG#M0000349 */
typedef enum
{
	CARRY_NONE,
	CARRY_MONTH,
	CARRY_MONTHDAY,
	CARRY_HOUR,
	CARRY_MINUTE,
	CARRY_SECOND
} CarryDomain;
/* End - BUG#M0000349 */

/* Begin - BUG#M0000349 */
static ScheduleData* transform_schedule(Schedule *schedule, Timestamp start_date);
static void schedule_validity_check(Schedule *schedule, ScheduleData *sch);
static void init_array_from_byclause(Schedule *schedule, char *array, ByClauseArrayType type);
static void init_array_from_start_date(struct pg_tm * tm, char *array, ByClauseArrayType type);
static void init_array_from_frequency(int interval, struct pg_tm * tm, char *array, ByClauseArrayType type);
static void init_array_all_set(char *array, ByClauseArrayType type);
static void init_negative_monthday_array(Schedule *schedule, char *array, int *negative_monthday_count);
static char *update_monthday_array(ScheduleData *sch, int year, int month);
static Timestamp next_run_date_yearly(Timestamp last_date, ScheduleData *sch);
static Timestamp next_run_date_monthly(Timestamp last_date, ScheduleData *sch);
static Timestamp next_run_date_daily(Timestamp last_date, ScheduleData *sch);
static Timestamp next_run_date_hourly(Timestamp last_date, ScheduleData *sch);
static Timestamp next_run_date_minutely(Timestamp last_date, ScheduleData *sch);
static Timestamp next_run_date_secondly(Timestamp last_date, ScheduleData *sch);
static Timestamp job_next_run_date(Timestamp last_date, ScheduleData *sch, freqType freq);
static int32 fetch_second(char *second, int index, bool carry);
static int32 fetch_minute(char *minute, int index, bool carry);
static int32 fetch_hour(char *hour, int index, bool carry);
static int32 fetch_monthday(char *monthday, int index, bool carry);
static int32 fetch_month(char *month, int index, bool carry);
static int fetch_array_first_set(char *array, CalendarType type);

/* End - BUG#M0000349 */


/* Begin - BUG#M0000349 */
/*
 * Calculates the next date a job will run based on a calendar syntax expression.
 *
 * start_date:
 *	this is job's start date or the last calculated theoretical execution date.
 *	the start_date parameter is not the same as Oracle. perhaps the theoretical
 *	execution time of the last calculation of the JOB should be obtained from the
 *	system table instead of being passed through start_date parameters.
 *
 * return_date_after:
 *	this parameter indicates that the next execution date of the job we calculate
 *	must be after return_date_after. typically, this is the actual date the job
 *	last run.this parameter is mainly used to handle long running time of the job.
 *
 * calendar_string:
 *	Calendar syntax expression.
 *
 * first_evaluation:
 *	Used to identify whether it is the first call to calculate the date of the
 *	next job run. In the future this parameter should be removed and obtained
 *	from the system table.
 *
 * If successful, the next run date of the JOB will be returned.
 * If the calendar syntax expression is unreasonable, throw an error.
 * If the given calendar_string is not a calendar syntax, returns NULL.
 */
Datum
evaluate_calendar_string(PG_FUNCTION_ARGS)
{
	Timestamp		start_date;
	Timestamp		last_date;
	Timestamp		return_date_after;
	char			*calendar_string;
	Schedule		*schedule = palloc(sizeof(Schedule));
	ScheduleData	*transformed;
	Timestamp		next_date;
	bool			first_evaluation = false;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	else
		calendar_string = text_to_cstring(PG_GETARG_TEXT_PP(0));

	if (PG_ARGISNULL(1))
		start_date = DatumGetTimestamp(DirectFunctionCall1(timestamptz_timestamp, TimestampTzGetDatum(GetCurrentTimestamp())));
	else
		start_date = PG_GETARG_TIMESTAMP(1);

	if (PG_ARGISNULL(2))
		return_date_after = DatumGetTimestamp(DirectFunctionCall1(timestamptz_timestamp, TimestampTzGetDatum(GetCurrentTimestamp())));
	else
		return_date_after = PG_GETARG_TIMESTAMP(2);

	if (PG_ARGISNULL(3))
		PG_RETURN_NULL();
	else
		first_evaluation = PG_GETARG_BOOL(3);

	/*
	 * Phase 1
	 * Given a calendar_string, do lexical and grammer analysis.
	 * If it is a calendar syntax, return a raw parse tree.
	 */
	schedule_scanner_init(calendar_string);


	if (schedule_yyparse(schedule) != 0)
		schedule_yyerror(schedule, "schedule parser failed");


	schedule_scanner_finish();

	/*
	 * Phase 2
	 * Analyze a raw parse tree.
	 */
	transformed = transform_schedule(schedule, start_date);

	/*
	 * Phase 3
	 * Calculate the next run date.
	 */
	if (first_evaluation)
	{
		switch(schedule->freq->predef_freq_type)
		{
			case YEARLY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("year"),
												TimestampGetDatum(start_date)));
				break;
			case MONTHLY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("month"),
												TimestampGetDatum(start_date)));
				break;
			case DAILY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("day"),
												TimestampGetDatum(start_date)));
				break;
			case HOURLY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("hour"),
												TimestampGetDatum(start_date)));
				break;
			case MINUTELY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("minute"),
												TimestampGetDatum(start_date)));
				break;
			case SECONDLY:
				last_date = DatumGetTimestamp(DirectFunctionCall2(timestamp_trunc, 
												CStringGetTextDatum("second"),
												TimestampGetDatum(start_date)));
				break;
			case WEEKLY:
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("The frequency WEEKLY is not supported yet.")));
				break;
			default:
				elog(ERROR, "Invalid frequency type was specified."); /* shouldn't happen */
		}
	}
	else
		last_date = start_date;

	next_date = job_next_run_date(last_date, transformed, schedule->freq->predef_freq_type);

	while (return_date_after > next_date)
	{
		next_date = job_next_run_date(next_date, transformed, schedule->freq->predef_freq_type);
	}

	PG_RETURN_TIMESTAMP(next_date);
}
/* End - BUG#M0000349 */

static void
schedule_validity_check(Schedule *schedule, ScheduleData *sch)
{
	List		*calendar_list = schedule->calendar_list;
	ListCell	*cell;

	/*
	 * Not support user-defined frequency types.
	 * Predefined frequency 'WEEKLY' are not supported.
	 */
	if (schedule->freq->user_freq_type != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("User defined frequency type are not supported.")));

	if (schedule->freq->predef_freq_type == WEEKLY)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("The frequency WEEKLY is not supported yet.")));

	/* Check each schedule clause */
	foreach(cell,calendar_list)
	{
		Calendar *cal = lfirst(cell);
		switch(cal->calendar_type)
		{
			case INTERVAL:
				{
					/*
					 * NOTE:
					 * The following description is from Oracle official documentation:
					 * This specifies a positive integer representing how often the recurrence repeats.
					 * The default is 1, which means every second for secondly, every day for daily,and
					 * so on. The maximum value is 99.
					 *
					 * In fact, our test results are different from its documentation description. the
					 * actual test shows that the maximum value of INTERVAL is allowed to exceed 99,
					 * For example: 
					 *		FREQ=MINUTELY;INTERVAL=345;BYMONTH=5;BYMONTHDAY=28;BYHOUR=19
					 * Our implementation is correct even if it exceeds the maximum value of 99, but
					 * we choose to trust Oracle's documentation.
					 */
					CalendarElem	*elem = (CalendarElem *)linitial(cal->elem_list);
					int		freq_interval = elem->elem_value.int_val;

					if (freq_interval < 0 || freq_interval > 99)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("the valid range of interval clause is 1 through 99.")));
					sch->interval = freq_interval;
				}
				break;
			case BYMONTH:
				{
					ListCell		*val_cell;

					if (sch->has_bymonth)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("calendar clause BYMONTH specified more than once.")));
					else
						sch->has_bymonth = true;

					foreach(val_cell, cal->elem_list)
					{
						CalendarElem	*elem;
						int		month_val;

						elem = (CalendarElem *)lfirst(val_cell);
						month_val = elem->elem_value.int_val;

						if (month_val < 1 || month_val > 12)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("Invalid BYMONTH clause value.")));
					}
				}
				break;
			case BYWEEKNO:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("Currently byweekno clause is not supported.")));
				break;
			case BYYEARDAY:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("Currently byyearday clause is not supported.")));
				break;
			case BYDATE:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("Currently bydate clause is not supported.")));
				break;
			case BYMONTHDAY:
				{
					ListCell		*val_cell;

					if (sch->has_bymonthday)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("calendar clause BYMONTHDAY specified more than once.")));
					else
						sch->has_bymonthday = true;

					foreach(val_cell, cal->elem_list)
					{
						CalendarElem	*elem;
						int		monthday_val;

						elem = (CalendarElem *)lfirst(val_cell);
						monthday_val = elem->elem_value.int_val;

						if (monthday_val == 0 || monthday_val < -31 || monthday_val > 31)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("Invalid BYMONTHDAY clause value.")));
					}
				}
				break;
			case BYDAY:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("Currently byday clause is not supported.")));
				break;
			case BYHOUR:
				{
					ListCell		*val_cell;

					if (sch->has_byhour)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("calendar clause BYHOUR specified more than once.")));
					else
						sch->has_byhour = true;

					foreach(val_cell, cal->elem_list)
					{
						CalendarElem	*elem;
						int		hour_val;

						elem = (CalendarElem *)lfirst(val_cell);
						hour_val = elem->elem_value.int_val;

						if (hour_val < 0 || hour_val > 23)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("Invalid BYHOUR clause value.")));
					}
				}
				break;
			case BYMINUTE:
				{
					ListCell		*val_cell;

					if (sch->has_byminute)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("calendar clause BYMINUTE specified more than once.")));
					else
						sch->has_byminute = true;

					foreach(val_cell, cal->elem_list)
					{
						CalendarElem	*elem;
						int		minute_val;

						elem = (CalendarElem *)lfirst(val_cell);
						minute_val = elem->elem_value.int_val;

						if (minute_val < 0 || minute_val > 59)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("Invalid BYMINUTE clause value.")));
					}
				}
				break;
			case BYSECOND:
				{
					ListCell		*val_cell;

					if (sch->has_bysecond)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("calendar clause BYSECOND specified more than once.")));
					else
						sch->has_bysecond = true;

					foreach(val_cell, cal->elem_list)
					{
						CalendarElem	*elem;
						int		second_val;

						elem = (CalendarElem *)lfirst(val_cell);
						second_val = elem->elem_value.int_val;

						if (second_val < 0 || second_val > 59)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("Invalid BYSECOND clause value.")));
					}
				}
				break;
			default:
				elog(ERROR, "Invalid calendar syntax clause type."); /* shouldn't happen */
		}

	}

	/* If user explicitly specified INTERVAL=0, we corrected it. */
	if (sch->interval == 0)
		sch->interval = 1;
}

/*
 * Check whether a word is one of kewwords.
 */
const keyword *
keywordLookup(const char *text,
				  const keyword *keywords,
				  int num_keywords)
{
	int			len,
				i;
	char		word[NAMEDATALEN];
	const keyword *low;
	const keyword *high;

	len = strlen(text);
	/* We assume all keywords are shorter than NAMEDATALEN. */
	if (len >= NAMEDATALEN)
		return NULL;

	/*
	 * Apply an ASCII-only downcasing.  We must not use tolower() since it may
	 * produce the wrong translation in some locales (eg, Turkish).
	 */
	for (i = 0; i < len; i++)
	{
		char		ch = text[i];

		if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 'A';
		word[i] = ch;
	}
	word[len] = '\0';

	/*
	 * Now do a binary search using plain strcmp() comparison.
	 */
	low = keywords;
	high = keywords + (num_keywords - 1);
	while (low <= high)
	{
		const keyword *middle;
		int			difference;

		middle = low + (high - low) / 2;
		difference = strcmp(middle->name, word);
		if (difference == 0)
			return middle;
		else if (difference < 0)
			low = middle + 1;
		else
			high = middle - 1;
	}

	return NULL;
}

/* Begin - BUG#M0000349 */
static ScheduleData*
transform_schedule(Schedule *schedule, Timestamp start_date)
{
	ScheduleData *sch = palloc0(sizeof(ScheduleData));
	fsec_t		fsec;
	struct pg_tm tt,
		   *tm = &tt;

	sch->interval = 1;
	sch->negative_monthday_count = 0;
	sch->has_bymonth = false;
	sch->has_bymonthday = false;
	sch->has_byhour = false;
	sch->has_byminute = false;
	sch->has_bysecond = false;

	/* parse tree check */
	schedule_validity_check(schedule, sch);

	/*
	 * Initialize arrays.
	 */
	if (timestamp2tm(start_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	 switch(schedule->freq->predef_freq_type)
	 {
	 	/*
	 	 * If the BY clause of the datetime fields before FREQ is not specified, these fields will be executed in each cycle.
	 	 * The datetime field corresponding to FREQ is initialized according to the INTERVAL clause.
	 	 * If the BY clause of the datetime fields after FREQ is not specified, the values of these fields from start_date.
	 	 */
	 	case YEARLY:
			{
				/* month array */
				if (sch->has_bymonth)
					init_array_from_byclause(schedule, sch->month, BYMONTH_ARRAY);
				else
					init_array_from_start_date(tm, sch->month, BYMONTH_ARRAY);

				/* monthday array */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
					init_array_from_start_date(tm, sch->monthday, BYMONTHDAY_ARRAY);

				/* hour array */
				if (sch->has_byhour)
					init_array_from_byclause(schedule, sch->hour, BYHOUR_ARRAY);
				else
					init_array_from_start_date(tm, sch->hour, BYHOUR_ARRAY);

				/* minute array */
				if (sch->has_byminute)
					init_array_from_byclause(schedule, sch->minute, BYMINUTE_ARRAY);
				else
					init_array_from_start_date(tm, sch->minute, BYMINUTE_ARRAY);

				/* second array */
				if (sch->has_bysecond)
					init_array_from_byclause(schedule, sch->second, BYSECOND_ARRAY);
				else
					init_array_from_start_date(tm, sch->second, BYSECOND_ARRAY);
	 		}
			break;
	 	case MONTHLY:
			{
				/* FREQ */
				init_array_from_frequency(sch->interval, tm, sch->month, BYMONTH_ARRAY);
				if (sch->has_bymonth)
				{
					char	freq_month[12] = {0};
					bool	conflict = true;
					int		i;

					init_array_from_byclause(schedule, freq_month, BYMONTH_ARRAY);

					for (i = 0; i < 12; i++)
					{
						if (freq_month[i] == sch->month[i] && sch->month[i] == 1)
							conflict = false;
						else
							sch->month[i] = 0;
					}

					if (conflict)
						elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
				}

				/* monthday array */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
					init_array_from_start_date(tm, sch->monthday, BYMONTHDAY_ARRAY);

				/* hour array */
				if (sch->has_byhour)
					init_array_from_byclause(schedule, sch->hour, BYHOUR_ARRAY);
				else
					init_array_from_start_date(tm, sch->hour, BYHOUR_ARRAY);

				/* minute array */
				if (sch->has_byminute)
					init_array_from_byclause(schedule, sch->minute, BYMINUTE_ARRAY);
				else
					init_array_from_start_date(tm, sch->minute, BYMINUTE_ARRAY);

				/* second array */
				if (sch->has_bysecond)
					init_array_from_byclause(schedule, sch->second, BYSECOND_ARRAY);
				else
					init_array_from_start_date(tm, sch->second, BYSECOND_ARRAY);
	 		}
			break;
	 	case DAILY:
			{
				/* month array */
				if (sch->has_bymonth)
					init_array_from_byclause(schedule, sch->month, BYMONTH_ARRAY);
				else
					init_array_all_set(sch->month, BYMONTH_ARRAY);

				/* FREQ */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
				{
					/*
					 * If the FREQ domain does not have a BY clause, then every time after 
					 * the last FREQ domain plus INTERVAL meets the requirements, so we can
					 * assume that it is executed every DAY.
					 */
					init_array_all_set(sch->monthday, BYMONTHDAY_ARRAY);
				}

				/* hour array */
				if (sch->has_byhour)
					init_array_from_byclause(schedule, sch->hour, BYHOUR_ARRAY);
				else
					init_array_from_start_date(tm, sch->hour, BYHOUR_ARRAY);

				/* minute array */
				if (sch->has_byminute)
					init_array_from_byclause(schedule, sch->minute, BYMINUTE_ARRAY);
				else
					init_array_from_start_date(tm, sch->minute, BYMINUTE_ARRAY);

				/* second array */
				if (sch->has_bysecond)
					init_array_from_byclause(schedule, sch->second, BYSECOND_ARRAY);
				else
					init_array_from_start_date(tm, sch->second, BYSECOND_ARRAY);
	 		}
			break;
	 	case HOURLY:
			{
				/* month array */
				if (sch->has_bymonth)
					init_array_from_byclause(schedule, sch->month, BYMONTH_ARRAY);
				else
					init_array_all_set(sch->month, BYMONTH_ARRAY);

				/* monthday array */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
					init_array_all_set(sch->monthday, BYMONTHDAY_ARRAY);

				/* FREQ */
				init_array_from_frequency(sch->interval, tm, sch->hour, BYHOUR_ARRAY);
				if (sch->has_byhour)
				{
					char	by_hour[24] = {0};
					bool	conflict = true;
					int		i;

					init_array_from_byclause(schedule, by_hour, BYHOUR_ARRAY);

					for (i = 0; i < 24; i++)
					{
						if (sch->hour[i] == by_hour[i] && sch->hour[i] == 1)
							conflict = false;
						else
							sch->hour[i] = 0;
					}

					if (conflict)
						elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
				}

				/* minute array */
				if (sch->has_byminute)
					init_array_from_byclause(schedule, sch->minute, BYMINUTE_ARRAY);
				else
					init_array_from_start_date(tm, sch->minute, BYMINUTE_ARRAY);

				/* second array */
				if (sch->has_bysecond)
					init_array_from_byclause(schedule, sch->second, BYSECOND_ARRAY);
				else
					init_array_from_start_date(tm, sch->second, BYSECOND_ARRAY);
	 		}
			break;
	 	case MINUTELY:
			{
				/* month array */
				if (sch->has_bymonth)
					init_array_from_byclause(schedule, sch->month, BYMONTH_ARRAY);
				else
					init_array_all_set(sch->month, BYMONTH_ARRAY);

				/* monthday array */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
					init_array_all_set(sch->monthday, BYMONTHDAY_ARRAY);

				/* hour array */
				if (sch->has_byhour)
					init_array_from_byclause(schedule, sch->hour, BYHOUR_ARRAY);
				else
					init_array_all_set(sch->hour, BYHOUR_ARRAY);

				/* FREQ */
				init_array_from_frequency(sch->interval, tm, sch->minute, BYMINUTE_ARRAY);
				if (sch->has_byminute)
				{
					char	by_minute[60] = {0};
					bool	conflict = true;
					int		i;

					init_array_from_byclause(schedule, by_minute, BYMINUTE_ARRAY);

					for (i = 0; i < 60; i++)
					{
						if (sch->minute[i] == by_minute[i] && sch->minute[i] == 1)
							conflict = false;
						else
							sch->minute[i] = 0;
					}

					if (conflict)
						elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
				}

				/* second array */
				if (sch->has_bysecond)
					init_array_from_byclause(schedule, sch->second, BYSECOND_ARRAY);
				else
					init_array_from_start_date(tm, sch->second, BYSECOND_ARRAY);
	 		}
			break;
	 	case SECONDLY:
			{
				/* month array */
				if (sch->has_bymonth)
					init_array_from_byclause(schedule, sch->month, BYMONTH_ARRAY);
				else
					init_array_all_set(sch->month, BYMONTH_ARRAY);

				/* monthday array */
				if (sch->has_bymonthday)
				{
					init_array_from_byclause(schedule, sch->monthday, BYMONTHDAY_ARRAY);
					init_negative_monthday_array(schedule, (char *) sch->negative_monthday, &(sch->negative_monthday_count));
				}
				else
					init_array_all_set(sch->monthday, BYMONTHDAY_ARRAY);

				/* hour array */
				if (sch->has_byhour)
					init_array_from_byclause(schedule, sch->hour, BYHOUR_ARRAY);
				else
					init_array_all_set(sch->hour, BYHOUR_ARRAY);

				/* minute array */
				if (sch->has_byminute)
					init_array_from_byclause(schedule, sch->minute, BYMINUTE_ARRAY);
				else
					init_array_all_set(sch->minute, BYMINUTE_ARRAY);

				/* FREQ */
				init_array_from_frequency(sch->interval, tm, sch->second, BYSECOND_ARRAY);
				if (sch->has_bysecond)
				{
					char	by_second[60] = {0};
					bool	conflict = true;
					int		i;

					init_array_from_byclause(schedule, by_second, BYSECOND_ARRAY);

					for (i = 0; i < 60; i++)
					{
						if (sch->second[i] == by_second[i] && sch->second[i] == 1)
							conflict = false;
						else
							sch->second[i] = 0;
					}

					if (conflict)
						elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
				}
	 		}
			break;
	 	default:
			break;
	 }

	/*
	 * Final Check
	 * For obviously wrong, we don't need to execute the algorithm to calculate the next run time.
	 * We detect the following conflicts: 
	 * 1.If no negative MONTHDAY then the minimum value of the list of days specified
	 *	 by the BYMONTHDAY clause is greater than the maximum days of all months in the
	 *	 list of months specified by BYMONTH, we will directly throw an error, if exist
	 *	 negative MONTHDAY,we need to check separately according to the specified month
	 *	 example:
	 *    (BYMONTH = 2,6;BYMONTHDAY = 31) not ok.
	 *    (BYMONTH = 2,6;BYMONTHDAY = 30,31) ok.
	 *    (BYMONTH = 2,6;BYMONTHDAY = -31) not ok.
	 *    (BYMONTH = 2,6;BYMONTHDAY = -30,-31) ok.
	 * 2. TODO: All BY clause arrays must have at least one element set?
	 *	more check maybe added in the future?
	 */
	Assert(sch->interval > 0);

	if (sch->negative_monthday_count > 0)
	{
		int		idx;
		int		month_max_day = 0;
		bool	hit = false;

		for (idx = 0; idx < 12; idx++)
		{
			int		i;
			char	*updated_monthday = NULL;

			if (sch->month[idx] == 1)
			{
				month_max_day = schedule_day_tab[1][idx];
				updated_monthday = update_monthday_array(sch, 2024, idx + 1);

				for (i = 0; i < month_max_day; i++)
					if (updated_monthday[i] == 1)
						hit = true;
				pfree(updated_monthday);
			}
			else
				continue;

			if (hit)
				break;
		}

		if (!hit)
			elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
	}
	else
	{
		int		idx;
		int		month_max_day = 0;
		int		monthday_min_day = 0;

		for (idx = 0; idx < 12; idx++)
		{
			int	tmp = 0;

			/* It is unclear whether YEAR is a leap year, so the maximum value is selected */
			if (sch->month[idx] == 1)
				tmp = schedule_day_tab[1][idx];
			if (tmp > month_max_day)
				month_max_day = tmp;
		}

		for (idx = 0; idx < 31; idx++)
		{
			/* It is unclear whether YEAR is a leap year, so the maximum value is selected */
			if (sch->monthday[idx] == 1)
			{
				monthday_min_day = idx + 1;
				break;
			}
		}

		if (monthday_min_day > month_max_day)
			elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
	}

	return sch;
}

static void
init_array_from_byclause(Schedule *schedule, char *array, ByClauseArrayType type)
{
	ListCell	*cell,
				*val_cell;

	foreach(cell,schedule->calendar_list)
	{
		Calendar *cal = lfirst(cell);
		ByClauseArrayType calendar_type;

		switch (cal->calendar_type)
		{
			case BYMONTH:
				calendar_type = BYMONTH_ARRAY;
				break;
			case BYMONTHDAY:
				calendar_type = BYMONTHDAY_ARRAY;
				break;
			case BYHOUR:
				calendar_type = BYHOUR_ARRAY;
				break;
			case BYMINUTE:
				calendar_type = BYMINUTE_ARRAY;
				break;
			case BYSECOND:
				calendar_type = BYSECOND_ARRAY;
				break;
			/* keep compiler quiet */
			case INTERVAL:
			case BYWEEKNO:
			case BYYEARDAY:
			case BYDATE:
			case BYDAY:
			default:
				continue;
		}

		if (calendar_type == type)
		{
			switch (type)
			{
				case BYMONTH_ARRAY:
					{
						CalendarElem	*elem;

						foreach(val_cell, cal->elem_list)
						{
							elem = (CalendarElem *)lfirst(val_cell);
							array[elem->elem_value.int_val - 1] = 1;
						}
					}
					break;
				case BYMONTHDAY_ARRAY:
					{
						CalendarElem	*elem;

						/* Only positive monthdays are considered */
						foreach(val_cell, cal->elem_list)
						{
							elem = (CalendarElem *)lfirst(val_cell);
							if (elem->elem_value.int_val > 0)
								array[elem->elem_value.int_val - 1] = 1;
						}
					}
					break;
				case BYHOUR_ARRAY:
				case BYMINUTE_ARRAY:
				case BYSECOND_ARRAY:
					{
						CalendarElem	*elem;

						foreach(val_cell, cal->elem_list)
						{
							elem = (CalendarElem *)lfirst(val_cell);
							array[elem->elem_value.int_val] = 1;
						}
					}
					break;
				default:
					elog(ERROR, "An invalid datetime domain was specified during initialization.");	/* shouldn't happen */
			}

			break;
		}
	}
}

static void
init_array_from_start_date(struct pg_tm * tm, char *array, ByClauseArrayType type)
{
	switch (type)
	{
		case BYMONTH_ARRAY:
			array[tm->tm_mon - 1] = 1;
			break;
		case BYMONTHDAY_ARRAY:
			array[tm->tm_mday - 1] = 1;
			break;
		case BYHOUR_ARRAY:
			array[tm->tm_hour] = 1;
			break;
		case BYMINUTE_ARRAY:
			array[tm->tm_min] = 1;
			break;
		case BYSECOND_ARRAY:
			array[tm->tm_sec] = 1;
			break;
		default:
			break;
	}
}

static void
init_array_from_frequency(int interval, struct pg_tm * tm, char *array, ByClauseArrayType type)
{
	switch (type)
	{
		case BYMONTH_ARRAY:
			{
				int	idx = tm->tm_mon - 1;

				interval = interval % 12;

				if (interval == 0)
					array[idx] = 1;
				else
				{
					do {
						array[idx] = 1;
						idx = (idx + interval) % 12;
					} while (idx != tm->tm_mon - 1);
				}
			}
			break;
		case BYMONTHDAY_ARRAY:
			{
				/*
				 * TODO:
				 * 28 29 30 31.
				 * Currently, only the value of the BY clause is used temporarily
				 */
				 
			}
			break;
		case BYHOUR_ARRAY:
			{
				int	idx = tm->tm_hour;

				interval = interval % 24;

				if (interval == 0)
					array[idx] = 1;
				else
				{
					do {
						array[idx] = 1;
						idx = (idx + interval) % 24;
					} while (idx != tm->tm_hour);
				}
			}
			break;
		case BYMINUTE_ARRAY:
			{
				int	idx = tm->tm_min;

				interval = interval % 60;

				if (interval == 0)
					array[idx] = 1;
				else
				{
					do {
						array[idx] = 1;
						idx = (idx + interval) % 60;
					} while (idx != tm->tm_min);
				}
			}
			break;
		case BYSECOND_ARRAY:
			{
				int	idx = tm->tm_sec;

				interval = interval % 60;

				if (interval == 0)
					array[idx] = 1;
				else
				{
					do {
						array[idx] = 1;
						idx = (idx + interval) % 60;
					} while (idx != tm->tm_sec);
				}
			}
			break;
	}
}

static void
init_array_all_set(char *array, ByClauseArrayType type)
{
	int idx;

	switch (type)
	{
		case BYMONTH_ARRAY:
			for (idx = 0; idx < 12; idx++)
				array[idx] = 1;
			break;
		case BYMONTHDAY_ARRAY:
			for (idx = 0; idx < 31; idx++)	/* May not be used */
				array[idx] = 1;
			break;
		case BYHOUR_ARRAY:
			for (idx = 0; idx < 24; idx++)
				array[idx] = 1;
			break;
		case BYMINUTE_ARRAY:
		case BYSECOND_ARRAY:
			for (idx = 0; idx < 60; idx++)
				array[idx] = 1;
			break;
	}
}

static void
init_negative_monthday_array(Schedule *schedule, char *array, int *negative_monthday_count)
{
	ListCell	*cell,
				*val_cell;

	foreach(cell,schedule->calendar_list)
	{
		Calendar *cal = lfirst(cell);

		if (cal->calendar_type == BYMONTHDAY)
		{
			CalendarElem	*elem;
			int		i = 0;

			/* Only negative monthdays are considered */
			foreach(val_cell, cal->elem_list)
			{
				elem = (CalendarElem *)lfirst(val_cell);
				if (elem->elem_value.int_val < 0)
					array[i++] = elem->elem_value.int_val;
			}

			if (negative_monthday_count)
				*negative_monthday_count = i;
			break;
		}
	}
}

/*
 * If the BYMONTHDAY clause specifies a negative number,
 * we store it separately in the negative_monthday array.
 * Since the number of days in each month is different,
 * the day pointed to by the negative number is also different.
 * we need to perform dynamic calculations based on the year and
 * month when using it.
 */
static char *
update_monthday_array(ScheduleData *sch, int year, int month)
{
	int		i;
	int		mondays = schedule_day_tab[schedule_isleap(year)][month - 1];
	char	*res;

	res = (char *) palloc0(31 * sizeof(char));

	for (i = 0; i < 31; i++)
		res[i] = sch->monthday[i];

	if (sch->negative_monthday_count <= 0)
		return res;

	for(i = 0; i < sch->negative_monthday_count; i++)
	{
		int	idx = mondays + sch->negative_monthday[i];

		if (idx >= 0)
			res[idx] = 1;
	}

	return res;
}

static Timestamp
next_run_date_yearly(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year;
	int32		r_month;
	int32		r_mday;
	int32		r_hour;
	int32		r_min;
	int32		r_sec;
	CarryDomain	carry = CARRY_NONE;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	fsec_t		fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(last_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	r_year = tm->tm_year;
	r_month = tm->tm_mon;
	r_mday = tm->tm_mday;
	r_hour = tm->tm_hour;
	r_min = tm->tm_min;
	r_sec = tm->tm_sec;

	r_sec = fetch_second(second, r_sec, true);
	if (r_sec == -1)
		carry = CARRY_SECOND;

	r_min = fetch_minute(minute, r_min, carry == CARRY_SECOND ? true : false);
	if (r_min == -1)
		carry = CARRY_MINUTE;

	r_hour = fetch_hour(hour, r_hour, carry == CARRY_MINUTE ? true : false);
	if (r_hour == -1)
		carry = CARRY_HOUR;

	if (negative_monthday_count > 0)
	{
		char	*updated_monthday = NULL;

		updated_monthday = update_monthday_array(sch, r_year, r_month);
		r_mday = fetch_monthday(updated_monthday, r_mday, carry == CARRY_HOUR ? true : false);
		if (r_mday == -1)
			carry = CARRY_MONTHDAY;

		r_month = fetch_month(month, r_month, carry == CARRY_MONTHDAY ? true : false);
		if (r_month == -1)
			carry = CARRY_MONTH;
		pfree(updated_monthday);
	}
	else
	{
		r_mday = fetch_monthday(monthday, r_mday, carry == CARRY_HOUR ? true : false);
		if (r_mday == -1)
			carry = CARRY_MONTHDAY;

		r_month = fetch_month(month, r_month, carry == CARRY_MONTHDAY ? true : false);
		if (r_month == -1)
			carry = CARRY_MONTH;
	}

	r_year = (carry == CARRY_MONTH ? r_year + interval : r_year);

	/* Update the carry of all fields after FREQ */
	switch (carry)
	{
		case CARRY_MONTH:
			{
				r_month = fetch_array_first_set(month, BYMONTH);
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					int		i;

					updated_monthday = update_monthday_array(sch, r_year, r_month);
					for(i = 0; i < 31; i++)
					{
						if (updated_monthday[i] == 1)
							break;
					}
					if (i == 31)
						r_mday = -1;
					else
						r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					pfree(updated_monthday);
				}
				else
				{
					r_mday = fetch_array_first_set(monthday, BYMONTHDAY);
				}
				r_hour = fetch_array_first_set(hour, BYHOUR);
				r_min = fetch_array_first_set(minute, BYMINUTE);
				r_sec = fetch_array_first_set(second, BYSECOND);
			}
			break;
		case CARRY_MONTHDAY:
			{
				r_sec = fetch_array_first_set(second, BYSECOND);
				r_min = fetch_array_first_set(minute, BYMINUTE);
				r_hour = fetch_array_first_set(hour, BYHOUR);
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					int		i;

					updated_monthday = update_monthday_array(sch, r_year, r_month);
					for(i = 0; i < 31; i++)
					{
						if (updated_monthday[i] == 1)
							break;
					}
					if (i == 31)
						r_mday = -1;
					else
						r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					pfree(updated_monthday);
				}
				else
				{
					r_mday = fetch_array_first_set(monthday, BYMONTHDAY);
				}
				
			}
			break;
		case CARRY_HOUR:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			r_hour = fetch_array_first_set(hour, BYHOUR);
			break;
		case CARRY_MINUTE:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			break;
		case CARRY_SECOND:
			r_sec = fetch_array_first_set(second, BYSECOND);
			break;
		case CARRY_NONE:
			break;
	}

	/* Checks for leading boundary of month value */
	if (sch->negative_monthday_count > 0 && r_mday == -1)
	{
		bool	month_boundary_invalid = true;

		r_sec = fetch_array_first_set(second, BYSECOND);
		r_min = fetch_array_first_set(minute, BYMINUTE);
		r_hour = fetch_array_first_set(hour, BYHOUR);

		while (month_boundary_invalid)
		{
			r_month = fetch_month(month, r_month, true);	/* step next month */

			if (r_month == -1)
			{
				char	*updated_monthday = NULL;
				int		i;

				/* next FREQ cycle */
				r_year += interval;
				r_month = fetch_array_first_set(month, BYMONTH);
				updated_monthday = update_monthday_array(sch, r_year, r_month);
				for(i = 0; i < 31; i++)
				{
					if (updated_monthday[i] == 1)
						break;
				}
				if (i == 31)
					month_boundary_invalid = true;
				else
				{
					month_boundary_invalid = false;
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
				}
				pfree(updated_monthday);
			}
			else
			{
				char	*updated_monthday = NULL;
				int		i;

				updated_monthday = update_monthday_array(sch, r_year, r_month);
				for(i = 0; i < 31; i++)
				{
					if (updated_monthday[i] == 1)
						break;
				}
				if (i == 31)
					month_boundary_invalid = true;
				else
				{
					month_boundary_invalid = false;
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
				}
				pfree(updated_monthday);
			}

			/* too long? */
			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}	
	}

	/* Checks for trailing boundary of month value */
	if (r_mday > schedule_day_tab[schedule_isleap(r_year)][r_month - 1])
	{
		bool	month_boundary_invalid = true;

		r_sec = fetch_array_first_set(second, BYSECOND);
		r_min = fetch_array_first_set(minute, BYMINUTE);
		r_hour = fetch_array_first_set(hour, BYHOUR);
		r_mday = fetch_array_first_set(monthday, BYMONTHDAY);

		while (month_boundary_invalid)
		{
			r_month = fetch_month(month, r_month, true);

			if (r_month == -1)
			{
				/* next FREQ cycle */
				r_year += interval;
				r_month = fetch_array_first_set(month, BYMONTH);
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					updated_monthday = update_monthday_array(sch, r_year, r_month);
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					pfree(updated_monthday);
				}
				month_boundary_invalid = r_mday > schedule_day_tab[schedule_isleap(r_year)][r_month - 1];
			}
			else
			{
				/* within current FREQ cycle */
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					updated_monthday = update_monthday_array(sch, r_year, r_month);
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					pfree(updated_monthday);
				}
				month_boundary_invalid = r_mday > schedule_day_tab[schedule_isleap(r_year)][r_month - 1];
			}

			/* too long? */
			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}	
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
next_run_date_monthly(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year;
	int32		r_month;
	int32		r_mday;
	int32		r_hour;
	int32		r_min;
	int32		r_sec;
	CarryDomain	carry = CARRY_NONE;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	fsec_t		fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(last_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	r_year = tm->tm_year;
	r_month = tm->tm_mon;
	r_mday = tm->tm_mday;
	r_hour = tm->tm_hour;
	r_min = tm->tm_min;
	r_sec = tm->tm_sec;

	r_sec = fetch_second(second, r_sec, true);
	if (r_sec == -1)
		carry = CARRY_SECOND;

	r_min = fetch_minute(minute, r_min, carry == CARRY_SECOND ? true : false);
	if (r_min == -1)
		carry = CARRY_MINUTE;

	r_hour = fetch_hour(hour, r_hour, carry == CARRY_MINUTE ? true : false);
	if (r_hour == -1)
		carry = CARRY_HOUR;

	/* invalid month boundaries may be fetch */
	if (negative_monthday_count > 0)
	{
		char	*updated_monthday = NULL;

		updated_monthday = update_monthday_array(sch, r_year, r_month);
		r_mday = fetch_monthday(updated_monthday, r_mday, carry == CARRY_HOUR ? true : false);
		if (r_mday == -1)
			carry = CARRY_MONTHDAY;
		pfree(updated_monthday);
	}
	else
	{
		r_mday = fetch_monthday(monthday, r_mday, carry == CARRY_HOUR ? true : false);
		if (r_mday == -1)
			carry = CARRY_MONTHDAY;
	}

retry:
	/* Carry in the FREQ domain */
	if (carry == CARRY_MONTHDAY)
	{
		/* next FREQ cycle */
		bool		before_freq_ok = false;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp = last_date;

		freq_interval->month = interval;
		freq_interval->day = 0;
		freq_interval->time = 0;

		while (!before_freq_ok)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			r_year = tm->tm_year;	/* year always ok */
			if (month[tm->tm_mon - 1] == 1)
			{
				before_freq_ok = true;
				r_month = tm->tm_mon;
			}
			else
				start_timestamp = next_timestamp;

			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}
	}
	else
	{
		/* 
		 * Within current FREQ cycle.
		 * The year and month values are the year and month of the last job run,
		 * but if this is the first calculation, last_date is not the last time
		 * the job was run, so check again.
		 */
		if (month[tm->tm_mon - 1] == 1)
		{
			r_month = tm->tm_mon;
			r_year = tm->tm_year;
		}
		else
		{
			carry = CARRY_MONTHDAY;
			goto retry;
		}
	}

	/* Update the carry of all fields after FREQ */
	switch (carry)
	{
		case CARRY_MONTHDAY:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			r_hour = fetch_array_first_set(hour, BYHOUR);
			if (negative_monthday_count > 0)
			{
				char	*updated_monthday = NULL;
				int		i;

				updated_monthday = update_monthday_array(sch, r_year, r_month);
				for(i = 0; i < 31; i++)
				{
					if (updated_monthday[i] == 1)
						break;
				}
				if (i == 31)
					r_mday = -1;
				else
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
				pfree(updated_monthday);
			}
			else
			{
				r_mday = fetch_array_first_set(monthday, BYMONTHDAY);
			}
			break;
		case CARRY_HOUR:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			r_hour = fetch_array_first_set(hour, BYHOUR);
			break;
		case CARRY_MINUTE:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			break;
		case CARRY_SECOND:
			r_sec = fetch_array_first_set(second, BYSECOND);
			break;
		default:
			break;
	}

	/* Checks for leading boundary of month value */
	if (negative_monthday_count > 0 && r_mday == -1)
	{
		bool		month_boundary_invalid = true;
		struct pg_tm tt, *tm = &tt;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp;

		freq_interval->month = interval;
		freq_interval->day = 0;
		freq_interval->time = 0;

		r_sec = fetch_array_first_set(second, BYSECOND);
		r_min = fetch_array_first_set(minute, BYMINUTE);
		r_hour = fetch_array_first_set(hour, BYHOUR);

		tm->tm_year = r_year;
		tm->tm_mon = r_month;
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;

		if (tm2timestamp(tm, 0, NULL, &start_timestamp) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));

		while (month_boundary_invalid)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			if (month[tm->tm_mon - 1] == 1)
			{
				char	*updated_monthday = NULL;
				int		i;

				updated_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);
				for(i = 0; i < 31; i++)
				{
					if (updated_monthday[i] == 1)
						break;
				}
				if (i == 31)
				{
					month_boundary_invalid = true;
					start_timestamp = next_timestamp;
				}
				else
				{
					month_boundary_invalid = false;
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					r_month = tm->tm_mon;
					r_year = tm->tm_year;
				}
				pfree(updated_monthday);
			}
			else
				start_timestamp = next_timestamp;

			/* too long? */
			if (tm->tm_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}	
	}

	/* Checks for trailing boundary of month value */
	if (r_mday > schedule_day_tab[schedule_isleap(r_year)][r_month - 1])
	{
		bool		month_boundary_invalid = true;
		struct pg_tm tt, *tm = &tt;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp;

		freq_interval->month = interval;
		freq_interval->day = 0;
		freq_interval->time = 0;

		r_sec = fetch_array_first_set(second, BYSECOND);
		r_min = fetch_array_first_set(minute, BYMINUTE);
		r_hour = fetch_array_first_set(hour, BYHOUR);
		r_mday = fetch_array_first_set(monthday, BYMONTHDAY);

		tm->tm_year = r_year;
		tm->tm_mon = r_month;
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;

		if (tm2timestamp(tm, 0, NULL, &start_timestamp) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));

		while (month_boundary_invalid)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			if (month[tm->tm_mon - 1] == 1)
			{
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					updated_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);
					r_mday = fetch_array_first_set(updated_monthday, BYMONTHDAY);
					pfree(updated_monthday);
				}

				if (r_mday <= schedule_day_tab[schedule_isleap(tm->tm_year)][tm->tm_mon - 1])
				{
					month_boundary_invalid = false;
					r_month = tm->tm_mon;
					r_year = tm->tm_year;
				}
				else
					start_timestamp = next_timestamp;
			}
			else
				start_timestamp = next_timestamp;

			/* too long? */
			if (tm->tm_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}	
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
next_run_date_daily(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year;
	int32		r_month;
	int32		r_mday;
	int32		r_hour;
	int32		r_min;
	int32		r_sec;
	CarryDomain	carry = CARRY_NONE;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	fsec_t		fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(last_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	r_year = tm->tm_year;
	r_month = tm->tm_mon;
	r_mday = tm->tm_mday;
	r_hour = tm->tm_hour;
	r_min = tm->tm_min;
	r_sec = tm->tm_sec;

	r_sec = fetch_second(second, r_sec, true);
	if (r_sec == -1)
		carry = CARRY_SECOND;

	r_min = fetch_minute(minute, r_min, carry == CARRY_SECOND ? true : false);
	if (r_min == -1)
		carry = CARRY_MINUTE;

	r_hour = fetch_hour(hour, r_hour, carry == CARRY_MINUTE ? true : false);
	if (r_hour == -1)
		carry = CARRY_HOUR;

retry:
	/* Carry in the FREQ domain */
	if (carry == CARRY_HOUR)
	{
		/* next FREQ cycle */
		bool		before_freq_ok = false;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp = last_date;

		freq_interval->month = 0;
		freq_interval->day = interval;
		freq_interval->time = 0;

		while (!before_freq_ok)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			r_year = tm->tm_year;	/* year always ok */
			if (month[tm->tm_mon - 1] == 1)
			{
				if (negative_monthday_count > 0)
				{
					char	*updated_monthday = NULL;
					updated_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);

					if (updated_monthday[tm->tm_mday - 1] == 1)
					{
						before_freq_ok = true;
						r_month = tm->tm_mon;
						r_mday = tm->tm_mday;
					}
					else
						start_timestamp = next_timestamp;
					pfree(updated_monthday);
				}
				else
				{
					if (monthday[tm->tm_mday - 1] == 1)
					{
						before_freq_ok = true;
						r_month = tm->tm_mon;
						r_mday = tm->tm_mday;
					}
					else
						start_timestamp = next_timestamp;
				}
			}
			else
				start_timestamp = next_timestamp;

			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}
	}
	else
	{
		/* Within current FREQ cycle */
		if (month[tm->tm_mon - 1] == 1)
		{
			if (negative_monthday_count > 0)
			{
				char	*updated_monthday = NULL;
				updated_monthday = update_monthday_array(sch, r_year, tm->tm_mon);

				if (updated_monthday[tm->tm_mday - 1] == 1)
				{
					r_mday = tm->tm_mday;
					r_month = tm->tm_mon;
					r_year = tm->tm_year;
				}
				else
				{
					carry = CARRY_HOUR;
					goto retry;
				}
				pfree(updated_monthday);
			}
			else
			{
				if (monthday[tm->tm_mday - 1] == 1)
				{
					r_mday = tm->tm_mday;
					r_month = tm->tm_mon;
					r_year = tm->tm_year;
				}
				else
				{
					carry = CARRY_HOUR;
					goto retry;
				}
			}
		}
		else
		{
			carry = CARRY_HOUR;
			goto retry;
		}
	}

	/* Update the carry of all fields after FREQ */
	switch (carry)
	{
		case CARRY_HOUR:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			r_hour = fetch_array_first_set(hour, BYHOUR);
			break;
		case CARRY_MINUTE:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			break;
		case CARRY_SECOND:
			r_sec = fetch_array_first_set(second, BYSECOND);
			break;
		default:
			break;
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
next_run_date_hourly(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year;
	int32		r_month;
	int32		r_mday;
	int32		r_hour;
	int32		r_min;
	int32		r_sec;
	CarryDomain	carry = CARRY_NONE;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	fsec_t		fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(last_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	r_year = tm->tm_year;
	r_month = tm->tm_mon;
	r_mday = tm->tm_mday;
	r_hour = tm->tm_hour;
	r_min = tm->tm_min;
	r_sec = tm->tm_sec;

	r_sec = fetch_second(second, r_sec, true);
	if (r_sec == -1)
		carry = CARRY_SECOND;

	r_min = fetch_minute(minute, r_min, carry == CARRY_SECOND ? true : false);
	if (r_min == -1)
		carry = CARRY_MINUTE;

retry:
	/* Carry in the FREQ domain */
	if (carry == CARRY_MINUTE)
	{
		/* HERE */
		/* next FREQ cycle */
		bool		before_freq_ok = false;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp = last_date;

		freq_interval->month = 0;
		freq_interval->day = 0;
#ifdef HAVE_INT64_TIMESTAMP
		freq_interval->time = interval * INT64CONST(60) * INT64CONST(60) * USECS_PER_SEC;
#else
		freq_interval->time = interval * (double) MINS_PER_HOUR * (double) SECS_PER_MINUTE;
#endif

		while (!before_freq_ok)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			char		*tmp_monthday;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			r_year = tm->tm_year;	/* year always ok */

			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				tmp_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);
			else
				tmp_monthday = monthday;
				
			if (month[tm->tm_mon - 1] == 1 && tmp_monthday[tm->tm_mday - 1] == 1 && hour[tm->tm_hour] == 1)
			{
				before_freq_ok = true;
				r_month = tm->tm_mon;
				r_mday = tm->tm_mday;
				r_hour = tm->tm_hour;
			}
			else
				start_timestamp = next_timestamp;

			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);

			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}
	}
	else
	{
		char		*tmp_monthday;

		/* Within current FREQ cycle */
		if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
			tmp_monthday = update_monthday_array(sch, r_year, tm->tm_mon);
		else
			tmp_monthday = monthday;

		if (month[tm->tm_mon - 1] == 1 && tmp_monthday[tm->tm_mday - 1] == 1 && hour[tm->tm_hour] == 1)
		{
			r_hour = tm->tm_hour;
			r_mday = tm->tm_mday;
			r_month = tm->tm_mon;
			r_year = tm->tm_year;

			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);
		}
		else
		{
			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);

			carry = CARRY_MINUTE;
			goto retry;
		}
	}

	/* Update the carry of all fields after FREQ */
	switch (carry)
	{
		case CARRY_MINUTE:
			r_sec = fetch_array_first_set(second, BYSECOND);
			r_min = fetch_array_first_set(minute, BYMINUTE);
			break;
		case CARRY_SECOND:
			r_sec = fetch_array_first_set(second, BYSECOND);
			break;
		default:
			break;
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
next_run_date_minutely(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year;
	int32		r_month;
	int32		r_mday;
	int32		r_hour;
	int32		r_min;
	int32		r_sec;
	CarryDomain	carry = CARRY_NONE;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	fsec_t		fsec;
	struct pg_tm tt, *tm = &tt;

	if (timestamp2tm(last_date, NULL, tm, &fsec, NULL, NULL) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	r_year = tm->tm_year;
	r_month = tm->tm_mon;
	r_mday = tm->tm_mday;
	r_hour = tm->tm_hour;
	r_min = tm->tm_min;
	r_sec = tm->tm_sec;

	r_sec = fetch_second(second, r_sec, true);
	if (r_sec == -1)
		carry = CARRY_SECOND;

retry:
	/* Carry in the FREQ domain */
	if (carry == CARRY_SECOND)
	{
		/* HERE */
		/* next FREQ cycle */
		bool		before_freq_ok = false;
		Interval	*freq_interval = palloc(sizeof(Interval));
		Timestamp	start_timestamp = last_date;

		freq_interval->month = 0;
		freq_interval->day = 0;
#ifdef HAVE_INT64_TIMESTAMP
		freq_interval->time = interval * INT64CONST(60) * USECS_PER_SEC;
#else
		freq_interval->time = interval * (double) SECS_PER_MINUTE;
#endif

		while (!before_freq_ok)
		{
			Timestamp	next_timestamp;
			fsec_t		fsec;
			char		*tmp_monthday;
			struct pg_tm tt, *tm = &tt;

			next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
															TimestampGetDatum(start_timestamp),
															IntervalPGetDatum(freq_interval)));

			if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						 errmsg("timestamp out of range")));

			r_year = tm->tm_year;	/* year always ok */

			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				tmp_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);
			else
				tmp_monthday = monthday;

			if (month[tm->tm_mon - 1] == 1 &&
				tmp_monthday[tm->tm_mday - 1] == 1 &&
				hour[tm->tm_hour] == 1 &&
				minute[tm->tm_min] == 1)
			{
				before_freq_ok = true;
				r_month = tm->tm_mon;
				r_mday = tm->tm_mday;
				r_hour = tm->tm_hour;
				r_min = tm->tm_min;
			}
			else
				start_timestamp = next_timestamp;

			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);

			if (r_year > 9999)
				elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
		}
	}
	else
	{
		/* Within current FREQ cycle */
		char	*tmp_monthday;

		if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
			tmp_monthday = update_monthday_array(sch, r_year, tm->tm_mon);
		else
			tmp_monthday = monthday;

		if (month[tm->tm_mon - 1] == 1 &&
			tmp_monthday[tm->tm_mday - 1] == 1 &&
			hour[tm->tm_hour] == 1 &&
			minute[tm->tm_min] == 1)
		{
			r_min = tm->tm_min;
			r_hour = tm->tm_hour;
			r_mday = tm->tm_mday;
			r_month = tm->tm_mon;
			r_year = tm->tm_year;
			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);
		}
		else
		{
			if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
				pfree(tmp_monthday);
			carry = CARRY_SECOND;
			goto retry;
		}
	}

	/* Update the carry of all fields after FREQ */
	switch (carry)
	{
		case CARRY_SECOND:
			r_sec = fetch_array_first_set(second, BYSECOND);
			break;
		default:
			break;
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
next_run_date_secondly(Timestamp last_date, ScheduleData *sch)
{
	int32		r_year = 0;
	int32		r_month = 0;
	int32		r_mday = 0;
	int32		r_hour = 0;
	int32		r_min = 0;
	int32		r_sec = 0;
	int			interval = sch->interval;
	char		*month = sch->month;
	char		*monthday = sch->monthday;
	char		*hour = sch->hour;
	char		*minute = sch->minute;
	char		*second = sch->second;
	int			negative_monthday_count = sch->negative_monthday_count;
	Timestamp	result;
	struct pg_tm tt;
	bool		before_freq_ok = false;
	Interval	*freq_interval = palloc(sizeof(Interval));
	Timestamp	start_timestamp = last_date;

	freq_interval->month = 0;
	freq_interval->day = 0;
#ifdef HAVE_INT64_TIMESTAMP
	freq_interval->time = interval * USECS_PER_SEC;
#else
	freq_interval->time = interval;
#endif

	while (!before_freq_ok)
	{
		Timestamp	next_timestamp;
		fsec_t		fsec;
		char		*tmp_monthday;
		struct pg_tm tt, *tm = &tt;

		next_timestamp = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval,
														TimestampGetDatum(start_timestamp),
														IntervalPGetDatum(freq_interval)));

		if (timestamp2tm(next_timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));

		r_year = tm->tm_year;	/* year always ok */

		if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
			tmp_monthday = update_monthday_array(sch, tm->tm_year, tm->tm_mon);
		else
			tmp_monthday = monthday;

		if (month[tm->tm_mon - 1] == 1 &&
			tmp_monthday[tm->tm_mday - 1] == 1 &&
			hour[tm->tm_hour] == 1 &&
			minute[tm->tm_min] == 1 &&
			second[tm->tm_sec] == 1)
		{
			before_freq_ok = true;
			r_month = tm->tm_mon;
			r_mday = tm->tm_mday;
			r_hour = tm->tm_hour;
			r_min = tm->tm_min;
			r_sec = tm->tm_sec;
		}
		else
			start_timestamp = next_timestamp;

		if (month[tm->tm_mon - 1] == 1 && negative_monthday_count > 0)
			pfree(tmp_monthday);

		if (r_year > 9999)
			elog(ERROR, "IVY-27419: unable to determine valid execution date from repeat interval");
	}

	tt.tm_year = r_year;
	tt.tm_mon = r_month;
	tt.tm_mday = r_mday;
	tt.tm_hour = r_hour;
	tt.tm_min = r_min;
	tt.tm_sec = r_sec;

	if (tm2timestamp(&tt, 0, NULL, &result) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	return result;
}

static Timestamp
job_next_run_date(Timestamp last_date, ScheduleData *sch, freqType freq)
{
	switch (freq)
	{
		case YEARLY:
			return next_run_date_yearly(last_date, sch);
		case MONTHLY:
			return next_run_date_monthly(last_date, sch);
		case DAILY:
			return next_run_date_daily(last_date, sch);
		case HOURLY:
			return next_run_date_hourly(last_date, sch);
		case MINUTELY:
			return next_run_date_minutely(last_date, sch);
		case SECONDLY:
			return next_run_date_secondly(last_date, sch);
		case WEEKLY:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("The frequency WEEKLY is not supported yet.")));
			break;
		default:
			elog(ERROR, "Invalid frequency type was specified."); /* shouldn't happen */
	}

	// meanless
	return 0;
}


static int32
fetch_second(char *second, int index, bool carry)
{
	int	sec = -1;
	int	i;

	if (index < 0 || index > 59)
		elog(ERROR, "Invalid second value.");	/* shouldn't happen */

	if (carry)
	{
		index += 1;
		if (index >= 60)
		{
			return -1;
		}
	}

	for (i = index; i < 60; i++)
		if (second[i] == 1)
		{
			sec = i;
			break;
		}

	return sec;
}

static int32
fetch_minute(char *minute, int index, bool carry)
{
	int	min = -1;
	int	i;

	if (index < 0 || index > 59)
		elog(ERROR, "Invalid minute value.");	/* shouldn't happen */

	if (carry)
	{
		index += 1;
		if (index >= 60)
		{
			return -1;
		}
	}

	for (i = index; i < 60; i++)
		if (minute[i] == 1)
		{
			min = i;
			break;
		}

	return min;
}

static int32
fetch_hour(char *hour, int index, bool carry)
{
	int	h = -1;
	int	i;

	if (index < 0 || index > 24)
		elog(ERROR, "Invalid hour value.");	/* shouldn't happen */

	if (carry)
	{
		index += 1;
		if (index >= 24)
		{
			return -1;
		}
	}

	for (i = index; i < 24; i++)
		if (hour[i] == 1)
		{
			h = i;
			break;
		}

	return h;
}

static int32
fetch_monthday(char *monthday, int index, bool carry)
{
	int	day = -1;
	int	i;

	if (index < 1 || index > 31)
		elog(ERROR, "Invalid monthday value.");	/* shouldn't happen */

	if (carry)
	{
		index += 1;
		if (index > 31)
		{
			return -1;
		}
	}

	for (i = index; i <= 31; i++)
		if (monthday[i - 1] == 1)
		{
			day = i;
			break;
		}

	return day;
}

static int32
fetch_month(char *month, int index, bool carry)
{
	int	mon = -1;
	int	i;

	if (index < 1 || index > 12)
		elog(ERROR, "Invalid monthday value.");	/* shouldn't happen */

	if (carry)
	{
		index += 1;
		if (index > 12)
		{
			return -1;
		}
	}

	for (i = index; i <= 12; i++)
		if (month[i - 1] == 1)
		{
			mon = i;
			break;
		}

	return mon;
}

static int
fetch_array_first_set(char *array, CalendarType type)
{
	int idx;

	switch (type)
	{
		case BYMONTH:
			for (idx = 0; idx < 12; idx++)
				if (array[idx] == 1)
					return idx + 1;
			break;
		case BYMONTHDAY:
			for (idx = 0; idx < 31; idx++)
				if (array[idx] == 1)
					return idx + 1;
			break;
		case BYHOUR:
			for (idx = 0; idx < 24; idx++)
				if (array[idx] == 1)
					return idx;
			break;
		case BYMINUTE:
		case BYSECOND:
			for (idx = 0; idx < 60; idx++)
				if (array[idx] == 1)
					return idx;
			break;
		default:
			elog(ERROR, "An invalid array type was specified."); /* shouldn't happen */
	}

	/*
	 * The code reache here, indicating that the value of someone array are all 0,
	 * which is an error and should not occur, we must fix it.
	 */
	elog(ERROR, "DBMS_JOB internal error.");

	// meanless
	return 0;
}

/* End - BUG#M0000349 */
