--1 Associative Array
--2 Nested Tables
--3 Varrays
--4 mult-dim collection
--5 the collection in package
--6 the element of the collection is record
--7 the element of the collection is %rowtype
--8 the element of the collection is cursor
--9 SELECT INTO collection
--10 FETCH INTO
--11 RETURNING INTO collection
--12 EXECUTE USING collection
--13 collection appears in DML
--13 more test cases

CREATE TABLE t1(i int, j int);
INSERT INTO t1 VALUES(1, 11);
INSERT INTO t1 VALUES(2, 22);
CREATE TABLE t2(i int, j date);
INSERT INTO t2 VALUES(1, date '2017-04-21');

--1 Associative Array
--Declaring Associative Array
--The data type of index can be either a string type (VARCHAR2, VARCHAR, TEXT) or int type
DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY VARCHAR(10);
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY INT;
BEGIN
  NULL;
END;
/

--Implementation restriction: unsupported table index type
DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY CHAR;
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY NUMBER;
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF VARCHAR2(15) NOT NULL INDEX BY INT;
  a population;
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF VARCHAR2(15) INDEX BY INT;
  a population NOT NULL;
BEGIN
  NULL;
END;
/

--Declaring Associative Array in a package
CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS TABLE OF VARCHAR2(20) INDEX BY INTEGER;
  FUNCTION Init_My_AA RETURN INT;
END My_Types;
/
CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN INT IS
  BEGIN
    RETURN 1;
  END Init_My_AA;
END My_Types;
/
DROP PACKAGE My_Types;


CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS TABLE OF VARCHAR2(20) INDEX BY INTEGER;
  FUNCTION Init_My_AA RETURN My_AA;
END My_Types;
/

CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN My_AA IS
    Ret My_AA;
  BEGIN
    RETURN Ret;
  END Init_My_AA;
END;
/
DROP PACKAGE My_Types;

--Declaring Associative Array, declare a variable
DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
  city_population  population;
  city_population2  city_population%TYPE;
BEGIN
  NULL;
END;
/

<<l1>>
DECLARE
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
  city_population  population;
  city_population2  city_population%TYPE;
  city_population3  l1.city_population%TYPE;
BEGIN
  NULL;
END;
/

CREATE OR REPLACE  FUNCTION fun1 RETURN INT IS
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
  city_population  fun1.population;
  city_population2  fun1.city_population%TYPE;
BEGIN
  RETURN 1;
END;
/
DROP FUNCTION fun1;

CREATE OR REPLACE  FUNCTION fun RETURN INT IS
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
  city_population  population;
BEGIN
  RETURN 1;
END;
/
DROP FUNCTION fun;

--init a associative array variable
CREATE OR REPLACE  FUNCTION fun RETURN INT IS
  TYPE population IS TABLE OF NUMBER INDEX BY VARCHAR2(64);
  city_population  population;
BEGIN
  RETURN 1;
END;
/

SELECT fun() FROM DUAL;
DROP FUNCTION fun;


--associative array variable element assignment
DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  city_population(2) := 100;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF population INDEX BY INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population(2) := 100;
  city_population2(4)(5) := 100;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF population INDEX BY INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population(2) := 100;
  city_population(2) := 200;
  city_population2(3) := city_population;
  city_population2(4)(5) := 100;
END;
/

DECLARE
  city_population  INT;
BEGIN
  city_population(2) := 100;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
  var varchar2(10) := 'abc';
BEGIN
  city_population(2) := var;
END;
/


--exceptions
DECLARE
  TYPE population IS TABLE OF VARCHAR2(3) INDEX BY INT;
  city_population  population;
BEGIN
  city_population(2) := 'abcdef';
END;
/

DECLARE
  TYPE triplet IS TABLE OF INTEGER INDEX BY INTEGER;
  TYPE trio    IS TABLE OF INTEGER INDEX BY INTEGER;
  group1 triplet;
  group2 triplet;
  group3 trio;
BEGIN
  group1(1) := 1;
  group2 := group1;  -- succeeds
  group3 := group1;  -- fails
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  city_population(-2147483649) := 1;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  city_population(2147483648) := 1;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  raise notice '%', city_population(1);
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  city_population(null) := 1;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  a := 1;
  raise notice '%', b(1);  
END;
/

--associative array variable assignment
DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF INT INDEX BY INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population2(2) := 100;
  city_population := city_population2;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF population INDEX BY INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population(2) := 100;
  city_population2(1) := city_population;
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF population INDEX BY INT;
  TYPE population3 IS TABLE OF INT INDEX BY INT;
  city_population3  population3;
  city_population2  population2;
BEGIN
  city_population3(2) := 100;
  city_population2(1) := city_population3;
END;
/

--associative array variable as out parameter
DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
  ret int;
  FUNCTION fout(a OUT population) RETURN INT AS
  BEGIN
    a(1) := 100;
	return 2;
  END;
BEGIN
  ret := fout(city_population);
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  v1  INT;
  ret population;
  FUNCTION fout(a OUT INT) RETURN population AS
    r1 population;
  BEGIN
    r1(1) := 100;
	a := 2;
	return r1;
  END;
BEGIN
  ret := fout(v1);
  raise notice 'v1 = %, ret(1) = %', v1, ret(1);
END;
/

CREATE TABLE tc(i int);
DECLARE
  TYPE EmpCurTyp IS REF CURSOR RETURN tc%ROWTYPE;
  TYPE assoc1 IS TABLE OF EmpCurTyp INDEX BY varchar(10);
  a assoc1;
BEGIN
   null;
END;
/
DROP TABLE tc;




--associative array element evaluation
DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  city_population  population;
BEGIN
  city_population(2) := 100;
  raise notice 'city_population(2) = %', city_population(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT INDEX BY INT;
  TYPE population2 IS TABLE OF population INDEX BY INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population(2) := 100;
  city_population2(1) := city_population;
  raise notice 'city_population2(1)(2) = %', city_population2(1)(2);
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  TYPE fourth_type IS TABLE OF third_type INDEX BY INT;
  a fourth_type;
BEGIN
  a(1)(2)(3)(4) := 12;
  raise notice '%',a(1)(2)(3)(4);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  a(1) := 11;  
  a(1) := 22;
  raise notice '%', a(1); 
END;
/

--one-dim a(1) := b(1)
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  b mytype;
BEGIN
  a(2) := 11;  
  b(1) := a(2);
  raise notice '%', b(1); 
END;
/

--one-dim a := b 
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  b mytype;
BEGIN
  a(1) := 11;  
  a(2) := 22;
  b := a;
  raise notice '%, %', b(1), b(2);  
END;
/

--associative array of associative array
DECLARE
  TYPE first_type IS TABLE OF varchar(10) INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10);
  a first_type;
  b second_type;
BEGIN
  a('tom') := 'good';
  b('Jone')('Jim') := 'high';
  raise notice '%', b('Jone')('Jim');  
END;
/

DECLARE
  TYPE first_type IS TABLE OF varchar(10) INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10);
  a first_type;
  b second_type;
BEGIN
  b('aa')('bb') := 'abc';
  raise notice '%', b('aa')('bb'); 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type  INDEX BY INT;
  a first_type;
  b second_type;
BEGIN
  a(1) := 1;
  b(1) := a;
  raise notice '%', b(1)(1); 
END;
/

--associative array assignment
DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  a1 first_type; 
  a2 first_type;    
  b second_type;
  c1 third_type;
BEGIN
  a1(1) := 1;
  a1(2) := 2;
  a1(3) := 0;
  a2(1) := 11;
  b(1) := a1;
  raise notice '%,%,%', b(1)(1),b(1)(2),b(1)(3);
  b(1) := a2;
  raise notice '%', b(1)(1); 
  b(1)(1) := 22;
  raise notice '%', b(1)(1); 
  c1(1)(2) := a1;
  raise notice '%,%,%',c1(1)(2)(1),c1(1)(2)(2),c1(1)(2)(3);
  c1(1)(2)(3) := 10;
  raise notice '%,%,%',c1(1)(2)(1),c1(1)(2)(2),c1(1)(2)(3);
  a1 := b(1);
  raise notice '%',a1(1);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  b mytype;
BEGIN
  a(0) := 11;  
  raise notice '%', a(0);  
END;
/

--colletion variable assigns to a ordinary variable
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  i int;
BEGIN
  a(1) := 11;  
  i := a(1);
  raise notice '%', i; 
END;
/

DECLARE
  TYPE mytype IS TABLE OF VARCHAR(10) INDEX BY VARCHAR(10);
  a mytype;
  i VARCHAR;
BEGIN
  a('Jone') := 'new';  
  i := a('Jone');
  raise notice '%', i; 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  a first_type;
  b second_type;
  i int;
BEGIN
  a(1) := 11;
  b(1)(1) := 111;
  i := b(1)(1);
  raise notice '%',  i;  
END;
/

--If the data type of index is a string type, sort order is determined by the initialization parameters NLS_SORT and NLS_COMP 
SET NLS_SORT = 'BINARY_CI';
SET NLS_COMP = 'LINGUISTIC';

DECLARE
  TYPE name_array IS TABLE OF VARCHAR2(100) INDEX BY VARCHAR2(30);
  v_names name_array;
  v_key VARCHAR2(30);
BEGIN
  v_names('Smith') := 'John';
  v_names('SMITH') := 'Mike'; 
  raise notice 'BINARY count is: %', v_names.COUNT;
END;
/

SET NLS_SORT = 'BINARY';
SET NLS_COMP = 'BINARY';
DECLARE
  TYPE name_array IS TABLE OF VARCHAR2(100) INDEX BY VARCHAR2(30);
  v_names name_array;
  v_key VARCHAR2(30);
BEGIN
  v_names('Smith') := 'John';
  v_names('SMITH') := 'Mike';
  raise notice 'BINARY count is: %', v_names.COUNT;
END;
/

RESET NLS_SORT;
RESET NLS_COMP;


--one-dim associative array first and last methods
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  a(1) := 1;
  raise notice '%', a.first();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  i int;
  j int;  
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  raise notice '%,%', a.first(),a.last();
  i := a.first();
  j := a.last();
  raise notice '%,%', i, j;
END;
/

DECLARE
  TYPE first_type IS TABLE OF int INDEX BY VARCHAR(10);
   a first_type;
   i VARCHAR(10);
   j VARCHAR(10);
BEGIN
   a('Tom') := 1;
   a('Jone') := 2;
   a('Sun') := 3; 
   i := a.first();
   j := a.last();
   raise notice '%,%', i,j;
END;
/
  
--mult-dim associative array FIRST and LAST method
DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10);
  TYPE third_type IS TABLE OF second_type INDEX BY VARCHAR(10);
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a('Tom') := 1;
  a('Jone') := 2;
  a('Sun') := 3;
  b('a') := a;
  b('t') := a;
  b('s') := a;
  c('China') := b;
  c('American') := b;
  c('France') := b;
  raise notice '%,%,%,%,%,%', a.first, b.first(), b('a').first, c.first, c('American').first(), c('American')('s').first();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INTEGER;
  a mytype;
BEGIN
  raise notice '%', a.first(); 
END;
/

--one-dim associative array COUNT method
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  i int;
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  raise notice '%', a.count();
  i := a.count();
  raise notice '%', i;
END;
/

DECLARE
  TYPE first_type IS TABLE OF int INDEX BY VARCHAR(10);
   a first_type;
   i VARCHAR(10);
BEGIN
   a('Tom') := 1;
   a('Jone') := 2;
   a('Sun') := 3; 
   i := a.count();
   raise notice '%', i;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INTEGER;
  a mytype;
BEGIN
  raise notice '%', a.count(); 
END;
/

--mult-dim associative array COUNT method
DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10);
  TYPE third_type IS TABLE OF second_type INDEX BY VARCHAR(10);
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a('Tom') := 1;
  a('Jone') := 2;
  a('Sun') := 3;
  b('a') := a;
  b('t') := a;
  b('s') := a;
  b('y') := a;
  c('China') := b;
  c('American') := b;
  raise notice '%,%,%,%,%,%', a.count, b.count(), b('a').count, c.count, c('American').count, c('American')('s').count();
END;
/

--one-dim associative array LIMIT method
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT ;
  a mytype;
BEGIN
  a(1) := 1;
  raise notice '%', a.limit(); 
END;
/

--mult-dim associative array LIMIT method
DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10);
  TYPE third_type IS TABLE OF second_type INDEX BY VARCHAR(10);
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a('Tom') := 1;
  a('Jone') := 2;
  a('Sun') := 3;
  b('a') := a;
  b('t') := a;
  b('s') := a;
  b('y') := a;
  c('China') := b;
  c('American') := b;
  raise notice '%,%,%,%,%,%', a.limit, b.limit(), b('a').limit, c.limit, c('American').limit(), c('American')('s').limit;
END;
/

--associative array PRIOR method
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY VARCHAR(10);
  b mytype;
BEGIN
  b('Tom') := 1;
  b('Jone') := 2;
  b('Sun') := 3;
  raise notice 'b.prior(''Jone'') = %, b.prior(''Sun'') = %', b.prior('Jone'), b.prior('Sun');
  raise notice 'b.prior(''Dave'') = %, b.prior(''Tom'') = %',b.prior('Dave'), b.prior('Tom');  
  raise notice 'b.prior(NULL) = %', b.prior(NULL);
END; 
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1('Tom') := 1;
  a1('Jone') := 2;
  a1('Sun') := 3;
  a2('a') := 11;
  a2('b') := 22;
  b(1) := a1;
  b(2) := a2;
  b(3) := a1;
  raise notice 'a1.prior(''Sun'') = %, b.prior(2) = %, b(2).prior(''b'') = %', a1.prior('Sun'),b.prior(2), b(2).prior('b');  
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype ;
BEGIN
  raise notice '%', a.prior(1); --null
  raise notice '%', a.next(1);  --null
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype ;
BEGIN
  a(1) := 1; 
  --a.delete(1);
  raise notice '%', a.prior(1); --null
  raise notice '%', a.next(1);  --null
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a(1) := 11;
  a(2) := 22;
  a(3) := 33;
  a2(100) := 101;
  a2(200) := 202;
  b(1) := a;
  b(2) := a2;
  b2(1) := a;
  b2(3) := a2;
  c(1) := b;
  c(2) := b2;
  raise notice 'a.prior(2) = %, b.prior(2) = %, b(1).prior(3) = %, c(2)(3).prior(200) =%', a.prior(2), b.prior(2), b(1).prior(3), c(2)(3).prior(200); 
END;
/

--associative array NEXT method
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY VARCHAR(10);
  b mytype;
BEGIN
  b('Tom') := 1;
  b('Jone') := 2;
  b('Sun') := 3;
  raise notice ' b.NEXT(''Jone'') = %, b.NEXT(''Sun'') = %', b.NEXT('Jone'), b.NEXT('Sun');
  raise notice '%,%', b.NEXT('Dave'), b.NEXT('Tom');  
  raise notice 'b.NEXT(NULL) = %', b.NEXT(NULL);
END; 
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY VARCHAR(10);
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1('Tom') := 1;
  a1('Jone') := 2;
  a1('Sun') := 3;
  a2('a') := 11;
  a2('b') := 22;
  b(1) := a1;
  b(2) := a2;
  b(3) := a1;
  raise notice 'a1.NEXT(''Sun'') = %, b.NEXT(1) = %, b(2).NEXT(''a'') = %', a1.NEXT('Sun'), b.NEXT(1), b(2).NEXT('a');
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype ;
BEGIN
  raise notice '%', a.NEXT(1); --null
  raise notice '%', a.NEXT(1);  --null
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype ;
BEGIN
  a(1) := 1; 
  --a.delete(1);
  raise notice '%', a.NEXT(1); --null
  raise notice '%', a.NEXT(1);  --null
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a(1) := 11;
  a(2) := 22;
  a(3) := 33;
  a2(100) := 101;
  a2(200) := 202;
  b(1) := a;
  b(2) := a2;
  b2(1) := a;
  b2(3) := a2;
  c(1) := b;
  c(2) := b2;
  raise notice 'a.NEXT(2) = %, b.NEXT(1) = %, b(1).NEXT(2) = %, c(2)(3).NEXT(100) =%', a.NEXT(2), b.NEXT(1), b(1).NEXT(2), c(2)(3).NEXT(100); 
END;
/

--associative array EXISTS method
DECLARE
  TYPE mytype2 IS TABLE OF INT INDEX BY INT;
  b mytype2;
BEGIN
  b(1) := 1;
  b(2) := 2;
  b(3) := 3;
  raise notice 'b.EXISTS(1) = %, b.EXISTS(2) = %', b.EXISTS(1), b.EXISTS(2);
  raise notice 'b.EXISTS(0) = %, b.EXISTS(2) = %, b.EXISTS(4) = %', b.EXISTS(0), b.EXISTS(2), b.EXISTS(4);  
  raise notice 'b.EXISTS(NULL) = %', b.EXISTS(NULL);  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INTEGER INDEX BY VARCHAR(20);
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(20);
  a first_type;
  b second_type;
BEGIN
  b(1) := a;
  b(2) := a;
  raise notice 'a.exists(1) = %, b(1).exists(1) = %', a.exists(1), b(1).exists(1);
  a(1) := 11;
  b(3) := a;
  raise notice 'b(3).exists(1) = %',b(3).exists(1);
END;
/

