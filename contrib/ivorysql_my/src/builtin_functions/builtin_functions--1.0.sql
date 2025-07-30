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
    SELECT ivymy.period_diff(cast($1 as numeric), cast($2 as numeric));
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
    SELECT sec_to_time(cast($1 as numeric));
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

/*TIMESTAMPADD*/
CREATE OR REPLACE FUNCTION ivymy.timestampadd(text, int, datetime(6))
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

CREATE FUNCTION ivymy.bin(int8)
RETURNS varchar
AS 'MODULE_PATHNAME','to_bin64'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

-- cannot cast numeric to int8, because mysql will do round off
CREATE FUNCTION ivymy.bin(numeric)
RETURNS varchar
AS 'MODULE_PATHNAME','bin_num'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bin(int4)
RETURNS varchar
AS $$
select ivymy.bin($1::int8);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(text)
RETURNS varchar
AS 'MODULE_PATHNAME','bin_text'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bin(datetime)
RETURNS varchar
AS $$
select ivymy.bin($1::text);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION ivymy.bin(bit)
RETURNS varchar
AS $$
select ivymy.bin($1::int8);
$$ LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

-- char

CREATE FUNCTION ivymy.char(VARIADIC int[])
RETURNS varbinary
AS 'MODULE_PATHNAME','myschar'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

-- elt
CREATE FUNCTION ivymy.elt(bigint, VARIADIC text[])
RETURNS VARCHAR
AS 'MODULE_PATHNAME','elt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

-- field
CREATE FUNCTION ivymy.field(VARIADIC arr text[])
RETURNS int
AS 'MODULE_PATHNAME','field'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

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

-- hex
CREATE OR REPLACE FUNCTION ivymy.hex(text)
RETURNS varchar
AS 'MODULE_PATHNAME', 'hex_str'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(int8)
RETURNS varchar
AS 'MODULE_PATHNAME','hex_num'
LANGUAGE C PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(int4)
RETURNS varchar
AS $$
select hex($1::int8);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(numeric)
RETURNS varchar
AS $$
select hex($1::int8);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(bool)
RETURNS varchar
AS $$
select hex($1::int);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(float8)
RETURNS varchar
AS $$
select hex($1::int8);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(date)
RETURNS varchar
AS $$
select hex($1::text);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(datetime)
RETURNS varchar
AS $$
select hex($1::text);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(time)
RETURNS varchar
AS $$
select hex($1::text);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION ivymy.hex(binary)
RETURNS varchar
AS 'MODULE_PATHNAME','hex_bytea'
LANGUAGE C PARALLEL SAFE IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.hex(varbinary)
RETURNS varchar
AS $$
select hex($1::binary);
$$ LANGUAGE SQL IMMUTABLE STRICT;

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


-- soundex

CREATE FUNCTION ivymy.soundex(text)
RETURNS VARCHAR
AS 'MODULE_PATHNAME','soundex'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(bit)
RETURNS VARCHAR
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(bool)
RETURNS VARCHAR
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.soundex(numeric)
RETURNS VARCHAR
AS $$
    SELECT ''
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


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


-- to_base64

CREATE FUNCTION ivymy.to_base64(text)
RETURNS varchar
AS 'MODULE_PATHNAME','base64_encode'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;


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

--unix_timestamp
CREATE FUNCTION ivymy.unix_timestamp()
RETURNS numeric
AS 'MODULE_PATHNAME','unix_timestamp_no_args'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

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
RETURNS timestamp(6)
AS 'MODULE_PATHNAME','utc_timestamp_func'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

--dayname
CREATE FUNCTION ivymy.dayname(date)
RETURNS text
AS 'MODULE_PATHNAME','dayname'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(datetime)
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(datetime(6))
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(numeric)
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(time)
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(time(6))
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(text)
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(timestamp)
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.dayname(timestamp(6))
RETURNS text
AS $$
	select ivymy.dayname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--month
