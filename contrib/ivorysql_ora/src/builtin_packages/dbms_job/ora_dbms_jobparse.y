%{
/* ReqID:SRS-PLSQL-DBMS_JOB */
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "miscadmin.h"
#include "../../include/ora_dbms_jobdata.h"

/*
 * Bison doesn't allocate anything that needs to live across parser calls,
 * so we can easily have it use palloc instead of malloc.  This prevents
 * memory leaks if we error out during parsing.  Note this only works with
 * bison >= 2.0.  However, in bison 1.875 the default is to use alloca()
 * if possible, so there's not really much problem anyhow, at least if
 * you're building with gcc.
 */
#define YYMALLOC palloc
#define YYFREE   pfree
extern const char* schedule_get_ereportscanbuf(void);
%}

/* BISON Declarations */
%expect 0
%name-prefix = "schedule_yy"
%parse-param {Schedule *result}


%union {
	int				 	 num;
	char 				*text;
	freqType			 freq_type;
	Frequency			*freq;
	CalendarType		 calendar_type;
	SignType			 sign_type;
	ModiferType			 mod_type;
	ModiferUnit			 mod_units;
	Modifer				*mod;
	ModiferValue		*mod_value;
	CalendarElem		*calendar_elem;
	Calendar			*calendar;
	List				*list;
	Schedule			*schedule;
}

%token <freq_type> 	YEARLY_K MONTHLY_K WEEKLY_K
					DAILY_K HOURLY_K MINUTELY_K SECONDLY_K
%token <num>	NUM MONTH_K WEEKDAY_K
%token <text>	DATE_P FREQ_K IDENT
%token <calendar_type>	INTERVAL_K BYMONTH_K
				BYWEEKNO_K BYYEARDAY_K BYDATE_K BYMONTHDAY_K
				BYDAY_K BYHOUR_K BYMINUTE_K BYSECOND_K
%token <mod_type>	SPAN_K OFFSET_K
%token <mod_units>	WEEK_K DAY_K

%type <sign_type>		sign
%type <mod_value>		dur_times
%type <mod_type>		modifer_type
%type <mod> 			offset_span opt_offset_span
%type <calendar_elem>	calendar_integer_val date_clause intervalnum numeric_month monthday
						yearday weeknumber byday hour minute second

%type <calendar>		calendar_clause interval_clause bymonth_clause
						byweekno_clause byyearday_clause bydate_clause
						bymonthday_clause byday_clause byhour_clause
						byminute_clause bysecond_clause

%type <list>			date_list calendar_list monthlist weeknumber_list
						yearday_list monthday_list byday_list hour_list
						minute_list second_list

%type <freq_type>		freq_var
%type <freq>			frequency_clause
%type <schedule>		regular_schedule


/* Grammar follows */
%%

regular_schedule: 
	frequency_clause ';' calendar_list {result->freq = $1; result->calendar_list = $3;}
	| frequency_clause ';' calendar_list ';'  {result->freq = $1; result->calendar_list = $3;}
    | frequency_clause ';'   {result->freq = $1;}
    | frequency_clause   {result->freq = $1;}
    | /*EMPTY*/
        { ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("IVY-27465: invalid value %s for attribute REPEAT_INTERVAL", schedule_get_ereportscanbuf())));
            YYERROR;
        }	
	;

frequency_clause: FREQ_K '=' freq_var
			{
				Frequency *val = palloc(sizeof(Frequency));
				memset(result, 0, sizeof(Schedule));
				val->predef_freq_type = $3;
				val->user_freq_type = NULL;
				$$ = val;
			}
		| FREQ_K '=' IDENT
			{
				Frequency *val = palloc(sizeof(Frequency));
				val->user_freq_type = pstrdup($3);
				$$ = val;
				ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("IVY-27412: repeat interval or calendar contains invalid identifier:\n\"%s\".\"%s\"", GetUserNameFromId(GetUserId(), false),$3)));
						YYERROR;				
			}
	;

freq_var: YEARLY_K
		|MONTHLY_K
		|WEEKLY_K
		|DAILY_K
		|HOURLY_K
		|MINUTELY_K
		|SECONDLY_K
	;

calendar_list: calendar_clause
			{
				$$ = list_make1($1);
			}
		| calendar_list ';' calendar_clause
			{
				$$ = lappend($1, $3);
			}
	;

calendar_clause: interval_clause
		| bymonth_clause
		| byweekno_clause
		| byyearday_clause
		| bydate_clause
		| bymonthday_clause
		| byday_clause
		| byhour_clause
		| byminute_clause
		| bysecond_clause

interval_clause: INTERVAL_K '=' intervalnum
				{
					Calendar *val = palloc(sizeof(Calendar));

					val->calendar_type = $1;
					val->elem_list = list_make1($3);
					$$ = val;
				}
	;

