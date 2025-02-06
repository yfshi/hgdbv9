create table VALUE_TEST1(a int not null, b int default 3);
--show dolphin.sql_mode;
--插入NULL值
insert into VALUE_TEST1 values();--error
insert into VALUE_TEST1 values();--warning
select * from VALUE_TEST1;
drop table VALUE_TEST1;

create table TEST(a int, b int default 3);
insert into TEST values();
select * from TEST;

--引用值列表中先前设置的任何列
insert into TEST values(1, a * 2);
insert into TEST(a,b) values(2, a * 2);
insert into TEST(a,b) values(2, a * 2 ),(3, a * 3);
insert TEST(a,b) value(b * 2, 2);
select * from TEST;
drop table TEST;

create table TEST1(a varchar(2) not null, b date not null);
insert into TEST1 values();--error
insert into TEST1 values();--2 warnings
select * from TEST1;
drop table TEST1;

create table TEST1(a varchar(2), b date);
insert into TEST1 values();
drop table TEST1;

--insert ON DUPLICATE KEY UPDATE用法
create table T1(a int unique, b int, c int);
INSERT INTO T1 (a, b, c) VALUES (1, 2, 3);
INSERT INTO T1 (a, b, c) value(b * c + 1, b * 2, a * b * 2);
INSERT T1(b,c) value(2, b * 2);
select *from T1;

INSERT INTO T1 (a, b, c) VALUES (1, 2, 3)
ON DUPLICATE KEY UPDATE c = c + 1;
select *from T1;

INSERT INTO T1 (a, b, c) VALUES (1, 2, 3)
ON DUPLICATE KEY UPDATE c = values(a) + values(b);
select *from T1;

insert into T1 set a = 1, b = 2, c = 3
on duplicate key update c = values(c) + values(c);
select *from T1;

create table T2(a int, b int, c int);
insert into T2 values(1, 2, 3);
insert into T1 select * from T2 on duplicate key update c = 10;

create view VIEW_T2 as select * from T2;
insert into T1 select * from VIEW_T2 on duplicate key update c = 20;
select *from T1;
insert into T1 select a,b from VIEW_T2 on duplicate key update c = 20;
select *from T1;
insert into T1 select a,b from T2;
select *from T1;
drop table T2 cascade;
--插入时，查询列需要匹配插入的列个数。
--insert ...select 用法
create table TEST(id int, idd int);
insert into TEST values(1, 2);
insert into TEST select id from TEST;
insert into TEST select id from (select id from TEST) as a;
select * from TEST;
drop table TEST;
drop table T1;

--temporary table用法
create temporary table T1(col1 int, col2 int);
create table TEST1(a int,b int);
insert into T1 values(1,2);
insert into TEST1 select * from T1;
insert into T1 select * from T1;--error 临时表中不允许

--当t1是TEMPORARY 表时，不能使用 INSERT INTO t ... SELECT ... FROM t，因为 TEMPORARY 表不能在同一语句中被引用两次。
--insert set用法
delete from TEST1;
insert into TEST1 set a = 1,b = 2;
select * from TEST1;
drop table TEST1, T1;

--insert delayed用法
create table TEST(id int);
insert delayed  into TEST values(1);--warning

create table T2(id int);
insert into T2 values(1);
insert into TEST() select * from T2;
insert into TEST() select * from T2;
insert into TEST() values(),();
select * from T2;
drop table TEST,T2;

create table TFUNC(a int, b int);
create or replace function att(b int) returns int AS
$$
declare a int:=100;
begin
	insert into TFUNC values(10, a * 3);
	return a;
end;
$$ language plpgsql;

create or replace function att(b int) returns int AS
$$
declare c int:=100;
begin
	insert into TFUNC values(10, a * 3);
	return c;
end;
$$ language plpgsql;
drop table TFUNC;

create table T1(a int, b int);
create table T2(c int, d int);
create table  CTT1(e int primary key, f int);
insert into T1 values(2,3);
insert into T2 values(3,4);
insert into  CTT1 values(2,3);
insert into  CTT1 value(f * 2, 4);
begin;
insert into  CTT1 select a,b from T1 union select c,d from T2 order by 1 
on duplicate key update e = e + 1;
select * from  CTT1;
rollback;

