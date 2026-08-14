-- V920 MySQL datetime function
set TIMEZONE='Asia/Shanghai';
-- Test DATE_FORMAT function
-- Test basic functionality
SELECT date_format('2023-12-25 10:30:45', '%Y-%m-%d') AS date_only;
SELECT date_format('2023-12-25 10:30:45', '%H:%i:%s') AS time_only;
SELECT date_format('2023-12-25 10:30:45', '%Y-%m-%d %H:%i:%s') AS datetime_full;

-- Test various format specifiers
SELECT date_format('2023-12-25 10:30:45', '%Y') AS year_full;
SELECT date_format('2023-12-25 10:30:45', '%y') AS year_short;
SELECT date_format('2023-12-25 10:30:45', '%m') AS month_2digits;
SELECT date_format('2023-12-25 10:30:45', '%c') AS month_numeric;
SELECT date_format('2023-12-25 10:30:45', '%d') AS day_2digits;
SELECT date_format('2023-12-25 10:30:45', '%e') AS day_numeric;
SELECT date_format('2023-12-25 10:30:45', '%H') AS hour_24_2digits;
SELECT date_format('2023-12-25 10:30:45', '%h') AS hour_12_2digits;
SELECT date_format('2023-12-25 10:30:45', '%k') AS hour_24_numeric;
SELECT date_format('2023-12-25 10:30:45', '%l') AS hour_12_numeric;
SELECT date_format('2023-12-25 10:30:45', '%i') AS minutes;
SELECT date_format('2023-12-25 10:30:45', '%s') AS seconds;
SELECT date_format('2023-12-25 10:30:45', '%p') AS ampm;

-- Test combined format specifiers
SELECT date_format('2023-12-25 10:30:45', '%r') AS time_12hour;
SELECT date_format('2023-12-25 10:30:45', '%T') AS time_24hour;

-- Test weekday and month names
SELECT date_format('2023-12-25 10:30:45', '%W') AS weekday_full;
SELECT date_format('2023-12-25 10:30:45', '%a') AS weekday_abbreviated;
SELECT date_format('2023-12-25 10:30:45', '%w') AS weekday_number;
SELECT date_format('2023-12-25 10:30:45', '%M') AS month_full;
SELECT date_format('2023-12-25 10:30:45', '%b') AS month_abbreviated;

-- Test day with suffix
SELECT date_format('2023-12-25 10:30:45', '%D') AS day_with_suffix;
SELECT date_format('2023-12-01 10:30:45', '%D') AS day_with_suffix_1st;
SELECT date_format('2023-12-02 10:30:45', '%D') AS day_with_suffix_2nd;
SELECT date_format('2023-12-03 10:30:45', '%D') AS day_with_suffix_3rd;

-- Test day of year
SELECT date_format('2023-12-31 10:30:45', '%j') AS day_of_year;

-- Test week numbers
SELECT date_format('2023-12-25 10:30:45', '%U') AS week_number_sunday;
SELECT date_format('2023-12-25 10:30:45', '%u') AS week_number_monday;

-- Test year for week number
SELECT date_format('2023-12-25 10:30:45', '%X') AS year_for_week_sunday;
SELECT date_format('2023-12-25 10:30:45', '%x') AS year_for_week_monday;

-- Test literal %
SELECT date_format('2023-12-25 10:30:45', 'Year: %Y%%, Month: %m%%') AS literal_percent;

-- Test with date type
SELECT date_format('2023-12-25', '%Y-%m-%d') AS date_type_test;

-- Test with time type
SELECT date_format('10:30:45', '%H:%i:%s') AS time_type_test;

-- Test with timestamp type
SELECT date_format('2023-12-25 10:30:45', '%Y-%m-%d %H:%i:%s') AS timestamp_type_test;

-- Test edge cases
SELECT date_format('2024-02-29 12:00:00', '%Y-%m-%d') AS leap_day;
SELECT date_format('2023-01-01 00:00:00', '%H:%i:%s') AS midnight;
SELECT date_format('2023-12-31 23:59:59', '%H:%i:%s') AS new_year_eve;

