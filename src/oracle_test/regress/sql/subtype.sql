----------------------------------------
----------------test usage--------------
----------------------------------------
--1. in anonymous block.
DECLARE
  SUBTYPE my_subtype IS INTEGER;
  val1 my_subtype := 123;
BEGIN
  raise notice 'val1: %', val1;
END;
/

--2. in procedure
CREATE OR REPLACE PROCEDURE my_pro AS
  SUBTYPE my_subtype IS INTEGER;
  val1 my_subtype := 123;
BEGIN
  raise notice 'val1: %', val1;
END;
/
CALL my_pro();
DROP PROCEDURE my_pro;

--3. in function
CREATE OR REPLACE FUNCTION my_func RETURN INTEGER AS
  SUBTYPE my_subtype IS INTEGER;
  val1 my_subtype := 123;
  val2 my_subtype;
  FUNCTION my_sub_func(p1 my_subtype) RETURN my_subtype AS
  BEGIN
    RETURN p1;
  END;
BEGIN
  val2 := my_sub_func(val1);
  raise notice 'val2: %', val2;
  RETURN 1;
END;
/
SELECT my_func() FROM DUAL;
DROP FUNCTION my_func();

--4. in package
CREATE OR REPLACE PACKAGE my_pkg AS
  SUBTYPE age IS INTEGER;
  PROCEDURE display_info(p_age IN age);
  FUNCTION get_default_age RETURN age;
END my_pkg;
/

CREATE OR REPLACE PACKAGE BODY my_pkg AS
  PROCEDURE display_info(p_age IN age) IS
  DECLARE
    SUBTYPE my_subtype IS INTEGER;
    score my_subtype := 60;
  BEGIN
    raise notice 'Age: %, Score: %', p_age, score;
  END display_info;
  FUNCTION get_default_age RETURN age IS
  BEGIN
    RETURN 30;
  END get_default_age;
END my_pkg;
/

DECLARE
  v_age my_pkg.age := 45;
BEGIN
  my_pkg.display_info(p_age => v_age);
  raise notice 'Default age: %', my_pkg.get_default_age();
END;
/
DROP PACKAGE my_pkg;

----------------------------------------
--------------test basetype-------------
----------------------------------------
--1. use simple type as basetype
DECLARE
  SUBTYPE my_subtype1 IS VARCHAR(10);
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype1;
  val2 my_subtype2;
BEGIN
  val1 := 'test';
  raise notice 'val1: %', val1;
  val2 := val1;
  raise notice 'val2: %', val2;
END;
/

--2. use array as basetype
DECLARE
  SUBTYPE my_subtype1 IS int[];
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype1;
  val2 my_subtype2;
BEGIN
  val1 := ARRAY[1,2,3,4];
  raise notice 'val1: %', val1[2];
  val2 := val1;
  raise notice 'val2: %', val2[2];
END;
/

--3. use composite type as basetype
CREATE TYPE rec_type AS (id INTEGER, name VARCHAR2(30));
DECLARE
  SUBTYPE my_subtype1 IS rec_type;
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype1;
  val2 my_subtype2;
BEGIN
  val1.id := 100;
  val1.name := 'John';
  raise notice 'val1: %', val1;
  val2 := val1;
  raise notice 'val2: %', val2;
END;
/
DROP TYPE rec_type;

--4. use explicit cursor as basetype
CREATE TABLE employees(id INTEGER, name VARCHAR2(30));
INSERT INTO employees VALUES(100, 'John');
DECLARE
  CURSOR c1 IS SELECT * FROM employees;
  SUBTYPE my_subtype1 IS c1%rowtype;
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype1;
  val2 my_subtype2;
BEGIN
  OPEN c1;
  FETCH c1 INTO val1;
  raise info 'val1: %', val1;
  CLOSE c1;
  val2 := val1;
  raise info 'val2: %', val2;
END;
/
DROP TABLE employees;

--5. use udt type cursor as basetype
CREATE TABLE employees(id INTEGER, name VARCHAR2(30));
INSERT INTO employees VALUES(100, 'John');
DECLARE
  TYPE empcurtyp IS REF CURSOR RETURN employees%ROWTYPE;
  SUBTYPE my_subtype1 IS empcurtyp;
  SUBTYPE my_subtype2 IS my_subtype1;
  cursor1 my_subtype1;
  cursor2 my_subtype2;
  val1 employees%ROWTYPE;
  val2 employees%ROWTYPE;
