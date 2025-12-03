--
-- Test compatible Oracle ROWNUM pseudo-column.
--
drop table if exists eemp;
CREATE TABLE eemp(
    empno           NUMBER(4) NOT NULL CONSTRAINT emp_pk PRIMARY KEY,
    ename           VARCHAR2(10),
    job             VARCHAR2(9),
    mgr             NUMBER(4),
    hiredate        DATE,
    sal             NUMBER(7,2) CONSTRAINT emp_sal_ck CHECK (sal > 0),
    comm            NUMBER(7,2),
    deptno          NUMBER(2)
);

INSERT INTO eemp VALUES (7369,'SMITH','CLERK',7902, to_date('17-10-1980','dd-mm-yyyy'),800,NULL,20);
INSERT INTO eemp VALUES (7499,'ALLEN','SALESMAN',7698,to_date('20-02-1981','dd-mm-yyyy'),1600,300,30);
INSERT INTO eemp VALUES (7521,'WARD','SALESMAN',7698,to_date('22-02-1981','dd-mm-yyyy'),1250,500,30);
INSERT INTO eemp VALUES (7566,'JONES','MANAGER',7839,to_date('02-04-1981','dd-mm-yyyy'),2975,NULL,20);
INSERT INTO eemp VALUES (7654,'MARTIN','SALESMAN',7698,to_date('28-09-1981','dd-mm-yyyy'),1250,1400,30);
INSERT INTO eemp VALUES (7698,'BLAKE','MANAGER',7839,to_date('01-05-1981','dd-mm-yyyy'),2850,NULL,30);
INSERT INTO eemp VALUES (7782,'CLARK','MANAGER',7839,to_date('09-06-1981','dd-mm-yyyy'),2450,NULL,10);
INSERT INTO eemp VALUES (7788,'SCOTT','ANALYST',7566,to_date('17-04-1987','dd-mm-yyyy'),3000,NULL,20);
INSERT INTO eemp VALUES (7839,'KING','PRESIDENT',NULL,to_date('17-11-1981','dd-mm-yyyy'),5000,NULL,10);
INSERT INTO eemp VALUES (7844,'TURNER','SALESMAN',7698,to_date('08-09-1981','dd-mm-yyyy'),1500,0,30);
INSERT INTO eemp VALUES (7876,'ADAMS','CLERK',7788,to_date('23-05-1987','dd-mm-yyyy'),1100,NULL,20);
INSERT INTO eemp VALUES (7900,'JAMES','CLERK',7698,to_date('03-10-1981','dd-mm-yyyy'),950,NULL,30);
INSERT INTO eemp VALUES (7902,'FORD','ANALYST',7566,to_date('17-10-1980','dd-mm-yyyy'),3000,NULL,20);
INSERT INTO eemp VALUES (7934,'MILLER','CLERK',7782,to_date('23-02-1982','dd-mm-yyyy'),1300,NULL,10);


select rownum, * from eemp;

select rownum, * from eemp where rownum  > 5;

select rownum, * from eemp where rownum  < 5;

select rownum, * from eemp where rownum  != 5;

select rownum, * from eemp where rownum  >= 5;

select rownum, * from eemp where rownum  <= 5;

select rownum, * from eemp where rownum  = 5;

select rownum, * from eemp where rownum = 1;

select rownum, * from eemp where rownum > 1;

select rownum, * from eemp where rownum >= 1;

select rownum, * from eemp where rownum > 0;

select rownum, * from eemp where rownum < 0;

select empno, ename, job rownum from eemp; -- error

select empno, ename, job as rownum from eemp; -- error

select * from eemp where rownum <= 5 order by sal desc;

select * from
	(select * from eemp order by sal desc)
  where rownum <= 5;

select *
  from (select * from eemp where rownum < 8 ORDER BY deptno)
  where rownum < 5;

-- Pagination with ROWNUM
select *
	from
	(select a.*, rownum rnum
		from
		(select empno, sal
			from eemp
		 order by sal) a
		where rownum <= 12
	)
	where rnum >= 8;

select deptno from eemp where rownum < 5 group by deptno;

select rownum from eemp group by deptno; -- error

select empno, deptno, avg(sal) as averge_sal from eemp
	where rownum < 5 group by empno, deptno;

select deptno, MAX(rownum), MIN(rownum), SUM(rownum), AVG(rownum)
	from eemp GROUP BY deptno ORDER BY deptno;

select
   ename,
   hiredate
from
   eemp
where
   hiredate = (select max(hiredate) from eemp)