drop table if exists t1;
SET sql_mode = default;

-- #
-- # Test of date_format()
-- #
create table t1 (d date);
insert into t1 values ('2004-07-14'),('2005-07-14');
select date_format(d,'%d') from t1 order by 1;
drop table t1;


-- #
-- # Test of locale dependent date format (WL#2928 Date Translation NRE)
-- #
set names latin1;
select date_format('2004-01-01','%W (%a), %e %M (%b) %Y');
set lc_time_names=ru_RU;
set names koi8r;
--select date_format('2004-01-01','%W (%a), %e %M (%b) %Y');
set lc_time_names=de_DE;
set names latin1;
select date_format('2004-01-01','%W (%a), %e %M (%b) %Y');
set names latin1;
set lc_time_names=en_US;

-- #
-- # Bug #14016 
-- #
create table t1 (f1 datetime);
insert into t1 (f1) values ('2005-01-01');
insert into t1 (f1) values ('2005-02-01');
select date_format(f1, '%m') as d1, date_format(f1, '%M') as d2 from t1 order by date_format(f1, '%M');
drop table t1;

-- #
-- # Bug#20729:  Bad date_format() call makes mysql server crash
-- #
SELECT DATE_FORMAT('%Y-%m-%d %H:%i:%s', 1151414896);

-- #
-- # Bug #41470: DATE_FORMAT() crashes the complete server with a valid date
-- #

-- # show that these two do not crash the server:
SELECT DATE_FORMAT('0000-01-01','%W %d %M %Y') as valid_date;
SELECT DATE_FORMAT('0000-02-28','%W %d %M %Y') as valid_date;
-- # show that date within the Gregorian range render correct results: (THU)
SELECT DATE_FORMAT('2009-01-01','%W %d %M %Y') as valid_date;

/* Begin - BUG#8076 8075 8074*/
set sql_mode= '';
SELECT DATE_FORMAT(00000000, '%m');
set sql_mode = default;
SELECT DATE_FORMAT('0000-00-00', '%m');
SELECT DATE_FORMAT(00000000, '%m');
SELECT DATE_FORMAT(CAST('23:59:59.999999' AS TIME(6)), '%H:%i:%s.%f') AS max_time;
/* End - BUG#8076 8075 8074*/

/* Begin - BUG#8071*/
SELECT DATE_FORMAT('1999-01-01', '%X');
/* End - BUG#8071*/

/* Begin - BUG#8076 8075 8074*/
SELECT DATE_FORMAT('2025-01-01', '%U');
SELECT DATE_FORMAT('2025-01-01', '%V');
/* End - BUG#8076 8075 8074*/

