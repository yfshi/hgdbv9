/***************************************************************
 *
 * Datatime Functions. 
 *
 ***************************************************************/

/*ISNULL*/
CREATE OR REPLACE FUNCTION ivymy.isnull(anyelement)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'mys_isnull'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

/*DAY*/
CREATE OR REPLACE FUNCTION ivymy.day(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(numeric) RETURNS int 
AS $$
    SELECT ivymy.day(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(text) RETURNS int 
AS $$
    SELECT ivymy.day(cast($1 as datetime));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(datetime) RETURNS int 
AS $$
    SELECT ivymy.day(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*DAYOFMONTH*/
CREATE OR REPLACE FUNCTION ivymy.dayofmonth(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(numeric) RETURNS int 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(text) RETURNS int 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as datetime));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(datetime) RETURNS int 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*DAYOFWEEK*/
CREATE OR REPLACE FUNCTION ivymy.dayofweek(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_dayofweek'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(numeric) RETURNS int 
AS $$
    SELECT ivymy.dayofweek(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(text) RETURNS int 
AS $$
    SELECT ivymy.dayofweek(cast($1 as datetime));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(datetime) RETURNS int 
AS $$
    SELECT ivymy.dayofweek(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*DAYOFYEAR*/
CREATE OR REPLACE FUNCTION ivymy.dayofyear(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_dayofyear'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(numeric) RETURNS int 
AS $$
    SELECT ivymy.dayofyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(text) RETURNS int 
AS $$
    SELECT ivymy.dayofyear(cast($1 as datetime));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(datetime) RETURNS int 
AS $$
    SELECT ivymy.dayofyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*QUARTER*/
CREATE OR REPLACE FUNCTION ivymy.quarter(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_quarter'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(numeric) RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(text) RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as datetime));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(datetime) RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*HOUR*/
CREATE OR REPLACE FUNCTION ivymy.hour(time(6)) RETURNS int
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'GetHour';

CREATE OR REPLACE FUNCTION ivymy.hour(text) RETURNS int
AS $$
    SELECT ivymy.hour(cast($1 as time(6)));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*MICROSECOND*/
CREATE OR REPLACE FUNCTION ivymy.microsecond(time(6)) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'GetMicrosecond';

CREATE OR REPLACE FUNCTION ivymy.microsecond(text) RETURNS int8
AS $$
    SELECT ivymy.microsecond(cast($1 as time(6)));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*MINUTE*/
CREATE OR REPLACE FUNCTION ivymy.minute(time(6)) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'GetMinute';

CREATE OR REPLACE FUNCTION ivymy.minute(text) RETURNS int8
AS $$
    SELECT ivymy.minute(cast($1 as time(6)));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SECOND*/
CREATE OR REPLACE FUNCTION ivymy.second(time(6)) RETURNS int
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'GetSecond';

CREATE OR REPLACE FUNCTION ivymy.second(text) RETURNS int 
AS $$
    SELECT ivymy.second(cast($1 as time(6)));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*WEEKDAY*/
CREATE OR REPLACE FUNCTION ivymy.weekday(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_weekday'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekday(text) RETURNS int 
AS $$
    SELECT ivymy.weekday(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekday(numeric) RETURNS int 
AS $$
    SELECT ivymy.weekday(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*WEEKOFYEAR*/
CREATE OR REPLACE FUNCTION ivymy.weekofyear(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_weekofyear'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekofyear(text) RETURNS int 
AS $$
    SELECT ivymy.weekofyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekofyear(numeric) RETURNS int 
AS $$
    SELECT ivymy.weekofyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*YEAR*/
CREATE OR REPLACE FUNCTION ivymy.myyear(date) RETURNS int 
AS 'MODULE_PATHNAME', 'ivymy_year'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myyear(text) RETURNS int 
AS $$
    SELECT ivymy.myyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myyear(numeric) RETURNS int 
AS $$
    SELECT ivymy.myyear(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*CURDATE*/
CREATE OR REPLACE FUNCTION ivymy.curdate() RETURNS date 
AS $$
    SELECT cast(pg_catalog.now() as date);
$$ LANGUAGE SQL PARALLEL SAFE;

/*CURTIME*/
CREATE OR REPLACE FUNCTION ivymy.myscurtime(integer) RETURNS time 
AS 'MODULE_PATHNAME', 'ivymy_curtime_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myscurtime() RETURNS time 
AS $$
    SELECT cast(pg_catalog.now() as time);
$$ LANGUAGE SQL PARALLEL SAFE;

/*NOW*/
CREATE OR REPLACE FUNCTION ivymy.now(integer) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.now() returns datetime 
AS $$
     SELECT cast(pg_catalog.now() as datetime);
 $$ LANGUAGE SQL PARALLEL SAFE;

/*CURRENT_TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.current_timestamp(integer) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.current_timestamp() returns datetime 
AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$ LANGUAGE SQL PARALLEL SAFE;

/*LOCALTIME*/
CREATE OR REPLACE FUNCTION ivymy.Localtime(integer) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtime() returns datetime AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$ LANGUAGE SQL PARALLEL SAFE;

/*LOCALTIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.Localtimestamp(integer) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtimestamp() returns datetime AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$ LANGUAGE SQL PARALLEL SAFE;

/*SYSDATE*/
CREATE OR REPLACE FUNCTION ivymy.sysdate(integer) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_sysdate_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sysdate() returns datetime AS $$
    SELECT cast( ivymy.sysdate(0) as datetime);
$$ LANGUAGE SQL PARALLEL SAFE;

/*MAKEDATE*/
CREATE OR REPLACE FUNCTION ivymy.makedate(int8, int8) RETURNS date
AS 'MODULE_PATHNAME', 'ivymy_makedate'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

/*MAKETIME*/
CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int8, numeric) RETURNS time(6)
AS 'MODULE_PATHNAME', 'ivymy_maketime'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(text, text, text) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(numeric, numeric, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_ADD*/
CREATE OR REPLACE FUNCTION ivymy.period_add(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_add';

CREATE OR REPLACE FUNCTION ivymy.period_add(int, int) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(text, text) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_DIFF*/
CREATE OR REPLACE FUNCTION ivymy.period_diff(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_diff';

CREATE OR REPLACE FUNCTION ivymy.period_diff(int, int) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(text, text) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SEC_TO_TIME */
CREATE OR REPLACE FUNCTION ivymy.sec_to_time(numeric) RETURNS time(6) 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_sec_to_time';

CREATE OR REPLACE FUNCTION ivymy.sec_to_time(text) RETURNS time(6) 
AS $$
    SELECT ivymy.sec_to_time(cast($1 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SUBTIME*/
CREATE OR REPLACE FUNCTION ivymy.subtime("any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_subtime';

/*TIMEDIFF*/
CREATE OR REPLACE FUNCTION ivymy.timediff("any", "any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timediff';

/*TIME*/
CREATE OR REPLACE FUNCTION ivymy.mytime("any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_time';

/*TIME_FORMAT*/
CREATE OR REPLACE FUNCTION ivymy.time_format(text, text) RETURNS text 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_time_format';

CREATE OR REPLACE FUNCTION ivymy.time_format(numeric, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.time_format(datetime, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(text) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg1'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(numeric) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(datetime) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(text, text) RETURNS datetime(6) 
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg2'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(numeric, numeric) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(datetime, datetime) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMPADD*/
CREATE OR REPLACE FUNCTION ivymy.mytimestampadd(text, "any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timestampadd';
/* End - SRS-MYSQL-BUILDINFUNCTION */

/*MAKEDATE*/
CREATE OR REPLACE FUNCTION ivymy.makedate(int8, int8) RETURNS date
AS 'MODULE_PATHNAME', 'ivymy_makedate'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

/*MAKETIME*/
CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int8, numeric) RETURNS time(6)
AS 'MODULE_PATHNAME', 'ivymy_maketime'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(text, text, text) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(numeric, numeric, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_ADD*/
CREATE OR REPLACE FUNCTION ivymy.period_add(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_add';

CREATE OR REPLACE FUNCTION ivymy.period_add(int, int) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(text, text) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_DIFF*/
CREATE OR REPLACE FUNCTION ivymy.period_diff(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_diff';

CREATE OR REPLACE FUNCTION ivymy.period_diff(int, int) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(text, text) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SEC_TO_TIME */
CREATE OR REPLACE FUNCTION ivymy.sec_to_time(numeric) RETURNS time(6) 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_sec_to_time';

CREATE OR REPLACE FUNCTION ivymy.sec_to_time(text) RETURNS time(6) 
AS $$
    SELECT ivymy.sec_to_time(cast($1 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SUBTIME*/
CREATE OR REPLACE FUNCTION ivymy.subtime("any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_subtime';

/*TIMEDIFF*/
CREATE OR REPLACE FUNCTION ivymy.timediff("any", "any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timediff';

/*TIME*/
CREATE OR REPLACE FUNCTION ivymy.mytime("any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_time';

/*TIME_FORMAT*/
CREATE OR REPLACE FUNCTION ivymy.time_format(text, text) RETURNS text 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_time_format';

CREATE OR REPLACE FUNCTION ivymy.time_format(numeric, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.time_format(datetime, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(text) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg1'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(numeric) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(datetime) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(text, text) RETURNS datetime(6) 
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg2'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(numeric, numeric) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(datetime, datetime) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMPADD*/
CREATE OR REPLACE FUNCTION ivymy.mytimestampadd(text, "any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timestampadd';
/* End - SRS-MYSQL-BUILDINFUNCTION */

/*MAKEDATE*/
CREATE OR REPLACE FUNCTION ivymy.makedate(int8, int8) RETURNS date
AS 'MODULE_PATHNAME', 'ivymy_makedate'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

/*MAKETIME*/
CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int8, numeric) RETURNS time(6)
AS 'MODULE_PATHNAME', 'ivymy_maketime'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(text, text, text) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(numeric, numeric, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int, int) returns time(6) AS $$
    SELECT ivymy.maketime(cast($1 as int8), cast($2 as int8), cast($3 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_ADD*/
CREATE OR REPLACE FUNCTION ivymy.period_add(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_add';

CREATE OR REPLACE FUNCTION ivymy.period_add(int, int) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_add(text, text) returns int8 AS $$
    SELECT ivymy.period_add(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*PERIOD_DIFF*/
CREATE OR REPLACE FUNCTION ivymy.period_diff(int8, int8) RETURNS int8 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_period_diff';

CREATE OR REPLACE FUNCTION ivymy.period_diff(int, int) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(text, text) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as numeric), cast($2 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SEC_TO_TIME */
CREATE OR REPLACE FUNCTION ivymy.sec_to_time(numeric) RETURNS time(6) 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_sec_to_time';

CREATE OR REPLACE FUNCTION ivymy.sec_to_time(text) RETURNS time(6) 
AS $$
    SELECT ivymy.sec_to_time(cast($1 as numeric));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SUBTIME*/
CREATE OR REPLACE FUNCTION ivymy.subtime("any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_subtime';

/*TIMEDIFF*/
CREATE OR REPLACE FUNCTION ivymy.timediff("any", "any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timediff';

/*TIME*/
CREATE OR REPLACE FUNCTION ivymy.mytime("any") RETURNS time(6) 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_time';

/*TIME_FORMAT*/
CREATE OR REPLACE FUNCTION ivymy.time_format(text, text) RETURNS text 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'ivymy_time_format';

CREATE OR REPLACE FUNCTION ivymy.time_format(numeric, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.time_format(datetime, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(text) RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg1'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(numeric) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp1(datetime) RETURNS datetime(6)
AS $$
    SELECT ivymy.mytimestamp1(cast($1 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(text, text) RETURNS datetime(6) 
AS 'MODULE_PATHNAME', 'ivymy_timestamp_arg2'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(numeric, numeric) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.mytimestamp2(datetime, datetime) RETURNS datetime(6) 
AS $$
    SELECT ivymy.mytimestamp2(cast($1 as text), cast($2 as text));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*TIMESTAMPADD*/
CREATE OR REPLACE FUNCTION ivymy.mytimestampadd(text, "any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'ivymy_timestampadd';
/* End - SRS-MYSQL-BUILDINFUNCTION */

/* FIX BUG 4814 */
CREATE FUNCTION ivymy.substring(pg_catalog.text, pg_catalog.int4, pg_catalog.int8)
RETURNS pg_catalog.text
AS 'text_substr'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;