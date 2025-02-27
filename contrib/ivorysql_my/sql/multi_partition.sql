--partition relations
drop table if exists test_p,test_p1,test_p2 ;
create table test_p(id int) partition by range (id);
create table p1 partition of test_p for values from (0) to (100);
create table p2 partition of test_p for values from (101) to (200);

create table test_p1(id int) partition by range (id);
create table p11 partition of test_p1 for values from (0) to (100);
create table p12 partition of test_p1 for values from (101) to (200);

create table test_p2(id int) partition by range (id);
create table p21 partition of test_p2 for values from (0) to (100);
create table p22 partition of test_p2 for values from (101) to (200);

insert into test_p values(150);
insert into test_p1 values(150);
insert into test_p2 values(150);
select * from test_p;
select * from test_p1;
update test_p a,test_p1 b,test_p2 c set a.id = 15,b.id = 15,c.id = 15 where a.id = b.id and a.id = c.id;
select * from test_p;
select * from test_p1;
select * from test_p2;
--one partition relation
update test_p2 set id = 160;
select * from test_p2;
update test_p2 partition(p21) set id = 120;--0
update test_p2 partition(p22) set id = 15;--error
update test_p2 partition(p22,p21) set id = 15;
select * from test_p2;

update test_p partition(p1),test_p1 partition(p21) set test_p.id = 15,test_p1.id = 160 where test_p.id = test_p1.id;--error
update test_p partition(p1),test_p1 partition(p11) set test_p.id = 16,test_p1.id = 16 where test_p.id = test_p1.id;
select * from test_p;
select * from test_p1;
update test_p partition(p2),test_p1 partition(p11) set test_p.id = 160,test_p1.id = 16 where test_p.id = test_p1.id;--0
update test_p partition(p2),test_p1 partition(p12) set test_p.id = 160,test_p1.id = 160 where test_p.id = test_p1.id;--0
select * from test_p;
select * from test_p1;
update test_p a,test_p1 b set a.id = 160,b.id = 160 where a.id = b.id;
select * from test_p;
select * from test_p1;
update test_p partition(p2),test_p1 partition(p21) set test_p.id = 170,test_p1.id = 170 where test_p.id = test_p1.id;--error
update test_p partition(p1,p2),test_p1 partition(p11,p12) set test_p.id = 150,test_p1.id = 150 where test_p.id = test_p1.id;
select * from test_p;
select * from test_p1;
update test_p partition(p2),test_p1 partition(p12) set test_p.id = 170,test_p1.id = 170 where test_p.id = test_p1.id;
select * from test_p;
select * from test_p1;

--partition relations and base relatinons
create table test(id int);
insert into test value(170);
create table test1(id int,idd int);
insert into test1 values(170,17);
update test_p a,test_p1 b,test c,test1 d  set a.id = 120,b.id = 120,c.id = 120,d.id = 120 where a.id = b.id and b.id=d.id;
select * from test_p;
select * from test_p1;
select * from test;
select * from test1;
update test_p a,test_p1 b,test c,test1 d  set a.id = 120,b.id = 120,c.id = 120,d.idd = 120 where a.id = b.id and b.id=d.id;
select * from test_p;
select * from test_p1;
select * from test;
select * from test1;
begin;
update test_p a,test_p1 b,test c,test1 d  set a.id = 12,b.id = 12,d.idd = 12 where a.id = b.id and b.id=d.idd;
update test_p partition(p2) a,test_p1 partition(p12) b,test c,test1 d  set a.id = 12,b.id = 12,d.idd = 12 where a.id = b.id and b.id=d.idd;
select * from test_p;
select * from test_p1;
select * from test;
select * from test1;
rollback;

begin;
update test_p a,test_p1 b,test c,test1 d  set a.id = 150,b.id = 140,d.idd = 130 where a.id = b.id and b.id=d.idd;
select * from test_p;
select * from test_p1;
select * from test;
select * from test1;
rollback;
--pbe
begin;
PREPARE multi_partrel(int, int) AS
	update test_p partition(p2) a,test_p1 partition(p12) b set a.id=$1, b.id=$2 where a.id = b.id;
