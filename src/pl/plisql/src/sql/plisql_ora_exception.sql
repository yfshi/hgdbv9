--test plsql_warnings
--ok
ALTER SESSION SET PLSQL_WARNINGS='ENABLE:ALL';

--ok
ALTER SESSION SET PLSQL_WARNINGS='ENABLE:PERFORMANCE';

--create loc_var procedure
CREATE OR REPLACE PROCEDURE loc_var(id integer) IS
  var1 integer;
BEGIN
   var1 := 1;
end;
/

--alter procedure compile ok
ALTER PROCEDURE loc_var
  COMPILE PLSQL_WARNINGS='ENABLE:PERFORMANCE';

--use list input ok
ALTER SESSION
  SET PLSQL_WARNINGS='ENABLE:SEVERE', 'DISABLE:PERFORMANCE', 'ERROR:06002';

--user define exception
--ok
CREATE OR REPLACE PROCEDURE account_status (
  due_date DATE,
  today    DATE
) AUTHID DEFINER
IS
  past_due  EXCEPTION;  -- declare exception
BEGIN
  IF due_date < today THEN
    RAISE past_due;  -- explicitly raise exception
  END IF;
EXCEPTION
  WHEN past_due THEN  -- handle exception
    raise info '%','Account past due.';
END;
/

--ok
--print Account past due.
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));
END;
/


--predefine exception
--handle predefined zero_divide exception
--ok and print Company had zero earnings.
DECLARE
  stock_price   NUMBER := 9.73;
  net_earnings  NUMBER := 0;
  pe_ratio      NUMBER;
BEGIN
  pe_ratio := stock_price / net_earnings;  -- raises ZERO_DIVIDE exception
  raise info '%','Price/earnings ratio = ' || pe_ratio;
EXCEPTION
  WHEN ZERO_DIVIDE THEN
    raise info '%','Company had zero earnings.';
    pe_ratio := NULL;
END;
/


--user standard.xxx
--ok and print Company had zero earnings.
DECLARE
  stock_price   NUMBER := 9.73;
  net_earnings  NUMBER := 0;
  pe_ratio      NUMBER;
BEGIN
  pe_ratio := stock_price / net_earnings;  -- raises ZERO_DIVIDE exception
  raise info '%','Price/earnings ratio = ' || pe_ratio;
EXCEPTION
  WHEN standard.ZERO_DIVIDE THEN
    raise info '%','Company had zero earnings.';
    pe_ratio := NULL;
END;
/

--rowtype_mismatch exception
CREATE TABLE EMPLOYEES(id integer,name varchar2(256),name1 varchar2(256));

CREATE TABLE DEPARTMENTS(id integer,name varchar2(256));

CREATE OR REPLACE PACKAGE emp_dept_data AUTHID DEFINER AS
  TYPE cv_type IS REF CURSOR;
  PROCEDURE open_cv (
    cv       IN OUT cv_type,
    discrim  IN     integer
  );
  END emp_dept_data;
/

CREATE OR REPLACE PACKAGE BODY emp_dept_data AS
  PROCEDURE open_cv (
    cv      IN OUT cv_type,
    discrim IN     integer) IS
  BEGIN
    IF discrim = 1 THEN
    OPEN cv FOR
      SELECT * FROM EMPLOYEES ORDER BY id;
    ELSIF discrim = 2 THEN
      OPEN cv FOR
        SELECT * FROM DEPARTMENTS ORDER BY id;
    END IF;
  END open_cv;
END emp_dept_data;
/

--ok and print Row type mismatch, fetching EMPLOYEES data ...
DECLARE
  emp_rec   EMPLOYEES%ROWTYPE;
  dept_rec  DEPARTMENTS%ROWTYPE;
  cv        Emp_dept_data.CV_TYPE;
BEGIN
  emp_dept_data.open_cv(cv, 1);  -- Open cv for EMPLOYEES fetch.
  FETCH cv INTO dept_rec;        -- Fetch from DEPARTMENTS.
  raise info '%',dept_rec.id;
  raise info '%','  ' || dept_rec.name;
EXCEPTION
  WHEN ROWTYPE_MISMATCH THEN
     BEGIN
       raise info 'Row type mismatch, fetching EMPLOYEES data ...';
     END;
END;
/

INSERT INTO EMPLOYEES values(90,'King','ok');

--ok and print
--Row type mismatch, fetching EMPLOYEES data ...
--90  King
DECLARE
  emp_rec   EMPLOYEES%ROWTYPE;
  dept_rec  DEPARTMENTS%ROWTYPE;
  cv        Emp_dept_data.CV_TYPE;
BEGIN
  emp_dept_data.open_cv(cv, 1);  -- Open cv for EMPLOYEES fetch.
  FETCH cv INTO dept_rec;        -- Fetch from DEPARTMENTS.
  raise info '%',dept_rec.id;
  raise info '%','  ' || dept_rec.name;
EXCEPTION
  WHEN ROWTYPE_MISMATCH THEN
     BEGIN
       raise info '%','Row type mismatch, fetching EMPLOYEES data ...';
	   --oracle maybe not need open again
	   emp_dept_data.open_cv(cv, 1);
       FETCH cv INTO emp_rec;
       raise info '% %',emp_rec.id, emp_rec.name;
     END;
END;
/


--PRAGMA exception_init
--print handle zero excep
DECLARE
  zero_excep exception;
  PRAGMA exception_init(zero_excep, -1476);
  var1 integer;
BEGIN
  var1 := 100 / 0;
exception
  WHEN zero_excep THEN
     raise info 'handle zero excep';
end;
/

--raise
--raise udt exception
CREATE OR REPLACE PROCEDURE account_status (
  due_date DATE,
  today    DATE
) AUTHID DEFINER
IS
  past_due  EXCEPTION;  -- declare exception
