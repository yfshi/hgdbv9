--single table
drop table if exists t,t1,t2;
create table t(id int,name varchar(10));
create table t1(id int,name varchar(10));
create table t2(id int,name varchar(10));
insert into t values(1,'aa'),(2,'bb');
insert into t1 values(1,'aa'),(2,'bb'); 
insert into t2 values(1,'aa'),(2,'bb'); 
select * from t;
delete from t as aa;--error
delete from t aa;--error
begin;
delete from t order by id limit 1;
select * from t;
delete from t where id = 2;
select * from t;
rollback;

--BUG#Z4581
--error
delete t;--error

--multi tables
delete from t using t2;--error
begin;
delete from t using t,t2;
select * from t;
rollback;

begin;
DELETE t, t1 FROM t INNER JOIN t1 INNER JOIN t2
WHERE t.id=t1.id AND t1.id=t2.id;
select * from t;
select * from t1;
select * from t2;
rollback;

begin;
DELETE t.*, t1 FROM t , t1 INNER JOIN t2
WHERE t.id=t1.id AND t1.id=t2.id;
select * from t;
select * from t1;
select * from t2;
rollback;

begin;
DELETE FROM t, t1 using t INNER JOIN t1 INNER JOIN t2
WHERE t.id=t1.id AND t1.id=t2.id;
select * from t;
select * from t1;
select * from t2;
rollback;

begin;
DELETE t1 FROM t1 left join t2 on t1.id=t2.id 
where t2.id is not NULL ;
select * from t1;
select * from t2;
rollback;

begin;
DELETE a1, a2 FROM t1 AS a1 INNER JOIN t2 AS a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

begin;
DELETE FROM a1, a2 USING t1 AS a1 INNER JOIN t2 AS a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

begin;
DELETE FROM a1, a2 USING t1 AS a1 CROSS JOIN t2 AS a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

begin;
DELETE FROM a1.*, a2 USING t1 a1,t2 a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

begin;
DELETE FROM a1.*, a2.* USING t1 a1,t2 a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

begin;
DELETE a1.*, a2.* FROM t1 a1,t2 a2
WHERE a1.id=a2.id;
select * from t1;
select * from t2;
rollback;

--error alias rules
DELETE t1 AS a1, t2 AS a2 FROM t1 INNER JOIN t2
WHERE a1.id=a2.id;

DELETE FROM t1 AS a1, t2 AS a2 USING t1 INNER JOIN t2
WHERE a1.id=a2.id;

--pbe
create table tt(id int primary key,num int);
insert tt  value(2, 3),(5, 6);
PREPARE del (int) AS
	delete from tt where id = $1;
EXECUTE del(5);
select * from tt;
drop table tt;

PREPARE del1  AS
	DELETE a1.*, a2.* FROM t1 a1,t2 a2
	WHERE a1.id=a2.id;
begin;
execute del1;
end;

--single table indexhint
--error
create index idk on t1(id);
create index idk2 on t2(id);
delete from t1 use index(idk);--error
--multi tables indexhint
explain (costs false) delete t1 from t1 use index(idk);
explain (costs false) delete from t1 using t1  use index(idk);
explain (costs false) delete t1,t2 from t1 use index(idk),t2 use index (idk2) where t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index(idk) inner join  t2 use index (idk2) on  t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index(idk) left join  t2 use index (idk2) on  t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index(idk) right join  t2 use index (idk2) on  t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 ignore index(idk) left join  t2 use index (idk2) on  t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index(idk) cross join  t2 use index (idk2) on  t1.id = t2.id;--error
explain (costs false) delete t1,t2 from t1 use index for join (idk),t2 use index (idk2) where t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index for order by (idk),t2 use index (idk2) where t1.id = t2.id;
explain (costs false) delete t1,t2 from t1 use index for group by (idk),t2 use index (idk2) where t1.id = t2.id;
drop table if exists t,t1,t2;


create table t1(a int);
create table t2(b int);
insert into t1 values(1),(2);
insert into t2 values(3),(4);
create view v1 as select * from t1;