BEGIN
  OPEN cursor1 FOR SELECT * FROM employees;
  FETCH cursor1 INTO val1;
  raise info 'val1: %', val1;
  CLOSE cursor1;
  OPEN cursor2 FOR SELECT * FROM employees;
  FETCH cursor2 INTO val2;
  raise info 'val2: %', val2;
  CLOSE cursor2;
END;
/
DROP TABLE employees;

--6. use ora record as basetype
DECLARE
  TYPE rec_type IS RECORD (id INTEGER, name VARCHAR2(30));
  SUBTYPE my_subtype1 IS rec_type;
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype1;
  val2 my_subtype2;
BEGIN
  val1.id := 100;
  val1.name := 'John';
  raise notice 'val1: %', val1;
  val2.id := 100;
  val2.name := 'John';
  raise notice 'val2: %', val2;
END;
/

----------------------------------------
-------------test constraint------------
----------------------------------------
--1. not null constraint
DECLARE
  SUBTYPE my_subtype1 IS INTEGER NOT NULL;
  val1 my_subtype1;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER;
  val1 my_subtype1 NOT NULL;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER NOT NULL;
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype2;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER;
  SUBTYPE my_subtype2 IS my_subtype1 NOT NULL;
  val1 my_subtype2;
BEGIN
  NULL;
END;
/

--2. typmod constraint
DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2(8);
  val1 my_subtype1 := 'HelloWorld';
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2;
  val1 my_subtype1(8) := 'HelloWorld';
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2(8);
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype2 := 'HelloWorld';
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2;
  SUBTYPE my_subtype2 IS my_subtype1(8);
  val1 my_subtype2 := 'HelloWorld';
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2(20);
  SUBTYPE my_subtype2 IS my_subtype1(8);
  val1 my_subtype2 := 'HelloWorld';
BEGIN
  NULL;
END;
/

--success
DECLARE
  SUBTYPE my_subtype1 IS VARCHAR2(8);
  SUBTYPE my_subtype2 IS my_subtype1(20);
  val1 my_subtype2 := 'HelloWorld';
BEGIN
  NULL;
END;
/

CREATE OR REPLACE PACKAGE my_pkg AS
  SUBTYPE name1 IS VARCHAR2(10);
  SUBTYPE name2 IS VARCHAR2(20);
  PROCEDURE display_info(p_name1 IN name1);
  PROCEDURE display_info(p_name2 IN name2);
END my_pkg;
/

CREATE OR REPLACE PACKAGE BODY my_pkg AS
  PROCEDURE display_info(p_name1 IN name1) IS
  BEGIN
    raise notice 'Name1: %', p_name1;
  END display_info;
  PROCEDURE display_info(p_name2 IN name2) IS
  BEGIN
    raise notice 'Name2: %', p_name2;
  END display_info;
END my_pkg;
/

DECLARE
  my_name1 my_pkg.name1 := 'Bob';
  my_name2 my_pkg.name2 := 'John';
BEGIN
  my_pkg.display_info(p_name1 => my_name2);
  my_pkg.display_info(p_name2 => my_name1);
END;
/
DROP PACKAGE my_pkg;

--3. range constraint
DECLARE
  SUBTYPE my_subtype1 IS INTEGER RANGE -9..9;
  val1 my_subtype1 := 5;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER RANGE -9..9;
  val1 my_subtype1 := -5;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER RANGE -9..9;
  val1 my_subtype1 := 10;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER RANGE 9..-9;
  val1 my_subtype1 := 10;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER RANGE -9..9;
  SUBTYPE my_subtype2 IS my_subtype1;
  val1 my_subtype2 := 10;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS INTEGER;
  SUBTYPE my_subtype2 IS my_subtype1 RANGE -9..9;
  val1 my_subtype2 := 10;
BEGIN
  NULL;
END;
/

DECLARE
  SUBTYPE my_subtype1 IS FLOAT RANGE -9..9;
  val1 my_subtype1 := 10;
BEGIN
  NULL;
END;
/