-- Test FROM_UNIXTIME function with precision handling
-- Test FROM_UNIXTIME with integer timestamp (precision 0)
SELECT ivymy.from_unixtime(0::bigint) AS epoch;
SELECT ivymy.from_unixtime(1000000000::bigint) AS billion_seconds;
SELECT ivymy.from_unixtime(1609459200::bigint) AS start_of_2021;
-- Test FROM_UNIXTIME with decimal timestamp (precision matches input)
SELECT ivymy.from_unixtime(1000000000.1::numeric) AS precision_1;
SELECT ivymy.from_unixtime(1000000000.12::numeric) AS precision_2;
SELECT ivymy.from_unixtime(1000000000.123::numeric) AS precision_3;
SELECT ivymy.from_unixtime(1000000000.1234::numeric) AS precision_4;
SELECT ivymy.from_unixtime(1000000000.12345::numeric) AS precision_5;
SELECT ivymy.from_unixtime(1000000000.123456::numeric) AS precision_6;
SELECT ivymy.from_unixtime(1000000000.1234567::numeric) AS precision_7_truncated;
-- Test FROM_UNIXTIME with float timestamp (precision 6, returns text)
SELECT ivymy.from_unixtime(1000000000.5::float8) AS float_half_second;
SELECT ivymy.from_unixtime(1000000000.0::float8) AS float_zero_fractional;
SELECT ivymy.from_unixtime(1000000000.1::float8) AS float_one_digit;
SELECT ivymy.from_unixtime(1000000000.12::float8) AS float_two_digits;
-- Test FROM_UNIXTIME with format
SELECT ivymy.from_unixtime(1000000000::float8, '%Y-%m-%d %H:%i:%s') AS formatted;
SELECT ivymy.from_unixtime(1000000000::float8, '%Y-%m-%d') AS date_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%H:%i:%s') AS time_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%W %M %Y') AS weekday_month_year;
-- Test FROM_UNIXTIME with NULL
SELECT ivymy.from_unixtime(NULL::bigint) AS null_input;
-- Test FROM_UNIXTIME with negative timestamp (before 1970)
SELECT ivymy.from_unixtime(-1000000000::bigint) AS before_epoch;
-- Test FROM_UNIXTIME with very large timestamp
SELECT ivymy.from_unixtime(2000000000::bigint) AS future_timestamp;
-- Test FROM_UNIXTIME with format and NULL
SELECT ivymy.from_unixtime(1000000000::float8, NULL) AS null_format;
SELECT ivymy.from_unixtime(NULL::float8, '%Y-%m-%d') AS null_timestamp_format;
-- Test FROM_UNIXTIME with various format strings
SELECT ivymy.from_unixtime(1000000000::float8, '%Y') AS year_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%m') AS month_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%d') AS day_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%H') AS hour_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%i') AS minute_only;
SELECT ivymy.from_unixtime(1000000000::float8, '%s') AS second_only;
-- Test FROM_UNIXTIME with decimal and format
SELECT ivymy.from_unixtime(1000000000.123456::numeric, '%Y-%m-%d %H:%i:%s.%f') AS formatted_with_microseconds;

/* Begin - BUG#8078 */
SELECT FROM_UNIXTIME(-1::numeric);
SELECT FROM_UNIXTIME(2147483648::numeric);
SELECT FROM_UNIXTIME(-1::bigint);
SELECT FROM_UNIXTIME(2147483648::bigint);
SELECT FROM_UNIXTIME(-1::float8);
SELECT FROM_UNIXTIME(2147483648::float8);
/* End - BUG#8078 */

set TIMEZONE=default;
-- date_add
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 SECOND);
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 MINUTE);
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 HOUR);
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 DAY);
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 MONTH);
SELECT date_add('1997-12-31 23:59:59', INTERVAL 1 YEAR);
SELECT date_add('1997-12-31 23:59:59', INTERVAL '1:1' HOUR_MINUTE);
SELECT date_add('1997-12-31 23:59:59', INTERVAL '1:1:1' HOUR_SECOND);
SELECT date_add('1997-12-31 23:59:59', INTERVAL '1 1:1' DAY_MINUTE);
SELECT date_add('1997-12-31 23:59:59', INTERVAL '1 1:1:1' DAY_SECOND);
SELECT date_add('1997-12-31', INTERVAL 1 SECOND);
SELECT date_add('1997-12-31', INTERVAL 1 DAY);
SELECT date_add(NULL, INTERVAL 100000 SECOND);
SELECT date_add('1998-01-30', INTERVAL 1 MONTH);
SELECT date_add('1996-02-29', INTERVAL 1 YEAR);
SELECT DATE_ADD(cast('1998-08-14 23:59:59' as date), INTERVAL 1 DAY);
SELECT DATE_ADD('1998-08-14'::date, INTERVAL 1 DAY);
-- date_sub
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 SECOND);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 MINUTE);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 HOUR);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 DAY);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 MONTH);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL 1 YEAR);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL '1:1' HOUR_MINUTE);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL '1:1:1' HOUR_SECOND);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL '1 1:1' DAY_MINUTE);
SELECT date_sub('1998-01-01 00:00:00', INTERVAL '1 1:1:1' DAY_SECOND);
SELECT date_sub('1998-01-02', INTERVAL 31 DAY);
SELECT date_sub('0000-00-00 00:00:00', INTERVAL 1 SECOND);
SELECT date_sub('0050-01-01 00:00:01', INTERVAL 2 SECOND);
SELECT date_sub('0199-01-01 00:00:01', INTERVAL 2 SECOND);
SELECT date_add('0199-12-31 23:59:59', INTERVAL 2 SECOND);
SELECT date_sub('0200-01-01 00:00:01', INTERVAL 2 SECOND);
SELECT date_sub('0200-01-01 00:00:01', INTERVAL 1 SECOND);