--触发器
create table TT2(id int);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into TT2 values(22);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER update_ar_trigger1
AFTER insert ON  CTT1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into TT2 values(44);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER update_ar_trigger2
AFTER update ON  CTT1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
begin;
insert into  CTT1 select a,b from T1 union select c,d from T2 order by 1 
on duplicate key update e = e+1;
select * from  CTT1;
select * from TT2;
rollback;
drop table  CTT1;
drop table TT2;

--pbe的支持
create table TT(e int primary key,f int);
insert TT  value(2, 3),(5, 6);
PREPARE prtt (int, int) AS
	INSERT INTO TT VALUES($1, $2) on duplicate key update e = $1 + $2;
EXECUTE prtt(5,6);
select * from TT;

PREPARE prtt1 (int) AS
	INSERT INTO TT VALUES($1, $1 * 4) on duplicate key update e = $1 * 2;
EXECUTE prtt1(5);
select * from TT;

PREPARE prtt2 (int) AS
	INSERT INTO TT VALUES($1, $1 * 4);
EXECUTE prtt2(6);
select * from TT;

PREPARE rtt AS
	INSERT INTO TT select * from T1 union select * from T2 order by 1 on duplicate key update e = e + 1;
EXECUTE rtt;
select * from TT;
drop table T1,T2,TT;

-- TEST description
\h INSERT

-- TEST permission
--- TEST with no sequence column
CREATE TABLE T00 (col1 INT DEFAULT 1 PRIMARY KEY, col2 INT);
INSERT INTO T00 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 5;

--- TEST with sequnce column
CREATE TABLE T01 (col1 INT , col2 BIGSERIAL PRIMARY KEY, col3 INT);
INSERT INTO T01 VALUES(1) ON DUPLICATE KEY UPDATE col3 = 5;

-- TEST ommit INSERT target column
CREATE TABLE T02 (col1 INT DEFAULT 1 PRIMARY KEY, col2 INT, col3 INT);
INSERT INTO T02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col2 = 20;
SELECT * FROM T02 ORDER BY 1, 2;
INSERT INTO T02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col2 = 20;
SELECT * FROM T02 ORDER BY 1, 2;

ALTER TABLE T02 DROP COLUMN col2;
ALTER TABLE T02 ADD COLUMN col4 INT;
INSERT INTO T02 VALUES(1, 2, 3) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM T02 ORDER BY 1, 2;
INSERT INTO T02 VALUES(2, 3, 4) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM T02 ORDER BY 1, 2;
INSERT INTO T02 VALUES(2, 3, 4) ON DUPLICATE KEY UPDATE col4 = 40;
SELECT * FROM T02 ORDER BY 1, 2;

-- TEST restriction
--- TEST replication table
CREATE TABLE T03 (col1 int PRIMARY KEY, col2 INT, col3 smallserial);
--- error: not allowed volatile function as default value
INSERT INTO T03(col2) VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;

ALTER TABLE T03 DROP COLUMN col3;
--- error: primary key are not allowed to update
INSERT INTO T03 VALUES(1) ON DUPLICATE KEY UPDATE col1 = 1;
--- error: clause other than VALUSES are not allowed to use
INSERT INTO T03 SELECT * FROM T03 ON DUPLICATE KEY UPDATE col2 = 1;
--- error: expression index are not supported
CREATE UNIQUE INDEX u_expr_index ON T03 USING btree (abs(col1));
INSERT INTO T03 VALUES(-10, 10) ON DUPLICATE KEY UPDATE col2 = 20;
DROP INDEX u_expr_index;

-- TEST with stream operator on
INSERT INTO T03 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;
SELECT * FROM T03;

INSERT INTO T03 VALUES(1) ON DUPLICATE KEY UPDATE col2 = 100;
SELECT * FROM T03;
SELECT * FROM T03;

--- TEST PBE
PREPARE p1 AS INSERT INTO T03 VALUES($1, $2) ON DUPLICATE KEY UPDATE col2 = $1 * 100;
EXECUTE p1(5, 50);
SELECT * FROM T03 WHERE col1 = 5;
EXECUTE p1(5, 50);
SELECT * FROM T03 WHERE col1 = 5;
DELETE from T03 WHERE col1 = 5;

---- TEST with primary key
INSERT INTO T03 VALUES(2) ON DUPLICATE KEY UPDATE col2 = 200;
SELECT * FROM T03;

INSERT INTO T03 VALUES(2) ON DUPLICATE KEY UPDATE col2 = 200;
SELECT * FROM T03;
SELECT * FROM T03;