EXECUTE multi_partrel(160,160);
select * from test_p;
select * from test_p1;
rollback;

--procedure
CREATE OR REPLACE PROCEDURE proc_mutil(c1 int,c2 int)
as $$ 
BEGIN
update test_p partition(p2) a,test_p1 partition(p12) b set a.id=c1, b.id=c2 where a.id = b.id;
END $$ language plmysql;

begin;
CALL proc_mutil(150,150);
select * from test_p;
select * from test_p1;
rollback;

--trigger
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);

CREATE OR REPLACE FUNCTION triupdate_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src value(10);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION triupdate_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 value(20);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER updatear_trigger1
AFTER UPDATE ON test_p 
FOR EACH ROW
EXECUTE PROCEDURE triupdate_func1();

CREATE TRIGGER updatear_trigger2
AFTER UPDATE ON test_p1 
FOR EACH ROW
EXECUTE PROCEDURE triupdate_func2();
begin;
update test_p partition(p2) a,test_p1 partition(p12) b set a.id=160, b.id=160 where a.id = b.id;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from test_p;
select * from test_p1;
rollback;

drop trigger updatear_trigger1 on test_p cascade;
drop trigger updatear_trigger2 on test_p1 cascade;

-- BRupdate trig
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src(src_id INT);
CREATE TABLE test_trigger_src1(src_id1 INT);
CREATE OR REPLACE FUNCTION triupdate_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src values(30);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION triupdate_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 values(40);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER updatebf_trigger1
BEFORE UPDATE ON test_p 
FOR EACH ROW
EXECUTE PROCEDURE triupdate_func1();

CREATE TRIGGER updatebf_trigger2
BEFORE UPDATE ON test_p1 
FOR EACH ROW
EXECUTE PROCEDURE triupdate_func2();
begin;
insert into test_p value(12);
insert into test_p1 value(12);
update test_p partition(p1,p2) a,test_p1 partition(p11,p12) b set a.id=16, b.id=16 where a.id = b.id;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from test_p;
select * from test_p1;
rollback;

drop trigger updatebf_trigger1 on test_p cascade;
drop trigger updatebf_trigger2 on test_p1 cascade;
drop table test_trigger_src,test_trigger_src1;
drop table test_p,test_p1,test_p2,test,test1;

create table test(a int,b int);
insert into test value(1,2),(150,20);
create table mysidxpart1 (a int, b int, primary key (a)) partition by range (a);
create table mysidxpart12 partition of mysidxpart1 for values from (0) to (1000) partition by range (a);
create table mysidxpart13 partition of mysidxpart12 for values from (0) to (100);
create table mysidxpart14 partition of mysidxpart12 for values from (101) to (200);
insert into mysidxpart12 partition(mysidxpart13,mysidxpart14)(a,b) values(1,2),(150,20);

create table mysidxpart11 (a int, b int, primary key (a)) partition by range (a);
create table mysidxpart21 partition of mysidxpart11 for values from (0) to (1000) partition by range (a);
create table mysidxpart31 partition of mysidxpart21 for values from (0) to (100);
create table mysidxpart41 partition of mysidxpart21 for values from (101) to (200);
insert into mysidxpart21 partition(mysidxpart31,mysidxpart41)(a,b) values(1,2),(150,20);

update mysidxpart12 partition(mysidxpart13) aa use index(mysidxpart12_pkey),mysidxpart21 partition (mysidxpart31) bb set aa.a = aa.a + 10,bb.a = bb.a + 10 where aa.b = bb.b;
select * from mysidxpart12;
select * from mysidxpart21;

begin;
delete aa,bb from mysidxpart12 partition(mysidxpart13) aa,mysidxpart21 partition (mysidxpart31) bb
where aa.b = bb.b;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

