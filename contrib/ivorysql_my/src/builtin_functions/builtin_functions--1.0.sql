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
CREATE OR REPLACE FUNCTION ivymy.day(date)
RETURNS int 
AS 'MODULE_PATHNAME', 'dayofmonth'
LANGUAGE C
STABLE
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(numeric)
RETURNS int 
AS 'SELECT ivymy.day(cast($1 as date))'
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(text)
RETURNS int 
AS 'SELECT ivymy.day(cast($1 as datetime))'
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.day(datetime)
RETURNS int 
AS 'SELECT ivymy.day(cast($1 as date))'
LANGUAGE SQL
STRICT
PARALLEL SAFE;

/* Begin - bug#z5063 */
CREATE OR REPLACE FUNCTION ivymy.day(bool)
RETURNS int 
AS 'SELECT ivymy.day($1::int)'
LANGUAGE SQL
STRICT
PARALLEL SAFE;
/* End - bug#z5063 */

/*DAYOFMONTH*/
CREATE OR REPLACE FUNCTION ivymy.dayofmonth(date) RETURNS int 
AS 'MODULE_PATHNAME', 'dayofmonth'
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
AS 'MODULE_PATHNAME', 'dayofweek'
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

CREATE OR REPLACE FUNCTION ivymy.dayofweek(time(6)) RETURNS int 
AS $$
    SELECT ivymy.dayofweek(cast($1 as date));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*DAYOFYEAR*/
CREATE OR REPLACE FUNCTION ivymy.dayofyear(date) RETURNS int 
AS 'MODULE_PATHNAME', 'dayofyear'
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
CREATE OR REPLACE FUNCTION ivymy.quarter(date) 
RETURNS int 
AS 'MODULE_PATHNAME', 'quarter'
LANGUAGE C 
STABLE 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(numeric) 
RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as date));
$$ 
LANGUAGE SQL 
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(text) 
RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as datetime));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.quarter(datetime) 
RETURNS int 
AS $$
    SELECT ivymy.quarter(cast($1 as date));
$$ 
LANGUAGE SQL
STRICT 
PARALLEL SAFE;

/* Begin - bug#z5083 */
CREATE OR REPLACE FUNCTION ivymy.quarter(bool) 
RETURNS int 
AS $$
    SELECT ivymy.quarter($1::int);
$$ 
LANGUAGE SQL
STRICT 
PARALLEL SAFE;
/* End - bug#z5083 */

/*HOUR*/
CREATE OR REPLACE FUNCTION ivymy.hour(time(6))
RETURNS int
as 'MODULE_PATHNAME', 'hour'
LANGUAGE C 
STABLE 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.hour(text)
RETURNS int
AS $$
    SELECT ivymy.hour(cast($1 as time(6)));
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;

/* Begin - bug#z5093 */
CREATE OR REPLACE FUNCTION ivymy.hour(int)
RETURNS int
AS $$
    SELECT ivymy.hour($1::text);
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.hour(bool)
RETURNS int
AS $$
    SELECT ivymy.hour($1::int);
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;
/* End - bug#z5093 */

/* Begin - bug#z5587 */
CREATE OR REPLACE FUNCTION ivymy.hour(time)
RETURNS int
AS $$
    SELECT ivymy.hour(cast($1 as time(6)));
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;
/* End - bug#z5587 */

/*MICROSECOND*/
CREATE OR REPLACE FUNCTION ivymy.microsecond(time(6))
RETURNS int8 
as 'MODULE_PATHNAME', 'microsecond'
LANGUAGE C 
STABLE 
STRICT 
PARALLEL SAFE ;

CREATE OR REPLACE FUNCTION ivymy.microsecond(text) 
RETURNS int8
AS $$
    SELECT ivymy.microsecond(cast($1 as time(6)));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

/* Begin - bug#z5093 */
CREATE OR REPLACE FUNCTION ivymy.microsecond(int) 
RETURNS int8
AS $$
    SELECT ivymy.microsecond($1::text);
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.microsecond(bool) 
RETURNS int8
AS $$
    SELECT ivymy.microsecond($1::int);
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;
/* End - bug#z5093 */

/* Begin - bug#z5585 */
CREATE OR REPLACE FUNCTION ivymy.microsecond(datetime) 
RETURNS int8
AS $$
    SELECT ivymy.microsecond(cast($1 as time(6)));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;
/* End - bug#z5585 */

/*MINUTE*/
CREATE OR REPLACE FUNCTION ivymy.minute(time(6)) 
RETURNS int8 
as 'MODULE_PATHNAME', 'minute'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.minute(text) 
RETURNS int8
AS $$
    SELECT ivymy.minute(cast($1 as time(6)));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

/* Begin - bug#z5096 */
CREATE OR REPLACE FUNCTION ivymy.minute(int) 
RETURNS int8
AS $$
    SELECT ivymy.minute($1::text);
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.minute(bool) 
RETURNS int8
AS $$
    SELECT ivymy.minute($1::int);
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;
/* End - bug#z5096 */

/* Begin - bug#z5588 */
CREATE OR REPLACE FUNCTION ivymy.minute(datetime) 
RETURNS int8
AS $$
    SELECT ivymy.minute(cast($1 as time(6)));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;
/* End - bug#z5588 */

/*SECOND*/
CREATE OR REPLACE FUNCTION ivymy.second(time(6))
RETURNS int
as 'MODULE_PATHNAME', 'second'
LANGUAGE C 
STABLE 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.second(text)
RETURNS int 
AS $$
    SELECT ivymy.second(cast($1 as time(6)));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

/* Begin - bug#z5099 */
CREATE OR REPLACE FUNCTION ivymy.second(int)
RETURNS int 
AS $$
    SELECT ivymy.second($1::text);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;
/* End - bug#z5099 */

/* Begin - bug#z5101 */
CREATE OR REPLACE FUNCTION ivymy.second(bool)
RETURNS int 
AS $$
    SELECT ivymy.second($1::int);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;
/* End - bug#z5101 */

/*WEEKDAY*/
CREATE OR REPLACE FUNCTION ivymy.weekday(date) RETURNS int 
AS 'MODULE_PATHNAME', 'weekday'
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
AS 'MODULE_PATHNAME', 'weekofyear'
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
AS 'MODULE_PATHNAME', 'year'
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
CREATE OR REPLACE FUNCTION ivymy.curtime(integer) RETURNS time(6)
AS 'MODULE_PATHNAME', 'curtime'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.curtime() RETURNS time 
AS $$
    SELECT cast(pg_catalog.now() as time);
$$ LANGUAGE SQL PARALLEL SAFE;

/*NOW*/
CREATE OR REPLACE FUNCTION ivymy.now(integer) 
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'now'
LANGUAGE C
STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.now() 
RETURNS datetime 
AS $$
     SELECT cast(pg_catalog.now() as datetime);
$$ LANGUAGE SQL 
PARALLEL SAFE;

/*CURRENT_TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.current_timestamp(integer)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'now'
LANGUAGE C
STABLE 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.current_timestamp() 
RETURNS datetime 
AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$ 
LANGUAGE SQL 
PARALLEL SAFE;

/*LOCALTIME*/
CREATE OR REPLACE FUNCTION ivymy.localtime(integer)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'now'
LANGUAGE C 
STABLE 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.localtime() 
RETURNS datetime
AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$
LANGUAGE SQL
PARALLEL SAFE;