---- TEST with unique key without NOT NULL constraint
-- ALTER TABLE T03 DROP CONSTRAINT T03_pkey;
CREATE FUNCTION DROPCONSTRAINT() RETURNS text AS $$
DECLARE
	lowernum int;
BEGIN
	select setting::int into lowernum from pg_settings where name = 'lower_case_table_names' ;
	if lowernum = 0 then
		ALTER TABLE T03 DROP CONSTRAINT T03_pkey;
	else
		ALTER TABLE T03 DROP CONSTRAINT t03_pkey;
	end if;
	
	return 'ok';
  
END
$$ LANGUAGE plpgsql;
select DROPCONSTRAINT();

ALTER TABLE T03 ADD COLUMN col3 INT;
CREATE UNIQUE INDEX ON T03 (col1, col3);
---- unique constraints might contain NULL, depends on the plan
----- for cn light and fqs, it can be done
INSERT INTO T03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
----- for stream or pgxc it can not be done
INSERT INTO T03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;

---- TEST with unique key with NOT NULL constraint, should success
CREATE UNIQUE INDEX ON T03 (col1);
INSERT INTO T03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
SELECT * FROM T03;

INSERT INTO T03 VALUES(3) ON DUPLICATE KEY UPDATE col2 = 300;
SELECT * FROM T03;
SELECT * FROM T03;

---- TEST PBE
PREPARE p2 AS INSERT INTO T03 VALUES($1, $2) ON DUPLICATE KEY UPDATE col2 = $1 * 100;
EXECUTE p2(5, 50);
SELECT * FROM T03 WHERE col1 = 5;
EXECUTE p2(5, 50);
SELECT * FROM T03 WHERE col1 = 5;

--- error: TEST with clause
WITH tmp(col1, col2) AS (SELECT * FROM T01)
INSERT INTO T01 SELECT * FROM tmp ON DUPLICATE KEY UPDATE col1 = 1;

WITH RECURSIVE RQ AS
    (
    SELECT col1, col2 FROM T00 WHERE col1 = 1
    UNION ALL
    SELECT origin.col1, RQ.col2
    FROM RQ JOIN T00 AS origin ON origin.col1 = RQ.col1
    )
    INSERT INTO T03 SELECT * FROM RQ ON DUPLICATE KEY UPDATE col1 = RQ.col1;

--- error: TEST returning clause
INSERT INTO T01 VALUES (1) ON DUPLICATE KEY UPDATE col1 = 1 RETURNING NOT(1::bool);

--- error: distribute key are not allowed to UPDATE
CREATE TABLE T04 (col1 INT, col2 INT)  ;
INSERT INTO T04 VALUES (1) ON DUPLICATE KEY UPDATE col1 = 5;

--- error: unique index referenced column are not allowed to UPDATE
CREATE UNIQUE INDEX t04_u_index ON T04(col1, col2);
INSERT INTO T04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
DROP INDEX t04_u_index;

--- error: primary key referenced column are not allowed to UPDATE
ALTER TABLE T04 ADD PRIMARY KEY (col1, col2);
INSERT INTO T04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;

--- error: invalid column
INSERT INTO T04 (col2, col3) VALUES (2, 3) ON DUPLICATE KEY UPDATE col2 = 5;

--- error: duplicate column
INSERT INTO T04 (col2, col2) VALUES (2, 3) ON DUPLICATE KEY UPDATE col2 = 5;

-- error: target column more than insert target
INSERT INTO T04 (col1, col2) VALUES (2, 3, 4) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 (col1, col2) VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 (col1, col2) SELECT col1 FROM T04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 (col1, col2) SELECT *, col1 FROM T04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 VALUES (2, 3, 4) ON DUPLICATE KEY UPDATE col2 = 5;
--INSERT INTO T04 VALUES (1) ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 SELECT col1 FROM T04 ON DUPLICATE KEY UPDATE col2 = 5;
INSERT INTO T04 SELECT *, col1 FROM T04 ON DUPLICATE KEY UPDATE col2 = 5;

-- TEST DEFAULT VALUES
TRUNCATE T00;
TRUNCATE T01;

--- without sequence
----should insert
INSERT INTO T00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col2 = col1;
SELECT * FROM T00 ORDER BY 1, 2;
---- should update
INSERT INTO T00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col2 = col1;
SELECT * FROM T00 ORDER BY 1, 2;

