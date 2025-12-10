create table value_test(a int not null, b int default 3);
--show sql_mode;
insert into value_test values();--error
insert into value_test values();--warning
select * from value_test;

create table test(a int, b int default 3);
insert into test values();
select * from test;

insert into test values(1, a * 2);
insert into test(a,b) values(2, a * 2);
insert into test(a,b) values(2, a * 2 ),(3, a * 3);
insert test(a,b) value(b * 2, 2);
select * from test;
drop table test;

create table test1(a varchar(2) not null, b date not null);
insert into test1 values();--error
insert into test1 values();--2 warnings
select * from test1;
drop table test1;

create table test1(a varchar(2), b date);
insert into test1 values();
drop table test1;

create table t1(a int unique, b int, c int);
INSERT INTO t1 (a, b, c) VALUES (1, 2, 3);
INSERT INTO t1 (a, b, c) value(b * c + 1, b * 2, a * b * 2);
INSERT t1(b,c) value(2, b * 2);
select *from t1;

INSERT INTO t1 (a, b, c) VALUES (1, 2, 3)
ON DUPLICATE KEY UPDATE c = c + 1;
select *from t1;

INSERT INTO t1 (a, b, c) VALUES (1, 2, 3)
ON DUPLICATE KEY UPDATE c = values(a) + values(b);
select *from t1;

insert into t1 set a = 1, b = 2, c = 3
on duplicate key update c = values(c) + values(c);
select *from t1;

create table t2(a int, b int, c int);
insert into t2 values(1, 2, 3);
insert into t1 select * from t2 on duplicate key update c = 10;

create view view_t2 as select * from t2;
insert into t1 select * from view_t2 on duplicate key update c = 20;
select *from t1;
insert into t1 select a,b from view_t2 on duplicate key update c = 20;
select *from t1;
insert into t1 select a,b from t2;
select *from t1;
drop table t2 cascade;
--insert ...select
create table test(id int, idd int);
insert into test values(1, 2);
insert into test select id from test;
insert into test select id from (select id from test) as a;
select * from test;
drop table test;
drop table t1;

--temporary table
create temporary table t1(col1 int, col2 int);
create table test1(a int,b int);
insert into t1 values(1,2);
insert into test1 select * from t1;
insert into t1 select * from t1;--error

--insert set
delete from test1;
insert into test1 set a = 1,b = 2;
select * from test1;
drop table test1, t1;

--insert delayed
create table test(id int);
insert delayed  into test values(1);--warning

create table t2(id int);
insert into t2 values(1);
insert into test() select * from t2;
insert into test() select * from t2;
insert into test() values(),();
select * from t2;
drop table test,t2;

create table tfunc(a int, b int);
create or replace function att(b int) returns int AS
$$
declare a int:=100;
begin
	insert into tfunc values(10, a * 3);
	return a;
end;
$$ language plpgsql;

create or replace function att(b int) returns int AS
$$
declare c int:=100;
begin
	insert into tfunc values(10, a * 3);
	return c;
end;
$$ language plpgsql;
drop table tfunc;

create table t1(a int, b int);
create table t2(c int, d int);
create table tt1(e int primary key, f int);
insert into t1 values(2,3);
insert into t2 values(3,4);
insert into tt1 values(2,3);
insert into tt1 value(f * 2, 4);
begin;
insert into tt1 select a,b from t1 union select c,d from t2 order by 1 
on duplicate key update e = e + 1;
select * from tt1;
rollback;

create table tt2(id int);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into tt2 values(22);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER update_ar_trigger1
AFTER insert ON tt1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into tt2 values(44);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER update_ar_trigger2
AFTER update ON tt1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
begin;
insert into tt1 select a,b from t1 union select c,d from t2 order by 1 
on duplicate key update e = e+1;
select * from tt1;
select * from tt2;
rollback;
drop table tt2;

create table tt(e int primary key,f int);
insert tt  value(2, 3),(5, 6);
PREPARE prtt (int, int) AS
	INSERT INTO tt VALUES($1, $2) on duplicate key update e = $1 + $2;
EXECUTE prtt(5,6);
select * from tt;

