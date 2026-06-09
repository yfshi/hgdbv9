--test implicit cursor
--ok
SET plisql.plisql_default_into_strict = false;
SET plisql.plisql_cursor_name_exclude_oid = true;
CREATE TABLE departments(department_id integer,
department_name varchar2(256),location_id number(38, 0));

--ok
INSERT INTO departments VALUES(2700,'name-2700', 1);

--ok
CREATE TABLE dept_temp AS
  SELECT * FROM departments;

--ok
CREATE OR REPLACE PROCEDURE p (
  dept_no NUMBER
) AUTHID CURRENT_USER AS
BEGIN
  DELETE FROM dept_temp
  WHERE department_id = dept_no;
  IF SQL%FOUND THEN
    raise info 'Delete succeeded for department number %', dept_no;
  ELSE
    raise info 'No department number %',dept_no;
  END IF;
END;
/

--ok
--print Delete succeeded for department number 2700
--No department number 400
BEGIN
  call p(2700);
  call p(400);
END;
/


--implicit attribute
--ok
--print
--implicit cursor first found is null
--var1 = <NULL>
--implicit cursor second found
--var1 = 1
--implicit cursor third not found
--var1 = 0
--implicit cursor fourth not found
--var1 = 0
DECLARE
  var1 integer;
BEGIN
  IF sql%isopen THEN
	raise info 'implicit cursor first open';
  END if;
  IF sql%found THEN
	raise info 'implicit cursor first found';
  END if;
  IF sql%notfound THEN
	raise info 'implicit cursor first not found';
  END if;
  IF sql%isopen IS NULL THEN
	raise info 'implicit cursor first open is null';
  END if;
  IF sql%found IS NULL THEN
	raise info 'implicit cursor first found is null';
  END if;
  var1 := sql%rowcount;
  raise info 'var1 = %', var1;
  UPDATE departments SET department_name = 'update_name';
  IF sql%isopen THEN
	raise info 'implicit cursor second open';
  END if;
  IF sql%found THEN
	raise info 'implicit cursor second found';
  END if;
  IF sql%notfound THEN
	raise info 'implicit cursor second not found';
  END if;
  IF sql%notfound  IS NULL THEN
	raise info 'implicit cursor second notfound is null';
  END if;
  var1 := sql%rowcount;
  raise info 'var1 = %', var1;
  UPDATE departments SET department_name = 'update_name' WHERE
  department_id = -1;
  IF sql%isopen THEN
	raise info 'implicit cursor third open';
  END if;
  IF sql%found THEN
	raise info 'implicit cursor third found';
  END if;
  IF sql%notfound THEN
	raise info 'implicit cursor third not found';
  END if;
  IF sql%notfound  IS NULL THEN
	raise info 'implicit cursor third notfound is null';
  END if;
  var1 := sql%rowcount;
  raise info 'var1 = %', var1;
  var1 := ABS(-1);
  IF sql%isopen THEN
	raise info 'implicit cursor fourth open';
  END if;
  IF sql%found THEN
	raise info 'implicit cursor fourth found';
  END if;
  IF sql%notfound THEN
	raise info 'implicit cursor fourth not found';
  END if;
  IF sql%notfound  IS NULL THEN
	raise info 'implicit cursor fourth notfound is null';
  END if;
  var1 := sql%rowcount;
  raise info 'var1 = %', var1;
end;
/

--nested function change sql%attribute
--ok and print
--rowcount:<NULL>
--netsted rowcount:1
--after nested function rowcount = 1
DECLARE
  var1 integer;
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
    UPDATE departments SET department_name = 'update_name';
	raise info 'netsted rowcount:%', sql%rowcount;
    RETURN 23;
  end;
BEGIN
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_f(23);
  raise info 'after nested function rowcount = %', sql%rowcount;
end;
/

--schema function change attribute
--ok
CREATE OR REPLACE FUNCTION test_sch_f(id integer) RETURN integer IS
BEGIN
  UPDATE departments SET department_name = 'update_name';
  raise info 'schema function rowcount:%', sql%rowcount;
  RETURN 23;
END;
/

--ok
--print
--rowcount = <NULL>
--schema function rowcount:1
--after schema function rowcount:1
DECLARE
  var1 integer;
BEGIN
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_sch_f(23);
  raise info 'after schema function rowcount:%', sql%rowcount;
end;
/


--again
--print
--rowcount = <NULL>
--schema function rowcount:1
--after schema function rowcount:1
DECLARE
  var1 integer;
BEGIN
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_sch_f(23);
  raise info 'after schema function rowcount:%', sql%rowcount;
end;
/

--package function
--ok
CREATE OR REPLACE package test_pkg IS
  FUNCTION test_f(id integer) RETURN integer;
end;
/

--ok
CREATE OR REPLACE package body test_pkg IS
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
     UPDATE departments SET department_name = 'update_name';
	 raise info 'package function rowcount:%', sql%rowcount;
	 RETURN 23;
  end;
end;
/

--ok
--print
-- rowcount = <NULL>
--package function rowcount:1
--after package function rowcount = 1
DECLARE
  var1 integer;
BEGIN
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_pkg.test_f(23);
  raise info 'after package function rowcount = %', sql%rowcount;
end;
/

--ok
CREATE TABLE employees(employee_id integer, manager_id integer, department_id integer,
                        job_id varchar2(256), salary number,
						first_name varchar2(256),
						last_name varchar(256),
						hire_date date);

INSERT INTO employees VALUES(1, 122, 1,1,20000,'test-122-1','test-122-1',to_date('2010-12-1','yyyy-mm-dd'));
INSERT INTO employees VALUES(2, 122,1,2,30000, 'test-122-2','test-122-2',to_date('2010-11-1', 'yyyy-mm-dd'));

--ok
CREATE TABLE employees_temp AS
  SELECT * FROM employees;

--ok
--print Number of employees deleted: 2
DECLARE
  mgr_no NUMBER(6) := 122;
BEGIN
  DELETE FROM employees_temp WHERE manager_id = mgr_no;
  raise info 'Number of employees deleted: %',TO_CHAR(SQL%ROWCOUNT);
END;
/


--select into
--ok and print
--sql found is true
--sql rowcount is:1
DECLARE
  var1 integer;
BEGIN
  SELECT department_id INTO var1 FROM departments;
  IF sql%found THEN
    raise info 'sql found is true';
  END if;
  IF sql%found IS NULL THEN
     raise info 'sql found is null';
  END if;
  IF sql%notfound THEN
    raise info 'sql not found is true';
  END if;
  IF sql%notfound IS NULL THEN
    raise info 'sql not found is null';
  END if;
  raise info 'sql rowcount is:%', sql%rowcount;
end;
/

--use sql aggregate ok
-- print
-- sql found is true
--sql rowcount is:1
DECLARE
  var1 integer;
BEGIN
  SELECT COUNT(department_id) INTO var1 FROM departments WHERE department_id = -1;
  IF sql%found THEN
    raise info 'sql found is true';
  END if;
  IF sql%found IS NULL THEN
     raise info 'sql found is null';
  END if;
  IF sql%notfound THEN
    raise info 'sql not found is true';
  END if;
  IF sql%notfound IS NULL THEN
    raise info 'sql not found is null';
  END if;
  raise info 'sql rowcount is:%', sql%rowcount;
end;
/

--complex condition ok
--and print INFO:  found is true
DECLARE
  var1 integer;
BEGIN
  DELETE FROM departments;
  IF sql%isopen AND sql%found AND sql%notfound THEN
	raise info 'all is true';
  ELSIF (sql%found AND sql%notfound) THEN
	raise info 'found and not found';
  ELSIF (sql%found) THEN
	raise info 'found is true';
  END if;
end;
/

--assign report error
DECLARE
  var1 integer;
BEGIN
  DELETE FROM departments;
  sql%ROWCOUNT := 1;
end;
/


--attribute used in dml report error
DECLARE
  var1 integer;
BEGIN
	INSERT INTO departments VALUES(1,'welcome', 2);
	INSERT INTO departments VALUES(2,'xiexie', sql%rowcount);
end;
/

--error
DECLARE
  var1 integer;
BEGIN
	INSERT INTO departments VALUES(1,'welcome', 2);
	DELETE FROM departments WHERE DEPARTMENT_ID = sql%rowcount;
end;
/

--explicit cursor
--explicit cursor declaration and definition
CREATE TABLE locations(location_id integer,country_id varchar2(256),
city varchar2(256));


--ok
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1

  CURSOR c2 IS                             -- Declare and define c2
    SELECT employee_id, job_id, salary FROM employees
    WHERE salary > 2000;

  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
    SELECT * FROM departments              -- repeating return type
    WHERE department_id = 110;

  CURSOR c3 RETURN locations%ROWTYPE;     -- Declare c3

  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
BEGIN
  NULL;
END;
/

--ok
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1

  CURSOR c2 IS                             -- Declare and define c2
    SELECT employee_id, job_id, salary FROM employees
    WHERE salary > 2000;

  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
    SELECT * FROM departments              -- repeating return type
    WHERE department_id = 110;

  CURSOR c3 RETURN locations%rowtype IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
BEGIN
  NULL;
END;
/

--ok
--cursor has declare but not define
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
BEGIN
  NULL;
END;
/

--raise error
--use c1 not define %rowtype;
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
  var1 c1%rowtype;
BEGIN
  NULL;
END;
/

--ok
--add define
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
    SELECT * FROM departments              -- repeating return type
    WHERE department_id = 110;
  var1 c1%rowtype;
BEGIN
  NULL;
END;
/

--%rowtype before define
--raise error
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
  var1 c1%rowtype;
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
    SELECT * FROM departments              -- repeating return type
    WHERE department_id = 110;
BEGIN
  NULL;
END;
/

--ok
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
	CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
  var1 c1%rowtype;
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
BEGIN
  NULL;
END;
/

--repeate define
--raise error
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
	CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
  CURSOR c3 IS                             -- Define c3,
    SELECT * FROM locations                -- omitting return type
    WHERE country_id = 'JP';
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 120;
BEGIN
  NULL;
END;
/

--check
--main block declare,and nested function define
--raise error
DECLARE
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  FUNCTION test_f(id integer) RETURN integer IS
    CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
  BEGIN
    RETURN 23;
  end;
BEGIN
  OPEN c1;
end;
/


--
--package specification declare
-- body define
--ok
CREATE OR REPLACE package test_pkg IS
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  FUNCTION test_f(id integer) RETURN integer;
END test_pkg;
/

--ok
CREATE OR REPLACE package body test_pkg IS
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
    RETURN 23;
  end;
BEGIN
   OPEN c1;
end;
/

--ok
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  CLOSE test_pkg.c1;
end;
/

--body remove define
--error for c1 has not a define
CREATE OR REPLACE package body test_pkg IS
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
    RETURN 23;
  end;
BEGIN
   OPEN c1;
end;
/

drop package test_pkg;

--package specification has define
CREATE OR REPLACE package test_pkg IS
  CURSOR c1 RETURN departments%ROWTYPE;    -- Declare c1
  FUNCTION test_f(id integer) RETURN integer;
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
END test_pkg;
/

--open
--oracle raise error,but we success
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  open test_pkg.c1;
end;
/


--only define
CREATE OR REPLACE package test_pkg IS
  FUNCTION test_f(id integer) RETURN integer;
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
END test_pkg;
/


--ok
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  open test_pkg.c1;
end;
/

--ok
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  close test_pkg.c1;
end;
/

DROP package test_pkg;

--package specification has declare and define
CREATE OR REPLACE package test_pkg IS
  CURSOR c1 RETURN departments%ROWTYPE IS  -- Define c1,
	SELECT * FROM departments              -- repeating return type
	WHERE department_id = 110;
end;
/

--ok
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  NULL;
end;
/

--ok
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  OPEN test_pkg.c1;
  CLOSE test_pkg.c1;
end;
/

DROP package test_pkg;

