/***************************************************************
 *
 * Datatime Functions. 
 *
 ***************************************************************/

-- function isnull
CREATE OR REPLACE FUNCTION ivymy.isnull(anyelement)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'mys_isnull'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

/*DAY*/
CREATE OR REPLACE FUNCTION ivymy.day(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(numeric) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth_num'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(text) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdatetime));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(ivymy.mysdatetime) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(ivymy.mystimestamps) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(timestamp(0) with time zone) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(ivymy.mystime) RETURNS int4 
AS $$
    SELECT ivymy.day(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

/*DAYOFMONTH*/
CREATE OR REPLACE FUNCTION ivymy.dayofmonth(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(numeric) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofmonth_num'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(text) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdatetime));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(ivymy.mysdatetime) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(ivymy.mystimestamps) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(timestamp(0) with time zone) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofmonth(ivymy.mystime) RETURNS int4 
AS $$
    SELECT ivymy.dayofmonth(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

/*DAYOFWEEK*/
CREATE OR REPLACE FUNCTION ivymy.dayofweek(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofweek'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(numeric) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofweek_num'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(text) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdatetime));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(ivymy.mysdatetime) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(ivymy.mystimestamps) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(timestamp(0) with time zone) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofweek(ivymy.mystime) RETURNS int4 
AS $$
    SELECT ivymy.dayofweek(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;
/*DAYOFYEAR*/
CREATE OR REPLACE FUNCTION ivymy.dayofyear(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofyear'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(numeric) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_dayofyear_num'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(text) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdatetime));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(ivymy.mysdatetime) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(ivymy.mystimestamps) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(timestamp(0) with time zone) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.dayofyear(ivymy.mystime) RETURNS int4 
AS $$
    SELECT ivymy.dayofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

/*QUARTER*/
CREATE OR REPLACE FUNCTION ivymy.quarter(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_quarter'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(numeric) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_quarter_num'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(text) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdatetime));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(ivymy.mysdatetime) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(ivymy.mystimestamps) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(timestamp(0) with time zone) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(ivymy.mystime) RETURNS int4 
AS $$
    SELECT ivymy.quarter(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

/*HOUR、MICROSECOND、MINUTE、SECOND*/
CREATE OR REPLACE FUNCTION ivymy.hour (ivymy.mystimes) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetHour';
CREATE OR REPLACE FUNCTION ivymy.microsecond (ivymy.mystimes) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetMicrosecond';
CREATE OR REPLACE FUNCTION ivymy.minute (ivymy.mystimes) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetMinute';
CREATE OR REPLACE FUNCTION ivymy.second (ivymy.mystimes) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetSecond';

CREATE OR REPLACE FUNCTION ivymy.hour (text) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetHour_text';
CREATE OR REPLACE FUNCTION ivymy.microsecond (text) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetMicrosecond_text';
CREATE OR REPLACE FUNCTION ivymy.minute (text) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetMinute_text';
CREATE OR REPLACE FUNCTION ivymy.second (text) RETURNS int8 LANGUAGE C STABLE STRICT PARALLEL SAFE as 'MODULE_PATHNAME', 'GetSecond_text';

CREATE OR REPLACE FUNCTION ivymy.hour(ivymy.mysdate) RETURNS int4 
AS $$
    SELECT ivymy.hour(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.microsecond(ivymy.mysdate) RETURNS int4 
AS $$
    SELECT ivymy.microsecond(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.minute(ivymy.mysdate) RETURNS int4 
AS $$
    SELECT ivymy.minute(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.second(ivymy.mysdate) RETURNS int4 
AS $$
    SELECT ivymy.second(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.hour(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.hour(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.microsecond(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.microsecond(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.minute(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.minute(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.second(ivymy.mystimestamp) RETURNS int4 
AS $$
    SELECT ivymy.second(cast($1 as ivymy.mystimes));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

/*WEEKDAY WEEKOFYEAR YEAR*/
CREATE OR REPLACE FUNCTION ivymy.weekday(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_weekday'
LANGUAGE C STABLE RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekday(text) RETURNS int4 
AS $$
    SELECT ivymy.weekday(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekday(numeric) RETURNS int4 
AS $$
    SELECT ivymy.weekday(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekofyear(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_weekofyear'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekofyear(text) RETURNS int4 
AS $$
    SELECT ivymy.weekofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.weekofyear(numeric) RETURNS int4 
AS $$
    SELECT ivymy.weekofyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myyear(ivymy.mysdate) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_year'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myyear(text) RETURNS int4 
AS 'MODULE_PATHNAME', 'ivymy_year_text'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myyear(numeric) RETURNS int4 
AS $$
    SELECT ivymy.myyear(cast($1 as ivymy.mysdate));
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT PARALLEL SAFE;


/*CURDATE CURRENT_DATE CURTIME CURRENT_TIME*/

CREATE OR REPLACE FUNCTION ivymy.curdate() RETURNS ivymy.mysdate 
AS $$
    SELECT cast(pg_catalog.now() as ivymy.mysdate);
$$ LANGUAGE SQL PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myscurtime(integer) RETURNS ivymy.mystime 
AS 'MODULE_PATHNAME', 'ivymy_curtime_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.myscurtime() RETURNS ivymy.mystime 
AS $$
    SELECT cast(pg_catalog.now() as ivymy.mystime);
$$ LANGUAGE SQL PARALLEL SAFE;

/*NOW LOCALTIMESTAMP LOCALTIME CURRENT_TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.now(integer) RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.now() returns ivymy.mysdatetime 
AS $$
     SELECT cast(pg_catalog.now() as ivymy.mysdatetime);
 $$ LANGUAGE SQL PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.current_timestamp(integer) RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.current_timestamp() returns ivymy.mysdatetime 
AS $$
    SELECT cast(pg_catalog.now() as ivymy.mysdatetime);
$$ LANGUAGE SQL PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtime(integer) RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtime() returns ivymy.mysdatetime AS $$
    SELECT cast(pg_catalog.now() as ivymy.mysdatetime);
$$ LANGUAGE SQL PARALLEL SAFE;


CREATE OR REPLACE FUNCTION ivymy.Localtimestamp(integer) RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'ivymy_now_fsp'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtimestamp() returns ivymy.mysdatetime AS $$
    SELECT cast(pg_catalog.now() as ivymy.mysdatetime);
$$ LANGUAGE SQL PARALLEL SAFE;

/* FIX BUG 4814 */
CREATE FUNCTION ivymy.substring(pg_catalog.text, pg_catalog.int4, pg_catalog.int8)
RETURNS pg_catalog.text
AS 'text_substr'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;