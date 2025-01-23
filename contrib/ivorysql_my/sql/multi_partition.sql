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
create table idxpart1 (a int, b int, primary key (a)) partition by range (a);
create table idxpart12 partition of idxpart1 for values from (0) to (1000) partition by range (a);
create table idxpart13 partition of idxpart12 for values from (0) to (100);
create table idxpart14 partition of idxpart12 for values from (101) to (200);
insert into idxpart12 partition(idxpart13,idxpart14)(a,b) values(1,2),(150,20);

create table idxpart11 (a int, b int, primary key (a)) partition by range (a);
create table idxpart21 partition of idxpart11 for values from (0) to (1000) partition by range (a);
create table idxpart31 partition of idxpart21 for values from (0) to (100);
create table idxpart41 partition of idxpart21 for values from (101) to (200);
insert into idxpart21 partition(idxpart31,idxpart41)(a,b) values(1,2),(150,20);

update idxpart12 partition(idxpart13) aa use index(idxpart12_pkey),idxpart21 partition (idxpart31) bb set aa.a = aa.a + 10,bb.a = bb.a + 10 where aa.b = bb.b;
select * from idxpart12;
select * from idxpart21;

begin;
delete aa,bb from idxpart12 partition(idxpart13) aa,idxpart21 partition (idxpart31) bb
where aa.b = bb.b;
select * from idxpart12;
select * from idxpart21;
rollback;

begin;
update idxpart1 aa,idxpart11 bb set aa.a = aa.a + 1,bb.a = bb.a + 1 where aa.b = bb.b;
select * from idxpart12;
select * from idxpart21;
update idxpart1 aa ,idxpart11 bb set aa.a = aa.a + 1,bb.a = bb.a + 1 where aa.b = bb.b;
select * from idxpart12;
select * from idxpart21;
rollback;
--single table delete
begin;
delete from idxpart12 partition(idxpart13);
select * from idxpart12;
rollback;

--partition relation and base relation
begin;
delete from aa,bb,cc using idxpart12 partition(idxpart13) aa ,idxpart21 partition (idxpart31) bb ,test cc
where aa.b = bb.b and aa.b = cc.b;
select * from idxpart12;
select * from idxpart21;
select * from test;
rollback;

begin;
delete from aa,bb,cc using idxpart12 partition(idxpart13,idxpart14) aa ,idxpart21 partition (idxpart31,idxpart41) bb ,test cc
where aa.b = bb.b and aa.b = cc.b;
select * from idxpart12;
select * from idxpart21;
select * from test;
rollback;

--pbe
PREPARE multi_part_del AS
	delete aa,bb from idxpart12 partition(idxpart13) aa,idxpart21 partition (idxpart31) bb where aa.b = bb.b;
begin;
EXECUTE multi_part_del;
select * from idxpart12;
select * from idxpart21;
rollback;

--procedure
CREATE OR REPLACE PROCEDURE proc_mutil_del()
as $$ 
BEGIN
	delete from aa,bb,cc using idxpart12 partition(idxpart13,idxpart14) aa ,idxpart21 partition (idxpart31,idxpart41) bb ,test cc
	where aa.b = bb.b and aa.b = cc.b;
END $$ language plmysql;

begin;
CALL proc_mutil_del();
select * from idxpart12;
select * from idxpart21;
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
AFTER DELETE ON idxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletear_trigger2
AFTER DELETE ON idxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
delete from aa,bb using idxpart12 partition(idxpart13,idxpart14) aa ,idxpart21 partition (idxpart31,idxpart41) bb
	where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from idxpart12;
select * from idxpart21;
rollback;

drop trigger deletear_trigger1 on idxpart12 cascade;
drop trigger deletear_trigger2 on idxpart21 cascade;
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
AFTER DELETE ON idxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletear_trigger2
AFTER DELETE ON idxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
delete from aa,bb using idxpart12 partition(idxpart13,idxpart14) aa ,idxpart21 partition (idxpart31,idxpart41) bb
	where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from idxpart12;
select * from idxpart21;
rollback;

drop trigger deletear_trigger1 on idxpart12 cascade;
drop trigger deletear_trigger2 on idxpart21 cascade;
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
BEFORE DELETE ON idxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletebf_trigger2
BEFORE DELETE ON idxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
select * from idxpart12;
select * from idxpart21;
delete aa,bb from idxpart12 partition(idxpart14) aa,idxpart21 partition (idxpart41) bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from idxpart12;
select * from idxpart21;
rollback;

