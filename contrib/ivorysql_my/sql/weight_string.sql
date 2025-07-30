-- benchmark test
select hex(WEIGHT_STRING('aa' collate utf8_general_ci as char(3)));
select hex(WEIGHT_STRING('a' collate utf8_general_ci as char(1)));
select hex(WEIGHT_STRING('ab' collate utf8_general_ci as char(1)));
select hex(WEIGHT_STRING('aa' collate utf8_general_ci as binary(3)));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as binary(2)));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 ));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 ASC));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 DESC));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 DESC REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 DESC, 1 REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 1 DESC, 2 REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 2 DESC, 1 REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 2 DESC  REVERSE));
select hex(WEIGHT_STRING('abc' collate utf8_general_ci as char(2) LEVEL 2,3,5 REVERSE));
select hex(WEIGHT_STRING(123));
select hex(WEIGHT_STRING(123.25));

-- literal
SELECT hex(WEIGHT_STRING('123456789'::integer as char(10)));
SELECT hex(WEIGHT_STRING('123456789.0'::real as char(20)));
SELECT hex(WEIGHT_STRING('123456789.0'::double precision as char(20)));
SELECT hex(WEIGHT_STRING('1'::boolean as char(10)));
SELECT hex(WEIGHT_STRING('123456789'::integer as binary(10)));
SELECT hex(WEIGHT_STRING('123456789.0'::real as binary (20)));
SELECT hex(WEIGHT_STRING('123456789.0'::double precision as binary (20)));
SELECT hex(WEIGHT_STRING('1'::boolean as binary(10)));
SELECT hex(WEIGHT_STRING('123456789'::character(9) as binary (20)));
SELECT hex(WEIGHT_STRING('123456789'::text as binary (20)));
SELECT hex(WEIGHT_STRING('2023-06-01 00:23:59'::date as binary (20)));
SELECT hex(WEIGHT_STRING('2023-06-01 00:23:59'::datetime as binary (20)));
SELECT hex(WEIGHT_STRING('2023-06-01 00:23:59'::time as binary (20)));
SELECT hex(WEIGHT_STRING(date '2021-12-31' as binary (10)));

-- NULL
select hex(WEIGHT_STRING(NULL));
select hex(WEIGHT_STRING(NULL as binary(10)));
select hex(WEIGHT_STRING(NULL as char(10)));
select hex(WEIGHT_STRING('test' as char(NULL)));

-- invalid parameters
select hex(WEIGHT_STRING('a' as char(-1)));
select hex(WEIGHT_STRING('abc' as char(2) LEVEL 2-5 DESC));
SELECT hex(WEIGHT_STRING('abc' as char(cast ( '10' as integer )) LEVEL '1'::numeric REVERSE));
SELECT hex(WEIGHT_STRING('ab' AS CHAR(1E+308)));
SELECT hex(WEIGHT_STRING('ab' AS BINARY(1E+308)));
SELECT hex(WEIGHT_STRING('ab' AS CHAR(1E+309)));
SELECT hex(WEIGHT_STRING('ab' AS BINARY(1E+309)));
select hex(WEIGHT_STRING('a' as char(0)));
select hex(WEIGHT_STRING('a' as binary(0)));
select hex(WEIGHT_STRING('a' as char(-1)));
select hex(WEIGHT_STRING('a' as binary(-1)));
SELECT hex(WEIGHT_STRING('Hello' LEVEL -1));