--associative array DELETE method
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  b mytype;
BEGIN
  b(1) := 1;
  b(2) := 2;
  raise notice 'before delete b count is %', b.count();
  b.delete();
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  b mytype;
BEGIN
  b(1) := 1;
  b(2) := 2;
  raise notice 'before delete b count is %', b.count();
  b.delete;
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10) INDEX BY VARCHAR(4);
  a mytype;
  b mytype2;
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  b('a') := 'Tom';
  b('b') := 'Jone';
  b('c') := 'Sun';
  raise notice 'before delete a count is %', a.count();
  a.delete(2);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete('b');
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10) INDEX BY VARCHAR(4);
  a mytype;
  b mytype2;
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  b('a') := 'Tom';
  b('b') := 'Jone';
  b('c') := 'Sun';
  raise notice 'before delete a count is %', a.count();
  a.delete(2,3);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete('a', 'b');
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10) INDEX BY VARCHAR(4);
  a mytype;
  b mytype2;
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  b('a') := 'Tom';
  b('b') := 'Jone';
  b('c') := 'Sun';
  raise notice 'before delete a count is %', a.count();
  a.delete(2,2);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete('a', 'a');
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10) INDEX BY VARCHAR(4);
  a mytype;
  b mytype2;
BEGIN
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  b('a') := 'Tom';
  b('b') := 'Jone';
  b('c') := 'Sun'; 
  raise notice 'before delete a count is %', a.count();
  a.delete(2,1);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete('b','a');
  raise notice 'after delete b count is %', b.count();  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(5);
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  a1 first_type;
  a2 first_type;
  b1 second_type;
  b2 second_type;  
  c1 third_type;
  c2 third_type;
BEGIN
  a1(1) := 1;
  a1(2) := 2;
  a1(3) := 3;
  a2(1) := 11;
  a2(2) := 22;
  a2(3) := 33;
  c1(1) := b1;
  c1(2) := b2;
  c2(1) := b1;
  c2(2) := b2;
  b1(1) := a1;
  b1(2) := a2;
  b1(3) := a2;
  b1(4) := a2;
  b2(1) := a1;
  b2(2) := a1; 
  b2(3) := a2;
  b2(4) := a2;
  
  raise notice 'before delete b1 count is %', b1.count();
  b1.delete(2);
  raise notice 'after delete b1 count is %', b1.count(); 
   
  raise notice 'before delete b1 count is %', b1.count();
  b1.delete(1,1);
  raise notice 'after delete b1 count is %', b1.count(); 
  
  raise notice 'before delete b1 count is %', b1.count();
  b1.delete();
  raise notice 'after delete b1 count is %', b1.count(); 
  
  raise notice 'before delete b2 count is %', b2.count();
  b2.delete(1,3);
  raise notice 'after delete b2 count is %', b2.count(); 
  
  raise notice 'before delete c1 count is %', c1.count();
  c1.delete(1);
  raise notice 'after delete c1 count is %', c1.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  b mytype;
BEGIN
  b(1) := 1;
  b(2) := 2;
  b(3) := 3;
  b.delete(0,6);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  b mytype;
BEGIN
  b(1) := 1;
  b(2) := 2;
  b(3) := 3;

  b.delete(-1,2);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  b mytype;
BEGIN
  b(1) := 1;
  b(2) := 2;
  b(3) := 3;
  b.delete(2,5);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

--associative array comarision
DECLARE  
  TYPE ints_t IS TABLE OF VARCHAR2(5) INDEX BY INTEGER;  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NULL THEN
    raise notice 'l_ints is null';
  ELSE 
    raise notice 'l_ints is not null';
  END IF;
END;
/

DECLARE  
  TYPE ints_t IS TABLE OF VARCHAR2(5) INDEX BY INTEGER;  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NOT NULL THEN
    raise notice 'l_ints is not null';
  ELSE
    raise notice 'l_ints is null';
  END IF;   
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  IF a IS NULL THEN
    raise notice 'a is null';
  ELSE 
    raise notice 'a is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype;
  b mytype2;
BEGIN
  IF b IS NULL THEN
    raise notice 'b is null';
  ELSE 
    raise notice 'b is not null';
  END IF;

  b(1) := a;
  IF b(1) IS NULL THEN
    raise notice 'b(1) is null';
  ELSE 
    raise notice 'b(1) is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
  b mytype;
BEGIN
  IF a = b THEN
    raise notice 'equal';
  ELSE 
    raise notice 'not equal';
  END IF;
END;
/

--2 Nested Tables
--Declaring nested tables in a package
CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS TABLE OF VARCHAR2(20);
  FUNCTION Init_My_AA RETURN INT;
END My_Types;
/
CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN INT IS
  BEGIN
    RETURN 1;
  END Init_My_AA;
END My_Types;
/
DROP PACKAGE My_Types;


CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS TABLE OF VARCHAR2(20);
  FUNCTION Init_My_AA RETURN My_AA;
END My_Types;
/

CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN My_AA IS
    Ret My_AA;
  BEGIN
    RETURN Ret;
  END Init_My_AA;
END;
/
DROP PACKAGE My_Types;

--Declaring nested tables, declare a variable
DECLARE
  TYPE population IS TABLE OF NUMBER;
  city_population  population;
  city_population2  city_population%TYPE;
BEGIN
  NULL;
END;
/

<<l1>>
DECLARE
  TYPE population IS TABLE OF NUMBER;
  city_population  population;
  city_population2  city_population%TYPE;
  city_population3  l1.city_population%TYPE;
BEGIN
  NULL;
END;
/

CREATE OR REPLACE  FUNCTION fun1 RETURN INT IS
  TYPE population IS TABLE OF NUMBER;
  city_population  fun1.population;
  city_population2  fun1.city_population%TYPE;
BEGIN
  RETURN 1;
END;
/
DROP FUNCTION fun1;

CREATE OR REPLACE  FUNCTION fun RETURN INT IS
  TYPE population IS TABLE OF NUMBER;
  city_population  population;
BEGIN
  RETURN 1;
END;
/
DROP FUNCTION fun;

DECLARE
  TYPE population IS TABLE OF VARCHAR2(15) NOT NULL;
  a population;
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS TABLE OF VARCHAR2(15);
  a population NOT NULL;
BEGIN
  NULL;
END;
/

--init a nested table variable, collection constructor is a function with the same name as a collection type
DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
 
  dept_names dnames_tab := dnames_tab(
    'Shipping','Sales','Finance','Payroll');
 
  empty_set dnames_tab;
 
  PROCEDURE print_dept_names_status IS
  BEGIN
    IF dept_names IS NULL THEN
      raise notice 'dept_names is null.';
    ELSE
      raise notice 'dept_names is not null.';
    END IF;
  END  print_dept_names_status;
 
BEGIN
  print_dept_names_status;
  dept_names := empty_set;
  print_dept_names_status;
  dept_names := dnames_tab (
    'Shipping','Sales','Finance','Payroll');
  print_dept_names_status;
END;
/

--nested table variable element assignment
DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population := population(1, 2);
BEGIN
  city_population(2) := 100;
  raise notice 'city_population(1) = %, city_population(2) = %', city_population(1), city_population(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  TYPE population2 IS TABLE OF population;
  city_population  population := population(100);
  city_population2  population2;
BEGIN
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
  var varchar2(10) := 'abc';
BEGIN
  city_population := population(var);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
  var int := 100;
BEGIN
  city_population := population(var);
  raise notice 'city_population(1) = %', city_population(1);
END;
/

--exceptions
DECLARE
  TYPE population IS TABLE OF VARCHAR2(3);
  city_population  population;
BEGIN
  city_population(2) := 'abcdef';
END;
/

DECLARE
  TYPE triplet IS TABLE OF INTEGER;
  TYPE trio    IS TABLE OF INTEGER;
  group1 triplet := triplet(1);
  group2 triplet;
  group3 trio;
BEGIN
  group1(1) := 1;
  group2 := group1;  -- succeeds
END;
/

DECLARE
  TYPE triplet IS TABLE OF INTEGER;
  TYPE trio    IS TABLE OF INTEGER;
  group1 triplet := triplet(1);
  group2 triplet;
  group3 trio;
BEGIN
  group1(1) := 1;
  group3 := group1;  -- fails
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
BEGIN
  city_population(-2147483649) := 1;
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
BEGIN
  city_population(2147483648) := 1;
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
BEGIN
  raise notice '%', city_population(1);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
BEGIN
  city_population(1) := 1;
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
BEGIN
  city_population(null) := 1;
END;
/

--nested table variable assignment
DECLARE
  TYPE population IS TABLE OF INT;
  TYPE population2 IS TABLE OF INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population2 := population2(100);
  city_population := city_population2;
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  TYPE population2 IS TABLE OF population;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population := population(100);
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  TYPE population2 IS TABLE OF population;
  TYPE population3 IS TABLE OF INT;
  city_population3  population3;
  city_population2  population2;
BEGIN
  city_population3 := population3(100);
  city_population2 := population2(city_population3);
END;
/

--nested table variable as out parameter
DECLARE
  TYPE population IS TABLE OF INT;
  city_population  population;
  ret int;
  FUNCTION fout(a OUT population) RETURN INT AS
  BEGIN
    a := population(100);
	return 2;
  END;
BEGIN
  ret := fout(city_population);
END;
/

DECLARE
  TYPE population IS TABLE OF INT;
  v1  INT;
  ret population;
  FUNCTION fout(a OUT INT) RETURN population AS
    r1 population;
  BEGIN
    r1 := population(100);
	a := 2;
	return r1;
  END;
BEGIN
  ret := fout(v1);
  raise notice 'v1 = %, ret(1) = %', v1, ret(1);
END;
/

--nested table element evaluation
DECLARE
  TYPE population IS TABLE OF INT;
  TYPE population2 IS TABLE OF population;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population := population(100);
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  TYPE fourth_type IS TABLE OF third_type;
  a first_type;
  b second_type;
  c third_type;
  d fourth_type;
BEGIN
  a := first_type(11, 22);
  b := second_type(a, a);
  c := third_type(b, b);
  d := fourth_type(c);
  raise notice '%', d(1)(2)(2)(2);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  a := mytype(11);  
  a(1) := 22;
  raise notice '%', a(1); 
END;
/

--one-dim a(1) := b(1)
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(11);
  b mytype;
BEGIN
  b := mytype(a(1));
  raise notice '%', b(1); 
END;
/

--one-dim a := b 
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
  b mytype;
BEGIN
  a := mytype(11, 22);  
  b := a;
  raise notice '%, %', b(1), b(2);  
END;
/

--nested table assignment
DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a1 first_type; 
  a2 first_type;    
  b second_type;
  b2 second_type; 
  c1 third_type;
BEGIN
  a1 := first_type(1, 2, 0);
  a2 := first_type(11);
  b := second_type(a1);
  raise notice '%,%,%', b(1)(1),b(1)(2),b(1)(3);
  b(1) := a2;
  raise notice '%', b(1)(1); 
  b(1)(1) := 22;
  raise notice '%', b(1)(1);
  b2 := second_type(a1);
  c1 := third_type(b, b2);
  raise notice '%,%,%',c1(2)(1)(1),c1(2)(1)(2),c1(2)(1)(3);
  c1(2)(1)(3) := 10;
  raise notice '%,%,%',c1(2)(1)(1),c1(2)(1)(2),c1(2)(1)(3);
  a1 := b(1);
  raise notice '%',a1(1);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(2) := 11;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(0) := 11;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(3) := 11;
END;
/

--colletion variable assigns to a ordinary variable
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
  i int;
BEGIN
  a := mytype(11);  
  i := a(1);
  raise notice '%', i; 
END;
/

DECLARE
  TYPE mytype IS TABLE OF VARCHAR(10);
  a mytype;
  i VARCHAR;
BEGIN
  a := mytype('new');
  i := a(1);
  raise notice '%', i; 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type;
  b second_type;
  i int;
BEGIN
  a := first_type(11);
  b := second_type(a);
  i := b(1)(1);
  raise notice '%', i;  
END;
/

--nested table variable compare with NULL or NOT NULL
DECLARE  
  TYPE ints_t IS TABLE OF VARCHAR2(5);  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NULL THEN
    raise notice 'l_ints is null';
  ELSE 
    raise notice 'l_ints is not null';
  END IF;
END;
/

DECLARE  
  TYPE ints_t IS TABLE OF VARCHAR2(5);  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NOT NULL THEN
    raise notice 'l_ints is not null';
  ELSE
    raise notice 'l_ints is null';
  END IF;   
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  IF a IS NULL THEN
    raise notice 'a is null';
  ELSE 
    raise notice 'a is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF mytype;
  a mytype;
  b mytype2;
BEGIN
  IF b IS NULL THEN
    raise notice 'b is null';
  ELSE 
    raise notice 'b is not null';
  END IF;

  b := mytype2(a);
  IF b(1) IS NULL THEN
    raise notice 'b(1) is null';
  ELSE 
    raise notice 'b(1) is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF mytype;
  a mytype;
  b mytype2;
BEGIN
  IF b IS NULL THEN
    raise notice 'b is null';
  ELSE 
    raise notice 'b is not null';
  END IF;

  a := mytype(1);
  b := mytype2(a);
  IF b(1) IS NULL THEN
    raise notice 'b(1) is null';
  ELSE 
    raise notice 'b(1) is not null';
  END IF;
END;
/

--two nested table variables comaprision
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
  b mytype;
BEGIN
  IF a = b THEN
    raise notice '=';
  ELSE 
    raise notice '!=';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
  b mytype;
BEGIN
  IF a = b THEN
    raise notice '=';
  ELSE 
    raise notice '!=';
  END IF;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
  b mytype := mytype();
BEGIN
  IF a = b THEN
    raise notice '=';
  ELSE 
    raise notice '!=';
  END IF;
END;
/

DECLARE
  TYPE first_type IS TABLE OF FLOAT;
  a1 first_type := first_type(2,1);
  a2 first_type := first_type(1,2);
BEGIN
  IF a1 = a2  THEN
     raise notice 'a1 = a2';
  ELSE
	 raise notice 'a1 != a2';
  END IF;
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a1 first_type := first_type(3,1);
  a2 first_type := first_type(1,2,1);
  TYPE second_type IS TABLE OF first_type;
  b1 second_type := second_type(a1);
  b2 second_type := second_type(a2);
  b3 second_type := second_type(first_type(2,1,1));
BEGIN
  IF b1 = b2  THEN
     raise notice 'b1 = b2';
  ELSE
	 raise notice 'b1 != b2';
  END IF;  
  raise notice '%,%', b1(1)(1),b1(1)(2); --3,1
  raise notice '%,%,%', b2(1)(1),b2(1)(2),b2(1)(3);   --1,2,1
  IF b2 = b3  THEN
     raise notice 'b2 = b3';
  ELSE
	 raise notice 'b2 != b3';
  END IF; 
  raise notice '%,%,%', b2(1)(1),b2(1)(2),b2(1)(3);   --1,2,1 
  raise notice '%,%,%', b3(1)(1),b3(1)(2),b3(1)(3);   --2,1,1
END;
/

DECLARE
  TYPE first_type IS TABLE OF VARCHAR(10);
  a1 first_type := first_type('King','Tom');
  a2 first_type := first_type('tom','king');
  TYPE second_type IS TABLE OF first_type;
  b1 second_type := second_type(a2);
  b2 second_type := second_type(a1);
  b3 second_type := second_type(first_type('King','Tom'));
BEGIN
  IF b1 = b2  THEN
     raise notice 'b1 = b2';
  ELSE
	 raise notice 'b1 != b2';
  END IF;
  IF b2(1) = b3(1)  THEN
     raise notice 'b2 = b3';
  ELSE
	 raise notice 'b2 != b3';
  END IF; 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a1 first_type := first_type(2,1);
  a2 first_type := first_type(11,12);
  TYPE second_type IS TABLE OF first_type;
  b1 second_type := second_type(a1);
  b2 second_type := second_type(a1);
  TYPE third_type IS TABLE OF second_type;
  b3 second_type := second_type(first_type(4), first_type(4), first_type(3));
  b4 second_type := second_type(first_type(3), first_type(4), first_type(4));
  c1 third_type;
  c2 third_type ;  
BEGIN
  c1 := third_type(second_type(a1,a1,a2),second_type(a1,a2), b3,b1,b1);
  c2 := third_type(b1,b1,second_type(a1,a2),b4,second_type(a1,a2,a1));
     IF c1 = c2  THEN  
       raise notice 'c1 = c2';
     ELSE
    raise notice 'c1 != c2';
     END IF;
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a1 first_type;
  a2 first_type := first_type();
  a3 first_type;
  a4 first_type := first_type();
BEGIN
  IF a1 = a2 THEN
     raise notice 'a1 = a2';
  ELSE
	 raise notice 'a1 != a2';
  END IF;
  IF a1 = a3  THEN
     raise notice 'a1 = a3';
  ELSE
	 raise notice 'a1 != a3';
  END IF; 
  IF a2 = a4  THEN
     raise notice 'a2 = a4';
  ELSE
	 raise notice 'a2 != a4';
  END IF;   
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,3);
  b1 second_type := second_type(a,first_type(1,2,3));
  b2 second_type := second_type(a,first_type(1,2,3));
BEGIN
  IF b1(1) = a THEN
     raise notice 'b1(1) is equal to a';
  ELSE
	 raise notice 'b1(1) is not equal to a';
  END IF;

  IF a = b1(1) THEN
     raise notice 'a is equal to b1(1)';
  ELSE
	 raise notice 'a is not equal to b1(1)';
  END IF;  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,4);
  b1 second_type := second_type(a,first_type(1,2,3));
  b2 second_type := second_type(a,first_type(1,2,3));
BEGIN
  IF b1(2) = a THEN
     raise notice 'b1(2) is equal to a';
  ELSE
	 raise notice 'b1(2) is not equal to a';
  END IF;

  IF a = b1(2) THEN
     raise notice 'a is equal to b1(2)';
  ELSE
	 raise notice 'a is not equal to b1(2)';
  END IF;  
END;
/

DECLARE
  TYPE first_type IS TABLE OF VARCHAR(10);
  a1 first_type := first_type('King','Tom');
  a2 first_type := first_type('tom','king');
  TYPE second_type IS TABLE OF first_type;
  b1 second_type := second_type(a2);
  b2 second_type := second_type(a1);
  b3 second_type := second_type(first_type('King','Tom'));
