create table t1 (id integer, x integer);
insert into t1 values(0, 0);
set autocommit=0;
update t1 set x=1 where id = 0;
select * from t1;
commit;
drop table t1;

create table t1 (id integer, x integer);
create table t2 (b integer, a integer);
insert into t1 values(0, 0), (300, 300);
insert into t2 values(0, 0), (1, 20), (2, 30);
commit;
select * from t2;
select * from t1;
update t2 set a=2 where b = 0;
select * from t2;
update t1 set x=2 where id = 0;
update t1 set x=1 where id = 0;
select * from t1;
commit;
select * from t1;
commit;
drop table t1, t2;
commit;
SET autocommit= 1;

create table t1(a int not null, b int, primary key(a));
insert into t1 values(1,1),(2,2),(3,1),(4,2),(5,1),(6,2),(7,3);
commit;
set autocommit = 0;
select * from t1;
update t1 set b = 5 where b = 1;
set autocommit = 0;
select * from t1 where a = 2 and b = 2 for update;
commit;
SET autocommit= 1;

SET autocommit=0;
CREATE USER u1;
CREATE ROLE r1;
RENAME USER u1 TO u2;
RENAME USER r1 TO r2;;
-- DROP ROLE u2;
-- DROP USER u2;
ROLLBACK;
CREATE USER u1;
CREATE ROLE r1;
RENAME USER u1 TO u2;
RENAME USER r1 TO r2;;
DROP USER u2;
DROP USER r2;
commit;
SET autocommit= 1;

drop table if exists test_table;
SET autocommit = 1;
CREATE TABLE test_table (a text);
CREATE DATABASE test_drop;
INSERT INTO test_table values('aaaaa');
SELECT * FROM test_table;
ROLLBACK;
SELECT * FROM test_table;

SET autocommit = 0;
-- DML
-- rollback the insert statement
INSERT INTO test_table values('bbbbb');
SELECT * FROM test_table;
ROLLBACK;
SELECT * FROM test_table;

-- commit the insert statement
INSERT INTO test_table values('ccccc');
SELECT * FROM test_table;
COMMIT;
SELECT * FROM test_table;

-- commit the insert statement auto
INSERT INTO test_table values('ddddd');
SELECT * FROM test_table;
SET autocommit = 1;
SELECT * FROM test_table;

SET autocommit = 0;
-- DDL
-- rollback the create table statement
CREATE TABLE test_a (a text);
INSERT INTO test_a values('aaaaa');
SELECT * FROM test_a;
ROLLBACK;
SELECT * FROM test_a;
COMMIT;

-- commit the create table statement
CREATE TABLE test_b (a text);
INSERT INTO test_b values('aaaaa');
SELECT * FROM test_b;
COMMIT;
SELECT * FROM test_b;

-- commit the create table statement auto
CREATE TABLE test_c (a text);
INSERT INTO test_c values('aaaaa');
SELECT * FROM test_c;
SET autocommit = 1;
SELECT * FROM test_c;

-- set autocommit = 0 in a transaction block
SET autocommit = 1;
TRUNCATE test_table;
BEGIN;
INSERT INTO test_table values('aaaaa');
SET autocommit = 0;
SET autocommit = 1;
SHOW autocommit;
SELECT * FROM test_table;

-- only set autocommit = 1 cannot commit transaction
BEGIN;
INSERT INTO test_table values('bbbbb');
SET autocommit = 1;
ROLLBACK;
SELECT * FROM test_table;

-- set autocommit = 0 and rollback
BEGIN;
INSERT INTO test_table values('ccccc');
SET autocommit = 0;
ROLLBACK;
SELECT * FROM test_table;
SHOW autocommit;

SET autocommit = 1;

begin;
create table t (c int);
set autocommit = true;
rollback;
create table t (c int);
show autocommit;

begin;
insert into t values (1);
set autocommit = false;
rollback;
select * from t;
show autocommit;

set autocommit = true;
insert into t values (2);
set autocommit = false;
rollback;
select * from t;
show autocommit;

insert into t values (3);
set autocommit = true;
rollback;
select * from t;
show autocommit;

START TRANSACTION;
insert into t values (4);
rollback;
select * from t;

START TRANSACTION;
insert into t values (4);
commit;
select * from t;