BEGIN
  IF due_date < today THEN
    RAISE past_due;  -- explicitly raise exception
  END IF;
EXCEPTION
  WHEN past_due THEN  -- handle exception
    raise info '%','Account past due.';
END;
/

--ok and print Account past due.
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));
END;
/

--raise predefined exception
CREATE TABLE t (c NUMBER);

CREATE PROCEDURE p (n NUMBER) AUTHID DEFINER IS
  default_number NUMBER := 0;
BEGIN
  IF n < 0 THEN
    RAISE INVALID_NUMBER;  -- raise explicitly
  ELSE
    INSERT INTO t VALUES(TO_NUMBER('100.00', '9G999'));  -- raise implicitly
  END IF;
EXCEPTION
  WHEN INVALID_NUMBER THEN
    raise info '%','Substituting default value for invalid number.';
    INSERT INTO t VALUES(default_number);
END;
/

--ok and print
--Substituting default value for invalid number.
BEGIN
  p(-1);
END;
/

--action implicit raise
--and print Substituting default value for invalid number.

BEGIN
  p(1);
END;
/

--reraise current exception using raise
--ok and print
--Salary 20000 is out of range.
--Maximum salary is 10000.
--Revising salary from 20000 to 10000.
DECLARE
  salary_too_high   EXCEPTION;
  current_salary    NUMBER := 20000;
  max_salary        NUMBER := 10000;
  erroneous_salary  NUMBER;
BEGIN
  BEGIN
    IF current_salary > max_salary THEN
      RAISE salary_too_high;   -- raise exception
    END IF;
  EXCEPTION
    WHEN salary_too_high THEN  -- start handling exception
      erroneous_salary := current_salary;
      raise info '%','Salary ' || erroneous_salary ||' is out of range.';
      raise info '%','Maximum salary is ' || max_salary || '.';
      RAISE;  -- reraise current exception (exception name is optional)
  END;
EXCEPTION
  WHEN salary_too_high THEN    -- finish handling exception
    current_salary := max_salary;
    raise info '%','Revising salary from ' || erroneous_salary ||
      ' to ' || current_salary || '.';
END;
/


--raise_application_error
CREATE OR REPLACE PROCEDURE account_status (
  due_date DATE,
  today    DATE
) AUTHID DEFINER
IS
BEGIN
  IF due_date < today THEN                   -- explicitly raise exception
    RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
  END IF;
END;
/

--ok and
--print IVY-20000: Account past due.
DECLARE
  past_due  EXCEPTION;                       -- declare exception
  PRAGMA EXCEPTION_INIT (past_due, -20000);  -- assign error code to exception
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));   -- invoke procedure
EXCEPTION
  WHEN past_due THEN                         -- handle exception
    raise info '%',TO_CHAR(SQLERRM(-20000));
END;
/


--ok
---and print
--IVY-20000:
--SQLCODE=-20000 SQLERRM = IVY-20000:
DECLARE
  past_due  EXCEPTION;                       -- declare exception
  PRAGMA EXCEPTION_INIT (past_due, -20000);  -- assign error code to exception
BEGIN
  raise past_due;
EXCEPTION
  WHEN past_due THEN                         -- handle exception
    raise info '%',TO_CHAR(SQLERRM(-20000));
    raise info '%', 'SQLCODE= ' || sqlcode || ' SQLERRM = ' || SQLERRM;
END;
/

--ok
--and print
--IVY-20000: Account past due
--IVY-20000:
DECLARE
  past_due  EXCEPTION;                       -- declare exception
  PRAGMA EXCEPTION_INIT (past_due, -20000);  -- assign error code to exception
BEGIN
  begin
    RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
  exception
    when past_due then
       raise info '%', TO_CHAR(SQLERRM(-20000));
  end;
  raise info '%',TO_CHAR(SQLERRM(-20000));
END;
/

--ok
--and print
--IVY-20000: Account past due.
--IVY-20001: Account past due1.
--IVY-20000:
--access ok
DECLARE
  past_due  EXCEPTION;                       -- declare exception
  PRAGMA EXCEPTION_INIT (past_due, -20000);  -- assign error code to exception
BEGIN
    RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
 exception
    when past_due then
       raise info '%',TO_CHAR(SQLERRM(-20000));
       declare
          past_due2 exception;
	  PRAGMA EXCEPTION_INIT (past_due2, -20001);
       begin
          RAISE_APPLICATION_ERROR(-20001, 'Account past due1.');
       exception
         when past_due2 then
	    raise info '%',TO_CHAR(SQLERRM(-20001));
	    raise info '%',TO_CHAR(SQLERRM(-20000));
	    raise info '%','access ok';
	end;
END;
/

--use DBMS_STANDARD package
CREATE OR REPLACE PROCEDURE account_status (
  due_date DATE,
  today    DATE
) AUTHID DEFINER
IS
BEGIN
  IF due_date < today THEN                   -- explicitly raise exception
    DBMS_STANDARD.RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
  END IF;
END;
/

--ok
DECLARE
  past_due  EXCEPTION;                       -- declare exception
  PRAGMA EXCEPTION_INIT (past_due, -20000);  -- assign error code to exception
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));   -- invoke procedure

EXCEPTION
  WHEN past_due THEN                         -- handle exception
    raise info '%',TO_CHAR(SQLERRM(-20000));
END;
/

--test sqlcode
--no exception call
BEGIN
  raise info '%','sqlcode = ' || sqlcode;
  raise info '%','sqlcode = ' || standard.sqlcode;
end;
/

--exception call
DECLARE
  var1 integer;
BEGIN
   var1 := 100 / 0;
