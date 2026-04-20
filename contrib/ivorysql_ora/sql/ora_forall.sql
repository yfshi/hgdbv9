--1 FORALL index IN lower_bound .. upper_bound
--2 FORALL index IN VALUES OF
--3 FORALL index IN INDEX OF
--4 SQL%BULK_ROWCOUNT
--5 BULK COLLECT INTO in FORALL Statement
--6 dynamic sql in FORALL
--7 EXCEPTION

CREATE TABLE t1(id1 int, id2 varchar(20));
CREATE TABLE t2(id1 int, id2 varchar(20));
INSERT INTO t1 values(11, 'a');
INSERT INTO t1 values(12, 'b');
INSERT INTO t1 values(13, 'c');

CREATE TABLE employee(empno int, ename varchar(20), departno int);
CREATE TABLE employees_temp AS SELECT * FROM employee;

--1 FORALL index IN lower_bound .. upper_bound
DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF varchar(20);
  a mytype;
  b mytype2;
BEGIN
  SELECT id1, id2 BULK COLLECT INTO a, b FROM t1;
  FORALL i IN 1 .. 3
   insert into t2 values (a(i),b(i)); 
END;
/

SELECT * FROM t2 ORDER BY id1;

DELETE FROM t2;
DECLARE
  TYPE mytype IS VARRAY(100) OF INT;
  TYPE mytype2 IS TABLE OF varchar(20);
  a mytype;
  b mytype2;
BEGIN
  SELECT id1, id2 BULK COLLECT INTO a, b FROM t1;
  FORALL i IN 1 .. 3
   insert into t2 values (a(i),b(i)); 
END;
/
SELECT * FROM t2 ORDER BY id1;

DECLARE
  TYPE mytype IS VARRAY(100) OF INT;
  TYPE mytype2 IS TABLE OF varchar(20);
  a mytype := mytype(20, 30, 40);
  b mytype2 := mytype2('newa', 'newb', 'newc');
  c mytype := mytype(11, 12, 13);
BEGIN
  FORALL i IN 1 .. 3
   update t2 set id1 = a(i), id2 = b(i) where id1 = c(i); 
END;
/
SELECT * FROM t2 ORDER BY id1;


DECLARE
  TYPE mytype IS VARRAY(100) OF INT;
  TYPE mytype2 IS TABLE OF varchar(20);
  a mytype := mytype(20, 30, 40);
  b mytype2 := mytype2('newa', 'newb', 'newc');
BEGIN
  FORALL i IN 1 .. 3
    delete from t2 where id1 = a(i) and id2 = b(i); 
END;
/
SELECT * FROM t2 ORDER BY id1;


--merge
CREATE TABLE source_table (
    id NUMBER,
    name VARCHAR2(100),
    salary NUMBER
);

CREATE TABLE target_table (
    id NUMBER PRIMARY KEY,
    name VARCHAR2(100),
    salary NUMBER
);

INSERT INTO source_table VALUES (1, 'Alice', 5000);
INSERT INTO source_table VALUES (2, 'Bob', 6000);
INSERT INTO source_table VALUES (3, 'Charlie', 5500);
INSERT INTO source_table VALUES (4, 'David', 7000);
INSERT INTO source_table VALUES (5, 'Eve', 4500);

DECLARE
    TYPE id_array_type IS TABLE OF NUMBER INDEX BY INTEGER;
    TYPE name_array_type IS TABLE OF VARCHAR2(100) INDEX BY INTEGER;
    TYPE salary_array_type IS TABLE OF NUMBER INDEX BY INTEGER;

    v_ids id_array_type;
    v_names name_array_type;
    v_salaries salary_array_type;
BEGIN
    SELECT id, name, salary 
    BULK COLLECT INTO v_ids, v_names, v_salaries
    FROM source_table;

    FORALL i IN 1..v_ids.COUNT
        MERGE INTO target_table t
        USING (SELECT v_ids(i) AS id, v_names(i) AS name, v_salaries(i) AS salary FROM dual) s
        ON (t.id = s.id)
        WHEN MATCHED THEN
            UPDATE SET t.salary = s.salary, t.name = s.name
        WHEN NOT MATCHED THEN
            INSERT (id, name, salary) 
            VALUES (s.id, s.name, s.salary);
    
    --DBMS_OUTPUT.PUT_LINE('Merged ' || SQL%ROWCOUNT || ' rows');