intervalnum: calendar_integer_val
    { 
	  if ($1->elem_value.int_val >= 1 && $1->elem_value.int_val <= 99) 
      {
            $$ = $1;
      }
      else if ($1->elem_value.int_val == 0)
      {
            $1->elem_value.int_val = 1;
			$$ = $1;
      } 
      else if ($1->elem_value.int_val > 99)
      {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("IVY-27421: calendar expression restriction MAX_FREQUENCY_INTERVAL encountered")));
            YYERROR;
      }
	  else
      {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("IVY-27418: syntax error in repeat interval or calendar")));
            YYERROR;
      } 
    }				
	;


bymonth_clause: BYMONTH_K '=' monthlist
    {
        if (result->has_bymonth) 
        {
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("IVY-27423: calendar clause BYMONTH specified more than once")));
            YYERROR;
        }
		else
		{
						
			Calendar *val = palloc(sizeof(Calendar));

			val->calendar_type = $1;
			val->elem_list = $3;
			$$ = val;
			result->has_bymonth = true;
		}

    }				
	;

monthlist: numeric_month
		{
			$$ = list_make1($1);
		}
	| monthlist ',' numeric_month
		{
			$$ = lappend($1, $3);
		}

numeric_month: calendar_integer_val
			{ 
				if ($1->elem_value.int_val >= 1 && $1->elem_value.int_val <= 12) 
				{
					$$ = $1;
				} 
				else
				{
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("IVY-27414: Invalid BYMONTH clause value")));
					YYERROR;
				} 
			}
			| MONTH_K
				{
					CalendarElem *val = palloc(sizeof(CalendarElem));

					val->elem_value.int_val = $1;
					val->elem_modifer = NULL;
					$$ = val;
				}
			| IDENT
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("IVY-27414: Invalid BYMONTH clause value")));
				YYERROR;
			} 			
	;

byweekno_clause: BYWEEKNO_K '=' weeknumber_list
				{
					Calendar *val = palloc(sizeof(Calendar));

					val->calendar_type = $1;
					val->elem_list = $3;
					$$ = val;
				}
	;

weeknumber_list: weeknumber
				{
					$$ = list_make1($1);
				}
		| weeknumber_list ',' weeknumber
				{
					$$ = lappend($1, $3);
				}
	;

weeknumber: calendar_integer_val
		{
			$$ = $1;
		}
	;

byyearday_clause: BYYEARDAY_K '=' yearday_list
		{
			Calendar *val = palloc(sizeof(Calendar));

			val->calendar_type = $1;
			val->elem_list = $3;
			$$ = val;
		}
	;

yearday_list: yearday
			{
				$$ = list_make1($1);
			}
		| yearday_list ',' yearday
			{
				$$ = lappend($1, $3);
			}
	;

yearday: calendar_integer_val
		{
			$$ = $1;
		}
	;


bymonthday_clause: BYMONTHDAY_K '=' monthday_list
		{
			if (result->has_bymonthday) 
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("IVY-27423: calendar clause BYMONTHDAY specified more than once")));
				YYERROR;
			}
			else
			{

				Calendar *val = palloc(sizeof(Calendar));

				val->calendar_type = $1;
				val->elem_list = $3;
				$$ = val;
				
				result->has_bymonthday = true;
			}
		}
	;

monthday_list : monthday
			{
				$$ = list_make1($1);
			}
		| monthday_list ',' monthday
			{
				$$ = lappend($1, $3);
			}
	;

monthday: calendar_integer_val
			{
				if ($1->elem_value.int_val == 0 || $1->elem_value.int_val < -31 || $1->elem_value.int_val > 31)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("IVY-27414: Invalid BYMONTHDAY clause value")));
					YYERROR;
				}
				else
				{
					$$ = $1;
				}
			}
	;


byday_clause: BYDAY_K '=' byday_list
		{
			Calendar *val = palloc(sizeof(Calendar));

			val->calendar_type = $1;
			val->elem_list = $3;
			$$ = val;
		}
	;

byday_list: byday
			{
				$$ = list_make1($1);
			}
		| byday_list ',' byday
			{
				$$ = lappend($1, $3);
			}
	;

byday: NUM WEEKDAY_K
			{
				CalendarElem *val = palloc(sizeof(CalendarElem));

				val->elem_value.list_val = list_make2_int($1, $2);
				val->elem_modifer = NULL;
				$$ = val;
			}
		| WEEKDAY_K
			{
				CalendarElem *val = palloc(sizeof(CalendarElem));

				val->elem_value.list_val = list_make1_int($1);
				val->elem_modifer = NULL;
				$$ = val;
			}
	;


