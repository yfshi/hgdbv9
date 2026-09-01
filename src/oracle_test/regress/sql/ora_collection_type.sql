--
-- SQL Collection types: nested table and varray
--

set serveroutput on;

--
-- create varray
--
CREATE TYPE vtype1 IS VARRAY(2) OF INT;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARRAY(2) OF (INT) PERSISTABLE;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARRAY(2) OF (INT) NOT PERSISTABLE;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF INT;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF (INT) PERSISTABLE;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARRAY(2) OF INT NOT NULL;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARRAY(2) OF (INT NOT NULL) PERSISTABLE;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARRAY(2) OF (INT NOT NULL) NOT PERSISTABLE;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF INT NOT NULL;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF (INT NOT NULL) PERSISTABLE;
CREATE OR REPLACE TYPE vtype1 IS VARRAY(2) OF (INT NOT NULL) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF INT;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) PERSISTABLE;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF INT;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) PERSISTABLE;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF INT;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) PERSISTABLE;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 FORCE IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF INT;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) PERSISTABLE;
DROP TYPE vtype1;
CREATE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 IS VARRAY(2) OF INT;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) PERSISTABLE;
DROP TYPE vtype1;
CREATE NONEDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF INT;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) PERSISTABLE;
CREATE OR REPLACE EDITIONABLE TYPE vtype1 IS VARRAY(2) OF (INT) NOT PERSISTABLE;
DROP TYPE vtype1;
CREATE TYPE vtype1 IS VARYING ARRAY(2) OF INT;
DROP TYPE vtype1;

--
-- create nested table
--
CREATE TYPE ntype1 IS TABLE OF INT;
DROP TYPE ntype1;
CREATE TYPE ntype1 IS TABLE OF (INT) PERSISTABLE;
DROP TYPE ntype1;
CREATE TYPE ntype1 IS TABLE OF (INT) NOT PERSISTABLE;
DROP TYPE ntype1;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF INT;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF (INT) PERSISTABLE;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF (INT) NOT PERSISTABLE;
DROP TYPE ntype1;
CREATE TYPE ntype1 IS TABLE OF INT NOT NULL;
DROP TYPE ntype1;
CREATE TYPE ntype1 IS TABLE OF (INT NOT NULL) PERSISTABLE;
DROP TYPE ntype1;
CREATE TYPE ntype1 IS TABLE OF (INT NOT NULL) NOT PERSISTABLE;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF INT NOT NULL;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF (INT NOT NULL) PERSISTABLE;
CREATE OR REPLACE TYPE ntype1 IS TABLE OF (INT NOT NULL) NOT PERSISTABLE;
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF INT;
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) PERSISTABLE;
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) NOT PERSISTABLE;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 FORCE IS TABLE OF INT;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) PERSISTABLE;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) NOT PERSISTABLE;
CREATE OR REPLACE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF INT;
CREATE OR REPLACE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) PERSISTABLE; 
CREATE OR REPLACE EDITIONABLE TYPE ntype1 FORCE IS TABLE OF (INT) NOT PERSISTABLE; 
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 IS TABLE OF INT;
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 IS TABLE OF (INT) PERSISTABLE;
DROP TYPE ntype1;
CREATE EDITIONABLE TYPE ntype1 IS TABLE OF (INT) NOT PERSISTABLE;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 IS TABLE OF INT;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 IS TABLE OF (INT) PERSISTABLE;
DROP TYPE ntype1;
CREATE NONEDITIONABLE TYPE ntype1 IS TABLE OF (INT) NOT PERSISTABLE;
CREATE OR REPLACE EDITIONABLE TYPE ntype1 IS TABLE OF INT;
CREATE OR REPLACE EDITIONABLE TYPE ntype1 IS TABLE OF (INT) PERSISTABLE;
CREATE OR REPLACE EDITIONABLE TYPE ntype1 IS TABLE OF (INT) NOT PERSISTABLE;
DROP TYPE ntype1;

--
-- nested table collection constructor
--
CREATE OR REPLACE TYPE dnames_var IS TABLE OF VARCHAR2(100);
CREATE OR REPLACE TYPE ids_var IS TABLE OF INT;
CREATE TYPE depts_id IS TABLE OF INT;

--table column datatype is collection type defined in SQL
CREATE TABLE depts(id int, dept_names dnames_var);
CREATE TABLE tab_1(id int, c1 dnames_var) NESTED TABLE c1 STORE AS cnt1;
CREATE TABLE t0(id int, c1 int) NESTED TABLE c1 STORE AS cnt2;

INSERT INTO depts VALUES(1, dnames_var('Shipping','Sales'));
INSERT INTO tab_1 VALUES(1, dnames_var('Benefits', 'Advertising', 'Contracting'));
SELECT * FROM depts ORDER BY id;
SELECT * FROM tab_1 ORDER BY id;

BEGIN
  INSERT INTO depts VALUES(2, dnames_var('Finance','Payroll'));
  INSERT INTO tab_1 VALUES(2, dnames_var('Advertising', 'Contracting'));
END;
/
SELECT * FROM depts ORDER BY id;
SELECT * FROM tab_1 ORDER BY id;

DECLARE
  v1 dnames_var;
BEGIN
  INSERT INTO depts VALUES(3, dnames_var('a','b','c'));
END;
/
SELECT * FROM depts ORDER BY id;

DECLARE
  v1 dnames_var := dnames_var('d','e','f');
BEGIN
  INSERT INTO depts VALUES(4, v1);
END;
/
SELECT * FROM depts ORDER BY id;

--create type in a named schema
CREATE SCHEMA test_schema;
CREATE TYPE test_schema.int_set_t AS TABLE OF INT;

DECLARE
  v test_schema.int_set_t;
BEGIN
  v := test_schema.int_set_t(1,2);

  FOR i IN v.FIRST .. v.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v(' || i || ')= ' || v(i));
  END LOOP; 
END;
/

CREATE TABLE test_schema.t_var(c1 integer, c2 test_schema.int_set_t) nested table c2 store as test_schema.ctvar;

INSERT INTO test_schema.t_var VALUES(1, test_schema.int_set_t(1,2,3));
INSERT INTO test_schema.t_var VALUES(2, test_schema.int_set_t(4,5,6));
INSERT INTO test_schema.t_var VALUES(3, test_schema.int_set_t(7,8,9));

SELECT * FROM test_schema.t_var ORDER BY c1;

UPDATE test_schema.t_var SET c2 = test_schema.int_set_t(11,12,13) WHERE c1 = 1;
DELETE FROM test_schema.t_var WHERE c1 = 3;
SELECT * FROM test_schema.t_var ORDER BY c1;

DROP TABLE test_schema.t_var;
DROP TYPE test_schema.int_set_t;
DROP SCHEMA test_schema;


CREATE OR REPLACE EDITIONABLE TYPE edition_tbl FORCE AS TABLE OF (INT NOT NULL) PERSISTABLE;
CREATE NONEDITIONABLE TYPE persist_varr AS VARRAY(15) OF (VARCHAR(50) NOT NULL) NOT PERSISTABLE;

DECLARE
    v_t edition_tbl := edition_tbl(10,20,30);
    v_v persist_varr := persist_varr('a ','b');
BEGIN
    DBMS_OUTPUT.PUT_LINE('EDITIONABLE nested value '||v_t(1));
    DBMS_OUTPUT.PUT_LINE('NONEDITIONABLE varrray value '||v_v(1));
EXCEPTION
    WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('exception is '||SQLERRM);
END;
/
DROP  TYPE edition_tbl;
DROP TYPE persist_varr;



CREATE TYPE Foursome IS TABLE OF VARCHAR2(15);
CREATE TYPE varr_phone AS VARRAY(3) OF NUMBER(11);

DECLARE
    team Foursome := Foursome();
    phones varr_phone := varr_phone();
BEGIN
    DBMS_OUTPUT.PUT_LINE('The number of elements in a nested table empty collection is ' || team.COUNT);
    DBMS_OUTPUT.PUT_LINE('The number of elements in a varray empty collection is ' || phones.COUNT);

    IF team IS NOT NULL AND phones IS NOT NULL THEN
        DBMS_OUTPUT.PUT_LINE('The empty constructor returns a non-null collection.');
    END IF;
