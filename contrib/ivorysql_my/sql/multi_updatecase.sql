
\h update


---ten table
drop table if exists T1;
drop table if exists T2;
drop table if exists T3;
drop table if exists T4;
drop table if exists T5;
drop table if exists T6;
drop table if exists T7;
drop table if exists T8;
drop table if exists T9;
drop table if exists T10;

create table T1(a int,b int);
create table T2(c int ,d int);
create table T3(e int, f int);
create table T4(g int, h int);
create table T5(i int, j int);
create table T6(h int, k int);
create table T7(m int, n int);
create table T8(q int, w int);
create table T9(x int, y int);
create table T10(o int, p int);
insert into  T1 values(1,2),(2,3);
insert into T2 values(1,2),(2,3);
insert into T3 values(1,2),(2,3);
insert into T4 values(1,2),(2,3);
insert into T5 values(1,2),(2,3);
insert into T6 values(1,2),(2,3);
insert into T7 values(1,2),(2,3);
insert into T8 values(1,2),(2,3);
insert into T9 values(1,2),(2,3);
insert into T10 values(1,2),(2,3);


--subquery
begin;
update  T1 ta,(select c,d from T2)tb set ta.a = tb.c + 20 where ta.a = tb.c;
select * from  T1;
rollback;

--subquery clonum can not update,error
update  T1 ta,(select c,d from T2)tb set ta.a = tb.c + 20, tb.d = 33 where ta.a = tb.c;--error

--LOW_PRIORITY
begin;
update LOW_PRIORITY  T1,T2 set  T1.a = T2.d,T2.c = T2.c * 2
where  T1.a = T2.c ;
select * from  T1;
select * from T2;
rollback;

--IGNORE
begin;
update IGNORE  T1,T2 set  T1.a = T2.d,T2.c = T2.c * 2
where  T1.a = T2.c ;
select * from  T1;
select * from T2;
rollback;

begin;
update  T1,T2 set  T1.a = T2.d,T2.c = T2.c * 2
where  T1.a = T2.c ;
select * from  T1;
select * from T2;
rollback;

begin;
update  T1,T2,T3 set  T1.a = T2.d,T2.c = T2.c + 12,T3.f =  T3.f + 22
where  T1.a = T2.c;
select * from  T1;
select * from T2;
select * from T3;
rollback;

begin;
update  T1,T2,T3,T4 set  T1.a = T2.d,T2.c = T2.c + 12,T3.f =  T3.f + 22,T4.h = 33
where  T1.a = T2.c;
select * from  T1;
select * from T2;
select * from T3;
select * from T4;
rollback;

begin;
update  T1,T2,T3,T4,T5 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44
where  T1.a = T2.c and  T1.a = T3.e and T5.i = T2.c and T4.g = T3.e ;
select * from  T1;
select * from T2;
select * from T3;
select * from T4;
select * from T5;
rollback;

begin;
update T2, T1,T3,T4,T5 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44
where  T1.a = T2.c and  T1.a = T3.e;
select * from  T1;
select * from T2;
select * from T3;
select * from T4;
select * from T5;
rollback;

begin;
update T2, T1,T3,T4,T5,T6 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55
where  T1.a = T2.c and  T1.a = T3.e;
select * from  T1;
select * from T2;
select * from T3;
select * from T4;
select * from T5;
select * from T6;
rollback;

begin;
update T2, T1,T3,T4,T5,T6 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55
where  T1.a = T2.c and  T1.a = T3.e;
rollback;

begin;
update T2, T1,T3,T4,T5,T6,T7 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55,T7.m = 66
where  T1.a = T2.c and  T1.a = T3.e;
rollback;


begin;
update T2, T1,T3,T4,T5,T6,T7,T8 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55,T7.m = 66,
T8.q = 77
where  T1.a = T2.c and  T1.a = T3.e;
rollback;

begin;
update T2, T1,T3,T4,T5,T6,T7,T8,T9 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55,T7.m = 66,
T8.q = 77,T9.x = 88
where  T1.a = T2.c and  T1.a = T3.e;
rollback;

