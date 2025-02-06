--单表
drop table if exists T,T1,T2;
create table T(id int,name varchar(10));
create table T1(id int,name varchar(10));
create table T2(id int,name varchar(10));
insert into T values(1,'aa'),(2,'bb');
insert into T1 values(1,'aa'),(2,'bb'); 
insert into T2 values(1,'aa'),(2,'bb'); 
select * from T;
delete from T as AA;--error
delete from T AA;--error
begin;
delete from T order by id limit 1;
select * from T;
delete from T where id = 2;
select * from T;
rollback;

--多表
delete from T using T2;--error
begin;
delete from T using T,T2;
select * from T;
rollback;

begin;
DELETE T, T1 FROM T INNER JOIN T1 INNER JOIN T2
WHERE T.id=T1.id AND T1.id=T2.id;
select * from T;
select * from T1;
select * from T2;
rollback;

begin;
DELETE T.*, T1 FROM T , T1 INNER JOIN T2
WHERE T.id=T1.id AND T1.id=T2.id;
select * from T;
select * from T1;
select * from T2;
rollback;

begin;
DELETE FROM T, T1 using T INNER JOIN T1 INNER JOIN T2
WHERE T.id=T1.id AND T1.id=T2.id;
select * from T;
select * from T1;
select * from T2;
rollback;

begin;
DELETE T1 FROM T1 left join T2 on T1.id=T2.id 
where T2.id is not NULL ;
select * from T1;
select * from T2;
rollback;

begin;
DELETE A1, A2 FROM T1 AS A1 INNER JOIN T2 AS A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

begin;
DELETE FROM A1, A2 USING T1 AS A1 INNER JOIN T2 AS A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

begin;
DELETE FROM A1, A2 USING T1 AS A1 CROSS JOIN T2 AS A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

begin;
DELETE FROM A1.*, A2 USING T1 A1,T2 A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

begin;
DELETE FROM A1.*, A2.* USING T1 A1,T2 A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

begin;
DELETE A1.*, A2.* FROM T1 A1,T2 A2
WHERE A1.id=A2.id;
select * from T1;
select * from T2;
rollback;

--error 别名的规则
DELETE T1 AS A1, T2 AS A2 FROM T1 INNER JOIN T2
WHERE A1.id=A2.id;

DELETE FROM T1 AS A1, T2 AS A2 USING T1 INNER JOIN T2
WHERE A1.id=A2.id;

--pbe
create table tt(id int primary key,num int);
insert tt  value(2, 3),(5, 6);
PREPARE del (int) AS
	delete from tt where id = $1;
EXECUTE del(5);
select * from tt;
drop table tt;

PREPARE del1  AS
	DELETE A1.*, A2.* FROM T1 A1,T2 A2
	WHERE A1.id=A2.id;
begin;
execute del1;
end;

--单表使用indexhint
--error
create index idk on T1(id);
create index idk2 on T2(id);
delete from T1 use index(idk);--error
--多表语法使用indexhint
explain (costs false) delete T1 from T1 use index(idk);
explain (costs false) delete from T1 using T1  use index(idk);
explain (costs false) delete T1,T2 from T1 use index(idk),T2 use index (idk2) where T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index(idk) inner join  T2 use index (idk2) on  T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index(idk) left join  T2 use index (idk2) on  T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index(idk) right join  T2 use index (idk2) on  T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 ignore index(idk) left join  T2 use index (idk2) on  T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index(idk) cross join  T2 use index (idk2) on  T1.id = T2.id;--error
explain (costs false) delete T1,T2 from T1 use index for join (idk),T2 use index (idk2) where T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index for order by (idk),T2 use index (idk2) where T1.id = T2.id;
explain (costs false) delete T1,T2 from T1 use index for group by (idk),T2 use index (idk2) where T1.id = T2.id;
drop table if exists T,T1,T2;


create table T1(a int);
create table T2(b int);
insert into T1 values(1),(2);
insert into T2 values(3),(4);
create view V1 as select * from T1;