--explicit cursor support parameter
DELETE FROM employees;
INSERT INTO employees VALUES(3, 122,1,'ST_CLERK',30000, 'wang','xing', to_date('2021-10-11', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(4, 122,1,'ST_CLERK',40000, 'liu','min', to_date('2021-11-12', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(4, 122,2,'SA_REP',40000, 'li','min', to_date('2022-11-19', 'yyyy-mm-dd'));


--ok
--and print
--Overpaid Stock Clerks:
----------------------
--xing, wang (by 25000)
--min, liu (by 35000)
-------------------------------
--Overpaid Sales Representatives:
-------------------------------
--min, li (by 30000)
DECLARE
  CURSOR c (job VARCHAR2, max_sal NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;

  PROCEDURE print_overpaid IS
    last_name_   employees.last_name%TYPE;
    first_name_  employees.first_name%TYPE;
    overpayment_      employees.salary%TYPE;
  BEGIN
    LOOP
      FETCH c INTO last_name_, first_name_, overpayment_;
      EXIT WHEN c%NOTFOUND;
	  raise info '%,% (by %)', last_name_, first_name_, overpayment_;
    END LOOP;
  END print_overpaid;
BEGIN
  raise info '----------------------';
  raise info 'Overpaid Stock Clerks:';
  raise info '----------------------';
  OPEN c('ST_CLERK', 5000);
  call print_overpaid();
  CLOSE c;

  raise info '-------------------------------';
  raise info 'Overpaid Sales Representatives:';
  raise info '-------------------------------';
  OPEN c('SA_REP', 10000);
  call print_overpaid();
  CLOSE c;
END;
/

--cursor has out parameter
--raise error but oracle ok
--maybe not use?
--ok
DECLARE
  CURSOR c (job VARCHAR2, max_sal out NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;
BEGIN
    NULL;
END;
/


--raise error
DECLARE
  CURSOR c (job VARCHAR2, max_sal out NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;

  PROCEDURE print_overpaid IS
    last_name_   employees.last_name%TYPE;
    first_name_  employees.first_name%TYPE;
    overpayment_      employees.salary%TYPE;
  BEGIN
    LOOP
      FETCH c INTO last_name_, first_name_, overpayment_;
      EXIT WHEN c%NOTFOUND;
      raise info '%,% (by %)', last_name_, first_name_, overpayment_;
    END LOOP;
  END print_overpaid;
BEGIN
  raise info '----------------------';
  raise info 'Overpaid Stock Clerks:';
  raise info '----------------------';
  OPEN c('ST_CLERK', 5000);
  call print_overpaid();
  CLOSE c;

  raise info '-------------------------------';
  raise info 'Overpaid Sales Representatives:';
  raise info '-------------------------------';
  OPEN c('SA_REP', 10000);
  call print_overpaid();
  CLOSE c;
END;
/


--ok
--print
----------------------
--Overpaid Stock Clerks:
----------------------
-------------------------------
--Overpaid Sales Representatives:
-------------------------------
--
DECLARE
  var1 number;
  CURSOR c (job VARCHAR2, max_sal out NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;

  PROCEDURE print_overpaid IS
    last_name_   employees.last_name%TYPE;
    first_name_  employees.first_name%TYPE;
    overpayment_      employees.salary%TYPE;
  BEGIN
    LOOP
      FETCH c INTO last_name_, first_name_, overpayment_;
      EXIT WHEN c%NOTFOUND;
      raise info '%,% (by %)', last_name_, first_name_, overpayment_;
    END LOOP;
  END print_overpaid;
BEGIN
  raise info '----------------------';
  raise info 'Overpaid Stock Clerks:';
  raise info '----------------------';
  var1 := 5000;
  OPEN c('ST_CLERK', var1);
  call print_overpaid();
  CLOSE c;

  raise info '-------------------------------';
  raise info 'Overpaid Sales Representatives:';
  raise info '-------------------------------';
  var1 := 10000;
  OPEN c('SA_REP', var1);
  call print_overpaid();
  CLOSE c;
END;
/

--check out parameter
--var1 =
DECLARE
  var1 number;
  CURSOR c (job VARCHAR2, max_sal out NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;

  PROCEDURE print_overpaid IS
    last_name_   employees.last_name%TYPE;
    first_name_  employees.first_name%TYPE;
    overpayment_      employees.salary%TYPE;
  BEGIN
    LOOP
      FETCH c INTO last_name_, first_name_, overpayment_;
      EXIT WHEN c%NOTFOUND;
      raise info '%,% (by %)', last_name_, first_name_, overpayment_;
    END LOOP;
  END print_overpaid;

BEGIN
  raise info '----------------------';
  raise info 'Overpaid Stock Clerks:';
  raise info '----------------------';
  var1 := 5000;
  OPEN c('ST_CLERK', var1);
  raise info 'var1 = %', var1;
  call print_overpaid();
  CLOSE c;

  raise info '-------------------------------';
  raise info 'Overpaid Sales Representatives:';
  raise info '-------------------------------';
  var1 := 10000;
  OPEN c('SA_REP', var1);
  raise info 'var1 = %', var1;
  call print_overpaid();
  CLOSE c;
END;
/

--use in
--ok and print
--Overpaid Stock Clerks:
----------------------
--xing,wang (by 25000)
--min,liu (by 35000)
-------------------------------
--Overpaid Sales Representatives:
-------------------------------
--min,li (by 30000)
DECLARE
  var1 number;
  CURSOR c (job VARCHAR2, max_sal NUMBER) IS
    SELECT last_name, first_name, (salary - max_sal) overpayment
    FROM employees
    WHERE job_id = job
    AND salary > max_sal
    ORDER BY salary;

  PROCEDURE print_overpaid IS
    last_name_   employees.last_name%TYPE;
    first_name_  employees.first_name%TYPE;
    overpayment_      employees.salary%TYPE;
  BEGIN
    LOOP
      FETCH c INTO last_name_, first_name_, overpayment_;
      EXIT WHEN c%NOTFOUND;
      raise info '%,% (by %)', last_name_, first_name_, overpayment_;
    END LOOP;
  END print_overpaid;

BEGIN
  raise info '----------------------';
  raise info 'Overpaid Stock Clerks:';
  raise info '----------------------';
  var1 := 5000;
  OPEN c('ST_CLERK', var1);
  call print_overpaid();
  CLOSE c;

  raise info '-------------------------------';
  raise info 'Overpaid Sales Representatives:';
  raise info '-------------------------------';
  var1 := 10000;
  OPEN c('SA_REP', var1);
  call print_overpaid();
  CLOSE c;
END;
/

--explicit curosr paramster include default value
--
INSERT INTO locations VALUES(1700,'China','BeiJing');
INSERT INTO locations VALUES(1800,'Candina','Toronto');
INSERT INTO locations VALUES(1900,'American','Whitehorse');

INSERT INTO departments values(2,'yafa',1700);
INSERT INTO departments values(3,'xiaoshou',1800);
INSERT INTO departments VALUES(4,'gonggua',1900);

INSERT INTO employees VALUES(5,123,2,'neihe', 30000,'zhang','san', to_date('2022-12-11', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(6,124,3,'kehu', 40000,'zhang','si', to_date('2022-12-12', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(7,125,4,'zhuguan', 30000,'zhang','liu', to_date('2022-12-23', 'yyyy-mm-dd'));

--ok
DECLARE
  CURSOR c (location NUMBER DEFAULT 1700) IS
    SELECT d.department_name,
           e.last_name manager,
           l.city
    FROM departments d, employees e, locations l
    WHERE l.location_id = location
      AND l.location_id = d.location_id
      AND d.department_id = e.department_id
    ORDER BY d.department_id,manager;

  PROCEDURE print_depts IS
    dept_name  departments.department_name%TYPE;
    mgr_name   employees.last_name%TYPE;
    city_name  locations.city%TYPE;
  BEGIN
    LOOP
      FETCH c INTO dept_name, mgr_name, city_name;
      EXIT WHEN c%NOTFOUND;
	  raise info '%(Manager:%)',dept_name, mgr_name;
    END LOOP;
  END print_depts;

BEGIN
  raise info 'DEPARTMENTS AT HEADQUARTERS:';
  raise info '--------------------------------';
  OPEN c;
  call print_depts();
  raise info '--------------------------------';
  CLOSE c;

  raise info 'DEPARTMENTS IN CANADA:';
  raise info '--------------------------------';
  OPEN c(1800); -- Toronto
  call print_depts();
  CLOSE c;
  OPEN c(1900); -- Whitehorse
  call print_depts();
  CLOSE c;
END;
/

--out has default value
--raise error
DECLARE
  CURSOR c (location out NUMBER DEFAULT 1700) IS
    SELECT d.department_name,
           e.last_name manager,
           l.city
    FROM departments d, employees e, locations l
    WHERE l.location_id = location
      AND l.location_id = d.location_id
      AND d.department_id = e.department_id
    ORDER BY d.department_id;
BEGIN
  NULL;
END;
/

--use fetch in a loop
DECLARE
  CURSOR c1 IS
    SELECT last_name, job_id FROM employees
    WHERE REGEXP_LIKE (job_id, 'S[HT]_CLERK')
    ORDER BY last_name;
  v_lastname  employees.last_name%TYPE;  -- variable for last_name
  v_jobid     employees.job_id%TYPE;     -- variable for job_id

  CURSOR c2 IS
    SELECT * FROM employees
    WHERE REGEXP_LIKE (job_id, '[ACADFIMKSA]_M[ANGR]')
    ORDER BY job_id;
  v_employees employees%ROWTYPE;  -- record variable for row of table

BEGIN
  OPEN c1;
  LOOP  -- Fetches 2 columns into variables
    FETCH c1 INTO v_lastname, v_jobid;
    EXIT WHEN c1%NOTFOUND;
	raise info '%',RPAD(v_lastname, 25, ' ') || v_jobid;
  END LOOP;
  CLOSE c1;
  raise info '-------------------------------------';

  OPEN c2;
  LOOP  -- Fetches entire row into the v_employees record
    FETCH c2 INTO v_employees;
    EXIT WHEN c2%NOTFOUND;
    raise info '%',RPAD(v_employees.last_name, 25, ' ') ||
                               v_employees.job_id;
  END LOOP;
  CLOSE c2;
END;
/

--fetch into different variable with the same explicit cursor
create table jobs(job_id varchar2(256),job_title varchar2(256));

INSERT INTO jobs VALUES('meigong','rengong');

INSERT INTO employees VALUES(8,100,4,'meigong', 30000,'shima','xiangru', to_date('2022-12-15', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(9,100,4,'meigong', 40000,'shima','guang', to_date('2022-12-24', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(10,100,4,'meigong', 50000,'fan','li', to_date('2022-12-25', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(11,100,4,'meigong', 70000,'guan','xing', to_date('2022-12-26', 'yyyy-mm-dd'));
INSERT INTO employees VALUES(12,100,4,'meigong', 80000,'dai','li', to_date('2022-12-30', 'yyyy-mm-dd'));

--ok
DECLARE
  CURSOR c IS
    SELECT e.job_id, j.job_title
    FROM employees e, jobs j
    WHERE e.job_id = j.job_id AND e.manager_id = 100
    ORDER BY last_name;
  -- Record variables for rows of cursor result set:
  job1 c%ROWTYPE;
  job2 c%ROWTYPE;
  job3 c%ROWTYPE;
  job4 c%ROWTYPE;
  job5 c%ROWTYPE;

BEGIN
  OPEN c;
  FETCH c INTO job1;  -- fetches first row
  FETCH c INTO job2;  -- fetches second row
  FETCH c INTO job3;  -- fetches third row
  FETCH c INTO job4;  -- fetches fourth row
  FETCH c INTO job5;  -- fetches fifth row
  CLOSE c;

  raise info '%',job1.job_title || ' (' || job1.job_id || ')';
  raise info '%',job2.job_title || ' (' || job2.job_id || ')';
  raise info '%',job3.job_title || ' (' || job3.job_id || ')';
  raise info '%',job4.job_title || ' (' || job4.job_id || ')';
  raise info '%',job5.job_title || ' (' || job5.job_id || ')';
END;
/

--variable change after open
--doesn't change variable value
DECLARE
  sal           employees.salary%TYPE;
  sal_multiple  employees.salary%TYPE;
  factor        INTEGER := 2;
  CURSOR c1 IS
    SELECT salary, salary*factor FROM employees
    WHERE job_id LIKE 'ST_%';
BEGIN
  OPEN c1;  -- PL/SQL evaluates factor
  LOOP
    FETCH c1 INTO sal, sal_multiple;
    EXIT WHEN c1%NOTFOUND;
    raise info '%','factor = ' || factor;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
    factor := factor + 1;  -- Does not affect sal_multiple
  END LOOP;
  CLOSE c1;
END;
/

--change variable values before open will change
--result sets
DECLARE
  sal           employees.salary%TYPE;
  sal_multiple  employees.salary%TYPE;
  factor        INTEGER := 2;
  CURSOR c1 IS
    SELECT salary, salary*factor FROM employees
    WHERE job_id LIKE 'ST_%';
BEGIN
  raise info '%','factor = ' || factor;
  OPEN c1;  -- PL/SQL evaluates factor
  LOOP
    FETCH c1 INTO sal, sal_multiple;
    EXIT WHEN c1%NOTFOUND;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
  END LOOP;
  CLOSE c1;
  factor := factor + 1;
  raise info '%','factor = ' || factor;
  OPEN c1;  -- PL/SQL evaluates factor
  LOOP
    FETCH c1 INTO sal, sal_multiple;
    EXIT WHEN c1%NOTFOUND;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
  END LOOP;
  CLOSE c1;
END;
/

--virtual columns should use alias
DECLARE
  CURSOR c1 IS
    SELECT employee_id,
           (salary * .05) raise
    FROM employees
    WHERE job_id LIKE 'ST%'
    ORDER BY employee_id;
  emp_rec c1%ROWTYPE;
BEGIN
  OPEN c1;
  LOOP
    FETCH c1 INTO emp_rec;
    EXIT WHEN c1%NOTFOUND;
    raise info '%',
      'Raise for employee #' || emp_rec.employee_id ||
      ' is $' || emp_rec.raise;
  END LOOP;
  CLOSE c1;
END;
/

--explicit curosr use %isopen attribute
DECLARE
  CURSOR c1 IS
    SELECT last_name, salary FROM employees
    WHERE ROWNUM < 11;

  the_name employees.last_name%TYPE;
  the_salary employees.salary%TYPE;
BEGIN
  IF NOT c1%ISOPEN THEN
    OPEN c1;
  END IF;

  FETCH c1 INTO the_name, the_salary;

  IF c1%ISOPEN THEN
    CLOSE c1;
  END IF;
END;
/

--use %found attribute
DECLARE
  CURSOR c1 IS
    SELECT last_name, salary FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name,salary;

  my_ename   employees.last_name%TYPE;
  my_salary  employees.salary%TYPE;
BEGIN
  OPEN c1;
  LOOP
    FETCH c1 INTO my_ename, my_salary;
    IF c1%FOUND THEN  -- fetch succeeded
      raise info '%','Name = ' || my_ename || ', salary = ' || my_salary;
    ELSE  -- fetch failed
      EXIT;
    END IF;
  END LOOP;
END;
/

--use %notfound attribute
DECLARE
  CURSOR c1 IS
    SELECT last_name, salary FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name,salary;

   id integer := 0;
   my_ename   employees.last_name%TYPE;
   my_salary  employees.salary%TYPE;
BEGIN
  OPEN c1;
  LOOP
    FETCH c1 INTO my_ename, my_salary;
    IF c1%NOTFOUND THEN -- fetch failed
      EXIT;
    ELSE  -- fetch succeeded
      raise info '%', 'Name = ' || my_ename || ', salary = ' || my_salary;
    END IF;
	id := id + 1;
  END LOOP;
  raise info '%','id = ' || id;
END;
/

--use %rowcount attribute
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
  name  employees.last_name%TYPE;
BEGIN
  OPEN c1;
  LOOP
    FETCH c1 INTO name;
    EXIT WHEN c1%NOTFOUND OR c1%NOTFOUND IS NULL;
    raise info '%', c1%ROWCOUNT || '. ' || name;
    IF c1%ROWCOUNT = 5 THEN
       raise info '%', '--- Fetched 5th row ---';
    END IF;
  END LOOP;
  CLOSE c1;
END;
/


--one row
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
  var1 varchar2(256);
  var2 varchar2(256);
BEGIN
  OPEN c1;
  SELECT sql_text,CURSOR_TYPE  INTO var1,var2 FROM v$open_cursor WHERE sql_text LIKE '%employees%';
  raise info 'SQL_TEXT = %',var1;
  raise info 'CURSOR_TYPE = %',var2;
end;
/


--raise error
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
  var1 varchar2(256);
BEGIN
  CLOSE c1;
end;
/

--ok
--again open the same explicit cursor
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
  var1 varchar2(256);
BEGIN
  OPEN c1;
end;
/

--only about a opened cursor and transaction commit
--not close
--open for anothers sql
--ok
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees a
    WHERE ROWNUM < 11
    ORDER BY last_name;
  var1 varchar2(256);
BEGIN
  OPEN c1;
end;
/


--different block use the same sql
--ok
DECLARE
  CURSOR c1 IS
    SELECT first_name FROM employees a
    WHERE ROWNUM < 11
    ORDER BY first_name;
BEGIN
   DECLARE
      CURSOR c1 IS
    SELECT first_name FROM employees a
    WHERE ROWNUM < 11
    ORDER BY first_name;
  BEGIN
     OPEN c1;
  end;
  OPEN c1;
end;
/


--explicit cursor define in package
CREATE OR REPLACE package test_pkg IS
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
end;
/

--ok
BEGIN
  OPEN test_pkg.c1;
end;
/


--one rows
SELECT sql_text,CURSOR_TYPE FROM v$open_cursor where sql_text LIKE '%employees%';

--ok
--print test_pkg.c1 is open
BEGIN
  IF test_pkg.c1%isopen THEN
	raise info 'test_pkg.c1 is open';
  ELSE
	raise info 'test_pkg.c1 is not open';
  END if;
end;
/

--raise error
BEGIN
  OPEN test_pkg.c1;
end;
/

--ok
BEGIN
  close test_pkg.c1;
end;
/

--one row
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees
    WHERE ROWNUM < 11
    ORDER BY last_name;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var1;
  raise info '%','last_name:' || var1.last_name;
end;
/

--ok
BEGIN
 OPEN test_pkg.c1;
end;
/

--print one row
DECLARE
  var1 test_pkg.c1%rowtype;
BEGIN
  FETCH test_pkg.c1 INTO var1;
  raise info '%','last_name:' || var1.last_name;
end;
/

--one row
SELECT sql_text FROM v$open_cursor ORDER BY sql_text;

--drop package
DROP package test_pkg;

--none row
SELECT sql_text FROM v$open_cursor ORDER BY sql_text;

--package private global cursor is global?
CREATE OR REPLACE package test_pkg IS
  procedure open_cursor(id integer);
  PROCEDURE fetch_cursor(id integer);
  PROCEDURE close_cursor(id integer);
end;
/


CREATE OR REPLACE package body test_pkg IS
  CURSOR c1 IS
    SELECT last_name FROM employees absd
    WHERE ROWNUM < 11
    ORDER BY last_name;
 var1 c1%rowtype;
 PROCEDURE open_cursor(id integer) IS
 BEGIN
    OPEN c1;
 end;
 PROCEDURE fetch_cursor(id integer) IS
 BEGIN
    FETCH c1 INTO var1;
	raise info '%','last_name:' || var1.last_name;
 end;
 PROCEDURE close_cursor(id integer) IS
 BEGIN
   CLOSE c1;
 end;
end;
/


--ok
BEGIN
  call test_pkg.open_cursor(0);
end;
/

--one row
BEGIN
  call test_pkg.fetch_cursor(0);
end;
/


--one row
BEGIN
  call test_pkg.fetch_cursor(0);
end;
/


--ok
BEGIN
 call test_pkg.CLOSE_cursor(0);
end;
/

--privite cursor close
--always print open c1
DECLARE
   var1 integer;
   i integer;
   FUNCTION test_f(id integer) RETURN integer IS
     CURSOR c1 IS
    SELECT last_name FROM employees absd
    WHERE ROWNUM < 11
    ORDER BY last_name;
  BEGIN
    IF c1%isopen THEN
       raise info 'c1 is open';
	ELSE
	   OPEN c1;
	   raise info 'open c1';
	END if;
    RETURN 23;
  end;
BEGIN
  FOR i IN 1 .. 10 LOOP
    var1 := test_f(i);
  END loop;
end;
/


CREATE OR REPLACE package test_pkg IS
  FUNCTION test_p(id integer) RETURN integer;
end;
/


CREATE OR REPLACE package body test_pkg IS
  FUNCTION test_p(id integer) RETURN integer IS
    CURSOR c1 IS
    SELECT last_name FROM employees absd
    WHERE ROWNUM < 11
    ORDER BY last_name;
  BEGIN
    IF c1%isopen THEN
       raise info 'c1 is open';
	ELSE
	   OPEN c1;
	   raise info 'open c1';
	END if;
    RETURN 23;
  end;
end;
/

--ok
--and print open c1
DECLARE
  i integer;
  var1 integer;
BEGIN
  FOR i IN 1 .. 10 LOOP
    var1 := test_pkg.test_p(i);
  END loop;
end;
/

--return type mismatch select
--ok not use
DECLARE
  CURSOR c1 RETURN employees%rowtype;
  CURSOR c1 IS SELECT * FROM jobs;
BEGIN
  NULL;
end;
/

--oracle failed,but we success
DECLARE
  CURSOR c1 RETURN jobs%rowtype;
  CURSOR c1 IS SELECT * FROM jobs;
BEGIN
  OPEN c1;
end;
/

--mismatch error
DECLARE
  CURSOR c1 RETURN employees%rowtype;
  CURSOR c1 IS SELECT * FROM jobs;
BEGIN
  OPEN c1;
end;
/

--mismatch return as query
DECLARE
  CURSOR c1 RETURN employees%rowtype IS SELECT * FROM jobs;
BEGIN
  OPEN c1;
end;
/

--raise error
DECLARE
  CURSOR c1(id integer) IS SELECT * FROM employees WHERE EMPLOYEE_ID = id;
  CURSOR c1(ids2 integer) IS SELECT * FROM employees WHERE MANAGER_ID = ids2;
BEGIN
  OPEN c1(id=>1);
  OPEN c1(ids2=>1);
end;
/


--raise error
DECLARE
  CURSOR c1(id integer) IS SELECT * FROM employees WHERE EMPLOYEE_ID = id;
  CURSOR c1(name varchar2) IS SELECT * FROM employees WHERE FIRST_NAME = name;
  var1 integer;
  var2 varchar2(256);
BEGIN
  var1 := 1;
  var2 := 'man';
  OPEN c1(var1);
  OPEN c1(var2);
end;
/

--cursor return rowtype
--error
DECLARE
  CURSOR c1 RETURN employees%ROWTYPE;
  CURSOR c2 RETURN c1%rowtype IS SELECT * FROM employees;
BEGIN
  OPEN c2;
  close c2;
end;
/

--raise error
DECLARE
  CURSOR c2 RETURN  employees.EMPLOYEE_ID%type IS SELECT * FROM employees;
BEGIN
  OPEN c2;
  close c2;
end;
/

--raise error
DECLARE
  CURSOR c2 RETURN  integer IS SELECT * FROM employees;
BEGIN
  OPEN c2;
  close c2;
end;
/

--raise error
DECLARE
  CURSOR c2 RETURN  integer;
BEGIN
  NULL;
end;
/

--return type is match %rowtype use query or return?
CREATE TABLE test_id_match1(id integer,name varchar2(256));
CREATE TABLE test_id_match2(id1 integer,name1 varchar2(256));

INSERT INTO test_id_match2 VALUES(1,'one');


--ok
DECLARE
  CURSOR c RETURN test_id_match1%rowtype IS SELECT * FROM test_id_match2;
  var1 c%rowtype;
BEGIN
  OPEN c;
  FETCH c INTO var1;
  raise info '%',var1.id1 || ':' || var1.name1;
  CLOSE c;
end;
/

--cursor arguments
--ok
DECLARE
  CURSOR c1(id integer DEFAULT 1,name varchar2 DEFAULT 'one') is
  SELECT * FROM test_id_match2 WHERE id1 = id AND name1 = name;
  var1 c1%rowtype;
  var2  integer;
BEGIN
  -- use all default value */
  OPEN c1;
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use a default value */
  OPEN c1(id=>1);
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use name as default value
  OPEN c1(name=>'one');
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use variable
  var2 := 1;
  OPEN c1(id=>var2);
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use default variable
  OPEN c1(1);
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use common
  OPEN c1(1,'one');
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
  --use name,id
  OPEN c1(name=>'one',id=>1);
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
end;
/

--ok
DECLARE
  CURSOR c1(id integer DEFAULT 1,name varchar2 ) is
  SELECT * FROM test_id_match2 WHERE id1 = id AND name1 = name;
  var1 c1%rowtype;
  var2  integer;
BEGIN
  OPEN c1(name=>'one');
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
end;
/

--error
DECLARE
  CURSOR c1(id integer DEFAULT 1,name varchar2 ) is
  SELECT * FROM test_id_match2 WHERE id1 = id AND name1 = name;
  var1 c1%rowtype;
  var2  integer;
BEGIN
  OPEN c1(1);
  FETCH c1 INTO var1;
  raise info '%',var1.id1 || ' ' || var1.name1;
  CLOSE c1;
end;
/

--cursor%rowtype assign another rowvar
--raise error
DECLARE
  CURSOR c1 IS SELECT * FROM employees;
  var1 c1%rowtype;
  CURSOR c2 IS SELECT * FROM test_id_match2;
BEGIN
  OPEN c2;
  FETCH c2 into var1;
  raise info '%',var1.EMPLOYEE_ID || 'xx:' || var1.MANAGER_ID;
  CLOSE c2;
end;
/

--ok
DECLARE
  CURSOR c2 IS SELECT * FROM test_id_match2;
  var1 c2%rowtype;
BEGIN
  OPEN c2;
  FETCH c2 into var1;
  raise info '%',var1.id1 || 'xx:' || var1.name1;
  CLOSE c2;
  OPEN c2;
  FETCH c2 INTO var1;
  raise info '%',var1.id1 || 'xx:' || var1.name1;
  CLOSE c2;
end;
/

--cursor variable
--ref cursor define and cursor variable define
--ok
DECLARE
  TYPE empcurtyp IS REF CURSOR RETURN employees%ROWTYPE;  -- strong type
  TYPE genericcurtyp IS REF CURSOR;                       -- weak type

  cursor1  empcurtyp;       -- strong cursor variable
  cursor2  genericcurtyp;   -- weak cursor variable
  my_cursor SYS_REFCURSOR;  -- weak cursor variable

  TYPE deptcurtyp IS REF CURSOR RETURN departments%ROWTYPE;  -- strong type
  dept_cv deptcurtyp;  -- strong cursor variable
BEGIN
  NULL;
END;
/

--
--strong ref cursor type
--
--raise error for name6
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees absd
    WHERE ROWNUM < 11
    ORDER BY last_name;
  type name1 IS ref CURSOR RETURN employees%ROWTYPE; --db_table_or_view%rowtype
  type name2 IS ref CURSOR RETURN c1%rowtype;		--cursor%rowtype;
  var1 name1;
  type name3 IS ref CURSOR RETURN var1%rowtype;		--cursor_variable%rowtype;
  type name6 IS ref CURSOR RETURN name1;			--ref_cursor_type
BEGIN
  NULL;
end;
/

--raise error for return sys_refcursor
DECLARE
  CURSOR c1 IS
    SELECT last_name FROM employees absd
    WHERE ROWNUM < 11
    ORDER BY last_name;
  type name1 IS ref CURSOR RETURN employees%ROWTYPE; --db_table_or_view%rowtype
  type name2 IS ref CURSOR RETURN c1%rowtype;		--cursor%rowtype;
  var1 name1;
  type name3 IS ref CURSOR RETURN var1%rowtype;		--cursor_variable%rowtype;
  type name6 IS ref CURSOR RETURN sys_refcursor;			--ref_cursor_type
BEGIN
  NULL;
end;
/

--raise error for ref cursor type
DECLARE
  type type1 IS ref cursor;
  type name6 IS ref CURSOR RETURN type1;			--ref_cursor_type
BEGIN
  NULL;
end;
/

--ref cursor return %rowtype
--error
DECLARE
  type c1 IS ref CURSOR RETURN employees%rowtype;
  type c2 IS ref CURSOR RETURN c1;
BEGIN
  OPEN c2 FOR SELECT * FROM employees;
end;
/


--has not return
--error
DECLARE
  type c1 IS ref CURSOR;
  var1 c1;
  type c2 IS ref CURSOR RETURN var1%rowtype;
  var2 c2;
BEGIN
  OPEN var2 FOR SELECT * FROM employees;
  CLOSE var2;
end;
/

--ok
DECLARE
  type c1 IS ref CURSOR RETURN employees%rowtype;
  var1 c1;
  type c2 IS ref CURSOR RETURN var1%rowtype;
  var2 c2;
BEGIN
  OPEN var2 FOR SELECT * FROM employees;
  CLOSE var2;
end;
/

--fetch data from cursor variable
DECLARE
  cv SYS_REFCURSOR;  -- cursor variable
  v_lastname  employees.last_name%TYPE;  -- variable for last_name
  v_jobid     employees.job_id%TYPE;     -- variable for job_id

  query_2 VARCHAR2(200) :=
    'SELECT * FROM employees
    WHERE REGEXP_LIKE (job_id, ''[ACADFIMKSA]_M[ANGR]'')
    ORDER BY job_id';

  v_employees employees%ROWTYPE;  -- record variable row of table

BEGIN
  OPEN cv FOR
    SELECT last_name, job_id FROM employees
    WHERE REGEXP_LIKE (job_id, 'S[HT]_CLERK')
    ORDER BY last_name;

  LOOP  -- Fetches 2 columns into variables
    FETCH cv INTO v_lastname, v_jobid;
    EXIT WHEN cv%NOTFOUND;
    raise info '%', RPAD(v_lastname, 25, ' ') || v_jobid;
  END LOOP;

  raise info '-------------------------------------';

  OPEN cv FOR query_2;
  LOOP  -- Fetches entire row into the v_employees record
    FETCH cv INTO v_employees;
    EXIT WHEN cv%NOTFOUND;
    raise info '%', RPAD(v_employees.last_name, 25, ' ') ||
                               v_employees.job_id;
  END LOOP;

  CLOSE cv;
END;
/

--cursor variable include variable which is
--changed before opened for doesn't change
--result cache
DECLARE
  sal           employees.salary%TYPE;
  sal_multiple  employees.salary%TYPE;
  factor        INTEGER := 2;
  cv SYS_REFCURSOR;
BEGIN
  OPEN cv FOR
    SELECT salary, salary*factor
    FROM employees
    WHERE job_id LIKE 'ST_%';   -- PL/SQL evaluates factor
  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise info '%','factor = ' || factor;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
    factor := factor + 1;  -- Does not affect sal_multiple
  END LOOP;
  CLOSE cv;
END;
/

--cursor variable include variable which is
--changed after opened for which changes
--result cache
DECLARE
  sal           employees.salary%TYPE;
  sal_multiple  employees.salary%TYPE;
  factor        INTEGER := 2;
  cv SYS_REFCURSOR;
BEGIN
  raise info '%','factor = ' || factor;
  OPEN cv FOR
    SELECT salary, salary*factor
    FROM employees
    WHERE job_id LIKE 'ST_%';   -- PL/SQL evaluates factor
  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
  END LOOP;
  factor := factor + 1;
  raise info '%','factor = ' || factor;
  OPEN cv FOR
    SELECT salary, salary*factor
    FROM employees
    WHERE job_id LIKE 'ST_%';   -- PL/SQL evaluates factor
  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise info '%','sal          = ' || sal;
    raise info '%','sal_multiple = ' || sal_multiple;
  END LOOP;
  CLOSE cv;
END;
/

--ref type as parameter type and assign it to
--out variable
CREATE OR REPLACE PACKAGE emp_data AUTHID DEFINER AS
  TYPE empcurtyp IS REF CURSOR RETURN employees%ROWTYPE;
  PROCEDURE open_emp_cv (emp_cv IN OUT empcurtyp);
END emp_data;
/

CREATE OR REPLACE PACKAGE BODY emp_data AS
  PROCEDURE open_emp_cv (emp_cv IN OUT EmpCurTyp) IS
  BEGIN
    OPEN emp_cv FOR SELECT * FROM employees ORDER BY
	EMPLOYEE_ID,last_NAME,FIRST_NAME;
  END open_emp_cv;
END emp_data;
/

--use
DECLARE
  var1 emp_data.empcurtyp;
  var2 var1%rowtype;
BEGIN
  call emp_data.open_emp_cv(var1);
  LOOP
    FETCH var1 INTO var2;
	IF var1%notfound THEN
		exit;
	END if;
	raise info '%','id :' || var2.EMPLOYEE_ID;
	raise info '%','first_name:' || var2.FIRST_NAME;
	raise info '%','last_name:' || var2.last_NAME;
  END loop;
  CLOSE var1;
end;
/

--assign different query but its return type is all
--the same
CREATE OR REPLACE PACKAGE emp_data AUTHID DEFINER AS
  TYPE empcurtyp IS REF CURSOR RETURN employees%ROWTYPE;
  PROCEDURE open_emp_cv (emp_cv IN OUT empcurtyp, choice INT);
END emp_data;
/

CREATE OR REPLACE PACKAGE BODY emp_data AS
  PROCEDURE open_emp_cv (emp_cv IN OUT empcurtyp, choice INT) IS
  BEGIN
    IF choice = 1 THEN
      OPEN emp_cv FOR SELECT *
      FROM employees
      WHERE salary IS NOT NULL ORDER BY EMPLOYEE_ID;
    ELSIF choice = 2 THEN
      OPEN emp_cv FOR SELECT *
      FROM employees
      WHERE salary > 2500 ORDER BY EMPLOYEE_ID;
    ELSIF choice = 3 THEN
      OPEN emp_cv FOR SELECT *
      FROM employees
      WHERE department_id = 100 ORDER BY EMPLOYEE_ID;
    END IF;
  END;
END emp_data;
/


--use
DECLARE
  var1 emp_data.empcurtyp;
  var2 employees%rowtype;
BEGIN
  call emp_data.open_emp_cv(var1, 1);
  raise info '%','first one';
  LOOP
    FETCH var1 INTO var2;
	EXIT WHEN var1%notfound;
	raise info '%',var2.EMPLOYEE_ID;
  END loop;
  CLOSE var1;
  call emp_data.open_emp_cv(var1, 2);
  raise info '%', 'second two';
  LOOP
    FETCH var1 INTO var2;
	EXIT WHEN var1%notfound;
	raise info '%', var2.EMPLOYEE_ID;
  END loop;
  CLOSE var1;
  call emp_data.open_emp_cv(var1, 3);
  raise info '%', 'third three';
  LOOP
    FETCH var1 INTO var2;
	EXIT WHEN var1%notfound;
	raise info '%', var2.EMPLOYEE_ID;
  END loop;
  CLOSE var1;
end;
/


--diff return type assign to same type ref cursor
CREATE OR REPLACE PACKAGE admin_data AUTHID DEFINER AS
  TYPE gencurtyp IS REF CURSOR;
  PROCEDURE open_cv (generic_cv IN OUT gencurtyp, choice INT);
END admin_data;
/

CREATE OR REPLACE PACKAGE BODY admin_data AS
  PROCEDURE open_cv (generic_cv IN OUT gencurtyp, choice INT) IS
  BEGIN
    IF choice = 1 THEN
      OPEN generic_cv FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
    ELSIF choice = 2 THEN
      OPEN generic_cv FOR SELECT * FROM departments ORDER BY DEPARTMENT_ID;
    ELSIF choice = 3 THEN
      OPEN generic_cv FOR SELECT * FROM jobs ORDER BY JOB_ID;
    END IF;
  END;
END admin_data;
/

--use
DECLARE
  var1 admin_data.gencurtyp;
  var2 employees%rowtype;
  var3 departments%rowtype;
  var4 jobs%rowtype;
BEGIN
  call admin_data.open_cv(var1, 1);
  raise info '%','first one';
  LOOP
    FETCH var1 INTO var2;
	EXIT WHEN var1%notfound;
	raise info '%', var2.EMPLOYEE_ID;
  END loop;
  CLOSE var1;
  call admin_data.open_cv(var1, 2);
  raise info '%', 'second two';
  LOOP
    FETCH var1 INTO var3;
	EXIT WHEN var1%notfound;
	raise info '%', var3.DEPARTMENT_ID;
  END loop;
  CLOSE var1;
  call admin_data.open_cv(var1, 3);
  raise info '%','third three';
  LOOP
    FETCH var1 INTO var4;
	EXIT WHEN var1%notfound;
	raise info '%', var4.JOB_ID;
  END loop;
  CLOSE var1;
end;
/


--host variable
variable emp_cv refcursor;
VAR dept_cv refcursor;
VAR loc_cv refcursor;


/* PL/SQL anonymous block in host environment */
BEGIN
  OPEN :emp_cv FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  OPEN :dept_cv FOR SELECT * FROM departments ORDER BY DEPARTMENT_ID;
  OPEN :loc_cv FOR SELECT * FROM locations ORDER BY LOCATION_ID;
END;
/

--use
DECLARE
  var1 employees%rowtype;
  var2 departments%rowtype;
  var3 locations%rowtype;
BEGIN
  FETCH :emp_cv INTO var1;
  raise info '%',var1.EMPLOYEE_ID;
  FETCH :dept_cv INTO var2;
  raise info '%', var2.DEPARTMENT_ID;
  FETCH :loc_cv INTO var3;
  raise info '%', var3.LOCATION_ID;
end;
/

/* PL/SQL anonymous block in host environment */
BEGIN
  CLOSE :emp_cv;
  CLOSE :dept_cv;
  CLOSE :loc_cv;
END;
/

variable c1 refcursor;

BEGIN
  OPEN :c1 FOR SELECT * FROM test_id_match2;
END;
/

--ok
DECLARE
  var1 sys_refcursor;
  var2 test_id_match2%rowtype;
BEGIN
  var1 := :c1;
  FETCH var1 INTO var2;
  raise info 'var2.id1 = %,var2.name = %',var2.id1,var2.name1;
  CLOSE var1;
end;
/

--raise error
BEGIN
  CLOSE :c1;
end;
/

--assign a init value
--oracle raise error, but we success
VAR var1 refcursor = '123';
VAR var1 refcursor = 123;

--ok
BEGIN
  OPEN :var1 FOR SELECT * FROM test_id_match2;
end;
/

--ok
PRINT var1

--error
PRINT var1

--ok
BEGIN
  OPEN :var1 FOR SELECT * FROM test_id_match2;
end;
/
--ok
PRINT var1;

--ok
BEGIN
  OPEN :var1 FOR SELECT * FROM test_id_match2;
end;
/

--ok
variable var1 refcursor;

--raise error
PRINT var1;

--ok
variable var1 refcursor;
variable var2 refcursor;

BEGIN
  OPEN :var1 FOR SELECT * FROM test_id_match1;
  OPEN :var2 FOR SELECT * FROM test_id_match2;
end;
/

--ok
EXEC :var1 := :var2;

PRINT var1;


--strong cursor and no strong cursor assign
--assign
--print var2 is open
--var1 is open
--var2 is again open
----fetch error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  type mds3 IS ref CURSOR RETURN locations%rowtype;
  type mds4 IS ref cursor;
  var1 mds1;
  var2 mds2;
  var3 mds3;
  var4 mds4;
  var5 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  --assign strong to no strong
  var2 := var1;
  IF var2%isopen THEN
     raise info 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info 'var1 is open';
  END if;
  CLOSE var1;
  IF var2%isopen THEN
     raise info 'var2 is again open';
	 FETCH var2 INTO var5;
	 raise info '%',var5.EMPLOYEE_ID;
	 CLOSE var2;
  END if;
end;
/

--again open
--print
--var2 is open
--var1 is open
--var2 is again open
-- then raise error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  type mds3 IS ref CURSOR RETURN locations%rowtype;
  type mds4 IS ref cursor;
  var1 mds1;
  var2 mds2;
  var3 mds3;
  var4 mds4;
  var5 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  --assign strong to no strong
  var2 := var1;
  IF var2%isopen THEN
     raise info '%','var2 is open';
  END if;
  IF var1%isopen THEN
     raise info '%','var1 is open';
  END if;
  OPEN var1 FOR SELECT * FROM employees;
  CLOSE var1;
  IF var2%isopen THEN
     raise info '%','var2 is again open';
	 FETCH var2 INTO var5;
	 raise info '%',var5.EMPLOYEE_ID;
	 CLOSE var2;
  END if;
end;
/

--fetch and again open
--ok and print
--var2 is open
--var1 is open
--3
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  type mds3 IS ref CURSOR RETURN locations%rowtype;
  type mds4 IS ref cursor;
  var1 mds1;
  var2 mds2;
  var3 mds3;
  var4 mds4;
  var5 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  --assign strong to no strong
  var2 := var1;
  IF var2%isopen THEN
     raise info 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info 'var1 is open';
  END if;
  FETCH var2 INTO var5;
  raise info '%',var5.EMPLOYEE_ID;
  OPEN var1 FOR SELECT * FROM employees;
  CLOSE var1;
  IF var2%isopen THEN
     raise info '%', 'var2 is again open';
	 FETCH var2 INTO var5;
	 raise info '%', var5.EMPLOYEE_ID;
	 CLOSE var2;
  END if;
end;
/

--error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  type mds3 IS ref CURSOR RETURN locations%rowtype;
  type mds4 IS ref cursor;
  var1 mds1;
  var2 mds2;
  var3 mds3;
  var4 mds4;
  var5 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  --assign strong to no strong
  var2 := var1;
  IF var2%isopen THEN
     raise info 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info 'var1 is open';
  END if;
  OPEN var1 FOR SELECT * FROM employees;
  CLOSE var1;
  IF var2%isopen THEN
     raise info '%', 'var2 is again open';
	 FETCH var2 INTO var5;
	 raise info '%', var5.EMPLOYEE_ID;
	 CLOSE var2;
  END if;
end;
/


--raise error
--query return type
--not the same as declare cursor return type
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  CLOSE var1;
end;
/

--include a param
--raise error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 integer;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs WHERE job_id = var2;
  CLOSE var1;
end;
/

--explicit cursor
--raise error
DECLARE
  CURSOR c1(id integer) RETURN employees%rowtype;
  CURSOR c1(id integer) RETURN employees%rowtype IS SELECT * FROM jobs WHERE job_id = id;
BEGIN
  OPEN c1(1);
  CLOSE c1;
end;
/

--again open will close origin ?
--print fetch var1,xxx,again fetch var1
--xx,fetch var2,xxx,close var1,close var2
--then raise error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 mds1;
  var3 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info 'fetch var1';
  raise info '%',var3.EMPLOYEE_ID;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info '%','again fetch var1';
  raise info '%', var3.EMPLOYEE_ID;
  FETCH var2 INTO var3;
  raise info '%','fetch var2';
  raise info '%', var3.EMPLOYEE_ID;
  raise info '%','close var1';
  CLOSE var1;
  raise info '%', 'close var2';
  CLOSE var2;
end;
/

--ok
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 mds1;
  var3 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info '%','fetch var1';
  raise info '%', var3.EMPLOYEE_ID;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info '%','again fetch var1';
  raise info '%',var3.EMPLOYEE_ID;
  FETCH var2 INTO var3;
  raise info '%', 'fetch var2';
  raise info '%', var3.EMPLOYEE_ID;
  raise info '%','close var1';
  CLOSE var1;
  IF var2%isopen then
	raise info '%', 'close var2';
	CLOSE var2;
  END if;
end;
/

--strong and no strong type
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 sys_refcursor;
  var3 var1%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info '%','fetch var1';
  raise info '%',var3.EMPLOYEE_ID;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  FETCH var1 INTO var3;
  raise info '%','again fetch var1';
  raise info '%', var3.EMPLOYEE_ID;
  FETCH var2 INTO var3;
  raise info '%','fetch var2';
  raise info '%', var3.EMPLOYEE_ID;
  raise info '%', 'close var1';
  CLOSE var1;
  IF var2%isopen then
	raise info '%', 'close var2';
	CLOSE var2;
  END if;
end;
/

--not close origin
--but var2 is pointer to new
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 jobs%rowtype;
  var4 employees%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM jobs ORDER BY job_id;
  LOOP
    FETCH var2 INTO var4;
	EXIT WHEN var2%notfound;
	raise info '%',var3.job_id;
 END loop;
 FETCH var1 INTO var4;
 IF var1%notfound THEN
	raise info '%','var1 not found';
 END if;
 CLOSE var1;
 IF var2%isopen THEN
   raise info '%', 'var2 is open';
   CLOSE var2; --raise error
 END if;
end;
/

INSERT INTO jobs VALUES(1,'yanfa');
INSERT INTO jobs VALUES(2,'guanli');
INSERT INTO jobs VALUES(3,'xiaoshou');


--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 jobs%rowtype;
  var4 employees%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM jobs ORDER BY job_id;
  LOOP
    FETCH var2 INTO var3;
	EXIT WHEN var2%notfound;
	raise info '%',var3.job_id;
 END loop;
 FETCH var1 INTO var3;
 IF var1%notfound THEN
	raise info '%','var1 not found';
 END if;
 CLOSE var1;
 IF var2%isopen THEN
   raise info '%', 'var2 is open';
   CLOSE var2; --raise error
 END if;
end;
/

--different strong type not allow to assign
--error
DECLARE
  type mds IS ref CURSOR RETURN employees%rowtype;
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds;
  var2 mds1;
BEGIN
  var2 := var1;
end;
/

--no strong type var assign to strong type
--ok
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 sys_refcursor;
BEGIN
  var1 := var2;
end;
/

--ok
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 sys_refcursor;
BEGIN
  OPEN var2 FOR SELECT * FROM employees;
  var1 := var2;
end;
/

--ok
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 sys_refcursor;
  var3 var1%rowtype;
BEGIN
  OPEN var2 FOR SELECT * FROM employees ORDER BY EMPLOYEE_ID;
  var1 := var2;
  FETCH var1 INTO var3;
  raise info '%',var3.EMPLOYEE_ID;
  CLOSE var1;
end;
/


--wrong result type assign
--error
DECLARE
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  var1 mds1;
  var2 sys_refcursor;
  var3 jobs%rowtype;
BEGIN
  raise info '%','assign';
  OPEN var2 FOR SELECT * FROM jobs ORDER BY job_id;
  var1 := var2;
end;
/

--subproc paramter
--strong paramster accept weak ref cursor
--query not match
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var5 integer;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var4;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  var5 := test_f1(var1);
  CLOSE var1;
end;
/

--ok
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var5 integer;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var4;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM employees;
  var5 := test_f1(var1);
  CLOSE var1;
end;
/

--weak parameter as strong or weak
DECLARE
  type mds IS ref cursor;
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  var1 integer;
  var2 mds;
  var3 mds2;
  var4 mds1;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
     IF id%isopen THEN
        raise info '%','isopen';
     END if;
     RETURN 1;
  end;
 BEGIN
   OPEN var2 FOR SELECT * FROM jobs;
   var1 := test_f(var2);
   var1 := test_f(var3);
   OPEN var4 FOR SELECT * FROM employees;
   var1 := test_f(var4);
   CLOSE var3;
   CLOSE var4;
 end;
 /

--weak parameter as strong or weak
--ok
DECLARE
  type mds IS ref cursor;
  type mds1 IS ref CURSOR RETURN employees%rowtype;
  type mds2 IS ref cursor;
  var1 integer;
  var2 mds;
  var3 mds2;
  var4 mds1;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
     IF id%isopen THEN
        raise info '%', 'isopen';
     END if;
     RETURN 1;
  end;
 BEGIN
   OPEN var2 FOR SELECT * FROM jobs;
   var1 := test_f(var2);
   var1 := test_f(var3);
   OPEN var4 FOR SELECT * FROM employees;
   var1 := test_f(var4);
   CLOSE var2;
   CLOSE var4;
 end;
 /

--weak function reload
--error
DECLARE
  type mds IS ref cursor;
  type mds1 IS ref cursor;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
    IF id%isopen THEN
        raise info '%','isopen';
	END if;
	raise info '%','invoke param mds func';
	RETURN 1;
  end;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%', 'invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var1);
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /

 --one weak and one strong function reload
 --invoke error
DECLARE
  type mds IS ref cursor;
  type mds1 IS ref CURSOR RETURN jobs%rowtype;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
    IF id%isopen THEN
        raise info '%','isopen';
	END if;
	raise info '%','invoke param mds func';
	RETURN 1;
  end;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%','invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var1);
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /

 --raise error
 DECLARE
  type mds IS ref cursor;
  type mds1 IS ref CURSOR RETURN jobs%rowtype;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
    IF id%isopen THEN
        raise info '%','isopen';
	END if;
	raise info '%','invoke param mds func';
	RETURN 1;
  end;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%','invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /

 --two strong cursor type
 --ok
DECLARE
  type mds IS ref CURSOR RETURN jobs%rowtype;
  type mds1 IS ref CURSOR RETURN jobs%rowtype;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds) RETURN integer IS
  BEGIN
    IF id%isopen THEN
        raise info '%','isopen';
	END if;
	raise info '%','invoke param mds func';
	RETURN 1;
  end;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%','invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var1);
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /


--error
DECLARE
  type mds IS ref CURSOR RETURN jobs%rowtype;
  type mds1 IS ref CURSOR RETURN jobs%rowtype;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%','invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var1);
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /

 --ok
DECLARE
  type mds IS ref CURSOR;
  type mds1 IS ref CURSOR RETURN jobs%rowtype;
  var1 mds;
  var2 mds1;
  var3 integer;
  FUNCTION test_f(id mds1) RETURN integer IS
  begin
     IF id%isopen THEN
        raise info '%','isopen';
	 END if;
	 raise info '%', 'invoke param mds1 func';
	 RETURN 2;
  end;
 BEGIN
   OPEN var1 FOR SELECT * FROM jobs;
   OPEN var2 FOR SELECT * FROM jobs;
   var3 := test_f(var1);
   var3 := test_f(var2);
   CLOSE var1;
   CLOSE var2;
 end;
 /

--error
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 type mds2 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var2 mds1;
 var5 integer;
 var6 mds2;
 FUNCTION test_f(id mds) RETURN integer IS
   var3 id%rowtype;
 BEGIN
   FETCH id INTO var3;
   RETURN 1;
 end;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var3;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  OPEN var2 FOR SELECT * FROM employees;
  OPEN var6 FOR SELECT * FROM employees;
  var5 := test_f(var1);
  var5 := test_f(var2);
  raise info '%','no strong ref cursor ok';
  raise info '%', 'accept no strong ref cursor';
  var5 := test_f1(var1);
  raise info '%','accept the same strong ref cursor';
  var5 := test_f1(var2);
  raise info '%','accept not the same strong ref cursor';
  var5 := test_f1(var6);
  CLOSE var1;
  CLOSE var2;
  CLOSE var6;
end;
/

--error
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 type mds2 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var2 mds1;
 var5 integer;
 var6 mds2;
 FUNCTION test_f(id mds) RETURN integer IS
 BEGIN
   RETURN 1;
 end;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var4;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  OPEN var2 FOR SELECT * FROM employees;
  OPEN var6 FOR SELECT * FROM employees;
  var5 := test_f(var1);
  var5 := test_f(var2);
  raise info '%', 'no strong ref cursor ok';
  raise info '%','accept no strong ref cursor';
  var5 := test_f1(var1);
  raise info '%','accept the same strong ref cursor';
  var5 := test_f1(var2);
  raise info '%','accept not the same strong ref cursor';
  var5 := test_f1(var6);
  CLOSE var1;
  CLOSE var2;
  CLOSE var6;
end;
/

--raise error
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 type mds2 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var2 mds1;
 var5 integer;
 var6 mds2;
 FUNCTION test_f(id mds) RETURN integer IS
 BEGIN
   RETURN 1;
 end;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var4;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  OPEN var2 FOR SELECT * FROM employees;
  OPEN var6 FOR SELECT * FROM employees;
  var5 := test_f(var1);
  var5 := test_f(var2);
  raise info '%','no strong ref cursor ok';
  raise info '%','accept no strong ref cursor';
  var5 := test_f1(var1);
  raise info '%','accept the same strong ref cursor';
  var5 := test_f1(var2);
  CLOSE var1;
  CLOSE var2;
  CLOSE var6;
end;
/

--ok
DECLARE
 type mds IS ref cursor;
 type mds1 IS ref CURSOR RETURN employees%rowtype;
 type mds2 IS ref CURSOR RETURN employees%rowtype;
 var1 mds;
 var2 mds1;
 var5 integer;
 var6 mds2;
 FUNCTION test_f(id mds) RETURN integer IS
 BEGIN
   RETURN 1;
 end;
 FUNCTION test_f1(id mds1) RETURN integer IS
   var4 id%rowtype;
 BEGIN
   FETCH id INTO var4;
   RETURN 2;
 end;
BEGIN
  OPEN var1 FOR SELECT * FROM employees;
  OPEN var2 FOR SELECT * FROM employees;
  OPEN var6 FOR SELECT * FROM employees;
  var5 := test_f(var1);
  var5 := test_f(var2);
  raise info '%','no strong ref cursor ok';
  raise info '%','accept no strong ref cursor';
  var5 := test_f1(var1);
  raise info '%','accept the same strong ref cursor';
  var5 := test_f1(var2);
  CLOSE var1;
  CLOSE var2;
  CLOSE var6;
end;
/


--for loop statment
--implicit cursor for loop
BEGIN
  FOR item IN (
    SELECT last_name, job_id
    FROM employees
    ORDER BY last_name,job_id
  )
  LOOP
    raise info '%', 'Name = ' || item.last_name || ', Job = ' || item.job_id;
  END LOOP;
END;
/


--explicit cursor for loop
--ok
DECLARE
  CURSOR c1 IS
    SELECT last_name, job_id FROM employees
    ORDER BY last_name, job_id;
BEGIN
  FOR item IN c1
  LOOP
    raise info '%','Name = ' || item.last_name || ', Job = ' || item.job_id;
  END LOOP;
END;
/


--for loop explicit cursor which include paramter
--ok
DECLARE
  CURSOR c1 (job VARCHAR2, max_wage NUMBER) IS
    SELECT * FROM employees
    WHERE job_id = job
    AND salary > max_wage;
BEGIN
  FOR person IN c1('ST_CLERK', 3000)
  LOOP
     -- process data record
    raise info '%',
      'Name = ' || person.last_name || ', salary = ' ||
      person.salary || ', Job Id = ' || person.job_id;
  END LOOP;
END;
/

--ok
BEGIN
  FOR item IN (
    SELECT first_name || ' ' || last_name AS full_name,
           salary * 10                    AS dream_salary
    FROM employees
    WHERE ROWNUM <= 5
    ORDER BY dream_salary DESC, last_name ASC
  ) LOOP
   raise info '%', item.full_name || ' dreams of making ' || item.dream_salary;
  END LOOP;
END;
/


--cursor variable assign
--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
  var4 varchar2(256);
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  FETCH var2 INTO var3;
  raise info '%','job_id := ' || var3;
  OPEN var1 FOR SELECT JOB_TITLE FROM jobs ORDER BY job_id;
  FETCH var2 INTO var4;
  raise info '%','job_title = ' || var4;
  CLOSE var2;
end;
/

--decide isopen attribute for cursor variable
--assign
--print
----var2 is open
----var1 is open
----after close var1
-----var2 is open
--then raise error
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
  var4 varchar2(256);
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var2%isopen THEN
    raise info '%','var2 is open';
  END if;
  IF var1%isopen THEN
    raise info '%','var1 is open';
  END if;
  CLOSE var1;
  raise info '%', 'after close var1';
  IF var1%isopen THEN
     raise info '%','var1 is open';
	 CLOSE var1;
  END if;
  IF var2%isopen THEN
     raise info '%','var2 is open';
	 CLOSE var2;
  END if;
end;
/


--exchage var1 and var2
--print
------var2 is open
------var1 is open
------after close var2
------var1 is open
--then raise error
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
  var4 varchar2(256);
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var2%isopen THEN
    raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
    raise info '%', 'var1 is open';
  END if;
  CLOSE var2;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
	 CLOSE var2;
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
	 CLOSE var1;
  END if;
end;
/


--fetch var1
--ok and print
---var2 is open
---var1 is open
---after close var2
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
  var4 varchar2(256);
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var2%isopen THEN
    raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
    raise info '%', 'var1 is open';
  END if;
  FETCH var1 INTO var3;
  CLOSE var2;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
	 CLOSE var2;
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
	 CLOSE var1;
  END if;
end;
/

--fetch var2
--print
-----var2 is open
-----var1 is open
-----after close var2
-----var1 is open
--raise error
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
  var4 varchar2(256);
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var2%isopen THEN
    raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
    raise info '%', 'var1 is open';
  END if;
  FETCH var2 INTO var3;
  CLOSE var2;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
	 CLOSE var2;
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
	 CLOSE var1;
  END if;
end;
/


--maybe isnull
--ok and print
--after close var2
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  CLOSE var2;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
  END if;
  IF var2%isopen IS NULL THEN
     raise info '%', 'var2%isopen is null';
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
  END if;
  IF var1%isopen IS null THEN
     raise info '%', 'var1%isopen is open';
  END if;
end;
/

--ok and print
--after close var2
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  CLOSE var2;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info '%','var1 is open';
  END if;
end;
/

--ok and print
--after close var2
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  CLOSE var1;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
  END if;
end;
/


--ok
--and print
----var1 is open
----var2 is open
----after close var2
----var2 is open
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var1%isopen THEN
      raise info '%','var1 is open';
  END if;
  IF var2%isopen THEN
      raise info '%', 'var2 is open';
  END if;
  CLOSE var1;
  raise info '%','after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
  END if;
end;
/


--reference has felected
--ok and print
-----var1 is open
-----after close var2
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 number;
BEGIN
  OPEN var1 FOR SELECT job_id FROM jobs ORDER BY job_id;
  var2 := var1;
  IF var1%isopen THEN
      raise info '%', 'var1 is open';
  END if;
  CLOSE var1;
  raise info '%', 'after close var2';
  IF var2%isopen THEN
     raise info '%', 'var2 is open';
  END if;
  IF var1%isopen THEN
     raise info '%', 'var1 is open';
  END if;
end;
/


--loop open cursor
--ok
DELETE FROM jobs WHERE job_id = 'meigong';
--ok
DECLARE
  id integer;
  ret integer;
  FUNCTION test_f(id varchar2) RETURN integer IS
    var1 sys_refcursor;
	var2 varchar2(256);
	var3 varchar2(256);
  BEGIN
    var2 := 'select job_id from jobs where job_id = ' || id;
    OPEN var1 FOR var2;
	FETCH var1 INTO var3;
	RETURN id;
 end;
BEGIN
  FOR id IN 1 .. 1000 LOOP
    ret := test_f(id);
  END loop;
end;
/


--out variable
--raise error
--maximum open cursors exceeded
DECLARE
  id integer;
  ret integer;
  var3 sys_refcursor;
  FUNCTION test_f(id integer, cur out sys_refcursor) RETURN integer IS
    var1 sys_refcursor;
	var2 varchar2(256);
  BEGIN
    var2 := 'select job_id from jobs where job_id = ' || id;
    OPEN var1 FOR var2;
	FETCH var1 INTO ret;
	cur := var1;
	RETURN id;
 end;
BEGIN
  FOR id IN 1 .. 1000 LOOP
    ret := test_f(id, var3);
  END loop;
end;
/


--ret
--raise error
--maximum open cursors exceeded
DECLARE
  id integer;
  ret integer;
  var3 sys_refcursor;
  FUNCTION test_f(id integer) RETURN sys_refcursor IS
    var1 sys_refcursor;
	var2 varchar2(256);
  BEGIN
    var2 := 'select job_id from jobs where job_id = ' || id;
    OPEN var1 FOR var2;
	FETCH var1 INTO ret;
	RETURN var1;
 end;
BEGIN
  FOR id IN 1 .. 1000 LOOP
    var3 := test_f(id);
  END loop;
end;
/


--explicit cursor
--ok
DECLARE
  id integer;
  ret integer;
  FUNCTION test_f(id integer) RETURN integer IS
    CURSOR c IS SELECT job_id FROM jobs WHERE job_id = id;
	var2 varchar2(256);
  BEGIN
    OPEN c;
	FETCH c INTO var2;
    raise info '%', 'var2 = ' || var2;
	RETURN id;
 end;
BEGIN
  FOR id IN 1 .. 60 LOOP
    ret := test_f(id);
  END loop;
end;
/


--the same name as explicit cursor and varname
--error
DECLARE
  CURSOR c1 IS SELECT * FROM jobs;
  c1 integer;
BEGIN
  OPEN c1;
end;
/

--ok
DECLARE
  CURSOR c1 IS SELECT * FROM jobs;
BEGIN
  OPEN c1;
  CLOSE c1;
end;
/

--oracle error,but we success
DECLARE
  CURSOR c3 RETURN departments%ROWTYPE;
  CURSOR c3 IS
    SELECT * FROM departments
    WHERE department_id = '1';
BEGIN
  NULL;
END;
/


--oracle error,but we success
DECLARE
  CURSOR c3 RETURN departments%ROWTYPE;

  CURSOR c3 IS
    SELECT * FROM departments
    WHERE department_id = '1';
BEGIN
  OPEN c3;
  CLOSE c3;
END;
/

--ok
DECLARE
  CURSOR c3 RETURN departments%ROWTYPE;

  CURSOR c3 RETURN departments%ROWTYPE IS
    SELECT * FROM departments
    WHERE department_id = '1';
BEGIN
  OPEN c3;
  CLOSE c3;
END;
/


DELETE FROM departments;

insert into departments values(1,'dept1',10);


--ok
DECLARE
  CURSOR c1 IS SELECT * FROM departments ORDER BY department_id;
  var1 departments%rowtype;
BEGIN
  OPEN c1;
  INSERT INTO departments VALUES(2, 'dept2', 12);
  commit;
  LOOP
    FETCH c1 INTO var1;
	EXIT WHEN c1%notfound;
	raise info '%', 'department_id :' || var1.department_id;
  END loop;
 end;
 /


 --again
 --ok and print
---department_id :1
---department_id :2
DECLARE
  CURSOR c1 IS SELECT * FROM departments ORDER BY department_id;
  var1 departments%rowtype;
BEGIN
  OPEN c1;
  INSERT INTO departments VALUES(2, 'dept2', 12);
  commit;
  LOOP
    FETCH c1 INTO var1;
	EXIT WHEN c1%notfound;
	raise info '%', 'department_id :' || var1.department_id;
  END loop;
  CLOSE c1;
 end;
 /

--ok and print
-----department_id :1
-----rowcount :1
-----department_id :2
-----rowcount :2
-----department_id :2
-----rowcount :3
 DECLARE
  CURSOR c1 IS SELECT * FROM departments ORDER BY department_id;
  var1 departments%rowtype;
BEGIN
  OPEN c1;
  INSERT INTO departments VALUES(2, 'dept2', 12);
  commit;
  LOOP
    FETCH c1 INTO var1;
	EXIT WHEN c1%notfound;
	raise info '%', 'department_id :' || var1.department_id;
	raise info '%', 'rowcount :' || c1%rowcount;
  END loop;
  CLOSE c1;
 end;
 /


--implicit cursor attribute is global?
DROP FUNCTION test_f;
CREATE OR REPLACE FUNCTION test_f(id integer) RETURN integer IS
BEGIN
  IF sql%found THEN
     raise info '%', 'sql%found';
  END if;
  raise info '%', 'sql%rowcount :' || sql%rowcount;
  RETURN 1;
end;
/


--ok
--print
-----out sql%found
-----out sql%rowcount :4
-----sql%found
-----sql%rowcount :4
DECLARE
  var1 integer;
BEGIN
  DELETE FROM departments;
  IF sql%found THEN
     raise info '%','out sql%found';
  END if;
  raise info '%', 'out sql%rowcount :' || sql%rowcount;
  var1 := test_f(23);
end;
/


--ok
SELECT test_f(23) FROM dual;

--ok
DECLARE
  isopen integer;
  found integer;
  notfound integer;
  ROWCOUNT integer;
  bulk_rowcount integer;
  bulk_exceptions integer;
BEGIN
  isopen := 1;
  found := 1;
  notfound := 1;
  ROWCOUNT := 1;
  bulk_rowcount := 2;
  bulk_exceptions := 3;
end;
/

--can be assignable
CREATE TABLE test_insert(id integer);

--raise error
DECLARE
  var1 integer;
BEGIN
   INSERT INTO test_insert VALUES(1);
   sql%ROWCOUNT := 1;
end;
/


--raise error
DECLARE
  CURSOR c1 IS SELECT * FROM test_insert;
BEGIN
  OPEN c1;
  c1%ROWCOUNT := 2;
  CLOSE c1;
end;
/


--as subprogram paramster
--ok
DECLARE
  var1 integer;
  FUNCTION test_f(id number) RETURN integer IS
  BEGIN
     RETURN id;
  end;
BEGIN
  DELETE FROM test_insert;
  var1 := test_f(sql%rowcount);
end;
/


DECLARE
  var1 integer;
  FUNCTION test_f(id out number) RETURN integer IS
  BEGIN
     id := 2;
     RETURN id;
  end;
BEGIN
  DELETE FROM test_insert;
  var1 := test_f(sql%rowcount);
  raise info '%',sql%rowcount;
end;
/


CREATE OR REPLACE FUNCTION test_f(id number) RETURN integer IS
BEGIN
  RETURN id;
end;
/


variable x refcursor;

begin
	open :x for select * from jobs;
end;
/

--raise error
SELECT test_f(:x%rowcount) FROM dual;

--ok
BEGIN
  raise info '%', :x%rowcount;
end;
/

--ok
BEGIN
  CLOSE :x;
end;
/

--explicit cursor use default value
--ok
DECLARE
  CURSOR c(id IN integer := 1) IS SELECT * FROM jobs WHERE job_id = id;
  var1 c%rowtype;
BEGIN
  OPEN c;
  FETCH c INTO var1;
  raise info '%', 'c job_id:' || var1.job_id;
  CLOSE c;
  OPEN c(3);
  FETCH c INTO var1;
  raise info '%', 'c(3) job_id:' || var1.job_id;
  CLOSE c;
end;
/

--error
DECLARE
  CURSOR c(id IN integer := 1,
     title varchar2) IS SELECT * FROM jobs WHERE job_id = id AND JOB_TITLE = title;
  var1 c%rowtype;
BEGIN
  OPEN c('yanfa');
  FETCH c INTO var1;
  raise info '%', 'c(yanfa) job_id:' || var1.job_id;
  CLOSE c;
end;
/


--ok
DECLARE
  CURSOR c(id IN integer := 1,
     title varchar2) IS SELECT * FROM jobs WHERE job_id = id AND JOB_TITLE = title;
  var1 c%rowtype;
begin
  OPEN c(title=>'yanfa');
  FETCH c INTO var1;
  raise info '%', 'c(title=>yanfa) job_id:' || var1.job_id;
  CLOSE c;
end;
/
--error
DECLARE
  CURSOR c(id IN integer := 1,
     title varchar2) IS SELECT * FROM jobs WHERE job_id = id AND JOB_TITLE = title;
  var1 c%rowtype;
begin
  OPEN c(title=>'yanfa', 1);
  FETCH c INTO var1;
  raise info '%', 'c(title=>yanfa) job_id:' || var1.job_id;
  CLOSE c;
end;
/


--ok
DECLARE
  CURSOR c(id IN integer := 1,
     title varchar2) IS SELECT * FROM jobs WHERE job_id = id AND JOB_TITLE = title;
  var1 c%rowtype;
begin
  OPEN c(1,title=>'yanfa');
  FETCH c INTO var1;
  raise info '%', 'c(title=>yanfa) job_id:' || var1.job_id;
  CLOSE c;
end;
/

CREATE TABLE test_init(id integer);

INSERT INTO test_init VALUES(1);


--package init block will change sql%attribute
CREATE OR REPLACE package test_init_pkg IS
  var1 integer := 26;
end;
/

CREATE OR REPLACE package body test_init_pkg IS
BEGIN
  UPDATE test_init SET id = var1 + 1;
  raise info '%', 'init block';
  raise info 'rowucount = %',sql%rowcount;
end;
/

--ok and print
-----before init block
-----rowcount = <NULL>
-----init block
-----rowucount = 1
-----after init block
-----rowcount = 1
BEGIN
  raise info '%', 'before init block';
  raise info 'rowcount = %', sql%rowcount;
  test_init_pkg.var1 := 26;
  raise info '%', 'after init block';
  raise info 'rowcount = %', sql%rowcount;
end;
/


--again and print
-----before init block
-----rowcount = <NULL>
-----after init block
-----rowcount = <NULL>
BEGIN
  raise info '%', 'before init block';
  raise info 'rowcount = %', sql%rowcount;
  test_init_pkg.var1 := 26;
  raise info '%', 'after init block';
  raise info 'rowcount = %', sql%rowcount;
end;
/

--when raise error, which is success
DECLARE
  var1 integer;
  FUNCTION test_f(ids integer) return integer IS
    var1 integer;
  BEGIN
     UPDATE test_init SET id = ids + 1;
	 raise info '%','update';
	 raise info 'rowcount = %', sql%rowcount;
	 var1 := 1 / 0;
	 RETURN ids;
  end;
BEGIN
  raise info '%', 'before test_f';
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_f(23);
  raise info '%','after test_f';
  raise info 'rowcount = %', sql%rowcount;
exception
  WHEN others THEN
      raise info '%', 'exception';
      raise info 'rowcount = %',sql%rowcount;
end;
/

--again
DECLARE
  var1 integer;
  FUNCTION test_f(ids integer) return integer IS
    var1 integer;
  BEGIN
     UPDATE test_init SET id = ids + 1;
	 raise info '%','update';
	 raise info 'rowcount = %', sql%rowcount;
	 var1 := 1 / 0;
	 RETURN ids;
  end;
BEGIN
  raise info '%', 'before test_f';
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_f(23);
  raise info '%','after test_f';
  raise info 'rowcount = %', sql%rowcount;
exception
  WHEN others THEN
      raise info '%', 'exception';
      raise info 'rowcount = %',sql%rowcount;
end;
/


--error
DECLARE
  var1 integer;
  FUNCTION test_f(ids integer) return integer IS
    var1 integer;
  BEGIN
     UPDATE test_init SET id = ids + 1;
	 raise info '%', 'update';
	 raise info 'rowcount = %', sql%rowcount;
	 var1 := 1 / 0;
	 RETURN ids;
  end;
BEGIN
  raise info '%', 'before test_f';
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_f(23);
  raise info 'after test_f';
  raise info 'rowcount = %', sql%rowcount;
end;
/


--again
DECLARE
  var1 integer;
  FUNCTION test_f(ids integer) return integer IS
    var1 integer;
  BEGIN
     UPDATE test_init SET id = ids + 1;
	 raise info '%', 'update';
	 raise info 'rowcount = %', sql%rowcount;
	 var1 := 1 / 0;
	 RETURN ids;
  end;
BEGIN
  raise info '%', 'before test_f';
  raise info 'rowcount = %', sql%rowcount;
  var1 := test_f(23);
  raise info 'after test_f';
  raise info 'rowcount = %', sql%rowcount;
end;
/

--after assign var,which is closed?
CREATE TABLE test_insert1(id integer);
CREATE TABLE test_jobs(id integer);

INSERT INTO test_insert1 VALUES(5);
INSERT INTO test_insert1 VALUES(6);
INSERT INTO test_insert1 VALUES(7);
INSERT INTO test_jobs VALUES(1);
INSERT INTO test_jobs VALUES(2);
INSERT INTO test_jobs VALUES(3);

--oracle raise error, but we ok and print
-----var3:var4:5
-----var2:var4:1
-----var1:var4:2
-----close var3
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 sys_refcursor;
  var4 integer;
BEGIN
  OPEN var1 FOR SELECT * FROM test_jobs;
  OPEN var2 FOR SELECT * FROM test_insert1;
  var3 := var2;
  var2 := var1;
  --var3 is open or close and pointer which
  IF var3%isopen THEN
     FETCH var3 INTO var4;
	 raise info '%', 'var3:var4:' || var4;
  END if;
  IF var2%isopen THEN
    FETCH var2 INTO var4;
	raise info '%', 'var2:var4:' || var4;
  END if;
  IF var1%isopen THEN
    FETCH var1 INTO var4;
	raise info '%', 'var1:var4:' || var4;
  END if;
  CLOSE var1;
  IF var2%isopen THEN
     raise info '%', 'close var2';
     CLOSE var2;
  END if;
  IF var3%isopen THEN
     raise info '%', 'close var3';
     CLOSE var3;
  END if;
end;
/

--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 sys_refcursor;
  var4 integer;
BEGIN
  OPEN var1 FOR SELECT * FROM test_jobs;
  OPEN var2 FOR SELECT * FROM test_insert1;
  var3 := var2;
  var2 := var1;
  --var3 is open or close and pointer which
  IF var3%isopen THEN
     FETCH var3 INTO var4;
	 raise info '%', 'var3:var4:' || var4;
  END if;
  IF var2%isopen THEN
    FETCH var2 INTO var4;
	raise info '%', 'var2:var4:' || var4;
  END if;
  IF var1%isopen THEN
    FETCH var1 INTO var4;
	raise info '%', 'var1:var4:' || var4;
  END if;
  CLOSE var1;
end;
/


--repeate open for
--ok and print
----before repate open for var3:var4:1
----var1:var4:5
----var3:var4:6
DECLARE
  var1 sys_refcursor;
  var3 sys_refcursor;
  var4 integer;
BEGIN
  OPEN var1 FOR SELECT * FROM test_jobs;
  var3 := var1;
  --var3 is open or close and pointer which
  IF var3%isopen THEN
     FETCH var3 INTO var4;
	 raise info '%', 'before repate open for var3:var4:' || var4;
  END if;
  OPEN var1 FOR SELECT * FROM test_insert1;
  IF var1%isopen THEN
    FETCH var1 INTO var4;
	raise info '%', 'var1:var4:' || var4;
  END if;

  IF var3%isopen THEN
     FETCH var3 INTO var4;
	 raise info '%', 'var3:var4:' || var4;
  END if;
  CLOSE var1;
end;
/

--error
DECLARE
  var1 integer;
BEGIN
  UPDATE test_insert SET id = 2;
  INSERT INTO test_insert VALUES(sql%rowcount);
end;
/

--ok
DECLARE
  var1 integer;
BEGIN
  UPDATE test_insert SET id = 2;
  raise info '%', 'sql%rowcount:' || sql%rowcount;
  --INSERT INTO test_insert VALUES(sql%rowcount);
end;
/

--error
DECLARE
  var1 integer;
BEGIN
  UPDATE test_insert SET id = 2;
  SELECT COUNT(*) INTO var1 FROM test_insert WHERE id = sql%ROWCOUNT;
end;
/

--ok
DECLARE
  var1 integer;
BEGIN
  UPDATE test_insert SET id = 2;
  raise info '%', 'sql%rowcount:' || sql%rowcount;
  var1 := sql%rowcount;
  SELECT COUNT(*) INTO var1 FROM test_insert WHERE id = var1;
end;
/

--error
DECLARE
  var1 integer;
  CURSOR c1 IS SELECT * FROM test_insert;
  var2 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var2;
  SELECT COUNT(id) INTO var1 FROM test_insert WHERE id = c1%rowcount;
end;
/

--ok
DECLARE
  var1 integer;
  CURSOR c1 IS SELECT * FROM test_insert;
  var2 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var2;
  var1 := c1%rowcount;
  SELECT COUNT(id) INTO var1 FROM test_insert WHERE id = var1;
end;
/

--explicit cursor as params
--error
DECLARE
  var1 integer;
  CURSOR c1 IS SELECT * FROM test_insert;
  var2 c1%rowtype;
  FUNCTION test_f(cur sys_refcursor) RETURN integer IS
  BEGIN
    FETCH cur INTO var2;
    RETURN 1;
  end;
BEGIN
  OPEN c1;
  FETCH c1 INTO var2;
  var1 := test_f(c1);
end;
/

--ok
DECLARE
  var1 integer;
  c1 sys_refcursor;
  var2 test_insert%rowtype;
  FUNCTION test_f(cur sys_refcursor) RETURN integer IS
  BEGIN
    FETCH cur INTO var2;
    RETURN 1;
  end;
BEGIN
  OPEN c1 FOR SELECT * FROM test_insert;
  FETCH c1 INTO var2;
  var1 := test_f(c1);
end;
/


--package
CREATE OR REPLACE package test_pkg IS
  CURSOR c1 IS SELECT * FROM test_insert;
  type c2 IS ref cursor;
end;
/

--error
DECLARE
  var1 sys_refcursor;
BEGIN
  var1 := test_pkg.c1;
end;
/


--package define a ref cursor type
create or replace package test_pkg is
  type c_type is ref cursor;
end;
/

create or replace function test_f(cur test_pkg.c_type) return integer is
begin
  if cur%isopen then
    raise info '%','cur is open';
  end if;
  return 1;
end;
/

--ok
declare
  type c_type is ref cursor;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from test_insert;
  var2 := test_f(var1);
end;
/

drop function test_f(cur test_pkg.c_type);

create or replace package test_pkg is
  type c_type is ref CURSOR RETURN test_insert%rowtype;
end;
/


create or replace function test_f(cur test_pkg.c_type) return integer is
begin
  if cur%isopen then
    raise info '%','cur is open';
  end if;
  return 1;
end;
/

--ok
declare
  type c_type is ref cursor;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from test_insert;
  var2 := test_f(var1);
end;
/

--oracle ok,but we failed
declare
  type c_type is ref cursor;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from jobs;
  var2 := test_f(var1);
end;
/

--use fetch
create or replace function test_f1(cur test_pkg.c_type) return integer IS
  var1 test_insert%rowtype;
begin
  if cur%isopen then
    raise info '%','cur is open';
	FETCH cur INTO var1;
  end if;
  return 1;
end;
/


declare
  type c_type is ref cursor;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from jobs;
  var2 := test_f1(var1);
end;
/

DROP FUNCTION test_f1;

--error
declare
  type c_type is ref CURSOR RETURN test_insert%rowtype;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from test_insert;
  var2 := test_f(var1);
end;
/


create or replace package test_pkg is
  type c_type is ref CURSOR;
end;
/

--ok
declare
  type c_type is ref CURSOR RETURN test_insert%rowtype;
  var1 c_type;
  var2 integer;
begin
  open var1 for select * from test_insert;
  var2 := test_f(var1);
end;
/


--error
DECLARE
  type c_type IS ref cursor;
  type c_type1 IS ref CURSOR RETURN test_insert%rowtype;
  var1 c_type;
  var2 c_type1;
  var3 integer;
  FUNCTION test_f(cur c_type) RETURN integer IS
  BEGIN
     raise info '%', 'invoke test_f(c_type)';
	 RETURN 1;
  end;
  FUNCTION test_f(cur c_type1) RETURN integer IS
  BEGIN
    raise info '%','invoke test_f(c_type1)';
	RETURN 2;
  end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  OPEN var2 FOR SELECT * FROM test_insert;
  var3 := test_f(var1);
  var3 := test_f(var2);
end;
/


--ok
DECLARE
  type c_type IS ref CURSOR RETURN jobs%rowtype;
  type c_type1 IS ref CURSOR RETURN test_insert%rowtype;
  var1 c_type;
  var2 c_type1;
  var3 integer;
  FUNCTION test_f(cur c_type) RETURN integer IS
  BEGIN
     raise info '%', 'invoke test_f(c_type)';
	 RETURN 1;
  end;
  FUNCTION test_f(cur c_type1) RETURN integer IS
  BEGIN
    raise info '%', 'invoke test_f(c_type1)';
	RETURN 2;
  end;
BEGIN
  OPEN var1 FOR SELECT * FROM jobs;
  OPEN var2 FOR SELECT * FROM test_insert;
  var3 := test_f(var1);
  var3 := test_f(var2);
end;
/


--after assign,cursor%rowcount has continue
DELETE FROM jobs;

INSERT INTO jobs VALUES(1);
INSERT INTO jobs VALUES(2);
INSERT INTO jobs VALUES(3);

--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 integer;
BEGIN
	OPEN var1 FOR SELECT * FROM jobs;
	FETCH var1 INTO var3;
	raise info '%', 'var1%rowcount := ' || var1%rowcount;
	raise info '%', 'var3 = ' || var3;
	var2 := var1;
	raise info '%','var2%rowcount := ' || var2%rowcount;
	LOOP
		FETCH var2 INTO var3;
		IF var2%notfound THEN
			exit;
		END if;
		raise info '%','var2%rowcount := ' || var2%rowcount;
		raise info 'var3 = %',var3;
		raise info '%','var1%rowcount := ' || var1%rowcount;
    END LOOP;
end;
/


--ok
--and print
-----fetch var1
-----var1%rowcount := 1
-----var3 = 1
-----assign
-----var2%rowcount := 1
-----fetch var2
-----var2%rowcount := 2
-----var3 = 2
-----var1%rowcount := 1
-----fetch var1
-----var2%rowcount := 2
-----var3 = 3
----var1%rowcount := 3
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 integer;
BEGIN
	OPEN var1 FOR SELECT * FROM jobs;
	raise info '%','fetch var1';
	FETCH var1 INTO var3;
	raise info '%','var1%rowcount := ' || var1%rowcount;
	raise info '%','var3 = ' || var3;
	var2 := var1;
	raise info '%','assign';
	raise info '%','var2%rowcount := ' || var2%rowcount;
	FETCH var2 INTO var3;
	raise info '%','fetch var2';
	raise info '%','var2%rowcount := ' || var2%rowcount;
	raise info '%','var3 = ' || var3;
	raise info '%','var1%rowcount := ' || var1%rowcount;
	FETCH var1 INTO var3;
	raise info '%','fetch var1';
    raise info '%','var2%rowcount := ' || var2%rowcount;
	raise info '%','var3 = ' || var3;
	raise info '%','var1%rowcount := ' || var1%rowcount;
end;
/


---internel strong cursor can assign out strong cursor?
--failed
DECLARE
  type c_type IS ref CURSOR RETURN jobs%rowtype;
  var1 c_type;
  var3 integer;
  FUNCTION test_f(id integer) RETURN integer IS
    type c_type2 IS ref CURSOR RETURN jobs%rowtype;
	var2 c_type2;
  BEGIN
    OPEN var2 FOR SELECT * FROM jobs;
	var1 := var2;
	RETURN 1;
  end;
BEGIN
   var3 := test_f(23);
end;
/


--as return type
CREATE OR REPLACE package test_pkg IS
  type c_type IS ref CURSOR RETURN jobs%rowtype;
end;
/

--error
DECLARE
  var1 test_pkg.c_type;
  type c_type IS ref CURSOR RETURN jobs%rowtype;
  FUNCTION test_f(id integer) RETURN c_type IS
    var2 c_type;
  BEGIN
	OPEN var2 FOR SELECT * FROM jobs;
	RETURN var2;
  end;
BEGIN
   var1 := test_f(23);
end;
/

--ok
DECLARE
  var1 sys_refcursor;
  type c_type IS ref CURSOR RETURN jobs%rowtype;
  type c_type2 IS ref CURSOR RETURN jobs%rowtype;
  var2 c_type;
  var3 c_type2;
  var4 jobs%rowtype;
BEGIN
   OPEN var2 FOR SELECT * FROM jobs;
   var1 := var2;
   var3 := var1;
   FETCH var3 INTO var4;
   CLOSE var3;
end;
/


--implicit cursor in trigger
CREATE TABLE test_trig(id integer);
CREATE TABLE test_job(id integer);

INSERT INTO test_job VALUES(1);
INSERT INTO test_job VALUES(2);


CREATE OR REPLACE FUNCTION test_after_triger_proc() RETURN TRIGGER AS
BEGIN
	DELETE FROM test_job;
	raise info '%','sql%rowcount = ' || sql%rowcount;
	RETURN new;
end;
/

CREATE OR REPLACE TRIGGER test_after_trig after INSERT ON test_trig
 execute PROCEDURE test_after_triger_proc();

--ok and print
--sql%rowcount = 2
--sql%rowcount = 1
begin
   INSERT INTO test_trig VALUES(1);
   raise info '%','sql%rowcount = ' || sql%rowcount;
end;
/

drop trigger test_after_trig on test_trig;
DROP FUNCTION test_after_triger_proc;

CREATE OR REPLACE FUNCTION test_f(id integer) RETURN integer IS
BEGIN
	DELETE FROM test_job;
	raise info '%','sql%rowcount = ' || sql%rowcount;
	RETURN 2;
end;
/

--ok
DECLARE
	var1 integer;
begin
   INSERT INTO test_trig VALUES(1);
   var1 := test_f(23);
   raise info '%','sql%rowcount = ' || sql%rowcount;
end;
/

INSERT INTO test_job VALUES(1);
INSERT INTO test_job VALUES(2);

CREATE OR REPLACE FUNCTION test_f(id integer) RETURN integer IS
BEGIN
	raise info '%','sql%rowcount = ' || sql%rowcount;
	INSERT INTO test_job VALUES(3);
	RETURN 2;
end;
/


CREATE OR REPLACE function test_after_triger_proc() RETURN trigger as
declare
	var1 integer;
BEGIN
	DELETE FROM test_job;
	raise info '%','sql%rowcount = ' || sql%rowcount;
	var1 := test_f(23);
	raise info '%','sql%rowcount = ' || sql%rowcount;
	RETURN new;
end;
/


CREATE OR REPLACE TRIGGER test_after_trig after INSERT ON test_trig
 execute PROCEDURE test_after_triger_proc();

--ok
---and print
----sql%rowcount = 2
----sql%rowcount = 2
----sql%rowcount = 1
----sql%rowcount = 1
DECLARE
	var1 integer;
begin
   INSERT INTO test_trig VALUES(1);
   raise info '%','sql%rowcount = ' || sql%rowcount;
end;
/

drop trigger test_after_trig on test_trig;
DROP FUNCTION test_after_triger_proc;

--ok
--cursor%rowtype
DECLARE
  CURSOR c1 IS SELECT * FROM test_job;
  var1 c1%rowtype;
BEGIN
	var1.id := 1;
end;
/

--ok
DECLARE
  var2 integer;
  CURSOR c1 IS SELECT * FROM test_job WHERE id = var2;
  var1 c1%rowtype;
BEGIN
	var1.id := 1;
end;
/

--error
DECLARE
  type c1_type IS ref cursor;
  var1 c1_type;
  var2 var1%rowtype;
BEGIN
	OPEN var1 FOR SELECT * FROM test_job;
	var2.id := 1;
	CLOSE var1;
end;
/

--error
DECLARE
  CURSOR c RETURN jobs%rowtype;
  var1 integer;
  CURSOR c RETURN jobs%rowtype IS SELECT var1 FROM dual;
BEGIN
	OPEN c;
	CLOSE c;
end;
/

--ok
DECLARE
  CURSOR c RETURN test_job%rowtype;
  var1 integer;
  CURSOR c RETURN test_job%rowtype IS SELECT var1 FROM dual;
BEGIN
	OPEN c;
	CLOSE c;
end;
/


--type sys_refcursor is xxx
--
--error
DECLARE
  type sys_refcursor IS ref CURSOR RETURN employees%rowtype;
  var1 sys_refcursor;
BEGIN
	OPEN var1 FOR SELECT * FROM jobs;
	CLOSE var1;
end;
/

--ok
DECLARE
  var1 sys_refcursor;
BEGIN
	OPEN var1 FOR SELECT * FROM jobs;
	CLOSE var1;
end;
/

--explicit cursor args not match
--error
DECLARE
  CURSOR c1(id integer) RETURN jobs%rowtype;
  CURSOR c1(id integer,name varchar2) RETURN jobs%rowtype IS
    SELECT * FROM jobs;
BEGIN
	OPEN c1;
	CLOSE c1;
end;
/

--ok
--default not the same
--var1 = 1
DECLARE
  CURSOR c1(id1 integer DEFAULT 1) RETURN test_jobs%rowtype;
  CURSOR c1(id1 integer DEFAULT 2) RETURN test_jobs%rowtype IS
    SELECT id FROM test_jobs WHERE id = id1;
  var1 integer;
BEGIN
	OPEN c1;
	FETCH c1 INTO var1;
	raise info '%','var1 = ' || var1;
	CLOSE c1;
end;
/


--ok
--var1 = 2
DECLARE
  CURSOR c1(id1 integer DEFAULT 1) RETURN test_jobs%rowtype;
  CURSOR c1(id1 integer DEFAULT 2) RETURN test_jobs%rowtype IS
    SELECT id FROM test_jobs WHERE id = id1;
  var1 integer;
BEGIN
	OPEN c1(2);
	FETCH c1 INTO var1;
	raise info '%','var1 = ' || var1;
	CLOSE c1;
end;
/

--subproargname is the same
--error
DECLARE
  var1 integer;
  FUNCTION test_f(id integer,id varchar2) RETURN integer IS
  BEGIN
		RETURN 23;
  end;
BEGIN
	var1 := test_f(23,'xiexie');
end;
/

--ok
--cursor function
DECLARE
  var1 integer;
   procedure cursor(id integer) IS
  BEGIN
		raise info '%','cursor function';
  end;
  FUNCTION CURSOR RETURN integer IS
  BEGIN
      RETURN 1;
  END;
BEGIN
	call CURSOR(23);
	var1 := cursor;
end;
/

--ok
--cursor function
DECLARE
  var1 integer;
   procedure cursor(id integer) IS
  BEGIN
		raise info '%','cursor function';
  END cursor;
  FUNCTION CURSOR RETURN integer IS
  BEGIN
      RETURN 1;
  END cursor;
BEGIN
	call CURSOR(23);
	var1 := cursor;
end;
/

--ok
--var1 = 2
DECLARE
  CURSOR c1(id1 integer DEFAULT 1) RETURN test_jobs%rowtype;
  CURSOR c1(id1 integer DEFAULT 2) RETURN test_jobs%rowtype IS
    SELECT id FROM test_jobs WHERE id = id1;
  var1 integer;
BEGIN
	OPEN c1(2);
	FETCH c1 INTO var1;
	raise info '%','var1 = ' || var1;
	CLOSE c1;
end;
/

--ok
DECLARE
  ref integer;
BEGIN
  ref := 1;
end;
/


CREATE TABLE test(id integer,name varchar2(256));

INSERT INTO test VALUES(1,'one');
INSERT INTO test VALUES(2,'two');
INSERT INTO test VALUES(3,'three');

CREATE OR REPLACE package test_pkg IS
  CURSOR c1 IS SELECT * FROM test;
  type mds IS ref CURSOR RETURN c1%rowtype;
  FUNCTION test_f(id c1%rowtype,id2 mds) RETURN integer;
end;
/

--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  --CLOSE var2;
  IF var1%isopen THEN
		raise info '%','var1 is open';
		CLOSE var1;
  END if;
end;
/

--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  CLOSE var2;
  IF var1%isopen THEN
		raise info '%','var1 is open';
		CLOSE var1;
  END if;
end;
/


--error
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  IF var1%isopen THEN
		raise info '%','var1 is open 1';
		CLOSE var1;
  END if;
  CLOSE var2;
  IF var1%isopen THEN
		raise info '%','var1 is open 2';
		CLOSE var1;
  END if;
end;
/

--error
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  IF var1%isopen THEN
		raise info '%','var1 is open 1';
		CLOSE var1;
  END if;
  var2 := var1;
  CLOSE var2;
  IF var1%isopen THEN
		raise info '%','var1 is open 2';
		CLOSE var1;
  END if;
end;
/

--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  FETCH var1 INTO var3;
  CLOSE var2;
  IF var1%isopen THEN
		raise info '%','var1 is open';
		CLOSE var1;
  END if;
end;
/

--ok
---and print xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM test;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/

--ok and print
--xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  FETCH var2 INTO var3;
  OPEN var1 FOR SELECT * FROM test;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/

--ok and print
--var2 is open
--var2 is open
--xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
  END if;
  OPEN var1 FOR SELECT * FROM test;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/


--ok and print
--xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  OPEN var1 FOR SELECT * FROM test;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/

--ok and print
--xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  FETCH var1 INTO var3;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/

--ok and print
--xiexie
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  FETCH var2 INTO var3;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/


--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 test%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM test;
  var2 := var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
  END if;
  CLOSE var1;
  IF var2%isopen THEN
		raise info '%','var2 is open';
		--CLOSE var2;
  END if;
  raise info '%','xiexie';
end;
/

CREATE TABLE mds(id integer,name varchar2(256));

INSERT INTO mds VALUES(1,'one');
INSERT INTO mds VALUES(2,'two');
INSERT INTO mds VALUES(3,'three');

CREATE OR REPLACE package test_p_1 IS
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
  FUNCTION test_f(id integer) RETURN c_1;
  FUNCTION test_f2(id c_2) RETURN integer;
end;
/

CREATE OR REPLACE package body test_p_1 is
  FUNCTION test_f(id integer) RETURN c_1 IS
    var1 c_1;
  BEGIN
	OPEN var1 FOR SELECT * FROM mds;
	RETURN var1;
  end;
  FUNCTION test_f2(id c_2) RETURN integer IS
     var2 mds%rowtype;
  BEGIN
	 FETCH id INTO var2;
	 RETURN 2;
  end;
 end;
 /

--error
DECLARE
  var1 test_p_1.c_1;
  var2 integer;
BEGIN
  var1 := test_p_1.test_f(23);
  var2 := test_p_1.test_f2(var1);
end;
/

--error
DECLARE
  var1 integer;
BEGIN
  var1 := test_p_1.test_f2(test_p_1.test_f(23));
end;
/

--out strong cursor with wrong type
--error
DECLARE
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
  var1 c_1;
  var3 integer;
  FUNCTION test_f(cur out c_1) RETURN integer IS
     var2 c_2;
  BEGIN
	 cur := var2;
	 RETURN 23;
  end;
BEGIN
	var3 := test_f(var1);
end;
/

--error
DECLARE
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
  var1 c_1;
  var3 integer;
  FUNCTION test_f(cur out c_1) RETURN integer IS
     var2 c_2;
  BEGIN
     raise info '%', 'xiexie';
	 cur := var2;
	 RETURN 23;
  end;
BEGIN
	var3 := test_f(var1);
end;
/

--error
DECLARE
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
  var1 c_1;
  var3 integer;
  FUNCTION test_f(id integer) RETURN c_2 IS
     var2 c_2;
  BEGIN
	 RETURN var2;
  end;
BEGIN
    raise info '%','xiexie';
	var1 := test_f(var3);
end;
/


CREATE OR REPLACE package test_pkg IS
  var1 integer;
  type c_type IS ref cursor;
end;
/

--error
DECLARE
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
  var1 c_1;
  var3 integer;
  FUNCTION test_f(id integer) RETURN c_2 IS
     var2 c_2;
  BEGIN
	 RETURN var2;
  end;
BEGIN
    test_pkg.var1 := 145;
	var1 := test_f(var3);
end;
/

DROP FUNCTION test_f(test_pkg.c_type);
DROP package test_pkg;


CREATE OR REPLACE package test_pkg IS
  type c_1 IS ref CURSOR RETURN mds%rowtype;
  type c_2 IS ref CURSOR RETURN mds%rowtype;
end;
/

DROP FUNCTION test_f(number);
DROP FUNCTION test_f(integer);

CREATE OR REPLACE FUNCTION test_f(id integer) RETURN test_pkg.c_1 IS
  var1 test_pkg.c_1;
BEGIN
	RETURN var1;
end;
/


CREATE OR REPLACE FUNCTION test_f1(id integer) RETURN integer IS
  var1 test_pkg.c_1;
BEGIN
	var1 := test_f(id);
	RETURN id;
end;
/

--ok
DECLARE
  var1 integer;
BEGIN
  var1 := test_f1(23);
end;
/

--ok
CREATE OR REPLACE FUNCTION test_f(id integer) RETURN test_pkg.c_2 IS
  var1 test_pkg.c_2;
BEGIN
	RETURN var1;
end;
/

--again execute
--error
DECLARE
  var1 integer;
BEGIN
  var1 := test_f1(23);
end;
/


--error
DECLARE
  var1 integer;
BEGIN
  var1 := test_f1(23);
end;
/

CREATE OR REPLACE FUNCTION test_f(id integer) RETURN test_pkg.c_1 IS
  var1 test_pkg.c_1;
BEGIN
	RETURN var1;
end;
/

--ok
DECLARE
  var1 integer;
BEGIN
  var1 := test_f1(23);
end;
/


--ok
DECLARE
  var1 integer := 0;
BEGIN
	INSERT INTO mds VALUES(1,'sfjdsf');
	IF sql%ROWCOUNT > var1 THEN
		raise info '%','xixie';
	ELSE
		raise info '%','welcome';
	END if;
end;
/

--ok
DECLARE
  var1 integer;
  FUNCTION test_f(idi integer) RETURN integer IS
  BEGIN
		INSERT INTO mds VALUES(1,'xixie');
		RETURN sql%rowcount;
  end;
BEGIN
  var1 := test_f(23);
end;
/

--ok
DECLARE
  var1 number;
  FUNCTION test_f(idi number) RETURN number IS
  BEGIN
		INSERT INTO mds VALUES(1,'xixie');
		RETURN sql%rowcount;
  end;
BEGIN
  var1 := test_f(sql%rowcount);
end;
/

--error
DECLARE
  var1 integer;
BEGIN
	raise info '%',sql%mdsx;
end;
/


--ok
DECLARE
  var1 sys_refcursor;
  var2 sys_refcursor;
  var3 mds%rowtype;
BEGIN
  OPEN var1 FOR SELECT * FROM mds;
  var2 := var1;
  FETCH var2 INTO var3;
  raise info '%', 'var1%rowcount:'|| var1%rowcount;
  IF var1%found then
	raise info '%', 'var1%found:';
  END if;
  IF var1%notfound then
	raise info '%', 'var1%notfound:';
  END if;
end;
/

--fetch can reflect sql attribute
DROP FUNCTION test_f;

CREATE OR REPLACE FUNCTION test_f(id integer) RETURN sys_refcursor IS
  var1 sys_refcursor;
BEGIN
  OPEN var1 FOR SELECT * FROM mds;
  RETURN var1;
end;
/

CREATE OR REPLACE FUNCTION test_fetch(cur sys_refcursor) RETURN integer IS
  var1 mds%rowtype;
BEGIN
  FETCH cur INTO var1;
  raise info '%', 'var1.id = ' || var1.id;
  IF sql%found then
	raise info '%', 'sql_found';
  END if;
  raise info '%', 'sql.rowcount = ' || sql%rowcount;
  RETURN 23;
end;
/

--ok and print
----var1.id = 1
----sql.rowcount =
DECLARE
  var1 sys_refcursor;
  var2 integer;
BEGIN
  var1 := test_f(1);
  var2 := test_fetch(var1);
end;
/


DECLARE
  var1 integer;
BEGIN
  FOR var1 IN 1 .. 10 LOOP
     raise info '%', 'sql.rowcount = ' || sql%rowcount;
  END loop;
  raise info '%', 'after sql.rowcount = ' || sql%rowcount;
end;
/

DECLARE
  var1 mds%rowtype;
BEGIN
  FOR var1 IN (SELECT * FROM mds ORDER BY id,name) LOOP
    raise info '%', 'var1.id = ' || var1.id;
	raise info '%', 'sql.rowcount=' || sql%rowcount;
  END loop;
  raise info '%','finish sql.rowcount=' || sql%rowcount;
end;
/

--for cursor
DECLARE
  var1 mds%rowtype;
  CURSOR c1 IS SELECT * FROM mds;
BEGIN
  FOR var1 IN c1 LOOP
    raise info '%', 'var1.id = ' || var1.id;
	raise info '%', 'sql.rowcount=' || sql%rowcount;
  END loop;
  raise info '%','finish sql.rowcount=' || sql%rowcount;
end;
/

create or replace package dwdai_pkg IS
  var0 mds%rowtype;
  var1 integer;
  function test(id integer,name varchar) return integer;
  var2 public.dwdai_pkg.var0%type;
  CURSOR c1 RETURN mds%rowtype;
end;
/

create or replace package body dwdai_pkg IS
  CURSOR c1 RETURN mds%rowtype IS SELECT * FROM mds;
  var3 public.dwdai_pkg.var2%type;
  var4 c1%rowtype;
  function test(id integer,name varchar) return integer is
        var2 public.dwdai_pkg.var2%type;
  begin
      dwdai_pkg.var1 := 23;
      public.dwdai_pkg.var1 := 23;
      return 23;
  end;
BEGIN
  var4.id := 1;
end;
/

--ok
DECLARE
  var1 dwdai_pkg.c1%rowtype;
BEGIN
  var1.id := 1;
end;
/


--explicit cursor use open for
--error
DECLARE
  CURSOR c1 RETURN mds%rowtype;
BEGIN
 OPEN c1 FOR SELECT * FROM mds;
 CLOSE c1;
end;
/


--has a define
--error
DECLARE
  CURSOR c1 RETURN mds%rowtype IS SELECT * FROM mds;
BEGIN
 OPEN c1 FOR SELECT * FROM mds;
 CLOSE c1;
end;
/

--ok
DECLARE
  CURSOR c1 RETURN mds%rowtype IS SELECT * FROM mds;
BEGIN
 OPEN c1;
 CLOSE c1;
end;
/


--test cursor%rowcount,cursor%found
CREATE TABLE test_cursor_attribute(id integer,name varchar2(256));

INSERT INTO test_cursor_attribute VALUES(1,'one');
INSERT INTO test_cursor_attribute VALUES(2,'two');
INSERT INTO test_cursor_attribute VALUES(3,'three');
INSERT INTO test_cursor_attribute VALUES(4,'four');

--before open
--test isopen
--ok
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  IF c1%isopen THEN
     raise info '%','c1 is open';
  END if;
end;
/

--test notfound
--error at line 4
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%', 'c1 not found is true';
  END if;
end;
/

--test found
--error at line 4
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  IF c1%found IS null THEN
     raise info '%','c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%','c1 found is true';
  END if;
end;
/

--test rowcount
--error at line 4
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%', 'c1 rowcount is null';
  END if;
  raise info '%', 'c1%rowcount = ' || c1%rowcount;
end;
/

--after open before fetch
--test isopen
--ok and print c1 is open
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  OPEN c1;
  IF c1%isopen THEN
     raise info '%', 'c1 is open';
  END if;
end;
/


--test notfound
--ok and print c1 not found is null
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  OPEN c1;
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%','c1 not found is true';
  END if;
end;
/

--test found
--ok and print
--c1 found is nul
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  OPEN c1;
  IF c1%found IS null THEN
     raise info '%','c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%', 'c1 found is true';
  END if;
end;
/

--test rowcount
--ok and print
--c1%rowcount = 0
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
BEGIN
  OPEN c1;
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%', 'c1 rowcount is null';
  END if;
  raise info '%', 'c1%rowcount = ' || c1%rowcount;
end;
/

--after fetch
--test isopen
--ok and print
--c1 is open
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var1;
  IF c1%isopen THEN
     raise info '%', 'c1 is open';
  END if;
end;
/

--test notfound
--ok
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var1;
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%','c1 not found is true';
  END if;
end;
/

--test found
--ok and print
--c1 found is true
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var1;
  IF c1%found IS null THEN
     raise info '%', 'c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%','c1 found is true';
  END if;
end;
/

--test rowcount
--ok and print
--c1%rowcount = 1
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  FETCH c1 INTO var1;
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%','c1 rowcount is null';
  END if;
  raise info '%','c1%rowcount = ' || c1%rowcount;
end;
/

--again fetch
--test notfound
--ok
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%notfound IS null THEN
		 raise info '%','c1 not found is null';
	  END if;
	  IF c1%notfound THEN
		 raise info '%', 'c1 not found is true';
	  END if;
  END loop;
end;
/

--test found
--ok and print 4 times
--c1 found is true
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%found IS null THEN
		 raise info '%','c1 found is null';
	  END if;
	  IF c1%found THEN
		 raise info '%', 'c1 found is true';
	  END if;
  END loop;
end;
/

--test rowcount
--ok
DECLARE
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
BEGIN
  OPEN c1;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%ROWCOUNT IS NULL THEN
		  raise info '%','c1 rowcount is null';
	  END if;
	  raise info '%','c1%rowcount = ' || c1%rowcount;
 END loop;
end;
/

--ref cursor
--before open
--test isopen
--ok
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  IF c1%isopen THEN
     raise info '%', 'c1 is open';
  END if;
end;
/


--test notfound
--error at line 5
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%', 'c1 not found is true';
  END if;
end;
/


--test found
--error at line 5
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  IF c1%found IS null THEN
     raise info '%','c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%','c1 found is true';
  END if;
end;
/


--test rowcount
--error at line 5
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%','c1 rowcount is null';
  END if;
  raise info '%','c1%rowcount = ' || c1%rowcount;
end;
/

--after open before fetch
--test isopen
--ok and print c1 is open
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  IF c1%isopen THEN
     raise info '%','c1 is open';
  END if;
  CLOSE c1;
end;
/

--test notfound
--ok and print c1 not found is null
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%', 'c1 not found is true';
  END if;
  CLOSE c1;
end;
/

--test found
--ok and print c1 found is null
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  IF c1%found IS null THEN
     raise info '%','c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%','c1 found is true';
  END if;
  CLOSE c1;
end;
/


--test rowcount
--ok and print
--c1%rowcount = 0
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%','c1 rowcount is null';
  END if;
  raise info '%','c1%rowcount = ' || c1%rowcount;
  CLOSE c1;
end;
/


--after fetch
--test isopen
--ok and print
--c1 is open
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  IF c1%isopen THEN
     raise info '%', 'c1 is open';
  END if;
  CLOSE c1;
end;
/

--test notfound
--ok
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  IF c1%notfound IS null THEN
     raise info '%','c1 not found is null';
  END if;
  IF c1%notfound THEN
     raise info '%','c1 not found is true';
  END if;
  CLOSE c1;
end;
/


--test found
--ok and print
--c1 found is true
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  IF c1%found IS null THEN
     raise info '%','c1 found is null';
  END if;
  IF c1%found THEN
     raise info '%','c1 found is true';
  END if;
  CLOSE c1;
end;
/


--test rowcount
--ok and print
--c1%rowcount = 1
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  IF c1%ROWCOUNT IS NULL THEN
      raise info '%','c1 rowcount is null';
  END if;
  raise info '%','c1%rowcount = ' || c1%rowcount;
  CLOSE c1;
end;
/

--again fetch
--test notfound
--ok
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%notfound IS null THEN
		 raise info '%','c1 not found is null';
	  END if;
	  IF c1%notfound THEN
		 raise info '%','c1 not found is true';
	  END if;
  END loop;
  CLOSE c1;
end;
/


--test found
--ok and print 4 times
--c1 found is true
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%found IS null THEN
		 raise info '%','c1 found is null';
	  END if;
	  IF c1%found THEN
		 raise info '%','c1 found is true';
	  END if;
  END loop;
  CLOSE c1;
end;
/


--test rowcount
--ok and print 4 times
--c1%rowcount = x
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  loop
	  FETCH c1 INTO var1;
	  EXIT WHEN c1%notfound;
	  IF c1%ROWCOUNT IS NULL THEN
		  raise info '%','c1 rowcount is null';
	  END if;
	  raise info '%','c1%rowcount = ' || c1%rowcount;
 END loop;
 CLOSE c1;
end;
/

---attribute %rowcount,%found,%notfound has change after
---assign
--ok and print
--c1 is found
--c1%rowcount = 1
--c2 is found
--c2%rowcount = 1
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  c2 := c1;
  IF c1%found then
	raise info '%','c1 is found';
  END if;
  raise info '%','c1%rowcount = ' || c1%rowcount;
  IF c2%found then
	raise info '%','c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
end;
/

--ok and print
--c1 is found
--c1%rowcount = 2
--c2 is found
--c2%rowcount = 2
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  c2 := c1;
  FETCH c2 INTO var1;
  IF c1%found then
	raise info '%', 'c1 is found';
  END if;
  raise info '%', 'c1%rowcount = ' || c1%rowcount;
  IF c2%found then
	raise info '%', 'c2 is found';
  END if;
  raise info '%', 'c2%rowcount = ' || c2%rowcount;
end;
/

--ok and print
---before assign c1 is found
---before assign c1%rowcount = 1
---after assign c1 is found
---after assign c1%rowcount = 1
---c2 is found
---c2%rowcount = 2
---after assign c1 is found
---after assign c1%rowcount = 3
---c2 is found
---c2%rowcount = 2
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  IF c1%found then
	raise info '%', 'before assign c1 is found';
  END if;
  raise info '%', 'before assign c1%rowcount = ' || c1%rowcount;
  c2 := c1;
  FETCH c2 INTO var1;
  IF c1%found then
	raise info '%','after assign c1 is found';
  END if;
  raise info '%', 'after assign c1%rowcount = ' || c1%rowcount;
  IF c2%found then
	raise info '%', 'c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
  FETCH c1 INTO var1;
  IF c1%found then
	raise info '%', 'after assign c1 is found';
  END if;
  raise info '%', 'after assign c1%rowcount = ' || c1%rowcount;
  IF c2%found then
	raise info '%', 'c2 is found';
  END if;
  raise info '%', 'c2%rowcount = ' || c2%rowcount;
end;
/

--error
---print
---c2 is open
---c2 is found
---c2%rowcount = 2
---after close c2
--then raise error
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  c2 := c1;
  FETCH c2 INTO var1;
  IF c2%isopen THEN
    raise info '%', 'c2 is open';
  END if;
  IF c2%found THEN
    raise info '%','c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
  CLOSE c2;
  raise info 'after close c2';
  IF c2%isopen THEN
    raise info '%','c2 is open';
  END if;
  IF c2%found THEN
    raise info '%','c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
end;
/


--ok and print
---c2 is open
---c2 is found
---c2%rowcount = 2
---after close c1
---c2 is open
---c2 is found
---c2%rowcount = 2
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  FETCH c1 INTO var1;
  c2 := c1;
  FETCH c2 INTO var1;
  IF c2%isopen THEN
    raise info '%','c2 is open';
  END if;
  IF c2%found THEN
    raise info '%','c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
  CLOSE c1;
  raise info '%','after close c1';
  IF c2%isopen THEN
    raise info '%','c2 is open';
  END if;
  IF c2%found THEN
    raise info '%','c2 is found';
  END if;
  raise info '%','c2%rowcount = ' || c2%rowcount;
end;
/

--ok and print
---c2 is open
---c2 is open
DECLARE
  type c_type IS ref CURSOR RETURN test_cursor_attribute%rowtype;
  c1 c_type;
  var1 c1%rowtype;
  c2 c_type;
BEGIN
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  c2 := c1;
  IF c2%isopen THEN
     raise info '%','c2 is open';
  END if;
  CLOSE c2;
  OPEN c1 FOR SELECT * FROM test_cursor_attribute ORDER BY id;
  c2 := c1;
  IF c2%isopen THEN
     raise info '%','c2 is open';
  END if;
end;
/


--package cursor
CREATE OR REPLACE package test_cursor_pkg IS
  CURSOR c1 IS SELECT * FROM test_cursor_attribute ORDER BY id;
  var1 c1%rowtype;
  FUNCTION test_open(id integer) RETURN integer;
  FUNCTION test_fetch(id integer) RETURN integer;
  FUNCTION test_close(id integer) RETURN integer;
end;
/

CREATE OR REPLACE package body test_cursor_pkg IS
  FUNCTION test_open(id integer) RETURN integer IS
  BEGIN
    OPEN c1;
	RETURN 1;
  end;
  FUNCTION test_fetch(id integer) RETURN integer IS
  BEGIN
    FETCH c1 INTO var1;
	IF c1%found THEN
      raise info '%',var1;
	  RETURN 1;
	END if;
	RETURN 2;
  end;
  FUNCTION test_close(id integer) RETURN integer IS
  BEGIN
    IF c1%isopen THEN
       CLOSE c1;
	END if;
	RETURN 1;
  end;
end;
/

--open
DECLARE
  var1 integer;
BEGIN
  var1 := test_cursor_pkg.test_open(1);
end;
/

--fetch
DECLARE
  var1 integer;
BEGIN
  LOOP
    var1 := test_cursor_pkg.test_fetch(1);
	IF var1 = 2 THEN
      exit;
	END if;
 END loop;
end;
/

--close
DECLARE
  var1 integer;
BEGIN
  var1 := test_cursor_pkg.test_close(1);
end;
/

--test bind variable
--maybe test a function arguments
variable c1 refcursor;

create table test_sds(id integer,name varchar2(256));

insert into test_sds values(1,'one');
insert into test_sds values(2,'two');
insert into test_sds values(3,'three');

CREATE OR REPLACE FUNCTION test_fetch_ref_cursor(var1 sys_refcursor) RETURN integer IS
  var2 test_sds%rowtype;
BEGIN
  FETCH var1 INTO var2;
  raise info '%', 'id = ' || var2.id || ' name = ' || var2.name;
  RETURN 1;
end;
/


CREATE OR REPLACE FUNCTION test_close_ref_cursor(var1 sys_refcursor) RETURN integer IS
  var2 test_sds%rowtype;
BEGIN
  close var1;
  RETURN 1;
end;
/

--ok
begin
  open :c1 for select * from test_sds order by id;
end;
/

--ok
select test_fetch_ref_cursor(:c1) from dual;

--ok
select test_fetch_ref_cursor(:c1) from dual;

--ok
select test_fetch_ref_cursor(:c1) from dual;

--ok
select test_close_ref_cursor(:c1) from dual;


--clean data
DROP TABLE test_sds;
DROP TABLE departments;
DROP TABLE dept_temp;
DROP TABLE employees;
DROP TABLE employees_temp;
DROP TABLE jobs;
DROP TABLE locations;
DROP TABLE mds;
DROP TABLE test;
DROP TABLE test_id_match1;
DROP TABLE test_id_match2;
DROP TABLE test_init;
DROP TABLE test_insert;
DROP TABLE test_insert1;
DROP TABLE test_job;
DROP TABLE test_jobs;
DROP TABLE test_trig;
DROP TABLE test_cursor_attribute;

DROP package test_pkg;
DROP package admin_data;
DROP package dwdai_pkg;
DROP package emp_data;
DROP package test_init_pkg;
DROP package test_p_1;
DROP package test_cursor_pkg;


DROP PROCEDURE p;
DROP FUNCTION test_sch_f;
DROP FUNCTION test_f;
DROP FUNCTION test_f1;
DROP FUNCTION test_fetch;
DROP FUNCTION test_fetch_ref_cursor;
DROP FUNCTION test_close_ref_cursor;


--add ora_cursor.sql
set plisql.variable_conflict=use_column;
begin
	create table t_cursor_test
	(id integer,
	name text,
	salary integer
	);

	RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
	RAISE notice 'sql FOUND %', SQL%FOUND;
	RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
	RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;

	for i in 1..50 loop
	insert into t_cursor_test values(i, 'str'||i , i*10);
	end loop;
end;
/

select count(*) from t_cursor_test;


DECLARE
   id integer;
   name text;
  CURSOR c1 (minid integer , maxid integer = 2) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1(1);
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
  for r in c1(1) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/

DECLARE
   id integer;
   name text;
  CURSOR c1  (minid integer , maxid integer = 2) is
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN

  OPEN c1(1);
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;

  for r in c1(1) loop
   raise notice '% % ', r.id, r.name;
  end loop;

END;
/

DECLARE
   id integer;
  CURSOR c1  (minid integer , maxid integer) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN

 for id in c1(3, 5) loop
   raise notice '%', (id.id);
  end loop;
END;
/

DECLARE
   id integer;
   name text;
  CURSOR c1  (minid in integer := 2, maxid in integer ) is
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  for r in c1(maxid => 5) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/

DECLARE
   id integer;
  CURSOR c1  (minid integer := 1, maxid integer default 3) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN

 for id in c1() loop
   raise notice '%', ( id.id);
  end loop;
END;
/

DECLARE
   id integer;
   name text;
   CURSOR c1 (minid integer = 2 , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1(1);
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
 for r in c1(1) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/

DECLARE
   id integer;
   name text;
  CURSOR c1  (minid integer = 2 , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1(1, 10);
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
 for r in c1(1,10) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/


DECLARE
   id integer;
   name text;
  CURSOR c1  (minid integer = 2 , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1(maxid := 5);
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
 for r in c1(maxid := 5) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/

DECLARE
   id integer;
   name text;
  CURSOR c1  (minid integer = 2 , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1();
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
 for r in c1() loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/


DECLARE
   id integer;
   name text;
  CURSOR c1  (minid integer = 2 , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
  r record;
BEGIN
  OPEN c1;

   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
  for r in c1 loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/


DECLARE
   id integer;
   name text;
   minid integer := 1;
   maxid integer := 5;
  CURSOR c1  (minid integer = minid , maxid integer = (minid+maxid-minid)) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1;
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;

  for r in c1 loop
   raise notice '% % ', r.id, r.name;
  end loop;

  minid := 3;
  maxid := 9;

  OPEN c1;
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;

  for r in c1 loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/


--error
DECLARE
   id integer;
   name text;
  CURSOR c1 (minid integer , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN

  OPEN c1(maxid := 5);

   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
END;
/

--error
DECLARE
   id integer;
   name text;
  CURSOR c1 (minid integer , maxid integer = 3) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN

 for r in c1(maxid := 5) loop
   raise notice '% % ', r.id, r.name;
  end loop;
END;
/

--error
DECLARE
   id integer;
   name text;
  CURSOR c1 (minid integer , maxid integer) for
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  OPEN c1;
   loop
   FETCH c1 INTO id, name;
    exit when c1%NOTfound;
   raise notice '% % ', id, name;
	end loop;
  close c1;
END;
/

--ok
DECLARE
  cv sys_REFCURSOR;
  sal integer;
  sal_multiple integer;
  factor        INTEGER := 2;
BEGIN
  OPEN cv FOR
    SELECT salary, salary*factor FROM t_cursor_test where  id <= 3 order by id;
  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise notice '%',('factor = ' || factor);
    raise notice '%',('sal          = ' || sal);
    raise notice '%',('sal_multiple = ' || sal_multiple);
    factor := factor + 1;  -- Does not affect sal_multiple
  END LOOP;

  CLOSE cv;
END;
/

DECLARE
  cv SYS_REFCURSOR;
  sal integer;
  sal_multiple integer;
  factor        INTEGER := 2;
BEGIN
  raise notice '%',('factor = ' || factor);
  OPEN cv FOR
    SELECT salary, salary*factor FROM t_cursor_test where  id <= 3 order by id;

  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise notice '%',('sal          = ' || sal);
    raise notice '%',('sal_multiple = ' || sal_multiple);
  END LOOP;
  factor := factor + 1;
  raise notice '%',('factor = ' || factor);
  OPEN cv FOR
     SELECT salary, salary*factor FROM t_cursor_test where  id <= 3 order by id;
  LOOP
    FETCH cv INTO sal, sal_multiple;
    EXIT WHEN cv%NOTFOUND;
    raise notice '%',('sal          = ' || sal);
    raise notice '%',('sal_multiple = ' || sal_multiple);
  END LOOP;
  CLOSE cv;
END;
/

--error
DECLARE
  CURSOR c1  (minid integer := 1, maxid integer default 3) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
BEGIN
  open c1;
  open c1;
  close c1;
END;
/

--error
DECLARE
  CURSOR c1  (minid integer := 1, maxid integer default 2) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
  r record;
BEGIN
	raise notice 'c1 isopen is %', c1%isopen;
	open c1;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;
	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;
	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;
	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;
	close c1;
	raise notice 'c1 isopen is %', c1%isopen;
	open c1;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;
	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	fetch c1 into r;

	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	close c1;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
END;
/

DECLARE
  r record;
BEGIN
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
	DELETE FROM t_cursor_test
		WHERE id > 45;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	DELETE FROM t_cursor_test
		WHERE id > 49;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	update t_cursor_test set name=name||'new'
		WHERE id > 40;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	update t_cursor_test set name=name||'new'
		WHERE id > 45;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	select count(1) into r from t_cursor_test;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	select * into r from t_cursor_test where id > 45;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
	select * into r from t_cursor_test where id = 40;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
END;
/

--error
DECLARE
  CURSOR c1  (minid integer := 1, maxid integer default 2) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
  r record;
BEGIN
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
	open c1;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
	fetch c1 into r;
	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	fetch c1 into r;

	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	fetch c1 into r;

	raise notice 'c1 isopen is %', c1%isopen;
	raise notice 'c1 found is %', c1%found;
	raise notice 'c1 notfound is %', c1%notfound;
	raise notice 'c1 rowcount is %', c1%rowcount;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	close c1;

	raise notice 'c1 isopen is %', c1%isopen;

	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;

	raise notice 'c1 found is %', c1%found;
END;
/

DECLARE
  CURSOR c1  (minid integer := 1, maxid integer default 10) is
    SELECT id FROM t_cursor_test where id >= minid and id <= maxid order by id;
  r record;
BEGIN
  if not c1%isopen then
	raise notice 'c1 is not open';
	open c1;
  end if;
  if  c1%isopen then
	raise notice 'c1 is open';
  end if;
  loop
	  fetch c1 into r;
	  if c1%found then
		raise notice 'c1 found row count %, record %', c1%rowcount, r;
	  end if;
	  IF c1%ROWCOUNT = 5 THEN
       raise notice '%', ('--- Fetched 5th row ---');
    END IF;
	  if c1%notfound then
	    raise notice 'c1 not found row count %', c1%rowcount;
	   exit;
	  end if;
  end loop;
  close c1;
  if not c1%isopen then
	raise notice 'c1 is close';
  end if;
END;
/

--error
DECLARE
  CURSOR c1  (minid integer := 1, maxid integer default 10) is
    SELECT * FROM t_cursor_test where id >= minid and id <= maxid order by id;
begin
	open c1;
	if c1%fond then
		null;
	end if;
	close c1;
end;
/


DECLARE
  r record;
BEGIN
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
	SELECT * into r FROM t_cursor_test where id > 10 ;

    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
END;
/


DECLARE
  count integer;
BEGIN
	update t_cursor_test set name = name || 'new' where id <= 10;
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
END;
/


BEGIN
  for r1 in  'select * from t_cursor_test where id >= $1 and id <= $2 order by id'  using 2,5 loop
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
	raise notice '% ',  r1;
  end loop;
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
END;
/

BEGIN
  for r1 in  select * from t_cursor_test where id >= 1 and id <= 5 order by id  loop
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
	raise notice '% ',  r1;
  end loop;
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
END;
/

select count(1) from t_cursor_test;

BEGIN
  DELETE FROM t_cursor_test
  WHERE id > 40;
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
  DELETE FROM t_cursor_test
  WHERE id = 45;
    RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
    RAISE notice 'sql FOUND %', SQL%FOUND;
    RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
    RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
END;
/


begin
	drop table t_cursor_test;
	create table t_cursor_test
	(id integer,
	name text,
	salary integer
	);
	RAISE notice 'sql ISOPEN %', SQL%ISOPEN;
	RAISE notice 'sql FOUND %', SQL%FOUND;
	RAISE notice 'sql NOTFOUND %', SQL%NOTFOUND;
	RAISE notice 'sql ROWCOUNT %', SQL%ROWCOUNT;
	for i in 1..50 loop
	insert into t_cursor_test values(i, 'str'||i , i*10);
	end loop;
end;
/

--error
declare
  v1 int;
begin
  if v1%isopn then
    null;
  end if;
end;
/

create table t_0000042(a int);
insert into t_0000042 values(1);
insert into t_0000042 values(2);

begin
  for r in (select * from t_0000042) loop
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
  end loop;
end;
/

declare
  r t_0000042%rowtype;
begin
  select * into r from t_0000042 where rownum = 1;
  for r in (select * from t_0000042) loop
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
  end loop;
end;
/

declare
  r t_0000042%rowtype;
begin
  for r in (select * from t_0000042) loop
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
  end loop;
  select * into r from t_0000042 where rownum = 1;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
end;
/

declare
  r t_0000042%rowtype;
begin
  select * into r from t_0000042 where rownum = 1;
  for i in (select * from t_0000042) loop
	raise notice '%', i.a;
  end loop;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
end;
/

declare
  r t_0000042%rowtype;
begin
  for i in (select * from t_0000042) loop
	raise notice '%', i.a;
  end loop;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
  select * into r from t_0000042 where rownum = 1;
	raise notice 'SQL isopen is %', SQL%isopen;
	raise notice 'SQL found is %', SQL%found;
	raise notice 'SQL notfound is %', SQL%notfound;
	raise notice 'SQL rowcount is %', SQL%rowcount;
end;
/
drop table t_0000042;

create table t_cursor(a varchar(23));

--report error
create or replace function f3(c text) return integer as
	cursor c is select * from t_cursor order by a;
	r record;
begin
	open c;
	loop
		exit when c%notfound;
		fetch c into r;
		raise info '%',r;
	end loop;
	close c;
end;
/
DROP function f3;
--test ok
create or replace function f3(c text) return setof t_cursor as
	cursor c_1 is select * from t_cursor order by a;
	r record;
begin
	open c_1;
	loop
		exit when c_1%notfound;
		fetch c_1 into r;
		return next r;
	end loop;
	close c_1;
end;
/

insert into t_cursor values('a');
insert into t_cursor values('b');

select * from f3('xiexie');

drop function f3(text);
drop table t_cursor;


create table departments_0000173(department_id integer,department_name varchar(256), manager_id integer,location_id integer);
insert into departments_0000173 values(1,'wanghong',1,2);
insert into departments_0000173 values(2,'zhangwei',1,2);
insert into departments_0000173 values(3,'zhousan',1,2);

CREATE TABLE dept_temp_0000173 AS
SELECT * FROM departments_0000173;

-- implict cursor
CREATE OR REPLACE PROCEDURE p_0000173 (
dept_no INTEGER
)AS
BEGIN
	DELETE FROM dept_temp_0000173
	WHERE department_id = dept_no;
	--test isopen
	if SQL%ISOPEN THEN
		raise info '%','delete stmt is open';
	else
		raise info '%', 'delete stmt is not open';
	end if;
	--test FOUND
	IF SQL%FOUND THEN
		raise info '%','Delete succeeded for department number';
	ELSE
		raise info '%','No department number';
	END IF;
	--test NOFOUND
	if SQL%NOTFOUND THEN
		raise info '%', 'No department number';
	else
		raise info '%', 'Delete succeeded for department number';
	end if;
	--test rowcount
	raise info '%', 'rowcount is '||SQL%ROWCOUNT;
END;
/

BEGIN
	call p_0000173(2);
	call p_0000173(400);
END;
/

drop procedure p_0000173(int);
drop table dept_temp_0000173;
drop table departments_0000173;

create or replace procedure p1_0000173(a int) as
  v1 int;
begin
  v1 := a;
end;
/

create or replace procedure p2_0000173(a out int) as
begin
  a := 2;
end;
/

create or replace function f_0000173(a int) return int as
  v int;
begin
  v := a;
  return v;
end;
/

create or replace procedure test_p_0000173 as
  r int;
  r1 int;
begin
  r := 1;
  call p1_0000173(r);
  if SQL%FOUND then
    raise info '%','p1_0000173 is found';
  else
    raise info '%', 'p1_0000173 is not found';
  end if;
  call p2_0000173(r);
  if SQL%FOUND then
    raise info '%', 'p2_0000173 is found';
  else
    raise info '%', 'p2_0000173 is not found';
  end if;
  r1 := f_0000173(r);
  if SQL%FOUND then
    raise info '%', 'f_0000173 is found';
  else
    raise info '%','f_0000173 is not found';
  end if;
end;
/

call test_p_0000173();

create or replace procedure test_p_0000173 as
  r int;
  r1 int;
begin
  r := 1;
  call p1_0000173(r);
  if SQL%FOUND then
    raise info '%','p1_0000173 is found';
  else
    raise info '%','p1_0000173 is not found';
  end if;
  call p2_0000173(r);
  if SQL%FOUND then
    raise info '%','p2_0000173 is found';
  else
    raise info '%','p2_0000173 is not found';
  end if;
  select f_0000173(r) into r1 from dual;
  if SQL%FOUND then
    raise info '%','f_0000173 is found';
  else
    raise info '%','f_0000173 is not found';
  end if;
end;
/

call test_p_0000173();



drop procedure test_p_0000173();
drop procedure p1_0000173(int);
drop procedure p2_0000173(int);
drop function f_0000173(int);


create table dwdai_test1(id integer,name varchar(23));
create table dwdai_test2(id integer,name varchar(23));

insert into dwdai_test1 values(1,'xiexie');
insert into dwdai_test2 values(2,'welcome');

-- print welcome and xiexie
declare
        r dwdai_test1%rowtype;
	id integer;
        cursor cc is select * from dwdai_test1;
	function f1 return int as
		cursor cc is select * from dwdai_test2;
		r dwdai_test2%rowtype;
	begin
		open cc;
		fetch cc into r;
		raise info '%',r.name;
		close cc;
		return 23;
	end;
begin
        open cc;
	fetch cc into r;
        id := f1();
	raise info '%',r.name;
	close cc;
end;
/


drop table dwdai_test1;
drop table dwdai_test2;


create table t_cursor(id integer,name varchar2(23));

--insert one row
insert into t_cursor values(1,'one');

--support declare and define with argtype
declare
   cursor c1(mid integer) return t_cursor%rowtype;
   var1 integer;
   cursor c1(mid integer) return t_cursor%rowtype is select *from t_cursor where id = mid;
begin
  var1 := 1;
  for r in c1(var1) LOOP
    raise info '%',r.id;
	raise info '%',r.name;
  end loop;
end;
/

drop table t_cursor;

CREATE TABLE t_row1(department_id integer,department_name varchar(256),
	manager_id integer,location_id integer);

INSERT INTO t_row1 VALUES(1, 'Develop', 2, 3);

CREATE OR REPLACE FUNCTION f_t1(i INT) RETURN INT AS
  CURSOR cur IS SELECT department_id,department_name FROM t_row1;
  c1 cur%rowtype;
BEGIN
  c1.department_id := 10;
  c1.department_name := 'Human Resources';
  RETURN 1;
END;
/

--ok
SELECT f_t1(1) FROM DUAL;

--ok
SELECT f_t1(1) FROM DUAL;



--clean data
drop function f_t1(integer);
drop table t_row1;


--package has reflect cursor
create table test(id integer,name varchar2(23));

create or replace package pkg is
  name varchar2(23) := '1';
  function change_name(id integer) return integer;
end;
/

create or replace package body pkg is
 function change_name(id integer) return integer is
  begin
     name := 'xiexie';
	 return 1;
  end;
end;
/

insert into test values(1,'xiexie');

--print no found and no data
declare
  cursor c1 is select *from test where id = pkg.change_name(23) and name = pkg.name;
  var2 c1%rowtype;
begin
  var2.name := 'no data';
  open c1;
  fetch c1 into var2;
  if c1%notfound then
    raise info '%','no found';
  else
    raise info '%','found data';
  end if;
  raise info '%',var2.name;
  close c1;
end;
/

--print no found and no data
declare
  cursor c1 is select *from test where id = pkg.change_name(23) and name = pkg.name;
  var2 c1%rowtype;
begin
  var2.name := 'no data';
  open c1;
  fetch c1 into var2;
  if c1%notfound then
    raise info '%','no found';
  else
    raise info '%','found data';
  end if;
  raise info '%',var2.name;
  close c1;
end;
/

--print 1
begin
  raise info '%',pkg.name;
end;
/

begin
  raise info '%',pkg.change_name(23);
  raise info '%',pkg.name;
end;
/


declare
  cursor c1 is select *from test where id = pkg.change_name(23) and name = pkg.name;
  var2 c1%rowtype;
begin
  var2.name := 'no data';
  open c1;
  fetch c1 into var2;
  if c1%notfound then
    raise info'%','no found';
  else
    raise info '%','found data';
  end if;
  raise info '%',var2.name;
  close c1;
end;
/

create or replace package body pkg is
 function change_name(id integer) return integer is
  begin
     name := 'xiexie';
	 raise info '%','call change_name';
	 return 1;
  end;
end;
/

--print no found and no data
declare
  cursor c1 is select *from test where id = pkg.change_name(23) and name = pkg.name;
  var2 c1%rowtype;
begin
  var2.name := 'no data';
  open c1;
  fetch c1 into var2;
  if c1%notfound then
    raise info '%','no found';
  else
    raise info '%','found data';
  end if;
  raise info '%',var2.name;
  close c1;
end;
/

--print call change_name and found data and 1
declare
  cursor c1 is select pkg.change_name(id), pkg.name from test;
  var2 c1%rowtype;
begin
  var2.name := 'no data';
  open c1;
  fetch c1 into var2;
  if c1%notfound then
    raise info '%','no found';
  else
    raise info '%','found data';
  end if;
  raise info '%',var2.name;
  close c1;
end;
/

drop package pkg;
drop table test;


create table test_cursor(id integer,name varchar(23));

--print no data
declare
  cursor c1 is select *from test_cursor;
  var1 c1%rowtype;
 begin
   var1.name := 'no data';
   open c1;
   fetch c1 into var1;
   raise info '%',var1.name;
   close c1;
end;
/

drop table test_cursor;


--global cursor should report error
declare
  cursor c1 is select 1 from dual;
  var1 integer;
  function test_f(id integer) return integer is
  begin
    open c1;
    return 1;
  end;
  function test_f1(id integer) return integer is
  begin
     open c1;
	 return 2;
  end;
begin
  var1 := test_f(test_f1(23));
end;
/

--test ok
declare
  cursor c1 is select 1 from dual;
  var1 integer;
  function test_f1(id integer) return integer is
    cursor c2 is select 1 from dual;
  begin
     open c2;
	 return 1;
  end;
begin
 for i in 1.. 10 LOOP
   var1 := test_f1(23);
 end loop;
end;
/

--test ok
declare
  cursor c1 is select 1 from dual;
  var1 integer;
  function test_f1(id integer) return integer is
    cursor c2 is select 1 from dual;
  begin
     open c2;
	 return 1;
  end;
begin
 for i in 1.. 10 LOOP
   var1 := test_f1(23);
 end loop;
end;
/


--raise error
declare
  cursor c1 is select 1 from dual;
  var1 integer;
  function test_f1(id integer) return integer is
    cursor c1 is select 2 from dual;
	var1 integer;
	function test_ff1(id integer) return integer is
	begin
	  open c1;
	  return 1;
	end;
	function test_ff2(id integer) return integer is
	begin
	   open c1;
	   return 2;
	end;
  begin
     var1 := test_ff1(test_ff2(23));
     return var1;
  end;
begin
  var1 := test_f1(23);
end;
/

create or replace package u8c_pkg is
 type md_ref is ref cursor;
 function test_out(id out integer) return md_ref;
end;
/



create or replace package body u8c_pkg is
   function test_out(id out integer) return md_ref is
     var1 md_ref;
   begin
       open var1 for select 1 from dual;
	   id := 23;
	   return var1;
	end;
end;
/

--print 11111111
declare
  var1 integer;
  var2 u8c_pkg.md_ref;
begin
  for i in 1 .. 10 LOOP
    var2 := u8c_pkg.test_out(var1);
	fetch var2 into var1;
	close var2;
	raise info '%',var1;
  end loop;
end;
/

drop package u8c_pkg;
DROP TABLE t_refcursor;
DROP TABLE t_cursor_test;


--add ora_refcursor
create table departments(department_id integer,department_name varchar(256), manager_id integer,location_id integer);
insert into departments values(1,'wanghong',1,2);
insert into departments values(2,'zhangwei',1,2);
insert into departments values(3,'zhousan',1,2);

create table employees(employee_id integer, first_name varchar(256),last_name varchar(256),salary integer,job_id integer);

insert into employees values(1,'zhang','san',200000,1);
insert into employees values(2,'wang','wu',4000000,2);
insert into employees values(3,'zhou','yang',500000,3);
insert into employees values(4,'li','dong',6000000,4);

create table locations(country_id varchar(256), name varchar(256));
insert into locations values('JP','riben');
insert into locations values('AM','american');
insert into locations values('CH','china');

create table mds(ids integer,name varchar(23));
insert into mds values(1,'xiexie');
insert into mds values(1,'zhangwei');

CREATE TABLE dept_temp AS
SELECT * FROM departments;

--test
declare
	id integer;
	l_name varchar(256);
	l_num integer;
	cursor c1 is select name,ids *id from mds where ids = id;
begin
 id := 1;
 open c1;
 LOOP
 fetch c1 into l_name,l_num;
 exit when c1%NOTFOUND;
 raise info 'l_name=%',l_name;
 raise info 'l_num=%',l_num;
 end LOOP;
 close c1;
end;
/


declare
 cursor c1 is select * from mds;
 r c1%rowtype;
 id integer;
begin
 id := r.ids;
 raise notice 'id=%',id;
 raise notice 'r=%',r;
end;
/

--test a table has droppend column
create table test_dropped(id integer,name varchar(23),salary integer);
alter table test_dropped drop column salary;
insert into test_dropped values(1,'xiexie');

--test ok
declare
 cursor c1 is select * from test_dropped;
 var1 c1%rowtype;
begin
    select * into var1 from test_dropped;
    raise info '%',var1;
end;
/

drop table test_dropped;


--test ref cursor as out parameter
create or replace function test_udt_var(id integer, refs out sys_refcursor) return sys_refcursor as
  type cursor_type is ref cursor;
  cursor_var1 cursor_type;
  cursor_var2 cursor_type;
begin
   open cursor_var1 for select * from employees;
   open cursor_var2 for select * from mds;
   refs := cursor_var1;
   return cursor_var2;
end;
/

declare
  type cursor_type is ref cursor;
  cursor_var1 cursor_type;
  cursor_var2 cursor_type;
  rowvar1 employees%rowtype;
  rowvar2 mds%rowtype;
begin
   cursor_var2 := test_udt_var(23,cursor_var1);
   LOOP
	FETCH cursor_var1 into rowvar1;
        EXIT WHEN cursor_var1%NOTFOUND;
        raise info '%',rowvar1;
   END LOOP;
   LOOP
        FETCH cursor_var2 into rowvar2;
        EXIT WHEN cursor_var2%NOTFOUND;
        raise info '%', rowvar2;
   END LOOP;
   close cursor_var1;
   close cursor_var2;
end;
/

--test more than one out parameters
create or replace function test_udt_var(id out integer, refs1 out sys_refcursor, refs2 out sys_refcursor) return sys_refcursor as
  type cursor_type is ref cursor;
  cursor_var1 cursor_type;
  cursor_var2 cursor_type;
  cursor_var3 cursor_type;
begin
   open cursor_var1 for select * from employees;
   cursor_var3 := test_udt_var(23,cursor_var2);
   id := 3;
   refs1 := cursor_var1;
   refs2 := cursor_var2;
   return cursor_var3;
end;
/

declare
  type cursor_type is ref cursor;
  cursor_var1 cursor_type;
  cursor_var2 cursor_type;
  cursor_var3 cursor_type;
  rowvar record;
  id integer;
begin
   cursor_var3 := test_udt_var(id, cursor_var1, cursor_var2);
   raise info '%', id;
   raise info '%', 'cursor_var1 context';
   LOOP
        FETCH cursor_var1 into rowvar;
        EXIT WHEN cursor_var1%NOTFOUND;
        raise info '%',rowvar;
   END LOOP;
   raise info '%','cursor_var2 context';
   LOOP
        FETCH cursor_var2 into rowvar;
        EXIT WHEN cursor_var2%NOTFOUND;
        raise info '%', rowvar;
   END LOOP;
   raise info '%', 'cursor_var3 context';
   LOOP
       FETCH cursor_var3 into rowvar;
       EXIT WHEN cursor_var3%NOTFOUND;
       raise info '%', rowvar;
   END LOOP;
   close cursor_var1;
   close cursor_var2;
   close cursor_var3;
end;
/

drop function test_udt_var(integer,sys_refcursor,sys_refcursor);
drop function test_udt_var(integer,sys_refcursor);


--test type failed
create type test_type as (id integer,name sys_refcursor);
create type test_type as (id integer);
--test add attribute failed
alter type test_type add attribute name sys_refcursor;
--test alter attribute failed
alter type test_type alter attribute id set data type sys_refcursor;
drop type test_type;

--test table all failed
create table test_refcurosr(id integer,name sys_refcursor);
create table test_refcursor(id integer);

--test add column
alter table test_refcursor add column name sys_refcursor;
--test alter column
alter table test_refcursor alter column id set data type sys_refcursor;
drop table test_refcursor;


--two null
create or replace function test_udt_var(id integer,out_var out sys_refcursor) return sys_refcursor
as
   c1 sys_refcursor;
   c2 sys_refcursor;
begin
    out_var := c1;
    return c2;
end;
/

declare
  c1 sys_refcursor;
  c2 sys_refcursor;
begin
    c2 := test_udt_var(23,c1);
    raise info '%,%',c1,c2;
end;
/
--one null one open
create or replace function test_udt_var(id integer,out_var out sys_refcursor) return sys_refcursor
as
   c1 sys_refcursor;
   c2 sys_refcursor;
begin
    out_var := c1;
    open out_var for select * from mds;
    return c2;
end;
/

declare
   c1 sys_refcursor;
   c2 sys_refcursor;
   r record;
begin
   c2 := test_udt_var(23,c1);
   if c2%isopen then
     fetch c2 into r;
     raise info 'c2:%',r;
     close c2;
   end if;
   if c1%isopen then
      fetch c1 into r;
      raise info 'c1:%',r;
      close c1;
   end if;
end;
/
drop function test_udt_var(integer,sys_refcursor);

--test assignment by function
create or replace function test_cursor(arg1 out sys_refcursor) return sys_refcursor as
    type cur_type is ref cursor return mds%rowtype;
    type cur_type2 is ref cursor return employees%rowtype;
    var1 cur_type;
    var2 cur_type2;
begin
   open var1 for select * from mds;
   open var2 for select * from employees;
   arg1 := var2;
   return var1;
end;
/

declare
 type cur_type1 is ref cursor return mds%rowtype;
 type cur_type2 is ref cursor return employees%rowtype;
 var1 cur_type1;
 var2 cur_type2;
 r record;
begin
    var1 := test_cursor(var2);
    fetch var1 into r;
    raise info 'var1:%',var1;
    raise info '%',r;
    fetch var2 into r;
    raise info 'var2:%',var2;
    raise info '%',r;
    close var1;
    close var2;
end;
/

--test function call function
create or replace function test_udt_var(id out sys_refcursor,name out varchar) return sys_refcursor as
  type cur_type is ref cursor return mds%rowtype;
  type cur_type2 is ref cursor return employees%rowtype;
  var1 cur_type;
  var2 cur_type2;
begin
   open var1 for select * from mds;
   open var2 for select * from employees;
   id := var1;
   name := 'interal function';
   return var2;
end;
/
create or replace function test_cursor(id out sys_refcursor,name out varchar) return sys_refcursor as
     c1 sys_refcursor;
begin
    id := test_udt_var(c1,name);
    return c1;
end;
/

declare
  c1 sys_refcursor;
  c2 sys_refcursor;
  r record;
  name varchar(256);
begin
    c1 := test_cursor(c2,name);
    raise info '%',name;
    fetch c1 into r;
    raise info 'c1:%',c1;
    raise info '%',r;
    raise info 'c2:%',c2;
    raise info '%',r;
    c1 := c2;
    close c1;
end;
/
drop function test_cursor(sys_refcursor,varchar);
drop function test_udt_var(sys_refcursor,varchar);

--drop tables
drop table mds;
drop table departments;
drop table employees;
drop table locations;
drop table dept_temp;


create table tb1(id number(4), name varchar2(30));
insert into tb1 values (10, 'smith');
insert into tb1 values (20, 'allen');
insert into tb1 values (30, 'ward');
create table tb2(t_id number(4), t_name varchar2(30));

--ok
declare
 c1 sys_refcursor;
 v_id    tb1.id%type;
 v_name tb1.name%type;
begin
 open c1 for select id, name from tb1 using;
 loop
   fetch c1 into v_id, v_name;
   exit when c1%notfound;
   insert into tb2 values(v_id, v_name);
 end loop;
 close c1;
end;
/

--ok
declare
 c1 sys_refcursor;
 v_id    tb1.id%type;
 v_name tb1.name%type;
begin
 open c1 for select id, name from tb1
 using;
 loop
   fetch c1 into v_id, v_name;
   exit when c1%notfound;
   insert into tb2 values(v_id, v_name);
 end loop;
 close c1;
end;
/

--error
declare
 c1 sys_refcursor;
 v_id    tb1.id%type;
 v_name tb1.name%type;
begin
 open c1 for select id, name from tb1 WHERE id = :1 using 1;
 loop
   fetch c1 into v_id, v_name;
   exit when c1%notfound;
   insert into tb2 values(v_id, v_name);
 end loop;
 close c1;
end;
/

create or replace package pkg is
 procedure p1(id integer);
end;
/

create or replace package body pkg is
 procedure p1(id integer) is
  c_id tb1.id%type;
  c_name  tb1.name%type;
  type typ1 is ref cursor return tb1%rowtype;
  c1 typ1;
  procedure p1_1(c1 in out typ1) is
   begin
   open c1 for select * from tb1;
  end;
 begin
  call p1_1(c1);
  loop
   fetch c1 into c_id, c_name;
    exit when c1%notfound;
   insert into tb2 values(c_id, c_name);
  end loop;
  close c1;
 end;
begin
 null;
end;
/

--ok
call pkg.p1(1);


create or replace package body pkg is
 procedure p1(id integer) is
  c_id tb1.id%type;
  c_name  tb1.name%type;
  type typ1 is ref cursor return tb1%rowtype;
  c1 typ1;
  c2 typ1;
  procedure p1_1(c1 in out typ1, c2 out typ1) is
   begin
   open c1 for select * from tb1;
   OPEN c2 FOR SELECT * FROM tb2;
  end;
 begin
  call p1_1(c1, c2);
  loop
   fetch c1 into c_id, c_name;
    exit when c1%notfound;
   insert into tb2 values(c_id, c_name);
  end loop;
  close c1;
  loop
   fetch c2 into c_id, c_name;
    exit when c2%notfound;
   insert into tb2 values(c_id, c_name);
  end loop;
  close c2;
 end;
begin
 null;
end;
/

--ok
call pkg.p1(1);

--ok
declare
 cursor c1 is select * from tb1;
 tab1 c1%rowtype;
 tab2 c1%rowtype;
 tab3 c1%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/


--ok
declare
 cursor c1 is select * from tb1;
 type c2 is ref cursor return c1%rowtype;
 var2 c2;
 tab1 var2%rowtype;
 tab2 var2%rowtype;
 tab3 var2%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

--ok
declare
 cursor c1 is select * from tb1;
 type c2 is ref cursor return tb1%rowtype;
 var2 c2;
 tab1 var2%rowtype;
 tab2 var2%rowtype;
 tab3 var2%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

--error
declare
 cursor c1 is select * from tb1;
 type c2 is ref cursor return c1%rowtype;
 var2 c2;
 tab1 record;
 tab2 record;
 tab3 record;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

--test package'cursor attribute
DROP package pkg;
CREATE OR REPLACE package pkg IS
  CURSOR c1 IS SELECT * FROM tb1 ORDER BY id;
  FUNCTION test_f(id integer) RETURN integer;
  var1 integer;
end;
/

CREATE OR REPLACE package body pkg IS
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
    CLOSE c1;
	RETURN 1;
  end;
BEGIN
	OPEN c1;
end;
/

BEGIN
  pkg.var1 := 1;
end;
/

DECLARE
  var1 tb1%rowtype;
  ret integer;
BEGIN
  IF pkg.c1%isopen then
	  LOOP
		FETCH pkg.c1 INTO var1;
		EXIT WHEN pkg.c1%notfound;
		raise info '%',var1;
	 END loop;
 END if;
 raise info '%',pkg.c1%rowcount;
 ret := pkg.test_f(1);
 IF pkg.c1%isopen THEN
   CLOSE pkg.c1;
 END if;
end;
/

--check data
DELETE FROM tb1;
insert into tb1 values (10, 'smith');
insert into tb1 values (20, 'allen');
insert into tb1 values (30, 'ward');

DELETE FROM tb2;

declare
 cursor c1 is select * from tb1;
 tab1 c1%rowtype;
 tab2 c1%rowtype;
 tab3 c1%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

SELECT * FROM tb2 ORDER BY t_id;
DELETE FROM tb2;


--ok
declare
 cursor c1 is select * from tb1;
 type c2 is ref cursor return c1%rowtype;
 var2 c2;
 tab1 var2%rowtype;
 tab2 var2%rowtype;
 tab3 var2%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

SELECT * FROM tb2 ORDER BY t_id;
DELETE FROM tb2;

--ok
declare
 cursor c1 is select * from tb1;
 type c2 is ref cursor return tb1%rowtype;
 var2 c2;
 tab1 var2%rowtype;
 tab2 var2%rowtype;
 tab3 var2%rowtype;
begin
 open c1;
 fetch c1 into tab1;
 fetch c1 into tab2;
 fetch c1 into tab3;
 insert into tb2 values tab1;
 insert into tb2 values tab2;
 insert into tb2 values tab3;
 close c1;
end;
/

SELECT * FROM tb2 ORDER BY t_id;
DELETE FROM tb2;

--clean data
DROP TABLE tb1;
DROP TABLE tb2;
DROP package pkg;

create table tb1(id integer,name varchar2(256));

create or replace package test_pkg is
  type ctype is ref cursor;
end;
/

insert into tb1 values(1,'one');
insert into tb1 values(2,'two');
insert into tb1 values(3,'three');

--ok
declare
  var1 test_pkg.ctype;
  var2 tb1%rowtype;
  function test_f(id integer) return test_pkg.ctype is
    type ctype1 is ref cursor;
    var1 ctype1;
  begin
    open var1 for select * from tb1 ORDER BY id;
    return var1;
  end;
begin
  var1 := test_f(1);
  raise info '%',var1;
  LOOP
    fetch var1 into var2;
    exit when var1%notfound;
    raise info '%', var2;
  end loop;
end;
/

create or replace package test_pkg is
  type ctype is ref cursor return tb1%rowtype;
end;
/

--ok
declare
  var1 test_pkg.ctype;
  var2 tb1%rowtype;
  function test_f(id integer) return test_pkg.ctype is
    type ctype1 is ref cursor;
    var1 ctype1;
  begin
    open var1 for select * from tb1 ORDER BY id;
    return var1;
  end;
begin
  var1 := test_f(1);
  raise info '%',var1;
  LOOP
    fetch var1 into var2;
    exit when var1%notfound;
    raise info '%', var2;
  end loop;
end;
/

--error
declare
  var1 test_pkg.ctype;
  var2 tb1%rowtype;
  function test_f(id integer) return test_pkg.ctype is
    type ctype1 is ref cursor return tb1%rowtype;
    var1 ctype1;
  begin
    open var1 for select * from tb1;
    return var1;
  end;
begin
  var1 := test_f(1);
  raise info '%',var1;
  LOOP
    fetch var1 into var2;
    exit when var1%notfound;
    raise info '%', var2;
  end loop;
end;
/

--ok
create or replace function test_f(id integer) return integer is
  var1 test_pkg.ctype;
  var2 tb1%rowtype;
  function test_f(id integer) return test_pkg.ctype is
    type ctype1 is ref cursor;
    var1 ctype1;
  begin
    open var1 for select * from tb1 ORDER BY id;
    return var1;
  end;
begin
  var1 := test_f(1);
  raise info '%',var1;
  LOOP
    fetch var1 into var2;
    exit when var1%notfound;
    raise info '%', var2;
  end loop;
  return 1;
end;
/

--ok
declare
  ret integer;
  var1 integer;
begin
  for var1 in 1 .. 10 LOOP
    ret := test_f(var1);
  end loop;
end;
/

--50
show open_cursors;

discard all;

--ok 50
DECLARE
  id integer;
  ret integer;
  var3 sys_refcursor;
  FUNCTION test_f(id integer) RETURN sys_refcursor IS
    var1 sys_refcursor;
        var2 varchar2(256);
  BEGIN
    var2 := 'select id from tb1';
    OPEN var1 FOR var2;
    FETCH var1 INTO ret;
    RETURN var1;
 end;
BEGIN
  FOR id IN 1 .. 49 LOOP
    var3 := test_f(id);
  END loop;
end;
/

--failed
DECLARE
  id integer;
  ret integer;
  var3 sys_refcursor;
  FUNCTION test_f(id integer) RETURN sys_refcursor IS
    var1 sys_refcursor;
        var2 varchar2(256);
  BEGIN
    var2 := 'select id from tb1';
    OPEN var1 FOR var2;
    FETCH var1 INTO ret;
    RETURN var1;
 end;
BEGIN
  FOR id IN 1 .. 50 LOOP
    var3 := test_f(id);
  END loop;
end;
/

discard all;

SET open_cursors = 51;

--ok
DECLARE
  id integer;
  ret integer;
  var3 sys_refcursor;
  FUNCTION test_f(id integer) RETURN sys_refcursor IS
    var1 sys_refcursor;
        var2 varchar2(256);
  BEGIN
    var2 := 'select id from tb1';
    OPEN var1 FOR var2;
    FETCH var1 INTO ret;
    RETURN var1;
 end;
BEGIN
  FOR id IN 1 .. 50 LOOP
    var3 := test_f(id);
  END loop;
end;
/

discard all;

reset open_cursors;


--test dump
create or replace function test_f(id integer) return integer is
# option dump
  type c1 is ref cursor return tb1%rowtype;
  type c2 is ref cursor;
  cursor c3 return tb1%rowtype;
  cursor c3 return tb1%rowtype is select * from tb1;
  var1 c1;
  var2 c2;
  var3 c3%rowtype;
begin
  var3.id := 1;
  return 1;
end;
/


create or replace package test_pkg is
  # option dump
  type c1 is ref cursor return tb1%rowtype;
  type c2 is ref cursor;
  cursor c3 return tb1%rowtype;
  cursor c3 return tb1%rowtype is select * from tb1;
  var3 c3%rowtype;
  function test_f(id integer,name varchar2(256)) return integer;
end;
/

create or replace package body test_pkg is
  # option dump
  type c11 is ref cursor return tb1%rowtype;
  type c21 is ref cursor;
  cursor c33 return tb1%rowtype;
  cursor c33 return tb1%rowtype is select * from tb1;
  var4 c33%rowtype;
  function test_f(id integer,name varchar2(256)) return integer is
  begin
    return 1;
  end;
end;
/

CREATE OR REPLACE package test_pkg IS
  type c1 is ref cursor return tb1%rowtype;
  type c2 is ref cursor;
  cursor c3 return tb1%rowtype;
  cursor c3 return tb1%rowtype is select * from tb1;
  var3 c3%rowtype;
  FUNCTION f1(x anyelement) RETURN anyelement;
end;
/

CREATE OR REPLACE package body test_pkg is
   function f1(x anyelement) return anyelement as
   BEGIN
	open c3;
	LOOP
	  fetch c3 into var3;
	  exit when c3%notfound;
	  raise info 'var3=%',var3;
	end loop;
	close c3;
	return x + 1;
   END;
end;
/

declare
  var1 integer;
  var2 number;
begin
   var1 := test_pkg.f1(42);
   var2 := test_pkg.f1(4.5);
   raise info 'var1=%,var2=%',var1,var2;
end;
/

declare
  var1 integer;
  var2 number;
begin
   var1 := test_pkg.f1(42);
   var2 := test_pkg.f1(4.5);
   raise info 'var1=%,var2=%',var1,var2;
end;
/



--ok
CREATE OR REPLACE package test_pkg is
  type c1 is ref cursor return tb1%rowtype;
  type c2 is ref cursor;
  cursor c3 return tb1%rowtype;
  cursor c3 return tb1%rowtype is select * from tb1;
  var3 c3%rowtype;
  function f1(x anyarray) return anyarray;
end;
/
CREATE OR REPLACE package body test_pkg IS
  FUNCTION f1(x anyarray) RETURN anyarray is
  begin
     open c3;
	LOOP
	  fetch c3 into var3;
	  exit when c3%notfound;
	  raise info 'var3=%',var3;
	end loop;
	close c3;
     return x;
  end;
end;
/

BEGIN
  raise info '%,%', test_pkg.f1(array[2,4]),test_pkg.f1(array[4.5, 7.7]);
end;
/

BEGIN
  raise info '%,%', test_pkg.f1(array[4,5]),test_pkg.f1(array[5.5, 7.8]);
end;
/

--clean data
DROP FUNCTION test_f;
DROP package test_pkg;
DROP TABLE tb1;

--bug#Z1350
create table tb1(id number(4), name varchar2(30));

insert into tb1 values (10, 'smith');
insert into tb1 values (20, 'allen');
insert into tb1 values (30, 'ward');

--ok
declare
	cursor c1 is select * from tb1;
	cursor c2 return c1%rowtype;
begin
	null;
end;
/


--ok
declare
	cursor c1 is select * from tb1;
	cursor c2 return c1%rowtype is select * from tb1 order by id;
	var2 c2%rowtype;
begin
	open c2;
	LOOP
	   fetch c2 into var2;
	   exit when c2%notfound;
	   raise info '%',var2;
	end loop;
	close c2;
end;
/

create table test_1(id integer);

--error
declare
	cursor c1 is select * from tb1;
	cursor c2 return c1%rowtype is select * from test_1 order by id;
	var2 c2%rowtype;
begin
	open c2;
	LOOP
	   fetch c2 into var2;
	   exit when c2%notfound;
	   raise info '%',var2;
	end loop;
	close c2;
end;
/

--ok
declare
  type typ1 is record(id number(4), name varchar2(30));
  r1 typ1;
  cursor c1 return r1%type;
begin
  NULL;
end;
/

--ok
declare
  type typ1 is record(id number(4), name varchar2(30));
  r1 typ1;
  cursor c1 return r1%type;
  cursor c1 return r1%type is select * from tb1 order by id;
begin
  open c1;
  LOOP
    fetch c1 into r1;
    exit when c1%notfound;
    raise info '%', r1;
  end loop;
end;
/

--ok
declare
  type typ1 is record(ids number(4), names varchar2(30));
  r1 typ1;
  cursor c1 return r1%type;
  cursor c1 return r1%type is select * from tb1 order by id;
begin
  open c1;
  LOOP
    fetch c1 into r1;
    exit when c1%notfound;
    raise info '%', r1;
  end loop;
end;
/

--faield
declare
  type typ1 is record(ids number(4), names varchar2(30), mdss varchar2(23));
  r1 typ1;
  cursor c1 return r1%type;
  cursor c1 return r1%type is select * from tb1 order by id;
begin
  open c1;
  LOOP
    fetch c1 into r1;
    exit when c1%notfound;
    raise info '%', r1;
  end loop;
end;
/

--failed
declare
  type typ1 is record(ids number(4), names varchar2(30));
  r1 typ1;
  cursor c1 return r1%type;
  cursor c1 return r1%type is select * from tb1 order by id;
  type typ2 is record(ids number(4), names varchar2(30),name1 varchar2(30));
  r2 typ2;
begin
  open c1;
  LOOP
    fetch c1 into r2;
    exit when c1%notfound;
    raise info '%', r2;
  end loop;
end;
/

--failed
declare
  type typ1 is record(id number(4), name varchar2(30));
  r1 typ1;
  cursor c1 return r1%type;
  cursor c1 return r1%type is select * from test_1;
begin
  open c1;
  LOOP
    fetch c1 into r1;
    exit when c1%notfound;
    raise info '%', r1;
  end loop;
end;
/

--ok
declare
  type typ1 is record(id number(4), name varchar2(30));
  cursor c1 return typ1;
begin
  NULL;
end;
/

--ok
declare
  type typ1 is record(id number(4), name varchar2(30));
  cursor c1 return typ1 is select * from tb1 order by id;
  var1 c1%rowtype;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%', var1;
  end loop;
end;
/

--failed
declare
  type typ1 is record(id number(4), name varchar2(30));
  cursor c1 return typ1 is select * from test_1;
  var1 c1%rowtype;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%', var1;
  end loop;
end;
/


create table tb2(id number(4), name varchar2(30), ids integer);

insert into tb2 values (10, 'smith', 1);
insert into tb2 values (20, 'allen', 2);
insert into tb2 values (30, 'ward', 3);

--error
declare
  type typ1 is record(id number(4), name varchar2(30));
  cursor c1 return typ1 is select * from tb2;
  var1 c1%rowtype;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%', var1;
  end loop;
end;
/

--error
declare
 cursor c1 is select * from tb1;
 var1 tb2%rowtype;
begin
  open c1;
  fetch c1 into var1;
  close c1;
end;
/


--error
declare
 cursor c1 is select * from tb1;
 cursor c2 is select * from tb2;
 var1 c2%rowtype;
begin
  open c1;
  fetch c1 into var1;
  close c1;
end;
/

alter table tb2 drop column ids;

--ok
declare
  type typ1 is record(id number(4), name varchar2(30));
  cursor c1 return typ1 is select * from tb2 order by id;
  var1 c1%rowtype;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%', var1;
  end loop;
end;
/

--clean data
drop table tb2;
drop table tb1;
drop table test_1;

--bug#Z1372
create type test_rowtype as (id integer,name varchar2(256));

create table test_mds(id test_rowtype,name varchar2(256));

insert into test_mds values('(1,abc)','one');
insert into test_mds values('(2,cdef)','two');

--ok
declare
  cursor c1 is select id from test_mds order by name;
  var1 c1%rowtype;
begin
  open c1;
  loop
    fetch c1 into var1;
    exit when c1%notfound;
    raise info '%',var1;
    raise info '%',var1.id;
  end loop;
  close c1;
 end;
 /


--ok
declare
  cursor c1 is select * from test_mds order by name;
  var1 c1%rowtype;
begin
  open c1;
  loop
    fetch c1 into var1;
    exit when c1%notfound;
    raise info '%',var1;
  end loop;
  close c1;
 end;
 /

--ok
declare
  cursor c1 is select id from test_mds order by name;
  var1 test_rowtype;
begin
  open c1;
  loop
    fetch c1 into var1;
    exit when c1%notfound;
    raise info '%',var1;
  end loop;
  close c1;
 end;
 /

--ok
declare
  var1 test_rowtype;
begin
  select id into var1 from test_mds limit 1;
 end;
 /

create type test_mdsss as (id integer,name varchar2(256),name2 varchar2(256));

--error
declare
  cursor c1 is select id from test_mds order by name;
  var1 test_mdsss;
begin
  open c1;
  loop
    fetch c1 into var1;
    exit when c1%notfound;
    raise info '%',var1;
  end loop;
  close c1;
 end;
 /

set ivorysql.default_with_rowids to on ;
create table tb1_tirt001171843 (a int,b int);
insert into tb1_tirt001171843 values (100, 101);
create or replace function tf_r() return int
as
declare
	rr rowid;
begin
	select rowid from tb1_tirt001171843 into rr;
	return 1;
end;
/

--ok
select tf_r() from dual;


--two columns into one rowtype var
create table test_table(id integer,name varchar2(256));

--ok
declare
  var1 test_table%rowtype;
  cursor c1 is select id,name from test_table;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
 end loop;
end;
/

create table test_one_col(id integer);

--ok
declare
  var1 test_one_col%rowtype;
  cursor c1 is select id from test_one_col;
begin
  open c1;
  fetch c1 into var1;
  close c1;
end;
/

drop table test_one_col;

create table test_one_col(id test_mdsss);

insert into test_one_col values((1,'one','two'));
insert into test_one_col values((2,'three','two'));

--ok
declare
  var1 test_one_col%rowtype;
  cursor c1 is select id from test_one_col order by id;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%',var1;
	raise info '%', var1.id;
  end loop;
  close c1;
end;
/

--ok
declare
  var1 test_mdsss;
  cursor c1 is select id from test_one_col order by id;
begin
  open c1;
  loop
	fetch c1 into var1;
	exit when c1%notfound;
	raise info '%',var1;
	raise info '%', var1.id;
  end loop;
  close c1;
end;
/

--clean data
drop table tb1_tirt001171843;
drop table test_one_col;
drop table test_table;
drop function tf_r;
drop table test_mds;
drop type test_mdsss;
drop type test_rowtype;


create type test_mds as (id integer);

create table test_mdss(id test_mds);

insert into test_mdss values('(1)');

--ok
declare
  var1 test_mds;
begin
  select id into var1 from test_mdss;
  raise info '%',var1;
end;
/

--ok
declare
  var1 test_mdss%rowtype;
begin
  select id into var1 from test_mdss;
end;
/

--ok
declare
  var1 test_mdss%rowtype;
begin
  select * into var1 from test_mdss;
end;
/

--ok
declare
  var1 test_mds;
begin
  select * into var1 from test_mdss;
end;
/

--ok
declare
  var1 test_mdss.id%type;
begin
  select * into var1 from test_mdss;
end;
/


insert into test_mdss values('(2)');
insert into test_mdss values('(3)');

--ok
declare
  var1 test_mds;
  cursor c1 is select id from test_mdss order by id;
  var2 c1%rowtype;
  var3 test_mdss.id%type;
begin
  open c1;
  fetch c1 into var1;
  fetch c1 into var2;
  fetch c1 into var3;
  raise info '%-%-%',var1,var2,var3;
  close c1;
end;
/

--ok
declare
  var1 test_mds;
  cursor c1 is select * from test_mdss order by id;
  var2 c1%rowtype;
  var3 test_mdss.id%type;
begin
  open c1;
  fetch c1 into var1;
  fetch c1 into var2;
  fetch c1 into var3;
  raise info '%-%-%',var1,var2,var3;
  close c1;
end;
/

--clean data
drop table test_mdss;
drop type test_mds;

--bug#Z1504
create table test_wq(id integer,name varchar2(256));

insert into test_wq values(1,'one');

create or replace package test_pkg is
 type c_cursor is ref cursor;
end;
/

create or replace procedure test1(agsdm varchar2,pRecCur in out test_pkg.c_cursor) is
  aSQL varchar2(8000);
begin
  open PrecCur for select * from test_wq;
end;
/

--ok
declare
  var1 test_pkg.c_cursor;
  var2 test_wq%rowtype;
begin
  test1('xiexie', var1);
  commit;
  loop
	fetch var1 into var2;
	exit when var1%notfound;
	raise info '%', var2;
  end loop;
  close var1;
end;
/

create or replace procedure test2(agsdm varchar2, pRecCur in out test_pkg.c_cursor) is
  aSQL varchar2(8000);
begin
  aSQL := 'select * from test_wq; ';
  open pRecCur for aSQL;
  if pRecCur%isopen then
    raise info '%', 'pRecCur is open';
    raise info '%-%-%', pRecCur%found,pRecCur%notfound, pRecCur%rowcount;
  end if;
end;
/

--ok
declare
  var1 test_pkg.c_cursor;
  var2 test_wq%rowtype;
begin
  test2('xiexie', var1);
  commit;
  loop
	fetch var1 into var2;
	exit when var1%notfound;
	raise info '%', var2;
  end loop;
  close var1;
end;
/

--clean data
drop procedure test1;
drop procedure test2;
drop table test_wq;
DROP package test_pkg;


--bug#M454
create type test_mds as (id integer);

create table test_mdss(id test_mds);
insert into test_mdss values('(1)');

--ok
DECLARE
  type c_type IS record(id test_mds);
  CURSOR c1 RETURN c_type IS SELECT id FROM test_mdss;
  var1 c_type;
BEGIN
  OPEN c1;
  LOOP
    FETCH c1 INTO var1;
    EXIT WHEN c1%notfound;
    raise info '%', var1.id;
  END loop;
end;
/


--ok
declare
  type c_type is record(id test_mds);
  var1 c_type;
begin
  select id into var1 from test_mdss;
  raise info '%',var1.id;
end;
/

--clean data
DROP TABLE test_mdss;
DROP type test_mds;


--bug#Z1349
create table tb1(id number(4), name varchar2(30));

insert into tb1 values (10, 'smith');
insert into tb1 values (20, 'allen');
insert into tb1 values (30, 'ward');

create table tb2(t_id number(4), t_name varchar2(30));


--ok and print sql is found
declare
	c number(4);
	count number(4);
begin
	select id into c from tb1;
exception
when too_many_rows then
	count := sql%rowcount;
	if (sql%isopen) then
	   raise info '%','sql is open';
	end if;
	if (sql%found) then
	   raise info '%','sql is found';
	end if;
	insert into tb2 values (count, 'scott');
end;
/

--1 scott
select * from tb2;

--bug#Z1351
--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1' using '20';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1' using in '20';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1 and name = :2' using '20','allen';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1 and name = :2' using IN '20',IN 'allen';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1 and name = :2' using IN '20','allen';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--ok
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1 and name = :2' using '20',IN 'allen';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--error
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1' using out '20';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--error
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1' using in out '20';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

--error
declare
	c1 sys_refcursor;
	v_id tb1.id%type;
	v_name tb1.name%type;
begin
	open c1 for 'select id, name from tb1 where id = :1 and name = :2' using '20',out 'allen';
	loop
		fetch c1 into v_id, v_name;
		exit when c1%notfound;
		insert into tb2 values(v_id, v_name);
	end loop;
	close c1;
end;
/

SELECT * FROM tb2 ORDER BY t_id;

--clean data
DROP TABLE tb2;
DROP TABLE tb1;

--bug#Z5404
--ok raise this is invalid cursor
declare
  cursor c1 is select 1 as id from dual;
 begin
   close c1;
 exception
  when invalid_cursor then
     raise info '%', 'this is invalid cursor';
 end;
 /

 --ok raise this is invalid cursor
 declare
  cursor c1 is select 1 as id from dual;
  var1 c1%rowtype;
 begin
   fetch c1 into var1;
 exception
  when invalid_cursor then
     raise info '%', 'this is invalid cursor';
 end;
 /

--ok raise this is invalid cursor
declare
	cursor c1 is select 1 as id from dual;
begin
	close c1;
	loop
		raise notice '%','cursor record: ' || record.id;
	end loop;
exception
	when invalid_cursor then
	raise notice '%','this is invalid_cursor';
	when others then
	raise notice '%','unexpected error: ' || sqlerrm;
end;
/

SET SERVEROUTPUT ON
create table tb1(id number(4), name varchar2(30));
insert into tb1 values (10, 'smith');
insert into tb1 values (20, 'allen');
insert into tb1 values (30, 'ward');
create table tb2(t_id number(4), t_name varchar2(30));

declare
  a int[] := array[1,2,3,4];
  cursor c1 is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1;
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  dbms_output.put_line('row_count:' || c1%rowcount);
  raise info 'row_count: %', c1%rowcount;
  insert into tb2 values(v_id, 'v_name');
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1;
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  raise info 'a is: %', a;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('row_count:' || c1%rowcount);
  raise info 'row_count: %', c1%rowcount;
  end loop;
end;
/


declare
  a int[] := array[1,2,3,4];
  cursor c1 is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1;
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  raise info 'a is: %', a[1];
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('row_count:' || c1%rowcount);
  raise info 'row_count: %', c1%rowcount;
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1;
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('row_count:' || c1%rowcount);
  raise info 'row_count: %', c1%rowcount;
  raise info 'a is: %', a[2];
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 (minid integer , maxid integer = 2) is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1(1);
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('row_count:' || c1%rowcount);
  raise info 'row_count: %', c1%rowcount;
  raise info 'a is: %', a[2];
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 (minid integer , maxid integer = 2) is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1(1);
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('a[1]=' || a[1] || ', rowcount=' || c1%rowcount || ', a[2]=' || a[2]);
  raise info 'a[1]=%, row_count=%, a[2]=%', a[1], c1%rowcount, a[2];
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 (minid integer , maxid integer = 2) is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1(1);
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('rowcount=' || c1%rowcount || ', a[1]=' || a[1] || ', a[2]=' || a[2]);
  raise info 'row_count=%, a[1]=%, a[2]=%', c1%rowcount, a[1], a[2];
  end loop;
end;
/

declare
  a int[] := array[1,2,3,4];
  cursor c1 (minid integer , maxid integer = 2) is select * from unnest(a);
  v_id tb1.id%type;
begin
  open c1(1);
  loop
  fetch c1 into v_id;
  exit when c1%NOTfound;
  insert into tb2 values(v_id, 'v_name');
  dbms_output.put_line('a[1]=' || a[1] || ', a[2]=' || a[2] || ', rowcount=' || c1%rowcount);
  raise info 'a[1]=%, a[2]=%, row_count=%', a[1], a[2], c1%rowcount;
  end loop;
end;
/

declare
  TYPE first_type IS TABLE OF int;
  TYPE second_type IS TABLE OF first_type;
  a second_type;
  b first_type := first_type(10);
  c second_type;
  d second_type;

  type deptrectyp is record (
  dept_id integer not null := 10,
  dept_name varchar(30) not null := 'admins',
  mgr_id     integer,
  loc_id     integer
  );
  record_var deptrectyp;

  arr int[] := array[1,2,3];
  cursor c1 is select * from unnest(arr);
  v_id tb1.id%type;
begin
  a := second_type(first_type(1), first_type(2), first_type(3));
  c := second_type(first_type(21), first_type(22));
  d := second_type(first_type(31));
  a(1)(1) := 11;
  a(1)(1) := b(1);
  a(1)(1) := c(1)(1);
  d := a;

  raise notice '%,%,%', a(1)(1),a(2)(1),a(3)(1);
  raise notice '%,%,%', d(1)(1),d(2)(1),d(3)(1);
  open c1;
  loop
    fetch c1 into v_id;
    exit when c1%NOTfound;
    dbms_output.put_line('a(1)(1)=' || a(1)(1) || ', record_var.dept_id=' || record_var.dept_id || ', rowcount=' || c1%rowcount);
    raise info 'a(1)(1):%, record_var.dept_id:%, rowcount:%', a(1)(1), record_var.dept_id, c1%rowcount;
    dbms_output.put_line('rowcount=' || c1%rowcount || ', d(2)(1)=' || d(2)(1) || ', record_var.dept_id=' || record_var.dept_id);
    raise info 'rowcount:%, d(2)(1):%, record_var.dept_id:%, ', c1%rowcount, d(2)(1), record_var.dept_id;
    dbms_output.put_line('a=' || a(c1%rowcount)(1) || ', record_var.dept_id=' || record_var.dept_id || ', rowcount=' || c1%rowcount);
    raise info 'a:%, record_var.dept_id:%, rowcount:%', a(c1%rowcount)(1), record_var.dept_id, c1%rowcount;
    dbms_output.put_line('d=' || d(c1%rowcount)(1) || ', record_var.dept_id=' || record_var.dept_id);
    raise info 'd:%, record_var.dept_id:%', d(c1%rowcount)(1), record_var.dept_id;
  end loop;
end;
/
SET SERVEROUTPUT OFF

--clean data
drop table tb2;
drop table tb1;


--bug#Z8311
--test open cursor for
create table test_select(id integer, name varchar2(256));

insert into test_select values(1, 'hello open for');

--open for sconst
--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for 'select * from test_select';
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--open for with
--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for with t1 as(select * from test_select),t2 as (select * from test_select)
                  select * from t1 union all select * from t2;
  LOOP
    fetch var1 into var2;
	exit when var1%notfound;
    raise info '%', var2;
  end loop;
  close var1;
end;
/

--open for datum
--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 varchar2(256) := 'select * from test_select';
begin
  open var1 for var3;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--open for '(' ')'
--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for ('select * from test_select');
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for ('select * from test_select'));
  fetch var1 into var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for (with t1 as(select * from test_select),t2 as (select * from test_select)
            select * from t1 union all select * from t2);
  fetch var1 into var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 varchar2(256) := 'select * from test_select';
begin
  open var1 for (var3);
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 varchar2(256) := 'select * from test_select';
begin
  open var1 for (var3));
  fetch var1 into var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for (select * from test_select);
  fetch var1 into var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for select * from test_select;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--test using
--common select
--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
begin
  var3 := 1;
  open var1 for select * from test_select where id = :x using var3;
  fetch var1 into var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
begin
  var3 := 1;
  open var1 for select * from test_select where id = :x using 1;
  fetch var1 into var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
begin
  var3 := 1;
  open var1 for 'select * from test_select where id = :x' using 1;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
begin
  var3 := 1;
  open var1 for ('select * from test_select where id = :x' using 1);
  fetch var1 into var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
begin
  var3 := 1;
  open var1 for ('select * from test_select where id = :x') using 1;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
  var4 varchar2(256) := 'select * from test_select where id = :1';
begin
  var3 := 1;
  open var1 for (var4) using 1;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--ok
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
  var4 varchar2(256) := 'select * from test_select where id = :1';
begin
  var3 := 1;
  open var1 for var4 using 1;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--error
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  var3 integer;
  var4 varchar2(256) := 'select * from test_select where id = :1';
begin
  var3 := 1;
  open var1 for (var4 using 1);
  fetch var1 into var2;
  close var1;
end;
/

CREATE TABLE dept_test (
    dept_id   NUMBER PRIMARY KEY,
    dept_name VARCHAR2(50)
);

CREATE TABLE emp_test (
    emp_id    NUMBER PRIMARY KEY,
    emp_name  VARCHAR2(50),
    salary    NUMBER,
    dept_id   NUMBER REFERENCES dept_test(dept_id)
);

INSERT INTO dept_test VALUES (10, 'ACCOUNTING');
INSERT INTO dept_test VALUES (20, 'RESEARCH');
INSERT INTO dept_test VALUES (30, 'SALES');
INSERT INTO dept_test VALUES (40, 'OPERATIONS');

INSERT INTO emp_test VALUES (1, 'SMITH',  800,  20);
INSERT INTO emp_test VALUES (2, 'ALLEN',  1600, 30);
INSERT INTO emp_test VALUES (3, 'WARD',   1250, 30);
INSERT INTO emp_test VALUES (4, 'JONES',  2975, 20);
INSERT INTO emp_test VALUES (5, 'MARTIN', 1250, 30);
INSERT INTO emp_test VALUES (6, 'BLAKE',  2850, 30);
INSERT INTO emp_test VALUES (7, 'CLARK',  2450, 10);
INSERT INTO emp_test VALUES (8, 'SCOTT',  3000, 20);
INSERT INTO emp_test VALUES (9, 'KING',   5000, 10);
INSERT INTO emp_test VALUES (10, 'TURNER', 1500, 30);
INSERT INTO emp_test VALUES (11, 'ADAMS',  1100, 20);
INSERT INTO emp_test VALUES (12, 'JAMES',   950, 30);
INSERT INTO emp_test VALUES (13, 'FORD',   3000, 20);
INSERT INTO emp_test VALUES (14, 'MILLER', 1300, 10);


CREATE OR REPLACE PACKAGE pkg_cursor_with_demo IS
    PROCEDURE proc_show_dept_salary_rank;
END pkg_cursor_with_demo;
/


CREATE OR REPLACE PACKAGE BODY pkg_cursor_with_demo IS
    PROCEDURE proc_show_dept_salary_rank IS
        cur SYS_REFCURSOR;
        v_emp_id         emp_test.emp_id%TYPE;
        v_emp_name       emp_test.emp_name%TYPE;
        v_salary         emp_test.salary%TYPE;
        v_dept_name      dept_test.dept_name%TYPE;
        v_dept_avg_salary NUMBER;
        v_dept_rank      NUMBER;
    BEGIN
        raise info '=== salary of part order  =====';
        raise info '%', RPAD('EMP_ID',6) || ' ' ||
                             RPAD('NAME',10) || ' ' ||
                             RPAD('SALARY',8) || ' ' ||
                             RPAD('DEPT',15) || ' ' ||
                             RPAD('DEPT_AVG',10) || ' ' ||
                             'DEPT_RANK';
        raise info '--------------------------------------------------------------';

        OPEN cur FOR
            WITH dept_avg_salary AS (
                SELECT dept_id,
                       AVG(salary) AS avg_sal
                FROM emp_test
                GROUP BY dept_id
            ),
            dept_salary_rank AS (
                SELECT dept_id,
                       avg_sal,
                       DENSE_RANK() OVER (ORDER BY avg_sal DESC) AS dept_rank
                FROM dept_avg_salary
            )
            SELECT e.emp_id,
                   e.emp_name,
                   e.salary,
                   d.dept_name,
                   r.avg_sal AS dept_avg_salary,
                   r.dept_rank
            FROM emp_test e
            JOIN dept_test d ON e.dept_id = d.dept_id
            JOIN dept_salary_rank r ON e.dept_id = r.dept_id
            ORDER BY r.dept_rank, e.salary DESC,e.emp_name;

        LOOP
            FETCH cur INTO v_emp_id, v_emp_name, v_salary,
                          v_dept_name, v_dept_avg_salary, v_dept_rank;
            EXIT WHEN cur%NOTFOUND;

            raise info '%',
                RPAD(v_emp_id, 6) || ' ' ||
                RPAD(v_emp_name, 10) || ' ' ||
                RPAD(v_salary, 8) || ' ' ||
                RPAD(v_dept_name, 15) || ' ' ||
                RPAD(TO_CHAR(v_dept_avg_salary, '99990.99'), 10) || ' ' ||
                v_dept_rank;
        END LOOP;

        CLOSE cur;

        raise info '===== finish output =====';
    EXCEPTION
        WHEN OTHERS THEN
            IF cur%ISOPEN THEN
                CLOSE cur;
            END IF;
            raise info '%', 'exception happend: ' || SQLERRM;
            RAISE;
    END proc_show_dept_salary_rank;

END pkg_cursor_with_demo;
/

--ok
EXEC pkg_cursor_with_demo.proc_show_dept_salary_rank;


declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  type t is record(id integer,name varchar2(256));
  var3 t;
begin
  var3.name := 'select * from test_select';
  open var1 for var3.name;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--collection
declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
  type t is record(id integer,name varchar2(256));
  type mt is table of t index by pls_integer;
  var3 t;
  var4 mt;
begin
  var3.name := 'select * from test_select';
  var4(1) := var3;
  open var1 for var4(1).name;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--use package global name
create or replace package test_pkg_var is
  type tr is record(id integer,name varchar2(256));
  type mt is table of tr index by pls_integer;
  var3 tr;
  var4 mt;
  function test_f(id integer) return integer;
end;
/


create or replace package body test_pkg_var is
  function test_f(id integer) return integer is
  begin
    return 1;
  end;
begin
  var3.name := 'select * from test_select';
  var4(1) := var3;
end;
/

declare
  var1 sys_refcursor;
  var2 test_select%rowtype;
begin
  open var1 for test_pkg_var.var3.name;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
  open var1 for test_pkg_var.var4(1).name;
  fetch var1 into var2;
  raise info '%', var2;
  close var1;
end;
/

--clean data
drop package pkg_cursor_with_demo;
drop table emp_test;
drop table dept_test;
drop table test_select;
drop package test_pkg_var;