BEGIN
  IF b1 != b2  THEN
     raise notice 'b1 != b2';
  ELSE
	 raise notice 'b1 = b2';
  END IF;
  IF b2(1) != b3(1)  THEN
     raise notice 'b2 != b3';
  ELSE
	 raise notice 'b2 = b3';
  END IF; 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,4);
  b1 second_type := second_type(a,first_type(1,2,3));
  b2 second_type := second_type(a,first_type(1,2,3));
BEGIN
  IF b1(2) <> a THEN
     raise notice 'b1(2) is not equal to a';
  ELSE
	 raise notice 'b1(2) is equal to a';
  END IF;

  IF a <> b1(2) THEN
     raise notice 'a is not equal to b1(2)';
  ELSE
	 raise notice 'a is equal to b1(2)';
  END IF;  
END;
/

DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
  dept_names dnames_tab := dnames_tab('Shipping','Sales','Finance','Payroll');
  empty_set dnames_tab; -- Not initialized, therefore null
  PROCEDURE print_dept_names_status IS
  BEGIN
  IF dept_names IS NULL THEN
    raise notice 'dept_names is null.';
  ELSE
    raise notice 'dept_names is not null.';
  END IF;
  END print_dept_names_status;
BEGIN
  print_dept_names_status;
  dept_names := empty_set; -- Assign null collection to dept_names
  print_dept_names_status;
  dept_names := dnames_tab (
  'Shipping','Sales','Finance','Payroll'); -- Re-initialize dept_names
  print_dept_names_status;
END;
/

DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
  TYPE dnames_tab2 IS TABLE OF dnames_tab;
  dept_names dnames_tab := dnames_tab('Shipping','Sales','Finance','Payroll');
  empty_set dnames_tab; -- Not initialized, therefore null
  dept_names2 dnames_tab2 := dnames_tab2(dept_names);
BEGIN
  dept_names2(1) := empty_set; -- Assign null collection to dept_names2(1)
  IF dept_names2(1) IS NULL THEN
    raise notice 'dept_names2(1) is null.';
  ELSE
    raise notice 'dept_names2(1) is not null.';
  END IF;
END;
/

DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
  TYPE dnames_tab2 IS TABLE OF dnames_tab;
  dept_names dnames_tab := dnames_tab('Shipping','Sales','Finance','Payroll');
  empty_set dnames_tab; -- Not initialized, therefore null
  dept_names2 dnames_tab2 := dnames_tab2(dept_names, empty_set);
BEGIN
  dept_names2(1) := dept_names2(2); -- Assign null collection to dept_names(1)
  IF dept_names2(1) IS NULL THEN
    raise notice 'dept_names2(1) is null.';
  ELSE
    raise notice 'dept_names2(1) is not null.';
  END IF;
END;
/

DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
  dept_names dnames_tab := dnames_tab('Shipping', NULL,'Finance','Payroll');
BEGIN
  dept_names(1) := dept_names(2); -- Assign null collection to dept_names
  IF dept_names(1) IS NULL THEN
    raise notice 'dept_names(1) is null.';
  ELSE
    raise notice 'dept_names(1) is not null.';
  END IF;
END;
/

DECLARE
  TYPE dnames_tab IS TABLE OF VARCHAR2(30);
  dept_names dnames_tab;
BEGIN
  dept_names := dnames_tab();
  IF dept_names IS NULL THEN
    raise notice 'dept_names is null.';
  ELSE
    raise notice 'dept_names is not null.';
  END IF;
END;
/

--one-dim nested table first and last methods
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(10);
BEGIN
  raise notice '%', a.first();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.first() = %, a.last() = %', a.first(), a.last();
  i := a.first();
  j := a.last();
  raise notice 'i = %, j = %', i, j;
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1, 2, 3);
  i VARCHAR(10);
  j VARCHAR(10);
BEGIN
  a.delete(1);
  i := a.first();
  j := a.last();
  raise notice 'i = %, j = %', i, j;
END;
/

--mult-dim nested table FIRST and LAST method
DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  b.delete(1);
  c := third_type(b, b, b);
  c.delete(1, 2);
  raise notice 'a.first =  %, b.first = %, b(2).first = %', a.first, b.first(), b(2).first;
  raise notice 'c.first =  %, c(3).first() = %, c(3)(2).first = %', c.first, c(3).first(), c(3)(2).first;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  raise notice '%', a.first(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  raise notice '%', a.last(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.first();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.last();
END;
/

--one-dim nested table COUNT method
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(10);
BEGIN
  raise notice '%', a.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.count() = %', a.count();
  i := a.count();
  raise notice 'i = %', i;
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1, 2, 3);
  i INT;
BEGIN
  a.delete(1);
  i := a.count();
  raise notice 'i = %', i;
END;
/

--mult-dim nested table COUNT method
DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  b.delete(1);
  c := third_type(b, b, b);
  c.delete(1, 2);
  raise notice 'a.count =  %, b.count = %, b(2).count = %', a.count, b.count(), b(2).count;
  raise notice 'c.count =  %, c(3).count() = %, c(3)(2).count = %', c.count, c(3).count(), c(3)(2).count;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  raise notice '%', a.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.count();
END;
/

--one-dim nested table LIMIT method
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.limit() = %', a.limit();
  i := a.limit();
  raise notice 'i = %', i;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  raise notice '%', a.limit(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.limit();
END;
/

--mult-dim nested table LIMIT method
DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  b.delete(1);
  c := third_type(b, b, b);
  c.delete(1, 2);
  raise notice 'a.limit =  %, b.limit = %, b(2).limit = %', a.limit, b.limit(), b(2).limit;
  raise notice 'c.limit =  %, c(3).limit() = %, c(3)(2).limit = %', c.limit, c(3).limit(), c(3)(2).limit;
END;
/


--nested table PRIOR method
DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype := mytype(10, 20, 30);
BEGIN
  raise notice 'b.prior(2) = %, b.prior(3) = %', b.prior(2), b.prior(3);
  raise notice 'b.prior(4) = %, b.prior(1) = %',b.prior(4), b.prior(1);  
  raise notice 'b.prior(NULL) = %', b.prior(NULL);
END; 
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1 := first_type(10, 11, 12);
  a2 := first_type(11, 22);
  b := second_type(a1, a2, a1);
  raise notice 'a1.prior(3) = %, b.prior(2) = %, b(2).prior(2) = %', a1.prior(3),b.prior(2), b(2).prior(2);  
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype ;
BEGIN
  raise notice '%', a.prior(1); --error
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype ;
BEGIN
  a(1) := 1;
  a.delete(1);
  raise notice '%', a.prior(1); --error
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a := first_type(11, 22, 33);
  a2 := first_type(101, 202);
  b := second_type(a, a2);
  b2 := second_type(a, a2, a2, a);
  b2.delete(2);
  c := third_type(b, b2);
  raise notice 'a.prior(2) = %, b2.prior(3) = %, b(1).prior(3) = %, c(2)(3).prior(2) =%',
  a.prior(2), b2.prior(3), b(1).prior(3), c(2)(3).prior(2); 
END;
/


--nested table NEXT method
DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype := mytype(10, 20, 30);
BEGIN
  raise notice 'b.next(2) = %, b.next(3) = %', b.next(2), b.next(3);
  raise notice 'b.next(4) = %, b.next(1) = %',b.next(4), b.next(1);  
  raise notice 'b.next(NULL) = %', b.next(NULL);
END; 
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1 := first_type(10, 11, 12);
  a2 := first_type(11, 22);
  b := second_type(a1, a2, a1);
  raise notice 'a1.next(3) = %, b.next(2) = %, b(2).next(2) = %', a1.next(3), b.next(2), b(2).next(2);  
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype ;
BEGIN
  raise notice '%', a.next(1); --error
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype ;
BEGIN
  a(1) := 1;
  a.delete(1);
  raise notice '%', a.next(1); --error
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a := first_type(11, 22, 33);
  a2 := first_type(101, 202);
  b := second_type(a, a2);
  b2 := second_type(a, a2, a2, a);
  b2.delete(2);
  c := third_type(b, b2);
  raise notice 'a.next(2) = %, b2.next(1) = %, b(1).next(3) = %, c(2)(3).next(2) =%',
  a.next(2), b2.next(1), b(1).next(3), c(2)(3).next(2); 
END;
/

--nested table EXISTS method
DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype := mytype(10, 11, 12);
BEGIN
  raise notice 'b.EXISTS(1) = %, b.EXISTS(2) = %', b.EXISTS(1), b.EXISTS(2);
  raise notice 'b.EXISTS(0) = %, b.EXISTS(4) = %', b.EXISTS(0), b.EXISTS(4);  
  raise notice 'b.EXISTS(NULL) = %', b.EXISTS(NULL);  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INTEGER;
  TYPE second_type IS TABLE OF first_type;
  a first_type;
  b second_type;
BEGIN
  b := second_type(a, a);
  raise notice 'a.exists(1) = %, b(1).exists(1) = %', a.exists(1), b(1).exists(1);
  a := first_type(11);
  b := second_type(a, a, a);
  raise notice 'b(3).exists(1) = %',b(3).exists(1);
END;
/

--nested table DELETE method
DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype := mytype(1, 2);
BEGIN
  raise notice 'before delete b count is %', b.count();
  b.delete();
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype := mytype(1, 2);
BEGIN
  raise notice 'before delete b count is %', b.count();
  b.delete;
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10);
  a mytype;
  b mytype2;
BEGIN
  a := mytype(1, 2, 3);
  b := mytype2('Tom', 'Jone', 'Sun');
  raise notice 'before delete a count is %', a.count();
  a.delete(2);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete(2);
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10);
  a mytype;
  b mytype2;
BEGIN
  a := mytype(1, 2, 3);
  b := mytype2('Tom', 'Jone', 'Sun');
  raise notice 'before delete a count is %', a.count();
  a.delete(2, 3);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete(1, 2);
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10);
  a mytype;
  b mytype2;
BEGIN
  a := mytype(1, 2, 3);
  b := mytype2('Tom', 'Jone', 'Sun');
  raise notice 'before delete a count is %', a.count();
  a.delete(2, 2);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete(2, 2);
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  TYPE mytype2 IS TABLE OF VARCHAR(10);
  a mytype;
  b mytype2;
BEGIN
  a := mytype(1, 2, 3);
  b := mytype2('Tom', 'Jone', 'Sun');
  raise notice 'before delete a count is %', a.count();
  a.delete(2, 1);
  raise notice 'after delete a count is %', a.count(); 
  raise notice 'before delete b count is %', b.count();
  b.delete(2, 1);
  raise notice 'after delete b count is %', b.count();  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a1 first_type;
  a2 first_type;
  b1 second_type;
  b2 second_type;  
  c1 third_type;
  c2 third_type;
BEGIN
  a1 := first_type(1, 2, 3);
  a2 := first_type(11, 22, 33);
  c1 := third_type(b1, b2);
  c2 := third_type(b1, b2);
  b1 := second_type(a1, a2, a2, a2);
  b2 := second_type(a1, a1, a2, a2);

  raise notice 'before delete b1 count is %', b1.count();
  b1.delete(2);
  raise notice 'after delete b1 count is %', b1.count(); 
   
  raise notice 'before delete b1 count is %', b1.count();
  b1.delete(1,1);
  raise notice 'after delete b1 count is %', b1.count(); 
  
  raise notice 'before delete b1 count is %', b1.count();
  b1.delete();
  raise notice 'after delete b1 count is %', b1.count(); 
  
  raise notice 'before delete b2 count is %', b2.count();
  b2.delete(1,3);
  raise notice 'after delete b2 count is %', b2.count(); 
  
  raise notice 'before delete c1 count is %', c1.count();
  c1.delete(1);
  raise notice 'after delete c1 count is %', c1.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype;
BEGIN
  b := mytype(1, 2, 3);
  b.delete(0,6);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype;
BEGIN
  b := mytype(1, 2, 3);
  b.delete(-1,2);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype;
BEGIN
  b := mytype(1, 2, 3);
  b.delete(2,5);
  raise notice 'after dedete b count is %', b.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype ;
BEGIN
  b.delete;
END; 
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype ;
BEGIN
  b.delete(1);
END; 
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  b mytype ;
BEGIN
  b.delete(1, 2);
END; 
/


--mested table TRIM method
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim();
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim;
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(2);
  raise notice '%', a.count;
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.trim(0);
  raise notice 'after trim a count is %', a.count(); 

  a.trim(NULL);
  raise notice 'after trim b count is %', a.count();
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF VARCHAR(10) ;
  a mytype1 := mytype1('Tom','Jone','Sun');
BEGIN
  a.delete(2);
  raise notice 'before trim a count is %', a.count();
  a.trim(2);
  raise notice 'after trim a count is %', a.count();
  raise notice 'a(1) is %', a(1);
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF INT;
  a mytype1 := mytype1(1,2,3,4,5);
BEGIN
  a.delete(3,4);
  raise notice 'before trim a count is %', a.count();
  a.trim(4);
  raise notice 'after trim a count is %', a.count();
  raise notice 'a(1) is %', a(1);
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF VARCHAR(10) ;
  a mytype1 := mytype1('Tom','Jone','Sun','Bell','King');
BEGIN
  a.delete(3,4);
  raise notice 'before trim a count is %', a.count();
  a.trim(4);
  raise notice 'after trim a count is %', a.count();
  raise notice 'a(1) is %', a(1);
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF INT;
  a mytype1 := mytype1(1, 2, 3);
  b mytype1 := mytype1(1, 2, 3); 
BEGIN
  a.delete(2); 
  if a = b then
    raise notice '=';
  else
    raise notice '!=';
  end if;
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF INT;
  a mytype1 := mytype1(1, 2, 3);
  b mytype1 := mytype1(1, 2, 3); 
BEGIN
  a.delete(2);
  b.delete(2);
  if a = b then
    raise notice '=';
  else
    raise notice '!=';
  end if;
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF INT;
  a mytype1 := mytype1(1, 2, 3);
  b mytype1 := mytype1(1, 2, 3); 
BEGIN
  a.delete(2);
  b.delete(1);
  if a = b then
    raise notice '=';
  else
    raise notice '!=';
  end if;
END;
/

DECLARE
  TYPE mytype1 IS TABLE OF INT;
  a mytype1 := mytype1(1, 2, 3);
BEGIN
  a.delete(1); 
  a(1) := 11;
  raise notice 'a(1) = %', a(1);
END;
/

DECLARE
  TYPE nt_type IS TABLE OF INTEGER;
  nt nt_type := nt_type(11, 22, 33);
  PROCEDURE print_nt(n nt_type) AS
    i  NUMBER;
  BEGIN
    i := nt.FIRST;
    
    IF i IS NULL THEN
      raise notice 'nt is empty';
    ELSE
      WHILE i IS NOT NULL LOOP
        raise notice 'nt(%) = %', i, nt(i);
        i := nt.NEXT(i);
      END LOOP;
    END IF;
 
	raise notice '---';
  END;
BEGIN
  print_nt(nt);

  nt.EXTEND(2,1);
  print_nt(nt);

  nt.DELETE(5);
  print_nt(nt);

  nt.EXTEND;
  print_nt(nt);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(-1);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(4); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  a.trim(); 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1,2,3);  
BEGIN
  a.delete(1);
  a.delete(2);
  a.delete(3);
  a.trim(2);
  raise notice '%', a.count; --0
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1,2);  
BEGIN
  a.delete(1);
  a.delete(2);
  a.trim(2);
  raise notice '%,%,%', a.count,a.first,a.last; --0, null,null
END;
/

--nested table EXTEND method
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend();
  raise notice 'after extend a count is %, the new element is %', a.count(), a(4); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend;
  raise notice 'after extend a count is %', a.count(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(2);
  raise notice 'after extend a count is %, the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(2,3);
  raise notice 'after extend a count is %,the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.delete(3);
  raise notice 'before extend a count is %', a.count();
  a.extend(1);
  raise notice 'after extend a count is %', a.count();
  raise notice 'after extend a count is %', a.last(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend('2','3');
  raise notice 'after extend a count is %', a.count(); 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1,2,3);
BEGIN
  a.delete(2, 3);
  raise notice 'a.first is %, a.last is %', a.first(), a.last();
  a.extend();
  raise notice 'a.first is %, a.last is %', a.first(), a.last();
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,3);
  b second_type := second_type(a,first_type(1,2,3));
BEGIN
  raise notice 'before extend b(1) count is %', b(1).count();
  b(1).extend();
  raise notice 'after extend b(1) count is %', b(1).count();
  raise notice 'before extend b count is %', b.count();
  b.extend(2);
  raise notice 'after extend b count is %', b.count();  
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,3);
  b second_type := second_type(first_type(1),a);
BEGIN
  raise notice 'before extend b(2) count is %', b(2).count();
  b(2).extend();
  raise notice 'after extend b(2) count is %', b(2).count();
  raise notice 'before extend b count is %', b.count();
  b.extend(2, 2);
  raise notice 'after extend b count is %', b.count();
  raise notice 'new add elem b(3)(2) is %', b(3)(2); 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type(1,2,3);  
BEGIN
  a.delete(1);
  a.delete(2);
  a.delete(3);
  a.extend(2);
  raise notice '%,%', a.first(),a.last(); --4,5
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  a first_type := first_type();
BEGIN
  a.extend(3);
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  raise notice 'a.first is %',a.first(); --1
  raise notice 'a.last is %',a.last();   --3
  raise notice 'before delete a count is %', a.count(); --3
  a.delete(1, 2);
  raise notice 'after delete a count is %', a.count();  --1
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype;
BEGIN
  a.extend(); 
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.extend(NULL);
  raise notice 'after trim b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.extend(0);
  raise notice 'after trim b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.extend(-2);
  raise notice 'after trim b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1, 2, 3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(3, 2);
  raise notice 'after extend a count is %,the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.extend(-1, 2);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.extend(1, -2);
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(0, 2);
  raise notice 'after extend a count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT ;
  a mytype ;
  TYPE second IS TABLE OF mytype;
  b second := second(a);