begin;
update mysidxpart1 aa,mysidxpart11 bb set aa.a = aa.a + 1,bb.a = bb.a + 1 where aa.b = bb.b;
select * from mysidxpart12;
select * from mysidxpart21;
update mysidxpart1 aa ,mysidxpart11 bb set aa.a = aa.a + 1,bb.a = bb.a + 1 where aa.b = bb.b;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;
--single table delete
begin;
delete from mysidxpart12 partition(mysidxpart13);
select * from mysidxpart12;
rollback;

--partition relation and base relation
begin;
delete from aa,bb,cc using mysidxpart12 partition(mysidxpart13) aa ,mysidxpart21 partition (mysidxpart31) bb ,test cc
where aa.b = bb.b and aa.b = cc.b;
select * from mysidxpart12;
select * from mysidxpart21;
select * from test;
rollback;

begin;
delete from aa,bb,cc using mysidxpart12 partition(mysidxpart13,mysidxpart14) aa ,mysidxpart21 partition (mysidxpart31,mysidxpart41) bb ,test cc
where aa.b = bb.b and aa.b = cc.b;
select * from mysidxpart12;
select * from mysidxpart21;
select * from test;
rollback;

--pbe
PREPARE multi_part_del AS
	delete aa,bb from mysidxpart12 partition(mysidxpart13) aa,mysidxpart21 partition (mysidxpart31) bb where aa.b = bb.b;
begin;
EXECUTE multi_part_del;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

--procedure
CREATE OR REPLACE PROCEDURE proc_mutil_del()
as $$ 
BEGIN
	delete from aa,bb,cc using mysidxpart12 partition(mysidxpart13,mysidxpart14) aa ,mysidxpart21 partition (mysidxpart31,mysidxpart41) bb ,test cc
	where aa.b = bb.b and aa.b = cc.b;
END $$ language plmysql;

begin;
CALL proc_mutil_del();
select * from mysidxpart12;
select * from mysidxpart21;
select * from test;
rollback;
drop PROCEDURE proc_mutil_del();

--trigger
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);
CREATE OR REPLACE FUNCTION tridelete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src value(10);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tridelete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 value(20);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER deletear_trigger1
AFTER DELETE ON mysidxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletear_trigger2
AFTER DELETE ON mysidxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
delete from aa,bb using mysidxpart12 partition(mysidxpart13,mysidxpart14) aa ,mysidxpart21 partition (mysidxpart31,mysidxpart41) bb
	where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

drop trigger deletear_trigger1 on mysidxpart12 cascade;
drop trigger deletear_trigger2 on mysidxpart21 cascade;
drop function tridelete_func1();
drop function tridelete_func2();
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;

CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);

CREATE OR REPLACE FUNCTION tridelete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src value(10);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tridelete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 value(20);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER deletear_trigger1
AFTER DELETE ON mysidxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletear_trigger2
AFTER DELETE ON mysidxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
delete from aa,bb using mysidxpart12 partition(mysidxpart13,mysidxpart14) aa ,mysidxpart21 partition (mysidxpart31,mysidxpart41) bb
	where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

drop trigger deletear_trigger1 on mysidxpart12 cascade;
drop trigger deletear_trigger2 on mysidxpart21 cascade;
drop function tridelete_func1();
drop function tridelete_func2();

-- BRupdate trig
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src(src_id INT);
CREATE TABLE test_trigger_src1(src_id1 INT);
CREATE OR REPLACE FUNCTION tridelete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src values(30);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tridelete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 values(40);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER deletebf_trigger1
BEFORE DELETE ON mysidxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletebf_trigger2
BEFORE DELETE ON mysidxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
select * from mysidxpart12;
select * from mysidxpart21;
delete aa,bb from mysidxpart12 partition(mysidxpart14) aa,mysidxpart21 partition (mysidxpart41) bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

drop trigger deletebf_trigger1 on mysidxpart12 cascade;
drop trigger deletebf_trigger2 on mysidxpart21 cascade;
drop function tridelete_func1();
drop function tridelete_func2();
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;