/* Begin - bug#8425 */
SELECT SUBDATE('9999-10-27', INTERVAL '9990-99' YEAR_MONTH);
/* End - bug#8425 */

/* Begin - bug#8426 */
SELECT DATE_SUB('2023-01-01 10:00:00', INTERVAL '1-1' DAY_HOUR);
SELECT DATE_SUB('2023-01-01 10:00:00', INTERVAL '1.1' DAY_HOUR);
/* End - bug#8426 */

/* Begin - bug#8084 */
SELECT ADDDATE('0000-12-30', 1);
/* End - bug#8084 */

-- adddate
SELECT adddate('1997-12-31 23:59:59.000001', 10);
SELECT adddate('1997-12-31 23:59:59.000001', interval 10 day);
SELECT adddate(19971231235959, interval 10 day);
SELECT adddate('1997-12-31 23:59:59.000001'::pg_catalog.timestamp, 10);
SELECT adddate(19971231235959, 10);

-- subdate
SELECT subdate('1997-12-31 23:59:59.000001', 10);
SELECT subdate('1997-12-31 23:59:59.000001', interval 10 day);
SELECT subdate(19971231235959, interval 10 day);
SELECT subdate('1997-12-31 23:59:59.000001'::pg_catalog.timestamp, 10);
SELECT subdate(19971231235959, 10);

/* Begin - BUG#8088 */
SELECT ADDDATE('1997-4-30', '1qwe');
SELECT ADDDATE('1997-4-30', 'qaz10');
SELECT ADDDATE('1997-4-30', '02qaz10');
SELECT SUBDATE('1997-4-30', '1qwe');
SELECT SUBDATE('1997-4-30', 'qaz10');
SELECT SUBDATE('1997-4-30', '02qaz10');
/* Begin - BUG#8088 */

/* Begin - bug#z8089 #z8084 */
SELECT ADDDATE('9999-12-31', 1);
SELECT ADDDATE('1994-07-03', 9999999);
SELECT ADDDATE('1994-07-03', -9999999);
/* End - bug#z8089 #z8084 */

/* Begin - bug#z8126 */
SELECT UNIX_TIMESTAMP('1970-01-01 00:00:00');
SELECT UNIX_TIMESTAMP('1969-12-31 23:59:59');
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07');
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:08');
SELECT UNIX_TIMESTAMP('1900-01-01 00:00:00');
SELECT UNIX_TIMESTAMP('2037-12-31 23:59:59');
/* End - bug#z8126 */

/* Begin - bug#z8234 */
SELECT DATE_ADD('2023-03-31', INTERVAL 9.9 DAY);
SELECT DATE_ADD('2023-03-31', INTERVAL 9.1 DAY);
SELECT DATE_ADD('2023-05-20 00:00:00', INTERVAL 2.44449 HOUR);
SELECT DATE_ADD('2023-05-20 00:00:00', INTERVAL 2.50001 HOUR);
/* End - bug#z8234 */

/* Begin - bug#z8081 */
set time_zone = '+08:00';
SELECT FROM_UNIXTIME(0);
set time_zone = '+00:00';
SELECT FROM_UNIXTIME(0);
/* End - bug#z8081 */

/* Begin - bug#z8237 */
SELECT DATE_ADD('988-01-01', INTERVAL '1-3' YEAR_MONTH);
SELECT DATE_ADD('23-01-01', INTERVAL '1-3' YEAR_MONTH);
SELECT DATE_ADD('3-01-01', INTERVAL '1-3' YEAR_MONTH);
/* End - bug#z8237 */

/* Begin - bug#z8241 */
SELECT DATE_ADD('1967-12-31 00:00:00.000001', INTERVAL 999999 MICROSECOND);
SELECT DATE_ADD('2023-10-27 00:00:00.999999', INTERVAL 1 MICROSECOND);
/* End - bug#z8241 */

