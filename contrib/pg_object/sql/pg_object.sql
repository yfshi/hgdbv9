/*
 * This sql file is a test of the system table pg_object, 
 * storing, modifying and deleting operations.
 */

set datestyle to 'ISO, YMD';
set pg_object.enable to on;
/* Test tables and views */

set timezone to prc;
/*
 * test 1:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int);
select * from pg_object;

/*
 * test 1:
 *		step 2
 * create view and query the pg_object table.
 */
drop table test cascade;
select * from pg_object;


/*
 * test 2:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int);
select * from pg_object;

/*
 * test 2:
 *		step 2
 * alter table name and query the pg_object table.
 */
alter table test rename to newtest;
select * from pg_object;

/*
 * test 2:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table newtest;
select * from pg_object;


/*
 * test 3:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int, name text, addr text);
select * from pg_object;

/*
 * test 3:
 *		step 2
 * alter table schema and query the pg_object table.
 */
create schema mysql;
alter table test set schema mysql ;
select * from pg_object;

/*
 * test 3:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table mysql.test;
select * from pg_object;

/*
 * test 4:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int);
select * from pg_object;

/*
 * test 4:
 *		step 2
 * create view and query the pg_object table.
 */
create view vtest as select * from test;
select * from pg_object;

/*
 * test 4:
 *		step 3
 * alter view name and query the pg_object table.
 */
alter view vtest rename to newvtest;
select * from pg_object;

/*
 * test 4:
 *		step 4
 * alter view schema and query the pg_object table.
 */
alter view newvtest set schema mysql;
select * from pg_object;

/*
 * test 4:
 *		step 5
 * drop view and query the pg_object table.
 */
drop view mysql.newvtest;
select * from pg_object;

/*
 * test 4:
 *		step 6
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/*
 * test 5:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int);
select * from pg_object;

/*
 * test 5:
 *		step 2
 * create view and query the pg_object table.
 */
create view vtest as select * from test;
select * from pg_object;

/*
 * test 5:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table test cascade;
select * from pg_object;


/* Test functions and stored procedures below */

/*
 * test 1:
 *		step 1
 * create function and query the pg_object table.
 */
CREATE OR REPLACE FUNCTION testfunc(num INT)
RETURNS INT
as $$
DECLARE
i int;
a text;
BEGIN
	i = 0;
	num = num +1;
	a = function1(112);
	i = function2(num);
	return i;
END;
$$ LANGUAGE plpgsql;
/

select * from pg_object;

/*
 * test 1:
 *		step 2
 * alter function schema and query the pg_object table.
 */
alter function testfunc set schema mysql;
select * from pg_object;

/*
 * test 1:
 *		step 3
 * alter function name and query the pg_object table.
 */
alter function mysql.testfunc rename to newtestfunc;
select * from pg_object;

/*
 * test 1:
 *		step 4
 * drop function and query the pg_object table.
 */
drop function mysql.newtestfunc ;
select * from pg_object;

/*
 * test 2:
 *		step 1
 * create procedure and query the pg_object table.
 */
CREATE OR REPLACE PROCEDURE testproc(num INT)
as $$
DECLARE
i int;
a text;
BEGIN
	i = 0;
	num = num +1;
	a = function1(112);
	i = function2(num);
	raise notice ' i = %', i;
END;
$$ LANGUAGE plpgsql;
/
/*
 * test 2:
 *		step 2
 * alter procedure schema and query the pg_object table.
 */
alter procedure testproc set schema mysql;
select * from pg_object;

/*
 * test 2:
 *		step 3
 * alter procedure name and query the pg_object table.
 */
alter procedure mysql.testproc rename to newtestproc;
select * from pg_object;

/*
 * test 2:
 *		step 4
 * drop procedure and query the pg_object table.
 */
drop procedure mysql.newtestproc ;
select * from pg_object;


/* Test database below */
set ivorysql.compatible_mode to pg;
/*
 * test 1:
 *		step 1
 * create database and query the pg_object table.
 */
create database test;
select * from pg_object;

/*
 * test 1:
 *		step 2
 * drop database and query the pg_object table.
 */
drop database test;
select * from pg_object;

set ivorysql.compatible_mode to oracle;

/* The following tests are all operations on columns. */

/*
 * test 1:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int, name text);
select * from pg_object;

/* 
 *test 1:
 *		step 2
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/*
 * test 2:
 *		step 1
 *create table and query the pg_object table.
 */
create table test(id int, name text);
select * from pg_object;

/*
 * test 2:
 *		step 2
 * Modify the test table column name and query the pg_object table.
 */
alter table test rename column id to idd;
select * from pg_object;

/*
 * test 2:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/* 
 * test 3:
 *		step 1
 *create table and query the pg_object table.
 */
create table test(id int, name text);
select * from pg_object;

/* 
 * test 3:
 *		step 2
 * Add columns to the test table and query the pg_object table.
 */
alter table test add column addr text, add column mflag bool;
select * from pg_object;

/*
 * test 3:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/* 
 * test 4:
 *		step 1
 *create table and query the pg_object table.
 */
create table test(id int, name text);
select * from pg_object;

/* 
 * test 4:
 *		step 2
 * drop columns to the test table and query the pg_object table.
 */
alter table test drop column name;
select * from pg_object;

/* 
 * test 4:
 *		step 3
 * Add columns to the test table and query the pg_object table.
 */
alter table test add column name text, add column addr text, add column mflag bool;
select * from pg_object;

/* 
 * test 4:
 *		step 4
 * drop columns to the test table and query the pg_object table.
 */
alter table test drop column name, drop column mflag;
select * from pg_object;

/*
 * test 4:
 *		step 5
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/* 
 * test 5:
 *		step 1
 * create table and query the pg_object table.
 */
create table test(id int, name text);
select * from pg_object;

/* 
 * test 5:
 *		step 2
 * change cloumn data type and query the pg_object table.
 */
alter table test alter column id set data type numeric;
select * from pg_object;

/*
 * test 5:
 *		step 3
 * drop table and query the pg_object table.
 */
drop table test;
select * from pg_object;

/* The following tests are all schema. */

/*
 * test 1:
 *		step 1
 * create chema and query the pg_object table
 */
create schema aa;
select * from pg_object ;

/*
 * test 1:
 *		step 2
 * rename schema and query the pg_object table
 */
alter schema aa rename to bb;
select * from pg_object ;

/*
 * test 1:
 *		step 3
 * drop chema and query the pg_object table
 */
drop schema bb;
drop schema mysql;
select * from pg_object ;


set pg_object.enable to on;