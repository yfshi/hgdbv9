/********************************************
 *
 *	Compatible Oracle Trigger regression test
 *
 ********************************************/

drop table if exists empt;
CREATE TABLE empt (
	emp_id NUMBER PRIMARY KEY,
	name VARCHAR2(100),
	salary NUMBER,
	hire_date DATE
);

drop table if exists empt_logs;
CREATE TABLE empt_logs (
	log_id NUMBER,
	emp_id NUMBER,
	action VARCHAR2(100), -- 'INSERT', 'UPDATE', 'DELETE'
	log_date DATE
);

--
-- Test simple_dml trigger grammer
--
--  schema.trigger
create schema tscha;
create schema bschb;

CREATE OR REPLACE TRIGGER tscha.trgins
AFTER INSERT ON empt
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (1, :NEW.emp_id, 'tscha.trgins-> INSERT (a)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

CREATE OR REPLACE TRIGGER bschb.trgins
AFTER INSERT ON empt
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (2, :NEW.emp_id, 'bschb.trgins-> INSERT (b)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

select tgname from pg_trigger where tgname = 'trgins'; -- only one row
select objname from pg_object where objname = 'trgins'; -- only one row

INSERT INTO empt VALUES (7380,'JODEN',900, to_date('22-09-1989','dd-mm-yyyy'));

select * from empt;
select * from empt_logs; -- bschb.trgins will be fire

/* Begin - BUG#Z6516 */
DROP TRIGGER IF EXISTS tscha.vtgwschema;
/* End - BUG#Z6516 */

drop trigger trgins;
drop schema tscha;
drop schema bschb;

-- check schema report the error
CREATE OR REPLACE TRIGGER cschb.trgins2
AFTER INSERT ON empt
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (3, :NEW.emp_id, 'cschb.trgins-> INSERT (b)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

--
-- Test REFERENCING clause
--
CREATE OR REPLACE TRIGGER trgref
AFTER INSERT ON empt
REFERENCING NEW AS NEW_TABLE
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (4, :NEW_TABLE.emp_id, 'trgref-> INSERT (100)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

-- get trigger defination
SELECT pg_catalog.pg_get_triggerdef(t.oid, true) FROM pg_trigger t WHERE t.tgname = 'trgref';

INSERT INTO empt VALUES (7382,'REFERENCING',100, to_date('24-03-2025','dd-mm-yyyy'));

select * from empt;
select * from empt_logs;

--
-- Test alter trigger
--

-- disable
alter trigger trgref disable;
select tgname, tgenabled from pg_trigger where tgname = 'trgref';
INSERT INTO empt VALUES (7383,'ALTER trigger disable',500, to_date('24-03-2025','dd-mm-yyyy'));
select * from empt;
select * from empt_logs;

-- enable
alter trigger trgref enable;
select tgname, tgenabled from pg_trigger where tgname = 'trgref';
INSERT INTO empt VALUES (7384,'ALTER trigger enbale',600, to_date('24-03-2025','dd-mm-yyyy'));
select * from empt;
select * from empt_logs;

-- editionable only support grammer
alter trigger trgref editionable;

-- noneditionable only support grammer
alter trigger trgref noneditionable;

-- rename
alter trigger trgref rename to retrgins;

select tgname from pg_trigger where tgname = 'retrgins';
select objname from pg_object where objname = 'retrgins'; -- pg_object tuples will also be modified sync 

INSERT INTO empt VALUES (7385,'ALTER trigger rename',600, to_date('24-03-2025','dd-mm-yyyy'));
select * from empt;
select * from empt_logs;

-- compile
alter trigger retrgins compile;
INSERT INTO empt VALUES (7386,'ALTER trigger compile',700, to_date('24-03-2025','dd-mm-yyyy'));
select * from empt;
select * from empt_logs;

drop trigger retrgins;
select tgname from pg_trigger where tgname = 'retrgins';
select objname from pg_object where objname = 'retrgins';

--
-- Test FOLLOWS/PRECEDES clause
--

CREATE TRIGGER trgins1
AFTER INSERT ON empt
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (5, :NEW.emp_id, 'trgins1-> INSERT (1)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

CREATE TRIGGER tgprecedes
AFTER INSERT ON empt
FOR EACH ROW PRECEDES trgins1
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (6, :NEW.emp_id, 'tgprecedes-> INSERT (-1)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

CREATE TRIGGER trgins2
AFTER INSERT ON empt
FOR EACH ROW
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (7, :NEW.emp_id, 'trgins2-> INSERT (2)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

CREATE TRIGGER at1
AFTER INSERT ON empt
FOR EACH ROW FOLLOWS trgins1, trgins2
BEGIN
	INSERT INTO empt_logs (log_id, emp_id, action, log_date)
	VALUES (8, :NEW.emp_id, 'at1-> INSERT (3)', to_date('03-09-2025','dd-mm-yyyy'));
END;
/

INSERT INTO empt VALUES (7387,'TEST FOLLOWS',900, to_date('24-03-2025','dd-mm-yyyy'));

select * from empt;
select * from empt_logs;

drop trigger trgins1; -- error, report dependency

--clean
drop trigger at1;
drop trigger trgins1;
drop trigger trgins2;

drop table empt;
drop table empt_logs;

--
-- Test instead of trigger for view
--
drop table if exists departments;
CREATE TABLE departments (
	dept_id NUMBER,
	dept_name VARCHAR2(100)
);

drop table if exists employees;
CREATE TABLE employees (
	emp_id NUMBER,
	emp_name VARCHAR2(100),
	dept_id NUMBER
);

-- create view
CREATE VIEW employee_info AS
 SELECT e.emp_id, e.emp_name, d.dept_name
 FROM employees e
  JOIN departments d ON e.dept_id = d.dept_id;


CREATE OR REPLACE TRIGGER trg_emp_info_insert
INSTEAD OF INSERT ON employee_info
FOR EACH ROW
BEGIN
	DECLARE
		v_dept_id departments.dept_id%TYPE;
	BEGIN
		SELECT dept_id INTO v_dept_id FROM departments
		 WHERE dept_name = :NEW.dept_name;

		INSERT INTO employees(emp_id, emp_name, dept_id)
		 VALUES (:NEW.emp_id, :NEW.emp_name, v_dept_id);
	EXCEPTION
		WHEN NO_DATA_FOUND THEN
			RAISE_APPLICATION_ERROR(-20001, 'Department not found: ' || :NEW.dept_name);
	END;
END;
/

CREATE OR REPLACE TRIGGER trg_emp_info_update
INSTEAD OF UPDATE ON employee_info
FOR EACH ROW
BEGIN
	DECLARE
		v_dept_id departments.dept_id%TYPE;
	BEGIN
		SELECT dept_id INTO v_dept_id FROM departments
		 WHERE dept_name = :NEW.dept_name;

	UPDATE employees
	  SET emp_name = :NEW.emp_name,
		  dept_id = v_dept_id
	  WHERE emp_id = :OLD.emp_id;
	EXCEPTION
	  WHEN NO_DATA_FOUND THEN
		RAISE_APPLICATION_ERROR(-20002, 'Department not found: ' || :NEW.dept_name);
	END;
END;
/

CREATE TRIGGER trg_emp_info_delete
INSTEAD OF DELETE ON employee_info
FOR EACH ROW
BEGIN
	DELETE FROM employees
	WHERE emp_id = :OLD.emp_id;
END;
/

/* Begin - BUG#Z6177 */
CREATE OR REPLACE TRIGGER trg_emp_info_delete
INSTEAD OF DELETE ON employee_info
BEGIN
	DELETE FROM employees
	WHERE emp_id = :OLD.emp_id;
END;
/

/* Begin - BUG#Z6215 6216 */
select trigger_name, trigger_type, triggering_event, base_object_type, table_name
 from user_triggers where upper(trigger_name) = 'TRG_EMP_INFO_DELETE';
/* End - BUG#Z6215 6216 */
/* End - BUG#Z6177 */

INSERT INTO departments VALUES (10, 'HR');
INSERT INTO departments VALUES (20, 'IT');

-- fire insert trigger
INSERT INTO employee_info (emp_id, emp_name, dept_name) VALUES (1, 'Alice', 'HR');
select * from employees;
select * from departments;

-- fire update trigger
UPDATE employee_info SET emp_name = 'Alice Smith', dept_name = 'IT' WHERE emp_id = 1;
select * from employees;
select * from departments;

-- fire delete trigger
DELETE FROM employee_info WHERE emp_id = 1;
select * from employees;
select * from departments;

drop trigger trg_emp_info_insert;
drop trigger trg_emp_info_update;
drop trigger trg_emp_info_delete;
drop view employee_info;
drop table employees;
drop table departments;

--
-- Test Trigger body is CALL a procedure
--
CREATE TABLE call_emp(
	empno	NUMBER PRIMARY KEY,
	ename	VARCHAR2(50),
	sal		NUMBER
);

INSERT INTO call_emp VALUES (1001, 'Alice', 5000);
INSERT INTO call_emp VALUES (1002, 'Bob',   6000);
INSERT INTO call_emp VALUES (1003, 'Carol', 7000);

CREATE TABLE call_emp_audit (
	audit_id    NUMBER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
	empno       NUMBER,
	old_sal     NUMBER,
	new_sal     NUMBER,
	change_date DATE
);

CREATE OR REPLACE PROCEDURE call_log_emp_update (
	p_empno    NUMBER,
	p_old_sal  NUMBER,
	p_new_sal  NUMBER
) IS
BEGIN
	INSERT INTO call_emp_audit (empno, old_sal, new_sal, change_date)
	VALUES (p_empno, p_old_sal, p_new_sal, TO_DATE('2025-11-11','yyyy-mm-dd'));
END;
/

CREATE OR REPLACE TRIGGER trg_emp_update
AFTER UPDATE OF sal ON call_emp
FOR EACH ROW
CALL call_log_emp_update(:NEW.empno, :OLD.sal, :NEW.sal)
/

SELECT * FROM call_emp;

UPDATE call_emp SET sal = sal + 500 WHERE empno = 1002;

SELECT * FROM call_emp_audit;
SELECT * FROM call_emp;

--cleanup
DROP TRIGGER trg_emp_update;
DROP PROCEDURE call_log_emp_update;
DROP TABLE call_emp_audit;
DROP TABLE call_emp;

/* Begin - BUG#Z6170, BUG#Z6196 ,BUG#Z6187 */
CREATE TABLE tb1 (
    id NUMBER PRIMARY KEY,
    name VARCHAR2(50),
    salary NUMBER(10,2)
);

CREATE TRIGGER tgtiotsc
BEFORE INSERT ON tb1
FOR EACH ROW
BEGIN
    IF :NEW.id IS NULL THEN
        SELECT NVL(MAX(id), 0) + 100 INTO :NEW.id FROM tb1;
    END IF;
END;
/

CREATE OR REPLACE TRIGGER tgtiotsc
BEFORE INSERT ON tb1
FOR EACH ROW
BEGIN
    IF :NEW.id IS NULL THEN
        SELECT NVL(MAX(id), 0) + 200 INTO :NEW.id FROM tb1;
    END IF;
END;
/

/* Begin - BUG#Z6215 6216 */
select trigger_name, trigger_type, triggering_event, base_object_type, table_name
 from user_triggers where upper(trigger_name) = 'TGTIOTSC';
/* End - BUG#Z6215 6216 */

/* Begin - BUG#Z6339 */
select trigger_name, crossedition
 from user_triggers where upper(trigger_name) = 'TGTIOTSC';
/* End - BUG#Z6339 */

INSERT INTO tb1 (name, salary) VALUES ('Wang 5', 7000); -- fire trigger
INSERT INTO tb1 (id, name, salary) VALUES (300, 'Li 4', 6000); -- unfire trigger

SELECT * FROM tb1;

-- cleanup
DROP TRIGGER tgtiotsc;
DROP TABLE tb1;
/* End - BUG#Z6170, BUG#Z6196 ,BUG#Z6187 */

/* Begin - BUG#Z6366 */
drop table if exists t1;
drop table if exists t2;

create table t1(
  emp_id  number primary key,
  emp_name varchar2(50),
  salary number(10,2)
);

create table t2(
  log_id  number primary key,
  operation varchar2(50),
  emp_id number
);

create view vt1 as
  select emp_id, emp_name, salary from t1;

CREATE SEQUENCE log_seq;

CREATE OR REPLACE TRIGGER trg_vt1_ins
INSTEAD OF INSERT ON vt1
FOR EACH ROW
BEGIN
    INSERT INTO t1(emp_id, emp_name, salary)
        VALUES (:NEW.emp_id, :NEW.emp_name, :NEW.salary);

    INSERT INTO t2(log_id, operation, emp_id)
        VALUES (log_seq.nextval, 'INSERT', :NEW.emp_id);
END;
/

CREATE OR REPLACE TRIGGER trg_vt1_upd
INSTEAD OF UPDATE ON vt1
FOR EACH ROW
BEGIN
    UPDATE t1
       SET emp_name = :NEW.emp_name,
           salary   = :NEW.salary
     WHERE emp_id   = :OLD.emp_id;

    INSERT INTO t2(log_id, operation, emp_id)
        VALUES (log_seq.nextval, 'UPDATE', :OLD.emp_id);
END;
/

CREATE OR REPLACE TRIGGER trg_vt1_del
INSTEAD OF DELETE ON vt1
FOR EACH ROW
BEGIN
    DELETE FROM t1 WHERE emp_id = :OLD.emp_id;

    INSERT INTO t2(log_id, operation, emp_id)
        VALUES (log_seq.nextval, 'DELETE', :OLD.emp_id);
END;
/

-- get trigger defination
SELECT pg_catalog.pg_get_triggerdef(t.oid, true) FROM pg_trigger t WHERE t.tgname = 'trg_vt1_upd';

--
-- Test insert
--
INSERT INTO vt1 VALUES (1, 'Tom', 5000); -- fire trg_vt1_ins trigger
select * from t1 order by emp_id;
select * from t2;

-- disable trg_vt1_ins
alter trigger trg_vt1_ins disable;

select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_INS';
INSERT INTO vt1 VALUES (2, 'Bob', 2000); -- not fire trg_vt1_ins trigger
select * from t1 order by emp_id;
select * from t2;

-- enable trg_vt1_ins
alter trigger trg_vt1_ins enable;
select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_INS';
INSERT INTO vt1 VALUES (3, 'Kobe', 9000); -- fire trg_vt1_ins trigger
select * from t1 order by emp_id;
select * from t2;

--
-- Test update
--
UPDATE vt1 SET salary = 6000 WHERE emp_id = 1; -- fire trg_vt1_upd trigger
select * from t1 order by emp_id;
select * from t2;

-- disable trg_vt1_upd
alter trigger trg_vt1_upd disable;
select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_UPD';
UPDATE vt1 SET salary = 50 WHERE emp_id = 2; -- not fire trg_vt1_upd trigger
select * from t1 order by emp_id;
select * from t2;

-- enable trg_vt1_upd
alter trigger trg_vt1_upd enable;
select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_UPD';
UPDATE vt1 SET salary = 2025 WHERE emp_id = 3; -- fire trg_vt1_upd trigger
select * from t1 order by emp_id;
select * from t2;

--
-- Test delete
--
DELETE FROM vt1 WHERE emp_id = 1; -- fire trg_vt1_del trigger
select * from t1 order by emp_id;
select * from t2;

-- disable trg_vt1_del
alter trigger trg_vt1_del disable;
select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_DEL';
DELETE FROM vt1 WHERE emp_id = 2; -- not fire trg_vt1_del trigger
select * from t1 order by emp_id;
select * from t2;

-- enable trg_vt1_del
alter trigger trg_vt1_del enable;
select trigger_name, table_name, trigger_type, status from all_triggers where upper(trigger_name) = 'TRG_VT1_DEL';
DELETE FROM vt1 WHERE emp_id = 3; -- fire trg_vt1_del trigger
select * from t1 order by emp_id;
select * from t2;

-- cleanup
drop trigger trg_vt1_del;
drop trigger trg_vt1_upd;
drop trigger trg_vt1_ins;
drop view vt1;
drop table t1;
drop table t2;
drop sequence log_seq;
/* End - BUG#Z6366 */

/* Begin - BUG#Z6577 */
drop table if exists test_table;
CREATE TABLE test_table (
    id NUMBER PRIMARY KEY,
    value VARCHAR2(50)
);

CREATE OR REPLACE TRIGGER first_trigger
BEFORE INSERT ON test_table
FOR EACH ROW
BEGIN
    DBMS_OUTPUT.PUT_LINE('1111');
END;
/

CREATE OR REPLACE TRIGGER second_trigger
BEFORE INSERT ON test_table
FOR EACH ROW
FOLLOWS first_trigger
BEGIN
    DBMS_OUTPUT.PUT_LINE('2222');
END;
/

CREATE OR REPLACE TRIGGER thrid_trigger
BEFORE INSERT ON test_table
FOR EACH ROW
FOLLOWS second_trigger
BEGIN
    DBMS_OUTPUT.PUT_LINE('3333');
END;
/

CREATE OR REPLACE TRIGGER fourth_trigger
BEFORE INSERT ON test_table
FOR EACH ROW DISABLE
BEGIN
    DBMS_OUTPUT.PUT_LINE('4444');
END;
/

SET SERVEROUTPUT ON;

INSERT INTO test_table (id, value) VALUES (1, 'Test');

-- get trigger defination
SELECT pg_catalog.pg_get_triggerdef(t.oid, true) FROM pg_trigger t WHERE t.tgname = 'fourth_trigger';

--cleanup
drop trigger fourth_trigger;
drop trigger thrid_trigger;
drop trigger second_trigger;
drop trigger first_trigger;
drop table test_table;
SET SERVEROUTPUT OFF;
/* End - BUG#Z6577 */

/* Begin - BUG#Z6560 */
drop table if exists T1;
drop table if exists T2;
CREATE TABLE T1 (
    emp_id NUMBER PRIMARY KEY,
    emp_name VARCHAR2(50) NOT NULL,
    salary NUMBER(10,2),
    email VARCHAR2(100)
);

CREATE TABLE T2 (
    log_id NUMBER PRIMARY KEY,
    operation VARCHAR2(20),
    emp_id NUMBER,
    error_msg VARCHAR2(1000)
);

CREATE SEQUENCE TSEQ;

INSERT INTO T1 (emp_id, emp_name, salary, email) VALUES (1001, 'TEST EMP', 5000, 'test@company.com');

CREATE OR REPLACE TRIGGER test_basic_referencing
BEFORE UPDATE ON T1
REFERENCING OLD AS original NEW AS updated
FOR EACH ROW
WHEN (original.salary != updated.salary)
BEGIN
    INSERT INTO T2 (log_id, operation, emp_id, error_msg)
    VALUES (TSEQ.NEXTVAL, 'UPDATE', :updated.emp_id,
           'BASIC_REFERENCING: Salary changed ' || :original.salary || '->' || :updated.salary);
END;
/

UPDATE T1 SET salary = 6000 WHERE emp_id = 1001;
SELECT * FROM T1;
SELECT * FROM T2 WHERE error_msg LIKE 'BASIC_REFERENCING%';

--cleanup
drop trigger test_basic_referencing;
drop sequence TSEQ;
drop table T1;
drop table T2;
/* End - BUG#Z6560 */

/* Begin - BUG#Z6516 */
DROP TRIGGER IF EXISTS vskill;
/* End - BUG#Z6516 */