begin;
update T2, T1,T3,T4,T5,T6,T7,T8,T9,T10 set  T1.a = T2.d,T2.c = 12,T3.f = 22,T4.h = 33,T5.j = 44,T6.h = 55,
T7.m = 66,T8.q = 77,T9.x = 88,T10.o = 99 
where  T1.a = T2.c and  T1.a = T3.e and T7.m = T10.o;
select * from  T1;
select * from T2;
select * from T3;
select * from T4;
select * from T5;
select * from T6;
select * from T7;
select * from T8;
select * from T9;
select * from T10;
rollback;

begin;
update  T1 left join T2 on  T1.a = T2.c left join T3 on  T1.a = T3.e set  T1.a = T2.d + 5,T2.c = 12,T3.f = 22;
select * from  T1;
select * from T2;
select * from T3;
rollback;

drop table  T1,T2,T3,T4,T5,T6,T7,T8,T9,T10;

create table TT(id int);
insert into TT select generate_series(1,1000000);
create index tt_idx on TT(id);
create table CTT1(id int);
insert into CTT1 select generate_series(1,1000000);
create index tt1_idx on CTT1(id);
analyze TT;
analyze CTT1;
explain (costs false) update TT,CTT1 set TT.id = TT.id + 11,CTT1.id = CTT1.id + 10 where TT.id = CTT1.id and TT.id = 88;

begin;
update TT,CTT1 set TT.id = TT.id + 11,CTT1.id = CTT1.id + 10 where TT.id = CTT1.id and TT.id = 88;
select * from TT where id = 99;
select * from CTT1 where id = 98;
end;