exception
  WHEN others THEN
      raise info '%','sqlcode = ' || sqlcode;
	  raise info '%','sqlcode = ' || standard.sqlcode;
end;
/


--test sqlerrm
--noraml call sqlerrm
BEGIN
  raise info '%','sqlerrm = ' || sqlerrm;
  raise info '%','sqlerrm = ' || standard.sqlerrm;
end;
/

--exception call
DECLARE
  var1 integer;
BEGIN
   var1 := 100 / 0;
exception
  WHEN others THEN
      raise info '%','sqlerrm = ' || sqlerrm;
	  raise info '%','sqlerrm = ' || standard.sqlerrm;
end;
/

--test +100
BEGIN
  raise info '%','SQLERRM(+100): ' || TO_CHAR(SQLERRM(+100));
END;
/

--test not 100 but a postivie value
BEGIN
  raise info '%','SQLERRM(101): ' || TO_CHAR(SQLERRM(101));
END;
/

--a common error code
BEGIN
  raise info '%','SQLERRM(-6511): ' || TO_CHAR(SQLERRM(-6511));
END;
/

--not a common error code
BEGIN
  raise info '%','SQLERRM(-50000): ' || TO_CHAR(SQLERRM(-50000));
END;
/

--special 0
BEGIN
  raise info '%','SQLERRM(0): ' || TO_CHAR(SQLERRM(0));
END;
/

--EXCEPTION_INIT init global var
--error for not init local exception
DECLARE
  var1 exception;
  var2 integer;
  FUNCTION test_f(id integer) RETURN integer IS
    PRAGMA EXCEPTION_INIT (var1, -20000);
  BEGIN
    RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
    return 1;
  end;
BEGIN
  var2 := test_f(23);
exception
  WHEN var1 THEN
     raise info '%','handle var1';
end;
/

--error
DECLARE
  var1 exception;
  var2 integer;
  FUNCTION test_f(id integer) RETURN integer IS
    --PRAGMA EXCEPTION_INIT (var1, -20000);
  BEGIN
    RAISE_APPLICATION_ERROR(-20000, 'Account past due.');
    return 1;
  end;
BEGIN
  var2 := test_f(23);
exception
  WHEN var1 THEN
     raise info '%','handle var1';
end;
/

--error
declare
  var1 exception;
  function test_f(id integer) return integer;
  var2 integer;
  function test_f(id integer) return integer is
    var1 exception;
  begin
     raise var1;
     return 1;
  end;
begin
  var2 := test_f(1);
exception
  when var1 then
     raise info '%','handle var1 exception';
end;
/

--ok
--and print
--handle var1 exception
declare
  var1 exception;
  function test_f(id integer) return integer;
  var2 integer;
  function test_f(id integer) return integer is
  begin
     raise var1;
     return 1;
  end;
begin
  var2 := test_f(1);
exception
  when var1 then
     raise info '%','handle var1 exception';
end;
/


--sqlerrm use number parameter
DECLARE
  var1 number;
BEGIN
  var1 := 0;
  raise info '%','sqlerrm = ' || sqlerrm(var1);
end;
/

--check exception is null
--oracle raise error, but we success
DECLARE
  var1 exception;
BEGIN
  IF var1 IS NULL THEN
     raise info '%','exception is null';
  ELSE
     raise info '%','exception is not null';
  END if;
end;
/

--ok
DECLARE
  var1 integer;
BEGIN
  IF var1 IS NULL THEN
     raise info '%','var1 is null';
  ELSE
     raise info '%','var1 is not null';
  END if;
end;
/

--exception_init
--errorcode is -20000----20999
--error
DECLARE
  var1 exception;
  pragma exception_init(var1, -19999);
BEGIN
  raise var1;
end;
/

--error
DECLARE
  var1 exception;
  pragma exception_init(var1, -20001);
BEGIN
  raise var1;
end;
/

--ok
DECLARE
  var1 exception;
  pragma exception_init(var1, -19999);
BEGIN
  raise var1;
exception
WHEN others THEN
   raise info '%','ok';
end;
/

--others as user defined exception
DECLARE
  others exception;
BEGIN
   raise others;
exception
  WHEN others THEN
     raise info '%','pk';
end;
/

--error
DECLARE
  others exception;
BEGIN
   raise others;
exception
  WHEN others THEN
     raise info '%','pk';
  WHEN others THEN
     raise info '%','pk1';
end;
/


--error
DECLARE
  others exception;
  var2 exception;
BEGIN
   raise others;
exception
  WHEN others or var2 THEN
	 raise info '%','test';
end;
/

--error
DECLARE
  others exception;
  var2 exception;
BEGIN
   raise others;
exception
  WHEN var2 or others THEN
     raise info '%','test';
end;
/

--ok
DECLARE
  others exception;
  var1 exception;
BEGIN
   raise var1;
exception
  WHEN others THEN
     raise info '%','pk1';
end;
/

--error
DECLARE
  others exception;
BEGIN
   raise others;
end;
/


--raise_application_error
--ok
BEGIN
   raise_application_error(-19999,'invalide error code');
exception
   WHEN others THEN
     raise info '%',sqlcode || '_' || sqlerrm;
end;
/

--ok
BEGIN
   raise_application_error(-20001,'invalide error code');
exception
   WHEN others THEN
      raise info '%', sqlcode || '_' || sqlerrm;
end;
/

--error
DECLARE
  var1 integer;
BEGIN
  raise var1;
end;
/

--oracle failed,but we success
DECLARE
  var1 integer;
  var2 exception;
  FUNCTION test_f(id exception) RETURN integer IS
  BEGIN
    RETURN 1;
  end;
BEGIN
  var1 := test_f(var2);