CREATE FUNCTION ivymy.monthname(date)
RETURNS text
AS 'MODULE_PATHNAME','monthname'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(datetime)
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(datetime(6))
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(numeric)
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(time)
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(time(6))
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(text)
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(timestamp)
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.monthname(timestamp(6))
RETURNS text
AS $$
	select ivymy.monthname($1::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--time_to_sec
CREATE FUNCTION ivymy.time_to_sec(time(6))
RETURNS bigint
AS 'MODULE_PATHNAME','time_to_sec'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(time)
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(text)
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(numeric)
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(datetime)
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(datetime(6))
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(timestamp)
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.time_to_sec(timestamp(6))
RETURNS bigint
AS $$
	select ivymy.time_to_sec($1::time(6));
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


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

CREATE FUNCTION ivymy.week(date, int)
RETURNS int
AS 'MODULE_PATHNAME','mys_week_mode'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(datetime(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(numeric, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(time(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(text, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp, int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.week(timestamp(6), int)
RETURNS int
AS $$
	select ivymy.week($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--yearweek
CREATE FUNCTION ivymy.yearweek(date, int)
RETURNS bigint
AS 'MODULE_PATHNAME','yearweek'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime, int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime(6), int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(numeric, int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time, int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time(6), int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(text, int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp, int)
RETURNS int
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp(6), int)
RETURNS bigint
AS $$
	select ivymy.yearweek($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(date)
RETURNS int
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime)
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(datetime(6))
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(numeric)
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time)
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(time(6))
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(text)
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp)
RETURNS int
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.yearweek(timestamp(6))
RETURNS bigint
AS $$
	select ivymy.yearweek($1, 0);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--datediff
CREATE FUNCTION ivymy.datediff(date, date)
RETURNS bigint
AS 'MODULE_PATHNAME','datediff'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, datetime)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), datetime(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time, time)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), time(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::ivymy.mysdate, $2::ivymy.mysdate);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(date, numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, date)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, datetime)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, datetime(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(ivymy.mystime, numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::ivymy.mysdate, $2::ivymy.mysdate);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, time)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, time(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(date, text)
RETURNS bigint
AS $$
	select ivymy.datediff($1, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, date)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime, text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, ivymy.mysdatetime)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(datetime(6), text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, datetime(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time, text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, time)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(time(6), text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, time(6))
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(numeric, text)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.datediff(text, numeric)
RETURNS bigint
AS $$
	select ivymy.datediff($1::date, $2::date);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

--from_days
CREATE FUNCTION ivymy.from_days(numeric)
RETURNS date
AS 'MODULE_PATHNAME','from_days'
LANGUAGE C
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
	select ivymy.from_days($1::numeric);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(datetime)
RETURNS date
AS $$
	select ivymy.from_days($1::numeric);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(timestamp)
RETURNS date
AS $$
	select ivymy.from_days($1::numeric);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(timestamp(6))
RETURNS date
AS $$
	select ivymy.from_days($1::numeric);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(time)
RETURNS date
AS $$
	select ivymy.from_days($1::numeric);
$$ LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.from_days(time(6))
RETURNS date
AS $$
	select ivymy.from_days($1::numeric);
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

CREATE FUNCTION ivymy.get_format(int, text)
RETURNS text
AS 'MODULE_PATHNAME','get_format'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.replace(int8, int8, int8)
RETURNS varchar
AS $$
    select replace($1::text, $2::text, $3::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.replace(int4, int4, int4)
RETURNS varchar
AS $$
    select replace($1::text, $2::text, $3::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.replace(numeric, numeric, numeric)
RETURNS varchar
AS $$
    select replace($1::text, $2::text, $3::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.conv(text, int4, int4)
RETURNS varchar
AS 'MODULE_PATHNAME', 'conv'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.conv(numeric, int4, int4)
RETURNS varchar
AS $$
    select ivymy.conv($1::text, $2, $3);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;


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


CREATE FUNCTION ivymy.unhex(text)
RETURNS varbinary
AS 'MODULE_PATHNAME','unhex'
LANGUAGE C
IMMUTABLE
PARALLEL
SAFE
STRICT;

CREATE FUNCTION ivymy.unhex(numeric)
RETURNS varbinary
AS $$
    select ivymy.unhex($1::text);
$$
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

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


-- ifnull

CREATE OR REPLACE FUNCTION ivymy.ifnull(tinyint, tinyint)
RETURNS tinyint
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(int2, int2)
RETURNS int2
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(int4, int4)
RETURNS int4
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(int8, int8)
RETURNS int8
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(bool, bool)
RETURNS bool
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(text, text)
RETURNS text
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(varchar, varchar)
RETURNS varchar
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(char, char)
RETURNS char
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(float, float)
RETURNS float
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(float8, float8)
RETURNS float8
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(numeric, numeric)
RETURNS numeric
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(time, time)
RETURNS time
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(date, date)
RETURNS date
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE OR REPLACE FUNCTION ivymy.ifnull(timestamp, timestamp)
RETURNS timestamp
LANGUAGE sql
AS $$ SELECT CASE WHEN $1 IS NOT NULL THEN $1 ELSE $2 END; $$;

CREATE FUNCTION ivymy.ifnull(p1 pg_catalog.bool, p2 text)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 text, p2 int4)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1, p2::text);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 int2, p2 text)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 int4, p2 text)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 bigint, p2 text)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 numeric, p2 text)
RETURNS text IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 date, p2 text)
RETURNS timestamp IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 time, p2 text)
RETURNS timestamp IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 timestamp, p2 text)
RETURNS timestamp IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;

CREATE FUNCTION ivymy.ifnull(p1 timestamptz, p2 text)
RETURNS timestamptz IMMUTABLE AS $$
SELECT coalesce(p1::text, p2);
$$ LANGUAGE sql;