PREPARE prtt1 (int) AS
	INSERT INTO tt VALUES($1, $1 * 4) on duplicate key update e = $1 * 2;
EXECUTE prtt1(5);
select * from tt;

PREPARE prtt2 (int) AS
	INSERT INTO tt VALUES($1, $1 * 4);
EXECUTE prtt2(6);
select * from tt;

PREPARE rtt AS
	INSERT INTO tt select * from t1 union select * from t2 order by 1 on duplicate key update e = e + 1;
EXECUTE rtt;
select * from tt;
drop table t1,t2,tt;

-- test description
\h INSERT

-- test permission
--- test with no sequence column
CREATE TABLE t00 (col1 INT DEFAULT 1 PRIMARY KEY, col2 INT);
INSERT INTO t00 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 5;

--- test with sequnce column
CREATE TABLE t01 (col1 INT , col2 BIGSERIAL PRIMARY KEY, col3 INT);
INSERT INTO t01 VALUES(1) ON DUPLICATE KEY UPDATE col3 = 5;

-- test ommit INSERT target column
CREATE TABLE t02 (col1 INT DEFAULT 1 PRIMARY KEY, col2 INT, col3 INT);
INSERT INTO t02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col2 = 20;
SELECT * FROM t02 ORDER BY 1, 2;
INSERT INTO t02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col2 = 20;
SELECT * FROM t02 ORDER BY 1, 2;

ALTER TABLE t02 DROP COLUMN col2;
ALTER TABLE t02 ADD COLUMN col4 INT;
INSERT INTO t02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM t02 ORDER BY 1, 2;
INSERT INTO t02 VALUES(2, 3, 4) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM t02 ORDER BY 1, 2;
INSERT INTO t02 VALUES(2, 3, 4) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM t02 ORDER BY 1, 2;

-- test restriction
--- test replication table
CREATE TABLE t03 (col1 int PRIMARY KEY, col2 INT, col3 smallserial);
--- error: not allowed volatile function as default value
INSERT INTO t03(col2) VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;

ALTER TABLE t03 DROP COLUMN col3;
--- error: primary key are not allowed to update
INSERT INTO t03 VALUES(1) ON DUPLICATE KEY UPDATE col1 = 1;
--- error: clause other than VALUSES are not allowed to use
INSERT INTO t03 SELECT * FROM t03 ON DUPLICATE KEY UPDATE col2 = 1;
--- error: expression index are not supported
CREATE UNIQUE INDEX u_expr_index ON t03 USING btree (abs(col1));
INSERT INTO t03 VALUES(-10, 10) ON DUPLICATE KEY UPDATE col2 = 20;
DROP INDEX u_expr_index;

-- test with stream operator on
INSERT INTO t03 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;
SELECT * FROM t03;

INSERT INTO t03 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;
SELECT * FROM t03;
SELECT * FROM t03;

--- test PBE
PREPARE p1 AS INSERT INTO t03 VALUES($1, $2) ON DUPLICATE KEY UPDATE col2 = $1 * 100;
EXECUTE p1(5, 50);
SELECT * FROM t03 WHERE col1 = 5;
EXECUTE p1(5, 50);
SELECT * FROM t03 WHERE col1 = 5;
DELETE from t03 WHERE col1 = 5;

---- test with primary key
INSERT INTO t03 VALUES(2) ON DUPLICATE KEY UPDATE col2 = 200;
SELECT * FROM t03;

INSERT INTO t03 VALUES(2) ON DUPLICATE KEY UPDATE col2 = 200;
SELECT * FROM t03;
SELECT * FROM t03;

---- test with unique key without NOT NULL constraint
ALTER TABLE t03 DROP CONSTRAINT t03_pkey;
ALTER TABLE t03 ADD COLUMN col3 INT;
CREATE UNIQUE INDEX ON t03 (col1, col3);
---- unique constraints might contain NULL, depends on the plan
----- for cn light and fqs, it can be done
INSERT INTO t03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
----- for stream or pgxc it can not be done
INSERT INTO t03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;

---- test with unique key with NOT NULL constraint, should success
CREATE UNIQUE INDEX ON t03 (col1);
INSERT INTO t03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
SELECT * FROM t03;

