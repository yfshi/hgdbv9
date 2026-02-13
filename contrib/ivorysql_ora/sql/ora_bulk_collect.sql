--1 SELECT BULK COLLECT
--2 INSERT/UPDATE/DELETE RETURNING BULK COLLECT
--3 FETCH BULK COLLECT
--4 EXECUTE IMMEDIATE BULK COLLECT

--create tables
create table employee(empno int, ename varchar(20), hiredate date, departno int);
insert into employee values(1, 'kim', to_date('2017-10-18','yyyy-mm-dd'), 1);
insert into employee values(2, 'Joe', to_date('2017-9-18','yyyy-mm-dd'), 2);
insert into employee values(3, 'Jone', to_date('2017-8-1','yyyy-mm-dd'), 10);
insert into employee values(4, 'Lily', to_date('2017-8-2','yyyy-mm-dd'), 10);
insert into employee values(5, 'Leo', to_date('2017-8-3','yyyy-mm-dd'), 20);
insert into employee values(6, 'Emily', to_date('2017-8-4','yyyy-mm-dd'), 30);

create table tmp as select * from employee;
create table tmp2 as select empno, ename from employee;

--1 SELECT BULK COLLECT
DECLARE
  TYPE NumTab IS TABLE OF employee.empno%TYPE;
  TYPE NameTab IS TABLE OF employee.ename%TYPE;

  enums NumTab;
  names NameTab;

  PROCEDURE print () IS
    n int;
  BEGIN
    n := enums.COUNT;
    IF n = 0 THEN
      raise notice 'Collections are empty.';
    ELSE
      raise notice 'First % employee:', n;
 
      FOR i IN 1 .. n LOOP
        raise notice 'Employee # % : %', enums(i), names(i);
      END LOOP;
    END IF;
  END;
BEGIN
  SELECT empno, ename
  BULK COLLECT INTO enums, names
  FROM employee
  ORDER BY empno;

  print();
END;
/

--multidimensional collection
DECLARE
  TYPE type1 IS TABLE OF VARCHAR2(20) INDEX BY INT; 
  TYPE type2 IS TABLE OF type1 INDEX BY INT;
  TYPE type3 IS TABLE OF type2 INDEX BY INT;
  a type1; 
  b type2;
  c type3;
BEGIN
  SELECT ename BULK COLLECT INTO c(1)(1) FROM tmp;

  FOR i IN 1 .. c(1)(1).count LOOP
     raise notice '%', c(1)(1)(i);
  END LOOP;
END;
/

--after bulk collect statement, call collection EXTEND method
DECLARE
  TYPE NumTab IS VARRAY(50) OF employee.empno%TYPE;
  TYPE NameTab IS VARRAY(50) OF employee.ename%TYPE;
 
  enums NumTab;
  names NameTab;
 
  PROCEDURE print () IS
    n int;
  BEGIN
    n := enums.COUNT;
    IF n = 0 THEN
      raise notice 'Collections are empty.';
    ELSE
      raise notice 'First % employee:', n;
 
      FOR i IN 1 .. n LOOP
        raise notice 'Employee # % : %', enums(i), names(i);
      END LOOP;
    END IF;
  END;
 
BEGIN
  SELECT empno, ename
  BULK COLLECT INTO enums, names
  FROM employee
  ORDER BY empno;

  enums.extend(2, 1);
  names.extend(2, 1);
  print();
END;
/

--If the query returns no result, bulk collect will not report an error, but will delete the initial values of the collection
DECLARE
  TYPE NumTab IS TABLE OF employee.empno%TYPE;
  TYPE NameTab IS TABLE OF employee.ename%TYPE;
 
  enums NumTab := NumTab(1, 2);
  names NameTab := NameTab('a', 'b');
 
  PROCEDURE print () IS
    n int;
  BEGIN
    n := enums.COUNT;
    IF n = 0 THEN
      raise notice 'Collections are empty.';
    ELSE
      raise notice 'First % employee:', n;
 
      FOR i IN 1 .. n LOOP
        raise notice ' Employee #%:%', enums(i), names(i);
      END LOOP;
    END IF;
  END;
 
