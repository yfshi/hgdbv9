/*-------------------------------------------------------------------------
 *
 * datetime.c
 *	  Support functions for date/time types.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/datetime.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "access/htup_details.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "common/string.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/guc.h"	/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/memutils.h"
#include "utils/ora_compatible.h"	/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/tzparser.h"
#include "utils/varlena.h"	/* ReqID:SRS-MYSQL-DATETYPE */
#include "time.h" /* ReqID:SRS-MYSQL-DATETYPE */

static int	DecodeNumber(int flen, char *field, bool haveTextMonth,
						 int fmask, int *tmask,
						 struct pg_tm *tm, fsec_t *fsec, bool *is2digits);
static int	DecodeNumberField(int len, char *str,
							  int fmask, int *tmask,
							  struct pg_tm *tm, fsec_t *fsec, bool *is2digits);
static int	DecodeTime(char *str, int fmask, int range,
					   int *tmask, struct pg_tm *tm, fsec_t *fsec);
static const datetkn *datebsearch(const char *key, const datetkn *base, int nel);
static int	DecodeDate(char *str, int fmask, int *tmask, bool *is2digits,
					   struct pg_tm *tm);
static char *AppendSeconds(char *cp, int sec, fsec_t fsec,
						   int precision, bool fillzeros);
static void AdjustFractSeconds(double frac, struct pg_tm *tm, fsec_t *fsec,
							   int scale);
static void AdjustFractDays(double frac, struct pg_tm *tm, fsec_t *fsec,
							int scale);
static int	DetermineTimeZoneOffsetInternal(struct pg_tm *tm, pg_tz *tzp,
											pg_time_t *tp);
static bool DetermineTimeZoneAbbrevOffsetInternal(pg_time_t t,
												  const char *abbr, pg_tz *tzp,
												  int *offset, int *isdst);
static pg_tz *FetchDynamicTimeZone(TimeZoneAbbrevTable *tbl, const datetkn *tp);

/* Begin - ReqID:SRS-MYSQL-DATETYPE */
static int CalmicroSecondsPower(int num);

static bool check_datetime(struct pg_tm *tm, bool not_zero_date, int flag, int *warn);

static void get_datetime_from_num(int64 num, struct pg_tm *tm);

bool check_datetime_range(struct pg_tm *tm, int fsec, int ftype);
/* End - ReqID:SRS-MYSQL-DATETYPE */

const int	day_tab[2][13] =
{
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0}
};

const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

const char *const days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
"Thursday", "Friday", "Saturday", NULL};


/*****************************************************************************
 *	 PRIVATE ROUTINES														 *
 *****************************************************************************/

/*
 * datetktbl holds date/time keywords.
 *
 * Note that this table must be strictly alphabetically ordered to allow an
 * O(ln(N)) search algorithm to be used.
 *
 * The token field must be NUL-terminated; we truncate entries to TOKMAXLEN
 * characters to fit.
 *
 * The static table contains no TZ, DTZ, or DYNTZ entries; rather those
 * are loaded from configuration files and stored in zoneabbrevtbl, whose
 * abbrevs[] field has the same format as the static datetktbl.
 */
static const datetkn datetktbl[] = {
	/* token, type, value */
	{EARLY, RESERV, DTK_EARLY}, /* "-infinity" reserved for "early time" */
	{DA_D, ADBC, AD},			/* "ad" for years > 0 */
	{"allballs", RESERV, DTK_ZULU}, /* 00:00:00 */
	{"am", AMPM, AM},
	{"apr", MONTH, 4},
	{"april", MONTH, 4},
	{"at", IGNORE_DTF, 0},		/* "at" (throwaway) */
	{"aug", MONTH, 8},
	{"august", MONTH, 8},
	{DB_C, ADBC, BC},			/* "bc" for years <= 0 */
	{"d", UNITS, DTK_DAY},		/* "day of month" for ISO input */
	{"dec", MONTH, 12},
	{"december", MONTH, 12},
	{"dow", UNITS, DTK_DOW},	/* day of week */
	{"doy", UNITS, DTK_DOY},	/* day of year */
	{"dst", DTZMOD, SECS_PER_HOUR},
	{EPOCH, RESERV, DTK_EPOCH}, /* "epoch" reserved for system epoch time */
	{"feb", MONTH, 2},
	{"february", MONTH, 2},
	{"fri", DOW, 5},
	{"friday", DOW, 5},
	{"h", UNITS, DTK_HOUR},		/* "hour" */
	{LATE, RESERV, DTK_LATE},	/* "infinity" reserved for "late time" */
	{"isodow", UNITS, DTK_ISODOW},	/* ISO day of week, Sunday == 7 */
	{"isoyear", UNITS, DTK_ISOYEAR},	/* year in terms of the ISO week date */
	{"j", UNITS, DTK_JULIAN},
	{"jan", MONTH, 1},
	{"january", MONTH, 1},
	{"jd", UNITS, DTK_JULIAN},
	{"jul", MONTH, 7},
	{"julian", UNITS, DTK_JULIAN},
	{"july", MONTH, 7},
	{"jun", MONTH, 6},
	{"june", MONTH, 6},
	{"m", UNITS, DTK_MONTH},	/* "month" for ISO input */
	{"mar", MONTH, 3},
	{"march", MONTH, 3},
	{"may", MONTH, 5},
	{"mm", UNITS, DTK_MINUTE},	/* "minute" for ISO input */
	{"mon", DOW, 1},
	{"monday", DOW, 1},
	{"nov", MONTH, 11},
	{"november", MONTH, 11},
	{NOW, RESERV, DTK_NOW},		/* current transaction time */
	{"oct", MONTH, 10},
	{"october", MONTH, 10},
	{"on", IGNORE_DTF, 0},		/* "on" (throwaway) */
	{"pm", AMPM, PM},
	{"s", UNITS, DTK_SECOND},	/* "seconds" for ISO input */
	{"sat", DOW, 6},
	{"saturday", DOW, 6},
	{"sep", MONTH, 9},
	{"sept", MONTH, 9},
	{"september", MONTH, 9},
	{"sun", DOW, 0},
	{"sunday", DOW, 0},
	{"t", ISOTIME, DTK_TIME},	/* Filler for ISO time fields */
	{"thu", DOW, 4},
	{"thur", DOW, 4},
	{"thurs", DOW, 4},
	{"thursday", DOW, 4},
	{TODAY, RESERV, DTK_TODAY}, /* midnight */
	{TOMORROW, RESERV, DTK_TOMORROW},	/* tomorrow midnight */
	{"tue", DOW, 2},
	{"tues", DOW, 2},
	{"tuesday", DOW, 2},
	{"wed", DOW, 3},
	{"wednesday", DOW, 3},
	{"weds", DOW, 3},
	{"y", UNITS, DTK_YEAR},		/* "year" for ISO input */
	{YESTERDAY, RESERV, DTK_YESTERDAY}	/* yesterday midnight */
};

static const int szdatetktbl = sizeof datetktbl / sizeof datetktbl[0];

/*
 * deltatktbl: same format as datetktbl, but holds keywords used to represent
 * time units (eg, for intervals, and for EXTRACT).
 */
static const datetkn deltatktbl[] = {
	/* token, type, value */
	{"@", IGNORE_DTF, 0},		/* postgres relative prefix */
	{DAGO, AGO, 0},				/* "ago" indicates negative time offset */
	{"c", UNITS, DTK_CENTURY},	/* "century" relative */
	{"cent", UNITS, DTK_CENTURY},	/* "century" relative */
	{"centuries", UNITS, DTK_CENTURY},	/* "centuries" relative */
	{DCENTURY, UNITS, DTK_CENTURY}, /* "century" relative */
	{"d", UNITS, DTK_DAY},		/* "day" relative */
	{DDAY, UNITS, DTK_DAY},		/* "day" relative */
	{"days", UNITS, DTK_DAY},	/* "days" relative */
	{"dec", UNITS, DTK_DECADE}, /* "decade" relative */
	{DDECADE, UNITS, DTK_DECADE},	/* "decade" relative */
	{"decades", UNITS, DTK_DECADE}, /* "decades" relative */
	{"decs", UNITS, DTK_DECADE},	/* "decades" relative */
	{"h", UNITS, DTK_HOUR},		/* "hour" relative */
	{DHOUR, UNITS, DTK_HOUR},	/* "hour" relative */
	{"hours", UNITS, DTK_HOUR}, /* "hours" relative */
	{"hr", UNITS, DTK_HOUR},	/* "hour" relative */
	{"hrs", UNITS, DTK_HOUR},	/* "hours" relative */
	{"m", UNITS, DTK_MINUTE},	/* "minute" relative */
	{"microsecon", UNITS, DTK_MICROSEC},	/* "microsecond" relative */
	{"mil", UNITS, DTK_MILLENNIUM}, /* "millennium" relative */
	{"millennia", UNITS, DTK_MILLENNIUM},	/* "millennia" relative */
	{DMILLENNIUM, UNITS, DTK_MILLENNIUM},	/* "millennium" relative */
	{"millisecon", UNITS, DTK_MILLISEC},	/* relative */
	{"mils", UNITS, DTK_MILLENNIUM},	/* "millennia" relative */
	{"min", UNITS, DTK_MINUTE}, /* "minute" relative */
	{"mins", UNITS, DTK_MINUTE},	/* "minutes" relative */
	{DMINUTE, UNITS, DTK_MINUTE},	/* "minute" relative */
	{"minutes", UNITS, DTK_MINUTE}, /* "minutes" relative */
	{"mon", UNITS, DTK_MONTH},	/* "months" relative */
	{"mons", UNITS, DTK_MONTH}, /* "months" relative */
	{DMONTH, UNITS, DTK_MONTH}, /* "month" relative */
	{"months", UNITS, DTK_MONTH},
	{"ms", UNITS, DTK_MILLISEC},
	{"msec", UNITS, DTK_MILLISEC},
	{DMILLISEC, UNITS, DTK_MILLISEC},
	{"mseconds", UNITS, DTK_MILLISEC},
	{"msecs", UNITS, DTK_MILLISEC},
	{"qtr", UNITS, DTK_QUARTER},	/* "quarter" relative */
	{DQUARTER, UNITS, DTK_QUARTER}, /* "quarter" relative */
	{"s", UNITS, DTK_SECOND},
	{"sec", UNITS, DTK_SECOND},
	{DSECOND, UNITS, DTK_SECOND},
	{"seconds", UNITS, DTK_SECOND},
	{"secs", UNITS, DTK_SECOND},
	{DTIMEZONE, UNITS, DTK_TZ}, /* "timezone" time offset */
	{"timezone_h", UNITS, DTK_TZ_HOUR}, /* timezone hour units */
	{"timezone_m", UNITS, DTK_TZ_MINUTE},	/* timezone minutes units */
	{"us", UNITS, DTK_MICROSEC},	/* "microsecond" relative */
	{"usec", UNITS, DTK_MICROSEC},	/* "microsecond" relative */
	{DMICROSEC, UNITS, DTK_MICROSEC},	/* "microsecond" relative */
	{"useconds", UNITS, DTK_MICROSEC},	/* "microseconds" relative */
	{"usecs", UNITS, DTK_MICROSEC}, /* "microseconds" relative */
	{"w", UNITS, DTK_WEEK},		/* "week" relative */
	{DWEEK, UNITS, DTK_WEEK},	/* "week" relative */
	{"weeks", UNITS, DTK_WEEK}, /* "weeks" relative */
	{"y", UNITS, DTK_YEAR},		/* "year" relative */
	{DYEAR, UNITS, DTK_YEAR},	/* "year" relative */
	{"years", UNITS, DTK_YEAR}, /* "years" relative */
	{"yr", UNITS, DTK_YEAR},	/* "year" relative */
	{"yrs", UNITS, DTK_YEAR}	/* "years" relative */
};

static const int szdeltatktbl = sizeof deltatktbl / sizeof deltatktbl[0];

static TimeZoneAbbrevTable *zoneabbrevtbl = NULL;

/* Caches of recent lookup results in the above tables */

static const datetkn *datecache[MAXDATEFIELDS] = {NULL};

static const datetkn *deltacache[MAXDATEFIELDS] = {NULL};

static const datetkn *abbrevcache[MAXDATEFIELDS] = {NULL};


/*
 * Calendar time to Julian date conversions.
 * Julian date is commonly used in astronomical applications,
 *	since it is numerically accurate and computationally simple.
 * The algorithms here will accurately convert between Julian day
 *	and calendar date for all non-negative Julian days
 *	(i.e. from Nov 24, -4713 on).
 *
 * Rewritten to eliminate overflow problems. This now allows the
 * routines to work correctly for all Julian day counts from
 * 0 to 2147483647	(Nov 24, -4713 to Jun 3, 5874898) assuming
 * a 32-bit integer. Longer types should also work to the limits
 * of their precision.
 *
 * Actually, date2j() will work sanely, in the sense of producing
 * valid negative Julian dates, significantly before Nov 24, -4713.
 * We rely on it to do so back to Nov 1, -4713; see IS_VALID_JULIAN()
 * and associated commentary in timestamp.h.
 */

int
date2j(int y, int m, int d)
{
	int			julian;
	int			century;

	/*
	 * The default date values are determined as follows:
	 *	the year is the current year, as returned by SYSDATE.
	 *	the month is the current month, as returned by SYSDATE.
	 *	the day is 01 (the first day of the month).
	 *	the hour, minute, and second are all 0.
	 *
	 *	 Begin - ReqID:SRS-GKC-DATATYPE_001
	 */
	if ((y == 0 || m == 0) && ORA_MODE == compatible_mode)
	{
		TimestampTz current_date;
		int			tz;
		struct pg_tm tt,
				   *tm = &tt;
		fsec_t		fsec;
		const char *tzn;

		current_date = GetCurrentTimestamp();
		if (timestamp2tm(current_date, &tz, tm, &fsec, &tzn, NULL) == 0)
		{
			if (y == 0)
			{
				y = tm->tm_year;
			}

			if (m == 0)
			{
				m = tm->tm_mon;
			}

			if (d > day_tab[isleap(y)][m - 1])
				ereport(ERROR,
						(errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
					  errmsg("The value of day must be between 1 and the last day of the month.")));

		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
		}
	}
	/* End - ReqID:SRS-GKC-DATATYPE_001 */

	if (m > 2)
	{
		m += 1;
		y += 4800;
	}
	else
	{
		m += 13;
		y += 4799;
	}

	century = y / 100;
	julian = y * 365 - 32167;
	julian += y / 4 - century + century / 4;
	julian += 7834 * m / 256 + d;

	return julian;
}								/* date2j() */

void
j2date(int jd, int *year, int *month, int *day)
{
	unsigned int julian;
	unsigned int quad;
	unsigned int extra;
	int			y;

	julian = jd;
	julian += 32044;
	quad = julian / 146097;
	extra = (julian - quad * 146097) * 4 + 3;
	julian += 60 + quad * 3 + extra / 146097;
	quad = julian / 1461;
	julian -= quad * 1461;
	y = julian * 4 / 1461;
	julian = ((y != 0) ? ((julian + 305) % 365) : ((julian + 306) % 366))
		+ 123;
	y += quad * 4;
	*year = y - 4800;
	quad = julian * 2141 / 65536;
	*day = julian - 7834 * quad / 256;
	*month = (quad + 10) % MONTHS_PER_YEAR + 1;
}								/* j2date() */


/*
 * j2day - convert Julian date to day-of-week (0..6 == Sun..Sat)
 *
 * Note: various places use the locution j2day(date - 1) to produce a
 * result according to the convention 0..6 = Mon..Sun.  This is a bit of
 * a crock, but will work as long as the computation here is just a modulo.
 */
int
j2day(int date)
{
	date += 1;
	date %= 7;
	/* Cope if division truncates towards zero, as it probably does */
	if (date < 0)
		date += 7;

	return date;
}								/* j2day() */


/*
 * GetCurrentDateTime()
 *
 * Get the transaction start time ("now()") broken down as a struct pg_tm,
 * converted according to the session timezone setting.
 *
 * This is just a convenience wrapper for GetCurrentTimeUsec, to cover the
 * case where caller doesn't need either fractional seconds or tz offset.
 */
void
GetCurrentDateTime(struct pg_tm *tm)
{
	fsec_t		fsec;

	GetCurrentTimeUsec(tm, &fsec, NULL);
}

/*
 * GetCurrentTimeUsec()
 *
 * Get the transaction start time ("now()") broken down as a struct pg_tm,
 * including fractional seconds and timezone offset.  The time is converted
 * according to the session timezone setting.
 *
 * Callers may pass tzp = NULL if they don't need the offset, but this does
 * not affect the conversion behavior (unlike timestamp2tm()).
 *
 * Internally, we cache the result, since this could be called many times
 * in a transaction, within which now() doesn't change.
 */
void
GetCurrentTimeUsec(struct pg_tm *tm, fsec_t *fsec, int *tzp)
{
	TimestampTz cur_ts = GetCurrentTransactionStartTimestamp();

	/*
	 * The cache key must include both current time and current timezone.  By
	 * representing the timezone by just a pointer, we're assuming that
	 * distinct timezone settings could never have the same pointer value.
	 * This is true by virtue of the hashtable used inside pg_tzset();
	 * however, it might need another look if we ever allow entries in that
	 * hash to be recycled.
	 */
	static TimestampTz cache_ts = 0;
	static pg_tz *cache_timezone = NULL;
	static struct pg_tm cache_tm;
	static fsec_t cache_fsec;
	static int	cache_tz;

	if (cur_ts != cache_ts || session_timezone != cache_timezone)
	{
		/*
		 * Make sure cache is marked invalid in case of error after partial
		 * update within timestamp2tm.
		 */
		cache_timezone = NULL;

		/*
		 * Perform the computation, storing results into cache.  We do not
		 * really expect any error here, since current time surely ought to be
		 * within range, but check just for sanity's sake.
		 */
		if (timestamp2tm(cur_ts, &cache_tz, &cache_tm, &cache_fsec,
						 NULL, session_timezone) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));

		/* OK, so mark the cache valid. */
		cache_ts = cur_ts;
		cache_timezone = session_timezone;
	}

	*tm = cache_tm;
	*fsec = cache_fsec;
	if (tzp != NULL)
		*tzp = cache_tz;
}


/*
 * Append seconds and fractional seconds (if any) at *cp.
 *
 * precision is the max number of fraction digits, fillzeros says to
 * pad to two integral-seconds digits.
 *
 * Returns a pointer to the new end of string.  No NUL terminator is put
 * there; callers are responsible for NUL terminating str themselves.
 *
 * Note that any sign is stripped from the input seconds values.
 */
static char *
AppendSeconds(char *cp, int sec, fsec_t fsec, int precision, bool fillzeros)
{
	Assert(precision >= 0);

	if (fillzeros)
		cp = pg_ultostr_zeropad(cp, Abs(sec), 2);
	else
		cp = pg_ultostr(cp, Abs(sec));

	/* fsec_t is just an int32 */
	if (fsec != 0)
	{
		int32		value = Abs(fsec);
		char	   *end = &cp[precision + 1];
		bool		gotnonzero = false;

		*cp++ = '.';

		/*
		 * Append the fractional seconds part.  Note that we don't want any
		 * trailing zeros here, so since we're building the number in reverse
		 * we'll skip appending zeros until we've output a non-zero digit.
		 */
		while (precision--)
		{
			int32		oldval = value;
			int32		remainder;

			value /= 10;
			remainder = oldval - value * 10;

			/* check if we got a non-zero */
			if (remainder)
				gotnonzero = true;

			if (gotnonzero)
				cp[precision] = '0' + remainder;
			else
				end = &cp[precision];
		}

		/*
		 * If we still have a non-zero value then precision must have not been
		 * enough to print the number.  We punt the problem to pg_ltostr(),
		 * which will generate a correct answer in the minimum valid width.
		 */
		if (value)
			return pg_ultostr(cp, Abs(fsec));

		return end;
	}
	else
		return cp;
}


/*
 * Variant of above that's specialized to timestamp case.
 *
 * Returns a pointer to the new end of string.  No NUL terminator is put
 * there; callers are responsible for NUL terminating str themselves.
 */
static char *
AppendTimestampSeconds(char *cp, struct pg_tm *tm, fsec_t fsec)
{
	return AppendSeconds(cp, tm->tm_sec, fsec, MAX_TIMESTAMP_PRECISION, true);
}

/*
 * Multiply frac by scale (to produce seconds) and add to *tm & *fsec.
 * We assume the input frac is less than 1 so overflow is not an issue.
 */
static void
AdjustFractSeconds(double frac, struct pg_tm *tm, fsec_t *fsec, int scale)
{
	int			sec;

	if (frac == 0)
		return;
	frac *= scale;
	sec = (int) frac;
	tm->tm_sec += sec;
	frac -= sec;
	*fsec += rint(frac * 1000000);
}

/* As above, but initial scale produces days */
static void
AdjustFractDays(double frac, struct pg_tm *tm, fsec_t *fsec, int scale)
{
	int			extra_days;

	if (frac == 0)
		return;
	frac *= scale;
	extra_days = (int) frac;
	tm->tm_mday += extra_days;
	frac -= extra_days;
	AdjustFractSeconds(frac, tm, fsec, SECS_PER_DAY);
}

/* Fetch a fractional-second value with suitable error checking */
static int
ParseFractionalSecond(char *cp, fsec_t *fsec)
{
	double		frac;

	/* Caller should always pass the start of the fraction part */
	Assert(*cp == '.');
	errno = 0;
	frac = strtod(cp, &cp);
	/* check for parse failure */
	if (*cp != '\0' || errno != 0)
		return DTERR_BAD_FORMAT;
	*fsec = rint(frac * 1000000);
	return 0;
}


/* ParseDateTime()
 *	Break string into tokens based on a date/time context.
 *	Returns 0 if successful, DTERR code if bogus input detected.
 *
 * timestr - the input string
 * workbuf - workspace for field string storage. This must be
 *	 larger than the largest legal input for this datetime type --
 *	 some additional space will be needed to NUL terminate fields.
 * buflen - the size of workbuf
 * field[] - pointers to field strings are returned in this array
 * ftype[] - field type indicators are returned in this array
 * maxfields - dimensions of the above two arrays
 * *numfields - set to the actual number of fields detected
 *
 * The fields extracted from the input are stored as separate,
 * null-terminated strings in the workspace at workbuf. Any text is
 * converted to lower case.
 *
 * Several field types are assigned:
 *	DTK_NUMBER - digits and (possibly) a decimal point
 *	DTK_DATE - digits and two delimiters, or digits and text
 *	DTK_TIME - digits, colon delimiters, and possibly a decimal point
 *	DTK_STRING - text (no digits or punctuation)
 *	DTK_SPECIAL - leading "+" or "-" followed by text
 *	DTK_TZ - leading "+" or "-" followed by digits (also eats ':', '.', '-')
 *
 * Note that some field types can hold unexpected items:
 *	DTK_NUMBER can hold date fields (yy.ddd)
 *	DTK_STRING can hold months (January) and time zones (PST)
 *	DTK_DATE can hold time zone names (America/New_York, GMT-8)
 */