and
   rownum = 1;

select rownum, empno, ename, job, mgr, sal, deptno from eemp
	where deptno = 20 and rownum <= 5;

select rownum, empno, ename, job, mgr, sal, deptno from eemp where (deptno = 20 and rownum <= 5)
	or (job = 'CLERK' and rownum <= 10);

--
-- Test View
--
create view e_v as select * from eemp;

select rownum, * from e_v where rownum < 5;

drop view e_v;
drop table eemp;

--
-- Test ROWNUM in update/delete
--
drop table if exists tr;
create table tr(a int, b int);

insert into tr values(11, 2);
insert into tr values(3, 5);
insert into tr values(2, 9);
insert into tr values(100, 17);
insert into tr values(33, 14);
insert into tr values(21, 6);
insert into tr values(50, 36);

select * from tr order by a;

select * from tr where rownum < 3;

update tr set b = a + b where rownum < 3;

select * from tr order by a;

update tr set b = rownum;

select * from tr;

delete from tr where rownum < 3;

select * from tr;

drop table tr;

--
-- Test ROWNUM in Merge command
--
drop table if exists t1;
drop table if exists s1;
create table t1(id number, name varchar2(32), score numeric(8));
create table s1(id number, name varchar2(32), score numeric(8));

insert into t1 values (1, 'Jack', 540);
insert into t1 values (2, 'Rose', null);
insert into t1 values (3, 'William', 650);
insert into t1 values (4, 'Caledon', 620);
insert into t1 values (5, 'Fabrizio', 600);
insert into t1 values (6, 'Thomas', null);
insert into t1 values (7, 'Ruth', 680);
insert into t1 values (8, 'Spacer', 555);

insert into s1 values (7, 'Ruth', 690);
insert into s1 values (8, 'Spicer', 620);
insert into s1 values (9, 'Wallace', 600);
insert into s1 values (10, 'Lizzy', null);
insert into s1 values (11, 'Brock', 705);
insert into s1 values (1, 'Booker', 709);
insert into s1 values (2, 'Tony', null);

select * from t1;
select * from s1;

begin;
merge into t1 a
using (select id, name, score from s1 where rownum < 3) b
 on (a.id = b.id)
when matched then
 update set a.name = b.name,
            a.score = b.score
;
select * from t1 order by id;
rollback;

begin;
merge into t1 a
using (select id, name, score from s1) b
 on (a.id = b.id)
when matched then
 update set a.name = b.name,
            a.score = b.score where rownum < 3
;
select * from t1 order by id;
rollback;

begin;
merge into t1 a
using (select id, name, score from s1) b
 on (a.id = b.id)
when matched then
 update set a.score = rownum
;

select * from t1 order by id;
rollback;

drop table t1;
drop table s1;

--
-- Test ROWNUM in partition table
--
drop table if exists partition_range;
CREATE TABLE partition_range (
	   c1 int,
	   c2 int
)PARTITION BY RANGE (c2);

CREATE TABLE p1 PARTITION OF partition_range FOR VALUES FROM (1) to (1000);
CREATE TABLE p2 PARTITION OF partition_range FOR VALUES FROM (1000) to (2000);
CREATE TABLE p3 PARTITION OF partition_range FOR VALUES FROM (2000) to (2500);

insert into partition_range values(1,200);
insert into partition_range values(1,300);
insert into partition_range values(1,400);
insert into partition_range values(1,1500);
insert into partition_range values(1,1600);
insert into partition_range values(1,1700);
insert into partition_range values(1,2100);
insert into partition_range values(1,2300);

select * from partition_range;

select rownum, * from partition_range;

select rownum, * from partition_range where rownum < 5;

select rownum, * from p1;

select rownum, * from p1 where rownum < 2;

drop table partition_range cascade;

--
-- Cursor test
--
CREATE TABLE test_cursor_null (c1 int);
CREATE TABLE test_cursor (c1 serial, c2 int);

create or replace procedure prepare_test_cursor()
as
	i int;
begin
	for i in 1..60 loop
		insert into test_cursor(c2) values (i);
	end loop;
end;
/

call prepare_test_cursor();

-- Case 1: curse with hold
-- 1.1 null
DECLARE C1 CURSOR WITH HOLD FOR SELECT * FROM test_cursor_null;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 5 IN C1;

FETCH RELATIVE 1 IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

CLOSE C1;

-- 1.2 common value
DECLARE C1 CURSOR WITH HOLD FOR SELECT * FROM test_cursor;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;