BEGIN
  SELECT empno, ename
  BULK COLLECT INTO enums, names
  FROM employee WHERE departno = 1000
  ORDER BY empno;
 
  print();
END;
/


DECLARE
  TYPE NumTab IS TABLE OF employee.empno%TYPE;
  TYPE NameTab IS TABLE OF employee.ename%TYPE;
 
  enums NumTab;
  names NameTab;
 
  PROCEDURE print () IS
    n int;
  BEGIN
    n := enums.COUNT;
    IF n = 0 THEN
      raise notice 'Collections are empty.';
    ELSE
      raise notice 'First % employee:', n;
 
      FOR i IN 1 .. n LOOP
        raise notice ' Employee #%:%', enums(i), names(i);
      END LOOP;
    END IF;
  END;
 
BEGIN
  SELECT empno, ename
  BULK COLLECT INTO enums, names
  FROM employee WHERE departno = 1000
  ORDER BY empno;
 
  print();
END;
/

--select bulk collect into multidimensional collection
DECLARE
  TYPE type1 IS TABLE OF VARCHAR2(20) INDEX BY INT; 
  TYPE type2 IS TABLE OF type1 INDEX BY INT;
  TYPE type3 IS TABLE OF type2 INDEX BY INT;
  a type1; 
  b type2;
  c type3;
BEGIN
  SELECT ename BULK COLLECT INTO c(1)(1) FROM tmp;

  FOR i IN 1 .. c(1)(1).count LOOP
     raise notice '%', c(1)(1)(i);
  END LOOP;
  c(1)(1)(7) := 'new';
  FOR i IN 1 .. c(1)(1).count LOOP
     raise notice '%', c(1)(1)(i);
  END LOOP;  
END;
/


--the collection and record
DECLARE
  TYPE type1 IS TABLE OF INT; 
  TYPE record1 IS RECORD(id1 type1);
  TYPE type2 IS TABLE OF record1;
  TYPE nest_type1 IS TABLE OF VARCHAR(10);
  TYPE nest_type2 IS TABLE OF nest_type1;
  edno   type1; 
  r1 record1;
  edno2  type2;
  ename1  nest_type1;
  ename2  nest_type2 := nest_type2(ename1);
BEGIN
  r1.id1 := edno;
  edno2 := type2(r1);
  SELECT empno, ename BULK   COLLECT INTO edno2(1).id1, ename2(1) FROM  tmp; 
  FOR i IN edno2(1).id1.FIRST .. edno2(1).id1.LAST
  LOOP
   raise notice '%','Employee #' || edno2(1).id1(i) || ' name is ' || ename2(1)(i);
  END LOOP;
  
  SELECT empno BULK COLLECT INTO r1.id1 FROM tmp;  
  raise notice 'r1.id1 count is %', r1.id1.count;
  FOR i IN r1.id1.FIRST .. r1.id1.LAST
  LOOP
   raise notice '%','Employee #' || r1.id1(i);
  END LOOP;
END;
/

--the collection and record, collection var has initial value
DECLARE
  TYPE type1 IS TABLE OF INT; 
  TYPE record1 IS RECORD(id1 type1);
  TYPE type2 IS TABLE OF record1;
  TYPE nest_type1 IS TABLE OF VARCHAR(10);
  TYPE nest_type2 IS TABLE OF nest_type1;
  edno   type1 := type1(1); 
  r1 record1;
  edno2  type2;
  ename1  nest_type1 := nest_type1('a');
  ename2  nest_type2 := nest_type2(ename1);
BEGIN
  r1.id1 := edno;
  edno2 := type2(r1);
  SELECT empno, ename BULK COLLECT INTO edno2(1).id1, ename2(1)  FROM  tmp; 
  FOR i IN edno2(1).id1.FIRST .. edno2(1).id1.LAST LOOP
    raise notice '%','Employee #' || edno2(1).id1(i) || ' name is ' || ename2(1)(i);
  END LOOP;

  SELECT empno BULK COLLECT INTO r1.id1  FROM  tmp;  
  raise notice 'r1.id1 count is %', r1.id1.count;
  FOR i IN r1.id1.FIRST .. r1.id1.LAST LOOP
    raise notice '%','Employee #' || r1.id1(i);
  END LOOP;