int
ParseDateTime(const char *timestr, char *workbuf, size_t buflen,
			  char **field, int *ftype, int maxfields, int *numfields)
{
	int			nf = 0;
	const char *cp = timestr;
	char	   *bufp = workbuf;
	const char *bufend = workbuf + buflen;

	/*
	 * Set the character pointed-to by "bufptr" to "newchar", and increment
	 * "bufptr". "end" gives the end of the buffer -- we return an error if
	 * there is no space left to append a character to the buffer. Note that
	 * "bufptr" is evaluated twice.
	 */
#define APPEND_CHAR(bufptr, end, newchar)		\
	do											\
	{											\
		if (((bufptr) + 1) >= (end))			\
			return DTERR_BAD_FORMAT;			\
		*(bufptr)++ = newchar;					\
	} while (0)

	/* outer loop through fields */
	while (*cp != '\0')
	{
		/* Ignore spaces between fields */
		if (isspace((unsigned char) *cp))
		{
			cp++;
			continue;
		}

		/* Record start of current field */
		if (nf >= maxfields)
			return DTERR_BAD_FORMAT;
		field[nf] = bufp;

		/* leading digit? then date or time */
		if (isdigit((unsigned char) *cp))
		{
			APPEND_CHAR(bufp, bufend, *cp++);
			while (isdigit((unsigned char) *cp))
				APPEND_CHAR(bufp, bufend, *cp++);

			/* time field? */
			if (*cp == ':')
			{
				ftype[nf] = DTK_TIME;
				APPEND_CHAR(bufp, bufend, *cp++);
				while (isdigit((unsigned char) *cp) ||
					   (*cp == ':') || (*cp == '.'))
					APPEND_CHAR(bufp, bufend, *cp++);
			}
			/* date field? allow embedded text month */
			else if (*cp == '-' || *cp == '/' || *cp == '.')
			{
				/* save delimiting character to use later */
				char		delim = *cp;

				APPEND_CHAR(bufp, bufend, *cp++);
				/* second field is all digits? then no embedded text month */
				if (isdigit((unsigned char) *cp))
				{
					ftype[nf] = ((delim == '.') ? DTK_NUMBER : DTK_DATE);
					while (isdigit((unsigned char) *cp))
						APPEND_CHAR(bufp, bufend, *cp++);

					/*
					 * insist that the delimiters match to get a three-field
					 * date.
					 */
					if (*cp == delim)
					{
						ftype[nf] = DTK_DATE;
						APPEND_CHAR(bufp, bufend, *cp++);
						while (isdigit((unsigned char) *cp) || *cp == delim)
							APPEND_CHAR(bufp, bufend, *cp++);
					}
				}
				else
				{
					ftype[nf] = DTK_DATE;
					while (isalnum((unsigned char) *cp) || *cp == delim)
						APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));
				}
			}

			/*
			 * otherwise, number only and will determine year, month, day, or
			 * concatenated fields later...
			 */
			else
				ftype[nf] = DTK_NUMBER;
		}
		/* Leading decimal point? Then fractional seconds... */
		else if (*cp == '.')
		{
			APPEND_CHAR(bufp, bufend, *cp++);
			while (isdigit((unsigned char) *cp))
				APPEND_CHAR(bufp, bufend, *cp++);

			ftype[nf] = DTK_NUMBER;
		}

		/*
		 * text? then date string, month, day of week, special, or timezone
		 */
		else if (isalpha((unsigned char) *cp))
		{
			bool		is_date;

			ftype[nf] = DTK_STRING;
			APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));
			while (isalpha((unsigned char) *cp))
				APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));

			/*
			 * Dates can have embedded '-', '/', or '.' separators.  It could
			 * also be a timezone name containing embedded '/', '+', '-', '_',
			 * or ':' (but '_' or ':' can't be the first punctuation). If the
			 * next character is a digit or '+', we need to check whether what
			 * we have so far is a recognized non-timezone keyword --- if so,
			 * don't believe that this is the start of a timezone.
			 */
			is_date = false;
			if (*cp == '-' || *cp == '/' || *cp == '.')
				is_date = true;
			else if (*cp == '+' || isdigit((unsigned char) *cp))
			{
				*bufp = '\0';	/* null-terminate current field value */
				/* we need search only the core token table, not TZ names */
				if (datebsearch(field[nf], datetktbl, szdatetktbl) == NULL)
					is_date = true;
			}
			if (is_date)
			{
				ftype[nf] = DTK_DATE;
				do
				{
					APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));
				} while (*cp == '+' || *cp == '-' ||
						 *cp == '/' || *cp == '_' ||
						 *cp == '.' || *cp == ':' ||
						 isalnum((unsigned char) *cp));
			}
		}
		/* sign? then special or numeric timezone */
		else if (*cp == '+' || *cp == '-')
		{
			APPEND_CHAR(bufp, bufend, *cp++);
			/* soak up leading whitespace */
			while (isspace((unsigned char) *cp))
				cp++;
			/* numeric timezone? */
			/* note that "DTK_TZ" could also be a signed float or yyyy-mm */
			if (isdigit((unsigned char) *cp))
			{
				ftype[nf] = DTK_TZ;
				APPEND_CHAR(bufp, bufend, *cp++);
				while (isdigit((unsigned char) *cp) ||
					   *cp == ':' || *cp == '.' || *cp == '-')
					APPEND_CHAR(bufp, bufend, *cp++);
			}
			/* special? */
			else if (isalpha((unsigned char) *cp))
			{
				ftype[nf] = DTK_SPECIAL;
				APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));
				while (isalpha((unsigned char) *cp))
					APPEND_CHAR(bufp, bufend, pg_tolower((unsigned char) *cp++));
			}
			/* otherwise something wrong... */
			else
				return DTERR_BAD_FORMAT;
		}
		/* ignore other punctuation but use as delimiter */
		else if (ispunct((unsigned char) *cp))
		{
			cp++;
			continue;
		}
		/* otherwise, something is not right... */
		else
			return DTERR_BAD_FORMAT;

		/* force in a delimiter after each field */
		*bufp++ = '\0';
		nf++;
	}

	*numfields = nf;

	return 0;
}


/* DecodeDateTime()
 * Interpret previously parsed fields for general date and time.
 * Return 0 if full date, 1 if only time, and negative DTERR code if problems.
 * (Currently, all callers treat 1 as an error return too.)
 *
 *		External format(s):
 *				"<weekday> <month>-<day>-<year> <hour>:<minute>:<second>"
 *				"Fri Feb-7-1997 15:23:27"
 *				"Feb-7-1997 15:23:27"
 *				"2-7-1997 15:23:27"
 *				"1997-2-7 15:23:27"
 *				"1997.038 15:23:27"		(day of year 1-366)
 *		Also supports input in compact time:
 *				"970207 152327"
 *				"97038 152327"
 *				"20011225T040506.789-07"
 *
 * Use the system-provided functions to get the current time zone
 * if not specified in the input string.
 *
 * If the date is outside the range of pg_time_t (in practice that could only
 * happen if pg_time_t is just 32 bits), then assume UTC time zone - thomas
 * 1997-05-27
 */
