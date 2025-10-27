--show variables
show global variables where VARIABLE_NAME = 'sql_mode';
show session variables where VARIABLE_NAME = 'sql_mode';
show global variables where VARIABLE_NAME like 'sql_mod%';
show session variables where VARIABLE_NAME like 'sql_mod%';
show variables where VARIABLE_NAME like 'sql_mod%';
show variables where VARIABLE_NAME like 'sql_mo%';

show global variables where VARIABLE_NAME = 'sql_quote_show_create';
show session variables where VARIABLE_NAME = 'sql_quote_show_create';
show variables where VARIABLE_NAME like 'sql_quote_show_%';
show variables where VARIABLE_NAME like 'sql_quote%';

--show global variables;

--show create database
create database aa;
show create database aa;
set sql_quote_show_create = false;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
reset sql_quote_show_create;
drop schema aa;

create database aa CHARACTER SET = UTF8;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

create database aa CHARACTER SET UTF8;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

CREATE database aa COLLATE  = "collatetest";
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

CREATE database aa COLLATE "collatetest";
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

--show databases
create database aa;
create database bb;
create database cc;
show databases where database = 'aa' or database = 'bb' or database = 'cc';
show databases like 'a%';
show databases like 'b%';
show databases like 'cc%';

show schemas where database = 'aa' or database = 'bb' or database = 'cc';
show schemas like 'a%';
show schemas like 'b%';
show schemas like 'cc%';

drop schema aa;
drop schema bb;
drop schema cc;
show databases where database = 'aa' or database = 'bb' or database = 'cc';
show databases like 'a%';
show databases like 'b%';
show databases like 'cc%';

show schemas where database = 'aa' or database = 'bb' or database = 'cc';
show schemas like 'a%';
show schemas like 'b%';
show schemas like 'c%';

--show columns
create table tb(id int);
create table test(id int, name text, addr text);
show full columns from tb;
show full fields in tb;
show full columns from tb from public;
show full columns from tb in public;

show full columns from tb where Field = 'name';
show full columns from tb like 'na%';
show full fields in tb like 'na%';
show full fields in tb where Field = 'name';

show columns from tb from public where Field = 'name';
show columns from tb in public like 'na%';
drop table tb;
drop table test;

--desc table
create table test(id int, name text, addr text);
desc test;
desc test id;
desc test 'na%';
describe test;
describe test id;
describe test 'na%';

create view vtest as select * from test;
desc vtest;
desc vtest id;
desc vtest 'na%';
drop view vtest;
drop table test;

--show create view
--create table tb(id int, name text);
--create view v_tb as select * from tb;
--create view v_tb1 as select id from tb;
--create view v_tb2 as select name from tb;
--show create view v_tb;
--show create view v_tb1;
--show create view v_tb2;
--drop view v_tb;
--drop view v_tb1;
--drop view v_tb2;
--drop table tb;

--create view vtb as select 1 + 1;
--show create view vtb;
--drop view vtb;

--show index
create table tb(id int, name text);
create index on tb(id);

show index from tb;
show indexes from tb;
show keys from tb;

show index in tb;
show indexes in tb;
show keys in tb;

show index from tb from public;
show index from tb from public where "Table" = 'tb';
show index from tb in public;
show index from tb in public where "Table" = 'tb';

show indexes from tb;
show indexes from tb from public where "Table" = 'tb';
show indexes from tb in public;
show indexes from tb in public where "Table" = 'tb';

show keys from tb;
show keys from tb from public where "Table" = 'tb';
show keys from tb in public;
show keys from tb in public where "Table" = 'tb';

drop table tb;

--
--show table status
--because have time, time is always change,so don't test these.
--

--create table tb(id int, name text);
--show table status;
--show table status from public;
--show table status in public;
--show table status from public like 'tb';
--show table status in public like 'tb';
--show table status from public where Name = 'tb';
--show table status in public where Name = 'tb';
--drop table tb;

--show tables;
create table tb(id int, name text);
create temporary table tb1(id int, name text);
create table logs (id serial primary key, message text) with (autovacuum_enabled = false);
create foreign data wrapper dummy;
create server s0 foreign data wrapper dummy;
create foreign table ft1 (
	c1 integer options ("param 1" 'val1') not null,
	c2 text options (param2 'val2', param3 'val3') check (c2 <> ''),
	c3 date,
	check (c3 between '1994-01-01'::date and '1994-01-31'::date)
) server s0 options (delimiter ',', quote '"', "be quoted" 'value');
create table tab_part_create (a int) partition by list (a);

show tables where Tables_public = 'tb';
show full tables where Tables_public = 'tb1';
show full tables from public where Tables_public = 'logs';
show full tables in public where Tables_public = 'ft1';
show full tables in public where Tables_public = 'tab_part_create';

drop table tb;
drop table tb1;
drop table logs;
drop foreign table ft1;
drop server s0;
drop foreign data wrapper dummy;
drop table tab_part_create;

--show triggers
--CREATE TABLE trig_tbl (id int, val text);
--CREATE FUNCTION trig_func()
--RETURNS trigger LANGUAGE plpgsql AS $$
--BEGIN
--  NEW.val := upper(NEW.val);
--  RETURN NEW;
--END;$$;
--CREATE TRIGGER before_ins BEFORE INSERT ON trig_tbl
--FOR EACH ROW EXECUTE FUNCTION trig_func();
--
--SHOW TRIGGERS from public;
--SHOW TRIGGERS in public like 'trig_tbl';
--SHOW TRIGGERS in public like 'trig_tb%';
--SHOW TRIGGERS in public where Trigger = 'before_ins';
--
--drop  TRIGGER before_ins ON trig_tbl;
--drop FUNCTION trig_func;
--drop TABLE trig_tbl;