END;
/

--the collection and record, query returns null , delete colelction vars' initial value
DELETE FROM tmp;
DECLARE
  TYPE type1 IS TABLE OF INT; 
  TYPE record1 IS RECORD(id1 type1);
  TYPE type2 IS TABLE OF record1;
  TYPE nest_type1 IS TABLE OF VARCHAR(10);
  TYPE nest_type2 IS TABLE OF nest_type1;
  edno   type1; 
  r1 record1;
  edno2  type2;
  ename1  nest_type1;
  ename2  nest_type2 := nest_type2(ename1);
BEGIN
  r1.id1 := edno;
  edno2 := type2(r1);
  SELECT empno, ename BULK COLLECT INTO edno2(1).id1, ename2(1)  FROM  tmp;
  raise notice 'edno2(1).id1 count is %', edno2(1).id1.count; --no data found
END;
/

--ROWNUM clause to limit the number of rows and the collection size
DELETE FROM tmp;
INSERT INTO tmp SELECT * FROM employee;

DECLARE
  TYPE type1 IS TABLE OF INT;  
  edno   type1;  
BEGIN
  SELECT empno BULK COLLECT INTO edno FROM tmp where rownum < 5;  
  FOR i IN edno.FIRST .. edno.LAST LOOP
    raise notice '%','Employee #' || edno(i);
  END LOOP;
END;
/

--FETCH FIRST clause to limit the number of rows and the collection size
DECLARE
  TYPE type1 IS TABLE OF INT;  
  edno   type1;  
BEGIN
  SELECT empno BULK COLLECT INTO edno FROM tmp FETCH FIRST 3 ROWS ONLY;  
  FOR i IN edno.FIRST .. edno.LAST LOOP
    raise notice '%','Employee #' || edno(i);
  END LOOP;
END;
/

--test in package
CREATE OR REPLACE package test_pkg IS
  FUNCTION test_f(id integer) RETURN integer;
  PROCEDURE test_p(id integer);
end;
/

CREATE OR REPLACE package body test_pkg is
  FUNCTION test_f(id integer) RETURN integer IS
    TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
    TYPE NameList IS TABLE OF tmp.ename%TYPE;
    partno numtab;
    names  NameList;
    var1 integer := 0;
  BEGIN
	SELECT departno, ename BULK COLLECT INTO partno, names FROM tmp;

	FOR i IN 1 .. partno.COUNT LOOP
	   raise notice 'departno: % : %', partno(i), names(i);
	END LOOP;

	RETURN 1;
  END;
  PROCEDURE test_p(id integer) IS
    var1 integer;
  BEGIN
	  var1 := test_f(23);
  END;
END;
/

BEGIN
  test_pkg.test_p(23);
END;
/

CREATE OR REPLACE package body test_pkg is
  FUNCTION test_f(id integer) RETURN integer IS
    TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
    TYPE NameList IS TABLE OF tmp.ename%TYPE;
    TYPE numtab2 IS TABLE OF numtab INDEX BY INT;
    TYPE NameList2 IS TABLE OF NameList INDEX BY INT;
    TYPE numtab3 IS TABLE OF numtab2 INDEX BY INT;
    TYPE NameList3 IS TABLE OF NameList2 INDEX BY INT;
    partno numtab3;
    names  NameList3;
    var1 integer := 0;
  BEGIN
	SELECT departno, ename BULK COLLECT INTO partno(1)(1), names(1)(1) FROM tmp;

	FOR i IN 1 .. partno(1)(1).COUNT LOOP
	   raise notice 'departno Id: % : %', partno(1)(1)(i), names(1)(1)(i);
	END LOOP;

	RETURN 1;
  END;
  PROCEDURE test_p(id integer) IS
    var1 integer;
  BEGIN
	  var1 := test_f(23);
  END;