END;
/
SELECT * FROM target_table ORDER BY id;
DROP TABLE source_table;
DROP TABLE target_table;


DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER INDEX BY INTEGER;
  TYPE IntList2 IS TABLE OF IntList INDEX BY INTEGER;
  depts NumList := NumList(1, 2);
  ids IntList;
  ids2 IntList2;
BEGIN
  ids(1) := 10;
  ids(2) := 20;
  ids2(1) := ids;
  ids2(2) := ids;

  FORALL i IN 1 .. 2
    INSERT INTO employees_temp(empno, departno) VALUES(ids2(1)(1), depts(i));
END;
/
---------------

DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER INDEX BY INTEGER;
  TYPE IntList2 IS TABLE OF IntList INDEX BY INTEGER;

  depts NumList := NumList(1, 2);
  ids IntList;
  ids2 IntList2;
BEGIN
  ids(1) := 10;
  ids(2) := 20;
  ids2(1) := ids;
  ids2(2) := ids;

  FORALL i IN 1 .. 2
    INSERT INTO employees_temp(empno, departno) VALUES(ids2(1)(i), depts(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  FORALL i IN 3 .. 1
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  FORALL i IN 1.2 .. 2.6
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/ 
SELECT * FROM employees_temp ORDER BY empno;


--Exceptions
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  ids.delete(3);
  FORALL i IN 1 .. 3
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 10);
BEGIN
  FORALL i IN depts.FIRST .. depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(i);

  DBMS_OUTPUT.PUT_LINE('i is  ' || i);
END;
/

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 10);
BEGIN
  FORALL i IN depts.FIRST .. depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(i) + i;
END;
/

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 10);
BEGIN
  FORALL i IN depts.FIRST .. depts.LAST
    DELETE FROM employees_temp;
END;
/

DECLARE
  TYPE IntList IS TABLE OF INTEGER;
  ids IntList := IntList(7, 8, 9);
BEGIN
  FORALL i IN 1 .. 2
    INSERT INTO employees_temp(empno) VALUES(ids(1));
END;
/

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  FORALL i IN -1 .. 3
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  FORALL i IN 2 .. 5
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  depts NumList := NumList(1, 2, 30);
  ids IntList := IntList(7, 8, 9);
BEGIN
  ids.delete(1);
  depts.delete(1);
  FORALL i IN 1 .. 2
    INSERT INTO employees_temp(empno, departno) VALUES(ids(i), depts(i));
END;
/