--table and view as same
begin;
delete from t1,v1 using t1,v1;
select * from v1;
rollback;
--table and view not same
begin;
delete from t2,v1 using t2,v1;
select * from t2;
select * from v1;
rollback;
create view v2 as select * from t1,t2;
delete from v2;--error
delete v2,t1 from v2,t1;--error
delete quick from t1.*;--error
delete quick from t1.* using t2;--error
delete ignore from t1.*;--error
delete ignore from t1 using t2;--error
delete ignore from t1.* using t2;--error
delete LOW_PRIORITY t1 from t2;--error
delete t1 from t2;--error
delete t1.* from t2;--error
delete from t1,t2;--error
delete from t1 using t2;--error
delete from t1.* using t2;--error
delete t1,t2 from t1;--error
delete t1.*,t2 from t1;--error
delete from t1,t2 using t1;--error
delete from t1.*,t2.* using t1;--error
delete t1.* from t1,t2 using t2;--error
drop view v2;
create view v2 as select * from t2;
begin;
delete from v1,v2 using v1,v2;
select * from v1;
select * from v2;
rollback;

begin;
delete v1,v2.* from v1 inner join v2;
select * from v1;
select * from v2;
rollback;


delete t1 from t1,t2;
delete from t2.* using t1,t2;
select * from t1;
select * from t2;
drop view v1,v2;
drop table t1,t2;

drop table if exists t1;
drop table if exists t2;
drop table if exists t3;
drop table if exists t4;

create table t1 (a int NOT NULL, b int);
create table t2 (a int NOT NULL, b int);
create table t3 (a int NOT NULL, b int);
create table t4 (a int NOT NULL, b int);
INSERT INTO t1 values (1, 11),(2, 22),(3, 33);
INSERT INTO t2 values (2, 22),(2, 33),(3, 44);
INSERT INTO t3 values (1, 22),(3, 33),(2, 44);
INSERT INTO t4 values (1, 11),(3, 22),(3, 33);

begin;
DELETE t1.*,t2,t3 FROM t2 left join t3 on t2.b =t3.b left join t1 on  t1.a =t2.a left join t4 on t2.b =t4.b ;
select * from t1;
select * from t2;
select * from t3;
rollback;

begin;
DELETE t1.*,t2,t3 FROM t2 join t3 on t2.b =t3.b join t1 on t1.a =t2.a join t4 on t2.b =t4.b ;
select * from t1;
select * from t2;
select * from t3;
rollback;

begin;
DELETE t4,t2,t3 FROM t2 join t3 on t2.b =t3.b join t1 on t1.a =t2.a join t4 on t2.b =t4.b ;
select * from t2;
select * from t3;
select * from t4;
rollback;

begin;
DELETE t1.*,t2,t3,t4.* FROM t2 join t3 on t2.b =t3.b join t1 on t1.a =t2.a join t4 on t2.b =t4.b ;
select * from t1;
select * from t2;
select * from t3;
select * from t4;
rollback;


drop table t1,t2,t3,t4;

drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
drop table if exists t_t_mutil_t3;
create table t_t_mutil_t1(col1 int,col2 int);
create table t_t_mutil_t2(col1 int,col2 int);
create table t_t_mutil_t3(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,1),(1,1);
insert into t_t_mutil_t2 values(1,1),(1,2);
insert into t_t_mutil_t3 values(1,1),(1,3);
-- subselect
begin;
delete t_t_mutil_t1,t_t_mutil_t2 from (select * from t_t_mutil_t3) as a;--error
rollback;
begin;
delete from t_t_mutil_t1 a,t_t_mutil_t2 b,t_t_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;--error
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
select * from t_t_mutil_t3;
rollback;
-- delete xx from xxx;
begin;
delete t_t_mutil_t1 a from t_t_mutil_t2 b,t_t_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;--error
rollback;
begin;
delete a from t_t_mutil_t1 a,t_t_mutil_t2 b,t_t_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;
rollback;
begin;
delete t_t_mutil_t1 a,t_t_mutil_t2 b from t_t_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;--error
rollback;
begin;
delete a,b from t_t_mutil_t1 a,t_t_mutil_t2 b,t_t_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;
rollback;
begin;
delete a from t_t_mutil_t1 a left join t_t_mutil_t2 b on a.col2=b.col2;
rollback;
delete a from t_t_mutil_t1 a left join t_t_mutil_t2 b on a.col2=b.col2 limit 1; --error
delete a from t_t_mutil_t1 a left join t_t_mutil_t2 b on a.col2=b.col2 order by a.col2; --error
delete a from t_t_mutil_t1 a left join t_t_mutil_t2 b on a.col2=b.col2 returning *; --error
delete t_t_mutil_t1 a from t_t_mutil_t1 a left join t_t_mutil_t2 b on a.col2=b.col2 limit 1; --error
-- condition is false
delete from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col1 = 1 and a.col1 = 2;--error
--alias erro
delete from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;--error
-- left join
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int,col2 int);
create table t_mutil_t2(col1 int,col2 int);
insert into t_mutil_t1 values(1,2),(1,4);
insert into t_mutil_t2 values(2,3),(3,3);
delete from t_mutil_t1,t_mutil_t2 using t_mutil_t2 left join t_mutil_t1 on t_mutil_t1.col2=t_mutil_t2.col1;
select * from t_mutil_t1;
select * from t_mutil_t2;
-- procedure
CREATE OR REPLACE PROCEDURE proc_mutil
(
c1 int,
c2 int
)
as $$
BEGIN
	delete from a,b using t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=c1 and b.col2=c2;