END;
/

BEGIN
  test_pkg.test_p(23);
END;
/

DROP package test_pkg;


--reference collection variable in a package
CREATE OR REPLACE package test_pkg2 IS
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  nos numtab;
  names  NameList;
end;
/

DECLARE
BEGIN
  SELECT empno, ename BULK COLLECT INTO test_pkg2.nos, test_pkg2.names FROM tmp;

  FOR i IN 1 .. test_pkg2.nos.COUNT LOOP
     raise notice 'Employee Id: % : %', test_pkg2.nos(i), test_pkg2.names(i);
  END LOOP;
END;
/

DECLARE
  nos test_pkg2.numtab;
  names test_pkg2.NameList;
BEGIN
  SELECT departno, ename BULK COLLECT INTO nos, names FROM tmp;

  FOR i IN 1 .. nos.COUNT LOOP
     raise notice 'Employee Id: % : %', nos(i), names(i);
  END LOOP;
END;
/
DROP package test_pkg2;


--select bulk collect into a collection which value datatype is %rowtype
DELETE FROM tmp;
INSERT INTO tmp SELECT * FROM employee;

DECLARE
  TYPE nested_emp_type IS TABLE OF tmp%ROWTYPE;  
  emp_tab   nested_emp_type;  
BEGIN 
  SELECT empno, ename, hiredate, departno  BULK COLLECT INTO emp_tab FROM tmp;

  FOR i IN emp_tab.FIRST .. emp_tab.LAST LOOP  
    raise notice 'Employee Id: %, name: %, hiredate: %, departno: %', emp_tab(i).empno, emp_tab(i).ename, emp_tab(i).hiredate, emp_tab(i).departno;
  END LOOP;	
END;
/

--select bulk collect into a collection which value datatype is cursor
DECLARE
  CURSOR c1 RETURN tmp%ROWTYPE IS SELECT * FROM tmp;
  TYPE nested_emp_type IS TABLE OF c1%ROWTYPE;  
  emp_tab   nested_emp_type;  
BEGIN 
  SELECT empno, ename, hiredate, departno  BULK COLLECT INTO emp_tab FROM tmp;

  FOR i IN emp_tab.FIRST .. emp_tab.LAST LOOP  
    raise notice 'Employee Id: %, name: %, hiredate: %, departno: %', emp_tab(i).empno, emp_tab(i).ename, emp_tab(i).hiredate, emp_tab(i).departno;
  END LOOP;	
END;
/

--select bulk collect into a collection which value datatype is record
DECLARE
  TYPE emp_rec_type IS RECORD            
  (  
     empno      tmp.empno%TYPE,
     ename      tmp.ename%TYPE,
     hiredate   tmp.hiredate%TYPE,
	 departno   tmp.departno%TYPE
  );
  TYPE nested_emp_type IS TABLE OF emp_rec_type;  
  emp_tab   nested_emp_type;  
BEGIN 
  SELECT empno, ename, hiredate, departno  BULK COLLECT INTO emp_tab FROM tmp;

  FOR i IN emp_tab.FIRST .. emp_tab.LAST LOOP  
    raise notice 'Employee Id: %, name: %, hiredate: %, departno: %', emp_tab(i).empno, emp_tab(i).ename, emp_tab(i).hiredate, emp_tab(i).departno;
  END LOOP;	
END;
/

--select bulk collect into a collection which value datatype is subtype
DECLARE
  TYPE emp_rec_type IS RECORD            
  (  
     empno      tmp.empno%TYPE,
     ename      tmp.ename%TYPE,
     hiredate   tmp.hiredate%TYPE,
	 departno   tmp.departno%TYPE
  );
  SUBTYPE my_subtype IS emp_rec_type;
  TYPE nested_emp_type IS TABLE OF emp_rec_type;  
  emp_tab   nested_emp_type;  
