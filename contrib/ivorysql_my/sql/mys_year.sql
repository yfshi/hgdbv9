SELECT set_config('search_path', 'ivymy, '||current_setting('search_path'), false);
set ivorysql.compatible_mode to mysql;
set datestyle to 'ISO,YMD';

-- #
-- # Test year
-- #
drop table if exists t1;

create table t1 (y year,y4 year(4));
insert into t1 values (0,0),(1999,1999),(2000,2000),(2001,2001),(70,70),(69,69);
select * from t1;
select * from t1 order by y;
select * from t1 order by y4;
drop table t1;

create table t1 (y year);
insert into t1 values (now());
-- select if(y = now(), 1, 0) from t1;
drop table t1;

-- #
-- # Bug #27176: Assigning a string to an year column has unexpected results
-- #
create table t1(a year);
insert ignore into t1 values (2000.5), ('2000.5'), ('2001a'), ('2.001E3');
select * from t1;
drop table t1;

--echo #
--echo # Bug #49480: WHERE using YEAR columns returns unexpected results
--echo #
CREATE TABLE t2(yy YEAR(4), c2 CHAR(4));
CREATE TABLE t4(yyyy YEAR(4), c4 CHAR(4));

INSERT INTO t2 (c2) VALUES (NULL),(1970),(1999),(2000),(2001),(2069);
INSERT INTO t4 (c4) SELECT c2 FROM t2;
UPDATE t2 SET yy = c2;
UPDATE t4 SET yyyy = c4;

SELECT * FROM t2;
SELECT * FROM t4;

--echo # Comparison of YEAR with YEAR
SELECT * FROM t4 a, t4 b WHERE a.yyyy = b.yyyy;
SELECT * FROM t4 a, t4 b WHERE a.yyyy <=> b.yyyy;
SELECT * FROM t4 a, t4 b WHERE a.yyyy < b.yyyy;

--echo # Comparison with constants:

SELECT * FROM t2 WHERE yy = NULL;
SELECT * FROM t4 WHERE yyyy = NULL;
SELECT * FROM t2 WHERE yy <=> NULL;
SELECT * FROM t4 WHERE yyyy <=> NULL;
SELECT * FROM t2 WHERE yy < NULL;
SELECT * FROM t2 WHERE yy > NULL;

SELECT * FROM t2 WHERE yy = NOW();
SELECT * FROM t4 WHERE yyyy = NOW();

SELECT * FROM t2 WHERE yy = 99;
SELECT * FROM t2 WHERE 99 = yy;
SELECT * FROM t4 WHERE yyyy = 99;

SELECT * FROM t2 WHERE yy = 'test';
SELECT * FROM t4 WHERE yyyy = 'test';

SELECT * FROM t2 WHERE yy = '1999';
SELECT * FROM t4 WHERE yyyy = '1999';

SELECT * FROM t2 WHERE yy = 1999;
SELECT * FROM t4 WHERE yyyy = 1999;

SELECT * FROM t2 WHERE yy = 1999.1;
SELECT * FROM t4 WHERE yyyy = 1999.1;

SELECT * FROM t2 WHERE yy = 1998.9;
SELECT * FROM t4 WHERE yyyy = 1998.9;

--echo # Coverage tests for YEAR with zero/2000 constants:

SELECT * FROM t2 WHERE yy = 0;
SELECT * FROM t2 WHERE yy = '0';
SELECT * FROM t2 WHERE yy = '0000';
SELECT * FROM t2 WHERE yy = '2000';
SELECT * FROM t2 WHERE yy = 2000;

SELECT * FROM t4 WHERE yyyy = 0;
SELECT * FROM t4 WHERE yyyy = '0';
SELECT * FROM t4 WHERE yyyy = '0000';
SELECT * FROM t4 WHERE yyyy = '2000';
SELECT * FROM t4 WHERE yyyy = 2000;

--echo # Comparison with constants those are out of YEAR range
--echo # (coverage test for backward compatibility)

SELECT COUNT(yy) FROM t2;
SELECT COUNT(yyyy) FROM t4;

SELECT COUNT(*) FROM t2 WHERE yy = -1;
SELECT COUNT(*) FROM t4 WHERE yyyy > -1;
SELECT COUNT(*) FROM t2 WHERE yy > -1000000000000000000;
SELECT COUNT(*) FROM t4 WHERE yyyy > -1000000000000000000;

SELECT COUNT(*) FROM t2 WHERE yy < 2156;
SELECT COUNT(*) FROM t4 WHERE yyyy < 2156;
SELECT COUNT(*) FROM t2 WHERE yy < 1000000000000000000;
SELECT COUNT(*) FROM t4 WHERE yyyy < 1000000000000000000;

SELECT * FROM t2 WHERE yy < 123;
SELECT * FROM t2 WHERE yy > 123;
SELECT * FROM t4 WHERE yyyy < 123;
SELECT * FROM t4 WHERE yyyy > 123;

DROP TABLE t2, t4;

--echo #
--echo # Bug #49910: Behavioural change in SELECT/WHERE on YEAR(4) data type
--echo #

CREATE TABLE t1 (y YEAR NOT NULL, s VARCHAR(4));
-- INSERT IGNORE INTO t1 (s) VALUES ('bad');
INSERT INTO t1 (y, s) VALUES (0, 0), (2000, 2000), (2001, 2001);