END;
/

DECLARE
  v_name1 VARCHAR2(15) := 'testA';
  v_name2 VARCHAR2(15) := 'testB';
  v_num1 NUMBER(11) := 13800138000;
  v_num2 NUMBER(11) := 13900139000;
  tab_test Foursome;
  var_test varr_phone;
BEGIN
  tab_test := Foursome(v_name1, v_name2, 'testC');
  var_test := varr_phone(v_num1 + 1, v_num2 - 1);
  DBMS_OUTPUT.PUT_LINE('tab_test.COUNT is '||tab_test.COUNT);
  DBMS_OUTPUT.PUT_LINE('var_test.COUNT is '|| var_test.COUNT);
END;
/

DECLARE
  team Foursome;
BEGIN
  team := Foursome(100,200);

  FOR i IN 1 .. team.COUNT LOOP
    DBMS_OUTPUT.PUT_LINE('team(' || i||') = ' || team(i));
  END LOOP;

EXCEPTION
  WHEN OTHERS THEN
      DBMS_OUTPUT.PUT_LINE('catch execption '||SQLERRM);
END;
/

DECLARE
  team Foursome;
BEGIN
  team := Foursome(100,200);
  team.extend(1, 1);

  FOR i IN 1 .. team.COUNT LOOP
    DBMS_OUTPUT.PUT_LINE('team(' || i||') = ' || team(i));
  END LOOP;

  team.delete(2);
  DBMS_OUTPUT.PUT_LINE('after delete, team.first = ' || team.first);
  DBMS_OUTPUT.PUT_LINE('after delete, team.last = ' || team.last);
  DBMS_OUTPUT.PUT_LINE('after delete, team.prior(3) = ' || team.prior(3));
  DBMS_OUTPUT.PUT_LINE('after delete, team.next(1) = ' || team.next(1));
  team.trim(2);
  DBMS_OUTPUT.PUT_LINE('after tim, team.first = ' || team.first);
  DBMS_OUTPUT.PUT_LINE('after tim,team.last = ' || team.last);

  IF team.exists(1) THEN
    DBMS_OUTPUT.PUT_LINE('team.exists(1) exists');
  ELSE
    DBMS_OUTPUT.PUT_LINE('team.exists(1) does not exist');
  END IF;
END;
/

DROP TYPE Foursome;
DROP TYPE varr_phone;


--RETURN collection variable
CREATE OR REPLACE FUNCTION fun(a int) RETURN dnames_var
AS
  res dnames_var := dnames_var('fa','fb');
BEGIN
  RETURN res;
END;
/

DECLARE
BEGIN 
  INSERT INTO depts VALUES(6, fun(1)); 
END;
/
SELECT * FROM depts ORDER BY id;


DECLARE
  v1 dnames_var;
BEGIN
  v1 := fun(1);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP; 
END;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
BEGIN
  v2 := fun(1);
  v1 := v2;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP; 
END;
/

--RETURN collection constructor
CREATE OR REPLACE FUNCTION fun2(a varchar(100)) RETURN dnames_var
AS
BEGIN
  RETURN dnames_var('fc', 'fd', a);
END;
/

DECLARE
BEGIN 
  INSERT INTO depts VALUES(7, fun2('fe')); 
END;
/
SELECT * FROM depts ORDER BY id;

DECLARE
  v1 dnames_var;
BEGIN
  v1 := fun2('fe');
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

CREATE OR REPLACE FUNCTION fun3(a dnames_var) RETURN dnames_var
AS
  res dnames_var;
BEGIN
  res := a;
  RETURN res;
END;
/

DECLARE
BEGIN 
  INSERT INTO depts VALUES(8, fun3(dnames_var('3a', '3b', '3c'))); 
END;
/
SELECT * FROM depts ORDER BY id;

DECLARE
  v1 dnames_var;
  v2 dnames_var := dnames_var('3a2', '3b2', '3c2');
BEGIN
  INSERT INTO depts VALUES(9, fun3(v2)); 
  v1 := fun3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

SELECT * FROM depts ORDER BY id;

--SELECT column FROM table into collection variable
DECLARE
  v1 dnames_var;
BEGIN
  SELECT dept_names INTO v1 FROM depts WHERE id = 1;

  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var := dnames_var('d1', 'd2', 'd3');
BEGIN
  v1 := v2;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

--function has one OUT collection parameter
CREATE OR REPLACE FUNCTION fun4(a OUT dnames_var) RETURN dnames_var
AS
BEGIN
  a := dnames_var('f4a', 'f4b');
  RETURN dnames_var('f4e', 'f4f');
END;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
BEGIN
  v1 := fun4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/


CREATE OR REPLACE FUNCTION fun5(a OUT dnames_var) RETURN VARCHAR2
AS
  res VARCHAR2(20) := 'RESULT';
BEGIN
  a := dnames_var('f5a', 'f5b');
  RETURN res;
END;
/

DECLARE
  v1 VARCHAR2(20);
  v2 dnames_var;
BEGIN
  v1 := fun5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/


CREATE OR REPLACE FUNCTION fun6(a OUT varchar(100)) RETURN dnames_var
AS
BEGIN
  a := 'newvalue';
  RETURN dnames_var('f6a', 'f6b', 'f6c');
END;
/

DECLARE
  v1 varchar(100) := 'f6_var';
BEGIN 
  INSERT INTO depts VALUES(10, fun6(v1));
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);
END;
/
SELECT * FROM depts ORDER BY id;

DECLARE
  v1 dnames_var;
  v2 varchar(100) := 'f6_var';