BEGIN 
  SELECT empno, ename, hiredate, departno  BULK COLLECT INTO emp_tab FROM tmp;

  FOR i IN emp_tab.FIRST .. emp_tab.LAST LOOP  
    raise notice 'Employee Id: %, name: %, hiredate: %, departno: %', emp_tab(i).empno, emp_tab(i).ename, emp_tab(i).hiredate, emp_tab(i).departno;
  END LOOP;	
END;
/

--record field datatype can do cast with query column datatype
DECLARE
  TYPE myrecord IS RECORD(empno varchar(20), ename varchar(20));
  TYPE NameSet IS TABLE OF myrecord;
  stock_managers  NameSet;  -- nested table of records
BEGIN 
  SELECT empno, ename
    BULK COLLECT INTO stock_managers
    FROM employee
    WHERE departno = 10
    ORDER BY hiredate;

  FOR i IN stock_managers.FIRST .. stock_managers.LAST LOOP  
    raise notice 'Employee Id: %, name: %', stock_managers(i).empno, stock_managers(i).ename;
  END LOOP;	
END;
/


CREATE TABLE tmp4(empno varchar(20), ename varchar(20));

DECLARE
  TYPE NameSet IS TABLE OF tmp4%ROWTYPE;
  stock_managers  NameSet;
BEGIN 
  SELECT empno, ename
    BULK COLLECT INTO stock_managers
    FROM employee
    WHERE departno = 10
    ORDER BY hiredate;

  FOR i IN stock_managers.FIRST .. stock_managers.LAST LOOP  
    raise notice 'Employee Id: %, name: %', stock_managers(i).empno, stock_managers(i).ename;
  END LOOP;	
END;
/

DECLARE
  CURSOR c1 RETURN tmp4%ROWTYPE IS SELECT * FROM tmp4;
  TYPE nested_emp_type IS TABLE OF c1%ROWTYPE;  
  emp_tab   nested_emp_type;  
BEGIN 
  SELECT empno, ename BULK COLLECT INTO emp_tab FROM employee;

  FOR i IN emp_tab.FIRST .. emp_tab.LAST LOOP  
    raise notice 'Employee Id: %, name: %', emp_tab(i).empno, emp_tab(i).ename;
  END LOOP;	
END;
/
DROP TABLE tmp4;


--error
DECLARE
  TYPE nested_emp_type IS TABLE OF tmp%ROWTYPE;  
  emp_tab nested_emp_type;  
BEGIN 
  SELECT empno BULK COLLECT INTO emp_tab FROM tmp;
END; 
/

CREATE  TABLE tmp3(ID INT);
DECLARE
  TYPE nested_emp_type IS TABLE OF tmp3%ROWTYPE;  
  emp_tab nested_emp_type;  
BEGIN 
  SELECT * BULK COLLECT INTO emp_tab FROM employee;
END; 
/
DROP TABLE tmp3;


--exception
DECLARE
  TYPE NumList IS TABLE OF employee.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF employee.ename%TYPE;
  names  NameList;
BEGIN
  DELETE FROM employee
  WHERE departno = 10
  RETURNING empno, ename
  BULK COLLECT INTO enums;
END;
/
--ERROR: not enough values


DECLARE
  TYPE NumList IS TABLE OF employee.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF employee.ename%TYPE;
  names  NameList;
BEGIN
  DELETE FROM employee
  WHERE departno = 10
  RETURNING empno, ename
  BULK COLLECT INTO enums, names, names;
END;
/
--ERROR: too many values


--The table in the DML statement does not exist
DECLARE
  TYPE NumList IS TABLE OF employee.empno%TYPE;
  enums  NumList;
BEGIN
  DELETE FROM employee2
  RETURNING empno
  BULK COLLECT INTO enums;
END;
/

--the number of rows in the result cannot exceed the VARRAY declared maximum size
SELECT COUNT(*) FROM tmp;
DECLARE
  TYPE NumList IS VARRAY(5) OF tmp.empno%TYPE;
  enums  NumList;
BEGIN
  SELECT empno
  BULK COLLECT INTO enums
  FROM tmp
  ORDER BY empno;
END;
/