BEGIN
  b.extend(1);
  raise notice '%', b(2).count; --0
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT ;
  a mytype ;
  TYPE second IS TABLE OF mytype;
  b second := second(a);
BEGIN
  b.extend(1, 1);
  raise notice '%', b(2).count; --0
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.delete();
  raise notice '%,%',a.first, a.last;
  a.extend(2); 
  raise notice '%,%',a.first, a.last; --1,2
END;
/


--3 Varrays
--Declaring varrays in a package
CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS VARRAY(10) OF VARCHAR2(20);
  FUNCTION Init_My_AA RETURN INT;
END My_Types;
/

CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN INT IS
  BEGIN
    RETURN 1;
  END Init_My_AA;
END My_Types;
/

DROP PACKAGE My_Types;


CREATE OR REPLACE PACKAGE My_Types AUTHID CURRENT_USER IS
  TYPE My_AA IS VARRAY(10) OF VARCHAR2(20);
  FUNCTION Init_My_AA RETURN My_AA;
END My_Types;
/

CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_AA RETURN My_AA IS
    Ret My_AA;
  BEGIN
    RETURN Ret;
  END Init_My_AA;
END;
/
DROP PACKAGE My_Types;

--Declaring varrays, declare a variable
DECLARE
  TYPE population IS VARRAY(10) OF NUMBER;
  city_population  population;
  city_population2  city_population%TYPE;
BEGIN
  NULL;
END;
/

<<l1>>
DECLARE
  TYPE population IS VARRAY(10) OF NUMBER;
  city_population  population;
  city_population2  city_population%TYPE;
  city_population3  l1.city_population%TYPE;
BEGIN
  NULL;
END;
/

CREATE OR REPLACE  FUNCTION fun1 RETURN INT IS
  TYPE population IS VARRAY(10) OF NUMBER;
  city_population  fun1.population;
  city_population2  fun1.city_population%TYPE;
BEGIN
  RETURN 1;
END;
/

DROP FUNCTION fun1;

CREATE OR REPLACE  FUNCTION fun RETURN INT IS
  TYPE population IS VARRAY(10) OF NUMBER;
  city_population  population;
BEGIN
  RETURN 1;
END;
/
DROP FUNCTION fun;

DECLARE
  TYPE population IS VARRAY(10) OF VARCHAR2(15) NOT NULL;
  a population;
BEGIN
  NULL;
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF VARCHAR2(15);
  a population NOT NULL;
BEGIN
  NULL;
END;
/

--init a varray variable, collection constructor is a function with the same name as a collection type
DECLARE
  TYPE dnames_tab IS VARRAY(10) OF VARCHAR2(30);
 
  dept_names dnames_tab := dnames_tab(
    'Shipping','Sales','Finance','Payroll');
 
  empty_set dnames_tab;
 
  PROCEDURE print_dept_names_status IS
  BEGIN
    IF dept_names IS NULL THEN
      raise notice 'dept_names is null.';
    ELSE
      raise notice 'dept_names is not null.';
    END IF;
  END  print_dept_names_status;
 
BEGIN
  print_dept_names_status;
  dept_names := empty_set;
  print_dept_names_status;
  dept_names := dnames_tab (
    'Shipping','Sales','Finance','Payroll');
  print_dept_names_status;
END;
/

--varray variable element assignment
DECLARE
  TYPE population IS VARRAY(20) OF INT;
  city_population  population := population(1, 2);
BEGIN
  city_population(2) := 100;
  raise notice 'city_population(1) = %, city_population(2) = %', city_population(1), city_population(2);
END;
/

DECLARE
  TYPE population IS VARRAY(20) OF INT;
  TYPE population2 IS VARRAY(20) OF population;
  city_population  population := population(100);
  city_population2  population2;
BEGIN
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
  var varchar2(10) := 'abc';
BEGIN
  city_population := population(var);
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
  var int := 100;
BEGIN
  city_population := population(var);
  raise notice 'city_population(1) = %', city_population(1);
END;
/

--exceptions
DECLARE
  TYPE population IS VARRAY(10) OF VARCHAR2(3);
  city_population  population;
BEGIN
  city_population(2) := 'abcdef';
END;
/

DECLARE
  TYPE triplet IS VARRAY(10) OF INTEGER;
  TYPE trio    IS VARRAY(10) OF INTEGER;
  group1 triplet := triplet(1);
  group2 triplet;
  group3 trio;
BEGIN
  group1(1) := 1;
  group2 := group1;  -- succeeds
END;
/

DECLARE
  TYPE triplet IS VARRAY(10) OF INTEGER;
  TYPE trio    IS VARRAY(10) OF INTEGER;
  group1 triplet := triplet(1);
  group2 triplet;
  group3 trio;
BEGIN
  group1(1) := 1;
  group3 := group1;  -- fails
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
BEGIN
  city_population(-2147483649) := 1;
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
BEGIN
  city_population(2147483648) := 1;
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
BEGIN
  raise notice '%', city_population(1);
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
BEGIN
  city_population(1) := 1;
END;
/

DECLARE
  TYPE population IS VARRAY(10) OF INT;
  city_population  population;
BEGIN
  city_population(null) := 1;
END;
/


--varray variable assignment
DECLARE
  TYPE population IS VARRAY(20) OF INT;
  TYPE population2 IS VARRAY(20) OF INT;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population2 := population2(100);
  city_population := city_population2;
END;
/

DECLARE
  TYPE population IS VARRAY(20) OF INT;
  TYPE population2 IS VARRAY(20) OF population;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population := population(100);
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE population IS VARRAY(20) OF INT;
  TYPE population2 IS VARRAY(20) OF population;
  TYPE population3 IS VARRAY(20) OF INT;
  city_population3  population3;
  city_population2  population2;
BEGIN
  city_population3 := population3(100);
  city_population2 := population2(city_population3);
END;
/

--varray variable as out parameter
DECLARE
  TYPE population IS VARRAY(20) OF INT;
  city_population  population;
  ret int;
  FUNCTION fout(a OUT population) RETURN INT AS
  BEGIN
    a := population(100);
	return 2;
  END;
BEGIN
  ret := fout(city_population);
END;
/

DECLARE
  TYPE population IS VARRAY(20) OF INT;
  v1  INT;
  ret population;
  FUNCTION fout(a OUT INT) RETURN population AS
    r1 population;
  BEGIN
    r1 := population(100);
	a := 2;
	return r1;
  END;
BEGIN
  ret := fout(v1);
  raise notice 'v1 = %, ret(1) = %', v1, ret(1);
END;
/

--varray element evaluation
DECLARE
  TYPE population IS VARRAY(20) OF INT;
  TYPE population2 IS VARRAY(20) OF population;
  city_population  population;
  city_population2  population2;
BEGIN
  city_population := population(100);
  city_population2 := population2(city_population);
  raise notice 'city_population2(1)(1) = %', city_population2(1)(1);
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(20) OF second_type;
  TYPE fourth_type IS VARRAY(20) OF third_type;
  a first_type;
  b second_type;
  c third_type;
  d fourth_type;
BEGIN
  a := first_type(11, 22);
  b := second_type(a, a);
  c := third_type(b, b);
  d := fourth_type(c);
  raise notice '%', d(1)(2)(2)(2);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
BEGIN
  a := mytype(11);  
  a(1) := 22;
  raise notice '%', a(1); 
END;
/

--one-dim a(1) := b(1)
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(11);
  b mytype;
BEGIN
  b := mytype(a(1));
  raise notice '%', b(1); 
END;
/

--one-dim a := b 
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
  b mytype;
BEGIN
  a := mytype(11, 22);  
  b := a;
  raise notice '%, %', b(1), b(2);  
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(20) OF second_type;
  a1 first_type; 
  a2 first_type;    
  b second_type;
  b2 second_type; 
  c1 third_type;
BEGIN
  a1 := first_type(1, 2, 0);
  a2 := first_type(11);
  b := second_type(a1);
  raise notice '%,%,%', b(1)(1),b(1)(2),b(1)(3);
  b(1) := a2;
  raise notice '%', b(1)(1); 
  b(1)(1) := 22;
  raise notice '%', b(1)(1);
  b2 := second_type(a1);
  c1 := third_type(b, b2);
  raise notice '%,%,%',c1(2)(1)(1),c1(2)(1)(2),c1(2)(1)(3);
  c1(2)(1)(3) := 10;
  raise notice '%,%,%',c1(2)(1)(1),c1(2)(1)(2),c1(2)(1)(3);
  a1 := b(1);
  raise notice '%',a1(1);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(2) := 11;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(0) := 11;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  a(3) := 11;
END;
/

--colletion variable assigns to a ordinary variable
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
  i int;
BEGIN
  a := mytype(11);  
  i := a(1);
  raise notice '%', i; 
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF VARCHAR(10);
  a mytype;
  i VARCHAR;
BEGIN
  a := mytype('new');
  i := a(1);
  raise notice '%', i; 
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  a first_type;
  b second_type;
  i int;
BEGIN
  a := first_type(11);
  b := second_type(a);
  i := b(1)(1);
  raise notice '%', i;  
END;
/

--varray variable compare with NULL or NOT NULL
DECLARE  
  TYPE ints_t IS VARRAY(20) OF VARCHAR2(5);  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NULL THEN
    raise notice 'l_ints is null';
  ELSE 
    raise notice 'l_ints is not null';
  END IF;
END;
/

DECLARE  
  TYPE ints_t IS VARRAY(20) OF VARCHAR2(5);  
  l_ints   ints_t; 
BEGIN
  IF l_ints IS NOT NULL THEN
    raise notice 'l_ints is not null';
  ELSE
    raise notice 'l_ints is null';
  END IF;   
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
BEGIN
  IF a IS NULL THEN
    raise notice 'a is null';
  ELSE 
    raise notice 'a is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  TYPE mytype2 IS VARRAY(20) OF mytype;
  a mytype;
  b mytype2;
BEGIN
  IF b IS NULL THEN
    raise notice 'b is null';
  ELSE 
    raise notice 'b is not null';
  END IF;

  b := mytype2(a);
  IF b(1) IS NULL THEN
    raise notice 'b(1) is null';
  ELSE 
    raise notice 'b(1) is not null';
  END IF;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  TYPE mytype2 IS VARRAY(20) OF mytype;
  a mytype;
  b mytype2;
BEGIN
  IF b IS NULL THEN
    raise notice 'b is null';
  ELSE 
    raise notice 'b is not null';
  END IF;

  a := mytype(1);
  b := mytype2(a);
  IF b(1) IS NULL THEN
    raise notice 'b(1) is null';
  ELSE 
    raise notice 'b(1) is not null';
  END IF;
END;
/

--two varray variables comaprision
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
  b mytype;
BEGIN
  IF a = b THEN
    raise notice '=';
  ELSE 
    raise notice '!=';
  END IF;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
  b mytype;
BEGIN
  IF a != b THEN
    raise notice '=';
  ELSE 
    raise notice '!=';
  END IF;
END;
/

--one-dim varray first and last methods
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype(1);
BEGIN
  raise notice 'a.first() = %, a.last() = %', a.first(), a.last();
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.first() = %, a.last() = %', a.first(), a.last();
  i := a.first();
  j := a.last();
  raise notice 'i = %, j = %', i, j;
END;
/

DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  a first_type := first_type(1, 2, 3);
  i VARCHAR(10);
  j VARCHAR(10);
BEGIN
  i := a.first();
  j := a.last();
  raise notice 'i = %, j = %', i, j;
END;
/