CREATE TABLE test_trigger_src(src_id INT);
CREATE TABLE test_trigger_src1(src_id1 INT);
CREATE OR REPLACE FUNCTION tridelete_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src values(30);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE OR REPLACE FUNCTION tridelete_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into test_trigger_src1 values(40);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER deletebf_trigger1
BEFORE DELETE ON mysidxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletebf_trigger2
BEFORE DELETE ON mysidxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
select * from mysidxpart12;
select * from mysidxpart21;
delete aa,bb from mysidxpart12 partition(mysidxpart14) aa,mysidxpart21 partition (mysidxpart41) bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

begin;
select * from mysidxpart12;
select * from mysidxpart21;
delete aa,bb from mysidxpart12 aa,mysidxpart21 bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from mysidxpart12;
select * from mysidxpart21;
rollback;

drop trigger deletebf_trigger1 on mysidxpart12 cascade;
drop trigger deletebf_trigger2 on mysidxpart21 cascade;
drop function tridelete_func1();
drop function tridelete_func2();
drop table test_trigger_src,test_trigger_src1;
drop table mysidxpart1,mysidxpart11,test;

drop table if exists mysidxpart,mysidxpart2;
create table mysidxpart (a int, b int, primary key (a)) partition by range (a);
create table mysidxpart2 partition of mysidxpart for values from (0) to (1000) partition by range (a);
create table mysidxpart3 partition of mysidxpart2 for values from (0) to (100);
create table mysidxpart4 partition of mysidxpart2 for values from (101) to (200);
create table mysidxpart5 partition of mysidxpart2 for values from (201) to (300);
create table mysidxpart6 partition of mysidxpart2 for values from (301) to (400);
create table mysidxpart7 partition of mysidxpart2 for values from (401) to (500);

insert into mysidxpart2 partition(mysidxpart3)(a,b) values(1,2),(150,10);--error
insert into mysidxpart2 partition(mysidxpart3,mysidxpart4)(a,b) values(1,2),(150,10);
select * from mysidxpart2;
insert into mysidxpart2 partition(mysidxpart3)(a,b) values(1,5) on duplicate key update a = a + 1;
select * from mysidxpart2;
insert into mysidxpart2 partition(mysidxpart3)(a,b) values(150,5) on duplicate key update a = a + 10;--error
insert into mysidxpart2 partition(mysidxpart3,mysidxpart4,mysidxpart5)(a,b) values(150,5) on duplicate key update a = a + 10;
select * from mysidxpart2;
insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) values(220,15),(330,20),(440,25);
select * from mysidxpart2;
--pbe
begin;
PREPARE multi_part_insert(int,int) AS
	insert into mysidxpart values($1,$2) on duplicate key update a = a + 20;
EXECUTE multi_part_insert(160,20);
select * from mysidxpart;
rollback;

--procedure
CREATE OR REPLACE PROCEDURE proc_mutil_insert()
as $$ 
BEGIN
	insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) 
	values(220,15),(330,20),(440,25) on duplicate key update a = values(a) + values(b);
END $$ language plmysql;

begin;
CALL proc_mutil_insert();
select * from mysidxpart;
rollback;
drop PROCEDURE proc_mutil_insert();
--aftrigger
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);

CREATE OR REPLACE FUNCTION trinsert_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src value(10);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertar_trigger1
AFTER INSERT ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func1();

CREATE OR REPLACE FUNCTION trinsert_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src1 value(20);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertar_trigger2
AFTER UPDATE ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from mysidxpart2;
insert into mysidxpart2 values(5,3);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertar_trigger1 on mysidxpart2 cascade;
drop trigger insertar_trigger2 on mysidxpart2 cascade;
drop function trinsert_func1();
drop function trinsert_func2();

drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);

CREATE OR REPLACE FUNCTION trinsert_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src value(10);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertar_trigger1
AFTER INSERT ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func1();