---the index datatype of associate array must be integer
DECLARE
  TYPE NumTab IS TABLE OF employee.empno%TYPE INDEX BY VARCHAR2;
  enums NumTab;
BEGIN
  SELECT empno
  BULK COLLECT INTO enums
  FROM employee;
END;
/

--datatype cast failed
DECLARE
  TYPE NumTab IS TABLE OF INT INDEX BY INTEGER;
  enums NumTab;
BEGIN
  SELECT ename
  BULK COLLECT INTO enums
  FROM employee;
END;
/

--The number of record fields is different from the number of result sets returned by SELECT
DECLARE
  TYPE myrecord IS RECORD(empno int, ename varchar(20));
  TYPE NameSet IS TABLE OF myrecord;
  stock_managers  NameSet;
BEGIN 
  SELECT empno, ename, hiredate
    BULK COLLECT INTO stock_managers
    FROM employee
    WHERE departno = 10
    ORDER BY hiredate;
END;
/

DECLARE
  TYPE myrecord IS RECORD(empno int, ename varchar(20), hiredate date);
  TYPE NameSet IS TABLE OF myrecord;
  stock_managers  NameSet;  -- nested table of records
BEGIN 
  SELECT ename, ename
    BULK COLLECT INTO stock_managers
    FROM employee
    WHERE departno = 10
    ORDER BY hiredate;
END;
/


DECLARE
  TYPE myrecord IS RECORD(empno int, ename varchar(20));
  TYPE NameSet IS TABLE OF myrecord;
  stock_managers  NameSet;  -- nested table of records
BEGIN 
  SELECT ename, ename
    BULK COLLECT INTO stock_managers
    FROM employee
    WHERE departno = 10
    ORDER BY hiredate;
  FOR i IN stock_managers.FIRST .. stock_managers.LAST LOOP  
    raise notice 'Employee Id: %, name: %', stock_managers(i).empno, stock_managers(i).ename;
  END LOOP;	
END;
/


--2 INSERT/UPDATE/DELETE RETURNING BULK COLLECT
DELETE FROM tmp;
INSERT INTO tmp SELECT * FROM employee;

DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList;
BEGIN
  DELETE FROM tmp RETURNING empno, ename BULK COLLECT INTO enums, names;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

DELETE FROM tmp;
INSERT INTO tmp SELECT * FROM employee;
DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList := NumList(11);
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList := NameList('abc');
BEGIN
  DELETE FROM tmp RETURNING empno, ename BULK COLLECT INTO enums, names;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

DELETE FROM tmp;
DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList := NumList(11);
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList := NameList('abc');
BEGIN
  DELETE FROM tmp RETURNING empno, ename BULK COLLECT INTO enums, names;

  IF enums.count = 0 AND names.count = 0 THEN
    raise notice 'colletions are empty';
  END IF;
END;
/


DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList;
BEGIN
 INSERT INTO tmp VALUES
   (1, 'kim', to_date('2017-10-18','yyyy-mm-dd'), 1),
   (2, 'Joe', to_date('2017-9-18','yyyy-mm-dd'), 2)
   RETURNING empno, ename BULK COLLECT INTO enums, names;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList;
BEGIN
  UPDATE tmp set empno = 0, ename = 'newname' 
   RETURNING empno, ename BULK COLLECT INTO enums, names;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/


--3 FETCH BULK COLLECT
DELETE FROM tmp;
INSERT INTO tmp SELECT * FROM employee;

DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/

--LIMIT clause
DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names LIMIT 3;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/

DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names LIMIT 2.6;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/

DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names LIMIT 2.1;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/

DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names LIMIT 2.1 + 3.4;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/

DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
  limit_value numeric := 2.7;
BEGIN
  OPEN c1;

  FETCH c1 BULK COLLECT INTO emptno, names LIMIT limit_value;
   raise notice '------- Results from One Bulk Fetch --------';
  FOR i IN 1 .. emptno.COUNT LOOP
     raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
  END LOOP;
  CLOSE c1;
END;
/