drop trigger deletebf_trigger1 on idxpart12 cascade;
drop trigger deletebf_trigger2 on idxpart21 cascade;
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
BEFORE DELETE ON idxpart12
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func1();

CREATE TRIGGER deletebf_trigger2
BEFORE DELETE ON idxpart21
FOR EACH ROW
EXECUTE PROCEDURE tridelete_func2();
begin;
select * from idxpart12;
select * from idxpart21;
delete aa,bb from idxpart12 partition(idxpart14) aa,idxpart21 partition (idxpart41) bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from idxpart12;
select * from idxpart21;
rollback;

begin;
select * from idxpart12;
select * from idxpart21;
delete aa,bb from idxpart12 aa,idxpart21 bb where aa.b = bb.b;
select * from test_trigger_src;
select * from test_trigger_src1;
select * from idxpart12;
select * from idxpart21;
rollback;

drop trigger deletebf_trigger1 on idxpart12 cascade;
drop trigger deletebf_trigger2 on idxpart21 cascade;
drop function tridelete_func1();
drop function tridelete_func2();
drop table test_trigger_src,test_trigger_src1;
drop table idxpart1,idxpart11,test;

drop table if exists idxpart,idxpart2;
create table idxpart (a int, b int, primary key (a)) partition by range (a);
create table idxpart2 partition of idxpart for values from (0) to (1000) partition by range (a);
create table idxpart3 partition of idxpart2 for values from (0) to (100);
create table idxpart4 partition of idxpart2 for values from (101) to (200);
create table idxpart5 partition of idxpart2 for values from (201) to (300);
create table idxpart6 partition of idxpart2 for values from (301) to (400);
create table idxpart7 partition of idxpart2 for values from (401) to (500);

insert into idxpart2 partition(idxpart3)(a,b) values(1,2),(150,10);--error
insert into idxpart2 partition(idxpart3,idxpart4)(a,b) values(1,2),(150,10);
select * from idxpart2;
insert into idxpart2 partition(idxpart3)(a,b) values(1,5) on duplicate key update a = a + 1;
select * from idxpart2;
insert into idxpart2 partition(idxpart3)(a,b) values(150,5) on duplicate key update a = a + 10;--error
insert into idxpart2 partition(idxpart3,idxpart4,idxpart5)(a,b) values(150,5) on duplicate key update a = a + 10;
select * from idxpart2;
insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) values(220,15),(330,20),(440,25);
select * from idxpart2;
--pbe
begin;
PREPARE multi_part_insert(int,int) AS
	insert into idxpart values($1,$2) on duplicate key update a = a + 20;
EXECUTE multi_part_insert(160,20);
select * from idxpart;
rollback;

--procedure
CREATE OR REPLACE PROCEDURE proc_mutil_insert()
as $$ 
BEGIN
	insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) 
	values(220,15),(330,20),(440,25) on duplicate key update a = values(a) + values(b);
END $$ language plmysql;

begin;
CALL proc_mutil_insert();
select * from idxpart;
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
AFTER INSERT ON idxpart2 
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
AFTER UPDATE ON idxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from idxpart2;
insert into idxpart2 values(5,3);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertar_trigger1 on idxpart2 cascade;
drop trigger insertar_trigger2 on idxpart2 cascade;
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
AFTER INSERT ON idxpart2 
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
AFTER UPDATE ON idxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from idxpart2;
insert into idxpart2 values(5,3);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertar_trigger1 on idxpart2 cascade;
drop trigger insertar_trigger2 on idxpart2 cascade;
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
BEFORE INSERT ON idxpart2 
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
BEFORE UPDATE ON idxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from idxpart2;
insert into idxpart2 values(5,3);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertbf_trigger1 on idxpart2 cascade;
drop trigger insertbf_trigger2 on idxpart2 cascade;
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
BEFORE INSERT ON idxpart2 
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
BEFORE UPDATE ON idxpart2 
FOR EACH ROW
EXECUTE PROCEDURE trinsert_func2();

begin;
select * from idxpart2;
insert into idxpart2 values(5,3);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
insert into idxpart2 partition(idxpart5,idxpart6,idxpart7) 
	values(220,15),(330,20) on duplicate key update a = values(a) + values(b);
select * from idxpart2;
select * from test_trigger_src;
select * from test_trigger_src1;
rollback;

drop trigger insertbf_trigger1 on idxpart2 cascade;
drop trigger insertbf_trigger2 on idxpart2 cascade;
drop function trinsert_func1();
drop function trinsert_func2();
drop table idxpart,test_trigger_src,test_trigger_src1;
