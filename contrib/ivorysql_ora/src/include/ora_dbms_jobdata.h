/*--------------------------------------------------------------------
 * 
 * File: ora_dbms_jobdata.h
 *
 * Abstract: 
 * 		Definitions for the DBMS_JOB PACKAGE.
 *
 * Authored by dongyongbo@highgo.com, 20241024.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/include/ora_dbms_jobdata.h
 *
 *--------------------------------------------------------------------
 */
#ifndef SCHEDULEDATA_H
#define SCHEDULEDATA_H

#include <math.h>
#include "postgres.h"
#include "nodes/pg_list.h"
#include "utils/builtins.h"

/* structure of keywords */
typedef struct
{
	char	*name;
	int		 value;
	int		 token;
} keyword;

/* valid weekday value */
typedef enum
{
	MON,
	TUE,
	WED,
	THU,
	FRI,
	SAT,
	SUN
} weekDay;

/* use number to replace month name */
typedef enum
{
	JAN = 1,
	FEB = 2,
	MAR = 3,
	APR = 4,
	MAY = 5,
	JUN = 6,
	JUL = 7,
	AUG = 8,
	SEP = 9,
	OCT = 10,
	NOV = 11,
	DEC = 12
} monthDay;

/* valid FREQ value */
typedef	enum
{
	YEARLY,
	MONTHLY,
	WEEKLY,
	DAILY,
	HOURLY,
	MINUTELY,
	SECONDLY
} freqType;

/* valid schedule clause */
typedef enum
{
	INTERVAL,
	BYMONTH,
	BYWEEKNO,
	BYYEARDAY,
	BYDATE,
	BYMONTHDAY,
	BYDAY,
	BYHOUR,
	BYMINUTE,
	BYSECOND
} CalendarType;

typedef enum
{
	SPAN,			/* SPAN */
	OFFSET			/* OFFSET */
} ModiferType;

typedef enum
{
	plus,			/* + */
	minus,			/* - */
	range			/* ^ */
} SignType;

typedef enum
{
	WEEK,			/* W */
	DAY				/* D */
} ModiferUnit;

typedef struct
{
	int				value;
	ModiferUnit		modifer_units;
} ModiferValue;

typedef struct
{
	SignType		 sign_type;
	ModiferType		 modifer_type;
	ModiferValue 	*modifer_value;
} Modifer;

typedef union
{
	int		 int_val;
	char	*str_val;
	List	*list_val;
} Valunion;

typedef struct
{
	Valunion	 elem_value;
	Modifer		*elem_modifer;
} CalendarElem;

typedef struct
{
	CalendarType	 calendar_type;
	List			*elem_list;
} Calendar;

typedef struct
{
	freqType	 predef_freq_type;	/* predefined frequency */
	char		*user_freq_type;		/* user defined frequency */
} Frequency;

typedef struct
{
	Frequency	*freq;
	List		*calendar_list;
	bool has_bymonth;
    bool has_bymonthday;
    bool has_byhour;
    bool has_byminute;
    bool has_bysecond;
} Schedule;

/* Begin - BUG#M0000349 */
typedef enum
{
	BYMONTH_ARRAY,
	BYMONTHDAY_ARRAY,
	BYHOUR_ARRAY,
	BYMINUTE_ARRAY,
	BYSECOND_ARRAY
} ByClauseArrayType;

/* Analyze tree node */
typedef struct ScheduleData
{
	int		interval;
	char	month[12];
	char	monthday[31];
	signed char	negative_monthday[31]; /* BUG#Z5339 */
	int		negative_monthday_count;
	char	hour[24];
	char	minute[60];
	char	second[60];
	bool	has_bymonth;
	bool	has_bymonthday;
	bool	has_byhour;
	bool	has_byminute;
	bool	has_bysecond;
} ScheduleData;
/* End - BUG#M0000349 */

extern const keyword	keywords[];
extern const int		NumKeywords;

/* in schedulescan.l */
extern int	schedule_yylex(void);
extern void schedule_yyerror(Schedule *result, const char *message);
extern void schedule_scanner_init(const char *str);
extern void schedule_scanner_finish(void);

/* in schedulegparse.y */
extern int	schedule_yyparse(Schedule *result);

/* in schedule.c */
extern const keyword *keywordLookup(const char *text, const keyword *keywords, int num_keywords);

#endif /* SCHEDULEDATA_H */