INSERT INTO t03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
SELECT * FROM t03;
SELECT * FROM t03;

---- test PBE
PREPARE p2 AS INSERT INTO t03 VALUES($1, $2) ON DUPLICATE KEY UPDATE col2 = $1 * 100;
EXECUTE p2(5, 50);
SELECT * FROM t03 WHERE col1 = 5;
EXECUTE p2(5, 50);
SELECT * FROM t03 WHERE col1 = 5;

--- error: test with clause
WITH tmp(col1, col2) AS (SELECT * FROM t01)
INSERT INTO t01 SELECT * FROM tmp ON DUPLICATE KEY UPDATE col1 = 1;

WITH RECURSIVE rq AS
    (
    SELECT col1, col2 FROM t00 WHERE col1 = 1
    UNION ALL
    SELECT origin.col1, rq.col2
    FROM rq JOIN t00 AS origin ON origin.col1 = rq.col1
    )
    INSERT INTO t03 SELECT * FROM rq ON DUPLICATE KEY UPDATE col1 = rq.col1;

--- error: test returning clause
INSERT INTO t01 VALUES (1) ON DUPLICATE KEY UPDATE col1 = 1 RETURNING NOT(1::bool);

--- error: distribute key are not allowed to UPDATE
CREATE TABLE t04 (col1 INT, col2 INT)  ;
INSERT INTO t04 VALUES (1) ON DUPLICATE KEY UPDATE col1 = 5;

--- error: unique index referenced column are not allowed to UPDATE
CREATE UNIQUE INDEX t04_u_index ON t04(col1, col2);
INSERT INTO t04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
DROP INDEX t04_u_index;

--- error: primary key referenced column are not allowed to UPDATE
ALTER TABLE t04 ADD PRIMARY KEY (col1, col2);
INSERT INTO t04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;

--- error: invalid column
INSERT INTO t04 (col2, col3) VALUES (2, 3) ON DUPLICATE KEY UPDATE col2 = 5;

--- error: duplicate column
INSERT INTO t04 (col2, col2) VALUES (2, 3) ON DUPLICATE KEY UPDATE col2 = 5;

-- error: target column more than insert target
INSERT INTO t04 (col1, col2) VALUES (2, 3, 4) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 (col1, col2) VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 (col1, col2) SELECT col1 FROM t04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 (col1, col2) SELECT *, col1 FROM t04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 VALUES (2, 3, 4) ON DUPLICATE KEY UPDATE col2 = 5;
--INSERT INTO t04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 SELECT col1 FROM t04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO t04 SELECT *, col1 FROM t04 ON DUPLICATE KEY UPDATE col2 = 5;

-- test DEFAULT VALUES
TRUNCATE t00;
TRUNCATE t01;

--- without sequence
----should insert
INSERT INTO t00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col2 = col1;
SELECT * FROM t00 ORDER BY 1, 2;
---- should update
INSERT INTO t00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col2 = col1;
SELECT * FROM t00 ORDER BY 1, 2;

--- test drop column
TRUNCATE t00;
ALTER TABLE t00 DROP COLUMN col2;
ALTER TABLE t00 ADD COLUMN col3 INT DEFAULT 100;
----should insert
INSERT INTO t00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col3 = col1;
SELECT * FROM t00 ORDER BY 1, 2;
---- should update
INSERT INTO t00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col3 = col1;
SELECT * FROM t00 ORDER BY 1, 2;

--- with sequence
----should insert
INSERT INTO t01 DEFAULT VALUES ON DUPLICATE KEY UPDATE col1 = col2;
INSERT INTO t01 (col2) SELECT col2 + 1 FROM t01 LIMIT 1;
SELECT * FROM t01 ORDER BY 1, 2, 3;

---- should update
INSERT INTO t01 DEFAULT VALUES ON DUPLICATE KEY UPDATE col1 = col2;
SELECT * FROM t01 ORDER BY 1, 2, 3;