int
DecodeDateTime(char **field, int *ftype, int nf,
			   int *dtype, struct pg_tm *tm, fsec_t *fsec, int *tzp)
{
	int			fmask = 0,
				tmask,
				type;
	int			ptype = 0;		/* "prefix type" for ISO y2001m02d04 format */
	int			i;
	int			val;
	int			dterr;
	int			mer = HR24;
	bool		haveTextMonth = false;
	bool		isjulian = false;
	bool		is2digits = false;
	bool		bc = false;
	pg_tz	   *namedTz = NULL;
	pg_tz	   *abbrevTz = NULL;
	pg_tz	   *valtz;
	char	   *abbrev = NULL;
	struct pg_tm cur_tm;

	/*
	 * We'll insist on at least all of the date fields, but initialize the
	 * remaining fields in case they are not set later...
	 */
	*dtype = DTK_DATE;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	*fsec = 0;
	/* don't know daylight savings time status apriori */
	tm->tm_isdst = -1;
	if (tzp != NULL)
		*tzp = 0;

	for (i = 0; i < nf; i++)
	{
		switch (ftype[i])
		{
			case DTK_DATE:

				/*
				 * Integral julian day with attached time zone? All other
				 * forms with JD will be separated into distinct fields, so we
				 * handle just this case here.
				 */
				if (ptype == DTK_JULIAN)
				{
					char	   *cp;
					int			val;

					if (tzp == NULL)
						return DTERR_BAD_FORMAT;

					errno = 0;
					val = strtoint(field[i], &cp, 10);
					if (errno == ERANGE || val < 0)
						return DTERR_FIELD_OVERFLOW;

					j2date(val, &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
					isjulian = true;

					/* Get the time zone from the end of the string */
					dterr = DecodeTimezone(cp, tzp);
					if (dterr)
						return dterr;

					tmask = DTK_DATE_M | DTK_TIME_M | DTK_M(TZ);
					ptype = 0;
					break;
				}

				/*
				 * Already have a date? Then this might be a time zone name
				 * with embedded punctuation (e.g. "America/New_York") or a
				 * run-together time with trailing time zone (e.g. hhmmss-zz).
				 * - thomas 2001-12-25
				 *
				 * We consider it a time zone if we already have month & day.
				 * This is to allow the form "mmm dd hhmmss tz year", which
				 * we've historically accepted.
				 */
				else if (ptype != 0 ||
						 ((fmask & (DTK_M(MONTH) | DTK_M(DAY))) ==
						  (DTK_M(MONTH) | DTK_M(DAY))))
				{
					/* No time zone accepted? Then quit... */
					if (tzp == NULL)
						return DTERR_BAD_FORMAT;

					if (isdigit((unsigned char) *field[i]) || ptype != 0)
					{
						char	   *cp;

						if (ptype != 0)
						{
							/* Sanity check; should not fail this test */
							if (ptype != DTK_TIME)
								return DTERR_BAD_FORMAT;
							ptype = 0;
						}

						/*
						 * Starts with a digit but we already have a time
						 * field? Then we are in trouble with a date and time
						 * already...
						 */
						if ((fmask & DTK_TIME_M) == DTK_TIME_M)
							return DTERR_BAD_FORMAT;

						if ((cp = strchr(field[i], '-')) == NULL)
							return DTERR_BAD_FORMAT;

						/* Get the time zone from the end of the string */
						dterr = DecodeTimezone(cp, tzp);
						if (dterr)
							return dterr;
						*cp = '\0';

						/*
						 * Then read the rest of the field as a concatenated
						 * time
						 */
						dterr = DecodeNumberField(strlen(field[i]), field[i],
												  fmask,
												  &tmask, tm,
												  fsec, &is2digits);
						if (dterr < 0)
							return dterr;

						/*
						 * modify tmask after returning from
						 * DecodeNumberField()
						 */
						tmask |= DTK_M(TZ);
					}
					else
					{
						namedTz = pg_tzset(field[i]);
						if (!namedTz)
						{
							/*
							 * We should return an error code instead of
							 * ereport'ing directly, but then there is no way
							 * to report the bad time zone name.
							 */
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("time zone \"%s\" not recognized",
											field[i])));
						}
						/* we'll apply the zone setting below */
						tmask = DTK_M(TZ);
					}
				}
				else
				{
					dterr = DecodeDate(field[i], fmask,
									   &tmask, &is2digits, tm);
					if (dterr)
						return dterr;
				}
				break;

			case DTK_TIME:

				/*
				 * This might be an ISO time following a "t" field.
				 */
				if (ptype != 0)
				{
					/* Sanity check; should not fail this test */
					if (ptype != DTK_TIME)
						return DTERR_BAD_FORMAT;
					ptype = 0;
				}
				dterr = DecodeTime(field[i], fmask, INTERVAL_FULL_RANGE,
								   &tmask, tm, fsec);
				if (dterr)
					return dterr;

				/* check for time overflow */
				if (time_overflows(tm->tm_hour, tm->tm_min, tm->tm_sec,
								   *fsec))
					return DTERR_FIELD_OVERFLOW;
				break;

			case DTK_TZ:
				{
					int			tz;

					if (tzp == NULL)
						return DTERR_BAD_FORMAT;

					dterr = DecodeTimezone(field[i], &tz);
					if (dterr)
						return dterr;
					*tzp = tz;
					tmask = DTK_M(TZ);
				}
				break;

			case DTK_NUMBER:

				/*
				 * Was this an "ISO date" with embedded field labels? An
				 * example is "y2001m02d04" - thomas 2001-02-04
				 */
				if (ptype != 0)
				{
					char	   *cp;
					int			val;

					errno = 0;
					val = strtoint(field[i], &cp, 10);
					if (errno == ERANGE)
						return DTERR_FIELD_OVERFLOW;

					/*
					 * only a few kinds are allowed to have an embedded
					 * decimal
					 */
					if (*cp == '.')
						switch (ptype)
						{
							case DTK_JULIAN:
							case DTK_TIME:
							case DTK_SECOND:
								break;
							default:
								return DTERR_BAD_FORMAT;
								break;
						}
					else if (*cp != '\0')
						return DTERR_BAD_FORMAT;

					switch (ptype)
					{
						case DTK_YEAR:
							tm->tm_year = val;
							tmask = DTK_M(YEAR);
							break;

						case DTK_MONTH:

							/*
							 * already have a month and hour? then assume
							 * minutes
							 */
							if ((fmask & DTK_M(MONTH)) != 0 &&
								(fmask & DTK_M(HOUR)) != 0)
							{
								tm->tm_min = val;
								tmask = DTK_M(MINUTE);
							}
							else
							{
								tm->tm_mon = val;
								tmask = DTK_M(MONTH);
							}
							break;

						case DTK_DAY:
							tm->tm_mday = val;
							tmask = DTK_M(DAY);
							break;

						case DTK_HOUR:
							tm->tm_hour = val;
							tmask = DTK_M(HOUR);
							break;

						case DTK_MINUTE:
							tm->tm_min = val;
							tmask = DTK_M(MINUTE);
							break;

						case DTK_SECOND:
							tm->tm_sec = val;
							tmask = DTK_M(SECOND);
							if (*cp == '.')
							{
								dterr = ParseFractionalSecond(cp, fsec);
								if (dterr)
									return dterr;
								tmask = DTK_ALL_SECS_M;
							}
							break;

						case DTK_TZ:
							tmask = DTK_M(TZ);
							dterr = DecodeTimezone(field[i], tzp);
							if (dterr)
								return dterr;
							break;

						case DTK_JULIAN:
							/* previous field was a label for "julian date" */
							if (val < 0)
								return DTERR_FIELD_OVERFLOW;
							tmask = DTK_DATE_M;
							j2date(val, &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
							isjulian = true;

							/* fractional Julian Day? */
							if (*cp == '.')
							{
								double		time;

								errno = 0;
								time = strtod(cp, &cp);
								if (*cp != '\0' || errno != 0)
									return DTERR_BAD_FORMAT;
								time *= USECS_PER_DAY;
								dt2time(time,
										&tm->tm_hour, &tm->tm_min,
										&tm->tm_sec, fsec);
								tmask |= DTK_TIME_M;
							}
							break;

						case DTK_TIME:
							/* previous field was "t" for ISO time */
							dterr = DecodeNumberField(strlen(field[i]), field[i],
													  (fmask | DTK_DATE_M),
													  &tmask, tm,
													  fsec, &is2digits);
							if (dterr < 0)
								return dterr;
							if (tmask != DTK_TIME_M)
								return DTERR_BAD_FORMAT;
							break;

						default:
							return DTERR_BAD_FORMAT;
							break;
					}

					ptype = 0;
					*dtype = DTK_DATE;
				}
				else
				{
					char	   *cp;
					int			flen;

					flen = strlen(field[i]);
					cp = strchr(field[i], '.');

					/* Embedded decimal and no date yet? */
					if (cp != NULL && !(fmask & DTK_DATE_M))
					{
						dterr = DecodeDate(field[i], fmask,
										   &tmask, &is2digits, tm);
						if (dterr)
							return dterr;
					}
					/* embedded decimal and several digits before? */
					else if (cp != NULL && flen - strlen(cp) > 2)
					{
						/*
						 * Interpret as a concatenated date or time Set the
						 * type field to allow decoding other fields later.
						 * Example: 20011223 or 040506
						 */
						dterr = DecodeNumberField(flen, field[i], fmask,
												  &tmask, tm,
												  fsec, &is2digits);
						if (dterr < 0)
							return dterr;
					}

					/*
					 * Is this a YMD or HMS specification, or a year number?
					 * YMD and HMS are required to be six digits or more, so
					 * if it is 5 digits, it is a year.  If it is six or more
					 * digits, we assume it is YMD or HMS unless no date and
					 * no time values have been specified.  This forces 6+
					 * digit years to be at the end of the string, or to use
					 * the ISO date specification.
					 */
					else if (flen >= 6 && (!(fmask & DTK_DATE_M) ||
										   !(fmask & DTK_TIME_M)))
					{
						dterr = DecodeNumberField(flen, field[i], fmask,
												  &tmask, tm,
												  fsec, &is2digits);
						if (dterr < 0)
							return dterr;
					}
					/* otherwise it is a single date/time field... */
					else
					{
						dterr = DecodeNumber(flen, field[i],
											 haveTextMonth, fmask,
											 &tmask, tm,
											 fsec, &is2digits);
						if (dterr)
							return dterr;
					}
				}
				break;

			case DTK_STRING:
			case DTK_SPECIAL:
				/* timezone abbrevs take precedence over built-in tokens */
				type = DecodeTimezoneAbbrev(i, field[i], &val, &valtz);
				if (type == UNKNOWN_FIELD)
					type = DecodeSpecial(i, field[i], &val);
				if (type == IGNORE_DTF)
					continue;

				tmask = DTK_M(type);
				switch (type)
				{
					case RESERV:
						switch (val)
						{
							case DTK_NOW:
								tmask = (DTK_DATE_M | DTK_TIME_M | DTK_M(TZ));
								*dtype = DTK_DATE;
								GetCurrentTimeUsec(tm, fsec, tzp);
								break;

							case DTK_YESTERDAY:
								tmask = DTK_DATE_M;
								*dtype = DTK_DATE;
								GetCurrentDateTime(&cur_tm);
								j2date(date2j(cur_tm.tm_year, cur_tm.tm_mon, cur_tm.tm_mday) - 1,
									   &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
								break;

							case DTK_TODAY:
								tmask = DTK_DATE_M;
								*dtype = DTK_DATE;
								GetCurrentDateTime(&cur_tm);
								tm->tm_year = cur_tm.tm_year;
								tm->tm_mon = cur_tm.tm_mon;
								tm->tm_mday = cur_tm.tm_mday;
								break;

							case DTK_TOMORROW:
								tmask = DTK_DATE_M;
								*dtype = DTK_DATE;
								GetCurrentDateTime(&cur_tm);
								j2date(date2j(cur_tm.tm_year, cur_tm.tm_mon, cur_tm.tm_mday) + 1,
									   &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
								break;

							case DTK_ZULU:
								tmask = (DTK_TIME_M | DTK_M(TZ));
								*dtype = DTK_DATE;
								tm->tm_hour = 0;
								tm->tm_min = 0;
								tm->tm_sec = 0;
								if (tzp != NULL)
									*tzp = 0;
								break;

							default:
								*dtype = val;
						}

						break;

					case MONTH:

						/*
						 * already have a (numeric) month? then see if we can
						 * substitute...
						 */
						if ((fmask & DTK_M(MONTH)) && !haveTextMonth &&
							!(fmask & DTK_M(DAY)) && tm->tm_mon >= 1 &&
							tm->tm_mon <= 31)
						{
							tm->tm_mday = tm->tm_mon;
							tmask = DTK_M(DAY);
						}
						haveTextMonth = true;
						tm->tm_mon = val;
						break;

					case DTZMOD:

						/*
						 * daylight savings time modifier (solves "MET DST"
						 * syntax)
						 */
						tmask |= DTK_M(DTZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp -= val;
						break;

					case DTZ:

						/*
						 * set mask for TZ here _or_ check for DTZ later when
						 * getting default timezone
						 */
						tmask |= DTK_M(TZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp = -val;
						break;

					case TZ:
						tm->tm_isdst = 0;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp = -val;
						break;

					case DYNTZ:
						tmask |= DTK_M(TZ);
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						/* we'll determine the actual offset later */
						abbrevTz = valtz;
						abbrev = field[i];
						break;

					case AMPM:
						mer = val;
						break;

					case ADBC:
						bc = (val == BC);
						break;

					case DOW:
						tm->tm_wday = val;
						break;

					case UNITS:
						tmask = 0;
						ptype = val;
						break;

					case ISOTIME:

						/*
						 * This is a filler field "t" indicating that the next
						 * field is time. Try to verify that this is sensible.
						 */
						tmask = 0;

						/* No preceding date? Then quit... */
						if ((fmask & DTK_DATE_M) != DTK_DATE_M)
							return DTERR_BAD_FORMAT;

						/***
						 * We will need one of the following fields:
						 *	DTK_NUMBER should be hhmmss.fff
						 *	DTK_TIME should be hh:mm:ss.fff
						 *	DTK_DATE should be hhmmss-zz
						 ***/
						if (i >= nf - 1 ||
							(ftype[i + 1] != DTK_NUMBER &&
							 ftype[i + 1] != DTK_TIME &&
							 ftype[i + 1] != DTK_DATE))
							return DTERR_BAD_FORMAT;

						ptype = val;
						break;

					case UNKNOWN_FIELD:

						/*
						 * Before giving up and declaring error, check to see
						 * if it is an all-alpha timezone name.
						 */
						namedTz = pg_tzset(field[i]);
						if (!namedTz)
							return DTERR_BAD_FORMAT;
						/* we'll apply the zone setting below */
						tmask = DTK_M(TZ);
						break;

					default:
						return DTERR_BAD_FORMAT;
				}
				break;

			default:
				return DTERR_BAD_FORMAT;
		}

		if (tmask & fmask)
			return DTERR_BAD_FORMAT;
		fmask |= tmask;
	}							/* end loop over fields */

	/* do final checking/adjustment of Y/M/D fields */
	dterr = ValidateDate(fmask, isjulian, is2digits, bc, tm);
	if (dterr)
		return dterr;

	/* handle AM/PM */
	if (mer != HR24 && tm->tm_hour > HOURS_PER_DAY / 2)
		return DTERR_FIELD_OVERFLOW;
	if (mer == AM && tm->tm_hour == HOURS_PER_DAY / 2)
		tm->tm_hour = 0;
	else if (mer == PM && tm->tm_hour != HOURS_PER_DAY / 2)
		tm->tm_hour += HOURS_PER_DAY / 2;

	/* do additional checking for full date specs... */
	if (*dtype == DTK_DATE)
	{
		if ((fmask & DTK_DATE_M) != DTK_DATE_M)
		{
			if ((fmask & DTK_TIME_M) == DTK_TIME_M)
				return 1;
			return DTERR_BAD_FORMAT;
		}

		/*
		 * If we had a full timezone spec, compute the offset (we could not do
		 * it before, because we need the date to resolve DST status).
		 */
		if (namedTz != NULL)
		{
			/* daylight savings time modifier disallowed with full TZ */
			if (fmask & DTK_M(DTZMOD))
				return DTERR_BAD_FORMAT;

			*tzp = DetermineTimeZoneOffset(tm, namedTz);
		}

		/*
		 * Likewise, if we had a dynamic timezone abbreviation, resolve it
		 * now.
		 */
		if (abbrevTz != NULL)
		{
			/* daylight savings time modifier disallowed with dynamic TZ */
			if (fmask & DTK_M(DTZMOD))
				return DTERR_BAD_FORMAT;

			*tzp = DetermineTimeZoneAbbrevOffset(tm, abbrev, abbrevTz);
		}

		/* timezone not specified? then use session timezone */
		if (tzp != NULL && !(fmask & DTK_M(TZ)))
		{
			/*
			 * daylight savings time modifier but no standard timezone? then
			 * error
			 */
			if (fmask & DTK_M(DTZMOD))
				return DTERR_BAD_FORMAT;

			*tzp = DetermineTimeZoneOffset(tm, session_timezone);
		}
	}

	return 0;
}


/* DetermineTimeZoneOffset()
 *
 * Given a struct pg_tm in which tm_year, tm_mon, tm_mday, tm_hour, tm_min,
 * and tm_sec fields are set, and a zic-style time zone definition, determine
 * the applicable GMT offset and daylight-savings status at that time.
 * Set the struct pg_tm's tm_isdst field accordingly, and return the GMT
 * offset as the function result.
 *
 * Note: if the date is out of the range we can deal with, we return zero
 * as the GMT offset and set tm_isdst = 0.  We don't throw an error here,
 * though probably some higher-level code will.
 */
int
DetermineTimeZoneOffset(struct pg_tm *tm, pg_tz *tzp)
{
	pg_time_t	t;

	return DetermineTimeZoneOffsetInternal(tm, tzp, &t);
}


/* DetermineTimeZoneOffsetInternal()
 *
 * As above, but also return the actual UTC time imputed to the date/time
 * into *tp.
 *
 * In event of an out-of-range date, we punt by returning zero into *tp.
 * This is okay for the immediate callers but is a good reason for not
 * exposing this worker function globally.
 *
 * Note: it might seem that we should use mktime() for this, but bitter
 * experience teaches otherwise.  This code is much faster than most versions
 * of mktime(), anyway.
 */
static int
DetermineTimeZoneOffsetInternal(struct pg_tm *tm, pg_tz *tzp, pg_time_t *tp)
{
	int			date,
				sec;
	pg_time_t	day,
				mytime,
				prevtime,
				boundary,
				beforetime,
				aftertime;
	long int	before_gmtoff,
				after_gmtoff;
	int			before_isdst,
				after_isdst;
	int			res;

	/*
	 * First, generate the pg_time_t value corresponding to the given
	 * y/m/d/h/m/s taken as GMT time.  If this overflows, punt and decide the
	 * timezone is GMT.  (For a valid Julian date, integer overflow should be
	 * impossible with 64-bit pg_time_t, but let's check for safety.)
	 */
	if (!IS_VALID_JULIAN(tm->tm_year, tm->tm_mon, tm->tm_mday))
		goto overflow;
	date = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - UNIX_EPOCH_JDATE;

	day = ((pg_time_t) date) * SECS_PER_DAY;
	if (day / SECS_PER_DAY != date)
		goto overflow;
	sec = tm->tm_sec + (tm->tm_min + tm->tm_hour * MINS_PER_HOUR) * SECS_PER_MINUTE;
	mytime = day + sec;
	/* since sec >= 0, overflow could only be from +day to -mytime */
	if (mytime < 0 && day > 0)
		goto overflow;

	/*
	 * Find the DST time boundary just before or following the target time. We
	 * assume that all zones have GMT offsets less than 24 hours, and that DST
	 * boundaries can't be closer together than 48 hours, so backing up 24
	 * hours and finding the "next" boundary will work.
	 */
	prevtime = mytime - SECS_PER_DAY;
	if (mytime < 0 && prevtime > 0)
		goto overflow;

	res = pg_next_dst_boundary(&prevtime,
							   &before_gmtoff, &before_isdst,
							   &boundary,
							   &after_gmtoff, &after_isdst,
							   tzp);
	if (res < 0)
		goto overflow;			/* failure? */

	if (res == 0)
	{
		/* Non-DST zone, life is simple */
		tm->tm_isdst = before_isdst;
		*tp = mytime - before_gmtoff;
		return -(int) before_gmtoff;
	}

	/*
	 * Form the candidate pg_time_t values with local-time adjustment
	 */
	beforetime = mytime - before_gmtoff;
	if ((before_gmtoff > 0 &&
		 mytime < 0 && beforetime > 0) ||
		(before_gmtoff <= 0 &&
		 mytime > 0 && beforetime < 0))
		goto overflow;
	aftertime = mytime - after_gmtoff;
	if ((after_gmtoff > 0 &&
		 mytime < 0 && aftertime > 0) ||
		(after_gmtoff <= 0 &&
		 mytime > 0 && aftertime < 0))
		goto overflow;

	/*
	 * If both before or both after the boundary time, we know what to do. The
	 * boundary time itself is considered to be after the transition, which
	 * means we can accept aftertime == boundary in the second case.
	 */
	if (beforetime < boundary && aftertime < boundary)
	{
		tm->tm_isdst = before_isdst;
		*tp = beforetime;
		return -(int) before_gmtoff;
	}
	if (beforetime > boundary && aftertime >= boundary)
	{
		tm->tm_isdst = after_isdst;
		*tp = aftertime;
		return -(int) after_gmtoff;
	}

	/*
	 * It's an invalid or ambiguous time due to timezone transition.  In a
	 * spring-forward transition, prefer the "before" interpretation; in a
	 * fall-back transition, prefer "after".  (We used to define and implement
	 * this test as "prefer the standard-time interpretation", but that rule
	 * does not help to resolve the behavior when both times are reported as
	 * standard time; which does happen, eg Europe/Moscow in Oct 2014.  Also,
	 * in some zones such as Europe/Dublin, there is widespread confusion
	 * about which time offset is "standard" time, so it's fortunate that our
	 * behavior doesn't depend on that.)
	 */
	if (beforetime > aftertime)
	{
		tm->tm_isdst = before_isdst;
		*tp = beforetime;
		return -(int) before_gmtoff;
	}
	tm->tm_isdst = after_isdst;
	*tp = aftertime;
	return -(int) after_gmtoff;

overflow:
	/* Given date is out of range, so assume UTC */
	tm->tm_isdst = 0;
	*tp = 0;
	return 0;
}


/* DetermineTimeZoneAbbrevOffset()
 *
 * Determine the GMT offset and DST flag to be attributed to a dynamic
 * time zone abbreviation, that is one whose meaning has changed over time.
 * *tm contains the local time at which the meaning should be determined,
 * and tm->tm_isdst receives the DST flag.
 *
 * This differs from the behavior of DetermineTimeZoneOffset() in that a
 * standard-time or daylight-time abbreviation forces use of the corresponding
 * GMT offset even when the zone was then in DS or standard time respectively.
 * (However, that happens only if we can match the given abbreviation to some
 * abbreviation that appears in the IANA timezone data.  Otherwise, we fall
 * back to doing DetermineTimeZoneOffset().)
 */
int
DetermineTimeZoneAbbrevOffset(struct pg_tm *tm, const char *abbr, pg_tz *tzp)
{
	pg_time_t	t;
	int			zone_offset;
	int			abbr_offset;
	int			abbr_isdst;

	/*
	 * Compute the UTC time we want to probe at.  (In event of overflow, we'll
	 * probe at the epoch, which is a bit random but probably doesn't matter.)
	 */
	zone_offset = DetermineTimeZoneOffsetInternal(tm, tzp, &t);

	/*
	 * Try to match the abbreviation to something in the zone definition.
	 */
	if (DetermineTimeZoneAbbrevOffsetInternal(t, abbr, tzp,
											  &abbr_offset, &abbr_isdst))
	{
		/* Success, so use the abbrev-specific answers. */
		tm->tm_isdst = abbr_isdst;
		return abbr_offset;
	}

	/*
	 * No match, so use the answers we already got from
	 * DetermineTimeZoneOffsetInternal.
	 */
	return zone_offset;
}


/* DetermineTimeZoneAbbrevOffsetTS()
 *
 * As above but the probe time is specified as a TimestampTz (hence, UTC time),
 * and DST status is returned into *isdst rather than into tm->tm_isdst.
 */
int
DetermineTimeZoneAbbrevOffsetTS(TimestampTz ts, const char *abbr,
								pg_tz *tzp, int *isdst)
{
	pg_time_t	t = timestamptz_to_time_t(ts);
	int			zone_offset;
	int			abbr_offset;
	int			tz;
	struct pg_tm tm;
	fsec_t		fsec;

	/*
	 * If the abbrev matches anything in the zone data, this is pretty easy.
	 */
	if (DetermineTimeZoneAbbrevOffsetInternal(t, abbr, tzp,
											  &abbr_offset, isdst))
		return abbr_offset;

	/*
	 * Else, break down the timestamp so we can use DetermineTimeZoneOffset.
	 */
	if (timestamp2tm(ts, &tz, &tm, &fsec, NULL, tzp) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));

	zone_offset = DetermineTimeZoneOffset(&tm, tzp);
	*isdst = tm.tm_isdst;
	return zone_offset;
}


/* DetermineTimeZoneAbbrevOffsetInternal()
 *
 * Workhorse for above two functions: work from a pg_time_t probe instant.
 * On success, return GMT offset and DST status into *offset and *isdst.
 */
static bool
DetermineTimeZoneAbbrevOffsetInternal(pg_time_t t, const char *abbr, pg_tz *tzp,
									  int *offset, int *isdst)
{
	char		upabbr[TZ_STRLEN_MAX + 1];
	unsigned char *p;
	long int	gmtoff;

	/* We need to force the abbrev to upper case */
	strlcpy(upabbr, abbr, sizeof(upabbr));
	for (p = (unsigned char *) upabbr; *p; p++)
		*p = pg_toupper(*p);

	/* Look up the abbrev's meaning at this time in this zone */
	if (pg_interpret_timezone_abbrev(upabbr,
									 &t,
									 &gmtoff,
									 isdst,
									 tzp))
	{
		/* Change sign to agree with DetermineTimeZoneOffset() */
		*offset = (int) -gmtoff;
		return true;
	}
	return false;
}


/* DecodeTimeOnly()
 * Interpret parsed string as time fields only.
 * Returns 0 if successful, DTERR code if bogus input detected.
 *
 * Note that support for time zone is here for
 * SQL TIME WITH TIME ZONE, but it reveals
 * bogosity with SQL date/time standards, since
 * we must infer a time zone from current time.
 * - thomas 2000-03-10
 * Allow specifying date to get a better time zone,
 * if time zones are allowed. - thomas 2001-12-26
 */
int
DecodeTimeOnly(char **field, int *ftype, int nf,
			   int *dtype, struct pg_tm *tm, fsec_t *fsec, int *tzp)
{
	int			fmask = 0,
				tmask,
				type;
	int			ptype = 0;		/* "prefix type" for ISO h04mm05s06 format */
	int			i;
	int			val;
	int			dterr;
	bool		isjulian = false;
	bool		is2digits = false;
	bool		bc = false;
	int			mer = HR24;
	pg_tz	   *namedTz = NULL;
	pg_tz	   *abbrevTz = NULL;
	char	   *abbrev = NULL;
	pg_tz	   *valtz;

	*dtype = DTK_TIME;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	*fsec = 0;
	/* don't know daylight savings time status apriori */
	tm->tm_isdst = -1;

	if (tzp != NULL)
		*tzp = 0;

	for (i = 0; i < nf; i++)
	{
		switch (ftype[i])
		{
			case DTK_DATE:

				/*
				 * Time zone not allowed? Then should not accept dates or time
				 * zones no matter what else!
				 */
				if (tzp == NULL)
					return DTERR_BAD_FORMAT;

				/* Under limited circumstances, we will accept a date... */
				if (i == 0 && nf >= 2 &&
					(ftype[nf - 1] == DTK_DATE || ftype[1] == DTK_TIME))
				{
					dterr = DecodeDate(field[i], fmask,
									   &tmask, &is2digits, tm);
					if (dterr)
						return dterr;
				}
				/* otherwise, this is a time and/or time zone */
				else
				{
					if (isdigit((unsigned char) *field[i]))
					{
						char	   *cp;

						/*
						 * Starts with a digit but we already have a time
						 * field? Then we are in trouble with time already...
						 */
						if ((fmask & DTK_TIME_M) == DTK_TIME_M)
							return DTERR_BAD_FORMAT;

						/*
						 * Should not get here and fail. Sanity check only...
						 */
						if ((cp = strchr(field[i], '-')) == NULL)
							return DTERR_BAD_FORMAT;

						/* Get the time zone from the end of the string */
						dterr = DecodeTimezone(cp, tzp);
						if (dterr)
							return dterr;
						*cp = '\0';

						/*
						 * Then read the rest of the field as a concatenated
						 * time
						 */
						dterr = DecodeNumberField(strlen(field[i]), field[i],
												  (fmask | DTK_DATE_M),
												  &tmask, tm,
												  fsec, &is2digits);
						if (dterr < 0)
							return dterr;
						ftype[i] = dterr;

						tmask |= DTK_M(TZ);
					}
					else
					{
						namedTz = pg_tzset(field[i]);
						if (!namedTz)
						{
							/*
							 * We should return an error code instead of
							 * ereport'ing directly, but then there is no way
							 * to report the bad time zone name.
							 */
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
									 errmsg("time zone \"%s\" not recognized",
											field[i])));
						}
						/* we'll apply the zone setting below */
						ftype[i] = DTK_TZ;
						tmask = DTK_M(TZ);
					}
				}
				break;

			case DTK_TIME:
				dterr = DecodeTime(field[i], (fmask | DTK_DATE_M),
								   INTERVAL_FULL_RANGE,
								   &tmask, tm, fsec);
				if (dterr)
					return dterr;
				break;

			case DTK_TZ:
				{
					int			tz;

					if (tzp == NULL)
						return DTERR_BAD_FORMAT;

					dterr = DecodeTimezone(field[i], &tz);
					if (dterr)
						return dterr;
					*tzp = tz;
					tmask = DTK_M(TZ);
				}
				break;

			case DTK_NUMBER:

				/*
				 * Was this an "ISO time" with embedded field labels? An
				 * example is "h04mm05s06" - thomas 2001-02-04
				 */
				if (ptype != 0)
				{
					char	   *cp;
					int			val;

					/* Only accept a date under limited circumstances */
					switch (ptype)
					{
						case DTK_JULIAN:
						case DTK_YEAR:
						case DTK_MONTH:
						case DTK_DAY:
							if (tzp == NULL)
								return DTERR_BAD_FORMAT;
						default:
							break;
					}

					errno = 0;
					val = strtoint(field[i], &cp, 10);
					if (errno == ERANGE)
						return DTERR_FIELD_OVERFLOW;

					/*
					 * only a few kinds are allowed to have an embedded
					 * decimal
					 */
					if (*cp == '.')
						switch (ptype)
						{
							case DTK_JULIAN:
							case DTK_TIME:
							case DTK_SECOND:
								break;
							default:
								return DTERR_BAD_FORMAT;
								break;
						}
					else if (*cp != '\0')
						return DTERR_BAD_FORMAT;

					switch (ptype)
					{
						case DTK_YEAR:
							tm->tm_year = val;
							tmask = DTK_M(YEAR);
							break;

						case DTK_MONTH:

							/*
							 * already have a month and hour? then assume
							 * minutes
							 */
							if ((fmask & DTK_M(MONTH)) != 0 &&
								(fmask & DTK_M(HOUR)) != 0)
							{
								tm->tm_min = val;
								tmask = DTK_M(MINUTE);
							}
							else
							{
								tm->tm_mon = val;
								tmask = DTK_M(MONTH);
							}
							break;

						case DTK_DAY:
							tm->tm_mday = val;
							tmask = DTK_M(DAY);
							break;

						case DTK_HOUR:
							tm->tm_hour = val;
							tmask = DTK_M(HOUR);
							break;

						case DTK_MINUTE:
							tm->tm_min = val;
							tmask = DTK_M(MINUTE);
							break;

						case DTK_SECOND:
							tm->tm_sec = val;
							tmask = DTK_M(SECOND);
							if (*cp == '.')
							{
								dterr = ParseFractionalSecond(cp, fsec);
								if (dterr)
									return dterr;
								tmask = DTK_ALL_SECS_M;
							}
							break;

						case DTK_TZ:
							tmask = DTK_M(TZ);
							dterr = DecodeTimezone(field[i], tzp);
							if (dterr)
								return dterr;
							break;

						case DTK_JULIAN:
							/* previous field was a label for "julian date" */
							if (val < 0)
								return DTERR_FIELD_OVERFLOW;
							tmask = DTK_DATE_M;
							j2date(val, &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
							isjulian = true;

							if (*cp == '.')
							{
								double		time;

								errno = 0;
								time = strtod(cp, &cp);
								if (*cp != '\0' || errno != 0)
									return DTERR_BAD_FORMAT;
								time *= USECS_PER_DAY;
								dt2time(time,
										&tm->tm_hour, &tm->tm_min,
										&tm->tm_sec, fsec);
								tmask |= DTK_TIME_M;
							}
							break;

						case DTK_TIME:
							/* previous field was "t" for ISO time */
							dterr = DecodeNumberField(strlen(field[i]), field[i],
													  (fmask | DTK_DATE_M),
													  &tmask, tm,
													  fsec, &is2digits);
							if (dterr < 0)
								return dterr;
							ftype[i] = dterr;

							if (tmask != DTK_TIME_M)
								return DTERR_BAD_FORMAT;
							break;

						default:
							return DTERR_BAD_FORMAT;
							break;
					}

					ptype = 0;
					*dtype = DTK_DATE;
				}
				else
				{
					char	   *cp;
					int			flen;

					flen = strlen(field[i]);
					cp = strchr(field[i], '.');

					/* Embedded decimal? */
					if (cp != NULL)
					{
						/*
						 * Under limited circumstances, we will accept a
						 * date...
						 */
						if (i == 0 && nf >= 2 && ftype[nf - 1] == DTK_DATE)
						{
							dterr = DecodeDate(field[i], fmask,
											   &tmask, &is2digits, tm);
							if (dterr)
								return dterr;
						}
						/* embedded decimal and several digits before? */
						else if (flen - strlen(cp) > 2)
						{
							/*
							 * Interpret as a concatenated date or time Set
							 * the type field to allow decoding other fields
							 * later. Example: 20011223 or 040506
							 */
							dterr = DecodeNumberField(flen, field[i],
													  (fmask | DTK_DATE_M),
													  &tmask, tm,
													  fsec, &is2digits);
							if (dterr < 0)
								return dterr;
							ftype[i] = dterr;
						}
						else
							return DTERR_BAD_FORMAT;
					}
					else if (flen > 4)
					{
						dterr = DecodeNumberField(flen, field[i],
												  (fmask | DTK_DATE_M),
												  &tmask, tm,
												  fsec, &is2digits);
						if (dterr < 0)
							return dterr;
						ftype[i] = dterr;
					}
					/* otherwise it is a single date/time field... */
					else
					{
						dterr = DecodeNumber(flen, field[i],
											 false,
											 (fmask | DTK_DATE_M),
											 &tmask, tm,
											 fsec, &is2digits);
						if (dterr)
							return dterr;
					}
				}
				break;

			case DTK_STRING:
			case DTK_SPECIAL:
				/* timezone abbrevs take precedence over built-in tokens */
				type = DecodeTimezoneAbbrev(i, field[i], &val, &valtz);
				if (type == UNKNOWN_FIELD)
					type = DecodeSpecial(i, field[i], &val);
				if (type == IGNORE_DTF)
					continue;

				tmask = DTK_M(type);
				switch (type)
				{
					case RESERV:
						switch (val)
						{
							case DTK_NOW:
								tmask = DTK_TIME_M;
								*dtype = DTK_TIME;
								GetCurrentTimeUsec(tm, fsec, NULL);
								break;

							case DTK_ZULU:
								tmask = (DTK_TIME_M | DTK_M(TZ));
								*dtype = DTK_TIME;
								tm->tm_hour = 0;
								tm->tm_min = 0;
								tm->tm_sec = 0;
								tm->tm_isdst = 0;
								break;

							default:
								return DTERR_BAD_FORMAT;
						}

						break;

					case DTZMOD:

						/*
						 * daylight savings time modifier (solves "MET DST"
						 * syntax)
						 */
						tmask |= DTK_M(DTZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp -= val;
						break;

					case DTZ:

						/*
						 * set mask for TZ here _or_ check for DTZ later when
						 * getting default timezone
						 */
						tmask |= DTK_M(TZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp = -val;
						ftype[i] = DTK_TZ;
						break;

					case TZ:
						tm->tm_isdst = 0;
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						*tzp = -val;
						ftype[i] = DTK_TZ;
						break;

					case DYNTZ:
						tmask |= DTK_M(TZ);
						if (tzp == NULL)
							return DTERR_BAD_FORMAT;
						/* we'll determine the actual offset later */
						abbrevTz = valtz;
						abbrev = field[i];
						ftype[i] = DTK_TZ;
						break;

					case AMPM:
						mer = val;
						break;

					case ADBC:
						bc = (val == BC);
						break;

					case UNITS:
						tmask = 0;
						ptype = val;
						break;

					case ISOTIME:
						tmask = 0;

						/***
						 * We will need one of the following fields:
						 *	DTK_NUMBER should be hhmmss.fff
						 *	DTK_TIME should be hh:mm:ss.fff
						 *	DTK_DATE should be hhmmss-zz
						 ***/
						if (i >= nf - 1 ||
							(ftype[i + 1] != DTK_NUMBER &&
							 ftype[i + 1] != DTK_TIME &&
							 ftype[i + 1] != DTK_DATE))
							return DTERR_BAD_FORMAT;

						ptype = val;
						break;

					case UNKNOWN_FIELD:

						/*
						 * Before giving up and declaring error, check to see
						 * if it is an all-alpha timezone name.
						 */
						namedTz = pg_tzset(field[i]);
						if (!namedTz)
							return DTERR_BAD_FORMAT;
						/* we'll apply the zone setting below */
						tmask = DTK_M(TZ);
						break;

					default:
						return DTERR_BAD_FORMAT;
				}
				break;

			default:
				return DTERR_BAD_FORMAT;
		}

		if (tmask & fmask)
			return DTERR_BAD_FORMAT;
		fmask |= tmask;
	}							/* end loop over fields */

	/* do final checking/adjustment of Y/M/D fields */
	dterr = ValidateDate(fmask, isjulian, is2digits, bc, tm);
	if (dterr)
		return dterr;

	/* handle AM/PM */
	if (mer != HR24 && tm->tm_hour > HOURS_PER_DAY / 2)
		return DTERR_FIELD_OVERFLOW;
	if (mer == AM && tm->tm_hour == HOURS_PER_DAY / 2)
		tm->tm_hour = 0;
	else if (mer == PM && tm->tm_hour != HOURS_PER_DAY / 2)
		tm->tm_hour += HOURS_PER_DAY / 2;

	/* check for time overflow */
	if (time_overflows(tm->tm_hour, tm->tm_min, tm->tm_sec, *fsec))
		return DTERR_FIELD_OVERFLOW;

	if ((fmask & DTK_TIME_M) != DTK_TIME_M)
		return DTERR_BAD_FORMAT;

	/*
	 * If we had a full timezone spec, compute the offset (we could not do it
	 * before, because we may need the date to resolve DST status).
	 */
	if (namedTz != NULL)
	{
		long int	gmtoff;

		/* daylight savings time modifier disallowed with full TZ */
		if (fmask & DTK_M(DTZMOD))
			return DTERR_BAD_FORMAT;

		/* if non-DST zone, we do not need to know the date */
		if (pg_get_timezone_offset(namedTz, &gmtoff))
		{
			*tzp = -(int) gmtoff;
		}
		else
		{
			/* a date has to be specified */
			if ((fmask & DTK_DATE_M) != DTK_DATE_M)
				return DTERR_BAD_FORMAT;
			*tzp = DetermineTimeZoneOffset(tm, namedTz);
		}
	}

	/*
	 * Likewise, if we had a dynamic timezone abbreviation, resolve it now.
	 */
	if (abbrevTz != NULL)
	{
		struct pg_tm tt,
				   *tmp = &tt;

		/*
		 * daylight savings time modifier but no standard timezone? then error
		 */
		if (fmask & DTK_M(DTZMOD))
			return DTERR_BAD_FORMAT;

		if ((fmask & DTK_DATE_M) == 0)
			GetCurrentDateTime(tmp);
		else
		{
			/* a date has to be specified */
			if ((fmask & DTK_DATE_M) != DTK_DATE_M)
				return DTERR_BAD_FORMAT;
			tmp->tm_year = tm->tm_year;
			tmp->tm_mon = tm->tm_mon;
			tmp->tm_mday = tm->tm_mday;
		}
		tmp->tm_hour = tm->tm_hour;
		tmp->tm_min = tm->tm_min;
		tmp->tm_sec = tm->tm_sec;
		*tzp = DetermineTimeZoneAbbrevOffset(tmp, abbrev, abbrevTz);
		tm->tm_isdst = tmp->tm_isdst;
	}

	/* timezone not specified? then use session timezone */
	if (tzp != NULL && !(fmask & DTK_M(TZ)))
	{
		struct pg_tm tt,
				   *tmp = &tt;

		/*
		 * daylight savings time modifier but no standard timezone? then error
		 */
		if (fmask & DTK_M(DTZMOD))
			return DTERR_BAD_FORMAT;

		if ((fmask & DTK_DATE_M) == 0)
			GetCurrentDateTime(tmp);
		else
		{
			/* a date has to be specified */
			if ((fmask & DTK_DATE_M) != DTK_DATE_M)
				return DTERR_BAD_FORMAT;
			tmp->tm_year = tm->tm_year;
			tmp->tm_mon = tm->tm_mon;
			tmp->tm_mday = tm->tm_mday;
		}
		tmp->tm_hour = tm->tm_hour;
		tmp->tm_min = tm->tm_min;
		tmp->tm_sec = tm->tm_sec;
		*tzp = DetermineTimeZoneOffset(tmp, session_timezone);
		tm->tm_isdst = tmp->tm_isdst;
	}

	return 0;
}

/* DecodeDate()
 * Decode date string which includes delimiters.
 * Return 0 if okay, a DTERR code if not.
 *
 *	str: field to be parsed
 *	fmask: bitmask for field types already seen
 *	*tmask: receives bitmask for fields found here
 *	*is2digits: set to true if we find 2-digit year
 *	*tm: field values are stored into appropriate members of this struct
 */
static int
DecodeDate(char *str, int fmask, int *tmask, bool *is2digits,
		   struct pg_tm *tm)
{
	fsec_t		fsec;
	int			nf = 0;
	int			i,
				len;
	int			dterr;
	bool		haveTextMonth = false;
	int			type,
				val,
				dmask = 0;
	char	   *field[MAXDATEFIELDS];

	*tmask = 0;

	/* parse this string... */
	while (*str != '\0' && nf < MAXDATEFIELDS)
	{
		/* skip field separators */
		while (*str != '\0' && !isalnum((unsigned char) *str))
			str++;

		if (*str == '\0')
			return DTERR_BAD_FORMAT;	/* end of string after separator */

		field[nf] = str;
		if (isdigit((unsigned char) *str))
		{
			while (isdigit((unsigned char) *str))
				str++;
		}
		else if (isalpha((unsigned char) *str))
		{
			while (isalpha((unsigned char) *str))
				str++;
		}

		/* Just get rid of any non-digit, non-alpha characters... */
		if (*str != '\0')
			*str++ = '\0';
		nf++;
	}

	/* look first for text fields, since that will be unambiguous month */
	for (i = 0; i < nf; i++)
	{
		if (isalpha((unsigned char) *field[i]))
		{
			type = DecodeSpecial(i, field[i], &val);
			if (type == IGNORE_DTF)
				continue;

			dmask = DTK_M(type);
			switch (type)
			{
				case MONTH:
					tm->tm_mon = val;
					haveTextMonth = true;
					break;

				default:
					return DTERR_BAD_FORMAT;
			}
			if (fmask & dmask)
				return DTERR_BAD_FORMAT;

			fmask |= dmask;
			*tmask |= dmask;

			/* mark this field as being completed */
			field[i] = NULL;
		}
	}

	/* now pick up remaining numeric fields */
	for (i = 0; i < nf; i++)
	{
		if (field[i] == NULL)
			continue;

		if ((len = strlen(field[i])) <= 0)
			return DTERR_BAD_FORMAT;

		dterr = DecodeNumber(len, field[i], haveTextMonth, fmask,
							 &dmask, tm,
							 &fsec, is2digits);
		if (dterr)
			return dterr;

		if (fmask & dmask)
			return DTERR_BAD_FORMAT;

		fmask |= dmask;
		*tmask |= dmask;
	}

	if ((fmask & ~(DTK_M(DOY) | DTK_M(TZ))) != DTK_DATE_M)
		return DTERR_BAD_FORMAT;

	/* validation of the field values must wait until ValidateDate() */

	return 0;
}

/* ValidateDate()
 * Check valid year/month/day values, handle BC and DOY cases
 * Return 0 if okay, a DTERR code if not.
 */
int
ValidateDate(int fmask, bool isjulian, bool is2digits, bool bc,
			 struct pg_tm *tm)
{
	if (fmask & DTK_M(YEAR))
	{
		if (isjulian)
		{
			/* tm_year is correct and should not be touched */
		}
		else if (bc)
		{
			/* there is no year zero in AD/BC notation */
			if (tm->tm_year <= 0)
				return DTERR_FIELD_OVERFLOW;
			/* internally, we represent 1 BC as year zero, 2 BC as -1, etc */
			tm->tm_year = -(tm->tm_year - 1);
		}
		else if (is2digits)
		{
			/* process 1 or 2-digit input as 1970-2069 AD, allow '0' and '00' */
			if (tm->tm_year < 0)	/* just paranoia */
				return DTERR_FIELD_OVERFLOW;
			if (tm->tm_year < 70)
				tm->tm_year += 2000;
			else if (tm->tm_year < 100)
				tm->tm_year += 1900;
		}
		else
		{
			/* there is no year zero in AD/BC notation */
			if (tm->tm_year <= 0)
				return DTERR_FIELD_OVERFLOW;
		}
	}

	/* now that we have correct year, decode DOY */
	if (fmask & DTK_M(DOY))
	{
		j2date(date2j(tm->tm_year, 1, 1) + tm->tm_yday - 1,
			   &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
	}

	/* check for valid month */
	if (fmask & DTK_M(MONTH))
	{
		if (tm->tm_mon < 1 || tm->tm_mon > MONTHS_PER_YEAR)
			return DTERR_MD_FIELD_OVERFLOW;
	}

	/* minimal check for valid day */
	if (fmask & DTK_M(DAY))
	{
		if (tm->tm_mday < 1 || tm->tm_mday > 31)
			return DTERR_MD_FIELD_OVERFLOW;
	}

	if ((fmask & DTK_DATE_M) == DTK_DATE_M)
	{
		/*
		 * Check for valid day of month, now that we know for sure the month
		 * and year.  Note we don't use MD_FIELD_OVERFLOW here, since it seems
		 * unlikely that "Feb 29" is a YMD-order error.
		 */
		if (tm->tm_mday > day_tab[isleap(tm->tm_year)][tm->tm_mon - 1])
			return DTERR_FIELD_OVERFLOW;
	}

	return 0;
}


/* DecodeTime()
 * Decode time string which includes delimiters.
 * Return 0 if okay, a DTERR code if not.
 *
 * Only check the lower limit on hours, since this same code can be
 * used to represent time spans.
 */
static int
DecodeTime(char *str, int fmask, int range,
		   int *tmask, struct pg_tm *tm, fsec_t *fsec)
{
	char	   *cp;
	int			dterr;

	*tmask = DTK_TIME_M;

	errno = 0;
	tm->tm_hour = strtoint(str, &cp, 10);
	if (errno == ERANGE)
		return DTERR_FIELD_OVERFLOW;
	if (*cp != ':')
		return DTERR_BAD_FORMAT;
	errno = 0;
	tm->tm_min = strtoint(cp + 1, &cp, 10);
	if (errno == ERANGE)
		return DTERR_FIELD_OVERFLOW;
	if (*cp == '\0')
	{
		tm->tm_sec = 0;
		*fsec = 0;
		/* If it's a MINUTE TO SECOND interval, take 2 fields as being mm:ss */
		if (range == (INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND)))
		{
			tm->tm_sec = tm->tm_min;
			tm->tm_min = tm->tm_hour;
			tm->tm_hour = 0;
		}
	}
	else if (*cp == '.')
	{
		/* always assume mm:ss.sss is MINUTE TO SECOND */
		dterr = ParseFractionalSecond(cp, fsec);
		if (dterr)
			return dterr;
		tm->tm_sec = tm->tm_min;
		tm->tm_min = tm->tm_hour;
		tm->tm_hour = 0;
	}
	else if (*cp == ':')
	{
		errno = 0;
		tm->tm_sec = strtoint(cp + 1, &cp, 10);
		if (errno == ERANGE)
			return DTERR_FIELD_OVERFLOW;
		if (*cp == '\0')
			*fsec = 0;
		else if (*cp == '.')
		{
			dterr = ParseFractionalSecond(cp, fsec);
			if (dterr)
				return dterr;
		}
		else
			return DTERR_BAD_FORMAT;
	}
	else
		return DTERR_BAD_FORMAT;

	/* do a sanity check */
	if (tm->tm_hour < 0 || tm->tm_min < 0 || tm->tm_min > MINS_PER_HOUR - 1 ||
		tm->tm_sec < 0 || tm->tm_sec > SECS_PER_MINUTE ||
		*fsec < INT64CONST(0) ||
		*fsec > USECS_PER_SEC)
		return DTERR_FIELD_OVERFLOW;

	return 0;
}


/* DecodeNumber()
 * Interpret plain numeric field as a date value in context.
 * Return 0 if okay, a DTERR code if not.
 */
static int
DecodeNumber(int flen, char *str, bool haveTextMonth, int fmask,
			 int *tmask, struct pg_tm *tm, fsec_t *fsec, bool *is2digits)
{
	int			val;
	char	   *cp;
	int			dterr;

	*tmask = 0;

	errno = 0;
	val = strtoint(str, &cp, 10);
	if (errno == ERANGE)
		return DTERR_FIELD_OVERFLOW;
	if (cp == str)
		return DTERR_BAD_FORMAT;

	if (*cp == '.')
	{
		/*
		 * More than two digits before decimal point? Then could be a date or
		 * a run-together time: 2001.360 20011225 040506.789
		 */
		if (cp - str > 2)
		{
			dterr = DecodeNumberField(flen, str,
									  (fmask | DTK_DATE_M),
									  tmask, tm,
									  fsec, is2digits);
			if (dterr < 0)
				return dterr;
			return 0;
		}

		dterr = ParseFractionalSecond(cp, fsec);
		if (dterr)
			return dterr;
	}
	else if (*cp != '\0')
		return DTERR_BAD_FORMAT;

	/* Special case for day of year */
	if (flen == 3 && (fmask & DTK_DATE_M) == DTK_M(YEAR) && val >= 1 &&
		val <= 366)
	{
		*tmask = (DTK_M(DOY) | DTK_M(MONTH) | DTK_M(DAY));
		tm->tm_yday = val;
		/* tm_mon and tm_mday can't actually be set yet ... */
		return 0;
	}

	/* Switch based on what we have so far */
	switch (fmask & DTK_DATE_M)
	{
		case 0:

			/*
			 * Nothing so far; make a decision about what we think the input
			 * is.  There used to be lots of heuristics here, but the
			 * consensus now is to be paranoid.  It *must* be either
			 * YYYY-MM-DD (with a more-than-two-digit year field), or the
			 * field order defined by DateOrder.
			 */
			if (flen >= 3 || DateOrder == DATEORDER_YMD)
			{
				*tmask = DTK_M(YEAR);
				tm->tm_year = val;
			}
			else if (DateOrder == DATEORDER_DMY)
			{
				*tmask = DTK_M(DAY);
				tm->tm_mday = val;
			}
			else
			{
				*tmask = DTK_M(MONTH);
				tm->tm_mon = val;
			}
			break;

		case (DTK_M(YEAR)):
			/* Must be at second field of YY-MM-DD */
			*tmask = DTK_M(MONTH);
			tm->tm_mon = val;
			break;

		case (DTK_M(MONTH)):
			if (haveTextMonth)
			{
				/*
				 * We are at the first numeric field of a date that included a
				 * textual month name.  We want to support the variants
				 * MON-DD-YYYY, DD-MON-YYYY, and YYYY-MON-DD as unambiguous
				 * inputs.  We will also accept MON-DD-YY or DD-MON-YY in
				 * either DMY or MDY modes, as well as YY-MON-DD in YMD mode.
				 */
				if (flen >= 3 || DateOrder == DATEORDER_YMD)
				{
					*tmask = DTK_M(YEAR);
					tm->tm_year = val;
				}
				else
				{
					*tmask = DTK_M(DAY);
					tm->tm_mday = val;
				}
			}
			else
			{
				/* Must be at second field of MM-DD-YY */
				*tmask = DTK_M(DAY);
				tm->tm_mday = val;
			}
			break;

		case (DTK_M(YEAR) | DTK_M(MONTH)):
			if (haveTextMonth)
			{
				/* Need to accept DD-MON-YYYY even in YMD mode */
				if (flen >= 3 && *is2digits)
				{
					/* Guess that first numeric field is day was wrong */
					*tmask = DTK_M(DAY);	/* YEAR is already set */
					tm->tm_mday = tm->tm_year;
					tm->tm_year = val;
					*is2digits = false;
				}
				else
				{
					*tmask = DTK_M(DAY);
					tm->tm_mday = val;
				}
			}
			else
			{
				/* Must be at third field of YY-MM-DD */
				*tmask = DTK_M(DAY);
				tm->tm_mday = val;
			}
			break;

		case (DTK_M(DAY)):
			/* Must be at second field of DD-MM-YY */
			*tmask = DTK_M(MONTH);
			tm->tm_mon = val;
			break;

		case (DTK_M(MONTH) | DTK_M(DAY)):
			/* Must be at third field of DD-MM-YY or MM-DD-YY */
			*tmask = DTK_M(YEAR);
			tm->tm_year = val;
			break;

		case (DTK_M(YEAR) | DTK_M(MONTH) | DTK_M(DAY)):
			/* we have all the date, so it must be a time field */
			dterr = DecodeNumberField(flen, str, fmask,
									  tmask, tm,
									  fsec, is2digits);
			if (dterr < 0)
				return dterr;
			return 0;

		default:
			/* Anything else is bogus input */
			return DTERR_BAD_FORMAT;
	}

	/*
	 * When processing a year field, mark it for adjustment if it's only one
	 * or two digits.
	 */
	if (*tmask == DTK_M(YEAR))
		*is2digits = (flen <= 2);

	return 0;
}


/* DecodeNumberField()
 * Interpret numeric string as a concatenated date or time field.
 * Return a DTK token (>= 0) if successful, a DTERR code (< 0) if not.
 *
 * Use the context of previously decoded fields to help with
 * the interpretation.
 */
static int
DecodeNumberField(int len, char *str, int fmask,
				  int *tmask, struct pg_tm *tm, fsec_t *fsec, bool *is2digits)
{
	char	   *cp;

	/*
	 * Have a decimal point? Then this is a date or something with a seconds
	 * field...
	 */
	if ((cp = strchr(str, '.')) != NULL)
	{
		/*
		 * Can we use ParseFractionalSecond here?  Not clear whether trailing
		 * junk should be rejected ...
		 */
		double		frac;

		errno = 0;
		frac = strtod(cp, NULL);
		if (errno != 0)
			return DTERR_BAD_FORMAT;
		*fsec = rint(frac * 1000000);
		/* Now truncate off the fraction for further processing */
		*cp = '\0';
		len = strlen(str);
	}
	/* No decimal point and no complete date yet? */
	else if ((fmask & DTK_DATE_M) != DTK_DATE_M)
	{
		if (len >= 6)
		{
			*tmask = DTK_DATE_M;

			/*
			 * Start from end and consider first 2 as Day, next 2 as Month,
			 * and the rest as Year.
			 */
			tm->tm_mday = atoi(str + (len - 2));
			*(str + (len - 2)) = '\0';
			tm->tm_mon = atoi(str + (len - 4));
			*(str + (len - 4)) = '\0';
			tm->tm_year = atoi(str);
			if ((len - 4) == 2)
				*is2digits = true;

			return DTK_DATE;
		}
	}

	/* not all time fields are specified? */
	if ((fmask & DTK_TIME_M) != DTK_TIME_M)
	{
		/* hhmmss */
		if (len == 6)
		{
			*tmask = DTK_TIME_M;
			tm->tm_sec = atoi(str + 4);
			*(str + 4) = '\0';
			tm->tm_min = atoi(str + 2);
			*(str + 2) = '\0';
			tm->tm_hour = atoi(str);

			return DTK_TIME;
		}
		/* hhmm? */
		else if (len == 4)
		{
			*tmask = DTK_TIME_M;
			tm->tm_sec = 0;
			tm->tm_min = atoi(str + 2);
			*(str + 2) = '\0';
			tm->tm_hour = atoi(str);

			return DTK_TIME;
		}
	}

	return DTERR_BAD_FORMAT;
}

/* Begin - ReqID:SRS-MYSQL-DATETYPE */

/* CalmicroSecondsPower
* Calculates milliseconds raised to powers
*/
static int
CalmicroSecondsPower(int num)
{
  int	  ans = 1;
  while(num)
  {
	  ans *= 10;
	  num--;
  }
  return ans;
}

/* Check datetime */
static bool 
check_datetime(struct pg_tm *tm, bool not_zero_date, int flag, int *warn)
{
	if (not_zero_date)
	{
		if (((flag & TIME_NO_ZERO_IN_DATE) || !(flag & TIME_FUZZY_DATE)) &&
			(tm->tm_mon == 0 || tm->tm_mday == 0))
		{
			*warn = MYSQL_TIME_WARN_ZERO_IN_DATE;
			return true;
		}
		/*
		* Check for valid day of month, now that we know for sure the month
		* and year.	Note we don't use MD_FIELD_OVERFLOW here, since it seems
		* unlikely that "Feb 29" is a YMD-order error.
		*/
		else if ((!(flag & TIME_INVALID_DATES) && tm->tm_mon > 0 &&
			tm->tm_mday > day_tab[isleap(tm->tm_year)][tm->tm_mon - 1]))
		{
			*warn = MYSQL_TIME_WARN_OUT_OF_RANGE;
			return true;
		}
	}
	else if (flag & TIME_NO_ZERO_DATE)
	{
		*warn = MYSQL_TIME_WARN_ZERO_DATE;
		return true;
	}

	return false;
}

int internal_format_positions[] = {0, 1, 2, 3, 4, 5, 6, 255};

/*
* check datetime range
*/
bool
check_datetime_range(struct pg_tm *tm, int fsec, int ftype)
{

	return
		tm->tm_year > 9999 || tm->tm_mon > 12  || tm->tm_mday > 31 || 
		tm->tm_min > 59 || tm->tm_sec > 59 || fsec > 999999 ||
		(tm->tm_hour >
		(ftype == MYSQL_TIMESTAMP_TIME ? TIME_MAX_HOUR : 23));
}

/* Calculate the impact of nanoseconds on datetime */
bool
mysdatetime_add_nanoseconds_with_round(struct pg_tm *tm, int *fsec, int *warn, int nanoseconds)
{
	if (nanoseconds < 500)
		return false;

	*fsec += (nanoseconds + 500) / 1000;

	if (*fsec < 1000000)
		return false;
	if (check_datetime(tm, non_zero_date(tm), (TIME_NO_ZERO_IN_DATE | TIME_NO_ZERO_DATE), warn))
		return true;

	if(*fsec == 1000000)
	{
		*fsec %= 1000000;
		tm->tm_sec++;
	}
	if(tm->tm_sec == 60)
	{
		tm->tm_sec = 0;
		tm->tm_min++;
	}
	if(tm->tm_min == 60)
	{
		tm->tm_min = 0;
		tm->tm_hour++;
	}
	if(tm->tm_hour == 24)
	{
		tm->tm_hour = 0;
		tm->tm_mday++;
	}
	if(tm->tm_mday > day_tab[isleap(tm->tm_year)][tm->tm_mon - 1])
	{
		tm->tm_mday = 1;
		tm->tm_mon++;
	}
	if(tm->tm_mon > 12)
	{
		tm->tm_mon = 1;
		tm->tm_year++;
	}
	if(tm->tm_year > 9999)
	{
		*warn |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return true;
	}
	return false;
}

/* Calculate the seconds: Microseconds rounded to seconds */
bool
mysdatetime_rounded_seconds(struct pg_tm *tm, int *fsec, int *warn)
{
	if(*fsec < 500000)
	{
		return false; 
	}

	if(tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0 )
	{
		return false; 
	}
	
	tm->tm_sec++;
	if(tm->tm_sec == 60)
	{
		tm->tm_sec = 0;
		tm->tm_min++;
	}
	if(tm->tm_min == 60)
	{
		tm->tm_min = 0;
		tm->tm_hour++;
	}
	if(tm->tm_hour == 24)
	{
		tm->tm_hour = 0;
		tm->tm_mday++;
	}
	if(tm->tm_mday > day_tab[isleap(tm->tm_year)][tm->tm_mon - 1])
	{
		tm->tm_mday = 1;
		tm->tm_mon++;
	}
	if(tm->tm_mon > 12)
	{
		tm->tm_mon = 1;
		tm->tm_year++;
	}
	if(tm->tm_year > 9999)
	{
		*warn |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return true;
	}
	return false;
}

/*
* Convert a timestamp string to a MYSQL date and time value.
*/
int 
mystr_to_datetime(const char *str, int length, struct pg_tm *tm, int flag,
					  fsec_t *fsec, mysql_dttype *ftype, int *warn, int *nanoseconds, int * fsec_len)
{
	int	  field_length = 0,
		  year_length = 0,
		  digits,
		  i, 
		  number_of_fields;
	int	  date[MAX_DATE_PARTS];
	int	  date_len[MAX_DATE_PARTS];
	int	  add_hours = 0,
		  start_loop;
	long	  not_zero_date,
		  allow_space;
	bool	  is_internal_format;
	const char *pos,
			 *last_field_pos = NULL;
	const char *end = str + length;
	int		 *format_position;
	bool	  found_delimitier = 0,
		  found_space = 0;
	int	  frac_pos,
		  frac_len;

	/* Skip space at start */
	for (; str != end && isspace((unsigned char)*str) ; str++)
		;

	if (str == end || !isdigit((unsigned char)*str))
	{
		*warn = MYSQL_TIME_WARN_TRUNCATED;
		*ftype = MYSQL_TIMESTAMP_NONE;
		return -1;
	}

	is_internal_format = false;
	format_position = internal_format_positions;

	/*
	* Calculate number of digits in first part.
	* If length= 8 or >= 14 then year is of format YYYY.
	* (YYYY-MM-DD,  YYYYMMDD, YYYYYMMDDHHMMSS).
	*/
	for (pos = str; pos != end && (isdigit((unsigned char)*pos) || *pos == 'T'); pos++)
		;
	digits = (pos - str);

	start_loop = 0;								  /* Start of scan loop */
	date_len[format_position[0]] = 0; 			  /* Length of year field */

	if (pos == end || *pos == '.')
	{
		/* Found date in internal format (only numbers like YYYYMMDD) */
		year_length = (digits == 4 || digits == 8 || digits >= 14) ? 4 : 2;
		field_length = year_length;
		is_internal_format = true;
		format_position = internal_format_positions;
	}
	else
	{
		field_length = format_position[0] == 0 ? 4 : 2;
	}

	i = format_position[0] > format_position[1] ? format_position[0] : format_position[1];
	set_bigger(i, format_position[2]);
	allow_space = ((1 << i) | (1 << format_position[6]));
	allow_space &= (1 | 2 | 4 | 8 | 64);

	not_zero_date = 0;
	for (i = start_loop; i < MAX_DATE_PARTS - 1 && str != end &&
		isdigit((unsigned char)*str); i++)
	{
		const char *start = str;
		long		  tmp_value = (int)(*str++ - '0');

		/* Determine the internal format and millisecond segment. */
		bool		  scan_until_delim = !is_internal_format && ((i != format_position[6]));
		while (str != end && isdigit((unsigned char)str[0]) &&
			(scan_until_delim || --field_length))
		{
			tmp_value = tmp_value * 10 + (long)(*str - '0');
			str++;
		}
		date_len[i] = (str - start);

		/* Impossible date part */
		if (tmp_value > 999999)
		{
			*warn = MYSQL_TIME_WARN_TRUNCATED;
			*ftype = MYSQL_TIMESTAMP_NONE;
			return -1;
		}
		date[i] = tmp_value;
		not_zero_date |= tmp_value;

		/* Length of next field */
		field_length = format_position[i + 1] == 0 ? 4 : 2;

		/* Register last found part */
		if ((last_field_pos = str) == end)
		{
			i++;
			break;
		}
		/* Allow a 'T' after day to allow CCYYMMDDT type of fields */
		if (i == format_position[2] && *str == 'T')
		{
			str++;
			continue;
		}
		if (i == format_position[5])	  /* Seconds */
		{
			/* Followed by part seconds */
			if (*str == '.')
			{
				str++;
				/*
				* Shift last_field_pos, so '2001-01-01 00:00:00.'
				*is treated as a valid value
				*/
				last_field_pos = str;
				field_length = 6; 	  /* 6 digits */
			}
			else if (isdigit((unsigned char)str[0]))
			{
				/*
				* We do not see a decimal point which would have indicated a
				* fractional second part in further read. So we skip the further
				* processing of digits.
				*/
				i++;
				break;
			}
			continue;
		}

		while (str != end && (ispunct((unsigned char)*str) || isspace((unsigned char)*str)))
		{
			if (isspace((unsigned char)*str))
			{
				if (!(allow_space & (1 << i)))
				{
					*warn = MYSQL_TIME_WARN_TRUNCATED;
					*ftype = MYSQL_TIMESTAMP_NONE;
					return -1;
				}
				found_space = 1;
			}
			str++;
			/* Should be a 'normal' date */
			found_delimitier = 1;
		}
		/* Check if next position is AM/PM */
		if (i == format_position[6])		  /* Seconds, time for AM/PM */
		{
			/* Skip AM/PM part */
			i++;
			if (format_position[7] != 255)  /* If using AM/PM */
			{
				if (str + 2 <= end && (str[1] == 'M' || str[1] == 'm'))
				{
					if (str[0] == 'p' || str[0] == 'P')
					{
						//flagpm = true;
						add_hours = 12;
					}
					else if (str[0] != 'a' || str[0] != 'A')
						continue; 			  /* Not AM/PM */
					str += 2; 				  /* Skip AM/PM */
					/* Skip space after AM/PM */
					while (str != end && isspace((unsigned char)*str))
						str++;
				}
			}
		}
		last_field_pos = str;
	}

	/* check is datetime,eg 2011-01-11 12:23:33. */
	if (found_delimitier && !found_space && (flag & TIME_DATETIME_ONLY))
	{
		*warn = MYSQL_TIME_WARN_TRUNCATED;
		*ftype = MYSQL_TIMESTAMP_NONE;
		return -1;
	}

	str = last_field_pos;
	number_of_fields = i - start_loop;
	while (i < MAX_DATE_PARTS)
	{
		date_len[i] = 0;
		date[i++] = 0;
	}
	if (!is_internal_format)
	{
		year_length = date_len[format_position[0]];
		if (!year_length) 	  /* Year must be specified */
		{
			*warn = MYSQL_TIME_WARN_TRUNCATED;
			*ftype = MYSQL_TIMESTAMP_NONE;
			return -1;
		}

		tm->tm_year = date[format_position[0]];
		tm->tm_mon  = date[format_position[1]];
		tm->tm_mday = date[format_position[2]];
		tm->tm_hour = date[format_position[3]];
		tm->tm_min  = date[format_position[4]];
		tm->tm_sec  = date[format_position[5]];

		frac_pos = format_position[6];
		frac_len= date_len[frac_pos];
		if (frac_len < 6)
			date[frac_pos] *= CalmicroSecondsPower( DATETIME_MAX_DECIMALS - date_len[6]);;
		*fsec = date[frac_pos];
		*fsec_len = frac_len;

		if (format_position[7] != 255)
		{
			if (tm->tm_hour > 12)
			{
				*warn = MYSQL_TIME_WARN_TRUNCATED;
				return -1;
			}
			tm->tm_hour= tm->tm_hour % 12 + add_hours;
		}
	}
	else
	{
		tm->tm_year = date[0];
		tm->tm_mon  = date[1];
		tm->tm_mday = date[2];
		tm->tm_hour = date[3];
		tm->tm_min  = date[4];
		tm->tm_sec  = date[5];
		if (date_len[6] < 6)
		{
			date[6] *= CalmicroSecondsPower( DATETIME_MAX_DECIMALS - date_len[6]);
		}
		frac_len = date_len[6];
		*fsec = date[6];

		*fsec_len = frac_len;
	}

	if (year_length == 2 && not_zero_date)
		tm->tm_year+= (tm->tm_year < YY_PART_YEAR ? 2000 : 1900);

	*ftype = number_of_fields <= 3 ? MYSQL_TIMESTAMP_DATE : MYSQL_TIMESTAMP_DATETIME;

	if (number_of_fields < 3 || check_datetime_range(tm, *fsec, *ftype))
	{
		/* Only give warning for a zero date if there is some garbage after */
		if (!not_zero_date)
		{
			for (; str != end ; str++)
			{
				if (!isspace((unsigned char)*str))
				{
					/* Give warning */
					not_zero_date = 1;
					break;
				}
			}
		}
		*warn |= not_zero_date ? MYSQL_TIME_WARN_TRUNCATED : MYSQL_TIME_WARN_ZERO_DATE;
		MYS_ZERO_tm(tm);
		*ftype = MYSQL_TIMESTAMP_ERROR;
		return -1;
	}

	if(check_datetime(tm, not_zero_date != 0, flag, warn))
	{
		MYS_ZERO_tm(tm);
		*ftype = MYSQL_TIMESTAMP_ERROR;
		return -1;
	}

	/* Scan all digits left after microseconds */
	if (frac_len == 6 && str != end)
	{
		if (isdigit((unsigned char)*str))
		{
			/*
			* We don't need the exact nanoseconds value.
			* Knowing the first digit is enough for rounding.
			*/
			*nanoseconds = 100 * (int) (*str++ - '0');
			for (; str != end && isdigit((unsigned char)*str); str++)
			{

			}
		}
	}

	for (; str != end ; str++)
	{
		if (!isspace((unsigned char)*str))
		{
			*warn = MYSQL_TIME_WARN_TRUNCATED;
			break;
		}
	}

	return 0;
}

static void
get_datetime_from_num(int64 num, struct pg_tm *tm)
{
	long part1, part2;

	part1 = (long)(num / 1000000);
	part2 = (long)(num - (long long)part1 * 1000000);
	tm->tm_year = (int)(part1 / 10000);
	part1 %= 10000;
	tm->tm_mon = (int)part1 / 100;
	tm->tm_mday = (int)part1 % 100;
	tm->tm_hour = (int)(part2 / 10000);
	part2 %= 10000;
	tm->tm_min = (int)part2 / 100;
	tm->tm_sec = (int)part2 % 100;
}

/* Check if the time part is zero */
bool
non_zero_time(const struct pg_tm *tm, int fsec)
{
	return tm->tm_hour || tm->tm_min || tm->tm_sec || fsec;
}

bool
non_zero_date(const struct pg_tm *tm)
{
	return tm->tm_year || tm->tm_mon || tm->tm_mday;
}

/* Convert datetime value specified as number to broken-down TIME
 * representation and form value of DATETIME type .
 */
int number_to_datetime(int64 num, struct pg_tm *tm, int flag, int *fsec, mysql_dttype *ftype, int *warn)
{
	*warn = 0;
	*ftype = MYSQL_TIMESTAMP_DATE;

	if (num == 0 || num >= 10000101000000)
	{
		*ftype = MYSQL_TIMESTAMP_DATETIME;
		/* 9999-12-31 23:59:59 */
		if (num > 99991231235959)
		{
			*warn = MYSQL_TIME_WARN_OUT_OF_RANGE;
			return -1;
		}
		goto isdatetime;
	}

	if (num < 101)
		goto err;

	if (num <= (YY_PART_YEAR - 1) * 10000 + 1231)
	{
		/* YYMMDD, year: 2000-2069 */
		num = (num + 20000000) * 1000000;
		goto isdatetime;
	}

	if (num < (YY_PART_YEAR) * 10000 + 101)
		goto err;

	if (num <= 991231)
	{
		/* YYMMDD, year: 1970-1999 */
		num = (num + 19000000) * 1000000;
		goto isdatetime;
	}

	/*
	 * Though officially we support DATE values from 1000-01-01 only, one can
	 * easily insert a value like 1-1-1. So, for consistency reasons such dates
	 * are allowed when TIME_FUZZY_DATE is set.
	 */
	if (num <= 99991231)
	{
		num = num * 1000000;
		goto isdatetime;
	}

	if (num < 101000000)
		goto err;

	*ftype = MYSQL_TIMESTAMP_DATETIME;

	if (num <= (YY_PART_YEAR - 1) * 10000000000 + 1231235959)
	{
		/* YYMMDDHHMMSS, 2000-2069 */
		num = num + 20000000000000;
		goto isdatetime;
	}

	if (num < YY_PART_YEAR * 10000000000 + 101000000)
		goto err;
		
	if (num <= 991231235959)
		num = num + 19000000000000; /* YYMMDDHHMMSS, 1970-1999 */

isdatetime:
	get_datetime_from_num(num, tm);
	if (!check_datetime(tm, (num != 0), flag, warn) && !check_datetime_range(tm, *fsec, *ftype))
		return 1;
err:
	*warn = MYSQL_TIME_WARN_TRUNCATED;
	return -1;
}
/* End - ReqID:SRS-MYSQL-DATETYPE */

/* Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE */
/*
  Calculate nr of day since year 0 in new date-system (from 1615)

  SYNOPSIS
    calc_daynr()
    year		 Year (exact 4 digit year, no year conversions)
    month		 Month
    day			 Day

  NOTES: 0000-00-00 is a valid date, and will return 0

  RETURN
    Days since 0000-00-00
*/

static inline long calc_daynr(uint32 year,uint32 month,uint32 day)
{
	long delsum;
	int temp;
	int y= year;                                  /* may be < 0 temporarily */

	if (y == 0 && month == 0)
		return 0;				/* Skip errors */
	/* Cast to int to be able to handle month == 0 */
	delsum= (long) (365 * y + 31 *((int) month - 1) + (int) day);
	if (month <= 2)
		y--;
	else
		delsum-= (long) ((int) month * 4 + 23) / 10;
	temp=(int) ((y/100+1)*3)/4;

	return (delsum+(int) y/4-temp);
} /* calc_daynr */

/*
  Function to check sanity of a TIMESTAMP value

  DESCRIPTION
    Check if a given MYSQL_TIME value fits in TIMESTAMP range.
    This function doesn't make precise check, but rather a rough
    estimate.

  RETURN VALUES
    TRUE    The value seems sane
    FALSE   The MYSQL_TIME value is definitely out of range
*/

bool validate_timestamp_range(struct pg_tm *tm) /* ReqID:SRS-MYSQL-BUILDINFUNCTION */
{
	if ((tm->tm_year > TIMESTAMP_MAX_YEAR || tm->tm_year < TIMESTAMP_MIN_YEAR) ||
		(tm->tm_year == TIMESTAMP_MAX_YEAR && (tm->tm_mon > 1 || tm->tm_mday > 19)) ||
		(tm->tm_year == TIMESTAMP_MIN_YEAR && (tm->tm_mon < 12 || tm->tm_mday < 31)))
		return false;

	return true;
}

/*
  Convert time in MYSQL_TIME representation in system time zone to its
  my_time_t form (number of seconds in UTC since begginning of Unix Epoch).

  SYNOPSIS
    my_system_gmt_sec()
      t               - time value to be converted
      my_timezone     - pointer to long where offset of system time zone
                        from UTC will be stored for caching
      in_dst_time_gap - set to true if time falls into spring time-gap

  NOTES
    The idea is to cache the time zone offset from UTC (including daylight 
    saving time) for the next call to make things faster. But currently we 
    just calculate this offset during startup (by calling my_init_time() 
    function) and use it all the time.
    Time value provided should be legal time value (e.g. '2003-01-01 25:00:00'
    is not allowed).

  RETURN VALUE
    Time in UTC seconds since Unix Epoch representation.
*/
int64
my_system_gmt_sec(struct pg_tm *tm, long *my_timezone, bool *in_dst_time_gap)
{
	uint32 loop;
	time_t tmp= 0;
	int shift= 0;
	struct pg_tm tmp_time;
	struct pg_tm *t= &tmp_time;
	struct tm *l_time;
	long diff, current_timezone;
	int my_time_zone = 0;
	/*
	Use temp variable to avoid trashing input data, which could happen in
	case of shift required for boundary dates processing.
	*/
	memcpy(&tmp_time, tm, sizeof(struct pg_tm));

	if (!validate_timestamp_range(t))
		return 0;

	/*
	Calculate the gmt time based on current time and timezone
	The -1 on the end is to ensure that if have a date that exists twice
	(like 2002-10-27 02:00:0 MET), we will find the initial date.

	By doing -3600 we will have to call localtime_r() several times, but
	I couldn't come up with a better way to get a repeatable result :(

	We can't use mktime() as it's buggy on many platforms and not thread safe.

	Note: this code assumes that our time_t estimation is not too far away
	from real value (we assume that localtime_r(tmp) will return something
	within 24 hrs from t) which is probably true for all current time zones.

	Note2: For the dates, which have time_t representation close to
	MAX_INT32 (efficient time_t limit for supported platforms), we should
	do a small trick to avoid overflow. That is, convert the date, which is
	two days earlier, and then add these days to the final value.

	The same trick is done for the values close to 0 in time_t
	representation for platfroms with unsigned time_t (QNX).

	To be more verbose, here is a sample (extracted from the code below):
	(calc_daynr(2038, 1, 19) - (long) days_at_timestart)*86400L + 4*3600L
	would return -2147480896 because of the long type overflow. In result
	we would get 1901 year in localtime_r(), which is an obvious error.

	Alike problem raises with the dates close to Epoch. E.g.
	(calc_daynr(1969, 12, 31) - (long) days_at_timestart)*86400L + 23*3600L
	will give -3600.

	On some platforms, (E.g. on QNX) time_t is unsigned and localtime(-3600)
	wil give us a date around 2106 year. Which is no good.

	Theoreticaly, there could be problems with the latter conversion:
	there are at least two timezones, which had time switches near 1 Jan
	of 1970 (because of political reasons). These are America/Hermosillo and
	America/Mazatlan time zones. They changed their offset on
	1970-01-01 08:00:00 UTC from UTC-8 to UTC-7. For these zones
	the code below will give incorrect results for dates close to
	1970-01-01, in the case OS takes into account these historical switches.
	Luckily, it seems that we support only one platform with unsigned
	time_t. It's QNX. And QNX does not support historical timezone data at all.
	E.g. there are no /usr/share/zoneinfo/ files or any other mean to supply
	historical information for localtime_r() etc. That is, the problem is not
	relevant to QNX.

	We are safe with shifts close to MAX_INT32, as there are no known
	time switches on Jan 2038 yet :)
	*/
	if ((t->tm_year == TIMESTAMP_MAX_YEAR) && (t->tm_mon == 1) && (t->tm_mday > 4))
	{
	/*
		Below we will pass (uint) (t->day - shift) to calc_daynr.
		As we don't want to get an overflow here, we will shift
		only safe dates. That's why we have (t->day > 4) above.
	*/
	t->tm_mday-= 2;
	shift= 2;
	}

	tmp= (time_t) (((calc_daynr((uint32) t->tm_year, (uint32) t->tm_mon, (uint32) t->tm_mday) -
					(long) DAYS_AT_TIMESTART) * SECONDS_IN_24H +
					(long) t->tm_hour*3600L +
					(long) (t->tm_min*60 + t->tm_sec)) + (time_t) my_time_zone -
					3600);

	current_timezone= my_time_zone;
	l_time = localtime(&tmp);
	if (l_time == NULL)
		return 0;

	for (loop=0;
		loop < 2 &&
		(t->tm_hour != (uint32) l_time->tm_hour ||
		t->tm_min != (uint32) l_time->tm_min ||
		t->tm_sec != (uint32) l_time->tm_sec);
		loop++)
	{ /* One check should be enough ? */
		/* Get difference in days */
		int days = t->tm_mday - l_time->tm_mday;
		if (days < -1)
			days = 1; /* Month has wrapped */
		else if (days > 1)
			days = -1;

		diff = (3600L * (long)(days * 24 + ((int)t->tm_hour - (int)l_time->tm_hour)) +
				(long)(60 * ((int)t->tm_min - (int)l_time->tm_min)) +
				(long)((int)t->tm_sec - (int)l_time->tm_sec));
		current_timezone += diff + 3600; /* Compensate for -3600 above */
		tmp += (time_t)diff;

		l_time = localtime(&tmp);
		if (l_time == NULL)
			return 0;
	}
	/*
	Fix that if we are in the non existing daylight saving time hour
	we move the start of the next real hour.

	This code doesn't handle such exotical thing as time-gaps whose length
	is more than one hour or non-integer (latter can theoretically happen
	if one of seconds will be removed due leap correction, or because of
	general time correction like it happened for Africa/Monrovia time zone
	in year 1972).
	*/
	if (loop == 2 && t->tm_hour != (uint32) l_time->tm_hour)
	{
		int days= t->tm_mday - l_time->tm_mday;
		if (days < -1)
			days=1;					/* Month has wrapped */
		else if (days > 1)
			days= -1;
		diff=(3600L*(long) (days*24+((int) t->tm_hour - (int) l_time->tm_hour))+
			(long) (60*((int) t->tm_min - (int) l_time->tm_min)) +
			(long) ((int) t->tm_sec - (int) l_time->tm_sec));
		if (diff == 3600)
			tmp+=3600 - t->tm_min*60 - t->tm_sec;	/* Move to next hour */
		else if (diff == -3600)
			tmp-=t->tm_min*60 + t->tm_sec;		/* Move to previous hour */

		*in_dst_time_gap= 1;
	}
	*my_timezone= current_timezone;


	/* shift back, if we were dealing with boundary dates */
	tmp+= shift * SECONDS_IN_24H;

	/*
	This is possible for dates, which slightly exceed boundaries.
	Conversion will pass ok for them, but we don't allow them.
	First check will pass for platforms with signed time_t.
	instruction above (tmp+= shift*86400L) could exceed
	MAX_INT32 (== TIMESTAMP_MAX_VALUE) and overflow will happen.
	So, tmp < TIMESTAMP_MIN_VALUE will be triggered. On platfroms
	with unsigned time_t tmp+= shift*86400L might result in a number,
	larger then TIMESTAMP_MAX_VALUE, so another check will work.
	*/
	if (!IS_TIME_T_VALID_FOR_TIMESTAMP(tmp))
	tmp= 0;

	return (int64) tmp;
} /* my_system_gmt_sec */

bool datetime_with_no_zero_in_date_to_timeval(struct pg_tm *tm, fsec_t fsec, int *warn)
{
	long	my_timezone = 0;
	bool	in_dst_time_gap = 0;

	if (!tm->tm_mon) /* Zero date */
	{
		if (non_zero_time(tm, fsec))
		{
			/*
			Return error for zero date with non-zero time, e.g.:
			'0000-00-00 10:20:30' or '0000-00-00 00:00:00.123456'
			*/
			*warn |= MYSQL_TIME_WARN_TRUNCATED;
			return true;
		}
		return true;
	}

	if (!my_system_gmt_sec(tm, &my_timezone, &in_dst_time_gap))
	{
		/*
		Date was outside of the supported timestamp range.
		For example: '3001-01-01 00:00:00' or '1000-01-01 00:00:00'
		*/
		*warn |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return true;
	}
	else if (in_dst_time_gap)
	{
		/*
		Set MYSQL_TIME_WARN_INVALID_TIMESTAMP warning to indicate
		that date was fine but pointed to winter/summer time switch gap.
		In this case tm is set to the fist second after gap.
		For example: '2003-03-30 02:30:00 MSK' -> '2003-03-30 03:00:00 MSK'
		*/
		*warn |= MYSQL_TIME_WARN_INVALID_TIMESTAMP;
	}
	return false;
}

/* End - ReqID:SRS-MYSQL-TIMESTAMPTYPE */


/* Begin - ReqID:SRS-MYSQL-TIMETYPE */

bool set_mystime_warning(char *str, char *datetype, int is_err, int warn, int is_strict)
{
	if(!is_err && !warn)
	{
		return false;
	}
	if(is_err) /* Returns an error result, the value is not 0. */
	{
		if(is_strict) /* Is strict mode*/
		{
			if(warn & MYSQL_TIME_WARN_TRUNCATED)
			{
				ereport(ERROR, (errcode(ERRCODE_INVALID_DATETIME_FORMAT), errmsg("Incorrect TIME value: '%s'", str)));
			}
			if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE | MYSQL_TIME_WARN_ZERO_IN_DATE))
			{
				ereport(ERROR, (errcode(ERRCODE_INVALID_DATETIME_FORMAT), errmsg("Incorrect TIME value: '%s'", str)));
			}
		}
		else /* non-strict mode */
		{
			if(warn & (MYSQL_TIME_WARN_ZERO_DATE | MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
			{
				ereport(WARNING, errcode(ERRCODE_INVALID_DATETIME_FORMAT), (errmsg("Out of range value.")));
			}
			if(warn & MYSQL_TIME_WARN_TRUNCATED)
			{
				ereport(WARNING, errcode(ERRCODE_INVALID_DATETIME_FORMAT), (errmsg("Data truncated for column.")));
			}
			return true;
		}
	}
	else /* Returns the correct result with a value of 0 */
	{
		if(warn & MYSQL_TIME_WARN_OUT_OF_RANGE)
		{
			ereport(WARNING, errcode(ERRCODE_INVALID_DATETIME_FORMAT), (errmsg("Out of range value.")));
		}
		else if(warn & (MYSQL_TIME_WARN_INVALID_TIMESTAMP |
						MYSQL_TIME_WARN_ZERO_DATE | MYSQL_TIME_NOTE_TRUNCATED | MYSQL_TIME_WARN_ZERO_IN_DATE))
		{
			ereport(WARNING, errcode(ERRCODE_INVALID_DATETIME_FORMAT), (errmsg("Data truncated for column.")));
		}
		else if(warn & MYSQL_TIME_WARN_TRUNCATED)
		{
			int	EREPORT_LEVEL = is_strict?ERROR:WARNING;
			ereport(EREPORT_LEVEL, errcode(ERRCODE_INVALID_DATETIME_FORMAT), (errmsg("Data truncated for column.")));
		}
	}
	return false;
}

bool
num_set_mystime_warning(char *str, char *datetype, bool is_err, int warn, int is_strict, int *is_out_range)
{
	if (!is_err && !warn)
	{
		return false;
	}

	if (is_err) /* Returns an error result, the value is not 0. */
	{
		if(is_strict)/* strict mode */
		{
			if(warn & MYSQL_TIME_WARN_TRUNCATED)
				ereport(ERROR, (errmsg("Incorrect TIME value: '%s'", str)));
			if(warn & (MYSQL_TIME_WARN_OUT_OF_RANGE | MYSQL_TIME_WARN_ZERO_DATE |
				MYSQL_TIME_WARN_ZERO_IN_DATE))
				ereport(ERROR, (errmsg("Incorrect TIME value: '%s'", str)));

		}
		else /* non-strict mode */
		{
			if(warn & (MYSQL_TIME_WARN_ZERO_DATE |
				MYSQL_TIME_WARN_ZERO_IN_DATE | MYSQL_TIME_WARN_OUT_OF_RANGE))
			{
				ereport(WARNING, (errmsg("Out of range value.")));
				*is_out_range = true;
			}
			if(warn & MYSQL_TIME_WARN_TRUNCATED)
			{
				ereport(WARNING, (errmsg("Data truncated for column.")));
			}
			return true;
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
			*is_out_range = true;
		}
		if ((warn & MYSQL_TIME_NOTE_TRUNCATED) &&
			!(warn & MYSQL_TIME_WARN_TRUNCATED))
		{
			ereport(WARNING, (errmsg("Incorrect TIME value:'%s'", str)));
		}
	}
	return false;
}

static bool
check_time_range_quick(struct pg_tm *tm, fsec_t fsec)
{
	long long hour = (long long)tm->tm_hour + 24LL * tm->tm_mday;

	if (hour <= TIME_MAX_HOUR &&
		(hour != TIME_MAX_HOUR || tm->tm_min != TIME_MAX_MINUTE ||
		 tm->tm_sec != TIME_MAX_SECOND || !fsec))
		return false;

	return true;
}

static void 
adjust_time_range(struct pg_tm *tm, fsec_t *fsec, int *warning) 
{
	if (check_time_range_quick(tm, *fsec))
	{
		tm->tm_mday = *fsec = 0;
		tm->tm_hour = TIME_MAX_HOUR;
		tm->tm_min = TIME_MAX_MINUTE;
		tm->tm_sec = TIME_MAX_SECOND;

		*warning |= MYSQL_TIME_WARN_TRUNCATED;
	}
}

bool
time_add_nanoseconds_with_round(struct pg_tm *tm, fsec_t *fsec, int nanoseconds, int *warnings)
{
	if (nanoseconds < 500)
		return false;

	*fsec += (nanoseconds + 500) / 1000;
	if (*fsec < 1000000)
		goto ret;

	*fsec %= 1000000;
	if (tm->tm_sec < 59)
	{
		tm->tm_sec++;
		goto ret;
	}

	tm->tm_sec = 0;
	if (tm->tm_min < 59)
	{
		tm->tm_min++;
		goto ret;
	}
	tm->tm_min = 0;
	tm->tm_hour++;

ret:
	/*
	 * We can get '838:59:59.000001' at this point, which
	 * is bigger than the maximum possible value '838:59:59.000000'.
	 * Checking only "hour > 838" is not enough.
	 * Do full adjust_time_range().
	 */
	adjust_time_range(tm, fsec, warnings);
	return false;
}

bool
mystime_rounded_seconds(struct pg_tm *tm, int *fsec, int *warn, int dec)
{
	
	if(*fsec < 500000)
	{
		return false; 
	}

	tm->tm_sec++;
	if(tm->tm_sec > TIME_MAX_SECOND)
	{
		tm->tm_sec = 0;
		tm->tm_min++;
	}
	if(tm->tm_min > TIME_MAX_MINUTE)
	{
		tm->tm_min = 0;
		tm->tm_hour++;
	}
	if(tm->tm_hour > TIME_MAX_HOUR)
	{
		tm->tm_mday = *fsec= 0;
		tm->tm_hour = TIME_MAX_HOUR;
		tm->tm_min = TIME_MAX_MINUTE;
		tm->tm_sec = TIME_MAX_SECOND;
		*warn |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return true;
	}
	return false;
}

/*
 Convert a time string to a MYSQL_TIME struct.

  SYNOPSIS
   str_to_time()
   str                  A string in full TIMESTAMP format or
                        [-] DAYS [H]H:MM:SS, [H]H:MM:SS, [M]M:SS, [H]HMMSS,
                        [M]MSS or [S]S
                        There may be an optional [.second_part] after seconds
   length               Length of str
   l_time               Store result here
   status               Conversion status

   status.warning is set to:
     MYSQL_TIME_WARN_TRUNCATED flag if the input string
                        was cut during conversion, and/or
     MYSQL_TIME_WARN_OUT_OF_RANGE flag, if the value is out of range.

   NOTES
     Because of the extra days argument, this function can only
     work with times where the time arguments are in the above order.

   RETURN
     0  ok
     1  error
*/

int
str_to_time(const char *str, size_t length, struct pg_tm *tm,
			fsec_t *fsec, mysql_dttype *ftype, int *warn, int *nanoseconds, int *neg)
{
	int64 date[5]; /* BUG#Z5567 */
	int64 value;
	const char *end = str + length, *end_of_days;
	bool found_days, found_hours;
	uint32 state;

	memset(date, 0, sizeof(date));

	*neg = 0;

	/* Skip space at start */
	for (; str != end && isspace((unsigned char)*str); str++)
		length--;

	if (str != end && *str == '-')
	{
		*neg = 1;
		str++;
		length--;
	}
	if (str == end)
		return -1;

	/* Check first if this is a full TIMESTAMP */
	if (length >= 12)
	{
		int fsec_len;
		/* Probably full timestamp */
		(void)mystr_to_datetime(str, strlen(str), tm, (TIME_FUZZY_DATE | TIME_DATETIME_ONLY),
								fsec, ftype, warn, nanoseconds, &fsec_len);
		if (*ftype >= MYSQL_TIMESTAMP_ERROR)
		{
			return *ftype == MYSQL_TIMESTAMP_ERROR ? -1 : 0;
		}
		*warn = 0;
	}

	/* Not a timestamp. Try to get this as a DAYS_TO_SECOND string */
	for (value = 0; str != end && isdigit((unsigned char)*str); str++)
		value = value * 10L + (long)(*str - '0');

	if (value > UINT_MAX)
		return -1;

	/* Skip all space after 'days' */
	end_of_days = str;
	for (; str != end && isspace((unsigned char)*str); str++)
		;

	state = 0;
	found_days = found_hours = 0;
	if ((uint32)(end - str) > 1 && str != end_of_days &&
		isdigit((unsigned char)*str))
	{ /* Found days part */
		date[0] = (uint64)value;
		state = 1; /* Assume next is hours */
		found_days = 1;
	}
	else if ((end - str) > 1 && *str == ':' &&
			 isdigit((unsigned char)str[1]))
	{
		date[0] = 0; /* Assume we found hours */
		date[1] = (uint64)value;
		state = 2;
		found_hours = 1;
		str++; /* skip ':' */
	}
	else
	{
		/* String given as one number; assume HHMMSS format */
		date[0] = 0;
		date[1] = (uint64)(value / 10000);
		date[2] = (uint64)(value / 100 % 100);
		date[3] = (uint64)(value % 100);
		state = 4;
		goto fractional;
	}

	/* Read hours, minutes and seconds */
	for (;;)
	{
		for (value = 0; str != end && isdigit((unsigned char)*str); str++)
		{
			value = value * 10L + (long)(*str - '0');
		}
		date[state++] = (uint64)value;

		if (state == 4 || (end - str) < 2 || *str != ':' ||
			!isdigit((unsigned char)str[1]))
			break;
		str++; /* Skip time_separator (':') */
	}

	if (state != 4)
	{ /* Not HH:MM:SS */
		/* Fix the date to assume that seconds was given */
		if (!found_hours && !found_days)
		{
			size_t len = sizeof(long) * (state - 1);
			memmove((unsigned char *)(date + 4) - len, (unsigned char *)(date + state) - len, len);
			memset(date, 0, sizeof(long) * (4 - state));
		}
		else
			memset((date + state), 0, sizeof(long) * (4 - state));
	}

fractional:
	/* Get fractional second part */
	if ((end - str) >= 2 && *str == '.' && isdigit((unsigned char)str[1]))
	{
		int field_length = 5;
		str++;
		value = (uint32)(unsigned char)(*str - '0');
		while (++str != end && isdigit((unsigned char)*str))
		{
			if (field_length-- > 0)
				value = value * 10 + (uint32)(unsigned char)(*str - '0');
		}
		if (field_length >= 0)
		{
			*fsec = DATETIME_MAX_DECIMALS - field_length;
			if (field_length > 0)
				value *= (long)CalmicroSecondsPower(field_length);
		}
		else
		{
			/* Scan digits left after microseconds */
			*fsec = 6;
			*nanoseconds = 100 * (int)(str[-1] - '0');
			for (; str != end && isdigit((unsigned char)*str); str++)
			{
			}
		}
		date[4] = (uint64)value;
	}
	else if ((end - str) == 1 && *str == '.')
	{
		str++;
		date[4] = 0;
	}
	else
		date[4] = 0;

	/* Check for exponent part: E<gigit> | E<sign><digit> */
	/* (may occur as result of %g formatting of time value) */
	if ((end - str) > 1 &&
		(*str == 'e' || *str == 'E') &&
		(isdigit((unsigned char)str[1]) ||
		 ((str[1] == '-' || str[1] == '+') &&
		  (end - str) > 2 && isdigit((unsigned char)str[2]))))
		return -1;

	if (internal_format_positions[7] != 255)
	{
		/* Read a possible AM/PM */
		while (str != end && isspace((unsigned char)*str))
			str++;
		if (str + 2 <= end && (str[1] == 'M' || str[1] == 'm'))
		{
			if (str[0] == 'p' || str[0] == 'P')
			{
				str += 2;
				date[1] = date[1] % 12 + 12;
			}
			else if (str[0] == 'a' || str[0] == 'A')
				str += 2;
		}
	}

	/* Integer overflow checks */
	if (date[0] > UINT_MAX || date[1] > UINT_MAX ||
		date[2] > UINT_MAX || date[3] > UINT_MAX ||
		date[4] > UINT_MAX)
		return -1;

	tm->tm_year = 0; /* For protocol::store_time */
	tm->tm_mon = 0;
	tm->tm_mday = 0;
	/* Begin - BUG#Z5567 */
	if ((date[1] + date[0] * 24) > TIME_MAX_HOUR)
		tm->tm_hour = TIME_MAX_HOUR + 1;
	else
		tm->tm_hour = date[1] + date[0] * 24; /* Mix days and hours */
	/* End - BUG#Z5567 */
	tm->tm_min = date[2];
	tm->tm_sec = date[3];
	*fsec = date[4];
	*ftype = MYSQL_TIMESTAMP_TIME;

	if (tm->tm_min >= 60 || tm->tm_sec >= 60 || *fsec > 999999)
	{
		*warn |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return -1;
	}

	/* Adjust the value into supported MYSQL_TIME range */
	adjust_time_range(tm, fsec, warn);

	/* Check if there is garbage at end of the MYSQL_TIME specification */
	if (str != end)
	{
		do
		{
			if (!isspace((unsigned char)*str))
			{
				*warn |= MYSQL_TIME_WARN_TRUNCATED;
				break;
			}
		} while (++str != end);
	}
	return 0;
}


/**
  Convert number to TIME
  @param nr            Number to convert.
  @param OUT ltime     Variable to convert to.
  @param OUT warnings  Warning vector.

  @retval false OK
  @retval true No. is out of range
*/
bool
number_to_time(int64 nr, struct pg_tm *tm, int *fsec, mysql_dttype *ftype, int *neg, int *warnings)
{
	if (nr > TIME_MAX_VALUE)
	{
		/* For huge numbers try full DATETIME, like str_to_time does. */
		if (nr >= 10000000000LL) /* '0001-00-00 00-00-00' */
		{
			int warnings_backup = *warnings;
			if (number_to_datetime(nr, tm, 0, fsec, ftype, warnings) != -1LL)
				return false;
			*warnings = warnings_backup;
		}

		memset(tm, 0, sizeof(*tm));
	
		tm->tm_hour = TIME_MAX_HOUR;
		tm->tm_min = TIME_MAX_MINUTE;
		tm->tm_sec = TIME_MAX_SECOND;

		*neg = 0;
		*ftype = MYSQL_TIMESTAMP_TIME;
		*warnings |= MYSQL_TIME_WARN_OUT_OF_RANGE;

		return true;
	}
	
	if (nr < -TIME_MAX_VALUE)
	{
		memset(tm, 0, sizeof(*tm));

		tm->tm_hour = TIME_MAX_HOUR;
		tm->tm_min = TIME_MAX_MINUTE;
		tm->tm_sec = TIME_MAX_SECOND;

		*neg = 1;
		*ftype = MYSQL_TIMESTAMP_TIME;

		*warnings |= MYSQL_TIME_WARN_OUT_OF_RANGE;

		return true;
	}

	if ((*neg = (nr < 0)))
		nr = -nr;

	if (nr % 100 >= 60 || nr / 100 % 100 >= 60) /* Check hours and minutes */
	{
		memset(tm, 0, sizeof(*tm));
		*ftype = MYSQL_TIMESTAMP_TIME;
		*warnings |= MYSQL_TIME_WARN_OUT_OF_RANGE;
		return true;
	}

	*ftype = MYSQL_TIMESTAMP_TIME;
	tm->tm_year = tm->tm_mon = tm->tm_mday = 0;

	tm->tm_sec = (int)(nr % 100);
	tm->tm_min = (int)(nr / 100) % 100;
	tm->tm_hour = (int)(nr / 10000);
	
	*fsec = 0;
	
	return false;
}
/* End - ReqID:SRS-MYSQL-TIMETYPE */

/* DecodeTimezone()
 * Interpret string as a numeric timezone.
 *
 * Return 0 if okay (and set *tzp), a DTERR code if not okay.
 */
int
DecodeTimezone(char *str, int *tzp)
{
	int			tz;
	int			hr,
				min,
				sec = 0;
	char	   *cp;

	/* leading character must be "+" or "-" */
	if (*str != '+' && *str != '-')
		return DTERR_BAD_FORMAT;

	errno = 0;
	hr = strtoint(str + 1, &cp, 10);
	if (errno == ERANGE)
		return DTERR_TZDISP_OVERFLOW;

	/* explicit delimiter? */
	if (*cp == ':')
	{
		errno = 0;
		min = strtoint(cp + 1, &cp, 10);
		if (errno == ERANGE)
			return DTERR_TZDISP_OVERFLOW;
		if (*cp == ':')
		{
			errno = 0;
			sec = strtoint(cp + 1, &cp, 10);
			if (errno == ERANGE)
				return DTERR_TZDISP_OVERFLOW;
		}
	}
	/* otherwise, might have run things together... */
	else if (*cp == '\0' && strlen(str) > 3)
	{
		min = hr % 100;
		hr = hr / 100;
		/* we could, but don't, support a run-together hhmmss format */
	}
	else
		min = 0;

	/* Range-check the values; see notes in datatype/timestamp.h */
	if (hr < 0 || hr > MAX_TZDISP_HOUR)
		return DTERR_TZDISP_OVERFLOW;
	if (min < 0 || min >= MINS_PER_HOUR)
		return DTERR_TZDISP_OVERFLOW;
	if (sec < 0 || sec >= SECS_PER_MINUTE)
		return DTERR_TZDISP_OVERFLOW;

	tz = (hr * MINS_PER_HOUR + min) * SECS_PER_MINUTE + sec;
	if (*str == '-')
		tz = -tz;

	*tzp = -tz;

	if (*cp != '\0')
		return DTERR_BAD_FORMAT;

	return 0;
}


/* DecodeTimezoneAbbrev()
 * Interpret string as a timezone abbreviation, if possible.
 *
 * Returns an abbreviation type (TZ, DTZ, or DYNTZ), or UNKNOWN_FIELD if
 * string is not any known abbreviation.  On success, set *offset and *tz to
 * represent the UTC offset (for TZ or DTZ) or underlying zone (for DYNTZ).
 * Note that full timezone names (such as America/New_York) are not handled
 * here, mostly for historical reasons.
 *
 * Given string must be lowercased already.
 *
 * Implement a cache lookup since it is likely that dates
 *	will be related in format.
 */
int
DecodeTimezoneAbbrev(int field, char *lowtoken,
					 int *offset, pg_tz **tz)
{
	int			type;
	const datetkn *tp;

	tp = abbrevcache[field];
	/* use strncmp so that we match truncated tokens */
	if (tp == NULL || strncmp(lowtoken, tp->token, TOKMAXLEN) != 0)
	{
		if (zoneabbrevtbl)
			tp = datebsearch(lowtoken, zoneabbrevtbl->abbrevs,
							 zoneabbrevtbl->numabbrevs);
		else
			tp = NULL;
	}
	if (tp == NULL)
	{
		type = UNKNOWN_FIELD;
		*offset = 0;
		*tz = NULL;
	}
	else
	{
		abbrevcache[field] = tp;
		type = tp->type;
		if (type == DYNTZ)
		{
			*offset = 0;
			*tz = FetchDynamicTimeZone(zoneabbrevtbl, tp);
		}
		else
		{
			*offset = tp->value;
			*tz = NULL;
		}
	}

	return type;
}


/* DecodeSpecial()
 * Decode text string using lookup table.
 *
 * Recognizes the keywords listed in datetktbl.
 * Note: at one time this would also recognize timezone abbreviations,
 * but no more; use DecodeTimezoneAbbrev for that.
 *
 * Given string must be lowercased already.
 *
 * Implement a cache lookup since it is likely that dates
 *	will be related in format.
 */
int
DecodeSpecial(int field, char *lowtoken, int *val)
{
	int			type;
	const datetkn *tp;

	tp = datecache[field];
	/* use strncmp so that we match truncated tokens */
	if (tp == NULL || strncmp(lowtoken, tp->token, TOKMAXLEN) != 0)
	{
		tp = datebsearch(lowtoken, datetktbl, szdatetktbl);
	}
	if (tp == NULL)
	{
		type = UNKNOWN_FIELD;
		*val = 0;
	}
	else
	{
		datecache[field] = tp;
		type = tp->type;
		*val = tp->value;
	}

	return type;
}


/* ClearPgTm
 *
 * Zero out a pg_tm and associated fsec_t
 */
static inline void
ClearPgTm(struct pg_tm *tm, fsec_t *fsec)
{
	tm->tm_year = 0;
	tm->tm_mon = 0;
	tm->tm_mday = 0;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	*fsec = 0;
}


/* DecodeInterval()
 * Interpret previously parsed fields for general time interval.
 * Returns 0 if successful, DTERR code if bogus input detected.
 * dtype, tm, fsec are output parameters.
 *
 * Allow "date" field DTK_DATE since this could be just
 *	an unsigned floating point number. - thomas 1997-11-16
 *
 * Allow ISO-style time span, with implicit units on number of days
 *	preceding an hh:mm:ss field. - thomas 1998-04-30
 */
int
DecodeInterval(char **field, int *ftype, int nf, int range,
			   int *dtype, struct pg_tm *tm, fsec_t *fsec)
{
	bool		is_before = false;
	char	   *cp;
	int			fmask = 0,
				tmask,
				type;
	int			i;
	int			dterr;
	int			val;
	double		fval = 0;

	*dtype = DTK_DELTA;
	type = IGNORE_DTF;
	ClearPgTm(tm, fsec);

	/* read through list backwards to pick up units before values */
	for (i = nf - 1; i >= 0; i--)
	{
		switch (ftype[i])
		{
			case DTK_TIME:
				dterr = DecodeTime(field[i], fmask, range,
								   &tmask, tm, fsec);
				if (dterr)
					return dterr;
				type = DTK_DAY;
				break;

			case DTK_TZ:

				/*
				 * Timezone means a token with a leading sign character and at
				 * least one digit; there could be ':', '.', '-' embedded in
				 * it as well.
				 */
				Assert(*field[i] == '-' || *field[i] == '+');

				/*
				 * Check for signed hh:mm or hh:mm:ss.  If so, process exactly
				 * like DTK_TIME case above, plus handling the sign.
				 */
				if (strchr(field[i] + 1, ':') != NULL &&
					DecodeTime(field[i] + 1, fmask, range,
							   &tmask, tm, fsec) == 0)
				{
					if (*field[i] == '-')
					{
						/* flip the sign on all fields */
						tm->tm_hour = -tm->tm_hour;
						tm->tm_min = -tm->tm_min;
						tm->tm_sec = -tm->tm_sec;
						*fsec = -(*fsec);
					}

					/*
					 * Set the next type to be a day, if units are not
					 * specified. This handles the case of '1 +02:03' since we
					 * are reading right to left.
					 */
					type = DTK_DAY;
					break;
				}

				/*
				 * Otherwise, fall through to DTK_NUMBER case, which can
				 * handle signed float numbers and signed year-month values.
				 */

				/* FALLTHROUGH */

			case DTK_DATE:
			case DTK_NUMBER:
				if (type == IGNORE_DTF)
				{
					/* use typmod to decide what rightmost field is */
					switch (range)
					{
						case INTERVAL_MASK(YEAR):
							type = DTK_YEAR;
							break;
						case INTERVAL_MASK(MONTH):
						case INTERVAL_MASK(YEAR) | INTERVAL_MASK(MONTH):
							type = DTK_MONTH;
							break;
						case INTERVAL_MASK(DAY):
							type = DTK_DAY;
							break;
						case INTERVAL_MASK(HOUR):
						case INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR):
							type = DTK_HOUR;
							break;
						case INTERVAL_MASK(MINUTE):
						case INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE):
						case INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE):
							type = DTK_MINUTE;
							break;
						case INTERVAL_MASK(SECOND):
						case INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND):
						case INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND):
						case INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND):
							type = DTK_SECOND;
							break;
						/* Begin - bug#8026 */
						case INTERVAL_MASK(MICROSECOND):
							type = DTK_MICROSEC;
							break;
						/* End - bug#8026 */
						default:
							type = DTK_SECOND;
							break;
					}
				}

				errno = 0;
				val = strtoint(field[i], &cp, 10);
				if (errno == ERANGE)
					return DTERR_FIELD_OVERFLOW;

				/* Begin - bug#8026 */
				if (compatible_mode == MYS_MODE &&
					(range == (INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR)) ||
					 range == (INTERVAL_MASK(YEAR) | INTERVAL_MASK(MONTH))))
				{
					int			val2 = 0;
					int 		mulit = 0;

					if (type == DTK_HOUR)
						mulit = HOURS_PER_DAY;
					else if (type == DTK_MONTH)
						mulit = MONTHS_PER_YEAR;

					if (*cp != '\0')
					{
						val2 = strtoint(cp + 1, &cp, 10);
						if (errno == ERANGE || val2 < 0)
							return DTERR_FIELD_OVERFLOW;
						if (*cp != '\0')
							return DTERR_BAD_FORMAT;
						
						if (*field[i] == '-')
							val2 = -val2;
						
						if (((double) val * mulit + val2) > INT_MAX ||
							((double) val * mulit + val2) < INT_MIN)
							return DTERR_FIELD_OVERFLOW;
						val = val * mulit + val2;
						fval = 0;
					}
				}
				/* End - bug#8026 */
				else if (*cp == '-')
				{
					/* SQL "years-months" syntax */
					int			val2;

					val2 = strtoint(cp + 1, &cp, 10);
					if (errno == ERANGE || val2 < 0 || val2 >= MONTHS_PER_YEAR)
						return DTERR_FIELD_OVERFLOW;
					if (*cp != '\0')
						return DTERR_BAD_FORMAT;
					type = DTK_MONTH;
					if (*field[i] == '-')
						val2 = -val2;
					if (((double) val * MONTHS_PER_YEAR + val2) > INT_MAX ||
						((double) val * MONTHS_PER_YEAR + val2) < INT_MIN)
						return DTERR_FIELD_OVERFLOW;
					val = val * MONTHS_PER_YEAR + val2;
					fval = 0;
				}
				else if (*cp == '.')
				{
					errno = 0;
					fval = strtod(cp, &cp);
					if (*cp != '\0' || errno != 0)
						return DTERR_BAD_FORMAT;

					if (*field[i] == '-')
						fval = -fval;
				}
				else if (*cp == '\0')
					fval = 0;
				else
					return DTERR_BAD_FORMAT;

				tmask = 0;		/* DTK_M(type); */

				switch (type)
				{
					case DTK_MICROSEC:
						*fsec += rint(val + fval);
						tmask = DTK_M(MICROSECOND);
						break;

					case DTK_MILLISEC:
						/* avoid overflowing the fsec field */
						tm->tm_sec += val / 1000;
						val -= (val / 1000) * 1000;
						*fsec += rint((val + fval) * 1000);
						tmask = DTK_M(MILLISECOND);
						break;

					case DTK_SECOND:
						tm->tm_sec += val;
						*fsec += rint(fval * 1000000);

						/*
						 * If any subseconds were specified, consider this
						 * microsecond and millisecond input as well.
						 */
						if (fval == 0)
							tmask = DTK_M(SECOND);
						else
							tmask = DTK_ALL_SECS_M;
						break;

					case DTK_MINUTE:
						tm->tm_min += val;
						AdjustFractSeconds(fval, tm, fsec, SECS_PER_MINUTE);
						tmask = DTK_M(MINUTE);
						break;

					case DTK_HOUR:
						tm->tm_hour += val;
						AdjustFractSeconds(fval, tm, fsec, SECS_PER_HOUR);
						tmask = DTK_M(HOUR);
						type = DTK_DAY; /* set for next field */
						break;

					case DTK_DAY:
						tm->tm_mday += val;
						AdjustFractSeconds(fval, tm, fsec, SECS_PER_DAY);
						tmask = DTK_M(DAY);
						break;

					case DTK_WEEK:
						tm->tm_mday += val * 7;
						AdjustFractDays(fval, tm, fsec, 7);
						tmask = DTK_M(WEEK);
						break;

					case DTK_MONTH:
						tm->tm_mon += val;
						AdjustFractDays(fval, tm, fsec, DAYS_PER_MONTH);
						tmask = DTK_M(MONTH);
						break;

					case DTK_YEAR:
						tm->tm_year += val;
						if (fval != 0)
							tm->tm_mon += fval * MONTHS_PER_YEAR;
						tmask = DTK_M(YEAR);
						break;

					case DTK_DECADE:
						tm->tm_year += val * 10;
						if (fval != 0)
							tm->tm_mon += fval * MONTHS_PER_YEAR * 10;
						tmask = DTK_M(DECADE);
						break;

					case DTK_CENTURY:
						tm->tm_year += val * 100;
						if (fval != 0)
							tm->tm_mon += fval * MONTHS_PER_YEAR * 100;
						tmask = DTK_M(CENTURY);
						break;

					case DTK_MILLENNIUM:
						tm->tm_year += val * 1000;
						if (fval != 0)
							tm->tm_mon += fval * MONTHS_PER_YEAR * 1000;
						tmask = DTK_M(MILLENNIUM);
						break;

					default:
						return DTERR_BAD_FORMAT;
				}
				break;

			case DTK_STRING:
			case DTK_SPECIAL:
				type = DecodeUnits(i, field[i], &val);
				if (type == IGNORE_DTF)
					continue;

				tmask = 0;		/* DTK_M(type); */
				switch (type)
				{
					case UNITS:
						type = val;
						break;

					case AGO:
						is_before = true;
						type = val;
						break;

					case RESERV:
						tmask = (DTK_DATE_M | DTK_TIME_M);
						*dtype = val;
						break;

					default:
						return DTERR_BAD_FORMAT;
				}
				break;

			default:
				return DTERR_BAD_FORMAT;
		}

		if (tmask & fmask)
			return DTERR_BAD_FORMAT;
		fmask |= tmask;
	}

	/* ensure that at least one time field has been found */
	if (fmask == 0)
		return DTERR_BAD_FORMAT;

	/* ensure fractional seconds are fractional */
	if (*fsec != 0)
	{
		int			sec;

		sec = *fsec / USECS_PER_SEC;
		*fsec -= sec * USECS_PER_SEC;
		tm->tm_sec += sec;
	}

	/*----------
	 * The SQL standard defines the interval literal
	 *	 '-1 1:00:00'
	 * to mean "negative 1 days and negative 1 hours", while Postgres
	 * traditionally treats this as meaning "negative 1 days and positive
	 * 1 hours".  In SQL_STANDARD intervalstyle, we apply the leading sign
	 * to all fields if there are no other explicit signs.
	 *
	 * We leave the signs alone if there are additional explicit signs.
	 * This protects us against misinterpreting postgres-style dump output,
	 * since the postgres-style output code has always put an explicit sign on
	 * all fields following a negative field.  But note that SQL-spec output
	 * is ambiguous and can be misinterpreted on load!	(So it's best practice
	 * to dump in postgres style, not SQL style.)
	 *----------
	 */
	if (IntervalStyle == INTSTYLE_SQL_STANDARD && *field[0] == '-')
	{
		/* Check for additional explicit signs */
		bool		more_signs = false;

		for (i = 1; i < nf; i++)
		{
			if (*field[i] == '-' || *field[i] == '+')
			{
				more_signs = true;
				break;
			}
		}

		if (!more_signs)
		{
			/*
			 * Rather than re-determining which field was field[0], just force
			 * 'em all negative.
			 */
			if (*fsec > 0)
				*fsec = -(*fsec);
			if (tm->tm_sec > 0)
				tm->tm_sec = -tm->tm_sec;
			if (tm->tm_min > 0)
				tm->tm_min = -tm->tm_min;
			if (tm->tm_hour > 0)
				tm->tm_hour = -tm->tm_hour;
			if (tm->tm_mday > 0)
				tm->tm_mday = -tm->tm_mday;
			if (tm->tm_mon > 0)
				tm->tm_mon = -tm->tm_mon;
			if (tm->tm_year > 0)
				tm->tm_year = -tm->tm_year;
		}
	}

	/* finally, AGO negates everything */
	if (is_before)
	{
		*fsec = -(*fsec);
		tm->tm_sec = -tm->tm_sec;
		tm->tm_min = -tm->tm_min;
		tm->tm_hour = -tm->tm_hour;
		tm->tm_mday = -tm->tm_mday;
		tm->tm_mon = -tm->tm_mon;
		tm->tm_year = -tm->tm_year;
	}

	return 0;
}