--mult-dim varray FIRST and LAST method
DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  TYPE second_type IS VARRAY(30) OF first_type;
  TYPE third_type IS VARRAY(30) OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  c := third_type(b, b, b);
  raise notice 'a.first =  %, b.first = %, b(2).first = %', a.first, b.first(), b(2).first;
  raise notice 'c.first =  %, c(3).first() = %, c(3)(2).first = %', c.first, c(3).first(), c(3)(2).first;
  raise notice 'a.last =  %, b.last = %, b(2).last = %', a.last, b.last(), b(2).last;
  raise notice 'c.last =  %, c(3).last() = %, c(3)(2).last = %', c.last, c(3).last(), c(3)(2).last;
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype;
BEGIN
  raise notice '%', a.first(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype;
BEGIN
  raise notice '%', a.last(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.first();
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.last();
END;
/

--one-dim varray COUNT method
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype(10);
BEGIN
  raise notice '%', a.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.count() = %', a.count();
  i := a.count();
  raise notice 'i = %', i;
END;
/

DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  a first_type := first_type(1, 2, 3);
  i INT;
BEGIN
  a.delete();
  i := a.count();
  raise notice 'i = %', i;
END;
/

--mult-dim varray COUNT method
DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  TYPE second_type IS VARRAY(30) OF first_type;
  TYPE third_type IS VARRAY(30) OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  c := third_type(b, b, b);
  raise notice 'a.count =  %, b.count = %, b(2).count = %', a.count, b.count(), b(2).count;
  raise notice 'c.count =  %, c(3).count() = %, c(3)(2).count = %', c.count, c(3).count(), c(3)(2).count;
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype;
BEGIN
  raise notice '%', a.count(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.count();
END;
/

--one-dim varray LIMIT method
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype := mytype(10, 20, 30);
  i int;
  j int;  
BEGIN
  raise notice 'a.limit() = %', a.limit();
  i := a.limit();
  raise notice 'i = %', i;
END;
/

DECLARE
  TYPE mytype IS VARRAY(10) OF INT;
  a mytype;
BEGIN
  raise notice '%', a.limit(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype();
BEGIN
  raise notice '%', a.limit();
END;
/

--mult-dim varray LIMIT method
DECLARE
  TYPE first_type IS VARRAY(10) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(30) OF second_type;
  a first_type;
  b second_type ;
  c third_type;  
BEGIN
  a := first_type(10, 20, 30);
  b := second_type(a, a);
  c := third_type(b, b, b);
  raise notice 'a.limit =  %, b.limit = %, b(2).limit = %', a.limit, b.limit(), b(2).limit;
  raise notice 'c.limit =  %, c(3).limit() = %, c(3)(2).limit = %', c.limit, c(3).limit(), c(3)(2).limit;
END;
/

--varray PRIOR method
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  b mytype := mytype(10, 20, 30);
BEGIN
  raise notice 'b.prior(2) = %, b.prior(3) = %', b.prior(2), b.prior(3);
  raise notice 'b.prior(4) = %, b.prior(1) = %',b.prior(4), b.prior(1);  
  raise notice 'b.prior(NULL) = %', b.prior(NULL);
END; 
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1 := first_type(10, 11, 12);
  a2 := first_type(11, 22);
  b := second_type(a1, a2, a1);
  raise notice 'a1.prior(3) = %, b.prior(2) = %, b(2).prior(2) = %', a1.prior(3),b.prior(2), b(2).prior(2);  
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype ;
BEGIN
  raise notice '%', a.prior(1); --error
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype ;
BEGIN
  a(1) := 1;
  a.delete();
  raise notice '%', a.prior(1); --error
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(20) OF second_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a := first_type(11, 22, 33);
  a2 := first_type(101, 202);
  b := second_type(a, a2);
  b2 := second_type(a, a2, a2, a);
  c := third_type(b, b2);
  raise notice 'a.prior(2) = %, b2.prior(3) = %, b(1).prior(3) = %, c(2)(3).prior(2) = %',
  a.prior(2), b2.prior(3), b(1).prior(3), c(2)(3).prior(2); 
END;
/

--varray NEXT method
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(10, 20, 30);
BEGIN
  raise notice 'b.next(2) = %, b.next(3) = %', b.next(2), b.next(3);
  raise notice 'b.next(4) = %, b.next(1) = %',b.next(4), b.next(1);  
  raise notice 'b.next(NULL) = %', b.next(NULL);
END; 
/

DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  TYPE second_type IS VARRAY(30) OF first_type;
  a1 first_type;
  a2 first_type;
  b second_type;
BEGIN
  a1 := first_type(10, 11, 12);
  a2 := first_type(11, 22);
  b := second_type(a1, a2, a1);
  raise notice 'a1.next(3) = %, b.next(2) = %, b(2).next(2) = %', a1.next(3), b.next(2), b(2).next(2);  
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype ;
BEGIN
  raise notice '%', a.next(1); --error
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  a mytype ;
BEGIN
  a(1) := 1;
  a.delete();
  raise notice '%', a.next(1); --error
END;
/

DECLARE
  TYPE first_type IS VARRAY(30) OF INT;
  TYPE second_type IS VARRAY(30) OF first_type;
  TYPE third_type IS VARRAY(30) OF second_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type;
  c third_type;
BEGIN
  a := first_type(11, 22, 33);
  a2 := first_type(101, 202);
  b := second_type(a, a2);
  b2 := second_type(a, a2, a2, a);
  c := third_type(b, b2);
  raise notice 'a.next(2) = %, b2.next(1) = %, b(1).next(3) = %, c(2)(3).next(2) =%',
  a.next(2), b2.next(1), b(1).next(3), c(2)(3).next(2); 
END;
/

--varray EXISTS method
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(10, 11, 12);
BEGIN
  raise notice 'b.EXISTS(1) = %, b.EXISTS(2) = %', b.EXISTS(1), b.EXISTS(2);
  raise notice 'b.EXISTS(0) = %, b.EXISTS(4) = %', b.EXISTS(0), b.EXISTS(4);  
  raise notice 'b.EXISTS(NULL) = %', b.EXISTS(NULL);  
END;
/

DECLARE
  TYPE first_type IS VARRAY(30) OF INTEGER;
  TYPE second_type IS VARRAY(30) OF first_type;
  a first_type;
  b second_type;
BEGIN
  b := second_type(a, a);
  raise notice 'a.exists(1) = %, b(1).exists(1) = %', a.exists(1), b(1).exists(1);
  a := first_type(11);
  b := second_type(a, a, a);
  raise notice 'b(3).exists(1) = %',b(3).exists(1);
END;
/

--varray DELETE method
DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(1, 2);
BEGIN
  raise notice 'before delete b count is %', b.count();
  b.delete();
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(1, 2);
BEGIN
  raise notice 'before delete b count is %', b.count();
  b.delete;
  raise notice 'after delete b count is %', b.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype;
BEGIN
  b.delete;
END; 
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype;
BEGIN
  b.delete(1);
END; 
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(1, 2, 3);
BEGIN
  b.delete(1);
END; 
/

DECLARE
  TYPE mytype IS VARRAY(30) OF INT;
  b mytype := mytype(1, 2, 3);
BEGIN
  b.delete(1, 2);
END; 
/

--varray TRIM method
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim();
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim;
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(2);
  raise notice '%', a.count;
  FOR i in 1 .. a.count LOOP
    raise notice 'a(%) = %', i, a(i);
  END LOOP;
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.trim(0);
  raise notice 'after trim a count is %', a.count(); 

  a.trim(NULL);
  raise notice 'after trim b count is %', a.count();
END;
/

DECLARE
  TYPE mytype1 IS VARRAY(20) OF VARCHAR(10) ;
  a mytype1 := mytype1('Tom','Jone','Sun');
BEGIN
  raise notice 'before trim a count is %', a.count();
  a.trim(2);
  raise notice 'after trim a count is %', a.count();
  raise notice 'a(1) is %', a(1);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(-1);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.trim(4); 
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  a first_type := first_type(1,2,3);  
BEGIN
  a.trim(3);
  raise notice '%', a.count; --0
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  a first_type := first_type(1,2);  
BEGIN
  a.trim(2);
  raise notice '%,%,%', a.count,a.first,a.last; --0, null,null
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
BEGIN
  a.trim(); 
END;
/

--varray EXTEND method
DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend();
  raise notice 'after extend a count is %, the new element is %', a.count(), a(4); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend;
  raise notice 'after extend a count is %', a.count(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(2);
  raise notice 'after extend a count is %, the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(2,3);
  raise notice 'after extend a count is %,the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend('2','3');
  raise notice 'after extend a count is %,the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  a first_type := first_type(1,2,3);
BEGIN
  raise notice 'a.first is %, a.last is %', a.first(), a.last();
  a.extend();
  raise notice 'a.first is %, a.last is %', a.first(), a.last();
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  a first_type := first_type(1,2,3);
  b second_type := second_type(a,first_type(1,2,3));
BEGIN
  raise notice 'before extend b(1) count is %', b(1).count();
  b(1).extend();
  raise notice 'after extend b(1) count is %', b(1).count();
  raise notice 'before extend b count is %', b.count();
  b.extend(2);
  raise notice 'after extend b count is %', b.count();  
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  a first_type := first_type(1,2,3);
  b second_type := second_type(first_type(1),a);
BEGIN
  raise notice 'before extend b(2) count is %', b(2).count();
  b(2).extend();
  raise notice 'after extend b(2) count is %', b(2).count();
  raise notice 'before extend b count is %', b.count();
  b.extend(2, 2);
  raise notice 'after extend b count is %', b.count();
  raise notice 'new add elem b(3)(2) is %', b(3)(2); 
END;
/


DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  a first_type := first_type();
BEGIN
  a.extend(3);
  a(1) := 1;
  a(2) := 2;
  a(3) := 3;
  raise notice 'a.first is %',a.first(); --1
  raise notice 'a.last is %',a.last();   --3
  raise notice 'before delete a count is %', a.count(); --3
  a.delete();
  raise notice 'after delete a count is %', a.count();  --0
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype;
BEGIN
  a.extend(); 
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(NULL);
  raise notice 'after extend b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(0);
  raise notice 'after extend b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(-2);
  raise notice 'after extend b count is %', a.count();
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1, 2, 3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(3, 2);
  raise notice 'after extend a count is %,the new element is %,%', a.count(),a(4),a(5);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.extend(-1, 2);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  a.extend(1, -2);
END;
/

DECLARE
  TYPE mytype IS VARRAY(20) OF INT;
  a mytype := mytype(1,2,3);
BEGIN
  raise notice 'before extend a count is %', a.count();
  a.extend(0, 2);
  raise notice 'after extend a count is %', a.count();
END;
/


--4 mult-dim collection
--varray of varray
DECLARE
  TYPE first_type IS VARRAY(3) OF INT;
  TYPE second_type IS VARRAY(3) OF first_type;
  a second_type;
  b first_type := first_type(10,11);   
  c second_type;
BEGIN
  a := second_type(first_type(1), first_type(2), first_type(3));
  c := second_type(first_type(21), first_type(22));
  a(1)(1) := 22;
  a(1)(1) := c(1)(1);
  a(1) := b;
  raise notice '%,%', a(1)(1),a(1)(2);
END;
/

DECLARE
  TYPE first_type IS VARRAY(3) OF INT;
  TYPE second_type IS VARRAY(3) OF first_type;
  a second_type;
  b first_type := first_type(10);   
  c second_type;
BEGIN
  a := second_type(first_type(1));
  c := second_type(first_type(2,3));
  a(1) := c(1);
  raise notice '%,%', a(1)(1),a(1)(2);
END;
/

DECLARE
  TYPE first_type IS VARRAY(3) OF INT;
  TYPE second_type IS VARRAY(3) OF first_type;
  TYPE third_type IS VARRAY(3) OF second_type;
  a second_type;
  b first_type := first_type(10);   
  c second_type;
  d third_type;
  f third_type;
BEGIN
  a := second_type(first_type(1,1));
  c := second_type(first_type(2,3));
  d := third_type(second_type(first_type(11,22)));
  f := third_type(second_type(first_type(11,22)),second_type(first_type(33,44)));
  d(1) := c;
  d(1) := a;
  d(1) := a;
  d := f;
  c := f(2);
  c := f(1);
  raise notice '%,%,%,%', d(1)(1)(1),d(1)(1)(2),d(2)(1)(1),d(2)(1)(2);
  raise notice '%,%',c(1)(1),c(1)(2);
END;
/

--nested table of nested table
DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a second_type;
  b first_type := first_type(10);   
  c second_type;
  d third_type;
  f third_type;
BEGIN
  a := second_type(first_type(1,1));
  c := second_type(first_type(2,3));
  d := third_type(second_type(first_type(11,22)));
  f := third_type(second_type(first_type(11,22)),second_type(first_type(33,44)));
  d(1) := c;
  d(1) := a;
  d(1) := a;
  d := f;
  c := f(2);
  c := f(1);
  raise notice '%,%,%,%', d(1)(1)(1),d(1)(1)(2),d(2)(1)(1),d(2)(1)(2);
  raise notice '%,%',c(1)(1),c(1)(2);
END;
/

DECLARE
  TYPE first_type IS TABLE OF int;
  TYPE second_type IS TABLE OF first_type;
  a second_type;
  b first_type := first_type(10);
  c second_type;
  d second_type;
BEGIN
  a := second_type(first_type(1), first_type(2), first_type(3));
  c := second_type(first_type(21), first_type(22));
  d := second_type(first_type(31));
  a(1)(1) := 11;
  a(1)(1) := b(1);
  a(1)(1) := c(1)(1);
  d := a; 
  raise notice '%,%,%', a(1)(1),a(2)(1),a(3)(1); 
  raise notice '%,%,%', d(1)(1),d(2)(1),d(3)(1);
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a second_type;
  b first_type := first_type(10,11);    
  c second_type;
BEGIN
  a := second_type(first_type(1), first_type(2), first_type(3));
  c := second_type(first_type(21), first_type(22));
  a(1)(1) := 22;
  a(1)(1) := c(1)(1);
  a(1) := b;
  raise notice '%,%', a(1)(1),a(1)(2);
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  a second_type;
  b first_type := first_type(10);   
  c second_type;
BEGIN
  a := second_type(first_type(1));
  c := second_type(first_type(2,3));
  a(1) := c(1);
  raise notice '%,%', a(1)(1),a(1)(2);
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  a second_type;
  b first_type := first_type(10);   
  c second_type;
  d third_type;
BEGIN
  a := second_type(first_type(1,1));
  c := second_type(first_type(2,3));
  d := third_type(second_type(first_type(11,22)));
  d(1) := c;
  d(1) := a;
  d(1) := a;
  raise notice '%,%', d(1)(1)(1),d(1)(1)(2);
END;
/


--associate assignment
DECLARE
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  a1 first_type; 
  a2 first_type;    
  b second_type;
  c1 third_type;
BEGIN
  a1(1) := 1;
  a1(2) := 2;
  a1(3) := 0;
  a2(1) := 11;
  b(1) := a1;
  raise notice '%,%,%', b(1)(1),b(1)(2),b(1)(3);
  b(1) := a2;
  raise notice '%', b(1)(1); 
  b(1)(1) := 22;
  raise notice '%', b(1)(1); 
  c1(1)(2) := a1;
  raise notice '%,%,%',c1(1)(2)(1),c1(1)(2)(2),c1(1)(2)(3);
  c1(1)(2)(3) := 10;
  raise notice '%,%,%',c1(1)(2)(1),c1(1)(2)(2),c1(1)(2)(3);
  a1 := b(1);
  raise notice '%',a1(1);
END;
/

--mult-dim combination
DECLARE
  TYPE first_type IS TABLE OF INT; 
  TYPE second_type IS TABLE OF first_type INDEX BY VARCHAR(10); 
  TYPE third_type IS VARRAY(10) OF second_type;
  TYPE fourth_type IS TABLE OF third_type;
  a first_type := first_type();
  b second_type;
  b2 second_type;
  c third_type := third_type();
  d fourth_type := fourth_type();
BEGIN
  a := first_type(1,2,3);
     raise notice 'a is %,%,%', a(1),a(2),a(3);
  b('Tom') := a;
  raise notice 'b is %,%,%', b('Tom')(1),b('Tom')(2),b('Tom')(3);
  b2('Kim') := first_type(11,22,33);
  raise notice 'b2 is %,%,%', b2('Kim')(1),b2('Kim')(2),b2('Kim')(3);
  c := third_type(b, b2);
  raise notice 'c(1) is %,%,%', c(1)('Tom')(1),c(1)('Tom')(2),c(1)('Tom')(3);
  raise notice 'c(2) is %,%,%', c(2)('Kim')(1),c(2)('Kim')(2),c(2)('Kim')(3);
END;	
/

DECLARE
  TYPE first_type IS TABLE OF INT ; 
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1);
  b second_type;
BEGIN
  b := second_type(first_type(111),a);
  raise notice '%,%', b(1)(1),b(2)(1); 
END;
/

DECLARE
  TYPE first_type IS TABLE OF INT ; 
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1);
  b1 second_type;
  b2 second_type;
  c second_type := second_type(first_type(222));
  a2 first_type := first_type(2);
BEGIN
  b1 := second_type(a,first_type(111),c(1),a2);
  b2 := second_type(first_type(111),a,c(1),a2);
  raise notice '%,%,%,%', b1(1)(1),b1(2)(1),b1(3)(1),b1(4)(1);  
  raise notice '%,%,%,%', b2(1)(1),b2(2)(1),b2(3)(1),b2(4)(1);  	
  END
/

DECLARE
  TYPE first_type IS TABLE OF INT; 
  TYPE second_type IS VARRAY(3 )OF first_type;
  TYPE third_type IS TABLE OF second_type INDEX BY VARCHAR(10);
  TYPE fourth_type IS TABLE OF third_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type := second_type(first_type());
  c third_type;
  d fourth_type := fourth_type(c);
BEGIN
  a := first_type(1,2,3);
  raise notice 'a is %,%,%', a(1),a(2),a(3);
  b := second_type(a);
  raise notice 'b is %,%,%', b(1)(1),b(1)(2),b(1)(3);
  a2 := first_type(11,22,33);
  b2(1) := a2;
  raise notice 'b2 is %,%,%', b2(1)(1),b2(1)(2),b2(1)(3);
  c('Tom') := b;
  c('Kim') := b2;
  raise notice 'c(1) is %,%,%', c('Tom')(1)(1),c('Tom')(1)(2),c('Tom')(1)(3);
  raise notice 'c(2) is %,%,%', c('Kim')(1)(1),c('Kim')(1)(2),c('Kim')(1)(3);
  d(1) := c;
  raise notice 'd(1) is %,%,%', d(1)('Tom')(1)(1),d(1)('Tom')(1)(2),d(1)('Tom')(1)(3);
  raise notice 'd(1) is %,%,%', d(1)('Kim')(1)(1),d(1)('Kim')(1)(2),d(1)('Kim')(1)(3);
END;
/

CREATE OR REPLACE FUNCTION f1(i int) return int AS
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1);
BEGIN
  a(1) := 11;
  a(1) := 22;
  return a(1);
END;
/
SELECT f1(1);
DROP FUNCTION f1(int);

CREATE OR REPLACE FUNCTION f1(i int) return int AS 
DECLARE
  TYPE first_type IS TABLE OF INT; 
  TYPE second_type IS VARRAY(3 )OF first_type; 
  TYPE third_type IS TABLE OF second_type INDEX BY VARCHAR(10); 
  TYPE fourth_type IS TABLE OF third_type;
  a first_type;
  a2 first_type;
  b second_type;
  b2 second_type := second_type(first_type());
  c third_type;
  d fourth_type := fourth_type(c);
BEGIN
  a := first_type(1,2,3);
  raise notice 'a is %,%,%', a(1),a(2),a(3);
  b := second_type(a);
  raise notice 'b is %,%,%', b(1)(1),b(1)(2),b(1)(3);
  a2 := first_type(11,22,33);
  b2(1) := a2;
  raise notice 'b2 is %,%,%', b2(1)(1),b2(1)(2),b2(1)(3);
  c('Tom') := b;
  c('Kim') := b2;
  raise notice 'c(1) is %,%,%', c('Tom')(1)(1),c('Tom')(1)(2),c('Tom')(1)(3);
  raise notice 'c(2) is %,%,%', c('Kim')(1)(1),c('Kim')(1)(2),c('Kim')(1)(3);
  d(1) := c;
  raise notice 'd(1) is %,%,%', d(1)('Tom')(1)(1),d(1)('Tom')(1)(2),d(1)('Tom')(1)(3);
  raise notice 'd(1) is %,%,%', d(1)('Kim')(1)(1),d(1)('Kim')(1)(2),d(1)('Kim')(1)(3);
  return 1;
END;
/

SELECT f1(1);
DROP FUNCTION f1(int);

--5 the collection in package
--declare a associate array in a package
CREATE OR REPLACE PACKAGE pkg1 AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
  FUNCTION f1(id int) RETURN INT;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg1 AS
  FUNCTION f1(id int) RETURN INT AS
  DECLARE
    v11 pkg1.first_type;
	v12 pkg1.first_type;
	v3 third_type;
  BEGIN
    v11(1) := 100;
	v11(2) := 200;
	v11(3) := 300;
	v12 := v11;
	v12.delete(1);
	v3(1)(2)(3) := 10;
	raise notice 'v3(1)(2)(3) = %', v3(1)(2)(3);
	raise notice 'v11.count = %', v11.count;
	raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
	raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
	raise notice 'v3(1)(2).exists(3) = %', v3(1)(2).exists(3);
	return  v11(1) + v12.count;
  END;
END;
/

SELECT pkg1.f1(1);
DROP PACKAGE pkg1;

--declare a varray in a package
CREATE OR REPLACE PACKAGE pkg3 AS
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(20) OF second_type;
  FUNCTION f1(id int) RETURN INT;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg3 AS
  FUNCTION f1(id int) RETURN INT AS
  DECLARE
    v11 pkg3.first_type;
	v12 pkg3.first_type;
	v3 pkg3.third_type;
  BEGIN
    v11 := pkg3.first_type(100, 200, 300);
	v12 := v11;
	v3 := third_type(second_type(first_type(10, 20)));
	raise notice 'v3(1)(1)(1) = %', v3(1)(1)(1);
	raise notice 'v11.count = %', v11.count;
	raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
	raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
	raise notice 'v3(1)(1).exists(1) = %', v3(1)(1).exists(1);
	return  v11(1) + v12.count;
  END;
END;
/

SELECT pkg3.f1(1);
DROP PACKAGE pkg3;


--declare a nested table in a package
CREATE OR REPLACE PACKAGE pkg3 AS
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
  FUNCTION f1(id int) RETURN INT;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg3 AS
  FUNCTION f1(id int) RETURN INT AS
  DECLARE
    v11 pkg3.first_type;
	v12 pkg3.first_type;
	v3 pkg3.third_type;
  BEGIN
    v11 := pkg3.first_type(100, 200, 300);
	v12 := v11;
	v3 := third_type(second_type(first_type(10, 20)));
	raise notice 'v3(1)(1)(1) = %', v3(1)(1)(1);
	raise notice 'v11.count = %', v11.count;
	raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
	raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
	raise notice 'v3(1)(1).exists(1) = %', v3(1)(1).exists(1);
	return  v11(1) + v12.count;
  END;
END;
/

SELECT pkg3.f1(1);
DROP PACKAGE pkg3;


--the argument datatype of a package function is collection type
CREATE OR REPLACE PACKAGE pkg4 AS
  TYPE first_type IS TABLE OF INT;
  FUNCTION f1(c1 OUT first_type) RETURN INT;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg4 AS
  FUNCTION f1(c1 OUT first_type) RETURN INT AS
  DECLARE
  BEGIN
    c1 := pkg4.first_type(100, 200, 300);
	return  c1.count;
  END;
END;
/

DECLARE
  x pkg4.first_type;
  res int;
BEGIN
  res := pkg4.f1(x);
  raise notice 'res = %, x(2) = %', res, x(2);
END;
/

DROP PACKAGE pkg4;

--declare a variable that datatyepe is associate array in package
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  TYPE second_type IS TABLE OF first_type INDEX BY INT;
  TYPE third_type IS TABLE OF second_type INDEX BY INT;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
    v11 pkg5.first_type;
	v12 pkg5.first_type;
	v3  pkg5.third_type;
BEGIN
    v11(1) := 100;
	v11(2) := 200;
	v11(3) := 300;
	v12 := v11;
	v12.delete(1);
	v3(1)(2)(3) := 10;
	raise notice 'v3(1)(2)(3) = %', v3(1)(2)(3);
	raise notice 'v11.count = %', v11.count;
	raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
	raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
	raise notice 'v3(1)(2).exists(3) = %', v3(1)(2).exists(3);
	return  v11(1) + v12.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);
DROP PACKAGE pkg5;


--declare a variable that datatyepe is nested table in package
CREATE OR REPLACE PACKAGE pkg6 AS
  TYPE first_type IS TABLE OF INT;
  TYPE second_type IS TABLE OF first_type;
  TYPE third_type IS TABLE OF second_type;
END;
/

CREATE OR REPLACE FUNCTION testf2(id int) RETURN INT AS
  v11 pkg6.first_type;
  v12 pkg6.first_type;
  v3 pkg6.third_type;
BEGIN
  v11 := pkg6.first_type(100, 200, 300);
  v12 := v11;
  v3 := pkg6.third_type(pkg6.second_type(pkg6.first_type(10, 20)));
  raise notice 'v3(1)(1)(1) = %', v3(1)(1)(1);
  raise notice 'v11.count = %', v11.count;
  raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
  raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
  raise notice 'v3(1)(1).exists(1) = %', v3(1)(1).exists(1);
  return  v11(1) + v12.count;
END;
/

SELECT testf2(1);
DROP FUNCTION testf2(id int);
DROP PACKAGE pkg6;


--declare a variable that datatyepe is varray in package
CREATE OR REPLACE PACKAGE pkg7 AS
  TYPE first_type IS VARRAY(10) OF INT;
  TYPE second_type IS VARRAY(20) OF first_type;
  TYPE third_type IS VARRAY(30) OF second_type;
END;
/

CREATE OR REPLACE FUNCTION testf2(id int) RETURN INT AS
  v11 pkg7.first_type;
  v12 pkg7.first_type;
  v3 pkg7.third_type;
BEGIN
  v11 := pkg7.first_type(100, 200, 300);
  v12 := v11;
  v3 := pkg7.third_type(pkg7.second_type(pkg7.first_type(10, 20)));
  raise notice 'v3(1)(1)(1) = %', v3(1)(1)(1);
  raise notice 'v11.count = %', v11.count;
  raise notice 'v12.first = %, v12.last = %', v12.first, v12.last;
  raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
  raise notice 'v3(1)(1).exists(1) = %', v3(1)(1).exists(1);
  raise notice 'v3(1).limit = %', v3(1).limit;
  return  v11(1) + v12.count;
END;
/
SELECT testf2(1);
DROP FUNCTION testf2(id int);
DROP PACKAGE pkg7;


--declare a variable that datatyepe is package.associatearray%TYPE
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
  v11 pkg5.pvar%TYPE;
BEGIN
  v11(1) := 100;
  v11(2) := 200;
  v11(3) := 300;
  raise notice 'v11.count = %', v11.count;
  raise notice 'v11.first = %, v11.last = %', v11.first, v11.last;
  raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
  return  v11(1) + v11.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);