--sorted_result
SELECT * FROM t1 ta, t1 tb WHERE ta.y = tb.y;
SELECT * FROM t1 WHERE t1.y = 0;
SELECT * FROM t1 WHERE t1.y = 2000;

--sorted_result
-- SELECT ta.y AS ta_y, ta.s, tb.y AS tb_y, tb.s FROM t1 ta, t1 tb HAVING ta_y = tb_y;

DROP TABLE t1;

--echo #
--echo # Bug #59211: Select Returns Different Value for min(year) Function
--echo #

CREATE TABLE t1(c1 YEAR);
INSERT INTO t1 VALUES (1901),(2155),(0000);
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(c1) AS min_value, MAX(c1) FROM t1;
DROP TABLE t1;

--echo #
--echo # WL#6219: Deprecate and remove YEAR(2) type
--echo #

CREATE TABLE t1 (c1 YEAR(4), c2 YEAR(4));
-- ALTER TABLE t1 MODIFY COLUMN c2 YEAR(4);
DROP TABLE t1;

--echo #
--echo # WL#6263: Remove YEAR(2) type
--echo #

--error ER_INVALID_YEAR_COLUMN_LENGTH
CREATE TABLE t1 (c1 YEAR(2));
--error ER_INVALID_YEAR_COLUMN_LENGTH
CREATE TABLE t1 (c1 YEAR(0));
--error ER_INVALID_YEAR_COLUMN_LENGTH
CREATE TABLE t1 (c1 YEAR(4294967295));
--error ER_INVALID_YEAR_COLUMN_LENGTH
CREATE TABLE t1 (c1 YEAR(4294967296));

CREATE TABLE t1 (c1 YEAR(4));
-- SHOW CREATE TABLE t1;
INSERT INTO t1 VALUES (55);
UPDATE t1 SET c1=2016;
-- CHECK TABLE t1 FOR UPGRADE; 
-- REPAIR TABLE t1;
DELETE FROM t1;
--error ER_INVALID_YEAR_COLUMN_LENGTH
-- ALTER TABLE t1 MODIFY COLUMN c1 YEAR(2);
DROP TABLE t1;

--echo #
--echo # Start of 5.6 tests
--echo #
-- SET timestamp=UNIX_TIMESTAMP('2011-12-31 15:44:00');
CREATE TABLE t1 (a YEAR);
INSERT INTO t1 VALUES (CURRENT_TIME);
-- INSERT INTO t1 VALUES (TIME'15:44:00');
-- INSERT INTO t1 VALUES (TIME'25:00:00');
INSERT INTO t1 VALUES ('15:44:00'::time);
INSERT INTO t1 VALUES ('25:00:00'::time);
-- SELECT * FROM t1;
DROP TABLE t1;
-- SET timestamp=DEFAULT;

--echo #
--echo # Bug#28172538 YEAR RANGE CHECK FAILS WITH MAX YEAR FLOAT CONSTANT
--echo #
CREATE TABLE t(y YEAR);
INSERT INTO t VALUES (2155), (2155.0);
SELECT * FROM t;
--echo Used to fail
INSERT INTO t VALUES (2155.0E00);
INSERT INTO t VALUES (2.1550E+03);
SELECT * FROM t;
DROP TABLE t;

--echo #
--echo # ReqID:SRS-MYSQL-YEARTYPE
--echo #
CREATE TABLE test_year(y YEAR NOT NULL);
INSERT INTO test_year(y) VALUES(2022),(10),(98);
INSERT INTO test_year(y) VALUES(0);
INSERT INTO test_year(y) VALUES('0');
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES(2166);
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES(1800);
-- INSERT INTO test_year(y) VALUES(curdate()); -- no support
-- INSERT INTO test_year(y) VALUES(curtime()); -- no support
-- INSERT INTO test_year(y) VALUES(current_timestamp()); -- no support
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES('20230718');
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES('-2023');
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES(-2023);
-- error Out of range value for column 'y' at row 1
INSERT INTO test_year(y) VALUES(-20);
SELECT * FROM test_year;

INSERT INTO test_year(y) VALUES(now());
INSERT INTO test_year(y) VALUES(current_timestamp);

CREATE INDEX tyear ON test_year(y);
SET ENABLE_SEQSCAN = OFF;
EXPLAIN (COSTS OFF)
SELECT * FROM test_year WHERE y = 2022;
SET ENABLE_SEQSCAN = ON;
DROP INDEX tyear;
-- DROP INDEX tyear ON test_year; -- no support

-- CREATE INDEX hash_id USING HASH ON test_year(y); -- no support
-- EXPLAIN (COSTS OFF) SELECT * FROM test_year WHERE y = 2022; -- no support
-- DROP INDEX hash_id; -- no support

DROP TABLE test_year;

-- Support + - * / arithmetic operator
CREATE TABLE t(y YEAR);
INSERT INTO t VALUES (0),(1999),(2000),(2001),(70),(69);
SELECT y+1 FROM t;
SELECT y-1 FROM t;
SELECT y*2 FROM t;
SELECT y/2 FROM t;

SELECT 1+y FROM t;
SELECT 2025-y FROM t;
SELECT 2*y FROM t;
SELECT 2025/y FROM t where y != 0;

SELECT set_config('search_path', trim(leading 'ivymy, ' from current_setting('search_path')), false);