/*
 * Helper functions to avoid duplicated code in DecodeISO8601Interval.
 *
 * Parse a decimal value and break it into integer and fractional parts.
 * Returns 0 or DTERR code.
 */
static int
ParseISO8601Number(char *str, char **endptr, int *ipart, double *fpart)
{
	double		val;

	if (!(isdigit((unsigned char) *str) || *str == '-' || *str == '.'))
		return DTERR_BAD_FORMAT;
	errno = 0;
	val = strtod(str, endptr);
	/* did we not see anything that looks like a double? */
	if (*endptr == str || errno != 0)
		return DTERR_BAD_FORMAT;
	/* watch out for overflow */
	if (val < INT_MIN || val > INT_MAX)
		return DTERR_FIELD_OVERFLOW;
	/* be very sure we truncate towards zero (cf dtrunc()) */
	if (val >= 0)
		*ipart = (int) floor(val);
	else
		*ipart = (int) -floor(-val);
	*fpart = val - *ipart;
	return 0;
}

/*
 * Determine number of integral digits in a valid ISO 8601 number field
 * (we should ignore sign and any fraction part)
 */
static int
ISO8601IntegerWidth(char *fieldstart)
{
	/* We might have had a leading '-' */
	if (*fieldstart == '-')
		fieldstart++;
	return strspn(fieldstart, "0123456789");
}