DECLARE
  v1 pkg5.pvar%TYPE;
  v2 pkg5.first_type;
BEGIN
  v1(1) := 100;
  v1(2) := 200;
  v1(3) := 300;
  v2 := v1;
  raise notice 'v2(2) = %', v2(2);
END;
/

DROP PACKAGE pkg5;


--declare a variable that datatyepe is package.nestedtable%TYPE
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS TABLE OF INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
  v11 pkg5.pvar%TYPE;
BEGIN
  v11 := pkg5.first_type(100, 200, 300);
  raise notice 'v11.count = %', v11.count;
  raise notice 'v11.first = %, v11.last = %', v11.first, v11.last;
  raise notice 'v11.prior(2) = %, v11.next(2) = %', v11.prior(2), v11.next(2);
  return  v11(1) + v11.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);

DECLARE
  v1 pkg5.pvar%TYPE;
  v2 pkg5.first_type;
BEGIN
  v1 := pkg5.first_type(100, 200, 300);
  v2 := v1;
  raise notice 'v2(2) = %', v2(2);
END;
/

DROP PACKAGE pkg5;

--declare a package global associate array variable
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
BEGIN
    pkg5.pvar(1) := 100;
	pkg5.pvar(2) := 200;
	pkg5.pvar(3) := 300;
	raise notice 'pkg5.pvar.count = %', pkg5.pvar.count;
	raise notice 'pkg5.pvar.first = %, pkg5.pvar.last = %', pkg5.pvar.first, pkg5.pvar.last;
	raise notice 'pkg5.pvar.prior(2) = %, pkg5.pvar.next(2) = %', pkg5.pvar.prior(2), pkg5.pvar.next(2);
	return  pkg5.pvar(1) + pkg5.pvar.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);
DROP PACKAGE pkg5;


--declare a package global nested table variable
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS TABLE OF INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
BEGIN
    pkg5.pvar := pkg5.first_type(100, 200, 300);
	raise notice 'pkg5.pvar.count = %', pkg5.pvar.count;
	raise notice 'pkg5.pvar.first = %, pkg5.pvar.last = %', pkg5.pvar.first, pkg5.pvar.last;
	raise notice 'pkg5.pvar.prior(2) = %, pkg5.pvar.next(2) = %', pkg5.pvar.prior(2), pkg5.pvar.next(2);
	return  pkg5.pvar(1) + pkg5.pvar.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);
DROP PACKAGE pkg5;

--declare a package global varray variable
CREATE OR REPLACE PACKAGE pkg5 AS
  TYPE first_type IS VARRAY(20) OF INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int) RETURN INT AS
BEGIN
    pkg5.pvar := pkg5.first_type(100, 200, 300);
	raise notice 'pkg5.pvar.count = %', pkg5.pvar.count;
	raise notice 'pkg5.pvar.first = %, pkg5.pvar.last = %', pkg5.pvar.first, pkg5.pvar.last;
	raise notice 'pkg5.pvar.prior(2) = %, pkg5.pvar.next(2) = %', pkg5.pvar.prior(2), pkg5.pvar.next(2);
	raise notice 'pkg5.pvar.limit = %', pkg5.pvar.limit;
	return  pkg5.pvar(1) + pkg5.pvar.count;
END;
/

SELECT testf(1);
DROP FUNCTION testf(id int);
DROP PACKAGE pkg5;


--the datatype of a normal function argument and return are collection type
CREATE OR REPLACE PACKAGE pkg AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
END;
/

CREATE OR REPLACE FUNCTION testf(id int, nt pkg.first_type) RETURN pkg.first_type AS
  v1 pkg.first_type;
BEGIN
  v1 := nt;
  return v1;
END;
/

DECLARE
  res pkg.first_type;
  a pkg.first_type;
BEGIN
  a(1) := 10;
  a(2) := 20;
  a(3) := 30;
  res := testf(1, a);
  raise notice 'res.count = %', res.count;
  raise notice 'res.first = %, res.last = %', res.first, res.last;
  raise notice 'res.prior(2) = %, res.next(2) = %', res.prior(2), res.next(2);
  raise notice 'a(2) = %', a(2);
END;
/

DROP FUNCTION testf(int, pkg.first_type);

CREATE OR REPLACE FUNCTION testf2(id int, nt pkg.first_type, nt2 OUT pkg.first_type) RETURN pkg.first_type AS
  v1 pkg.first_type;
BEGIN
  v1 := nt;
  nt2(1) := nt(1) + 1;
  nt2(2) := nt(2) + 1;
  return v1;
END;
/

DECLARE
  res pkg.first_type;
  a pkg.first_type;
  b a%TYPE;
BEGIN
  a(1) := 10;
  a(2) := 20;
  a(3) := 30;
  res := testf2(1, a, b);
  raise notice 'res.count = %', res.count;
  raise notice 'res.first = %, res.last = %', res.first, res.last;
  raise notice 'res.prior(2) = %, res.next(2) = %', res.prior(2), res.next(2);
  raise notice 'a(2) = %', a(2);
  raise notice 'b.count = %', b.count;
  raise notice 'b.prior(2) = %, b.next(2) = %', b.prior(2), b.next(2);
  FOR i in  b.first .. b.last LOOP
    raise notice 'b(%) = %', i, b(i);
  END LOOP;
END;
/

DROP FUNCTION testf2(int, pkg.first_type, pkg.first_type);
DROP PACKAGE pkg;


--the datatype of a normal function argument and return are package.collection_var%TYPE
CREATE OR REPLACE PACKAGE pkg AS
  TYPE first_type IS TABLE OF INT INDEX BY INT;
  pvar first_type;
END;
/

CREATE OR REPLACE FUNCTION testf(id int, nt pkg.pvar%TYPE) RETURN pkg.pvar%TYPE AS
  v1 pkg.first_type;
BEGIN
  v1 := nt;
  return v1;
END;
/

DECLARE
  res pkg.first_type;
  a pkg.first_type;
BEGIN
  a(1) := 10;
  a(2) := 20;
  a(3) := 30;
  res := testf(1, a);
  raise notice 'res.count = %', res.count;
  raise notice 'res.first = %, res.last = %', res.first, res.last;
  raise notice 'res.prior(2) = %, res.next(2) = %', res.prior(2), res.next(2);
  raise notice 'a(2) = %', a(2);
END;
/

DROP FUNCTION testf(int, pkg.first_type);
DROP PACKAGE pkg;


--6 the element of the collection is record
DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  TYPE population IS TABLE OF myrecord INDEX BY VARCHAR2(64);
  r myrecord;
  a population;
BEGIN
  r.id := 11;
  r.name := 'new';
  a(1) := r;
  raise notice 'a(1).id = %, a(1).name = %', a(1).id, a(1).name;
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  TYPE population IS TABLE OF myrecord INDEX BY VARCHAR2(64);
  r myrecord;
  a population;
  a2 population;
BEGIN
  r.id := 11;
  r.name := 'new';
  a(1) := r;
  a2 := a;
  raise notice 'a2(1).id = %, a2(1).name = %', a2(1).id, a2(1).name;
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  TYPE population IS TABLE OF myrecord;
  r myrecord;
  r2 myrecord;
  a population;
BEGIN
  r.id := 11;
  r.name := 'new';
  r2.id := 12;
  r2.name := 'new2';
  a := population(r, r2);
  raise notice 'a(2).id = %, a(2).name = %', a(2).id, a(2).name;
  a(2).id := 100;
  a(2).name := 'new100';
  raise notice 'a(2).id = %, a(2).name = %', a(2).id, a(2).name;
END;
/


DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  TYPE population IS VARRAY(10) OF myrecord;
  r myrecord;
  r2 myrecord;
  a population;
BEGIN
  r.id := 11;
  r.name := 'new';
  r2.id := 12;
  r2.name := 'new2';
  a := population(r, r2);
  raise notice 'a(2).id = %, a(2).name = %', a(2).id, a(2).name;
  raise notice 'a.count = %', a.count;
  raise notice 'a.first = %, a.last = %', a.first, a.last;
  raise notice 'a.prior(2) = %, a.next(2) = %', a.prior(2), a.next(2);
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  TYPE population IS VARRAY(10) OF myrecord;
  TYPE population2 IS TABLE OF population;
  TYPE population3 IS TABLE OF population2 INDEX BY INT;
  r myrecord;
  r2 myrecord;
  a population;
  b population2;
  c population3;
BEGIN
  r.id := 11;
  r.name := 'new';
  r2.id := 12;
  r2.name := 'new2';
  a := population(r, r2);
  b := population2(a);
  c(2) := b;
  raise notice 'c(2)(1)(2).id = %, c(2)(1)(2).name = %', c(2)(1)(2).id, c(2)(1)(2).name;
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT);
  TYPE population IS TABLE OF myrecord INDEX BY INT;
  TYPE myrecord2 IS RECORD(i population);
  TYPE population2 IS TABLE OF myrecord2 INDEX BY INT;
  r myrecord;
  a population;
  r2 myrecord2;
  b population2;
BEGIN
  r.id := 11;
  a(2) := r;
  r2.i := a;
  b(1) := r2;
  raise notice 'b(1).i(2).id = %', b(1).i(2).id;
  b(1).i(2).id := 100;
  raise notice 'b(1).i(2).id = %', b(1).i(2).id;
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT);
  TYPE population IS TABLE OF myrecord;
  TYPE myrecord2 IS RECORD(i population);
  TYPE population2 IS VARRAY(20) OF myrecord2;
  r myrecord;
  a population;
  r2 myrecord2;
  b population2;
BEGIN
  r.id := 11;
  a := population(r, r);
  r2.i := a;
  b := population2(r2, r2);
  raise notice 'b(1).i(2).id = %', b(1).i(2).id;
  b(1).i(2).id := 100;
  raise notice 'b(1).i(2).id = %', b(1).i(2).id;
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT);
  TYPE population IS TABLE OF myrecord;
  TYPE myrecord2 IS RECORD(i population);
  TYPE population2 IS VARRAY(20) OF myrecord2;
  r myrecord;
  a population;
  r2 myrecord2;
  b population2;
BEGIN
  r.id := 11;
  a := population(r, r);
  r2.i := a;
  b := population2(r2, r2);
  raise notice 'b(1).i.count = %', b(1).i.count;
  raise notice 'b(1).i.next(1) = %', b(1).i.next(1);
END;
/

DECLARE
  TYPE record_type IS RECORD(i int, j int); 
  TYPE mytype IS TABLE OF record_type;
  rec record_type;
  a mytype;
  j int;
BEGIN
  rec.i := 1;
  rec.j := 2;
  a := mytype();
  a.extend(3);
  a(2) := rec;
  j := a(2).i;
  raise notice 'j = %', j;
END;
/

DECLARE
  TYPE record_type IS RECORD(i int, j int); 
  TYPE mytype IS TABLE OF record_type;
  rec record_type;
  a mytype;
  j int;
BEGIN
  rec.i := 1;
  rec.j := 2;
  a := mytype(rec);
  a.extend(3,1);
  j := a(2).i;
  raise notice '%', j;
END;
/

DECLARE
  TYPE record_type IS RECORD(i int, j int);  
  TYPE mytype IS VARRAY(5) OF record_type;
  rec record_type;
  a mytype;
  j int;
BEGIN
  rec.i := 1;
  rec.j := 2;
  a := mytype(rec);
  a.extend(3,1);
  j := a(2).i;
  raise notice '%', j;
END;
/

DELETE FROM  t1;
DECLARE
  TYPE record_type IS RECORD(i int, j int); 
  TYPE mytype IS VARRAY(5) OF record_type;
  rec record_type;
  a mytype;
BEGIN
  a := mytype(rec);
  raise notice '%,%', a(1).i,a(1).j;
END;
/

CREATE OR REPLACE PACKAGE pkg_nest is
  TYPE first_type IS TABLE OF INT index by INT;
  TYPE second_type IS TABLE OF first_type index by varchar(20);
  TYPE myrecord IS RECORD(i1 second_type);
  TYPE myrecord2 IS RECORD(c1 myrecord);
  v_arry_req second_type;
  r myrecord2;
END;
/

DECLARE
  v1 varchar2(20) := 'one';
  v3 integer := 10;
BEGIN
  pkg_nest.r.c1.i1(v1)(v3) := 11;
  raise notice 'pkg_nest.r.c1.i1(v1)(v3) = %', pkg_nest.r.c1.i1(v1)(v3);
END;
/
DROP PACKAGE pkg_nest;


CREATE OR REPLACE PACKAGE pkg_collect is
  TYPE first_type IS TABLE OF INT index by integer;
  TYPE second_type IS TABLE OF first_type index by varchar(20);
  type myrecord is record(i1 second_type);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  r myrecord2;
END;
/

DECLARE
  v1 varchar2(20) := 'one';
  v3 integer := 10;
BEGIN
  pkg_collect.r.c1.i1(v1)(v3) := 11;
  raise notice 'pkg_collect.r.c1.i1(v1)(v3) = %', pkg_collect.r.c1.i1(v1)(v3);
END;
/
DROP PACKAGE pkg_collect;

CREATE OR REPLACE PACKAGE pkg_99 IS
  type t_record_req is record( session_id NUMBER);
  TYPE t_req IS TABLE OF t_record_req INDEX BY INTEGER;
  v_arry_req t_req;
END;
/

DECLARE
  reqid                    NUMBER := 10;
  l_session_id             NUMBER;
BEGIN
  l_session_id := 4;
  pkg_99.v_arry_req(l_session_id).session_id := reqid;
  raise notice 'pkg_99.v_arry_req(l_session_id).session_id = %', pkg_99.v_arry_req(l_session_id).session_id;
END;
/

--use a package collection var in another package
CREATE OR REPLACE PACKAGE p2 IS
  procedure pp1(c int);
END;
/

CREATE OR REPLACE PACKAGE BODY p2 IS
  procedure pp1(c int) as
    reqid                    NUMBER := 10;
    l_session_id             NUMBER;
  BEGIN
    l_session_id := 4;
    pkg_99.v_arry_req(l_session_id).session_id := reqid;
	raise notice 'pkg_99.v_arry_req(l_session_id).session_id = %', pkg_99.v_arry_req(l_session_id).session_id;
  END;
END;
/
call p2.pp1(1);
call p2.pp1(1);
DROP PACKAGE p2;
DROP PACKAGE pkg_99;



CREATE OR REPLACE PACKAGE p1 as
  type src_rec is record(
    has_error           boolean,
    name                varchar2(25));
  type src_tab is table of src_rec index by integer;
  type ez_control is record(
    src_cnt             number(15),
    sources             src_tab
 );
  FUNCTION get_src(p_control in out ez_control) RETURN boolean;
END;
/