/* Begin - bug#z8229 */
SELECT SUBDATE('2023-05-15', INTERVAL 2 WEEK);
SELECT SUBDATE('2023-05-15', INTERVAL 1 QUARTER);
SELECT SUBDATE('2029-04-01', INTERVAL '1-3' YEAR_MONTH);
SELECT SUBDATE('2030-01-02 02:00:00', INTERVAL '1 2' DAY_HOUR);
SELECT SUBDATE('2023-01-01 01:30:00', INTERVAL '1:30' HOUR_MINUTE);
/* End - bug#z8229 */

/* Begin - bug#z8241 */
SELECT DATE_ADD('2023-10-27 00:00:00.999999', INTERVAL 1 MICROSECOND);
SELECT DATE_ADD('1967-12-31 00:00:00.000001', INTERVAL 999999 MICROSECOND);
/* End - bug#z8241 */

/* Begin - bug#8424 */
SELECT FROM_UNIXTIME(1672576434, '%a'); -- sun
SELECT FROM_UNIXTIME(1672662834, '%a'); -- mon
SELECT FROM_UNIXTIME(1672749234, '%a'); -- tue
SELECT FROM_UNIXTIME(1672835634, '%a'); -- wed
SELECT FROM_UNIXTIME(1672922034, '%a'); -- thu
SELECT FROM_UNIXTIME(1673008434, '%a'); -- fri
SELECT FROM_UNIXTIME(1673094834, '%a'); -- sat
/* End - bug#8424 */

-- EXTRACT
create table extract_date(d mysdate);
insert into extract_date values ('2025-04-20');
select extract(microsecond from d) from extract_date;
select extract(second from d) from extract_date;
select extract(minute from d) from extract_date;
select extract(hour from d) from extract_date;
select extract(day from d) from extract_date;
select extract(week from d) from extract_date;
select extract(month from d) from extract_date;
select extract(quarter from d) from extract_date;
select extract(year from d) from extract_date;
select extract(second_microsecond from d) from extract_date;
select extract(minute_microsecond from d) from extract_date;
select extract(minute_second from d) from extract_date;
select extract(hour_microsecond from d) from extract_date;
select extract(hour_second from d) from extract_date;
select extract(hour_minute from d) from extract_date;
select extract(day_microsecond from d) from extract_date;
select extract(day_second from d) from extract_date;
select extract(day_minute from d) from extract_date;
select extract(day_hour from d) from extract_date;
select extract(year_month from d) from extract_date;
drop table extract_date;

create table extract_time(t mystime);
insert into extract_time values('15:02:40');
select extract(microsecond from t) from extract_time;
select extract(second from t) from extract_time;
select extract(minute from t) from extract_time;
select extract(hour from t) from extract_time;
select extract(second_microsecond from t) from extract_time;
select extract(minute_microsecond from t) from extract_time;
select extract(minute_second from t) from extract_time;
select extract(hour_microsecond from t) from extract_time;
select extract(hour_second from t) from extract_time;
select extract(hour_minute from t) from extract_time;
select extract(day_microsecond from t) from extract_time;
select extract(day_second from t) from extract_time;
select extract(day_minute from t) from extract_time;
select extract(day_hour from t) from extract_time;
drop table extract_time;

create table extract_times(ts mystimes(6));
insert into extract_times values('09:21:35.010203');
select extract(microsecond from ts) from extract_times;
select extract(second from ts) from extract_times;
select extract(minute from ts) from extract_times;
select extract(hour from ts) from extract_times;
select extract(second_microsecond from ts) from extract_times;
select extract(minute_microsecond from ts) from extract_times;
select extract(minute_second from ts) from extract_times;
select extract(hour_microsecond from ts) from extract_times;
select extract(hour_second from ts) from extract_times;
select extract(hour_minute from ts) from extract_times;
select extract(day_microsecond from ts) from extract_times;
select extract(day_second from ts) from extract_times;
select extract(day_minute from ts) from extract_times;
select extract(day_hour from ts) from extract_times;
drop table extract_times;