/* DecodeISO8601Interval()
 *	Decode an ISO 8601 time interval of the "format with designators"
 *	(section 4.4.3.2) or "alternative format" (section 4.4.3.3)
 *	Examples:  P1D	for 1 day
 *			   PT1H for 1 hour
 *			   P2Y6M7DT1H30M for 2 years, 6 months, 7 days 1 hour 30 min
 *			   P0002-06-07T01:30:00 the same value in alternative format
 *
 * Returns 0 if successful, DTERR code if bogus input detected.
 * Note: error code should be DTERR_BAD_FORMAT if input doesn't look like
 * ISO8601, otherwise this could cause unexpected error messages.
 * dtype, tm, fsec are output parameters.
 *
 *	A couple exceptions from the spec:
 *	 - a week field ('W') may coexist with other units
 *	 - allows decimals in fields other than the least significant unit.
 */
int
DecodeISO8601Interval(char *str,
					  int *dtype, struct pg_tm *tm, fsec_t *fsec)
{
	bool		datepart = true;
	bool		havefield = false;

	*dtype = DTK_DELTA;
	ClearPgTm(tm, fsec);

	if (strlen(str) < 2 || str[0] != 'P')
		return DTERR_BAD_FORMAT;

	str++;
	while (*str)
	{
		char	   *fieldstart;
		int			val;
		double		fval = 0;
		char		unit;
		int			dterr;

		if (*str == 'T')		/* T indicates the beginning of the time part */
		{
			datepart = false;
			havefield = false;
			str++;
			continue;
		}

		fieldstart = str;
		dterr = ParseISO8601Number(str, &str, &val, &fval);
		if (dterr)
			return dterr;

		/*
		 * Note: we could step off the end of the string here.  Code below
		 * *must* exit the loop if unit == '\0'.
		 */
		unit = *str++;

		if (datepart)
		{
			switch (unit)		/* before T: Y M W D */
			{
				case 'Y':
					tm->tm_year += val;
					tm->tm_mon += (fval * MONTHS_PER_YEAR);
					break;
				case 'M':
					tm->tm_mon += val;
					AdjustFractDays(fval, tm, fsec, DAYS_PER_MONTH);
					break;
				case 'W':
					tm->tm_mday += val * 7;
					AdjustFractDays(fval, tm, fsec, 7);
					break;
				case 'D':
					tm->tm_mday += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_DAY);
					break;
				case 'T':		/* ISO 8601 4.4.3.3 Alternative Format / Basic */
				case '\0':
					if (ISO8601IntegerWidth(fieldstart) == 8 && !havefield)
					{
						tm->tm_year += val / 10000;
						tm->tm_mon += (val / 100) % 100;
						tm->tm_mday += val % 100;
						AdjustFractSeconds(fval, tm, fsec, SECS_PER_DAY);
						if (unit == '\0')
							return 0;
						datepart = false;
						havefield = false;
						continue;
					}
					/* Else fall through to extended alternative format */
					/* FALLTHROUGH */
				case '-':		/* ISO 8601 4.4.3.3 Alternative Format,
								 * Extended */
					if (havefield)
						return DTERR_BAD_FORMAT;

					tm->tm_year += val;
					tm->tm_mon += (fval * MONTHS_PER_YEAR);
					if (unit == '\0')
						return 0;
					if (unit == 'T')
					{
						datepart = false;
						havefield = false;
						continue;
					}

					dterr = ParseISO8601Number(str, &str, &val, &fval);
					if (dterr)
						return dterr;
					tm->tm_mon += val;
					AdjustFractDays(fval, tm, fsec, DAYS_PER_MONTH);
					if (*str == '\0')
						return 0;
					if (*str == 'T')
					{
						datepart = false;
						havefield = false;
						continue;
					}
					if (*str != '-')
						return DTERR_BAD_FORMAT;
					str++;

					dterr = ParseISO8601Number(str, &str, &val, &fval);
					if (dterr)
						return dterr;
					tm->tm_mday += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_DAY);
					if (*str == '\0')
						return 0;
					if (*str == 'T')
					{
						datepart = false;
						havefield = false;
						continue;
					}
					return DTERR_BAD_FORMAT;
				default:
					/* not a valid date unit suffix */
					return DTERR_BAD_FORMAT;
			}
		}
		else
		{
			switch (unit)		/* after T: H M S */
			{
				case 'H':
					tm->tm_hour += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_HOUR);
					break;
				case 'M':
					tm->tm_min += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_MINUTE);
					break;
				case 'S':
					tm->tm_sec += val;
					AdjustFractSeconds(fval, tm, fsec, 1);
					break;
				case '\0':		/* ISO 8601 4.4.3.3 Alternative Format */
					if (ISO8601IntegerWidth(fieldstart) == 6 && !havefield)
					{
						tm->tm_hour += val / 10000;
						tm->tm_min += (val / 100) % 100;
						tm->tm_sec += val % 100;
						AdjustFractSeconds(fval, tm, fsec, 1);
						return 0;
					}
					/* Else fall through to extended alternative format */
					/* FALLTHROUGH */
				case ':':		/* ISO 8601 4.4.3.3 Alternative Format,
								 * Extended */
					if (havefield)
						return DTERR_BAD_FORMAT;

					tm->tm_hour += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_HOUR);
					if (unit == '\0')
						return 0;

					dterr = ParseISO8601Number(str, &str, &val, &fval);
					if (dterr)
						return dterr;
					tm->tm_min += val;
					AdjustFractSeconds(fval, tm, fsec, SECS_PER_MINUTE);
					if (*str == '\0')
						return 0;
					if (*str != ':')
						return DTERR_BAD_FORMAT;
					str++;

					dterr = ParseISO8601Number(str, &str, &val, &fval);
					if (dterr)
						return dterr;
					tm->tm_sec += val;
					AdjustFractSeconds(fval, tm, fsec, 1);
					if (*str == '\0')
						return 0;
					return DTERR_BAD_FORMAT;

				default:
					/* not a valid time unit suffix */
					return DTERR_BAD_FORMAT;
			}
		}

		havefield = true;
	}

	return 0;
}


