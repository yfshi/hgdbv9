%{
/*
 * ReqID:SRS-PLSQL-DBMS_JOB A scanner for Oracle calendar syntax expression
 */
/* No reason to constrain amount of data slurped */
#include "../../include/ora_dbms_jobdata.h"
#define YY_READ_BUF_SIZE 16777216

/* Avoid exit() on fatal scanner errors (a bit ugly -- see yy_fatal_error) */
#undef fprintf
#define fprintf(file, fmt, msg)  fprintf_to_ereport(fmt, msg)

static void
fprintf_to_ereport(const char *fmt, const char *msg)
{
	ereport(ERROR, (errmsg_internal("%s", msg)));
}

/* Handles to the buffer that the lexer uses internally */
static YY_BUFFER_STATE scanbufhandle;
static char *scanbuf;
static int	scanbuflen;
static char *ereportscanbuf;

#define SCHEDULE_KEYWORD(a,b, c) {a,b,c},

const keyword keywords[] =
{
	SCHEDULE_KEYWORD("APR", APR, MONTH_K)
	SCHEDULE_KEYWORD("AUG", AUG, MONTH_K)
	SCHEDULE_KEYWORD("BYDATE", BYDATE, BYDATE_K)
	SCHEDULE_KEYWORD("BYDAY", BYDAY, BYDAY_K)
	SCHEDULE_KEYWORD("BYHOUR", BYHOUR, BYHOUR_K)
	SCHEDULE_KEYWORD("BYMINUTE", BYMINUTE, BYMINUTE_K)
	SCHEDULE_KEYWORD("BYMONTH", BYMONTH, BYMONTH_K)
	SCHEDULE_KEYWORD("BYMONTHDAY", BYMONTHDAY, BYMONTHDAY_K)
	SCHEDULE_KEYWORD("BYSECOND", BYSECOND, BYSECOND_K)
	SCHEDULE_KEYWORD("BYWEEKNO", BYWEEKNO, BYWEEKNO_K)
	SCHEDULE_KEYWORD("BYYEARDAY", BYYEARDAY, BYYEARDAY_K)
	SCHEDULE_KEYWORD("D", DAY, DAY_K)
	SCHEDULE_KEYWORD("DAILY", DAILY, DAILY_K)
	SCHEDULE_KEYWORD("DEC", DEC, MONTH_K)
	SCHEDULE_KEYWORD("FEB", FEB, MONTH_K)
	SCHEDULE_KEYWORD("FREQ", 0, FREQ_K)
	SCHEDULE_KEYWORD("FRI", FRI, WEEKDAY_K)
	SCHEDULE_KEYWORD("HOURLY", HOURLY, HOURLY_K)
	SCHEDULE_KEYWORD("INTERVAL", INTERVAL, INTERVAL_K)
	SCHEDULE_KEYWORD("JAN", JAN, MONTH_K)
	SCHEDULE_KEYWORD("JUL", JUL, MONTH_K)
	SCHEDULE_KEYWORD("JUN", JUN, MONTH_K)
	SCHEDULE_KEYWORD("MAR", MAR, MONTH_K)
	SCHEDULE_KEYWORD("MAY", MAY, MONTH_K)
	SCHEDULE_KEYWORD("MINUTELY", MINUTELY, MINUTELY_K)
	SCHEDULE_KEYWORD("MON", MON, WEEKDAY_K)
	SCHEDULE_KEYWORD("MONTHLY", MONTHLY, MONTHLY_K)
	SCHEDULE_KEYWORD("NOV", NOV, MONTH_K)
	SCHEDULE_KEYWORD("OCT", OCT, MONTH_K)
	SCHEDULE_KEYWORD("OFFSET", OFFSET, OFFSET_K)
	SCHEDULE_KEYWORD("SAT", SAT, WEEKDAY_K)
	SCHEDULE_KEYWORD("SECONDLY",SECONDLY, SECONDLY_K)
	SCHEDULE_KEYWORD("SEP", SEP, MONTH_K)
	SCHEDULE_KEYWORD("SPAN", SPAN, SPAN_K)
	SCHEDULE_KEYWORD("SUN", SUN, WEEKDAY_K)
	SCHEDULE_KEYWORD("THU", THU, WEEKDAY_K)
	SCHEDULE_KEYWORD("TUE", TUE, WEEKDAY_K)
	SCHEDULE_KEYWORD("W", WEEK, WEEK_K)
	SCHEDULE_KEYWORD("WED", WED, WEEKDAY_K)
	SCHEDULE_KEYWORD("WEEKLY", WEEKLY, WEEKLY_K)
	SCHEDULE_KEYWORD("YEARLY", YEARLY, YEARLY_K)
};

const int NumKeywords = sizeof(keywords)/sizeof(keywords[0]);

%}

%option 8bit
%option never-interactive
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
%option prefix="schedule_yy"

yyyy			[0-9][0-9][0-9][0-9]
mm				[0-1][0-9]
dd				[0-3][0-9]
ymd			    {yyyy}{mm}{dd}
md				{mm}{dd}
date			{ymd}|{md}

ident_start		[A-Za-z\200-\377_]
ident_cont		[A-Za-z\200-\377_0-9\$#]
identifer		{ident_start}{ident_cont}*

digit			[0-9]
integer 		{digit}+


%%

{date}			{ yylval.text = yytext;			return DATE_P; }
{integer}		{ yylval.num = atoi(yytext);		return NUM; }
{identifer}	{
					const keyword *keywordelem;

					/* Is it a keyword? */
					keywordelem = keywordLookup(yytext,
												keywords,
												NumKeywords);
					if (keywordelem != NULL)
					{
						yylval.num = keywordelem->value;
						return keywordelem->token;
					}

					/*
					 * No.  Convert the identifier to lower case, and truncate
					 * if necessary.
					 */
					yylval.text = yytext;
					return IDENT;
				}
[ \t\n\r\f]+ /* discard spaces */
.            return yytext[0]; /* alert parser of the garbage */

%%

void
yyerror(Schedule *result, const char *message)
{
	if (*yytext == YY_END_OF_BUFFER_CHAR)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("IVY-27418: syntax error in repeat interval or calendar"),
				 /* translator: %s is typically "syntax error" */
				 errdetail("%s at end of input", message)));
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("IVY-27418: syntax error in repeat interval or calendar"),
				 /* translator: first %s is typically "syntax error" */
				 errdetail("%s at or near \"%s\"", message, yytext)));
	}
}


/*
 * Called before any actual parsing is done
 */
void
schedule_scanner_init(const char *str)
{
	Size	slen = strlen(str);

	/*
	 * Might be left over after ereport()
	 */
	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);

	/*
	 * Make a scan buffer with special termination needed by flex.
	 */
	ereportscanbuf = palloc(slen);
	memcpy(ereportscanbuf, str, slen);

	scanbuflen = slen;
	scanbuf = palloc(slen + 2);
	memcpy(scanbuf, str, slen);
	scanbuf[slen] = scanbuf[slen + 1] = YY_END_OF_BUFFER_CHAR;
	scanbufhandle = yy_scan_buffer(scanbuf, slen + 2);

	BEGIN(INITIAL);
}


/*
 * Called after parsing is done to clean up after schedule_scanner_init()
 */
void
schedule_scanner_finish(void)
{
	yy_delete_buffer(scanbufhandle);
	pfree(scanbuf);
	pfree(ereportscanbuf);
}

const char* schedule_get_ereportscanbuf(void)
{
    return ereportscanbuf;
}