-- 1.3 limit..offset
DECLARE C1 CURSOR WITH HOLD FOR SELECT * FROM test_cursor WHERE rownum < 10 ORDER BY c2 LIMIT 1;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;


DECLARE C1 SCROLL CURSOR WITH HOLD FOR
SELECT *
FROM (SELECT *, rownum rn FROM test_cursor WHERE rownum <= 25) AS foo
WHERE rn > 5 LIMIT 10 OFFSET 5;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;

-- Case 2: cursor with transaction
-- 2.1 null
BEGIN;
DECLARE C1 SCROLL CURSOR FOR SELECT * FROM test_cursor_null;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 5 IN C1;

FETCH RELATIVE 1 IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;
COMMIT;

-- 2.2 common value
BEGIN;
DECLARE C1 SCROLL CURSOR FOR SELECT * FROM test_cursor;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;

-- 2.3 limit..offset
BEGIN;
DECLARE C1 SCROLL CURSOR FOR
SELECT * FROM test_cursor WHERE rownum < 10 ORDER BY c2 LIMIT 1;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;
COMMIT;

BEGIN;
DECLARE C1 SCROLL CURSOR FOR
SELECT *
FROM (SELECT *, rownum rn FROM test_cursor WHERE rownum <= 25) AS foo
WHERE rn > 5 LIMIT 10 OFFSET 5;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;
COMMIT;

-- Case 3: special test case
BEGIN;
DECLARE C1 SCROLL CURSOR FOR
SELECT *
FROM (SELECT *, rownum rn FROM test_cursor WHERE rownum <= 25) AS foo
WHERE rn > 5 LIMIT 10 OFFSET 5;

FETCH 50 IN C1;

FETCH FIRST IN C1;

FETCH LAST IN C1;

FETCH PRIOR IN C1;

FETCH NEXT IN C1;

FETCH ABSOLUTE 0 IN C1;

FETCH RELATIVE 0 IN C1;

FETCH ABSOLUTE 17 IN C1;

FETCH RELATIVE -3 IN C1;

FETCH ABSOLUTE -3 IN C1;

FETCH RELATIVE 13 IN C1;

FETCH BACKWARD 50 IN C1;

FETCH FORWARD 50 IN C1;

CLOSE C1;
COMMIT;

-- cleanup
DROP PROCEDURE prepare_test_cursor();
DROP TABLE test_cursor_null;
DROP TABLE test_cursor;

/*Begin - BUG#Z1278*/
create table tr(a int, b int);

insert into tr values(11, 2);
insert into tr values(3, 5);
insert into tr values(2, 9);
insert into tr values(100, 17);
insert into tr values(33, 14);
insert into tr values(21, 6);
insert into tr values(50, 36);

select rownum, * from tr where rownum < 4;

show compute_query_id;
set compute_query_id to on; --enable jumblequery
show compute_query_id;
select rownum, * from tr where rownum < 4;
create view vtr as select * from tr;
select rownum, * from vtr;
reset compute_query_id;
drop table tr;
/*End - BUG#Z1278*/

/* Begin - BUG#M0000450 and BUG#M0000451 */
create or replace package pkg_rownum is
  procedure test_pf(id integer, rownum in out varchar2);
end;
/

create or replace package body pkg_rownum is
  procedure test_pf(id integer,rownum in out varchar2) is
    var1 varchar2(23);
  begin
    rownum := 'xiexie' || rownum;
	var1 := rownum;
  end;
end;
/

create or replace function test_f(id integer, rownum in out varchar2) return integer is
begin
  pkg_rownum.test_pf(id, rownum);
  pkg_rownum.test_pf(id=>id,rownum=>rownum);
  raise notice '%', rownum;
  return 23;
end;
/

--print xiexiexiexie
declare
  id integer;
  rownum varchar2(256);
  ret integer;
begin
  ret := test_f(id, rownum);
end;
/

--print NULL
declare
   rownum varchar2(256);
begin
  select rownum into rownum from dual;
  raise notice '%', rownum;
end;
/

--print 111
declare
  rownum integer;
begin
  rownum := 111;
  raise notice '%', rownum;
end;
/

create or replace function test_inquote("Rownum" integer) return integer is
begin
  return 23;
end;
/

declare
 rownum integer;
 ret integer;
begin
  ret := test_inquote("Rownum"=>rownum);
end;
/

--cleanup
drop function test_inquote;
drop function test_f;
drop package pkg_rownum;
/* End - BUG#M0000450 and BUG#M0000451 */