end;
/

--error
DECLARE
  var1 integer;
  var1 exception;
BEGIN
  raise var1;
end;
/

--oracle failed,but we success
DECLARE
  exception integer;
BEGIN
  exception := 1;
end;
/

--
-- exception_init as a datum name?
-- ok
--
DECLARE
  exception_init integer;
  var1 exception;
  pragma exception_init(var1,-20001);
BEGIN
  exception_init := 1;
end;
/


--init package.exception
CREATE OR REPLACE package test_exception_pkg IS
  var1 exception;
end;
/

--error
DECLARE
  var2 integer;
  pragma exception_init(test_exception_pkg.var1, -1722);
BEGIN
  var2 := 3;
end;
/

DECLARE
  var2 integer;
  pragma exception_init(var2, -1722);
BEGIN
  var2 := 3;
end;
/

--raise error
DECLARE
  var2 exception;
  pragma exception_init(var2, -1722);
BEGIN
  raise var2;
end;
/


--error
DECLARE
  var2 exception;
  FUNCTION test_f(id integer) RETURN integer is
    pragma exception_init(var2, -1722);
  BEGIN
      RETURN 1;
  end;
BEGIN
  raise var2;
end;
/


declare
  var1 exception;
begin
   raise var1;
exception
  when var1 then
     raise info '%','sqlcode 1 = ' || sqlcode;
     raise info '%','sqlerrm 1 = ' || sqlerrm;
     declare
        var2 integer;
     begin
         raise info '%','sqlcode 2 = ' || sqlcode;
	     raise info '%','sqlerrm  2 = ' || sqlerrm;
	     var2 := 100 / 0;
     exception
       when others then
           raise info '%','sqlcode 3 = ' || sqlcode;
		   raise info '%','sqlerrm  3 = ' || sqlerrm;
     end;
     raise info '%','sqlcode 4 = ' || sqlcode;
     raise info '%', 'sqlerrm  4 = ' || sqlerrm;
end;
/


--ok and print
--sqlcode 1 = 1
--sqlerrm 1 = User-Defined Exception
--sqlcode 2 = 0
--sqlerrm 2 = IVY-0000: normal, successful completion
declare
  var1 integer;
begin
  declare
     var2 exception;
  begin
     raise var2;
  exception
  when var2 then
     raise info '%','sqlcode 1 = ' || sqlcode;
     raise info '%','sqlerrm 1 = ' || sqlerrm;
   end;
   raise info '%','sqlcode 2 = ' || sqlcode;
   raise info '%','sqlerrm 2 = ' || sqlerrm;
end;
/


create or replace function test_f(id integer) return integer is
begin
  raise info '%','sqlcode func = ' || sqlcode;
  raise info '%','sqlerrm func = ' || sqlerrm;
  return 1;
end;
/

--ok and print
--sqlcode 1 = 1
--sqlerrm 1 = User-Defined Exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
--sqlcode 2 = 0
--sqlerrm 2 = IVY-0000: normal, successful completion
declare
  var1 integer;
begin
  declare
     var2 exception;
     var3 integer;
  begin
     raise var2;
  exception
  when var2 then
     raise info '%','sqlcode 1 = ' || sqlcode;
     raise info '%','sqlerrm 1 = ' || sqlerrm;
     var3 := test_f(23);
   end;
   raise info '%','sqlcode 2 = ' || sqlcode;
   raise info '%','sqlerrm 2 = ' || sqlerrm;
end;
/


--ok
declare
  var1 exception;
begin
   begin
      raise var1;
   end;
exception
   when var1 then
     raise info '%','sqlcode = ' || sqlcode;
     raise info '%','sqlerrm = ' || sqlerrm;
end;
/

--ok
begin
  raise info '%',sqlerrm(100);
  raise info '%',sqlerrm(-1476);
end;
/

begin
  raise info '%',sqlerrm(200);
  raise info '%', sqlerrm(1476);
end;
/

--oracle failed,but we success
declare
  var1 exception;
  function test_f(var2 exception) return exception is
    var1 exception;
  begin
    return var1;
  end;
begin
  var1 := test_f(var1);
  raise info '%', var1;
end;
/


--ok
DECLARE
  var1 exception;
BEGIN
  BEGIN
     raise var1;
  exception
    WHEN var1 THEN
        raise info '%',sqlcode || sqlerrm;
		raise info '%',sqlcode || sqlerrm;
    end;
    raise info '%',sqlcode || sqlerrm;
	raise info '%',sqlcode || sqlerrm;
end;
/

--ok
DECLARE
  var1 exception;
BEGIN
  BEGIN
     raise var1;
  end;
exception
   WHEN var1 then
    raise info '%',sqlcode || sqlerrm;
	raise info '%',sqlcode || sqlerrm;
end;
/


--raise application error
DECLARE
  var1 exception;
BEGIN
  BEGIN
     raise_application_error(-20000,'error message');
  exception
    WHEN others THEN
        raise info '%',sqlcode || sqlerrm;
		raise info '%',sqlcode || sqlerrm;
    end;
    raise info '%',sqlcode || sqlerrm;
	raise info '%', sqlcode || sqlerrm;
end;
/
--ok
DECLARE
  var1 exception;
BEGIN
  BEGIN
     raise_application_error(-20000,'error message',true);
  exception
    WHEN others THEN
        raise info '%',sqlcode || sqlerrm;
		raise info '%',sqlcode || sqlerrm;
    end;
    raise info '%',sqlcode || sqlerrm;
	raise info '%',sqlcode || sqlerrm;
end;
/

--ok
DECLARE
  var1 exception;