/* DecodeUnits()
 * Decode text string using lookup table.
 *
 * This routine recognizes keywords associated with time interval units.
 *
 * Given string must be lowercased already.
 *
 * Implement a cache lookup since it is likely that dates
 *	will be related in format.
 */
int
DecodeUnits(int field, char *lowtoken, int *val)
{
	int			type;
	const datetkn *tp;

	tp = deltacache[field];
	/* use strncmp so that we match truncated tokens */
	if (tp == NULL || strncmp(lowtoken, tp->token, TOKMAXLEN) != 0)
	{
		tp = datebsearch(lowtoken, deltatktbl, szdeltatktbl);
	}
	if (tp == NULL)
	{
		type = UNKNOWN_FIELD;
		*val = 0;
	}
	else
	{
		deltacache[field] = tp;
		type = tp->type;
		*val = tp->value;
	}

	return type;
}								/* DecodeUnits() */

/*
 * Report an error detected by one of the datetime input processing routines.
 *
 * dterr is the error code, str is the original input string, datatype is
 * the name of the datatype we were trying to accept.
 *
 * Note: it might seem useless to distinguish DTERR_INTERVAL_OVERFLOW and
 * DTERR_TZDISP_OVERFLOW from DTERR_FIELD_OVERFLOW, but SQL99 mandates three
 * separate SQLSTATE codes, so ...
 */