--表和视图是同一个
begin;
delete from T1,V1 using T1,V1;
select * from V1;
rollback;
--表和视图非同一个
begin;
delete from T2,V1 using T2,V1;
select * from T2;
select * from V1;
rollback;
create view V2 as select * from T1,T2;
delete from V2;--error
delete V2,T1 from V2,T1;--error
delete quick from T1.*;--error
delete quick from T1.* using T2;--error
delete ignore from T1.*;--error
delete ignore from T1 using T2;--error
delete ignore from T1.* using T2;--error
delete LOW_PRIORITY T1 from T2;--error
delete T1 from T2;--error
delete T1.* from T2;--error
delete from T1,T2;--error
delete from T1 using T2;--error
delete from T1.* using T2;--error
delete T1,T2 from T1;--error
delete T1.*,T2 from T1;--error
delete from T1,T2 using T1;--error
delete from T1.*,T2.* using T1;--error
delete T1.* from T1,T2 using T2;--error
drop view V2;
create view V2 as select * from T2;
begin;
delete from V1,V2 using V1,V2;
select * from V1;
select * from V2;
rollback;

begin;
delete V1,V2.* from V1 inner join V2;
select * from V1;
select * from V2;
rollback;


delete T1 from T1,T2;
delete from T2.* using T1,T2;
select * from T1;
select * from T2;
drop view V1,V2;
drop table T1,T2;

drop table if exists T1;
drop table if exists T2;
drop table if exists t3;
drop table if exists t4;

create table T1 (a int NOT NULL, b int);
create table T2 (a int NOT NULL, b int);
create table t3 (a int NOT NULL, b int);
create table t4 (a int NOT NULL, b int);
INSERT INTO T1 values (1, 11),(2, 22),(3, 33);
INSERT INTO T2 values (2, 22),(2, 33),(3, 44);
INSERT INTO t3 values (1, 22),(3, 33),(2, 44);
INSERT INTO t4 values (1, 11),(3, 22),(3, 33);

begin;
DELETE T1.*,T2,t3 FROM T2 left join t3 on T2.b =t3.b left join T1 on  T1.a =T2.a left join t4 on T2.b =t4.b ;
select * from T1;
select * from T2;
select * from t3;
rollback;

begin;
DELETE T1.*,T2,t3 FROM T2 join t3 on T2.b =t3.b join T1 on T1.a =T2.a join t4 on T2.b =t4.b ;
select * from T1;
select * from T2;
select * from t3;
rollback;

begin;
DELETE t4,T2,t3 FROM T2 join t3 on T2.b =t3.b join T1 on T1.a =T2.a join t4 on T2.b =t4.b ;
select * from T2;
select * from t3;
select * from t4;
rollback;

begin;
DELETE T1.*,T2,t3,t4.* FROM T2 join t3 on T2.b =t3.b join T1 on T1.a =T2.a join t4 on T2.b =t4.b ;
select * from T1;
select * from T2;
select * from t3;
select * from t4;
rollback;


drop table T1,T2,t3,t4;

drop table if exists CT_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
drop table if exists T_T_MUTIL_T3;
create table CT_T_MUTIL_T1(col1 int,col2 int);
create table T_T_MUTIL_T2(col1 int,col2 int);
create table T_T_MUTIL_T3(col1 int,col2 int);
insert into CT_T_MUTIL_T1 values(1,1),(1,1);
insert into T_T_MUTIL_T2 values(1,1),(1,2);
insert into T_T_MUTIL_T3 values(1,1),(1,3);
-- subselect
begin;
delete CT_T_MUTIL_T1,T_T_MUTIL_T2 from (select * from T_T_MUTIL_T3) as a;--error
rollback;
begin;
delete from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b,T_T_MUTIL_T3 c where a.col2=b.col2 and b.col2=c.col2;--error
select * from CT_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
select * from T_T_MUTIL_T3;
rollback;
-- delete xx from xxx;
begin;
delete CT_T_MUTIL_T1 A from T_T_MUTIL_T2 B,T_T_MUTIL_T3 C where A.col2=B.col2 and B.col2=C.col2;--error
rollback;
begin;
delete A from CT_T_MUTIL_T1 A,T_T_MUTIL_T2 B,T_T_MUTIL_T3 C where A.col2=B.col2 and B.col2=C.col2;
rollback;
begin;
delete CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b from T_T_MUTIL_T3 c where a.col2=b.col2 and b.col2=c.col2;--error
rollback;
begin;
delete a,b from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b,T_T_MUTIL_T3 c where a.col2=b.col2 and b.col2=c.col2;
rollback;
begin;
delete a from CT_T_MUTIL_T1 a left join T_T_MUTIL_T2 b on a.col2=b.col2;
rollback;
delete a from CT_T_MUTIL_T1 a left join T_T_MUTIL_T2 b on a.col2=b.col2 limit 1; --error
delete a from CT_T_MUTIL_T1 a left join T_T_MUTIL_T2 b on a.col2=b.col2 order by a.col2; --error
delete a from CT_T_MUTIL_T1 a left join T_T_MUTIL_T2 b on a.col2=b.col2 returning *; --error
delete CT_T_MUTIL_T1 a from CT_T_MUTIL_T1 a left join T_T_MUTIL_T2 b on a.col2=b.col2 limit 1; --error
-- condition is false
delete from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col1 = 1 and a.col1 = 2;--error
--alias erro
delete from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;--error
-- left join
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int,col2 int);
create table T_MUTIL_T2(col1 int,col2 int);
insert into T_MUTIL_T1 values(1,2),(1,4);
insert into T_MUTIL_T2 values(2,3),(3,3);
delete from T_MUTIL_T1,T_MUTIL_T2 using T_MUTIL_T2 left join T_MUTIL_T1 on T_MUTIL_T1.col2=T_MUTIL_T2.col1;
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
-- procedure
CREATE OR REPLACE PROCEDURE proc_mutil
(
c1 int,
c2 int
)
as $$
BEGIN
	delete from a,b using CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=c1 and b.col2=c2;