CREATE OR REPLACE PACKAGE BODY p1 as
  FUNCTION get_src(p_control in out ez_control) RETURN boolean is
    l_tempbuf varchar(100);
	n1 int := 2;
  BEGIN
    l_tempbuf := p_control.sources(n1).name;
    RETURN true;
  END;
end;
/

CREATE OR REPLACE PACKAGE BODY p1 as
  FUNCTION get_src(p_control in out ez_control) RETURN boolean is
    l_tempbuf varchar(100);
	n1 int := 2;
  BEGIN
   l_tempbuf := '(''' || p_control.sources(n1).name || p_control.sources(n1).name || ')''';
   raise notice 'l_tempbuf = %', l_tempbuf;
   RETURN true;
  END;
END;
/

DECLARE
  r1 p1.src_rec;
  r2 p1.ez_control;
  res boolean;
  a1 p1.src_tab;
BEGIN
  r1.name := 'ori';
  a1(2) := r1;
  r2.sources := a1;
  res := p1.get_src(r2);
END;
/
DROP PACKAGE p1;

create or replace package test_pkg is
  type batch_rec is record(id integer,name varchar2(256));
  type batch_tab is table of batch_rec index by integer;
  type src_rec is record(id integer,batches batch_tab);
  type src_tab is table of src_rec index by integer;
  type ez_control is record(id integer,sources src_tab);
  procedure test_main(id integer);
  function get_j1(p_control in out ez_control) return varchar2;
end;
/
create or replace package body test_pkg is
  procedure test_main(id integer) is
     ret varchar2(256);
     l_control ez_control;
  begin
     l_control.id := 1;
     ret := get_j1(l_control);
  end;
  function get_j1(p_control in out ez_control) return varchar2 is
    ret varchar2(256);
    p_src_num integer;
    p_bch_num integer;
    p_hd_num integer;
    function laid(p_amt_conv in boolean,
                  p_src_num out integer,
      p_bch_num out integer,
      p_hd_num out integer) return varchar2 is
      src_num integer := 1;
      bch_num integer := 2;
    begin
      p_control.sources(src_num).batches(bch_num).id := 100;
      p_control.sources(src_num).batches(bch_num).name := 'ok';
      raise info '%, %',p_control.sources(1).batches(2).id, p_control.sources(src_num).batches(bch_num).name;
      return 'ok';
    end;
   begin
      ret := laid(true,p_src_num,p_bch_num,p_hd_num);
      return ret;
   end;
end;
/

declare
begin
  test_pkg.test_main(23);
end;
/
drop package test_pkg;


--7 the element of the collection is %rowtype
INSERT INTO t1 VALUES(1, 11);
INSERT INTO t1 VALUES(2, 22);

--nested table
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a := mytype(row1);
  raise notice '%,%', a(1).i, a(1).j;
  a(1) := row1;
  raise notice '%,%', a(1).i, a(1).j; 
END;
/

--two collection variable assignment
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS VARRAY(20) OF t1%ROWTYPE;
  a mytype;
  b mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a := mytype(row1);
  b := a;
  raise notice '%,%', b(1).i, b(1).j; 
END;
/

--row.field assignment
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
  j int;
BEGIN
  select * from t1 where i = 1 into row1;
  a := mytype(row1);
  a(1).i := 11;
  j := a(1).i;
  raise notice '%, %', j, a(1).i;
END;
/

--test extend method
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
  j int;
BEGIN
  select * from t1 where i = 1 into row1;
  a := mytype();
  a.extend(3);
  raise notice 'a(2).i = %', a(2).i;
  a(2) := row1;
  j := a(2).i;
  raise notice 'j = %', j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
  j int;
BEGIN
  select * from t1 where i = 1 into row1;
  a := mytype(row1);
  a.extend(3,1);
  j := a(2).i;
  raise notice 'j = %', j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1 ;
  a := mytype(row1);
  a(1).i := 10;
  row1.i := a(1).i;
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
  raise notice 'row1.i = %, row1.i = %', row1.i, row1.j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  row2 t1%ROWTYPE;
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1 ;
  a := mytype(row1);
  row2 := a(1);
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
  raise notice 'row2.i = %, row2.j = %', row2.i, row2.j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
BEGIN
  row1.i := 1;
  row1.j := 2;
  a := mytype(row1);
  row1 := a(1);
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
  raise notice 'row1.i = %, row1.i = %', row1.i, row1.j;
END;
/

DECLARE
  row1 t1%ROWTYPE;
  TYPE mytype IS TABLE OF t1%ROWTYPE;
  a mytype;
BEGIN
  a := mytype(row1);
  raise notice '%', a(1).i;
END;
/

--varray
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS VARRAY(5) OF t1%ROWTYPE;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a := mytype(row1);
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
  a(1) := row1;
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j; 
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS VARRAY(5) OF t1%ROWTYPE;
  a mytype;
  b mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a := mytype(row1);
  b := a;
  raise notice 'b(1).i = %, b(1).j = %', b(1).i, b(1).j; 
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS VARRAY(5) OF t1%ROWTYPE;
  a mytype;
  j int;
BEGIN
  select * from t1 where i = 1 into row1;
  a := mytype(row1);
  a.extend(3,1);
  j := a(2).i;
  raise notice 'j = %', j;
END;
/

--associate array
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a(1) := row1;
  raise notice '%,%', a(1).i,a(1).j;
END;
/

--two collection variable assignment
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
  b mytype;
BEGIN
  select * into row1 from t1 where i = 1;
  a(1) := row1; 
  b := a;
  raise notice '%,%', b(1).i,b(1).j; 
END;
/

--row.field assignment
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
  j int;
BEGIN
  select * from t1 where i = 1 into row1;
  a(1) := row1;
  a(1).i := 11;
  j := a(1).i;
  raise notice '%,%', j,a(1).i;
END;
/

--collect vaiable is assigned to row and row.field
DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1 ;
  a(1) := row1;
  a(1).i := 11;
  row1.i := a(1).i;
  raise notice '%,%', a(1).i,a(1).j;
  raise notice '%,%', row1.i,row1.j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  row2 t1%ROWTYPE;
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  select * into row1 from t1 where i = 1 ;
  a(1) := row1;
  row2 := a(1);
  raise notice '%,%', a(1).i,a(1).j;
  raise notice '%,%', row2.i,row2.j;
END;
/

DECLARE
  row1 t1%ROWTYPE; 
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  row1.i := 1;
  row1.j := 2;
  a(1) := row1;
  row1 := a(1);
  raise notice '%,%', a(1).i,a(1).j;
  raise notice '%,%', row1.i,row1.j;
END;
/

DECLARE
  row1 t1%ROWTYPE;
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  a(1) := row1;
  raise notice '%', a(1).i;
END;
/

DECLARE
  TYPE myrecord IS RECORD(m t1%ROWTYPE);
  TYPE mytype IS TABLE OF myrecord INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype2;
BEGIN
  select i, j into a(1)(2).m from t1 where i = 1 ;
  raise notice 'a(1)(2).m.i = %, a(1)(2).m.j = %', a(1)(2).m.i, a(1)(2).m.j;
END;
/

DECLARE
  TYPE myrecord IS RECORD(m t1%ROWTYPE);
  TYPE mytype IS TABLE OF myrecord INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype2;
BEGIN
  select i, j into a(1)(2).m from t1 where i = 1 ;
  raise notice 'a(1)(2).m.i = %, a(1)(2).m.j = %', a(1)(2).m.i, a(1)(2).m.j;
  select i, j into a(1)(2).m from t1 where i = 1 ;
  raise notice 'a(1)(2).m.i = %, a(1)(2).m.j = %', a(1)(2).m.i, a(1)(2).m.j;
END;
/


--8 the element of the collection is cursor
CREATE table ta(id NUMBER(6), first_names VARCHAR2(20), salary NUMBER);
INSERT INTO ta VALUES(1,'mary',1000);
INSERT INTO ta VALUES(2,'lily',2000);

DECLARE
  CURSOR c2 IS SELECT id, first_names, salary FROM ta WHERE salary = 2000;
  CURSOR c1 RETURN ta%ROWTYPE IS SELECT * FROM ta WHERE id = 1;
  TYPE reclist IS TABLE OF c1%ROWTYPE INDEX BY INT;
  TYPE reclist2 IS TABLE OF c2%ROWTYPE INDEX BY INT;
  recs reclist;
  recs2 reclist ;
BEGIN
  OPEN c1 ;
  FETCH c1 INTO recs(1);
  CLOSE c1;
  FOR i IN recs.FIRST .. recs.LAST LOOP
    raise notice 'Id = %,first_names = %,salary = %',recs(i).id, recs(i).first_names, recs(i).salary;
  END LOOP;
  OPEN c2;
  FETCH c2 INTO recs2(1);
  CLOSE c2;
  --recs2 := recs;
  FOR i IN recs2.FIRST .. recs2.LAST LOOP
    raise notice 'Id = %,first_names = %,salary = %',recs2(i).id, recs2(i).first_names, recs2(i).salary;
  END LOOP;
END;
/

DECLARE
  CURSOR c2 IS SELECT id, first_names, salary FROM ta WHERE salary = 2000;
  CURSOR c1 RETURN ta%ROWTYPE IS SELECT * FROM ta WHERE id = 1;
  TYPE reclist IS TABLE OF c1%ROWTYPE INDEX BY INT;
  TYPE reclist2 IS TABLE OF c2%ROWTYPE INDEX BY INT;
  recs reclist;
  recs2 reclist ;
BEGIN
  OPEN c1 ;
  FETCH c1 INTO recs(1).id, recs(1).first_names, recs(1).salary;
  CLOSE c1;
  FOR i IN recs.FIRST .. recs.LAST LOOP
    raise notice 'Id = %,first_names = %,salary = %',recs(i).id, recs(i).first_names, recs(i).salary;
  END LOOP;
  OPEN c2;
  FETCH c2 INTO recs2(1).id, recs2(1).first_names, recs2(1).salary;
  CLOSE c2;
  FOR i IN recs2.FIRST .. recs2.LAST LOOP
    raise notice 'Id = %,first_names = %,salary = %',recs2(i).id, recs2(i).first_names, recs2(i).salary;
  END LOOP;
END;
/
DROP TABLE ta;


CREATE OR REPLACE PACKAGE pkg2 is
  TYPE second_type IS TABLE OF int;
  type myrecord is record(i1 second_type);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  r myrecord2;
  function f1(col out myrecord2) return int;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg2 is
  function f1(col  out myrecord2) return int as
  begin
      col.c1.i1 := pkg2.second_type(11,22);
      raise notice 'col.c1.i1.COUNT IS %', col.c1.i1.COUNT;
    return 1;
  end;
END;
/
DECLARE
  a pkg2.second_type := pkg2.second_type();
  res int;
BEGIN
  pkg2.r.c1.i1 := a;
  res := pkg2.f1(pkg2.r);
  raise notice 'pkg2.r.c1.i1.COUNT IS %', pkg2.r.c1.i1.COUNT;
END;
/
DROP PACKAGE pkg2;

CREATE OR REPLACE PACKAGE pkg2 is
  TYPE second_type IS TABLE OF int;
  type myrecord is record(i1 second_type);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  r myrecord2;
  function f1(col out myrecord2) return int;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg2 is
  function f1(col  out myrecord2) return int as
  begin
      raise notice 'col.c1.i1(1) %', col.c1.i1(1);
    return 1;
  end;
END;
/
DECLARE
  a pkg2.second_type := pkg2.second_type(11,22);
  res int;
BEGIN
  pkg2.r.c1.i1 := a;
  res := pkg2.f1(pkg2.r);
END;
/
DROP PACKAGE pkg2;

DECLARE
  TYPE second_type IS TABLE OF int;
  type myrecord0 is record(j1 second_type);
  type myrecord is record(i1 myrecord0);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  col myrecord2;
BEGIN
   col.c1.i1.j1 := second_type(11,22, 33);  
   raise notice 'col.c1.i1.j1.COUNT IS %', col.c1.i1.j1.COUNT;
   for i in col.c1.i1.j1.first .. col.c1.i1.j1.last loop
     raise notice 'col.c1.i1.j1(%) IS %', i, col.c1.i1.j1(i);
   end loop;
   col.c1.i1.j1.delete(1);
   for i in col.c1.i1.j1.first .. col.c1.i1.j1.last loop
     raise notice 'after delete, col.c1.i1.j1(%) IS %', i, col.c1.i1.j1(i);
   end loop;
END;
/

CREATE OR REPLACE PACKAGE pkg3 is
  TYPE second_type IS TABLE OF int INDEX BY INT;
  type myrecord is record(i1 second_type);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  r myrecord2;
  function f1(col out myrecord2) return int;
END;
/
CREATE OR REPLACE PACKAGE BODY pkg3 is
  function f1(col  out myrecord2) return int as
  begin
      raise notice 'col.c1.i1.COUNT %', col.c1.i1.COUNT;
      col.c1.i1(1) := 11;
	  col.c1.i1(2) := 22;
      raise notice 'col.c1.i1.COUNT IS %', col.c1.i1.COUNT;
    return 1;
  end;
END;
/
DECLARE
  a pkg3.second_type;
  res int;
BEGIN
  a(1) := 100;
  pkg3.r.c1.i1 := a;
  res := pkg3.f1(pkg3.r);
  raise notice 'pkg3.r.c1.i1.COUNT IS %', pkg3.r.c1.i1.COUNT;
END;
/

DROP PACKAGE pkg3;


--9 SELECT INTO collection
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  select i, j into a(1), a(2) from t1 where i = 1;
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

DECLARE
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  select i, j into a(1) from t1 where i = 1;
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
END;
/


CREATE OR REPLACE PACKAGE pkg_99 IS
  type t_record_req is record(session_id NUMBER);
  TYPE t_req IS TABLE OF t_record_req INDEX BY INTEGER;
  v_arry_req t_req;
END;
/
DECLARE
  l_session_id             NUMBER;
BEGIN
  l_session_id := 4;
  select j into pkg_99.v_arry_req(l_session_id).session_id from t1 where i = 1;
  raise notice 'pkg_99.v_arry_req(l_session_id).session_id = %', pkg_99.v_arry_req(l_session_id).session_id;
END;
/
DROP PACKAGE pkg_99;


DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  select i, j into a(1), a(2) from t1 where i = 1;
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

DECLARE
  TYPE mytype IS VARRAY(10) OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  select i, j into a(1), a(2) from t1 where i = 1;
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

DECLARE
  TYPE mytype IS TABLE OF t1%ROWTYPE INDEX BY INT;
  a mytype;
BEGIN
  select i, j into a(1) from t1 where i = 1 ;
  raise notice 'a(1).i = %, a(1).j = %', a(1).i, a(1).j;
END;
/

DECLARE
  TYPE myrecord IS RECORD(m t1%ROWTYPE);
  TYPE mytype IS TABLE OF myrecord INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype2;
BEGIN
  select i, j into a(1)(2).m from t1 where i = 1 ;
  raise notice 'a(1)(2).m.i = %, a(1)(2).m.j = %', a(1)(2).m.i, a(1)(2).m.j;
END;
/ 

DECLARE
  TYPE myrecord IS RECORD(m t1%ROWTYPE);
  TYPE mytype IS TABLE OF myrecord INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype2;
BEGIN
  select i, j into a(1)(2).m.i, a(1)(2).m.j from t1 where i = 1 ;
  raise notice 'a(1)(2).m.i = %, a(1)(2).m.j = %', a(1)(2).m.i, a(1)(2).m.j;
END;
/

DECLARE
  TYPE myrecord IS RECORD(m t1%ROWTYPE);
  TYPE myrecord2 IS RECORD(n myrecord);
  TYPE mytype IS TABLE OF myrecord2 INDEX BY INT;
  TYPE mytype2 IS TABLE OF mytype INDEX BY INT;
  a mytype2;
BEGIN
  a(1)(2).n.m.i = 11 ;
  a(1)(2).n.m.i = 11 ;
  raise notice 'a(1)(2).n.m.i = %', a(1)(2).n.m.i;
END;
/


--10 FETCH INTO
CREATE table t3( id NUMBER(6), birth date) ;
insert into t3 values(1,date '1998-05-01');
insert into t3 values(2,date '2008-06-01');
insert into t3 values(3,date '2018-07-01');

DECLARE
  TYPE tabCurTyp IS REF CURSOR return t3%ROWTYPE;
  tab_cv tabCurTyp;
  TYPE idlist IS TABLE OF t3.id%TYPE INDEX BY INT;
  TYPE birthlist IS TABLE OF t3.birth%TYPE INDEX BY INT;
  ids idlist;
  births birthlist;
  i int := 1;
BEGIN
  OPEN tab_cv FOR SELECT id, birth FROM t3;
  LOOP
    FETCH tab_cv INTO ids(i), births(i);
	raise notice 'ids(%) = %, births(%) = %', i, ids(i), i, births(i);
	i := i + 1;
	EXIT WHEN tab_cv%NOTFOUND;
  END LOOP;
  IF tab_cv%ISOPEN THEN
    CLOSE tab_cv;
  END IF;
END;
/


DECLARE
  TYPE tabCurTyp IS REF CURSOR return t3%ROWTYPE;
  tab_cv tabCurTyp;
  TYPE idlist IS TABLE OF t3.id%TYPE;
  TYPE birthlist IS TABLE OF t3.birth%TYPE;
  ids idlist := idlist();
  births birthlist := birthlist();
  i int := 1;
BEGIN
  OPEN tab_cv FOR SELECT id, birth FROM t3;
  LOOP
    ids.extend(1);
	births.extend(1);
    FETCH tab_cv INTO ids(i), births(i);
	raise notice 'ids(%) = %, births(%) = %', i, ids(i), i, births(i);
	i := i + 1;
	EXIT WHEN tab_cv%NOTFOUND;
  END LOOP;
  IF tab_cv%ISOPEN THEN
    CLOSE tab_cv;
  END IF;
END;
/