--- TEST drop column
TRUNCATE T00;
ALTER TABLE T00 DROP COLUMN col2;
ALTER TABLE T00 ADD COLUMN col3 INT DEFAULT 100;
----should insert
INSERT INTO T00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col3 = col1;
SELECT * FROM T00 ORDER BY 1, 2;
---- should update
INSERT INTO T00 DEFAULT VALUES ON DUPLICATE KEY UPDATE col3 = col1;
SELECT * FROM T00 ORDER BY 1, 2;

--- with sequence
----should insert
INSERT INTO T01 DEFAULT VALUES ON DUPLICATE KEY UPDATE col1 = col2;
INSERT INTO T01 (col2) SELECT col2 + 1 FROM T01 LIMIT 1;
SELECT * FROM T01 ORDER BY 1, 2, 3;

---- should update
INSERT INTO T01 DEFAULT VALUES ON DUPLICATE KEY UPDATE col1 = col2;
SELECT * FROM T01 ORDER BY 1, 2, 3;


-- TEST VALUES(DEFAULT)
CREATE TABLE T05 (col1 INT , col2 INT DEFAULT 1 PRIMARY KEY, col3 INT DEFAULT 100);
--- should insert
INSERT INTO T05 VALUES(DEFAULT) ON DUPLICATE KEY UPDATE col3 = 1000;
SELECT * FROM T05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO T05 VALUES(DEFAULT) ON DUPLICATE KEY UPDATE col3 = 1000;
SELECT * FROM T05 ORDER BY 1, 2, 3;

-- TEST UPDATE DEFAULT
INSERT INTO T05 (col1, col2, col3) VALUES (2, 2, 2) ON DUPLICATE KEY UPDATE col3 = DEFAULT;
SELECT * FROM T05 ORDER BY 1, 2, 3;
INSERT INTO T05 (col1, col2, col3) VALUES (2, 2, 2) ON DUPLICATE KEY UPDATE col3 = DEFAULT;
SELECT * FROM T05 ORDER BY 1, 2, 3;

-- TEST VALUES (DEFAULT, ...)
TRUNCATE T05;
--- should insert
INSERT INTO T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM T05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM T05 ORDER BY 1, 2, 3;

--- TEST drop coulmn
BEGIN;
ALTER TABLE T05 ADD COLUMN col4 INT;
ALTER TABLE T05 ADD COLUMN col5 INT DEFAULT 500;
ALTER TABLE T05 DROP COLUMN col3;
TRUNCATE T05;
INSERT INTO T05 VALUES(DEFAULT, DEFAULT, DEFAULT, 600) ON DUPLICATE KEY UPDATE col5 = DEFAULT;
SELECT * FROM T05 ORDER BY 1, 2, 3;
INSERT INTO T05 VALUES(DEFAULT, DEFAULT, DEFAULT, 600) ON DUPLICATE KEY UPDATE col5 = DEFAULT;
SELECT * FROM T05 ORDER BY 1, 2, 3;
ROLLBACK;


drop table T05;
create schema TEST_INSERT;
CREATE TABLE TEST_INSERT.T05 (col1 INT , col2 INT DEFAULT 1 PRIMARY KEY, col3 INT DEFAULT 100);
--- should insert
INSERT INTO TEST_INSERT.T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM TEST_INSERT.T05 ORDER BY 1, 2, 3;

--- should update
INSERT INTO TEST_INSERT.T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE col3 = DEFAULT, col1 = col3;
SELECT * FROM TEST_INSERT.T05 ORDER BY 1, 2, 3;

--问题 mysql均支持
--- TEST using table name on update
---error
INSERT INTO TEST_INSERT.T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE T05.col3 = DEFAULT, T05.col1 = T05.col3 + 1;
SELECT * FROM TEST_INSERT.T05 ORDER BY 1, 2, 3;

--- error: should not append schema
INSERT INTO TEST_INSERT.T05 VALUES(DEFAULT, DEFAULT, 200), (DEFAULT, 200, DEFAULT)
ON DUPLICATE KEY UPDATE TEST_INSERT.T05.col3 = DEFAULT, TEST_INSERT.T05.col1 = TEST_INSERT.T05.col3 + 2;
SELECT * FROM TEST_INSERT.T05 ORDER BY 1, 2, 3;

drop table T01,T00,T02,T03,T04;
drop schema TEST_INSERT cascade;