END;
$$ language plmysql;
begin;
CALL proc_mutil(1,2);
select * from t_t_mutil_t1;
select * from t_t_mutil_t2;
rollback;

--schema.tb_name
CREATE SCHEMA multiblock2;
drop table if exists multiblock2.t_d_mutil_bc_1;
drop table if exists multiblock2.t_d_mutil_bc_2;
CREATE TABLE multiblock2.t_d_mutil_bc_1(col1 int, col2 int);
CREATE TABLE multiblock2.t_d_mutil_bc_2(col1 int, col2 int);
insert into multiblock2.t_d_mutil_bc_1 values(1,2),(2,2);
insert into multiblock2.t_d_mutil_bc_2 values(2,2),(3,3);
delete a.*,b.* from multiblock2.t_d_mutil_bc_1 a,multiblock2.t_d_mutil_bc_2 b where a.col2=b.col2;
select * from multiblock2.t_d_mutil_bc_1;
select * from multiblock2.t_d_mutil_bc_2;
-- subquery
delete a.*,b.* from t_t_mutil_t1 a,(select * from t_t_mutil_t2) b where a.col1=b.col1; --error
--mateview
CREATE MATERIALIZED VIEW mate_multiview1 as select * from t_t_mutil_t1;
CREATE MATERIALIZED VIEW mate_multiview2 as select * from t_t_mutil_t2;
delete a,b,c from t_t_mutil_t1 a,mate_multiview1 b,mate_multiview2 c ;
drop MATERIALIZED VIEW mate_multiview1;
drop MATERIALIZED VIEW mate_multiview2;
--explain plan
explain delete a,b from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;
explain(verbose) delete a,b from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;
explain(format xml) delete a,b from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;
explain(format json) delete a,b from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;
explain(format yaml) delete a.*, b.* from t_t_mutil_t1 a,t_t_mutil_t2 b where a.col2=b.col2;
-- temp table
drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
create temporary table t_t_mutil_t1(col1 int, col2 int);
create temporary table t_t_mutil_t2(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,2),(2,2);
insert into t_t_mutil_t2 values(2,2),(3,3);
delete a.*,b.* from  t_t_mutil_t1 a,t_t_mutil_t2 b where a.col1=b.col1;
select * from t_t_mutil_t1;--1,2
select * from t_t_mutil_t2;--3,3
-- order by or limit
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int,col2 int);
create table t_mutil_t2(col1 int,col2 int);
insert into t_mutil_t1 values(1,2),(1,2),(3,3);
insert into t_mutil_t2 values(2,3),(2,3),(1,3);
delete from a,b using t_mutil_t1 a,t_mutil_t2 b where a.col1=b.col2 order by a.col2;  --error
delete from a,b using t_mutil_t1 a,t_mutil_t2 b where a.col1=b.col2 limit 1;    --error
select * from t_mutil_t1;
select * from t_mutil_t2;
-- returning
delete a,b from t_mutil_t1 a,t_mutil_t2 b where a.col1=b.col2 returning *; --error
-- left join
begin;
delete from a,b using t_mutil_t1 a left join t_mutil_t2 b on a.col1=b.col1;
select * from t_mutil_t1;--empty
select * from t_mutil_t2;--2,3--2,3
rollback;
--
drop table if exists t_u_mutil_t1;
drop table if exists t_u_mutil_t2;
drop table if exists t_u_mutil_t3;
create table t_u_mutil_t1(col1 int, col2 int);
create table t_u_mutil_t2(col1 int, col2 int);
create table t_u_mutil_t3(col1 int, col2 int);
insert into t_u_mutil_t1 values(1,2);
insert into t_u_mutil_t2 values(1,2);
insert into t_u_mutil_t3 values(1,2);
delete a,b,c from t_u_mutil_t1 a,t_u_mutil_t2 b,t_u_mutil_t3 c where a.col2=b.col2 and b.col2=c.col2;
select * from t_u_mutil_t1;--empty
select * from t_u_mutil_t2;--empty
select * from t_u_mutil_t3;--empty
-- has index
drop table if exists t_mutil_t1;
drop table if exists t_mutil_t2;
create table t_mutil_t1(col1 int);
create table t_mutil_t2(col1 int, col2 int);
create index idx1 on t_mutil_t1(col1);
create index idx2 on t_mutil_t2(col1);
insert into t_mutil_t1 values(1),(2);
insert into t_mutil_t2 values(2,2),(3,3);
begin;
delete a,b from t_mutil_t1 a,t_mutil_t2 b where a.col1=b.col1;
select * from t_mutil_t1;--1
select * from t_mutil_t2;--3,3
rollback;
-- ARdelete trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl1 WHERE des1_id1=OLD.src1_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl2 WHERE des2_id1=OLD.src2_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
AFTER delete ON test_trigger_src_tbl1 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
AFTER delete ON test_trigger_src_tbl2 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func2();
delete a,b from test_trigger_src_tbl1 a,test_trigger_src_tbl2 b  WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;--empty
SELECT * FROM test_trigger_src_tbl2;--empty
SELECT * FROM test_trigger_des_tbl1;--empty
SELECT * FROM test_trigger_des_tbl2;--empty
-- BRdelete trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl1;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl2;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
BEFORE delete ON test_trigger_src_tbl1 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
BEFORE delete ON test_trigger_src_tbl2 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func2();
delete a.*,b.* from test_trigger_src_tbl1 a,test_trigger_src_tbl2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 
-- BSdelete trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl2;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER delete_bs_trigger1
BEFORE delete ON test_trigger_src_tbl1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_bs_trigger2
BEFORE delete ON test_trigger_src_tbl2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func2();
delete a.*,b.* from test_trigger_src_tbl1 a,test_trigger_src_tbl2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 
-- ASdelete trig
drop table if exists test_trigger_src_tbl1;
drop table if exists test_trigger_src_tbl2;
drop table if exists test_trigger_des_tbl1;
drop table if exists test_trigger_des_tbl2;
CREATE TABLE test_trigger_src_tbl1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE test_trigger_src_tbl2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE test_trigger_des_tbl1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE test_trigger_des_tbl2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from test_trigger_des_tbl2;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO test_trigger_src_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_src_tbl2 VALUES(300,400,500);
INSERT INTO test_trigger_des_tbl1 VALUES(100,200,300);
INSERT INTO test_trigger_des_tbl2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
AFTER delete ON test_trigger_src_tbl1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
AFTER delete ON test_trigger_src_tbl2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func2();
delete a,b from test_trigger_src_tbl1 a,test_trigger_src_tbl2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM test_trigger_src_tbl1;
SELECT * FROM test_trigger_src_tbl2;
SELECT * FROM test_trigger_des_tbl1; 
SELECT * FROM test_trigger_des_tbl2; 
drop function tri_delete_func1;
drop function tri_delete_func2;
-- delete alias
drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
create table t_t_mutil_t1(col1 int,col2 int);
create table t_t_mutil_t2(col1 int,col2 int);
delete from t_t_mutil_t1 a,t_t_mutil_t2 b using t_t_mutil_t1 a,t_t_mutil_t2 b where a.col1=b.col1;--error
delete from a,b using t_t_mutil_t1 a,t_t_mutil_t2 b where a.col1=b.col1;
delete from t_t_mutil_t1 a,t_t_mutil_t2 b using a,b where a.col1=b.col1;--error
delete from t_t_mutil_t1 a where a.col1=1;--error
delete from t_t_mutil_t1 a using a where a.col1=1;--error
delete from a using t_t_mutil_t1 a where a.col1=1;
delete from t_t_mutil_t1 a using t_t_mutil_t2 a where a.col1=1;--error
delete from a using t_t_mutil_t1 a where a.col1=1;
delete from t_t_mutil_t1,t_t_mutil_t1 using t_t_mutil_t1,t_t_mutil_t1 where t_t_mutil_t1.col1=1;--error
delete from t_t_mutil_t1 using t_t_mutil_t1,t_t_mutil_t1 where t_t_mutil_t1.col1=1;--error
delete from t_t_mutil_t1 a,t_t_mutil_t1 a where a.col1=1;--error
delete from t_t_mutil_t1 a,t_t_mutil_t1 b where a.col1=1;--error
-- view
drop table if exists t_t_mutil_t1;
drop table if exists t_t_mutil_t2;
drop table if exists t_t_mutil_t3;
create table t_t_mutil_t1(col1 int,col2 int);
create table t_t_mutil_t2(col1 int,col2 int,col3 int);
create table t_t_mutil_t3(col1 int,col2 int);
insert into t_t_mutil_t1 values(1,1),(1,1);
insert into t_t_mutil_t2(col1,col2) values(1,1),(1,2);
insert into t_t_mutil_t3 values(1,1),(1,3);
drop view if exists multiview1,multiview2,multiview3;
create view multiview1 as select * from t_t_mutil_t1;
create view multiview2 as select * from t_t_mutil_t2;
create view multiview3 as select * from t_t_mutil_t3;
delete a,b,c from multiview1 a,multiview2 b,multiview3 c where a.col1 = b.col1 and b.col2 = c.col2;
select * from t_t_mutil_t1;--empty
select * from t_t_mutil_t2;--1,2,NULL
select * from t_t_mutil_t3;--1,3
-- left join
insert into t_t_mutil_t1 values(1,1),(1,1);
insert into t_t_mutil_t2(col1,col2) values(1,1),(1,2);
delete from multiview1,t_t_mutil_t2 using t_t_mutil_t2 left join multiview1 on multiview1.col2=t_t_mutil_t2.col1;
select * from t_t_mutil_t1;--empty
select * from t_t_mutil_t2;--empty
drop view multiview1;
drop view multiview2;
drop view multiview3;
drop table t_mutil_t1,t_mutil_t2;
drop table t_t_mutil_t1,t_t_mutil_t2,t_t_mutil_t3;
drop table t_u_mutil_t1,t_u_mutil_t2,t_u_mutil_t3;
drop table test_trigger_des_tbl1,test_trigger_des_tbl2,test_trigger_src_tbl1,test_trigger_src_tbl2;