-- test VALUES(DEFAULT)
CREATE TABLE t05 (col1 INT , col2 INT DEFAULT 1 PRIMARY KEY, col3 INT DEFAULT 100);
--- should insert
INSERT INTO t05 VALUES(DEFAULT) ON DUPLICATE KEY UPDATE col3 = 1000;
SELECT * FROM t05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO t05 VALUES(DEFAULT) ON DUPLICATE KEY UPDATE col3 = 1000;
SELECT * FROM t05 ORDER BY 1, 2, 3;

-- test UPDATE DEFAULT
INSERT INTO t05 (col1, col2, col3) VALUES (2, 2, 2) ON DUPLICATE KEY UPDATE col3 = DEFAULT;
SELECT * FROM t05 ORDER BY 1, 2, 3;
INSERT INTO t05 (col1, col2, col3) VALUES (2, 2, 2) ON DUPLICATE KEY UPDATE col3 = DEFAULT;
SELECT * FROM t05 ORDER BY 1, 2, 3;

-- test VALUES (DEFAULT, ...)
TRUNCATE t05;
--- should insert
INSERT INTO t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM t05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM t05 ORDER BY 1, 2, 3;

--- test drop coulmn
BEGIN;
ALTER TABLE t05 ADD COLUMN col4 INT;
ALTER TABLE t05 ADD COLUMN col5 INT DEFAULT 500;
ALTER TABLE t05 DROP COLUMN col3;
TRUNCATE t05;
INSERT INTO t05 VALUES(DEFAULT, DEFAULT, DEFAULT, 600) ON DUPLICATE KEY UPDATE col5 = DEFAULT;
SELECT * FROM t05 ORDER BY 1, 2, 3;
INSERT INTO t05 VALUES(DEFAULT, DEFAULT, DEFAULT, 600) ON DUPLICATE KEY UPDATE col5 = DEFAULT;
SELECT * FROM t05 ORDER BY 1, 2, 3;
ROLLBACK;


drop table t05;
create schema test_insert;
CREATE TABLE test_insert.t05 (col1 INT , col2 INT DEFAULT 1 PRIMARY KEY, col3 INT DEFAULT 100);
--- should insert
INSERT INTO test_insert.t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM test_insert.t05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO test_insert.t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM test_insert.t05 ORDER BY 1, 2, 3;

--- test using table name on update
INSERT INTO test_insert.t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE t05.col3 = DEFAULT, t05.col1 = t05.col3 + 1;
SELECT * FROM test_insert.t05 ORDER BY 1, 2, 3;

INSERT INTO test_insert.t05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE test_insert.t05.col3 = DEFAULT, test_insert.t05.col1 = test_insert.t05.col3 + 2;
SELECT * FROM test_insert.t05 ORDER BY 1, 2, 3;

drop table t01,t00,t02,t03,t04;
drop schema test_insert cascade;

--BUG#Z1434,1435
create table test(a int, b int default 3);
insert test(a,b) value(b * 2, 2);
insert test(a,b) value(2, b * 2);
create table test1(a int, b int);
insert test1(a,b) value(b * 2, 2);
insert test1(a,b) value(2, b * 2);
create table test2(a int , b varchar(50));
set mysql.enable_ignore_columns_list to on;
insert into test2 values(1);
insert into test2 values(2),(3);
set mysql.enable_ignore_columns_list to default;
insert into test2 values(1);
insert into test2 values(2),(3);
select * from test2;
drop table test,test1,test2;

/* Begin - bug#5797 */
create table test(a int default 1);
insert into test values (a);
select * from test;
drop table test;

create table test(a int default 1, b int);
insert into test values (2, a); -- 2, 2
insert into test values (b, 2); -- null, 2
insert into test values (a, a); -- 1, 1
insert into test values (b, b); -- null, null
insert into test values (a, b);	-- 1, null
insert into test values (b, a); -- null, null
insert into test(b) values (a); -- 1, 1
insert into test(a) values (b); -- null, null
select * from test;
drop table test;

create table test(a int default 1, b int default 2);
insert into test(a, b) values (1, (select b where b != 2)); -- error
insert into test(a, b) values (1, (select b from test limit 1));	-- error
insert into test(a, b) values (1, (select 1 from (select 1 from test) tt limit 1));  -- ok
insert into test(b) values (a = (select a));  -- error
select * from test;
drop table test;

/* End - bug#5797 */