BEGIN
  BEGIN
     raise_application_error(-20000,'error message',false);
  exception
    WHEN others THEN
        raise info '%',sqlcode || sqlerrm;
		raise info '%',sqlcode || sqlerrm;
    end;
    raise info '%',sqlcode || sqlerrm;
	raise info '%',sqlcode || sqlerrm;
end;
/


---three level raise error
--and print
--one exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
--two exception
--sqlcode func = -1476
--sqlerrm func = IVY-01476: divisor is equal to zero
--tree exception
--sqlcode func = -6511
--sqlerrm func = IVY-06511: PL/SQL: cursor already open
--after two exception
--sqlcode func = -1476
--sqlerrm func = IVY-01476: divisor is equal to zero
--after one exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','sqlcode func = '|| sqlcode;
	raise info '%','sqlerrm func = ' || sqlerrm;
  end;
BEGIN
   raise var1;
exception
  WHEN var1 then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
        var2 := 100 / 0;
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  DECLARE
             CURSOR c1 IS SELECT * FROM dual;
		  BEGIN
             OPEN c1;
			 OPEN c1;
		  exception
		       WHEN others THEN
                    raise info '%','tree exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /

--ok and print
--one exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
--middle one exception and two exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
--two exception
--sqlcode func = -1476
--sqlerrm func = IVY-01476: divisor is equal to zero
--middle two exception and three exception
--sqlcode func = -1476
--sqlerrm func = IVY-01476: divisor is equal to zero
--three exception
--sqlcode func = -6511
--sqlerrm func = IVY-06511: PL/SQL: cursor already open
--after two exception
--sqlcode func = -1476
--sqlerrm func = IVY-01476: divisor is equal to zero
--after one exception
--sqlcode func = 1
--sqlerrm func = User-Defined Exception
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','sqlcode func = '|| sqlcode;
	raise info '%','sqlerrm func = ' || sqlerrm;
  end;
BEGIN
   raise var1;
exception
  WHEN var1 then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        var2 := 100 / 0;
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  DECLARE
             CURSOR c1 IS SELECT * FROM dual;
		  BEGIN
             OPEN c1;
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 OPEN c1;
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /

--ok
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','sqlcode func = '|| sqlcode;
	raise info '%','sqlerrm func = ' || sqlerrm;
  end;
BEGIN
   raise var1;
exception
  WHEN var1 then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception two success';
		print_sqlcode();
        DECLARE
             CURSOR c1 IS SELECT * FROM dual;
		  BEGIN
             OPEN c1;
			 raise info '%','middle two sucess and three exception';
			 print_sqlcode();
			 OPEN c1;
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
		  raise info '%','after three exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /


--middle no exception
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','sqlcode func = '|| sqlcode;
	raise info '%','sqlerrm func = ' || sqlerrm;
  end;
BEGIN
   raise var1;
exception
  WHEN var1 then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        DECLARE
             CURSOR c1 IS SELECT * FROM dual;
		  BEGIN
             OPEN c1;
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 OPEN c1;
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after three exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /


--use format errorstack
--DBMS_UTILITY.FORMAT_ERROR_STACK
---we use plisql_error_stack
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%', 'error stack '|| plisql_error_stack();
  end;
BEGIN
   raise_application_error(-20000,'error msg1');
exception
  WHEN others then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        raise_application_error(-20001,'error msg2');
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  BEGIN
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 raise_application_error(-20002,'error msg3');
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /


DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%', 'error stack '|| plisql_error_stack() || '\nsqlerrm = ' || sqlerrm;
  end;
BEGIN
   raise_application_error(-20000,'error msg1');
exception
  WHEN others then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        raise_application_error(-20001,'error msg2');
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  BEGIN
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 raise_application_error(-20002,'error msg3');
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /

--ok has error statck
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack;
  end;
BEGIN
   raise_application_error(-20000,'error msg1', true);
exception
  WHEN others then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        raise_application_error(-20001,'error msg2', true);
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  BEGIN
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 raise_application_error(-20002,'error msg3', true);
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /

--common error
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack;
  end;
BEGIN
   raise var1;
exception
  WHEN others then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        raise var1;
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  BEGIN
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 raise var1;
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /


DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack || 'sqlerrm = ' || sqlerrm || ' sqlcode = ' || sqlcode;
  end;
BEGIN
   raise var1;
exception
  WHEN others then
     raise info '%','one exception';
	 print_sqlcode();
	 BEGIN
		raise info '%','middle one exception and two exception';
		print_sqlcode();
        raise var1;
	 exception
	    WHEN others then
	      raise info '%','two exception';
		  print_sqlcode();
		  BEGIN
			 raise info '%','middle two exception and three exception';
			 print_sqlcode();
			 raise var1;
		  exception
		       WHEN others THEN
                    raise info '%','three exception';
					print_sqlcode();
		  end;
          raise info '%','after two exception';
		  print_sqlcode();
	 end;
      raise info '%','after one exception';
	  print_sqlcode();
 end;
 /

--common
DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack;
  end;
BEGIN
   print_sqlcode();
 end;
 /


DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack || 'mds';
  end;
BEGIN
   print_sqlcode();
 end;
 /


DECLARE
  var1 exception;
  var2 integer;
  PROCEDURE print_sqlcode IS
  BEGIN
    raise info '%','error stack '|| plisql_error_stack || 'mds';
  end;
BEGIN
   raise var1;
exception
  WHEN var1 then
   print_sqlcode();
 end;
 /

--raise_application_error num code
--error
BEGIN
  raise_application_error(-21000,'test error');
end;
/



BEGIN
  raise_application_error(-21000,'test error');
exception
  WHEN others THEN
    raise info '%','sqlcode = ' || sqlcode;
	raise info '%','sqlerrm = ' || sqlerrm;
