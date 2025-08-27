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
