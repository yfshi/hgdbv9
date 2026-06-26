
\h update


---ten table
drop table if exists t1;
drop table if exists t2;
drop table if exists t3;
drop table if exists t4;
drop table if exists t5;
drop table if exists t6;
drop table if exists t7;
drop table if exists t8;
drop table if exists t9;
drop table if exists t10;

create table t1(a int,b int);
create table t2(c int ,d int);
create table t3(e int, f int);
create table t4(g int, h int);
create table t5(i int, j int);
create table t6(h int, k int);
create table t7(m int, n int);
create table t8(q int, w int);
create table t9(x int, y int);
create table t10(o int, p int);
insert into t1 values(1,2),(2,3);
insert into t2 values(1,2),(2,3);
insert into t3 values(1,2),(2,3);
insert into t4 values(1,2),(2,3);
insert into t5 values(1,2),(2,3);
insert into t6 values(1,2),(2,3);
insert into t7 values(1,2),(2,3);
insert into t8 values(1,2),(2,3);
insert into t9 values(1,2),(2,3);
insert into t10 values(1,2),(2,3);


--subquery
begin;
update t1 ta,(select c,d from t2)tb set ta.a = tb.c + 20 where ta.a = tb.c;
select * from t1;
rollback;

--subquery clonum can not update,error
update t1 ta,(select c,d from t2)tb set ta.a = tb.c + 20, tb.d = 33 where ta.a = tb.c;--error

--LOW_PRIORITY
begin;
update LOW_PRIORITY t1,t2 set t1.a = t2.d,t2.c = t2.c * 2
where t1.a = t2.c ;
select * from t1;
select * from t2;
rollback;

--IGNORE
begin;
update IGNORE t1,t2 set t1.a = t2.d,t2.c = t2.c * 2
where t1.a = t2.c ;
select * from t1;
select * from t2;
rollback;

begin;
update t1,t2 set t1.a = t2.d,t2.c = t2.c * 2
where t1.a = t2.c ;
select * from t1;
select * from t2;
rollback;

begin;
update t1,t2,t3 set t1.a = t2.d,t2.c = t2.c + 12,t3.f =  t3.f + 22
where t1.a = t2.c;
select * from t1;
select * from t2;
select * from t3;
rollback;

begin;
update t1,t2,t3,t4 set t1.a = t2.d,t2.c = t2.c + 12,t3.f =  t3.f + 22,t4.h = 33
where t1.a = t2.c;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
rollback;

begin;
update t1,t2,t3,t4,t5 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44
where t1.a = t2.c and t1.a = t3.e and t5.i = t2.c and t4.g = t3.e ;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
select * from t5;
rollback;

begin;
update t2,t1,t3,t4,t5 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44
where t1.a = t2.c and t1.a = t3.e;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
select * from t5;
rollback;

begin;
update t2,t1,t3,t4,t5,t6 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55
where t1.a = t2.c and t1.a = t3.e;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
select * from t5;
select * from t6;
rollback;

begin;
update t2,t1,t3,t4,t5,t6 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55
where t1.a = t2.c and t1.a = t3.e;
rollback;

begin;
update t2,t1,t3,t4,t5,t6,t7 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55,t7.m = 66
where t1.a = t2.c and t1.a = t3.e;
rollback;


begin;
update t2,t1,t3,t4,t5,t6,t7,t8 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55,t7.m = 66,
t8.q = 77
where t1.a = t2.c and t1.a = t3.e;
rollback;

begin;
update t2,t1,t3,t4,t5,t6,t7,t8,t9 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55,t7.m = 66,
t8.q = 77,t9.x = 88
where t1.a = t2.c and t1.a = t3.e;
rollback;

begin;
update t2,t1,t3,t4,t5,t6,t7,t8,t9,t10 set t1.a = t2.d,t2.c = 12,t3.f = 22,t4.h = 33,t5.j = 44,t6.h = 55,
t7.m = 66,t8.q = 77,t9.x = 88,t10.o = 99 
where t1.a = t2.c and t1.a = t3.e and t7.m = t10.o;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
select * from t5;
select * from t6;
select * from t7;
select * from t8;
select * from t9;
select * from t10;
rollback;