DECLARE
  TYPE tabCurTyp IS REF CURSOR return t3%ROWTYPE;
  tab_cv tabCurTyp;
  TYPE idlist IS VARRAY(20) OF t3.id%TYPE;
  TYPE birthlist IS VARRAY(20) OF t3.birth%TYPE;
  ids idlist := idlist();
  births birthlist := birthlist();
  i int := 1;
BEGIN
  OPEN tab_cv FOR SELECT id, birth FROM t3;
  LOOP
    ids.extend(1);
	births.extend(1);
    FETCH tab_cv INTO ids(i), births(i);
	raise notice 'ids(%) = %, births(%) = %', i, ids(i), i, births(i);
	i := i + 1;
	EXIT WHEN tab_cv%NOTFOUND;
  END LOOP;
  IF tab_cv%ISOPEN THEN
    CLOSE tab_cv;
  END IF;
END;
/

DROP TABLE t3;

--11 RETURNING INTO collection
DECLARE
  TYPE mytype IS TABLE OF INT INDEX BY INT;
  a mytype;
BEGIN
  DELETE FROM t1 WHERE i = 1 RETURNING i, j INTO a(1), a(2);
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

INSERT INTO t1 VALUES(1, 11);

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  DELETE FROM t1 WHERE i = 1 RETURNING i, j INTO a(1), a(2);
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

DECLARE
  TYPE mytype IS VARRAY(10) OF INT;
  a mytype := mytype();
BEGIN
  a.extend(2);
  DELETE FROM t1 WHERE i = 2 RETURNING i, j INTO a(1), a(2);
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/


--12 EXECUTE USING collection
create table departs(department_id integer, department_name varchar(256), manager_id integer, location_id integer);
insert into departs values(1,'development', 1, 1);
insert into departs values(2,'technical support', 2, 1);
insert into departs values(3,'customer service ', 3, 1);
insert into departs values(4,'administrative', 4, 2);

--execute immediate 'select ...' into collect element
DECLARE
  TYPE mytype IS TABLE OF VARCHAR(20);
  a mytype := mytype('value1','value2');
BEGIN
  EXECUTE IMMEDIATE 'select department_name from departs where department_id = 1' INTO a(1) ;
  raise notice 'departname = %',a(1); --'development'
END;
/

--into multiple collection elements
DECLARE
  TYPE mytype IS TABLE OF VARCHAR(20);
  a mytype := mytype('value1','value2');
  TYPE mytype2 IS TABLE OF INT;
  b mytype2 := mytype2(0);
  var_locid int;
BEGIN
  EXECUTE IMMEDIATE 'select department_name,location_id, department_id
    from departs where department_id = 1'
    INTO a(1), var_locid, b(1);
  raise notice 'departname = %',a(1); --'development'
  raise notice 'location_id = %',var_locid; --1
  raise notice 'department_id = %',b(1); --1
END;
/

--using IN collection element
DECLARE
  TYPE mytype IS TABLE OF VARCHAR(20);
  a mytype := mytype('development','technical support');
  var_departid int;
BEGIN
  EXECUTE IMMEDIATE 'select department_id from departs where department_name = :x'
    into var_departid using a(1) ;
  raise notice 'departname = %',var_departid; --1
END;
/

DECLARE
  TYPE second_type IS TABLE OF VARCHAR(20);
  type myrecord0 is record(j1 second_type);
  type myrecord is record(i1 myrecord0);
  type myrecord2 is record(c1 myrecord);
  col myrecord2;
  var_departid int;
BEGIN
  col.c1.i1.j1 := second_type('development','technical support');  
  EXECUTE IMMEDIATE 'select department_id from departs where department_name = :x'
    into var_departid using col.c1.i1.j1(1);
  raise notice 'departname = %',var_departid; --1
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(1,2);
  var_departid int;
BEGIN
  EXECUTE IMMEDIATE 'select department_id from departs where location_id = :x'
    into var_departid  using IN a(1) + a(1);
  raise notice 'var_departid = %',var_departid; --4
END;
/


--using in multiple collection elements
DECLARE
  TYPE first_type IS TABLE OF VARCHAR(20);
  a first_type := first_type('development','technical support');
  TYPE second_type IS TABLE OF first_type;
  b second_type := second_type(a); 
  var_departid int;
BEGIN
  EXECUTE IMMEDIATE 'select department_id from departs where department_name = :x'
    into var_departid using b(1)(1);
  raise notice 'var_departid = %',var_departid; --1 
END;
/

--execute immediate ''  using in out collection element
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(2,1);
BEGIN
  EXECUTE IMMEDIATE 'select department_id from departs where location_id = :x' 
    using in out a(1);
  raise notice '%',a(1); -- 4
END;
/

--return into collection element
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(2,3);
  var_departid int;
  var_locid int;
BEGIN
  EXECUTE IMMEDIATE 'update departs set department_id = 22
    where location_id = 2 returning location_id, department_id into :x,:y'
	return into a(1), a(2);
  raise notice '%', a(1); --2
  raise notice '%', a(2); --22
END;
/

--USING OUT collection element
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(2,3);
  var_departmid int;
BEGIN
  EXECUTE IMMEDIATE 'update departs set department_id = 4 where location_id = :y returning department_id into :x'
    using a(1), out a(2);
  raise notice 'a(1) = %,a(2) = %' ,a(1), a(2);  -- a(1) = 2, a(2) = 4
END;
/

DECLARE
  TYPE first_type IS VARRAY(20) OF INT;
  TYPE second_type IS TABLE OF first_type;
  a first_type := first_type(1,2,3);
  b second_type := second_type(a); 
  var_departmid int;
BEGIN
  EXECUTE IMMEDIATE 'update departs set department_id = 5 where location_id = 2 returning department_id,location_id into :x,:y'
    using out var_departmid, b(1)(2);
  raise notice '%', var_departmid; --5
  raise notice '%', b(1)(2); --2
END;
/

--execute a procedure
CREATE OR REPLACE PROCEDURE addcount(a integer, b integer, c out integer) AS
BEGIN
  c := a + b;
END;
/

DECLARE
  TYPE mytype IS TABLE OF INT;
  myvar mytype := mytype(1,3);
  b integer;
BEGIN
  b := 3;
  execute immediate 'call addcount(:x, :y, :z)' using myvar(1), b, out myvar(2);
  raise notice 'myvar(1) = %', myvar(1); --1
  raise notice 'myvar(2) = %', myvar(2); --4
END;
/

DROP PROCEDURE addcount(integer,integer,integer);	

--execute a anonymous block
CREATE OR REPLACE PROCEDURE create_dept(deptid in out integer,
  dname in varchar2,mgrid in integer,locid in integer) AS
BEGIN
  INSERT INTO departs(
    department_id,
    department_name,
    manager_id,
    location_id
  )VALUES (deptid, dname, mgrid, locid);
END;
/

DECLARE
  plsql_block VARCHAR2(500);
  new_dname   VARCHAR2(30) := 'Advertising';
  new_mgrid   INTEGER    := 200;
  TYPE mytype IS TABLE OF INTEGER;
  a mytype := mytype(11,22);  
BEGIN
  plsql_block := 'BEGIN create_dept(:a, :b, :c, :d); END;';

  EXECUTE IMMEDIATE plsql_block
     USING IN OUT a(1), new_dname, new_mgrid, new_mgrid;	 
END;
/
SELECT * FROM departs;
DROP PROCEDURE create_dept;

DROP TABLE departs;


--13 collection appears in DML
CREATE TABLE tc(id INT, name varchar(20), age INT, salary INT);
INSERT INTO tc VALUES(1, 'name1', 10, 1000);
INSERT INTO tc VALUES(2, 'name2', 20, 2000);

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  TYPE population2 IS TABLE OF VARCHAR2(15) NOT NULL INDEX BY INT;
  a1 population;
  a2 population2;
BEGIN
  SELECT id, name INTO a1(1), a2(1) FROM tc WHERE age = 10;
  raise notice 'a1(1) = %, a2(1) = %', a1(1), a2(1);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  SELECT salary INTO a(2) FROM tc WHERE age = a(1) and name like 'name1';
  raise notice 'a(2) = %', a(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  a(3) := 1000;
  SELECT id INTO a(2) FROM tc WHERE age = a(1) and name like 'name1' and salary = a(3);
  raise notice 'a(2) = %', a(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  a(3) := 1000;
  SELECT id INTO a(2) FROM tc WHERE age = a(1) and salary = a(3) and name like 'name1';
  raise notice 'a(2) = %', a(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  SELECT id INTO a(2) FROM tc WHERE age = a(1);
  raise notice 'a(2) = %', a(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  SELECT id INTO a(2) FROM tc WHERE name like 'name1' and age = a(1);
  raise notice 'a(2) = %', a(2);
END;
/

DECLARE
  TYPE second_type IS TABLE OF int;
  type myrecord0 is record(j1 second_type);
  type myrecord is record(i1 myrecord0);
  type myrecord2 is record(c1 myrecord);
  v_arry_req second_type;
  col myrecord2;
BEGIN
   col.c1.i1.j1 := second_type(10, 22, 33);  
   SELECT id INTO  col.c1.i1.j1(2) FROM tc WHERE age = col.c1.i1.j1(1) and name like 'name1';
   raise notice '%', col.c1.i1.j1(2);
END;
/


CREATE OR REPLACE PACKAGE pkg6 is 
  TYPE second_type IS TABLE OF int;
  type myrecord0 is record(j1 second_type);
  type myrecord is record(i1 myrecord0);
  type myrecord2 is record(c1 myrecord);
  va second_type;
  vr myrecord2;
END;
/

DECLARE
  col pkg6.myrecord2;
BEGIN
  col.c1.i1.j1 := pkg6.second_type(20, 22, 33);  
  SELECT id INTO  col.c1.i1.j1(2) FROM tc WHERE age = col.c1.i1.j1(1) and name like 'name2';
  raise notice '%', col.c1.i1.j1(2);
END;
/
DECLARE
BEGIN
   pkg6.vr.c1.i1.j1 := pkg6.second_type(20, 22, 33);  
   SELECT id INTO  pkg6.vr.c1.i1.j1(2) FROM tc WHERE age = pkg6.vr.c1.i1.j1(1) and name like 'name2';
   raise notice '%', pkg6.vr.c1.i1.j1(2);
END;
/

DECLARE
BEGIN
   pkg6.vr.c1.i1.j1 := pkg6.second_type(20, 22, 33);  
   DELETE FROM tc WHERE age = pkg6.vr.c1.i1.j1(1) and name like 'name2' RETURNING id INTO pkg6.vr.c1.i1.j1(2);
   raise notice '%', pkg6.vr.c1.i1.j1(2);
END;
/

DECLARE
  TYPE population IS TABLE OF INT NOT NULL INDEX BY INT;
  a population;
BEGIN
  a(1) := 10;
  a(3) := 1000;
  DELETE FROM tc WHERE age = a(1) and name like 'name1' and salary = a(3) RETURNING id INTO a(2);
  raise notice 'a(2) = %', a(2);
END;
/

DROP TABLE tc;
DROP PACKAGE pkg6;


--14 more test cases
--collection variables support constant keyword
DECLARE
  TYPE typ1 IS TABLE OF INT;
  a constant typ1 := typ1(1,2,3);
BEGIN
  raise notice '%', a(1);
END;
/

DECLARE
  TYPE typ1 IS TABLE OF INT INDEX BY VARCHAR(20);
  a typ1;
  b constant typ1 := a;
BEGIN
   b('1') := 1;
END;
/

DECLARE
  TYPE typ1 IS TABLE OF INT;
  a typ1 := typ1(1,2);
  b constant typ1 :=  a;
BEGIN
  raise notice '%', b(1) ;
  b(1) := 1;
END;
/

DECLARE
  TYPE typ1 IS VARRAY(3) OF INT;
  a typ1 := typ1(1,2);
  b constant typ1 :=  a;
BEGIN
  raise notice '%', b(1) ;
  b(1) := 1;
END;
/

--collection variables  support not null keyword
DECLARE
  TYPE typ1 IS TABLE OF INT INDEX BY VARCHAR(20);
  a  typ1 not null;
BEGIN
  null;
END;
/

DECLARE
  TYPE typ1 IS TABLE OF INT;
  a  typ1 not null;
BEGIN
  null;
END;
/

DECLARE
  TYPE typ1 IS VARRAY(3) OF INT;
  a  typ1 not null;
BEGIN
  null;
END;
/


create or replace package pkg_con as
  TYPE typ1 IS TABLE OF INT;
  v1 constant typ1 := typ1(1);
end;
/

declare
begin
  pkg_con.v1 := pkg_con.typ1(2);
end;
/

create or replace package pkg_con as
  TYPE typ1 IS RECORD(id int, name varchar(20));
  r1 constant typ1 := (1,1);
end;
/

declare
begin
  pkg_con.r1 := (1,1);
end;
/
drop package pkg_con;

CREATE OR REPLACE PACKAGE pkg AS
  TYPE NumList IS TABLE OF NUMBER;
  PROCEDURE print_numlist (nums NumList);
END pkg;
/
CREATE OR REPLACE PACKAGE BODY pkg AS
  PROCEDURE print_numlist (nums NumList) IS
  BEGIN
    FOR i IN nums.FIRST..nums.LAST LOOP
      raise notice '%', nums(i);
    END LOOP;
  END;
END pkg;
/
DECLARE
  TYPE NumList IS TABLE OF NUMBER;  -- local type identical to package type
  n1 pkg.NumList := pkg.NumList(2,4);  -- package type
  n2     NumList :=     NumList(6,8);  -- local type
BEGIN
  pkg.print_numlist(n1);  -- succeeds
END;
/

DECLARE
  TYPE NumList IS TABLE OF NUMBER;  -- local type identical to package type
  n1 pkg.NumList := pkg.NumList(2,4);  -- package type
  n2     NumList :=     NumList(6,8);  -- local type
BEGIN
  pkg.print_numlist(n2);  -- fails
END;
/

DROP PACKAGE pkg;

--the element datatype of collection is SUBTYPE
DECLARE
  SUBTYPE my_subtype IS INTEGER;
  TYPE mytype IS TABLE OF my_subtype INDEX BY INT;
  a mytype;
BEGIN
  a(1) := 11;
  raise notice '%', a(1);
END;
/

DECLARE
  SUBTYPE my_subtype IS VARCHAR(20);
  TYPE mytype IS TABLE OF my_subtype;
  a mytype := mytype('a', 'b', 'c');
BEGIN
  raise notice 'a(1) = %, a(2) = %', a(1), a(2);
END;
/

DECLARE
  TYPE myrecord IS RECORD(id INT, name VARCHAR(20));
  SUBTYPE my_subtype IS myrecord;
  TYPE population IS TABLE OF my_subtype INDEX BY VARCHAR2(64);
  r my_subtype;
  a population;
BEGIN
  r.id := 11;
  r.name := 'new';
  a(1) := r;
  raise notice 'a(1).id = %, a(1).name = %', a(1).id, a(1).name;
END;
/

CREATE TYPE rec_type AS(id INT, name VARCHAR(20));
DECLARE
  SUBTYPE my_subtype IS rec_type;
  TYPE mytype IS VARRAY(200) OF my_subtype;
  a mytype := mytype();
  r my_subtype;
BEGIN
  r.id := 100;
  r.name := 'new';
  a.extend();
  a(1) := r;
  raise notice 'a(1).id = %, a(1).name = %',a(1).id, a(1).name;
END;
/
DROP TYPE rec_type;

/* Begin - BUG#Z7311 */
create table tb_dsql(empno number, ename varchar2(30), empsal number);
insert into tb_dsql values(1,'wang',100);

CREATE OR REPLACE PACKAGE pkg_dsql AS
   PROCEDURE proc_with_params(id number, name OUT varchar2);
END pkg_dsql;
 /

 CREATE OR REPLACE PACKAGE BODY pkg_dsql AS
   PROCEDURE proc_with_params(id number, name OUT varchar2) IS
   BEGIN
    DELETE FROM tb_dsql WHERE empno = id RETURNING ename INTO name;
   END;
END pkg_dsql;
 /

DECLARE
  dyn_stmt VARCHAR2(100);
  v_name VARCHAR2(100);
BEGIN
   dyn_stmt := 'BEGIN pkg_dsql.proc_with_params(:x,:y); END;';
   EXECUTE IMMEDIATE dyn_stmt USING 1 RETURNING INTO v_name;
   raise notice 'v_name=%', v_name;
END;
 /


insert into tb_dsql values(1,'wang',100);

DECLARE
  dyn_stmt VARCHAR2(100);
  TYPE mytype1 IS TABLE OF VARCHAR2(100)  INDEX BY INT;
  v_name mytype1;
BEGIN
   dyn_stmt := 'BEGIN pkg_dsql.proc_with_params(:x,:y); END;';
   EXECUTE IMMEDIATE dyn_stmt USING 1 RETURNING INTO v_name(1);
   raise notice 'v_name=%', v_name(1);
END;
 /

insert into tb_dsql values(1, 'tom', 100);

DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype(2,3);
  var_departid int;
  var_locid int;
BEGIN
  EXECUTE IMMEDIATE 'update  tb_dsql  set  empsal = 220
  returning empno,  empsal into :x,:y'
	return into a(1), a(2);
  raise notice '%', a(1); --1
  raise notice '%', a(2); --220
END;
/
DECLARE
  TYPE mytype IS TABLE OF INT;
  a mytype := mytype();
  var_departid int;
  var_locid int;
BEGIN
  a.extend(2);
  EXECUTE IMMEDIATE 'update  tb_dsql  set  empsal = 230
  returning empno,  empsal into :x,:y'
	return into a(1), a(2);
  raise notice '%', a(1); --1
  raise notice '%', a(2); --230
END;
/

DROP PACKAGE pkg_dsql;
DROP TABLE tb_dsql;
/* End - BUG#Z7311 */

DROP TABLE t1;
DROP TABLE t2;