CREATE TABLE dyn_student (id int PRIMARY KEY, name text, score number);

-- 1. base test
CREATE SEQUENCE student_seq;

CREATE OR REPLACE PROCEDURE create_stu (
  stuid IN OUT NUMBER,
  name  IN     VARCHAR2,
  score  OUT     NUMBER
) AUTHID DEFINER AS
DECLARE
  max_score NUMBER(6) := 99;
BEGIN
  stuid := student_seq.NEXTVAL;
  INSERT INTO dyn_student VALUES (stuid, name, max_score);
  score := max_score;
END;
/

DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN OUT new_stuid, new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

-- error: IN bound to INOUT
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING new_stuid, new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

-- error: OUT bound to INOUT
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING OUT new_stuid, new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

-- error: OUT bound to IN
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN OUT new_stuid, OUT new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

-- success: INOUT bound to IN
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN OUT new_stuid, IN OUT new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

-- error: IN bound to OUT
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN OUT new_stuid, new_name, IN new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

--error: OUT bound to ALL IN
CREATE OR REPLACE PROCEDURE create_stu_2 (
  stuid IN NUMBER,
  name  IN VARCHAR2,
  score IN NUMBER
) AUTHID DEFINER AS
BEGIN
  INSERT INTO dyn_student VALUES (stuid, name, score);
END;
/

DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4) := 11;
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu_2(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN new_stuid, IN new_name, OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

DROP PROCEDURE create_stu_2;

-- success: INOUT bound to OUT
DECLARE
  plsql_block VARCHAR2(500);
  new_stuid  NUMBER(4);
  new_name   VARCHAR2(30) := 'xx';
  new_score   NUMBER;
BEGIN
  plsql_block := 'BEGIN create_stu(:a, :b, :c); END;';
  EXECUTE IMMEDIATE plsql_block
    USING IN OUT new_stuid, new_name, IN OUT new_score;
  RAISE NOTICE 'stuid is %, score is %', new_stuid, new_score;
END;
/

SELECT * FROM dyn_student order by id;
DROP PROCEDURE create_stu;
DROP SEQUENCE student_seq;

-- 2. boolean type parameter
CREATE OR REPLACE PROCEDURE pro_bool_test (x BOOLEAN) AUTHID DEFINER AS
BEGIN
  IF x THEN
    RAISE NOTICE 'x is true';
  ELSE
    RAISE NOTICE 'x is false';
  END IF;
END;
/

DECLARE
  dyn_str VARCHAR2(200);
  b        BOOLEAN := TRUE;
  c        BOOLEAN := FALSE;
BEGIN
  dyn_str := 'BEGIN pro_bool_test(:x); END;';
  EXECUTE IMMEDIATE dyn_str USING b;
  EXECUTE IMMEDIATE dyn_str USING c;
END;
/

DROP PROCEDURE pro_bool_test;

-- 3. record type parameter
CREATE OR REPLACE PACKAGE pkg_record_test AUTHID DEFINER AS
  TYPE rec IS RECORD (n1 NUMBER, n2 NUMBER);
  PROCEDURE p (i OUT rec, j NUMBER, k NUMBER);
END pkg_record_test;
/

CREATE OR REPLACE PACKAGE BODY pkg_record_test AS
  PROCEDURE p (i OUT rec, j NUMBER, k NUMBER) AS
  BEGIN
    i.n1 := j;
    i.n2 := k;
  END p;
END pkg_record_test;
/

DECLARE
  r       pkg_record_test.rec;
  dyn_str VARCHAR2(3000);
BEGIN
  dyn_str := 'BEGIN pkg_record_test.p(:x, 3, 4); END;';
  EXECUTE IMMEDIATE dyn_str USING OUT r;
      RAISE NOTICE 'r = %', r;
END;
/

-- package record
CREATE OR REPLACE PACKAGE pkg_student_vars AS
  g_student_rec pkg_record_test.rec;
END pkg_student_vars;
/

DECLARE
  dyn_str VARCHAR2(3000);
BEGIN
  dyn_str := 'BEGIN pkg_record_test.p(:x, 3, 4); END;';
  EXECUTE IMMEDIATE dyn_str USING OUT pkg_student_vars.g_student_rec;
      RAISE NOTICE 'r = %', pkg_student_vars.g_student_rec;
END;
/

BEGIN
  RAISE NOTICE 'r = %', pkg_student_vars.g_student_rec;
END;
/

DROP PACKAGE pkg_record_test;

-- 4. NULL parameter
CREATE TABLE dyn_student_temp AS SELECT * FROM dyn_student;

DECLARE
  var_null  CHAR(1);  -- Set to NULL automatically at run time
  id        NUMBER(3) := 1;
BEGIN
  EXECUTE IMMEDIATE 'UPDATE dyn_student_temp SET name = :x where id = :y'
    USING var_null, id;
END;
/

SELECT * FROM dyn_student_temp order by id;
DROP TABLE dyn_student_temp;

-- 5. cursor type paramter
DECLARE
  TYPE StuCurTyp  IS REF CURSOR;
  v_stu_cursor    StuCurTyp;
  stu_record      dyn_student%ROWTYPE;
  v_stmt_str      VARCHAR2(200);
  v_id            dyn_student.id%TYPE;
BEGIN
  v_stmt_str := 'SELECT * FROM dyn_student WHERE id = :j';
  OPEN v_stu_cursor FOR v_stmt_str USING 2;
  LOOP
    FETCH v_stu_cursor INTO stu_record;
    EXIT WHEN v_stu_cursor%NOTFOUND;
        RAISE NOTICE 'stu_record: %', stu_record;
  END LOOP;
  CLOSE v_stu_cursor;
END;
/

-- 6. repeat placeholder
CREATE PROCEDURE pro_add_int (
  a INTEGER, b INTEGER,
  c INTEGER, d INTEGER )
IS
BEGIN
      RAISE NOTICE 'a + b + c + d = %', a+b+c+d;
END;
/

DECLARE
  a INTEGER := 1;
  b INTEGER := 2;
  plsql_block VARCHAR2(100);
BEGIN
  plsql_block := 'BEGIN pro_add_int(:x, :x, :y, :x); END;';
  EXECUTE IMMEDIATE plsql_block USING a, b;
  EXECUTE IMMEDIATE plsql_block USING b, a;
END;
/

DECLARE
  a INTEGER := 1;
  b INTEGER := 2;
  plsql_block VARCHAR2(100);
BEGIN
  plsql_block := 'BEGIN pro_add_int(:x, :x, :y, :x); END;';
  EXECUTE IMMEDIATE plsql_block USING a, a, b, a; --failed
END;
/

DROP PROCEDURE pro_add_int;

-- 7. RETURNING
DECLARE
  v_stu_id NUMBER := 1;
  v_updated_name dyn_student.name%TYPE;
  v_updated_score dyn_student.score%TYPE;
BEGIN
  EXECUTE IMMEDIATE '
    UPDATE dyn_student 
    SET name = ''yy'', score = score - 5
    WHERE id = :1
    RETURNING name, score INTO :2, :3'
  USING v_stu_id
  RETURNING INTO v_updated_name, v_updated_score;
  raise notice 'v_updated_name: %', v_updated_name;
  raise notice 'v_updated_score: %', v_updated_score;
  COMMIT;
END;
/

DECLARE
  new_id integer := 2;
  stu_name varchar2(30);
  stu_score NUMBER;
BEGIN
  EXECUTE IMMEDIATE '
    UPDATE dyn_student 
    SET score = score - 6 
    WHERE id = :1 
    RETURNING name, score into :2,:3' 
  USING new_id, OUT stu_name, OUT stu_score;
  RAISE NOTICE 'name %, score %', stu_name, stu_score;
END;
/

-- 8. PACKAGE
CREATE OR REPLACE PACKAGE pkg_company_vars AS
  g_company_name VARCHAR2(100) := 'ACME Corporation';
  g_max_salary NUMBER := 100000;
  g_active_status VARCHAR2(10) := 'ACTIVE';
END pkg_company_vars;
/

DECLARE
  v_sql VARCHAR2(1000);
  v_result VARCHAR2(100);
BEGIN
  v_sql := 'BEGIN :result := pkg_company_vars.g_company_name; END;';
  EXECUTE IMMEDIATE v_sql USING OUT v_result;
  RAISE NOTICE 'Dynamic access: %', v_result;

  v_sql := 'BEGIN :result := pkg_company_vars.g_max_salary; END;';
  EXECUTE IMMEDIATE v_sql USING OUT v_result;
  RAISE NOTICE 'Max salary: %', v_result;
END;
/

CREATE OR REPLACE PACKAGE pkg_student_vars AS
  g_student_name VARCHAR2(100) := 'None';
  g_student_score NUMBER := 0;
END pkg_student_vars;
/

DECLARE
  v_sql VARCHAR2(1000);
  v_id NUMBER := 2;
BEGIN
  v_sql := 'UPDATE dyn_student SET name = ''zz''
            WHERE id = :v_id
            RETURNING name, score INTO :1, :2';
  EXECUTE IMMEDIATE v_sql USING v_id
    RETURNING INTO pkg_student_vars.g_student_name, pkg_student_vars.g_student_score;
  RAISE NOTICE 'name: %, score: %', pkg_student_vars.g_student_name, pkg_student_vars.g_student_score;
END;
/

BEGIN
  RAISE NOTICE 'name: %, score: %', pkg_student_vars.g_student_name, pkg_student_vars.g_student_score;
END;
/

-- final check
SELECT id, name, score FROM dyn_student order by id;

DROP TABLE dyn_student;