BEGIN
  v1 := fun6(v2);

  DBMS_OUTPUT.PUT_LINE('v2 = ' || v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

--function has two OUT collection parameters
CREATE OR REPLACE FUNCTION fun7(a OUT dnames_var, b OUT dnames_var) RETURN dnames_var
AS
  res dnames_var;
BEGIN
  a := dnames_var('f7a', 'f7b', 'f7c');
  b := dnames_var('f7d', 'f7e', 'f7f');
  res := a;
  RETURN res;
END;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
  v3 dnames_var;
BEGIN
  v1 := fun7(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
  FOR i IN v3.FIRST .. v3.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v3(' || i || ')= ' || v3(i));
  END LOOP;
END;
/


--DML
CREATE TABLE depts2 (region VARCHAR2(25), dept_names dnames_var);

INSERT INTO depts2 VALUES('Europe', dnames_var('Shipping','Sales','Finance'));
INSERT INTO depts2 VALUES('Americas', dnames_var('Sales','Finance','Shipping'));

DECLARE
  new_dnames dnames_var := dnames_var('Benefits', 'Advertising', 'Contracting');
  some_dnames dnames_var;
  some_dnames2 dnames_var;
BEGIN
  UPDATE depts2 SET dept_names = new_dnames WHERE region = 'Europe';

  SELECT dept_names INTO some_dnames FROM depts2 WHERE region = 'Europe';
  FOR i IN some_dnames.FIRST .. some_dnames.LAST
  LOOP
	DBMS_OUTPUT.PUT_LINE('some_dnames(' || i || ')= ' || some_dnames(i));
  END LOOP;

  DELETE depts2 WHERE region = 'Americas' RETURNING dept_names INTO some_dnames2;
  FOR i IN some_dnames2.FIRST .. some_dnames2.LAST
  LOOP
    DBMS_OUTPUT.PUT_LINE('some_dnames2(' || i || ')= ' || some_dnames2(i));
  END LOOP;
END;
/
SELECT * FROM depts2;
DROP TABLE depts2;


--table has two collection columns
CREATE TABLE depts3 (region VARCHAR2(25), dept_names dnames_var, dept_ids ids_var);

INSERT INTO depts3 VALUES('Europe', dnames_var('Shipping','Sales','Finance'), ids_var(101, 102));
INSERT INTO depts3 VALUES('Americas', dnames_var('Sales','Finance','Shipping'), ids_var(107, 108));

DECLARE
  new_dnames dnames_var := dnames_var('Benefits', 'Advertising', 'Contracting');
  new_ids  ids_var := ids_var(103, 104);
  some_dnames dnames_var;
  some_dnames2 dnames_var;
  some_ids ids_var;
  some_ids2 ids_var;
BEGIN
  UPDATE depts3 SET dept_names = new_dnames, dept_ids = new_ids WHERE region = 'Europe';

  SELECT dept_names, dept_ids INTO some_dnames, some_ids FROM depts3 WHERE region = 'Europe';
  FOR i IN some_dnames.FIRST .. some_dnames.LAST
  LOOP
    DBMS_OUTPUT.PUT_LINE('some_dnames(' || i || ')= ' || some_dnames(i));
  END LOOP;

  FOR i IN some_ids.FIRST .. some_ids.LAST
  LOOP
	DBMS_OUTPUT.PUT_LINE('some_ids(' || i || ')= ' || some_ids(i));
  END LOOP;

  DELETE depts3 WHERE region = 'Americas' RETURNING dept_names, dept_ids INTO some_dnames2, some_ids2;
  FOR i IN some_dnames2.FIRST .. some_dnames2.LAST
  LOOP
    DBMS_OUTPUT.PUT_LINE('some_dnames2(' || i || ')= ' || some_dnames2(i));
  END LOOP;

  FOR i IN some_ids2.FIRST .. some_ids2.LAST
  LOOP
    DBMS_OUTPUT.PUT_LINE('some_ids2(' || i || ')= ' || some_ids2(i));
  END LOOP;
END;
/

DROP TABLE depts3;


--inline function return datatype is SQL collection
DELETE FROM depts;

CREATE OR REPLACE PROCEDURE proc1(a int)
AS
  v1 dnames_var;
  v2 dnames_var;
  FUNCTION f1(a int) RETURN dnames_var
  AS
    res dnames_var := dnames_var('fa','fb');
  BEGIN
    RETURN res;
  END;
BEGIN
  INSERT INTO depts VALUES(1, f1(1));

  v1 := f1(1);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;

  v2 := v1;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP; 
END;
/

CALL proc1(1);
SELECT * FROM depts ORDER BY id;


CREATE OR REPLACE PROCEDURE proc2(a int)
AS
  v1 dnames_var;
  v2 dnames_var;
  FUNCTION f2(a varchar(100)) RETURN dnames_var
  AS
    res dnames_var := dnames_var('fa','fb');
  BEGIN
    RETURN dnames_var('fc', 'fd', a);
  END;
BEGIN
  INSERT INTO depts VALUES(2, f2('fe')); 

  v1 := f2('fe');
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;

  v2 := v1;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP; 
END;
/
CALL proc2(1);
SELECT * FROM depts ORDER BY id;


--inline function IN parameter datatype is SQL collection
CREATE OR REPLACE PROCEDURE proc3(a int)
AS
  v1 dnames_var;
  v2 dnames_var := dnames_var('3a2', '3b2', '3c2');
  FUNCTION f3(a dnames_var) RETURN dnames_var
  AS
    res dnames_var;
  BEGIN
    res := a;
    RETURN res;
  END;
BEGIN
  INSERT INTO depts VALUES(3, f3(dnames_var('3a', '3b', '3c')));  
  INSERT INTO depts VALUES(4, f3(v2)); 
  v1 := f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

CALL proc3(1);
SELECT * FROM depts ORDER BY id;

--inline function OUT parameter datatype is SQL collection
CREATE OR REPLACE PROCEDURE proc4(a int)
AS
  v1 dnames_var;
  v2 dnames_var;
  FUNCTION f4(a OUT dnames_var) RETURN dnames_var
  AS
  BEGIN
    a := dnames_var('f4a', 'f4b');
    RETURN dnames_var('f4e', 'f4f');
  END;
BEGIN
  v1 := f4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/

CALL proc4(1);


CREATE OR REPLACE PROCEDURE proc5(a int)
AS
  v1 VARCHAR2(20);
  v2 dnames_var;
  FUNCTION f5(a OUT dnames_var) RETURN VARCHAR2
  AS
    res VARCHAR2(20) := 'RESULT';
  BEGIN
    a := dnames_var('f5a', 'f5b');
    RETURN res;
  END;
BEGIN
  v1 := f5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/

CALL proc5(1);


--inline function has two OUT collection parameters
CREATE OR REPLACE PROCEDURE proc6(a int)
AS
  v1 dnames_var;
  v2 dnames_var;
  v3 dnames_var;
  FUNCTION f6(a OUT dnames_var, b OUT dnames_var) RETURN dnames_var
  AS
    res dnames_var;
  BEGIN
    a := dnames_var('f7a', 'f7b', 'f7c');
    b := dnames_var('f7d', 'f7e', 'f7f');
    res := a;
    RETURN res;
  END;
BEGIN
  v1 := f6(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
  FOR i IN v3.FIRST .. v3.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v3(' || i || ')= ' || v3(i));
  END LOOP;
END;
/

CALL proc6(1);


--package
--package function return datatype is SQL collection
DELETE FROM depts;

CREATE OR REPLACE PACKAGE pkg1 AS
  FUNCTION f1(a int) RETURN dnames_var;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg1 AS
  FUNCTION f1(a int) RETURN dnames_var
  AS
    res dnames_var := dnames_var('fa','fb');
  BEGIN
    RETURN res;
  END;
END pkg1;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
BEGIN
  INSERT INTO depts VALUES(1, pkg1.f1(1));

  v1 := pkg1.f1(1);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;

  v2 := v1;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP; 
END;
/

SELECT * FROM depts ORDER BY id;

CREATE OR REPLACE PACKAGE pkg2 AS
  FUNCTION f2(a varchar(100)) RETURN dnames_var;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg2 AS
  FUNCTION f2(a varchar(100)) RETURN dnames_var
  AS
    res dnames_var := dnames_var('fa','fb');
  BEGIN
    RETURN dnames_var('fc', 'fd', a);
  END;
END pkg2;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
BEGIN
  INSERT INTO depts VALUES(2, pkg2.f2('fe')); 

  v1 := pkg2.f2('fe');
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;

  v2 := v1;
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP; 
END;
/

SELECT * FROM depts ORDER BY id;


--package function IN parameter datatype is SQL collection
CREATE OR REPLACE PACKAGE pkg3 AS
  FUNCTION f3(a dnames_var) RETURN dnames_var;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg3 AS
  FUNCTION f3(a dnames_var) RETURN dnames_var
  AS
    res dnames_var;
  BEGIN
    res := a;
    RETURN res;
  END;
END pkg3;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var := dnames_var('3a2', '3b2', '3c2');
BEGIN
  INSERT INTO depts VALUES(3, pkg3.f3(dnames_var('3a', '3b', '3c')));  
  INSERT INTO depts VALUES(4, pkg3.f3(v2)); 
  v1 := pkg3.f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

SELECT * FROM depts ORDER BY id;

--package function OUT parameter datatype is SQL collection
CREATE OR REPLACE PACKAGE pkg4 AS
  FUNCTION f4(a OUT dnames_var) RETURN dnames_var;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg4 AS
  FUNCTION f4(a OUT dnames_var) RETURN dnames_var
  AS
  BEGIN
    a := dnames_var('f4a', 'f4b');
    RETURN dnames_var('f4e', 'f4f');
  END;
END pkg4;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
BEGIN
  v1 := pkg4.f4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/

CREATE OR REPLACE PACKAGE pkg5 AS
  FUNCTION f5(a OUT dnames_var) RETURN VARCHAR2;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg5 AS
  FUNCTION f5(a OUT dnames_var) RETURN VARCHAR2
  AS
    res VARCHAR2(20) := 'RESULT';
  BEGIN
    a := dnames_var('f5a', 'f5b');
    RETURN res;
  END;
END pkg5;
/

DECLARE
  v1 VARCHAR2(20);
  v2 dnames_var;
BEGIN
  v1 := pkg5.f5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/

--package function has two OUT collection parameters
CREATE OR REPLACE PACKAGE pkg6 AS
  FUNCTION f6(a OUT dnames_var, b OUT dnames_var) RETURN dnames_var;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg6 AS
  FUNCTION f6(a OUT dnames_var, b OUT dnames_var) RETURN dnames_var
  AS
    res dnames_var;
  BEGIN
    a := dnames_var('f7a', 'f7b', 'f7c');
    b := dnames_var('f7d', 'f7e', 'f7f');
    res := a;
    RETURN res;
  END;
END pkg6;
/

DECLARE
  v1 dnames_var;
  v2 dnames_var;
  v3 dnames_var;
BEGIN
  v1 := pkg6.f6(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
  FOR i IN v3.FIRST .. v3.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v3(' || i || ')= ' || v3(i));
  END LOOP;
END;
/


CREATE OR REPLACE PACKAGE pkg7 AS
  pd depts_id;
  FUNCTION f1(id INT) RETURN INT;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg7 AS
  FUNCTION f1(id INT) RETURN INT AS
    res depts_id;
  BEGIN
    pd := depts_id(id, 2, 3);
    res := pd;
	RETURN res(1);
  END;
END;
/

DECLARE
  var INT;
BEGIN
  var := pkg7.f1(100);
  DBMS_OUTPUT.PUT_LINE('var = ' || var);
END;
/

CREATE OR REPLACE PACKAGE pkg8 AS
  pd depts_id;
END;
/

DELETE FROM depts;

DECLARE
  v1 pkg8.pd%TYPE;
  names dnames_var := dnames_var('d','e','f');
BEGIN
  v1 := depts_id(1, 2, 3);
  INSERT INTO depts VALUES(v1(2), names);
END;
/

SELECT * FROM depts ORDER BY id;



DROP PACKAGE pkg1;
DROP PACKAGE pkg2;
DROP PACKAGE pkg3;
DROP PACKAGE pkg4;
DROP PACKAGE pkg5;
DROP PACKAGE pkg6;
DROP PACKAGE pkg7;
DROP PACKAGE pkg8;
DROP FUNCTION fun;
DROP FUNCTION fun2;
DROP FUNCTION fun3;
DROP FUNCTION fun4;
DROP FUNCTION fun5;
DROP FUNCTION fun6;
DROP FUNCTION fun7;
DROP PROCEDURE proc1;
DROP PROCEDURE proc2;
DROP PROCEDURE proc3;
DROP PROCEDURE proc4;
DROP PROCEDURE proc5;
DROP PROCEDURE proc6;
DROP TABLE tab_1;
DROP TABLE depts;
DROP TYPE dnames_var;
DROP TYPE depts_id;

--
-- varray collection constructor
--
CREATE TYPE dnames_var IS VARRAY(7) OF VARCHAR2(30);

CREATE TABLE depts (id int, region VARCHAR2(25), dept_names dnames_var);

INSERT INTO depts VALUES(1, 'Europe', dnames_var('Shipping','Sales','Finance'));
INSERT INTO depts VALUES(2, 'Americas', dnames_var('Sales','Finance','Shipping'));
SELECT * FROM depts ORDER BY id;

BEGIN
  INSERT INTO depts VALUES(3, 'Asia', dnames_var('Advertising','Sales'));
  UPDATE depts SET dept_names = dnames_var('Benefits', 'Contracting')  WHERE region = 'Asia';
END;
/
SELECT * FROM depts ORDER BY id;

--more than limit, it raises an error
INSERT INTO depts VALUES('Americas', dnames_var('Sales','Finance','Shipping', 'a','b','c','d','e'));

DROP TABLE depts;
DROP TYPE dnames_var;


--multilevel collections
--the element datatype of a nestd table is nestd table
CREATE TYPE mytype1 IS TABLE OF INT;
CREATE TYPE mytype2 IS TABLE OF mytype1;

CREATE TABLE test2(id int, c1 mytype2) nested table c1 store as ntest2;
INSERT INTO test2 VALUES(1, mytype2(mytype1(1, 11)));

SELECT * FROM test2 ORDER BY id;

DECLARE
  v3 mytype2;
BEGIN
  v3 := mytype2(mytype1(30, 31));

  FOR i IN v3.FIRST .. v3.LAST
  LOOP
    FOR j IN v3(i).FIRST .. v3(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v3(' || i || ')(' || j ||')= ' || v3(i)(j));
    END LOOP; 
  END LOOP;  
 
  DBMS_OUTPUT.PUT_LINE('v3(1).count is ' || v3(1).count);
  v3(1).extend(1, 1);
  DBMS_OUTPUT.PUT_LINE('v3(1).count is ' || v3(1).count);
END;
/

DECLARE
  v3 mytype2;
BEGIN
  v3 := mytype2(mytype1(30, 31));

  DBMS_OUTPUT.PUT_LINE('before extend');

  FOR i IN v3.FIRST .. v3.LAST
  LOOP
    FOR j IN v3(i).FIRST .. v3(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v3(' || i || ')(' || j ||')= ' || v3(i)(j));
    END LOOP; 
  END LOOP;  
 
  v3.extend(1, 1);

  DBMS_OUTPUT.PUT_LINE('after extend');
  FOR i IN v3.FIRST .. v3.LAST
  LOOP
    FOR j IN v3(i).FIRST .. v3(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v3(' || i || ')(' || j ||')= ' || v3(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

DECLARE
  v1 mytype1 := mytype1(20, 21);
  v2 mytype2 := mytype2(v1);
  v3 mytype2;
BEGIN
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;  

  v3 := mytype2(mytype1(30, 31));

  FOR i IN v3.FIRST .. v3.LAST
  LOOP
    FOR j IN v3(i).FIRST .. v3(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v3(' || i || ')(' || j ||')= ' || v3(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

--INSERT INTO test2 VALUES(2, mytype2(mytype2(2, 22)));--error

--INSERT statement in PLSQL
DECLARE
  v1 mytype1 := mytype1(3, 33);
  v2 mytype2 := mytype2(v1); 
BEGIN
  INSERT INTO test2 VALUES(3, mytype2(v1)); 
  INSERT INTO test2 VALUES(4, mytype2(mytype1(4, 5, 6))); 
END;
/

SELECT * FROM test2 ORDER BY id;

DROP TABLE test2;
DROP TYPE mytype2;
DROP TYPE mytype1;


--the element datatype of a nestd table is varray
CREATE OR REPLACE TYPE vtype2 IS VARRAY(10) OF INT;
CREATE OR REPLACE TYPE ntype2 IS TABLE OF vtype2;

DECLARE
  v1 vtype2 := vtype2(1,2);
  v2 ntype2 := ntype2(v1);
BEGIN
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

DROP TYPE ntype2;
DROP TYPE vtype2;


CREATE OR REPLACE TYPE ntype1 IS TABLE OF INT;
CREATE OR REPLACE TYPE ntype2 IS TABLE OF ntype1;
CREATE TABLE test1(id int, c1 ntype1) nested table c1 store as c1test1;
CREATE TABLE test2(id int, c2 ntype2) nested table c2 store as c2test2;

DECLARE
  v1 ntype1;
  v2 ntype2;
BEGIN
  DELETE FROM test1;
  INSERT INTO test1 VALUES(1, ntype1(1, 2));
  select c1 into v1 from test1;
  dbms_output.put_line('v1(2) = ' || v1(2));
  DELETE FROM test2;
  INSERT INTO test2 VALUES(100, ntype2(ntype1(11, 22)));
  select c2 into v2 from test2;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;

  INSERT INTO test1 VALUES(2, ntype1(3, 4));
  INSERT INTO test2 VALUES(101, v2);
  INSERT INTO test2 VALUES(102, ntype2(v1));
END;
/

SELECT * FROM test1 ORDER BY id;
SELECT * FROM test2 ORDER BY id;

DROP TABLE test2;
DROP TABLE test1;
DROP TYPE ntype2;
DROP TYPE ntype1;


CREATE TYPE base_arr IS VARRAY(5) OF VARCHAR2(10);
CREATE TYPE base_tab IS TABLE OF NUMBER;
CREATE TYPE arr_arr IS VARRAY(3) OF base_arr;
CREATE TYPE tab_arr IS VARRAY(2) OF base_tab;
CREATE TYPE tab_tab IS TABLE OF base_tab;

DECLARE
  v_arr arr_arr := arr_arr(base_arr('A','B'), base_arr('C'));
  v_tabarr tab_arr := tab_arr(base_tab(1,2));
  v_tabtab tab_tab := tab_tab(base_tab(10,20));
BEGIN
  DBMS_OUTPUT.PUT_LINE('v_arr(1)(1) = ' || v_arr(1)(1));
  DBMS_OUTPUT.PUT_LINE('v_tabarr(1)(1) = '|| v_tabarr(1)(1));
  DBMS_OUTPUT.PUT_LINE('v_tabtab(1)(1) = ' || v_tabtab(1)(1));
END;
/

DROP TYPE arr_arr;
DROP TYPE tab_arr;
DROP TYPE tab_tab;
DROP TYPE base_arr;
DROP TYPE base_tab;


--pg_proc function return datatype is multilevel collection
CREATE OR REPLACE TYPE dnames_var IS TABLE OF VARCHAR2(100);
CREATE OR REPLACE TYPE type2 IS VARRAY(10) OF dnames_var;
CREATE TABLE depts(id int, dept_names dnames_var);
CREATE TABLE depts2(id int, dept_names2 type2);

CREATE OR REPLACE FUNCTION f1(a int) RETURN type2
AS
  res type2 := type2(dnames_var('fa','fb'), dnames_var('fc','fd'));
BEGIN
  RETURN res;
END;
/


DECLARE
  v1 type2;
  v2 type2;
BEGIN
  INSERT INTO depts2 VALUES(1, f1(1));
 
  v1 := f1(1);
  FOR i IN v1.FIRST .. v1.LAST LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts2 ORDER BY id;

CREATE OR REPLACE FUNCTION f2(a varchar(100)) RETURN type2
AS
BEGIN
  RETURN type2(dnames_var('fe','ff'), dnames_var('fh', a));
END;
/

DECLARE
  v1 type2;
  v2 type2;
BEGIN
  INSERT INTO depts2 VALUES(2, f2('fe')); 
  
  v1 := f2('fe');
  INSERT INTO depts VALUES(2, v1(1)); 

  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts ORDER BY id;
SELECT * FROM depts2 ORDER BY id;

--pg_proc function IN parameter datatype is SQL multilevel collection
CREATE OR REPLACE FUNCTION f3(a type2) RETURN type2
AS
  res type2;
BEGIN
  res := a;
  RETURN res;
END;
/

DECLARE
  v1 type2;
  v2 type2 := type2(dnames_var('3a2', '3b2', '3c2'));
BEGIN
  INSERT INTO depts2 VALUES(3, f3(type2(dnames_var('3a', '3b', '3c'))));  
  INSERT INTO depts2 VALUES(4, f3(v2)); 
  v1 := f3(v2);
  FOR i IN v1.FIRST .. v1.LAST LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts2 ORDER BY id;

--pg_proc function OUT parameter datatype is SQL multilevel collection
CREATE OR REPLACE FUNCTION f4(a OUT type2) RETURN type2
AS
BEGIN
  a := type2(dnames_var('f4a', 'f4b'));
  RETURN type2(dnames_var('f4e', 'f4bf'));
END;
/

DECLARE
  v1 type2;
  v2 type2;
BEGIN
  v1 := f4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/


CREATE OR REPLACE FUNCTION f5(a OUT type2) RETURN VARCHAR2
AS
  res VARCHAR2(20) := 'RESULT';
BEGIN
  a := type2(dnames_var('f5a', 'f5b'));
  RETURN res;
END;
/
  
DECLARE
  v1 VARCHAR2(20);
  v2 type2;
BEGIN
  v1 := f5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/


--pg_proc function has two OUT multilevel collection parameters
CREATE OR REPLACE FUNCTION f6(a OUT type2, b OUT type2) RETURN type2
AS
  res type2;
BEGIN
  a := type2(dnames_var('f7a', 'f7b', 'f7c'));
  b := type2(dnames_var('f7d', 'f7e', 'f7f'));
  res := a;
  RETURN res;
END;
/

DECLARE
  v1 type2;
  v2 type2;
  v3 type2;
BEGIN
  v1 := f6(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP; 
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;

  FOR i IN v3.FIRST .. v3.LAST
  LOOP
    FOR j IN v3(i).FIRST .. v3(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v3(' || i || ')(' || j ||')= ' || v3(i)(j));
    END LOOP; 
  END LOOP;
END;
/

DROP FUNCTION f1;
DROP FUNCTION f2;
DROP FUNCTION f3;
DROP FUNCTION f4;
DROP FUNCTION f5;
DROP FUNCTION f6;
DROP TABLE depts;
DROP TABLE depts2;
DROP TYPE type2;
DROP TYPE dnames_var;



--inline function arguments and return datatype are multilevel collection
CREATE OR REPLACE TYPE dnames_var IS TABLE OF VARCHAR2(100);
CREATE OR REPLACE TYPE type2 IS VARRAY(10) OF dnames_var;
CREATE TABLE depts(id int, dept_names dnames_var);
CREATE TABLE depts2(id int, dept_names2 type2);

CREATE OR REPLACE PROCEDURE proc1(a int)
AS
  v1 type2;
  v2 type2;
  FUNCTION f1(a int) RETURN type2
  AS
    res type2 := type2(dnames_var('fa','fb'), dnames_var('fc','fd'));
  BEGIN
    RETURN res;
  END;
BEGIN
  INSERT INTO depts2 VALUES(1, f1(1));
 
  v1 := f1(1);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

CALL proc1(1);
SELECT * FROM depts2 ORDER BY id;

CREATE OR REPLACE PROCEDURE proc2(a int)
AS
  v1 type2;
  v2 type2;
  FUNCTION f2(a varchar(100)) RETURN type2
  AS
  BEGIN
    RETURN type2(dnames_var('fe','ff'), dnames_var('fh', a));
  END;
BEGIN

  INSERT INTO depts2 VALUES(2, f2('fe')); 
  
  v1 := f2('fe');
  INSERT INTO depts VALUES(2, v1(1)); 

  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/
CALL proc2(1);
SELECT * FROM depts ORDER BY id;
SELECT * FROM depts2 ORDER BY id;

--inline function IN parameter datatype is SQL multilevel collection
CREATE OR REPLACE PROCEDURE proc3(a int)
AS
  v1 type2;
  v2 type2 := type2(dnames_var('3a2', '3b2', '3c2'));
  FUNCTION f3(a type2) RETURN type2
  AS
    res type2;
  BEGIN
    res := a;
    RETURN res;
  END;
BEGIN
  INSERT INTO depts2 VALUES(3, f3(type2(dnames_var('3a', '3b', '3c'))));  
  INSERT INTO depts2 VALUES(4, f3(v2)); 
  v1 := f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;
END;
/

CALL proc3(1);
SELECT * FROM depts2 ORDER BY id;

--inline function OUT parameter datatype is SQL multilevel collection
CREATE OR REPLACE PROCEDURE proc4(a int)
AS
  v1 type2;
  v2 type2;
  FUNCTION f4(a OUT type2) RETURN type2
  AS
  BEGIN
    a := type2(dnames_var('f4a', 'f4b'));
    RETURN type2(dnames_var('f4e', 'f4bf'));
  END;
BEGIN
  v1 := f4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

CALL proc4(1);


CREATE OR REPLACE PROCEDURE proc5(a int)
AS
  v1 VARCHAR2(20);
  v2 type2;
  FUNCTION f5(a OUT type2) RETURN VARCHAR2
  AS
    res VARCHAR2(20) := 'RESULT';
  BEGIN
    a := type2(dnames_var('f5a', 'f5b'));
    RETURN res;
  END;
BEGIN
  v1 := f5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

CALL proc5(1);


--inline function has two OUT multilevel collection parameters
CREATE OR REPLACE PROCEDURE proc6(a int)
AS
  v1 type2;
  v2 type2;
  v3 type2;
  FUNCTION f6(a OUT type2, b OUT type2) RETURN type2
  AS
    res type2;
  BEGIN
    a := type2(dnames_var('f7a', 'f7b', 'f7c'));
    b := type2(dnames_var('f7d', 'f7e', 'f7f'));
    res := a;
    RETURN res;
  END;
BEGIN
  v1 := f6(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP; 
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;

END;
/

CALL proc6(1);

DROP PROCEDURE proc1;
DROP PROCEDURE proc2;
DROP PROCEDURE proc3;
DROP PROCEDURE proc4;
DROP PROCEDURE proc5;
DROP PROCEDURE proc6;
DROP TABLE depts;
DROP TABLE depts2;
DROP TYPE type2;
DROP TYPE dnames_var;

--package function arguments and return datatype are multilevel collection

CREATE OR REPLACE TYPE dnames_var IS TABLE OF VARCHAR2(100);
CREATE OR REPLACE TYPE type2 IS VARRAY(10) OF dnames_var;
CREATE TABLE depts(id int, dept_names dnames_var);
CREATE TABLE depts2(id int, dept_names2 type2);

CREATE OR REPLACE PACKAGE pkg1 AS
  FUNCTION f1(a int) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg1 AS
  FUNCTION f1(a int) RETURN type2
  AS
    res type2 := type2(dnames_var('fa','fb'), dnames_var('fc','fd'));
  BEGIN
    RETURN res;
  END;
END;
/

DECLARE
  v1 type2;
  v2 type2;
BEGIN
  INSERT INTO depts2 VALUES(1, pkg1.f1(1));
 
  v1 := pkg1.f1(1);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts2 ORDER BY id;


CREATE OR REPLACE PACKAGE pkg2 AS
  FUNCTION f2(a varchar(100)) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg2 AS
  FUNCTION f2(a varchar(100)) RETURN type2
  AS
  BEGIN
    RETURN type2(dnames_var('fe','ff'), dnames_var('fh', a));
  END;
END;
/

DECLARE
  v1 type2;
  v2 type2;
BEGIN
  INSERT INTO depts2 VALUES(2, pkg2.f2('fe')); 

  v1 := pkg2.f2('fe');
  INSERT INTO depts VALUES(2, v1(1)); 

  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  v2 := v1;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts ORDER BY id;
SELECT * FROM depts2 ORDER BY id;

--package function IN parameter datatype is SQL multilevel collection
CREATE OR REPLACE PACKAGE pkg3 AS
  FUNCTION f3(a type2) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg3 AS
  FUNCTION f3(a type2) RETURN type2
  AS
    res type2;
  BEGIN
    res := a;
    RETURN res;
  END;
END;
/

DECLARE
  v1 type2;
  v2 type2 := type2(dnames_var('3a2', '3b2', '3c2'));
BEGIN
  INSERT INTO depts2 VALUES(3, pkg3.f3(type2(dnames_var('3a', '3b', '3c'))));  
  INSERT INTO depts2 VALUES(4, pkg3.f3(v2)); 
  v1 := pkg3.f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;
END;
/

SELECT * FROM depts2 ORDER BY id;

--package function OUT parameter datatype is SQL multilevel collection
CREATE OR REPLACE PACKAGE pkg4 AS
  FUNCTION f4(a OUT type2) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg4 AS
  FUNCTION f4(a OUT type2) RETURN type2
  AS
  BEGIN
    a := type2(dnames_var('f4a', 'f4b'));
    RETURN type2(dnames_var('f4e', 'f4bf'));
  END;
END;
/

DECLARE
  v1 type2;
  v2 type2;
BEGIN
  v1 := pkg4.f4(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP;
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/


CREATE OR REPLACE PACKAGE pkg5 AS
  FUNCTION f5(a OUT type2) RETURN VARCHAR2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg5 AS
  FUNCTION f5(a OUT type2) RETURN VARCHAR2
  AS
    res VARCHAR2(20) := 'RESULT';
  BEGIN
    a := type2(dnames_var('f5a', 'f5b'));
    RETURN res;
  END;
END;
/

DECLARE
  v1 VARCHAR2(20);
  v2 type2;
BEGIN
  v1 := pkg5.f5(v2);
  DBMS_OUTPUT.PUT_LINE('v1 = ' || v1);

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP;
  END LOOP;
END;
/

--package function has two OUT multilevel collection parameters
CREATE OR REPLACE PACKAGE pkg6 AS
  FUNCTION f6(a OUT type2, b OUT type2) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg6 AS
  FUNCTION f6(a OUT type2, b OUT type2) RETURN type2
  AS
    res type2;
  BEGIN
    a := type2(dnames_var('f7a', 'f7b', 'f7c'));
    b := type2(dnames_var('f7d', 'f7e', 'f7f'));
    res := a;
    RETURN res;
  END;
END;
/

DECLARE
  v1 type2;
  v2 type2;
  v3 type2;
BEGIN
  v1 := pkg6.f6(v2, v3);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
    FOR j IN v1(i).FIRST .. v1(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v1(' || i || ')(' || j ||')= ' || v1(i)(j));
    END LOOP; 
  END LOOP;

  FOR i IN v2.FIRST .. v2.LAST
  LOOP
    FOR j IN v2(i).FIRST .. v2(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('v2(' || i || ')(' || j ||')= ' || v2(i)(j));
    END LOOP; 
  END LOOP;

END;
/

DROP PACKAGE pkg1;
DROP PACKAGE pkg2;
DROP PACKAGE pkg3;
DROP PACKAGE pkg4;
DROP PACKAGE pkg5;
DROP PACKAGE pkg6;
DROP TABLE depts;
DROP TABLE depts2;
DROP TYPE type2;
DROP TYPE dnames_var;





--the record field datatype is multilevelcollection
CREATE TYPE var_t IS VARRAY(4) OF DATE;
CREATE TYPE tab_t IS TABLE OF VARCHAR2(20);

DECLARE
  TYPE mix_rec IS RECORD(
    num_col NUMBER,
    var_col var_t,
    tab_col tab_t
  );
  rec mix_rec;
BEGIN
  rec.num_col := 999;
  rec.var_col := var_t(TO_DATE('2026-7-1'));
  rec.tab_col := tab_t('test1', 'test2');
  DBMS_OUTPUT.PUT_LINE('rec.var_col(1) = ' || rec.var_col(1));
  DBMS_OUTPUT.PUT_LINE('rec.tab_col(1) = ' || rec.tab_col(1));
END;
/

--raise an error, the datatype of the assignment is inconsistent with the dat type of the record field
DECLARE
  TYPE mix_rec IS RECORD(
    var_col var_t,
    tab_col tab_t
  );
  rec mix_rec;
BEGIN
  rec.var_col := tab_t('test1');
END;
/

DROP TYPE var_t;
DROP TYPE tab_t;


--pg_proc function arguments and return datatype are multilevel collection
CREATE OR REPLACE TYPE type1 IS VARRAY(10) OF INT;
CREATE OR REPLACE TYPE type2 IS TABLE OF type1;

CREATE OR REPLACE FUNCTION f1(a type1) RETURN type1
AS
  res type1;
  TYPE myrecord IS RECORD(id INT, depts type1);
  r myrecord;
BEGIN
  r.id := 1;
  r.depts := a;
  r.depts(1) := 100;
  RETURN r.depts;
END;
/

DECLARE
  invar  type1 := type1(1, 2, 3);
  res type1;
BEGIN
  res := f1(invar);
  FOR i IN res.FIRST .. res.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('res(' || i || ')= ' || res(i));
  END LOOP;  
END;
/

DROP FUNCTION f1(type1);

CREATE OR REPLACE FUNCTION f2(a type2) RETURN type2
AS
  res type2;
  TYPE myrecord IS RECORD(id INT, depts type2);
  r myrecord;
BEGIN
  r.id := 1;
  r.depts := a;
  r.depts(1)(1) := 100;
  r.depts(2)(1) := 200;
  RETURN r.depts;
END;
/

DECLARE
  invar  type1 := type1(1, 2, 3);
  invar2 type2 := type2(type1(10, 20), type1(30, 40));
  res type2;
BEGIN
  res := f2(invar2);
  FOR i IN res.FIRST .. res.LAST
  LOOP
    FOR j IN res(i).FIRST .. res(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('res(' || i || ')(' || j ||')= ' || res(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

DROP FUNCTION f2(type2);

CREATE OR REPLACE PROCEDURE p1(id int) AS
  TYPE record_type IS RECORD(c1 int, c2 type2);
  v1 type1 := type1(1, 2, 3);
  v2 type2;
  r record_type;
BEGIN
  v2 := type2(v1, v1);
  r.c1 := id;
  r.c2 := v2;

  FOR i IN r.c2.FIRST .. r.c2.LAST LOOP
    FOR j IN r.c2(i).FIRST .. r.c2(i).LAST LOOP
	  DBMS_OUTPUT.PUT_LINE('r.c2(' || i || ')(' || j ||')= ' || r.c2(i)(j));
    END LOOP;
  END LOOP;
END;
/

CALL p1(100);
DROP PROCEDURE p1;
DROP TYPE type2;
DROP TYPE type1;


--inline function arguments and return datatype are record
CREATE OR REPLACE TYPE type1 IS VARRAY(10) OF INT;
CREATE OR REPLACE TYPE type2 IS TABLE OF type1;

DECLARE
  invar2 type2 := type2(type1(10, 20), type1(30, 40));
  TYPE myrecord IS RECORD(id INT, depts type2);
  r myrecord;
  r2 myrecord;

  FUNCTION f2(a myrecord) RETURN myrecord
  AS
    res myrecord;
  BEGIN
    res := a;
    res.depts(1)(1) := 100;
    res.depts(2)(1) := 200;
    RETURN res;
  END;
BEGIN
  r.id := 1;
  r.depts := invar2;
  r2 := f2(r);

  FOR i IN r2.depts.FIRST .. r2.depts.LAST
  LOOP
    FOR j IN r2.depts(i).FIRST .. r2.depts(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('r2.depts(' || i || ')(' || j ||')= ' || r2.depts(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

DECLARE
  invar2 type2 := type2(type1(10, 20), type1(30, 40));
  res type2;
  TYPE myrecord IS RECORD(id INT, depts type2);
  r myrecord;
  r2 myrecord;

  FUNCTION f2(a type2, b OUT myrecord) RETURN type2
  AS
  BEGIN
    r.id := 1;
    r.depts := a;
    r.depts(1)(1) := 100;
    r.depts(2)(1) := 200;
	b := r;
    RETURN r.depts;
  END;
BEGIN
  res := f2(invar2, r2);
  FOR i IN res.FIRST .. res.LAST
  LOOP
    FOR j IN res(i).FIRST .. res(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('res(' || i || ')(' || j ||')= ' || res(i)(j));
    END LOOP; 
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('r2.id = ' || r2.id); 
  FOR i IN r2.depts.FIRST .. r2.depts.LAST
  LOOP
    FOR j IN r2.depts(i).FIRST .. r2.depts(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('r2.depts(' || i || ')(' || j ||')= ' || r2.depts(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

--package function arguments and return datatype are record
CREATE OR REPLACE PACKAGE pkg1 AS
  invar2 type2 := type2(type1(10, 20), type1(30, 40));
  TYPE myrecord IS RECORD(id INT, depts type2);
  r myrecord;
  r2 myrecord;
  FUNCTION f1(a myrecord) RETURN myrecord;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg1 AS
  FUNCTION f1(a myrecord) RETURN myrecord
  AS
    res myrecord;
  BEGIN
    res := a;
    res.depts(1)(1) := 100;
    res.depts(2)(1) := 200;
    RETURN res;
  END;
END;
/


BEGIN
  pkg1.r.id := 1;
  pkg1.r.depts := pkg1.invar2;
  pkg1.r2 := pkg1.f1(pkg1.r);

  FOR i IN  pkg1.r2.depts.FIRST ..  pkg1.r2.depts.LAST
  LOOP
    FOR j IN  pkg1.r2.depts(i).FIRST ..  pkg1.r2.depts(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE(' pkg1.r2.depts(' || i || ')(' || j ||')= ' ||  pkg1.r2.depts(i)(j));
    END LOOP; 
  END LOOP;  
END;
/


CREATE OR REPLACE PACKAGE pkg2 AS
  TYPE myrecord IS RECORD(id INT, depts type2);
  r myrecord;
  FUNCTION f2(a type2, b OUT myrecord) RETURN type2;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg2 AS
  FUNCTION f2(a type2, b OUT myrecord) RETURN type2
  AS
  BEGIN
    r.id := 1;
    r.depts := a;
    r.depts(1)(1) := 100;
    r.depts(2)(1) := 200;
	b := r;
    RETURN r.depts;
  END;
END;
/

DECLARE
  invar2 type2 := type2(type1(10, 20), type1(30, 40));
  res type2;
  r2 pkg2.myrecord;
BEGIN
  res := pkg2.f2(invar2, r2);
  FOR i IN res.FIRST .. res.LAST
  LOOP
    FOR j IN res(i).FIRST .. res(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('res(' || i || ')(' || j ||')= ' || res(i)(j));
    END LOOP; 
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('r2.id = ' || r2.id); 
  FOR i IN r2.depts.FIRST .. r2.depts.LAST
  LOOP
    FOR j IN r2.depts(i).FIRST .. r2.depts(i).LAST
    LOOP
       DBMS_OUTPUT.PUT_LINE('r2.depts(' || i || ')(' || j ||')= ' || r2.depts(i)(j));
    END LOOP; 
  END LOOP;  
END;
/

DROP PACKAGE pkg1;
DROP PACKAGE pkg2;
DROP TYPE type2;
DROP TYPE type1;




--Execpetions
--the limit value must be greater than 0
CREATE TYPE varr_type_err AS VARRAY(0) OF INT;

--assign a value for a IN MODE collection variable in a function, raises an error
CREATE TYPE depts_id IS TABLE OF INT;
CREATE OR REPLACE FUNCTION err_func2(p_in IN depts_id) RETURN INT AS
BEGIN
  p_in.EXTEND;
  p_in(1) := 999;
  RETURN 1;
END;
/
DROP FUNCTION err_func2;
DROP TYPE depts_id;


--assign a value for a IN MODE collection variable in a package, raises an error
CREATE TYPE var_num IS VARRAY(3) OF NUMBER;

CREATE OR REPLACE PACKAGE pkg_err1 AS
  FUNCTION test_func(p IN var_num) RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg_err1 AS
  FUNCTION test_func(p IN var_num) RETURN NUMBER
  IS
  BEGIN
    p.EXTEND;
    RETURN p.COUNT;
  END;
END;
/

DROP PACKAGE pkg_err1;
DROP TYPE var_num;


--The actual argument types and formal parameter types of the package function do not match.
CREATE TYPE tab_num IS TABLE OF INT;
CREATE TYPE arr_num IS VARRAY(5) OF INT;

CREATE OR REPLACE PACKAGE pkg_type AS
  FUNCTION get_tab(p tab_num) RETURN tab_num;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg_type AS
  FUNCTION get_tab(p tab_num) RETURN tab_num IS
  BEGIN
    RETURN p;
  END;
END;
/

DECLARE
  wrong_param arr_num := arr_num(1,2);
  res tab_num;
BEGIN
  res := pkg_type.get_tab(wrong_param);
END;
/

DROP PACKAGE pkg_type;
DROP TYPE tab_num;
DROP TYPE arr_num;


--disallowed Nested Table or Varray column in a Temporary table or a Partitioned table
CREATE TYPE dnames_var IS VARRAY(7) OF VARCHAR2(30);
CREATE TYPE typeVar IS TABLE OF VARCHAR2(20);
CREATE TABLE t_col(name dnames_var, dept typeVar);

CREATE GLOBAL TEMPORARY TABLE temp_coll_tab(
    id NUMBER,
    var_col dnames_var,
    tab_col typeVar
) ON COMMIT PRESERVE ROWS;

CREATE GLOBAL TEMPORARY TABLE temp_coll_tab2
   ON COMMIT PRESERVE ROWS 
   AS SELECT name, dept FROM t_col;

CREATE GLOBAL TEMPORARY table temp_coll_tab3 (LIKE t_col);

DROP TABLE t_col;
DROP TYPE dnames_var;
DROP TYPE typeVar;


--ALTER TYPE
--MODIFY LIMIT is supported only on schema-level varray types
CREATE TYPE ntype IS TABLE OF INT;
ALTER TYPE ntype MODIFY LIMIT 10 CASCADE;
DROP TYPE ntype;

CREATE TYPE vtype IS VARRAY(5) OF INT;
ALTER TYPE vtype MODIFY LIMIT 2 CASCADE;
ALTER TYPE vtype MODIFY LIMIT 10 CASCADE;
ALTER TYPE vtype MODIFY LIMIT 10 CASCADE;
DROP TYPE vtype;

--MODIFY ELEMENT TYPE datatype
CREATE TYPE type1 IS TABLE OF VARCHAR2(20);
ALTER TYPE type1 MODIFY ELEMENT TYPE VARCHAR2(64);
ALTER TYPE type1 MODIFY ELEMENT TYPE VARCHAR2(50);
ALTER TYPE type1 MODIFY ELEMENT TYPE INT;
DROP TYPE type1;

CREATE TYPE type2 IS TABLE OF NVARCHAR2(20);
ALTER TYPE type2 MODIFY ELEMENT TYPE NVARCHAR2(64);
ALTER TYPE type2 MODIFY ELEMENT TYPE NVARCHAR2(50);
DROP TYPE type2;

CREATE TYPE type3 IS VARRAY(10) OF NUMBER(11, 2);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(11, 6);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(15, 2);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(10, 2);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(15, 5);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(16, 2);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER(19, 7);
ALTER TYPE type3 MODIFY ELEMENT TYPE NUMBER;
DROP TYPE type3;

CREATE TYPE type4 IS VARRAY(10) OF numeric(11, 2);
ALTER TYPE type4 MODIFY ELEMENT TYPE NUMBER(19, 5);
ALTER TYPE type4 MODIFY ELEMENT TYPE numeric(29, 10);
ALTER TYPE type4 MODIFY ELEMENT TYPE numeric(29, 10);
ALTER TYPE type4 MODIFY ELEMENT TYPE numeric;
DROP TYPE type4;

CREATE TYPE type5 IS TABLE OF RAW(2);
ALTER TYPE type5 MODIFY ELEMENT TYPE RAW(5);
ALTER TYPE type5 MODIFY ELEMENT TYPE RAW(3);
DROP TYPE type5;

CREATE TYPE type6 IS TABLE OF binary_float;
ALTER TYPE type6 MODIFY ELEMENT TYPE binary_double;
DROP TYPE type6;


--alter type cascade
--table dependency
CREATE TYPE vtype IS VARRAY(5) OF INT;
CREATE TABLE tab_test(c1 vtype);

ALTER TYPE vtype MODIFY LIMIT 10;
ALTER TYPE vtype MODIFY LIMIT 11 CASCADE;
DROP TABLE tab_test;
DROP TYPE vtype;

--type dependency
CREATE TYPE vtype IS VARRAY(2) OF INT;
CREATE TYPE vtype2 IS VARRAY(3) OF vtype;

DECLARE
  v1 vtype;
  v2 vtype2;
BEGIN
  v1 := vtype(1, 2, 3, 4, 5);
  v2 := vtype2(v1, v1, v1);
END;
/

ALTER TYPE vtype MODIFY LIMIT 4;
ALTER TYPE vtype MODIFY LIMIT 5 CASCADE;

DECLARE
  v1 vtype;
  v2 vtype2;
BEGIN
  v1 := vtype(1, 2, 3, 4, 5);
  v2 := vtype2(v1, v1, v1);
END;
/
DROP TYPE vtype2;
DROP TYPE vtype;


--DROP TYPE
--force/validate
CREATE TYPE vtype IS VARRAY(5) OF INT;
CREATE TYPE vtype2 IS TABLE OF vtype;
DROP TYPE vtype;
DROP TYPE vtype VALIDATE;
DROP TYPE vtype FORCE;
SELECT COUNT(*) FROM pg_type WHERE typname like 'vtype';
SELECT COUNT(*) FROM pg_type WHERE typname like 'vtype2';

CREATE TYPE vtype IS VARRAY(3) OF INT;
CREATE TABLE tab_test(id int, c1 vtype);
DROP TYPE vtype;
DROP TYPE vtype VALIDATE;
DROP TYPE vtype FORCE;
SELECT COUNT(*) FROM pg_type WHERE typname like 'vtype';
DROP TABLE tab_test;

--cascade
CREATE TYPE vtype IS VARRAY(3) OF INT;
CREATE TYPE vtype2 IS VARRAY(5) OF vtype;
DROP TYPE vtype CASCADE;
SELECT COUNT(*) FROM pg_type WHERE typname like 'vtype';
SELECT COUNT(*) FROM pg_type WHERE typname like 'vtype2';

--CREATE SYNONYM FOR collection type
CREATE OR REPLACE EDITIONABLE TYPE dnames_var IS VARRAY(10) OF VARCHAR2(100);
CREATE TABLE depts(id int, dept_names dnames_var);
CREATE SYNONYM s_type1 for dnames_var;

INSERT INTO depts VALUES(0, s_type1('a0', 'b0'));

DECLARE
  a s_type1;
BEGIN
  a := s_type1('a1', 'b1');
  INSERT INTO depts values(1, a);
  INSERT INTO depts values(2, s_type1('a2', 'b2'));
END;
/
SELECT * FROM depts ORDER BY id;

CREATE OR REPLACE FUNCTION fun1(a s_type1) RETURN s_type1
AS
  res s_type1;
BEGIN
  res := a;
  RETURN res;
END;
/

DECLARE
BEGIN 
  INSERT INTO depts VALUES(3, fun1(s_type1('3a', '3b', '3c'))); 
END;
/

SELECT * FROM depts ORDER BY id;
DROP FUNCTION fun1(s_type1);

CREATE OR REPLACE PROCEDURE proc1(a int)
AS
  v1 s_type1;
  v2 s_type1 := s_type1('3a2', '3b2', '3c2');
  FUNCTION f3(a s_type1) RETURN s_type1
  AS
    res s_type1;
  BEGIN
    res := a;
    RETURN res;
  END;
BEGIN
  v1 := f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/
CALL proc1(1);
DROP PROCEDURE proc1(int);

CREATE OR REPLACE PROCEDURE proc1(a int)
AS
  v1 s_type1;
  v2 s_type1 := s_type1('3a2', '3b2', '3c2');
  FUNCTION f3(a s_type1) RETURN s_type1
  AS
    res s_type1;
  BEGIN
    res := a;
    RETURN res;
  END;
BEGIN
  INSERT INTO depts VALUES(4, f3(s_type1('3a', '3b', '3c')));  
  INSERT INTO depts VALUES(5, f3(v2)); 
  v1 := f3(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
END;
/

CALL proc1(1);
SELECT * FROM depts ORDER BY id;
DROP PROCEDURE proc1(int);

CREATE OR REPLACE PACKAGE pkg1 AS
  FUNCTION f1(a OUT s_type1) RETURN s_type1;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg1 AS
  FUNCTION f1(a OUT s_type1) RETURN s_type1
  AS
  BEGIN
    a := s_type1('f4a', 'f4b');
    RETURN s_type1('f4e', 'f4f');
  END;
END pkg1;
/

DECLARE
  v1 s_type1;
  v2 s_type1;
BEGIN
  v1 := pkg1.f1(v2);
  FOR i IN v1.FIRST .. v1.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v1(' || i || ')= ' || v1(i));
  END LOOP;
  FOR i IN v2.FIRST .. v2.LAST
  LOOP
     DBMS_OUTPUT.PUT_LINE('v2(' || i || ')= ' || v2(i));
  END LOOP;
END;
/

DROP PACKAGE pkg1;
DROP TABLE depts;
DROP SYNONYM s_type1;
DROP TYPE dnames_var;