/*LOCALTIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.Localtimestamp(integer)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'now'
LANGUAGE C
STABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.Localtimestamp()
returns datetime 
AS $$
    SELECT cast(pg_catalog.now() as datetime);
$$
LANGUAGE SQL
PARALLEL SAFE;

/*SYSDATE*/
CREATE OR REPLACE FUNCTION ivymy.sysdate(integer)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'sysdate'
LANGUAGE C
STABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sysdate()
RETURNS datetime
AS $$
    SELECT cast(ivymy.sysdate(0) as datetime);
$$
LANGUAGE SQL
PARALLEL SAFE;

/*MAKEDATE*/
CREATE OR REPLACE FUNCTION ivymy.makedate(int8, int8) RETURNS date
AS 'MODULE_PATHNAME', 'makedate'
LANGUAGE C STRICT STABLE PARALLEL SAFE;

/*MAKETIME*/
CREATE OR REPLACE FUNCTION ivymy.maketime(int8, int8, numeric) RETURNS time(6)
AS 'MODULE_PATHNAME', 'maketime'
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
as 'MODULE_PATHNAME', 'period_add';

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
as 'MODULE_PATHNAME', 'period_diff';

CREATE OR REPLACE FUNCTION ivymy.period_diff(int, int) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(numeric, numeric) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.period_diff(text, text) returns int8 AS $$
    SELECT ivymy.period_diff(cast($1 as int8), cast($2 as int8));
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/*SEC_TO_TIME */
CREATE OR REPLACE FUNCTION ivymy.sec_to_time(numeric)
RETURNS time(6)
AS 'MODULE_PATHNAME', 'sec_to_time'
LANGUAGE C
STABLE
STRICT;

CREATE OR REPLACE FUNCTION ivymy.sec_to_time(text)
RETURNS time(6) 
AS $$
    SELECT sec_to_time(cast($1 as numeric(99, 6)));
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE;

/*SUBTIME*/
CREATE OR REPLACE FUNCTION ivymy.subtime("any", "any") RETURNS text 
LANGUAGE C STABLE STRICT PARALLEL SAFE 
as 'MODULE_PATHNAME', 'subtime';

/*TIMEDIFF*/
CREATE OR REPLACE FUNCTION ivymy.timediff("any", "any") 
RETURNS time(6) 
as 'MODULE_PATHNAME', 'timediff'
LANGUAGE C
STABLE
STRICT
PARALLEL SAFE;

/*TIME*/
CREATE OR REPLACE FUNCTION ivymy.time("any")
RETURNS time(6)
AS 'MODULE_PATHNAME', 'mystime'
LANGUAGE C
STABLE
STRICT
PARALLEL SAFE;

/*TIME_FORMAT*/
CREATE OR REPLACE FUNCTION ivymy.time_format(text, text) RETURNS text 
LANGUAGE C STABLE STRICT 
as 'MODULE_PATHNAME', 'time_format';

CREATE OR REPLACE FUNCTION ivymy.time_format(numeric, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.time_format(datetime, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.time_format(date, text) RETURNS text
AS $$
    SELECT ivymy.time_format(cast($1 as text), $2);
$$ LANGUAGE SQL STRICT PARALLEL SAFE;

/* Begin - BUG#7487 */

CREATE OR REPLACE FUNCTION ivymy.to_char(mysdatetime, text) RETURNS text
AS $$
    SELECT pg_catalog.to_char($1::pg_catalog.timestamp, $2);
$$ LANGUAGE SQL STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.to_char(mysdatetimes, text) RETURNS text
AS $$
    SELECT pg_catalog.to_char($1::pg_catalog.timestamp, $2);
$$ LANGUAGE SQL STABLE PARALLEL SAFE;
/* Begin - BUG#7487 */

/*TIMESTAMP*/
CREATE OR REPLACE FUNCTION ivymy.timestamp(text)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'timestamp_arg1'
LANGUAGE C
STABLE 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(numeric)
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text));
$$ 
LANGUAGE SQL 
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(datetime) 
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text));
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(date) 
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text));
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(text, text) 
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'timestamp_arg2'
LANGUAGE C 
STABLE 
STRICT
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(numeric, numeric) 
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text), cast($2 as text));
$$ 
LANGUAGE SQL 
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(datetime, datetime) 
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text), cast($2 as text));
$$ 
LANGUAGE SQL
STRICT 
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.timestamp(date, date) 
RETURNS datetime(6)
AS $$
    SELECT ivymy.timestamp(cast($1 as text), cast($2 as text));
$$ 
LANGUAGE SQL
STRICT 
PARALLEL SAFE;
/*TIMESTAMPADD*/
CREATE OR REPLACE FUNCTION ivymy.timestampadd(int, int, datetime(6))
RETURNS datetime(6)
as 'MODULE_PATHNAME', 'timestampadd'
LANGUAGE C 
STABLE 
STRICT 
PARALLEL SAFE;
/* End - SRS-MYSQL-BUILDINFUNCTION */

/* FIX BUG 4814 */
CREATE FUNCTION ivymy.substring(pg_catalog.text, pg_catalog.int4, pg_catalog.int8)
RETURNS pg_catalog.text
AS 'text_substr'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/***************************************************************
 *
 * Character Functions.
 *
 ***************************************************************/
-- locate

CREATE FUNCTION ivymy.locate(text, text)
RETURNS integer
AS 'MODULE_PATHNAME','locate'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.locate(text, text, bigint)
RETURNS integer
AS 'MODULE_PATHNAME','locate'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.locate(text, text, int)
RETURNS integer
AS $$
    SELECT ivymy.locate($1, $2, $3::bigint);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.locate(text, text, numeric)
RETURNS integer
AS $$
    SELECT ivymy.locate($1, $2, $3::bigint);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


-- lcase
CREATE FUNCTION ivymy.lcase(text)
RETURNS varchar
AS 'lower'
LANGUAGE internal
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.lcase(numeric)
RETURNS varchar
AS $$
select lcase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.lcase(int4)
RETURNS varchar
AS $$
select lcase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.lcase(ivy_hex)
RETURNS varchar
AS $$
select lcase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

-- ucase