END;
$$ language plmysql;
begin;
CALL proc_mutil(1,2);
select * from CT_T_MUTIL_T1;
select * from T_T_MUTIL_T2;
rollback;

--schema.tb_name
CREATE SCHEMA CMULTIBLOCK2;
drop table if exists CMULTIBLOCK2.T_D_MUTIL_BC_1;
drop table if exists CMULTIBLOCK2.T_D_MUTIL_BC_2;
CREATE TABLE CMULTIBLOCK2.T_D_MUTIL_BC_1(col1 int, col2 int);
CREATE TABLE CMULTIBLOCK2.T_D_MUTIL_BC_2(col1 int, col2 int);
insert into CMULTIBLOCK2.T_D_MUTIL_BC_1 values(1,2),(2,2);
insert into CMULTIBLOCK2.T_D_MUTIL_BC_2 values(2,2),(3,3);
delete a.*,b.* from CMULTIBLOCK2.T_D_MUTIL_BC_1 a,CMULTIBLOCK2.T_D_MUTIL_BC_2 b where a.col2=b.col2;
select * from CMULTIBLOCK2.T_D_MUTIL_BC_1;
select * from CMULTIBLOCK2.T_D_MUTIL_BC_2;

DROP SCHEMA CMULTIBLOCK2 CASCADE;
-- subquery
delete a.*,b.* from CT_T_MUTIL_T1 a,(select * from T_T_MUTIL_T2) b where a.col1=b.col1; --error
--mateview
CREATE MATERIALIZED VIEW MATE_MULTIVIEW1 as select * from CT_T_MUTIL_T1;
CREATE MATERIALIZED VIEW MATE_MULTIVIEW2 as select * from T_T_MUTIL_T2;
delete a,b,c from CT_T_MUTIL_T1 a,MATE_MULTIVIEW1 b,MATE_MULTIVIEW2 c ;
drop MATERIALIZED VIEW MATE_MULTIVIEW1;
drop MATERIALIZED VIEW MATE_MULTIVIEW2;
--explain plan
explain delete a,b from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;
explain(verbose) delete a,b from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;
explain(format xml) delete a,b from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;
explain(format json) delete a,b from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;
explain(format yaml) delete a.*, b.* from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col2=b.col2;
-- temp table
drop table if exists CT_T_MUTIL_T1;