START TRANSACTION;
insert into t values (4);
set autocommit = false;
rollback;
select * from t;
show autocommit;

START TRANSACTION;
insert into t values (5);
set autocommit = true;
rollback;
select * from t;
show autocommit;

set autocommit = false;
insert into t values (6);
savepoint sp1;
select * from t;
insert into t values (7);
rollback to savepoint sp1;
select * from t;
commit;

insert into t values (7);
savepoint sp1;
select * from t;
insert into t values (8);
release savepoint sp1;
rollback to savepoint sp1;
select * from t;
commit;

insert into t values (9);
savepoint sp1;
select * from t;
insert into t values (10);
rollback to savepoint sp1;
select * from t;
set autocommit = true;

drop table if exists t;
create table t (c int);
set autocommit = false;
insert into t values (1);
SELECT * FROM t;
prepare transaction 'foo';
SELECT * FROM t1;
set autocommit = true;
commit prepared 'foo';
select * from t;
drop table t;

show div_precision_increment;
begin;
set div_precision_increment = 29;
rollback;
show div_precision_increment;
set div_precision_increment = 4;


-- pl testcase
SET autocommit = false;

DROP TABLE IF EXISTS test_autocommit CASCADE;
CREATE TABLE test_autocommit (
	id INT PRIMARY KEY,
	val TEXT
);

-- testcase1
CREATE OR REPLACE PROCEDURE proc_insert(p_id INT, p_val TEXT)
LANGUAGE plpgsql AS $$
BEGIN
	INSERT INTO test_autocommit VALUES (p_id, p_val);
END;
$$;

TRUNCATE test_autocommit;
COMMIT;

CALL proc_insert(1, 'uncommitted');
SELECT * FROM test_autocommit ORDER BY id;

ROLLBACK;
SELECT * FROM test_autocommit ORDER BY id;  -- empty

CALL proc_insert(1, 'committed');
COMMIT;
SELECT * FROM test_autocommit ORDER BY id;

TRUNCATE test_autocommit;
COMMIT;
DROP PROCEDURE proc_insert;

-- testcase 2：
CREATE OR REPLACE PROCEDURE proc_insert_error()
LANGUAGE plpgsql AS $$
BEGIN
	INSERT INTO test_autocommit VALUES (1, 'first');
	INSERT INTO test_autocommit VALUES (1, 'duplicate');  -- primary key conflict
END;
$$;

TRUNCATE test_autocommit;
COMMIT;

CALL proc_insert_error();  -- error
SELECT * FROM test_autocommit ORDER BY id;  -- error
ROLLBACK;
SELECT * FROM test_autocommit ORDER BY id;  -- empty

DROP PROCEDURE proc_insert_error;

-- testcase 3
CREATE OR REPLACE PROCEDURE proc_in_block(p_id INT, p_val TEXT)
LANGUAGE plpgsql AS $$
BEGIN
	INSERT INTO test_autocommit VALUES (p_id, p_val);
END;
$$;

TRUNCATE test_autocommit;
COMMIT;

BEGIN;
	CALL proc_in_block(1, 'inside_block');
COMMIT;
SELECT * FROM test_autocommit ORDER BY id; -- not empty

TRUNCATE test_autocommit;
COMMIT;

BEGIN;
	CALL proc_in_block(2, 'rollback_inside');
ROLLBACK;
SELECT * FROM test_autocommit ORDER BY id;  -- empty

DROP PROCEDURE proc_in_block;


-- testcase 4：autocommit=off
TRUNCATE test_autocommit;
COMMIT;

DO $$
BEGIN
	INSERT INTO test_autocommit VALUES (1, 'from_do');
END;
$$;
SELECT * FROM test_autocommit ORDER BY id;
ROLLBACK;
SELECT * FROM test_autocommit ORDER BY id;  -- empty

DO $$
BEGIN
	INSERT INTO test_autocommit VALUES (1, 'from_do_committed');
END;
$$;
COMMIT;
SELECT * FROM test_autocommit ORDER BY id;

-- error
TRUNCATE test_autocommit;
COMMIT;

DO $$
BEGIN
	INSERT INTO test_autocommit VALUES (1, 'first');
	INSERT INTO test_autocommit VALUES (1, 'duplicate');
END;
$$;  -- error
SELECT * FROM test_autocommit ORDER BY id;  -- empty
ROLLBACK;