create table extract_datetime(dt mysdatetime);
insert into extract_datetime values('2020-12-10 15:30:45');
select extract(microsecond from dt) from extract_datetime;
select extract(second from dt) from extract_datetime;
select extract(minute from dt) from extract_datetime;
select extract(hour from dt) from extract_datetime;
select extract(day from dt) from extract_datetime;
select extract(week from dt) from extract_datetime;
select extract(month from dt) from extract_datetime;
select extract(quarter from dt) from extract_datetime;
select extract(year from dt) from extract_datetime;
select extract(second_microsecond from dt) from extract_datetime;
select extract(minute_microsecond from dt) from extract_datetime;
select extract(minute_second from dt) from extract_datetime;
select extract(hour_microsecond from dt) from extract_datetime;
select extract(hour_second from dt) from extract_datetime;
select extract(hour_minute from dt) from extract_datetime;
select extract(day_microsecond from dt) from extract_datetime;
select extract(day_second from dt) from extract_datetime;
select extract(day_minute from dt) from extract_datetime;
select extract(day_hour from dt) from extract_datetime;
select extract(year_month from dt) from extract_datetime;
drop table extract_datetime;

create table extract_datetimes(dts mysdatetimes(6));
insert into extract_datetimes values('2005-02-15 06:51:19.113355');
select extract(microsecond from dts) from extract_datetimes;
select extract(second from dts) from extract_datetimes;
select extract(minute from dts) from extract_datetimes;
select extract(hour from dts) from extract_datetimes;
select extract(day from dts) from extract_datetimes;
select extract(week from dts) from extract_datetimes;
select extract(month from dts) from extract_datetimes;
select extract(quarter from dts) from extract_datetimes;
select extract(year from dts) from extract_datetimes;
select extract(second_microsecond from dts) from extract_datetimes;
select extract(minute_microsecond from dts) from extract_datetimes;
select extract(minute_second from dts) from extract_datetimes;
select extract(hour_microsecond from dts) from extract_datetimes;
select extract(hour_second from dts) from extract_datetimes;
select extract(hour_minute from dts) from extract_datetimes;
select extract(day_microsecond from dts) from extract_datetimes;
select extract(day_second from dts) from extract_datetimes;
select extract(day_minute from dts) from extract_datetimes;
select extract(day_hour from dts) from extract_datetimes;
select extract(year_month from dts) from extract_datetimes;
drop table extract_datetimes;

create table extract_timestamp(tp mystimestamp);
insert into extract_timestamp values('2018-10-01 03:47:16');
select extract(microsecond from tp) from extract_timestamp;
select extract(second from tp) from extract_timestamp;
select extract(minute from tp) from extract_timestamp;
select extract(hour from tp) from extract_timestamp;
select extract(day from tp) from extract_timestamp;
select extract(week from tp) from extract_timestamp;
select extract(month from tp) from extract_timestamp;
select extract(quarter from tp) from extract_timestamp;
select extract(year from tp) from extract_timestamp;
select extract(second_microsecond from tp) from extract_timestamp;
select extract(minute_microsecond from tp) from extract_timestamp;
select extract(minute_second from tp) from extract_timestamp;
select extract(hour_microsecond from tp) from extract_timestamp;
select extract(hour_second from tp) from extract_timestamp;
select extract(hour_minute from tp) from extract_timestamp;
select extract(day_microsecond from tp) from extract_timestamp;
select extract(day_second from tp) from extract_timestamp;
select extract(day_minute from tp) from extract_timestamp;
select extract(day_hour from tp) from extract_timestamp;
select extract(year_month from tp) from extract_timestamp;
drop table extract_timestamp;