--BUG#Z4241
CREATE TABLE person_3
(

id SERIAL NOT NULL,

name VARCHAR NOT NULL,

begin_time TIMESTAMP,

end_time TIMESTAMP

) PARTITION BY LIST (name);

create table person_3_1

partition of person_3 for values in ('张三','李四');

create table person_3_2

partition of person_3 for values in ('王五','赵六');

insert into person_3 (id, "name", begin_time, end_time)

values(1,'张三','2024-06-23 01:00:00','2024-06-23 01:00:00'),

(2,'李四','2024-06-24 01:00:00','2024-06-24 01:00:00'),

(1,'王五','2024-06-23 01:00:00','2024-06-23 01:00:00'),

(4,'赵六','2024-06-23 01:00:00','2024-06-23 01:00:00');

delete aa,bb from person_3 partition(person_3_1) aa ,person_3 partition (person_3_2) bb where aa.id = bb.id ;

select * from person_3_1;

select * from person_3_2;

drop table person_3;

-- OA-2505: test case
CREATE TABLE students(
    id INT PRIMARY KEY AUTO_INCREMENT,
    parent_id INT,
    name VARCHAR(50)
) ENGINE=InnoDB;

CREATE TABLE student_courses (
    student_id INT,
    score INT,
    PRIMARY KEY (student_id),
    FOREIGN KEY (student_id) REFERENCES students(id) ON DELETE CASCADE
) ENGINE=InnoDB;

INSERT INTO students (id,parent_id,name) VALUES (1,1,'张'),(2,1,'李');
INSERT INTO student_courses (student_id,score) VALUES (1,1),(2,1);
SELECT * FROM students;
SELECT * FROM student_courses;
DELETE FROM students where id=2;
SELECT * FROM students;
SELECT * FROM student_courses;
DROP TABLE student_courses;
DROP TABLE students;

-- bug#z6538: test case
create table test10pk(x varchar(10) primary key);
insert into test10pk values('abc');
create table test10fk (x varchar(10)  references test10pk (x) on update cascade on delete cascade);
insert into test10fk values('abc');
select * from test10pk;
select * from test10fk;
delete from test10pk where x = 'abc';
select * from test10pk;
select * from test10fk;
drop table test10fk;
drop table test10pk;