-- testcase 5
CREATE OR REPLACE PROCEDURE proc_error_handled()
LANGUAGE plpgsql AS $$
BEGIN
	INSERT INTO test_autocommit VALUES (1, 'first');
	INSERT INTO test_autocommit VALUES (1, 'duplicate');
EXCEPTION
	WHEN unique_violation THEN
		RAISE NOTICE 'Duplicate caught, transaction continues';
		INSERT INTO test_autocommit VALUES (2, 'after_exception');
END;
$$;

TRUNCATE test_autocommit;
COMMIT;

CALL proc_error_handled();
SELECT * FROM test_autocommit ORDER BY id;
COMMIT;
SELECT * FROM test_autocommit ORDER BY id;

TRUNCATE test_autocommit;
COMMIT;
DROP PROCEDURE proc_error_handled;

-- testcase 6: set autocommit values off to on 
CREATE OR REPLACE PROCEDURE proc_insert(p_id INT, p_val TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    INSERT INTO test_autocommit VALUES (p_id, p_val);
END;
$$;
CALL proc_insert(1, 'auto_committed');
SET autocommit = on;
SELECT * FROM test_autocommit ORDER BY id;


SET autocommit = off;
CALL proc_insert(2, 'need_commit');
SELECT * FROM test_autocommit ORDER BY id;	-- two rows
ROLLBACK;  
SELECT * FROM test_autocommit ORDER BY id;  -- only id=1

-- clean
TRUNCATE test_autocommit;
COMMIT;
DROP PROCEDURE proc_insert;

-- clean
DROP TABLE test_autocommit CASCADE;
SET autocommit = on;


DROP TABLE IF EXISTS t;
create table t(a int);


CREATE OR REPLACE PROCEDURE func1()
LANGUAGE plpgsql AS $$
BEGIN
	insert into t values (0);
	commit;
	insert into t values (1);
END;
$$;

CREATE OR REPLACE PROCEDURE func2()
LANGUAGE plpgsql AS $$
BEGIN
	insert into t values (0);
	set autocommit = 1;
	insert into t values (1);
END;
$$;

CREATE OR REPLACE PROCEDURE func3()
LANGUAGE plpgsql AS $$
BEGIN
	insert into t values (0);
	set autocommit = 0;
	insert into t values (1);
END;
$$;

-- case 1
set autocommit = 1;
truncate t;
set autocommit = 0;

begin;
call func1();
select * from t; -- 2 row
rollback;
select * from t; -- 1 row

-- case 2
set autocommit = 1;
truncate t;

begin;
call func1();
select * from t; -- 2 row
rollback;
select * from t; -- 2 row

-- case 3
set autocommit = 1;
truncate t;
set autocommit = 0;

begin;
call func2();
select * from t; -- 2 row
show autocommit;
rollback;
select * from t; -- 2 row
show autocommit;
insert into t values (2);
rollback;
select * from t; 

-- case 4
set autocommit = 1;
truncate t;

begin;
call func2();
select * from t; -- 2 row
show autocommit;
rollback;
select * from t; -- 0 row
show autocommit;
insert into t values (2);
rollback;
select * from t; 

-- case 5
set autocommit = 1;
truncate t;
set autocommit = 0;

begin;
call func3();
select * from t; -- 2 rows
show autocommit;
rollback;
select * from t; -- 0 row
show autocommit;
insert into t values (2);
rollback;
select * from t; 

-- case 6
set autocommit = 1;
truncate t;

begin;
call func3();
select * from t; -- 2 row
show autocommit;
rollback;
select * from t; -- 0 row
show autocommit;
insert into t values (2);
rollback;
select * from t;

-- from mysql testcase file sp_trans.test
delimiter |;
--disable_warnings
drop function if exists bug13825_0|
drop function if exists bug13825_1|
drop function if exists bug13825_2|
drop function if exists bug13825_3|
drop function if exists bug13825_4|
drop function if exists bug13825_5|
drop procedure if exists bug13825_0|
drop procedure if exists bug13825_1|
drop procedure if exists bug13825_2|
drop table if exists t1|
--enable_warnings
create table t1 (i int) engine=innodb|
create table t2 (i int) engine=innodb|
create function bug13825_0() returns int
begin
  rollback to savepoint x;
  return 1;
end|
create function bug13825_1() returns int
begin
  release savepoint x;
  return 1;
end|
create function bug13825_2() returns int
begin
  insert into t1 values (2);
  savepoint x;
  insert into t1 values (3);
  rollback to savepoint x;
  insert into t1 values (4);
  return 1;
end|
create procedure bug13825_0()
begin
  rollback to savepoint x;
end|
create procedure bug13825_1()
begin
  release savepoint x;
end|
create procedure bug13825_2()
begin
  savepoint x;
end|
insert into t2 values (1)|
create trigger t2_bi before insert on t2 for each row
  rollback to savepoint x|
create trigger t2_bu before update on t2 for each row
  release savepoint x|
create trigger t2_bd before delete on t2 for each row
begin
  insert into t1 values (2);
  savepoint x;
  insert into t1 values (3);
  rollback to savepoint x;
  insert into t1 values (4);
end|
create function bug13825_3(rb int) returns int
begin
  insert into t1 values(1);
  savepoint x;
  insert into t1 values(2);
  if rb then
    rollback to savepoint x;
  end if;
  insert into t1 values(3);
  return rb;
end|
create function bug13825_4() returns int
begin
  savepoint x;
  insert into t1 values(2);
  rollback to savepoint x;
  return 0;
end|
create function bug13825_5(p int) returns int
begin
  savepoint x;
  insert into t2 values(p);
  rollback to savepoint x;
  insert into t2 values(p+1);
  return p;
end|
set autocommit= 0|
# Test of savepoint level handling for stored functions and triggers
begin |
insert into t1 values (1)|
savepoint x|
--error ER_SP_DOES_NOT_EXIST
set @a:= bug13825_0()|
--error ER_SP_DOES_NOT_EXIST
insert into t2 values (2)|
--error ER_SP_DOES_NOT_EXIST
set @a:= bug13825_1()|
--error ER_SP_DOES_NOT_EXIST
update t2 set i = 2|
set @a:= bug13825_2()|
select * from t1|
rollback to savepoint x|
select * from t1|
delete from t2|
select * from t1|
rollback to savepoint x|
select * from t1|
# Of course savepoints set in function should not be visible from its caller
release savepoint x|
set @a:= bug13825_2()|
select * from t1|
--error ER_SP_DOES_NOT_EXIST
rollback to savepoint x|
delete from t1|
commit|
# Test of savepoint level handling for stored procedures
begin|
insert into t1 values (5)|
savepoint x|
insert into t1 values (6)|
call bug13825_0()|
select * from t1|
call bug13825_1()|
--error ER_SP_DOES_NOT_EXIST
rollback to savepoint x|
savepoint x|
insert into t1 values (7)|
call bug13825_2()|
rollback to savepoint x|
select * from t1|
delete from t1|
commit|
set autocommit= 1|
# Let us test that savepoints work inside of functions
# even in auto-commit mode
select bug13825_3(0)|
select * from t1|
delete from t1|
select bug13825_3(1)|
select * from t1|
delete from t1|
# Curious case: rolling back to savepoint which is set by first
# statement in function should not rollback whole transaction.
set autocommit= 0|
begin|
insert into t1 values (1)|
set @a:= bug13825_4()|
select * from t1|
delete from t1|
commit|
set autocommit= 1|
# Other curious case: savepoint in the middle of statement
drop table t2|
create table t2 (i int) engine=innodb|
insert into t1 values (1), (bug13825_5(2)), (3)|
select * from t1|
select * from t2|
# Cleanup
drop function bug13825_0|
drop function bug13825_1|
drop function bug13825_2|
drop function bug13825_3|
drop function bug13825_4|
drop function bug13825_5|
drop procedure bug13825_0|
drop procedure bug13825_1|
drop procedure bug13825_2|
drop table t1, t2|


#
# BUG#14840: CONTINUE handler problem
#
--disable_warnings
drop table if exists t3|
drop procedure if exists bug14840_1|
drop procedure if exists bug14840_2|
--enable_warnings

create table t3
(
  x int,
  y int,
  primary key (x)
) engine=InnoDB|

# This used to hang the client since the insert returned with an
# error status (left over from the update) even though it succeeded,
# which caused the execution to end at that point.
create procedure bug14840_1()
begin
  declare err int default 0;
  declare continue handler for sqlexception
    set err = err + 1;

  start transaction;
  update t3 set x = 1, y = 42 where x = 2;
  insert into t3 values (3, 4711);
  if err > 0 then
    rollback;
  else
    commit;
  end if;
  select * from t3;
end|

# A simpler (non-transactional) case: insert at select should be done
create procedure bug14840_2()
begin
  declare err int default 0;
  declare continue handler for sqlexception
    begin
      set err = err + 1;
      select err as 'Ping';
    end;

  update t3 set x = 1, y = 42 where x = 2;
  update t3 set x = 1, y = 42 where x = 2;
  insert into t3 values (3, 4711);
  select * from t3;
end|

insert into t3 values (1, 3), (2, 5)|
call bug14840_1()|

delete from t3|
insert into t3 values (1, 3), (2, 5)|
call bug14840_2()|

drop procedure bug14840_1|
drop procedure bug14840_2|
drop table t3|

delimiter |;


-- Test that autocommit cannot be set inside stored procedures, functions, or anonymous blocks
-- Create a test table
CREATE TABLE autocommit_test (id int, value text);
-- Test 1: Try to set autocommit in a SQL function (should fail)
CREATE FUNCTION test_set_autocommit_in_function() RETURNS void AS $$
    SET autocommit TO on;
$$ LANGUAGE SQL;
-- This should fail with error about setting autocommit inside function
SELECT test_set_autocommit_in_function();
-- Test 2: Try to set autocommit in a PL/pgSQL function (should fail)
CREATE OR REPLACE FUNCTION test_set_autocommit_in_plpgsql_function() RETURNS void AS $$
BEGIN
    EXECUTE 'SET autocommit TO on';
END;
$$ LANGUAGE plpgsql;
-- This should fail
SELECT test_set_autocommit_in_plpgsql_function();
-- Test 3: Try to set autocommit in a stored procedure (should fail)
CREATE PROCEDURE test_set_autocommit_in_procedure() AS $$
BEGIN
    EXECUTE 'SET autocommit TO off';
END;
$$ LANGUAGE plpgsql;
-- This should fail
CALL test_set_autocommit_in_procedure();
-- Test 4: Try to set autocommit in an anonymous block (should fail)
DO $$
BEGIN
    EXECUTE 'SET autocommit TO on';
END;
$$;
-- Test 5: Try to set autocommit in nested procedure call (should fail)
CREATE PROCEDURE inner_proc() AS $$
BEGIN
    EXECUTE 'SET autocommit TO on';
END;
$$ LANGUAGE plpgsql;
CREATE PROCEDURE outer_proc() AS $$
BEGIN
    CALL inner_proc();
END;
$$ LANGUAGE plpgsql;
-- This should fail when inner_proc tries to set autocommit
CALL outer_proc();
-- Test 6: Verify that setting other GUC parameters still works in procedures
CREATE PROCEDURE test_set_other_guc() AS $$
BEGIN
    EXECUTE 'SET work_mem TO ''64MB''';
    INSERT INTO autocommit_test VALUES (1, 'test');
END;
$$ LANGUAGE plpgsql;
-- This should succeed
CALL test_set_other_guc();
SELECT * FROM autocommit_test;
-- Test 7: Verify that setting autocommit outside of procedures still works
SET autocommit TO on;
SHOW autocommit;
SET autocommit TO off;
SHOW autocommit;
-- Test 8: Try using SET LOCAL in a transaction within a procedure (should fail)
CREATE PROCEDURE test_set_local_autocommit() AS $$
BEGIN
    EXECUTE 'SET LOCAL autocommit TO on';
END;
$$ LANGUAGE plpgsql;
-- This should fail
CALL test_set_local_autocommit();

-- Cleanup
DROP FUNCTION IF EXISTS test_set_autocommit_in_function();
DROP FUNCTION IF EXISTS test_set_autocommit_in_plpgsql_function();
DROP PROCEDURE IF EXISTS test_set_autocommit_in_procedure();
DROP PROCEDURE IF EXISTS inner_proc();
DROP PROCEDURE IF EXISTS outer_proc();
DROP PROCEDURE IF EXISTS test_set_other_guc();
DROP PROCEDURE IF EXISTS test_set_local_autocommit();
DROP TABLE IF EXISTS autocommit_test;