create table extract_timestamps(tps mystimestamps(6));
insert into extract_timestamps values('2010-08-31 12:37:05.123456');
select extract(microsecond from tps) from extract_timestamps;
select extract(second from tps) from extract_timestamps;
select extract(minute from tps) from extract_timestamps;
select extract(hour from tps) from extract_timestamps;
select extract(day from tps) from extract_timestamps;
select extract(week from tps) from extract_timestamps;
select extract(month from tps) from extract_timestamps;
select extract(quarter from tps) from extract_timestamps;
select extract(year from tps) from extract_timestamps;
select extract(second_microsecond from tps) from extract_timestamps;
select extract(minute_microsecond from tps) from extract_timestamps;
select extract(minute_second from tps) from extract_timestamps;
select extract(hour_microsecond from tps) from extract_timestamps;
select extract(hour_second from tps) from extract_timestamps;
select extract(hour_minute from tps) from extract_timestamps;
select extract(day_microsecond from tps) from extract_timestamps;
select extract(day_second from tps) from extract_timestamps;
select extract(day_minute from tps) from extract_timestamps;
select extract(day_hour from tps) from extract_timestamps;
select extract(year_month from tps) from extract_timestamps;
drop table extract_timestamps;

select extract(year from date'2003-06-30');
select extract(year_month from date'2003-06-30');
select extract(hour from time'19:42:01');
select extract(minute_second from time'19:42:01');
select extract(quarter from timestamp'2003-06-30 19:42:01');
select extract(day_microsecond from timestamp'2003-06-30 19:42:01');

select extract(DAY from '2017-01-21'::mysdate);
select extract(MINUTE from '14:09:42'::mystime);
select extract(MICROSECOND from '14:09:42.003003'::mystimes);
select extract(HOUR_MINUTE from '2017-01-21 14:09:42'::mysdatetime);
select extract(SECOND_MICROSECOND from '2017-01-21 14:09:42.003003'::mysdatetimes);
select extract(QUARTER from '2017-01-21 14:09:42'::mystimestamp);
select extract(DAY_HOUR from '2017-01-21 14:09:42.003003'::mystimestamps);

select extract(YEAR_MONTH from date'-2017-01-21');
select extract(MINUTE from time'-14:09:42');
select extract(MICROSECOND from time'-14:09:42.6060');
select extract(HOUR_MICROSECOND from time'-14:09:42.1');
select extract(SECOND_MICROSECOND from time'-14:09:42.05051');
select extract(QUARTER from timestamp'-2017-01-21 14:09:42');

create table extract_negtime(t mystime);
insert into extract_negtime values('-14:09:42');
select extract(microsecond from t) from extract_negtime;
select extract(hour from t) from extract_negtime;
select extract(second_microsecond from t) from extract_negtime;
select extract(minute_microsecond from t) from extract_negtime;
select extract(minute_second from t) from extract_negtime;
select extract(hour_microsecond from t) from extract_negtime;
select extract(hour_second from t) from extract_negtime;
select extract(hour_minute from t) from extract_negtime;
select extract(day_microsecond from t) from extract_negtime;
select extract(day_second from t) from extract_negtime;
select extract(day_minute from t) from extract_negtime;
select extract(day_hour from t) from extract_negtime;
drop table extract_negtime;

create table extract_negtimes(ts mystimes(6));
insert into extract_negtimes values('-14:09:42.000111');
select extract(microsecond from ts) from extract_negtimes;
select extract(hour from ts) from extract_negtimes;
select extract(second_microsecond from ts) from extract_negtimes;
select extract(minute_microsecond from ts) from extract_negtimes;
select extract(minute_second from ts) from extract_negtimes;
select extract(hour_microsecond from ts) from extract_negtimes;
select extract(hour_second from ts) from extract_negtimes;
select extract(hour_minute from ts) from extract_negtimes;
select extract(day_microsecond from ts) from extract_negtimes;
select extract(day_second from ts) from extract_negtimes;
select extract(day_minute from ts) from extract_negtimes;
select extract(day_hour from ts) from extract_negtimes;
drop table extract_negtimes;

select extract(year from 20200925), extract(month from 20200925), extract(day from 20200925);
select extract(year from 20200925121014), extract(month from 20200925121014), extract(day from 20200925121014);
select extract(hour from 20200925121014), extract(minute from 20200925121014), extract(second from 20200925121014);
select extract(quarter from 20200925121014), extract(year_month from 20200925121014), extract(day_microsecond from 20200925121014);
select extract(microsecond from 20200925121014.123456), extract(second from 20200925121014.1234);
select extract(year_month from 20200925121014.101010), extract(day_microsecond from 20200925121014.000012);