--test in package
CREATE OR REPLACE package test_pkg IS
  FUNCTION test_f(id integer) RETURN integer;
  PROCEDURE test_p(id integer);
end;
/

CREATE OR REPLACE package body test_pkg is
  FUNCTION test_f(id integer) RETURN integer IS
    TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
    TYPE NameList IS TABLE OF tmp.ename%TYPE;
    CURSOR c1 IS SELECT departno, ename FROM tmp;
    emptno numtab;
    names  NameList;
    var1 integer := 4;
  BEGIN
	OPEN c1;

	FETCH c1 BULK COLLECT INTO emptno,names LIMIT var1;
	 raise notice '------- Results from One Bulk Fetch --------';
	FOR i IN 1 .. emptno.COUNT LOOP
	   raise notice '%', 'Employee Id: ' || emptno(i) ||': ' || names(i);
	END LOOP;

	CLOSE c1;
	RETURN 1;
  END;
  PROCEDURE test_p(id integer) IS
    var1 integer;
    FUNCTION infun(id integer) RETURN integer IS
    BEGIN
  	  raise info '%',id;
      RETURN 1;
    END;
  BEGIN
	  var1 := test_f(23);
  exception
	  WHEN others then
	    raise info '%', sqlerrm;
	  var1 := infun(23);
  END;
END;
/

DECLARE
BEGIN
  test_pkg.test_p(23);
END;
/

DECLARE
BEGIN
  test_pkg.test_p(23);
END;
/

DROP package test_pkg;


--REFCURSOR
DECLARE  
  c1 REFCURSOR;
  TYPE NumList IS TABLE OF NUMBER INDEX BY INT;
  enums  NumList;
BEGIN
  OPEN c1 FOR
    SELECT empno FROM tmp;  

  FETCH c1 BULK COLLECT INTO enums;
  FOR i IN 1 .. enums.COUNT LOOP
    raise notice '%', 'Employee Id: ' || enums(i);
  END LOOP;

  CLOSE c1;
END;
/

DECLARE  
  cursor c1 return tmp2%rowtype is select * from tmp2;
  TYPE NumList IS TABLE OF tmp2.empno%TYPE;
  TYPE NameList IS TABLE OF tmp2.ename%TYPE;
  enums  NumList;
  names  NameList;
BEGIN
  OPEN c1;  
  LOOP
    FETCH c1 BULK COLLECT INTO enums, names;
    FOR i IN 1 .. enums.COUNT LOOP
       raise notice '%', 'Employee Id: ' || enums(i) ||': ' || names(i);
    END LOOP;
    EXIT WHEN c1%NOTFOUND;
  END LOOP;
  CLOSE c1;
END;
/

DECLARE
  TYPE c_type1 is ref cursor return tmp2%rowtype;
  TYPE NumList IS TABLE OF tmp2.empno%TYPE;
  TYPE NameList IS TABLE OF tmp2.ename%TYPE;
  enums  NumList;
  names  NameList;
  c1 c_type1;
BEGIN
  OPEN c1 FOR SELECT * FROM tmp2;
  LOOP
    FETCH c1 BULK COLLECT INTO enums, names;
    FOR i IN 1 .. enums.COUNT LOOP
       raise notice '%', 'Employee Id: ' || enums(i) || ': ' || names(i);
    END LOOP;
    EXIT WHEN c1%NOTFOUND;
  END LOOP;
  CLOSE c1;
END;
/


--SYS_REFCURSOR
DECLARE  
  c1 SYS_REFCURSOR;
  TYPE NumList IS TABLE OF NUMBER INDEX BY INT;
  enums  NumList;
BEGIN
  OPEN c1 FOR
    SELECT empno FROM tmp;  

  LOOP
    FETCH c1 BULK COLLECT INTO enums;
    FOR i IN 1 .. enums.COUNT LOOP
       raise notice '%', 'Employee Id: ' || enums(i);
    END LOOP;
    EXIT WHEN c1%NOTFOUND;
  END LOOP;
  CLOSE c1;
END;
/