end;
/


--error
DECLARE
  var1 exception;
  pragma EXCEPTION_INIT(var1, '-6511');
BEGIN
   raise var1;
exception
  WHEN others THEN
    raise info '%','success';
end;
/

--ok
BEGIN
	raise info '%',BINARY_FLOAT_NAN;
	raise info '%',BINARY_FLOAT_INFINITY;
	raise info '%',BINARY_FLOAT_MAX_NORMAL;
	raise info '%',BINARY_FLOAT_MIN_NORMAL;
	raise info '%',BINARY_FLOAT_MAX_SUBNORMAL;
	raise info '%',BINARY_FLOAT_MIN_SUBNORMAL;
	raise info '%',BINARY_DOUBLE_NAN;
	raise info '%',BINARY_DOUBLE_INFINITY;
	raise info '%',BINARY_DOUBLE_MAX_NORMAL;
	raise info '%',BINARY_DOUBLE_MAX_SUBNORMAL;
	raise info '%',BINARY_DOUBLE_MIN_NORMAL;
	raise info '%',BINARY_DOUBLE_MIN_SUBNORMAL;
end;
/

--ok
BEGIN
	raise info '%',standard.BINARY_FLOAT_NAN;
	raise info '%',standard.BINARY_FLOAT_INFINITY;
	raise info '%',standard.BINARY_FLOAT_MAX_NORMAL;
	raise info '%',standard.BINARY_FLOAT_MIN_NORMAL;
	raise info '%',standard.BINARY_FLOAT_MAX_SUBNORMAL;
	raise info '%',standard.BINARY_FLOAT_MIN_SUBNORMAL;
	raise info '%',standard.BINARY_DOUBLE_NAN;
	raise info '%',standard.BINARY_DOUBLE_INFINITY;
	raise info '%',standard.BINARY_DOUBLE_MAX_NORMAL;
	raise info '%',standard.BINARY_DOUBLE_MAX_SUBNORMAL;
	raise info '%',standard.BINARY_DOUBLE_MIN_NORMAL;
	raise info '%',standard.BINARY_DOUBLE_MIN_SUBNORMAL;
end;
/

--extral exception
--query_canceled
--or assert_failure
--ok
DECLARE
  var1 exception;
BEGIN
  raise var1;
exception
  WHEN query_canceled THEN
     raise info 'query canceled';
  WHEN others THEN
     raise info 'others hande';
end;
/

DECLARE
  var1 exception;
BEGIN
  raise var1;
exception
  WHEN assert_failure OR others THEN
     raise info 'others or assert_failure hande';
end;
/


--clean data
DROP PROCEDURE account_status;
DROP PROCEDURE loc_var;
DROP FUNCTION test_f;
DROP PROCEDURE p;
DROP package emp_dept_data;
DROP package test_exception_pkg;
DROP TABLE t;
DROP TABLE departments;
DROP TABLE employees;


--origin 9.5 regression test
CREATE  PROCEDURE account_status (
  due_date DATE,
  today    DATE
) AUTHID DEFINER
IS
  var1 exception;
  PRAGMA EXCEPTION_INIT(var1, 20000);
BEGIN
  IF due_date < today THEN                   -- explicitly raise exception
    raise var1;
  END IF;
END;
/

--error
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));
END;
/

--error
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));
EXCEPTION
  WHEN SQLSTATE '20000' THEN                         -- handle exception
    raise notice 'SQLCODE: %, SQLERRM: %', SQLCODE, SQLERRM;
END;
/

--ok
BEGIN
  account_status (TO_DATE('01-JUL-2010', 'DD-MON-YYYY'),
                  TO_DATE('09-JUL-2010', 'DD-MON-YYYY'));
EXCEPTION
  WHEN OTHERS THEN                         -- handle exception
    raise notice 'SQLCODE: %, SQLERRM: %', SQLCODE, SQLERRM;
END;
/

BEGIN
  raise case_not_found;
EXCEPTION
  WHEN OTHERS THEN                         -- handle exception
    raise notice 'SQLCODE: %, SQLERRM: %', SQLCODE, SQLERRM;
END;
/

BEGIN
  raise case_not_found;
EXCEPTION
  WHEN OTHERS THEN                         -- handle exception
    raise notice 'SQLCODE: %, SQLERRM: %', SQLCODE, SQLERRM(SQLCODE);
END;
/


DECLARE
   err_msg  text;
BEGIN
   FOR err_num IN 512 .. 522 LOOP
      err_msg := SQLERRM(err_num);
      raise notice '%', err_msg;
   END LOOP;
END;
/

--print 'internal exception handle' and report error
declare
  var1 exception;
  var2 integer;
 begin
   declare
      var1 exception;
    begin
       raise var1;
    exception
       when var1 then
         raise info 'internal exception handle';
	 raise;
       end;
    var2 := 23;
 exception
       when var1 then
          raise info 'foreign exception handle';
end;
/

--print 'internal exception handle' and 'foreign exception handle'
declare
  var1 exception;
  var2 integer;
 begin
   declare
      var2 integer;
    begin
       raise var1;
    exception
       when var1 then
         raise info 'internal exception handle';
	 raise;
       end;
    var2 := 23;
 exception
       when var1 then
          raise info 'foreign exception handle';
end;
/


create table test_table(id integer,name varchar(256));

--test no data found exception
CREATE OR REPLACE PROCEDURE select_item (
  t_column VARCHAR2,
  t_name   VARCHAR2
)
IS
  temp VARCHAR2(30);
BEGIN
  temp := t_column;  -- For error message if next SELECT fails
  select name into strict temp from test_table;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    raise info 'No Data found';
  WHEN OTHERS THEN
    raise info 'Unexpected error';
    RAISE;