/* Begin OA#2701 */
SELECT
	pg_typeof(DATE_ADD(CAST('2026-08-13 10:20:30' AS pg_catalog.timestamp), INTERVAL 1 DAY)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(DATE_SUB(CAST('2026-08-13 10:20:30' AS pg_catalog.timestamp), INTERVAL 1 DAY)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(DATE_ADD(CAST('2026-08-13 10:20:30' AS DATETIME), INTERVAL 1 DAY)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(DATE_SUB(CAST('2026-08-13 10:20:30' AS DATETIME), INTERVAL 1 DAY)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(DATE_ADD(CAST('2026-08-13 10:20:30.123456' AS DATETIME(6)), INTERVAL 1 MICROSECOND)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(DATE_SUB(CAST('2026-08-13 10:20:30.123456' AS DATETIME(6)), INTERVAL 1 MICROSECOND)) =
		'ivymy.mysdatetimes'::regtype AS ok;

SELECT
	pg_typeof(ADDDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.timestamp), 1)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(SUBDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.timestamp), 1)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(ADDDATE(CAST('2026-08-13 10:20:30' AS DATETIME), 1)) =
		'ivymy.mysdatetime'::regtype
	AND pg_typeof(SUBDATE(CAST('2026-08-13 10:20:30' AS DATETIME), 1)) =
		'ivymy.mysdatetime'::regtype 
	AND pg_typeof(ADDDATE(CAST('2026-08-13 10:20:30.123456' AS DATETIME(6)), 1)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(SUBDATE(CAST('2026-08-13 10:20:30.123456' AS DATETIME(6)), 1)) =
		'ivymy.mysdatetimes'::regtype AS ok;

select DATE_SUB(CAST('1998-08-14' AS DATE), INTERVAL 1 MONTH);

SELECT pg_typeof(DATE_SUB(CAST('1998-08-14' AS DATE), INTERVAL 1 MONTH)) =
		'ivymy.mysdatetimes'::regtype AS ok;

SELECT
	pg_typeof(DATE_ADD(CAST('1998-08-14' AS DATE), INTERVAL 1 DAY)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(DATE_ADD(CAST('1998-08-14' AS DATE), INTERVAL 1 HOUR)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(DATE_SUB(CAST('1998-08-14' AS DATE), INTERVAL 1 SECOND)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(ADDDATE(CAST('1998-08-14' AS DATE), 1)) =
		'ivymy.mysdatetimes'::regtype
	AND pg_typeof(SUBDATE(CAST('1998-08-14' AS DATE), 1)) =
		'ivymy.mysdatetimes'::regtype AS ok;

SELECT
	pg_typeof(DATE_ADD(CAST('2026-08-13 10:20:30' AS pg_catalog.text), INTERVAL 1 DAY)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(DATE_SUB(CAST('2026-08-13 10:20:30' AS pg_catalog.text), INTERVAL 1 DAY)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(DATE_ADD(CAST(20260813102030 AS pg_catalog.numeric), INTERVAL 1 DAY)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(DATE_SUB(CAST(20260813102030 AS pg_catalog.numeric), INTERVAL 1 DAY)) =
		'pg_catalog.text'::regtype AS ok;

SELECT
	pg_typeof(ADDDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.text), 1)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(SUBDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.text), 1)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(ADDDATE(CAST(20260813102030 AS pg_catalog.numeric), 1)) =
		'pg_catalog.text'::regtype
	AND pg_typeof(SUBDATE(CAST(20260813102030 AS pg_catalog.numeric), 1)) =
		'pg_catalog.text'::regtype AS ok;

SELECT
	pg_typeof(ADDDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.text), '1')) =
		'pg_catalog.text'::regtype
	AND pg_typeof(SUBDATE(CAST('2026-08-13 10:20:30' AS pg_catalog.text), '1')) =
		'pg_catalog.text'::regtype AS ok;
/* End OA#2701 */