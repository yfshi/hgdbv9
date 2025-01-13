--LOW_PRIORITY/DELAYED
create table test(id int primary key,name varchar(20));
insert into test values(1,'aa');
replace LOW_PRIORITY into test values(1,'bb');
select * from test;
replace DELAYED  into test set id = id + 1;
drop table test;
create table test(a int, b int default 3);
replace test(a,b) value(b * 2, 2);
replace test(a,b) value(2, b * 2);
select * from test;
create table test1(a int, b int);
replace test1(a,b) value(b * 2, 2);
replace test1(a,b) value(2, b * 2);
select * from test1;
--column not match value
create table test2(a int , b varchar(50));
set mysql.enable_ignore_columns_list to on;
replace into test2 values(1);
replace into test2 values(2),(3);
set mysql.enable_ignore_columns_list to default;
replace into test2 values(1);
replace into test2 values(2),(3);
select * from test2;
drop table test,test1,test2;

--defatult value
create table test2(id int default 2,name varchar(20));
insert into test2 values(1,'aa');
replace into test2 set id = id + 2;
select * from test2;
drop table test2;
--unique and default
create table test1(id int unique default 2,name varchar(20));
insert into test1 values(1,'aa');
replace into test1 set id = default(id) + 2;
replace into test1 set id = id + 2;
select * from test1;
drop table test1;
--unique
create table test1(id int unique,name varchar(20));
insert into test1 values(1,'aa');
replace into test1 set id = id + 2;
select * from test1;
drop table test1;
--primary key
create table test1(id int primary key,name varchar(20));
insert into test1 values(1,'aa');
replace into test1 set id = id + 2;
replace into test1 set id = id + 1;
select * from test1;
drop table test1;
--primary key and default
create table test1(id int primary key default 2,name varchar(20));
insert into test1 values(1,'aa');
replace into test1 set id = id + 2;
select * from test1;
drop table test1;
--temporary table
create temporary table t1(col1 int, col2 int);
create table test1(a int,b int);
insert into test1 values(1,2);
replace into t1 select * from test1;
replace into t1 select * from t1;--error
drop table test1;
--function
create table tfunc(a int, b int);
create or replace function att(b int) returns int AS
$$
declare a int:=100;
begin
	replace into tfunc values(10, a * 3);
	return a;
end;
$$ language plmysql;
select att(3);
create or replace function att(b int) returns int AS
$$
declare c int:=100;
begin
	replace into tfunc values(10, a * 3);
	return c;
end;
$$ language plmysql;
select att(2);
select * from tfunc;
drop table tfunc;
drop function att;
--trigger
create table tt1(a int primary key, b int);
create table tt2(id int,name text);
CREATE OR REPLACE FUNCTION tri_replace_func1() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into tt2 values(22,'tom');
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER replace_ar_trigger1
AFTER insert ON tt1 
FOR EACH ROW
EXECUTE PROCEDURE tri_replace_func1();
CREATE OR REPLACE FUNCTION tri_replace_func2() RETURNS TRIGGER AS
$$
DECLARE
BEGIN
insert into tt2 values(44,'tim');
RETURN OLD;
END
$$ LANGUAGE PLMYSQL;
CREATE TRIGGER replace_ar_trigger2
AFTER delete ON tt1 
FOR EACH ROW
EXECUTE PROCEDURE tri_replace_func2();
replace into tt1 value(1,2);
select * from tt1;
select * from tt2;
replace into tt1 value(1,3);
select * from tt1;
select * from tt2;
drop trigger replace_ar_trigger1 on tt1;
drop trigger replace_ar_trigger2 on tt2;
drop table tt1,tt2;
--pbe
create table tt(a int primary key,b int);
insert tt value(2, 3),(5, 6);
PREPARE ptt (int, int) AS
	replace INTO tt VALUES($1, $2);
EXECUTE ptt(5,10);
select * from tt;

PREPARE ptt1 (int) AS
	replace INTO tt VALUES($1, $1 * 4);
EXECUTE ptt1(5);
select * from tt;

PREPARE ptt2 (int) AS
	replace INTO tt VALUES($1, $1 * 4);
EXECUTE ptt2(6);
select * from tt;
drop table tt;
--partition table
create table idxpart (a int, b int, primary key (a)) partition by range (a);
create table idxpart2 partition of idxpart for values from (0) to (1000) partition by range (a);
create table idxpart3 partition of idxpart2 for values from (0) to (100);
create table idxpart4 partition of idxpart2 for values from (101) to (200);

insert into idxpart2 partition(idxpart3)(a,b) values(1,2),(150,10) on duplicate key update b = b + 2;--error
insert into idxpart2 partition(idxpart3,idxpart4)(a,b) values(1,2),(150,10) on duplicate key update b = b + 2;
replace into idxpart2 partition(idxpart3,idxpart4)(a,b) values(1,2),(150,20);
replace into idxpart2 partition(idxpart3,idxpart4) set a = 1;
replace into idxpart2 partition(idxpart3)(a,b) values(1,2),(150,20);--error
select * from idxpart2;
select * from idxpart3;
select * from idxpart4;
drop table idxpart;

create table text_hashp (a text primary key) partition by hash (a);
create table text_hashp0 partition of text_hashp for values with (modulus 2, remainder 0);
create table text_hashp1 partition of text_hashp for values with (modulus 2, remainder 1);
insert into text_hashp values('aa');
insert into text_hashp values('zz');
insert into text_hashp partition(text_hashp0,text_hashp1)(a) values('bb'),('yy');
insert into text_hashp partition(text_hashp0,text_hashp1)(a) values('bb'),('zzzzz') on duplicate key update a = 'cc';
replace into text_hashp partition(text_hashp0,text_hashp1) values('bb'),('yy');
replace into text_hashp partition(text_hashp0) values('bb'),('zz');--error
select * from text_hashp;
select * from text_hashp0;
select * from text_hashp1;
drop table text_hashp;

--BUG#Z1622
drop table if exists test;
CREATE TABLE test (id INT PRIMARY KEY, name VARCHAR(50) UNIQUE);
REPLACE INTO test VALUES (1, 'Alice');
REPLACE INTO test VALUES (2, 'Bob');
REPLACE INTO test VALUES (1, 'Duplicate Alice');
REPLACE INTO test VALUES (4, 'Bob');
select * from test;
REPLACE INTO test VALUES (1, 'Bob');
select * from test;
drop table test;
--BUG#Z1626
CREATE TABLE test2(a int , b varchar(50));
REPLACE INTO test2(a,b) VALUES(1);
REPLACE INTO test2(a,b) VALUES(1,'aa',3);
REPLACE INTO test2 VALUES(3);
REPLACE INTO test2 VALUES(4,'aa',3);
REPLACE INTO test2(a,b) VALUES(1,'aa',1),(2,'bb',1),(3,'cc',1);
REPLACE INTO test2(a,b) VALUES(1,'aa',1),(2,'bb'),(3,'cc');
drop table test2;