CREATE FUNCTION ivymy.ucase(text)
RETURNS varchar
AS 'upper'
LANGUAGE internal
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.ucase(numeric)
RETURNS varchar
AS $$
select ucase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.ucase(int4)
RETURNS varchar
AS $$
select ucase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.ucase(ivy_hex)
RETURNS varchar
AS $$
select ucase($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

-- insert

CREATE FUNCTION ivymy.insert(text, bigint, bigint, text)
RETURNS varchar
AS 'MODULE_PATHNAME','insert'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.insert(text, text, text, text)
RETURNS varchar
AS $$
    select ivymy.insert($1, $2::bigint, $3::bigint, $4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.insert(text, bigint, text, text)
RETURNS varchar
AS $$
    select ivymy.insert($1, $2, $3::bigint, $4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.insert(text, text, bigint, text)
RETURNS varchar
AS $$
    select ivymy.insert($1, $2::bigint, $3, $4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

-- bin

CREATE FUNCTION ivymy.bin(text)
RETURNS varchar
AS 'MODULE_PATHNAME','bin_text'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bin(numeric)
RETURNS varchar
AS $$
select ivymy.bin($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(int8)
RETURNS varchar
AS $$
select ivymy.bin($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(int4)
RETURNS varchar
AS $$
select ivymy.bin($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

/* Begin - BUG#Z5504 */
CREATE FUNCTION ivymy.bin(mysdatetime)
RETURNS varchar
AS $$
select ivymy.bin(mysdatetime_out($1)::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(mysdatetimes)
RETURNS varchar
AS $$
select ivymy.bin(mysdatetimes_out($1)::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(bool)
RETURNS varchar
AS $$
select ivymy.bin($1::int4);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(float4)
RETURNS varchar
AS $$
select ivymy.bin($1::int8);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(float8)
RETURNS varchar
AS $$
select ivymy.bin($1::int8);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;
/* End - BUG#Z5504 */

CREATE FUNCTION ivymy.bin(bit)
RETURNS varchar
AS $$
select ivymy.bin($1::int8);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.bin(ivy_hex)
RETURNS varchar
AS $$
select ivymy.bin($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

-- char

CREATE FUNCTION ivymy.char(VARIADIC int8[])
RETURNS varbinary
AS 'MODULE_PATHNAME','myschar'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5545 */
CREATE FUNCTION ivymy.concat(VARIADIC "any")
RETURNS text
AS 'text_concat'
LANGUAGE internal
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5545 */

-- elt
CREATE FUNCTION ivymy.elt(bigint, VARIADIC text[])
RETURNS VARCHAR
AS 'MODULE_PATHNAME','elt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5513 */
CREATE FUNCTION ivymy.elt(text, VARIADIC text[])
RETURNS VARCHAR
AS 'select ivymy.elt($1::bigint, VARIADIC $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.elt(numeric, VARIADIC text[])
RETURNS VARCHAR
AS 'select ivymy.elt($1::bigint, VARIADIC $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.elt(float8, VARIADIC text[])
RETURNS VARCHAR
AS 'select ivymy.elt($1::bigint, VARIADIC $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5513 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.elt(uint8, VARIADIC text[])
RETURNS VARCHAR
AS 'select ivymy.elt($1::bigint, VARIADIC $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- field
CREATE FUNCTION ivymy.field(VARIADIC arr text[])
RETURNS int
AS 'MODULE_PATHNAME','field'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5516 */
CREATE FUNCTION ivymy.field(VARIADIC "any")
RETURNS int
AS 'MODULE_PATHNAME','field_any'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5516 */

-- find_in_set

CREATE FUNCTION ivymy.find_in_set(text, text)
RETURNS int
AS 'MODULE_PATHNAME','find_in_set'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.find_in_set(int, text)
RETURNS int
AS 'select ivymy.find_in_set($1::text, $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.find_in_set(anyenum, text)
RETURNS int
AS 'select ivymy.find_in_set($1::text, $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.find_in_set(uint8, text)
RETURNS int
AS 'select ivymy.find_in_set($1::text, $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.find_in_set(ivy_hex, text)
RETURNS int
AS 'select ivymy.find_in_set($1::text, $2)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.find_in_set(ivy_hex, ivy_hex)
RETURNS int
AS 'select ivymy.find_in_set($1::text, $2::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- hex
CREATE OR REPLACE FUNCTION ivymy.hex(text)
RETURNS varchar
AS 'MODULE_PATHNAME', 'hex_str'
LANGUAGE C 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(int8)
RETURNS varchar
AS 'MODULE_PATHNAME','hex_num'
LANGUAGE C 
PARALLEL SAFE 
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(int4)
RETURNS varchar
AS $$
select hex($1::int8);
$$ 
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(numeric)
RETURNS varchar
AS $$
select hex($1::int8);
$$ 
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(bool)
RETURNS varchar
AS $$
select hex($1::int);
$$ 
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(float8)
RETURNS varchar
AS $$
select hex($1::int8);
$$ 
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(date)
RETURNS varchar
AS $$
select hex($1::text);
$$ 
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(datetime)
RETURNS varchar
AS $$
select hex($1::text);
$$ 
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(time)
RETURNS varchar
AS $$
select hex($1::text);
$$ 
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(binary)
RETURNS varchar
AS 'MODULE_PATHNAME','hex_bytea'
LANGUAGE C 
PARALLEL SAFE 
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(varbinary)
RETURNS varchar
AS $$
select hex($1::binary);
$$
LANGUAGE SQL 
PARALLEL SAFE
IMMUTABLE STRICT;

/* Begin - BUG#Z5524 */
CREATE OR REPLACE FUNCTION ivymy.hex(pg_catalog.bit)
RETURNS varchar
AS $$
select hex($1::int8);
$$ 
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE STRICT;
/* End - BUG#Z5524 */

/* Begin - BUG#Z5524 */
CREATE OR REPLACE FUNCTION ivymy.hex(ivymy.mysbit)
RETURNS varchar
AS $$
select hex($1::int8);
$$ 
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE STRICT;
/* End - BUG#Z5524 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE OR REPLACE FUNCTION ivymy.hex(ivy_hex)
RETURNS varchar
AS $$
select hex($1::binary);
$$
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

-- space

CREATE FUNCTION ivymy.space(int8)
RETURNS varchar
AS 'MODULE_PATHNAME','space'
LANGUAGE C PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.space(int4)
RETURNS varchar
AS $$
select space($1::int8);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.space(numeric)
RETURNS varchar
AS $$
select space($1::int8);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.space(text)
RETURNS varchar
AS $$
select space($1::int8);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE OR REPLACE FUNCTION ivymy.space(ivy_hex)
RETURNS varchar
AS $$
select space($1::int8);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

-- soundex

CREATE FUNCTION ivymy.soundex(text)
RETURNS varchar
AS 'MODULE_PATHNAME','soundex'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(bit)
RETURNS varchar
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(bool)
RETURNS varchar
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(numeric)
RETURNS varchar
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.soundex(ivy_hex)
RETURNS varchar
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- export_set

CREATE FUNCTION ivymy.export_set(int8, text, text, text default ',', int default 64)
RETURNS varchar
AS 'MODULE_PATHNAME', 'export_set'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.export_set(text, text, text, text default ',', int default 64)
RETURNS varchar
AS $$
    select export_set($1::int8, $2, $3, $4, $5);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - BUG#Z5515 */
CREATE FUNCTION ivymy.export_set(int8, text, text, text, numeric)
RETURNS varchar
AS $$
    select export_set($1, $2, $3, $4, ROUND($5)::int);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE FUNCTION ivymy.export_set(int8, text, text, numeric, int)
RETURNS varchar
AS $$
    select export_set($1, $2, $3, $4::text, $5);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE FUNCTION ivymy.export_set(int8, text, text, numeric)
RETURNS varchar
AS $$
    select export_set($1, $2, $3, $4::text, 64);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - BUG#Z5515 */

-- ord
CREATE FUNCTION ivymy.ord(text)
RETURNS bigint
AS 'MODULE_PATHNAME','ord'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.ord(numeric)
RETURNS bigint
AS 'select ivymy.ord($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5536 */
CREATE FUNCTION ivymy.ord(varbinary)
RETURNS bigint
AS 'select ivymy.ord($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5536 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.ord(ivy_hex)
RETURNS bigint
AS 'select ivymy.ord($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- substring_index
CREATE FUNCTION ivymy.substring_index(text, text, bigint)
RETURNS varchar
AS 'MODULE_PATHNAME','substring_index'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.substring_index(text, text, int)
RETURNS varchar
AS $$
    SELECT ivymy.substring_index($1, $2, $3::bigint)
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.substring_index(text, text, numeric)
RETURNS varchar
AS $$
    SELECT ivymy.substring_index($1, $2, $3::bigint)
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

-- from_base64

CREATE FUNCTION ivymy.from_base64(text)
RETURNS varbinary
AS 'MODULE_PATHNAME','base64_decode'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.from_base64(ivy_hex)
RETURNS varchar
AS $$
    SELECT ivymy.from_base64($1::text)
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- to_base64

CREATE FUNCTION ivymy.to_base64(text)
RETURNS varchar
AS 'MODULE_PATHNAME','base64_encode'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5524 */
CREATE FUNCTION ivymy.to_base64(varbinary)
RETURNS varchar
AS $$
select to_base64($1::text);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5524 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.to_base64(ivy_hex)
RETURNS varchar
AS $$
select to_base64($1::text);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- length
CREATE FUNCTION ivymy.length(text)
RETURNS int
AS 'MODULE_PATHNAME','mys_text_length'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.length(numeric)
RETURNS int
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.length(datetime)
RETURNS int
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.length(date)
RETURNS int
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.length(ivy_hex)
RETURNS int
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

-- octet_length

CREATE FUNCTION ivymy.octet_length(numeric)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.octet_length(boolean)
RETURNS int4
AS 'select length($1::integer)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.octet_length(timestamp)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.octet_length(datetime)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.octet_length(time)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.octet_length(date)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5537 */
CREATE FUNCTION ivymy.octet_length(binary)
RETURNS int4
AS 'select length($1::bytea)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5537 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.octet_length(ivy_hex)
RETURNS int4
AS 'select length($1::text)'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- bit_length

CREATE FUNCTION ivymy.bit_length(int)
RETURNS bigint
AS $$
    select bit_length($1::text);
$$
LANGUAGE sql
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_length(datetime)
RETURNS bigint
AS $$
    select bit_length($1::text);
$$
LANGUAGE sql
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_length(date)
RETURNS bigint
AS $$
    select bit_length($1::text);
$$
LANGUAGE sql
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.bit_length(ivy_hex)
RETURNS bigint
AS $$
    select bit_length($1::text);
$$
LANGUAGE sql
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

-- left

CREATE OR REPLACE FUNCTION ivymy.left(numeric, numeric)
RETURNS varchar
AS $$
select left($1::text, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.left(text, text)
RETURNS varchar
AS $$
select left($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - BUG#Z5492 */
CREATE OR REPLACE FUNCTION ivymy.left(text, numeric)
RETURNS varchar
AS $$
select left($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - BUG#Z5492 */

CREATE OR REPLACE FUNCTION ivymy.left(text, bool)
RETURNS varchar
AS $$
select left($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.left(bool, int4)
RETURNS varchar
AS $$
select left($1::int4::text, $2);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE OR REPLACE FUNCTION ivymy.left(ivy_hex, int4)
RETURNS varchar
AS $$
select left($1::text, $2);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.left(text, ivy_hex)
RETURNS varchar
AS $$
select left($1, $2::int);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.left(ivy_hex, ivy_hex)
RETURNS varchar
AS $$
select left($1::text, $2::int);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

/* Begin - bug#5180 */
CREATE OR REPLACE FUNCTION ivymy.left(myblob, int4)
RETURNS varbinary
AS 'text_left'
LANGUAGE INTERNAL
PARALLEL SAFE IMMUTABLE STRICT;
/* End - bug#5180 */

-- right

CREATE OR REPLACE FUNCTION ivymy.right(numeric, numeric)
RETURNS varchar
AS $$
select right($1::text, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.right(text, text)
RETURNS varchar
AS $$
select right($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - BUG#Z5495 */
CREATE OR REPLACE FUNCTION ivymy.right(text, numeric)
RETURNS varchar
AS $$
select right($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - BUG#Z5495 */

CREATE OR REPLACE FUNCTION ivymy.right(text, bool)
RETURNS varchar
AS $$
select right($1, $2::int4);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.right(bool, int4)
RETURNS varchar
AS $$
select right($1::int4::text, $2);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE OR REPLACE FUNCTION ivymy.right(ivy_hex, int4)
RETURNS varchar
AS $$
select right($1::text, $2);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.right(text, ivy_hex)
RETURNS varchar
AS $$
select right($1, $2::int);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.right(ivy_hex, ivy_hex)
RETURNS varchar
AS $$
select right($1::text, $2::int);
$$ LANGUAGE SQL PARALLEL SAFE IMMUTABLE STRICT;
/* End - SRS-MYSQL-HEX-LITERAL */

--to_days
CREATE FUNCTION ivymy.to_days(date)
RETURNS bigint
AS 'MODULE_PATHNAME','to_days'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(datetime)
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(datetime(6))
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(numeric)
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(time)
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(time(6))
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(text)
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(timestamp)
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_days(timestamp(6))
RETURNS bigint
AS $$
	select ivymy.to_days($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.to_days(ivy_hex)
RETURNS bigint
AS $$
	select ivymy.to_days($1::text);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

--to_seconds
CREATE FUNCTION ivymy.to_seconds(datetime(6))
RETURNS bigint
AS 'MODULE_PATHNAME','to_seconds'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(datetime)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(date)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(timestamp)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(timestamp(6))
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(time)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(time(6))
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(numeric)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.to_seconds(text)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::datetime(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.to_seconds(ivy_hex)
RETURNS bigint
AS $$
	select ivymy.to_seconds($1::text);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

--unix_timestamp
CREATE FUNCTION ivymy.unix_timestamp()
RETURNS numeric
AS 'MODULE_PATHNAME','unix_timestamp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.unix_timestamp(datetime)
RETURNS numeric
AS 'MODULE_PATHNAME','unix_timestamp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.unix_timestamp(p1 text)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::datetime);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.unix_timestamp(p1 integer)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::datetime);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.unix_timestamp(p1 bigint)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::datetime);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.unix_timestamp(p1 numeric)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::datetime);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.unix_timestamp(p1 date)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::datetime);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.unix_timestamp(p1 time)
RETURNS numeric
IMMUTABLE 
AS $$
SELECT ivymy.unix_timestamp(p1::numeric::datetime);
$$ 
LANGUAGE sql;

--utc_date
CREATE FUNCTION ivymy.utc_date()
RETURNS date
AS 'MODULE_PATHNAME','utc_date_func'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

--utc_time
CREATE FUNCTION ivymy.utc_time(int)
RETURNS time(6)
AS 'MODULE_PATHNAME','utc_time_func'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

--utc_timestamp
CREATE FUNCTION ivymy.utc_timestamp(int)
RETURNS datetime(6) /* BUG#Z5564 */
AS 'MODULE_PATHNAME','utc_timestamp_func'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5533 */
--dayname
CREATE FUNCTION ivymy.dayname(date)
RETURNS varchar
AS 'MODULE_PATHNAME','dayname'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(datetime)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(datetime(6))
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(numeric)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(time)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(time(6))
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(text)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(timestamp)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(timestamp(6))
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.dayname(ivy_hex)
RETURNS varchar
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

--month
CREATE FUNCTION ivymy.monthname(date)
RETURNS varchar
AS 'MODULE_PATHNAME','monthname'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(datetime)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(datetime(6))
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(numeric)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(time)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(time(6))
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(text)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(timestamp)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(timestamp(6))
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5533 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.monthname(ivy_hex)
RETURNS varchar
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

/* Begin - BUG#Z5568 */
--time_to_sec
CREATE FUNCTION ivymy.time_to_sec(time(6))
RETURNS int
AS 'MODULE_PATHNAME','time_to_sec'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(time)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(text)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(numeric)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(datetime)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(datetime(6))
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(timestamp)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(timestamp(6))
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(date)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5568 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.time_to_sec(ivy_hex)
RETURNS int
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

--month
CREATE FUNCTION ivymy.month(date)
RETURNS int
AS 'MODULE_PATHNAME','mys_month'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(datetime)
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(datetime(6))
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(numeric)
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(time)
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(time(6))
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(text)
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(timestamp)
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.month(timestamp(6))
RETURNS int
AS $$
	select ivymy.month($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--date
CREATE FUNCTION ivymy.date(date)
RETURNS date
AS 'MODULE_PATHNAME','mys_date'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(datetime)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(datetime(6))
RETURNS date
AS $$
	select ivymy.date($1::ivymy.mysdate);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(numeric)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(time)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(time(6))
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(text)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(timestamp)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.date(timestamp(6))
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.date(ivy_hex)
RETURNS date
AS $$
	select ivymy.date($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

--week
CREATE FUNCTION ivymy.week(date)
RETURNS int
AS 'MODULE_PATHNAME','mys_week'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime(6))
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(numeric)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time(6))
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(text)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp(6))
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.week(ivy_hex)
RETURNS int
AS $$
	select ivymy.week($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */

/* Begin - BUG#Z5528 */
CREATE FUNCTION ivymy.week(date, int)
RETURNS int
AS 'MODULE_PATHNAME','mys_week_mode'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(numeric, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(text, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5528 */

/* Begin - BUG#Z5530 */
--yearweek
CREATE FUNCTION ivymy.yearweek(date, int)
RETURNS int /* BUG#Z5576 */
AS 'MODULE_PATHNAME','yearweek'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime, int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime(6), int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(numeric, int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time, int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time(6), int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(text, int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp, int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp(6), int)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5530 */

CREATE FUNCTION ivymy.yearweek(date)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime(6))
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(numeric)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time(6))
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(text)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp(6))
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.yearweek(ivy_hex)
RETURNS int /* BUG#Z5576 */
AS $$
	select ivymy.yearweek($1::date, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */


--datediff
CREATE FUNCTION ivymy.datediff(date, date)
RETURNS int /* BUG#Z5751 */
AS 'MODULE_PATHNAME','datediff'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(date, datetime)
RETURNS int
AS $$
	select ivymy.datediff($1, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, date)
RETURNS int
AS $$
	select ivymy.datediff($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, datetime)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), datetime(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time, time)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), time(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::ivymy.mysdate, $2::ivymy.mysdate);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(date, numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, date)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, datetime)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, datetime(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(ivymy.mystime, numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::ivymy.mysdate, $2::ivymy.mysdate);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, time)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, time(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(date, text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, date)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, ivymy.mysdatetime)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, datetime(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time, text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, time)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, time(6))
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, text)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, numeric)
RETURNS int /* BUG#Z5751 */
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.datediff(ivy_hex, ivy_hex)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */


--from_days
CREATE FUNCTION ivymy.from_days(bigint)
RETURNS date
AS 'MODULE_PATHNAME','from_days'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(int)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(numeric)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(date)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(text)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(datetime(6))
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(datetime)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(timestamp)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(timestamp(6))
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(time)
RETURNS date
AS $$
	select ivymy.from_days($1::numeric::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(time(6))
RETURNS date
AS $$
	select ivymy.from_days($1::numeric::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(uint8)
RETURNS date
AS $$
	select ivymy.from_days($1::bigint);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--convert_tz
CREATE FUNCTION ivymy.convert_tz(datetime(6), text, text)
RETURNS datetime(6)
AS 'MODULE_PATHNAME','convert_tz'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(datetime, text, text)
RETURNS datetime(6)
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(time, text, text)
RETURNS datetime(6)
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(time(6), text, text)
RETURNS datetime(6)
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(numeric, text, text)
RETURNS datetime(6)
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(text, text, text)
RETURNS datetime(6)
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.convert_tz(date, text, text)
RETURNS datetime(6) /* BUG#Z6775 */
AS $$
	select ivymy.convert_tz($1::datetime(6), $2, $3);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.get_format(int, text)
RETURNS varchar /* BUG#Z5533 */
AS 'MODULE_PATHNAME','get_format'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - SRS-MYSQL-HEX-LITERAL */
CREATE FUNCTION ivymy.get_format(int, ivy_hex)
RETURNS varchar /* BUG#Z5533 */
AS $$
	select ivymy.get_format($1, $2::text);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - SRS-MYSQL-HEX-LITERAL */


CREATE FUNCTION ivymy.conv(text, int4, int4)
RETURNS varchar
AS 'MODULE_PATHNAME', 'conv'
LANGUAGE C
IMMUTABLE
STRICT;

/* Begin - BUG#Z5509 */
CREATE FUNCTION ivymy.conv(text, numeric, numeric)
RETURNS varchar
AS $$
    select ivymy.conv($1, round($2)::int4, round($3)::int4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(text, float8, float8)
RETURNS varchar
AS $$
    select ivymy.conv($1, $2::numeric, $3::numeric);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;
/* End - BUG#Z5509 */

CREATE FUNCTION ivymy.conv(numeric, int4, int4)
RETURNS varchar
AS $$
    select ivymy.conv($1::text, $2, $3);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - BUG#Z5507 BUG#Z5508 */
CREATE FUNCTION ivymy.conv(text, text, text)
RETURNS varchar
AS $$
    select conv($1, $2::int4, $3::int4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(text, int4, text)
RETURNS varchar
AS $$
    select conv($1, $2, $3::int4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(text, text, int4)
RETURNS varchar
AS $$
    select conv($1, $2::int4, $3);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(numeric, text, text)
RETURNS varchar
AS $$
    select conv($1::text, $2::int4, $3::int4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(numeric, int4, text)
RETURNS varchar
AS $$
    select conv($1::text, $2, $3::int4);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(numeric, text, int4)
RETURNS varchar
AS $$
    select conv($1::text, $2::int4, $3);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

/* End - BUG#Z5507 BUG#Z5508 */

-- strcmp

CREATE FUNCTION ivymy.strcmp(text, text)
RETURNS int
AS 'MODULE_PATHNAME','mys_strcmp'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(int4, int4)
RETURNS int
AS $$
    select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(int8, int8)
RETURNS int
AS $$
    select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(numeric, numeric)
RETURNS int
AS $$
    select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(date, date)
RETURNS int
AS $$
	select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(float8, float8)
RETURNS int
AS $$
	select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(binary, binary)
RETURNS int
AS $$
	select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.strcmp(varbinary, varbinary)
RETURNS int
AS $$
	select ivymy.strcmp($1::text, $2::text);
$$
LANGUAGE SQL
STRICT
IMMUTABLE;

-- unhex

CREATE FUNCTION ivymy.unhex(text)
RETURNS varbinary
AS 'MODULE_PATHNAME','unhex'
LANGUAGE C
IMMUTABLE
PARALLEL
SAFE
STRICT;

/* Begin - BUG#Z5543 */
CREATE FUNCTION ivymy.unhex(binary)
RETURNS varbinary
AS 'MODULE_PATHNAME','unhex'
LANGUAGE C
IMMUTABLE
PARALLEL
SAFE
STRICT;
/* End - BUG#Z5543 */

CREATE FUNCTION ivymy.unhex(numeric)
RETURNS varbinary
AS $$
    select ivymy.unhex($1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.unhex(ivymy.mysbit)
RETURNS varbinary
AS $$
    select ivymy.unhex($1::text);
$$
LANGUAGE SQL
PARALLEL
SAFE
STRICT;

-- weight_string(input, flags, type, nweights)
CREATE FUNCTION ivymy.weight_string(text, int4, int4, int4)
RETURNS varbinary
AS 'MODULE_PATHNAME', 'weight_string'
LANGUAGE C
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(boolean, int4, int4, int4)
RETURNS varbinary
AS 'MODULE_PATHNAME', 'weight_string_boolean'
LANGUAGE C
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(date, int4, int4, int4)
RETURNS varbinary
AS $$
	select ivymy.weight_string($1::text, $2, $3, $4);
$$
LANGUAGE SQL
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(time, int4, int4, int4)
RETURNS varbinary
AS $$
	select ivymy.weight_string($1::text, $2, $3, $4);
$$
LANGUAGE SQL
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(datetime, int4, int4, int4)
RETURNS varbinary
AS $$
	select ivymy.weight_string($1::text, $2, $3, $4);
$$
LANGUAGE SQL
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(mystimestamp, int4, int4, int4)
RETURNS varbinary
AS $$
	select ivymy.weight_string($1::text, $2, $3, $4);
$$
LANGUAGE SQL
STABLE
STRICT;

CREATE FUNCTION ivymy.weight_string(numeric, int4, int4, int4)
RETURNS varbinary AS
$$
BEGIN
    RETURN NULL;
END;
$$
LANGUAGE plpgsql;

CREATE FUNCTION ivymy.weight_string(float8, int4, int4, int4)
RETURNS varbinary AS
$$
BEGIN
    RETURN NULL;
END;
$$
LANGUAGE plpgsql;


-- instr

CREATE FUNCTION ivymy.instr(text, text)
RETURNS integer
AS $$
	SELECT ivymy.locate($2, $1);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.instr(int4, int4)
RETURNS integer
AS $$
	SELECT ivymy.locate($2::text, $1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.instr(int8, int8)
RETURNS integer
AS $$
	SELECT ivymy.locate($2::text, $1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.instr(numeric, numeric)
RETURNS integer
AS $$
	SELECT ivymy.locate($2::text, $1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.instr(binary, binary)
RETURNS integer
AS $$
	SELECT ivymy.locate($2::text, $1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

-- interval
CREATE OR REPLACE FUNCTION ivymy.interval(VARIADIC numeric[])
RETURNS int 
as 'MODULE_PATHNAME', 'interval'
LANGUAGE C 
STABLE;

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
CREATE FUNCTION ivymy.LAST_INSERT_ID()
RETURNS ivymy.uint8 IMMUTABLE AS $$
SELECT mys_get_lastval();
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.LAST_INSERT_ID(seqid int8)
RETURNS ivymy.uint8 IMMUTABLE AS $$
SELECT mys_set_lastval(seqid);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.LAST_INSERT_ID(seqid float4)
RETURNS ivymy.uint8 IMMUTABLE AS $$
SELECT mys_set_lastval(seqid::int);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.LAST_INSERT_ID(seqid float8)
RETURNS ivymy.uint8 IMMUTABLE AS $$
SELECT mys_set_lastval(seqid::int);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.LAST_INSERT_ID(seqid numeric)
RETURNS ivymy.uint8 IMMUTABLE AS $$
SELECT mys_set_lastval(seqid::int);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.show_next_autoinc(Oid)
RETURNS uint8
AS 'MODULE_PATHNAME', 'show_next_autoinc'
LANGUAGE C
STRICT;
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */


/* Begin - Req: SRS-MYSQL-PROTOCOL */
CREATE FUNCTION pg_catalog.database()
RETURNS varchar
AS 'MODULE_PATHNAME', 'database'
LANGUAGE C;
/* End - Req: SRS-MYSQL-PROTOCOL */

/* Begin - bug#z6050 */
CREATE FUNCTION ivymy.char_length(ivy_hex)
RETURNS int
AS 'MODULE_PATHNAME', 'char_length_varlena'
LANGUAGE C
IMMUTABLE;
/* End - bug#z6050 */

CREATE FUNCTION ivymy.STR_TO_DATE(text, text)
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','str_to_date'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
/* Begin - bug#z6402 bug#6407 */
CREATE FUNCTION ivymy.STR_TO_DATE(p1 integer, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.STR_TO_DATE(p1 bigint, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.STR_TO_DATE(p1 numeric, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.STR_TO_DATE(p1 date, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.STR_TO_DATE(p1 time, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;

CREATE FUNCTION ivymy.STR_TO_DATE(p1 datetime, p2 text)
RETURNS ivymy.mysdatetimes 
IMMUTABLE 
AS $$
SELECT ivymy.STR_TO_DATE(p1::text, p2);
$$ 
LANGUAGE sql;
/* End - bug#z6402 #z6407*/

CREATE FUNCTION ivymy.timestampdiff(int, datetime(6), datetime(6))
RETURNS bigint 
AS 'MODULE_PATHNAME','timestampdiff'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-------------------------------------------------------------------------------------------------
-- mystime type function 
-------------------------------------------------------------------------------------------------
CREATE FUNCTION ivymy.mod(ivymy.mysdate, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mysdate)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mysdatetime, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mysdatetime)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mysdatetimes, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mysdatetimes)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimestamp, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mystimestamp)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimestamps, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mystimestamps)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystime, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mystime)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimes, year)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(year, ivymy.mystimes)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

-------------------------------------------------------------------------------------------------
-- mystime type hex function 
-------------------------------------------------------------------------------------------------
CREATE FUNCTION ivymy.mod(ivymy.mysdate, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mysdate)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mysdatetime, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mysdatetime)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mysdatetimes, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mysdatetimes)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimestamp, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mystimestamp)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimestamps, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mystimestamps)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystime, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mystime)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.mystimes, ivy_hex)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.mod(ivy_hex, ivymy.mystimes)
RETURNS uint8
AS 'select $1::int8 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* Begin - bug#6769 */
CREATE FUNCTION pg_catalog.acldefault(text, oid)
RETURNS aclitem[]
AS $$
	SELECT pg_catalog.acldefault($1::"char", $2);
$$
LANGUAGE SQL
IMMUTABLE
PARALLEL SAFE;
/* End - bug#6769 */

/* Begin - bug#6988 */
CREATE FUNCTION ivymy.upper(int8)
RETURNS varchar
AS $$
    SELECT pg_catalog.upper($1::text);
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;

CREATE FUNCTION ivymy.upper(int4)
RETURNS varchar
AS $$
    SELECT pg_catalog.upper($1::text);
$$ 
LANGUAGE SQL
STRICT
PARALLEL SAFE;
/* End - bug#6988 */

--rand(seed)
CREATE FUNCTION ivymy.rand(bigint)
RETURNS double 
AS 'MODULE_PATHNAME','rand_seed'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

--rand()->random()
CREATE FUNCTION ivymy.rand()
RETURNS double 
AS 'drandom'
LANGUAGE internal
PARALLEL RESTRICTED
VOLATILE;

CREATE FUNCTION ivymy.format(numeric, integer, text default 'en_US')
RETURNS text 
AS 'MODULE_PATHNAME','numeric_format'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.format(double, integer, text default 'en_US')
RETURNS text 
AS 'SELECT ivymy.format($1::numeric, $2, $3)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.format(text, integer, text default 'en_US')
RETURNS text 
AS $$
	SELECT ivymy.format($1::numeric, $2, $3);
$$
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

/* Begin Bug#8445 8447 */
CREATE OR REPLACE FUNCTION ivymy.format(numeric, numeric, text DEFAULT 'en_US')
RETURNS text
AS $$
    SELECT ivymy.format($1, trunc($2)::integer, $3);
$$
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE OR REPLACE FUNCTION ivymy.format(boolean, integer, text DEFAULT 'en_US'::text)
RETURNS text
AS $$
    SELECT ivymy.format(CASE WHEN $1 THEN 1::numeric ELSE 0::numeric END, $2, $3);
$$
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;

CREATE OR REPLACE FUNCTION ivymy.format(boolean, numeric, text DEFAULT 'en_US'::text)
RETURNS text
AS $$
    SELECT ivymy.format(CASE WHEN $1 THEN 1::numeric ELSE 0::numeric END,
                        trunc($2)::integer,
                        $3);
$$
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE;
/* End Bug#8445 8447 */

CREATE FUNCTION ivymy.oct(text)
RETURNS varchar
AS 'MODULE_PATHNAME','oct'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.oct(bigint)
RETURNS varchar
AS $$
	SELECT ivymy.oct($1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.oct(ivymy.mysbit)
RETURNS varchar
AS $$
	SELECT ivymy.oct($1::bigint);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


CREATE FUNCTION ivymy.oct(ivy_hex)
RETURNS varchar
AS $$
	SELECT ivymy.oct($1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


CREATE FUNCTION ivymy.oct(numeric)
RETURNS varchar
AS $$
	/* Begin Bug#8448 */
	SELECT ivymy.oct(trunc($1)::bigint);
	/* End Bug#8448 */
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


CREATE FUNCTION ivymy.oct(double precision)
RETURNS varchar
AS $$
	/* Begin Bug#8448 */
	SELECT ivymy.oct(trunc($1)::bigint);
	/* End Bug#8448 */
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.aes_encrypt(text, text, text default NULL, text default '', text default '', text default '')
RETURNS varbinary
AS 'MODULE_PATHNAME','ivymy_aes_encrypt'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.aes_decrypt(text, text, text default '', text default '', text default '', text default '')
RETURNS varbinary
AS 'MODULE_PATHNAME','ivymy_aes_decrypt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Begin Bug#Z7500 */
CREATE FUNCTION ivymy.aes_decrypt(varbinary, text, text default '', text default '', text default '', text default '')
RETURNS varbinary
AS 'MODULE_PATHNAME','bytea_aes_decrypt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
/* End Bug#Z7500 */

/* Begin Bug#Z7502 */
CREATE FUNCTION ivymy.aes_encrypt(numeric, text, text default NULL, text default '', text default '', text default '')
RETURNS varbinary
AS $$
SELECT ivymy.aes_encrypt($1::text, $2, $3, $4, $5, $6);
$$
LANGUAGE sql
PARALLEL SAFE
IMMUTABLE;
/* End Bug#Z7502 */

/* Begin - OA#2098  Not Full Group */
CREATE FUNCTION ivymy.any_value_transfn(internal, anyelement)
RETURNS internal
AS 'MODULE_PATHNAME','any_value_transfn'
LANGUAGE C;

CREATE FUNCTION ivymy.any_value_finalfn(internal, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','any_value_finalfn'
LANGUAGE C;

CREATE AGGREGATE ivymy.any_value(anyelement) (
  SFUNC = ivymy.any_value_transfn,
  STYPE = internal,
  FINALFUNC_EXTRA,
  FINALFUNC = ivymy.any_value_finalfn
);
/* End - OA#2908  Not Full Group */

/* Begin - bug#7234 */
CREATE FUNCTION ivymy.group_concat_transfn(internal, text, VARIADIC "any")
RETURNS internal
AS 'MODULE_PATHNAME','group_concat_transfn'
LANGUAGE C;

CREATE FUNCTION ivymy.group_concat_finalfn(internal, text, VARIADIC "any")
RETURNS text
AS 'MODULE_PATHNAME','group_concat_finalfn'
LANGUAGE C;

CREATE AGGREGATE ivymy.group_concat(text, VARIADIC "any") (
  SFUNC = ivymy.group_concat_transfn,
  STYPE = internal,
  FINALFUNC_EXTRA,
  FINALFUNC = ivymy.group_concat_finalfn
);
/* End - bug#7234 */

/* Begin - SRS-MYSQL-BUILDINFUNCTION */
-- V920 MySQL datetime function
-- CREATE FUNCTION date_format
CREATE OR REPLACE FUNCTION ivymy.date_format(p1 ivymy.mysdatetimes, p2 text)
RETURNS text
AS 'MODULE_PATHNAME', 'date_format'
LANGUAGE C STRICT IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.date_format(p1 date, p2 text)
RETURNS text
IMMUTABLE STRICT
AS $$
SELECT date_format(p1::ivymy.mysdatetimes, p2);
$$
LANGUAGE sql
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.date_format(p1 datetime, p2 text)
RETURNS text
IMMUTABLE STRICT
AS $$
SELECT date_format(p1::ivymy.mysdatetimes, p2);
$$
LANGUAGE sql
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.date_format(p1 text, p2 text)
RETURNS text
IMMUTABLE STRICT
AS $$
SELECT date_format(p1::ivymy.mysdatetimes, p2);
$$
LANGUAGE sql
PARALLEL SAFE;

/* Begin - bug#z8076 8075 8074*/
CREATE OR REPLACE FUNCTION ivymy.date_format(p1 bigint, p2 text)
RETURNS text
IMMUTABLE STRICT
AS $$
SELECT date_format(p1::ivymy.mysdatetimes, p2);
$$
LANGUAGE sql
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.date_format(p1 time(6), p2 text)
RETURNS text
IMMUTABLE STRICT
AS $$
SELECT date_format(p1::ivymy.mysdatetimes, p2);
$$
LANGUAGE sql
PARALLEL SAFE;
/* End - bug#z8076 8075 8074*/

-- CREATE FUNCTION FROM_UNIXTIME
CREATE FUNCTION ivymy.from_unixtime(bigint)
RETURNS datetime
AS 'MODULE_PATHNAME', 'from_unixtime_int'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.from_unixtime(numeric)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'from_unixtime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.from_unixtime(double precision)
RETURNS datetime(6)
AS 'MODULE_PATHNAME', 'from_unixtime_float'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.from_unixtime(double precision, text)
RETURNS text
AS 'MODULE_PATHNAME', 'from_unixtime_format'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- CREATE FUNCTION adddate
CREATE FUNCTION ivymy.adddate(pg_catalog.timestamp, pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','timestamp_adddate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(datetime(6), pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','datetimes_adddate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(datetime, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(date, pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','date_adddate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(text, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(numeric, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(pg_catalog.timestamp, int)
RETURNS text
AS 'MODULE_PATHNAME','timestamp_adddate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(datetime(6), int)
RETURNS text
AS 'MODULE_PATHNAME','datetimes_adddate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(date, int)
RETURNS text
AS 'MODULE_PATHNAME','date_adddate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(numeric, int)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(text, int)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.adddate(datetime, int)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

/* Begin - bug#z8088 */
CREATE FUNCTION ivymy.adddate(text, text)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2::int)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;
/* End - bug#z8088 */

-- CREATE FUNCTION date_add
CREATE FUNCTION ivymy.date_add(datetime, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.date_add(date, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.date_add(text, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.date_add(numeric, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.adddate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

-- CREATE FUNCTION subdate
CREATE FUNCTION ivymy.subdate(pg_catalog.timestamp, pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','timestamp_subdate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(datetime(6), pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','datetimes_subdate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(datetime, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(date, pg_catalog.interval)
RETURNS text
AS 'MODULE_PATHNAME','date_subdate_interval'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(text, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(numeric, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(pg_catalog.timestamp, int)
RETURNS text
AS 'MODULE_PATHNAME','timestamp_subdate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(datetime(6), int)
RETURNS text
AS 'MODULE_PATHNAME','datetimes_subdate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(date, int)
RETURNS text
AS 'MODULE_PATHNAME','date_subdate_days'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(numeric, int)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(text, int)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.subdate(datetime, int)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

/* Begin - bug#z8088 */
CREATE FUNCTION ivymy.subdate(text, text)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2::int)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;
/* End - bug#z8088 */

-- CREATE FUNCTION date_sub
CREATE FUNCTION ivymy.date_sub(datetime, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.date_sub(date, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE FUNCTION ivymy.date_sub(text, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

/* Begin - bug#8427 */
CREATE FUNCTION ivymy.date_sub(numeric, pg_catalog.interval)
RETURNS text
AS 'SELECT ivymy.subdate($1::datetime(6), $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;
/* End - bug#8427 */
/* End - SRS-MYSQL-BUILDINFUNCTION */

/* Begin - ReqID: SRS-MYSQL-PROC */
CREATE FUNCTION ivymy.number_to_sql_mode(int8)
RETURNS text
AS 'MODULE_PATHNAME','number_to_sql_mode'
LANGUAGE C;

CREATE FUNCTION ivymy.number_to_sql_mode(uint8)
RETURNS text
AS 'MODULE_PATHNAME','number_to_sql_mode'
LANGUAGE C;
/* End - ReqID: SRS-MYSQL-PROC */

/* Begin - bug#8406 */
CREATE FUNCTION ivymy.row_count()
RETURNS bigint
AS 'MODULE_PATHNAME','row_count'
LANGUAGE C
VOLATILE
PARALLEL SAFE;
/* End - bug#8406 */

/* Begin - bug#8562 */
CREATE FUNCTION ivymy.found_rows()
RETURNS bigint
AS 'MODULE_PATHNAME','found_rows'
LANGUAGE C
VOLATILE
PARALLEL SAFE;
/* End - bug#8562 */

/* Begin - bug#8563 */
CREATE OR REPLACE FUNCTION ivymy.collation(anyelement)
RETURNS varchar
AS 'pg_collation_for'
LANGUAGE INTERNAL
STRICT
IMMUTABLE
PARALLEL SAFE;
/* End - bug#8563 */

/* Begin - SXJK-BUILDINFUNCTION */
CREATE OR REPLACE FUNCTION ivymy.crc32(varbinary)
RETURNS int8
AS 'MODULE_PATHNAME','crc32'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(text)
RETURNS int8
AS 'SELECT ivymy.crc32($1::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(int8)
RETURNS int8
AS 'SELECT ivymy.crc32($1::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(int4)
RETURNS int8
AS 'SELECT ivymy.crc32($1::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(numeric)
RETURNS int8
AS 'SELECT ivymy.crc32($1::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(ivy_hex)
RETURNS int8
AS 'SELECT ivymy.crc32($1::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.crc32(boolean)
RETURNS int8
AS 'SELECT ivymy.crc32($1::int8::ivymy.varbinary)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(numeric, int4)
RETURNS numeric
AS 'MODULE_PATHNAME','numeric_truncate'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(double, int4)
RETURNS double
AS 'MODULE_PATHNAME','double_truncate'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(int8, int4)
RETURNS int8
AS 'MODULE_PATHNAME','int_truncate'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(int8, numeric)
RETURNS int8
AS 'SELECT ivymy.truncate($1, $2::int4)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(int4, int4)
RETURNS int4
AS 'SELECT ivymy.truncate($1::int8, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(int4, numeric)
RETURNS int4
AS 'SELECT ivymy.truncate($1, $2::int4)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(numeric, numeric)
RETURNS numeric
AS 'SELECT ivymy.truncate($1, $2::int4)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(ivy_hex, numeric)
RETURNS double
AS 'SELECT ivymy.truncate($1::double, $2::int4)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(text, int4)
RETURNS double
AS 'SELECT ivymy.truncate($1::double, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.truncate(boolean, int4)
RETURNS int4
AS 'SELECT ivymy.truncate($1::int4, $2)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(double)
RETURNS double
AS 'MODULE_PATHNAME','double_sqrt'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(numeric)
RETURNS double
AS 'SELECT ivymy.sqrt($1::double)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(int8)
RETURNS double
AS 'SELECT ivymy.sqrt($1::double)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(int4)
RETURNS double
AS 'SELECT ivymy.sqrt($1::double)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(text)
RETURNS double
AS 'SELECT ivymy.sqrt($1::double)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(boolean)
RETURNS double
AS 'SELECT ivymy.sqrt($1::int4)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sqrt(ivy_hex)
RETURNS double
AS 'SELECT ivymy.sqrt($1::double)'
LANGUAGE SQL
STRICT
IMMUTABLE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sleep(double)
RETURNS int8
AS 'MODULE_PATHNAME','mys_sleep'
LANGUAGE C
VOLATILE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sleep(text)
RETURNS int8
AS 'SELECT ivymy.sleep($1::double)'
LANGUAGE SQL
VOLATILE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sleep(boolean)
RETURNS int8
AS 'SELECT ivymy.sleep($1::double)'
LANGUAGE SQL
VOLATILE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.sleep(ivy_hex)
RETURNS int8
AS 'SELECT ivymy.sleep($1::double)'
LANGUAGE SQL
VOLATILE
PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.current_user()
RETURNS VARCHAR
LANGUAGE SQL
STABLE
PARALLEL SAFE
AS $$
	SELECT concat(CURRENT_USER, '@%') as `CURRENT_USER()`;
$$;
/* End - SXJK-BUILDINFUNCTION */