void
DateTimeParseError(int dterr, const char *str, const char *datatype)
{
	switch (dterr)
	{
		case DTERR_FIELD_OVERFLOW:
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
					 errmsg("date/time field value out of range: \"%s\"",
							str)));
			break;
		case DTERR_MD_FIELD_OVERFLOW:
			/* <nanny>same as above, but add hint about DateStyle</nanny> */
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_FIELD_OVERFLOW),
					 errmsg("date/time field value out of range: \"%s\"",
							str),
					 errhint("Perhaps you need a different \"datestyle\" setting.")));
			break;
		case DTERR_INTERVAL_OVERFLOW:
			ereport(ERROR,
					(errcode(ERRCODE_INTERVAL_FIELD_OVERFLOW),
					 errmsg("interval field value out of range: \"%s\"",
							str)));
			break;
		case DTERR_TZDISP_OVERFLOW:
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TIME_ZONE_DISPLACEMENT_VALUE),
					 errmsg("time zone displacement out of range: \"%s\"",
							str)));
			break;
		case DTERR_BAD_FORMAT:
		default:
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_DATETIME_FORMAT),
					 errmsg("invalid input syntax for type %s: \"%s\"",
							datatype, str)));
			break;
	}
}

/* datebsearch()
 * Binary search -- from Knuth (6.2.1) Algorithm B.  Special case like this
 * is WAY faster than the generic bsearch().
 */
static const datetkn *
datebsearch(const char *key, const datetkn *base, int nel)
{
	if (nel > 0)
	{
		const datetkn *last = base + nel - 1,
				   *position;
		int			result;

		while (last >= base)
		{
			position = base + ((last - base) >> 1);
			/* precheck the first character for a bit of extra speed */
			result = (int) key[0] - (int) position->token[0];
			if (result == 0)
			{
				/* use strncmp so that we match truncated tokens */
				result = strncmp(key, position->token, TOKMAXLEN);
				if (result == 0)
					return position;
			}
			if (result < 0)
				last = position - 1;
			else
				base = position + 1;
		}
	}
	return NULL;
}

/* EncodeTimezone()
 *		Copies representation of a numeric timezone offset to str.
 *
 * Returns a pointer to the new end of string.  No NUL terminator is put
 * there; callers are responsible for NUL terminating str themselves.
 */
static char *
EncodeTimezone(char *str, int tz, int style)
{
	int			hour,
				min,
				sec;

	sec = abs(tz);
	min = sec / SECS_PER_MINUTE;
	sec -= min * SECS_PER_MINUTE;
	hour = min / MINS_PER_HOUR;
	min -= hour * MINS_PER_HOUR;

	/* TZ is negated compared to sign we wish to display ... */
	*str++ = (tz <= 0 ? '+' : '-');

	if (sec != 0)
	{
		str = pg_ultostr_zeropad(str, hour, 2);
		*str++ = ':';
		str = pg_ultostr_zeropad(str, min, 2);
		*str++ = ':';
		str = pg_ultostr_zeropad(str, sec, 2);
	}
	else if (min != 0 || style == USE_XSD_DATES)
	{
		str = pg_ultostr_zeropad(str, hour, 2);
		*str++ = ':';
		str = pg_ultostr_zeropad(str, min, 2);
	}
	else
		str = pg_ultostr_zeropad(str, hour, 2);
	return str;
}

/* EncodeDateOnly()
 * Encode date as local time.
 */
void
EncodeDateOnly(struct pg_tm *tm, int style, char *str)
{
	Assert(tm->tm_mon >= 1 && tm->tm_mon <= MONTHS_PER_YEAR);

	switch (style)
	{
		case USE_ISO_DATES:
		case USE_XSD_DATES:
			/* compatible with ISO date formats */
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			*str++ = '-';
			str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			*str++ = '-';
			str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			break;

		case USE_SQL_DATES:
			/* compatible with Oracle/Ingres date formats */
			if (DateOrder == DATEORDER_DMY)
			{
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
				*str++ = '/';
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			}
			else
			{
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
				*str++ = '/';
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			}
			*str++ = '/';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			break;

		case USE_GERMAN_DATES:
			/* German-style date format */
			str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			*str++ = '.';
			str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			*str++ = '.';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			break;

		case USE_POSTGRES_DATES:
		default:
			/* traditional date-only style for Postgres */
			if (DateOrder == DATEORDER_DMY)
			{
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
				*str++ = '-';
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			}
			else
			{
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
				*str++ = '-';
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			}
			*str++ = '-';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			break;
	}

	if (tm->tm_year <= 0)
	{
		memcpy(str, " BC", 3);	/* Don't copy NUL */
		str += 3;
	}
	*str = '\0';
}


/* EncodeTimeOnly()
 * Encode time fields only.
 *
 * tm and fsec are the value to encode, print_tz determines whether to include
 * a time zone (the difference between time and timetz types), tz is the
 * numeric time zone offset, style is the date style, str is where to write the
 * output.
 */
void
EncodeTimeOnly(struct pg_tm *tm, fsec_t fsec, bool print_tz, int tz, int style, char *str)
{
	str = pg_ultostr_zeropad(str, tm->tm_hour, 2);
	*str++ = ':';
	str = pg_ultostr_zeropad(str, tm->tm_min, 2);
	*str++ = ':';
	str = AppendSeconds(str, tm->tm_sec, fsec, MAX_TIME_PRECISION, true);
	if (print_tz)
		str = EncodeTimezone(str, tz, style);
	*str = '\0';
}


/* EncodeDateTime()
 * Encode date and time interpreted as local time.
 *
 * tm and fsec are the value to encode, print_tz determines whether to include
 * a time zone (the difference between timestamp and timestamptz types), tz is
 * the numeric time zone offset, tzn is the textual time zone, which if
 * specified will be used instead of tz by some styles, style is the date
 * style, str is where to write the output.
 *
 * Supported date styles:
 *	Postgres - day mon hh:mm:ss yyyy tz
 *	SQL - mm/dd/yyyy hh:mm:ss.ss tz
 *	ISO - yyyy-mm-dd hh:mm:ss+/-tz
 *	German - dd.mm.yyyy hh:mm:ss tz
 *	XSD - yyyy-mm-ddThh:mm:ss.ss+/-tz
 */
void
EncodeDateTime(struct pg_tm *tm, fsec_t fsec, bool print_tz, int tz, const char *tzn, int style, char *str)
{
	int			day;

	Assert(tm->tm_mon >= 1 && tm->tm_mon <= MONTHS_PER_YEAR);

	/*
	 * Negative tm_isdst means we have no valid time zone translation.
	 */
	if (tm->tm_isdst < 0)
		print_tz = false;

	switch (style)
	{
		case USE_ISO_DATES:
		case USE_XSD_DATES:
			/* Compatible with ISO-8601 date formats */
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			*str++ = '-';
			str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			*str++ = '-';
			str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			*str++ = (style == USE_ISO_DATES) ? ' ' : 'T';
			str = pg_ultostr_zeropad(str, tm->tm_hour, 2);
			*str++ = ':';
			str = pg_ultostr_zeropad(str, tm->tm_min, 2);
			*str++ = ':';
			str = AppendTimestampSeconds(str, tm, fsec);
			if (print_tz)
				str = EncodeTimezone(str, tz, style);
			break;

		case USE_SQL_DATES:
			/* Compatible with Oracle/Ingres date formats */
			if (DateOrder == DATEORDER_DMY)
			{
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
				*str++ = '/';
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			}
			else
			{
				str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
				*str++ = '/';
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			}
			*str++ = '/';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			*str++ = ' ';
			str = pg_ultostr_zeropad(str, tm->tm_hour, 2);
			*str++ = ':';
			str = pg_ultostr_zeropad(str, tm->tm_min, 2);
			*str++ = ':';
			str = AppendTimestampSeconds(str, tm, fsec);

			/*
			 * Note: the uses of %.*s in this function would be risky if the
			 * timezone names ever contain non-ASCII characters, since we are
			 * not being careful to do encoding-aware clipping.  However, all
			 * TZ abbreviations in the IANA database are plain ASCII.
			 */
			if (print_tz)
			{
				if (tzn)
				{
					sprintf(str, " %.*s", MAXTZLEN, tzn);
					str += strlen(str);
				}
				else
					str = EncodeTimezone(str, tz, style);
			}
			break;

		case USE_GERMAN_DATES:
			/* German variant on European style */
			str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			*str++ = '.';
			str = pg_ultostr_zeropad(str, tm->tm_mon, 2);
			*str++ = '.';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);
			*str++ = ' ';
			str = pg_ultostr_zeropad(str, tm->tm_hour, 2);
			*str++ = ':';
			str = pg_ultostr_zeropad(str, tm->tm_min, 2);
			*str++ = ':';
			str = AppendTimestampSeconds(str, tm, fsec);

			if (print_tz)
			{
				if (tzn)
				{
					sprintf(str, " %.*s", MAXTZLEN, tzn);
					str += strlen(str);
				}
				else
					str = EncodeTimezone(str, tz, style);
			}
			break;

		case USE_POSTGRES_DATES:
		default:
			/* Backward-compatible with traditional Postgres abstime dates */
			day = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday);
			tm->tm_wday = j2day(day);
			memcpy(str, days[tm->tm_wday], 3);
			str += 3;
			*str++ = ' ';
			if (DateOrder == DATEORDER_DMY)
			{
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
				*str++ = ' ';
				memcpy(str, months[tm->tm_mon - 1], 3);
				str += 3;
			}
			else
			{
				memcpy(str, months[tm->tm_mon - 1], 3);
				str += 3;
				*str++ = ' ';
				str = pg_ultostr_zeropad(str, tm->tm_mday, 2);
			}
			*str++ = ' ';
			str = pg_ultostr_zeropad(str, tm->tm_hour, 2);
			*str++ = ':';
			str = pg_ultostr_zeropad(str, tm->tm_min, 2);
			*str++ = ':';
			str = AppendTimestampSeconds(str, tm, fsec);
			*str++ = ' ';
			str = pg_ultostr_zeropad(str,
									 (tm->tm_year > 0) ? tm->tm_year : -(tm->tm_year - 1), 4);

			if (print_tz)
			{
				if (tzn)
				{
					sprintf(str, " %.*s", MAXTZLEN, tzn);
					str += strlen(str);
				}
				else
				{
					/*
					 * We have a time zone, but no string version. Use the
					 * numeric form, but be sure to include a leading space to
					 * avoid formatting something which would be rejected by
					 * the date/time parser later. - thomas 2001-10-19
					 */
					*str++ = ' ';
					str = EncodeTimezone(str, tz, style);
				}
			}
			break;
	}

	if (tm->tm_year <= 0)
	{
		memcpy(str, " BC", 3);	/* Don't copy NUL */
		str += 3;
	}
	*str = '\0';
}