--index hint
analyze TT;
explain (costs false)update TT use index(tt_idx)   set id = id + 4 where id = 99;
explain (costs false)update TT ignore index(tt_idx) set id = id + 4 where id = 99;
explain (costs false)update TT  force index(tt_idx) set id = id + 4 where id = 99;
explain (costs false)update TT use index(tt_idx) set id = id + 4 where id = 199;    
explain (costs false)update TT use key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT use index for join(tt_idx) set id = id + 4 where id = 399;
explain (costs false)update TT use index for order by (tt_idx) set id = id + 4 where id = 399;
explain (costs false)update TT use index for group by (tt_idx) set id = id + 4 where id = 399;
explain (costs false)update TT  ignore index(tt_idx) set id = id + 4 where id = 299;
explain (costs false)update TT  ignore key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  ignore index for join(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  ignore index for order by (tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  ignore index for group by(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  force index(tt_idx) set id = id + 4 where id = 299;
explain (costs false)update TT  force key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  force key for join(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  force key for order by (tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  force key for group by(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT use index(tt_idx)  set id = id + 4 where id = 199;
explain (costs false)update TT ignore index(tt_idx)  set id = id + 4 where id = 199;  
explain (costs false)update TT use index(tt_idx)  set id = id + 4 where id = 199;
explain (costs false)update TT  ignore index(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update TT  force index(tt_idx) set id = id + 4 where id = 199;
drop table TT,CTT1;

-- three relation
drop table if exists T_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
drop table if exists T_T_MUTIL_T3;
create table T_T_MUTIL_T1(col1 int,col2 int);
create table T_T_MUTIL_T2(col1 int,col2 int);
create table T_T_MUTIL_T3(col1 int,col2 int);
insert into T_T_MUTIL_T1 values(1,1),(1,1);
insert into T_T_MUTIL_T2 values(1,1),(1,2);
insert into T_T_MUTIL_T3 values(1,1),(1,3);
begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b,T_T_MUTIL_T3 c set b.col2=5,a.col2=4,c.col2=6 where a.col1=b.col1 and b.col1=c.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col2=7,b.col2=8 where a.col1=b.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
rollback;
-- subselect
begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=5,a.col2=4 where a.col1 in (select col1 from T_T_MUTIL_T2);
rollback;
-- setof type, report error
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col1 = generate_series(2,3), b.col1=1;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col1 = generate_series(2,3), a.col1=1;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col1 = generate_series(2,3),b.col1 =123;
-- condition is false
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col1=2,b.col1=2  where a.col1 = 1 and a.col1=2;
-- duplicate col
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set col1= 3; -- error
begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col1=3,T_T_MUTIL_T1.col2=3;
rollback;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col1=3,T_T_MUTIL_T1.col1=3; --error

begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
rollback;

-- procedure
CREATE OR REPLACE PROCEDURE proc_mutil
(
c1 int,
c2 int
)
as $$ 
BEGIN
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=c1, a.col2=c2 where a.col1=b.col1;
END $$ language plmysql;

update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=5, a.col2=6 where a.col1=b.col1;

CALL proc_mutil(5,6);
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
-- update one rel of two
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set a.col2 =1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2 =2;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
-- hash col
CREATE SCHEMA CMULTIBLOCK1;
drop table if exists CMULTIBLOCK1.T_MUTIL_BC_1;
drop table if exists CMULTIBLOCK1.t_mutil_bc_2;
CREATE TABLE CMULTIBLOCK1.T_MUTIL_BC_1(col1 int, col2 int);
CREATE TABLE CMULTIBLOCK1.t_mutil_bc_2(col1 int, col2 int);
insert into CMULTIBLOCK1.T_MUTIL_BC_1 values(1,2),(2,2);
insert into CMULTIBLOCK1.t_mutil_bc_2 values(2,2),(3,3);
update CMULTIBLOCK1.T_MUTIL_BC_1 a,CMULTIBLOCK1.t_mutil_bc_2 b set a.col2=5,b.col2=4 where a.col1=b.col1;
create table CHASHTABLE(hash int);
update CMULTIBLOCK1.T_MUTIL_BC_1 a,CHASHTABLE set CHASHTABLE.hash=1;
select * from CMULTIBLOCK1.T_MUTIL_BC_1;
select * from CMULTIBLOCK1.t_mutil_bc_2;
DROP SCHEMA CMULTIBLOCK1 cascade;
DROP TABLE CHASHTABLE;
-- subquery
update T_T_MUTIL_T1 a,(select * from T_T_MUTIL_T2) b set b.col1=5,a.col2=4 where a.col1=b.col1; --error
-- mateview
CREATE MATERIALIZED VIEW MATE_MULTIVIEW1 as select * from T_T_MUTIL_T1;
CREATE MATERIALIZED VIEW MATE_MULTIVIEW2 as select * from T_T_MUTIL_T2;
update T_T_MUTIL_T1 a,MATE_MULTIVIEW1 b,MATE_MULTIVIEW2 c set a.col1 = 4, b.col2 = 5,c.col2 =6; --error
drop MATERIALIZED VIEW MATE_MULTIVIEW1;
drop MATERIALIZED VIEW MATE_MULTIVIEW2;
-- same relname
begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T1 b set a.col2=5,b.col2=4 where a.col1=b.col1;
select * from T_T_MUTIL_T1;
rollback;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 a set a.col2=5; --error
-- different explain plan
explain(verbose) update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
explain(format xml) update public.T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
explain(format json) update public.T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
explain(format yaml) update public.T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
-- temp table
drop table if exists T_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
create temporary table T_T_MUTIL_T1(col1 int, col2 int);
create temporary table T_T_MUTIL_T2(col1 int,col2 int);
insert into T_T_MUTIL_T1 values(1,2),(2,2);
insert into T_T_MUTIL_T2 values(2,2),(3,3);
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
-- order by or limit
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int,col2 int);
create table T_MUTIL_T2(col1 int,col2 int);
insert into T_MUTIL_T1 values(1,2),(1,2);
insert into T_MUTIL_T2 values(2,3),(2,3);
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col2=4,b.col1=4 where a.col1=b.col2 order by a.col2;  --error
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col2=4,b.col1=4 where a.col1=b.col2 limit 1;    --error
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
-- returning
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col2=4,b.col1=4 where a.col1=b.col2 returning *; --error
-- left join
begin;
update T_MUTIL_T1 a left join T_MUTIL_T2 b on a.col1=b.col1 set a.col2=7,b.col2=8;
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
rollback;
-- default col
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int default 1,col2 int);
create table T_MUTIL_T2(col1 int,col2 int default 3);
insert into T_MUTIL_T1 values(default,2);
insert into T_MUTIL_T2 values(2,default);
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col2=4,b.col1=4 where a.col2=b.col1; 
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col1=5,b.col2=6 where a.col2=b.col1; 
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col1=default,b.col2=default where a.col2=b.col1; 
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
-- generated col
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int generated always as (col2*2) stored,col2 int);
create table T_MUTIL_T2(col1 int,col2 int generated always as (col1*3) stored);
insert into T_MUTIL_T1 values(default,2);
select * from T_MUTIL_T1;
update T_MUTIL_T1 set col1 = default,col2=4;
select * from T_MUTIL_T1;
-- has index
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int);
create table T_MUTIL_T2(col1 int,col2 int);
create index idx1 on T_MUTIL_T1(col1);
create index idx2 on T_MUTIL_T2(col1);
insert into T_MUTIL_T1 values(1),(2);
insert into T_MUTIL_T2 values(2,2),(3,3);
update T_MUTIL_T1 a,T_MUTIL_T2 b set a.col1=4,b.col1=5 where a.col1=b.col1;
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
-- primary key
drop table if exists T_MUTIL_PRIMARY;
create table T_MUTIL_PRIMARY(col1 int primary key);
insert into T_MUTIL_PRIMARY values(1),(2);
update T_MUTIL_PRIMARY a,T_MUTIL_PRIMARY b set a.col1=3,b.col1=3;
-- col of one table is the name of the other table
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(T_MUTIL_T2 int);
create table T_MUTIL_T2(col1 int);
insert into T_MUTIL_T1 values(1);
insert into T_MUTIL_T2 values(2);
update T_MUTIL_T1,T_MUTIL_T2 set T_MUTIL_T1.T_MUTIL_T2=3,T_MUTIL_T2.col1=4;
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
update T_MUTIL_T1,T_MUTIL_T2 set T_MUTIL_T2.col1=4,T_MUTIL_T1.T_MUTIL_T2=3;
-- duplicate tablename
update T_MUTIL_T1,T_MUTIL_T1 set T_MUTIL_T1.T_MUTIL_T2 = 5; -- error
-- duplicate colname
update T_MUTIL_T1 set T_MUTIL_T1.T_MUTIL_T2=4,T_MUTIL_T1.T_MUTIL_T2=5; -- error
update T_MUTIL_T1,T_MUTIL_T2 set T_MUTIL_T1.T_MUTIL_T2=4,T_MUTIL_T1.T_MUTIL_T2=5; -- error
-- ARupdate trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL1 SET des1_id3 = NEW.src1_id3 WHERE des1_id1=OLD.src1_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL2 SET des2_id3 = NEW.src2_id3 WHERE des2_id1=OLD.src2_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
AFTER UPDATE ON TEST_TRIGGER_SRC_TBL1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
AFTER UPDATE ON TEST_TRIGGER_SRC_TBL2 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
UPDATE TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 
-- BRupdate trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL1 SET des1_id3 = 800;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL2 SET des2_id3 = 800;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
BEFORE UPDATE ON TEST_TRIGGER_SRC_TBL1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
BEFORE UPDATE ON TEST_TRIGGER_SRC_TBL2 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
UPDATE TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 

-- BSupdate trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL1 SET des1_id3 = 800;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL2 SET des2_id3 = 900;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
BEFORE UPDATE ON TEST_TRIGGER_SRC_TBL1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
BEFORE UPDATE ON TEST_TRIGGER_SRC_TBL2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func2();
UPDATE TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 
-- ASupdate trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL1 SET des1_id3 = 800;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE TEST_TRIGGER_DES_TBL2 SET des2_id3 = 900;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
AFTER UPDATE ON TEST_TRIGGER_SRC_TBL1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
AFTER UPDATE ON TEST_TRIGGER_SRC_TBL2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func2();
UPDATE TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 


drop table if exists T_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
drop table if exists T_T_MUTIL_T3;
create table T_T_MUTIL_T1(col1 int,col2 int);
create table T_T_MUTIL_T2(col1 int,col2 int,col3 int);
create table T_T_MUTIL_T3(col1 int,col2 int);
insert into T_T_MUTIL_T1 values(1,1),(1,1);
insert into T_T_MUTIL_T2 values(1,1),(1,2);
insert into T_T_MUTIL_T3 values(1,1),(1,3);
begin;
update T_T_MUTIL_T1 a,T_T_MUTIL_T2 b,T_T_MUTIL_T3 c set b.col2=5,a.col2=4 where a.col1=b.col1 and b.col1=c.col1;
rollback;

begin;
update T_T_MUTIL_T1  T1,T_T_MUTIL_T2 T2 set T2.col2 = 3,  T1.col1 = 2;
select * from T_T_MUTIL_T1;
rollback;
begin;
update T_T_MUTIL_T1  T1, T_T_MUTIL_T1 T2, T_T_MUTIL_T1 T3 set T2.col2 = 3, T3.col1 = 2;
rollback;
begin;
update T_T_MUTIL_T2  T1, T_T_MUTIL_T2 T2, T_T_MUTIL_T2 T3 set  T1.col1 = 3, T3.col2 = 2, T2.col3 = 3;
rollback;
begin;
update T_T_MUTIL_T1  T1 inner join T_T_MUTIL_T2 T2 on  T1.col1=T2.col1 inner join T_T_MUTIL_T3 T3 on  T1.col1=T3.col1 set  T1.col2=3,T2.col2=4;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;

rollback;

-- view
create view MULTIVIEW1 as select * from T_T_MUTIL_T1;
create view MULTIVIEW2 as select * from T_T_MUTIL_T2;
create view MULTIVIEW3 as select * from T_T_MUTIL_T3;
begin;
update MULTIVIEW1 a,MULTIVIEW2 b,MULTIVIEW3 c set a.col2 = 6, b.col2 = 7,c.col2 =8 where a.col1 = b.col1 and a.col1 = c.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
rollback;

begin;
update T_T_MUTIL_T1 a,MULTIVIEW2 b,T_T_MUTIL_T3 c set a.col2 = 4, b.col2 = 5,c.col2 = 6 where a.col1 = b.col1 and a.col1 = c.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
rollback;

begin;
alter table T_T_MUTIL_T1 alter column col2 set default 999;
update T_T_MUTIL_T1 a,MULTIVIEW2 b,T_T_MUTIL_T3 c set a.col2 = default, b.col2 = 5,c.col2 = 6 where a.col1 = b.col1 and a.col1 = c.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
rollback;
-- left join
begin;
update MULTIVIEW1 a left join MULTIVIEW2 b on a.col1=b.col1 set a.col2=7,b.col2=8;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
rollback;
-- update same relation
begin;
update T_T_MUTIL_T1 a,MULTIVIEW2 b,T_T_MUTIL_T2 c,T_T_MUTIL_T3 d set a.col2 = 6, b.col2 = 9, c.col3 = 10, d.col2 = 6 where a.col1 = b.col1 and a.col1 = d.col1;
select * from T_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
rollback;
drop view MULTIVIEW1;
drop view MULTIVIEW2;
drop view MULTIVIEW3;
-- multi update with join
drop table if exists  T1;
drop table if exists T2;
CREATE TABLE  T1 (f1 int);
CREATE TABLE T2 (f1 int);
INSERT INTO T2  VALUES (1);
CREATE VIEW V1 AS SELECT * FROM T2;
UPDATE T2 AS A NATURAL JOIN V1 B SET B.f1 = 2;
select * from V1;
drop table  T1;
drop view V1;
drop table T2;