--2 FORALL index IN VALUES OF
DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList(1, 2, 30);
  names nameList := nameList('new1', 'new2', 'new3');
  ids IntList := IntList(1, 2, 3);
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(ids(i), names(i), depts(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList(1, 2, 30);
  names nameList := nameList('new1', 'new2', 'new3');
  ids IntList := IntList(1, 2, 3);
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(11, names(i), depts(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList(1, 2, 30);
  names nameList := nameList('new1', 'new2', 'new3');
  ids IntList := IntList(3, 1, 2);
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(ids(i), names(i), depts(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;


--forall can execute, but no rows are inserted
DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList();
  names nameList := nameList();
  ids IntList := IntList();
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(11, names(i), depts(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;

--forall can execute, but no rows are inserted
DELETE FROM employees_temp;
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER INDEX BY INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList(1);
  names nameList := nameList('A');
  ids IntList;
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(11, names(i), depts(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;

-- the collection should be indexed by integer
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY VARCHAR(20);
  names NameList;
BEGIN
  names('1') := 1;
 FORALL i IN VALUES OF names
   INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

--element type of collection should be integer
DECLARE
  TYPE NameList IS TABLE OF VARCHAR(20) INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 'A';
 FORALL i IN VALUES OF names
   INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

DECLARE
  TYPE NameList IS TABLE OF VARCHAR(20);
  names NameList := NameList('A');
BEGIN
 FORALL i IN VALUES OF names
   INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

--error
DECLARE
  TYPE IntList IS TABLE OF INTEGER;
  ids IntList := IntList(1, 0);
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno) VALUES(ids(i));
END;
/

--error
DECLARE
  TYPE IntList IS TABLE OF INTEGER;
  ids IntList := IntList(1, NULL);
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno) VALUES(ids(i));
END;
/ 

--error: Reference to uninitialized collection
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  TYPE IntList IS TABLE OF INTEGER;
  Type nameList IS TABLE OF VARCHAR(20);
  depts NumList := NumList();
  names nameList := nameList();
  ids IntList;
BEGIN
  FORALL i IN VALUES OF ids
    INSERT INTO employees_temp(empno, ename, departno) VALUES(11, names(i), depts(i));
END;
/


--3 FORALL index IN INDEX OF
DELETE FROM employees_temp;

DECLARE
  TYPE IntList IS TABLE OF INTEGER;
  ids IntList := IntList(5000, 0, 150, 4000, NULL);
BEGIN
  -- Delete invalid orders:
  FOR i IN 1..ids.LAST LOOP
    IF ids(i) IS NULL OR ids(i) = 0 THEN
	   DBMS_OUTPUT.PUT_LINE ('i == ' || i);
      ids.delete(i);
    END IF;
  END LOOP;

  FOR i IN 1..ids.LAST LOOP
    IF ids.EXISTS(i) THEN
      DBMS_OUTPUT.PUT_LINE (
        'Customer #' || i || ', ' || ids(i));
    END IF;
  END LOOP;

  FORALL i IN INDICES OF ids
    INSERT INTO employees_temp(empno) VALUES(ids(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE IntList IS TABLE OF INTEGER;
  ids IntList := IntList(5000, 0, 150, 4000, NULL);
BEGIN
 FOR i IN 1..ids.LAST LOOP
   IF ids.EXISTS(i) THEN
     DBMS_OUTPUT.PUT_LINE (
       'Customer #' || i || ', ' || ids(i));
   END IF;
 END LOOP;

  FORALL i IN INDICES OF ids
    INSERT INTO employees_temp(empno) VALUES(ids(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF VARCHAR(20) INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 'ABC';
  FORALL i IN INDICES OF names
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

--INDICES OF names between 2 and 3
DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between 2 and 3
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between 2 and 4
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;


DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between 5 and 4
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/ 
SELECT * FROM employees_temp ORDER BY empno;


DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between -5 and 4
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/ 
SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between -9 and -4
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/
SELECT * FROM employees_temp ORDER BY empno;

DELETE FROM employees_temp;
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY INTEGER;
  names NameList;
BEGIN
  names(1) := 1;
  names(2) := 2;
  names(3) := 3;
  FORALL i IN INDICES OF names between -9 and 0
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/ 
SELECT * FROM employees_temp ORDER BY empno;

--EXCEPTION
DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY VARCHAR(20);
  names NameList;
BEGIN
  names(1) := 1;
  FORALL i IN INDICES OF names
    INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/

DECLARE
  TYPE NameList IS TABLE OF INTEGER INDEX BY VARCHAR(20);
  names NameList;
BEGIN
  names('1') := 1;
  FORALL i IN INDICES OF names
   INSERT INTO employees_temp(empno, ename) VALUES(1, names(i));
END;
/


--4 SQL%BULK_ROWCOUNT
DELETE FROM employees_temp;
INSERT INTO employees_temp VALUES(1, 'kim', 1);
INSERT INTO employees_temp VALUES(2, 'Joe', 2);
INSERT INTO employees_temp VALUES(3, 'Jone', 10);
INSERT INTO employees_temp VALUES(4, 'Lily', 10);
INSERT INTO employees_temp VALUES(5, 'Leo', 20);
INSERT INTO employees_temp VALUES(6, 'Emily', 30);

SET SERVEROUTPUT ON

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  depts NumList := NumList(10, 20, 30);
BEGIN
  FORALL j IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp WHERE departno = depts(j);

  FOR i IN depts.FIRST..depts.LAST LOOP
    DBMS_OUTPUT.PUT_LINE (
      'Statement #' || i || ' deleted ' ||
      SQL%BULK_ROWCOUNT(i) || ' rows.'
    );
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('Total rows deleted: ' || SQL%ROWCOUNT);
END;
/

--SQL%BULK_ROWCOUNT exception
DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  depts NumList := NumList(10, 20, 30);
BEGIN
  FORALL j IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp WHERE departno = depts(j);

  DBMS_OUTPUT.PUT_LINE('Total rows deleted: ' ||  SQL%BULK_ROWCOUNT);
END;
/

DECLARE
BEGIN
   DBMS_OUTPUT.PUT_LINE('Total rows : ' ||  SQL%BULK_ROWCOUNT(1));
END;
/

DELETE FROM employees_temp;
INSERT INTO employees_temp VALUES(1, 'kim', 1);
INSERT INTO employees_temp VALUES(2, 'Joe', 2);
INSERT INTO employees_temp VALUES(3, 'Jone', 10);
INSERT INTO employees_temp VALUES(4, 'Lily', 10);
INSERT INTO employees_temp VALUES(5, 'Leo', 20);
INSERT INTO employees_temp VALUES(6, 'Emily', 30);

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  depts  NumList := NumList(10, 20, 30);

  TYPE enum_t IS TABLE OF employee.empno%TYPE;
  e_ids  enum_t;

  TYPE dept_t IS TABLE OF employee.departno%TYPE;
  d_ids  dept_t;
  n int;
BEGIN
  FORALL j IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(j)
    RETURNING empno, departno
    BULK COLLECT INTO e_ids, d_ids;

  n := SQL%BULK_ROWCOUNT(1) + depts(1) + 1 + depts(2);
  DBMS_OUTPUT.PUT_LINE('n = ' || n); --33

  n := SQL%BULK_ROWCOUNT(1) + SQL%BULK_ROWCOUNT(2); --3
  DBMS_OUTPUT.PUT_LINE('n = ' || n);

  n := SQL%BULK_ROWCOUNT(1) + depts(1) + 1 + depts(2) + SQL%BULK_ROWCOUNT(2) + 2 + depts(3) + SQL%BULK_ROWCOUNT(3); --67
  DBMS_OUTPUT.PUT_LINE('n = ' || n);

  n := depts(1) + SQL%BULK_ROWCOUNT(1) + 2 + SQL%BULK_ROWCOUNT(2) + depts(2);--35
  DBMS_OUTPUT.PUT_LINE('n = ' || n);

  n := depts(1) + SQL%BULK_ROWCOUNT(1) + 2 + depts(2) + SQL%BULK_ROWCOUNT(2) + 2 + depts(3); --67
  DBMS_OUTPUT.PUT_LINE('n = ' || n);
END;
/


DELETE FROM employees_temp;
INSERT INTO employees_temp VALUES(1, 'kim', 1);
INSERT INTO employees_temp VALUES(2, 'Joe', 2);
INSERT INTO employees_temp VALUES(3, 'Jone', 10);
INSERT INTO employees_temp VALUES(4, 'Lily', 10);
INSERT INTO employees_temp VALUES(5, 'Leo', 20);
INSERT INTO employees_temp VALUES(6, 'Emily', 30);

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  depts  NumList := NumList(10, 20, 30);

  TYPE enum_t IS TABLE OF employee.empno%TYPE;
  e_ids  enum_t;

  TYPE dept_t IS TABLE OF employee.departno%TYPE;
  d_ids  dept_t;
  n int;
BEGIN
  FORALL j IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(j)
    RETURNING empno, departno
    BULK COLLECT INTO e_ids, d_ids;

   DBMS_OUTPUT.PUT_LINE('n = ' || SQL%BULK_ROWCOUNT(1) ||   ',' || depts(1) || ','  || depts(2)); --n = 2,10,20

   DBMS_OUTPUT.PUT_LINE('n = '|| SQL%BULK_ROWCOUNT(1) ||  ',' || SQL%BULK_ROWCOUNT(2)); --n = 2,1

   DBMS_OUTPUT.PUT_LINE('n = '|| SQL%BULK_ROWCOUNT(1) || ',' || depts(1) || ',' ||  depts(2) || ',' || SQL%BULK_ROWCOUNT(2) ||  ',' ||depts(3)); --n = 2,10,20,1,30

   DBMS_OUTPUT.PUT_LINE('n = '|| depts(1) || ',' || SQL%BULK_ROWCOUNT(1) || ',' ||  SQL%BULK_ROWCOUNT(2) || ','|| depts(2) || ',' ||  SQL%BULK_ROWCOUNT(2) || ',' ||depts(3));--n = 10,2,1,20,1,30

   DBMS_OUTPUT.PUT_LINE('n = '|| depts(1) || ',' || SQL%BULK_ROWCOUNT(1) || ',' ||  depts(2) || ','|| SQL%BULK_ROWCOUNT(2) ||',' || depts(2)); --n = 10,2,20,1,20
END;
/


--5 BULK COLLECT INTO in FORALL Statement
DELETE FROM employees_temp;
INSERT INTO employees_temp VALUES(1, 'kim', 1);
INSERT INTO employees_temp VALUES(2, 'Joe', 2);
INSERT INTO employees_temp VALUES(3, 'Jone', 10);
INSERT INTO employees_temp VALUES(4, 'Lily', 10);
INSERT INTO employees_temp VALUES(5, 'Leo', 20);
INSERT INTO employees_temp VALUES(6, 'Emily', 30);

DECLARE
  TYPE NumList IS TABLE OF NUMBER;
  depts  NumList := NumList(10, 20, 30);

  TYPE enum_t IS TABLE OF employees_temp.empno%TYPE;
  e_ids  enum_t;

  TYPE dept_t IS TABLE OF employees_temp.departno%TYPE;
  d_ids  dept_t;
BEGIN
  FORALL j IN depts.FIRST .. depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(j)
    RETURNING empno, departno
    BULK COLLECT INTO e_ids, d_ids;

  FOR i IN e_ids.FIRST .. e_ids.LAST
  LOOP
    raise notice 'employees_temp #%  from dept #%', e_ids(i), d_ids(i);
  END LOOP;

  FOR i IN depts.FIRST..depts.LAST LOOP
    DBMS_OUTPUT.PUT_LINE (
      'Statement #' || i || ' deleted ' ||
      SQL%BULK_ROWCOUNT(i) || ' rows.'
    );
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('Total rows deleted: ' || SQL%ROWCOUNT);
END;
/

SELECT * FROM employees_temp ORDER BY empno;

--6 dynamic sql in FORALL
CREATE TABLE employees_test (
    emp_id     NUMBER,
    emp_name   VARCHAR2(50),
    salary     NUMBER,
    dept_id    NUMBER
);

INSERT INTO employees_test VALUES (1, 'Alice', 5000, 10);
INSERT INTO employees_test VALUES (2, 'Bob',   6000, 20);
INSERT INTO employees_test VALUES (3, 'Carol', 5500, 10);
INSERT INTO employees_test VALUES (4, 'David', 7000, 30);
INSERT INTO employees_test VALUES (5, 'Eve',   6500, 20);


DECLARE
  TYPE t_id_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_raise_tab IS TABLE OF NUMBER;

  v_ids   t_id_tab   := t_id_tab(1, 3, 5);
  v_raise t_raise_tab := t_raise_tab(500, 300, 400);
  TYPE t_out_id_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_out_old_sal  IS TABLE OF employees_test.salary%TYPE;
  TYPE t_out_new_sal  IS TABLE OF employees_test.salary%TYPE;

  v_out_ids     t_out_id_tab;
  v_out_old_sal t_out_old_sal;
  v_out_new_sal t_out_new_sal;

  v_sql VARCHAR2(500);
BEGIN
  v_out_ids     := t_out_id_tab();
  v_out_old_sal := t_out_old_sal();
  v_out_new_sal := t_out_new_sal();

  v_sql := 'UPDATE employees_test SET salary = salary + :1 ' ||
           'WHERE emp_id = :2 RETURNING emp_id, salary - :1, salary INTO :3, :4, :5';

  FORALL i IN 1 .. v_ids.COUNT
      EXECUTE IMMEDIATE v_sql
      USING v_raise(i), v_ids(i), v_raise(i)
      RETURNING BULK COLLECT INTO v_out_ids, v_out_old_sal, v_out_new_sal;

  FOR j IN 1 .. v_out_ids.COUNT LOOP
      DBMS_OUTPUT.PUT_LINE('v_out_ids(' || j || ')=' || v_out_ids(j) || 
                           ', v_out_old_sal(' || j || ')=' || v_out_old_sal(j) || 
                           ', v_out_new_sal(' || j || ')=' || v_out_new_sal(j));
  END LOOP;
END;
/

SELECT * FROM employees_test ORDER BY emp_id;


DELETE FROM employees_test;
DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;
  TYPE t_id_tab  IS TABLE OF NUMBER;

  v_ids	t_id_tab := t_id_tab(1, 2, 3);
  ds	t_id_tab := t_id_tab(1, 2, 3);
  v_names  t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal    t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept   t_dept_tab := t_dept_tab(10, 20, 10);

  TYPE t_id_out_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_name_out_tab IS TABLE OF employees_test.emp_name%TYPE;

  v_out_ids   t_id_out_tab;
  v_out_names t_name_out_tab;

  v_sql VARCHAR2(500);
BEGIN
  v_out_ids   := t_id_out_tab();
  v_out_names := t_name_out_tab();
  v_sql := 'INSERT INTO employees_test(emp_id, emp_name, salary, dept_id) ' ||
           'VALUES (:0, :1, :2, :3) RETURNING emp_id,  emp_name INTO :4, :5';

  FORALL i IN 1 .. v_names.COUNT
      EXECUTE IMMEDIATE v_sql
      USING v_ids(i), v_names(i), v_sal(i), v_dept(i)
      RETURNING BULK COLLECT INTO v_out_ids, v_out_names;
  
  DBMS_OUTPUT.PUT_LINE('' || v_out_ids.COUNT);
  FOR j IN 1 .. v_out_ids.COUNT LOOP
      DBMS_OUTPUT.PUT_LINE('ID = ' || v_out_ids(j) || ', Name =' || v_out_names(j));
  END LOOP;
END;
/

SELECT * FROM employees_test ORDER BY emp_id;


DELETE FROM employees_test;

DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;
  TYPE t_id_tab  IS TABLE OF NUMBER;
  v_ids t_id_tab := t_id_tab(1, 2, 3);
  ds t_id_tab := t_id_tab(1, 2, 3);
  v_names  t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept t_dept_tab := t_dept_tab(10, 20, 10);

  v_sql VARCHAR2(500);
BEGIN
  v_sql := 'INSERT INTO employees_test(emp_id, emp_name, salary, dept_id) ' ||
           'VALUES (:0, :1, :2, :3)';

  FORALL i IN 1 .. v_names.COUNT
      EXECUTE IMMEDIATE v_sql
      USING v_ids(i), v_names(i), v_sal(i), v_dept(i);
END;
/

SELECT * FROM employees_test ORDER BY emp_id;


DELETE FROM employees_test;

DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;
  TYPE t_id_tab  IS TABLE OF NUMBER;
  v_ids t_id_tab := t_id_tab(1, 2, 3);
  ds t_id_tab := t_id_tab(1, 2, 3);
  v_names  t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept t_dept_tab := t_dept_tab(10, 20, 10);

  v_sql VARCHAR2(500);
BEGIN
  v_sql := 'INSERT INTO employees_test(emp_id, emp_name, salary, dept_id) ' ||
           'VALUES (:0, :1, :2, :3)';

  FORALL i IN 1 .. v_names.COUNT - 1
      EXECUTE IMMEDIATE v_sql
      USING v_ids(i + 1), v_names(i + 1), v_sal(i + 1), v_dept(i + 1);
END;
/

SELECT * FROM employees_test ORDER BY emp_id;

DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;
  TYPE t_id_tab  IS TABLE OF NUMBER;

  v_ids t_id_tab := t_id_tab(1, 2, 3);
  v_names t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept t_dept_tab := t_dept_tab(10, 20, 10);

  TYPE t_id_out_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_name_out_tab IS TABLE OF employees_test.emp_name%TYPE;

  v_out_ids  INT;
  v_out_names t_name_out_tab;

  v_sql VARCHAR2(500);
BEGIN
  v_out_names := t_name_out_tab();
  v_sql := 'INSERT INTO employees_test(emp_id, emp_name, salary, dept_id) ' ||
           'VALUES (:0, :1, :2, :3) RETURNING emp_id INTO :4';
  
  FORALL i IN 1 .. v_names.COUNT
      EXECUTE IMMEDIATE v_sql
      USING v_ids(i), v_names(i), v_sal(i), v_dept(i)
      RETURNING INTO v_out_ids;
END;
/


DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;

  v_names t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept t_dept_tab := t_dept_tab(10, 20, 10);

  TYPE t_id_out_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_name_out_tab IS TABLE OF employees_test.emp_name%TYPE;

  v_out_ids  t_id_out_tab;
  v_out_names t_name_out_tab;

  v_sql VARCHAR2(500);
BEGIN
    v_out_ids   := t_id_out_tab();
    v_out_names := t_name_out_tab();
    v_sql := 'INSERT INTO employees_test( emp_name, salary, dept_id) ' ||
             'VALUES ( :1, :2, :3) ';
 
    FORALL i IN 1 .. v_names.COUNT
        EXECUTE IMMEDIATE v_sql
        USING 1, 2, 3;
END;
/

DECLARE
  TYPE t_name_tab IS TABLE OF VARCHAR2(50);
  TYPE t_sal_tab  IS TABLE OF NUMBER;
  TYPE t_dept_tab IS TABLE OF NUMBER;

  v_names t_name_tab := t_name_tab('Frank', 'Grace', 'Henry');
  v_sal t_sal_tab  := t_sal_tab(4800, 5200, 6100);
  v_dept t_dept_tab := t_dept_tab(10, 20, 10);

  TYPE t_id_out_tab   IS TABLE OF employees_test.emp_id%TYPE;
  TYPE t_name_out_tab IS TABLE OF employees_test.emp_name%TYPE;

  v_out_ids t_id_out_tab;
  v_out_names t_name_out_tab;

  v_sql VARCHAR2(500);
BEGIN
  v_out_ids   := t_id_out_tab();
  v_out_names := t_name_out_tab();
  v_sql := 'INSERT INTO employees_test( emp_name, salary, dept_id) ' ||
           'VALUES ( :1, :2, :3) ';

  FORALL i IN 1 .. v_names.COUNT
      EXECUTE IMMEDIATE v_sql
      USING v_names(i), v_sal(i), i;
END;
/

DROP TABLE employees_test;

--7 EXCEPTION
CREATE TABLE emp_temp (
  deptno NUMBER(2),
  job VARCHAR2(18)
);

CREATE OR REPLACE PROCEDURE p AUTHID DEFINER AS
  TYPE NumList IS TABLE OF NUMBER;
  depts        NumList := NumList(10, 20, 30);
 
  error_message  VARCHAR2(100);
  bad_stmt_no    INTEGER;
  bad_deptno     emp_temp.deptno%TYPE;
  bad_job        emp_temp.job%TYPE;
 
  dml_errors  EXCEPTION;
  PRAGMA EXCEPTION_INIT(dml_errors, -24381);
  PRAGMA AUTONOMOUS_TRANSACTION;
BEGIN
  INSERT INTO emp_temp (deptno, job) VALUES (10, 'Clerk');
  INSERT INTO emp_temp (deptno, job) VALUES (20, 'Bookkeeper');
  INSERT INTO emp_temp (deptno, job) VALUES (30, 'Analyst');
  COMMIT;

  -- Append 9-character string to each job:
  FORALL j IN depts.FIRST..depts.LAST
    UPDATE emp_temp SET job = job || ' (Senior)'
    WHERE deptno = depts(j); 
END;
/

BEGIN
  p;
END;
/

SELECT * FROM emp_temp;

DROP TABLE emp_temp;

--test INSERT ALL/FIRST
CREATE TABLE orders
   (order_id       NUMBER(12)   NOT NULL,
    customer_id    NUMBER(6)    NOT NULL,
    order_total    NUMBER(8,2),
    sales_rep_id   NUMBER(6)
   );

CREATE TABLE small_orders AS SELECT * FROM orders;
CREATE TABLE medium_orders AS SELECT * FROM orders;
CREATE TABLE large_orders AS SELECT * FROM orders;

INSERT INTO orders VALUES(1,1,99999,1);
INSERT INTO orders VALUES(2,2,88888,2);
INSERT INTO orders VALUES(3,3,188888,3);
INSERT INTO orders VALUES(4,4,198888,4);
INSERT INTO orders VALUES(5,5,210000, 5);

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 3);
BEGIN
  FORALL i IN depts.FIRST..depts.LAST
     INSERT ALL
       INTO small_orders(order_id,customer_id,order_total, sales_rep_id)
         VALUES(order_id, customer_id + 1, order_total, sales_rep_id + 1)
       INTO medium_orders VALUES(order_id, customer_id + depts(i), order_total, sales_rep_id + 2)
       INTO large_orders
     SELECT * FROM orders;

END;
/
SELECT *FROM small_orders ORDER BY order_id, customer_id;
SELECT *FROM medium_orders ORDER BY order_id, customer_id;
SELECT *FROM large_orders ORDER BY order_id, customer_id;


DELETE FROM small_orders;
DELETE FROM medium_orders;
DELETE FROM large_orders;

DECLARE
  TYPE NumList IS VARRAY(20) OF INT;
  ids NumList := NumList(1, 2, 3);
BEGIN
  FORALL i IN ids.FIRST .. ids.LAST
    INSERT FIRST
      WHEN order_total < 100000 THEN
         INTO small_orders(order_id,customer_id,order_total, sales_rep_id)
        VALUES(order_id, customer_id + ids(i), order_total, sales_rep_id + 1)
      WHEN order_total > 100000 AND order_total <= 200000 THEN
        INTO medium_orders
      WHEN order_total > 200000 THEN
        INTO large_orders
     SELECT * FROM orders;

END;
/

SELECT *FROM small_orders ORDER BY order_id, customer_id;
SELECT *FROM medium_orders ORDER BY order_id, customer_id;
SELECT *FROM large_orders ORDER BY order_id, customer_id;

DROP TABLE orders;
DROP TABLE small_orders;
DROP TABLE medium_orders;
DROP TABLE large_orders;

--test trigger
CREATE TABLE log(s varchar(20));
DELETE FROM employee;

INSERT INTO employee VALUES(1, 'kim', 1);
INSERT INTO employee VALUES(2, 'Joe', 2);
INSERT INTO employee VALUES(3, 'Jone', 10);
INSERT INTO employee VALUES(4, 'Lily', 10);
INSERT INTO employee VALUES(5, 'Leo', 20);
INSERT INTO employee VALUES(6, 'Emily', 30);
DROP TABLE employees_temp;
CREATE TABLE employees_temp AS SELECT * FROM employee;

CREATE OR REPLACE TRIGGER tri_check
AFTER DELETE ON employees_temp
BEGIN
  INSERT INTO log(s) VALUES ('delete');
END;
/

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 10);
BEGIN
  FORALL i IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(i);
END;
/

SELECT * FROM employees_temp ORDER BY empno;
SELECT *FROM log;

--test FOR EACH ROW trigger
DELETE FROM log;
DELETE FROM employee;

INSERT INTO employee VALUES(1, 'kim', 1);
INSERT INTO employee VALUES(2, 'Joe', 2);
INSERT INTO employee VALUES(3, 'Jone', 10);
INSERT INTO employee VALUES(4, 'Lily', 10);
INSERT INTO employee VALUES(5, 'Leo', 20);
INSERT INTO employee VALUES(6, 'Emily', 30);
DROP TABLE employees_temp;
CREATE TABLE employees_temp AS SELECT * FROM employee;

CREATE OR REPLACE TRIGGER tri_salary_log
AFTER DELETE  ON employees_temp
FOR EACH ROW
BEGIN
  INSERT INTO log(s) VALUES ('delete');
END;
/

DECLARE
  TYPE NumList IS VARRAY(20) OF NUMBER;
  depts NumList := NumList(1, 2, 10);
BEGIN
  FORALL i IN depts.FIRST..depts.LAST
    DELETE FROM employees_temp
    WHERE departno = depts(i);
END;
/

SELECT *FROM log;
SELECT * FROM employees_temp ORDER BY empno;

SET SERVEROUTPUT OFF

DROP TABLE t1;
DROP TABLE t2;
DROP TABLE employees_temp;
DROP TABLE employee;