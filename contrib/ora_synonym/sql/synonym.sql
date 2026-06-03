--
-- SYNONYMS
--

-- synonym syntax support
CREATE SYNONYM a for aa;
CREATE PUBLIC SYNONYM a for aa;
CREATE OR REPLACE EDITIONABLE PUBLIC SYNONYM a FOR bb;
DROP SYNONYM a Force;
DROP PUBLIC SYNONYM a Force;

--support table and views
CREATE TABLE test(id int, des text);
INSERT INTO test VALUES (1,'111'),(2,'222');
CREATE SYNONYM test_syn FOR test;
SELECT * FROM test_syn;
DROP SYNONYM test_syn;

--normal view
CREATE VIEW test_view AS SELECT * FROM TEST;
--materialized view
CREATE MATERIALIZED VIEW test_mview AS SELECT * FROM TEST;
CREATE SYNONYM test_view_syn FOR test_view;
CREATE SYNONYM test_mview_syn FOR test_mview;

SELECT * FROM test_view_syn;
SELECT * FROM test_mview_syn;
DROP SYNONYM test_view_syn;
DROP SYNONYM test_mview_syn;
DROP VIEW test_view;
DROP MATERIALIZED VIEW test_mview;
DROP TABLE test;

--supoort sequence
CREATE SEQUENCE seq;
CREATE SYNONYM seq_syn FOR seq;
SELECT seq_syn.NEXTVAL FROM DUAL;
DROP SYNONYM seq_syn Force;
DROP SEQUENCE seq;

--suport function
CREATE OR REPLACE FUNCTION calculate_area(p_number in number)
return number
is
begin
	return p_number * p_number;
end;
/
CREATE SYNONYM func_syn FOR calculate_area;
SELECT func_syn(5) FROM DUAL;
DROP SYNONYM func_syn Force;
DROP FUNCTION calculate_area;

--check loop chain
CREATE SYNONYM a FOR b;
CREATE SYNONYM b FOR a;
SELECT * FROM a;--ERROR
DROP SYNONYM a;
DROP SYNONYM b;

-- support procedure
CREATE OR REPLACE PROCEDURE greet_user AS
BEGIN
	RAISE INFO 'HELLO, WELCOME TO IVORYSQL';
END;
/

CREATE SYNONYM greet_syn FOR greet_user;

BEGIN
	greet_syn;
END;
/
DROP SYNONYM greet_syn;
DROP PROCEDURE greet_user;

--support user-define type
CREATE TYPE student as (id int, name text);
CREATE SYNONYM stu_syn for student;
CREATE TABLE test(id stu_syn);
select * from test;
DROP TABLE test;
DROP SYNONYM stu_syn;
DROP TYPE student;

-- synonym name same as objname at same namespace
CREATE SYNONYM a FOR a;--ERROR
CREATE SYNONYM aa.aa FOR aa.aa;--ERROR

-- object is invalid
CREATE SYNONYM a FOR b;
SELECT * FROM a;--error
DROP SYNONYM a;

-- only check synonym namespace
CREATE SYNONYM s1.bb for s2.bb;--error

-- test same name for object, private synonym and public synonym
create table test1(id int);
create table test2(id int);
create table test3(id int);
insert into test1 values (1);
insert into test2 values (2);
insert into test3 values (3);
create synonym test1 for test2;
create public synonym test1 for test3;
select * from test1;-- reference object
drop table test1;
drop table test2;
drop table test3;
drop synonym test1;
drop public synonym test1;

-- test COMMNET statement reference synonym
CREATE TABLE tab(id int, name varchar(20));
CREATE SYNONYM syntab FOR tab;
COMMENT ON TABLE syntab IS 'Employee data';
DROP TABLE tab;
DROP SYNONYM syntab;

-- GRANT and REVOKE statement reference synonym
create role user1;
CREATE TABLE tab(id int,name  varchar(20));
CREATE SYNONYM syntab for tab;
GRANT SELECT ON syntab to user1;
REVOKE SELECT ON syntab from user1;
DROP TABLE tab;
DROP SYNONYM syntab;
DROP ROLE user1;

-- reference synonym in SELECT,INSERT,UPDATE and DELETE statement
CREATE TABLE t1(id int, name varchar(20));
CREATE OR REPLACE SYNONYM syn1 FOR t1;
INSERT INTO syn1 VALUES (1, 'a');
INSERT INTO syn1 VALUES (2, 'b');
UPDATE syn1 SET id = 3 WHERE id = 1;
DELETE syn1 WHERE id = 2;
SELECT * FROM syn1;
DROP SYNONYM  syn1;
DROP TABLE t1;

-- reference synonym in EXPLAIN statement
CREATE TABLE t1(id int, name varchar(20));
CREATE OR REPLACE SYNONYM syn1 for t1;
EXPLAIN SELECT * FROM t1;
DROP SYNONYM syn1;
DROP TABLE t1;

-- reference synonym in LOCK TABLE statement
CREATE TABLE t1(id int, name varchar(20));
CREATE OR REPLACE SYNONYM syn1 FOR t1;
BEGIN WORK;
LOCK TABLE syn1 IN SHARE MODE NOWAIT;
INSERT INTO syn1 VALUES(0, 'new');
COMMIT WORK;
DROP SYNONYM syn1;
DROP TABLE t1;