CREATE OR REPLACE FUNCTION trinsert_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src1 value(20);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertar_trigger2
AFTER UPDATE ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from mysidxpart2;
insert into mysidxpart2 values(5,3);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertar_trigger1 on mysidxpart2 cascade;
drop trigger insertar_trigger2 on mysidxpart2 cascade;
drop function trinsert_func1();
drop function trinsert_func2();

--bf trigger
drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);
CREATE OR REPLACE FUNCTION trinsert_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src value(30);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertbf_trigger1
BEFORE INSERT ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func1();

CREATE OR REPLACE FUNCTION trinsert_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src1 value(40);
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertbf_trigger2
BEFORE UPDATE ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from mysidxpart2;
insert into mysidxpart2 values(5,3);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertbf_trigger1 on mysidxpart2 cascade;
drop trigger insertbf_trigger2 on mysidxpart2 cascade;
drop function trinsert_func1();
drop function trinsert_func2();


drop table if exists test_trigger_src;
drop table if exists test_trigger_src1;
CREATE TABLE test_trigger_src1(src1_id INT);
CREATE TABLE test_trigger_src(src_id INT);
CREATE OR REPLACE FUNCTION trinsert_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src value(30);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertbf_trigger1
BEFORE INSERT ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func1();

CREATE OR REPLACE FUNCTION trinsert_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
	insert into test_trigger_src1 value(40);
RETURN NEW;
END
$$ LANGUAGE PLMYSQL;

CREATE TRIGGER insertbf_trigger2
BEFORE UPDATE ON mysidxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from mysidxpart2;
insert into mysidxpart2 values(5,3);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into mysidxpart2 partition(mysidxpart5,mysidxpart6,mysidxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from mysidxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertbf_trigger1 on mysidxpart2 cascade;
drop trigger insertbf_trigger2 on mysidxpart2 cascade;
drop function trinsert_func1();
drop function trinsert_func2();
drop table mysidxpart,test_trigger_src,test_trigger_src1;

--BUG#Z4250
create table t1(num int,name text,salary int) partition by range (num);
create table p1 partition of t1 for values from (0) to (100);  
create table t2(num int,name text,score int) partition by range(num);
create table p11 partition of t2 for values from (0) to (100);	
insert into  t1 values(1,'a',200),(2,'b',300),(3,'c',100);
insert into t2 values(1,'xxx',60),(2,'b',90),(5,'zzz',70);
update t1 partition(p1) cross join t2  set t1.salary = t1.salary * 2;select  salary from t1;
update t1 partition(p1) a cross join t2 b on b.num = a.num set a.num=3,b.num = 6;
select count(*) from t1 where num =3;select count(*) from t2 where num =6;
drop table t1,t2;
--BUG#Z4273
CREATE TABLE pt1
(
	id SERIAL NOT NULL,
	name VARCHAR NOT NULL
) PARTITION BY LIST (name);
CREATE TABLE pt2
(
	id SERIAL NOT NULL,
	name VARCHAR NOT NULL
) PARTITION BY LIST (name);

create index index_id ON pt1 (id);
create index index_id2 ON pt2 (id);

create table pt1_1
partition of pt1 for values in ('张三','李四');
create table pt1_2
partition of pt1 for values in ('王五','赵六');
create table pt2_1
partition of pt2 for values in ('张三','李四');
create table pt2_2
partition of pt2 for values in ('王五','赵六');

insert into pt1(id, "name")
values(1,'张三'),(2,'李四'),(3,'王五'),(4,'赵六');
insert into pt2 (id, "name")
values(1,'张三'),(2,'李四'),(3,'王五'),(4,'赵六'); 
select * from pt1_1;
select * from pt2_1;
delete aa,bb from pt1 partition(pt1_1) aa use index(index_id),pt2 partition (pt2_1) bb use index(index_id2)   where aa.id = bb.id; 
select * from pt1_1;
select * from pt2_1;
drop table pt1,pt2;