/* Begin - OA-REQ-1698 */
drop table if exists empg;
CREATE TABLE empg (
    emp_id NUMBER,
    name VARCHAR2(100),
    salary NUMBER,
    hire_date DATE
);

INSERT INTO empg VALUES (7380,'JODEN',900, to_date('22-09-1989','dd-mm-yyyy'));
INSERT INTO empg VALUES (7380,NULL,900, to_date('23-09-1989','dd-mm-yyyy'));
INSERT INTO empg VALUES (7380,'HADEN',NULL, to_date('24-09-1989','dd-mm-yyyy'));

set ivorysql.enable_emptystring_to_null to on;

select * from empg where rownum <= 1 and emp_id = 7380 and salary = '';
select * from empg where rownum <= 1 and emp_id = 7380 and salary = NULL;
select * from empg where rownum <= 1 and salary IS NULL;

set ivorysql.enable_emptystring_to_null to off;
select * from empg where rownum <= 1 and emp_id = 7380 and salary = '';
select * from empg where rownum <= 1 and emp_id = 7380 and salary = NULL;
select * from empg where rownum <= 1 and salary IS NULL;

reset ivorysql.enable_emptystring_to_null;
drop table empg;
/* End - OA-REQ-1698 */


---bug#Z6495
create table lp (a char) partition by list (a);
create table lp_default partition of lp default;
create table lp_ef partition of lp for values in ('e', 'f');
create table lp_ad partition of lp for values in ('a', 'd');
create table lp_bc partition of lp for values in ('b', 'c');
create table lp_g partition of lp for values in ('g');


update lp set  a = 'g' where rownum < 3;

insert into lp values('e');
insert into lp values('a');
insert into lp values('b');
insert into lp values('g');

select  *from lp order by a;

update lp set a = 'x' where rownum < 2;

select count(*) from lp where a = 'x';

insert into lp values('x');

delete lp where a = 'x' and rownum < 1;

delete lp where a = 'x' and rownum < 2;

insert into lp values('m');
insert into lp values('m');

update lp set a = 'y' where a = 'm'  and rownum < 3;

select count(*) from lp where a = 'y';

update lp set a = 'x' where a = 'y' and rownum > 4;


--complex test
CREATE TABLE partiton_first_level (a int, b int, c varchar2(256)) PARTITION BY range (a);

CREATE TABLE partiton_second_test1 PARTITION OF partiton_first_level
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY list (b);

CREATE TABLE partiton_second_test2 PARTITION OF partiton_first_level
  FOR VALUES FROM (0) TO (100) PARTITION BY list (b);

CREATE TABLE partiton_second_test3 PARTITION OF partiton_first_level
  FOR VALUES FROM (100) TO (200) partition BY list(b);

CREATE TABLE partiton_second_leaf1 PARTITION OF partiton_second_test1 FOR VALUES IN (1);
CREATE TABLE partiton_second_leaf2 PARTITION OF partiton_second_test1 FOR VALUES IN (2);
CREATE TABLE partiton_second_leaf3 PARTITION OF partiton_second_test1 FOR VALUES IN (3);


CREATE TABLE partiton_second_leaf4 PARTITION OF partiton_second_test2 FOR VALUES IN (1);
CREATE TABLE partiton_second_leaf5 PARTITION OF partiton_second_test2 FOR VALUES IN (2);
CREATE TABLE partiton_second_leaf6 PARTITION OF partiton_second_test2 FOR VALUES IN (3);


CREATE TABLE partiton_second_leaf7 PARTITION OF partiton_second_test3 FOR VALUES IN (1);
CREATE TABLE partiton_second_leaf8 PARTITION OF partiton_second_test3 FOR VALUES IN (2);
CREATE TABLE partiton_second_leaf9 PARTITION OF partiton_second_test3 FOR VALUES IN (3);



CREATE TABLE partiton_second_test4 PARTITION OF partiton_first_level
  FOR VALUES FROM (200) TO (300);

insert into partiton_first_level values(-1, 1,'-1');
insert into partiton_first_level values(-3, 2 ,'-3');
insert into partiton_first_level values(-4, 3 ,'-4');


insert into partiton_first_level values(55, 1,'55');
insert into partiton_first_level values(78, 2 ,'78');
insert into partiton_first_level values(99, 3 ,'99');


insert into partiton_first_level values(104, 1,'104');
insert into partiton_first_level values(156, 2 ,'156');
insert into partiton_first_level values(199, 3 ,'199');