--error
DECLARE
  TYPE numtab IS TABLE OF NUMBER INDEX BY INT;
  TYPE NameList IS TABLE OF INT;
  CURSOR c1 IS SELECT empno, ename FROM tmp;
  emptno numtab;
  names  NameList;
  i int := 3;
BEGIN
  OPEN c1;
    FETCH c1 BULK COLLECT INTO emptno, names LIMIT 3;
  CLOSE c1;
END;
/


--4 EXECUTE IMMEDIATE BULK COLLECT
--execute 'select xxx from xxx using'  bulk collect into using
DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList;
  i int := 2;
BEGIN
  EXECUTE 'SELECT empno, ename FROM tmp where empno > :x' 
    BULK COLLECT INTO enums, names using i;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

--execute 'xxx'  returning bulk collect into
DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList; 
  sql_stmt  VARCHAR(200);
BEGIN
  sql_stmt := 'UPDATE tmp SET empno = empno + 10  RETURNING empno,ename INTO :1,:2'; 
  EXECUTE IMMEDIATE sql_stmt RETURNING BULK COLLECT INTO enums, names;
 
  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

--execute 'xxx' using xx returning bulk collect into
DECLARE
  TYPE NumList IS TABLE OF tmp.empno%TYPE;
  enums  NumList;
  TYPE NameList IS TABLE OF tmp.ename%TYPE;
  names  NameList; 
  sql_stmt  VARCHAR(200);
  i int := 12;
BEGIN
  sql_stmt := 'UPDATE tmp SET empno = empno + 10  where empno > :x RETURNING empno,ename INTO :1,:2'; 
  EXECUTE IMMEDIATE sql_stmt using i RETURNING BULK COLLECT INTO enums, names;

  FOR i IN enums.FIRST .. enums.LAST
  LOOP
    raise notice '%', 'Employee #' || enums(i) || ': ' || names(i);
  END LOOP;
END;
/

SELECT empno, ename FROM tmp;
DECLARE
   TYPE emp_rec_type IS RECORD            
    (  
       empno      tmp.empno%TYPE,
       ename      tmp.ename%TYPE 
    );  
    TYPE type1 IS TABLE OF emp_rec_type;  
	TYPE type2 IS TABLE OF type1;
	r1 emp_rec_type;
    nt1  type1; 
	nt2 type2;
BEGIN
    r1.empno := 1;
	r1.ename := 'Joe';
    nt1 := type1(r1);
	nt2 := type2(nt1);

	EXECUTE IMMEDIATE 'SELECT empno, ename FROM tmp' BULK COLLECT INTO nt2(1);
    FOR i IN nt2(1).FIRST .. nt2(1).LAST LOOP  
       raise notice '%', nt2(1)(i).empno || ' ' || nt2(1)(i).ename;
    END LOOP;		
END;
/

DECLARE
   TYPE emp_rec_type IS RECORD
    (
       empno      tmp.empno%TYPE,
       ename      tmp.ename%TYPE
    );
    TYPE type1 IS TABLE OF emp_rec_type;
   a type1;
BEGIN
EXECUTE IMMEDIATE 'update tmp set empno = 1 returning empno, ename  into :1 ' BULK COLLECT INTO a;
    FOR i IN a.FIRST .. a.LAST LOOP
       raise notice '%', a(i).empno || ' ' || a(i).ename;
    END LOOP;
end;
/

DECLARE
   TYPE emp_rec_type IS RECORD
    (
       empno      tmp.empno%TYPE,
       ename      tmp.ename%TYPE
    );
    TYPE type1 IS TABLE OF emp_rec_type;
    a type1;
BEGIN
EXECUTE IMMEDIATE 'update tmp set empno = 2 returning empno, ename  into :1 '  returning BULK COLLECT INTO a;
    FOR i IN a.FIRST .. a.LAST LOOP
       raise notice '%', a(i).empno || ' ' || a(i).ename;
    END LOOP;
end;
/

--clean
DROP TABLE tmp;
DROP TABLE tmp2;
DROP TABLE employee;