byhour_clause: BYHOUR_K '=' hour_list
		{
			if (result->has_byhour) 
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("IVY-27423: calendar clause BYHOUR specified more than once")));
				YYERROR;
			}
			else
			{
				Calendar *val = palloc(sizeof(Calendar));

				val->calendar_type = $1;
				val->elem_list = $3;
				$$ = val;

				result->has_byhour = true;
			}
		}
	;

hour_list: hour
			{
				$$ = list_make1($1);
			}
		| hour_list ',' hour
			{
				$$ = lappend($1, $3);
			}
	;

hour: calendar_integer_val
		{			
			if ($1->elem_value.int_val >= 0 && $1->elem_value.int_val <= 23) 
			{
				$$ = $1;
			} 
			else
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("IVY-27414: Invalid BYHOUR clause value")));
				YYERROR;
			} 
		}		
	;


byminute_clause: BYMINUTE_K '=' minute_list
		{
			if (result->has_byminute) 
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("IVY-27423: calendar clause BYMINUTE specified more than once")));
				YYERROR;
			}
			else
			{
				Calendar *val = palloc(sizeof(Calendar));

				val->calendar_type = $1;
				val->elem_list = $3;
				$$ = val;
				
				result->has_byminute = true;
			}
		}
	;

minute_list: minute
			{
				$$ = list_make1($1);
			}
		| minute_list ',' minute
			{
				$$ = lappend($1, $3);
			}
	;

minute: calendar_integer_val
		{			
			if ($1->elem_value.int_val >= 0 && $1->elem_value.int_val <= 59) 
			{
				$$ = $1;
			} 
			else
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("IVY-27414: Invalid BYMINUTE clause value")));
				YYERROR;
			} 
		}
	;

bysecond_clause: BYSECOND_K '=' second_list
		{
			if (result->has_bysecond) 
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("IVY-27423: calendar clause BYSECONS specified more than once")));
				YYERROR;
			}
			else
			{
				Calendar *val = palloc(sizeof(Calendar));

				val->calendar_type = $1;
				val->elem_list = $3;
				$$ = val;
				
				result->has_bysecond = true;
			}
		}
	;

second_list: second
			{
				$$ = list_make1($1);
			}
		| second_list ',' second
			{
				$$ = lappend($1, $3);
			}
	;

second: calendar_integer_val
		{
			if ($1->elem_value.int_val >= 0 && $1->elem_value.int_val <= 59) 
			{
				$$ = $1;
			} 
			else
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("IVY-27414: Invalid BYSECONS clause value")));
				YYERROR;
			}			
		}
	;

bydate_clause: BYDATE_K '=' date_list
		{
			Calendar *val = palloc(sizeof(Calendar));

			val->calendar_type = $1;
			val->elem_list = $3;
			$$ = val;
		}
	;

date_list: date_clause
		{
			$$ = list_make1($1);
		}
		| date_list ',' date_clause
		{
			$$ = lappend($1, $3);
		}
	;


date_clause: DATE_P opt_offset_span
		{
			CalendarElem *val = palloc(sizeof(CalendarElem));
			val->elem_value.str_val = pstrdup($1);
			val->elem_modifer = $2;
			$$ = val;
		}
	;

opt_offset_span: offset_span
			{
				$$ = $1;
			}
		| /* empty */	{ $$ = (Modifer *)NULL; }
	;

offset_span: sign modifer_type ':' dur_times
		{
			Modifer *val = palloc(sizeof(Modifer));

			val->sign_type = $1;
			val->modifer_type = $2;
			val->modifer_value = $4;
			$$ = val;
		}
	;

sign: '+'
			{
				$$ = plus;
			}
		| '-'
			{
				$$ = minus;
			}
		| '^'
			{
				$$ = range;
			}
	;

modifer_type: SPAN_K
		| OFFSET_K
	;

dur_times: NUM DAY_K
		{
			ModiferValue *val = palloc(sizeof(ModiferValue));
			val->value = $1;
			val->modifer_units = $2;
			$$ = val;
		}
		| NUM WEEK_K
		{
			ModiferValue *val = palloc(sizeof(ModiferValue));
			val->value = $1;
			val->modifer_units = $2;
			$$ = val;
		}
	;

calendar_integer_val: NUM
			{
				CalendarElem *val = palloc(sizeof(CalendarElem));

				val->elem_value.int_val = $1;
				val->elem_modifer = NULL;
				$$ = val;
			}
		| '-' NUM
			{
				CalendarElem *val = palloc(sizeof(CalendarElem));

				val->elem_value.int_val = 0 - $2;
				val->elem_modifer = NULL;
				$$ = val;
			}
	;
%%

#include "ora_dbms_jobscan.c"