/*
 * Helper functions to avoid duplicated code in EncodeInterval.
 */

/* Append an ISO-8601-style interval field, but only if value isn't zero */
static char *
AddISO8601IntPart(char *cp, int value, char units)
{
	if (value == 0)
		return cp;
	sprintf(cp, "%d%c", value, units);
	return cp + strlen(cp);
}

/* Append a postgres-style interval field, but only if value isn't zero */
static char *
AddPostgresIntPart(char *cp, int value, const char *units,
				   bool *is_zero, bool *is_before)
{
	if (value == 0)
		return cp;
	sprintf(cp, "%s%s%d %s%s",
			(!*is_zero) ? " " : "",
			(*is_before && value > 0) ? "+" : "",
			value,
			units,
			(value != 1) ? "s" : "");

	/*
	 * Each nonzero field sets is_before for (only) the next one.  This is a
	 * tad bizarre but it's how it worked before...
	 */
	*is_before = (value < 0);
	*is_zero = false;
	return cp + strlen(cp);
}

/* Append a verbose-style interval field, but only if value isn't zero */
static char *
AddVerboseIntPart(char *cp, int value, const char *units,
				  bool *is_zero, bool *is_before)
{
	if (value == 0)
		return cp;
	/* first nonzero value sets is_before */
	if (*is_zero)
	{
		*is_before = (value < 0);
		value = abs(value);
	}
	else if (*is_before)
		value = -value;
	sprintf(cp, " %d %s%s", value, units, (value == 1) ? "" : "s");
	*is_zero = false;
	return cp + strlen(cp);
}


/* EncodeInterval()
 * Interpret time structure as a delta time and convert to string.
 *
 * Support "traditional Postgres" and ISO-8601 styles.
 * Actually, afaik ISO does not address time interval formatting,
 *	but this looks similar to the spec for absolute date/time.
 * - thomas 1998-04-30
 *
 * Actually, afaik, ISO 8601 does specify formats for "time
 * intervals...[of the]...format with time-unit designators", which
 * are pretty ugly.  The format looks something like
 *	   P1Y1M1DT1H1M1.12345S
 * but useful for exchanging data with computers instead of humans.
 * - ron 2003-07-14
 *
 * And ISO's SQL 2008 standard specifies standards for
 * "year-month literal"s (that look like '2-3') and
 * "day-time literal"s (that look like ('4 5:6:7')
 */
void
EncodeInterval(struct pg_tm *tm, fsec_t fsec, int style, char *str)
{
	char	   *cp = str;
	int			year = tm->tm_year;
	int			mon = tm->tm_mon;
	int			mday = tm->tm_mday;
	int			hour = tm->tm_hour;
	int			min = tm->tm_min;
	int			sec = tm->tm_sec;
	bool		is_before = false;
	bool		is_zero = true;

	/*
	 * The sign of year and month are guaranteed to match, since they are
	 * stored internally as "month". But we'll need to check for is_before and
	 * is_zero when determining the signs of day and hour/minute/seconds
	 * fields.
	 */
	switch (style)
	{
			/* SQL Standard interval format */
		case INTSTYLE_SQL_STANDARD:
			{
				bool		has_negative = year < 0 || mon < 0 ||
				mday < 0 || hour < 0 ||
				min < 0 || sec < 0 || fsec < 0;
				bool		has_positive = year > 0 || mon > 0 ||
				mday > 0 || hour > 0 ||
				min > 0 || sec > 0 || fsec > 0;
				bool		has_year_month = year != 0 || mon != 0;
				bool		has_day_time = mday != 0 || hour != 0 ||
				min != 0 || sec != 0 || fsec != 0;
				bool		has_day = mday != 0;
				bool		sql_standard_value = !(has_negative && has_positive) &&
				!(has_year_month && has_day_time);

				/*
				 * SQL Standard wants only 1 "<sign>" preceding the whole
				 * interval ... but can't do that if mixed signs.
				 */
				if (has_negative && sql_standard_value)
				{
					*cp++ = '-';
					year = -year;
					mon = -mon;
					mday = -mday;
					hour = -hour;
					min = -min;
					sec = -sec;
					fsec = -fsec;
				}

				if (!has_negative && !has_positive)
				{
					sprintf(cp, "0");
				}
				else if (!sql_standard_value)
				{
					/*
					 * For non sql-standard interval values, force outputting
					 * the signs to avoid ambiguities with intervals with
					 * mixed sign components.
					 */
					char		year_sign = (year < 0 || mon < 0) ? '-' : '+';
					char		day_sign = (mday < 0) ? '-' : '+';
					char		sec_sign = (hour < 0 || min < 0 ||
											sec < 0 || fsec < 0) ? '-' : '+';

					sprintf(cp, "%c%d-%d %c%d %c%d:%02d:",
							year_sign, abs(year), abs(mon),
							day_sign, abs(mday),
							sec_sign, abs(hour), abs(min));
					cp += strlen(cp);
					cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, true);
					*cp = '\0';
				}
				else if (has_year_month)
				{
					sprintf(cp, "%d-%d", year, mon);
				}
				else if (has_day)
				{
					sprintf(cp, "%d %d:%02d:", mday, hour, min);
					cp += strlen(cp);
					cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, true);
					*cp = '\0';
				}
				else
				{
					sprintf(cp, "%d:%02d:", hour, min);
					cp += strlen(cp);
					cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, true);
					*cp = '\0';
				}
			}
			break;

			/* ISO 8601 "time-intervals by duration only" */
		case INTSTYLE_ISO_8601:
			/* special-case zero to avoid printing nothing */
			if (year == 0 && mon == 0 && mday == 0 &&
				hour == 0 && min == 0 && sec == 0 && fsec == 0)
			{
				sprintf(cp, "PT0S");
				break;
			}
			*cp++ = 'P';
			cp = AddISO8601IntPart(cp, year, 'Y');
			cp = AddISO8601IntPart(cp, mon, 'M');
			cp = AddISO8601IntPart(cp, mday, 'D');
			if (hour != 0 || min != 0 || sec != 0 || fsec != 0)
				*cp++ = 'T';
			cp = AddISO8601IntPart(cp, hour, 'H');
			cp = AddISO8601IntPart(cp, min, 'M');
			if (sec != 0 || fsec != 0)
			{
				if (sec < 0 || fsec < 0)
					*cp++ = '-';
				cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, false);
				*cp++ = 'S';
				*cp++ = '\0';
			}
			break;

			/* Compatible with postgresql < 8.4 when DateStyle = 'iso' */
		case INTSTYLE_POSTGRES:
			cp = AddPostgresIntPart(cp, year, "year", &is_zero, &is_before);

			/*
			 * Ideally we should spell out "month" like we do for "year" and
			 * "day".  However, for backward compatibility, we can't easily
			 * fix this.  bjm 2011-05-24
			 */
			cp = AddPostgresIntPart(cp, mon, "mon", &is_zero, &is_before);
			cp = AddPostgresIntPart(cp, mday, "day", &is_zero, &is_before);
			if (is_zero || hour != 0 || min != 0 || sec != 0 || fsec != 0)
			{
				bool		minus = (hour < 0 || min < 0 || sec < 0 || fsec < 0);

				sprintf(cp, "%s%s%02d:%02d:",
						is_zero ? "" : " ",
						(minus ? "-" : (is_before ? "+" : "")),
						abs(hour), abs(min));
				cp += strlen(cp);
				cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, true);
				*cp = '\0';
			}
			break;

			/* Compatible with postgresql < 8.4 when DateStyle != 'iso' */
		case INTSTYLE_POSTGRES_VERBOSE:
		default:
			strcpy(cp, "@");
			cp++;
			cp = AddVerboseIntPart(cp, year, "year", &is_zero, &is_before);
			cp = AddVerboseIntPart(cp, mon, "mon", &is_zero, &is_before);
			cp = AddVerboseIntPart(cp, mday, "day", &is_zero, &is_before);
			cp = AddVerboseIntPart(cp, hour, "hour", &is_zero, &is_before);
			cp = AddVerboseIntPart(cp, min, "min", &is_zero, &is_before);
			if (sec != 0 || fsec != 0)
			{
				*cp++ = ' ';
				if (sec < 0 || (sec == 0 && fsec < 0))
				{
					if (is_zero)
						is_before = true;
					else if (!is_before)
						*cp++ = '-';
				}
				else if (is_before)
					*cp++ = '-';
				cp = AppendSeconds(cp, sec, fsec, MAX_INTERVAL_PRECISION, false);
				/* We output "ago", not negatives, so use abs(). */
				sprintf(cp, " sec%s",
						(abs(sec) != 1 || fsec != 0) ? "s" : "");
				is_zero = false;
			}
			/* identically zero? then put in a unitless zero... */
			if (is_zero)
				strcat(cp, " 0");
			if (is_before)
				strcat(cp, " ago");
			break;
	}
}


/*
 * We've been burnt by stupid errors in the ordering of the datetkn tables
 * once too often.  Arrange to check them during postmaster start.
 */
static bool
CheckDateTokenTable(const char *tablename, const datetkn *base, int nel)
{
	bool		ok = true;
	int			i;

	for (i = 0; i < nel; i++)
	{
		/* check for token strings that don't fit */
		if (strlen(base[i].token) > TOKMAXLEN)
		{
			/* %.*s is safe since all our tokens are ASCII */
			elog(LOG, "token too long in %s table: \"%.*s\"",
				 tablename,
				 TOKMAXLEN + 1, base[i].token);
			ok = false;
			break;				/* don't risk applying strcmp */
		}
		/* check for out of order */
		if (i > 0 &&
			strcmp(base[i - 1].token, base[i].token) >= 0)
		{
			elog(LOG, "ordering error in %s table: \"%s\" >= \"%s\"",
				 tablename,
				 base[i - 1].token,
				 base[i].token);
			ok = false;
		}
	}
	return ok;
}

bool
CheckDateTokenTables(void)
{
	bool		ok = true;

	Assert(UNIX_EPOCH_JDATE == date2j(1970, 1, 1));
	Assert(POSTGRES_EPOCH_JDATE == date2j(2000, 1, 1));

	ok &= CheckDateTokenTable("datetktbl", datetktbl, szdatetktbl);
	ok &= CheckDateTokenTable("deltatktbl", deltatktbl, szdeltatktbl);
	return ok;
}

/*
 * Common code for temporal prosupport functions: simplify, if possible,
 * a call to a temporal type's length-coercion function.
 *
 * Types time, timetz, timestamp and timestamptz each have a range of allowed
 * precisions.  An unspecified precision is rigorously equivalent to the
 * highest specifiable precision.  We can replace the function call with a
 * no-op RelabelType if it is coercing to the same or higher precision as the
 * input is known to have.
 *
 * The input Node is always a FuncExpr, but to reduce the #include footprint
 * of datetime.h, we declare it as Node *.
 *
 * Note: timestamp_scale throws an error when the typmod is out of range, but
 * we can't get there from a cast: our typmodin will have caught it already.
 */
Node *
TemporalSimplify(int32 max_precis, Node *node)
{
	FuncExpr   *expr = castNode(FuncExpr, node);
	Node	   *ret = NULL;
	Node	   *typmod;

	Assert(list_length(expr->args) >= 2);

	typmod = (Node *) lsecond(expr->args);

	if (IsA(typmod, Const) && !((Const *) typmod)->constisnull)
	{
		Node	   *source = (Node *) linitial(expr->args);
		int32		old_precis = exprTypmod(source);
		int32		new_precis = DatumGetInt32(((Const *) typmod)->constvalue);

		if (new_precis < 0 || new_precis == max_precis ||
			(old_precis >= 0 && new_precis >= old_precis))
			ret = relabel_to_typmod(source, new_precis);
	}

	return ret;
}

/*
 * This function gets called during timezone config file load or reload
 * to create the final array of timezone tokens.  The argument array
 * is already sorted in name order.
 *
 * The result is a TimeZoneAbbrevTable (which must be a single malloc'd chunk)
 * or NULL on malloc failure.  No other error conditions are defined.
 */
TimeZoneAbbrevTable *
ConvertTimeZoneAbbrevs(struct tzEntry *abbrevs, int n)
{
	TimeZoneAbbrevTable *tbl;
	Size		tbl_size;
	int			i;

	/* Space for fixed fields and datetkn array */
	tbl_size = offsetof(TimeZoneAbbrevTable, abbrevs) +
		n * sizeof(datetkn);
	tbl_size = MAXALIGN(tbl_size);
	/* Count up space for dynamic abbreviations */
	for (i = 0; i < n; i++)
	{
		struct tzEntry *abbr = abbrevs + i;

		if (abbr->zone != NULL)
		{
			Size		dsize;

			dsize = offsetof(DynamicZoneAbbrev, zone) +
				strlen(abbr->zone) + 1;
			tbl_size += MAXALIGN(dsize);
		}
	}

	/* Alloc the result ... */
	tbl = malloc(tbl_size);
	if (!tbl)
		return NULL;

	/* ... and fill it in */
	tbl->tblsize = tbl_size;
	tbl->numabbrevs = n;
	/* in this loop, tbl_size reprises the space calculation above */
	tbl_size = offsetof(TimeZoneAbbrevTable, abbrevs) +
		n * sizeof(datetkn);
	tbl_size = MAXALIGN(tbl_size);
	for (i = 0; i < n; i++)
	{
		struct tzEntry *abbr = abbrevs + i;
		datetkn    *dtoken = tbl->abbrevs + i;

		/* use strlcpy to truncate name if necessary */
		strlcpy(dtoken->token, abbr->abbrev, TOKMAXLEN + 1);
		if (abbr->zone != NULL)
		{
			/* Allocate a DynamicZoneAbbrev for this abbreviation */
			DynamicZoneAbbrev *dtza;
			Size		dsize;

			dtza = (DynamicZoneAbbrev *) ((char *) tbl + tbl_size);
			dtza->tz = NULL;
			strcpy(dtza->zone, abbr->zone);

			dtoken->type = DYNTZ;
			/* value is offset from table start to DynamicZoneAbbrev */
			dtoken->value = (int32) tbl_size;

			dsize = offsetof(DynamicZoneAbbrev, zone) +
				strlen(abbr->zone) + 1;
			tbl_size += MAXALIGN(dsize);
		}
		else
		{
			dtoken->type = abbr->is_dst ? DTZ : TZ;
			dtoken->value = abbr->offset;
		}
	}

	/* Assert the two loops above agreed on size calculations */
	Assert(tbl->tblsize == tbl_size);

	/* Check the ordering, if testing */
	Assert(CheckDateTokenTable("timezone abbreviations", tbl->abbrevs, n));

	return tbl;
}

/*
 * Install a TimeZoneAbbrevTable as the active table.
 *
 * Caller is responsible that the passed table doesn't go away while in use.
 */
void
InstallTimeZoneAbbrevs(TimeZoneAbbrevTable *tbl)
{
	zoneabbrevtbl = tbl;
	/* reset abbrevcache, which may contain pointers into old table */
	memset(abbrevcache, 0, sizeof(abbrevcache));
}

/*
 * Helper subroutine to locate pg_tz timezone for a dynamic abbreviation.
 */
static pg_tz *
FetchDynamicTimeZone(TimeZoneAbbrevTable *tbl, const datetkn *tp)
{
	DynamicZoneAbbrev *dtza;

	/* Just some sanity checks to prevent indexing off into nowhere */
	Assert(tp->type == DYNTZ);
	Assert(tp->value > 0 && tp->value < tbl->tblsize);

	dtza = (DynamicZoneAbbrev *) ((char *) tbl + tp->value);

	/* Look up the underlying zone if we haven't already */
	if (dtza->tz == NULL)
	{
		dtza->tz = pg_tzset(dtza->zone);

		/*
		 * Ideally we'd let the caller ereport instead of doing it here, but
		 * then there is no way to report the bad time zone name.
		 */
		if (dtza->tz == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_CONFIG_FILE_ERROR),
					 errmsg("time zone \"%s\" not recognized",
							dtza->zone),
					 errdetail("This time zone name appears in the configuration file for time zone abbreviation \"%s\".",
							   tp->token)));
	}
	return dtza->tz;
}


/*
 * This set-returning function reads all the available time zone abbreviations
 * and returns a set of (abbrev, utc_offset, is_dst).
 */
Datum
pg_timezone_abbrevs(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	int		   *pindex;
	Datum		result;
	HeapTuple	tuple;
	Datum		values[3];
	bool		nulls[3];
	const datetkn *tp;
	char		buffer[TOKMAXLEN + 1];
	int			gmtoffset;
	bool		is_dst;
	unsigned char *p;
	struct pg_tm tm;
	Interval   *resInterval;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL())
	{
		TupleDesc	tupdesc;
		MemoryContext oldcontext;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/*
		 * switch to memory context appropriate for multiple function calls
		 */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* allocate memory for user context */
		pindex = (int *) palloc(sizeof(int));
		*pindex = 0;
		funcctx->user_fctx = (void *) pindex;

		/*
		 * build tupdesc for result tuples. This must match this function's
		 * pg_proc entry!
		 */
		tupdesc = CreateTemplateTupleDesc(3, false);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "abbrev",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "utc_offset",
						   INTERVALOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "is_dst",
						   BOOLOID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);
		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	pindex = (int *) funcctx->user_fctx;

	if (zoneabbrevtbl == NULL ||
		*pindex >= zoneabbrevtbl->numabbrevs)
		SRF_RETURN_DONE(funcctx);

	tp = zoneabbrevtbl->abbrevs + *pindex;

	switch (tp->type)
	{
		case TZ:
			gmtoffset = tp->value;
			is_dst = false;
			break;
		case DTZ:
			gmtoffset = tp->value;
			is_dst = true;
			break;
		case DYNTZ:
			{
				/* Determine the current meaning of the abbrev */
				pg_tz	   *tzp;
				TimestampTz now;
				int			isdst;

				tzp = FetchDynamicTimeZone(zoneabbrevtbl, tp);
				now = GetCurrentTransactionStartTimestamp();
				gmtoffset = -DetermineTimeZoneAbbrevOffsetTS(now,
															 tp->token,
															 tzp,
															 &isdst);
				is_dst = (bool) isdst;
				break;
			}
		default:
			elog(ERROR, "unrecognized timezone type %d", (int) tp->type);
			gmtoffset = 0;		/* keep compiler quiet */
			is_dst = false;
			break;
	}

	MemSet(nulls, 0, sizeof(nulls));

	/*
	 * Convert name to text, using upcasing conversion that is the inverse of
	 * what ParseDateTime() uses.
	 */
	strlcpy(buffer, tp->token, sizeof(buffer));
	for (p = (unsigned char *) buffer; *p; p++)
		*p = pg_toupper(*p);

	values[0] = CStringGetTextDatum(buffer);

	/* Convert offset (in seconds) to an interval */
	MemSet(&tm, 0, sizeof(struct pg_tm));
	tm.tm_sec = gmtoffset;
	resInterval = (Interval *) palloc(sizeof(Interval));
	tm2interval(&tm, 0, resInterval);
	values[1] = IntervalPGetDatum(resInterval);

	values[2] = BoolGetDatum(is_dst);

	(*pindex)++;

	tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
	result = HeapTupleGetDatum(tuple);

	SRF_RETURN_NEXT(funcctx, result);
}

/*
 * This set-returning function reads all the available full time zones
 * and returns a set of (name, abbrev, utc_offset, is_dst).
 */
Datum
pg_timezone_names(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	bool		randomAccess;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	pg_tzenum  *tzenum;
	pg_tz	   *tz;
	Datum		values[4];
	bool		nulls[4];
	int			tzoff;
	struct pg_tm tm;
	fsec_t		fsec;
	const char *tzn;
	Interval   *resInterval;
	struct pg_tm itm;
	MemoryContext oldcontext;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("materialize mode required, but it is not allowed in this context")));

	/* The tupdesc and tuplestore must be created in ecxt_per_query_memory */
	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);

	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	randomAccess = (rsinfo->allowedModes & SFRM_Materialize_Random) != 0;
	tupstore = tuplestore_begin_heap(randomAccess, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* initialize timezone scanning code */
	tzenum = pg_tzenumerate_start();

	/* search for another zone to display */
	for (;;)
	{
		tz = pg_tzenumerate_next(tzenum);
		if (!tz)
			break;

		/* Convert now() to local time in this zone */
		if (timestamp2tm(GetCurrentTransactionStartTimestamp(),
						 &tzoff, &tm, &fsec, &tzn, tz) != 0)
			continue;			/* ignore if conversion fails */

		/*
		 * IANA's rather silly "Factory" time zone used to emit ridiculously
		 * long "abbreviations" such as "Local time zone must be set--see zic
		 * manual page" or "Local time zone must be set--use tzsetup".  While
		 * modern versions of tzdb emit the much saner "-00", it seems some
		 * benighted packagers are hacking the IANA data so that it continues
		 * to produce these strings.  To prevent producing a weirdly wide
		 * abbrev column, reject ridiculously long abbreviations.
		 */
		if (tzn && strlen(tzn) > 31)
			continue;

		MemSet(nulls, 0, sizeof(nulls));

		values[0] = CStringGetTextDatum(pg_get_timezone_name(tz));
		values[1] = CStringGetTextDatum(tzn ? tzn : "");

		MemSet(&itm, 0, sizeof(struct pg_tm));
		itm.tm_sec = -tzoff;
		resInterval = (Interval *) palloc(sizeof(Interval));
		tm2interval(&itm, 0, resInterval);
		values[2] = IntervalPGetDatum(resInterval);

		values[3] = BoolGetDatum(tm.tm_isdst > 0);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	pg_tzenumerate_end(tzenum);
	return (Datum) 0;
}
