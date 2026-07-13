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