END;
/

--print No Data found
begin
  select_item('xiexie','column');
end;
/

DECLARE
  name_temp varchar2(256);
BEGIN
  select name into name_temp from test_table;
exception
  WHEN NO_DATA_FOUND THEN
     raise info 'No Data found';
end;
/

--print No Data found
declare
  temp varchar2(30);
begin
   select name into strict temp from test_table;
exception
  when NO_DATA_FOUND THEN
     raise info 'No Data found';
  when OTHERS THEN
     raise info 'Unexpected error';
     raise;
end;
/

insert into test_table values(1, 'xiexie');
--test ok
declare
  temp varchar2(30);
begin
   select name into strict temp from test_table;
exception
  when NO_DATA_FOUND THEN
     raise info 'No Data found';
  when OTHERS THEN
     raise info 'Unexpected error';
     raise;
end;
/

drop table test_table;
drop procedure select_item(varchar2, varchar2);


--print divide zero
DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT(udt_divide_zero, -1476);
  var1 integer;
 begin
    var1 := 23 / 0;
 exception
    WHEN udt_divide_zero THEN
        raise info 'divide zero';
 end;
 /

 DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT (udt_divide_zero, -20023);
  var1 integer;
 begin
   RAISE_APPLICATION_ERROR(-20023, 'divide zero error.');
 exception
    WHEN udt_divide_zero THEN
        raise info 'divide zero';
	raise info '%',SQLCODE;
 end;
 /

--print divide zero
DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT (udt_divide_zero, -1476);
  var1 integer;
 begin
    var1 := 23 / 0;
 exception
    WHEN udt_divide_zero THEN
        raise info 'divide zero';
 end;
 /

--test RAISE_APPLICATION_ERROR
--print divide zero
DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT (udt_divide_zero, -1476);
  var1 integer;
 begin
   raise udt_divide_zero;
 exception
    WHEN udt_divide_zero THEN
        raise info 'divide zero';
 end;
 /

--print divide zero
 DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT (udt_divide_zero, -20001);
  var1 integer;
 begin
   RAISE_APPLICATION_ERROR(-20001, 'divide zero error.');
 exception
    WHEN udt_divide_zero THEN
        raise info 'divide zero';
 end;
 /

-- failed and print divide zero error
 DECLARE
  udt_divide_zero exception;
  PRAGMA EXCEPTION_INIT (udt_divide_zero, -20001);
  var1 integer;
 begin
   RAISE_APPLICATION_ERROR(-20001, 'divide zero error.');
 end;
 /

--failed
declare
  var1 exception;
  var2 integer;
  function test_initexception(id integer) return integer;
  function test_initexception(id integer) return integer is
   var2 integer;
   pragma exception_init(var1, -1476);
  begin
    raise var1;
  end;
begin
   var2 := test_initexception(23);
exception
   when var1 then
      raise info 'handle divide zero';
end;
/

--print handle divide zero
declare
  var1 exception;
  var2 integer;
  pragma exception_init(var1, -1476);
  function test_initexception(id integer) return integer;
  function test_initexception(id integer) return integer is
   var2 integer;
  begin
    raise var1;
  end;
begin
   var2 := test_initexception(23);
exception
   when var1 then
      raise info 'handle divide zero';
end;
/

--test references to exception in package
--test ok
create or replace package pkg_exception is
var1 exception;
end;
/

--test ok
create or replace package body pkg_exception is
begin
  raise info 'init block';
end;
/

--print handle pkg_exception var1 exception
declare
 var1 integer;
begin
  raise pkg_exception.var1;
 exception
   when pkg_exception.var1 then
     raise info 'handle pkg_exception var1 exception';
end;
/

--create a function
create or replace function test_exception(id integer) return integer is
begin
	raise pkg_exception.var1;
	return id;
end;
/

select test_exception(23) FROM dual;

declare
  id integer;
begin
  id := test_exception(23);
end;
/

declare
 id integer;
begin
  id := test_exception(23);
exception
  when pkg_exception.var1 then
    raise info 'handle pkg_exception var1 exception';
    raise info '%,%', SQLCODE,SQLERRM;
end;
/

declare
  id integer;
begin
  id := test_exception(23);
exception
  when OTHERS then
     raise info '%,%', SQLCODE,SQLERRM;
end;
/

--recreate package
create or replace package pkg_exception is
var1 exception;
var2 integer;
end;
/

--again
--print handle pkg_exception var1 exception
declare
 var1 integer;
begin
  raise pkg_exception.var1;
 exception
   when pkg_exception.var1 then
     raise info 'handle pkg_exception var1 exception';
end;
/

declare
  var1 integer;
begin
  var1 := test_exception(23);
exception
   when others then
      raise info '%', SQLCODE;
end;
/


--recreate package body
create or replace package body pkg_exception is
begin
  var2 := 23;
  raise info 'init block';
end;
/


declare
 id integer;
begin
  id := test_exception(23);
exception
  when pkg_exception.var1 then
    raise info 'handle pkg_exception var1 exception';
    raise info '%,%', SQLCODE,SQLERRM;
end;
/

create or replace package pkg_exception is
var2 integer;
end;
/

declare
  var1 integer;
begin
  var1 := test_exception(23);
exception
   when others then
      raise info '%', SQLCODE;
end;
/

drop package pkg_exception;
drop function test_exception(integer);


--print 1 and User-Defined Exception
declare
  var2 exception;
  var1 exception;
begin
  raise var1;
exception
  when var1 then
     raise info '%',SQLCODE;
     raise info '%',SQLERRM;
end;
/

--test ok
declare
  var1 exception;
  pragma exception_init(var1, -1476);
begin
  raise var1;