-- reference synonym in MERGE statement
CREATE TABLE people_source(
	person_id INTEGER NOT NULL PRIMARY KEY,
	first_name VARCHAR(20) NOT NULL,
	last_name VARCHAR(20) NOT NULL,
	title VARCHAR(10) NOT NULL
);
CREATE TABLE people_target(
	person_id INTEGER NOT NULL PRIMARY KEY,
	first_name VARCHAR(20) NOT NULL,
	last_name VARCHAR(20) NOT NULL,
	title VARCHAR(10) NOT NULL
);
INSERT INTO people_target VALUES(1, 'John', 'Smith', 'Mr');
INSERT INTO people_target VALUES(2, 'Alice', 'jones', 'Mrs');
INSERT INTO people_target VALUES(3, 'Tony', 'james', 'Mr');
INSERT INTO people_source VALUES(2, 'Alice', 'jones', 'Mrs');
INSERT INTO people_source VALUES(3, 'Jane', 'Doe', 'Miss');
INSERT INTO people_source VALUES(4, 'Dave', 'Brown', 'Mr');
CREATE OR REPLACE SYNONYM syn1 FOR people_source;
CREATE OR REPLACE SYNONYM syn2 FOR people_target;
MERGE INTO syn2 pt USING syn1 ps ON (pt.person_id = ps.person_id)
    WHEN MATCHED THEN UPDATE
		SET pt.first_name = ps.first_name,
			pt.last_name = ps.last_name,
			pt.title = ps.title;
DROP SYNONYM syn1;
DROP SYNONYM syn2;
DROP TABLE people_source;
DROP TABLE people_target;

-- reference synonym in CALL statement
CREATE TABLE tab(a int);
CREATE OR REPLACE PROCEDURE insert_data(a int) AS
BEGIN
	INSERT INTO tab VALUES (a);
END;
/
CREATE OR REPLACE SYNONYM synprod FOR insert_data;
CALL synprod(3);
DROP PROCEDURE insert_data;
DROP SYNONYM synprod;
DROP TABLE tab;

-- system function name same as synonym name
CREATE FUNCTION f1(a int) RETURN NUMBER AS
BEGIN
	RETURN a+1;
END;
/
CREATE SYNONYM abs FOR f1;
SELECT ABS(1) FROM DUAL;--will reference system function
DROP SYNONYM abs;
DROP FUNCTION f1;

-- test packages
CREATE SCHEMA lk;
CREATE OR REPLACE PACKAGE calc_pkg AS
    FUNCTION calculate_are(radius IN NUMBER) RETURN NUMBER;
	FUNCTION calculate_are(length IN NUMBER, width IN NUMBER) RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY calc_pkg AS
	FUNCTION calculate_are(radius IN NUMBER) RETURN NUMBER IS
	BEGIN
		RETURN 3.14 * radius * radius;
	END;
	FUNCTION calculate_are(length IN NUMBER, width IN NUMBER) RETURN NUMBER IS
	BEGIN
		RETURN length * width;
	END;
END;
/
CREATE OR REPLACE FUNCTION lk.calculate_are (p_number IN NUMBER)
RETURN NUMBER IS
BEGIN
	RETURN p_number * p_number;
END;
/
create synonym lk for calc_pkg;
select lk.calculate_are(1) from dual;--will reference function from package
DROP SYNONYM lk;
select lk.calculate_are(1) from dual;--reference schema object
DROP FUNCTION lk.calculate_are;
DROP PACKAGE calc_pkg;
DROP SCHEMA lk;

-- test FuncCall
create function funca(id int) return int
as
begin
	return id * id;
end;
/
create synonym a for funca;
create synonym b for a;
create synonym c for b;
select c(2) from dual;

drop synonym c;
drop synonym b;
drop synonym a;
drop function funca;

-- test synonym case sensitivity
create table t1 (id int);
create synonym "aa" for t1;
select * from "aa";
drop synonym "aa";
drop table t1;

-- test event trigger
CREATE OR REPLACE FUNCTION trg_on_create_drop_synonym()
RETURNS event_trigger AS $$
BEGIN
    RAISE NOTICE 'test_evernt_trigger:% %',tg_event,tg_tag;
END;
$$ LANGUAGE plpgsql;
/

CREATE event TRIGGER trg_after_create_drop_synonym
on ddl_command_start
EXECUTE procedure trg_on_create_drop_synonym();
create synonym a for b;
drop synonym a;
DROP EVENT TRIGGER trg_after_create_drop_synonym;
DROP FUNCTION trg_on_create_drop_synonym;

--/*Begin - Bug#Z7636 */
-- fix warnings related to the ddl sql about synonym when log_statement is ddl
set log_statement = 'DDL';
create synonym test for test1;
alter synonym test compile;
drop synonym test;
reset log_statement;

--/* Begin - 0A-2431 */
create table a (a int, b int);
insert into a values (1, 100);
create synonym syn_1 for a;
create or replace procedure syn_test_proc()
as
	b_value int;
begin
	select b into b_value from syn_1 where syn_1.a=1;
	raise info 'b_value is: %', b_value;
end;
/
call syn_test_proc();
drop procedure syn_test_proc;
drop synonym syn_1;
drop table a;
-- /* End - OA-2431 */