begin;
update t1 left join t2 on t1.a = t2.c left join t3 on t1.a = t3.e set t1.a = t2.d + 5,t2.c = 12,t3.f = 22;
select * from t1;
select * from t2;
select * from t3;
rollback;

drop table t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;

create table tt(id int);
insert into tt select generate_series(1,1000000);
create index tt_idx on tt(id);
create table tt1(id int);
insert into tt1 select generate_series(1,1000000);
create index tt1_idx on tt1(id);
analyze tt;
analyze tt1;
explain (costs false) update tt,tt1 set tt.id = tt.id + 11,tt1.id = tt1.id + 10 where tt.id = tt1.id and tt.id = 88;

begin;
update tt,tt1 set tt.id = tt.id + 11,tt1.id = tt1.id + 10 where tt.id = tt1.id and tt.id = 88;
select * from tt where id = 99;
select * from tt1 where id = 98;
end;

--index hint
analyze tt;
explain (costs false)update tt use index(tt_idx)   set id = id + 4 where id = 99;
explain (costs false)update tt ignore index(tt_idx) set id = id + 4 where id = 99;
explain (costs false)update tt  force index(tt_idx) set id = id + 4 where id = 99;
explain (costs false)update tt use index(tt_idx) set id = id + 4 where id = 199;    
explain (costs false)update tt use key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt use index for join(tt_idx) set id = id + 4 where id = 399;
explain (costs false)update tt use index for order by (tt_idx) set id = id + 4 where id = 399;
explain (costs false)update tt use index for group by (tt_idx) set id = id + 4 where id = 399;
explain (costs false)update tt  ignore index(tt_idx) set id = id + 4 where id = 299;
explain (costs false)update tt  ignore key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  ignore index for join(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  ignore index for order by (tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  ignore index for group by(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  force index(tt_idx) set id = id + 4 where id = 299;
explain (costs false)update tt  force key(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  force key for join(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  force key for order by (tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  force key for group by(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt use index(tt_idx)  set id = id + 4 where id = 199;
explain (costs false)update tt ignore index(tt_idx)  set id = id + 4 where id = 199;  
explain (costs false)update tt use index(tt_idx)  set id = id + 4 where id = 199;
explain (costs false)update tt  ignore index(tt_idx) set id = id + 4 where id = 199;
explain (costs false)update tt  force index(tt_idx) set id = id + 4 where id = 199;
drop table tt,tt1;

-- three relation
drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
drop table if exists t_t_mutil_t3;
create table t_t_mutil_t1(col1 int,col2 int);
create table t_t_mutil_t2(col1 int,col2 int);
create table t_t_mutil_t3(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,1),(1,1);
insert into t_t_mutil_t2 values(1,1),(1,2);
insert into t_t_mutil_t3 values(1,1),(1,3);
begin;
update t_t_mutil_t1 a,t_t_mutil_t2 b,t_t_mutil_t3 c set b.col2=5,a.col2=4,c.col2=6 where a.col1=b.col1 and b.col1=c.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col2=7,b.col2=8 where a.col1=b.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
rollback;
-- subselect
begin;
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=5,a.col2=4 where a.col1 in (select col1 from t_t_mutil_t2);
rollback;
-- setof type, report error
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col1 = generate_series(2,3), b.col1=1;
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col1 = generate_series(2,3), a.col1=1;
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col1 = generate_series(2,3),b.col1 =123;
-- condition is false
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col1=2,b.col1=2  where a.col1 = 1 and a.col1=2;
-- duplicate col
update t_t_mutil_t1 a,t_t_mutil_t2 b set col1= 3; -- error
begin;
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col1=3,t_t_mutil_t1.col2=3;
rollback;
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col1=3,t_t_mutil_t1.col1=3; --error

begin;
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
rollback;

-- procedure
CREATE OR REPLACE PROCEDURE proc_mutil
(
c1 int,
c2 int
)
as $$ 
BEGIN
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=c1, a.col2=c2 where a.col1=b.col1;
END $$ language plmysql;

update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=5, a.col2=6 where a.col1=b.col1;

CALL proc_mutil(5,6);
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
-- update one rel of two
update t_t_mutil_t1 a,t_t_mutil_t2 b set a.col2 =1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2 =2;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
-- hash col
CREATE SCHEMA multiblock1;
drop table if exists multiblock1.t_mutil_bc_1;
drop table if exists multiblock1.t_mutil_bc_2;
CREATE TABLE multiblock1.t_mutil_bc_1(col1 int, col2 int);
CREATE TABLE multiblock1.t_mutil_bc_2(col1 int, col2 int);
insert into multiblock1.t_mutil_bc_1 values(1,2),(2,2);
insert into multiblock1.t_mutil_bc_2 values(2,2),(3,3);
update multiblock1.t_mutil_bc_1 a,multiblock1.t_mutil_bc_2 b set a.col2=5,b.col2=4 where a.col1=b.col1;
create table hashtable(hash int);
update multiblock1.t_mutil_bc_1 a,hashtable set hashtable.hash=1;
select * from multiblock1.t_mutil_bc_1;
select * from multiblock1.t_mutil_bc_2;
-- subquery
update t_t_mutil_t1 a,(select * from t_t_mutil_t2) b set b.col1=5,a.col2=4 where a.col1=b.col1; --error
-- mateview
CREATE MATERIALIZED VIEW mate_multiview1 as select * from t_t_mutil_t1;
CREATE MATERIALIZED VIEW mate_multiview2 as select * from t_t_mutil_t2;
update t_t_mutil_t1 a,mate_multiview1 b,mate_multiview2 c set a.col1 = 4, b.col2 = 5,c.col2 =6; --error
drop MATERIALIZED VIEW mate_multiview1;
drop MATERIALIZED VIEW mate_multiview2;
-- same relname
begin;
update t_t_mutil_t1 a,t_t_mutil_t1 b set a.col2=5,b.col2=4 where a.col1=b.col1;
select * from t_t_mutil_t1;
rollback;
update t_t_mutil_t1 a,t_t_mutil_t2 a set a.col2=5; --error
-- different explain plan
explain(verbose) update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
explain(format xml) update public.t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
explain(format json) update public.t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
explain(format yaml) update public.t_t_mutil_t1 a,t_t_mutil_t2 b set b.col2=5,a.col2=4 where a.col1=b.col1;
-- temp table
drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
create temporary table t_t_mutil_t1(col1 int, col2 int);
create temporary table t_t_mutil_t2(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,2),(2,2);
insert into t_t_mutil_t2 values(2,2),(3,3);
update t_t_mutil_t1 a,t_t_mutil_t2 b set b.col1=5,a.col2=4 where a.col1=b.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
-- order by or limit
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int,col2 int);
create table t_mutil_t2(col1 int,col2 int);
insert into t_mutil_t1 values(1,2),(1,2);
insert into t_mutil_t2 values(2,3),(2,3);
update t_mutil_t1 a,t_mutil_t2 b set a.col2=4,b.col1=4 where a.col1=b.col2 order by a.col2;  --error
update t_mutil_t1 a,t_mutil_t2 b set a.col2=4,b.col1=4 where a.col1=b.col2 limit 1;    --error
select * from t_mutil_t1;
select * from t_mutil_t2;
-- returning
update t_mutil_t1 a,t_mutil_t2 b set a.col2=4,b.col1=4 where a.col1=b.col2 returning *; --error
-- left join
begin;
update t_mutil_t1 a left join t_mutil_t2 b on a.col1=b.col1 set a.col2=7,b.col2=8;
select * from t_mutil_t1;
select * from t_mutil_t2;
rollback;
-- default col
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int default 1,col2 int);
create table t_mutil_t2(col1 int,col2 int default 3);
insert into t_mutil_t1 values(default,2);
insert into t_mutil_t2 values(2,default);
update t_mutil_t1 a,t_mutil_t2 b set a.col2=4,b.col1=4 where a.col2=b.col1; 
select * from t_mutil_t1;
select * from t_mutil_t2;
update t_mutil_t1 a,t_mutil_t2 b set a.col1=5,b.col2=6 where a.col2=b.col1; 
select * from t_mutil_t1;
select * from t_mutil_t2;
update t_mutil_t1 a,t_mutil_t2 b set a.col1=default,b.col2=default where a.col2=b.col1; 
select * from t_mutil_t1;
select * from t_mutil_t2;
-- generated col
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int generated always as (col2*2) stored,col2 int);
create table t_mutil_t2(col1 int,col2 int generated always as (col1*3) stored);
insert into t_mutil_t1 values(default,2);
select * from t_mutil_t1;
update t_mutil_t1 set col1 = default,col2=4;
select * from t_mutil_t1;
-- has index
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int);
create table t_mutil_t2(col1 int,col2 int);
create index idx1 on t_mutil_t1(col1);
create index idx2 on t_mutil_t2(col1);
insert into t_mutil_t1 values(1),(2);
insert into t_mutil_t2 values(2,2),(3,3);
update t_mutil_t1 a,t_mutil_t2 b set a.col1=4,b.col1=5 where a.col1=b.col1;
select * from t_mutil_t1;
select * from t_mutil_t2;
-- primary key
drop table if exists t_mutil_primary;
create table t_mutil_primary(col1 int primary key);
insert into t_mutil_primary values(1),(2);
update t_mutil_primary a,t_mutil_primary b set a.col1=3,b.col1=3;
-- col of one table is the name of the other table
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(t_mutil_t2 int);
create table t_mutil_t2(col1 int);
insert into t_mutil_t1 values(1);
insert into t_mutil_t2 values(2);
update t_mutil_t1,t_mutil_t2 set t_mutil_t1.t_mutil_t2=3,t_mutil_t2.col1=4;
select * from t_mutil_t1;
select * from t_mutil_t2;
update t_mutil_t1,t_mutil_t2 set t_mutil_t2.col1=4,t_mutil_t1.t_mutil_t2=3;
-- duplicate tablename
update t_mutil_t1,t_mutil_t1 set t_mutil_t1.t_mutil_t2 = 5; -- error
-- duplicate colname
update t_mutil_t1 set t_mutil_t1.t_mutil_t2=4,t_mutil_t1.t_mutil_t2=5; -- error
update t_mutil_t1,t_mutil_t2 set t_mutil_t1.t_mutil_t2=4,t_mutil_t1.t_mutil_t2=5; -- error
-- ARupdate trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl1 SET des1_id3 = NEW.src1_id3 WHERE des1_id1=OLD.src1_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl2 SET des2_id3 = NEW.src2_id3 WHERE des2_id1=OLD.src2_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
AFTER UPDATE ON test_trigger_src_tbl1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
AFTER UPDATE ON test_trigger_src_tbl2 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
UPDATE test_trigger_src_tbl1 a,test_trigger_src_tbl2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 
-- BRupdate trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl1 SET des1_id3 = 800;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl2 SET des2_id3 = 800;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
BEFORE UPDATE ON test_trigger_src_tbl1 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
BEFORE UPDATE ON test_trigger_src_tbl2 
FOR EACH ROW
EXECUTE PROCEDURE tri_update_func2();
UPDATE test_trigger_src_tbl1 a,test_trigger_src_tbl2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 

-- BSupdate trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl1 SET des1_id3 = 800;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl2 SET des2_id3 = 900;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
BEFORE UPDATE ON test_trigger_src_tbl1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
BEFORE UPDATE ON test_trigger_src_tbl2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func2();
UPDATE test_trigger_src_tbl1 a,test_trigger_src_tbl2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 
-- ASupdate trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_update_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl1 SET des1_id3 = 800;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_update_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
UPDATE test_trigger_des_tbl2 SET des2_id3 = 900;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER update_ar_trigger1
AFTER UPDATE ON test_trigger_src_tbl1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func1();

CREATE TRIGGER update_ar_trigger2
AFTER UPDATE ON test_trigger_src_tbl2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_update_func2();
UPDATE test_trigger_src_tbl1 a,test_trigger_src_tbl2 b SET a.src1_id3=700,b.src2_id1=600 WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 


drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
drop table if exists t_t_mutil_t3;
create table t_t_mutil_t1(col1 int,col2 int);
create table t_t_mutil_t2(col1 int,col2 int,col3 int);
create table t_t_mutil_t3(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,1),(1,1);
insert into t_t_mutil_t2 values(1,1),(1,2);
insert into t_t_mutil_t3 values(1,1),(1,3);
begin;
update t_t_mutil_t1 a,t_t_mutil_t2 b,t_t_mutil_t3 c set b.col2=5,a.col2=4 where a.col1=b.col1 and b.col1=c.col1;
rollback;

begin;
update t_t_mutil_t1 t1,t_t_mutil_t2 t2 set t2.col2 = 3, t1.col1 = 2;
select * from t_t_mutil_t1;
rollback;
begin;
update t_t_mutil_t1 t1, t_t_mutil_t1 t2, t_t_mutil_t1 t3 set t2.col2 = 3, t3.col1 = 2;
rollback;
begin;
update t_t_mutil_t2 t1, t_t_mutil_t2 t2, t_t_mutil_t2 t3 set t1.col1 = 3, t3.col2 = 2, t2.col3 = 3;
rollback;
begin;
update t_t_mutil_t1 t1 inner join t_t_mutil_t2 t2 on t1.col1=t2.col1 inner join t_t_mutil_t3 t3 on t1.col1=t3.col1 set t1.col2=3,t2.col2=4;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;

rollback;

-- view
create view multiview1 as select * from t_t_mutil_t1;
create view multiview2 as select * from t_t_mutil_t2;
create view multiview3 as select * from t_t_mutil_t3;
begin;
update multiview1 a,multiview2 b,multiview3 c set a.col2 = 6, b.col2 = 7,c.col2 =8 where a.col1 = b.col1 and a.col1 = c.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
rollback;

begin;
update t_t_mutil_t1 a,multiview2 b,t_t_mutil_t3 c set a.col2 = 4, b.col2 = 5,c.col2 = 6 where a.col1 = b.col1 and a.col1 = c.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
rollback;

begin;
alter table t_t_mutil_t1 alter column col2 set default 999;
update t_t_mutil_t1 a,multiview2 b,t_t_mutil_t3 c set a.col2 = default, b.col2 = 5,c.col2 = 6 where a.col1 = b.col1 and a.col1 = c.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
rollback;
-- left join
begin;
update multiview1 a left join multiview2 b on a.col1=b.col1 set a.col2=7,b.col2=8;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
rollback;
-- update same relation
begin;
update t_t_mutil_t1 a,multiview2 b,t_t_mutil_t2 c,t_t_mutil_t3 d set a.col2 = 6, b.col2 = 9, c.col3 = 10, d.col2 = 6 where a.col1 = b.col1 and a.col1 = d.col1;
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
rollback;
drop view multiview1;
drop view multiview2;
drop view multiview3;
-- multi update with join
drop table if exists t1;
drop table if exists t2;
CREATE TABLE t1 (f1 int);
CREATE TABLE t2 (f1 int);
INSERT INTO t2  VALUES (1);
CREATE VIEW v1 AS SELECT * FROM t2;
UPDATE t2 AS A NATURAL JOIN v1 B SET B.f1 = 2;
select * from v1;
drop table t1;
drop view v1;
drop table t2;
--bug1323,1324
create schema sch1;
CREATE TABLE sch1.t_t_mutil_t1(col1 int, col2 int);
CREATE TABLE sch1.t_t_mutil_t2(col1 int, col2 int);
insert into sch1.t_t_mutil_t1 values(1,2),(2,2);
insert into sch1.t_t_mutil_t2 values(2,2),(3,3);
--single table
update sch1.t_t_mutil_t1 set col1 = 4;
update sch1.t_t_mutil_t1 set t_t_mutil_t1.col1 = 5;
update sch1.t_t_mutil_t1 set sch1.t_t_mutil_t1.col1 = 6;
update sch1.t_t_mutil_t1 as a set col1 = 7;
update sch1.t_t_mutil_t1 as a set t_t_mutil_t1.col1 = 7;--error
update sch1.t_t_mutil_t1 as a set sch1.t_t_mutil_t1.col1 = 7;--error
select * from sch1.t_t_mutil_t1;
--multi tables
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set col1 = 7;--error
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set t_t_mutil_t1.col1 = 8;
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set sch1.t_t_mutil_t1.col1 = 9;
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set t_t_mutil_t1.col1 = 10,t_t_mutil_t2.col1 = 4;
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set sch1.t_t_mutil_t1.col1 = 11,t_t_mutil_t2.col1 = 5;
update sch1.t_t_mutil_t1 ,sch1.t_t_mutil_t2 set sch1.t_t_mutil_t1.col1 = 12,sch1.t_t_mutil_t2.col1 = 6;
update sch1.t_t_mutil_t1 as a ,sch1.t_t_mutil_t2 as b set t_t_mutil_t1.col1 = 13;--error
update sch1.t_t_mutil_t1 as a ,sch1.t_t_mutil_t2 as b set sch1.t_t_mutil_t1.col1 = 13;--error
update sch1.t_t_mutil_t1 as a ,sch1.t_t_mutil_t2 as b set a.col1 = 13, t_t_mutil_t1.col1 = 7;--error
update sch1.t_t_mutil_t1 as a ,sch1.t_t_mutil_t2 as b set a.col1 = 13, sch1.t_t_mutil_t1.col1 = 7;--error
update sch1.t_t_mutil_t1 as a ,sch1.t_t_mutil_t2 as b set a.col1 = 13, b.col1 = 7;
select * from sch1.t_t_mutil_t1;
select * from sch1.t_t_mutil_t2;
drop schema sch1 cascade;

-- begin -bug#z8393
-- MySQL-compatible duplicate assignment rewrite in UPDATE targetlists.
-- single-table UPDATE
CREATE TABLE mys_update_chain_t1 (
    id INT PRIMARY KEY,
    a  INT,
    b  INT
);
CREATE TABLE mys_update_chain_t2 (
    id INT PRIMARY KEY,
    x  INT
);
INSERT INTO mys_update_chain_t1 VALUES (1, 1, 2);
INSERT INTO mys_update_chain_t2 VALUES (1, 10);

UPDATE mys_update_chain_t1
  SET a = a + b, b = 300, a = a + 1
  WHERE id = 1;
SELECT * FROM mys_update_chain_t1 ORDER BY id;

-- multi-table UPDATE, verify rewrite for each updated table independently
UPDATE mys_update_chain_t1, mys_update_chain_t2
  SET mys_update_chain_t1.a = mys_update_chain_t1.a + mys_update_chain_t1.b,
      mys_update_chain_t2.x = mys_update_chain_t2.x + 1,
      mys_update_chain_t1.b = 300,
      mys_update_chain_t1.a = mys_update_chain_t1.a + mys_update_chain_t2.x,
      mys_update_chain_t2.x = mys_update_chain_t2.x + 2
  WHERE mys_update_chain_t1.id = mys_update_chain_t2.id
    AND mys_update_chain_t1.id = 1;
SELECT * FROM mys_update_chain_t1 ORDER BY id;
SELECT * FROM mys_update_chain_t2 ORDER BY id;

DROP TABLE mys_update_chain_t1;
DROP TABLE mys_update_chain_t2;

-- single-table with multiple rows: per-row chain rewrite
CREATE TABLE mys_upd_multi_t (id INT PRIMARY KEY, a INT, b INT);
INSERT INTO mys_upd_multi_t VALUES (1, 1, 2), (2, 10, 20);
UPDATE mys_upd_multi_t SET a = a + b, b = 300, a = a + 1;
SELECT * FROM mys_upd_multi_t ORDER BY id;
DROP TABLE mys_upd_multi_t;

-- multi-table UPDATE, duplicate assignment only on one table
-- (derived from MySQL 5.7 mysql-test/t/multi_update.test).
CREATE TABLE mys_upd_one_t1 (id INT PRIMARY KEY, v INT);
CREATE TABLE mys_upd_one_t2 (id INT PRIMARY KEY, v INT);
INSERT INTO mys_upd_one_t1 VALUES (1, 100), (2, 200);
INSERT INTO mys_upd_one_t2 VALUES (1, 5), (2, 7);
UPDATE mys_upd_one_t1, mys_upd_one_t2
  SET mys_upd_one_t1.v = mys_upd_one_t1.v + 1,
      mys_upd_one_t2.v = mys_upd_one_t2.v * 2,
      mys_upd_one_t1.v = mys_upd_one_t1.v + 100
  WHERE mys_upd_one_t1.id = mys_upd_one_t2.id;
SELECT * FROM mys_upd_one_t1 ORDER BY id;
SELECT * FROM mys_upd_one_t2 ORDER BY id;
DROP TABLE mys_upd_one_t1;
DROP TABLE mys_upd_one_t2;

-- multi-table UPDATE, duplicate assignment on both tables;
-- cross-table reads on the right-hand side still see the OLD row values,
-- matching MySQL's behavior (cf. mysql-test/t/multi_update.test scenarios
-- "UPDATE t1,t2 SET t1.d=t2.d,t2.d=30").
CREATE TABLE mys_upd_both_t1 (id INT PRIMARY KEY, v INT);
CREATE TABLE mys_upd_both_t2 (id INT PRIMARY KEY, v INT);
INSERT INTO mys_upd_both_t1 VALUES (1, 100);
INSERT INTO mys_upd_both_t2 VALUES (1, 5);
UPDATE mys_upd_both_t1, mys_upd_both_t2
  SET mys_upd_both_t1.v = mys_upd_both_t1.v + mys_upd_both_t2.v,
      mys_upd_both_t2.v = mys_upd_both_t2.v + 1,
      mys_upd_both_t1.v = mys_upd_both_t1.v + 2,
      mys_upd_both_t2.v = mys_upd_both_t2.v * 10
  WHERE mys_upd_both_t1.id = mys_upd_both_t2.id;
SELECT * FROM mys_upd_both_t1 ORDER BY id;
SELECT * FROM mys_upd_both_t2 ORDER BY id;
DROP TABLE mys_upd_both_t1;
DROP TABLE mys_upd_both_t2;

-- cross-column dependency with duplicate target column assignments:
-- apply left-to-right substitution environment across target columns.
CREATE TABLE mys_upd_cross_dup (a INT, b INT);
INSERT INTO mys_upd_cross_dup VALUES (10, 20);
UPDATE mys_upd_cross_dup
  SET a = a + b, b = 300, a = a + b, a = a + b;
SELECT * FROM mys_upd_cross_dup;
DROP TABLE mys_upd_cross_dup;

-- multi-table variant: keep per-table rewrite while preserving
-- left-to-right substitution for duplicate assignments on one target table.
CREATE TABLE mys_upd_cross_dup_mt_t1 (id INT PRIMARY KEY, a INT, b INT);
CREATE TABLE mys_upd_cross_dup_mt_t2 (id INT PRIMARY KEY, x INT);
INSERT INTO mys_upd_cross_dup_mt_t1 VALUES (1, 10, 20);
INSERT INTO mys_upd_cross_dup_mt_t2 VALUES (1, 5);
UPDATE mys_upd_cross_dup_mt_t1, mys_upd_cross_dup_mt_t2
  SET mys_upd_cross_dup_mt_t1.a = mys_upd_cross_dup_mt_t1.a + mys_upd_cross_dup_mt_t1.b,
      mys_upd_cross_dup_mt_t2.x = mys_upd_cross_dup_mt_t2.x + 1,
      mys_upd_cross_dup_mt_t1.b = 300,
      mys_upd_cross_dup_mt_t1.a = mys_upd_cross_dup_mt_t1.a + mys_upd_cross_dup_mt_t1.b,
      mys_upd_cross_dup_mt_t1.a = mys_upd_cross_dup_mt_t1.a + mys_upd_cross_dup_mt_t1.b
  WHERE mys_upd_cross_dup_mt_t1.id = mys_upd_cross_dup_mt_t2.id;
SELECT * FROM mys_upd_cross_dup_mt_t1 ORDER BY id;
SELECT * FROM mys_upd_cross_dup_mt_t2 ORDER BY id;
DROP TABLE mys_upd_cross_dup_mt_t1;
DROP TABLE mys_upd_cross_dup_mt_t2;

-- multi-table stress case: duplicate assignment chains on both updated tables.
CREATE TABLE mys_upd_both_dup_t1 (id INT PRIMARY KEY, a INT, b INT);
CREATE TABLE mys_upd_both_dup_t2 (id INT PRIMARY KEY, x INT, y INT);
INSERT INTO mys_upd_both_dup_t1 VALUES (1, 10, 20);
INSERT INTO mys_upd_both_dup_t2 VALUES (1, 1, 2);
UPDATE mys_upd_both_dup_t1, mys_upd_both_dup_t2
  SET mys_upd_both_dup_t1.a = mys_upd_both_dup_t1.a + mys_upd_both_dup_t1.b,
      mys_upd_both_dup_t2.x = mys_upd_both_dup_t2.x + mys_upd_both_dup_t2.y,
      mys_upd_both_dup_t1.b = 300,
      mys_upd_both_dup_t2.y = 400,
      mys_upd_both_dup_t1.a = mys_upd_both_dup_t1.a + mys_upd_both_dup_t1.b,
      mys_upd_both_dup_t2.x = mys_upd_both_dup_t2.x + mys_upd_both_dup_t2.y
  WHERE mys_upd_both_dup_t1.id = mys_upd_both_dup_t2.id;
SELECT * FROM mys_upd_both_dup_t1 ORDER BY id;
SELECT * FROM mys_upd_both_dup_t2 ORDER BY id;
DROP TABLE mys_upd_both_dup_t1;
DROP TABLE mys_upd_both_dup_t2;

-- type promotion in expression plus duplicate assignment rewrite:
-- keep rewritten substitutions type-safe via coercion to original Var type.
CREATE TABLE mys_upd_type_guard (a INT, b BIGINT);
INSERT INTO mys_upd_type_guard VALUES (10, 20);
UPDATE mys_upd_type_guard
  SET a = a + CAST(b AS BIGINT),
      b = CAST(30 AS BIGINT),
      a = a + b;
SELECT * FROM mys_upd_type_guard;
DROP TABLE mys_upd_type_guard;

-- cross-column dependency without duplicate target columns: every right-hand
-- side reads the OLD row, just like MySQL when the target column list contains
-- no repeats (cf. MySQL 5.7 mysql-test/t/update.test: "UPDATE t1 SET b=a, a=...").
CREATE TABLE mys_upd_cross (a TEXT, b TEXT);
INSERT INTO mys_upd_cross VALUES ('start trail', '');
UPDATE mys_upd_cross SET b = a, a = 'inject';
SELECT a, b FROM mys_upd_cross;
DROP TABLE mys_upd_cross;

create table update_t(a int, b int);
insert into update_t values (1, 3);
select * from update_t;
update update_t set a = 2, b = 2, a = a + b;
-- 4, 2
select * from update_t;

update update_t set a = 4, b = 3, a = a + 1;
-- 5, 3
select * from update_t;

update update_t set b = a + 1, a = a + 1, a = b;
-- 6, 6
select * from update_t;
drop table update_t;
-- end - bug#z8393