exception
  when var1 then
    raise info '%',SQLCODE;
    raise info '%',SQLERRM;
end;
/

--test SQLERRM() function
--test ok
declare
  var1 exception;
  pragma exception_init(var1, -1476);
begin
  raise var1;
exception
  when var1 then
    raise info '%', sqlerrm(sqlcode);
end;
/

--ok no sqlcode no found
declare
  var1 exception;
  pragma exception_init(var1, -1477);
begin
  raise var1;
exception
   when var1 then
      raise info '%', sqlerrm(sqlcode);
end;
/

--test ok
declare
  var1 exception;
begin
  raise var1;
exception
   when var1 then
      raise info '%', sqlerrm(sqlcode);
end;
/

--test ok
declare
  var1 exception;
  pragma exception_init(var1, -1476);
begin
  raise var1;
exception
   when others then
      raise info '%', sqlerrm(sqlcode);
end;
/

--ok
declare
  var1 exception;
  pragma exception_init(var1, -20001);
begin
  raise_application_error(-20001,'a text error');
exception
   when var1 then
      raise info '%', sqlerrm(sqlcode);
end;
/

--ok
declare
  var1 exception;
  pragma exception_init(var1, -20001);
begin
  raise_application_error(-20001,'a text error');
exception
   when others then
      raise info '%', sqlerrm(sqlcode);
end;
/


---test sqlstate as a udt exception
--test ok and print handle sqlstate exception
declare
 sqlstate exception;
begin
   raise sqlstate;
exception
   when sqlstate then
     raise info 'handle sqlstate exception';
end;
/


declare
  sqlcode exception;
begin
  raise sqlcode;
exception
  when sqlcode then
    raise info 'handle sqlcode exception';
end;
/

declare
  SQLCODE exception;
begin
  raise SQLCODE;
exception
  when SQLCODE then
    raise info 'handle SQLCODE exception';
end;
/

declare
  sqlerrm exception;
begin
  raise sqlerrm;
exception
  when sqlerrm then
    raise info 'handle sqlerrm exception';
end;
/

---test global sqlcode
declare
  sqlcode exception;
  var2 integer;
 begin
   declare
      var1 integer;
    begin
       raise sqlcode;
    exception
       when sqlcode then
         raise info 'internal exception handle';
       end;
    var2 := 23;
    raise sqlcode;
 exception
       when sqlcode then
          raise info 'foreign exception handle';
end;
/

declare
  sqlcode exception;
  var2 integer;
 begin
    begin
       raise sqlcode;
    exception
       when sqlcode then
         raise info 'internal exception handle';
       end;
    var2 := 23;
    raise sqlcode;
 exception
       when sqlcode then
          raise info 'foreign exception handle';
end;
/


create table dwdai_t(id number);

DECLARE
	default_number NUMBER := 0;
	errorException exception;
	errorCode number;
	errorMsg varchar2(1000);
	out_return varchar2(1000);
BEGIN
	INSERT INTO dwdai_t VALUES(TO_NUMBER('100.00', '9G999'));
EXCEPTION
	WHEN INVALID_NUMBER THEN
		errorCode := SQLCODE;
		errorMsg := SUBSTR(SQLERRM, 1, 200);
		out_return := 'errorCode=' || errorCode || ',errorMsg=' || errorMsg;

		raise info '%',out_return;
		raise info '%','xiexie';
END;
/

---virtual col flect fetch?
CREATE TABLE vtest1 (a integer, b int GENERATED ALWAYS AS (a * 2));

CREATE TABLE tab1 (id integer,description integer INVISIBLE);

CREATE TABLE common_t(a integer);

--error
DECLARE
  var1 common_t%rowtype;
BEGIN
  SELECT * INTO var1 FROM vtest1;
end;
/

--error
DECLARE
  var1 common_t%rowtype;
BEGIN
  SELECT * INTO var1 FROM tab1;
end;
/


INSERT INTO tab1 VALUES(1);
INSERT INTO vtest1(a) VALUES(2);

--error
DECLARE
  var1 common_t%rowtype;
BEGIN
  SELECT * INTO var1 FROM vtest1;
end;
/

--ok
DECLARE
  var1 common_t%rowtype;
BEGIN
  SELECT * INTO var1 FROM tab1;
end;
/

--test #OPTION DUMP
create or replace function test_f(id integer) return integer is
#OPTION DUMP
  var1 exception;
  PRAGMA exception_init(var1,-1746);
begin
  raise var1;
exception
 when var1 then
    raise info 'handle exception var1';
end;
/

--test one oraerrcode map to two pgerrcode
INSERT INTO _ivorysql_ora_exception_map VALUES('22012',-1755,'IVY-1755 test a divisor is equal to zero exception');
INSERT INTO _ivorysql_ora_exception_map VALUES('42P03',-1755,'IVY-1755 test a cursor already open exception');

--ok
DECLARE
  var1 exception;
  PRAGMA exception_init(var1,-1755);
  var2 integer;
  CURSOR c1 IS SELECT * FROM dual;
BEGIN
  var1 = 1 / 0;
exception
  WHEN var1 THEN
     raise info 'handle var1:%-%', sqlcode, sqlerrm;
	 OPEN c1;
	 BEGIN
        OPEN c1;
	 exception
	  WHEN var1 THEN
         raise info 'handle var1:%-%',sqlcode,sqlerrm;
	 end;
end;
/

DELETE FROM _ivorysql_ora_exception_map WHERE oraerrorcode = -1755;

--clean data
DROP TABLE dwdai_t;
DROP PROCEDURE account_status;
DROP TABLE vtest1;
DROP TABLE tab1;
DROP TABLE common_t;
DROP FUNCTION test_f;