create temporary table CT_T_MUTIL_T1(col1 int, col2 int);
create temporary table T_T_MUTIL_T2(col1 int,col2 int);
insert into CT_T_MUTIL_T1 values(1,2),(2,2);
insert into T_T_MUTIL_T2 values(2,2),(3,3);
delete a.*,b.* from  CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col1=b.col1;
select * from CT_T_MUTIL_T1;--1,2
select * from T_T_MUTIL_T2;--3,3
drop table if exists CT_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
-- order by or limit
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int,col2 int);
create table T_MUTIL_T2(col1 int,col2 int);
insert into T_MUTIL_T1 values(1,2),(1,2),(3,3);
insert into T_MUTIL_T2 values(2,3),(2,3),(1,3);
delete from a,b using T_MUTIL_T1 a,T_MUTIL_T2 b where a.col1=b.col2 order by a.col2;  --error
delete from a,b using T_MUTIL_T1 a,T_MUTIL_T2 b where a.col1=b.col2 limit 1;    --error
select * from T_MUTIL_T1;
select * from T_MUTIL_T2;
-- returning
delete a,b from T_MUTIL_T1 a,T_MUTIL_T2 b where a.col1=b.col2 returning *; --error
-- left join
begin;
delete from a,b using T_MUTIL_T1 a left join T_MUTIL_T2 b on a.col1=b.col1;
select * from T_MUTIL_T1;--empty
select * from T_MUTIL_T2;--2,3--2,3
rollback;
--
drop table if exists T_U_MUTIL_T1;
drop table if exists T_U_MUTIL_T2;
drop table if exists T_U_MUTIL_T3;
create table T_U_MUTIL_T1(col1 int, col2 int);
create table T_U_MUTIL_T2(col1 int, col2 int);
create table T_U_MUTIL_T3(col1 int, col2 int);
insert into T_U_MUTIL_T1 values(1,2);
insert into T_U_MUTIL_T2 values(1,2);
insert into T_U_MUTIL_T3 values(1,2);
delete a,b,c from T_U_MUTIL_T1 a,T_U_MUTIL_T2 b,T_U_MUTIL_T3 c where a.col2=b.col2 and b.col2=c.col2;
select * from T_U_MUTIL_T1;--empty
select * from T_U_MUTIL_T2;--empty
select * from T_U_MUTIL_T3;--empty
-- has index
drop table if exists T_MUTIL_T1;
drop table if exists T_MUTIL_T2;
create table T_MUTIL_T1(col1 int);
create table T_MUTIL_T2(col1 int, col2 int);
create index idx1 on T_MUTIL_T1(col1);
create index idx2 on T_MUTIL_T2(col1);
insert into T_MUTIL_T1 values(1),(2);
insert into T_MUTIL_T2 values(2,2),(3,3);
begin;
delete a,b from T_MUTIL_T1 a,T_MUTIL_T2 b where a.col1=b.col1;
select * from T_MUTIL_T1;--1
select * from T_MUTIL_T2;--3,3
rollback;
-- ARdelete trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL1 WHERE des1_id1=OLD.src1_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL2 WHERE des2_id1=OLD.src2_id1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
AFTER delete ON TEST_TRIGGER_SRC_TBL1 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
AFTER delete ON TEST_TRIGGER_SRC_TBL2 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func2();
delete a,b from TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b  WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;--empty
SELECT * FROM TEST_TRIGGER_SRC_TBL2;--empty
SELECT * FROM TEST_TRIGGER_DES_TBL1;--empty
SELECT * FROM TEST_TRIGGER_DES_TBL2;--empty
-- BRdelete trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL1;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL2;
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
BEFORE delete ON TEST_TRIGGER_SRC_TBL1 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
BEFORE delete ON TEST_TRIGGER_SRC_TBL2 
FOR EACH ROW
EXECUTE PROCEDURE tri_delete_func2();
delete a.*,b.* from TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 
-- BSdelete trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL2;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER delete_bs_trigger1
BEFORE delete ON TEST_TRIGGER_SRC_TBL1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_bs_trigger2
BEFORE delete ON TEST_TRIGGER_SRC_TBL2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func2();
delete a.*,b.* from TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 
-- ASdelete trig
drop table if exists TEST_TRIGGER_SRC_TBL1;
drop table if exists TEST_TRIGGER_SRC_TBL2;
drop table if exists TEST_TRIGGER_DES_TBL1;
drop table if exists TEST_TRIGGER_DES_TBL2;
CREATE TABLE TEST_TRIGGER_SRC_TBL1(src1_id1 INT, src1_id2 INT, src1_id3 INT);
CREATE TABLE TEST_TRIGGER_SRC_TBL2(src2_id1 INT, src2_id2 INT, src2_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL1(des1_id1 INT, des1_id2 INT, des1_id3 INT);
CREATE TABLE TEST_TRIGGER_DES_TBL2(des2_id1 INT, des2_id2 INT, des2_id3 INT);
CREATE OR REPLACE FUNCTION tri_delete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL1;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tri_delete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
delete from TEST_TRIGGER_DES_TBL2;
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
INSERT INTO TEST_TRIGGER_SRC_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_SRC_TBL2 VALUES(300,400,500);
INSERT INTO TEST_TRIGGER_DES_TBL1 VALUES(100,200,300);
INSERT INTO TEST_TRIGGER_DES_TBL2 VALUES(300,400,500);
CREATE TRIGGER delete_ar_trigger1
AFTER delete ON TEST_TRIGGER_SRC_TBL1 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func1();

CREATE TRIGGER delete_ar_trigger2
AFTER delete ON TEST_TRIGGER_SRC_TBL2 
FOR EACH STATEMENT
EXECUTE PROCEDURE tri_delete_func2();
delete a,b from TEST_TRIGGER_SRC_TBL1 a,TEST_TRIGGER_SRC_TBL2 b WHERE a.src1_id3=b.src2_id1;
SELECT * FROM TEST_TRIGGER_SRC_TBL1;
SELECT * FROM TEST_TRIGGER_SRC_TBL2;
SELECT * FROM TEST_TRIGGER_DES_TBL1; 
SELECT * FROM TEST_TRIGGER_DES_TBL2; 
drop function tri_delete_func1;
drop function tri_delete_func2;
-- delete alias
drop table if exists T_T_MUTIL_T2;
create table CT_T_MUTIL_T1(col1 int,col2 int);
create table T_T_MUTIL_T2(col1 int,col2 int);
delete from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b using CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col1=b.col1;--error
delete from a,b using CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b where a.col1=b.col1;
delete from CT_T_MUTIL_T1 a,T_T_MUTIL_T2 b using a,b where a.col1=b.col1;--error
delete from CT_T_MUTIL_T1 a where a.col1=1;--error
delete from CT_T_MUTIL_T1 a using a where a.col1=1;--error
delete from a using CT_T_MUTIL_T1 a where a.col1=1;
delete from CT_T_MUTIL_T1 a using T_T_MUTIL_T2 a where a.col1=1;--error
delete from a using CT_T_MUTIL_T1 a where a.col1=1;
delete from CT_T_MUTIL_T1,CT_T_MUTIL_T1 using CT_T_MUTIL_T1,CT_T_MUTIL_T1 where CT_T_MUTIL_T1.col1=1;--error
delete from CT_T_MUTIL_T1 using CT_T_MUTIL_T1,CT_T_MUTIL_T1 where CT_T_MUTIL_T1.col1=1;--error
delete from CT_T_MUTIL_T1 a,CT_T_MUTIL_T1 a where a.col1=1;--error
delete from CT_T_MUTIL_T1 a,CT_T_MUTIL_T1 b where a.col1=1;--error
-- view
drop table if exists CT_T_MUTIL_T1;
drop table if exists T_T_MUTIL_T2;
drop table if exists T_T_MUTIL_T3;
create table CT_T_MUTIL_T1(col1 int,col2 int);
create table T_T_MUTIL_T2(col1 int,col2 int,col3 int);
create table T_T_MUTIL_T3(col1 int,col2 int);
insert into CT_T_MUTIL_T1 values(1,1),(1,1);
insert into T_T_MUTIL_T2(col1,col2) values(1,1),(1,2);
insert into T_T_MUTIL_T3 values(1,1),(1,3);
drop view if exists multiview1,multiview2,multiview3;
create view multiview1 as select * from CT_T_MUTIL_T1;
create view multiview2 as select * from T_T_MUTIL_T2;
create view multiview3 as select * from T_T_MUTIL_T3;
delete a,b,c from multiview1 a,multiview2 b,multiview3 c where a.col1 = b.col1 and b.col2 = c.col2;
select * from CT_T_MUTIL_T1;--empty
select * from T_T_MUTIL_T2;--1,2,NULL
select * from T_T_MUTIL_T3;--1,3
-- left join
insert into CT_T_MUTIL_T1 values(1,1),(1,1);
insert into T_T_MUTIL_T2(col1,col2) values(1,1),(1,2);
delete from multiview1,T_T_MUTIL_T2 using T_T_MUTIL_T2 left join multiview1 on multiview1.col2=T_T_MUTIL_T2.col1;
select * from CT_T_MUTIL_T1;--empty
select * from T_T_MUTIL_T2;--empty
drop view multiview1;
drop view multiview2;
drop view multiview3;
drop table T_MUTIL_T1,T_MUTIL_T2;
drop table CT_T_MUTIL_T1,T_T_MUTIL_T2,T_T_MUTIL_T3;
drop table T_U_MUTIL_T1,T_U_MUTIL_T2,T_U_MUTIL_T3;
drop table TEST_TRIGGER_DES_TBL1,TEST_TRIGGER_DES_TBL2,TEST_TRIGGER_SRC_TBL1,TEST_TRIGGER_SRC_TBL2;