insert into partiton_first_level values(245, 9,'245');
insert into partiton_first_level values(235, 22 ,'235');
insert into partiton_first_level values(299, 28 ,'299');

--one row update
update partiton_first_level set a = 245,b = 28,c = 'xiexie' where a < -1 and rownum < 2;

--2 rows delete
delete from partiton_first_level where a >0 and a < 100 and rownum < 3;

--3 rows update
update partiton_first_level set a = a + 1,b = 3,c = c || 'xiexie' where a > 100 and a < 200 and rownum < 4;

--update 0
update partiton_first_level set a = '1',b = 4,c = 'sfjs' where a > 0 and rownum > 2;

--0 delete
delete from partiton_first_level where rownum > 2;

--2 rows delete
delete from partiton_first_level where rownum < 3;

--0 update
update partiton_first_level set a = 1000,b = 29,c = 'sfs' where rownum > 9;

--4 rows update
update partiton_first_level set a = 100,b = 1,c = 'sfs' where rownum < 5;

--clean data
drop table lp;
drop table partiton_first_level;

/* Begin - BUG#Z6554 */
DROP TABLE IF EXISTS TB1;
CREATE TABLE TB1 (
    emp_id NUMBER PRIMARY KEY,
    emp_name VARCHAR2(50),
    salary NUMBER(10,2)
);

CREATE VIEW VIEW1 AS
    SELECT emp_id, emp_name, salary FROM TB1;

INSERT INTO VIEW1 (emp_id, emp_name, salary)
  SELECT 101, 'Bob', 5000 FROM DUAL
UNION ALL
  SELECT 102, 'Tom', 6000 FROM DUAL
UNION ALL
  SELECT 103, 'Mary', 7000 FROM DUAL;

select * from VIEW1 order by emp_id;

update view1 set salary = salary -100 where salary > 4000 and rownum < 3;

select * from VIEW1 order by emp_id;

DROP VIEW VIEW1;
DROP TABLE TB1;

DROP TABLE IF EXISTS sales CASCADE;
CREATE TABLE sales (
    id          int,
    category    varchar2(20),
    amount      number,
    region      varchar2(20),
    status      varchar2(20),
    col1        varchar2(20),
    col2        varchar2(20),
    col3        varchar2(20),
    col4        varchar2(20),
    col5        varchar2(20),
    col6        varchar2(20),
    col7        varchar2(20),
    col8        varchar2(20),
    col9        varchar2(20),
    col10       varchar2(20)
) PARTITION BY RANGE (id);

CREATE TABLE sales_p1 PARTITION OF sales
FOR VALUES FROM (1) TO (301);

CREATE TABLE sales_p2 PARTITION OF sales
FOR VALUES FROM (301) TO (601);

CREATE TABLE sales_p3 PARTITION OF sales
FOR VALUES FROM (601) TO (1001);

CREATE OR REPLACE VIEW sales_view AS
SELECT
    id,
    category,
    amount,
    region,
    status,
    col1,
    col2,
    col3,
    col4,
    col5,
    col6,
    col7,
    ctid,
    0 AS mds,
    sysdate as md
FROM sales;

INSERT INTO sales (
    id, category, amount, region, status,
    col1, col2, col3, col4, col5,
    col6, col7, col8, col9, col10
)
SELECT
    gs,
    'cat-' || gs,
    gs * 1.25,
    'region-' || (gs % 5),
    CASE WHEN gs % 2 = 0 THEN 'OK' ELSE 'PENDING' END,
    'c1-' || gs, 'c2-' || gs, 'c3-' || gs, 'c4-' || gs, 'c5-' || gs,
    'c6-' || gs, 'c7-' || gs, 'c8-' || gs, 'c9-' || gs, 'c10-' || gs
FROM generate_series(1, 1000) gs;

UPDATE sales_view
SET amount = amount + 100,
    status = 'UPDATED'
WHERE id < 10;

select * from sales where id < 20 order by id;

UPDATE sales_view
SET amount = amount + 100,
    status = 'ROWNUM_TEST'
WHERE amount < 25 AND rownum < 20;

select * from sales where id < 20 order by id;

UPDATE sales_view
SET amount = amount - 10,
    status = 'ROWNUM_OR_ROWNUM'
WHERE amount < 100 AND rownum < 2 OR rownum < 8;

select * from sales where status = 'ROWNUM_OR_ROWNUM' order by id;

--cleanup
DROP VIEW sales_view;
DROP TABLE sales;
/* End - BUG#Z6554 */